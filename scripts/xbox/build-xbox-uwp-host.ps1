$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$project = Join-Path $repoRoot "platform\xbox_uwp_d3d12\TriAevumXboxUwpHost.vcxproj"
$programFilesX86 = [Environment]::GetFolderPath("ProgramFilesX86")
$vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $project)) {
    throw "UWP host project not found: $project"
}
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio 2026 with the UWP C++ workload."
}

$vsPath = & $vswhere -latest -products "*" -requires Microsoft.Component.MSBuild -property installationPath
if (-not $vsPath) {
    throw "Visual Studio with MSBuild was not detected."
}

$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    throw "MSBuild.exe not found: $msbuild"
}

$logDir = Join-Path $repoRoot "build-logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$logPath = Join-Path $logDir "xbox-uwp-d3d12-host-build.log"

Write-Host "TriAevum Xbox UWP D3D12 host build"
Write-Host "Visual Studio: $vsPath"
Write-Host "Project:       $project"
Write-Host "SDK:           10.0.26100.0"
Write-Host "Configuration: Debug | x64"
Write-Host "Log:           $logPath"
Write-Host ""

Push-Location $repoRoot
try {
    $arguments = @(
        $project,
        "/m",
        "/t:Build",
        "/p:Configuration=Debug",
        "/p:Platform=x64",
        "/p:WindowsTargetPlatformVersion=10.0.26100.0",
        "/p:AppxPackageSigningEnabled=false",
        "/v:minimal",
        "/fl",
        "/flp:logfile=$logPath;verbosity=normal"
    )
    & $msbuild @arguments
    $buildExitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}

if ($buildExitCode -ne 0) {
    Write-Host ""
    Write-Host "BUILD FAILED (exit code $buildExitCode). Send the log file above for diagnosis." -ForegroundColor Red
    exit $buildExitCode
}

Write-Host ""
Write-Host "BUILD SUCCEEDED. This confirms compilation/package generation only; it does not prove Xbox runtime compatibility." -ForegroundColor Green
