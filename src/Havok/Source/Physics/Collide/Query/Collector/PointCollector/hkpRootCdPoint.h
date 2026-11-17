#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class hkpCollidable;
struct hkpRootCdPoint;

// A contact point with the two root collidables it is between and the shape keys that lead from each root down to the
// shapes that touched (up to eight of them each). Not polymorphic. The PDB's source module is CameraCaster.cpp. The
// release PDB's struct stops at m_shapeKeyB (0x30 bytes, no shape key arrays); the exe's is the 0x70 one written here.
struct hkpRootCdPoint {
	hkpRootCdPoint();
	hkpRootCdPoint(const hkpRootCdPoint& __that);

	// Havok SDK (inline): compares the contact points' distances. The exe's out-of-line copy (thiscall, ret 8,
	// 0x9B30B0) returns the hkBool through the hidden result pointer.
	hkBool operator<(const hkpRootCdPoint& b) const;
	// The PDB's operator= (thiscall, ret 4, 0x6212C0), kept as a named forwarder so the
	// implicit copies src already makes of this struct keep their code.
	// Inferred type: __that: hkpRootCdPoint* from 0x620f72 (result of
	// hkArrayBase<hkpRootCdPoint>::operator[]<hkpRootCdPoint>), its members used at 0x6212cd
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
