# Xbox Series S UWP / Direct3D 12 port

## Status

This is an engineering assessment and implementation boundary, not a working Xbox
build. The current renderer is a Vulkan implementation. Do not describe this
branch as an operational D3D12 or UWP port until the renderer, platform layer,
and package have all been built and tested on the target.

## Findings from v0.8.0-beta.2

- The top-level build enables `OOT3D_ENABLE_VULKAN_RENDERER` by default and
  forces `THREE_DS_RECOMP_ENABLE_VULKAN` to the same value.
- The runtime's native 3DS renderer is guarded by
  `THREE_DS_RECOMP_ENABLE_VULKAN` and depends on Vulkan, shaderc, SPIR-V,
  Vulkan ImGui integration, and NVIDIA NRI interop.
- The main backend is not just an NRI client. `gfx_vulkan.cpp` and the
  related `gfx_vulkan_*.cpp` files issue Vulkan API calls directly for
  command recording, resources, descriptors, pipelines, synchronization,
  render passes, and presentation.
- The shared `GfxRenderingAPI` / PICA contracts provide a useful boundary
  for a second backend, but the current concrete backend is deeply Vulkan
  specific.
- The existing `THREE_DS_RECOMP_ENABLE_D3D12_NGX_PROVIDER` option is a
  developer-only interop path for NGX. It is not a D3D12 game renderer and
  must not be mistaken for one.
- The runtime currently uses SDL2 and desktop Win32/ImGui backend sources.
  UWP requires a separate window, input, lifecycle, filesystem, and packaging
  path; desktop Win32 entry points cannot simply be reused.

## Required implementation

1. **Confirm the target API contract.** Establish which Direct3D 12 device,
   queue, swap-chain, shader model, and resource-sharing interfaces are
   exposed to a sideloaded Xbox Series S UWP app in the intended Dev Mode
   environment. Do not assume that desktop D3D12, Xbox GDK D3D12, and UWP
   expose identical APIs.
2. **Introduce a renderer-neutral GPU layer.** Move resource, descriptor,
   pipeline, command-list, barrier, fence, and swap-chain ownership behind
   interfaces. Keep the current Vulkan backend working while doing this.
3. **Implement a real D3D12 backend.** Translate the existing renderer's
   resource formats, descriptor layouts, pipeline state, dynamic rendering,
   barriers, readback, timestamps, and multi-frame synchronization. NRI can
   be evaluated as an abstraction, but the existing Vulkan implementation
   cannot be switched to D3D12 by toggling its NRI option.
4. **Port shader production.** Convert GLSL/SPIR-V shader generation and
   shader-cache assumptions to DXIL/DXBC as appropriate for the confirmed
   target. Validate every generated shader permutation used by the PICA
   renderer and post-processing graph.
5. **Port presentation and input.** Replace desktop SDL/Win32 assumptions
   with a UWP-capable host, including Xbox controller input, focus/suspend/
   resume, resize/display changes, and safe teardown/recreation.
6. **Port storage and content preparation.** Audit all paths, process launch,
   dynamic-library loading, network downloads, writable directories, save
   data, caches, and user-selected ROM access against UWP capabilities.
7. **Package and qualify.** Produce an x64 MSIX/AppX package with a correct
   manifest, capabilities, assets, and dependencies. Install it in Xbox Dev
   Mode and validate boot, controller navigation, gameplay, suspend/resume,
   saves, graphics settings, and long-running stability.

## Non-goals and safeguards

- Do not remove or weaken existing Vulkan behavior to make a D3D12 build pass.
- Do not silently substitute a software renderer or claim GPU acceleration
  without verifying the active adapter and renderer.
- Do not bundle game ROMs, copyrighted game assets, or separately licensed
  modules without authorization.
- Do not publish an installable package until it has been built and tested
  against the actual Xbox Dev Mode target.
- Keep all work on the `xbox-series-s-uwp` branch; do not push to `main`.

## First technical milestone

The first code milestone is a minimal D3D12 device/swap-chain/present probe
inside the confirmed UWP host, followed by a backend contract implementation
for clear + triangle + texture upload. Only after that works on the console
should the full PICA renderer and effect graph be migrated.


## First code milestone (2026-11-19)

An opt-in, isolated D3D12 device probe has been added:

- `THREE_DS_RECOMP_ENABLE_D3D12_PROBE=ON` builds the
  `triaevum_d3d12_probe` static library on a Windows SDK host.
- `Fast::D3D12Probe::TryCreateHardwareDevice()` calls
  `D3D12CreateDevice` at feature level 11_0 and returns the HRESULT and a
  diagnostic message.
- The probe is deliberately separate from the Vulkan renderer and is OFF by
  default. It does not change the current rendering backend.

**Validation boundary:** this is only a Windows desktop SDK compile/runtime
probe. It does not create a swap chain, does not prove that the Xbox Dev Mode
UWP app model exposes D3D12, and is not evidence of a working Xbox port.
Before using this code in the Xbox host, confirm the supported Xbox UWP SDK
contract and compile/run the probe on the actual Series S target. If the
target's UWP API surface does not expose the required D3D12 interfaces, the
renderer plan must be revised rather than bypassing platform restrictions.


## D3D12 host smoke-test milestone (2026-11-19)

The isolated probe now has a standalone executable and exercises more than
device creation. TryCreateCoreObjects() creates a hardware D3D12 device,
a direct command queue, a two-entry non-shader-visible RTV descriptor heap,
and a fence, then checks that the RTV descriptor increment is non-zero.
The executable reports the device and core-object results separately.

A GitHub Actions job compiles this probe with MSVC and the Windows SDK. The
initial device-only version passed. The expanded queue/heap/fence version is
being checked by the subsequent workflow runs.

### Xbox API research note

Microsoft's Xbox Series X|S UWP guidance distinguishes App Mode from Game
Mode. It reports Direct3D 12 Feature Level 11.0 for Series X|S in both modes,
with Shader Models 5.1 through 6.4; the documented WARP exception in
Developer Mode is not a retail GPU-acceleration path. See:
https://walbourn.github.io/directx-and-uwp-on-xbox-series-x-s/

This makes D3D12 a plausible target for the Series S Dev Mode app, but it
does not validate this project's UWP package, swap-chain path, or actual
console runtime. The next console-specific gate remains a UWP-hosted
swap-chain + clear/present test on the Series S itself.

### Current validation matrix

| Check | Result |
|---|---|
| Standalone D3D12 device probe compiled with Windows SDK/MSVC | Passed |
| Direct queue + RTV heap + fence creation on Windows host | Workflow validation pending |
| Xbox Series S UWP package compilation | Not done |
| Xbox UWP swap-chain and present on hardware | Not done |
| TriAevum PICA D3D12 renderer | Not implemented |
| Installable Xbox package / gameplay test | Not available |


## Native UWP host source milestone (2026-11-20)

The branch now contains an isolated native C++/WinRT host under
`platform/xbox_uwp_d3d12/`:

- `TriAevumXboxUwpHost.cpp` implements an `IFrameworkView`, creates a D3D12
  device at feature level 11_0, a direct command queue, command allocators/list,
  RTV heap, fence, and a two-buffer CoreWindow swap chain. It clears the current
  back buffer to blue and presents it.
- `TriAevumXboxUwpHost.vcxproj` targets x64, UWP, MSVC v145, and Windows SDK
  10.0.26100.0.
- `Package.appxmanifest` targets `Windows.Xbox`.
- `GenerateAssets.ps1` generates placeholder package image assets before build.

The swap-chain call passes the direct command queue, as required by the D3D12
DXGI API contract. Microsoft documents that the first parameter is an
`ID3D12CommandQueue` for D3D12, not an `ID3D12Device`:
https://learn.microsoft.com/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforcorewindow

**Validation boundary:** these project and source files have been committed,
but no compile has yet been run. Package signing is disabled pending setup of a
development certificate. No installable package has been produced, and the
host has not been deployed to or executed on the Xbox. Suspend/resume,
window-size changes, controller input, and device-loss recovery remain future
work. The Vulkan renderer is unchanged.
