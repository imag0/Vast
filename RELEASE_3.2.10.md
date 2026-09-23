# Vast 3.2.10 release and verification notes

Identity: `com.ayomi.infinitecanvas`, code 45, min SDK 26, target SDK 36, ARM64. The canvas remains C/OpenGL ES in Android NativeActivity. No Compose, Gradle, cloud service, network permission or replacement document engine was introduced.

## Changes

- Fixed the length-dependent live-stroke raster path. Previously each move cleared a full-screen coverage buffer, replayed the entire accumulating spline and uploaded the UI surface. Work therefore grew with stroke length, with quadratic cumulative drawing work. The new live texture retains finalized coverage, advances a four-point Catmull-Rom window and replaces only its provisional tail. Texture updates cover changed pixels. The marker retains its flat ribbon behavior. Completed ink and persistence still commit once on pen-up.
- All editable workflows now use an Android `EditText` inside a native dialog: text/sticky notes, project/frame/place names, search and numeric calibration. `TextWatcher` synchronizes document state; the actual visible control paints immediately. Android owns selection, clipboard, keyboard focus and scrolling. Obsolete canvas input chrome is suppressed while the native editor is active.
- Android Views now implement Settings, project/frame/place lists, search results, insertion, layers, photo gallery, selection More, tool panels and the header/rail. Short dialogs fit their content; long panels scroll; search results use bounded two-line labels; unavailable history and locked frame mutations are disabled.
- Notes wrap at a stable maximum width and grow/shrink with content. Android text shaping renders Chinese and color emoji. Notes allow 4095 UTF-8 bytes instead of 95. A bounded raster cache prevents repeated text layout during ink.
- Workspace VWS1 still begins with its original records and UTF-8-safe short previews. Optional VXT1 trailing records preserve long text. Existing documents load without migration. **Downgrading and resaving with an older app discards the long-text extension.**
- Settings uses short grouped labels. Handwriting Search explains: “Search text you've written by hand. Recognition runs entirely on this device and does not change your handwriting. Nothing is uploaded.” Ink is indexed for search, never replaced by recognized text; photos are not OCR input.
- Animation steps follow elapsed monotonic time. Custom color picker, quarter-circle pen/eraser control, radial gesture menu, spatial selection handles and minimap remain canvas-owned.

## Measured long-stroke behavior

Physical device: HONOR YLE-W09, Android 16/API 36, 3000 × 1920. Inputs below were shell-injected stylus MotionEvents, **not a physical pen**. A synchronous injection includes Android dispatch, app processing and scheduling; it is not digitizer-to-photon latency or a direct GPU frame measurement.

The untouched 3.2.9 baseline was built, update-installed and measured before the renderer changed.

| Points in active stroke | Old CPU clear + full raster (µs) | New CPU append + packing (µs) |
| --- | ---: | ---: |
| 20 | 1238.841 | 2.356 |
| 200 | 921.352 | 2.696 |
| 2,000 | 6205.193 | 3.126 |
| 20,000 | 60345.505 | 3.345 |

These are production CPU routines compiled for and executed on the tablet. The new benchmark averages 50 exact-prefix appends at each size and stubs GL calls to count uploads; it excludes driver/GPU time. Dense circular samples uploaded 56/72/81/81 bytes, respectively. The old benchmark includes the full-screen clear that the old implementation performed. These timings describe changed work, not equivalent standalone full-frame tests.

For a 1,200-event live replay, baseline synchronous injection bucket means rose from **10.507 ms at 20 events to 49.460 ms at 1,200**. The updated path measured **8.342 / 8.509 / 8.505 / 8.717 ms** at 20/200/600/1,200 events. The updated final bucket includes a 140.537 ms maximum around the heavier completion path. Pen-up work remains a bottleneck; this release does not claim constant-time finalization.

The final-build **20,000-event continuous replay completed in 334.484 seconds**, including intentional 8 ms inter-event sleeps. Bucket means at 20/200/600/2,000/20,000 events were 23.039/8.511/8.485/8.497/8.556 ms. The first bucket includes cold-start work; the final bucket includes a **1,059.168 ms maximum** on this extreme completion path. Vast resamples incoming events, so event count is not identical to stored point count. An earlier run was rejected by Android at event 5,296 and is excluded from completion claims; the app remained running.

SurfaceFlinger reported an 8.333333 ms display period during the successful run. Its 125 captured presentation intervals had median 16.655469 ms, p95 16.655573 ms and maximum 16.655626 ms. This replay intentionally supplied roughly 60 updates/second after synchronous dispatch and its sleep; these data do not prove 120 frames/second throughput or physical pen latency.

Project 4 PSS snapshots were 173,751 KB just after cold launch, 209,583 KB during the long stroke, 394,019 KB just after completion and 402,075 KB during the subsequent idle/UI walkthrough. These include warmed rendering and OCR allocations and are not an isolated leak test. The observed increase is reported, not described as a memory improvement. Cold launch on the final build was 721 ms (single sample); baseline launch was 984 ms (single sample, different loaded scene).

## Tests and reproducibility

Build from this source directory (JDK and Python with cryptography required):

```sh
python tools/build_android_views.py --android-jar "$ANDROID_JAR" --d8-jar "$D8_JAR"
./build-native.sh
python tools/build_v2.py
python tools/inspect_release.py
python tools/package_source.py
```

Use SDK platform 36 `android.jar` and the SDK build-tools `lib/d8.jar`. Clang must target aarch64-linux-android26. The direct native/custom APK build is preserved; the small DEX is only the Android View adapter. Keep the existing `build/v2-key.pem` and `build/v2-cert.der` privately to produce update-compatible APKs; the source archive intentionally contains no signing key.

```powershell
pwsh -NoProfile -File tests/run_final_host_tests.ps1
```

The full matrix passes 303 named checks (OCR core 99, canvas OCR 15, persistence 83, input 51, legacy 40, product polish 15), plus recognizer/manager suites, incremental-render invariants and four real ONNX fixture recognitions (English, Chinese and mixed text). The fixture runtime is ONNX Runtime 1.28.0 Windows x64, configured in `tests/run_ppocr_fixture_windows.ps1`; this runtime is a test dependency, not shipped inside the source ZIP.

Shell-only device helpers are in `tests/DeviceUiProbe.java`, `DeviceStrokeReplay.java` and `DeviceGestureReplay.java`. Compile with javac `-source 8 -target 8`, D8 with min-api 26, push DEX and run:

```sh
adb -s 192.168.1.14:40041 shell 'CLASSPATH=/data/local/tmp/vast-qa.dex app_process /system/bin DeviceStrokeReplay 20000'
adb -s 192.168.1.14:40041 shell 'CLASSPATH=/data/local/tmp/vast-qa.dex app_process /system/bin DeviceUiProbe long @editor'
```

Device walkthrough exercised live English/Chinese/multiline editing, selection, a 528-UTF16-unit paragraph with emoji, save/reopen/cancel restoration, note search, frame rename/lock, calibration, place rename, project rename cancellation, selection More lock/unlock, native Settings and photo-gallery import/deletion of a generated fixture. The QA workspace is Project 4; existing Project 1 is preserved. Evidence is retained locally under `artifacts/polish-pass/`, excluded from the distributable source ZIP.

A clean extraction of the source bundle rebuilt both `libcanvas.so` and `classes.dex` byte-for-byte identically to the release binaries. Independent APK inspection passed manifest, bundled asset hashes, ZIP integrity, APK v2 signature and baseline signer matching. Update-install succeeded without uninstalling or clearing data.

Actual Android OCR also ran on isolated synthetic vector lettering: Search returned `Handwriting - HELL`, and tapping it fitted/highlighted the original ink. The replay took several seconds to draw the final O, outside the existing 1.8-second temporal grouping window. After force-stop/relaunch, the geometry-only initial scan returned `Handwriting - HELLO`. This validates real device inference, search integration and jumping, but also exposes a limitation: slow lettering may be split into separate recognition groups until a fresh scan. Ink near large non-text doodles can also group poorly. Recognition quality is not guaranteed.

Focus mode was explicitly entered on the final build, then exercised with one-finger panning, a two-finger pinch, injected pen lettering and the edge exit handle. No minimap or full-screen native overlay intercepted that run.

The canvas picker was visually inspected and dragged through hue/saturation and value changes; its red preview stayed saturated and recent-color selection restored the original color. Project 1 reopened with the same visible ink and view as the baseline, including its Paper theme. Its final PSS was 273,265 KB versus the baseline's 343,675 KB, but process history/cache state differed, so this is not a controlled memory-reduction claim. Final process-filtered logcat showed no app fatal exception/ANR or VastUi error; OEM graphics/resource warnings and one Binder invalid-argument warning remained. The generated public photo-import fixture was removed after testing; QA objects remain in Project 4.

## Known acceptance limits

- No physical stylus pressure, tilt, hover, palm rejection or subjective pen-feel acceptance occurred. Shell input does not establish these.
- Unicode and selection were exercised through Android accessibility editing. Actual Chinese IME composition, external hardware keyboards and clipboard round-trip still need manual acceptance.
- Source animation timing is refresh-independent, but stable 90/120 Hz GPU frame pacing was not measured. Native View gfxinfo does not account for the entire C/EGL presentation path.
- Photo manipulation and gesture cancellation have host coverage; exhaustive device combinations, all OEMs, split-screen, portrait and accessibility screen-reader audits remain open.
- Pen-up still rasterizes the completed scene and saves. Very large completed documents may pause there. Allocation failure retains the original slower raster fallback.
- Memory snapshots from different projects are not comparable; no across-project “memory optimized” claim is made.

The changes are regression-tested and device-smoke-tested, not a claim that every possible public-release acceptance scenario has passed.
