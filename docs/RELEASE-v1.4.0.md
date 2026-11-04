# Modo v1.4.0 Release Notes

**Version:** 1.4.0  
**Developer:** Atoma  

## New (macOS)

- **Share packs.** My Packs → right-click a pack → *Export Pack…* writes a `.modopack` file (plain JSON). *Import Pack…* adds the packs from such files. Imported packs get new ids, names are capped at 60 characters and every style value is clamped, so a hand-edited or hostile file cannot produce an invalid style. Files from a newer Modo, empty files and non-pack files are refused with a message.

## Tests

- 18 unit tests (was 7): shape geometry, pack files, and **rendering tests that draw real icons and check pixels** (transparent corners, hexagon vs circle, drop shadow, no colour cast in monochrome packs, colour preserved otherwise). Two deliberate breakages (no desaturation, hexagon drawn as a circle) were confirmed to fail the tests.
- 24 in-app self-test checks, including a real `.modopack` export and import on disk.
- Shape geometry moved into ModoKit (`IconShape.path(in:)`) so it is testable.

## Known limitations

- `.modopack` is not registered with macOS: double-clicking a file does not open Modo; use *Import Pack…*. The open/save panels themselves were not clicked through in an automated run (the file logic underneath was).
- Packs export and import on macOS only.
- The Windows build is unchanged since 1.2.0 and still **not run** on a real Windows machine.
