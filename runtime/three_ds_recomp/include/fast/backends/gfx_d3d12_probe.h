#pragma once

#include <string>

namespace Fast::D3D12Probe {

// Attempts to create a hardware D3D12 device at feature level 11_0.
// This isolated probe does not create a window or swap chain and does not
// establish Xbox UWP package, input, audio, or presentation compatibility.
struct Result {
    bool Available = false;
    long HResult = 0;
    std::string Message;
};

[[nodiscard]] Result TryCreateHardwareDevice();

// Exercises the first device-owned objects required by a renderer:
// a direct command queue, a non-shader-visible RTV descriptor heap, and a fence.
// This is still a host-side smoke test, not a complete rendering test.
[[nodiscard]] Result TryCreateCoreObjects();

} // namespace Fast::D3D12Probe
