#pragma once

#include <cstdint>
#include <span>

// Ini files with the rules of kernel32's GetPrivateProfile*/WritePrivateProfile* (Win32: those functions; POSIX: a
// parser to their rules). Files are game paths (POSIX: through NativePath); strings are NUL-terminated.
namespace platform {

// GetPrivateProfileStringA: the key's value (else the default) into out, truncated to out.size() - 1 characters;
// the count written without the NUL. Section and key match case-insensitively, the first match wins, the value is
// trimmed and loses one pair of enclosing quotes. A null key lists the section's keys (IniGetKeys).
uint32_t IniGetString(const char* file, const char* section, const char* key, const char* defaultValue,
	std::span<char> out);

// GetPrivateProfileStringA with a null key: the section's key names, each NUL-terminated, the list ending in an
// extra NUL (a short buffer truncates the last name and returns out.size() - 2); the default when the section has
// none.
uint32_t IniGetKeys(const char* file, const char* section, const char* defaultValue, std::span<char> out);

// GetPrivateProfileIntA: the key's value read as an integer (sign, 0x/0o/0b prefixes, up to the first non-digit),
// else the default.
uint32_t IniGetInt(const char* file, const char* section, const char* key, int32_t defaultValue);

// WritePrivateProfileStringA: sets the key (creating the section, the key and the file as needed); a null value
// deletes the key, a null key the section. False when the file cannot be written.
bool IniWriteString(const char* file, const char* section, const char* key, const char* value);

}  // namespace platform
