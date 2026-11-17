#pragma once

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class hkpCollidable;
struct hkpRootCdPoint;

// A contact point with the two root collidables it is between and the shape keys that lead from each root down to the
// shapes that touched (up to eight of them each). Not polymorphic.
struct hkpRootCdPoint {
	hkpRootCdPoint();
	hkpRootCdPoint(const hkpRootCdPoint& __that);

	// Compares the contact points' distances.
	hkBool operator<(const hkpRootCdPoint& b) const;
	hkpRootCdPoint& Assign(const hkpRootCdPoint& __that);

	hkContactPoint m_contact;
	const hkpCollidable* m_rootCollidableA;
	uint32_t m_shapeKeyA;
	uint32_t m_shapeKeysA[8];
	const hkpCollidable* m_rootCollidableB;
	uint32_t m_shapeKeyB;
	uint32_t m_shapeKeysB[8];
};

namespace hkAlgorithm {} // namespace hkAlgorithm
