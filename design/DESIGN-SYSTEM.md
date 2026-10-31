# IconMaker Design System (`design/DESIGN-SYSTEM.md`)

IconMaker follows Apple's macOS 26 Human Interface Guidelines (`mac-design`) and the 4 pt `Space` grid (`layout-grid`).

## 1. Accent & Semantic Colors (`UI/Theme.swift`)
- Default accent: `ThemeStore.defaultID = "system"`, with brand fallback `Google Blue` (`#0A84FF`) when macOS Accent Color is set to Multicolor.
- Never hard-code accent hex values in views; use `.tint`, `Palette.accent`, `Palette.accentText`, and `Palette.onAccent`.
- Use semantic system surfaces (`.background`, `.background.secondary`, `.quaternary`, `.separator`) so light mode, dark mode, and Increase Contrast adapt automatically.

## 2. Spacing & Layout Grid (`UI/Grid.swift`)
- Every padding, spacing, and frame size sits on the 4 pt grid via `Space` tokens:
  - `Space.xs` (4 pt), `Space.sm` (8 pt), `Space.md` (12 pt), `Space.lg` (16 pt), `Space.xl` (20 pt), `Space.xxl` (24 pt), `Space.section` (32 pt), `Space.margin` (20 pt).
- Custom detail containers mark their bounds with `.gridArea("detail", inset: Space.margin)` and stacked cards with `.gridProbe(..., area: "detail")`.

## 3. Radii & Color Picker Tokens (`UI/Tokens.swift`)
- `Radius.xs` (6 pt) for small badges, `Radius.sm` (8 pt) for sidebar selection pills and compact controls, `Radius.row` (10 pt) for row highlights, `Radius.md` (12 pt) for inspector cards, `Radius.lg` (16 pt) for hero preview stages.
- `ColorSwatchToken.presets` provides curated Google Pixel & Material You surface swatches.
- `EyedropperPicker` bridges `NSColorSampler` (the native macOS magnifying-glass eyedropper) and SwiftUI `ColorPicker` to `RoundIconStyle.customHex`.
