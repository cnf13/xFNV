#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <__stdarg_va_list.h>
#include <cstdarg>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <type_traits>

// The engine's counted string: heap buffer, current length and capacity (both capped at 0xFFFF). Only BSStringT<char>
// is used.
template <class Char>
class BSStringT {
public:
    BSStringT();
    BSStringT(const Char* apString);
    BSStringT(const BSStringT& aSrc);
    ~BSStringT();

    // A deep copy of aSrc's text.
    BSStringT& operator=(const BSStringT& aSrc) {
        Set(aSrc);
        return *this;
    }

    // The assigned text is copied into the string's own buffer.
    BSStringT& operator=(const Char* apString);
    // Appends apString.
    BSStringT& operator+=(const Char* apString);
    // Appends aSrc's text.
    BSStringT& operator+=(const BSStringT& aSrc);

    bool Set(const Char* apString, uint32_t aiMaxLen);
    // Copies aSrc's text.
    bool Set(const BSStringT& aSrc);

    BSStringT* Copy(const BSStringT& aSrc) {
        Set(aSrc);
        return this;
    }

    // The length; a string of 0xFFFF characters or more is measured.
    uint32_t Length() const;
    // A string of 0xFFFF characters or more is measured with BSstrlen.
    uint32_t GetLength() const;
    // The length in bytes, through GetLength.
    uint32_t ByteLength() const;
    uint32_t GetLenPlusOne() const;
    // The text (pString).
    const Char* String() const;
    // Compares the text with apString (ignoring case when abIgnoreCase); 0 when they are equal.
    int32_t StrCmp(const Char* apString, bool abIgnoreCase) const;
    int32_t StrCmp(const BSStringT& arString, bool abIgnoreCase) const;
    // Whether the text equals arString's, ignoring case. Defined for char in BSStringT.cpp.
    bool operator==(const BSStringT& arString) const;
    // Whether the text differs from arString's, ignoring case. Defined for char in BSStringT.cpp.
    bool operator!=(const BSStringT& arString) const;
    // Whether the text equals apString's, ignoring case: StrCmp(apString, true) == 0.
    bool operator==(const Char* apString) const;
    // Whether the text differs from apString's, ignoring case: !operator==(apString).
    bool operator!=(const Char* apString) const;
    // Whether the text differs from arString's, ignoring case; returns a normalised 0 or 1 as a 32-bit value. Defined
    // for char in BSStringT.cpp.
    int32_t IsNotEqual(const BSStringT& arString) const;
    // The character at aiIndex; a null string yields its null pointer. Defined for char in BSStringT.cpp.
    Char& operator[](uint32_t aiIndex);
    // A copy of the text without its {comments}: text between braces (nested braces counted) and every closing brace
    // are dropped. Defined for char in BSStringT.cpp.
    BSStringT* RemoveCurlyContent(BSStringT* apResult);
    // Upper-cases the text in place; nothing for a null string.
    void MakeUpper();

    // Lower-cases the letters A-Z of the text in place, as _strlwr_s does in the C locale.
    void MakeLower() {
        if (pString) {
            for (Char* pChar = pString; *pChar; ++pChar) {
                if (*pChar >= 'A' && *pChar <= 'Z') {
                    *pChar = static_cast<Char>(*pChar - 'A' + 'a');
                }
            }
        }
    }

    // Only BSStringT<char> formats: there is no formatter for other characters, so SPrintF and VSPrintF are constrained
    // to char (an explicit instantiation of BSStringT<char16_t> leaves them out). Formats apFormat's arguments into the
    // string: starts the argument list and hands it to VSPrintF.
    int32_t SPrintF(const Char* apFormat, ...)
    requires std::is_same_v<Char, char>
    {
        va_list apArguments;
        va_start(apArguments, apFormat);
        int32_t iResult = VSPrintF(apFormat, apArguments);
        va_end(apArguments);
        return iResult;
    }

    // Formats apFormat with aList into a 1024-character buffer and copies that in; returns the formatter's count. The
    // format is bounded by the buffer.
    int32_t VSPrintF(const Char* apFormat, va_list aList)
    requires std::is_same_v<Char, char>
    {
        Char acBuffer[1024];
        int32_t iResult = std::vsnprintf(acBuffer, sizeof(acBuffer), apFormat, aList);
        Set(acBuffer, 0);
        return iResult;
    }

    uint32_t MaxLength() const;

    // protected in the engine; public here so the replacement table can name them
    void SetLength(uint32_t aiLen);
    void SetMaxLength(uint32_t aiLen);

    // private in the engine; public here so the replacement table can name it
    void Init();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    Char* pString;
    uint16_t sLen;
    uint16_t sMaxLen;
};
#include "BSCore/BSStringT.inl"
