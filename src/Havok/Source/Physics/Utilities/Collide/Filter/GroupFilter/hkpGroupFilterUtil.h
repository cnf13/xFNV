#pragma once

#include <cstdint>

class hkpConstraintInstance;

// Havok's group-filter helper: static functions only, no instance members.
class hkpGroupFilterUtil {
public:
    // Gives both bodies of each constraint a sub-system id (bits 5-9) and a collision group (bits 10-14) in their
    // collision filter info, from the ids they already have or new ones from 1, and groupFilterSystemGroup in bits
    // 16-20, so the constraint's bodies do not collide with each other. Bodies without a shape are skipped.
    static void disableCollisionsBetweenConstraintBodies(
        const hkpConstraintInstance* const* constraints,
        int32_t numConstraints,
        int32_t groupFilterSystemGroup
    );
};
