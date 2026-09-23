# Vast 3.2.19

Package `com.ayomi.infinitecanvas`, version code 54, minimum SDK 26, target SDK 36, ARM64.

## Curated themes

Appearance now contains 24 complete presets, grouped in the native Android Settings panel:

- **Essentials:** Dark, OLED, Paper, Blueprint, Warm.
- **Dark collection:** Graphite, Midnight, Forest, Aubergine, Espresso, Nord, Carbon.
- **Light collection:** Porcelain, Sand, Sage, Lavender, Ice.
- **Chromatic collection:** Ocean, Mint, Rose, Sunset, Copper, Neon, Contrast.

Every preset defines all 17 theme roles together: canvas, grid, major grid, axes, three panel surfaces, borders, primary and secondary text, accent, CAD cyan, violet, warning, danger, success, and deepest shadow. The original five palettes remain byte-for-byte identical, so existing saved presets and customized themes retain their appearance.

## UX and compatibility

- The theme list is grouped and vertically scrollable using real Android Views.
- Selecting a theme rebuilds the dialog in the new light or dark Android context while retaining the current scroll position, so exploring lower themes does not jump back to the top.
- The selected preset remains visibly marked; any manual role customization naturally becomes a custom theme.
- The compact native fallback panel exposes the same 24 choices in a three-row grid.
- Theme persistence remains inside the existing metadata structure; no canvas, workspace, image, frame, or OCR format changes.

## Verification

- Automated checks require at least 20 presets, unique names, unique complete role sets, collision-free actions, primary-text contrast of at least 4.5:1 on canvas and panel surfaces, and secondary-text contrast of at least 3:1 on the canvas.
- Persistence coverage saves and reloads a newly added preset through the current metadata format.
- Native ARM64/OpenGL/OCR linking and Java 8/D8 compilation pass.
