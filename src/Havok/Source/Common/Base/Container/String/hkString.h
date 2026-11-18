#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include <stdarg.h>
#include <stdint.h>

// The string helpers the Havok code uses instead of the C run time. hkString has no instance data: every member is
// static.
class hkString {
public:
    // Whether a search or replace stops at the first match or goes through the whole string (4 bytes wide).
    enum ReplaceType : int32_t {
        REPLACE_ONE = 0,
        REPLACE_ALL = 1,
    };

    // Copies at most n characters.
    static void strNcpy(char* dst, const char* src, int32_t n);
    // The first c in string, or null.
    static const char* strChr(const char* haystack, int32_t needle);
    // The last c in string, or null.
    static const char* strRchr(const char* haystack, int32_t needle);
    static char toLower(char c);
    static char* strCpy(char* dst, const char* src);
    static int32_t strCasecmp(const char* a, const char* b);
    static int32_t strLen(const char* src);
    static float atof(const char* in);
    static char* strDup(const char* apcString, hkMemoryAllocator& apAllocator);
    // A copy of apcString from the calling thread's heap.
    static char* strDup(const char* apcString);
    // A copy of at most aiMaxLen characters of apcString from apAllocator.
    static char* strNdup(const char* apcString, int32_t aiMaxLen, hkMemoryAllocator& apAllocator);
    // A copy of at most aiMaxLen characters of apcString from the calling thread's heap.
    static char* strNdup(const char* apcString, int32_t aiMaxLen);
    // Frees a string from the calling thread's heap, as strDup made it.
    static void strFree(char* s);
    static int32_t strCmp(const char* a, const char* b);
    // Copies n bytes with memcpy and returns the destination.
    static void* memCpy(void* dst, const void* src, int32_t n);

    // Copies numWords 32-bit words.
    static void memCpy4(void* dst, const void* src, int32_t numWords) {
        const uint32_t* src32 = static_cast<const uint32_t*>(src);
        uint32_t* dst32 = static_cast<uint32_t*>(dst);
        for (int32_t i = 0; i < numWords; i++) {
            *(dst32++) = *(src32++);
        }
    }

    // Fills numWords 32-bit words at dst with value.
    static void memSet4(void* dst, const int32_t value, int32_t numWords) {
        uint32_t* dst32 = static_cast<uint32_t*>(dst);
        for (int32_t i = numWords - 1; i >= 0; i--) {
            *dst32 = static_cast<uint32_t>(value);
            dst32++;
        }
    }

    // The SDK's inline copy of numQuads 16-byte blocks as dwords (hkString.inl); none when numQuads is not positive.
    static void memCpy16(void* dst, const void* src, int32_t numQuads) {
        const uint32_t* src32 = static_cast<const uint32_t*>(src);
        uint32_t* dst32 = static_cast<uint32_t*>(dst);
        for (int32_t i = 0; i < numQuads; i++) {
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            dst32 += 4;
            src32 += 4;
        }
    }

    // The SDK's inline copy of numQuads 16-byte blocks as dwords (hkString.inl); at least one block is copied.
    static void memCpy16NonEmpty(void* dst, const void* src, int32_t numQuads) {
        const uint32_t* src32 = static_cast<const uint32_t*>(src);
        uint32_t* dst32 = static_cast<uint32_t*>(dst);
        do {
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            dst32 += 4;
            src32 += 4;
        }
        while (--numQuads > 0);
    }

    // Copies n bytes; the ranges are allowed to overlap.
    static void memMove(void* dst, const void* src, int32_t n);
    static void memSet(void* dst, int32_t c, int32_t n);
    static hkBool endsWith(const char* apcString, const char* apcEnding);
    // Havok SDK signature (hkBase.lib, tied by reloc).
    static int32_t strNcmp(const char* a, const char* b, int32_t n);
    // Havok SDK signature (hkBase.lib, tied by reloc); hkargs is the SDK's hk_va_list, a va_list.
    static int32_t vsnprintf(char* buf, int32_t len, const char* fmt, va_list hkargs);
    // The upper-case of a lower-case letter, else the character.
    static char toUpper(char c);
    // Formats into apcBuffer, writing at most aiMaxLen characters; returns the length the text needs.
    static int32_t snprintf(char* buf, int32_t len, const char* fmt, ...);
    // Formats into apcBuffer without a bound.
    static int32_t sprintf(char* buf, const char* fmt, ...);
    // Case-insensitive compare of at most aiMaxLen characters; the lowered characters compare signed.
    static int32_t strNcasecmp(const char* a, const char* b, int32_t n);
    // Lower-cases apcString in place and returns it.
    static char* strLwr(char* s);
    // Upper-cases apcString in place and returns it.
    static char* strUpr(char* s);
    // The first occurrence of apcSubString in apcString, or null.
    static const char* strStr(const char* haystack, const char* needle);
    // Appends the index of every occurrence of apcSubString in apcString to arOffsets: the first one always, the later
    // ones only for REPLACE_ALL. arResult says whether there was one at all.
    static hkBool findAllOccurrences(
        const char* apcString,
        const char* apcSubString,
        hkArray<int32_t>& arOffsets,
        ReplaceType eReplaceType
    );
    static void strFree_00CB1190(char* s, hkMemoryAllocator& alloc);
    static hkBool beginsWith(const char* a, const char* b);
    static int32_t lastIndexOf(const char* str, char c);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    static int32_t memCmp(const void* buf1, const void* buf2, int32_t n);
    // Copies size bytes, 16 at a time.
    template <int32_t size>
    static void memCpy16(void* dst, const void* src);
    // Only reads String (std::strtoul).
    static int32_t atoi(const char* String, int32_t Radix);
};
template <int32_t size>
void hkString::memCpy16(void* dst, const void* src) {
    hkString::memCpy16NonEmpty(dst, src, size / 16);
}
// hkStringPtr's comparisons (the SDK's inline bodies, hkStringPtr.inl); here, since they need hkString.
inline int32_t hkStringPtr::compareTo(const char* s) const {
    const char* t = cString();
    if (t && s) {
        return hkString::strCmp(t, s);
    }
    return t ? 1 : (s ? -1 : 0);
}

inline int32_t hkStringPtr::operator==(const char* cptr) const {
    return compareTo(cptr) == 0;
}

inline int32_t hkStringPtr::operator!=(const char* s) const {
    return compareTo(s) != 0;
}

inline int32_t hkStringPtr::startsWith(const char* s) const {
    return hkString::beginsWith(cString(), s);
}

inline int32_t hkStringPtr::endsWith(const char* s) const {
    return hkString::endsWith(cString(), s);
}
