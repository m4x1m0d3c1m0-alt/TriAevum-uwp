#include "fast/backends/gfx_d3d12_probe.h"

#include <iostream>

int main() {
    const auto device = Fast::D3D12Probe::TryCreateHardwareDevice();
    std::cout << "[device] " << device.Message << '\n';
    if (!device.Available) {
        std::cerr << "HRESULT: 0x" << std::hex
                  << static_cast<unsigned long>(device.HResult) << '\n';
        return 2;
    }

    const auto core = Fast::D3D12Probe::TryCreateCoreObjects();
    std::cout << "[core] " << core.Message << '\n';
    if (!core.Available) {
        std::cerr << "HRESULT: 0x" << std::hex
                  << static_cast<unsigned long>(core.HResult) << '\n';
        return 3;
    }

    return 0;
}
