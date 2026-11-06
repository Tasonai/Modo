#include "shell_integration.h"
#include "icon_engine.h"

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <gdiplus.h>

#include <algorithm>
#include <cwchar>

namespace ShellIntegration {

std::wstring GetAppDataRoot() {
    wchar_t buf[MAX_PATH] = {0};
    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\Modo";
    CreateDirectoryW(dir.c_str(), NULL);
    CreateDirectoryW((dir + L"\\icons").c_str(), NULL);
    CreateDirectoryW((dir + L"\\backup").c_str(), NULL);
    return dir;
}

std::wstring SanitizeFileName(const std::wstring& s) {
    std::wstring out;
    for (wchar_t c : s) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' ||
            c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
            out.push_back(L'_');
        } else {
            out.push_back(c);
        }
    }
    if (out.empty()) out = L"AppIcon";
    return out;
}

void TouchFileModificationTime(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        SYSTEMTIME st;
        FILETIME ft;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
        SetFileTime(h, NULL, &ft, &ft);
        CloseHandle(h);
    }
}

bool SetRegistryStringValue(HKEY hRoot, const std::wstring& subKey, const wchar_t* valueName, const std::wstring& val) {
    HKEY hKey = NULL;
    if (RegCreateKeyExW(hRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        LONG rc = RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return rc == ERROR_SUCCESS;
    }
    return false;
}

std::wstring GetRegistryStringValue(HKEY hRoot, const std::wstring& subKey, const wchar_t* valueName) {
    HKEY hKey = NULL;
    std::wstring out;
    if (RegOpenKeyExW(hRoot, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[MAX_PATH * 2] = {0};
        DWORD cb = sizeof(buf) - sizeof(wchar_t);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, valueName, NULL, &type, (LPBYTE)buf, &cb) == ERROR_SUCCESS &&
            (type == REG_SZ || type == REG_EXPAND_SZ)) {
            out = buf;
        }
        RegCloseKey(hKey);
    }
    return out;
}

void RefreshWindowsExplorerIconCache(bool restartExplorerForTaskbar) {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, NULL, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_ABORTIFHUNG, 500, NULL);
    wchar_t sysDir[MAX_PATH] = {0};
    if (GetSystemDirectoryW(sysDir, MAX_PATH)) {
        std::wstring ie4 = std::wstring(sysDir) + L"\\ie4uinit.exe";
        if (GetFileAttributesW(ie4.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShellExecuteW(NULL, L"open", ie4.c_str(), L"-show", NULL, SW_HIDE);
        }
    }
    if (restartExplorerForTaskbar) {
        HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
        if (hTray) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hTray, &pid);
            PostMessageW(hTray, WM_QUIT, 0, 0);
            Sleep(450);
            if (FindWindowW(L"Shell_TrayWnd", NULL) != NULL && pid != 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    WaitForSingleObject(hProc, 1000);
                    CloseHandle(hProc);
                }
            }
            Sleep(250);
            if (FindWindowW(L"Shell_TrayWnd", NULL) == NULL) {
                wchar_t winDir[MAX_PATH] = {0};
                GetWindowsDirectoryW(winDir, MAX_PATH);
                std::wstring expExe = std::wstring(winDir) + L"\\explorer.exe";
                ShellExecuteW(NULL, L"open", expExe.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
    }
}

bool ResolveShortcut(const std::wstring& lnkPath, std::wstring& outTarget, std::wstring& outCustomIcon) {
    std::wstring ext = PathFindExtensionW(lnkPath.c_str());
    if (_wcsicmp(ext.c_str(), L".url") == 0) {
        wchar_t iconFile[MAX_PATH] = {0};
        GetPrivateProfileStringW(L"InternetShortcut", L"IconFile", L"", iconFile, MAX_PATH, lnkPath.c_str());
        if (iconFile[0]) {
            wchar_t expanded[MAX_PATH] = {0};
            ExpandEnvironmentStringsW(iconFile, expanded, MAX_PATH);
            outCustomIcon = expanded;
            outTarget = expanded;
        }
        return true;
    }

    IShellLinkW* psl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) || !psl) {
        return false;
    }
    IPersistFile* ppf = nullptr;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
        if (SUCCEEDED(ppf->Load(lnkPath.c_str(), STGM_READ))) {
            wchar_t target[MAX_PATH] = {0};
            if (SUCCEEDED(psl->GetPath(target, MAX_PATH, NULL, SLGP_RAWPATH)) && target[0]) {
                wchar_t expanded[MAX_PATH] = {0};
                ExpandEnvironmentStringsW(target, expanded, MAX_PATH);
                outTarget = expanded;
            }
            wchar_t iconLoc[MAX_PATH] = {0};
            int iconIdx = 0;
            if (SUCCEEDED(psl->GetIconLocation(iconLoc, MAX_PATH, &iconIdx)) && iconLoc[0]) {
                wchar_t expandedIcon[MAX_PATH] = {0};
                ExpandEnvironmentStringsW(iconLoc, expandedIcon, MAX_PATH);
                outCustomIcon = expandedIcon;
            }
        }
        ppf->Release();
    }
    psl->Release();
    return !outTarget.empty() || !outCustomIcon.empty();
}

bool SetShortcutIconLocation(const std::wstring& lnkPath, const std::wstring& icoPath) {
    DWORD attrs = GetFileAttributesW(lnkPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))) {
        SetFileAttributesW(lnkPath.c_str(), attrs & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM));
    }

    std::wstring ext = PathFindExtensionW(lnkPath.c_str());
    if (_wcsicmp(ext.c_str(), L".url") == 0) {
        BOOL ok1 = WritePrivateProfileStringW(L"InternetShortcut", L"IconFile", icoPath.c_str(), lnkPath.c_str());
        BOOL ok2 = WritePrivateProfileStringW(L"InternetShortcut", L"IconIndex", L"0", lnkPath.c_str());
        WritePrivateProfileStringW(NULL, NULL, NULL, lnkPath.c_str());
        if (ok1 && ok2) {
            TouchFileModificationTime(lnkPath);
            return true;
        }
        return false;
    }

    IShellLinkW* psl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) || !psl) {
        return false;
    }
    bool ok = false;
    IPersistFile* ppf = nullptr;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
        if (SUCCEEDED(ppf->Load(lnkPath.c_str(), STGM_READWRITE))) {
            if (SUCCEEDED(psl->SetIconLocation(icoPath.c_str(), 0))) {
                if (SUCCEEDED(ppf->Save(lnkPath.c_str(), TRUE))) {
                    ok = true;
                }
            }
        }
        ppf->Release();
    }
    psl->Release();
    if (ok) {
        TouchFileModificationTime(lnkPath);
    }
    return ok;
}

static Bitmap* LoadSystemDllIcon(const std::wstring& dllPath, int negResId, const std::wstring& fallbackDll, int fallbackIdx, const std::wstring& safeCacheName, const CLSID* pngClsid) {
    std::wstring origCachePng = GetAppDataRoot() + L"\\backup\\" + safeCacheName + L"_orig.png";
    if (GetFileAttributesW(origCachePng.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Bitmap* cached = Bitmap::FromFile(origCachePng.c_str());
        if (cached && cached->GetLastStatus() == Ok && cached->GetWidth() >= 16) {
            return cached;
        }
        delete cached;
    }
    Bitmap* b = IconEngine::ExtractDirectPeOrIco256(dllPath, negResId);
    if (!b && !fallbackDll.empty()) {
        b = IconEngine::ExtractDirectPeOrIco256(fallbackDll, fallbackIdx);
    }
    if (!b) {
        b = IconEngine::LoadHighResIcon(dllPath, fallbackDll, safeCacheName, pngClsid);
    } else if (pngClsid) {
        b->Save(origCachePng.c_str(), pngClsid, NULL);
    }
    return b;
}

void AddBuiltInSystemEntries(std::vector<AppEntry>& apps, const CLSID* pngClsid) {
    wchar_t winDir[MAX_PATH] = {0};
    GetWindowsDirectoryW(winDir, MAX_PATH);
    std::wstring wDir = winDir[0] ? winDir : L"C:\\Windows";
    std::wstring explorerExe = wDir + L"\\explorer.exe";
    std::wstring imageresDll = wDir + L"\\System32\\imageres.dll";
    std::wstring shell32Dll  = wDir + L"\\System32\\shell32.dll";

    // 1. File Explorer (Проводник)
    {
        AppEntry fe;
        fe.name = L"File Explorer (Проводник)";
        fe.searchKeywords = L"file explorer проводник файловый менеджер папки windows explorer.exe";
        fe.targetPath = explorerExe;
        fe.category = L"Системный компонент Windows";
        fe.systemClsid = L"{52205fd8-5dfb-447d-801a-d0b52f2e83e1}";
        fe.defaultRestoreIcon = explorerExe + L",0";
        fe.isFileExplorer = true;
        fe.isSystemPinnedTop = true;

        wchar_t appData[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
            const wchar_t* relPaths[] = {
                L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\File Explorer.lnk",
                L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\Проводник.lnk",
                L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\Windows Explorer.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Tools\\File Explorer.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Tools\\Проводник.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\Windows System\\File Explorer.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\Windows System\\Проводник.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\Accessories\\System Tools\\File Explorer.lnk"
            };
            for (const wchar_t* rp : relPaths) {
                std::wstring p = std::wstring(appData) + rp;
                if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    fe.allLnkPaths.push_back(p);
                    if (fe.lnkPath.empty()) fe.lnkPath = p;
                    std::wstring t, cIcon;
                    ResolveShortcut(p, t, cIcon);
                    if (cIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) fe.isApplied = true;
                }
            }
        }
        wchar_t desk[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desk)) && desk[0]) {
            const wchar_t* deskNames[] = { L"\\File Explorer.lnk", L"\\Проводник.lnk", L"\\File Explorer (Проводник).lnk" };
            for (const wchar_t* dn : deskNames) {
                std::wstring p = std::wstring(desk) + dn;
                if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    fe.allLnkPaths.push_back(p);
                    if (fe.lnkPath.empty()) fe.lnkPath = p;
                    std::wstring t, cIcon;
                    ResolveShortcut(p, t, cIcon);
                    if (cIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) fe.isApplied = true;
                }
            }
        }
        std::wstring regCur = GetRegistryStringValue(
            HKEY_CURRENT_USER,
            L"Software\\Classes\\CLSID\\{52205fd8-5dfb-447d-801a-d0b52f2e83e1}\\DefaultIcon",
            NULL
        );
        if (regCur.find(L"\\IconMaker\\icons\\") != std::wstring::npos) fe.isApplied = true;

        fe.origBitmap = LoadSystemDllIcon(explorerExe, 0, shell32Dll, 4, L"System_FileExplorer", pngClsid);
        apps.push_back(fe);
    }

    // 2. The 5 Windows Desktop Shell CLSID Icons
    struct SysClsidDef {
        const wchar_t* name;
        const wchar_t* keywords;
        const wchar_t* clsid;
        int imageresNegId;
        int shell32FallbackIdx;
        const wchar_t* defaultIconStr;
        const wchar_t* cacheKey;
        bool isRecycleBin;
    };
    const SysClsidDef defs[] = {
        {
            L"Этот компьютер (This PC)",
            L"this pc этот компьютер мой компьютер my computer",
            L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
            -109, 15,
            L"%SystemRoot%\\System32\\imageres.dll,-109",
            L"System_ThisPC",
            false
        },
        {
            L"Корзина (Recycle Bin)",
            L"recycle bin корзина мусорка",
            L"{645FF040-5081-101B-9F08-00AA002F954E}",
            -55, 31,
            L"%SystemRoot%\\System32\\imageres.dll,-55",
            L"System_RecycleBin",
            true
        },
        {
            L"Папка пользователя (User's Files)",
            L"user files папка пользователя домашняя папка документы",
            L"{59031A47-3F72-44A7-89C5-5595FE6B30EE}",
            -123, 43,
            L"%SystemRoot%\\System32\\imageres.dll,-123",
            L"System_UserFiles",
            false
        },
        {
            L"Панель управления (Control Panel)",
            L"control panel панель управления настройки",
            L"{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}",
            -27, 21,
            L"%SystemRoot%\\System32\\imageres.dll,-27",
            L"System_ControlPanel",
            false
        },
        {
            L"Сеть (Network)",
            L"network сеть сетевое окружение",
            L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}",
            -25, 17,
            L"%SystemRoot%\\System32\\imageres.dll,-25",
            L"System_Network",
            false
        }
    };

    for (const auto& d : defs) {
        AppEntry e;
        e.name = d.name;
        e.searchKeywords = d.keywords;
        e.targetPath = imageresDll;
        e.category = L"Значок Рабочего стола Windows";
        e.systemClsid = d.clsid;
        e.defaultRestoreIcon = d.defaultIconStr;
        e.isRecycleBin = d.isRecycleBin;
        e.isSystemPinnedTop = true;

        std::wstring regKey = std::wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\") + d.clsid + L"\\DefaultIcon";
        std::wstring curVal = GetRegistryStringValue(HKEY_CURRENT_USER, regKey, NULL);
        if (curVal.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
            e.isApplied = true;
        }
        e.origBitmap = LoadSystemDllIcon(imageresDll, d.imageresNegId, shell32Dll, d.shell32FallbackIdx, d.cacheKey, pngClsid);
        apps.push_back(e);
    }
}

void ScanDirectoryForShortcuts(const std::wstring& dir, const std::wstring& categoryName, std::vector<AppEntry>& apps, int depth, bool isDesktopOrTaskbar) {
    if (depth > 3 || apps.size() >= 170) return;
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring fullPath = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ScanDirectoryForShortcuts(fullPath, categoryName, apps, depth + 1, isDesktopOrTaskbar);
        } else {
            std::wstring fname = fd.cFileName;
            bool isLnk = (fname.size() > 4 && _wcsicmp(fname.c_str() + fname.size() - 4, L".lnk") == 0);
            bool isUrl = (fname.size() > 4 && _wcsicmp(fname.c_str() + fname.size() - 4, L".url") == 0);
            if (isLnk || (isUrl && isDesktopOrTaskbar)) {
                std::wstring title = fname.substr(0, fname.size() - 4);
                std::wstring lowerTitle = title;
                for (auto& c : lowerTitle) c = towlower(c);
                if (lowerTitle.find(L"uninstall") != std::wstring::npos ||
                    lowerTitle.find(L"удалить") != std::wstring::npos ||
                    lowerTitle.find(L"деинстал") != std::wstring::npos) {
                    continue;
                }

                std::wstring target, customIcon;
                ResolveShortcut(fullPath, target, customIcon);

                // Merge File Explorer / Проводник shortcuts into our built-in File Explorer entry
                bool isExplorerLnk = (lowerTitle == L"file explorer" || lowerTitle == L"проводник" ||
                                      lowerTitle == L"windows explorer" || lowerTitle == L"file explorer (проводник)");
                if (!target.empty()) {
                    std::wstring baseExe = PathFindFileNameW(target.c_str());
                    if (_wcsicmp(baseExe.c_str(), L"explorer.exe") == 0) isExplorerLnk = true;
                }
                if (isExplorerLnk && !apps.empty() && apps[0].isFileExplorer) {
                    bool already = false;
                    for (const auto& p : apps[0].allLnkPaths) {
                        if (_wcsicmp(p.c_str(), fullPath.c_str()) == 0) { already = true; break; }
                    }
                    if (!already) apps[0].allLnkPaths.push_back(fullPath);
                    if (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
                        apps[0].isApplied = true;
                    }
                    continue;
                }

                if (isLnk && !isDesktopOrTaskbar && !target.empty()) {
                    std::wstring ext = PathFindExtensionW(target.c_str());
                    if (_wcsicmp(ext.c_str(), L".exe") != 0) continue;
                }

                bool exists = false;
                for (auto& existing : apps) {
                    bool sameName = (_wcsicmp(existing.name.c_str(), title.c_str()) == 0);
                    bool sameTarget = (!target.empty() && !existing.targetPath.empty() &&
                                       _wcsicmp(existing.targetPath.c_str(), target.c_str()) == 0);
                    if (sameName || sameTarget) {
                        bool pathAlreadyAdded = false;
                        for (const auto& p : existing.allLnkPaths) {
                            if (_wcsicmp(p.c_str(), fullPath.c_str()) == 0) {
                                pathAlreadyAdded = true;
                                break;
                            }
                        }
                        if (!pathAlreadyAdded) {
                            existing.allLnkPaths.push_back(fullPath);
                        }
                        if (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
                            existing.isApplied = true;
                        }
                        exists = true;
                        break;
                    }
                }
                if (exists) continue;

                AppEntry entry;
                entry.name = title;
                entry.lnkPath = fullPath;
                entry.allLnkPaths.push_back(fullPath);
                std::wstring iconSource = (!customIcon.empty() && customIcon.find(L"\\IconMaker\\icons\\") == std::wstring::npos)
                    ? customIcon
                    : (target.empty() ? fullPath : target);
                entry.targetPath = target.empty() ? iconSource : target;
                entry.category = categoryName;
                entry.isApplied = (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos);

                // If shortcut already has an IconMaker icon and we have a .bak shortcut, resolve original icon from .bak
                std::wstring safe = SanitizeFileName(entry.name);
                if (entry.isApplied) {
                    std::wstring bak0 = GetAppDataRoot() + L"\\backup\\" + safe + L"_0.lnk.bak";
                    if (GetFileAttributesW(bak0.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        std::wstring bTarget, bIcon;
                        ResolveShortcut(bak0, bTarget, bIcon);
                        if (!bIcon.empty() && bIcon.find(L"\\IconMaker\\icons\\") == std::wstring::npos) {
                            iconSource = bIcon;
                        } else if (!bTarget.empty()) {
                            iconSource = bTarget;
                        }
                    }
                }

                entry.origBitmap = IconEngine::LoadHighResIcon(iconSource, entry.lnkPath, safe);
                apps.push_back(entry);
            }
        }
    } while (FindNextFileW(hFind, &fd) && apps.size() < 170);

    FindClose(hFind);
}

bool ApplyRoundIconToEntry(AppEntry& app, const RoundStyle& style, const CLSID* pngClsid, uint32_t& revCounter, bool* outNeedsAdmin) {
    std::wstring root = GetAppDataRoot();
    std::wstring safe = SanitizeFileName(app.name);
    std::wstring iconsDir = root + L"\\icons";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((iconsDir + L"\\" + safe + L"*.ico").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            DeleteFileW((iconsDir + L"\\" + fd.cFileName).c_str());
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    wchar_t revSuffix[64];
    wsprintfW(revSuffix, L"_%u_%u.ico", (unsigned)(GetTickCount() & 0xFFFF), ++revCounter);
    std::wstring icoPath = iconsDir + L"\\" + safe + revSuffix;

    if (!IconEngine::SaveMultiSizeIco(app.origBitmap, style, icoPath, pngClsid)) return false;

    if (app.allLnkPaths.empty() && !app.lnkPath.empty()) {
        app.allLnkPaths.push_back(app.lnkPath);
    }

    int updatedCount = 0;
    bool permissionDenied = false;

    // 1. Update Registry CLSID DefaultIcon if this is a Windows System Icon or File Explorer
    if (!app.systemClsid.empty()) {
        std::wstring icoWithIdx = icoPath + L",0";
        std::wstring explorerClsidKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";
        std::wstring classesClsidKey  = L"Software\\Classes\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";

        if (SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, NULL, icoWithIdx)) {
            updatedCount++;
        }
        if (app.isRecycleBin) {
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"empty", icoWithIdx);
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"full",  icoWithIdx);
        }
        if (SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey, NULL, icoWithIdx)) {
            updatedCount++;
        }
        if (app.isRecycleBin) {
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey, L"empty", icoWithIdx);
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey, L"full",  icoWithIdx);
        }
    }

    // 2. Special handling for File Explorer (Проводник): ensure Taskbar & Start Menu shortcuts are updated
    if (app.isFileExplorer) {
        std::wstring icoWithIdx = icoPath + L",0";
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\explorer.exe\\DefaultIcon", NULL, icoWithIdx);

        wchar_t appData[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
            std::wstring tbDir = std::wstring(appData) + L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";
            if (GetFileAttributesW(tbDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                std::wstring tbExplorerLnk = tbDir + L"\\File Explorer.lnk";
                if (GetFileAttributesW(tbExplorerLnk.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    IShellLinkW* psl = nullptr;
                    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) && psl) {
                        psl->SetPath(app.targetPath.c_str());
                        psl->SetIconLocation(icoPath.c_str(), 0);
                        IPersistFile* ppf = nullptr;
                        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                            if (SUCCEEDED(ppf->Save(tbExplorerLnk.c_str(), TRUE))) {
                                app.allLnkPaths.push_back(tbExplorerLnk);
                                updatedCount++;
                            }
                            ppf->Release();
                        }
                        psl->Release();
                    }
                }
            }
        }
    }

    // 3. Update all associated .lnk and .url shortcuts
    for (size_t idx = 0; idx < app.allLnkPaths.size(); ++idx) {
        const std::wstring& lnk = app.allLnkPaths[idx];
        if (lnk.empty() || GetFileAttributesW(lnk.c_str()) == INVALID_FILE_ATTRIBUTES) continue;

        wchar_t bakName[MAX_PATH];
        wsprintfW(bakName, L"%s_%u.lnk.bak", safe.c_str(), (unsigned)idx);
        std::wstring backupLnk = root + L"\\backup\\" + bakName;
        if (GetFileAttributesW(backupLnk.c_str()) == INVALID_FILE_ATTRIBUTES) {
            CopyFileW(lnk.c_str(), backupLnk.c_str(), TRUE);
        }

        if (SetShortcutIconLocation(lnk, icoPath)) {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, lnk.c_str(), NULL);
            updatedCount++;
        } else {
            permissionDenied = true;
        }
    }

    if (outNeedsAdmin) {
        *outNeedsAdmin = permissionDenied && (updatedCount == 0);
    }
    app.isApplied = (updatedCount > 0 && !permissionDenied);
    return updatedCount > 0;
}

bool RestoreOriginalEntry(AppEntry& app) {
    if (!app.systemClsid.empty() && !app.defaultRestoreIcon.empty()) {
        std::wstring explorerClsidKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";
        std::wstring classesClsidKey  = L"Software\\Classes\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";
        SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, NULL, app.defaultRestoreIcon);
        SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey,  NULL, app.defaultRestoreIcon);
        if (app.isRecycleBin) {
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"empty", L"%SystemRoot%\\System32\\imageres.dll,-55");
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"full",  L"%SystemRoot%\\System32\\imageres.dll,-54");
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey,  L"empty", L"%SystemRoot%\\System32\\imageres.dll,-55");
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey,  L"full",  L"%SystemRoot%\\System32\\imageres.dll,-54");
        }
    }
    if (app.isFileExplorer) {
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\explorer.exe\\DefaultIcon", NULL, app.defaultRestoreIcon);
    }

    if (app.allLnkPaths.empty() && !app.lnkPath.empty()) {
        app.allLnkPaths.push_back(app.lnkPath);
    }
    std::wstring root = GetAppDataRoot();
    std::wstring safe = SanitizeFileName(app.name);
    for (size_t idx = 0; idx < app.allLnkPaths.size(); ++idx) {
        const std::wstring& lnk = app.allLnkPaths[idx];
        wchar_t bakName[MAX_PATH];
        wsprintfW(bakName, L"%s_%u.lnk.bak", safe.c_str(), (unsigned)idx);
        std::wstring backupLnk = root + L"\\backup\\" + bakName;
        if (GetFileAttributesW(backupLnk.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW(backupLnk.c_str(), lnk.c_str(), FALSE);
            TouchFileModificationTime(lnk);
        } else if (!app.targetPath.empty()) {
            SetShortcutIconLocation(lnk, app.targetPath);
        }
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, lnk.c_str(), NULL);
    }
    app.isApplied = false;
    return true;
}

bool ApplyCustomFolderIcon(const std::wstring& folderPath, const std::wstring& icoPath) {
    if (folderPath.empty() || icoPath.empty()) return false;
    std::wstring iniPath = folderPath + L"\\desktop.ini";

    // 1. Remove attributes if exists to allow overwrite
    SetFileAttributesW(iniPath.c_str(), FILE_ATTRIBUTE_NORMAL);

    // 2. Write desktop.ini ShellClassInfo keys
    WritePrivateProfileStringW(L".ShellClassInfo", L"IconResource", (icoPath + L",0").c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L".ShellClassInfo", L"IconFile", icoPath.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L".ShellClassInfo", L"IconIndex", L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"ViewState", L"FolderType", L"Generic", iniPath.c_str());

    // 3. Mark desktop.ini as System + Hidden
    SetFileAttributesW(iniPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    // 4. Mark directory with FILE_ATTRIBUTE_READONLY so Explorer evaluates desktop.ini
    DWORD attr = GetFileAttributesW(folderPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesW(folderPath.c_str(), attr | FILE_ATTRIBUTE_READONLY);
    }

    // 5. Notify Explorer shell
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, folderPath.c_str(), NULL);
    return true;
}

bool RestoreFolderIcon(const std::wstring& folderPath) {
    if (folderPath.empty()) return false;
    std::wstring iniPath = folderPath + L"\\desktop.ini";

    SetFileAttributesW(iniPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    DeleteFileW(iniPath.c_str());

    DWORD attr = GetFileAttributesW(folderPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesW(folderPath.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);
    }

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, folderPath.c_str(), NULL);
    return true;
}

bool RegisterShellContextMenu(const std::wstring& exePath, bool enable) {
    if (enable) {
        if (exePath.empty()) return false;
        std::wstring cmdExe = L"\"" + exePath + L"\" style \"%1\"";
        std::wstring cmdFolder = L"\"" + exePath + L"\" folder \"%1\"";
        std::wstring iconSpec = L"\"" + exePath + L"\",0";

        // 1. Files: .exe
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\exefile\\shell\\ModoStyle", L"", L"Стилизовать иконку в Modo...");
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\exefile\\shell\\ModoStyle", L"Icon", iconSpec);
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\exefile\\shell\\ModoStyle\\command", L"", cmdExe);

        // 2. Shortcuts: .lnk
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\lnkfile\\shell\\ModoStyle", L"", L"Стилизовать иконку в Modo...");
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\lnkfile\\shell\\ModoStyle", L"Icon", iconSpec);
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\lnkfile\\shell\\ModoStyle\\command", L"", cmdExe);

        // 3. Folders: Directory
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\ModoFolder", L"", L"Изменить цвет папки в Modo...");
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\ModoFolder", L"Icon", iconSpec);
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\ModoFolder\\command", L"", cmdFolder);

        return true;
    } else {
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\exefile\\shell\\ModoStyle\\command");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\exefile\\shell\\ModoStyle");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\lnkfile\\shell\\ModoStyle\\command");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\lnkfile\\shell\\ModoStyle");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\ModoFolder\\command");
        RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\ModoFolder");
        return true;
    }
}

void InstallSelfAndShortcuts(HWND hwnd, bool showConfirmation) {
    wchar_t selfPath[MAX_PATH * 2] = {0};
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) return;

    wchar_t localAppData[MAX_PATH] = {0};
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) return;

    std::wstring progDir = std::wstring(localAppData) + L"\\Programs";
    CreateDirectoryW(progDir.c_str(), NULL);
    std::wstring appDir = progDir + L"\\Modo";
    CreateDirectoryW(appDir.c_str(), NULL);

    std::wstring installedExe = appDir + L"\\Modo.exe";
    if (_wcsicmp(selfPath, installedExe.c_str()) != 0) {
        CopyFileW(selfPath, installedExe.c_str(), FALSE);
    } else {
        installedExe = selfPath;
    }

    auto makeShortcut = [&](int csidl, const wchar_t* lnkName) {
        wchar_t folder[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, 0, folder)) && folder[0]) {
            std::wstring lnkPath = std::wstring(folder) + L"\\" + lnkName;
            IShellLinkW* psl = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) && psl) {
                psl->SetPath(installedExe.c_str());
                psl->SetWorkingDirectory(appDir.c_str());
                psl->SetIconLocation(installedExe.c_str(), 0);
                psl->SetDescription(L"Modo Studio — Адаптивные иконки от Atoma");
                IPersistFile* ppf = nullptr;
                if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                    ppf->Save(lnkPath.c_str(), TRUE);
                    ppf->Release();
                }
                psl->Release();
            }
        }
    };

    makeShortcut(CSIDL_DESKTOPDIRECTORY, L"Modo Studio.lnk");
    makeShortcut(CSIDL_PROGRAMS, L"Modo Studio.lnk");

    // Enable Windows Explorer right-click context menu integration
    RegisterShellContextMenu(installedExe, true);

    if (showConfirmation && hwnd) {
        MessageBoxW(
            hwnd,
            L"Modo Studio успешно установлен!\n\n"
            L"• Папка: %LOCALAPPDATA%\\Programs\\Modo\\Modo.exe\n"
            L"• Ярлыки созданы на Рабочем столе и в меню «Пуск».\n"
            L"• В меню правой кнопки мыши Проводника добавлены пункты «Стилизовать в Modo...».",
            L"Modo Studio",
            MB_ICONINFORMATION
        );
    }
}

} // namespace ShellIntegration
