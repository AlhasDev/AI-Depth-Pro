[CmdletBinding()]
param([ValidatePattern('^\d+\.\d+\.\d+(-[a-zA-Z0-9.-]+)?$')][string]$Version = '0.2.0')
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$bundle = Join-Path $repo 'build\AI-Depth-Pro.ofx.bundle'
$dist = Join-Path $repo 'dist'
$stage = Join-Path $dist ('package-' + [Guid]::NewGuid().ToString('N'))
$required = @('Contents\Win64\AI-Depth-Pro.ofx', 'Contents\Win64\onnxruntime.dll',
    'Contents\Win64\onnxruntime_providers_shared.dll', 'Contents\Win64\DirectML.dll',
    'Contents\models\depth_anything_v2_vits.onnx')
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $bundle $file) -PathType Leaf)) { throw "Missing bundle file: $file. Run install.bat --build-only first." }
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null
try {
    Copy-Item -LiteralPath $bundle -Destination $stage -Recurse
    Copy-Item -LiteralPath (Join-Path $repo 'scripts\install-bundle.ps1') -Destination $stage
    Copy-Item -LiteralPath (Join-Path $repo 'release\install.bat') -Destination $stage
    Copy-Item -LiteralPath (Join-Path $repo 'release\README.txt') -Destination $stage
    Copy-Item -LiteralPath (Join-Path $repo 'LICENSE'), (Join-Path $repo 'THIRD_PARTY_NOTICES.md') -Destination $stage
    Copy-Item -LiteralPath (Join-Path $repo 'release\licenses') -Destination $stage -Recurse
    Copy-Item -LiteralPath (Join-Path $repo 'include\ONNX_RUNTIME_LICENSE') -Destination (Join-Path $stage 'licenses\ONNX_RUNTIME_LICENSE')
    Copy-Item -LiteralPath (Join-Path $repo 'include\openfx\LICENSE') -Destination (Join-Path $stage 'licenses\OPENFX_LICENSE')
    $archive = Join-Path $dist "AI-Depth-Pro-$Version-windows-x64.zip"
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -Force
    $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($archive))" | Set-Content -LiteralPath "$archive.sha256" -Encoding ASCII
    Write-Host "Release ready: $archive"
} finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
