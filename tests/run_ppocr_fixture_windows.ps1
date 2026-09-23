param(
    [string]$OrtRoot = "E:\_Dev\Vast-OCR-staging\host-ort-1.28.0-win-x64\onnxruntime-win-x64-1.28.0"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$gcc = (Get-Command gcc.exe -ErrorAction Stop).Source
$ortDll = Join-Path $OrtRoot "lib\onnxruntime.dll"

if (-not (Test-Path -LiteralPath $ortDll)) {
    throw "Missing host ONNX Runtime DLL: $ortDll"
}

$output = Join-Path $repo "build\test_ppocr_fixture_windows.exe"
& $gcc -std=c11 -O2 -Wall -Wextra -Werror `
    -I (Join-Path $repo "src") `
    -I (Join-Path $repo "third_party\onnxruntime\include") `
    (Join-Path $repo "src\ppocr_recognizer.c") `
    (Join-Path $repo "tests\host_win32_compat.c") `
    (Join-Path $repo "tests\test_ppocr_fixture_inference.c") `
    -o $output
if ($LASTEXITCODE -ne 0) { throw "Fixture test compilation failed" }

$oldOrtDll = $env:PPOCR_HOST_ORT_DLL
try {
    $env:PPOCR_HOST_ORT_DLL = $ortDll
    Push-Location $repo
    try {
        & $output
        if ($LASTEXITCODE -ne 0) { throw "Fixture inference failed" }
    }
    finally {
        Pop-Location
    }
}
finally {
    $env:PPOCR_HOST_ORT_DLL = $oldOrtDll
}
