#include "Havok/Source/Common/Base/Container/String/hkString.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdarg>
#include <cstring>

void hkString::strNcpy(char* dst, const char* src, int32_t n) {
    if (n) {
        std::strncpy(dst, src, n);
    }
}

const char* hkString::strChr(const char* haystack, int32_t needle) {
    return std::strchr(haystack, needle);
}

const char* hkString::strRchr(const char* haystack, int32_t needle) {
    return std::strrchr(haystack, needle);
}

char hkString::toLower(char c) {
    if (static_cast<uint8_t>(c - 'A') <= 'Z' - 'A') {
        return static_cast<char>(c + 32);
    }
    return c;
}

// No caller uses the result.
char* hkString::strCpy(char* dst, const char* src) {
    char* pcDest = dst;
    const char* pcSrc = src;
    char cChar;
    do {
        cChar = *pcSrc++;
        *pcDest++ = cChar;
    }
    while (cChar != 0);
    return const_cast<char*>(pcSrc);
}

// Case-insensitive compare of the lowered characters. The char compares are signed.
int32_t hkString::strCasecmp(const char* a, const char* b) {
    for (;;) {
        if (a[0] == 0 && b[0] == 0) {
            return 0;
        }
        const char cLeft = toLower(a[0]);
        const char cRight = toLower(b[0]);
        if (cLeft < cRight) {
            return -1;
        }
        if (cLeft > cRight) {
            return 1;
        }
        ++a;
        ++b;
    }
}

int32_t hkString::strLen(const char* src) {
    const char* pcEnd = src;
    while (*pcEnd != 0) {
        ++pcEnd;
    }
    return static_cast<int32_t>(pcEnd - src);
}

// The C run time's strtod, rounded to a float.
float hkString::atof(const char* in) {
    return static_cast<float>(std::strtod(in, nullptr));
}

char* hkString::strDup(const char* apcString, hkMemoryAllocator& apAllocator) {
    char* pcCopy = static_cast<char*>(hkMemoryRouter::easyAlloc(apAllocator, strLen(apcString) + 1));
    strCpy(pcCopy, apcString);
    return pcCopy;
}

char* hkString::strDup(const char* apcString) {
    hkMemoryRouter kRouter;
    kRouter.~hkMemoryRouter();
    ::new (&kRouter) hkMemoryRouter;
    return strDup(apcString, *hkMemoryRouter::getInstance().m_heap);
}

char* hkString::strNdup(const char* apcString, int32_t aiMaxLen, hkMemoryAllocator& apAllocator) {
    int32_t v3 = hkString::strLen(apcString);
    if (v3 > aiMaxLen) {
        v3 = aiMaxLen;
    }
    char* v4 = static_cast<char*>(hkMemoryRouter::easyAlloc(apAllocator, v3 + 1));
    hkString::strNcpy(v4, apcString, v3);
    v4[v3] = 0;
    return v4;
}

// A memory router is deliberately constructed on the stack and never used, as the game does; the copy comes from the
// thread's router.
char* hkString::strNdup(const char* apcString, int32_t aiMaxLen) {
    hkMemoryRouter kRouter;
    kRouter.~hkMemoryRouter();
    ::new (&kRouter) hkMemoryRouter;
    return strNdup(apcString, aiMaxLen, *hkMemoryRouter::getInstance().m_heap);
}

void hkString::strFree(char* s) {
    hkMemoryRouter* instance = &hkMemoryRouter::getInstance();
    hkString::strFree_00CB1190(s, *instance->m_heap);
}

// Compares the unsigned characters; returns -1, 0 or 1.
int32_t hkString::strCmp(const char* a, const char* b) {
    for (;;) {
        const uint8_t ucLeft = static_cast<uint8_t>(*a);
        const uint8_t ucRight = static_cast<uint8_t>(*b);
        if (ucLeft != ucRight) {
            return ucLeft < ucRight ? -1 : 1;
        }
        if (ucLeft == 0) {
            return 0;
        }
        ++a;
        ++b;
    }
}

// The engine's own is a jump into the C run time's memcpy; the copied bytes match.
void* hkString::memCpy(void* dst, const void* src, int32_t n) {
    return std::memcpy(dst, src, static_cast<size_t>(n));
}

// Forwards to the C run time's memmove, its int size passed on as memmove's size_t.
void hkString::memMove(void* dst, const void* src, int32_t n) {
    std::memmove(dst, src, static_cast<size_t>(n));
}

void hkString::memSet(void* dst, int32_t c, int32_t n) {
    std::memset(dst, c, n);
}

// Whether apcString ends with apcEnding; the lengths compare signed.
hkBool hkString::endsWith(const char* apcString, const char* apcEnding) {
    const int32_t iStringLen = strLen(apcString);
    const int32_t iEndingLen = strLen(apcEnding);
    if (iStringLen < iEndingLen) {
        return 0;
    }
    const char* pcTail = apcString + (iStringLen - iEndingLen);
    for (int32_t i = 0; i < iEndingLen; ++i) {
        if (pcTail[i] != apcEnding[i]) {
            return 0;
        }
    }
    return 1;
}

int32_t hkString::strNcmp(const char* a, const char* b, int32_t a3) {
    return std::strncmp(a, b, a3);
}

int32_t hkString::vsnprintf(char* a1, int32_t a2, const char* a3, va_list a4) {
    return _vsnprintf(a1, a2, a3, a4);
}

// The upper-case of a lower-case letter, else the character.
char hkString::toUpper(char c) {
    if (static_cast<uint8_t>(c - 97) <= 25u) {
        return c - 32;
    }
    return c;
}

// Formats into apcBuffer with at most aiMaxLen characters; the argument list is the caller's stack.
int32_t hkString::snprintf(char* buf, int32_t len, const char* fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);
    return _vsnprintf(buf, static_cast<size_t>(len), fmt, arguments);
}

// Formats into apcBuffer without a bound.
int32_t hkString::sprintf(char* buf, const char* fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);
    return std::vsprintf(buf, fmt, arguments);
}

// Case-insensitive compare of at most aiMaxLen characters: it stops at the first differing lowered character, when
// both strings end, or after aiMaxLen characters.
int32_t hkString::strNcasecmp(const char* a, const char* b, int32_t n) {
    for (int32_t i = 0;; ++i) {
        if (a[i] == 0 && b[i] == 0) {
            return 0;
        }
        if (i >= n) {
            return 0;
        }
        const char cLeft = toLower(a[i]);
        const char cRight = toLower(b[i]);
        if (cLeft < cRight) {
            return -1;
        }
        if (cLeft > cRight) {
            return 1;
        }
    }
}

// Lower-cases apcString in place and returns it.
char* hkString::strLwr(char* s) {
    for (char* pcChar = s; *pcChar != 0; ++pcChar) {
        *pcChar = toLower(*pcChar);
    }
    return s;
}

// Upper-cases apcString in place and returns it.
char* hkString::strUpr(char* s) {
    for (char* pcChar = s; *pcChar != 0; ++pcChar) {
        *pcChar = toUpper(*pcChar);
    }
    return s;
}

// The first occurrence of needle in haystack, or null.
const char* hkString::strStr(const char* haystack, const char* needle) {
    return std::strstr(haystack, needle);
}

// Appends the index of every occurrence of apcSubString in apcString to arOffsets, growing the array through the
// calling thread's heap: the first occurrence always, the later ones only for REPLACE_ALL. The result says whether
// there was one at all; the search goes on from the character after the match's start.
hkBool hkString::findAllOccurrences(
    const char* apcString,
    const char* apcSubString,
    hkArray<int32_t>& arOffsets,
    ReplaceType eReplaceType
) {
    const int32_t iSubLength = strLen(apcSubString);
    const char* pcFound = strStr(apcString, apcSubString);
    hkBool result;
    result.m_bool = 0;
    if (pcFound == nullptr) {
        return result;
    }
    for (;;) {
        result.m_bool = 1;
        hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
        if (arOffsets.m_size == (arOffsets.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*kRouter.m_heap, &arOffsets, sizeof(int32_t));
        }
        arOffsets.m_data[arOffsets.m_size] = static_cast<int32_t>(pcFound - apcString);
        ++arOffsets.m_size;
        if (eReplaceType == REPLACE_ONE) {
            return result;
        }
        pcFound = strStr(pcFound + iSubLength, apcSubString);
        if (pcFound == nullptr) {
            return result;
        }
    }
}

void hkString::strFree_00CB1190(char* s, hkMemoryAllocator& alloc) {
    hkMemoryRouter::easyFree(alloc, s);
}

hkBool hkString::beginsWith(const char* a, const char* b) {
    const char* v3 = b;
    char v4 = *b;
    if (*b) {
        while (v3[a - b] == v4) {
            v4 = *++v3;
            if (!v4) {
                goto LABEL_4;
            }
        }
        return 0;
    }

LABEL_4:;
    return 1;
}

int32_t hkString::lastIndexOf(const char* str, char c) {
    const char* v2 = hkString::strRchr(str, c);
    if (v2) {
        return v2 - str;
    }

    return -1;
}

// Compares byte by byte: the first differing byte gives -1 or 1, and 0 is returned when all n bytes are equal.
int32_t hkString::memCmp(const void* buf1, const void* buf2, int32_t n) {
    const uint8_t* p0 = static_cast<const uint8_t*>(buf1);
    const uint8_t* p1 = static_cast<const uint8_t*>(buf2);
    uint32_t left = static_cast<uint32_t>(n);
    // EXE-INLINED none: memcmp
    while (left >= 4 && p0[0] == p1[0] && p0[1] == p1[1] && p0[2] == p1[2] && p0[3] == p1[3]) {
        left -= 4;
        p0 += 4;
        p1 += 4;
    }
    if (!left) {
        return 0;
    }
    int32_t d = p0[0] - p1[0];
    if (d) {
        return (d >> 31) | 1;
    }
    if (left <= 1) {
        return 0;
    }
    d = p0[1] - p1[1];
    if (d) {
        return (d >> 31) | 1;
    }
    if (left <= 2) {
        return 0;
    }
    d = p0[2] - p1[2];
    if (d) {
        return (d >> 31) | 1;
    }
    if (left <= 3) {
        return 0;
    }
    d = p0[3] - p1[3];
    return (d >> 31) | 1;
}

int32_t hkString::atoi(const char* String, int32_t Radix) {
    return std::strtoul(String, nullptr, Radix);
}
