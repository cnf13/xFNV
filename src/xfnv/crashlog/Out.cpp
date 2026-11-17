#include "xfnv/crashlog/CrashLog.hpp"

namespace fnvd::crashlog {

Out& Out::chr(char acChar)
{
	if (uiUsed == sizeof(acBuffer)) {
		flush();
	}
	acBuffer[uiUsed++] = acChar;
	return *this;
}

Out& Out::str(const char* apText)
{
	while (*apText) {
		chr(*apText++);
	}
	return *this;
}

Out& Out::guardedStr(uintptr_t apText, uint32_t auiMax)
{
	// Read in pieces that never cross a page, so a string ending before an unreadable page still prints.
	while (auiMax) {
		char acPiece[64];
		uint32_t uiPiece = 64 - static_cast<uint32_t>(apText % 64);
		uiPiece = uiPiece < auiMax ? uiPiece : auiMax;
		if (!readMemory(acPiece, apText, uiPiece)) {
			return str("<unreadable>");
		}
		for (uint32_t i = 0; i < uiPiece; ++i) {
			if (!acPiece[i]) {
				return *this;
			}
			chr(acPiece[i] >= ' ' && acPiece[i] < 0x7F ? acPiece[i] : '?');
		}
		apText += uiPiece;
		auiMax -= uiPiece;
	}
	return str("...");
}

Out& Out::hex(uint64_t auiValue, int32_t aiDigits)
{
	char acDigits[16];
	int32_t iCount = 0;
	do {
		acDigits[iCount++] = "0123456789ABCDEF"[auiValue & 0xF];
		auiValue >>= 4;
	} while (auiValue || iCount < aiDigits);
	while (iCount) {
		chr(acDigits[--iCount]);
	}
	return *this;
}

Out& Out::dec(int64_t aiValue)
{
	char acDigits[20];
	int32_t iCount = 0;
	uint64_t uiValue = aiValue < 0 ? 0 - static_cast<uint64_t>(aiValue) : static_cast<uint64_t>(aiValue);
	if (aiValue < 0) {
		chr('-');
	}
	do {
		acDigits[iCount++] = static_cast<char>('0' + uiValue % 10);
		uiValue /= 10;
	} while (uiValue);
	while (iCount) {
		chr(acDigits[--iCount]);
	}
	return *this;
}

void Out::flush()
{
	if (uiUsed) {
		writeOut(acBuffer, uiUsed);
		uiUsed = 0;
	}
}

} // namespace fnvd::crashlog
