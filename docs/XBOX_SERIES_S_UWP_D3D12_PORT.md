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
