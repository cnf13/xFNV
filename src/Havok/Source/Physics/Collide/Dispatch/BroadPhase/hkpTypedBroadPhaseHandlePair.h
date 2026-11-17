#pragma once

#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"

// A pair of broad phase handles that are hkpTypedBroadPhaseHandles.
class hkpTypedBroadPhaseHandlePair : public hkpBroadPhaseHandlePair {
public:
    hkpTypedBroadPhaseHandle* getElementA() const {
        return static_cast<hkpTypedBroadPhaseHandle*>(m_a);
    }

    hkpTypedBroadPhaseHandle* getElementB() const {
        return static_cast<hkpTypedBroadPhaseHandle*>(m_b);
    }

    // The handle of the pair that is not elem.
    hkpTypedBroadPhaseHandle* getOther(hkpTypedBroadPhaseHandle* elem) const {
        uintptr_t a = reinterpret_cast<uintptr_t>(m_a);
        uintptr_t b = reinterpret_cast<uintptr_t>(m_b);
        uintptr_t e = reinterpret_cast<uintptr_t>(elem);
        return reinterpret_cast<hkpTypedBroadPhaseHandle*>(a ^ b ^ e);
    }
};
