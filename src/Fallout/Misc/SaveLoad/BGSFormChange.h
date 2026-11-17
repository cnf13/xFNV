#pragma once

#include "Reconstruction.hpp"

// The changes recorded for one form in a save game: its change flags. IDA-only; the PDB has no such class.
struct BGSFormChange {
	uint32_t changeFlags;
	// IDA: unsigned int unk004.
	uint8_t padding04[4];
};
