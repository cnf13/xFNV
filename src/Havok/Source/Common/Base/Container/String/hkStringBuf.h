#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include <stdint.h>

// A growable string: m_string holds the characters and the terminating zero, which its size counts.
class hkStringBuf {
public:
    // Whether a replace stops at the first match or goes through the whole string.
    enum ReplaceType : int32_t {
        REPLACE_ONE = 0,
        REPLACE_ALL = 1,
    };

    hkStringBuf();
    hkStringBuf(const char* s0, const char* s1, const char* s2, const char* s3, const char* s4, const char* s5);
    hkStringBuf(const char* s);
    hkStringBuf(const hkStringBuf& s);
    hkStringBuf(const char* apSrc, int32_t aiSize);
    ~hkStringBuf();

    // Whether the string ends with s. Returns the result as a full word, Havok's hkBool32.
    int32_t endsWith(const char* s) const;
    // Whether the text sorts before other (hkString::strCmp below 0).
    hkBool operator<(const char* other) const;
    // Appends s before the terminating zero, growing the storage through the temp allocator.
    hkStringBuf& operator+=(const char* other);
    // Empties the text, leaving the terminating zero.
    void clear();
    // Replaces the text with s.
    hkStringBuf& operator=(const char* s);
    // Copies s's characters into the storage, reallocating it from the temp allocator when it is too small.
    hkStringBuf& operator=(const hkStringBuf& s);
    // Rewrites the characters a to b, only the first when replaceType is REPLACE_ONE.
    void replace(char a, char b, ReplaceType replaceType);
    // Appends s0 to s5, skipping null ones.
    void appendJoin(const char* s0, const char* s1, const char* s2, const char* s3, const char* s4, const char* s5);
    // Inserts len characters of s at pos; a negative len takes the whole of s.
    void insert(int32_t pos, const char* s, int32_t len);
    // Replaces the text with the formatted string, growing the storage until the text fits.
    void printf(const char* fmt, ...);
    // Appends the formatted string to the text, growing the storage until the text fits.
    void appendPrintf(const char* fmt, ...);
    // Appends s to the text; a negative len takes the whole of s.
    void append(const char* s, int32_t len);
    // Sets the storage to the inline characters and replaces the text with arString's; acts as the constructor from an
    // hkStringPtr.
    hkStringBuf* constructFrom(const hkStringPtr& arString);
    // apcRight is only passed on to hkString::strCmp.
    int32_t compareTo(const char* apcRight) const;
    int32_t lastIndexOf(char a2, int32_t a3, int32_t a4);
    void setJoin(const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6);
    void pathBasename();
    void lowerCase();
    void upperCase();
    int32_t compareToIgnoreCase(const char* a1) const;
    int32_t startsWithCase(const char* a1) const;
    // Whether the text starts with s.
    uint32_t startsWith(const char* s) const;
    // Whether the text equals s (Havok's hkBool32).
    int32_t operator==(const char* s) const;
    int32_t indexOf(const char* a1, int32_t a2, int32_t a3);
    int32_t slice(int32_t a1, int32_t a2);
    void* getArray();
    int32_t indexOfCase(const char* needle);
    int32_t lastIndexOf(const char* needle, int32_t startIndex, int32_t endIndex);
    int32_t indexOf(char a1, int32_t a2, int32_t a3);
    int32_t endsWithCase(const char* s);
    int32_t split(int32_t sep, hkArray<const char*, hkContainerTempAllocator>& bits);
    void chompEnd(int32_t n);
    // Drops the first n characters.
    void chompStart(int32_t n);
    void set(const char* s, int32_t len);
    void pathDirname();
    void prepend(const char* s, int32_t len);

    hkInplaceArray<char, 128, hkContainerTempAllocator> m_string;
};

void replace(hkStringBuf* apThis, const char* oldText, const char* newText, int32_t type);
