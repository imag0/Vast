# Vast 3.3.0

Package `com.ayomi.infinitecanvas`, version code 55, minimum SDK 26, target SDK 36, ARM64.

## Project library

- Vast now supports up to 64 independent local projects instead of four.
- The native Android project panel lists active projects with favorites first and supports new project creation, names, comma-separated tags, favorites, archiving and duplication.
- Archived projects remain stored locally and can be restored by unarchiving the current project.
- Existing four-project preferences migrate automatically; the canvas, frame, workspace, image and OCR file formats remain compatible.

## Portable projects and recovery

- Export the current project as one `.vast` file through Android's system document picker. Import creates a new project and never overwrites an occupied slot.
- Packages include the six native project documents exactly as stored. Bounded sizes, member uniqueness and per-member checksums reject malformed or truncated packages before commit.
- Every project has ten rotating recovery snapshots. Snapshot creation uses hard links when the app-private filesystem supports them and a durable copy fallback otherwise.
- Restore requires confirmation. Import, restore and duplication use durable transaction markers and same-directory atomic renames; an interrupted operation is rolled back or cleaned up before the slot is exposed.
- Project exports and snapshots stay local. No cloud service or account was added.

## Left-handed controls

- Settings > Display > Accessibility adds a persistent **Left-handed controls** option.
- The compact/expanded rail and context tool panel swap sides and animate from their actual anchored edge.
- Text fields, confirmation dialogs and the system document picker remain real Android Views.

## Verification

- The full host regression matrix covers OCR, persistence, input cancellation, fling behavior, erasing, legacy 3.2.7 compatibility, retained rendering and real PP-OCR fixture inference.
- Product tests cover the 64-slot preference migration, portable package round-trip, rolling snapshot restore, interrupted-restore rollback and interrupted-import cleanup.
- Release inspection verifies APK v2 signing compatibility, exact package contents, version 3.3.0/code 55, ARM64-only native libraries, the non-exported document bridge and the updater-only network permission boundary.
