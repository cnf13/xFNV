#pragma once

#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Reconstruction.hpp"

class hkpRayCollidableFilter;
class hkpRayHitCollector;
class hkpRayShapeCollectionFilter;
struct hkpWorldRayCastInput;

// Casts a bundle of world rays against the collidables a k-d tree bundle query reaches, one output per ray.
class hkKdTreeCollidableRayBundleCaster : public hkKdTreeBundleCastCollector {
public:
    hkKdTreeCollidableRayBundleCaster();
    ~hkKdTreeCollidableRayBundleCaster() override;

    void addMidPhaseHit(hkPrimitiveId primId, const hkVector4Comparison& mask) override;
    void getHitFractions(hkVector4& hitFractions) const override;

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
    static void operator delete(void* apMemory);

    hkpWorldRayCastInput* m_input;
    const hkpRayCollidableFilter* m_filter;
    const hkpRayShapeCollectionFilter* m_collectionFilter;
    hkpWorldRayCastOutput* m_output;
};

// Casts one world ray against the collidables a k-d tree query reaches, feeding the hits to a collector.
class hkpKdTreeCollidableRaycaster : public hkKdTreeCastCollector {
public:
    ~hkpKdTreeCollidableRaycaster() override;

    hkBool* addMidPhaseHit(hkBool* arResult, hkPrimitiveId primitiveId) override;
    hkBool* hasHit(hkBool* arResult) override;
    float getHitFraction() override;

    hkBool* castRayIterative(
        hkBool* apResult,
        const hkKdTree* tree,
        const hkpWorldRayCastInput& input,
        hkpWorldRayCastOutput& output
    );

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
    static void operator delete(void* apMemory);

    const hkpWorldRayCastInput* m_input;
    const hkpRayCollidableFilter* m_filter;
    const hkpRayShapeCollectionFilter* m_collectionFilter;
    hkpWorldRayCastOutput* m_output;
    hkpRayHitCollector* m_collector;
};

inline constexpr fnvd::Vtable hkpKdTreeCollidableRaycaster_vftable{};
