#pragma once

#include <cstdint>

// The engine's byte-order helpers: each swaps the order of the values in place, for streams read on the other
// endianness.
class NiEndian {
public:
    // Swaps the byte order of the uiCount 16-bit values at pcValue.
    static void Swap16(char* pcValue, uint32_t uiCount) {
        for (uint32_t i = uiCount; i; --i) {
            char c = pcValue[0];
            pcValue[0] = pcValue[1];
            pcValue[1] = c;
            pcValue += 2;
        }
    }
    // Swaps the byte order of the uiCount 32-bit values at pcValue.
    static void Swap32(char* pcValue, uint32_t uiCount);
    static void Swap64(void* pvValue, int32_t iCount);
};
