# TriAevum Xbox UWP: integración del renderer Vulkan

Estado: análisis de código completado; integración del runtime todavía no implementada.
Rama: `xbox-series-s-uwp`.

## Evidencia confirmada en Series S

El probe independiente `platform/xbox_uwp_vulkan_probe` ha pasado en la Xbox Series S:
- `LoadPackagedLibrary("vulkan_dzn.dll")` funciona en el paquete UWP.
- Dozen expone Vulkan 1.2.354 y detecta `Microsoft Direct3D12 (SraKmd_arden)`.
- La superficie CoreWindow y la cola gráfica son compatibles.
- `vkCreateDevice` y `vkCreateSwapchainKHR` devuelven `VK_SUCCESS`.
- El probe posterior graba un comando `vkCmdClearColorImage`, hace submit y presenta con `vkQueuePresentKHR`; el usuario confirma que la Xbox muestra el fotograma azul.

Esto valida Dozen + DXIL + AppContainer + CoreWindow + presentación Vulkan en la consola objetivo. No valida todavía el renderer de TriAevum, NRI, shaderc ni el rendimiento del juego.

## Hallazgos del código real

### 1. El backend Vulkan no es independiente de SDL

Archivos principales:
- `runtime/three_ds_recomp/src/fast/backends/gfx_vulkan_lifecycle.cpp`
- `runtime/three_ds_recomp/src/fast/backends/gfx_vulkan_device.cpp`
- `runtime/three_ds_recomp/src/fast/backends/gfx_vulkan_internal.h`
- `runtime/three_ds_recomp/include/fast/backends/gfx_vulkan.h`

El constructor recibe `GfxWindowBackendSDL2*`. `Init()` exige `UsesVulkan()` y un `SDL_Window*`. El backend también usa SDL para preferencias/cache y obtiene el tamaño de ventana a través del backend SDL.

En Windows, `CreateSurface()` llama a `SDL_GetWindowWMInfo` y pasa `windowInfo.info.win.window` como HWND a `vkCreateWin32SurfaceKHR`. El probe Xbox, en cambio, usa un `CoreWindow` de WinRT reinterpretado como HWND, tal como espera el parche UWP de Mesa. Un HWND Win32 normal y el IUnknown de CoreWindow no son intercambiables. Por tanto, no basta con empaquetar el runtime actual junto a `vulkan_dzn.dll`: hace falta un camino UWP explícito que entregue el CoreWindow real y evite consultar SDL SysWM.

### 2. El runtime enlaza al loader Vulkan de escritorio

En `runtime/three_ds_recomp/CMakeLists.txt`, el bloque `THREE_DS_RECOMP_ENABLE_VULKAN` ejecuta `find_package(Vulkan REQUIRED)`, enlaza `ImGui` a `Vulkan::Vulkan` y el runtime usa llamadas globales `vk*`.

El probe no enlaza ni carga `vulkan-1.dll`: carga el ICD Dozen empaquetado y obtiene `vk_icdGetInstanceProcAddr`. La integración debe introducir una capa de dispatch compatible con UWP (por ejemplo Volk inicializado con un `PFN_vkGetInstanceProcAddr` personalizado, tras verificar el ABI de Dozen) y aplicar esa dispatch de forma coherente a runtime, ImGui y NRI. No se debe enlazar inadvertidamente el loader de escritorio en el APPX.

### 3. NRI se envuelve alrededor del dispositivo Vulkan existente

`runtime/three_ds_recomp/src/fast/extension/nri_interop_context.cpp` usa `nriCreateDeviceFromVKDevice`, recibe la instancia/dispositivo creados por TriAevum y luego solicita `CoreInterface` y `WrapperVKInterface`. Es un punto favorable: no necesitamos que NRI cree una segunda instancia Vulkan. Sin embargo, NRI también debe compilarse para UWP y usar la misma dispatch/ABI. La primera compilación UWP debe desactivar temporalmente DLSS/NGX, providers de escritorio y módulos opcionales, y solo reactivar NRI después de validar el camino base.

### 4. El compilador de shaders es otro límite independiente

El CMake Vulkan actual localiza `shaderc_shared.dll` dentro del Vulkan SDK de escritorio en Windows. No se ha demostrado que esa DLL sea cargable en Xbox UWP. El primer build debe usar shaders SPIR-V ya generados o compilar shaderc/glslang para UWP; no debe asumir que la DLL del SDK de escritorio funcionará en AppContainer.

## Orden de implementación

1. **Capa de carga Vulkan UWP:** cargar Dozen empaquetado y resolver dispatch global/instancia/dispositivo; eliminar dependencia del loader de escritorio solo en el target UWP. Mantener sin cambios los targets desktop.
2. **Contrato de ventana UWP:** añadir una entrada explícita CoreWindow al backend de presentación. No reinterpretar un HWND SDL como CoreWindow. Conservar SDL para desktop; decidir si el input/event loop UWP se resuelve con SDL compilado para UWP o con un backend WinRT pequeño.
3. **Target UWP del runtime:** separar las dependencias desktop no válidas (vcpkg desktop, shaderc DLL, archivos/configuración y APIs de escritorio) de las dependencias realmente necesarias. El target debe compilar el backend Vulkan y sus módulos básicos, sin los proveedores opcionales.
4. **Prueba Vulkan del runtime:** crear instancia/dispositivo/swapchain a través del código del backend TriAevum y presentar un clear frame, antes de intentar cargar contenido del juego.
5. **NRI y recursos:** activar NRI de forma opcional, auditar features/formatos/límites de Dozen y resolver los requisitos bloqueantes.
6. **Primer contenido:** shaders, buffers, texturas, draw y escena mínima; luego audio, mandos, guardado y ciclo de vida UWP.

## Reglas de seguridad técnica

- No cambiar `main`; trabajar solo en `xbox-series-s-uwp`.
- No retirar ni alterar la ruta de escritorio.
- No activar NRI, NGX/DLSS ni efectos opcionales por defecto hasta que cada uno pase sus comprobaciones.
- No afirmar que TriAevum está portado hasta que su propio backend presente una escena real en la Series S.
- Mantener licencias y atribuciones de Mesa, Volk, SDL y NRI si se incorporan fuentes de terceros.
