#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>

namespace ModoInstaller {

bool IsRunningFromInstalledLocation();
std::wstring GetDefaultInstallDirectory();
bool PerformInstall(HWND hParentWnd, bool desktopShortcut, bool startMenuShortcut, bool contextMenu, std::wstring& outInstalledExe, const std::wstring& customInstallDir = L"");
bool PerformUninstall(HWND hParentWnd, bool silent);
int RunSetupWizard(HINSTANCE hInstance, int nCmdShow, bool isUpdateMode = false);

} // namespace ModoInstaller
