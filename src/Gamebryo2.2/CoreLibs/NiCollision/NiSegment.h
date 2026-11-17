#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// a line segment from an origin along a direction whose length is the segment length
class NiSegment {
public:
	// The point at parameter fS along the segment: origin + fS * direction (hidden-pointer return, 0xEB6090).
	NiPoint3 GetPoint(float fS) const;
	// Inferred type: a2: NiSegment* from 0xeae699 (&NiCapsule+0x0 (parameter 0 of NiCapsule::operator==)), its members
	// used at 0xeb60e0
	int32_t IsEqual(NiSegment* a2);

	NiPoint3 m_kOrigin;
	NiPoint3 m_kDirection;
};
