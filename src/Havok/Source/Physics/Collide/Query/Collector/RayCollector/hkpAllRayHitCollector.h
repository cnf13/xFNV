#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"

// A ray hit collector that keeps every hit, in an inplace array of eight world ray cast outputs.
class hkpAllRayHitCollector : public hkpRayHitCollector {
public:
    hkpAllRayHitCollector();

    void addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) override;
    ~hkpAllRayHitCollector() override;

    // Empties the hits and resets the early-out fraction.
    void reset();
    // Sorts the hits with hkAlgorithm::quickSortRecursive when there are two or more.
    void sortHits();
    // m_hits.
    const hkArray<hkpWorldRayCastOutput>& getHits() const;

    hkInplaceArray<hkpWorldRayCastOutput, 8> m_hits;
};
