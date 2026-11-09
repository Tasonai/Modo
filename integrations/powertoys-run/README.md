# Modo Integration with Microsoft PowerToys Run

Integrate Modo CLI into Microsoft PowerToys Run to quickly style applications, folders, and generate presentation graphics.

## Prerequisites

1. Install Modo for Windows (`Modo-Windows-Setup.exe` or portable version).
2. Ensure `Modo.exe` or `modo.cmd` is located in your system `PATH` (the installer automatically handles this).
3. Ensure Microsoft PowerToys is installed and PowerToys Run is enabled (default shortcut: `Alt + Space`).

## Usage via PowerToys Run

Open PowerToys Run with `Alt + Space`, type `>` to activate the Windows Terminal/Shell plugin, and invoke `modo`:

### Quick Commands

- `> modo style "Google Chrome" squircle`
- `> modo style "Visual Studio Code" circle pastel`
- `> modo restore "Slack"`
- `> modo folder "D:\Projects\Modo" emerald`
- `> modo showcase "C:\Users\User\Desktop\banner.png"`
- `> modo shell clean-cache`

## Keyboard Shortcut Automation

You can also bind global shortcuts to specific Modo actions using PowerToys Keyboard Manager (e.g., mapping `Win + Alt + I` to launch Modo or run a quick style script).
