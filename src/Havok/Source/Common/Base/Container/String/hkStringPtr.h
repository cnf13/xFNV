#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

#include <stdint.h>

// A C string pointer that frees the string it points at when the low bit of the pointer is set.
class hkStringPtr {
public:
    enum StringFlags {
        // Set in the pointer's low bit when the pointer owns the string.
        OWNED_FLAG = 0x1,
    };

    hkStringPtr();
    // A loaded string pointer stays as loaded.
    hkStringPtr(hkFinishLoadedObjectFlag flag);
    hkStringPtr(const char* apcString);
    hkStringPtr(const char* apcString, int32_t aiLength);
    hkStringPtr(const hkStringPtr& arOther);
    ~hkStringPtr();

    // Copies s into an owned string, freeing the old one.
    hkStringPtr& operator=(const char* s);

    // Copies s's string (its pointer without OWNED_FLAG) into an owned string, freeing the
    // old one; returns this.
    hkStringPtr& operator=(const hkStringPtr& s);

    // The string without the ownership flag.
    const char* cString() const {
        return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(OWNED_FLAG));
    }

    operator const char*() const {
        return cString();
    }
    // The character at idx; the SDK asserts the index is within the string.
    char operator[](int32_t idx) const {
        return cString()[idx];
    }
    // The length of the string.
    int32_t getLength() const;
    // Replaces the string with the formatted text.
    void printf(const char* fmt, ...);
    // Replaces the string with a copy of the first len characters of s (len -1: all of it).
    void set(const char* s, int32_t len = -1);
    // Compares with s: a null string is the smallest. The SDK's hkBool32 results are plain ints. The SDK's inline
    // bodies are in hkString.h, after hkString (which includes this header through hkArray.h).
    int32_t compareTo(const char* s) const;
    int32_t operator==(const char* cptr) const;
    int32_t operator!=(const char* s) const;
    int32_t startsWith(const char* s) const;
    int32_t endsWith(const char* s) const;

    const char* m_stringAndFlag;
};

// File-static strDup of hkStringPtr.cpp: a heap copy of the string. Only assign calls it.
char* strDup(const char* src);
// File-static strNdup of hkStringPtr.cpp: a heap copy of at most aiMaxLen characters of apcString.
char* strNdup(const char* src, int32_t maxlen);
// Stores in apDest a copy of apcString (aiLength characters of it, or all of it for -1), freeing the string apDest
// owned. Returns the pointer apDest holds afterwards (the old one when apcString is already apDest's string), which
// callers do not read.
const char* assign(int32_t aiLength, const char* apcString, hkStringPtr* apDest);
// Formats apcFormat and its arguments into a 256-byte stack buffer that grows on the heap when the text does not fit,
// then stores the text in apDest; the buffer is freed again when it had grown. Returns the array's flag word.
uint32_t printfStringPtr(hkStringPtr* apDest, const char* apcFormat, ...);

// The empty string the string pointers share instead of owning a zero length buffer.
inline const char* s_emptyString{};
