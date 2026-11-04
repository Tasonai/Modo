# Modo v1.2.0 Release Notes

**Version:** 1.2.0  
**Developer:** Atoma  

## New

- **Curated icon packs** (7): Pixel Pure, Material You, Modo Noir, Atoma Mono, Squircle Studio, Editorial Paper, Cyber Hex.
- **Shapes** (5): Circle, Squircle, Rounded Rect, Pebble, Hexagon on macOS and Windows. Squircle is a true superellipse (n = 5): continuous curvature, a close approximation of the macOS outline, not Apple's exact curve.
- **My Packs** (macOS): save the current style under a name, switch to it in one click, delete from the context menu.
- **Export Pack** now writes a `Modo-<pack>` folder and a matching `.zip`.
- **Apply All** asks for confirmation and names the pack and app count.
- The selected style, pack and custom packs are remembered between launches.
- Windows: new "Form & icon packs" card (window height 840 px).
- Labels follow the active pack ("Custom Style" after manual edits).

## Fixes

- Monochrome packs no longer paint full-bleed icons as a solid block or leave colour casts; artwork is converted to grayscale with its alpha kept.

## Known limitations

- The Windows build was compiled but **not run** on a real Windows machine; the new card, shapes and grayscale path are untested there.
- My Packs and the Apply All confirmation exist on macOS only.
- The 840 px Windows window may not fit screens shorter than ~900 px.
