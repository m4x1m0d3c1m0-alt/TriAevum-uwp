# Xbox Series S Dev Mode: preparación de la prueba TriAevum

## Situación real del port

Esta rama todavía **no contiene una aplicación UWP instalable**. El ejecutable `triaevum_d3d12_probe.exe` es una prueba Win32 de escritorio y no se puede instalar en Xbox. Tampoco hay aún un proyecto de empaquetado MSIX/AppX, un host `CoreWindow`, una swap chain ni un renderer PICA D3D12.

No instales ni ejecutes el EXE de escritorio en la consola: no es un paquete Xbox.

## Preparar el PC

1. Windows 10/11 x64 actualizado.
2. Visual Studio 2022 o posterior, con el workload **Desarrollo de la plataforma universal de Windows (UWP)** y las herramientas C++ para UWP.
3. Instala un Windows 10 SDK compatible con el proyecto UWP y el componente de herramientas de empaquetado MSIX. Mantén la misma versión de SDK que se use al generar el proyecto.
4. Actualiza Visual Studio Installer y comprueba que el workload UWP no está pendiente de reinicio.
5. No hace falta instalar Vulkan SDK para la futura prueba D3D12 mínima. Para el TriAevum completo seguirán pendientes la migración del renderer y sus dependencias.

La compatibilidad de Visual Studio 2026 incluye desarrollo UWP para Xbox Series S/X. Consulta la matriz de compatibilidad de la versión concreta instalada antes de modificar el SDK.

## Preparar la Xbox

1. Arranca en **Dev Home** (no Retail Mode).
2. Inicia sesión con un usuario en la consola.
3. En Dev Home abre Remote Access Settings y activa Xbox Device Portal.
4. Anota la IP local que muestra la consola.
5. Mantén PC y Xbox en la misma red. Para el primer emparejamiento es preferible Ethernet.
6. No compartas públicamente el PIN de emparejamiento, contraseñas ni credenciales del Device Portal.

## Comprobar la conexión desde Windows

Abre PowerShell en la carpeta del repositorio y ejecuta:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\xbox\check-xbox-devmode.ps1 -XboxIp 192.168.1.50
```

Sustituye la IP de ejemplo por la que muestra Dev Home. El script solo comprueba conectividad TCP/HTTPS y detecta Visual Studio; no instala nada ni cambia la configuración de la consola.

El Device Portal suele estar en:

```
https://IP-DE-TU-XBOX:11443
```

El certificado HTTPS local puede generar un aviso del navegador. Comprueba que la IP coincide con la de tu consola antes de continuar.

## Cuando exista el paquete UWP

La prueba que hay que entregar antes de portar el juego será un paquete x64 firmado para sideload, con estas funciones:

- Arranque como aplicación UWP Xbox.
- Creación de dispositivo D3D12 y cola Direct.
- Creación de swap chain asociada al CoreWindow.
- Creación de RTVs, command allocator/list y fence.
- Clear de pantalla y Present repetido, con control correcto de resize, suspensión y reanudación.
- Diagnóstico visible de HRESULT y feature level.

La compilación de escritorio del probe no sustituye ninguna de estas comprobaciones.

## Instalación y depuración

**Opción recomendada: Visual Studio**

1. Empareja Visual Studio con la consola desde las opciones de Xbox/Remote Machine.
2. Selecciona la Xbox como destino (no Local Machine).
3. Compila la configuración x64.
4. Usa F5 para desplegar e iniciar con depuración, o Ctrl+F5 para desplegar sin depurador.

**Opción manual: Device Portal**

1. Genera desde Visual Studio un paquete AppX/MSIX firmado para sideload.
2. Abre Device Portal y ve a la página Home / Apps.
3. Carga el paquete y todas sus dependencias requeridas.
4. Instala y lanza la aplicación desde Dev Home.

No se debe generar un certificado improvisado ni distribuir un paquete sin firmar.

## Qué necesitamos verificar en la primera sesión

- La versión de Visual Studio y Windows SDK instalada.
- La IP y la conexión al Device Portal.
- Que Visual Studio detecta/empareja la Xbox.
- Que el proyecto UWP compila para el destino Xbox.
- Que la aplicación muestra el clear y mantiene la presentación sin errores.

## Referencias oficiales

- Visual Studio y desarrollo Xbox: https://learn.microsoft.com/windows/uwp/apps-for-xbox/development-options
- Preguntas frecuentes UWP Xbox y solución de problemas de despliegue: https://learn.microsoft.com/windows/uwp/xbox-apps/frequently-asked-questions
- Compatibilidad de Visual Studio: https://learn.microsoft.com/visualstudio/releases/2026/compatibility
- Despliegue y depuración UWP: https://learn.microsoft.com/visualstudio/debugger/deploy-windows-store-apps-from-visual-studio
- CoreWindow y DirectX UWP: https://learn.microsoft.com/windows/uwp/gaming/plan-your-directx-port
