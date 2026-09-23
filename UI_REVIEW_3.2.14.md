# Vast 3.2.14 visual review

Reviewed on the connected 3000 x 1920 Honor tablet using QA Project 4. Existing document objects were not edited or deleted. Search navigation, tools and theme selection changed during review; Dark was restored. Screenshot helper: `tests/DeviceVisualReview.java` (shell-only QA code, not shipped in the app).

## Reference comparison

The first reference informs one coherent nav pill with integrated line icons and labels, rather than separate floating buttons. The third informs the slim icon column and expanded label rail. The second informs curved radial petals with a pale, high-contrast active tool. Vast retains its six-tool gesture layout rather than copying the reference's four-action geometry. Materials are low-cost tinted translucency and gradients, not optical frosted blur.

## Captures

Device captures are under `artifacts/glass-review/vast-glass-review-final/` (development evidence excluded from the source ZIP):

| Capture | Review |
| --- | --- |
| 01-nav-and-expanded-tray | Unified pill, aligned icons/labels, selected Map state, expanded rail |
| 02-collapsed-tray | Slim icon-only rail, distinct selected state, expand affordance |
| 03-settings-drawing | Category hierarchy and readable label/value/stepper alignment |
| 04-settings-canvas | Quiet grouped sections and aligned native steppers |
| 05-settings-handwriting | Readable OCR explanation, native switch and scroll content |
| 06-search | Unicode result labels, consistent icons and panel material |
| 07-search-editor-ime | Native editor shows English and Chinese input; tablet IME affordance visible |
| 08-add-panel / 09-tool-tray | Shared panel language and clearly selected shape |
| 10-color-expanded / 11-color-collapsed | Color wheel and compact current-color corner swatch |
| 12-radial / 13-radial-active | Curved lobes and bright active Pen petal |
| 14-settings-paper / 15-paper-chrome | Light palette readability and selected states |
| 16-selection-controls | Matching context pill over existing ink/text |
| 17-selection-context | Readable actions despite busy background showing faintly through |
| 18-settings-over-content | Main controls retain contrast over overlapping QA text and ink |

The initial radial capture was invalid because the test synthesized an unspecified tool type. The helper now supplies explicit finger properties and captures the real open/active menu. English/Chinese text is entered into the native editor and canceled; it is not committed to the document.

## Scope and limits

Screenshots were inspected for clipping, spacing, contrast and consistency with the three reference images. The deliberately overlapping QA canvas notes visible behind panels are existing document objects, not overlapping editable controls. No photo-background contrast sweep or exhaustive phone/multiwindow matrix was performed. Device tests are synthetic, not physical digitizer latency measurements; final feel remains a user acceptance check.
