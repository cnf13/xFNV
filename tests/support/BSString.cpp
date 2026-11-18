// Stand-ins for BSCore/BSString.h's functions, as the game's own: each passes straight to the C runtime's (the
// reconstructed Microsoft one, src/SDK/MSVC, where it differs from the platform's).
#include "BSCore/BSString.h"

#include "SDK/MSVC/string.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

int32_t BSstricmp(const char* apString1, const char* apString2) {
    return _stricmp(apString1, apString2);
}

int32_t BSstrcpy(char* apString1, int32_t aiSize, const char* apString2) {
    return strcpy_s(apString1, aiSize, apString2);
}

int32_t BSstrcat(char* apString1, int32_t aiSize, const char* apString2) {
    return strcat_s(apString1, aiSize, apString2);
}

uint32_t BSstrlen(const char* apString) {
    return std::strlen(apString);
}

int32_t BSvsprintf(char* apString1, const char* apFormat, va_list aList) {
    return std::vsprintf(apString1, apFormat, aList);
}
