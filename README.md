# Vast 3.2.18

3.2.18 adds a confirmed **Clear project** action and a separate **Erase everything** stylus tool. Clear project removes every document object from the current project while preserving the project slot and its name. Erase everything removes unlocked ink, highlighter, photos, dimensions, text, shapes, and frame borders along the stylus path as one undoable gesture; finger input remains navigation-only. See [RELEASE_3.2.18.md](RELEASE_3.2.18.md).

3.2.17 keeps 8K photo work responsive by decoding and uploading one tile per frame, reusing one Android decoder session, and persisting ordinary photo transforms in a small metadata sidecar instead of rewriting the full pixel file. Hidden chrome now returns only after a completed empty-canvas tap, with the side rail compact; panning never reveals it. Native Android chrome slides in and out from its anchored edge. See [RELEASE_3.2.17.md](RELEASE_3.2.17.md).

3.2.15 matched the top navigation pill height to the compact side rail width: both use one shared 60 dp dimension. Icons and spacing fit inside 48 dp native button targets; labels remain visible. The tool tray retains its 16 dp gap below the shorter header.

Vast 3.2.14 introduces a shared translucent design system for real Android controls: unified navigation and selection pills, an icon rail with expandable labels, coordinated panels and editors, and curved radial petals. Native input behavior, retained drawing, controlled fling and fully local handwriting search remain. See [RELEASE_3.2.14.md](RELEASE_3.2.14.md) and [UI_REVIEW_3.2.14.md](UI_REVIEW_3.2.14.md) for implementation and screenshot review; [RELEASE_3.2.13.md](RELEASE_3.2.13.md) records inertial navigation.

Package: `com.ayomi.infinitecanvas`  
Version code: 53
Target SDK: 36  
Minimum SDK: 26  
ABI: arm64-v8a

## 3.2.14 translucent native UI

- Shared tinted gradient surfaces, fine highlights, restrained elevation and selected/pressed/focused states; no realtime blur or backdrop capture.
- Native Buttons, EditText, Switches, scrolling, dialogs and unit selection retain Android behavior with custom styling.
- A single navigation pill, an icon-only/expanded-label side rail, matching tool and context groups, and bright active radial petals.
- Dark and Paper palettes preserve readable text; native inputs explicitly style text, hints, cursor and selection.
- Device screenshot review covers the requested surfaces and busy ink/text backgrounds. Host input and retained-rendering checks pass; this is not a measured frame-rate guarantee.

## 3.2.13 inertial navigation (historical)

- Quick one- or two-finger camera pans continue along the measured release vector, while slow releases stop directly.
- Velocity is capped at 3200 px/s and reduced with constant 6400 px/s² elapsed-time deceleration. The exact stopping integration is consistent across 60/90/120 Hz; even an extreme release coasts at most 800 px over 0.5 seconds.
- Any new finger, pinch, stylus contact, cancellation, or lifecycle/input teardown stops momentum before active input is processed.
- Fling updates only camera translation and frame requests; document mutations, saves, OCR work, minimap rebuilds, and retained ink/text/image geometry stay deferred.
- The radial-menu progress ring now remains hidden for a 320 ms finger-pan grace period before filling; the configured hold duration is unchanged.

## 3.2.12 drawing and navigation (historical)

- Chronological historical samples, bounded velocity/timing-aware filtering and a causal local curve; no future-sample wait or pen-up reshaping.
- Fixed-size retained world-space ink chunks and cached text/image textures. GLES 3 pan/zoom changes camera uniforms instead of repainting a document bitmap. Existing GLES 2/software fallback remains.
- Ink and camera saves wait for a short idle period; lifecycle/project transitions still save. A process crash before that save can lose the latest unsaved ink or camera position.
- The bottom-right color quarter-circle mirrors Pen/Eraser and opens a native hue/saturation wheel, brightness slider and recent colors with interruptible 190/140 ms motion.
- Warm, subtly translucent native surfaces; stable project identity replaces flashing canvas branding. Inputs remain Android EditText with cursor, selection, IME and scrolling.
- Host suite and synthetic real-device checks pass. The user confirmed smooth physical writing with no noticeable jiggle or growing delay; synthetic injection is not physical pen-to-display latency.

## 3.2.11 eraser and menu polish (historical)

- Segment-chunk spatial queries replace whole-document eraser scans. Deleted ink repaints only its damaged area; moving the reticle over empty space does not upload the scene again.
- Finger navigation cannot start erasing. Reticle visibility follows the owning stylus pointer's contact, with explicit hover, release, cancellation and multi-pointer handling.
- Settings uses persistent category navigation, Layers uses compact visibility/lock rows, and contextual menus separate relevant properties and irreversible actions. Text entry remains Android EditText; no Compose migration.
- Native property animations and canvas transitions use elapsed time. A per-window high-refresh preference requests up to 120 Hz without changing the tablet's global refresh settings.
- Physical-tablet replay and screenshot checks passed for the covered paths, and the user confirmed correct, responsive physical-stylus erasing and finger navigation. Zero-jank 120 Hz presentation is not claimed; see the release report.

## 3.2.10 product polish (historical)

- Incremental live ink uses a fixed Catmull-Rom window and dirty-region texture uploads. Completed geometry is not rebuilt on every sample; pen-up still performs the final document commit.
- Android dialogs, EditText, TextWatcher, buttons, switches, scrolling and unit selection replace canvas-rendered editable fields and ordinary panels. Settings explains local handwriting search without implementation jargon.
- Notes support up to 4095 UTF-8 bytes, Unicode shaping, emoji, wrapping and content-driven width/height. The legacy workspace prefix is preserved; an optional VXT1 extension stores full text. Older versions can read the short preview but discard extended text if they resave it. Do not downgrade and resave long notes.
- Native controls use consistent 48dp targets, readable disabled states, restrained surfaces and short labels. Spatial color/gesture controls remain canvas-owned.
- Animations use elapsed monotonic time, not an assumed refresh rate. This is not a claim of measured 120 Hz presentation.

## 3.2.9 quality and reliability pass (historical)

- Corrects the locally declared `ANativeActivity` layout so Android's real `AAssetManager` is read from the proper ABI field and the bundled offline OCR runtime can initialize on device.
- Makes canvas, workspace and metadata readers reject or safely drop incomplete records instead of exposing partially read state; project switches also clear stale selections, transient gestures and layer state before loading the next document.
- Makes **Cancel** discard a newly created text note while preserving edits to pre-existing notes. Persistence coverage includes canceling a new note beside an existing note and verifying the saved/reloaded result.
- Stops continuously redrawing the native surface while the Android text editor is idle. Text polling now requests a frame only when content changes, while pause still synchronizes the final edit.
- Broadens GPU cache keys for selection, theme, CAD and camera-dependent UI state, and avoids rebuilding the minimap cache while the viewport remains inside its cached world bounds.
- Narrows several input-release save paths to the document components that changed, persists display/performance/CAD-unit settings, and handles dynamic allocation failures without overwriting the only live pointer.
- Replaces unsupported symbols in the native ASCII renderer with readable ASCII labels; Android View text remains Unicode-capable.
- Commits UI controls on release, tracks stable Android pointer IDs, and rolls back camera, selection, frame and photo mutations on `ACTION_CANCEL`, including one-finger-to-pinch transitions.
- Enforces class and per-object locks across clear, undo/redo, automatic shape conversion and selection actions; compacted CAD/frame arrays now remap or discard group references safely.
- Uses strict UTF-8 validation and explicit UTF-8/UTF-16 conversion for Android text editing, including supplementary Unicode characters.
- Atomically replaces documents after file `fsync`, then attempts a parent-directory durability barrier with an arm64 `syncfs` fallback. A completed rename remains a successful save on filesystems that do not expose that barrier; multi-file project duplication still requires the barrier and uses a recovery marker so a crash cannot expose a partial copy.

The package ID, minimum/target SDKs, ARM64-only native architecture and offline boundary remain unchanged. The canvas and OCR sidecar formats are unchanged; metadata written by 3.2.9 uses the new `ICM8` header while retaining readers for older metadata revisions.

## 3.2.8 offline handwriting search

- Bundles the official `PP-OCRv5_mobile_rec` recognition model, its verified preprocessing configuration and 18,383-entry character dictionary.
- Runs the converted ONNX model through ONNX Runtime for Android 1.28.0. Both the model and `libonnxruntime.so` are packaged in the APK; nothing is downloaded after installation.
- Segments Vast's original vector strokes in world coordinates and rasterizes only likely handwriting regions. It never screenshots the viewport, reads the OpenGL framebuffer, or OCRs UI, grid, photos, or backgrounds.
- Uses one low-priority, event-driven OCR worker. Geometry is deep-copied before the canvas call returns, recognition waits for writing to settle, and active stylus/pan/pinch interaction defers work.
- Extends the existing `search_collect(...)` and `search_jump(...)` path. Typed notes, frame names and saved places remain searchable; handwriting results use the same panel, fit their world bounds, and show a short non-destructive highlight.
- Mirrors creation, erase/delete, clear, undo/redo, movement, duplication and project changes into the index. Translation reuses recognition when geometry is unchanged; recolor, zoom, pan and theme changes do not invalidate OCR.
- Adds a fifth Settings tab for enabling offline handwriting search, reading its status and rebuilding OCR-derived data without touching canvas ink.
- Keeps project documents unchanged. OCR data is a disposable, versioned sidecar cache.

Historical release notes below describe the behavior of their named versions; statements that handwriting OCR was excluded apply to those older releases, not 3.2.8 or later.

## Using offline handwriting search

Open Settings and enable Offline handwriting search under Handwriting Search. Vast waits 850 ms after the current touch or stylus interaction ends, then groups nearby vector strokes into likely text regions and recognizes them on a low-priority background worker. The visible ink is never replaced by OCR text.

The normal Search panel combines typed notes, frame names, saved places and recognized handwriting. Handwriting matching uses normalized, case-insensitive substring search. Tap a handwriting result to fit and briefly highlight its original ink. While Settings reports Indexing or Rebuilding, handwriting results can be incomplete.

OCR reads Vast vector ink only. It does not read imported photos, typed text, the grid, backgrounds or application UI, and handwriting recognition can make mistakes. Low-confidence recognition is omitted from normal Search.

The model, dictionary and runtime are packaged in the APK. Ink geometry, rasterized regions, recognized text and search queries remain in the local process and app-private cache. The only network use is the version-manifest/APK updater; OCR never uses that permission and does not upload OCR data.

Rebuild handwriting index deletes only the disposable OCR cache for the current project and scans that project again. It does not delete or alter strokes, notes, photos, frames or places. Disabling OCR hides handwriting results and stops new recognition without changing canvas content.

## OCR architecture

| Module | Responsibility |
| --- | --- |
| `src/ocr_manager.c` | Owns the project lifecycle, one worker, bounded/coalesced dirty regions, immutable geometry snapshots, cache reuse, timing and event notifications. |
| `src/ocr_segmenter.c` | Groups eligible strokes into likely text lines using world-space bounds, relative height/baseline, spatial gaps and runtime completion times when available. |
| `src/ocr_identity.c` | Produces translation-, color-, theme- and viewport-invariant shape fingerprints without changing Vast's canvas format or claiming persistent stroke IDs. |
| `src/ocr_rasterizer.c` | Renders antialiased black ink on white, preserving relative stroke width at a stable 48-pixel recognition height. |
| `src/ppocr_recognizer.c` | Lazily creates and reuses one ONNX Runtime session, applies the verified PP-OCR preprocessing contract and performs CTC decoding. |
| `src/handwriting_index.c` | Stores original UTF-8, normalized searchable text, confidence, source associations and world bounds. |
| `src/ocr_unicode_android.c` | Uses Android `Normalizer` NFKC, `Locale.ROOT` lowercasing and collapsed Unicode whitespace for search. |
| `src/ocr_sidecar.c` | Loads and atomically checkpoints the untrusted, versioned `.vastocr` cache. |

The recognizer input is float32 `NCHW`, one BGR image at height 48 and dynamic width 320-3200. Pixels are normalized as `pixel / 127.5 - 1`; right padding is normalized zero. Input/output names and concrete output shape are read through ONNX Runtime. Decoding uses CTC blank class 0, raw-sequence repeat removal, dictionary mapping and the mean probability of retained symbols.

Very wide lines are split into deterministic world-space chunks with 20% overlap rather than being crushed into a narrow image. The vector rasterizer itself is capped at 1600 pixels; the model wrapper enforces the verified 3200-pixel maximum.

## Source identity and mutation behavior

The pre-existing `Stroke` array has runtime indices but no stable persisted 64-bit stroke identifier. Vast 3.2.8 therefore leaves `canvas.icv` compatible and associates OCR records by deterministic, normalized stroke-geometry fingerprints plus quantized positions within the region. Independent occurrence IDs distinguish duplicate, identical handwriting records inside the sidecar.

- New or shape-edited strokes dirty only their local handwriting neighborhood.
- Whole-stroke erasing/deletion deactivates stale records and reconciles the affected area.
- A pure translation updates cached world bounds when the complete recognized region moved intact.
- Duplication atomically registers copied geometry and clones complete reusable metadata when possible.
- Recolor, viewport movement, zoom and theme changes do nothing to the OCR index.
- The initial scan reads existing strokes in current-state batches of 32. Mutation APIs remain live during the scan: already-seen strokes update in place, not-yet-seen strokes are captured by a later fresh batch, and atomic duplicates are authoritative immediately. No full-document rescan is required.

The default recognition idle delay is 850 ms and the confidence inclusion threshold is 0.40. Low-confidence text remains cached for future threshold tuning but is not returned by normal Search.

## Sidecars and recovery

Project OCR caches use these app-private paths:

- Project 1: `canvas.vastocr`
- Projects 2-4: `canvas_1.vastocr`, `canvas_2.vastocr`, `canvas_3.vastocr`

The `VASTOCR\0` sidecar header carries independent format/index versions, the exact model SHA-256, project key, bounded record/payload counts, and CRC-32 checksums. Records retain original and normalized UTF-8, confidence, source fingerprints and world-space bounds. Saves use a temporary file, flush plus `fsync`/`_commit`, then atomic replacement.

Missing, truncated, corrupt, wrong-project, wrong-index or wrong-model sidecars never block canvas loading. They are rejected as cache data and current strokes are indexed again in the background. **Rebuild handwriting index** deletes only derived OCR metadata and schedules that same background rebuild.

## Offline and privacy boundary

- The generated manifest grants `INTERNET` only to fetch the public update manifest/APK. It has no `ACCESS_NETWORK_STATE`, analytics, cloud OCR, account, or advertising dependency.
- There is no Google Play Services, ML Kit, Firebase, Huawei cloud API, model downloader, remote configuration, analytics or OCR telemetry integration.
- ONNX Runtime telemetry is explicitly disabled before the session is created.
- Strokes, rasters, recognized strings and search queries stay in the local process and app-private cache.
- Only `arm64-v8a/libcanvas.so` and `arm64-v8a/libonnxruntime.so` are packaged; no extra ABI is introduced.

These design properties are covered by source and APK inspection. Airplane-mode behavior still requires the manual Android acceptance run below.

## Build 3.2.18

This remains the existing direct native/custom-APK build. It does not use Gradle or CMake.

Requirements:

- Clang capable of targeting `aarch64-linux-android26` (override with `CLANG` or `VAST_ANDROID_TARGET` if needed)
- Python 3 with `cryptography`
- JDK with `javac`/`java`, Android SDK platform 36 `android.jar`, and build-tools D8 JAR for the thin View adapter
- The checked-in model, dictionary, ONNX Runtime headers/library and licenses

```bash
cd Vast-v3.2.7-source
python3 tools/build_android_views.py --android-jar "$ANDROID_JAR" --d8-jar "$D8_JAR"
./build-native.sh
python3 tools/build_v2.py
python3 tools/package_source.py
```

Outputs:

```text
build/libcanvas.so
dist/Vast-v3.2.18-unsigned.apk
dist/Vast-v3.2.18.apk
dist/Vast-v3.2.18-source.zip
```

`tools/build_v2.py` refuses to package a model, dictionary, inference configuration, ORT library or license whose pinned SHA-256 does not match. It emits version 3.2.18/code 53, normalizes ZIP metadata for reproducible output with the same inputs/signing identity, and performs its own APK Signature Scheme v2 digest, signer, certificate and public-key verification after writing the APK. Preserve `build/v2-key.pem` and `build/v2-cert.der` when an update-compatible signing identity is required; do not distribute the private key in a source archive.

`tools/package_source.py` creates the source ZIP deterministically, writes a per-file SHA-256 manifest inside it, verifies ZIP integrity/unique names, and excludes generated build/dist/test data, executables and signing secrets.

## Host tests

On the validated Windows/WSL setup, run the complete matrix (including real host-native model inference) with:

```powershell
pwsh -NoProfile -File .\tests\run_final_host_tests.ps1
```

The script requires GCC in WSL and the official ONNX Runtime 1.28.0 Windows x64 package at the fixture runner's configured/default location. The equivalent individual GCC commands are below. `test_v30` expects an empty, writable `testdata30` directory; use a clean directory between runs.

```bash
mkdir -p build testdata30 testdata_canvas_ocr

gcc -std=c11 -O2 -w test_v30.c -lm -o build/test_v30
./build/test_v30

gcc -std=c11 -O2 -Wall -Wextra -Werror \
  test_ocr.c src/ocr_hash.c src/ocr_identity.c src/ocr_segmenter.c \
  src/ocr_rasterizer.c src/handwriting_index.c src/ocr_sidecar.c \
  -lm -o build/test_ocr
./build/test_ocr

gcc -std=c11 -O2 -Wall -Wextra -Werror \
  test_ppocr_recognizer.c -Isrc -Ithird_party/onnxruntime/include \
  -ldl -lm -o build/test_ppocr_recognizer
./build/test_ppocr_recognizer

gcc -std=c11 -O2 -Wall -Wextra -Werror -Isrc \
  tests/test_ocr_manager.c src/ocr_manager.c src/ocr_hash.c \
  src/ocr_identity.c src/ocr_segmenter.c src/ocr_rasterizer.c \
  src/handwriting_index.c src/ocr_sidecar.c src/ocr_unicode_android.c \
  -lpthread -lm -o build/test_ocr_manager
./build/test_ocr_manager

gcc -std=c11 -O2 -w test_canvas_ocr.c -lm -o build/test_canvas_ocr
./build/test_canvas_ocr

gcc -std=c11 -O2 -w tests/test_canvas_persistence.c -lm \
  -o build/test_canvas_persistence
./build/test_canvas_persistence

gcc -std=c11 -O2 -w tests/test_input_regressions.c -lm \
  -o build/test_input_regressions
./build/test_input_regressions
```

The pure recognizer test validates preprocessing, dynamic width, dictionary layout and CTC decoding without loading ORT. The manager suite supplies a deterministic mock recognizer so lifecycle, invalidation, cache, stale-job and concurrency behavior remain reproducible.

The real host-native fixture test requires the official ONNX Runtime 1.28.0 Windows x64 package separately; that x64 DLL is a test dependency and is not placed in the Android APK:

```powershell
pwsh -NoProfile -File .\tests\run_ppocr_fixture_windows.ps1 `
  -OrtRoot C:\path\to\onnxruntime-win-x64-1.28.0
```

It executes the bundled ONNX model through the production recognizer API and checks searchable terms in handwritten-looking Chinese, English and mixed fixtures. In the maintainer QA tree, `artifacts/ocr/ppocr-host-fixture-2026-09-16.log` records the actual recognized strings, confidence values and timings; generated QA artifacts are intentionally omitted from the public source archive.

After building the APK, reproduce the manifest, contents, hash, size delta and APK Signature Scheme v2 checks without Android SDK tools:

```powershell
python .\tools\inspect_release.py
```

The inspector always verifies the APK's exact member set, source hashes, v2 signature and generated/persisted certificate. In the maintainer tree it also compares the signer with `artifacts/baseline/Vast-v3.2.7-baseline.apk`; because that historical APK is intentionally omitted from the public source bundle, a clean extraction reports that update-signer comparison as skipped. Producing an update-compatible APK still requires the original `build/v2-key.pem` and `build/v2-cert.der` pair.

## Historical 3.2.9 acceptance notes

Current device results and limitations are in [RELEASE_3.2.12.md](RELEASE_3.2.12.md). The checklist below records an earlier release and remains useful for physical-pen/offline acceptance.

A physical HONOR YLE-W09 running Android 16 (API 36) was used for incremental 3.2.9 smoke testing. Update-install, cold launch, OCR manager startup to **Ready**, new-note cancellation, force-stop/relaunch persistence and the idle text-editor CPU regression were exercised. This was synthetic finger-input testing only: no real stylus was used, handwriting recognition was not accepted end-to-end on Android, and the airplane-mode cases below remain open. The display supports modes including 120 Hz, but the active smoke-test mode was 90 Hz and application frame pacing at 120 Hz was not measured or confirmed.

- [ ] **A — offline English:** enable airplane mode, disable Wi-Fi/mobile data, launch without Google services, write `engine pressure`, wait for idle OCR, search `pressure`, and verify the result navigates to the ink.
- [ ] **B — offline Simplified Chinese:** write `明天去学校`, search `学校`, and verify the result navigates to the ink.
- [ ] **C — zoom invariance:** radically change zoom and verify the same search result remains.
- [ ] **D — translation reuse:** move the handwriting thousands of world units, verify navigation uses its new bounds, and use debug timing/counters to confirm translation alone caused no inference.
- [ ] **E — erase consistency:** erase part or all of the handwriting and verify stale results disappear or the local region is re-recognized.
- [ ] **F — responsiveness:** rapidly write and navigate while work is queued; verify smooth ink and no obvious UI hitch.
- [ ] **G — offline restart:** force-stop and relaunch offline; verify cached handwriting search works and no model/network request occurs.
- [ ] **H — corrupt cache recovery:** in a test environment remove or corrupt the current `.vastocr` file; verify the canvas opens immediately and the index rebuilds in the background.

Also verify cold model initialization through a real recognition request, project switching/duplication, Settings enable/disable/rebuild, process shutdown, search result text rendering for Chinese, real-stylus pressure/tilt/hover behavior, and update-install signing compatibility against the intended production signing identity. Until these cases and real Android inference are run, 3.2.9 is host-validated and device-smoke-tested but **not fully device-acceptance-verified**.

# Vast 3.2.7

Vast is a native ARM64 infinite-canvas workspace for Android 16 tablets. The app uses a hybrid architecture: the document/canvas engine remains native C/OpenGL ES while selected application chrome uses Android platform Views.

## 3.2.7 interaction and picker pass

- Replaced the small floating Pen/Eraser circle with a much larger **quarter-circle anchored directly into the bottom-left corner**. The visible control is 88dp-class with a 96dp-class radial hit target.
- Text objects now **size themselves from their contents**. Short text grows horizontally, longer text wraps at a readable maximum width, multiline text grows vertically, and pinch gestures move text without fighting an artificial manual scale. Sizing updates while Android IME/hardware-keyboard text is mirrored live.
- Wired the previously inert single-selection **More** button. It now opens a contextual menu with Color where applicable, Lock/Unlock where supported, and Deselect. GPU UI invalidation tracks the menu state so it appears and disappears immediately.
- Fixed Focus navigation so **one-finger pan wins before hidden minimap/object hit targets**. A finger drag in Focus now pans from anywhere on the canvas except the explicit Focus-exit and Pen/Eraser controls; two-finger pan/zoom remains available.
- Rebuilt the color drawer in the visual language of Blender's color popup: a large circular **Hue/Saturation wheel**, narrow vertical **Value** strip, high-contrast selection cursors, split previous/current swatch, HSV + hex readout, recents and compact dark chrome.
- Fixed a wheel-compositing error where the border primitive could cover the rendered gamut and visually desaturate it. The rim is now painted below the wheel so saturated colors remain saturated.
- Stroke smoothing and the 3.2.5 antialiasing path are unchanged. Document formats remain compatible.
- Version code: 42. Target SDK: 36. Minimum SDK: 26. ABI: arm64-v8a.

# Vast 3.2.6

Vast 3.2.6 moved color-critical rail and picker surfaces into the renderer to avoid vendor overlay tinting.

## 3.2.6 colour/UI correction

- Fixed the grey/desaturated left rail and colour picker seen on some Android 16 vendor builds. The Android-native header remains, but the rail, pen/eraser toggle and colour-critical picker surfaces are now drawn in Vast's own RGBA overlay so vendor `PopupWindow` tinting cannot alter their colour or opacity.
- The old native View copies of those controls are explicitly hidden, preventing a dim duplicate layer from sitting over the renderer-owned controls.
- Rebuilt the saturation/value field as a continuous per-pixel gradient with rounded clipping instead of a coarse tiled grid.
- Rebuilt the hue strip as a continuous full-spectrum gradient.
- Picker cursors now use high-contrast rings plus an inner live colour preview. The drawer handle is tinted by the currently selected colour.
- Recent colours get a clear selected ring, and the active rail item gets a stronger accent indicator while inactive labels retain full contrast.
- Finger and stylus drag behaviour, recent-colour persistence and the 3.2.4/3.2.5 stroke geometry/AA path are unchanged.
- Version code: 41. Target SDK: 36. ABI: arm64-v8a.


## 3.2.3 stroke-quality pass

- Replaced visible raw polyline joins for pen ink with a screen-space resampled midpoint-quadratic spline. Existing saved strokes benefit immediately; no document migration is required.
- Pressure width is smoothed together with the centerline so pressure noise no longer produces needle-like pointed edges.
- Wet/live ink and committed ink now use the same spline renderer, avoiding the shape changing when the stylus lifts.
- At normal and close zoom the spline is evaluated at about one screen pixel per sub-segment; at far zoom it uses a slightly wider adaptive spacing while remaining curved.
- New installs default to 54% stroke smoothing instead of 40%, while saved user settings remain respected.

## 3.2.2 fixes

- Native Android shell theme colors now refresh immediately when a Vast theme or custom theme role changes, including the Vast title, project subtitle, buttons, rail, picker and outlines.
- Native shell animation work is isolated to the surface that is actually moving. The rail and right color drawer use Android View translations/alpha instead of rebuilding their layouts every animation frame.
- Native text entry is live: Android EditText contents are mirrored into notes, frame names, place names, search queries and project names while typing. Cancel restores the original value; Save/Done persists the live value.
- Stylus input can operate ordinary UI controls and the complete right-side color picker. Finger and stylus now share the same color drawer/picker interaction path.
- Normal ink rendering uses round tapered capsules for every segment and round caps/joins instead of extended pointed quads. Close and normal zoom no longer drops intermediate stroke points; LOD simplification is restricted to far zoom.
- The color picker fallback copy now explicitly indicates that either finger or stylus can be used.
- The native UI overlay remains separate from the OpenGL surface, preserving low-latency spatial rendering while using Android Views for high-frequency app chrome.

## Current interaction model

- Stylus draws, erases, lassos, edits CAD/shapes and can operate UI controls.
- One finger pans empty canvas and manipulates selected objects.
- Two fingers pan/zoom the viewport.
- Finger hold on empty canvas opens the radial tool menu after its arming delay. Stylus contact never opens the radial menu.
- Photos require a finger press-and-hold to select.
- Frames resize only from a corner and can lock their size.
- Text editing/renaming uses the Android system keyboard or a hardware keyboard.

---

# Historical 3.0 design notes

Vast is a native ARM64 infinite-canvas workspace for Android tablets. Version 3.0 keeps the existing C/OpenGL ES document/rendering engine and expands the UX around a canvas-first, stylus-first workflow rather than replacing the app with a new framework.

## Vast 3.0 interaction model

- **Stylus creates.** Pen, marker, eraser, lasso/select, shapes and CAD remain stylus-oriented.
- **One finger selects/manipulates.** Tap an object to select it and drag the selected object directly.
- **Two fingers navigate.** Empty-canvas two-finger gestures pan and pinch the viewport. A two-finger gesture begun on the current selection transforms that selection instead.
- **Two-finger tap = Undo; three-finger tap = Redo.** Visible Undo/Redo remain in the header.
- **Hold the stylus for the radial HUD.** The hold has a visible progress ring and zoom-aware movement tolerance.

## UI / UX

- Full-screen canvas-first shell with reduced persistent chrome.
- Workspace utility rail contains Add, Frames, Photos, CAD, Search, Layers, Map and Settings; drawing tools live in the radial HUD.
- Focus/Zen mode hides normal chrome while drawing and leaves only a small edge reveal target.
- Clean outlined pills with no nested/ghost capsule artifact.
- Context toolbar appears only for the active selection and offers context-sensitive Edit/Color/Photo/CAD, Duplicate, Group/Ungroup and Delete.
- Special-effect/parallax UI remains disabled; the renderer keeps the neutral dynamic grid.

## Radial Menu 3.0

- Pen
- Marker
- Eraser
- Size
- Color
- Select/Lasso

Brush size is deliberately two-stage: entering the Size wedge only arms sizing. The stylus must then move outward into the size scrub zone before the brush width changes. Brush range is 1-72 px.

## Frames 2.0

Frames are first-class workspace regions rather than decorative rectangles:

- Create by stylus drag.
- Persistent names and per-frame lock state.
- Jump/focus a frame.
- Rename, duplicate and delete.
- Move the frame together with strokes, photos, CAD dimensions, notes and shapes whose centers are inside it.
- Appear as landmarks on the canvas/minimap.
- Presentation mode steps through frames.
- Export selected frame to a rendered PNG in the app external-data directory when available.

## Selection / Lasso / Direct Manipulation

- Tap logical canvas objects to select them.
- Stylus lasso selects visible strokes, photos, CAD dimensions, notes and shapes.
- Clean single selection outline with four corner handles.
- Drag selection directly with one finger.
- Two-finger selection transform supports translation, scaling and rotation where the object type supports it.
- Mixed selections can be grouped/ungrouped.
- Duplicate supports ink, photos, CAD measurements, notes, shapes and frames.
- Color action recolors selected ink/shapes through the existing picker.

## Smart snapping

While moving a selection, Vast checks the dynamic grid plus nearby frame/object edges and centers. Guides appear only during manipulation and disappear on release. Photo rotation can independently toggle 15-degree angle snapping.

## Workspace objects

### Notes
- Text notes and sticky notes.
- Persistent position, size, color and text.
- Lightweight in-app editor.

### Shapes
- Line
- Rectangle
- Ellipse
- Arrow
- Pen hold-to-clean converts sufficiently straight strokes to lines and sufficiently closed loops to ellipses.

### Photos
- Existing photo import/editing retained.
- Direct movement and two-finger transform.
- Optional 15-degree angle snap.

### CAD
- Existing measurements, calibration, snap, comparison and deletion retained.

## Layers

On-demand Layers panel controls visibility and lock state for:

- Ink
- Marker
- Photos
- CAD
- Notes
- Shapes
- Frames

The panel is not permanently visible, keeping the canvas quiet.

## Navigation

- Interactive minimap: tap to jump around the document.
- Frames appear in the minimap.
- Saved Places / bookmarks store viewport center and zoom; bookmark markers appear in the minimap.
- Saved Places can be renamed, jumped to or deleted.
- Search indexes typed note/sticky text, frame names and saved-place names. Selecting a result navigates to it.

## Projects

Vast 3.0 includes four local project slots:

- Project 1 uses the original Vast document paths, preserving existing user data.
- Each project has independent ink, metadata, photos, frames and workspace objects.
- Rename current project.
- Duplicate current project into an empty slot.
- Switching projects saves the current project first.

## Persistence compatibility

- Package remains `com.ayomi.infinitecanvas`.
- Project 1 preserves the existing `canvas.icv`, `canvas.meta`, `images.icv`, and `frames.vfr` paths.
- Frame persistence reads old VFR1 and writes VFR2.
- New notes/shapes/bookmarks/groups/viewport/layer locks are stored in a separate `workspace.v3` file so old canvas data is not discarded.
- APK uses the same signing chain as Vast 2.2, so it is intended to update-install over the current build.

## Deliberately excluded from 3.0

Handwriting recognition/conversion was explicitly excluded from this build. Search therefore covers typed canvas data, frame names and saved places, but does not OCR/recognize handwritten ink.

## Build

```bash
cd /mnt/data/Vast-v3.0-source
python3 tools/build_android_views.py --android-jar "$ANDROID_JAR" --d8-jar "$D8_JAR"
./build-native.sh
python3 tools/build_v2.py
```

Output:

```text
dist/Vast-v3.0.apk
```

Target SDK: 36  
Minimum SDK: 26  
ABI: arm64-v8a

## Regression test

`test_v30.c` covers the major native model/interaction regressions including dynamic radial tolerance, frames, frame-content movement, minimap navigation, photo angle snap, notes/search, shape creation, mixed-object duplication, project duplication and PNG frame export.

## Vast 3.0.1 interaction corrections

- Restores reliable one-finger canvas panning; moving a finger off an empty-space hold cancels the radial gesture and immediately becomes a pan.
- The radial menu is now invoked by a finger hold on empty canvas rather than stylus contact. Stylus drawing/select input never arms the radial hold.
- Photos are selected only by a deliberate finger press-and-hold. Stylus Select/lasso no longer selects photos. Once selected, a photo can still be dragged and two-finger transformed.
- Single-photo dragging bypasses magnetic object/grid snapping to avoid sticky movement.
- Removed the floating contextual action bar for selected photos.
- Frames resize live only from corner handles; pinch gestures never resize frames. Per-frame Size lock disables corner resizing.
- Frame size-lock state persists in the VFR3 frame file format, backward compatible with VFR1/VFR2.
- Stroke selections no longer accept pinch-scale transforms or show resize handles.
- Frame/place renaming lives inside Frames & places and uses a native Android EditText, so hardware keyboards and the system IME are used instead of Vast's custom editor keyboard.
- The Vast/project identity remains visible at top-left even in Focus mode.

## Vast 3.1.0 visual/interaction polish

- Normalized floating surfaces to a flatter, quieter visual language: restrained 1dp-class outlines, very soft ambient elevation, consistent radii, and no top-edge shine.
- Active buttons now rely on an accent outline/text treatment instead of heavy filled interiors.
- Reduced UI color noise: primary workspace chrome now uses one accent language; semantic danger remains reserved for destructive actions.
- Header hierarchy is quieter: Vast remains top-left, project name is subordinate, and tool identity is handled by a compact quick-ink strip.
- Added a one-tap Pen/Eraser toggle in the top-left quick-ink strip.
- Added one-tap color swatches plus a current-color swatch that opens the full picker; selecting a quick color automatically returns to Pen.
- Radial menu finger-hold now has an additional 180ms arming delay before the visible progress begins/open can complete, reducing accidental invocation while panning.
- Radial hold progress ring radius increased from 28dp-class to 48dp-class so it remains visible around a fingertip.
- Selection visuals are thinner and quieter; stroke/photo selections do not show unnecessary resize handles.
- Frames use zoom-aware optical hierarchy: subtle at close zoom, stronger as navigation landmarks at far zoom, with one coherent accent language.
- Minimap surface fades to a much quieter state when it is not actively useful, while remaining fully visible during navigation/frame work.
- Dynamic grid transitions now crossfade between scale levels in both software and GPU paths to reduce density popping while zooming.
- Panel/action colors were normalized so Add, Layers, Shapes, Settings and contextual actions feel like one system instead of separate colorful widgets.

Handwriting recognition remains deliberately excluded.

## Vast 3.1.1 frame/editor cleanup

- Frame/place rename now visibly mirrors the live Android IME / hardware-keyboard text inside the **Frames & places** panel. The native `EditText` remains only as the system input bridge, so Vast no longer depends on a custom in-app keyboard for frame/place names.
- The selected-frame **More** action now opens the full **Frames & places** panel instead of doing nothing.
- New frames have a live translucent outline/fill/corner preview while dragging, rendered in the UI overlay so it remains visible even with the GPU document cache.
- Default frame names are compact/dynamic. Generated names are renumbered after deletion/load (`Frame 1`, `Frame 2`, ...), while custom names remain untouched.
- Removed the row of standalone color swatches from the top-left chrome. The quick-ink strip is now three consistent controls: **Pen**, **Eraser**, and **Color**. Pen/Eraser are direct one-tap switches; tapping Color cycles the compact quick palette while the radial Color action still opens the full picker.

## Vast 3.1.2 interaction changes

- Right-edge pull-out color drawer with finger-driven SV/hue picking.
- Six persistent recent colors under the Vast/project identity. Recent history only changes when a genuinely new picker color is committed; tapping an existing recent color never reorders it.
- Bottom-left circular Pen/Eraser toggle.
- Eraser radius reticle is visible only while the stylus is touching the display; no hover cursor.
- Color drawer can be dragged smoothly from the right edge and remains non-modal outside its own surface.
- Picker sessions are committed on close, app pause, and app destruction so newly chosen colors persist safely.


## Vast 3.2.0 native Android UI shell

Vast 3.2 keeps the native C/OpenGL ES canvas engine but moves ordinary app chrome onto real Android platform Views created through JNI. This is intentionally a hybrid migration rather than a canvas rewrite.

- Top-left Vast/project identity and six recent-color swatches are native Android `TextView`/`View` overlays.
- Search, Undo, Redo, Projects and Focus controls are native Android controls.
- The left utility rail and collapsed reveal handle are native Android controls while retaining the existing C gesture/hit architecture.
- Bottom-left Pen/Eraser toggle is now a native Android view.
- The right color drawer is presented with Android `GradientDrawable`/`LayerDrawable` surfaces and gradients while finger picking remains driven by the low-latency canvas input router.
- Text entry for notes, frame/place rename, search and project rename now uses a real Android `EditText` and the system/hardware keyboard; the custom on-canvas QWERTY keyboard has been removed from the editor flow.
- Canvas-spatial feedback remains in the native renderer: wet ink, lasso, selection geometry, Frames, snap guides, radial HUD, hold-progress ring, eraser reticle, minimap and live frame creation.
- No Compose/Gradle dependency was added, so the renderer remains small and the package/signing identity is unchanged.

Package: `com.ayomi.infinitecanvas`  
Version code: 35  
Target SDK: 36  
Minimum SDK: 26  
ABI: arm64-v8a

## 3.2.1 native-shell visibility hotfix

The 3.2.0 Android View shell could be hidden behind NativeActivity's rendering surface on real hardware. 3.2.1 hosts the native View chrome in a non-touchable full-window PopupWindow above the OpenGL/NativeWindow surface while preserving native canvas hit-testing beneath it. A software-rendered UI fallback remains available if the overlay host cannot be created. The overlay is explicitly dismissed and recreated with the native window lifecycle.


## Vast 3.2.4 stroke-quality pass

- Reworked pen input filtering to be speed-adaptive: slow handwriting receives stronger digitizer-jitter suppression while fast strokes catch up more aggressively to the stylus.
- Replaced eased per-event point insertion with uniform screen-space resampling so MotionEvent boundaries no longer create dense point clusters that survive as tiny corners.
- Added a five-tap spatial prefilter before spline construction, preserving exact first/last points while smoothing high-frequency centreline noise.
- Increased spline tessellation density to sub-pixel spacing and strengthened pressure filtering to eliminate needle-like width spikes and scalloped joins.
- Captures the final stylus-up position before committing the stroke, reducing short/lagging stroke tails.
- Existing saved ink benefits automatically because smoothing remains a render-time operation; the document format is unchanged.

Package: `com.ayomi.infinitecanvas`  
Version code: 39  
Target SDK: 36  
Minimum SDK: 26  
ABI: arm64-v8a


## Vast 3.2.5 color / edge-quality pass

- Reworked pen edge coverage with a wider signed-distance antialiasing ramp. The 3.2.4 spline geometry is retained, but diagonal and curved stroke boundaries now keep visible sub-pixel coverage after the GPU canvas cache is resampled.
- Rebuilt the right-side color picker around a larger continuous saturation/value field and a horizontal hue strip.
- Picker drags now remain captured and clamp at the field edges, so fast finger/stylus movement no longer drops when it briefly leaves the exact gradient bounds.
- Picker cursors move with Android View translations rather than allocating new layout parameters for each MotionEvent.
- Added six recent-color swatches inside the picker while preserving the existing rule that choosing an already-recent color does not reorder history.
- Native rail/picker surfaces now use higher-opacity theme-tonal fills and a faster visual fade-in so they no longer look like disabled gray controls during reveal animation.
- Existing stroke smoothing and document formats are unchanged.

Package: `com.ayomi.infinitecanvas`  
Version code: 40  
Target SDK: 36  
Minimum SDK: 26  
ABI: arm64-v8a
