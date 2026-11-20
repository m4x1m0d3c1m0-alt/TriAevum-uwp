param(
  [string]$MesaDirectory = $(if ($env:SMASH4_MESA) { $env:SMASH4_MESA } else { "C:\mesa-dozen" }),
)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$vendor = Join-Path $root "third_party\mesa-dozen-uwp"
$patch = Join-Path $vendor "patches\mesa-dozen-xbox.patch"
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw "Git no está instalado o no está en PATH." }
if (-not (Test-Path (Join-Path $MesaDirectory ".git"))) {
  if (Test-Path $MesaDirectory) { throw "La ruta existe pero no es un repositorio Git: $MesaDirectory" }
  git clone https://gitlab.freedesktop.org/mesa/mesa.git $MesaDirectory
  if ($LASTEXITCODE -ne 0) { throw "No se pudo clonar Mesa." }
}
git -C $MesaDirectory fetch origin a44c011
if ($LASTEXITCODE -ne 0) { throw "No se pudo obtener la revisión Mesa a44c011." }
git -C $MesaDirectory checkout --detach a44c011
if ($LASTEXITCODE -ne 0) { throw "No se pudo fijar Mesa en a44c011." }
$uwp = Join-Path $MesaDirectory "uwp"
New-Item -ItemType Directory -Force -Path $uwp | Out-Null
Copy-Item (Join-Path $vendor "mesa_sysstubs.c") (Join-Path $uwp "mesa_sysstubs.c") -Force
Copy-Item (Join-Path $vendor "mesa_uwp_compat.h") (Join-Path $uwp "mesa_uwp_compat.h") -Force
git -C $MesaDirectory apply --reverse --check $patch 2>$null
if ($LASTEXITCODE -eq 0) {
  Write-Host "El parche Dozen UWP ya estaba aplicado; no se repite."
} else {
  git -C $MesaDirectory apply --check $patch
  if ($LASTEXITCODE -ne 0) { throw "El parche no aplica limpiamente a esta revisión de Mesa. No se ha forzado." }
  git -C $MesaDirectory apply $patch
  if ($LASTEXITCODE -ne 0) { throw "No se pudo aplicar el parche Mesa Dozen UWP." }
}
Write-Host "Mesa preparada en: $MesaDirectory"
Write-Host "Revisión fijada: a44c011"
Write-Host "Parche Dozen UWP aplicado."
Write-Host "Siguiente: instalar Meson/Ninja y ejecutar scripts/xbox/build-mesa-dozen-uwp.ps1."
