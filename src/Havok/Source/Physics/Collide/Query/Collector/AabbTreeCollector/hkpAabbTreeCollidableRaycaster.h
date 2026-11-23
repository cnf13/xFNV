#pragma once

#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"

class hkpCollidable;
class hkpRayCollidableFilter;
class hkpRayHitCollector;
class hkpRayShapeCollectionFilter;
struct hkpWorldRayCastInput;

// Casts a world ray against the collidables an AABB tree query reaches, for either tree precision.
class hkpAabbTreeCollidableRaycaster
  : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector,
    public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector {
public:
    hkpAabbTreeCollidableRaycaster();
    ~hkpAabbTreeCollidableRaycaster() override;

    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) override;
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) override;

protected:
    void testCollidable(const hkpCollidable* col);

public:
    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size); it also settles the two
    // Collector bases' own.
    static void operator delete(void* apMemory);

    const hkpWorldRayCastInput* m_input;
    const hkpRayCollidableFilter* m_filter;
    const hkpRayShapeCollectionFilter* m_collectionFilter;
    hkpWorldRayCastOutput* m_output;
    hkpRayHitCollector* m_collector;
    float m_earlyOutHitFraction;
};
