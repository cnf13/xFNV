#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Reconstruction.hpp"

// a segment swept by a sphere
class NiCapsule {
public:
	bool operator==(const NiCapsule& kCapsule) const;

	NiSegment m_kSegment;
	float m_fRadius;
};
