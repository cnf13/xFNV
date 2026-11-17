#pragma once

#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Reconstruction.hpp"

// Stops collecting after the first colliding body pair.
class hkpFlagCdBodyPairCollector : public hkpCdBodyPairCollector {
public:
    hkpFlagCdBodyPairCollector() {}
    ~hkpFlagCdBodyPairCollector() override;

    void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) override;

    hkBool hasHit() const;

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
    static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpFlagCdBodyPairCollector_vftable_ConvexList{};
