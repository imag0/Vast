# Vast 3.2.18

Package `com.ayomi.infinitecanvas`, version code 53, minimum SDK 26, target SDK 36, ARM64.

## Changes

- Projects now contains a native Android **Clear project** danger action. Its confirmation names the irreversible scope explicitly; accepting it removes ink, highlighter, photos, CAD dimensions, text, shapes, frames, bookmarks, groups, selections, and undo history while retaining the current project slot, name, and settings.
- The Eraser tools panel now offers two distinct native choices: **Stroke eraser** and **Erase everything**.
- **Erase everything** removes unlocked ink/highlighter strokes and unlocked photos, CAD dimensions, text, shapes, and frame borders touched by the physical stylus. A complete stylus drag is recorded as one mixed-object undo step; undo and redo restore or remove the same objects together.
- Finger gestures remain camera navigation in both eraser modes. Physical eraser and stylus side-button quick erase remain stroke-only to avoid broad deletion from a hardware shortcut.
- Layer locks and per-object locks are honored. A frame is removed only when its border is touched, so erasing an object inside a frame does not implicitly delete the surrounding frame.

## Persistence and safety

- Clear project writes every empty project store immediately and rebuilds the derived local handwriting index from the resulting empty document.
- Universal eraser cancellation restores all objects removed during the in-progress gesture and creates no history entry.
- Universal eraser undo/redo dirties and saves the corresponding ink, photo, CAD, frame, and workspace stores without rebuilding object geometry merely because the camera moves.
- Clear project is intentionally not undoable; the native Android confirmation is required before the action reaches the canvas engine.

## Verification

- The complete host matrix passes, including OCR, malformed persistence fixtures, input ownership/cancellation, eraser spatial/damage tests, retained rendering, real PP-OCR fixtures, and the legacy product suite.
- New regression coverage verifies mixed-object erase, cancellation, one-step undo/redo, locked-object preservation, full project clearing, project identity preservation, and empty reload from disk.
- Native ARM64/OpenGL/OCR linking and Java 8/D8 compilation pass.
