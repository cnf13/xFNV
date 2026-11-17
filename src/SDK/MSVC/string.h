#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// The Microsoft-only functions of VC9's <string.h> the game calls, under their own names, and two functions on the
// game's 2-byte characters (rt). The standard functions are std:: (<cstring>).
//
// The Windows CRT declares and defines every one of the Microsoft-only functions itself (in its <string.h>, which
// <cstring> includes), with the contract written here: a second definition would be a second definition of the CRT's
// own C function. So on Windows the CRT's are used, and elsewhere they are declared here and string.cpp defines them
// to VC9's rules in the C locale (the game never sets another). On an invalid argument they store errno, call
// rt::invalid_parameter (which ends the process, crtdefs.h) and return the error.
#if !defined(_WIN32)

// memcpy_s, 0xEC7C66: copies astCount bytes; 0, or EINVAL (null pointer) / ERANGE (astCount > astDestSize), the
// destination zeroed first when it is not null.
int memcpy_s(void* apDest, size_t astDestSize, const void* apSrc, size_t astCount);

// memmove_s, 0xEC7CE3: as memcpy_s for overlapping buffers, but the destination is left as it is on an error.
int memmove_s(void* apDest, size_t astDestSize, const void* apSrc, size_t astCount);

// strcpy_s, 0xEC65A6: 0, EINVAL (null pointer or size 0) or ERANGE (the text does not fit: the bytes copied so far
// stay, the first is set to 0).
int strcpy_s(char* apcDest, size_t astDestSize, const char* apcSource);

// strcat_s, 0xEC6BD2: 0, EINVAL (null pointer, size 0 or an unterminated destination) or ERANGE, with the
// destination emptied on an error as in strcpy_s.
int strcat_s(char* apcDest, size_t astDestSize, const char* apcSource);

// strncpy_s, 0xEC8D5F: copies at most astCount characters and terminates; with astCount _TRUNCATE (rt::kTRUNCATE), a
// text that does not fit is cut to astDestSize - 1 characters and STRUNCATE returned, else it is an ERANGE error.
int strncpy_s(char* apDest, size_t astDestSize, const char* apSrc, size_t astCount);

// _stricmp, 0xEC68E4: compares with A-Z folded to a-z; the difference of the first differing folded bytes (unsigned).
// 0x7FFFFFFF for a null pointer.
int _stricmp(const char* apStr1, const char* apStr2);

// _strnicmp, 0xEC7EC0: _stricmp on at most astCount bytes, but returning -1, 0 or 1 (the CRT's assembly loop).
int _strnicmp(const char* apStr1, const char* apStr2, size_t astCount);

// strtok_s, 0xECC5F8 (the PDB's NiStrtok): the next token of apString (null: continue from *appContext), split at
// any byte of apDelimit, or null when none is left; null with EINVAL for a null context or delimiter list.
char* strtok_s(char* apString, const char* apDelimit, char** appContext);

// _strlwr_s, 0xEC8FEB: lower-cases A-Z of apString in place; 0, or EINVAL (null, or not terminated within astSize:
// then emptied).
int _strlwr_s(char* apString, size_t astSize);

// _strupr_s, 0xECBC5B (the PDB's BSstrupr): _strlwr_s's upper-case form.
int _strupr_s(char* apString, size_t astSize);

#endif

namespace rt {

    // VC9's memcpy copies overlapping ranges right (it picks the direction, as memmove does), and the game relies on it
    // (CombatController::AddFailedCombatProcedure shifts its list down in place); std::memcpy does not promise that.
    inline void* memcpy(void* apDest, const void* apSrc, size_t astCount) {
        return std::memmove(apDest, apSrc, astCount);
    }

    // wcslen, 0xECDB00 (cdecl): the length in char16_t elements of a string of the game's 2-byte characters (VC9's
    // wchar_t). Defined and registered in string.cpp.
    size_t wcslen(const char16_t* String);

    // _wcsicmp, 0xECC071: _stricmp on the game's 2-byte characters. Defined in string.cpp.
    int32_t _wcsicmp(const char16_t* apStr1, const char16_t* apStr2);

} // namespace rt
