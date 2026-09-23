# Vast 3.2.11 — eraser, input ownership and native menu polish

Package `com.ayomi.infinitecanvas`, version code 46, target SDK 36, minimum SDK 26,
arm64-v8a. Installed as an update on the HONOR YLE-W09 (Android 16), without
clearing app data. Testing used the existing QA Project 4, not the user's Project 1.

## What changed

- Erasing uses a compact BVH over chunks of at most 32 segments. Cached bounds
  reject distant geometry and split extremely long strokes into local queries.
  Inactive strokes stay indexed, so deletion and ordinary undo/redo need no
  rebuild. Geometry revision, stroke count and backing-array identity invalidate
  the index for drawing, movement, duplication, loading and project changes.
- Each erase gesture stores one undo action; inactive state replaces quadratic
  duplicate-ID searches. History capacity is reserved before destructive input.
  Persistence and OCR reconciliation remain outside active MOVE processing.
- The retained canvas is repaired only within erased ink's visual bounds. Nearby
  surviving strokes are queried through the same index and composited in document
  order. A no-hit move performs no canvas texture upload. The reticle is a shader
  uniform, not a freshly rasterized full-screen UI texture.
- Actual MotionEvent tool type and stable pointer ID own a stroke. Fingers in
  Eraser mode navigate without entering the erase path. Hover, UP, CANCEL, missing
  pointers, activity pause and input-queue destruction clear contact state.
  Cancellation restores erased ink. Crossing UI during a canvas-owned stroke
  cannot also activate that UI. Side-button actions cannot change tool mid-contact.
- Settings has a persistent category rail and compact Drawing, Canvas, Handwriting
  Search, Appearance and Stylus button sections. Pressure response is reachable
  in Drawing. Custom theme roles and swatches are visible together. Layers has
  one visibility/lock row per layer. Add, Projects, Frames/places and contextual
  actions have clearer grouping and selected states. Non-undoable deletion asks
  for confirmation; normal erasing does not.
- Ordinary menus and editors use existing Android Views, not Compose. Editors
  retain visible Unicode input, cursor, selection, scrolling and IME handling.
  Fixed a panel-model hash bug that ignored changes after the first 64 characters.
- Handwriting Search says what it does, that recognition stays on-device, that
  handwriting is unchanged, and that nothing is uploaded. Technical architecture
  stays in developer documentation rather than normal Settings.
- Native opening transitions use cancellable alpha/translation (140/180 ms).
  Dismissal is immediate and cancels outstanding animation. Spatial settling,
  level-of-detail and hold feedback use elapsed time, without spring overshoot.
  Native animation does not request a canvas texture rebuild every frame.
  The radial menu no longer draws a long instruction across its tool labels.

## Profile results

Device measurements below are observations, not a controlled hardware benchmark.
They exclude neither OS scheduling nor thermal/load variation. CPU lookup tests
exclude index construction, rendering, persistence and input dispatch.

| Cached miss query | Before (microseconds) | After (microseconds) |
| --- | ---: | ---: |
| 10 short strokes | 0.371 | 0.022 |
| 200 short strokes | 0.909 | 0.048 |
| 2,000 short strokes | 6.394 | 0.062 |
| 10,000 short strokes | 29.901 | 0.032 |
| Inside bounds of a 200,000-point circle, away from its segments | 962.383 | 0.024 |

The long-stroke case demonstrates why a stroke-wide bounding box alone was not
enough: the old path traversed every segment even though none was near the eraser.
The test uses repeated misses, not representative hit latency or frame time.

On the real tablet, the verified Eraser replay in the dense QA project dropped
from **364.2 ms** mean synchronous event dispatch in the old no-hit bucket to
**3.2 ms** in the corresponding new steady-state bucket. A sweep through ink
had later new-build bucket means **6.4–8.5 ms**, with a recorded maximum **29.1 ms**.
The old hit sweep was around **357.6 ms** in its last bucket. The previous sparse
project no-hit bucket was **99.6 ms**, revealing full-scene work even without a hit.

Method: `DeviceEraseReplay` injects real Android MotionEvents synchronously and
times dispatch completion, with an 8 ms pause between samples excluded from the
timing. These are **not physical-pen latency, FPS, or presentation measurements**.
Baseline uses 120 events; the improved replay uses 300 and traverses farther.
The project was populated with 2,000 short strokes plus existing QA content, but
the before/after documents were not byte-identical: a small earlier erased area
survived the update because old toolbar Undo had not persisted. This persistence
gap is fixed here. No precise speedup factor is claimed for the uncontrolled run.

A later active-device check reported display mode 4 / 120.00001 Hz and active
render rate 120.00001 Hz. With the final high-refresh preference, a 600-event
no-hit replay averaged **8.54–8.71 ms** per dispatch bucket. The display can return
to 60 Hz while idle; the app does not override global battery or display policy.

Raw local evidence is under `artifacts/eraser-pass/baseline` and `current`.
Use `verified-eraser-*`, `benchmark.txt`, `final-miss-dispatch.txt`, and
`display-active.txt`. Early files named `replay-empty`, `replay-dense-miss`, and
`replay-dense-hit` are excluded: those runs had the wrong tool selected. Likewise,
the explicit `legacy_full_scene_prepare_us` benchmark is not the optimized erase
hot path and must not be presented as its post-fix frame time.

## Verification

- Full host matrix: PASS, including OCR, persistence, input routing, product/UI
  state, legacy behavior, real ONNX recognition fixtures and incremental live ink.
- Input regression suite: 68 checks, including finger/two-finger navigation,
  hover, reordered pointer indices, secondary pointer UP, absent/zero-pointer
  CANCEL, UI crossing, cancellation rollback and one-action undo semantics.
- Spatial suite: 500,273 checks comparing production queries with brute force,
  and testing locks, activation changes, geometry invalidation and repaint order.
- Erase rendering suite: no-hit uploads, bounded damage uploads, pixel-equivalent
  repaint with overlapping translucent ink, repeated samples, cancellation and
  equal elapsed-time animation positions at 60/90/120 Hz: PASS.
- On-tablet synthetic contact screenshots: finger pan and pinch have no reticle;
  stylus contact has a reticle; release, hover and cancellation do not. Pinch replay
  completed after an initial automation overlap was corrected. No physical pen
  was operated by this agent.
- User-performed physical-tablet acceptance: the user confirmed that one/two-finger
  navigation, stylus contact/release reticle behavior and eraser responsiveness
  work correctly and feel responsive. This closes the hands-on eraser check;
  it is distinct from the automated evidence above.
- Native edit probes confirmed visible 528-character multiline English/Chinese/
  emoji text, query entry, rename and calibration input. This does not substitute
  for physical Chinese IME composition testing.
- Major native and spatial surfaces were captured and visually reviewed; see
  `MENU_REVIEW.md`. A final repeat review covered the revised custom-theme layout,
  stable Settings height, pressure control, and corrected radial labels.
- Rapid native navigation: 12 open/close cycles and 60 category changes in 5.055 s
  with 25–40 ms requested gaps; no stale dialog at completion. Android gfxinfo
  reported 702 frames, 48 janky frames (6.84%), median 5 ms, 90th percentile 8 ms,
  95th percentile 17 ms, 99th percentile 25 ms, and no slow bitmap uploads. These
  are Android View statistics, not native canvas presentation statistics. This
  is not a zero-jank result.

## Remaining acceptance limits

The physical eraser/navigation check was confirmed by the user. The broader pass
is **not certified entirely jank-free**: extended palm-rejection scenarios,
subjective menu motion at 120 Hz and long-session thermal behavior still need
acceptance. The app requested and the device reported 120 Hz, but
that alone does not prove every frame meets an 8.33 ms presentation deadline.

Spatial preparation is still a whole-index rebuild once after geometry changes,
usually prewarmed while Eraser is idle. Memory failure falls back to the correct
slower path. Deleting a viewport-spanning stroke legitimately damages a large
area; heavily overlapping surviving long strokes can still require expensive
raster work. Far-zoom constellation connections use a full-scene fallback.
Document persistence/OCR work at gesture completion can still be noticeable on
large documents, although the reticle is hidden before that work starts.

Native panels reconstruct their rows when their model/category changes, not on
every animation frame. The rapid-navigation missed deadlines above remain an
optimization opportunity. Canvas-only fallback panels remain in source for
adapter failure/host tests; ordinary panels on the reviewed tablet use Views.
Spatial color, radial, minimap and presentation overlays remain canvas-owned.

Deliverables: `dist/Vast-v3.2.11.apk` and `dist/Vast-v3.2.11-source.zip`.
Signing identity is preserved. The source bundle excludes build output, device
artifacts, app-private documents and signing secrets.
