#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"

class hkJobQueue;
class hkJobThreadPool;
class hkKdTree;
class hkpCdPointCollector;
class hkpCollidable;
class hkpCollidableCollidableFilter;
class hkpCollisionAgentConfig;
class hkpCollisionFilter;
class hkpCollisionInput;
class hkpLinearCastInput;
class hkpRayHitCollector;
struct hkpWorldRayCastInput;

template <class BOUNDING_VOLUME, typename IDX>
class hkAabbTree;

// The base of the world managers that keep a tree of the world's collidables for ray and linear cast queries.
class hkpTreeWorldManager : public hkReferencedObject {
public:
    enum Type : int32_t {
        MANAGER_KDTREE = 0x0,
        MANAGER_AABBTREE = 0x1,
    };

    virtual void updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) = 0;
    virtual bool isUpToDate() const = 0;
    virtual void setUpToDate(bool upToDate) = 0;
    virtual void castRay(
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        hkpWorldRayCastOutput& output
    ) const = 0;
    virtual void castRay(
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        hkpRayHitCollector& collector
    ) const = 0;
    virtual void linearCast(
        const hkpCollidable* collA,
        const hkpLinearCastInput& input,
        const hkpCollidableCollidableFilter* filter,
        const hkpCollisionInput& collInput,
        hkpCollisionAgentConfig* config,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* startPointCollector
    ) = 0;
    virtual int32_t calcWorkingBufferSize(int32_t maxObjects);
    virtual void setWorkingBuffer(void* ptr) {}

    Type m_type;
    hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_inactiveTree;
    hkKdTree* m_kdTree;
};
