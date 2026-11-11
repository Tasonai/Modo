#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "installer.h"
#include "shell_integration.h"

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <string>
#include <vector>

namespace ModoInstaller {

static const wchar_t* WIZARD_CLASS_NAME = L"ModoSetupWizardClass";
static const wchar_t* APP_VERSION = L"1.7.0";

enum ControlID {
    IDC_WIZARD_HEADER = 1001,
    IDC_WIZARD_PATH_LABEL,
    IDC_WIZARD_PATH_EDIT,
    IDC_WIZARD_BTN_BROWSE,
    IDC_WIZARD_CHK_DESKTOP,
    IDC_WIZARD_CHK_STARTMENU,
    IDC_WIZARD_CHK_CONTEXTMENU,
    IDC_WIZARD_CHK_LAUNCH,
    IDC_WIZARD_STATUS,
    IDC_WIZARD_PROGRESS,
    IDC_WIZARD_BTN_INSTALL,
    IDC_WIZARD_BTN_CANCEL
};

struct WizardState {
    HWND hHeader = NULL;
    HWND hPathLabel = NULL;
    HWND hPathEdit = NULL;
    HWND hBtnBrowse = NULL;
    HWND hChkDesktop = NULL;
    HWND hChkStartMenu = NULL;
    HWND hChkContextMenu = NULL;
    HWND hChkLaunch = NULL;
    HWND hStatus = NULL;
    HWND hProgress = NULL;
    HWND hBtnInstall = NULL;
    HWND hBtnCancel = NULL;
    HFONT hFontTitle = NULL;
    HFONT hFontRegular = NULL;
    HFONT hFontBold = NULL;
    bool isFinished = false;
    std::wstring installedExePath;
};

std::wstring GetDefaultInstallDirectory() {
    wchar_t localAppData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData)) && localAppData[0]) {
        return std::wstring(localAppData) + L"\\Programs\\Modo";
    }
    return L"C:\\Modo";
}

bool IsRunningFromInstalledLocation() {
    wchar_t selfPath[MAX_PATH * 2] = {0};
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) return false;
    std::wstring regLoc = ShellIntegration::GetRegistryStringValue(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Modo",
        L"InstallLocation"
    );
    if (!regLoc.empty() && _wcsnicmp(selfPath, regLoc.c_str(), regLoc.length()) == 0) {
        return true;
    }
    std::wstring defaultDir = GetDefaultInstallDirectory();
    return (_wcsnicmp(selfPath, defaultDir.c_str(), defaultDir.length()) == 0);
}

static bool CreateShellLink(const std::wstring& lnkPath, const std::wstring& targetExe, const std::wstring& workingDir, const std::wstring& description) {
    IShellLinkW* psl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) || !psl) {
        return false;
    }
    psl->SetPath(targetExe.c_str());
    psl->SetWorkingDirectory(workingDir.c_str());
    psl->SetIconLocation(targetExe.c_str(), 0);
    psl->SetDescription(description.c_str());

    IPersistFile* ppf = nullptr;
    bool ok = false;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
        if (SUCCEEDED(ppf->Save(lnkPath.c_str(), TRUE))) {
            ok = true;
        }
        ppf->Release();
    }
    psl->Release();
    return ok;
}

static bool RegisterUninstallEntry(const std::wstring& installedExe, const std::wstring& installDir) {
    HKEY hKey = NULL;
    const wchar_t* subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Modo";
    LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS || !hKey) return false;

    auto setStr = [&](const wchar_t* valName, const std::wstring& str) {
        RegSetValueExW(hKey, valName, 0, REG_SZ, (const BYTE*)str.c_str(), (DWORD)((str.length() + 1) * sizeof(wchar_t)));
    };
    auto setDword = [&](const wchar_t* valName, DWORD val) {
        RegSetValueExW(hKey, valName, 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
    };

    std::wstring displayName = L"Modo Studio";
    std::wstring publisher = L"Atoma";
    std::wstring iconPath = installedExe + L",0";
    std::wstring uninstallCmd = L"\"" + installedExe + L"\" --uninstall";
    std::wstring quietUninstallCmd = L"\"" + installedExe + L"\" --uninstall --silent";
    std::wstring urlInfo = L"https://github.com/Tasonai/Modo";

    setStr(L"DisplayName", displayName);
    setStr(L"DisplayVersion", APP_VERSION);
    setStr(L"Publisher", publisher);
    setStr(L"DisplayIcon", iconPath);
    setStr(L"InstallLocation", installDir);
    setStr(L"UninstallString", uninstallCmd);
    setStr(L"QuietUninstallString", quietUninstallCmd);
    setStr(L"HelpLink", urlInfo);
    setStr(L"URLInfoAbout", urlInfo);
    setDword(L"NoModify", 1);
    setDword(L"NoRepair", 1);
    setDword(L"EstimatedSize", 450);

    RegCloseKey(hKey);
    return true;
}

static bool IsSafeDirectoryPath(const std::wstring& dir) {
    if (dir.size() < 3) return false;
    for (wchar_t c : dir) {
        if (c == L'"' || c == L'&' || c == L'|' || c == L'<' || c == L'>' || c == L'^' || c == L'\r' || c == L'\n') {
            return false;
        }
    }
    return true;
}

bool PerformInstall(HWND hParentWnd, bool desktopShortcut, bool startMenuShortcut, bool contextMenu, std::wstring& outInstalledExe, const std::wstring& customInstallDir) {
    wchar_t selfPath[MAX_PATH * 2] = {0};
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) return false;

    std::wstring installDir = customInstallDir.empty() ? GetDefaultInstallDirectory() : customInstallDir;
    while (installDir.size() > 3 && (installDir.back() == L'\\' || installDir.back() == L'/')) {
        installDir.pop_back();
    }
    if (!IsSafeDirectoryPath(installDir)) {
        if (hParentWnd) {
            MessageBoxW(hParentWnd, L"Указан недопустимый путь папки установки.", L"Ошибка установки", MB_ICONERROR);
        }
        return false;
    }

    SHCreateDirectoryExW(hParentWnd, installDir.c_str(), NULL);

    std::wstring targetExe = installDir + L"\\Modo.exe";
    outInstalledExe = targetExe;

    if (_wcsicmp(selfPath, targetExe.c_str()) != 0) {
        // Attempt copy with retry in case target was recently closed
        bool copied = false;
        for (int retry = 0; retry < 5; ++retry) {
            if (CopyFileW(selfPath, targetExe.c_str(), FALSE)) {
                copied = true;
                break;
            }
            Sleep(200);
        }
        if (!copied) {
            if (hParentWnd) {
                MessageBoxW(hParentWnd, L"Не удалось скопировать Modo.exe в целевую папку. Убедитесь, что приложение закрыто.", L"Ошибка установки", MB_ICONERROR);
            }
            return false;
        }
    }

    // Desktop shortcut
    if (desktopShortcut) {
        wchar_t desktop[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop)) && desktop[0]) {
            std::wstring lnk = std::wstring(desktop) + L"\\Modo Studio.lnk";
            CreateShellLink(lnk, targetExe, installDir, L"Modo Studio — Адаптивные иконки от Atoma");
        }
    }

    // Start Menu shortcut
    if (startMenuShortcut) {
        wchar_t startMenu[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, startMenu)) && startMenu[0]) {
            std::wstring lnk = std::wstring(startMenu) + L"\\Modo Studio.lnk";
            CreateShellLink(lnk, targetExe, installDir, L"Modo Studio — Адаптивные иконки от Atoma");
        }
    }

    // Windows Explorer Right-Click Context Menu
    if (contextMenu) {
        ShellIntegration::RegisterShellContextMenu(targetExe, true);
    }

    // Register Add/Remove Programs entry
    RegisterUninstallEntry(targetExe, installDir);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return true;
}

bool PerformUninstall(HWND hParentWnd, bool silent) {
    if (!silent) {
        int choice = MessageBoxW(
            hParentWnd,
            L"Вы действительно хотите удалить Modo Studio с вашего компьютера?\n\nВсе изменённые значки Рабочего стола и ярлыки будут возвращены к стандартным системным иконкам.",
            L"Удаление Modo Studio",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
        );
        if (choice != IDYES) return false;
    }

    // Restore styled system CLSIDs and shortcuts before removing Modo
    ShellIntegration::RestoreAllStyledSystemIconsForUninstall();

    // Remove shortcuts
    wchar_t desktop[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop)) && desktop[0]) {
        std::wstring lnk = std::wstring(desktop) + L"\\Modo Studio.lnk";
        DeleteFileW(lnk.c_str());
    }

    wchar_t startMenu[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, startMenu)) && startMenu[0]) {
        std::wstring lnk = std::wstring(startMenu) + L"\\Modo Studio.lnk";
        DeleteFileW(lnk.c_str());
    }

    // Remove Shell context menu
    ShellIntegration::RegisterShellContextMenu(L"", false);

    std::wstring installDir = ShellIntegration::GetRegistryStringValue(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Modo",
        L"InstallLocation"
    );
    if (installDir.empty() || !IsSafeDirectoryPath(installDir)) {
        installDir = GetDefaultInstallDirectory();
    }

    // Remove Uninstall Registry Entry
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Modo");

    // Delete only Modo.exe and remove directory only if empty (never recursive rmdir /s on user-selected folders!)
    std::wstring targetExe = installDir + L"\\Modo.exe";
    if (IsSafeDirectoryPath(installDir)) {
        std::wstring cmd = L"cmd.exe /c timeout /t 2 /nobreak > nul & del /f /q \"" + targetExe + L"\" & rmdir \"" + installDir + L"\"";
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {0};
        std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back(L'\0');

        CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        if (pi.hProcess) CloseHandle(pi.hProcess);
        if (pi.hThread) CloseHandle(pi.hThread);
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    if (!silent) {
        MessageBoxW(
            hParentWnd,
            L"Modo Studio успешно удалена с вашего компьютера.",
            L"Modo Studio",
            MB_OK | MB_ICONINFORMATION
        );
    }
    return true;
}

static LRESULT CALLBACK WizardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WizardState* state = reinterpret_cast<WizardState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state = reinterpret_cast<WizardState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);

            state->hFontTitle = CreateFontW(
                22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );
            state->hFontBold = CreateFontW(
                15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );
            state->hFontRegular = CreateFontW(
                14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );

            // Title Header
            state->hHeader = CreateWindowExW(
                0, L"STATIC",
                L"Установка Modo Studio 1.7.0\nАдаптивный дизайн иконок для Windows и macOS",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                30, 24, 460, 50,
                hwnd, (HMENU)IDC_WIZARD_HEADER, cs->hInstance, NULL
            );
            SendMessageW(state->hHeader, WM_SETFONT, (WPARAM)state->hFontTitle, TRUE);

            // Path label
            state->hPathLabel = CreateWindowExW(
                0, L"STATIC",
                L"Папка установки (для текущего пользователя):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                30, 92, 460, 20,
                hwnd, (HMENU)IDC_WIZARD_PATH_LABEL, cs->hInstance, NULL
            );
            SendMessageW(state->hPathLabel, WM_SETFONT, (WPARAM)state->hFontBold, TRUE);

            // Path edit box + Browse button
            std::wstring defaultDir = GetDefaultInstallDirectory();
            state->hPathEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT",
                defaultDir.c_str(),
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                30, 116, 362, 26,
                hwnd, (HMENU)IDC_WIZARD_PATH_EDIT, cs->hInstance, NULL
            );
            SendMessageW(state->hPathEdit, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);

            state->hBtnBrowse = CreateWindowExW(
                0, L"BUTTON",
                L"Обзор...",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                400, 115, 90, 28,
                hwnd, (HMENU)IDC_WIZARD_BTN_BROWSE, cs->hInstance, NULL
            );
            SendMessageW(state->hBtnBrowse, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);

            // Options Checkboxes
            state->hChkDesktop = CreateWindowExW(
                0, L"BUTTON",
                L"Создать ярлык на Рабочем столе",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                30, 160, 460, 22,
                hwnd, (HMENU)IDC_WIZARD_CHK_DESKTOP, cs->hInstance, NULL
            );
            SendMessageW(state->hChkDesktop, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);
            SendMessageW(state->hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);

            state->hChkStartMenu = CreateWindowExW(
                0, L"BUTTON",
                L"Добавить ярлык в главное меню «Пуск»",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                30, 190, 460, 22,
                hwnd, (HMENU)IDC_WIZARD_CHK_STARTMENU, cs->hInstance, NULL
            );
            SendMessageW(state->hChkStartMenu, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);
            SendMessageW(state->hChkStartMenu, BM_SETCHECK, BST_CHECKED, 0);

            state->hChkContextMenu = CreateWindowExW(
                0, L"BUTTON",
                L"Интеграция в контекстное меню Проводника («Стилизовать в Modo»)",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                30, 220, 460, 22,
                hwnd, (HMENU)IDC_WIZARD_CHK_CONTEXTMENU, cs->hInstance, NULL
            );
            SendMessageW(state->hChkContextMenu, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);
            SendMessageW(state->hChkContextMenu, BM_SETCHECK, BST_CHECKED, 0);

            state->hChkLaunch = CreateWindowExW(
                0, L"BUTTON",
                L"Запустить Modo Studio после завершения установки",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                30, 250, 460, 22,
                hwnd, (HMENU)IDC_WIZARD_CHK_LAUNCH, cs->hInstance, NULL
            );
            SendMessageW(state->hChkLaunch, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);
            SendMessageW(state->hChkLaunch, BM_SETCHECK, BST_CHECKED, 0);

            // Status label
            state->hStatus = CreateWindowExW(
                0, L"STATIC",
                L"Нажмите «Установить» для продолжения.",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                30, 290, 460, 20,
                hwnd, (HMENU)IDC_WIZARD_STATUS, cs->hInstance, NULL
            );
            SendMessageW(state->hStatus, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);

            // Progress bar
            state->hProgress = CreateWindowExW(
                0, PROGRESS_CLASSW,
                NULL,
                WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                30, 314, 460, 16,
                hwnd, (HMENU)IDC_WIZARD_PROGRESS, cs->hInstance, NULL
            );
            SendMessageW(state->hProgress, PBM_SETRANGE32, 0, 100);
            SendMessageW(state->hProgress, PBM_SETPOS, 0, 0);

            // Buttons
            state->hBtnInstall = CreateWindowExW(
                0, L"BUTTON",
                L"Установить",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                270, 350, 110, 32,
                hwnd, (HMENU)IDC_WIZARD_BTN_INSTALL, cs->hInstance, NULL
            );
            SendMessageW(state->hBtnInstall, WM_SETFONT, (WPARAM)state->hFontBold, TRUE);

            state->hBtnCancel = CreateWindowExW(
                0, L"BUTTON",
                L"Отмена",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                390, 350, 100, 32,
                hwnd, (HMENU)IDC_WIZARD_BTN_CANCEL, cs->hInstance, NULL
            );
            SendMessageW(state->hBtnCancel, WM_SETFONT, (WPARAM)state->hFontRegular, TRUE);
            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (!state) break;

            if (wmId == IDC_WIZARD_BTN_CANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }

            if (wmId == IDC_WIZARD_BTN_BROWSE) {
                BROWSEINFOW bi = {0};
                bi.hwndOwner = hwnd;
                bi.lpszTitle = L"Выберите папку для установки Modo Studio:";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
                if (pidl) {
                    wchar_t chosen[MAX_PATH] = {0};
                    if (SHGetPathFromIDListW(pidl, chosen) && chosen[0]) {
                        std::wstring sub = chosen;
                        if (sub.size() > 3 && sub.back() != L'\\') sub += L"\\Modo";
                        else if (sub.size() <= 3) sub += L"Modo";
                        SetWindowTextW(state->hPathEdit, sub.c_str());
                    }
                    CoTaskMemFree(pidl);
                }
                return 0;
            }

            if (wmId == IDC_WIZARD_BTN_INSTALL) {
                if (state->isFinished) {
                    bool shouldLaunch = (SendMessageW(state->hChkLaunch, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (shouldLaunch && !state->installedExePath.empty()) {
                        ShellExecuteW(NULL, L"open", state->installedExePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                    DestroyWindow(hwnd);
                    return 0;
                }

                wchar_t customDirBuf[MAX_PATH * 2] = {0};
                GetWindowTextW(state->hPathEdit, customDirBuf, MAX_PATH * 2 - 1);

                // Perform install
                EnableWindow(state->hBtnInstall, FALSE);
                EnableWindow(state->hBtnCancel, FALSE);
                EnableWindow(state->hPathEdit, FALSE);
                EnableWindow(state->hBtnBrowse, FALSE);
                EnableWindow(state->hChkDesktop, FALSE);
                EnableWindow(state->hChkStartMenu, FALSE);
                EnableWindow(state->hChkContextMenu, FALSE);

                SetWindowTextW(state->hStatus, L"Копирование исполняемых файлов...");
                SendMessageW(state->hProgress, PBM_SETPOS, 25, 0);
                UpdateWindow(hwnd);

                bool desktop = (SendMessageW(state->hChkDesktop, BM_GETCHECK, 0, 0) == BST_CHECKED);
                bool startMenu = (SendMessageW(state->hChkStartMenu, BM_GETCHECK, 0, 0) == BST_CHECKED);
                bool contextMenu = (SendMessageW(state->hChkContextMenu, BM_GETCHECK, 0, 0) == BST_CHECKED);

                Sleep(100);
                SetWindowTextW(state->hStatus, L"Создание ярлыков...");
                SendMessageW(state->hProgress, PBM_SETPOS, 60, 0);
                UpdateWindow(hwnd);

                std::wstring installedExe;
                bool ok = PerformInstall(hwnd, desktop, startMenu, contextMenu, installedExe, customDirBuf);
                state->installedExePath = installedExe;

                if (ok) {
                    SendMessageW(state->hProgress, PBM_SETPOS, 100, 0);
                    SetWindowTextW(state->hStatus, L"Установка успешно завершена!");
                    SetWindowTextW(state->hBtnInstall, L"Завершить");
                    EnableWindow(state->hBtnInstall, TRUE);
                    ShowWindow(state->hBtnCancel, SW_HIDE);
                    state->isFinished = true;
                } else {
                    SetWindowTextW(state->hStatus, L"Ошибка при установке. Проверьте путь и права доступа.");
                    EnableWindow(state->hBtnInstall, TRUE);
                    EnableWindow(state->hBtnCancel, TRUE);
                    EnableWindow(state->hPathEdit, TRUE);
                    EnableWindow(state->hBtnBrowse, TRUE);
                    EnableWindow(state->hChkDesktop, TRUE);
                    EnableWindow(state->hChkStartMenu, TRUE);
                    EnableWindow(state->hChkContextMenu, TRUE);
                }
                return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, RGB(20, 20, 20));
            return (INT_PTR)GetStockObject(NULL_BRUSH);
        }

        case WM_DESTROY: {
            if (state) {
                if (state->hFontTitle) DeleteObject(state->hFontTitle);
                if (state->hFontRegular) DeleteObject(state->hFontRegular);
                if (state->hFontBold) DeleteObject(state->hFontBold);
            }
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int RunSetupWizard(HINSTANCE hInstance, int nCmdShow, bool isUpdateMode) {
    if (isUpdateMode) {
        // Fast background install without wizard dialog
        std::wstring installedExe;
        bool ok = PerformInstall(NULL, true, true, true, installedExe);
        if (ok) {
            ShellExecuteW(NULL, L"open", installedExe.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        return 0;
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WizardWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WIZARD_CLASS_NAME;
    RegisterClassExW(&wc);

    int winW = 530;
    int winH = 440;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;

    WizardState state;

    HWND hwnd = CreateWindowExW(
        0, WIZARD_CLASS_NAME,
        L"Установка Modo Studio",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, winW, winH,
        NULL, NULL, hInstance, &state
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

} // namespace ModoInstaller
