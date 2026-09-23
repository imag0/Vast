# Vast 3.2.12 UI review

Reviewed together on the 3000x1920 HONOR tablet, dark theme, user's existing 140%
canvas-control scale. Images are local QA artifacts, excluded from the distributable
source archive. The dark scheme has warm text, cool charcoal panels and muted blue
tool accents. There is no blur. Android selection/IME UI retains the device's own
appearance; it is intentionally not restyled into a canvas imitation.

| Requested state | Image under artifacts/navigation-pass/current | Review |
| --- | --- | --- |
| Clean canvas | scene.png | Unobstructed canvas and quiet project label; no Vast branding. |
| Pen selected | pen-final.png | Blue pen state, understated quarter-circle. |
| Eraser selected | eraser-final.png | Coral eraser state, matching size; no idle reticle. |
| Color collapsed | pen-final.png | Current-color quarter-circle at opposite corner, no redundant icon. |
| Color expanded | picker-final.png | Native panel near corner; wheel, brightness, current/previous/recent colors fit. |
| Settings | settings-final.png | Stable category rail, warm labels, restrained translucent surface. |
| Search | search-final.png and search-input-final.png | Consistent panel; real EditText visibly shows query. |
| Text editing | editing-final.png | Mixed English/Chinese/emoji, native selection handles and scrollable text. |
| Object selection | selection-final.png | Bounds and four-action native toolbar; no collision with corner controls. |
| Context menu | context-final.png | Same typography/radius/opacity, short relevant actions. |
| Focus | focus-final.png | Main chrome removed; project context and two corner tools remain. |

The later `clean-final.png` includes an intentional Undo toast, so `scene.png` is
the clean-canvas review image. The dense white area in QA images is the pre-existing
short-stroke stress grid, not a renderer fill fault. `zoom20-final.png` shows its
magnified strokes and the note's cached bitmap text at maximum zoom.

Joint assessment: corner geometry and native surfaces are coherent; modal panels
dim the canvas while nonmodal controls do not. The picker is physically separated
from the left tool. No app-owned editor text is hidden under duplicate canvas
fields. Android's floating text-selection bar may overlap the dialog title while
selection is active; it is owned/positioned by Android, not a duplicate Vast field.
Native menus are text-led, avoiding unrelated icon families. The existing pen and
eraser symbols remain visually distinct for tool-state recognition.

Interaction checks: finger and stylus wheel input updated the current swatch and
canvas quarter-circle, Previous color restored the entry color, Done dismissed
the popup, and ordinary canvas/native menu actions worked afterward. A 12-cycle
rapid Settings/category/dismiss test passed. Input regression tests cover contact
ownership/cancellation separately. A full TalkBack/navigation-inset matrix and
interrupt-at-every-animation-frame test remain unverified; animations are coded
to cancel/retarget and release touch ownership when closing.

Not claimed: every theme/device/orientation screenshot pass or physical-animation
frame pacing. The user subsequently confirmed smooth physical writing without
noticeable jiggle or growing delay; see RELEASE_3.2.12.md.
