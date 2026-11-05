@echo off
setlocal
set "MODO_EXE=%~dp0Modo-Windows-Portable.exe"
if not exist "%MODO_EXE%" (
    set "MODO_EXE=%LOCALAPPDATA%\Programs\IconMaker\IconMaker.exe"
)

if "%~1"=="" goto help
if "%~1"=="--help" goto help
if "%~1"=="-h" goto help
if "%~1"=="help" goto help
if "%~1"=="--version" goto version
if "%~1"=="-v" goto version
if "%~1"=="version" goto version

"%MODO_EXE%" %*
exit /b %ERRORLEVEL%

:version
echo Modo 1.6.0 (Windows x64) — Native Adaptive Icon Studio by Atoma
exit /b 0

:help
echo Modo — Native Adaptive Icon Studio (v1.6.0 Windows)
echo.
echo USAGE:
echo   modo ^<command^> [options]
echo.
echo COMMANDS:
echo   style ^<exe_or_lnk^>    Apply adaptive styling to Windows shortcut or executable
echo   restore ^<path^>        Restore original icon
echo   folder ^<folder_path^>  Style folder using desktop.ini and custom palette
echo   version                Show version
echo   help                   Show this help message
echo.
exit /b 0
