# Vast 3.2.12 — drawing, navigation and native color

Version code 47; package `com.ayomi.infinitecanvas`; min SDK 26, target 36,
arm64-v8a. Installed as a signed update on HONOR YLE-W09 / Android 16 without
clearing data. In response to the physical fast-writing and navigation checklist,
the user confirmed: **“Smooth, no noticeable jiggle or growing delay.”** The
remaining coverage limits below are separate from that successful acceptance.

## Causes and implementation

- The old stroke path expanded incoming points into interpolated document
  points, used a velocity-independent filter, and revisited an unsettled spline
  tail. The new path consumes historical samples before the current sample on
  MOVE and UP, with each sample's monotonic nanosecond timestamp. Exact duplicate
  position/pressure tuples alone are omitted from storage. No arbitrary sample
  thinning, coordinate quantization, reduced zoom, or refresh-rate cap was added.
- Position filtering has at most 6 ms time constant at low velocity, falls to
  zero at 600 screen pixels/second, and cannot trail the raw point by more than
  0.65 screen pixels. Pressure uses elapsed sample time, not event count. This
  filter bound is not a bound on total physical pen-to-display latency.
- A causal componentwise monotone Hermite piece uses the previous, last and new
  sample. It never waits for a future sample or changes an earlier centerline.
  Subdivision depends on world curvature, with a 1–64 bound independent of zoom.
  CPU export, live ink and retained ink share this curve. No pen-up refinement.
- Live ink appends to the existing incremental coverage mask; it does not redraw
  the growing prefix every frame. Retained VBOs use fixed 256-segment chunks and
  append only new geometry, with at most two chunk uploads per physical sample.
  The underlying compatible document point array still grows geometrically;
  occasional capacity reallocations are amortized, not hard real-time bounded.
- Previously, camera changes could invalidate an oversized CPU canvas bitmap,
  repaint strokes/text/shapes/photos and upload the entire result. GLES 3 now
  draws retained world-space ink/shapes, cached image textures and Android-shaped
  note tiles. Camera uniforms, bounds rejection and fixed-size decorations replace
  that bitmap path. Images rotate around their original center. Object ordering
  remains frames, regions, photos, notes, shapes, ink, dimensions, constellation,
  then UI. Marker self-overlap uses a single MAX coverage layer.
- Notes are laid out when content changes, not when camera scale changes. Glyph
  tiles are reused and offscreen tiles are skipped. The existing native bitmap
  text resolution is preserved; at extreme magnification it remains visibly
  rasterized, not newly vector text. Geometry and note texture caches are lazy:
  first exposure/content changes can still do work, unlike warmed camera frames.
- Input events are drained and acknowledged before one Choreographer frame is
  requested; the old 8 ms event-render gate is removed. Vsync scheduling does not
  deliberately delay/filter samples or change the device refresh setting.
- Ink snapshots, pending OCR stroke copies, minimap rebuilding and camera-position
  saves wait for 400 ms idle. The OCR worker retains its interaction cooldown.
  Pause and project changes still save synchronously. Atomic document replacement
  and existing compatibility formats remain; a process crash before the deferred
  save can lose the latest unsaved ink or camera position. An already-started idle
  save/inference is not preemptible. No claim of background-only persistence.
- The optimized whole-stroke eraser index and contact ownership remain. The new
  GPU path skips inactive ink directly; GLES 2 retains the tested damage-repair
  path. Finger navigation cannot acquire the stylus eraser reticle.
- Canvas branding was tied to transient chrome visibility and whole-popup
  recreation. The Vast mark is removed from normal canvas rendering. A quiet
  project label persists through pen DOWN/UP; native bars update existing windows
  rather than replaying every entrance on undo/tool state changes. Opening a modal
  intentionally hides ordinary canvas controls; that is not a loading logo.

Android reference contracts: [NDK MotionEvent history and timestamps](https://developer.android.com/ndk/reference/group/input)
and [Choreographer](https://developer.android.com/ndk/reference/group/choreographer).
The app is ARM64; the API 24 callback's `long` timestamp is 64-bit on this ABI.

## Native color and visual system

The bottom-right quarter-circle displays the selected color and mirrors the
bottom-left tool control's radius. A subtle outline keeps pale/dark colors visible.
Its expanded picker is Android Views: hue/saturation wheel, native brightness
SeekBar, current value, previous color and recent swatches. The wheel has pointer,
keyboard and hue accessibility actions. It opens near its corner in 190 ms and
closes in 140 ms using alpha/translation/scale; closing releases touch ownership
immediately and reopening cancels the outgoing animation. Bounds account for the
visible display area, and the native content scrolls when vertical space is short.

Reusable native styling uses warm light/dark foregrounds, approximately 94% opaque
surfaces, restrained 14 dp rounding, subtle half-dp borders and native ripple
feedback. No live blur or per-frame layout animation. EditText, IME, cursor,
selection, copy/paste, scrolling and native Back behavior are retained. The corner
launchers remain spatial canvas controls like the existing Pen/Eraser control;
the expanded color interaction does not depend on OpenGL UI. See
[UI_REVIEW_3.2.12.md](UI_REVIEW_3.2.12.md).

## Evidence and interpretation

Host matrix: PASS. Includes OCR core/preprocessing/manager, atomic persistence,
68 input-state checks, 500,273 spatial checks, eraser damage/motion, legacy product
behavior, 16 product-polish checks, incremental live ink, retained-scene checks
and four real PP-OCR model fixtures. Recognition fixtures are not a physical
handwriting usability test.

The retained suite appends 20,000 samples, checks bounded uploads and monotone
curves, then exercises 1,000 camera changes across 0.05–20x with no cached ink,
shape, image or text texture rebuild/upload. It also tests ordered MOVE/UP history,
the no-lag fast filter, deferred work and reused photo-allocation invalidation.
GL calls in this suite are mocked: its microsecond timings are CPU algorithm
checks, not GPU benchmarks. Device checks below exercise the actual GL driver.

Final tablet replay: 4,001 injected events over 63.6 seconds on QA Project 4.
The Android-delivered stream includes history/resampling, typically about 120
processed samples/second for about 60 delivered events/second. In full pen-input
intervals, received and processed sample counts match; queue drain batches remain
1. This does not establish the hardware digitizer's actual sample rate.

Near the end, sample processing averages 3.15–3.63 microseconds/sample. Retained
geometry work totals about 10.7–11.9 ms per one-second interval; live-mask work
totals about 4.2–4.7 ms. These are interval totals, not per-frame costs. CPU frame
work averages about 3.5–3.7 ms, with interval maxima around 6.2–6.7 ms, and sampled
event age stays around 1.7–2.5 ms in those final intervals. No growing processed
sample backlog was observed. Frame timings include CPU submission/swap but are
not display-present timestamps or physical pen latency.

Final warmed high-zoom pan intervals averaged about 2.4–2.8 ms CPU frame work.
Zoom-out intervals averaged about 2.6–3.2 ms, with occasional maxima up to 21.9 ms.
UI open/undo/first exposure have separate higher spikes (roughly 40 ms observed).
Zero-jank 120 Hz operation across all menus/load states is **not** claimed.

Synchronous Android injection for the long stroke had bucket means 4.7–8.1 ms,
maximum 13.3 ms; final pan mean 8.5 ms, maximum 29.5 ms. Eraser miss/hit-path replay
buckets were approximately 5.2–7.2 ms with maximum 13.2 ms. Injection completion
includes Android scheduling/resampling and is not physical pen latency or FPS.
The baseline and final documents/replay loads are not byte-identical, so no exact
speedup ratio is claimed. Final process PSS after the combined UI/ink run was
about 261 MiB; this is a point-in-time observation, not a peak-memory bound.

Local raw evidence: `artifacts/navigation-pass/current/host-tests-final.txt`,
`profile-final4000.txt`, `profile-final-navigation.txt`,
`stroke-final-build4000.txt`, `release-inspection.txt`, and reviewed PNGs.
Diagnostics store only a bounded ring of aggregate timings in the app's external
files directory (`interaction-profile.txt`), never coordinates or document text.
The source bundle intentionally excludes local artifacts and signing secrets.

Testing used QA Project 4. One early baseline synthetic circle was inadvertently
drawn in Project 1, disclosed immediately, undone once and checked visually
against the original handwriting before continuing in Project 4. Later eraser
test changes in Project 4 were undone. No application data was cleared.

## Physical acceptance

Automated input stopped before the user's test. The installed build was handed
over for fast physical writing of `engine pressure`, `navigation`, `明天去学校`,
cursive loops, repeated e, zigzags, circles, a long signature and diagonal hatching,
plus pan/pinch over ink and text. The user answered: **“Smooth, no noticeable
jiggle or growing delay.”** This is user-reported physical acceptance, not an
independently observed checklist of each individual mark.

The aggregate profile captured during that physical test is retained locally as
`profile-user-physical.txt`. Full continuous-contact intervals delivered and
processed approximately 476–480 samples/second, with roughly four samples per
120 Hz event/frame. All delivered/processed counts match in those pen intervals.
Processing was approximately 0.97–1.35 microseconds/sample, live-mask interval
totals 5.4–11.6 ms, geometry totals 1.7–3.2 ms and mean CPU frame work 0.42–0.50 ms.
Those continuous-contact intervals had one-event drain batches and maxima near
1.1 ms CPU frame work. Mixed short-stroke/hover intervals had batches up to 3 and
higher maxima; the log is not a hardware digitizer-rate specification or a
physical screen-presentation timing measurement. The user's content/camera state
differs from the dense synthetic stress scene, so these are not like-for-like
speedup measurements.

The earlier successful eraser report belongs to 3.2.11; this new confirmation
specifically concerns the installed 3.2.12 candidate.

On-device native checks passed for mixed-language visible editing/selection,
Search, object context/Focus, finger/stylus picker selection and 12 rapid menu
cycles with 60 category changes. Screenshots cover the requested UI states.
This pass has not independently completed a full accessibility-service audit,
every theme, rotation, every image-import path, or a physical frame-presentation
trace. GLES 2/software compatibility paths remain but do not gain GLES 3 retained
scene navigation performance. These limitations are not hidden as passing tests.
