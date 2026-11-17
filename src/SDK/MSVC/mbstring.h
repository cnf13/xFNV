#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"
#include <cstring>

// The Microsoft-only multibyte functions of VC9's <mbstring.h> the game calls, under their own names. No CRT header
// the program includes declares them (the Windows CRT's <mbstring.h> is not included), so they are defined here and
// in mbstring.cpp on every target.
//
// The CRT starts with the system's ANSI code page (_setmbcp(_MB_CP_ANSI)), 1252 for the game's languages: a
// single-byte code page, for which each _mbs*_l function (tested on the code page's ismbcodepage) takes the
// single-byte path written here: the str* function after the _mbs* function's own argument checks. The casts between
// unsigned char and char strings are the CRT's own: the same bytes.

// _mbsicmp, 0xEC858A (_mbsicmp_l with a null locale): both pointers checked (EINVAL, 0x7FFFFFFF), then _stricmp_l.
inline int32_t _mbsicmp(const unsigned char* apStr1, const unsigned char* apStr2)
{
	return _stricmp(reinterpret_cast<const char*>(apStr1), reinterpret_cast<const char*>(apStr2));
}

// _mbscmp, 0xEC8244 (_mbscmp_l): both pointers checked (EINVAL, 0x7FFFFFFF), then strcmp. Defined in mbstring.cpp.
int32_t _mbscmp(const unsigned char* apStr1, const unsigned char* apStr2);

// _mbsnbcmp, 0xECB849 (_mbsnbcmp_l): strncmp, before any argument check.
inline int32_t _mbsnbcmp(const unsigned char* apStr1, const unsigned char* apStr2, size_t astCount)
{
	return std::strncmp(reinterpret_cast<const char*>(apStr1), reinterpret_cast<const char*>(apStr2), astCount);
}

// _mbsnbicmp, 0xECBA8E (_mbsnbicmp_l): _strnicmp, before any argument check of its own.
inline int32_t _mbsnbicmp(const unsigned char* apStr1, const unsigned char* apStr2, size_t astCount)
{
	return _strnicmp(reinterpret_cast<const char*>(apStr1), reinterpret_cast<const char*>(apStr2), astCount);
}

// _mbsnbcpy, 0xECA903 (_mbsnbcpy_l): a null pointer with a nonzero count is EINVAL (null returned), else strncpy.
// Defined in mbstring.cpp.
unsigned char* _mbsnbcpy(unsigned char* apDest, const unsigned char* apSource, size_t astCount);

// _mbstok, 0xECE23E (_mbstok_l, _mbstok_s_l with the thread's own _mtoken context): strtok_s after the checks.
// Defined in mbstring.cpp.
unsigned char* _mbstok(unsigned char* apStr, const unsigned char* apDelim);
