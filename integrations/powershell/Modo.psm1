# Modo PowerShell Integration Module
# Provides native cmdlets and tab-completion for Modo CLI on Windows

function Get-ModoExecutable {
    $modoCmd = Get-Command modo.exe -ErrorAction SilentlyContinue
    if ($modoCmd) {
        return $modoCmd.Source
    }

    $localAppPath = Join-Path $env:LOCALAPPDATA "Programs\Modo\Modo.exe"
    if (Test-Path $localAppPath) {
        return $localAppPath
    }

    $portablePath = Join-Path $PSScriptRoot "..\..\windows-native\Modo.exe"
    if (Test-Path $portablePath) {
        return (Resolve-Path $portablePath).Path
    }

    throw "Modo.exe not found. Ensure Modo is installed or added to PATH."
}

function Set-ModoStyle {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$App,

        [Parameter(Position = 1)]
        [ValidateSet("squircle", "circle", "roundedRect", "pebble", "hexagon")]
        [string]$Shape = "squircle",

        [Parameter(Position = 2)]
        [ValidateSet("pastel", "dark", "white", "clear")]
        [string]$Background = "pastel",

        [Parameter()]
        [double]$Scale = 1.0
    )

    $exe = Get-ModoExecutable
    & $exe style $App $Shape $Background
}

function Restore-ModoIcon {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$App
    )

    $exe = Get-ModoExecutable
    & $exe restore $App
}

function Set-ModoFolder {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$Path,

        [Parameter(Position = 1)]
        [ValidateSet("appleBlue", "graphite", "purple", "teal", "sunsetOrange", "emerald", "coralRed", "amberGold", "obsidian")]
        [string]$Color = "appleBlue"
    )

    $resolved = (Resolve-Path $Path).Path
    $exe = Get-ModoExecutable
    & $exe folder $resolved $Color
}

function New-ModoBatch {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$InputFolder,

        [Parameter(Position = 1)]
        [string]$OutputFolder = "",

        [Parameter(Position = 2)]
        [string]$Style = "squircle"
    )

    $inResolved = (Resolve-Path $InputFolder).Path
    $exe = Get-ModoExecutable
    if ($OutputFolder) {
        $outResolved = (Resolve-Path $OutputFolder -ErrorAction SilentlyContinue)
        if (-not $outResolved) {
            $outResolved = $OutputFolder
        } else {
            $outResolved = $outResolved.Path
        }
        & $exe batch $inResolved $outResolved $Style
    } else {
        & $exe batch $inResolved
    }
}

function New-ModoShowcase {
    [CmdletBinding()]
    param(
        [Parameter(Position = 0)]
        [string]$OutputFile = "showcase.png"
    )

    $exe = Get-ModoExecutable
    & $exe showcase $OutputFile
}

function Export-ModoIcon {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$App,

        [Parameter(Position = 1)]
        [ValidateSet("ico", "png", "svg", "favicon", "xcode", "all")]
        [string]$Type = "all",

        [Parameter(Position = 2)]
        [string]$OutputFolder = ""
    )

    $exe = Get-ModoExecutable
    if ($OutputFolder) {
        & $exe export $App $Type $OutputFolder
    } else {
        & $exe export $App $Type
    }
}

function Reset-ModoIconCache {
    [CmdletBinding()]
    param()

    $exe = Get-ModoExecutable
    & $exe shell clean-cache
}

Export-ModuleMember -Function Set-ModoStyle, Restore-ModoIcon, Set-ModoFolder, New-ModoBatch, New-ModoShowcase, Export-ModoIcon, Reset-ModoIconCache
