#pragma once

#include "Reconstruction.hpp"

#include <memory>
#include <vector>

// FaceGen's linear control (BSFaceGenManager.h).
struct FanLinCtlS;
class FutBinaryFileC;

// The FaceGen manager's group of linear controls: a 2 by 2 grid of control lists. The PDB places the class in
// BSFaceGenManager.cpp.
class FanLinCtlGrpS {
public:
	FanLinCtlGrpS();
	~FanLinCtlGrpS();

	bool readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]);

	// IDA: BSFaceGenManager::Struct0::Struct00[2][2] grp (24-byte blocks: the exe's vectors).
	std::vector<FanLinCtlS, std::allocator<FanLinCtlS>> grp[2][2];
};
