[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BundlePath
)

$ErrorActionPreference = "Stop"
$bundle = (Resolve-Path -LiteralPath $BundlePath).Path
$pluginBinary = Join-Path $bundle "Contents\Win64\AI-Depth-Pro.ofx"
$model = Join-Path $bundle "Contents\models\depth_anything_v2_vits.onnx"

if (-not (Test-Path -LiteralPath $pluginBinary -PathType Leaf)) {
    throw "The built plugin binary is missing: $pluginBinary"
}
if (-not (Test-Path -LiteralPath $model -PathType Leaf)) {
    throw "The depth model is missing from the bundle: $model"
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdministrator = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdministrator) {
    $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -BundlePath `"$bundle`""
    $process = Start-Process -FilePath "powershell.exe" -Verb RunAs -ArgumentList $arguments -Wait -PassThru
    exit $process.ExitCode
}

$pluginsDirectory = Join-Path $env:ProgramFiles "Common Files\OFX\Plugins"
$destination = Join-Path $pluginsDirectory "AI-Depth-Pro.ofx.bundle"
New-Item -ItemType Directory -Force -Path $pluginsDirectory, $destination | Out-Null

& robocopy.exe $bundle $destination /MIR /R:2 /W:1 /NFL /NDL /NJH /NJS /NP
$robocopyExitCode = $LASTEXITCODE
if ($robocopyExitCode -ge 8) {
    throw "Copying the plugin failed with robocopy exit code $robocopyExitCode. Close DaVinci Resolve and try again."
}

Write-Host "Installed to: $destination"
