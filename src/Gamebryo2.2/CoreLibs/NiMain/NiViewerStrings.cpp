#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

char* NiGetViewerString(const char* apcRttiName) {
	size_t stLength = strlen(apcRttiName) + 9;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "--- %s ---", apcRttiName);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, bool abValue) {
	size_t stLength = strlen(apcPrefix) + 9;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	if (abValue)
		NiSprintf(pcString, stLength, "%s = true", apcPrefix);
	else
		NiSprintf(pcString, stLength, "%s = false", apcPrefix);
	return pcString;
}

// The character goes to NiSprintf sign-extended (movsx), as the C promotion of a char.
char* NiGetViewerString(const char* apcPrefix, char acValue) {
	size_t stLength = strlen(apcPrefix) + 5;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %c", apcPrefix, acValue);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, int32_t aiValue) {
	size_t stLength = strlen(apcPrefix) + 15;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %d", apcPrefix, aiValue);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, uint8_t aucValue) {
	size_t stLength = strlen(apcPrefix) + 7;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %u", apcPrefix, aucValue);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, int16_t asValue) {
	size_t stLength = strlen(apcPrefix) + 10;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %hd", apcPrefix, asValue);
	return pcString;
}

char* NiGetViewerString(const char* pcPrefix, uint16_t usValue) {
	size_t stLength = strlen(pcPrefix) + 9;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %hu", pcPrefix, usValue);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, uint32_t auiValue) {
	size_t stLength = strlen(apcPrefix) + 14;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %u", apcPrefix, auiValue);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, float afValue) {
	size_t stLength = strlen(apcPrefix) + 20;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %g", apcPrefix, static_cast<double>(afValue));
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, const void* apvValue) {
	size_t stLength = strlen(apcPrefix) + 16;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %p", apcPrefix, apvValue);
	return pcString;
}

char* NiGetViewerString(const char* apcPrefix, const char* apcValue) {
	if (!apcValue) {
		size_t stLength = strlen(apcPrefix) + 8;
		char* pcString = static_cast<char*>(_NiMalloc(stLength));
		NiSprintf(pcString, stLength, "%s = NULL", apcPrefix);
		return pcString;
	}

	size_t stLength = strlen(apcPrefix) + strlen(apcValue) + 4;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	NiSprintf(pcString, stLength, "%s = %s", apcPrefix, apcValue);
	return pcString;
}
