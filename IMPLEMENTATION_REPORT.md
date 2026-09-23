# Vast 3.2.8 offline handwriting search — implementation report

> Historical note: this report records the 3.2.8/code 43 OCR implementation handoff. For current 3.2.10 build instructions, device results and acceptance limitations, see `README.md` and `RELEASE_3.2.10.md`.

Date: 2026-09-16  
Source root: `E:\_Dev\Code\Android\Vast-v3.2.7-source`  
Release identity: `com.ayomi.infinitecanvas`, version 3.2.8/code 43, minSdk 26, targetSdk 36, `arm64-v8a`

## Delivery status

The offline OCR implementation, bundled model/runtime, native integration and host validation are complete in the source tree. Real PP-OCRv5 inference executed through the production recognizer API on a Windows x64 host and all four required fixture cases retained their searchable terms.

This release is **not Android device-acceptance-verified**. Android Debug Bridge 37.0.1 found no attached device or emulator, so the shipped arm64 Android `libonnxruntime.so`, NativeActivity lifecycle, airplane-mode behavior, Huawei compatibility and the manual acceptance cases have not been executed on Android. No Android inference or device result is claimed.

## Baseline verified before implementation

The supplied directory is not a Git repository. Its 16 original files were compared byte-for-byte with the supplied source archive, whose SHA-256 is `a6a9704d663e04070f6713e8b460ad617f250b053e5bb6be66f08f30502c25e9`.

The audit established these real integration points:

- Native entry point and UI/canvas engine: `ANativeActivity_onCreate(...)` in `src/canvas.c`.
- Ink: points are `float x, y, p`; strokes own point arrays, color, base width, bounds and an active flag. There were no stable persisted 64-bit stroke IDs, persisted completion timestamps or per-stroke affine matrices.
- Canvas persistence: the existing ICV1 stream writes active strokes and raw point arrays. It was not modified for OCR.
- Undo/redo: `ACT_DRAW`, `ACT_ERASE` and `ACT_CLEAR` operate using stroke-array indices/active state.
- Erasing: the current eraser deactivates whole strokes; it does not cut persisted stroke geometry into partial fragments.
- Search: `search_collect(...)` and `search_jump(...)` already handled typed notes, frame names and saved places/bookmarks with six visible rows.
- Projects: four local slots use separate document paths; project switching saves the current project before loading another.
- Build: `build-native.sh` directly links the native application and `tools/build_v2.py` constructs/signs the APK. Gradle and CMake do not exist in this application.

The unchanged v3.2.7 native build and APK build succeeded. The unchanged regression test printed 39 `PASS` lines and no `FAIL`; the baseline APK was 173,264 bytes and its SHA-256 was `80a6633c35037b384f1eeb6273fab44c66397b163737ab1a0b78d44f10077802`.

Baseline evidence:

- `artifacts/baseline/00-source-state.log`
- `artifacts/baseline/02-build-native.log`
- `artifacts/baseline/04-build-apk.log`
- `artifacts/baseline/05-regression-test.log`
- `artifacts/baseline/06-artifact-inspection.log`
- `artifacts/baseline/Vast-v3.2.7-baseline.apk`

## Implemented scope

The implementation is limited to the requested flow:

```text
Vast vector handwriting
        -> world-space line segmentation
        -> offscreen vector rasterization
        -> PP-OCRv5 recognition on one background worker
        -> hidden spatial text index
        -> existing Vast Search
```

It does not convert ink into visible text and does not add text detection, photo/PDF/screen OCR, semantic search, embeddings, summarization, cloud OCR or model download behavior.

### Source identity without changing the canvas format

`src/ocr_identity.c` hashes normalized relative stroke geometry: quantized segment vectors, point count and pressure profile. Absolute translation, ink color, theme, viewport and zoom are excluded. Region identity includes per-source shape hashes and quantized local placement, while a sidecar occurrence ID keeps identical duplicates spatially independent.

Runtime stroke indices remain runtime associations only and are deliberately not serialized as stable IDs. Existing canvas files remain authoritative and backward compatible.

### Segmentation and rasterization

`src/ocr_segmenter.c` groups active stroke snapshots in document/world coordinates using horizontal and vertical gaps, baseline proximity, relative height and temporal proximity when a runtime completion time is known. Existing persisted strokes have no timestamps and use the geometry-only fallback. Central defaults in `ocr_config_default(...)` include an 850 ms writing-idle delay and 1,800 ms temporal gap.

`src/ocr_rasterizer.c` renders original vector paths as antialiased dark ink on white, independent of viewport, theme, density, absolute position and ink color. It preserves meaningful relative pressure/brush width, uses a 48-pixel raster height with padding, and caps a single raster at 1600 pixels. `src/ocr_manager.c` splits over-wide lines into deterministic world-space chunks with 20% overlap before recognition.

### Recognition runtime

`src/ppocr_recognizer.c` copies the model/dictionary bytes supplied from Android assets, then lazily resolves `OrtGetApiBase` from `libonnxruntime.so`. It creates one reusable session on the OCR worker, not on first pen-down, and releases it at shutdown.

Session settings are conservative: sequential execution, one intra-op thread, one inter-op thread, basic graph optimization, memory pattern disabled, CPU arena disabled, error-only runtime logging and telemetry explicitly disabled.

The model's actual names and shapes are inspected through ONNX Runtime. The verified preprocessing/decoding contract is:

- input `x`: float32 `[N,3,48,W]`, BGR/NCHW, dynamic width;
- preserve aspect ratio at height 48, minimum tensor width 320, maximum 3200;
- bilinear resize, uint8-equivalent rounding, normalize with `pixel / 127.5 - 1`;
- normalized-zero right padding;
- output `fetch_name_0`: float32 `[N,T,18385]` Softmax probabilities;
- CTC blank class 0, raw repeat suppression followed by blank removal;
- dictionary class mapping with the final ASCII-space token;
- confidence is the mean maximum probability of retained output symbols.

### Worker and lifetime safety

`src/ocr_manager.c` owns one condition-variable-driven low-priority worker (`nice` 10 on Android), an eventfd integrated into Vast's looper and at most 32 coalesced dirty regions. It performs no frame polling and takes no wake lock. Stylus contact and high-frequency canvas interaction defer OCR until input settles.

Every manager API validates and deep-copies stroke geometry before returning. The worker never retains a pointer into Vast's mutable/reallocatable `Stroke` array. Document/job generations discard stale results. Initial indexing walks old projects in current-state batches of 32 while the canvas remains available. Mutation APIs remain active during the scan: already-seen runtime indices update authoritatively in place, not-yet-seen strokes are captured by their later fresh batch, and atomic duplicates are authoritative immediately. This avoids both stale commits and a full O(N) restart.

### Incremental index behavior

- Stroke completion queues only the affected local neighborhood.
- Whole-stroke erase/delete/clear and undo/redo update active state and reconcile local records.
- An intact translation updates cached bounds without inference; otherwise local reconciliation is queued.
- Duplication registers destination geometry and clones complete source-region metadata atomically when safe, avoiding unnecessary inference and source invalidation.
- Recolor, zoom, pan and theme changes never invalidate OCR.
- Project switching uses the matching sidecar; project duplication clones reusable cache data.
- A central 0.40 search-confidence threshold hides low-confidence garbage while retaining those records in the cache.

### Unicode and Search integration

Original OCR UTF-8 and normalized searchable UTF-8 are stored separately. On Android, `src/ocr_unicode_android.c` calls platform `java.text.Normalizer` with NFKC, then `String.toLowerCase(Locale.ROOT)`, then collapses Unicode whitespace. This provides Chinese substring matching and Unicode-aware Latin case normalization without a new framework or online service.

`SEL_HANDWRITING` extends the real `search_collect(...)`/`search_jump(...)` path in `src/canvas.c`. Search reserves capacity so neither handwriting nor existing note/frame/bookmark providers can completely starve the other. The visible six-row result snapshot is stable between paint and tap. Selecting a live handwriting handle fits its current world bounds and starts a fading cyan renderer-owned highlight; it does not recolor ink or open an editor. Android `TextView` overlays render Unicode search/result labels while hit testing stays in Vast's existing native UI path.

### Settings and lifecycle

The fifth Settings tab exposes **Offline handwriting search**, current manager state/counts and **Rebuild handwriting index**. The preference defaults on and is stored separately as `ocrprefs.v1`. Rebuild clears derived index/cache state only, leaves all strokes intact and schedules background recognition.

Pause/input-queue loss releases interaction deferral. Project switches can continue while an already deep-copied asynchronous checkpoint snapshot is written; explicit durable shutdown waits and performs a final checkpoint before freeing document strokes or the recognizer.

## Sidecar/cache format

The canvas remains the only source of truth. App-private sidecars are `canvas.vastocr` for Project 1 and `canvas_1.vastocr` through `canvas_3.vastocr` for Projects 2-4.

The independent sidecar format uses:

- magic `VASTOCR\0`;
- sidecar format and OCR index versions;
- exact 32-byte model fingerprint;
- project key;
- bounded record count/payload size;
- CRC-32 for header and payload;
- finite, ordered world bounds and confidence range checks;
- per-record source-count and UTF-8 length limits;
- original and normalized strings plus confidence and source geometry identity.

The reader caps the file at 64 MiB, records at 100,000, sources per record at 4,096 and either text field at 16,384 bytes. It rejects truncation, trailing data, invalid UTF-8, NaN/Inf, overflow, version/model/project mismatch and checksum errors before replacing the in-memory index. A failure never blocks canvas loading; the invalid cache is ignored and rebuilt asynchronously.

Checkpoints write a temporary file, flush and `fsync`/`_commit`, close it, then atomically rename/replace it.

## Model and runtime provenance

### PP-OCRv5 mobile recognition

- Model ID: `PP-OCRv5_mobile_rec`
- Official documentation: <https://www.paddleocr.ai/latest/en/version3.x/module_usage/text_recognition.html>
- Official archive: <https://paddle-model-ecology.bj.bcebos.com/paddlex/official_inference_model/paddle3.0.0/PP-OCRv5_mobile_rec_infer.tar>
- Archive size: 16,834,560 bytes
- Archive SHA-256: `566b9512b34e34a9f0db54d87b51fa5a0b9ed2cf1ab7e49728cc0b8b5a64f414`
- Conversion: PaddlePaddle 3.1.1 plus Paddle2ONNX 2.1.0, ONNX opset 11
- Determinism: two clean conversions were byte-identical
- Shipped `inference.onnx`: 16,537,720 bytes
- ONNX SHA-256: `dc7de8ee31d9246783cf346f3b207b4cb871e11824b1cfab2ff20aa1f1f8e67b`
- `inference.yml`: 148,345 bytes; SHA-256 `5dfeb2777f6d0db8177d8128a8acfcf6e6276dc4ac73ea3bf0dc06d6a5e85d8e`
- Dictionary: 18,383 lines, 74,012 bytes; SHA-256 `d1979e9f794c464c0d2e0b70a7fe14dd978e9dc644c0e71f14158cdf8342af1b`

The exact conversion command and metadata are recorded in `assets/ocr/ppocrv5_mobile_rec/model-metadata.json` and `THIRD_PARTY_NOTICES.md`.

### ONNX Runtime for Android

- Version: 1.28.0
- Official Maven AAR: <https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.28.0/onnxruntime-android-1.28.0.aar>
- AAR size: 45,634,470 bytes; SHA-256 `f351a0638696f54b35184290dbc001d66daae17281ad0b548d2c70347d53b8a9`
- Shipped library: `third_party/onnxruntime/lib/arm64-v8a/libonnxruntime.so`
- Shipped library size: 28,637,280 bytes
- Shipped library SHA-256: `f826d8efb03adf0a84f10e7ba408f9d4cd11b0a2ccd8d08aeb0f7451fb50cacc`

Only the `arm64-v8a` C runtime library and C/C++ headers are included. No Java wrapper and no `armeabi-v7a`, `x86` or `x86_64` Android binary is packaged. PaddleOCR, Paddle2ONNX and ONNX Runtime licenses/notices are preserved in the source and packaged APK.

## Privacy and network review

The custom manifest continues to request only the pre-existing media read permissions. It does not declare `INTERNET` or `ACCESS_NETWORK_STATE`. The OCR implementation has no HTTP client, socket path, model downloader, remote configuration, Google Play Services, ML Kit, Firebase, Huawei cloud API, analytics or recognized-text logging path. Model, dictionary, configuration, runtime and licenses are APK entries.

This is a static source/package property, not a claim that airplane mode was exercised on a device. That remains an explicit manual gate.

## Files changed

Modified original files:

- `src/canvas.c` — OCR lifecycle/event hooks, Search result type/navigation/highlight, mutation hooks, Settings and Unicode Android View labels.
- `build-native.sh` — OCR modules, ORT headers, `libdl` dependency and 16 KiB maximum page alignment.
- `tools/build_v2.py` — 3.2.8/code 43, pinned OCR/ORT assets and license packaging while retaining the custom v2 signer.
- `stubs/android_stub.c`, `stubs/libc_stub.c`, `stubs/libm_stub.c` — declarations needed by the preserved no-SDK direct native link.
- `README.md` — release architecture, build/test guidance and pending Android acceptance checklist.

Added implementation/test resources:

- `src/ocr_core.h`
- `src/ocr_hash.c`
- `src/ocr_identity.c`
- `src/ocr_segmenter.c`
- `src/ocr_rasterizer.c`
- `src/handwriting_index.c`
- `src/ocr_sidecar.c`
- `src/ocr_unicode_android.h`, `src/ocr_unicode_android.c`
- `src/ppocr_recognizer.h`, `src/ppocr_recognizer.c`
- `src/ocr_manager.h`, `src/ocr_manager.c`
- `src/compat/` portability headers and `stubs/libdl_stub.c`
- `assets/ocr/ppocrv5_mobile_rec/` model, dictionary, config and provenance metadata
- `third_party/onnxruntime/`, `third_party/paddleocr/`, `third_party/paddle2onnx/`
- `THIRD_PARTY_NOTICES.md`
- `test_ocr.c`, `test_ppocr_recognizer.c`, `test_canvas_ocr.c`
- `tests/test_ocr_manager.c`, `tests/test_ppocr_fixture_inference.c`, `tests/host_win32_compat.c`, `tests/run_ppocr_fixture_windows.ps1`
- `tests/run_final_host_tests.ps1` — reproducible one-command final host matrix
- `tests/fixtures/ocr/` handwritten-looking English, Simplified Chinese and mixed fixtures plus reference terms
- `tools/generate_ocr_fixtures.py`
- `tools/inspect_release.py` — standalone final APK manifest/content/hash/v2-signature inspector
- `tools/package_source.py` — deterministic source archive with embedded hashes and explicit signing-secret exclusion
- this implementation report

Unrelated renderer/font/JNI headers and the original `test_v30.c` were not modified.

## Executed validation and actual results

All commands were run from the source root unless stated otherwise. The authoritative captured output should be used instead of paraphrased success claims.

| Validation | Actual result | Evidence |
| --- | --- | --- |
| Original v3.2.7 native/APK baseline | Build succeeded | `artifacts/baseline/02-build-native.log`, `04-build-apk.log` |
| Original v3.2.7 regression | 39 PASS, 0 FAIL | `artifacts/baseline/05-regression-test.log` |
| Final AArch64 native build | Passed with `-z defs`; `libcanvas.so` produced | `artifacts/final/01-native-build.log` |
| OCR core/identity/segmentation/raster/index/sidecar tests | 99 checks passed | `artifacts/final/02-final-host-tests.log` |
| PP-OCR preprocessing/dictionary/CTC pure test | Passed, 0 reported failures | `artifacts/final/02-final-host-tests.log` |
| OCR manager lifecycle/invalidation/cache/concurrency tests | Main and checkpoint scenarios PASS | `artifacts/final/02-final-host-tests.log` |
| Canvas-to-OCR integration test | 15 PASS, 0 FAIL; `Canvas OCR integration tests passed` | `artifacts/final/02-final-host-tests.log` |
| Final existing `test_v30` regression | 39 PASS, 0 FAIL | `artifacts/final/02-final-host-tests.log` |
| Real bundled-model fixture inference, Windows x64 ORT 1.28.0 | 4/4 fixture term sets PASS | `artifacts/final/02-final-host-tests.log`, `artifacts/ocr/ppocr-host-fixture-2026-09-16.log` |
| Final ELF alignment/network-import audit | Passed | `artifacts/final/03-elf-network-audit.log` |
| Final APK build and v2 self-verification | Passed | `artifacts/final/04-apk-build-and-sign.log` |
| Final APK contents/manifest/signature audit | Passed | `artifacts/final/05-apk-inspection.log` |
| Android execution-target check | No device/emulator attached | `artifacts/final/06-device-availability.log` |
| GCC static analysis of OCR manager | `-fanalyzer -Werror` passed | `artifacts/final/07-static-analysis.log` |

The real fixture run printed:

```text
case=zh_engine recognized="发动机" confidence=0.999819934 raster=230x85 input=1x3x48x320 inference=57319us terms=PASS
case=zh_school recognized="明天去学校" confidence=0.999835432 raster=377x86 input=1x3x48x320 inference=65258us terms=PASS
case=en_terms recognized="engine Pressure navigation" confidence=0.951888025 raster=778x80 input=1x3x48x466 inference=111838us terms=PASS
case=mixed recognized="Engine发动机" confidence=0.991876900 raster=426x85 input=1x3x48x320 inference=56774us terms=PASS
session_ready_after=1 fixture_failures=0
```

The first call also measured 191,532 us of one-time session setup and 250,356 us total. Subsequent totals were 66,776 us, 114,177 us and 58,304 us. These are host x64 measurements, not Android performance figures.

The focused suites cover Unicode normalization/fallback and Chinese substring/Latin case-insensitive search; geometry fingerprint invariants and duplicate occurrences; segmentation translation/viewport invariance; stable vector rasterization; confidence filtering; sidecar round trips, missing/truncated/corrupt/hostile lengths, model/index/project mismatch and atomic replacement; new/delete/move/duplicate/undo/redo/project behavior; queue deferral/coalescing and stale-generation discard; existing-provider Search merging; result navigation/highlight; and no OCR invalidation for recolor/zoom/pan/theme changes.

The manager source also passed GCC `-fanalyzer -Werror`, including the former duplicate-region reallocation path, as captured in `artifacts/final/07-static-analysis.log`; the AArch64 link exported the expected manager API with no unresolved symbols. AddressSanitizer was not available in the installed MinGW toolchain because `libasan` is absent; that unavailable run is not claimed as a pass. The focused 32-region reallocation regression, analyzer and ordinary host suites provide the available coverage for that path.

## Build and test commands

Production build:

```bash
./build-native.sh
python3 tools/build_v2.py
python3 tools/package_source.py
```

The source packager writes `dist/Vast-v3.2.8-source.zip`, embeds `SOURCE_ARCHIVE_MANIFEST.sha256`, verifies ZIP integrity and unique names, and excludes `build/`, `dist/`, test-data directories, executables and private-key/keystore formats.

Complete host matrix on the validated Windows/WSL setup:

```powershell
pwsh -NoProfile -File .\tests\run_final_host_tests.ps1
```

Core tests:

```bash
gcc -std=c11 -O2 -Wall -Wextra -Werror \
  test_ocr.c src/ocr_hash.c src/ocr_identity.c src/ocr_segmenter.c \
  src/ocr_rasterizer.c src/handwriting_index.c src/ocr_sidecar.c \
  -lm -o build/test_ocr && ./build/test_ocr
```

Recognizer contract test:

```bash
gcc -std=c11 -O2 -Wall -Wextra -Werror \
  test_ppocr_recognizer.c -Isrc -Ithird_party/onnxruntime/include \
  -ldl -lm -o build/test_ppocr_recognizer && ./build/test_ppocr_recognizer
```

Manager test:

```bash
gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc \
  tests/test_ocr_manager.c src/ocr_manager.c src/ocr_hash.c \
  src/ocr_identity.c src/ocr_segmenter.c src/ocr_rasterizer.c \
  src/handwriting_index.c src/ocr_sidecar.c src/ocr_unicode_android.c \
  -lpthread -lm -o build/test_ocr_manager && ./build/test_ocr_manager
```

Canvas integration and existing regression:

```bash
gcc -std=c11 -O2 -w test_canvas_ocr.c -lm -o build/test_canvas_ocr
./build/test_canvas_ocr

gcc -std=c11 -O2 -w test_v30.c -lm -o build/test_v30
./build/test_v30
```

Real fixture inference:

```powershell
pwsh -NoProfile -File .\tests\run_ppocr_fixture_windows.ps1 `
  -OrtRoot C:\path\to\onnxruntime-win-x64-1.28.0
```

Final APK inspection:

```powershell
python .\tools\inspect_release.py
```

## APK size report

| Artifact/input | Exact size |
| --- | ---: |
| Original signed Vast 3.2.7 APK | 173,264 bytes |
| Converted `inference.onnx` | 16,537,720 bytes |
| Android arm64 `libonnxruntime.so` | 28,637,280 bytes |
| Character dictionary | 74,012 bytes |
| Inference YAML | 148,345 bytes |
| Model metadata JSON | 2,149 bytes |
| Core bundled OCR inputs above, uncompressed | 45,399,506 bytes |
| Final `libcanvas.so` | 430,392 bytes |
| Final signed Vast 3.2.8 APK | 24,991,909 bytes |
| Signed APK increase over v3.2.7 | 24,818,645 bytes |

All values are measured, not estimates. ZIP compression accounts for the final APK being smaller than the sum of its uncompressed model/runtime inputs.

Final artifact hashes:

- `build/libcanvas.so`: `54f86ecee0c2d72750b2ab45cb4b283d657da39fb1e6d1c515ada3f23609f1fd`
- `dist/Vast-v3.2.8.apk`: `9749b439b3d86f4fe5d86e1a1567ed708b1255945bdb6af675f8a896bcb4ba63`

## Final APK inspection

The frozen signed APK passed the repository's final inspection script:

- 12 unique ZIP entries; the ZIP integrity test passed.
- Manifest package `com.ayomi.infinitecanvas`, version 3.2.8/code 43, minSdk 26 and targetSdk 36 all matched.
- Exactly one ABI, `arm64-v8a`, is present with both `libcanvas.so` and `libonnxruntime.so`.
- The ONNX model, inference YAML, dictionary, metadata and required third-party license/notices are physically present.
- Both native libraries have 16 KiB-compatible `PT_LOAD` alignment (`0x4000`).
- Network-import counts are zero for both native libraries, and the manifest contains neither `INTERNET` nor `ACCESS_NETWORK_STATE`.
- APK Signature Scheme v2 signer signature and content digest verification passed.
- The persisted signing certificate SHA-256 is `dad8379ff3a9bbef56f5b322cb4681db0a95c738afc532957ef7972446fb7eaf`, matching the baseline signing identity.

See `artifacts/final/03-elf-network-audit.log`, `04-apk-build-and-sign.log` and `05-apk-inspection.log` for the actual command output.

## Android acceptance blocker and remaining limitations

- `adb devices` returned no device/emulator. No Android arm64 inference, installation, cold start, lifecycle stress, airplane-mode run, stylus-latency observation or update-install was possible.
- No Huawei tablet was present, so mainland-China Huawei compatibility is an architectural target, not a tested-device claim.
- The Windows fixture proves the bundled ONNX model, preprocessing and decoder through the production C recognizer API, but uses an official host x64 ONNX Runtime DLL rather than the packaged Android arm64 library.
- Recognition accuracy still depends on writing style. Search hides results below confidence 0.40; it does not visually convert or correct handwriting.
- Recognition-only intentionally relies on Vast's vector-line segmentation; no text detector is shipped.
- Vast remains arm64-only, matching the existing application target.

The release cannot be called fully verified until the eight manual cases in `README.md` and real inference through the shipped Android libraries have passed on an arm64 Android target.
