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

inline void NiEndian::Swap32(char* pcValue, uint32_t uiCount) {
    for (uint32_t i = uiCount; i; --i) {
        char c0 = pcValue[0];
        pcValue[0] = pcValue[3];
        pcValue[3] = c0;
        char c1 = pcValue[1];
        pcValue[1] = pcValue[2];
        pcValue[2] = c1;
        pcValue += 4;
    }
}

inline void NiEndian::Swap64(void* pvValue, int32_t iCount) {
    for (int32_t i = iCount; i; --i) {
        char c0 = *static_cast<uint8_t*>(pvValue);
        *static_cast<uint8_t*>(pvValue) = *(static_cast<uint8_t*>(pvValue) + 7);
        *(static_cast<uint8_t*>(pvValue) + 7) = c0;
        char c1 = *(static_cast<uint8_t*>(pvValue) + 1);
        *(static_cast<uint8_t*>(pvValue) + 1) = *(static_cast<uint8_t*>(pvValue) + 6);
        *(static_cast<uint8_t*>(pvValue) + 6) = c1;
        char c2 = *(static_cast<uint8_t*>(pvValue) + 2);
        *(static_cast<uint8_t*>(pvValue) + 2) = *(static_cast<uint8_t*>(pvValue) + 5);
        *(static_cast<uint8_t*>(pvValue) + 5) = c2;
        char c3 = *(static_cast<uint8_t*>(pvValue) + 3);
        *(static_cast<uint8_t*>(pvValue) + 3) = *(static_cast<uint8_t*>(pvValue) + 4);
        *(static_cast<uint8_t*>(pvValue) + 4) = c3;
        pvValue = static_cast<char*>(pvValue) + 8;
    }
}
