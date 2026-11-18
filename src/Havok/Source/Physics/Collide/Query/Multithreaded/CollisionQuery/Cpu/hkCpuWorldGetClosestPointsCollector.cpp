#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkCpuWorldGetClosestPointsCollector.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

using hkAabbTreeQueryUtils_lhkAabbTreeData_nhkAabbTreeAabb16_cunsigned_sshort_g_nCollector =
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector;

hkCpuWorldGetClosestPointsCollector::~hkCpuWorldGetClosestPointsCollector() {}

// The broad phase handle is the collidable's typed handle: its owner is the collidable it sits in.
float hkCpuWorldGetClosestPointsCollector::addBroadPhaseHandle(
    const hkpBroadPhaseHandle* broadphaseHandle,
    int32_t castIndex
) {
    const hkpTypedBroadPhaseHandle* pHandle = static_cast<const hkpTypedBroadPhaseHandle*>(broadphaseHandle);
    // getOwner: adds the handle's signed byte offset to its address as raw bytes.
    // EXE-INLINED 0x9B2C90
    const hkpCollidable* pCollidable =
        reinterpret_cast<const hkpCollidable*>(reinterpret_cast<const char*>(pHandle) + pHandle->m_ownerOffset);
    return testCollidable(pCollidable);
}

float hkCpuWorldGetClosestPointsCollector::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf
) {
    return testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
}

// The primitive id is an integer holding the collidable's address (the mediator's getPrimitiveId), so it is cast back
// from the integer to the collidable's pointer. The distance is dropped.
hkBool hkCpuWorldGetClosestPointsCollector::addMidPhaseHit(hkPrimitiveId primitiveId) {
    testCollidable(reinterpret_cast<const hkpCollidable*>(primitiveId));
    return 0;
}

hkBool hkCpuWorldGetClosestPointsCollector::hasHit() {
    hkpFixedBufferCdPointCollector* m_castCollector = this->m_castCollector;
    bool v3 = m_castCollector->m_numPoints.m_storage == 0;
    bool v4 = m_castCollector->m_numPoints.m_storage < 0;
    return !v4 && !v3;
}

float hkCpuWorldGetClosestPointsCollector::getHitFraction() {
    return m_castCollector->m_earlyOutDistance;
}

float hkCpuWorldGetClosestPointsCollector::testCollidable(const hkpCollidable* collidable) {
    const hkpShape* m_shape = collidable->m_shape;
    if (!collidable->m_shape) {
        return m_castCollector->m_earlyOutDistance;
    }
    const hkpCollidable* m_collidable = this->m_collidable;
    if (m_collidable != collidable) {
        if (!m_filter->isCollisionEnabled(*m_collidable, *collidable).m_bool) {
            return m_castCollector->m_earlyOutDistance;
        }
        m_input.m_dispatcher->m_agent2Func[m_input.m_dispatcher->m_agent2Types[m_shapeType][m_shape->m_type]]
            .m_getClosestPointFunc(*this->m_collidable, *collidable, m_input, *m_castCollector);
    }
    return m_castCollector->m_earlyOutDistance;
}
