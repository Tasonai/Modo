@echo off
setlocal
set "MODO_EXE=%~dp0Modo-Windows-Portable.exe"
if not exist "%MODO_EXE%" (
    set "MODO_EXE=%~dp0Modo.exe"
)
if not exist "%MODO_EXE%" (
    set "MODO_EXE=%LOCALAPPDATA%\Programs\Modo\Modo.exe"
)

if "%~1"=="" (
    start "" "%MODO_EXE%"
    exit /b 0
)

"%MODO_EXE%" %*
exit /b %ERRORLEVEL%
