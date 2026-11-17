#pragma once

#include "Reconstruction.hpp"

class hkpConstraintInstance;

// Havok's group-filter helper (the PDB's hkpGroupFilterUtil): static functions only, no instance members. The
// original source is Havok's Source/Physics/Utilities/Collide/Filter/GroupFilter/hkpGroupFilterUtil.cpp.
class hkpGroupFilterUtil {
public:
	// Gives both bodies of each constraint a sub-system id (bits 5-9) and a collision group
	// (bits 10-14) in their collision filter info, from the ids they already have or new ones from 1, and
	// groupFilterSystemGroup in bits 16-20, so the constraint's bodies do not collide with each other. Bodies
	// without a shape are skipped (HK_CALL cdecl, 0xD74B70, ret 0; the three stack arguments are constraints,
	// numConstraints and groupFilterSystemGroup).
	static void disableCollisionsBetweenConstraintBodies(const hkpConstraintInstance* const* constraints,
		int32_t numConstraints, int32_t groupFilterSystemGroup);
};
