# IconMaker

Native macOS app that turns every installed app icon into a crisp circular adaptive icon in Google Pixel / Material You style, previews Before and After side by side, applies circular icons to Mac apps or aliases in one click, and exports `.icns`, `.ico`, and `.png` icon packs.

## Replaces
Manual Photoshop/Figma circular masking and Get Info icon copy-pasting for every installed app.

## Main jobs
1. Scan installed apps in `/Applications` and `~/Applications` (plus drag-and-drop apps or images) and preview their circular Google Pixel / Material You icons live.
2. Customize the circular disc style (Google White, Material You Pastel auto-tint, Dark Material, Adaptive Edge, or Custom Color), glyph scale, elevation shadow, and full-bleed square masking.
3. Apply circular icons in one click via `NSWorkspace.shared.setIcon`, restore original icons at any time, or export multi-resolution `.icns`, `.ico`, and `1024×1024` `.png` icon packs.

## Data
- `AppIconEntry`: `uid: UUID`, `name: String`, `bundleIdentifier: String`, `appPath: String`, `category: String`, `isSelected: Bool`, `isApplied: Bool`, `customScale: Double?`, `customHexColor: String?`, `addedAt: Date`

## Shape
Sidebar window (`NavigationSplitView`: sidebar categories, middle icon grid/list, trailing inspector for live style controls and per-app overrides), macOS 26 or later.
Local data. Works offline. No accounts.

## System hooks
Drag and drop `.app` bundles or images in and out, Finder Reveal, `NSWorkspace.shared.setIcon` custom icon application and rollback.

## Reference
none

## Look
Native macOS 26 with Liquid Glass navigation chrome, SF Symbols, system typography, and a Google Blue default accent color.

## Not in version one
- Cloud sync or user accounts
- Kernel/SIP system bundle binary modification
- Paid icon store

## Design plan
1. **Archetype**: Sidebar window (`NavigationSplitView` with sidebar filter pages, main app icon list/grid, and detail inspector).
2. **Information architecture**:
   - All Apps (`all`)
   - Applied Round Icons (`applied`)
   - Custom Overrides (`customized`)
   - Exported Packs (`exported`)
3. **Command map**:
   - Scan Applications: `Cmd+R`
   - Apply Round Icons to Selected: `Cmd+Return`
   - Restore Original Icons: `Cmd+Shift+R`
   - Export Icon Pack: `Cmd+E`
   - Add Application or Image: `Cmd+O`
4. **Empty states**: `ContentUnavailableView` inviting the user to scan `/Applications` or drop an `.app` bundle.
5. **Feeling**: Crisp, tactile, instant visual feedback.
