#pragma once

#include <cstdint>

// "--- <name> ---", the heading of a class' lines.
char* NiGetViewerString(const char* apcRttiName);
// "<prefix> = true" / "<prefix> = false"
char* NiGetViewerString(const char* apcPrefix, bool abValue);
// "<prefix> = %c".
char* NiGetViewerString(const char* apcPrefix, char acValue);
// "<prefix> = %d"
char* NiGetViewerString(const char* apcPrefix, int32_t aiValue);
// "<prefix> = %u" for a byte.
char* NiGetViewerString(const char* apcPrefix, uint8_t aucValue);
// "<prefix> = %hd".
char* NiGetViewerString(const char* apcPrefix, int16_t asValue);
// "<prefix> = %hu".
char* NiGetViewerString(const char* pcPrefix, uint16_t usValue);
// "<prefix> = %u"
char* NiGetViewerString(const char* apcPrefix, uint32_t auiValue);
// "<prefix> = %g"
char* NiGetViewerString(const char* apcPrefix, float afValue);
// "<prefix> = %p"
char* NiGetViewerString(const char* apcPrefix, const void* apvValue);
// "<prefix> = <value>", or "<prefix> = NULL" for a null value
char* NiGetViewerString(const char* apcPrefix, const char* apcValue);

// Lines of the scene viewer's object panel (GetViewerStrings), each a fresh string from _NiMalloc that the panel frees.
// Pass the exact parameter type: there are separate overloads for the smaller integer types.
