#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsule.h"

// Tests only the low byte of NiSegment::IsEqual's result.
bool NiCapsule::operator==(const NiCapsule& kCapsule) const {
	NiSegment& kSegment = const_cast<NiSegment&>(m_kSegment);
	if (!static_cast<uint8_t>(kSegment.IsEqual(const_cast<NiSegment*>(&kCapsule.m_kSegment)))) {
		return false;
	}
	return kCapsule.m_fRadius == m_fRadius;
}
