#include "SDK/MSVC/string.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"

// The home of the exe's out-of-line VC9 <string.h> bodies: the bodies src reconstructs are defined and registered
// here.

namespace {

// VC9's _VALIDATE_RETURN_ERRCODE failure: errno, the invalid-parameter report, the error returned.
int Fail(int aiError)
{
	errno = aiError;
	rt::_invalid_parameter_noinfo();
	return aiError;
}

// A-Z to a-z, as the C locale's __ascii_tolower; any other value unchanged.
uint32_t AsciiLower(uint32_t auiChar)
{
	return auiChar - 'A' <= 'Z' - 'A' ? auiChar + ('a' - 'A') : auiChar;
}

} // namespace

namespace rt {

// wcslen, 0xECDB00: the exe scans the 16-bit char16_t elements until the terminator (two bytes per step) and returns
// the count before it.
size_t wcslen(const char16_t* String)
{
	const char16_t* pEnd = String;
	while (*pEnd) {
		++pEnd;
	}
	return static_cast<size_t>(pEnd - String);
}

// The C locale's path: __ascii_towlower on each 16-bit character.
int32_t _wcsicmp(const char16_t* apStr1, const char16_t* apStr2)
{
	if (!apStr1 || !apStr2) {
		Fail(kEINVAL);
		return 0x7FFFFFFF;
	}
	uint32_t uiChar1;
	uint32_t uiChar2;
	do {
		uiChar1 = AsciiLower(*apStr1++);
		uiChar2 = AsciiLower(*apStr2++);
	} while (uiChar1 != 0 && uiChar1 == uiChar2);
	return static_cast<int32_t>(uiChar1 - uiChar2);
}
} // namespace rt

// The Microsoft-only functions, where the C library has none (string.h says why not on Windows).
#if !defined(_WIN32)

namespace {

// VC9's _RESET_STRING then _RETURN_BUFFER_TOO_SMALL / _RETURN_DEST_NOT_NULL_TERMINATED.
int ResetAndFail(char* apDest, int aiError)
{
	*apDest = '\0';
	return Fail(aiError);
}

// _strlwr_s_l and _strupr_s_l in the C locale: a string not terminated within astSize is emptied (EINVAL); else
// the bytes from acFrom to acTo are shifted by aiDelta.
int ChangeCase(char* apString, size_t astSize, char acFrom, char acTo, int aiDelta)
{
	if (!apString) {
		return Fail(rt::kEINVAL);
	}
	size_t stLength = 0;
	while (stLength < astSize && apString[stLength] != '\0') {
		++stLength;
	}
	if (stLength >= astSize) {
		return ResetAndFail(apString, rt::kEINVAL);
	}
	for (char* pChar = apString; *pChar; ++pChar) {
		if (*pChar >= acFrom && *pChar <= acTo) {
			*pChar = static_cast<char>(*pChar + aiDelta);
		}
	}
	return 0;
}

} // namespace

// memcpy_s: a zero count copies nothing, whatever the pointers. A null source or a too small destination zeroes the
// destination before the error.
int memcpy_s(void* apDest, size_t astDestSize, const void* apSrc, size_t astCount)
{
	if (astCount == 0) {
		return 0;
	}
	if (!apDest) {
		return Fail(rt::kEINVAL);
	}
	if (!apSrc || astDestSize < astCount) {
		std::memset(apDest, 0, astDestSize);
		return Fail(apSrc ? rt::kERANGE : rt::kEINVAL);
	}
	std::memcpy(apDest, apSrc, astCount);
	return 0;
}

int memmove_s(void* apDest, size_t astDestSize, const void* apSrc, size_t astCount)
{
	if (astCount == 0) {
		return 0;
	}
	if (!apDest || !apSrc) {
		return Fail(rt::kEINVAL);
	}
	if (astDestSize < astCount) {
		return Fail(rt::kERANGE);
	}
	std::memmove(apDest, apSrc, astCount);
	return 0;
}

// The copy runs until the terminator or the end of the destination; a text that reached the end has written
// astDestSize bytes of it before the first byte is reset.
int strcpy_s(char* apcDest, size_t astDestSize, const char* apcSource)
{
	if (!apcDest || astDestSize == 0) {
		return Fail(rt::kEINVAL);
	}
	if (!apcSource) {
		return ResetAndFail(apcDest, rt::kEINVAL);
	}
	char* pDest = apcDest;
	size_t stAvailable = astDestSize;
	while ((*pDest++ = *apcSource++) != '\0' && --stAvailable > 0) {
	}
	if (stAvailable == 0) {
		return ResetAndFail(apcDest, rt::kERANGE);
	}
	return 0;
}

int strcat_s(char* apcDest, size_t astDestSize, const char* apcSource)
{
	if (!apcDest || astDestSize == 0) {
		return Fail(rt::kEINVAL);
	}
	if (!apcSource) {
		return ResetAndFail(apcDest, rt::kEINVAL);
	}
	char* pDest = apcDest;
	size_t stAvailable = astDestSize;
	while (stAvailable > 0 && *pDest != '\0') {
		++pDest;
		--stAvailable;
	}
	if (stAvailable == 0) {
		return ResetAndFail(apcDest, rt::kEINVAL);
	}
	while ((*pDest++ = *apcSource++) != '\0' && --stAvailable > 0) {
	}
	if (stAvailable == 0) {
		return ResetAndFail(apcDest, rt::kERANGE);
	}
	return 0;
}

// A zero count with no destination is no error; a zero count empties the destination. The copy stops at the
// terminator, at astCount characters (then terminated) or at the end of the destination.
int strncpy_s(char* apDest, size_t astDestSize, const char* apSrc, size_t astCount)
{
	if (astCount == 0 && !apDest && astDestSize == 0) {
		return 0;
	}
	if (!apDest || astDestSize == 0) {
		return Fail(rt::kEINVAL);
	}
	if (astCount == 0) {
		*apDest = '\0';
		return 0;
	}
	if (!apSrc) {
		return ResetAndFail(apDest, rt::kEINVAL);
	}
	char* pDest = apDest;
	size_t stAvailable = astDestSize;
	if (astCount == rt::kTRUNCATE) {
		while ((*pDest++ = *apSrc++) != '\0' && --stAvailable > 0) {
		}
	} else {
		while ((*pDest++ = *apSrc++) != '\0' && --stAvailable > 0 && --astCount > 0) {
		}
		if (astCount == 0) {
			*pDest = '\0';
		}
	}
	if (stAvailable == 0) {
		if (astCount == rt::kTRUNCATE) {
			apDest[astDestSize - 1] = '\0';
			return rt::kSTRUNCATE;
		}
		return ResetAndFail(apDest, rt::kERANGE);
	}
	return 0;
}

// The C locale's path (__ascii_stricmp, 0xEC67D6).
int _stricmp(const char* apStr1, const char* apStr2)
{
	if (!apStr1 || !apStr2) {
		Fail(rt::kEINVAL);
		return 0x7FFFFFFF;
	}
	uint32_t uiChar1;
	uint32_t uiChar2;
	do {
		uiChar1 = AsciiLower(static_cast<unsigned char>(*apStr1++));
		uiChar2 = AsciiLower(static_cast<unsigned char>(*apStr2++));
	} while (uiChar1 != 0 && uiChar1 == uiChar2);
	return static_cast<int>(uiChar1 - uiChar2);
}

// The C locale's path, the CRT's assembly __ascii_strnicmp: at a terminator on either side the raw bytes
// decide; otherwise the folded ones. The sign of the first difference as -1 or 1 (unsigned compare).
int _strnicmp(const char* apStr1, const char* apStr2, size_t astCount)
{
	if (!apStr1 || !apStr2 || astCount > 0x7FFFFFFF) {
		Fail(rt::kEINVAL);
		return 0x7FFFFFFF;
	}
	if (astCount == 0) {
		return 0;
	}
	uint32_t uiChar1;
	uint32_t uiChar2;
	do {
		uiChar1 = static_cast<unsigned char>(*apStr1);
		uiChar2 = static_cast<unsigned char>(*apStr2);
		if (uiChar1 == 0 || uiChar2 == 0) {
			break;
		}
		++apStr1;
		++apStr2;
		uiChar1 = AsciiLower(uiChar1);
		uiChar2 = AsciiLower(uiChar2);
		if (uiChar1 != uiChar2) {
			break;
		}
	} while (--astCount != 0);
	if (uiChar1 == uiChar2) {
		return 0;
	}
	return uiChar1 < uiChar2 ? -1 : 1;
}

// The delimiters as a 256-bit set, the terminator always in it: leading delimiters are skipped, the token ends at
// the next delimiter (overwritten with a terminator), and *appContext points past it.
char* strtok_s(char* apString, const char* apDelimit, char** appContext)
{
	if (!appContext || !apDelimit || (!apString && !*appContext)) {
		Fail(rt::kEINVAL);
		return nullptr;
	}
	uint8_t aucMap[32] = {};
	const char* pDelimit = apDelimit;
	do {
		const uint8_t ucDelimit = static_cast<uint8_t>(*pDelimit);
		aucMap[ucDelimit >> 3] |= static_cast<uint8_t>(1 << (ucDelimit & 7));
	} while (*pDelimit++);
	char* pString = apString ? apString : *appContext;
	while ((aucMap[static_cast<uint8_t>(*pString) >> 3] & (1 << (*pString & 7))) && *pString) {
		++pString;
	}
	char* pToken = pString;
	for (; *pString; ++pString) {
		if (aucMap[static_cast<uint8_t>(*pString) >> 3] & (1 << (*pString & 7))) {
			*pString++ = '\0';
			break;
		}
	}
	*appContext = pString;
	return pToken == pString ? nullptr : pToken;
}

int _strlwr_s(char* apString, size_t astSize)
{
	return ChangeCase(apString, astSize, 'A', 'Z', 'a' - 'A');
}

int _strupr_s(char* apString, size_t astSize)
{
	return ChangeCase(apString, astSize, 'a', 'z', 'A' - 'a');
}

#endif
