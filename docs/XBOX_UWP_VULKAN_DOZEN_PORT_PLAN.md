# Xbox Series S: estrategia Vulkan sobre Dozen

Estado: plan técnico de integración; todavía no es una implementación funcional del renderer en Xbox.

## Decisión

No continuar ampliando el host D3D12 de prueba como renderer final. Mantenerlo solo como referencia de diagnóstico mientras se prepara una ruta Vulkan UWP.

La referencia técnica principal para el puente Vulkan/Dozen será [jajdp/smash4-xbox](https://github.com/jajdp/smash4-xbox), porque documenta el trabajo real necesario para ejecutar Vulkan sobre D3D12 dentro de un AppContainer: compilación UWP de Mesa Dozen, carga de DXIL, stubs de APIs Win32, presentación CoreWindow, limitaciones de características y diagnóstico de D3D12. [JYRS20/WWHD-UWP](https://github.com/JYRS20/WWHD-UWP) se conserva como segunda referencia para la adaptación UWP y las pruebas comunicadas en Series S.

**Límite de la evidencia:** Smash4-UWP declara pruebas en Series X y advierte expresamente que Series S no se ha probado. Sus resultados no demuestran que TriAevum funcione en Series S. WWHD-UWP sí declara pruebas de usuario en Series S, pero su renderer no es el de TriAevum.

## Por qué no copiar el renderer ni el host completos

TriAevum usa su propio backend Vulkan y NRI. Smash4 modifica su renderer para eliminar dependencias de características Vulkan que Dozen no expone en Xbox, como buffer device address y external host memory, y para trabajar con el límite de 128 MiB por storage buffer. Esas modificaciones dependen de la arquitectura y del ABI de Smash4; no se pueden aplicar mecánicamente al renderer de TriAevum.

Tampoco se debe asumir que NRI funcionará solo porque Dozen exponga Vulkan 1.2. Hay que comprobar qué extensiones, features, formatos, límites y rutas de memoria solicita realmente el backend de TriAevum y cuáles ofrece el dispositivo Xbox.

## Orden de trabajo con puertas de validación

### 0. Mantener una base reproducible
- Trabajar exclusivamente en `xbox-series-s-uwp`; nunca modificar `main`.
- Mantener el host D3D12 actual como diagnóstico, sin confundir su compilación con un port funcional.
- Fijar una revisión concreta de Mesa/Dozen, DXIL y SDL3 UWP; registrar procedencia y licencias.
- No copiar binarios precompilados de otros repositorios al proyecto.

### 1. Prueba Vulkan UWP independiente (primer objetivo)
Crear un ejecutable UWP mínimo, separado del juego, que:
- cargue explícitamente el ICD Dozen empaquetado y el validador DXIL;
- inicialice Vulkan y enumere dispositivo, API, extensiones, features, formatos y límites;
- cree una superficie desde CoreWindow y una swapchain;
- limpie y presente fotogramas;
- registre cada etapa y HRESULT/VkResult en LocalState.

Esta prueba debe funcionar en el PC con Dozen y después en Xbox Series S. No integrar el renderer de TriAevum antes de superar esta puerta.

### 2. Inventario de compatibilidad del renderer TriAevum
Sobre el dispositivo real, comparar las capacidades registradas con:
- las features y extensiones solicitadas por el backend Vulkan de TriAevum;
- los requisitos de NRI y de sus recursos/pipelines;
- formatos de color/profundidad, MSAA, descriptor indexing, BDA, memoria externa, sincronización y límites de buffers;
- compilación de shaders y validación DXIL.

Clasificar cada requisito como disponible, opcional/desactivable o bloqueante. Desactivar módulos opcionales (DLSS/NGX, upscalers y efectos no esenciales) en el primer build.

### 3. Integración del renderer
Solo después del inventario:
- adaptar la carga de funciones Vulkan al entorno UWP sin depender de un loader de escritorio;
- conectar la superficie CoreWindow y la presentación;
- añadir los workarounds de Dozen que sean realmente necesarios para TriAevum;
- implementar una ruta de memoria compatible si el renderer depende de capacidades ausentes;
- mantener el backend de escritorio intacto mediante compilación condicional.

### 4. Arranque mínimo del juego
Primero menú/escena sencilla y un frame estable; después recursos, shaders, audio, mandos, guardado, suspensión/reanudación y optimización de memoria.

## Criterios de éxito

No considerar terminado ningún hito por el mero hecho de compilar o generar un APPX.

1. El probe UWP inicia en Series S y presenta fotogramas.
2. El inventario Vulkan se guarda en un log recuperable.
3. El backend TriAevum crea instancia/dispositivo y swapchain sin características no soportadas.
4. Se presenta una escena real de TriAevum.
5. Se mantiene estable durante una sesión prolongada y con mando.

## Referencias

- [jajdp/smash4-xbox](https://github.com/jajdp/smash4-xbox): Dozen UWP, WSI CoreWindow, DXIL, system stubs, límites Vulkan/D3D12 y diagnóstico. Sus pruebas publicadas son de Series X; Series S figura como no probada.
- [JYRS20/WWHD-UWP](https://github.com/JYRS20/WWHD-UWP): overlay UWP, SDL3, Dozen y pruebas comunicadas en Series S.
- Código actual de TriAevum: `runtime/three_ds_recomp/src/fast/backends/gfx_vulkan.cpp`, configuración Vulkan/NRI en `runtime/three_ds_recomp/CMakeLists.txt` y host de prueba en `platform/xbox_uwp_d3d12/`.

## Próximo cambio de código

Implementar el probe Vulkan UWP con diagnóstico, no portar todavía el renderer. Es el experimento más pequeño que puede demostrar si la cadena crítica Dozen + DXIL + AppContainer + CoreWindow funciona en la consola objetivo y evita invertir semanas adaptando NRI a una combinación de features que quizá no exista.


## Implementación inicial añadida (2026-11-20)

Se ha incorporado una primera infraestructura basada en el material público de `jajdp/smash4-xbox`:

- `third_party/mesa-dozen-uwp/patches/mesa-dozen-xbox.patch`: parche WSI CoreWindow y correcciones Dozen de esa referencia.
- `third_party/mesa-dozen-uwp/mesa_sysstubs.c` y `mesa_uwp_compat.h`: compatibilidad UWP/clang-cl.
- `scripts/xbox/prepare-mesa-dozen-uwp.ps1`: obtiene la rama estable `26.2` de Mesa y fija el árbol en el commit actual de esa rama, prepara los stubs y aplica el parche de forma idempotente.
- `scripts/xbox/build-mesa-dozen-uwp.ps1`: compila Dozen con entorno VS `x64 store`; prepara un venv Python con Meson/Ninja y dependencias.
- `platform/xbox_uwp_vulkan_probe/`: aplicación UWP independiente que carga `vulkan_dzn.dll`, crea instancia/dispositivo Vulkan y solicita una swapchain sobre CoreWindow.
- `scripts/xbox/build-xbox-vulkan-probe.ps1`: empaquetado del probe con Dozen UWP y DXIL.dll proporcionados localmente.

La aplicación registra el resultado en `LocalState\TriAevumVulkanProbe.log`. Aún no se ha compilado en el PC del usuario ni ejecutado en Series S. El siguiente paso es validar el build de Mesa, compilar el APPX, firmarlo e instalarlo. Solo después de confirmar `vkCreateSwapchainKHR(CoreWindow)` se añadirá clear/present y se empezará el inventario de capacidades que necesita TriAevum.

La primera versión del probe evita depender de `vulkan-1.dll`: carga `vulkan_dzn.dll` y usa su entrada ICD `vk_icdGetInstanceProcAddr`, siguiendo el patrón de `emelin2004/rpcs3-xbox/uwp/vkshim`. Esto debe comprobarse con el build UWP real de Mesa; no se da por validado hasta que la consola escriba el resultado en LocalState.


## Validación en Xbox Series S completada (2026-11-20)

La puerta 1 está superada en hardware real. El usuario ejecutó el probe UWP 0.1.0.3 y confirmó que aparece el fotograma azul. El log de 0.1.0.2 ya había registrado dispositivo, extensiones, cola gráfica y swapchain; 0.1.0.3 añadió adquisición de imagen, command pool/buffer, clear color, submit, present y espera de cola.

Resultado validado: Dozen traduce comandos Vulkan a D3D12 y presenta al CoreWindow en Xbox Series S. La pantalla azul es el clear color del swapchain, no una escena del juego.

La siguiente etapa ya no es ampliar el probe. Es integrar el loader/dispatch y el contrato de ventana CoreWindow en el backend Vulkan real. El análisis de los puntos de acoplamiento está en [XBOX_UWP_VULKAN_RUNTIME_INTEGRATION.md](XBOX_UWP_VULKAN_RUNTIME_INTEGRATION.md). En particular:
- el backend actual depende de SDL2 y usa `SDL_GetWindowWMInfo` para crear la superficie Win32;
- el runtime y Dear ImGui enlazan `Vulkan::Vulkan`, mientras el probe carga Dozen directamente mediante `vk_icdGetInstanceProcAddr`;
- NRI envuelve el dispositivo existente con `nriCreateDeviceFromVKDevice`, pero su dispatch y compilación UWP aún deben validarse;
- el runtime localiza actualmente `shaderc_shared.dll` del Vulkan SDK de escritorio, que no se ha probado en AppContainer.

No se ha modificado todavía el renderer de escritorio ni se ha intentado compilar el runtime para UWP.
