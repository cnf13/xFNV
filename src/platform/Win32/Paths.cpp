// Paths on Windows: the Win32 calls the exe makes, unchanged.
#include "platform/Paths.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <shlobj.h>

namespace platform {

    std::filesystem::path NativePath(std::string_view gamePath) {
        return std::filesystem::path(gamePath);
    }

    namespace {

        uint64_t FileTime(const FILETIME& time) {
            return (uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime;
        }

    } // namespace

    std::vector<FileEntry> FindFiles(std::string_view gamePattern) {
        std::vector<FileEntry> entries;
        const std::string pattern(gamePattern);
        WIN32_FIND_DATAA data;
        HANDLE find = FindFirstFileA(pattern.c_str(), &data);
        if (find == INVALID_HANDLE_VALUE) {
            return entries;
        }
        do {
            entries.push_back(
                {data.cFileName, (uint64_t(data.nFileSizeHigh) << 32) | data.nFileSizeLow, data.dwFileAttributes,
                 FileTime(data.ftLastWriteTime), FileTime(data.ftCreationTime), FileTime(data.ftLastAccessTime),
                 data.cAlternateFileName}
            );
        }
        while (FindNextFileA(find, &data));
        FindClose(find);
        return entries;
    }

    std::string UserFolder(UserFolderKind kind) {
        char path[MAX_PATH];
        const int csidl = kind == UserFolderKind::LocalAppData ? CSIDL_LOCAL_APPDATA : CSIDL_PERSONAL;
        if (FAILED(SHGetFolderPathA(nullptr, csidl, nullptr, 0, path))) {
            return {};
        }
        return path;
    }

    std::string InstallFolder() {
        char path[MAX_PATH + 1] = {};
        HKEY key;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Bethesda Softworks\\FalloutNV", 0, KEY_QUERY_VALUE, &key) !=
            ERROR_SUCCESS) {
            return {};
        }
        DWORD size = MAX_PATH;
        const LSTATUS status =
            RegQueryValueExA(key, "Installed Path", nullptr, nullptr, reinterpret_cast<BYTE*>(path), &size);
        RegCloseKey(key);
        return status == ERROR_SUCCESS ? std::string(path) : std::string();
    }

    std::string UserName() {
        char name[257];
        DWORD size = sizeof(name);
        if (!GetUserNameA(name, &size)) {
            return {};
        }
        return name;
    }

} // namespace platform
