#pragma once

#include "Reconstruction.hpp"

// One entry of a leveled list as the plugin file stores it. The PDB's source module is TESLeveledList.cpp.
struct LEVELED_OBJECT_FILE {
	// Swaps the fields read from a big-endian file, in field order.
	void Endian();

	uint16_t sLevel;
	uint32_t iFormID;
	uint16_t sCount;
};
