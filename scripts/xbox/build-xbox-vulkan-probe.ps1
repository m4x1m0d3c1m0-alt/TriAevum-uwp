param(
 [string]$MesaDirectory = $(if($env:SMASH4_MESA){$env:SMASH4_MESA}else{"C:\mesa-dozen"}),
 [string]$DxilValidator = $env:DXIL_VALIDATOR_DLL,
 [string]$VulkanSdk = $env:VULKAN_SDK,
 [ValidateSet("Debug","Release")][string]$Configuration="Release"
)
$ErrorActionPreference="Stop"
$root=(Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$project=Join-Path $root "platform\xbox_uwp_vulkan_probe\TriAevumVulkanProbe.vcxproj"
$mesaDll=Join-Path $MesaDirectory "build-uwp\src\microsoft\vulkan\vulkan_dzn.dll"
if(-not(Test-Path $mesaDll)){throw "No encuentro $mesaDll. Compila primero Mesa Dozen UWP."}
if(-not $DxilValidator -or -not(Test-Path $DxilValidator)){throw "Define DXIL_VALIDATOR_DLL con la ruta a DXIL.dll UWP oficial (1.8.2502.8 o posterior). No uses una DLL de escritorio."}
if(-not $VulkanSdk -or -not(Test-Path (Join-Path $VulkanSdk "Include\vulkan\vulkan.h"))){throw "Instala Vulkan-Headers/Vulkan SDK y define VULKAN_SDK."}
$vswhere=Join-Path "$env:ProgramFiles(x86)" "Microsoft Visual Studio\Installer\vswhere.exe"
$vsRoot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsRoot){throw "No encuentro Visual Studio con herramientas C++."}
$msbuild=Join-Path $vsRoot "MSBuild\Current\Bin\MSBuild.exe"
if(-not(Test-Path $msbuild)){throw "No encuentro MSBuild: $msbuild"}
$assets=Join-Path (Split-Path $project) "GenerateAssets.ps1"
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $assets -OutputDirectory (Join-Path (Split-Path $project) "Assets")
if($LASTEXITCODE -ne 0){throw "No se pudieron generar los assets del paquete."}
$mesaOutput=Join-Path $MesaDirectory "build-uwp\src\microsoft\vulkan"
& $msbuild $project /m /t:Rebuild "/p:Configuration=$Configuration" /p:Platform=x64 /p:AppxPackage=true "/p:VulkanSdk=$VulkanSdk" "/p:MesaUwpOutput=$mesaOutput" "/p:DxilValidatorPath=$DxilValidator" /verbosity:minimal
if($LASTEXITCODE -ne 0){throw "Falló la compilación del probe UWP."}
Write-Host "Compilación completada. Revisa platform\xbox_uwp_vulkan_probe\AppPackages."
Write-Host "El paquete no está firmado: fírmalo con el certificado de Dev Mode antes de instalarlo."
