#pragma once

#include <cstdint>

// The engine's byte-order helpers: each swaps the order of the values in place, for streams read on the other
// endianness.
class NiEndian {
public:
	// Swaps the byte order of the uiCount 32-bit values at pcValue.
	static void Swap32(char* pcValue, uint32_t uiCount);
	static void Swap64(void* a1, int32_t x0);
};
