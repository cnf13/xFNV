#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

class hkpCollidable;

// A ray cast's result for the whole world: the shape-level output plus the root collidable that was hit, null while
// nothing was.
struct hkpWorldRayCastOutput : hkpShapeRayCastOutput {
    hkpWorldRayCastOutput();
    hkpWorldRayCastOutput(const hkpWorldRayCastOutput& __that);

    // Copies the shape-level output and m_rootCollidable (defined in Projectile.cpp).
    hkpWorldRayCastOutput& operator=(const hkpWorldRayCastOutput& __that);

    // Orders hits by hit fraction.
    hkBool operator<(const hkpWorldRayCastOutput& b) const {
        return m_hitFraction < b.m_hitFraction;
    }

    // Whether the cast hit anything.
    hkBool hasHit() const {
        return m_rootCollidable != nullptr;
    }

    // Clears m_rootCollidable and resets the shape-level output.
    void reset() {
        m_rootCollidable = nullptr;
        hkpShapeRayCastOutput::reset_0();
    }

    hkpCollidable* m_rootCollidable;
};

inline hkpWorldRayCastOutput& hkpWorldRayCastOutput::operator=(const hkpWorldRayCastOutput& __that) {
    hkpShapeRayCastOutput::operator=(__that);
    m_rootCollidable = __that.m_rootCollidable;
    return *this;
}

inline hkpWorldRayCastOutput::hkpWorldRayCastOutput() : m_rootCollidable(nullptr) {}

inline hkpWorldRayCastOutput::hkpWorldRayCastOutput(const hkpWorldRayCastOutput& __that)
  : hkpShapeRayCastOutput(&__that), m_rootCollidable(__that.m_rootCollidable) {}
