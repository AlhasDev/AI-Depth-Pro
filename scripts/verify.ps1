$ErrorActionPreference = "Stop"
$project = Split-Path -Parent $PSScriptRoot
cmake -S $project -B (Join-Path $project "build") -G "Visual Studio 17 2022" -A x64 -DBUILD_INFERENCE_TESTS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build (Join-Path $project "build") --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir (Join-Path $project "build") -C Release --output-on-failure
exit $LASTEXITCODE
