# Xbox UWP D3D12 CoreWindow smoke-test host

This is the first native UWP host for the Xbox Series S Dev Mode investigation.
It is isolated from the desktop TriAevum runtime and does not change the Vulkan
renderer.

## Files

- `TriAevumXboxUwpHost.cpp`: native C++/WinRT view, D3D12 device, direct queue,
  two-buffer CoreWindow swap chain, RTVs, fence, and clear-to-blue Present loop.
- `TriAevumXboxUwpHost.vcxproj`: x64 UWP MSBuild project using Visual Studio
  2026's v145 toolset and Windows SDK 10.0.26100.0.
- `Package.appxmanifest`: package manifest targeting the Windows.Xbox device
  family.
- `GenerateAssets.ps1`: creates placeholder PNG tile/splash assets before build.

## Important DXGI detail

For a D3D12 swap chain, CreateSwapChainForCoreWindow receives the **direct
command queue** as its first argument, not ID3D12Device. This follows
Microsoft's API contract:
https://learn.microsoft.com/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforcorewindow

## Current status

The UWP source, project file, Xbox-targeted manifest, and placeholder-asset
generator are now committed. **The project has not yet been compiled**, and no
MSIX/AppX has been produced or installed. Package signing is currently disabled
in the project; a trusted development certificate must be configured before
installation on the console.

The source does not link to or initialize TriAevum's existing Vulkan renderer.
Suspend/resume, display-size recreation, controller input, and device-loss
recovery remain outside this first smoke test.

## Build gate

1. Open `TriAevumXboxUwpHost.vcxproj` in Visual Studio 2026.
2. Select `Debug` and `x64`, then build.
3. Resolve any SDK/project-system or C++/WinRT errors from the actual build log.
4. Configure a test signing certificate, build the package, and only then deploy
   it through Xbox Device Portal.
5. Verify the blue clear/present loop on the Series S.

The project assumes the installed UWP C++ workload, MSVC v145, and Windows SDK
10.0.26100.0. A successful desktop compile is not proof of Xbox runtime support.
