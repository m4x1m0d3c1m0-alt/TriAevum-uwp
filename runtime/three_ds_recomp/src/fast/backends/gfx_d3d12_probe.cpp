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

} // namespace Fast::D3D12Probe
