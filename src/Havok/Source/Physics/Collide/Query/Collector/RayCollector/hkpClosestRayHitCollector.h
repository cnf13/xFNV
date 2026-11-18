#pragma once

#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Reconstruction.hpp"

// A ray hit collector that keeps only the nearest hit.
class hkpClosestRayHitCollector : public hkpRayHitCollector {
public:
    hkpClosestRayHitCollector();

    // Keeps the hit when it is nearer than the one held.
    void addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) override;
    ~hkpClosestRayHitCollector() override;

    // Hides the base's reset: calls m_rayHit.reset(), then hkpRayHitCollector::reset.
    void reset();

    hkpWorldRayCastOutput m_rayHit;
};
inline constexpr xfnv::Vtable hkpClosestRayHitCollector_vftable{};
