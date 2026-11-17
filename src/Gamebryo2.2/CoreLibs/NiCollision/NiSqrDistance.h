#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiParallelogram.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// Owner of the static squared-distance overloads; there is no instance state or vtable.
class NiSqrDistance {
public:
	// Point to segment; fSegParam receives the closest segment parameter (cdecl, ST0,
	// 0xEAF1F0).
	static float Compute(const NiPoint3& kPnt, const NiSegment& kSeg, float& fSegParam);
	// Segment to segment (cdecl, ST0, 0xEAF310).
	static float Compute(const NiSegment& kSeg0, const NiSegment& kSeg1, float& fSeg0Param, float& fSeg1Param);
	// Segment to triangle (cdecl, ST0, 0xEB1D20).
	static float Compute(const NiSegment& kSeg, const NiTrigon& kTri, float& fSegParm, float& fTriParm0,
		float& fTriParm1);
	// Point to box (cdecl, ST0, 0xEB42E0).
	static float Compute(const NiPoint3& kPoint, const NiBox& kBox, float& fBoxParam0, float& fBoxParam1,
		float& fBoxParam2);
	// Segment to box (cdecl, ST0, 0xEB50F0).
	static float Compute(const NiSegment& kSeg, const NiBox& kBox, float& fSegParam, float& fBoxParam0,
		float& fBoxParam1, float& fBoxParam2);
	// Point to triangle (cdecl, ST0, 0xEB5450).
	static float Compute(const NiPoint3& kPnt, const NiTrigon& kTri, float& fTriParam0, float& fTriParam1);
	// Point to parallelogram (cdecl, ST0, 0xEB6150).
	static float Compute(const NiPoint3& kPnt, const NiParallelogram& kPgm, float& fPgmParam0, float& fPgmParam1);
	// Segment to parallelogram (cdecl, ST0, 0xEAFF20; NiDistSegPgm.cpp).
	static float Compute(const NiSegment& kSeg, const NiParallelogram& kPgm, float& fSegParam, float& fPgmParam0,
		float& fPgmParam1);
	// Triangle to parallelogram (cdecl, ST0, 0xEB3230; NiDistTriPgm.cpp): the segment
	// distances of the triangle's and the parallelogram's edges against each other.
	static float Compute(const NiTrigon& kTri, const NiParallelogram& kPgm, float& fTriParam0, float& fTriParam1,
		float& fPgmParam0, float& fPgmParam1);
};
