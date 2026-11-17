#include "BSHavok/bhkCharacterPointCollector.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkTrapListener.h"
#include "BSHavok/bhkWorld.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <cmath>

// Ends with hkpAllCdPointCollector's reset, after the members are set up.
bhkCharacterPointCollector::bhkCharacterPointCollector(bhkCharacterPointCollector* apStartCollector)
  : pStartCollector(apStartCollector) {
    hkpAllCdPointCollector::reset();
}

// The colliding objects' references are dropped; the arrays' destructors free their buffers.
bhkCharacterPointCollector::~bhkCharacterPointCollector() {
    for (int32_t i = 0; i < m_collidingObjects.m_size; ++i) {
        m_collidingObjects.m_data[i]->removeReference();
    }
}

// Every point appended to m_collisionPoints is copied whole from a local point whose m_shapeKeysA and m_shapeKeysB
// nothing writes: an uninitialised read reaching the array whenever a new object is added, kept deliberately as the
// game does. Records the touched object (new, or not removed), and for an entity or a character controller adds the
// hit; a hit on a triangle gets shape key A 2, or 1 where the triangle's normal, turned to face the contact's
// separating normal, is within half a degree of it, and then the hits are filtered (filterOutEdgeHits).
void bhkCharacterPointCollector::addCdPoint(const hkpCdPoint& event) {
    // EXE-INLINED 0x624020
    const hkpCdBody* pRootB = event.m_cdBodyB;
    while (pRootB->m_parent) {
        pRootB = pRootB->m_parent;
    }
    hkpWorldObject* pObject = static_cast<hkpWorldObject*>(static_cast<const hkpCollidable*>(pRootB)->getOwner());
    if (!pObject) {
        return;
    }
    // EXE-INLINED none: hkArrayBase<hkpWorldObject*>::indexOf
    int32_t iIndex = -1;
    int32_t iCount = m_collidingObjects.getSize();
    hkpWorldObject** ppObjects = m_collidingObjects.m_data;
    for (int32_t i = 0; i < iCount; ++i) {
        if (ppObjects[i] == pObject) {
            iIndex = i;
            break;
        }
    }
    if (iIndex == -1) {
        pObject->addReference();
        // EXE-INLINED none: hkArray<hkpWorldObject*>::pushBack
        hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
        if (m_collidingObjects.m_size ==
            (m_collidingObjects.m_capacityAndFlags & hkArrayBase<hkpWorldObject*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*rRouter.m_heap, &m_collidingObjects, sizeof(hkpWorldObject*));
        }
        m_collidingObjects.m_data[m_collidingObjects.m_size] = pObject;
        m_collidingObjects.m_size++;

        hkpRootCdPoint kPoint;
        kPoint.m_contact = event.m_contact;
        // EXE-INLINED 0x624020
        const hkpCdBody* pRootA = event.m_cdBodyA;
        while (pRootA->m_parent) {
            pRootA = pRootA->m_parent;
        }
        kPoint.m_rootCollidableA = static_cast<const hkpCollidable*>(pRootA);
        kPoint.m_shapeKeyA = event.m_cdBodyA->m_shapeKey;
        // EXE-INLINED 0x624020
        const hkpCdBody* pPointRootB = event.m_cdBodyB;
        while (pPointRootB->m_parent) {
            pPointRootB = pPointRootB->m_parent;
        }
        kPoint.m_rootCollidableB = static_cast<const hkpCollidable*>(pPointRootB);
        kPoint.m_shapeKeyB = event.m_cdBodyB->m_shapeKey;
        // EXE-INLINED none: hkArray<hkpRootCdPoint>::pushBack
        hkMemoryRouter& rPointRouter = hkMemoryRouter::getInstance();
        if (m_collisionPoints.m_size ==
            (m_collisionPoints.m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*rPointRouter.m_heap, &m_collisionPoints, sizeof(hkpRootCdPoint));
        }
        hkpRootCdPoint* pNewPoint = &m_collisionPoints.m_data[m_collisionPoints.m_size];
        if (pNewPoint) {
            *pNewPoint = kPoint;
        }
        m_collisionPoints.m_size++;
        // EXE-INLINED none: hkArray<bhkCharacterPointCollector::bhkCollisionObjectState>::pushBack
        hkMemoryRouter& rStateRouter = hkMemoryRouter::getInstance();
        if (m_collisionStates.m_size ==
            (m_collisionStates.m_capacityAndFlags & hkArrayBase<bhkCollisionObjectState>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*rStateRouter.m_heap, &m_collisionStates, sizeof(bhkCollisionObjectState));
        }
        bhkCollisionObjectState* pNewState = &m_collisionStates.m_data[m_collisionStates.m_size];
        if (pNewState) {
            *pNewState = COLLISION_OBJECT_NEW;
        }
        m_collisionStates.m_size++;
    }
    else {
        bhkCollisionObjectState& rState = m_collisionStates.m_data[iIndex];
        if (rState == COLLISION_OBJECT_REMOVED) {
            rState = COLLISION_OBJECT_UNTOUCHED;
        }
    }
    // The collision layer: the filter info's low 7 bits.
    if (pObject->m_collidable.m_broadPhaseHandle.m_type != hkpWorldObject::BROAD_PHASE_ENTITY &&
        (pObject->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0x7F) != L_CHARCONTROLLER) {
        return;
    }
    // EXE-INLINED none: hkArray<hkpRootCdPoint>::expandOne
    hkMemoryRouter& rHitRouter = hkMemoryRouter::getInstance();
    if (m_hits.m_size == (m_hits.m_capacityAndFlags & hkArrayBase<hkpRootCdPoint>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rHitRouter.m_heap, &m_hits, sizeof(hkpRootCdPoint));
    }
    hkpRootCdPoint& rHit = m_hits.m_data[m_hits.m_size++];
    rHit.m_contact = event.m_contact;
    // EXE-INLINED 0x624020
    const hkpCdBody* pHitRootA = event.m_cdBodyA;
    while (pHitRootA->m_parent) {
        pHitRootA = pHitRootA->m_parent;
    }
    rHit.m_rootCollidableA = static_cast<const hkpCollidable*>(pHitRootA);
    // EXE-INLINED 0x624020
    const hkpCdBody* pHitRootB = event.m_cdBodyB;
    while (pHitRootB->m_parent) {
        pHitRootB = pHitRootB->m_parent;
    }
    rHit.m_rootCollidableB = static_cast<const hkpCollidable*>(pHitRootB);
    rHit.m_shapeKeyA = CONTACTFEATURE_INVALID;
    rHit.m_shapeKeyB = event.m_cdBodyB->m_shapeKey;
    // cos(0.5 degrees), using the float nearest 0.5 degrees in radians.
    float fCosLimit = std::cos(0.008726646192371845);
    if (event.m_cdBodyB->m_shape->m_type.m_storage != HK_SHAPE_TRIANGLE) {
        return;
    }
    rHit.m_shapeKeyA = CONTACTFEATURE_NONFACE;
    const hkpCdBody* pBodyB = event.m_cdBodyB;
    const hkpTriangleShape* pTriangle = static_cast<const hkpTriangleShape*>(pBodyB->m_shape);
    hkVector4 kEdge0;
    kEdge0.setSub4(pTriangle->m_vertexB, pTriangle->m_vertexA);
    hkVector4 kEdge1;
    kEdge1.setSub4(pTriangle->m_vertexC, pTriangle->m_vertexB);
    hkVector4 kNormal;
    kNormal.setCross(kEdge0, kEdge1);
    kNormal.normalize3();
    kNormal.setRotatedDir(pBodyB->m_motion.pMotion->getRotation(), kNormal);
    hkVector4 kFacing = kNormal;
    if (static_cast<float>(kNormal.dot3(rHit.m_contact.m_separatingNormal)) < 0.0f) {
        kFacing.setNeg4(kNormal);
    }
    if (fCosLimit < static_cast<float>(kFacing.dot3(rHit.m_contact.m_separatingNormal))) {
        rHit.m_shapeKeyA = CONTACTFEATURE_FACE;
    }
    filterOutEdgeHits();
}

void bhkCharacterPointCollector::reset() {
    m_hits.m_size = (uint32_t)0;
    m_earlyOutDistance = 0x1.ffffdc0000000p+127f;
}

// Drops the hits behind an earlier face hit: a hit j whose shape key A holds CONTACTFEATURE_FACE (a triangle facing the
// sweep) removes a hit i that lies behind it ((j - i) . j's separating normal > 0), unless hit i is a face hit itself;
// then the same test of this collector's hits against the start collector's. Each hit i is tested against the hits j
// below i - 1 only, and the first pass stops at the first removal; the second pass does not stop.
void bhkCharacterPointCollector::filterOutEdgeHits() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    for (int32_t i = m_hits.getSize() - 1; i > 0; --i) {
        int32_t j = i - 1;
        if (j == 0) {
            continue;
        }
        hkpRootCdPoint* pHits = m_hits.m_data;
        do {
            --j;
            if (pHits[j].m_shapeKeyA == CONTACTFEATURE_FACE && pHits[i].m_shapeKeyA != CONTACTFEATURE_FACE) {
                hkVector4 kOffset;
                kOffset.setSub4(pHits[j].m_contact.m_position, pHits[i].m_contact.m_position);
                if (static_cast<float>(kOffset.dot3(pHits[j].m_contact.m_separatingNormal)) > 0.0f) {
                    // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the points'
                    // empty destruct there)
                    m_hits.m_size--;
                    if (m_hits.m_size != i) {
                        hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                            m_hits.m_data + i, m_hits.m_data + m_hits.m_size
                        );
                    }
                    break;
                }
            }
        }
        while (j != 0);
    }
    bhkCharacterPointCollector* pStart = pStartCollector;
    if (pStart) {
        int32_t k = pStart->m_hits.getSize();
        while (k != 0) {
            --k;
            int32_t m = m_hits.getSize();
            while (m != 0) {
                --m;
                const hkpRootCdPoint& rStart = pStartCollector->m_hits.m_data[k];
                const hkpRootCdPoint& rHit = m_hits.m_data[m];
                if (rStart.m_shapeKeyA == CONTACTFEATURE_FACE && rHit.m_shapeKeyA != CONTACTFEATURE_FACE) {
                    hkVector4 kOffset;
                    kOffset.setSub4(rStart.m_contact.m_position, rHit.m_contact.m_position);
                    if (static_cast<float>(kOffset.dot3(rStart.m_contact.m_separatingNormal)) > 0.0f) {
                        // EXE-INLINED none: hkArrayBase<hkpRootCdPoint>::removeAt (the exe has no call of the
                        // points' empty destruct there)
                        m_hits.m_size--;
                        if (m_hits.m_size != m) {
                            hkMemUtil::memCpyOneAligned<sizeof(hkpRootCdPoint), alignof(hkpRootCdPoint)>(
                                m_hits.m_data + m, m_hits.m_data + m_hits.m_size
                            );
                        }
                    }
                }
            }
        }
    }
}

// Reports the points of this sweep's trap and cloud-trap objects to their world's trap listener (added for a new
// object, removed for one the sweep no longer touches), then drops the objects marked removed and marks the rest
// removed for the next sweep.
void bhkCharacterPointCollector::handleCollisions() {
    m_hits.m_size = 0;
    m_earlyOutDistance = 3.40282e+38f;
    for (int32_t i = 0; i < m_collisionStates.m_size; ++i) {
        // The collision layer: the filter info's low 7 bits.
        uint32_t uiLayer = m_collidingObjects.m_data[i]->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0x7F;
        hkpRootCdPoint& rPoint = m_collisionPoints.m_data[i];
        hkpWorldObject* pOwner = static_cast<hkpWorldObject*>(rPoint.m_rootCollidableB->getOwner());
        ahkpWorld* pWorld = pOwner ? static_cast<ahkpWorld*>(pOwner->m_world) : nullptr;
        if (!pWorld) {
            continue;
        }
        bhkWorld* pbhkWorld = static_cast<bhkWorld*>(pWorld->m_userData);
        if (!pbhkWorld) {
            continue;
        }
        if (uiLayer != L_TRAP && uiLayer != L_CLOUDTRAP) {
            continue;
        }
        bhkEntityListener* pListener = pbhkWorld->pTrapListener;
        if (!pListener) {
            continue;
        }
        bhkCollisionObjectState eState = m_collisionStates.m_data[i];
        if (eState == COLLISION_OBJECT_NEW) {
            pListener->contactPointAddedCallback(rPoint);
        }
        else if (eState == COLLISION_OBJECT_REMOVED) {
            pListener->contactPointRemovedCallback(rPoint);
        }
    }
    for (int32_t i = m_collisionStates.m_size - 1; i >= 0; --i) {
        if (m_collisionStates.m_data[i] == COLLISION_OBJECT_REMOVED) {
            m_collidingObjects.m_data[i]->removeReference();
            m_collidingObjects.removeAt(i);
            m_collisionPoints.removeAt(i);
            m_collisionStates.removeAt(i);
        }
        else {
            m_collisionStates.m_data[i] = COLLISION_OBJECT_REMOVED;
        }
    }
}

void bhkCharacterPointCollector::operator delete(void* apMemory, size_t aSize) {
    hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
}
