#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiParallelogram.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// Owner of the static squared-distance overloads; there is no instance state or vtable.
class NiSqrDistance {
public:
	// Point to segment; fSegParam receives the closest segment parameter.
	static float Compute(const NiPoint3& kPnt, const NiSegment& kSeg, float& fSegParam);
	// Segment to segment.
	static float Compute(const NiSegment& kSeg0, const NiSegment& kSeg1, float& fSeg0Param, float& fSeg1Param);
	// Segment to triangle.
	static float Compute(const NiSegment& kSeg, const NiTrigon& kTri, float& fSegParm, float& fTriParm0,
		float& fTriParm1);
	// Point to box.
	static float Compute(const NiPoint3& kPoint, const NiBox& kBox, float& fBoxParam0, float& fBoxParam1,
		float& fBoxParam2);
	// Segment to box.
	static float Compute(const NiSegment& kSeg, const NiBox& kBox, float& fSegParam, float& fBoxParam0,
		float& fBoxParam1, float& fBoxParam2);
	static float Compute(const NiPoint3& kPnt, const NiTrigon& kTri, float& fTriParam0, float& fTriParam1);
	// Point to parallelogram.
	static float Compute(const NiPoint3& kPnt, const NiParallelogram& kPgm, float& fPgmParam0, float& fPgmParam1);
	static float Compute(const NiSegment& kSeg, const NiParallelogram& kPgm, float& fSegParam, float& fPgmParam0,
		float& fPgmParam1);
	// Triangle to parallelogram: the segment distances of the triangle's and the parallelogram's edges against each
	// other.
	static float Compute(const NiTrigon& kTri, const NiParallelogram& kPgm, float& fTriParam0, float& fTriParam1,
		float& fPgmParam0, float& fPgmParam1);
};
