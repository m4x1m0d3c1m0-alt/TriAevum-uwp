param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function New-PlaceholderPng {
    param([int]$Width, [int]$Height, [string]$Path, [string]$Caption)

    $bitmap = New-Object System.Drawing.Bitmap($Width, $Height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $background = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 16, 24, 40))
    $accent = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 50, 170, 255))
    $white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    $fontSize = [Math]::Max(10, [Math]::Min($Width, $Height) * 0.28)
    $font = New-Object System.Drawing.Font("Segoe UI", [single]$fontSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)

    $graphics.FillRectangle($background, 0, 0, $Width, $Height)
    $circle = [Math]::Max(4, [int]([Math]::Min($Width, $Height) * 0.30))
    $graphics.FillEllipse($accent, [single](($Width - $circle) / 2), [single](($Height - $circle) / 2), $circle, $circle)

    $text = if ($Width -gt ($Height * 1.5)) { $Caption } else { "TA" }
    $measure = $graphics.MeasureString($text, $font)
    $x = [single](($Width - $measure.Width) / 2)
    $y = [single](($Height - $measure.Height) / 2)
    $graphics.DrawString($text, $font, $white, $x, $y)
    $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)

    $font.Dispose()
    $white.Dispose()
    $accent.Dispose()
    $background.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
}

New-PlaceholderPng 50 50 (Join-Path $OutputDirectory "StoreLogo.png") "TA"
New-PlaceholderPng 150 150 (Join-Path $OutputDirectory "Square150x150Logo.png") "TA"
New-PlaceholderPng 44 44 (Join-Path $OutputDirectory "Square44x44Logo.png") "TA"
New-PlaceholderPng 310 150 (Join-Path $OutputDirectory "Wide310x150Logo.png") "TRIAEVUM D3D12"
New-PlaceholderPng 620 300 (Join-Path $OutputDirectory "SplashScreen.png") "TRIAEVUM D3D12"
