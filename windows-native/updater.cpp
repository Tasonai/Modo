#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "updater.h"

#include <windows.h>
#include <wininet.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <sstream>

namespace ModoUpdater {

static const wchar_t* CURRENT_VERSION = L"1.7.0";
static const wchar_t* MANIFEST_URL = L"https://raw.githubusercontent.com/Tasonai/Modo/main/dist/update.json";

const wchar_t* GetCurrentAppVersion() {
    return CURRENT_VERSION;
}

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    if (len <= 0) return std::wstring();
    std::wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &out[0], len);
    return out;
}

static std::vector<int> ParseVersionParts(const std::wstring& ver) {
    std::vector<int> parts;
    int curr = 0;
    bool inNum = false;
    for (wchar_t c : ver) {
        if (c >= L'0' && c <= L'9') {
            curr = curr * 10 + (c - L'0');
            inNum = true;
        } else if (c == L'.' || c == L'-') {
            if (inNum) {
                parts.push_back(curr);
                curr = 0;
                inNum = false;
            }
        }
    }
    if (inNum) parts.push_back(curr);
    while (parts.size() < 3) parts.push_back(0);
    return parts;
}

static bool IsNewerVersion(const std::wstring& remote, const std::wstring& local) {
    std::vector<int> r = ParseVersionParts(remote);
    std::vector<int> l = ParseVersionParts(local);
    for (size_t i = 0; i < std::max(r.size(), l.size()); ++i) {
        int rVal = (i < r.size()) ? r[i] : 0;
        int lVal = (i < l.size()) ? l[i] : 0;
        if (rVal > lVal) return true;
        if (rVal < lVal) return false;
    }
    return false;
}

static std::string ExtractJsonStringField(const std::string& json, const std::string& fieldName) {
    std::string key = "\"" + fieldName + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";
    pos += key.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.length() || json[pos] != '"') return "";
    pos++;
    std::string result;
    bool escaped = false;
    while (pos < json.length()) {
        char c = json[pos++];
        if (escaped) {
            if (c == 'n') result += '\n';
            else if (c == 'r') result += '\r';
            else if (c == 't') result += '\t';
            else result += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        } else {
            result += c;
        }
    }
    return result;
}

static std::string ExtractWindowsSetupUrl(const std::string& json) {
    // Look for Modo-Windows-Setup.exe asset URL
    size_t pos = json.find("Modo-Windows-Setup.exe");
    if (pos == std::string::npos) {
        pos = json.find("Modo-Windows-Portable.exe");
    }
    if (pos == std::string::npos) return "";

    std::string key = "\"downloadURL\"";
    size_t urlPos = json.find(key, pos);
    if (urlPos == std::string::npos) {
        // Search backwards in case downloadURL was before name
        urlPos = json.rfind(key, pos);
    }
    if (urlPos == std::string::npos) return "";

    urlPos += key.length();
    while (urlPos < json.length() && (json[urlPos] == ' ' || json[urlPos] == ':' || json[urlPos] == '\t' || json[urlPos] == '\r' || json[urlPos] == '\n')) {
        urlPos++;
    }
    if (urlPos >= json.length() || json[urlPos] != '"') return "";
    urlPos++;
    std::string result;
    while (urlPos < json.length() && json[urlPos] != '"') {
        result += json[urlPos++];
    }
    return result;
}

struct UpdateCheckContext {
    HWND hParent;
    bool silent;
};

static DWORD WINAPI UpdateCheckWorker(LPVOID param) {
    UpdateCheckContext* ctx = reinterpret_cast<UpdateCheckContext*>(param);
    HWND hParent = ctx->hParent;
    bool silent = ctx->silent;
    delete ctx;

    HINTERNET hInternet = InternetOpenW(
        L"Modo Studio/1.7.0 (Windows NT; Atoma Updater)",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL, NULL, 0
    );
    if (!hInternet) {
        if (!silent && hParent) {
            MessageBoxW(hParent, L"Не удалось инициализировать сетевое подключение WinINet.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    HINTERNET hUrl = InternetOpenUrlW(
        hInternet,
        MANIFEST_URL,
        NULL, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
        0
    );

    if (!hUrl) {
        InternetCloseHandle(hInternet);
        if (!silent && hParent) {
            MessageBoxW(hParent, L"Не удалось получить информацию об обновлениях с сервера.\nПроверьте подключение к Интернету.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    std::string json;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        json.append(buffer, bytesRead);
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (json.empty()) {
        if (!silent && hParent) {
            MessageBoxW(hParent, L"Сервер обновлений вернул пустой ответ.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    std::string remoteVerA = ExtractJsonStringField(json, "version");
    std::string releaseNotesA = ExtractJsonStringField(json, "releaseNotes");
    std::string downloadUrlA = ExtractWindowsSetupUrl(json);

    std::wstring remoteVer = Utf8ToWide(remoteVerA);
    std::wstring releaseNotes = Utf8ToWide(releaseNotesA);
    std::wstring downloadUrl = Utf8ToWide(downloadUrlA);

    if (remoteVer.empty() || downloadUrl.empty()) {
        if (!silent && hParent) {
            MessageBoxW(hParent, L"Не удалось разобрать манифест обновления.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    if (!IsNewerVersion(remoteVer, CURRENT_VERSION)) {
        if (!silent && hParent) {
            std::wstring msg = L"У вас установлена последняя версия Modo Studio (v" + std::wstring(CURRENT_VERSION) + L").\nОбновлений не требуется.";
            MessageBoxW(hParent, msg.c_str(), L"Обновление Modo Studio", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    // New version available!
    std::wstring prompt = L"Доступна новая версия Modo Studio: v" + remoteVer + L" (текущая: v" + std::wstring(CURRENT_VERSION) + L").\n\n";
    if (!releaseNotes.empty()) {
        prompt += L"Изменения в новой версии:\n" + releaseNotes + L"\n\n";
    }
    prompt += L"Хотите загрузить и установить обновление сейчас?";

    int choice = MessageBoxW(hParent, prompt.c_str(), L"Доступно обновление Modo Studio", MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON1);
    if (choice != IDYES) {
        return 0;
    }

    // User confirmed download!
    wchar_t tempDir[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring tempSetupPath = std::wstring(tempDir) + L"Modo-Setup-v" + remoteVer + L".exe";

    HINTERNET hDlInternet = InternetOpenW(L"Modo Studio (Windows NT; Download)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hDlInternet) {
        if (hParent) MessageBoxW(hParent, L"Не удалось начать загрузку обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    HINTERNET hDlUrl = InternetOpenUrlW(hDlInternet, downloadUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hDlUrl) {
        InternetCloseHandle(hDlInternet);
        if (hParent) MessageBoxW(hParent, L"Не удалось подключиться к серверу для загрузки файла обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    HANDLE hFile = CreateFileW(tempSetupPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hDlUrl);
        InternetCloseHandle(hDlInternet);
        if (hParent) MessageBoxW(hParent, L"Не удалось создать временный файл обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    char dlBuffer[16384];
    DWORD dlRead = 0;
    bool dlSuccess = true;
    while (InternetReadFile(hDlUrl, dlBuffer, sizeof(dlBuffer), &dlRead) && dlRead > 0) {
        DWORD written = 0;
        if (!WriteFile(hFile, dlBuffer, dlRead, &written, NULL) || written != dlRead) {
            dlSuccess = false;
            break;
        }
    }

    CloseHandle(hFile);
    InternetCloseHandle(hDlUrl);
    InternetCloseHandle(hDlInternet);

    if (!dlSuccess) {
        DeleteFileW(tempSetupPath.c_str());
        if (hParent) MessageBoxW(hParent, L"Произошла ошибка при загрузке обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Launch setup executable with --update flag and close current application
    HINSTANCE hInst = ShellExecuteW(NULL, L"open", tempSetupPath.c_str(), L"--update", NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hInst > 32) {
        if (hParent) {
            PostMessage(hParent, WM_CLOSE, 0, 0);
        } else {
            ExitProcess(0);
        }
    } else {
        if (hParent) MessageBoxW(hParent, L"Не удалось запустить мастер обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
    }

    return 0;
}

void CheckForUpdates(HWND hParentWnd, bool silent) {
    UpdateCheckContext* ctx = new UpdateCheckContext{ hParentWnd, silent };
    HANDLE hThread = CreateThread(NULL, 0, UpdateCheckWorker, ctx, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        delete ctx;
    }
}

} // namespace ModoUpdater
