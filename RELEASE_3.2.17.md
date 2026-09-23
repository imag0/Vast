# Vast 3.2.17

Package `com.ayomi.infinitecanvas`, version code 52, minimum SDK 26, target SDK 36, ARM64.

## Changes

- Massive photo import no longer monopolizes the NativeActivity input loop. Vast keeps one Android `BitmapRegionDecoder` open, decodes one bounded 1024 px output tile per animation tick, and processes Android input between tiles.
- GPU texture creation is incremental. At most one 2048 px image tile is converted and uploaded per rendered frame, while already uploaded tiles remain visible.
- Photo placement, scaling, rotation, and movement persist to a small per-project metadata sidecar. These ordinary edits no longer rewrite an 8K photo's full raw pixel payload. Pixel files are rewritten only when photo content is added, duplicated, or deleted.
- Hidden canvas chrome is revealed only by a completed stationary tap on empty canvas. Finger-down and pan keep chrome hidden, and the revealed side rail starts compact rather than expanded.
- The top navigation, compact side rail, tool panel, and contextual bar use native Android `View` entrance and exit animations from their anchored edges.

## Limits

- Vast still bounds imported photos to 8192 px on the longest edge and 64 megapixels. An 8000x8000 image requires about 256 MB of native pixel memory plus GPU texture memory.
- Import is cooperative rather than background-threaded: individual region decodes still consume one event-loop slice, but control returns between every 1024 px tile so Android does not see one multi-second input callback.
- Android may reclaim the app under extreme system-wide memory pressure. Vast cancels an unfinished import safely on lifecycle teardown.

## Verification

- Complete host matrix passes: OCR, persistence, input state, eraser, legacy v3.2.7, product polish, retained rendering, live-stroke performance, and real PP-OCR fixtures.
- Native ARM64/OpenGL/OCR link and Java 8/D8 builds pass.
- Real-device checks use an Honor tablet at 3000x1920 with an 8000x8000 fixture, ADB input injection, screenshots, logcat ANR inspection, and package metadata verification.
