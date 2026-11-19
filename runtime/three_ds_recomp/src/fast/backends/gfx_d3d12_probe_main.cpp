#include "fast/backends/gfx_d3d12_probe.h"

#include <iostream>

int main() {
    const auto result = Fast::D3D12Probe::TryCreateHardwareDevice();
    std::cout << result.Message << '\n';
    if (!result.Available) {
        std::cerr << "HRESULT: 0x" << std::hex
                  << static_cast<unsigned long>(result.HResult) << '\n';
        return 2;
    }
    return 0;
}
