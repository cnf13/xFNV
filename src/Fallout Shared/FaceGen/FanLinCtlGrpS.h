#pragma once

#include <memory>
#include <vector>

// FaceGen's linear control (BSFaceGenManager.h).
struct FanLinCtlS;
class FutBinaryFileC;

// The FaceGen manager's group of linear controls: a 2 by 2 grid of control lists.
class FanLinCtlGrpS {
public:
	FanLinCtlGrpS();
	~FanLinCtlGrpS();

	bool readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]);

	std::vector<FanLinCtlS, std::allocator<FanLinCtlS>> grp[2][2];
};
