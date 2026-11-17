#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// A line segment between two points (the navmesh math's, not BSSegmentedTriShape::Segment).
struct Segment {
	// The exe's locals of the class are left unbuilt.
	Segment() = default;
	Segment(const NiPoint3& aSegment, const NiPoint3& a2);

	void Offset(const NiPoint3& aOffset);
	NiPoint3* Delta(NiPoint3* apResult);
	// Copies aSegment's two points; returns this (thiscall, ret 4). The exe folds it with
	// PathingSplineKey::operator= (0x6E7D10), registered there.
	Segment& operator=(const Segment& aSegment);

	NiPoint3 Start;
	NiPoint3 End;
};
