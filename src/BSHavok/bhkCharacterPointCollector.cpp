#include "BSHavok/bhkCharacterPointCollector.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntityListener.h"
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
    for (hkpWorldObject* pObject : m_collidingObjects) {
        pObject->removeReference();
    }
}

// Every point appended to m_collisionPoints is copied whole from a local point whose m_shapeKeysA and m_shapeKeysB
// nothing writes: an uninitialised read reaching the array whenever a new object is added, kept deliberately.
// Records the touched object (new, or not removed), and for an entity or a character controller adds the
// hit; a hit on a triangle gets shape key A 2, or 1 where the triangle's normal, turned to face the contact's
// separating normal, is within half a degree of it, and then the hits are filtered (filterOutEdgeHits).
void bhkCharacterPointCollector::addCdPoint(const hkpCdPoint& event) {
    hkpWorldObject* pObject = static_cast<hkpWorldObject*>(event.m_cdBodyB->getRootCollidable()->getOwner());
    if (!pObject) {
        return;
    }
    int32_t iIndex = m_collidingObjects.indexOf(pObject);
    if (iIndex == -1) {
        pObject->addReference();
        m_collidingObjects.pushBack(pObject);

        hkpRootCdPoint kPoint;
        kPoint.m_contact = event.m_contact;
        kPoint.m_rootCollidableA = event.m_cdBodyA->getRootCollidable();
        kPoint.m_shapeKeyA = event.m_cdBodyA->m_shapeKey;
        kPoint.m_rootCollidableB = event.m_cdBodyB->getRootCollidable();
        kPoint.m_shapeKeyB = event.m_cdBodyB->m_shapeKey;
        m_collisionPoints.pushBack(kPoint);
        m_collisionStates.pushBack(COLLISION_OBJECT_NEW);
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
    hkpRootCdPoint& rHit = m_hits.expandOne();
    rHit.m_contact = event.m_contact;
    rHit.m_rootCollidableA = event.m_cdBodyA->getRootCollidable();
    rHit.m_rootCollidableB = event.m_cdBodyB->getRootCollidable();
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
    m_hits.m_size = 0;
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
                    m_hits.removeAt(i);
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
                        m_hits.removeAt(m);
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
    m_earlyOutDistance = HK_REAL_MAX;
    for (int32_t i = 0; i < m_collisionStates.m_size; ++i) {
        // The collision layer: the filter info's low 7 bits.
        uint32_t uiLayer = m_collidingObjects.m_data[i]->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0x7F;
        hkpRootCdPoint& rPoint = m_collisionPoints.m_data[i];
        hkpWorldObject* pOwner = static_cast<hkpWorldObject*>(rPoint.m_rootCollidableB->getOwner());
        ahkpWorld* pWorld = pOwner ? static_cast<ahkpWorld*>(pOwner->m_world) : nullptr;
        if (!pWorld) {
            continue;
        }
        bhkWorld* pbhkWorld = pWorld->m_userData;
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
