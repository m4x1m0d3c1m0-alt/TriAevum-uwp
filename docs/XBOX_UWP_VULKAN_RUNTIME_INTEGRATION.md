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


## Paso intermedio añadido: inventario de capacidades en la consola

Antes de enlazar el backend real, amplié el probe a la versión 0.1.0.4 para que capture el perfil del adaptador real. Además de extensiones y swapchain, ahora registra:
- features Vulkan core seleccionadas (geometría, tessellation, indirect draw, anisotropía, formatos BC, storage images, indexación dinámica, shader int64/float64, etc.);
- features Vulkan 1.1/1.2 relevantes para descriptor indexing, runtime descriptor arrays, scalar block layout, timeline semaphores, buffer device address y shader output viewport/layer;
- límites de push constants, storage/uniform buffers, descriptores, attachments, dimensiones de imagen/framebuffer y alineaciones;
- capacidades de superficie y formatos de presentación.

La razón para hacer este paso antes del dispatch UWP es evitar implementar un camino NRI/renderer que active características que el driver de Xbox no ofrece. La versión 0.1.0.4 es una prueba de diagnóstico; todavía no modifica el backend de TriAevum.


## Perfil de capacidades observado en Xbox Series S (probe 0.1.0.4)

El log de la consola identifica `Microsoft Direct3D12 (SraKmd_arden)`, Vulkan `1.2.354`, vendor `1414`, device `d000`. El sondeo de características core/Vulkan 1.2 informa:
- Disponibles: `geometryShader`, `independentBlend`, `samplerAnisotropy`, `textureCompressionBC`, `multiDrawIndirect`, `shaderStorageImageWriteWithoutFormat`, `shaderFloat64`, `shaderInt64`, `timelineSemaphore`, `scalarBlockLayout`, `uniformBufferStandardLayout`.
- No disponibles: `tessellationShader`, `shaderInt16`, `shaderFloat16`, `shaderInt8`, `descriptorIndexing`, `runtimeDescriptorArray`, `descriptorBindingPartiallyBound`, `bufferDeviceAddress`, `multiViewport`, `wideLines`.
- Las extensiones `VK_KHR_synchronization2` y `VK_KHR_dynamic_rendering` aparecen enumeradas. El log 0.1.0.4 no consultó sus bits de feature, por lo que su presencia en la lista no debe confundirse con soporte habilitable. El probe 0.1.0.5 añade la consulta explícita.
- El backend existente ya hace opcionales NIS/FSR en función de features de descriptor update-after-bind y shaderInt16, respectivamente. El perfil medido apunta a mantener ambas rutas desactivadas en la primera integración UWP.

El perfil es evidencia del ICD Dozen en esta consola, no una certificación de que todas las rutas Vulkan del renderer sean compatibles. Antes de activar NRI hay que confirmar synchronization2 y revisar su contrato de features; antes de integrar shaders hay que comprobar los formatos/operaciones que realmente usan.


## Perfil confirmado por el probe 0.1.0.5 (Xbox Series S)

El usuario ejecutó el probe 0.1.0.5 en la consola. El resultado vuelve a identificar el adaptador `Microsoft Direct3D12 (SraKmd_arden)`, Vulkan `1.2.354`, vendor `0x1414`, device `0xD000`. La consulta explícita de estructuras de características informa:

- `synchronization2=1` y `dynamicRendering=1`. El backend actual ya consulta ambas características y, cuando están disponibles junto con sus extensiones, las activa al crear el dispositivo. En esta Xbox, ese camino debe quedar habilitado; no hace falta desactivarlo por UWP.
- `timelineSemaphore=1`, `drawIndirectCount=1`, `scalarBlockLayout=1`, `uniformBufferStandardLayout=1`, `separateDepthStencilLayouts=1`, `samplerAnisotropy=1`, `textureCompressionBC=1`.
- `descriptorIndexing=0`, `descriptorBindingSampledImageUpdateAfterBind=0`, `descriptorBindingStorageImageUpdateAfterBind=0`, `descriptorBindingPartiallyBound=0`, `descriptorBindingVariableDescriptorCount=0`, `runtimeDescriptorArray=0` y las variantes de indexación no uniforme están a cero.
- `shaderInt16=0`, `shaderFloat16=0`, `shaderInt8=0`, `bufferDeviceAddress=0`, `tessellationShader=0`, `multiViewport=0`, `wideLines=0`.

### Consecuencia directa para NRI

El código de `gfx_vulkan_device.cpp` ya aplica gates conservadores antes de crear el dispositivo:

- NIS solo se anuncia a NRI si están disponibles `shaderStorageImageWriteWithoutFormat`, `timelineSemaphore`, `descriptorBindingSampledImageUpdateAfterBind` y `descriptorBindingStorageImageUpdateAfterBind`. En la Series S, NIS queda deshabilitado por los dos requisitos update-after-bind.
- FSR solo se anuncia a NRI si están disponibles `shaderInt16` y `timelineSemaphore`. En la Series S, FSR queda deshabilitado por `shaderInt16`.
- `synchronization2` y `dynamicRendering` sí se detectan y habilitan en este perfil. Debemos conservar esa negociación existente.

No se deben activar manualmente features que el driver informa como cero ni inferir compatibilidad a partir de que una extensión aparezca enumerada.

## Decisión de implementación: no hacer un cast de ventana ni un loader parcial

La inspección de `gfx_sdl.h`, `gfx_vulkan.h`, `gfx_vulkan_device.cpp` y `gfx_vulkan_lifecycle.cpp` concreta el alcance:

1. `GfxRenderingAPIVulkan` recibe actualmente `GfxWindowBackendSDL2*`; `Init()` exige `UsesVulkan()` y `GetNativeWindow()` SDL. La selección de colas también consulta `SDL_GetCurrentVideoDriver()`, y la caché de shaders recurre a `SDL_GetPrefPath()`.
2. La superficie Win32 actual obtiene un HWND mediante `SDL_GetWindowWMInfo`. La integración UWP debe entregar el CoreWindow como un objeto nativo WinRT explícito y aislar la llamada de superficie UWP en una rama de plataforma, sin simular un HWND SDL.
3. El runtime tiene llamadas globales `vk*` distribuidas por varias unidades; ImGui Vulkan se compila dentro del target `ImGui` y enlaza `Vulkan::Vulkan`. NRI se crea sobre el dispositivo existente, pero también requiere que su wrapper VK reciba una dispatch compatible. Por tanto, resolver solo `vkCreateInstance` no es suficiente: el plan de loader debe cubrir global, instancia y dispositivo, además de ImGui y NRI.
4. La primera fase ejecutable debe excluir providers NGX/DLSS, FSR, NIS, FidelityFX y otros módulos de escritorio no esenciales. NRI puede quedar apagado en el primer hito; después se habilitará con su propio contrato de loader/feature.

La implementación no debe introducir una dependencia nueva de Volk ni copiar código de terceros hasta verificar cómo inicializa su dispatch y cómo NRI/ImGui consumen los símbolos en este árbol. La siguiente tarea de código es inventariar las dependencias de loader del target Vulkan (runtime, ImGui, NRI y herramientas de shader) y crear una abstracción de plataforma pequeña para el contexto de ventana. Solo después se cambia el enlace CMake del target UWP. Así se evita un loader a medias que compile pero falle al cargar el APPX.


## Segunda inspección: loader compartido, ImGui y NRI (20-11-2026)

Se inspeccionaron los puntos de entrada exactos del backend y el wrapper NRI v180 fijado por el proyecto.

### Backend y superficie

- `gfx_vulkan_device.cpp::CreateInstance()` tiene una rama `_WIN32` que solicita `VK_KHR_surface` y `VK_KHR_win32_surface`. Es la lista correcta de extensiones que Dozen expone en la Xbox, pero no constituye por sí sola un host UWP.
- `CreateSurface()` usa `SDL_GetWindowWMInfo` y `windowInfo.info.win.window`. La ruta UWP debe sustituir solo la obtención del objeto nativo y llamar a la superficie Dozen con el CoreWindow IUnknown, siguiendo el mecanismo probado por `winrt::get_unknown(window)` en el probe.
- `gfx_vulkan_lifecycle.cpp::Init()` rechaza explícitamente cualquier ventana que no sea `GfxWindowBackendSDL2` con `UsesVulkan()`; el constructor y el miembro `mWindowBackend` también están tipados como SDL2. La solución completa necesita un contrato de ventana común o un backend UWP propio, no un cast de ventana.
- `gfx_vulkan_internal.h` incluye SDL2, SDL2 Vulkan y SDL2 SysWM de forma global para todas las unidades del backend. El trabajo de plataforma debe aislar esas inclusiones detrás de la implementación de escritorio.
- La selección de colas consulta `SDL_GetCurrentVideoDriver()`; el capability `PresentTearing` y la ruta de caché también consultan el backend SDL. Esos usos deben pasar por el contrato común de ventana/plataforma.
- `VulkanShaderCacheDirectory()` recurre a `SDL_GetPrefPath()` si no hay `TRIAEVUM_RENDERER_CACHE_DIR`. En UWP la ruta debe proceder de `ApplicationData::Current().LocalFolder()` o de un directorio explícito del host.

### Loader: opción recomendada para probar primero

La búsqueda del código de Volk confirma que ofrece `volkInitializeCustom(PFN_vkGetInstanceProcAddr)`, seguido de carga de dispatch de instancia y dispositivo. ImGui Vulkan incluye soporte condicional `IMGUI_IMPL_VULKAN_USE_VOLK`. Esto permite una ruta técnica concreta:

1. Compilar Volk como código estático del target UWP, sin `volkInitialize()` ni carga de `vulkan-1.dll`.
2. En UWP, cargar `vulkan_dzn.dll` con `LoadPackagedLibrary`, obtener `vk_icdGetInstanceProcAddr` y pasarlo a `volkInitializeCustom`.
3. Tras `vkCreateInstance`, llamar a `volkLoadInstance`; tras `vkCreateDevice`, llamar a `volkLoadDevice`. El backend actual usa símbolos globales `vk*`, por lo que este paso evita tener que escribir manualmente cientos de wrappers.
4. Compilar `imgui_impl_vulkan.cpp` con `IMGUI_IMPL_VULKAN_USE_VOLK` y garantizar que utiliza la misma instancia de Volk/dispatch global que el renderer.
5. Eliminar `Vulkan::Vulkan` del enlace UWP; mantener el enlace convencional para escritorio.

Esto es una propuesta de implementación, no un cambio ya probado. Antes de adoptarla hay que fijar la versión de Volk y comprobar que el código de la rama y el Vulkan-Headers empleado son compatibles.

### Hallazgo crítico de NRI

El wrapper NRI v180 (`NRIWrapperVK.h`) no recibe un `PFN_vkGetInstanceProcAddr` en `DeviceCreationVKDesc`. Sí recibe `libraryPath`, además de la instancia, dispositivo físico y dispositivo existentes.

En el código fijado de NRI (`Source/VK/DeviceVK.hpp`), `DeviceVK::Create()` hace lo siguiente incluso cuando envuelve un dispositivo existente:
- Carga una biblioteca con `LoadSharedLibrary(libraryPath ? libraryPath : NRI_VULKAN_LOADER_NAME)`.
- Resuelve el dispatch Vulkan de pre-instancia y de instancia desde esa biblioteca.
- En modo wrapper reutiliza la instancia y dispositivo entregados, pero conserva su propia tabla de funciones.

Por ello, pasar simplemente el dispositivo creado por Volk no basta. La compilación UWP inicial debe desactivar NRI (`THREE_DS_RECOMP_ENABLE_NRI=OFF`) hasta que el wrapper pueda cargar Dozen mediante una ruta AppContainer válida. Después se parcheará el loader de NRI para usar `LoadPackagedLibrary` bajo el target UWP y se le suministrará `vulkan_dzn.dll` como `libraryPath`; no se debe habilitar NRI antes de validar esa ruta.

### Orden de implementación resultante

A. Crear un target UWP mínimo que compile Volk y el backend Vulkan sin `Vulkan::Vulkan`, ImGui Vulkan convencional ni NRI.  
B. Añadir el host CoreWindow y el contrato de ventana UWP; conservar SDL2 sin cambios en escritorio.  
C. Crear instancia/superficie/dispositivo/swapchain a través del backend TriAevum y mostrar un clear frame.  
D. Activar ImGui mediante Volk.  
E. Adaptar el loader NRI a UWP y volver a habilitar NRI.  
F. Reincorporar shaders/runtime de compilación y módulos opcionales según su compatibilidad real.

No se considera terminado ningún hito por compilar únicamente: cada fase necesita prueba de arranque en la Xbox y un registro de resultado.
