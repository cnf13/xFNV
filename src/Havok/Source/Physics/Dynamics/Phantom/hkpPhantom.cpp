#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <cstring>
#include <xmmintrin.h>

hkpPhantom::MembersArray hkpPhantom::Members{
    {"overlapListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x8C,
     nullptr},
    {"phantomListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x98,
     nullptr}
};

hkClass hkpPhantomClass(
    "hkpPhantom",
    &hkpWorldObjectClass,
    164,
    nullptr,
    0,
    nullptr,
    0,
    hkpPhantom::Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

// The listeners are told first; the listener arrays are then freed and hkpWorldObject's destructor runs.
hkpPhantom::~hkpPhantom() {
    firePhantomDeleted();
}

// The base's statistics are collected under hkpWorldObject's class, not cls, then both listener arrays' storage: an
// array whose storage it doesn't own counts only when the collector includes unmanaged memory. That bool is tested as a
// byte against 0, so its byte is copied into a uint8_t: any nonzero byte is true.
void hkpPhantom::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    hkpWorldObject::calcContentStatistics(collector, &hkpWorldObjectClass);
    int32_t iCapacityAndFlags = m_overlapListeners.m_capacityAndFlags;
    uint8_t uiIncludeUnmanaged;
    std::memcpy(&uiIncludeUnmanaged, &collector->m_includeUnmanagedArrayMemory, 1u);
    if (iCapacityAndFlags >= 0 || uiIncludeUnmanaged != 0) {
        int32_t iAllocated = hkStatisticsCollector::getAllocatedSize(
            m_overlapListeners.m_data,
            (iCapacityAndFlags & hkArrayBase<hkpPhantomOverlapListener*>::CAPACITY_MASK) * 4
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "OvrlapLisPtr",
            m_overlapListeners.m_data,
            m_overlapListeners.m_size * 4,
            iAllocated
        );
    }
    iCapacityAndFlags = m_phantomListeners.m_capacityAndFlags;
    std::memcpy(&uiIncludeUnmanaged, &collector->m_includeUnmanagedArrayMemory, 1u);
    if (iCapacityAndFlags >= 0 || uiIncludeUnmanaged != 0) {
        int32_t iAllocated = hkStatisticsCollector::getAllocatedSize(
            m_phantomListeners.m_data,
            (iCapacityAndFlags & hkArrayBase<hkpPhantomListener*>::CAPACITY_MASK) * 4
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PhantmLisPtr",
            m_phantomListeners.m_data,
            m_phantomListeners.m_size * 4,
            iAllocated
        );
    }
}

void hkpPhantom::deallocateInternalArrays() {
    hkMemoryAllocator* m_heap;
    int32_t m_capacityAndFlags;
    hkMemoryAllocator* v4;
    int32_t v5;
    if (!m_overlapListeners.m_size) {
        m_heap = hkMemoryRouter::getInstance().m_heap;
        m_capacityAndFlags = m_overlapListeners.m_capacityAndFlags;
        m_overlapListeners.m_size = 0;
        if (m_capacityAndFlags >= 0) {
            m_heap->bufFree(m_overlapListeners.m_data, sizeof(hkpPhantomOverlapListener*) * m_capacityAndFlags);
        }
        m_overlapListeners.m_data = nullptr;
        m_overlapListeners.m_capacityAndFlags = 0x80000000u;
    }
    if (!m_phantomListeners.m_size) {
        v4 = hkMemoryRouter::getInstance().m_heap;
        v5 = m_phantomListeners.m_capacityAndFlags;
        m_phantomListeners.m_size = 0;
        if (v5 >= 0) {
            v4->bufFree(m_phantomListeners.m_data, sizeof(hkpPhantomListener*) * v5);
        }
        m_phantomListeners.m_data = nullptr;
        m_phantomListeners.m_capacityAndFlags = 0x80000000u;
    }
}

void hkpPhantom::removePhantomListener(hkpPhantomListener* el) {
    int32_t v5;
    int32_t m_size = m_phantomListeners.m_size;
    int32_t v3 = 0;
    v5 = -1;
    if (m_size > 0) {
        hkpPhantomListener** m_data = static_cast<hkArrayBase<hkpPhantomListener*>&>(m_phantomListeners).begin();
        do {
            if (*m_data == el) {
                v5 = v3;
                break;
            }
            ++v3;
            ++m_data;
        }
        while (v3 < m_size);
    }
    int32_t v6 = 4 * (--m_phantomListeners.m_size - v5);
    hkpPhantomListener** v7 = &m_phantomListeners.m_data[v5];
    if (v6 > 0) {
        uint32_t v8 = (static_cast<uint32_t>(v6 - 1) >> 2) + 1;
        do {
            v7[0] = v7[1];
            ++v7;
            --v8;
        }
        while (v8);
    }
}

// While the world's critical operations are locked (for phantoms too), an UpdatePhantomBP operation is queued;
// otherwise the broad phase moves the phantom's box under the world's lock, the pairs it reports are dispatched, the
// bounding volume data is set and the pending operations run. The two pair arrays are stack-allocated hkLocalArrays of
// m_broadPhaseUpdateSize pairs each. hkpWorldOperationQueue::queueOperation copies a whole BiggestOperation from the
// UpdatePhantomBP local, whose padding after m_type and the storage past its end nothing wrote; this is deliberate.
void hkpPhantom::updateBroadPhase(const hkAabb& aabb) {
    hkpWorld* pWorld = m_world;
    if (!pWorld) {
        return;
    }
    if (pWorld->m_criticalOperationsLockCount + pWorld->m_criticalOperationsLockCountForPhantoms) {
        hkWorldOperation::UpdatePhantomBP op;
        op.m_type = hkWorldOperation::PHANTOM_UPDATE_BROAD_PHASE;
        op.m_phantom = this;
        op.m_aabb = const_cast<hkAabb*>(&aabb);
        pWorld->queueOperation(op);
        return;
    }
    pWorld->lockCriticalOperations();

    const int32_t iNewCapacity = m_world->m_broadPhaseUpdateSize;
    hkLocalArray<hkpBroadPhaseHandlePair> newPairs(iNewCapacity);

    const int32_t iRemovedCapacity = m_world->m_broadPhaseUpdateSize;
    hkLocalArray<hkpBroadPhaseHandlePair> removedPairs(iRemovedCapacity);

    hkpBroadPhaseHandle* pHandle = m_collidable.getBroadPhaseHandle();
    m_world->getBroadPhase()->lock();
    m_world->getBroadPhase()->updateAabbs(&pHandle, &aabb, 1, newPairs, removedPairs);
    if (newPairs.m_size || removedPairs.m_size) {
        hkpTypedBroadPhaseDispatcher::removeDuplicates(newPairs, removedPairs);
        m_world->m_broadPhaseDispatcher->removePairs(
            static_cast<hkpTypedBroadPhaseHandlePair*>(removedPairs.m_data),
            removedPairs.m_size
        );
        hkpWorld* pFilterWorld = m_world;
        const hkpCollidableCollidableFilter* pFilter = static_cast<const hkpCollidableCollidableFilter*>(
            pFilterWorld->m_collisionFilter
        );
        pFilterWorld->m_broadPhaseDispatcher
            ->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.m_data), newPairs.m_size, pFilter);
        removeNullOverlapListeners();
    }
    m_world->getBroadPhase()->unlock();
    setBoundingVolumeData(aabb);
    hkpWorld* pTreeWorld = m_world;
    if (pTreeWorld->m_kdTreeManager) {
        pTreeWorld->m_kdTreeManager->setUpToDate(false);
    }
    m_world->unlockAndAttemptToExecutePendingOperations();
}

// The list is walked from the back so that a listener may remove itself while it is told.
void hkpPhantom::firePhantomAdded() {
    for (int32_t i = m_phantomListeners.m_size - 1; i >= 0; --i) {
        hkpPhantomListener* pkListener = m_phantomListeners.m_data[i];
        if (pkListener != nullptr) {
            pkListener->phantomAddedCallback(this);
        }
    }
    removeNullPhantomListeners();
}

// Same walk, but the listeners are told the phantom is gone.
void hkpPhantom::firePhantomRemoved() {
    for (int32_t i = m_phantomListeners.m_size - 1; i >= 0; --i) {
        hkpPhantomListener* pkListener = m_phantomListeners.m_data[i];
        if (pkListener != nullptr) {
            pkListener->phantomRemovedCallback(this);
        }
    }
    removeNullPhantomListeners();
}

// The box is quantised into the bounding volume data taken as an hkAabbUint32. The fourth lane of each corner is stored
// over the four bytes after m_min and m_max; then the expansion bytes are cleared, which leaves m_padding the high byte
// of the upper corner's fourth lane.
void hkpPhantom::setBoundingVolumeData(const hkAabb& aabb) {
    const hkpProcessCollisionInput* pInput = m_world->m_collisionInput;
    hkpCollidable::BoundingVolumeData& rData = m_collidable.m_boundingVolumeData;
    hkVector4Util::convertAabbToUint32(
        aabb,
        pInput->m_aabb32Info.m_bitOffsetLow,
        pInput->m_aabb32Info.m_bitOffsetHigh,
        pInput->m_aabb32Info.m_bitScale,
        reinterpret_cast<hkAabbUint32&>(rData)
    );
    rData.m_expansionShift = 0;
    rData.m_expansionMin[0] = 0;
    rData.m_expansionMin[1] = 0;
    rData.m_expansionMin[2] = 0;
    rData.m_expansionMax[0] = 0;
    rData.m_expansionMax[1] = 0;
    rData.m_expansionMax[2] = 0;
}

void hkpPhantom::addPhantomListener(hkpPhantomListener* el) {
    m_phantomListeners.pushBack(el);
}

void hkpPhantom::addPhantomOverlapListener(hkpPhantomOverlapListener* el) {
    m_overlapListeners.pushBack(el);
}

void hkpPhantom::removeNullOverlapListeners() {
    for (int32_t i = m_overlapListeners.m_size - 1; i >= 0; --i) {
        if (!m_overlapListeners.m_data[i]) {
            int32_t v2 = 4 * (--m_overlapListeners.m_size - i);
            hkpPhantomOverlapListener** v3 = &m_overlapListeners.m_data[i];
            if (v2 > 0) {
                uint32_t v4 = (static_cast<uint32_t>(v2 - 1) >> 2) + 1;
                do {
                    v3[0] = v3[1];
                    ++v3;
                    --v4;
                }
                while (v4);
            }
        }
    }
}

void hkpPhantom::removeNullPhantomListeners() {
    for (int32_t i = m_phantomListeners.m_size - 1; i >= 0; --i) {
        if (!m_phantomListeners.m_data[i]) {
            m_phantomListeners.removeAtAndCopy(i);
        }
    }
}

void hkpPhantom::removePhantomOverlapListener(hkpPhantomOverlapListener* el) {
    int32_t m_size_2;
    int32_t m_size = m_overlapListeners.m_size;
    int32_t m_size_1 = 0;
    m_size_2 = -1;
    if (m_size > 0) {
        hkpPhantomOverlapListener** m_data = m_overlapListeners.begin();
        do {
            if (*m_data == el) {
                m_size_2 = m_size_1;
                break;
            }
            ++m_size_1;
            ++m_data;
        }
        while (m_size_1 < m_size);
    }
    int32_t v6 = 4 * (--m_overlapListeners.m_size - m_size_2);
    hkpPhantomOverlapListener** v7 = &m_overlapListeners.m_data[m_size_2];
    if (v6 > 0) {
        // The count of moved elements is computed unsigned.
        uint32_t v8 = (static_cast<uint32_t>(v6 - 1) >> 2) + 1;
        do {
            v7[0] = v7[1];
            ++v7;
            --v8;
        }
        while (v8);
    }
}

void hkpPhantom::firePhantomDeleted() {
    for (int32_t i = m_phantomListeners.m_size - 1; i >= 0; --i) {
        hkpPhantomListener** m_data = m_phantomListeners.begin();
        bool v4 = m_data[i] == nullptr;
        hkpPhantomListener** v5 = &m_data[i];
        if (!v4) {
            (*v5)->phantomDeletedCallback(this);
        }
    }
}

void hkpPhantom::operator delete(void* p) {
    hkReferencedObject* pObject = static_cast<hkReferencedObject*>(p);
    hkMemoryRouter::getInstance().m_heap->blockFree(p, pObject->m_memSizeAndFlags);
}
