# Modo Alfred Workflow

Trigger Modo from Alfred using keyword `modo`:

- `modo <app_name>` — Style selected application with adaptive squircle.
- `modo restore <app_name>` — Restore original icon.
- `modo folder <folder_path>` — Tint selected folder in Finder.

## Script Filter Command
```bash
~/.local/bin/modo style "{query}" --shape squircle --bg pastel
```
