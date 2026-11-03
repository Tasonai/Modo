# Architecture Review: IconMaker (BAWA Check-in)

**Date:** 2026-11-04  
**Skill:** `improve-architecture` (BAWA Architecture Standard)

---

## 1. Survey & Codebase Telemetry

### File Size Hot Spots:
| File | Lines | Status / Signal |
| :--- | :---: | :--- |
| `windows-native/iconmaker_win32.cpp` | **2,994** | 🔴 **CRITICAL HOT SPOT** (> 1,000 lines rule). Holds GDI+ engine, Windows Explorer registry patcher, UI hit-testing, shortcut resolver, and event loop all in one translation unit. |
| `Sources/IconMaker/ContentView.swift` | **666** | 🟡 **WARNING** (> 600 lines rule). Holds main split view, navigation bar, list, detail card, color picker, and pan gestures. |
| `Sources/IconMaker/IconRenderer.swift` | **545** | 🟢 Healthy. Pure rendering pipeline with clear mathematical transforms. |
| `Sources/IconMaker/Models.swift` | **336** | 🟢 Healthy. SwiftData model + observable state. |

### Commit Velocity (Last 30 days):
- `windows-native/iconmaker_win32.cpp`: **9 commits** (highest churn).
- `Sources/IconMaker/ContentView.swift`: **7 commits**.

---

## 2. Refactoring Candidates

### Candidate 1: Modularize Windows Native Monolith (`iconmaker_win32.cpp`) [Strong]

- **Files:** `windows-native/iconmaker_win32.cpp` (2,994 lines)
- **Signal:** Single C++ file > 1,000 lines (approaching 3,000 lines), mixing system shell integrations with UI drawing.
- **Problem:**
  Any change to UI styling (buttons, colors, fonts) forces re-reading and re-compiling the entire Windows shell integration engine (registry CLSID modifiers, `.lnk` shell link resolvers, ICO multi-layer encoders, and icon cache purge routines).
- **Proposal:**
  Extract into 3 cohesive modules with clear seam interfaces:
  1. `icon_engine.h / .cpp`: GDI+ Bitmap processing, color extraction, auto-trim, and multi-size `.ico` generation.
  2. `shell_integration.h / .cpp`: Registry CLSID patcher, `IShellLinkW` resolution, backup management, and `SHChangeNotify` / Explorer restart routines.
  3. `ui_canvas.h / .cpp` (or clean `main.cpp`): Window procedure, hit-testing, and Fluent UI controls rendering.
- **Interface Sketch:**
  ```cpp
  // shell_integration.h
  namespace Shell {
      bool ResolveShortcut(const std::wstring& lnk, std::wstring& target, std::wstring& icon);
      bool ApplyRoundIcon(AppEntry& app, const std::wstring& icoPath, bool* outNeedsAdmin);
      bool RestoreOriginal(AppEntry& app);
      void PurgeExplorerIconCache(bool restartExplorer);
  }
  ```
- **Benefits:**
  - High locality: UI layout changes do not risk touching low-level registry and COM pointers.
  - Testability: Shell integration can be independently compiled and verified.
- **Proof of Same Behavior:**
  - `python3 windows-native/build_windows_installers.py` compiles clean PE32+ binaries with zero overlay and identical functionality.
- **Plan:**
  - Commit 1: Extract `shell_integration.h` and helper types without changing logic.
  - Commit 2: Extract `icon_engine.h` for GDI+ icon transforms.
  - Commit 3: Streamline `iconmaker_win32.cpp` to pure UI/event loop (< 1,000 lines).

---

### Candidate 2: Split SwiftUI `ContentView.swift` into Feature Views [Worth exploring]

- **Files:** `Sources/IconMaker/ContentView.swift` (666 lines)
- **Signal:** File passes 600 lines limit.
- **Problem:**
  `SidebarView`, `AppListView`, `AppRowView`, and `DetailInspectorView` are all declared in a single file alongside drop providers and toolbar menus.
- **Proposal:**
  Move detail subviews into dedicated files under `Sources/IconMaker/Views/`:
  - `Views/DetailInspectorView.swift`: Comparison cards, geometry controls, and pan gesture state.
  - `Views/AppListView.swift`: Category pills, search empty overlay, and row context menus.
- **Benefits:**
  - Faster incremental Swift compiler type-checking (`swift-frontend`).
  - Clearer view boundaries and simplified `@Environment(AppModel.self)` observation.
- **Proof of Same Behavior:**
  - `swift test` passes in 0.005s.
  - Headless snapshots (`APP_SNAPSHOT=/tmp/...`) match pixel-for-pixel with previous build.

---

### Candidate 3: Core Domain Extraction (`IconMakerKit`) [Speculative]

- **Files:** `Sources/IconMaker/IconRenderer.swift` -> `Sources/IconMakerKit/IconRendererKit.swift`
- **Signal:** Pure rendering algorithm lives in app target instead of library target.
- **Problem:**
  Currently `IconMakerKit` only holds `ItemTitle.swift`. The rendering mathematics (Material RGB blending, circular mask calculation) could be shared or headless-tested without AppKit GUI bindings.
- **Benefits:**
  - Unit tests can run in headless CLI environments without macOS window server.

---

## 3. Top Recommendation

**Candidate 1 (Modularize Windows Native Monolith)** is our **Top Recommendation [Strong]**.  
`windows-native/iconmaker_win32.cpp` is 2,994 lines long, accounting for ~47% of the entire codebase in a single file. Breaking it into `shell_integration` + `icon_engine` + `ui_canvas` will dramatically improve maintainability and follow the BAWA architectural guidelines.
