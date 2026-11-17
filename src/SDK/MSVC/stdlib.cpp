#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"
#include "SDK/MSVC/string.h"

#include <cstdio>
#include <cstring>
#include <string>

// The home of the exe's out-of-line VC9 <stdlib.h> bodies: the bodies src reconstructs are defined and registered
// here.

namespace {

// The VC9 error numbers the bodies return (errno.h).
using rt::kEILSEQ;
using rt::kEINVAL;
using rt::kERANGE;
using rt::kSTRUNCATE;
using rt::kTRUNCATE;

// VC9's _VALIDATE_RETURN_ERRCODE failure: errno, the invalid-parameter report, the error returned.
int Fail(int aiError)
{
	errno = aiError;
	rt::_invalid_parameter_noinfo();
	return aiError;
}

} // namespace

// The Microsoft-only functions, where the C library has none (stdlib.h says why not on Windows).
#if !defined(_WIN32)

namespace {

// VC9's _strflt (the CRT's STRFLT): a double as sign, decimal point position and significant digits, as _fltout2
// makes it. acMantissa holds the digits without the point (d1 d2 ... = 0.d1d2... * 10^iDecPt), or VC9's texts for
// the special values ("1#INF", "1#IND", "1#QNAN", "1#SNAN", with iDecPt 1).
struct StrFlt {
	bool bNegative;
	int32_t iDecPt;
	char acMantissa[24];
};

// _fltout2: 17 significant digits (VC9's $I10_OUTPUT), trailing zeros dropped; zero is "0" with iDecPt 1. The digits
// come from the C library's %.16e, the exact 17-digit rounding.
StrFlt FloatOut(double adValue)
{
	StrFlt kFlt = {};
	uint64_t ullBits;
	std::memcpy(&ullBits, &adValue, sizeof(ullBits));
	kFlt.bNegative = (ullBits >> 63) != 0;
	const uint64_t ullExponent = (ullBits >> 52) & 0x7FF;
	const uint64_t ullFraction = ullBits & 0xFFFFFFFFFFFFFull;
	kFlt.iDecPt = 1;
	if (ullExponent == 0x7FF) {
		const char* pText = "1#SNAN";
		if (ullFraction == 0) {
			pText = "1#INF";
		} else if (kFlt.bNegative && ullFraction == 0x8000000000000ull) {
			pText = "1#IND";
		} else if (ullFraction & 0x8000000000000ull) {
			pText = "1#QNAN";
		}
		std::strcpy(kFlt.acMantissa, pText);
		return kFlt;
	}
	if (ullExponent == 0 && ullFraction == 0) {
		std::strcpy(kFlt.acMantissa, "0");
		return kFlt;
	}
	// "d.dddddddddddddddde+X..." of the magnitude.
	char acText[40];
	std::snprintf(acText, sizeof(acText), "%.16e", kFlt.bNegative ? -adValue : adValue);
	int32_t iCount = 0;
	const char* pText = acText;
	for (; *pText && *pText != 'e'; ++pText) {
		if (*pText >= '0' && *pText <= '9') {
			kFlt.acMantissa[iCount++] = *pText;
		}
	}
	while (iCount > 1 && kFlt.acMantissa[iCount - 1] == '0') {
		--iCount;
	}
	kFlt.acMantissa[iCount] = '\0';
	kFlt.iDecPt = static_cast<int32_t>(std::strtol(pText + 1, nullptr, 10)) + 1;
	return kFlt;
}

// _fptostr: aiDigits digits of the mantissa (padded with '0') into apBuffer, rounded half up on the next digit; a
// carry out of the first digit moves the decimal point. apBuffer holds aiDigits + 2 characters.
void FloatToString(char* apBuffer, int32_t aiDigits, StrFlt& arFlt)
{
	char* pBuffer = apBuffer;
	const char* pMantissa = arFlt.acMantissa;
	// The first digit takes a carry out of the rounding (9.99 to 10.0).
	*pBuffer++ = '0';
	while (aiDigits > 0) {
		*pBuffer++ = *pMantissa ? *pMantissa++ : '0';
		--aiDigits;
	}
	*pBuffer = '\0';
	if (aiDigits >= 0 && *pMantissa >= '5') {
		--pBuffer;
		while (*pBuffer == '9') {
			*pBuffer-- = '0';
		}
		*pBuffer += 1;
	}
	if (*apBuffer == '1') {
		++arFlt.iDecPt;
	} else {
		std::memmove(apBuffer, apBuffer + 1, std::strlen(apBuffer + 1) + 1);
	}
}

// _cftoe with lower-case 'e': [-]d[.ddd]e(+|-)ddd, aiDecimals digits after the point.
void FloatToE(char* apBuffer, int32_t aiDecimals, StrFlt kFlt)
{
	char* pOut = apBuffer;
	if (kFlt.bNegative) {
		*pOut++ = '-';
	}
	std::string kDigits(static_cast<size_t>(aiDecimals > 0 ? aiDecimals : 0) + 3, '\0');
	FloatToString(kDigits.data(), aiDecimals + 1, kFlt);
	*pOut++ = kDigits[0];
	if (aiDecimals > 0) {
		*pOut++ = '.';
		std::memcpy(pOut, kDigits.data() + 1, static_cast<size_t>(aiDecimals));
		pOut += aiDecimals;
	}
	std::strcpy(pOut, "e+000");
	if (kFlt.acMantissa[0] != '0') {
		int32_t iExponent = kFlt.iDecPt - 1;
		if (iExponent < 0) {
			iExponent = -iExponent;
			pOut[1] = '-';
		}
		pOut[2] = static_cast<char>('0' + iExponent / 100 % 10);
		pOut[3] = static_cast<char>('0' + iExponent / 10 % 10);
		pOut[4] = static_cast<char>('0' + iExponent % 10);
	}
}

// _cftof: [-]ddd[.ddd], aiDecimals digits after the point; leading zeros after the point for a value below 1.
void FloatToF(char* apBuffer, int32_t aiDecimals, StrFlt kFlt)
{
	char* pOut = apBuffer;
	if (kFlt.bNegative) {
		*pOut++ = '-';
	}
	const int32_t iCount = aiDecimals + kFlt.iDecPt;
	std::string kDigits(static_cast<size_t>(iCount > 0 ? iCount : 0) + 2, '\0');
	FloatToString(kDigits.data(), iCount, kFlt);
	const char* pDigits = kDigits.data();
	const int32_t iDecPt = kFlt.iDecPt;
	if (iDecPt <= 0) {
		*pOut++ = '0';
	} else {
		for (int32_t i = 0; i < iDecPt && *pDigits; ++i) {
			*pOut++ = *pDigits++;
		}
	}
	if (aiDecimals > 0) {
		*pOut++ = '.';
		if (iDecPt < 0) {
			const int32_t iZeros = aiDecimals < -iDecPt ? aiDecimals : -iDecPt;
			for (int32_t i = 0; i < iZeros; ++i) {
				*pOut++ = '0';
			}
		}
	}
	std::strcpy(pOut, pDigits);
}

// _gcvt_s with an unbounded buffer: the Fortran G choice between FloatToF and FloatToE on the unrounded decimal
// point, then the zeros before the exponent (or the end) dropped, the point kept.
int GcvtS(char* apBuffer, double adValue, int32_t aiDigits)
{
	if (!apBuffer) {
		return Fail(kEINVAL);
	}
	*apBuffer = '\0';
	const StrFlt kFlt = FloatOut(adValue);
	const int32_t iMagnitude = kFlt.iDecPt - 1;
	if (iMagnitude < -1 || iMagnitude > aiDigits - 1) {
		FloatToE(apBuffer, aiDigits - 1, kFlt);
	} else {
		FloatToF(apBuffer, aiDigits - kFlt.iDecPt, kFlt);
	}
	char* pChar = apBuffer;
	while (*pChar && *pChar != '.') {
		++pChar;
	}
	if (*pChar++) {
		while (*pChar && *pChar != 'e') {
			++pChar;
		}
		char* pStop = pChar--;
		while (*pChar == '0') {
			--pChar;
		}
		while ((*++pChar = *pStop++) != '\0') {
		}
	}
	return 0;
}

} // namespace

char* _gcvt(double adValue, int aiDigits, char* apBuffer)
{
	return GcvtS(apBuffer, adValue, aiDigits) == 0 ? apBuffer : nullptr;
}

#endif

namespace rt {

size_t wcstombs(char* apDest, const char16_t* apSource, size_t astMaxCount)
{
	if (apDest && astMaxCount == 0) {
		return 0;
	}
	if (!apSource) {
		Fail(kEINVAL);
		return static_cast<size_t>(-1);
	}
	size_t stCount = 0;
	if (apDest) {
		while (stCount < astMaxCount) {
			if (*apSource > 255) {
				errno = kEILSEQ;
				return static_cast<size_t>(-1);
			}
			apDest[stCount] = static_cast<char>(*apSource);
			if (*apSource++ == u'\0') {
				return stCount;
			}
			++stCount;
		}
		return stCount;
	}
	for (; *apSource; ++apSource) {
		if (*apSource > 255) {
			errno = kEILSEQ;
			return static_cast<size_t>(-1);
		}
		++stCount;
	}
	return stCount;
}

int32_t wcstombs_s(size_t* apReturnValue, char* apDest, size_t astSizeInBytes, const char16_t* apSource,
	size_t astMaxCount)
{
	if (apDest ? astSizeInBytes == 0 : astSizeInBytes != 0) {
		return Fail(kEINVAL);
	}
	if (apDest) {
		*apDest = '\0';
	}
	if (apReturnValue) {
		*apReturnValue = 0;
	}
	const size_t stBufferSize = astMaxCount > astSizeInBytes ? astSizeInBytes : astMaxCount;
	if (stBufferSize > 0x7FFFFFFF) {
		return Fail(kEINVAL);
	}
	size_t stSize = wcstombs(apDest, apSource, stBufferSize);
	if (stSize == static_cast<size_t>(-1)) {
		if (apDest) {
			*apDest = '\0';
		}
		return errno;
	}
	// The terminator.
	++stSize;
	int32_t iResult = 0;
	if (apDest) {
		if (stSize > astSizeInBytes) {
			if (astMaxCount != kTRUNCATE) {
				*apDest = '\0';
				return Fail(kERANGE);
			}
			stSize = astSizeInBytes;
			iResult = kSTRUNCATE;
		}
		apDest[stSize - 1] = '\0';
	}
	if (apReturnValue) {
		*apReturnValue = stSize;
	}
	return iResult;
}
} // namespace rt

#if !defined(_WIN32)

// The drive is the path's first character when its second is ':'. The directory runs through the last '/' or '\',
// the extension from the last '.' after it; each part is copied with strncpy_s, which cannot fail once the size is
// checked. On an error every buffer given is emptied.
int _splitpath_s(const char* apPath, char* apDrive, size_t astDriveSize, char* apDir, size_t astDirSize,
	char* apFName, size_t astFNameSize, char* apExt, size_t astExtSize)
{
	bool bInvalid = !apPath || (!apDrive != (astDriveSize == 0)) || (!apDir != (astDirSize == 0))
		|| (!apFName != (astFNameSize == 0)) || (!apExt != (astExtSize == 0));
	if (!bInvalid) {
		const char* pPath = apPath;
		const char* pDriveEnd = *pPath ? pPath + 1 : pPath;
		bool bFits = true;
		if (*pDriveEnd == ':') {
			if (apDrive) {
				if (astDriveSize < 3) {
					bFits = false;
				} else {
					strncpy_s(apDrive, astDriveSize, pPath, 2);
				}
			}
			pPath = pDriveEnd + 1;
		} else if (apDrive) {
			*apDrive = '\0';
		}
		const char* pLastSlash = nullptr;
		const char* pDot = nullptr;
		const char* pEnd = pPath;
		if (bFits) {
			for (; *pEnd; ++pEnd) {
				if (*pEnd == '/' || *pEnd == '\\') {
					pLastSlash = pEnd + 1;
				} else if (*pEnd == '.') {
					pDot = pEnd;
				}
			}
			if (pLastSlash) {
				if (apDir) {
					const size_t stLength = static_cast<size_t>(pLastSlash - pPath);
					if (astDirSize <= stLength) {
						bFits = false;
					} else {
						strncpy_s(apDir, astDirSize, pPath, stLength);
					}
				}
				pPath = pLastSlash;
			} else if (apDir) {
				*apDir = '\0';
			}
		}
		if (bFits) {
			if (pDot && pDot >= pPath) {
				if (apFName) {
					const size_t stLength = static_cast<size_t>(pDot - pPath);
					if (astFNameSize <= stLength) {
						bFits = false;
					} else {
						strncpy_s(apFName, astFNameSize, pPath, stLength);
					}
				}
				if (bFits && apExt) {
					const size_t stLength = static_cast<size_t>(pEnd - pDot);
					if (astExtSize <= stLength) {
						bFits = false;
					} else {
						strncpy_s(apExt, astExtSize, pDot, stLength);
					}
				}
			} else {
				if (apFName) {
					const size_t stLength = static_cast<size_t>(pEnd - pPath);
					if (astFNameSize <= stLength) {
						bFits = false;
					} else {
						strncpy_s(apFName, astFNameSize, pPath, stLength);
					}
				}
				if (bFits && apExt) {
					*apExt = '\0';
				}
			}
		}
		if (bFits) {
			return 0;
		}
	}
	if (apDrive && astDriveSize > 0) {
		*apDrive = '\0';
	}
	if (apDir && astDirSize > 0) {
		*apDir = '\0';
	}
	if (apFName && astFNameSize > 0) {
		*apFName = '\0';
	}
	if (apExt && astExtSize > 0) {
		*apExt = '\0';
	}
	return Fail(bInvalid ? kEINVAL : kERANGE);
}

// Each part is written as it is counted; the count reaching the buffer's size empties the buffer (ERANGE).
int _makepath_s(char* apBuffer, size_t astBufferSize, const char* apDrive, const char* apDir,
	const char* apFName, const char* apExt)
{
	if (!apBuffer || astBufferSize == 0) {
		return Fail(kEINVAL);
	}
	size_t stWritten = 0;
	char* pOut = apBuffer;
	bool bFits = true;
	if (apDrive && *apDrive) {
		stWritten += 2;
		if (stWritten >= astBufferSize) {
			bFits = false;
		} else {
			*pOut++ = *apDrive;
			*pOut++ = ':';
		}
	}
	if (bFits && apDir && *apDir) {
		const char* pDir = apDir;
		do {
			if (++stWritten >= astBufferSize) {
				bFits = false;
				break;
			}
			*pOut++ = *pDir++;
		} while (*pDir);
		// _mbsdec of the end: in a single-byte code page the last character.
		if (bFits && pDir[-1] != '/' && pDir[-1] != '\\') {
			if (++stWritten >= astBufferSize) {
				bFits = false;
			} else {
				*pOut++ = '\\';
			}
		}
	}
	if (bFits && apFName) {
		for (const char* pName = apFName; *pName;) {
			if (++stWritten >= astBufferSize) {
				bFits = false;
				break;
			}
			*pOut++ = *pName++;
		}
	}
	if (bFits && apExt) {
		const char* pExt = apExt;
		if (*pExt && *pExt != '.') {
			if (++stWritten >= astBufferSize) {
				bFits = false;
			} else {
				*pOut++ = '.';
			}
		}
		while (bFits && *pExt) {
			if (++stWritten >= astBufferSize) {
				bFits = false;
				break;
			}
			*pOut++ = *pExt++;
		}
	}
	if (bFits && ++stWritten <= astBufferSize) {
		*pOut = '\0';
		return 0;
	}
	*apBuffer = '\0';
	return Fail(kERANGE);
}

#endif
