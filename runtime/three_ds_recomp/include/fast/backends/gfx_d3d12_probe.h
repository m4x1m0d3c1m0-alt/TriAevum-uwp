#pragma once

#include <string>

namespace Fast::D3D12Probe {

// Attempts to create a hardware D3D12 device at feature level 11_0.
// This isolated probe does not create a window or swap chain and does not
// establish Xbox UWP API availability.
struct Result {
    bool Available = false;
    long HResult = 0;
    std::string Message;
};

[[nodiscard]] Result TryCreateHardwareDevice();

} // namespace Fast::D3D12Probe
