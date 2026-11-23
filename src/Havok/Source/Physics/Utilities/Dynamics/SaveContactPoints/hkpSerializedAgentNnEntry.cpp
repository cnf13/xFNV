#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntryClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSerializedTrack1nInfoTypeInfo = {
    "hkpSerializedTrack1nInfo",
    finishLoadedObjecthkpSerializedTrack1nInfo,
    cleanupLoadedObjecthkpSerializedTrack1nInfo,
    nullptr,
    &typeid(hkpSerializedTrack1nInfo),
    sizeof(hkpSerializedTrack1nInfo)
};
hkTypeInfo hkpSerializedSubTrack1nInfoTypeInfo = {
    "hkpSerializedSubTrack1nInfo",
    finishLoadedObjecthkpSerializedSubTrack1nInfo,
    cleanupLoadedObjecthkpSerializedSubTrack1nInfo,
    nullptr,
    &typeid(hkpSerializedSubTrack1nInfo),
    sizeof(hkpSerializedSubTrack1nInfo)
};

// The saved sectors (512-byte blocks) and the sub tracks (32-byte blocks) go back to the thread's heap where the
// arrays own them; the arrays' storage goes with their members.
hkpSerializedTrack1nInfo::~hkpSerializedTrack1nInfo() {
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    if (m_sectors.m_capacityAndFlags >= 0) {
        for (hkpAgent1nSector* pSector : m_sectors) {
            if (pSector) {
                pHeap->blockFree(pSector, sizeof(hkpAgent1nSector));
            }
        }
    }
    if ((m_subTracks.m_capacityAndFlags & hkArrayBase<hkpSerializedSubTrack1nInfo*>::DONT_DEALLOCATE_FLAG) == 0) {
        for (hkpSerializedSubTrack1nInfo* pSubTrack : m_subTracks) {
            if (pSubTrack) {
                pSubTrack->~hkpSerializedSubTrack1nInfo();
                pHeap->blockFree(pSubTrack, sizeof(hkpSerializedSubTrack1nInfo));
            }
        }
    }
}

// No bodies, no ids, empty arrays and an empty contact atom (its type and info written out), version 1 and the endian
// check word.
hkpSerializedAgentNnEntry::hkpSerializedAgentNnEntry()
  : m_bodyA(nullptr), m_bodyB(nullptr), m_bodyAId(static_cast<hkUlong>(-1)), m_bodyBId(static_cast<hkUlong>(-1)),
    m_version(1) {
    m_useEntityIds.m_bool = 0;
    m_atom.m_type.m_storage = 22;
    m_atom.m_info.m_flags = 0;
    m_atom.m_info.m_index = 3;
    for (uint32_t& data : m_atom.m_info.m_data) {
        data = 0;
    }
    m_endianCheckBuffer[0] = ENDIAN_CHECK_VALUE & 0xFF;
    m_endianCheckBuffer[1] = ENDIAN_CHECK_VALUE >> 8;
    m_endianCheckBuffer[2] = 0;
    m_endianCheckBuffer[3] = 0;
}

// The two bodies lose their reference; the track info and the three arrays go with their members.
hkpSerializedAgentNnEntry::~hkpSerializedAgentNnEntry() {
    if (m_bodyA) {
        m_bodyA->removeReference();
    }
    if (m_bodyB) {
        m_bodyB->removeReference();
    }
}
