#pragma once

#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// The C-variadic function itself. A call without format arguments resolves to it.
// The format's arguments are passed on as a va_list; the formatter's count is returned unchanged.
inline int32_t BSsprintf(char* apString1, int32_t aiSize, const char* apFormat, ...) {
    va_list vList;
    va_start(vList, apFormat);
    int32_t iResult = vsprintf_s(apString1, aiSize, apFormat, vList);
    va_end(vList);
    return iResult;
}

// Case-sensitive compare.
int32_t BSstrcmp(const char* apString1, const char* apString2);
// Case-insensitive compare: the CRT _stricmp.
int32_t BSstricmp(const char* apString1, const char* apString2);
// Bounded copy of apString2 into an aiSize-byte buffer: the CRT strcpy_s; returns its errno.
// strcpy_s's result (0 or an errno) is returned unchanged.
inline int32_t BSstrcpy(char* apString1, int32_t aiSize, const char* apString2) {
    return strcpy_s(apString1, aiSize, apString2);
}

// Bounded append of apString2 to the string in an aiSize-byte buffer: the CRT strcat_s; returns its errno.
int32_t BSstrcat(char* apString1, int32_t aiSize, const char* apString2);
// String length: the CRT strlen.
uint32_t BSstrlen(const char* apString);
// Copies at most aiLen characters of apString2 into apString1: the CRT strncpy_s with a buffer of aiLen + 1;
// returns its errno.
int32_t BSstrncpy(char* apString1, const char* apString2, int32_t aiLen);
// Case-insensitive compare of at most auiCount characters.
int32_t BSstrnicmp(const char* apString1, const char* apString2, uint32_t aiCount);
// Bounded move of aiCount bytes from apSrc to apDest (a buffer of aiDestSize bytes): the CRT memmove_s; returns its
// errno.
int32_t BSmemmove(void* apDest, int32_t aiDestSize, void* apSrc, int32_t aiCount);

// vsnprintf into a buffer of aiSize bytes; returns vsnprintf's count. The template forwards calls with format arguments
// to the variadic BSsprintf: the arguments after the format get the `...` promotions.
template <class... A>
int32_t BSsprintf(char* apString1, int32_t aiSize, const char* apFormat, A... vList) {
    // The C-variadic overload by pointer: a plain call would pick this template again.
    int32_t (*pVariadic)(char*, int32_t, const char*, ...) = BSsprintf;
    return pVariadic(apString1, aiSize, apFormat, vList...);
}

// Case-insensitive strstr: the first place apString2 occurs in apString1, ignoring case; null when it does not.
inline const char* BSstristr(const char* apString1, const char* apString2) {
    int32_t iLen1 = std::strlen(apString1);
    int32_t iLen2 = std::strlen(apString2);
    for (int32_t i = 0; i < iLen1 - iLen2 + 1; ++i) {
        int32_t j;
        for (j = 0; j < iLen2; ++j) {
            int32_t iChar = std::tolower(apString1[j + i]);
            if (iChar != std::tolower(apString2[j])) {
                break;
            }
        }
        if (j == iLen2) {
            return &apString1[i];
        }
    }
    return nullptr;
}

inline uint32_t BSstrlen(const char* apString) {
    return std::strlen(apString);
}

inline int32_t BSstricmp(const char* apString1, const char* apString2) {
    return _stricmp(apString1, apString2);
}

inline int32_t BSstrlwr(char* apString1, int32_t aiSize) {
    return _strlwr_s(apString1, aiSize);
}

inline int32_t BSstrncpy(char* apString1, const char* apString2, int32_t aiLen) {
    return strncpy_s(apString1, aiLen + 1, apString2, aiLen);
}

inline void BSmemcpy(void* apDest, int32_t aiDestSize, const void* apSrc, int32_t aiCount) {
    memcpy_s(apDest, aiDestSize, apSrc, aiCount);
}

inline int32_t BSvsprintf(char* apString1, const char* apFormat, va_list aList) {
    return std::vsprintf(apString1, apFormat, aList);
}

inline int32_t BSstrcat(char* apString1, int32_t aiSize, const char* apString2) {
    return strcat_s(apString1, aiSize, apString2);
}

inline int32_t BSstrcmp(const char* apString1, const char* apString2) {
    return std::strcmp(apString1, apString2);
}

inline int32_t BSstrnicmp(const char* apString1, const char* apString2, uint32_t aiCount) {
    return _strnicmp(apString1, apString2, aiCount);
}

inline uint32_t BSstrtoul(const char* apString, char** apEndPtr, int32_t aiBase) {
    return std::strtoul(apString, apEndPtr, aiBase);
}

inline int32_t BSstrupr(char* apString1, int32_t aiSize) {
    return _strupr_s(apString1, aiSize);
}

// The int32_t sizes are passed on to memmove_s as 32-bit unsigned values, zero-extended.
inline int32_t BSmemmove(void* apDest, int32_t aiDestSize, void* apSrc, int32_t aiCount) {
    return memmove_s(apDest, static_cast<uint32_t>(aiDestSize), apSrc, static_cast<uint32_t>(aiCount));
}

// Prints the formatted text through the CRT's vprintf.
// The format's arguments go to vprintf as the argument list.
inline int32_t BSprintf(const char* apFormat, ...) {
    va_list varArgs;
    va_start(varArgs, apFormat);
    int32_t iResult = std::vprintf(apFormat, varArgs);
    va_end(varArgs);
    return iResult;
}
