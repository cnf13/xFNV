#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

// Havok SDK signature (hkpCollide.lib, tied by bytes).
static void finishLoadedObjecthkpLinkedCollidable(void* a1, int32_t a2);
// Havok SDK signature (hkpCollide.lib, tied by bytes).
static void cleanupLoadedObjecthkpLinkedCollidable(void* a1);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpLinkedCollidableTypeInfo = {
    "hkpLinkedCollidable",
    finishLoadedObjecthkpLinkedCollidable,
    cleanupLoadedObjecthkpLinkedCollidable,
    nullptr,
    &typeid(hkpLinkedCollidable),
    sizeof(hkpLinkedCollidable)
};
hkpLinkedCollidable::MembersArray hkpLinkedCollidable::Members{
    {"collisionEntries",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     offsetof(hkpLinkedCollidable, m_collisionEntries),
     nullptr}
};
hkClass hkpLinkedCollidableClass(
    "hkpLinkedCollidable",
    &hkpCollidableClass,
    sizeof(hkpLinkedCollidable),
    nullptr,
    0,
    nullptr,
    0,
    hkpLinkedCollidable::Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

// The SDK's finish constructor hkpLinkedCollidable(flag) in place: hkpCollidable(flag) clears the cached child AABBs,
// invalidates the bounding volume and sets the broad phase handle's owner offset when finishing; the collision entry
// array (not serialised) is default-constructed either way. The inline constructors are written out.
static void finishLoadedObjecthkpLinkedCollidable(void* a1, int32_t a2) {
    if (a1) {
        hkpLinkedCollidable* pCollidable = static_cast<hkpLinkedCollidable*>(a1);
        if (a2) {
            hkpCollidable::BoundingVolumeData& rData = pCollidable->m_boundingVolumeData;
            rData.m_capacityChildShapeAabbs = 0;
            rData.m_numChildShapeAabbs = 0;
            rData.m_childShapeAabbs = nullptr;
            rData.m_childShapeKeys = nullptr;
            rData.invalidate();
            pCollidable->m_broadPhaseHandle.setOwner(pCollidable);
        }
        pCollidable->m_collisionEntries.m_data = nullptr;
        pCollidable->m_collisionEntries.m_size = 0;
        pCollidable->m_collisionEntries.m_capacityAndFlags =
            hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG;
    }
}

// Destroys the object in place: only the collision entry array is cleared and deallocated; nothing of hkpCollidable's
// destructor runs here.
static void cleanupLoadedObjecthkpLinkedCollidable(void* a1) {
    hkpLinkedCollidable* pCollidable = static_cast<hkpLinkedCollidable*>(a1);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    hkArray<hkpLinkedCollidable::CollisionEntry>& rEntries = pCollidable->m_collisionEntries;
    rEntries.m_size = 0;
    if (!(rEntries.m_capacityAndFlags & hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG)) {
        pHeap->bufFree(rEntries.m_data, rEntries.getCapacity() * sizeof(hkpLinkedCollidable::CollisionEntry));
    }
    rEntries.m_capacityAndFlags = hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG;
    rEntries.m_data = nullptr;
}

void hkpLinkedCollidable::setCollisionInfoFlags(uint32_t aiFlags) {
    (&m_broadPhaseHandle)->SetCollisionFilterInfo(aiFlags);
}

// entries = m_collisionEntries, the SDK's inline hkArray::operator= for an element type that is not plain data: the
// storage grows through _reserve, the elements both arrays hold are assigned and the rest copy-constructed in place
// (with the placement new's null test). Only a fixed entity's list is sorted then.
void hkpLinkedCollidable::getCollisionEntriesSorted(hkArray<CollisionEntry>& entries) const {
    // EXE-INLINED none: hkArray<hkpLinkedCollidable::CollisionEntry>::operator=
    hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&entries);
    int32_t iOldSize = entries.m_size;
    int32_t iNewSize = m_collisionEntries.m_size;
    int32_t iCopied = (iNewSize > iOldSize) ? iOldSize : iNewSize;
    entries._reserve(alloc, iNewSize);
    hkArrayBase<CollisionEntry>::copy(entries.m_data, m_collisionEntries.m_data, iCopied);
    const CollisionEntry* pSrc = m_collisionEntries.m_data + iCopied;
    CollisionEntry* pDst = entries.m_data + iCopied;
    for (int32_t i = iNewSize - iCopied; i > 0; --i) {
        if (pDst != nullptr) {
            *pDst = *pSrc;
        }
        ++pDst;
        ++pSrc;
    }
    entries.m_size = iNewSize;

    // EXE-INLINED none: hkpEntity::isFixed
    const hkpEntity* pOwner = static_cast<const hkpEntity*>(getOwner());
    hkBool bFixed;
    if (pOwner->isFixed(&bFixed)->m_bool && iNewSize > 1) {
        // EXE-INLINED none: hkAlgorithm::quickSort
        hkAlgorithm::quickSortRecursive<CollisionEntry, bool (*)(const CollisionEntry&, const CollisionEntry&)>(
            entries.m_data, 0, iNewSize - 1, &cmpLessCollisionEntries
        );
    }
}

// The agent list at 0x50, returned by its address and not its contents.
hkArray<hkpLinkedCollidable::CollisionEntry>& hkpLinkedCollidable::getCollisionEntriesDeterministicUnchecked() {
    return m_collisionEntries;
}

// Sorts the entries with quickSort; then each agent entry learns its index on this collidable (slot 1 when this is its
// second collidable).
void hkpLinkedCollidable::sortEntries() {
    int32_t iSize = m_collisionEntries.m_size;
    if (iSize > 1) {
        // EXE-INLINED none: hkAlgorithm::quickSort
        hkAlgorithm::quickSortRecursive<CollisionEntry, bool (*)(const CollisionEntry&, const CollisionEntry&)>(
            m_collisionEntries.m_data, 0, iSize - 1, &cmpLessCollisionEntries
        );
    }
    for (int32_t i = 0; i < m_collisionEntries.m_size; ++i) {
        hkpAgentNnEntry* pEntry = m_collisionEntries.m_data[i].m_agentEntry;
        pEntry->m_agentIndexOnCollidable[this == pEntry->m_collidable[1]] = static_cast<uint16_t>(i);
    }
}

const hkClass& hkpLinkedCollidable::staticClass() {
    return hkpLinkedCollidableClass;
}

bool cmpLessCollisionEntries(
    const hkpLinkedCollidable::CollisionEntry& a1,
    const hkpLinkedCollidable::CollisionEntry& a2
) {
    return (bool)((uint8_t)((&a1)->m_partner->m_broadPhaseHandle.m_id < (&a2)->m_partner->m_broadPhaseHandle.m_id));
}
