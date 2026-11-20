# Probe Vulkan/Dozen para Xbox UWP

Proyecto UWP independiente, basado en la estructura CoreWindow del host D3D12 de TriAevum y en el WSI CoreWindow documentado por jajdp/smash4-xbox.

El ejecutable carga `vulkan_dzn.dll` desde el paquete (sin loader Vulkan de escritorio), resuelve la entrada ICD `vk_icdGetInstanceProcAddr` (sin cargar `vulkan-1.dll` ni un loader desktop), crea una instancia Vulkan, enumera GPU y extensiones, crea una superficie usando el CoreWindow real, selecciona una cola gráfica, crea un dispositivo y solicita una swapchain.

## Requisitos

- Mesa Dozen UWP compilado en `build-uwp`.
- `DXIL.dll` oficial compatible con UWP, versión 1.8.2502.8 o posterior.
- Vulkan-Headers / Vulkan SDK y `VULKAN_SDK` apuntando a su instalación.
- Visual Studio con C++ UWP y Windows SDK 10.0.26100.0.
- Certificado de firma para Xbox Dev Mode.

## Preparación y compilación

Desde la raíz del repositorio:

\`\`\`powershell
$env:SMASH4_MESA = 'C:\\mesa-dozen'
.\\scripts\\xbox\\prepare-mesa-dozen-uwp.ps1
# Instala Meson/Ninja y dependencias Python de Mesa; después:
.\\scripts\\xbox\\build-mesa-dozen-uwp.ps1
$env:VULKAN_SDK = 'C:\\VulkanSDK\\<version>'
$env:DXIL_VALIDATOR_DLL = 'C:\\ruta\\DXIL.dll'
.\\scripts\\xbox\\build-xbox-vulkan-probe.ps1
\`\`\`

No se descargan ni redistribuyen DXIL.dll ni binarios precompilados de Mesa. El probe empaqueta la DLL Dozen UWP compilada y el validador DXIL que proporciona el usuario.

## Diagnóstico

El resultado queda en `LocalState\TriAevumVulkanProbe.log`. La aplicación permanece abierta incluso si falla una fase para permitir recuperar el registro desde Device Portal.

El primer objetivo es que `vkCreateSwapchainKHR(CoreWindow)` termine con éxito. Este primer probe todavía no graba comandos ni presenta un fotograma: primero hay que confirmar que la inicialización Vulkan y la WSI funcionan correctamente en la consola.

No es todavía el renderer TriAevum ni un port jugable. Su función es aislar Dozen, AppContainer y CoreWindow antes de integrar NRI y el renderer Vulkan del juego.
