#pragma once

#include "types.h"

namespace ShellIntegration {

std::wstring GetAppDataRoot();
std::wstring SanitizeFileName(const std::wstring& s);

bool ResolveShortcut(const std::wstring& lnkPath, std::wstring& outTarget, std::wstring& outCustomIcon);
bool SetShortcutIconLocation(const std::wstring& lnkPath, const std::wstring& icoPath);

std::wstring GetRegistryStringValue(HKEY root, const std::wstring& subKey, const wchar_t* valueName);
bool SetRegistryStringValue(HKEY root, const std::wstring& subKey, const wchar_t* valueName, const std::wstring& val);

void TouchFileModificationTime(const std::wstring& path);
void RefreshWindowsExplorerIconCache(bool restartExplorer = false);

bool ApplyRoundIconToEntry(AppEntry& app, const RoundStyle& style, const CLSID* pngClsid, uint32_t& revCounter, bool* outNeedsAdmin = nullptr);
bool RestoreOriginalEntry(AppEntry& app);

bool ApplyCustomFolderIcon(const std::wstring& folderPath, const std::wstring& icoPath);
bool RestoreFolderIcon(const std::wstring& folderPath);

void ScanDirectoryForShortcuts(const std::wstring& dir, const std::wstring& categoryName, std::vector<AppEntry>& apps, int depth = 0, bool isDesktopOrTaskbar = false);
void AddBuiltInSystemEntries(std::vector<AppEntry>& apps, const CLSID* pngClsid);
void InstallSelfAndShortcuts(HWND hwnd, bool showConfirmation = false);
bool RegisterShellContextMenu(const std::wstring& exePath, bool enable);

} // namespace ShellIntegration
