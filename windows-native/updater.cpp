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
#include <wincrypt.h>
#include <wininet.h>
#include <shellapi.h>
#include <cwchar>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>

namespace ModoUpdater {

static const wchar_t* CURRENT_VERSION = L"1.7.0";
static const wchar_t* MANIFEST_URL = L"https://raw.githubusercontent.com/Tasonai/Modo/main/dist/update.json";
static const size_t MAX_MANIFEST_BYTES = 2 * 1024 * 1024;
static const DWORD MAX_INSTALLER_BYTES = 64 * 1024 * 1024;

const wchar_t* GetCurrentAppVersion() {
    return CURRENT_VERSION;
}

static HWND SafeParent(HWND hWnd) {
    return (hWnd && IsWindow(hWnd)) ? hWnd : NULL;
}

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    if (len <= 0) return std::wstring();
    std::wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &out[0], len);
    return out;
}

static std::wstring SanitizeVersionTag(const std::wstring& ver) {
    std::wstring clean;
    for (wchar_t c : ver) {
        if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'z') ||
            (c >= L'A' && c <= L'Z') || c == L'.' || c == L'-' || c == L'_') {
            clean.push_back(c);
            if (clean.size() >= 24) break;
        }
    }
    return clean;
}

static bool IsValidHexSha256(const std::string& hex) {
    if (hex.size() != 64) return false;
    for (unsigned char c : hex) {
        if (!std::isxdigit(c)) return false;
    }
    return true;
}

static bool ComputeFileSha256Hex(const std::wstring& filePath, std::string& outHex) {
    outHex.clear();
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CloseHandle(hFile);
        return false;
    }
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return false;
    }

    BYTE buf[16384];
    DWORD bytesRead = 0;
    bool ok = true;
    while (true) {
        if (!ReadFile(hFile, buf, sizeof(buf), &bytesRead, NULL)) {
            ok = false;
            break;
        }
        if (bytesRead == 0) break;
        if (!CryptHashData(hHash, buf, bytesRead, 0)) {
            ok = false;
            break;
        }
    }

    BYTE digest[32] = {0};
    DWORD digestLen = sizeof(digest);
    if (ok && CryptGetHashParam(hHash, HP_HASHVAL, digest, &digestLen, 0) && digestLen == 32) {
        static const char kHex[] = "0123456789abcdef";
        outHex.reserve(64);
        for (DWORD i = 0; i < 32; ++i) {
            outHex.push_back(kHex[(digest[i] >> 4) & 0xF]);
            outHex.push_back(kHex[digest[i] & 0xF]);
        }
    } else {
        ok = false;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);
    return ok;
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

static bool ExtractWindowsSetupAsset(const std::string& json, std::string& outUrl, std::string& outSha256) {
    outUrl.clear();
    outSha256.clear();
    size_t pos = json.find("\"Modo-Windows-Setup.exe\"");
    if (pos == std::string::npos) {
        pos = json.find("\"Modo-Windows-Portable.exe\"");
    }
    if (pos == std::string::npos) return false;

    size_t objStart = json.rfind('{', pos);
    size_t objEnd = json.find('}', pos);
    if (objStart == std::string::npos || objEnd == std::string::npos || objEnd <= objStart) {
        return false;
    }
    std::string assetSlice = json.substr(objStart, objEnd - objStart + 1);
    outUrl = ExtractJsonStringField(assetSlice, "downloadURL");
    outSha256 = ExtractJsonStringField(assetSlice, "sha256");
    for (char& c : outSha256) c = (char)std::tolower((unsigned char)c);
    return !outUrl.empty();
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
        if (!silent && SafeParent(hParent)) {
            MessageBoxW(SafeParent(hParent), L"Не удалось инициализировать сетевое подключение WinINet.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
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
        if (!silent && SafeParent(hParent)) {
            MessageBoxW(SafeParent(hParent), L"Не удалось получить информацию об обновлениях с сервера.\nПроверьте подключение к Интернету.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    std::string json;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (json.size() + bytesRead > MAX_MANIFEST_BYTES) {
            json.clear();
            break;
        }
        json.append(buffer, bytesRead);
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (json.empty()) {
        if (!silent && SafeParent(hParent)) {
            MessageBoxW(SafeParent(hParent), L"Сервер обновлений вернул пустой или некорректный ответ.", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    std::string remoteVerA = ExtractJsonStringField(json, "version");
    std::string releaseNotesA = ExtractJsonStringField(json, "releaseNotes");
    std::string downloadUrlA;
    std::string expectedSha256;
    ExtractWindowsSetupAsset(json, downloadUrlA, expectedSha256);

    std::wstring remoteVer = SanitizeVersionTag(Utf8ToWide(remoteVerA));
    std::wstring releaseNotes = Utf8ToWide(releaseNotesA);
    std::wstring downloadUrl = Utf8ToWide(downloadUrlA);

    if (remoteVer.empty() || downloadUrl.rfind(L"https://", 0) != 0 || !IsValidHexSha256(expectedSha256)) {
        if (!silent && SafeParent(hParent)) {
            MessageBoxW(SafeParent(hParent), L"Манифест обновления не прошёл проверку безопасности (требуется HTTPS и контрольная сумма SHA-256).", L"Обновление Modo Studio", MB_OK | MB_ICONWARNING);
        }
        return 1;
    }

    if (!IsNewerVersion(remoteVer, CURRENT_VERSION)) {
        if (!silent && SafeParent(hParent)) {
            std::wstring msg = L"У вас установлена последняя версия Modo Studio (v" + std::wstring(CURRENT_VERSION) + L").\nОбновлений не требуется.";
            MessageBoxW(SafeParent(hParent), msg.c_str(), L"Обновление Modo Studio", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    std::wstring prompt = L"Доступна новая версия Modo Studio: v" + remoteVer + L" (текущая: v" + std::wstring(CURRENT_VERSION) + L").\n\n";
    if (!releaseNotes.empty()) {
        prompt += L"Изменения в новой версии:\n" + releaseNotes + L"\n\n";
    }
    prompt += L"Хотите загрузить и установить обновление сейчас?";

    int choice = MessageBoxW(SafeParent(hParent), prompt.c_str(), L"Доступно обновление Modo Studio", MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON1);
    if (choice != IDYES) {
        return 0;
    }

    wchar_t tempDir[MAX_PATH] = {0};
    DWORD tempLen = GetTempPathW(MAX_PATH, tempDir);
    if (tempLen == 0 || tempLen >= MAX_PATH) {
        return 1;
    }
    wchar_t uniqueName[128] = {0};
    swprintf(uniqueName, sizeof(uniqueName) / sizeof(wchar_t), L"Modo-Setup-v%ls-%lu-%lu.exe",
             remoteVer.c_str(), (unsigned long)GetCurrentProcessId(), (unsigned long)GetTickCount());
    std::wstring tempSetupPath = std::wstring(tempDir) + uniqueName;

    HINTERNET hDlInternet = InternetOpenW(L"Modo Studio (Windows NT; Download)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hDlInternet) {
        if (SafeParent(hParent)) MessageBoxW(SafeParent(hParent), L"Не удалось начать загрузку обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    HINTERNET hDlUrl = InternetOpenUrlW(hDlInternet, downloadUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hDlUrl) {
        InternetCloseHandle(hDlInternet);
        if (SafeParent(hParent)) MessageBoxW(SafeParent(hParent), L"Не удалось подключиться к серверу для загрузки файла обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    // CREATE_NEW prevents pre-created symlink / file hijack in %TEMP%
    HANDLE hFile = CreateFileW(tempSetupPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hDlUrl);
        InternetCloseHandle(hDlInternet);
        if (SafeParent(hParent)) MessageBoxW(SafeParent(hParent), L"Не удалось создать защищённый временный файл обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    char dlBuffer[16384];
    DWORD dlRead = 0;
    DWORD totalDownloaded = 0;
    bool dlSuccess = true;
    while (InternetReadFile(hDlUrl, dlBuffer, sizeof(dlBuffer), &dlRead) && dlRead > 0) {
        if (totalDownloaded + dlRead > MAX_INSTALLER_BYTES) {
            dlSuccess = false;
            break;
        }
        DWORD written = 0;
        if (!WriteFile(hFile, dlBuffer, dlRead, &written, NULL) || written != dlRead) {
            dlSuccess = false;
            break;
        }
        totalDownloaded += dlRead;
    }

    CloseHandle(hFile);
    InternetCloseHandle(hDlUrl);
    InternetCloseHandle(hDlInternet);

    if (!dlSuccess || totalDownloaded < 4096) {
        DeleteFileW(tempSetupPath.c_str());
        if (SafeParent(hParent)) MessageBoxW(SafeParent(hParent), L"Произошла ошибка при загрузке обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Mandatory SHA-256 verification before executing the downloaded installer
    std::string actualSha256;
    if (!ComputeFileSha256Hex(tempSetupPath, actualSha256) || actualSha256 != expectedSha256) {
        DeleteFileW(tempSetupPath.c_str());
        if (SafeParent(hParent)) {
            MessageBoxW(SafeParent(hParent),
                        L"Проверка целостности не пройдена: контрольная сумма SHA-256 загруженного файла не совпадает с эталонной. Установка отменена.",
                        L"Ошибка безопасности обновления", MB_OK | MB_ICONERROR);
        }
        return 1;
    }

    HINSTANCE hInst = ShellExecuteW(NULL, L"open", tempSetupPath.c_str(), L"--update", NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hInst > 32) {
        HWND validWnd = SafeParent(hParent);
        if (validWnd) {
            PostMessageW(validWnd, WM_CLOSE, 0, 0);
        } else {
            ExitProcess(0);
        }
    } else {
        DeleteFileW(tempSetupPath.c_str());
        if (SafeParent(hParent)) MessageBoxW(SafeParent(hParent), L"Не удалось запустить мастер обновления.", L"Ошибка обновления", MB_OK | MB_ICONERROR);
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
