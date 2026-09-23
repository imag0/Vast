$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$WslRoot = (& wsl.exe -e wslpath -a $Root).Trim()
if ($LASTEXITCODE -ne 0 -or -not $WslRoot) {
    throw 'Unable to translate the repository path for WSL.'
}

function Invoke-WslTest {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Command
    )
    Write-Output "suite=$Name"
    Write-Output "command=$Command"
    & wsl.exe -e sh -lc $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE"
    }
}

Invoke-WslTest 'ocr-core' @"
cd '$WslRoot' && gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc test_ocr.c src/ocr_hash.c src/ocr_identity.c src/ocr_segmenter.c src/ocr_rasterizer.c src/handwriting_index.c src/ocr_sidecar.c -lm -o build/test_ocr_final && ./build/test_ocr_final
"@

Invoke-WslTest 'recognizer-preprocess-decoder' @"
cd '$WslRoot' && gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Ithird_party/onnxruntime/include test_ppocr_recognizer.c -ldl -lm -o build/test_ppocr_recognizer_final && ./build/test_ppocr_recognizer_final
"@

Invoke-WslTest 'manager-and-nonblocking-checkpoint' @"
cd '$WslRoot' && gcc -std=c11 -O2 -Wall -Wextra -Werror -DOCR_TEST_WRAP_SIDECAR -Isrc tests/test_ocr_manager.c src/ocr_manager.c src/ocr_hash.c src/ocr_identity.c src/ocr_segmenter.c src/ocr_rasterizer.c src/handwriting_index.c src/ocr_sidecar.c src/ocr_unicode_android.c -Wl,--wrap=ocr_sidecar_save_atomic -lpthread -lm -o build/test_ocr_manager_checkpoint.exe && ./build/test_ocr_manager_checkpoint.exe
"@

Invoke-WslTest 'canvas-ocr-integration' @"
cd '$WslRoot' && gcc -std=c11 -O2 -w test_canvas_ocr.c -lm -o build/test_canvas_ocr_final && ./build/test_canvas_ocr_final
"@

Invoke-WslTest 'canvas-persistence-regression' @"
cd '$WslRoot' && gcc -std=c11 -O2 -w tests/test_canvas_persistence.c -lm -o build/test_canvas_persistence_final && ./build/test_canvas_persistence_final
"@

Invoke-WslTest 'input-state-regression' @"
cd '$WslRoot' && gcc -std=c11 -O2 -w tests/test_input_regressions.c -lm -o build/test_input_regressions_final && ./build/test_input_regressions_final
"@

Invoke-WslTest 'erase-spatial-regression' @"
cd '$WslRoot' && gcc -std=c11 -O2 -w tests/test_erase_spatial.c -lm -o build/test_erase_spatial && ./build/test_erase_spatial
"@

Invoke-WslTest 'erase-damage-and-motion-regression' @"
cd '$WslRoot' && gcc -std=c11 -O3 -ffast-math -w -ffunction-sections -fdata-sections tests/test_erase_render.c -Wl,--gc-sections -lm -o build/test_erase_render && ./build/test_erase_render
"@

$LegacyCommand = @'
cd '{0}' && gcc -std=c11 -O2 -w test_v30.c -lm -o build/test_v30_final && run_dir=$(mktemp -d) && mkdir "$run_dir/testdata30" && cd "$run_dir" && '{0}/build/test_v30_final'
'@ -f $WslRoot
Invoke-WslTest 'legacy-v3.2.7-regression' $LegacyCommand

Invoke-WslTest 'product-polish' @"
cd '$WslRoot' && gcc -std=c11 -O2 -w tests/test_product_polish.c -lm -o build/test_product_polish && ./build/test_product_polish
"@

Invoke-WslTest 'incremental-live-stroke' @"
cd '$WslRoot' && gcc -std=c11 -O3 -ffast-math -w -ffunction-sections -fdata-sections tests/benchmark_live_stroke.c -Wl,--gc-sections -lm -o build/benchmark_live_stroke && ./build/benchmark_live_stroke
"@

Invoke-WslTest 'retained-ink-and-timestamped-input' @"
cd '$WslRoot' && gcc -std=c11 -O3 -w -ffunction-sections -fdata-sections tests/test_retained_ink.c -Wl,--gc-sections -lm -o build/test_retained_ink && ./build/test_retained_ink
"@

Write-Output 'suite=real-ppocr-fixtures'
Write-Output 'command=pwsh -NoProfile -File tests/run_ppocr_fixture_windows.ps1'
& pwsh -NoProfile -File (Join-Path $Root 'tests\run_ppocr_fixture_windows.ps1')
if ($LASTEXITCODE -ne 0) {
    throw "real PP-OCR fixture inference failed with exit code $LASTEXITCODE"
}

Write-Output 'FINAL HOST TEST MATRIX: PASS'
