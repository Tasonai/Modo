# Modo PowerShell Module

Native PowerShell cmdlets and tab-completion for automating Modo on Windows 10 and 11.

## Installation

Import the module directly in your current session:

```powershell
Import-Module ./integrations/powershell/Modo.psm1
```

Or add it permanently to your PowerShell `$PROFILE`:

```powershell
Add-Content $PROFILE "`nImport-Module '$((Resolve-Path ./integrations/powershell/Modo.psm1).Path)'"
```

## Available Cmdlets

### 1. Set-ModoStyle
Applies adaptive styling to any desktop shortcut, executable, or installed application.
```powershell
Set-ModoStyle -App "Google Chrome" -Shape squircle -Background pastel
Set-ModoStyle -App "Visual Studio Code" -Shape circle -Background dark
```

### 2. Restore-ModoIcon
Reverts any customized icon back to its original system state.
```powershell
Restore-ModoIcon -App "Google Chrome"
```

### 3. Set-ModoFolder
Customizes any directory via `desktop.ini` with custom color themes and badges.
```powershell
Set-ModoFolder -Path "D:\Projects\Modo" -Color emerald
```

### 4. New-ModoBatch
Converts all images in a folder into multi-size `.ico` bundles and 1024px master `.png` files.
```powershell
New-ModoBatch -InputFolder "C:\Assets\Logos" -OutputFolder "C:\Assets\Icons" -Style squircle
```

### 5. New-ModoShowcase
Generates a studio-grade 1920x1080 hero presentation banner for READMEs and portfolios.
```powershell
New-ModoShowcase -OutputFile "C:\Assets\banner.png"
```

### 6. Export-ModoIcon
Exports developer bundles (Xcode Asset Catalog, Web Favicon Kit, vector SVG, or all formats).
```powershell
Export-ModoIcon -App "Slack" -Type xcode -OutputFolder "C:\Export"
Export-ModoIcon -App "Figma" -Type favicon -OutputFolder "C:\WebAssets"
```

### 7. Reset-ModoIconCache
Forces Windows Explorer to purge and rebuild its icon cache.
```powershell
Reset-ModoIconCache
```
