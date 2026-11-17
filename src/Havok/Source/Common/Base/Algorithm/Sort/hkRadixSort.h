#pragma once

#include <cstdint>

// Havok's LSD radix sorts: one pass per key byte, stable, scattering through a caller-supplied buffer.
class hkRadixSort {
public:
	// A 16-bit key and two bytes of payload (4 bytes).
	struct SortData16 {
		union {
			uint16_t m_key;
			uint8_t m_keys[2];
		};

		uint16_t m_userData;
	};

	// A 32-bit key and four bytes of payload (8 bytes).
	struct SortData32 {
		union {
			uint32_t m_key;
			uint8_t m_keys[4];
		};

		uint32_t m_userData;
	};

	// Sorts numData records by m_key, low byte first and high byte second, leaving the result in data and using
	// buffer as the intermediate store.
	static void sort16(SortData16* data, int32_t numObjects, SortData16* buffer);
	// Sorts numData records by m_key, least significant byte first, leaving the result in data and using buffer as
	// the intermediate store.
	static void sort32(SortData32* data, int32_t numObjects, SortData32* buffer);
};
