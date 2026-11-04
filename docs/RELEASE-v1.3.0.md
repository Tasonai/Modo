# Modo v1.3.0 Release Notes

**Version:** 1.3.0  
**Developer:** Atoma  

## New (macOS)

- **Auto re-apply after app updates.** macOS drops a custom icon when an app is updated. Modo records the version of every app it styles and, on its own launch and whenever an app launches, styles the app again if its version changed or its custom-icon file vanished. The current style is used. Toggle: Settings → "Re-apply Modo icons after an app is updated" (on by default). Restoring an app's original icon stops tracking it.

## Fixes

- App version is read straight from `Info.plist`, not from the cached `Bundle`, so a freshly updated app is noticed.

## Tests

- 7 unit tests, 21 in-app self-test checks, including an end-to-end run on a throwaway `.app` (icon file removed, version bumped).

## Known limitations

- Detection runs when Modo is open or launched; an update that happens while Modo is not running is caught the next time Modo starts.
- Apps whose icon went through the read-only alias are re-applied on version change only.
- The Windows build is unchanged since 1.2.0 and still **not run** on a real Windows machine.
