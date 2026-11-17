#pragma once

#include "Reconstruction.hpp"

// A task packed into eight words: the first names what to do, the others are its arguments.
class BSPackedTask {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		PACKED_TASK_SIZE = 0x8,
	};

	// One word of a packed task, read as whichever type the task's function expects.
	union PackedTaskVariantData {
		bool bData;
		char pcDataA[4];
		int16_t psDataA[2];
		int32_t iData;
		float fData;
		void* pData;
	};

	PackedTaskVariantData pTaskDataA[PACKED_TASK_SIZE];
};
