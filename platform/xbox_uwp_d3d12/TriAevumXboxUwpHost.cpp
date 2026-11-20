// Minimal native UWP + Direct3D 12 CoreWindow smoke test.
// This is deliberately separate from the desktop TriAevum renderer.
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Storage.h>

namespace core = winrt::Windows::ApplicationModel::Core;
namespace display = winrt::Windows::Graphics::Display;
namespace ui = winrt::Windows::UI::Core;

using Microsoft::WRL::ComPtr;

namespace {

constexpr UINT kFrameCount = 2;

HANDLE g_diagnosticFile = INVALID_HANDLE_VALUE;
const char* g_diagnosticStage = "Startup";

void LogDiagnostic(const char* message) noexcept;

void OpenDiagnosticLog() noexcept {
    try {
        const auto folder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
        std::wstring path = folder.Path().c_str();
        path += L"\\TriAevumXboxUwpHost.log";
        g_diagnosticFile = CreateFileW(
            path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (g_diagnosticFile == INVALID_HANDLE_VALUE) {
            char message[128]{};
            std::snprintf(message, sizeof(message),
                "Could not open LocalState diagnostic log; Win32 error=%lu",
                static_cast<unsigned long>(GetLastError()));
            OutputDebugStringA(message);
            OutputDebugStringA("\r\n");
            return;
        }
        LogDiagnostic("===== TriAevum Xbox UWP host diagnostic session =====");
        LogDiagnostic("Log location: ApplicationData\\LocalFolder\\TriAevumXboxUwpHost.log");
    } catch (...) {
        OutputDebugStringA("Could not initialize the app LocalState diagnostic log\r\n");
    }
}

void LogDiagnostic(const char* message) noexcept {
    if (!message) return;
    OutputDebugStringA(message);
    OutputDebugStringA("\r\n");
    if (g_diagnosticFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        const DWORD length = static_cast<DWORD>(std::strlen(message));
        WriteFile(g_diagnosticFile, message, length, &written, nullptr);
        static constexpr char newline[] = "\r\n";
        WriteFile(g_diagnosticFile, newline, 2, &written, nullptr);
        FlushFileBuffers(g_diagnosticFile);
    }
}

void LogHresultDiagnostic(const char* label, HRESULT hr) noexcept {
    char message[256]{};
    std::snprintf(message, sizeof(message),
        "%s | stage=%s | HRESULT=0x%08lX",
        label ? label : "HRESULT failure",
        g_diagnosticStage ? g_diagnosticStage : "unknown",
        static_cast<unsigned long>(hr));
    LogDiagnostic(message);
}

void CloseDiagnosticLog() noexcept {
    if (g_diagnosticFile != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(g_diagnosticFile);
        CloseHandle(g_diagnosticFile);
        g_diagnosticFile = INVALID_HANDLE_VALUE;
    }
}

struct D3D12CoreWindowView
    : winrt::implements<D3D12CoreWindowView, core::IFrameworkView> {
    void Initialize(core::CoreApplicationView const&) {
        OpenDiagnosticLog();
        LogDiagnostic("IFrameworkView.Initialize");
    }

    void SetWindow(ui::CoreWindow const& window) {
        LogDiagnostic("IFrameworkView.SetWindow");
        m_window = window;
        m_window.Closed([this](ui::CoreWindow const&, ui::CoreWindowEventArgs const&) {
            m_closed = true;
        });
    }

    void Load(winrt::hstring const&) {}

    void Run() {
        LogDiagnostic("Run entered");
        try {
            g_diagnosticStage = "CoreWindow.Activate";
            LogDiagnostic("Activating CoreWindow");
            m_window.Activate();
            LogDiagnostic("CoreWindow activated");

            g_diagnosticStage = "CreateDeviceResources";
            LogDiagnostic("Creating D3D12 device resources");
            CreateDeviceResources();
            LogDiagnostic("D3D12 device resources created");

            g_diagnosticStage = "CreateWindowResources";
            LogDiagnostic("Creating CoreWindow swap chain and render targets");
            CreateWindowResources();
            LogDiagnostic("Swap chain and render targets created");

            bool firstFrame = true;
            while (!m_closed) {
                m_window.Dispatcher().ProcessEvents(
                    ui::CoreProcessEventsOption::ProcessAllIfPresent);
                g_diagnosticStage = firstFrame ? "First RenderFrame / Present" : "RenderFrame / Present";
                RenderFrame();
                if (firstFrame) {
                    LogDiagnostic("First Present succeeded");
                    firstFrame = false;
                }
            }

            g_diagnosticStage = "WaitForGpu";
            WaitForGpu();
            LogDiagnostic("Run completed normally");
        } catch (winrt::hresult_error const& error) {
            LogHresultDiagnostic("WinRT/D3D failure", error.code());
            LogDiagnostic("The host stopped after the failure recorded above");
        } catch (std::exception const& error) {
            LogDiagnostic("C++ std::exception caught");
            LogDiagnostic(error.what());
            LogDiagnostic("The host stopped after the exception recorded above");
        } catch (...) {
            LogDiagnostic("Unknown exception caught in IFrameworkView::Run");
            LogDiagnostic("The host stopped after the exception recorded above");
        }
        CloseDiagnosticLog();
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
        g_diagnosticStage = "D3D12CreateDevice";
        const HRESULT deviceHr = D3D12CreateDevice(
            nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()));
        if (FAILED(deviceHr)) LogHresultDiagnostic("D3D12CreateDevice failed", deviceHr);
        winrt::check_hresult(deviceHr);

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        g_diagnosticStage = "CreateCommandQueue";
        const HRESULT queueHr = m_device->CreateCommandQueue(
            &queueDesc, IID_PPV_ARGS(m_queue.GetAddressOf()));
        if (FAILED(queueHr)) LogHresultDiagnostic("CreateCommandQueue failed", queueHr);
        winrt::check_hresult(queueHr);

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
        g_diagnosticStage = "CreateDXGIFactory2";
        const HRESULT factoryHr = CreateDXGIFactory2(
            0, IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(factoryHr)) LogHresultDiagnostic("CreateDXGIFactory2 failed", factoryHr);
        winrt::check_hresult(factoryHr);

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
        g_diagnosticStage = "CreateSwapChainForCoreWindow";
        const HRESULT swapChainHr = factory->CreateSwapChainForCoreWindow(
            m_queue.Get(), winrt::get_unknown(m_window), &swapDesc,
            nullptr, swapChain1.GetAddressOf());
        if (FAILED(swapChainHr)) LogHresultDiagnostic("CreateSwapChainForCoreWindow failed", swapChainHr);
        winrt::check_hresult(swapChainHr);
        winrt::check_hresult(swapChain1.As(&m_swapChain));

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
        g_diagnosticStage = "IDXGISwapChain::Present";
        const HRESULT presentHr = m_swapChain->Present(1, 0);
        if (FAILED(presentHr)) LogHresultDiagnostic("Present failed", presentHr);
        winrt::check_hresult(presentHr);

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
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        LogDiagnostic("wWinMain: apartment initialized");
        core::CoreApplication::Run(winrt::make<D3D12CoreWindowViewSource>());
        LogDiagnostic("wWinMain: CoreApplication::Run returned");
    } catch (winrt::hresult_error const& error) {
        LogHresultDiagnostic("Unhandled WinRT exception", error.code());
    } catch (...) {
        LogDiagnostic("Unhandled exception escaped CoreApplication::Run");
    }
    CloseDiagnosticLog();
    return 0;
}
