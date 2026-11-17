#pragma once

#include <cstdint>

// The changes recorded for one form in a save game: its change flags.
struct BGSFormChange {
	uint32_t changeFlags;
	uint8_t padding04[4];
};
