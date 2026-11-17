#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <__stdarg_va_list.h>
#include <cstdarg>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <type_traits>

// The engine's counted string: heap buffer, current length and capacity (both capped at 0xFFFF).
// Only BSStringT<char> exists in the exe.
template <class Char> class BSStringT {
public:
	BSStringT();
	BSStringT(const Char* apString);
	BSStringT(const BSStringT& aSrc);
	~BSStringT();

	// A deep copy of aSrc's text, as the exe's BSStringT::Copy (0x501C10: Set(aSrc), returning this).
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

	// IDA name BSString::Copy. Set(aSrc) returning this (0x501C10).
	BSStringT* Copy(const BSStringT& aSrc) {
		Set(aSrc);
		return this;
	}

	// The length; a string of 0xFFFF characters or more is measured (inlined in the exe).
	uint32_t Length() const;
	// The out-of-line length; a string of 0xFFFF characters
	// or more is measured with BSstrlen.
	uint32_t GetLength() const;
	// The length in bytes, through the out-of-line GetLength.
	uint32_t ByteLength() const;
	// The length plus one, through the out-of-line GetLength.
	uint32_t GetLenPlusOne() const;
	// The text (pString). The exe's out-of-line getter is the shared one-field getter 0x559450.
	const Char* String() const;
	// Compares the text with apString (ignoring case when abIgnoreCase); 0 when they are equal.
	int32_t StrCmp(const Char* apString, bool abIgnoreCase) const;
	// StrCmp(arString's text, abIgnoreCase), the text read through the exe's shared one-field
	// getter (PDB: StrCmp(arString, abIgnoreCase)).
	int32_t StrCmp(const BSStringT& arString, bool abIgnoreCase) const;
	// Whether the text equals arString's, ignoring case. Defined for char in BSStringT.cpp.
	bool operator==(const BSStringT& arString) const;
	// Whether the text differs from arString's, ignoring case. Defined for char in BSStringT.cpp.
	bool operator!=(const BSStringT& arString) const;
	// Whether the text equals apString's, ignoring case: StrCmp(apString, true) == 0.
	bool operator==(const Char* apString) const;
	// Whether the text differs from apString's, ignoring case: !operator==(apString).
	bool operator!=(const Char* apString) const;
	// IDA name; not in the PDB. Whether the text differs from arString's, ignoring case: the exe's own body, which
	// leaves the normalised 0/1 in the whole eax and so returns 32 bits. Defined for char in BSStringT.cpp.
	int32_t IsNotEqual(const BSStringT& arString) const;
	// IDA name BSStringT::GetAt. The character at auiIndex; a null string yields its null pointer. Defined for char in
	// BSStringT.cpp.
	Char& operator[](uint32_t aiIndex);
	// The debug build's name (its DialogueResponse constructor calls it; not in names.py). A copy of the text without
	// its {comments}: text between braces (nested braces counted) and every closing brace are dropped. Returns the
	// string by value (hidden result pointer). Defined for char in BSStringT.cpp.
	BSStringT* RemoveCurlyContent(BSStringT* apResult);
	// Upper-cases the text in place; nothing for a null string.
	void MakeUpper();

	// Lower-cases the letters A-Z of the text in place (the exe's BSstrlwr, _strlwr_s, in the C locale; 0x4AFAD0).
	void MakeLower() {
		if (pString) {
			for (Char* pChar = pString; *pChar; ++pChar) {
				if (*pChar >= 'A' && *pChar <= 'Z') {
					*pChar = static_cast<Char>(*pChar - 'A' + 'a');
				}
			}
		}
	}

	// Only BSStringT<char> formats: the exe has SPrintF and VSPrintF for char only (0x406F60, 0x406F90) and there is
	// no formatter for other characters, so both are constrained to char (an explicit instantiation of
	// BSStringT<char16_t> leaves them out).
	// Formats apFormat's arguments into the string: starts the argument list and hands it to VSPrintF (0x406F60).
	int32_t SPrintF(const Char* apFormat, ...)
		requires std::is_same_v<Char, char>
	{
		va_list apArguments;
		va_start(apArguments, apFormat);
		int32_t iResult = VSPrintF(apFormat, apArguments);
		va_end(apArguments);
		return iResult;
	}

	// Formats apFormat with aList into a 1024-character buffer and copies that in; returns the formatter's count
	// (0x406F90). The exe's BSvsprintf is the unbounded vsprintf; natively the format is bounded by the buffer.
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

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	Char* pString;
	uint16_t sLen;
	uint16_t sMaxLen;
};
#include "BSCore/BSStringT.inl"
