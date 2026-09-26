[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

if (-not [Environment]::Is64BitOperatingSystem -or $env:OS -ne "Windows_NT") {
    throw "This setup script supports 64-bit Windows only."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$runtimeDir = Join-Path $repoRoot "deps\onnxruntime"
$modelDir = Join-Path $repoRoot "models"
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("ai-depth-pro-setup-" + [Guid]::NewGuid().ToString("N"))

$ortVersion = "1.24.4"
$directMLVersion = "1.15.4"
$modelRevision = "c70d1ddbcd93c9bda8098268cc3554adf5e8dd4f"

$ortPackageUrl = "https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime.directml/$ortVersion/microsoft.ml.onnxruntime.directml.$ortVersion.nupkg"
$ortPackageSha512 = "6633B2BF8F79BE17D55E84C9A76BAD4729FC8ABD53148BC28F407D24FF106460574B4A05C7E958ED9099A927675528505FA12DC588B64F754EB585DC9814D5E0"
$directMLPackageUrl = "https://api.nuget.org/v3-flatcontainer/microsoft.ai.directml/$directMLVersion/microsoft.ai.directml.$directMLVersion.nupkg"
$directMLPackageSha512 = "FDE767F56904ABC90FD53F65D8729C918AB7F6E3C5E1ECDD479908FC02B4535CF2B0860F7AB2ACB9B731D6CB809B72C3D5D4D02853FB8F5EA022A47BC44EF285"
$modelUrl = "https://huggingface.co/onnx-community/depth-anything-v2-small/resolve/$modelRevision/onnx/model.onnx"
$modelSha256 = "AFB6A5C28F3B6BF1618C6E43F02073EF9DFDC70E937502D51603E57B0A1DF10C"

$expectedFiles = @{
    "onnxruntime.dll" = "E7EEDEC6A6F26DC39DC948276A75EF6D2BEE3FFF944D874CEED0BBD3B97BFF40"
    "onnxruntime_providers_shared.dll" = "265C8DAF29637CB259CAC8BE9F08F2CD45F3883F0F0E4949CBFDDD5B4CBEC3B6"
    "DirectML.dll" = "C07CB8C2F870F0152BFB690F64C4B048E78BBC219EFAD420A9758FC896CBECB0"
}

function Test-VerifiedFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][ValidateSet("SHA256", "SHA512")][string]$Algorithm,
        [Parameter(Mandatory = $true)][string]$ExpectedHash
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }

    return (Get-FileHash -LiteralPath $Path -Algorithm $Algorithm).Hash -eq $ExpectedHash
}

function Get-VerifiedDownload {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter(Mandatory = $true)][ValidateSet("SHA256", "SHA512")][string]$Algorithm,
        [Parameter(Mandatory = $true)][string]$ExpectedHash
    )

    Write-Host "Downloading $Name..."
    Invoke-WebRequest -Uri $Uri -OutFile $Destination -UseBasicParsing
    if (-not (Test-VerifiedFile -Path $Destination -Algorithm $Algorithm -ExpectedHash $ExpectedHash)) {
        Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
        throw "$Name failed its $Algorithm integrity check."
    }
}

function Expand-NuGetPackage {
    param(
        [Parameter(Mandatory = $true)][string]$PackagePath,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $zipPath = "$PackagePath.zip"
    Copy-Item -LiteralPath $PackagePath -Destination $zipPath
    Expand-Archive -LiteralPath $zipPath -DestinationPath $Destination -Force
}

New-Item -ItemType Directory -Force -Path $runtimeDir, $modelDir, $tempRoot | Out-Null

try {
    $ortRuntimeReady = (-not $Force) -and
        (Test-VerifiedFile -Path (Join-Path $runtimeDir "onnxruntime.dll") -Algorithm SHA256 -ExpectedHash $expectedFiles["onnxruntime.dll"]) -and
        (Test-VerifiedFile -Path (Join-Path $runtimeDir "onnxruntime_providers_shared.dll") -Algorithm SHA256 -ExpectedHash $expectedFiles["onnxruntime_providers_shared.dll"])

    if ($ortRuntimeReady) {
        Write-Host "ONNX Runtime $ortVersion is already verified."
    } else {
        $ortPackage = Join-Path $tempRoot "onnxruntime-directml.nupkg"
        $ortExtract = Join-Path $tempRoot "onnxruntime-directml"
        Get-VerifiedDownload -Name "ONNX Runtime DirectML $ortVersion" -Uri $ortPackageUrl -Destination $ortPackage -Algorithm SHA512 -ExpectedHash $ortPackageSha512
        Expand-NuGetPackage -PackagePath $ortPackage -Destination $ortExtract
        Copy-Item -LiteralPath (Join-Path $ortExtract "runtimes\win-x64\native\onnxruntime.dll") -Destination $runtimeDir -Force
        Copy-Item -LiteralPath (Join-Path $ortExtract "runtimes\win-x64\native\onnxruntime_providers_shared.dll") -Destination $runtimeDir -Force
    }

    $directMLPath = Join-Path $runtimeDir "DirectML.dll"
    if ((-not $Force) -and (Test-VerifiedFile -Path $directMLPath -Algorithm SHA256 -ExpectedHash $expectedFiles["DirectML.dll"])) {
        Write-Host "DirectML $directMLVersion is already verified."
    } else {
        $directMLPackage = Join-Path $tempRoot "directml.nupkg"
        $directMLExtract = Join-Path $tempRoot "directml"
        Get-VerifiedDownload -Name "DirectML $directMLVersion" -Uri $directMLPackageUrl -Destination $directMLPackage -Algorithm SHA512 -ExpectedHash $directMLPackageSha512
        Expand-NuGetPackage -PackagePath $directMLPackage -Destination $directMLExtract
        Copy-Item -LiteralPath (Join-Path $directMLExtract "bin\x64-win\DirectML.dll") -Destination $runtimeDir -Force
    }

    $modelPath = Join-Path $modelDir "depth_anything_v2_vits.onnx"
    if ((-not $Force) -and (Test-VerifiedFile -Path $modelPath -Algorithm SHA256 -ExpectedHash $modelSha256)) {
        Write-Host "Depth Anything V2 Small model is already verified."
    } else {
        $downloadedModel = Join-Path $tempRoot "depth_anything_v2_vits.onnx"
        Get-VerifiedDownload -Name "Depth Anything V2 Small model" -Uri $modelUrl -Destination $downloadedModel -Algorithm SHA256 -ExpectedHash $modelSha256
        Move-Item -LiteralPath $downloadedModel -Destination $modelPath -Force
    }

    foreach ($file in $expectedFiles.GetEnumerator()) {
        $path = Join-Path $runtimeDir $file.Key
        if (-not (Test-VerifiedFile -Path $path -Algorithm SHA256 -ExpectedHash $file.Value)) {
            throw "$($file.Key) failed its final integrity check."
        }
    }
    if (-not (Test-VerifiedFile -Path $modelPath -Algorithm SHA256 -ExpectedHash $modelSha256)) {
        throw "The model failed its final integrity check."
    }

    Write-Host ""
    Write-Host "AI Depth Pro dependencies are ready."
    Write-Host "Runtime: $runtimeDir"
    Write-Host "Model:   $modelPath"
} finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
