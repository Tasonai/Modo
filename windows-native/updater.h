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

namespace ModoUpdater {

const wchar_t* GetCurrentAppVersion();
void CheckForUpdates(HWND hParentWnd, bool silent = false);

} // namespace ModoUpdater
