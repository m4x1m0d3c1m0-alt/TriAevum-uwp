// Minimal native UWP + Direct3D 12 CoreWindow smoke test.
// This is deliberately separate from the desktop TriAevum renderer.
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>

#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Core.h>

namespace core = winrt::Windows::ApplicationModel::Core;
namespace display = winrt::Windows::Graphics::Display;
namespace ui = winrt::Windows::UI::Core;

using Microsoft::WRL::ComPtr;

namespace {

constexpr UINT kFrameCount = 2;

struct D3D12CoreWindowView
    : winrt::implements<D3D12CoreWindowView, core::IFrameworkView> {
    void Initialize(core::CoreApplicationView const&) {}

    void SetWindow(ui::CoreWindow const& window) {
        m_window = window;
        m_window.Closed({ this, &D3D12CoreWindowView::OnClosed });
    }

    void Load(winrt::hstring const&) {}

    void Run() {
        m_window.Activate();
        CreateDeviceResources();
        CreateWindowResources();

        while (!m_closed) {
            m_window.Dispatcher().ProcessEvents(
                ui::CoreProcessEventsOption::ProcessAllIfPresent);
            RenderFrame();
        }

        WaitForGpu();
    }

    void Uninitialize() {
        WaitForGpu();
        if (m_fenceEvent) {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }
    }

private:
    void OnClosed(ui::CoreWindow const&, ui::CoreWindowEventArgs const&) {
        m_closed = true;
    }

    void CreateDeviceResources() {
        winrt::check_hresult(D3D12CreateDevice(
            nullptr,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(m_device.GetAddressOf())));

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        winrt::check_hresult(m_device->CreateCommandQueue(
            &queueDesc, IID_PPV_ARGS(m_queue.GetAddressOf())));

        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = kFrameCount;
        winrt::check_hresult(m_device->CreateDescriptorHeap(
            &rtvDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));
        m_rtvDescriptorSize =
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        winrt::check_hresult(m_device->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));
        m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (!m_fenceEvent) {
            winrt::throw_hresult(HRESULT_FROM_WIN32(GetLastError()));
        }

        for (UINT i = 0; i < kFrameCount; ++i) {
            winrt::check_hresult(m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(m_allocators[i].GetAddressOf())));
        }

        winrt::check_hresult(m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_allocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(m_commandList.GetAddressOf())));
        winrt::check_hresult(m_commandList->Close());
    }

    void CreateWindowResources() {
        const auto bounds = m_window.Bounds();
        const float dpi = display::DisplayInformation::GetForCurrentView().LogicalDpi();
        const UINT width = (std::max)(1u, static_cast<UINT>(bounds.Width * dpi / 96.0f));
        const UINT height = (std::max)(1u, static_cast<UINT>(bounds.Height * dpi / 96.0f));

        ComPtr<IDXGIFactory2> factory;
        winrt::check_hresult(CreateDXGIFactory2(
            0, IID_PPV_ARGS(factory.GetAddressOf())));

        DXGI_SWAP_CHAIN_DESC1 swapDesc{};
        swapDesc.Width = width;
        swapDesc.Height = height;
        swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapDesc.SampleDesc.Count = 1;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferCount = kFrameCount;
        swapDesc.Scaling = DXGI_SCALING_STRETCH;
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        ComPtr<IDXGISwapChain1> swapChain1;
        // For D3D12 this parameter must be the DIRECT command queue, not the device.
        winrt::check_hresult(factory->CreateSwapChainForCoreWindow(
            m_queue.Get(),
            winrt::get_unknown(m_window),
            &swapDesc,
            nullptr,
            swapChain1.GetAddressOf()));
        winrt::check_hresult(swapChain1.As(m_swapChain.GetAddressOf()));

        auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < kFrameCount; ++i) {
            winrt::check_hresult(m_swapChain->GetBuffer(
                i, IID_PPV_ARGS(m_backBuffers[i].GetAddressOf())));
            m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtv);
            rtv.ptr += m_rtvDescriptorSize;
        }
    }

    void RenderFrame() {
        const UINT index = m_swapChain->GetCurrentBackBufferIndex();
        WaitForFrame(index);

        winrt::check_hresult(m_allocators[index]->Reset());
        winrt::check_hresult(m_commandList->Reset(m_allocators[index].Get(), nullptr));

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_backBuffers[index].Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_commandList->ResourceBarrier(1, &barrier);

        auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += static_cast<SIZE_T>(index) * m_rtvDescriptorSize;
        m_commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        constexpr float clearColor[] = { 0.04f, 0.16f, 0.42f, 1.0f };
        m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_commandList->ResourceBarrier(1, &barrier);
        winrt::check_hresult(m_commandList->Close());

        ID3D12CommandList* lists[] = { m_commandList.Get() };
        m_queue->ExecuteCommandLists(1, lists);
        winrt::check_hresult(m_swapChain->Present(1, 0));

        const UINT64 fenceValue = ++m_nextFenceValue;
        winrt::check_hresult(m_queue->Signal(m_fence.Get(), fenceValue));
        m_frameFenceValues[index] = fenceValue;
    }

    void WaitForFrame(UINT index) {
        const UINT64 value = m_frameFenceValues[index];
        if (value != 0 && m_fence->GetCompletedValue() < value) {
            winrt::check_hresult(m_fence->SetEventOnCompletion(value, m_fenceEvent));
            WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
        }
    }

    void WaitForGpu() {
        if (!m_queue || !m_fence || !m_fenceEvent) {
            return;
        }

        const UINT64 value = ++m_nextFenceValue;
        if (SUCCEEDED(m_queue->Signal(m_fence.Get(), value)) &&
            m_fence->GetCompletedValue() < value &&
            SUCCEEDED(m_fence->SetEventOnCompletion(value, m_fenceEvent))) {
            WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
        }
    }

    ui::CoreWindow m_window{ nullptr };
    bool m_closed = false;
    UINT m_rtvDescriptorSize = 0;
    UINT64 m_nextFenceValue = 0;
    std::array<UINT64, kFrameCount> m_frameFenceValues{};
    HANDLE m_fenceEvent = nullptr;

    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12Fence> m_fence;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<IDXGISwapChain3> m_swapChain;
    std::array<ComPtr<ID3D12CommandAllocator>, kFrameCount> m_allocators;
    std::array<ComPtr<ID3D12Resource>, kFrameCount> m_backBuffers;
};

struct D3D12CoreWindowViewSource
    : winrt::implements<D3D12CoreWindowViewSource, core::IFrameworkViewSource> {
    core::IFrameworkView CreateView() {
        return winrt::make<D3D12CoreWindowView>();
    }
};

} // namespace

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    core::CoreApplication::Run(winrt::make<D3D12CoreWindowViewSource>());
    return 0;
}
