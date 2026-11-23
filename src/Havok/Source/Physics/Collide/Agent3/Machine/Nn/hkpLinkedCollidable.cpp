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
static void finishLoadedObjecthkpLinkedCollidable(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by bytes).
static void cleanupLoadedObjecthkpLinkedCollidable(void* p);

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
static void finishLoadedObjecthkpLinkedCollidable(void* p, int32_t finishing) {
    if (p) {
        hkpLinkedCollidable* pCollidable = static_cast<hkpLinkedCollidable*>(p);
        if (finishing) {
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
        pCollidable->m_collisionEntries
            .m_capacityAndFlags = hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG;
    }
}

// Destroys the object in place: only the collision entry array is cleared and deallocated; nothing of hkpCollidable's
// destructor runs here.
static void cleanupLoadedObjecthkpLinkedCollidable(void* p) {
    hkpLinkedCollidable* pCollidable = static_cast<hkpLinkedCollidable*>(p);
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
    m_broadPhaseHandle.SetCollisionFilterInfo(aiFlags);
}

// Copies the entries out; only a fixed entity's list is sorted then.
void hkpLinkedCollidable::getCollisionEntriesSorted(hkArray<CollisionEntry>& entries) const {
    entries = m_collisionEntries;
    const int32_t iNewSize = entries.getSize();

    const hkpEntity* pOwner = static_cast<const hkpEntity*>(getOwner());
    if (pOwner->isFixed().m_bool) {
        hkAlgorithm::quickSort(entries.m_data, iNewSize, &cmpLessCollisionEntries);
    }
}

// The agent list at 0x50, returned by its address and not its contents.
hkArray<hkpLinkedCollidable::CollisionEntry>& hkpLinkedCollidable::getCollisionEntriesDeterministicUnchecked() {
    return m_collisionEntries;
}

// Sorts the entries with quickSort; then each agent entry learns its index on this collidable (slot 1 when this is its
// second collidable).
void hkpLinkedCollidable::sortEntries() {
    hkAlgorithm::quickSort(m_collisionEntries.m_data, m_collisionEntries.m_size, &cmpLessCollisionEntries);
    for (int32_t i = 0; i < m_collisionEntries.m_size; ++i) {
        hkpAgentNnEntry* pEntry = m_collisionEntries.m_data[i].m_agentEntry;
        pEntry->m_agentIndexOnCollidable[this == pEntry->m_collidable[1]] = static_cast<uint16_t>(i);
    }
}

const hkClass& hkpLinkedCollidable::staticClass() {
    return hkpLinkedCollidableClass;
}

bool cmpLessCollisionEntries(
    const hkpLinkedCollidable::CollisionEntry& entry0,
    const hkpLinkedCollidable::CollisionEntry& entry1
) {
    return entry0.m_partner->m_broadPhaseHandle.m_id < entry1.m_partner->m_broadPhaseHandle.m_id;
}
