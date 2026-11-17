#pragma once

#include "SDK/MSVC/ehvec.h"

#include <memory>
#include <vector>

struct FffModeGS;

// The basis key and the two basis-mode vectors loaded from a FaceGen .egm file.
struct EGMData {
	EGMData(char* apFileName);
	~EGMData();

	static void operator delete(void* apMemory);

	uint32_t lEGMBasisKey;
	std::vector<FffModeGS, std::allocator<FffModeGS>> EGMBasis[2];
};
