#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class BSOcclusionQuery;

// A bounding sphere the shader accumulator keeps an occlusion query for: the sphere of a reference, the query that
// tests it and the last result. IDA calls it BSShaderAccumulator::ActorOcclusionTest (its list is
// BSShaderAccumulator::kActorOcclusionTestList); the PDB's name is ReferenceVolume.
struct ReferenceVolume {
	NiPoint3 Center;
	float fRadius;
	uint32_t uiReferenceID;
	BSOcclusionQuery* pOcclusionQuery;
	bool bOccluded;
	bool bWaiting;
	bool bPerformTest;
	uint8_t padding1B;
	uint32_t uiPixelCount;
};
