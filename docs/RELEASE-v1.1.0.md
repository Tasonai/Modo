# Modo v1.1.0 Release Notes

**Version:** 1.1.0  
**Developer:** Atoma  

## New

- **Curated icon packs** (7): Pixel Pure, Material You, Modo Noir, Atoma Mono, Squircle Studio, Editorial Paper, Cyber Hex. One click applies shape, surface, scale, shadow and tint.
- **Shape geometry** (5): Circle, Squircle, Rounded Rect, Pebble, Hexagon, on macOS and Windows.
- Selected style and pack are remembered between launches (macOS: UserDefaults, Windows: `style.cfg`).
- Windows: new "Form & icon packs" card. The window is taller (840 px).
- Labels follow the active pack ("Custom Style" after manual edits); the old "Round" wording is gone.

## Fixes

- Monochrome packs no longer paint full-bleed squircle icons as a solid block: artwork is desaturated instead.

## Known limitations

- The Windows build was compiled but **not run** on a real Windows machine; the new UI card is untested there.
- Hexagon and Pebble shapes use simple geometry (no continuous-curvature squircle yet: Squircle is a rounded rectangle).
- The 840 px Windows window may not fit screens shorter than ~900 px.
