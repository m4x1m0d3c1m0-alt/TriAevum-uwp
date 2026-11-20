param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^([0-9]{1,3}\.){3}[0-9]{1,3}$')]
    [string]$XboxIp
)

$ErrorActionPreference = 'Continue'
Write-Host ''
Write-Host 'TriAevum / Xbox Series S - comprobacion previa' -ForegroundColor Cyan
Write-Host "Consola: $XboxIp"

$pingOk = $false
try { $pingOk = Test-Connection -ComputerName $XboxIp -Count 1 -Quiet -ErrorAction Stop } catch {}
Write-Host ("ICMP (ping):             {0}" -f $(if ($pingOk) { 'RESPONDE' } else { 'SIN RESPUESTA / BLOQUEADO' }))

$tcpOk = $false
$client = New-Object System.Net.Sockets.TcpClient
try {
    $async = $client.BeginConnect($XboxIp, 11443, $null, $null)
    if ($async.AsyncWaitHandle.WaitOne(3000, $false) -and $client.Connected) {
        $client.EndConnect($async)
        $tcpOk = $true
    }
} catch { $tcpOk = $false } finally { $client.Close() }
Write-Host ("Device Portal TCP 11443: {0}" -f $(if ($tcpOk) { 'ACCESIBLE' } else { 'NO ACCESIBLE' }))

# Use the standard Windows Kits path directly; do not depend on environment variables.
$programFilesX86 = 'C:\Program Files (x86)'
$vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products '*' -property installationPath
    if ($vsPath) {
        Write-Host "Visual Studio:           $vsPath"
        $uwpPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Workload.Universal -property installationPath
        Write-Host ("Workload UWP:            {0}" -f $(if ($uwpPath) { 'DETECTADO' } else { 'NO DETECTADO (revisar Visual Studio Installer)' }))
    } else { Write-Host 'Visual Studio:           no detectado' }
} else { Write-Host "Visual Studio Installer: no encontrado en $vswhere" }

$sdkRoot = 'C:\Program Files (x86)\Windows Kits\10'
$includeRoot = Join-Path $sdkRoot 'Include'
$libRoot = Join-Path $sdkRoot 'Lib'
Write-Host "SDK Include examinado:   $includeRoot"
Write-Host "SDK Lib examinado:       $libRoot"
$validSdks = @()
if ((Test-Path $includeRoot) -and (Test-Path $libRoot)) {
    foreach ($includeDir in (Get-ChildItem -LiteralPath $includeRoot -Directory -ErrorAction SilentlyContinue)) {
        $version = $includeDir.Name
        $header = Join-Path $includeDir.FullName 'um\Windows.h'
        $libDir = Join-Path $libRoot $version
        if ((Test-Path -LiteralPath $header) -and (Test-Path -LiteralPath $libDir)) {
            $validSdks += $version
        }
    }
}
if ($validSdks.Count -gt 0) {
    Write-Host 'Windows SDK(s):'
    $validSdks | ForEach-Object { Write-Host "  $_" }
    if ($validSdks -contains '10.0.26100.0') {
        Write-Host 'SDK recomendado:         10.0.26100.0'
    } else {
        Write-Host "SDK recomendado:         $($validSdks[-1])"
    }
} else {
    Write-Host 'Windows SDK:             NO DETECTADO en la ruta estándar'
}

Write-Host ''
Write-Host "Device Portal: https://$($XboxIp):11443" -ForegroundColor Yellow
Write-Host 'Nota: un ping bloqueado no implica fallo si TCP 11443 responde.'
Write-Host 'Esta comprobacion no instala ni despliega aplicaciones.'
if (-not $tcpOk) { Write-Host 'Revisa Dev Home > Remote Access Settings, la red local y el firewall.' -ForegroundColor Yellow }
