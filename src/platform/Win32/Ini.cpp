// Ini files on Windows: kernel32's profile functions, as the exe calls them.
#include "platform/Ini.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace platform {

    uint32_t IniGetString(
        const char* file,
        const char* section,
        const char* key,
        const char* defaultValue,
        std::span<char> out
    ) {
        return GetPrivateProfileStringA(section, key, defaultValue, out.data(), static_cast<DWORD>(out.size()), file);
    }

    uint32_t IniGetKeys(const char* file, const char* section, const char* defaultValue, std::span<char> out) {
        return GetPrivateProfileStringA(
            section, nullptr, defaultValue, out.data(), static_cast<DWORD>(out.size()), file
        );
    }

    uint32_t IniGetInt(const char* file, const char* section, const char* key, int32_t defaultValue) {
        return GetPrivateProfileIntA(section, key, defaultValue, file);
    }

    bool IniWriteString(const char* file, const char* section, const char* key, const char* value) {
        return WritePrivateProfileStringA(section, key, value, file) != 0;
    }

} // namespace platform
