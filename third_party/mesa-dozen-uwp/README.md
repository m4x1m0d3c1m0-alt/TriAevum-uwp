# Mesa Dozen para Xbox UWP

Este directorio incorpora material técnico del proyecto [jajdp/smash4-xbox](https://github.com/jajdp/smash4-xbox), en particular el parche de Mesa, los stubs de imports Win32 y el encabezado de compatibilidad clang-cl.

- Proyecto de origen: https://github.com/jajdp/smash4-xbox
- Archivos de origen: `patches/mesa-dozen-xbox.patch`, `src/mesa_sysstubs.c`, `src/mesa_uwp_compat.h`, `tools/compilar-dozen-uwp.ps1`
- Licencia de los cambios UWP incorporados: MIT, copyright jajdp (2026); avisos y licencias originales de Mesa se mantienen para los ficheros modificados. Ver `LICENSE` de este directorio.
- Revisión de Mesa para la que se documentó el parche: `a44c011` (Mesa 26.2.x, commit de referencia del proyecto de origen).

## Uso

1. Clonar Mesa en una ruta corta y fijar la revisión indicada.
2. Copiar `mesa_sysstubs.c` y `mesa_uwp_compat.h` a `<mesa>\uwp\`.
3. Aplicar el parche con `git apply --check` antes de aplicarlo.
4. Ejecutar `scripts/xbox/build-mesa-dozen-uwp.ps1` desde PowerShell con las variables de entorno descritas en el script de origen.

No se incluyen binarios de Mesa, `vulkan_dzn.dll`, `DXIL.dll`, SDL ni SDKs. Deben compilarse u obtenerse de sus distribuidores oficiales y empaquetarse respetando sus licencias. No se debe usar una DLL Dozen compilada para escritorio en el paquete UWP.

El parche modifica el WSI Win32 de Mesa para que la superficie reciba un `CoreWindow` y cree la cadena con `CreateSwapChainForCoreWindow`. La aplicación UWP debe pasar el objeto CoreWindow al crear `VkWin32SurfaceKHR`; no debe pasar un HWND de escritorio.

Este es un primer bloque de infraestructura. No demuestra por sí mismo que TriAevum sea compatible con las extensiones, límites de memoria, formatos y características de Dozen en Xbox Series S.
