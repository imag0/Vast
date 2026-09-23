# Vast 3.2.16

Package `com.ayomi.infinitecanvas`, version code 51, minimum SDK 26, target SDK 36, ARM64.

## Changes

- Every completed pen/highlighter stroke stores its final world position and current zoom in that project's backward-compatible workspace extension. After the camera moves meaningfully away, the native top navigation exposes Return; it recenters and restores the saved writing zoom.
- Pressing an actually empty canvas area exits Focus mode and expands the native side rail. It does not open every modal panel or steal taps from photos, ink, frames, notes, shapes, or dimensions.
- One-finger pan now consumes Android historical samples and removes only the initial 6 dp touch slop instead of applying it as one jump. Fling uses a recent-weighted, time-based release velocity, retains the exact direction, keeps the established 3200 px/s cap, and remains immediately interruptible by any finger, pinch, stylus, cancellation, or lifecycle transition.
- Photo import no longer uses MediaStore's 1280 px thumbnail. It region-decodes the original into bounded 1024 px working tiles, honors 0/90/180/270 MediaStore orientation, keeps up to 8192 px per edge / 64 megapixels, and uploads 2048 px GL tiles so an 8000×8000 source is not constrained by a single GPU texture.
- Vast checks `https://raw.githubusercontent.com/imag0/Vast/main/updates/latest.json` at most twice per day and provides a manual Settings → Canvas → Check for updates action. Downloads are SHA-256 checked, package-name checked, and required to match the installed APK signing certificate before the Android installer is opened. Vast cannot silently install an update.

## Security and privacy boundary

`INTERNET` and `REQUEST_INSTALL_PACKAGES` are present only for the public GitHub updater and the user-confirmed Android installer. OCR remains fully local and has no cloud/API path. Update APK bytes are served through a non-exported, read-only, single-file ContentProvider with temporary URI permission.

The signing private key remains local under `build/` and is excluded from Git/source archives. Public GitHub content contains source, the signed APK, its SHA-256 manifest, and no private signing material.

## Limits

- Images above 8192 px on either edge are decoded with a power-of-two sample so the longest edge is at most 8192 px. A full 8000×8000 image is retained at native resolution, but consumes roughly 256 MB of native pixel memory plus GPU texture memory and produces a correspondingly large project image file.
- MediaStore orientation values outside 0/90/180/270 are treated as 0 degrees.
- Android requires the user to allow installs from Vast and confirm each package update. A missing network connection or unpublished update manifest leaves the current app unchanged.
- Real-device acceptance for 8K import and physical fling feel requires a connected tablet; host tests cannot reproduce its media decoder, GPU memory pressure, touch controller, or compositor.

## Verification

- 46/46 native workspace/input regression checks pass under Linux/WSL, including checkpoint capture/persistence/return, empty-canvas chrome restoration, and release-direction fling.
- Java 8 compilation and D8 conversion pass against Android API 36.
- Native ARM64/OpenGL/OCR link passes.
- APK inspector verifies version 3.2.16/code 51, exact package members, updater permissions/provider safety, source hashes, APK Signature Scheme v2 integrity, persisted certificate, and baseline signer continuity.
