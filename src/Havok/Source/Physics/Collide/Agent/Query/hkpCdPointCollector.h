#pragma once

#include "Reconstruction.hpp"

class hkpCdPoint;

// The visitor collision detection reports the contact points it found to; its owner lowers m_earlyOutDistance to
// stop the search once it has enough.
class hkpCdPointCollector {
public:
	hkpCdPointCollector();
	virtual ~hkpCdPointCollector();

	virtual void addCdPoint(const hkpCdPoint& aPoint) = 0;
	// The distance every contact point is compared against before it is reported: Havok's HK_REAL_MAX (3.40282e38f,
	// bits 0x7F7FFFEE, just below FLT_MAX), so the search collects nothing until the owner lowers it.
	virtual void reset() { m_earlyOutDistance = 3.40282e38f; }

	// Frees the block to the calling thread's router heap allocator (blockFree, vtable slot 2)
	// with the object's size, 8; nothing for null. The deleting destructors of this class and of
	// hkpCdBodyPairCollector call it.
	static void operator delete(void* p);

	float m_earlyOutDistance;
};
inline constexpr fnvd::Vtable hkpCdPointCollector_vftable{};
