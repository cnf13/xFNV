#pragma once

#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkpCdPointCollector;
class hkpCollidable;
class hkpCollidableCollidableFilter;
class hkpCollisionAgentConfig;
class hkpLinearCastInput;

// Casts a collidable along a path against the collidables an AABB tree query reaches. There is one instance per tree
// precision (hkAabbTreeData::hkAabbTreeAabb and ::hkAabbTreeAabb16); both derive from the collectors of both
// precisions. The generic members are defined in the .cpp, with hkAabbTreeAabb16 explicitly instantiated there.
template <class BOUNDING_VOLUME>
class hkpAabbTreeWorldLinearCaster
  : public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector,
    public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector {
public:
    // Initialises m_shapeInput's members here, since hkpLinearCastCollisionInput has no constructor.
    hkpAabbTreeWorldLinearCaster() {
        m_shapeInput.m_maxExtraPenetration = 1.1920929e-07f;
        m_shapeInput.m_weldClosestPoints.m_storage = 0;
    }

    ~hkpAabbTreeWorldLinearCaster() override;

    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) override;
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) override;

    // Havok SDK: casts collA along the input's path against the tree's collidables.
    void linearCast(
        const hkAabbTree<BOUNDING_VOLUME, uint16_t>* tree,
        const hkpCollidable* collA,
        const hkpLinearCastInput& input,
        const hkpCollidableCollidableFilter* filter,
        const hkpCollisionInput& collInput,
        hkpCollisionAgentConfig* config,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* startPointCollector
    );
    // Havok SDK (protected there).
    void testCollidable(const hkpCollidable* collB);

    // DeletingDestruct's free (ourFree); it also settles the two Collector bases' own.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    const hkpLinearCastInput* m_input;
    const hkpCollidableCollidableFilter* m_filter;
    hkpCdPointCollector* m_castCollector;
    hkpCdPointCollector* m_startPointCollector;
    const hkpCollidable* m_collidableA;
    hkpShapeType m_typeA;
    hkpLinearCastCollisionInput m_shapeInput;
};

template <class BOUNDING_VOLUME>
hkpAabbTreeWorldLinearCaster<BOUNDING_VOLUME>::~hkpAabbTreeWorldLinearCaster() {}
