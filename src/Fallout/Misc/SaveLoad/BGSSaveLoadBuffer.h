#pragma once

#include "Reconstruction.hpp"

// The size of a save-game buffer's items. Values from the Xbox PDB.
enum SIZE_TYPE : int32_t {
	SIZE_CHAR = 0,
	SIZE_SHORT = 1,
	SIZE_INT = 2,
	SIZE_COUNT = 3,
};

// A save-game byte buffer: the pointer to its bytes (IDA name; the PDB has no such class).
class BGSSaveLoadBuffer {
public:
	char* pBuffer;
};
