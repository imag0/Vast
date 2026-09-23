# Menu and motion review — 3.2.11

Inventory is based on `android_ui.inc`, `VastUi.java`, the canvas input router,
and spatial overlays. Device screenshot review is recorded separately below;
an implementation review is not evidence of physical-pen testing.

| Surface | Purpose and change needed |
| --- | --- |
| Header / edge rail | Common navigation, quiet native rows; retain enabled Undo/Redo; group creation, navigation and preferences. |
| Settings | Replace the all-settings list with persistent category navigation; compact content and explicit selected category. |
| Drawing / pressure | Group smoothing and pressure range; retain only existing controls. |
| Canvas | Grid and overview behavior; put performance preference behind an Advanced section. |
| Appearance / custom theme | Named presets, subordinate custom colors, explicit Back path. |
| Stylus button | Bindings only visible when enabled. |
| Handwriting Search | Toggle, concise privacy explanation, status, rebuild action. |
| Add | Short, grouped native actions for existing content types. |
| Search / query editor | Visible native text input, matching results, preserved return context. |
| Projects / rename | Current project selected; separate rename and duplicate from project navigation. |
| Layers | One compact visibility/lock row per layer, not three stacked rows. |
| Frames / places | Separate lists from selected-item actions; distinguish properties, organization and deletion. |
| Selection toolbar / More | Only supported actions for selected type; distinguish irreversible deletion. |
| Photos / import gallery | Native previews; selection-dependent actions; confirm non-undoable deletion. |
| Dimensions / calibration | Contextual value and native numeric input; confirm deletion. |
| Shapes | Clear active shape; consistent native choices. |
| Text / sticky / name editors | Android EditText and IME resize; fixed dismiss/save actions. |
| Confirmation dialogs | Native, concise, only for operations that cannot be recovered through Undo. |
| Spatial color control | Existing direct-manipulation color wheel; gesture follows finger without animation delay. |
| Radial tool chooser | Spatial marking gesture, not a conventional settings panel; quiet elapsed-time settling. |
| Minimap / frame creation / presentation | Canvas-spatial navigation; retain direct finger tracking and immediate input. |
| Pen/eraser corner toggle | Immediate state feedback; contact reticle independent of selected tool display. |

Motion policy: native content uses short, cancellable alpha/translation property
animations. State changes are immediate; closing never waits for opening. Canvas
spatial transitions use elapsed time without spring overshoot. No View hierarchy
is reconstructed by animation callbacks. Native panels do not require full canvas
texture updates while their own Views animate.

## Device visual review

Reviewed on the physical HONOR YLE-W09 at 3000 × 1920. Evidence is in
`artifacts/eraser-pass/current`; screenshots are deliberately not included in the
source bundle. Input was driven through Android accessibility actions and
injected MotionEvents, not a hand-operated physical pen.

| Surface reviewed | Evidence and outcome |
| --- | --- |
| All five Settings categories | `settings-drawing`, `settings-canvas`, `settings-ocr`, `settings-appearance`, `settings-stylus`; selected navigation is clear, explanatory OCR text concise. |
| Final Settings / pressure response | `settings-pressure-final`; seven compact control rows fit without clipping. Increase/decrease pressure response executed successfully and restored the prior choice. Fixed category-dependent dialog height jumps. |
| Custom theme | `custom-theme-final`; repeated review after replacing the oversized vertical list with left-side roles and a 6-column swatch palette. Back and Close are visible. |
| Layers | `layers`; visibility and lock align in one row. Final-build lock/unlock probe confirmed the displayed state updates. |
| Add, Frames, frame properties/actions, saved places | `add`, `frames`, `frame-properties`, `frame-actions`, `places`; sections separate content types and properties. Selected frame details require scrolling, but navigation and dismissal stay fixed. |
| Projects and rename | `projects`, `rename`; active project is checked, rename input visible, return context restored after IME dismissal. |
| Search and query | `search-input`, `search-results`; native query text visible and results reachable. This UI review does not establish accuracy on every handwriting style. |
| Text/sticky editor | `text-editor`; long multiline Unicode/emoji input scrolls with the caret visible and fixed Save/Cancel actions. |
| Shapes / selection / deletion | `shapes`, `selection-more`, `delete-confirmation`; selected shape marked, More contains relevant actions, irreversible deletion is separated and confirmed. Confirmation was canceled. |
| Dimensions and calibration | `dimensions`, `calibration`; value and unit visible, native input showed 125.5 while editing; canceled without applying a new calibration. |
| Photo tools and gallery | `photos`, `photo-gallery`; concise selection hint and uncluttered native thumbnail grid. |
| Color picker | `color-picker`; direct spatial color controls, clear Done action, no overlapping labels. |
| Radial menu | `radial-final`; initial review caught an instruction crossing tool labels. Removed the long default instruction and repeated capture; tool names now remain unobstructed. |
| Focus, minimap and presentation | `focus`, `pinch-final`, `presentation`; Focus hides tool chrome, overview remains a spatial navigator outside Focus, frame presentation provides count and Exit. |
| Reticle ownership | `finger-contact`, `pinch-final`, `contact-contact`, `contact-released`, `hover-contact`, `cancel-contact`, `cancel-released`; contact-only reticle and navigation behavior visible. |

The old separate pressure popup no longer has a primary navigation entry; its
useful pressure-response choice is now exposed in Drawing settings. Legacy
canvas fallback menus remain for adapter failure/host tests, but normal menus on
this tablet are Android Views. Spatial color, radial, minimap and presentation
overlays stay in the canvas.

## Motion and interaction evidence

`rapid-ui.txt`: 12 Settings open/close cycles and 60 category changes in 5.055 s,
with requested 25–40 ms gaps and no stale dialog afterward. No animation callback
reconstructs a hierarchy; row reconstruction is tied to state/category changes.
Initial failed replays were test-harness problems (an overlapping injected pinch
and a tap with UNKNOWN tool type); corrected sequential, FINGER-typed replays
passed. The corrected two-finger run is `pinch-final.txt`.

`rapid-ui-gfxinfo.txt`: 702 View frames, 48 janky frames (6.84%), median 5 ms,
90th percentile 8 ms, 95th percentile 17 ms, 99th percentile 25 ms, no slow bitmap
uploads. This stress pass is not jank-free. Native canvas frame presentation is
not measured by these View statistics.

`display-active.txt` reports an active 120.00001 Hz display/render rate after the
per-window preference was added; idle capture can report 60 Hz. Host tests also
check equal elapsed-time settling at 60, 90 and 120 Hz. Neither result substitutes
for subjective physical-pen/palm and sustained 120 Hz acceptance.

The user subsequently confirmed the physical Eraser check: one/two-finger
navigation, contact/release reticle behavior and responsiveness work correctly.
Remaining review limits: physical IME composition and OEM keyboard layouts,
extended palm scenarios, long-session thermal behavior, and zero-jank menu
presentation are not certified. See `RELEASE_3.2.11.md` for performance scope and
the explicitly outstanding acceptance work.
