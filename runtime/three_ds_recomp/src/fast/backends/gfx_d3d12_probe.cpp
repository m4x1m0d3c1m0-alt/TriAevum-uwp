#include "fast/backends/gfx_d3d12_probe.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <sstream>

namespace Fast::D3D12Probe {

Result TryCreateHardwareDevice() {
    Result result{};
    Microsoft::WRL::ComPtr<ID3D12Device> device;

    const HRESULT hr = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()));

    result.HResult = static_cast<long>(hr);
    result.Available = SUCCEEDED(hr);

    std::ostringstream message;
    if (result.Available) {
        message << "D3D12 hardware device created; feature level 11_0 minimum.";
    } else {
        message << "D3D12CreateDevice failed (HRESULT 0x"
                << std::hex << static_cast<unsigned long>(hr) << ").";
    }
    result.Message = message.str();
    return result;
}

Result TryCreateCoreObjects() {
    Result result{};
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;

    HRESULT hr = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()));
    if (FAILED(hr)) {
        result.HResult = static_cast<long>(hr);
        result.Message = "D3D12 device creation failed before core-object checks.";
        return result;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(queue.GetAddressOf()));
    if (FAILED(hr)) {
        result.HResult = static_cast<long>(hr);
        result.Message = "CreateCommandQueue failed.";
        return result;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = 2;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf()));
    if (FAILED(hr)) {
        result.HResult = static_cast<long>(hr);
        result.Message = "CreateDescriptorHeap(RTV) failed.";
        return result;
    }

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
    if (FAILED(hr)) {
        result.HResult = static_cast<long>(hr);
        result.Message = "CreateFence failed.";
        return result;
    }

    const UINT rtvDescriptorSize =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (rtvDescriptorSize == 0) {
        result.HResult = static_cast<long>(E_FAIL);
        result.Message = "D3D12 returned an invalid RTV descriptor increment.";
        return result;
    }

    result.Available = true;
    result.HResult = static_cast<long>(S_OK);
    result.Message =
        "D3D12 device, direct command queue, 2-entry RTV heap, and fence created.";
    return result;
}

} // namespace Fast::D3D12Probe
