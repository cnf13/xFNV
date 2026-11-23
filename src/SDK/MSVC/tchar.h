#pragma once

#include <cstddef>
#include <cstdint>

// The Microsoft-only functions of VC9's <tchar.h> in its _MBCS form that the exe compiled out of line, under their
// own names. No CRT header the program includes declares them (the Windows CRT's <tchar.h>, where they are macros, is
// not included), so they are defined in tchar.cpp on every target.

// _tcscmp, 0x469880 (cdecl). Under _MBCS, VC9's <tchar.h> defines it inline, casting to unsigned char and calling
// _mbscmp; the exe compiled it out of line.
int32_t _tcscmp(const char* apStr1, const char* apStr2);

// _tcsicmp, 0x46FD30 (cdecl). Under _MBCS, VC9's <tchar.h> defines it inline, casting to unsigned char and calling
// _mbsicmp (0xEC858A); the exe compiled it out of line.
int32_t _tcsicmp(const char* _String1, const char* _String2);

// _tcsncmp, 0x71CFD0 (cdecl). Under _MBCS, VC9's <tchar.h> defines it inline, casting to unsigned char and calling
// _mbsnbcmp (0xECB849); the exe compiled it out of line (IDA: _mbsnbcmp_).
int32_t _tcsncmp(const char* _s1, const char* _s2, size_t _n);

// _tcsnicmp, 0x71CFF0 (cdecl). Under _MBCS, VC9's <tchar.h> defines it inline, casting to unsigned char and calling
// _mbsnbicmp (0xECBA8E); the exe compiled it out of line (IDA: _mbsnbicmp_).
int32_t _tcsnicmp(const char* _String1, const char* _String2, size_t _Char_count);
