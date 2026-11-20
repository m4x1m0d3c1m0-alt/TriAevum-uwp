# Xbox UWP D3D12 CoreWindow smoke-test host

This is the first native UWP host source for the Xbox Series S Dev Mode
investigation. It is isolated from the desktop TriAevum runtime and does not
change the Vulkan renderer.

## What it implements

- Native C++/WinRT IFrameworkView / IFrameworkViewSource.
- UWP CoreWindow acquisition and event loop.
- D3D12 hardware device at feature level 11_0.
- Direct command queue, command allocators/list, RTV heap, and fence.
- Flip-sequential CoreWindow swap chain.
- Two-buffer clear-to-blue and Present loop with per-buffer fence tracking.

Important DXGI detail: for a D3D12 swap chain,
CreateSwapChainForCoreWindow receives the **direct command queue** as its
first argument, not ID3D12Device. This follows Microsoft's API contract:
https://learn.microsoft.com/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforcorewindow

## Status / limitations

This is source code for the first UWP host smoke test, **not yet a complete
Visual Studio UWP project or an installable MSIX/AppX package**. It has not yet
been compiled with the developer's Visual Studio installation or run on the
Xbox. The project file, package manifest, assets, suspend/resume handling,
display-size recreation, controller input, and device-loss recovery remain.

The source does not link to or initialize TriAevum's existing Vulkan renderer.
The next gate is to place this source in a correctly configured UWP C++
application project, compile it against the selected Windows SDK, then deploy
it to the Series S and verify the blue clear/present loop.

## Build assumptions to validate

- Visual Studio 2026 with the UWP C++ workload.
- Windows SDK 10.0.26100.0 (the developer PC also has 10.0.22621.0 and
  10.0.28000.0).
- x64 target and C++/WinRT headers.
- D3D12, DXGI, and CoreWindow API availability in the selected Xbox Dev Mode
  app model.

A successful desktop compile is not proof of Xbox runtime support.
