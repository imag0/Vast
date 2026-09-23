# Vast 3.2.14 - translucent native controls

Version 3.2.14 / code 49, package `com.ayomi.infinitecanvas`. Update-compatible signing identity and document formats are unchanged.

## Design and implementation

`VastStyle.java` centralizes tinted gradient surfaces, fine highlight borders, neutral text, vector icons, selected/pressed/focused states and native editor styling. Navigation and selection actions live inside unified floating pills. The side rail stays useful as a slim icon column and expands to labels. Tool trays, search, settings sections, dialogs, switches, unit selection and the color picker share the same materials and spacing. Narrow settings windows use horizontally scrollable categories.

The radial menu retains its gesture ownership and six actions, with rounded arc-distance petals and a strongly highlighted active tool. Its visual material is computed only in bounded menu regions. Corner controls share the neutral palette while keeping a real current-color swatch. These spatial controls remain renderer-owned; ordinary controls and all text input remain Android Views.

Native EditText keeps IME composition, selection, cursor, Unicode, wrapping, validation, filters and resize behavior. No Compose migration, screenshot-based blur, backdrop readback, external image assets or full-screen realtime filter was added. Popup animations use Android property transforms. The existing fling/drawing path is unchanged; UI reconstruction remains state-driven rather than camera-frame-driven.

## Validation

- Java/dex and native ARM64 builds pass; signed APK inspection passes identity, version, signature, packaged-source hashes, SDK/ABI and offline permission checks.
- Host input regressions: 78 checks, zero failures, including fling direction/clamping, 60/90/120 Hz integration, immediate touch/stylus cancellation and delayed radial progress.
- Product-polish regressions: 16 checks, zero failures, including native JSON, UTF-8 text, cancellation and persistence.
- Retained-rendering suite passes: 20k-sample bounded append, camera movement with no ink/text/photo geometry uploads, historical samples and idle persistence.
- Erase-render regressions pass: bounded damage uploads, no upload on misses, translucent-marker compositing, cancellation and elapsed-time motion.
- Final installed build passes 12 rapid settings open/close cycles with 60 category changes, plus the complete 18-capture visual flow. Installed package reports version 3.2.14/code 49; recent logs show no fatal or UI-show errors in the checked window.
- Tablet visual review uses native accessibility actions and explicit finger MotionEvents. See `UI_REVIEW_3.2.14.md` for screenshot coverage and limitations.

The preceding physical writing and eraser acceptance belongs to 3.2.12/3.2.11, not a new physical-pen measurement for this styling release. Review screenshots and synthetic interactions cannot establish zero-jank 120 Hz presentation.
