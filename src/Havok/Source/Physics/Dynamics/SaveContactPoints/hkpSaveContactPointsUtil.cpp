#include "Havok/Source/Physics/Dynamics/SaveContactPoints/hkpSaveContactPointsUtil.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/BoxBox/hkpBoxBoxAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/CapsuleTriangle/hkpCapsuleTriangleAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Collection3/hkpCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/ConvexList3/hkpConvexListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskCylinderAgent3/hkpPredGskCylinderAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Atom/hkpConstraintAtomUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpPhysicsSystemWithContacts.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSaveContactPointsEndianUtil.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"
#include "SDK/MSVC/string.h"

using hkpSaveContactPointsUtil_saveWorld_t =
    void (*)(const hkpSaveContactPointsUtil::SavePointsInput&, const hkpWorld*, hkpPhysicsSystemWithContacts*);

namespace {

    // The selector loadContactPoints builds over its entity list.
    class hkEntitySelectorListed : public hkpSaveContactPointsUtil::EntitySelector {
    public:
        // The default constructor leaves the map empty.
        hkEntitySelectorListed() = default;
        // The constructor Construct stands for.
        hkEntitySelectorListed(hkpEntity** entities, int32_t numEntities);
        ~hkEntitySelectorListed() override;

        // Whether entity is one of the listed ones: m_map's value for it, 0 when absent.
        int32_t isEntityOk(const hkpEntity* entity) override;

        // +0x4: the listed entities, each mapped to 1.
        hkPointerMap<const hkpEntity*, int32_t> m_map;
    };

    int32_t hkEntitySelectorListed::isEntityOk(const hkpEntity* entity) {
        return m_map.getWithDefault(entity, 0);
    }

    // The empty map reserved for numEntities keys, then each entity mapped to 1.
    hkEntitySelectorListed::hkEntitySelectorListed(hkpEntity** entities, int32_t numEntities) {
        m_map.m_map.reserve(numEntities);
        for (int32_t i = 0; i < numEntities; i++) {
            m_map.insert(entities[i], 1);
        }
    }

    // The map frees its own slots.
    hkEntitySelectorListed::~hkEntitySelectorListed() {}

    // The selector of every entity.
    class hkEntitySelectorAll : public hkpSaveContactPointsUtil::EntitySelector {
    public:
        // Every entity is ok: 1.
        int32_t isEntityOk(const hkpEntity* entity) override;
    };

    int32_t hkEntitySelectorAll::isEntityOk(const hkpEntity* entity) {
        return 1;
    }

} // namespace

// Saves the agent entries of the numEntities entities into sys, each pair once: an entity's collidable goes into a map
// before its entries are saved, and an entry whose partner is in the map already is skipped.
using hkpSaveContactPointsCollidableMap = hkPointerMap<const hkpLinkedCollidable*, int32_t>;

hkpSaveContactPointsUtil::EntitySelector::~EntitySelector() {}

void hkpSaveContactPointsUtil::EntitySelector::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The islands are gathered into one array, active first; every agent entry of their agent tracks is saved into a
// new hkpSerializedAgentNnEntry, which sys keeps when the save succeeds. A sector holds 512 bytes of entries, the
// last one m_bytesUsedInLastSector.
void hkpSaveContactPointsUtil::saveContactPoints(
    const SavePointsInput& input,
    const hkpWorld* world,
    hkpPhysicsSystemWithContacts* sys
) {
    hkArray<hkpSimulationIsland*> islands;
    islands = world->m_activeSimulationIslands;
    islands
        .insertAt(islands.m_size, world->m_inactiveSimulationIslands.m_data, world->m_inactiveSimulationIslands.m_size);
    for (int32_t i = 0; i < islands.m_size; ++i) {
        hkpSimulationIsland* pIsland = islands.m_data[i];
        for (int32_t iSector = 0; iSector < pIsland->m_agentTrack.m_sectors.m_size;) {
            hkpAgentNnTrack& track = islands.m_data[i]->m_agentTrack;
            uint8_t* pEntry = track.m_sectors.m_data[iSector]->m_data;
            ++iSector;
            uint8_t* pEnd;
            if (iSector == track.m_sectors.m_size) {
                pEnd = pEntry + pIsland->m_agentTrack.m_bytesUsedInLastSector;
            }
            else {
                pEnd = pEntry + sizeof(hkpAgentNnSector);
            }
            // A sector's bytes are an untyped stream of variable-sized entries, each giving its size in m_size, so each
            // entry is read through a cast of the byte pointer.
            for (; pEntry < pEnd; pEntry += reinterpret_cast<hkpAgentNnEntry*>(pEntry)->m_size) {
                hkpSerializedAgentNnEntry* pSerialized = new hkpSerializedAgentNnEntry;
                if (serializeCollisionEntry(
                        input,
                        reinterpret_cast<hkpAgentNnEntry*>(pEntry),
                        world->m_collisionInput,
                        *pSerialized
                    ) == 0) {
                    sys->addContact(pSerialized);
                }
                pSerialized->removeReference();
            }
            pIsland = islands.m_data[i];
        }
    }
}

// Havok SDK: saves the numEntries agent entries at entries into sys, each serialized into a new
// hkpSerializedAgentNnEntry with the collision input of the world of the first entry's first entity.
void hkpSaveContactPointsUtil::saveContactPoints(
    const hkpSaveContactPointsUtil::SavePointsInput& input,
    const hkpAgentNnEntry** entries,
    int32_t numEntries,
    hkpPhysicsSystemWithContacts* sys
) {
    hkpEntity* entity = hkpGetRigidBody(entries[0]->m_collidable[0]);
    hkpWorld* world = entity->m_world;
    for (int32_t i = 0; i < numEntries; ++i) {
        hkpSerializedAgentNnEntry* pSerialized = new hkpSerializedAgentNnEntry;
        if (serializeCollisionEntry(input, entries[i], world->m_collisionInput, *pSerialized) == HK_SUCCESS) {
            sys->addContact(pSerialized);
        }
        pSerialized->removeReference();
    }
}

// KEEP (not registered): serializeEntry's isNnEntry argument is a 1 written into the low byte of the frame storage that
// later receives agent1nTrack and passed as the whole 32-bit word, so its upper three bytes are stack garbage nothing
// wrote. The callee reads only the low byte.
hkResult hkpSaveContactPointsUtil::serializeCollisionEntry(
    const hkpSaveContactPointsUtil::SavePointsInput& input,
    const hkpAgentNnEntry* entry,
    const hkpProcessCollisionInput* collisionInput,
    hkpSerializedAgentNnEntry& serializedEntryOut
) {
    // Only a simple constraint contact manager with contact points is saved.
    hkpDynamicsContactMgr* dynamicsMgr = static_cast<hkpDynamicsContactMgr*>(entry->m_contactMgr);
    if (dynamicsMgr->getType() != hkpContactMgr::TYPE_SIMPLE_CONSTRAINT_CONTACT_MGR) {
        return HK_FAILURE;
    }
    if (static_cast<hkpSimpleConstraintContactMgr*>(entry->m_contactMgr)
            ->m_contactConstraintData.m_atom->m_numContactPoints == 0) {
        return HK_FAILURE;
    }
    const hkpAgent1nTrack* agent1nTrack;
    int32_t sizeOfThisEntry = -1;
    hkpSerializedAgentNnEntry::SerializedAgentType agentType = hkpSerializedAgentNnEntry::INVALID_AGENT_TYPE;
    if (serializeEntry(
            entry,
            hkBool(true),
            collisionInput,
            sizeOfThisEntry,
            agentType,
            agent1nTrack,
            &serializedEntryOut.m_trackInfo
        )) {
        return HK_FAILURE;
    }
    serializedEntryOut.m_agentType = agentType;
    if (sizeOfThisEntry) {
        hkString::memCpy(serializedEntryOut.m_nnEntryData, entry, sizeOfThisEntry);
    }

    const hkpSimpleConstraintContactMgr* mgr = static_cast<hkpSimpleConstraintContactMgr*>(entry->m_contactMgr);
    const hkInplaceArray<uint8_t, 8, hkContainerHeapAllocator>& cpIds = mgr->m_contactConstraintData.m_idMgrA.m_values;
    serializedEntryOut.m_cpIdMgr.setSize(cpIds.m_size);
    for (int32_t i = 0; i < serializedEntryOut.m_cpIdMgr.m_size; ++i) {
        serializedEntryOut.m_cpIdMgr.m_data[i] = cpIds.m_data[i];
    }

    const hkpSimpleContactConstraintAtom* atom = mgr->m_contactConstraintData.m_atom;
    serializedEntryOut.m_propertiesStream.setSize(atom->m_numContactPoints * atom->getContactPointPropertiesStriding());
    hkString::memCpy(
        serializedEntryOut.m_propertiesStream.m_data,
        atom->getContactPointPropertiesStream(0),
        atom->m_numContactPoints * atom->getContactPointPropertiesStriding()
    );

    hkArray<hkContactPoint>& contactPoints = serializedEntryOut.m_contactPoints;
    contactPoints.setSize(atom->m_numContactPoints);
    hkString::memCpy(
        contactPoints.m_data,
        atom->getContactPoints(),
        atom->m_numContactPoints * static_cast<int32_t>(sizeof(hkContactPoint))
    );

    serializedEntryOut.m_atom = *mgr->m_contactConstraintData.m_atom;

    // The SDK's hkpGetRigidBody, inline: the owner of each entity's collidable, else null.
    const hkpLinkedCollidable* collidableA = entry->m_collidable[0];
    hkpEntity* bodyA = collidableA->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
        ? static_cast<hkpEntity*>(collidableA->getOwner())
        : nullptr;
    const hkpLinkedCollidable* collidableB = entry->m_collidable[1];
    hkpEntity* bodyB = collidableB->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
        ? static_cast<hkpEntity*>(collidableB->getOwner())
        : nullptr;

    const bool useEntityIds = input.m_useEntityIds;
    serializedEntryOut.m_useEntityIds.m_bool = useEntityIds;
    if (useEntityIds) {
        serializedEntryOut.m_bodyAId = input.m_getIdForEntity(bodyA);
        serializedEntryOut.m_bodyBId = input.m_getIdForEntity(bodyB);
    }
    else {
        serializedEntryOut.m_bodyA = bodyA;
        serializedEntryOut.m_bodyB = bodyB;
        bodyA->addReference();
        serializedEntryOut.m_bodyB->addReference();
    }
    return HK_SUCCESS;
}

// The agent data follows the entry's header: a 1n machine's padded or time-of-impact entry, or the Nn machine's
// (isNnEntry). Null and end entries count as 16 bytes; entries calling an agent directly cannot be saved, and the
// other commands are skipped with no size given.
hkResult hkpSaveContactPointsUtil::serializeEntry(
    const hkpAgentEntry* entry,
    hkBool isNnEntry,
    const hkpProcessCollisionInput* input,
    int32_t& sizeOfThisEntryOut,
    hkpSerializedAgentNnEntry::SerializedAgentType& agentTypeOut,
    const hkpAgent1nTrack*& agent1nTrackOut,
    hkpSerializedTrack1nInfo* trackInfoOut
) {
    agent1nTrackOut = nullptr;
    // The stream is untyped memory: the agent data sits a header's size in bytes after the entry.
    const uint8_t* entryBytes = static_cast<const uint8_t*>(static_cast<const void*>(entry));
    const hkpAgentData* agentData;
    switch (entry->m_streamCommand) {
        case hkAgent3::STREAM_NULL:
        case hkAgent3::STREAM_END: sizeOfThisEntryOut = 16; return HK_SUCCESS;
        case hkAgent3::STREAM_CALL:
        case hkAgent3::STREAM_CALL_FLIPPED:
        case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
        case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
            agentData = isNnEntry ? entryBytes + sizeof(hkpAgentNnMachinePaddedEntry)
                                  : entryBytes + sizeof(hkpAgent1nMachinePaddedEntry);
            break;
        case hkAgent3::STREAM_CALL_WITH_TIM:
        case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED:
        case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
        case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM:
            agentData = isNnEntry ? entryBytes + sizeof(hkpAgentNnMachineTimEntry)
                                  : entryBytes + sizeof(hkpAgent1nMachineTimEntry);
            break;
        case hkAgent3::STREAM_CALL_AGENT:
        case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: return HK_FAILURE;
        default: return HK_SUCCESS;
    }

    const hkpSerializedAgentNnEntry::SerializedAgentType agentType = getSerializedAgentType(
        input->m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc
    );
    agentTypeOut = agentType;
    switch (agentType) {
        case hkpSerializedAgentNnEntry::BOX_BOX_AGENT3:
        case hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3:
        case hkpSerializedAgentNnEntry::PRED_GSK_AGENT3:
        case hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3: sizeOfThisEntryOut = entry->m_size; return HK_SUCCESS;
        case hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3: {
            sizeOfThisEntryOut = entry->m_size;
            // A convex list agent in GSK mode has no track of sub-agents; in stream mode its data, after the
            // hkpGskCache, holds one (hkConvexListAgent3::getAgent1nTrack).
            const hkpGskCache* gskCache = static_cast<const hkpGskCache*>(agentData);
            if (gskCache->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
                return HK_SUCCESS;
            }
            const hkConvexListAgent3Data* listData = static_cast<const hkConvexListAgent3Data*>(
                static_cast<const void*>(gskCache + 1)
            );
            agent1nTrackOut = &listData->m_agentTrack;
            return serialize1nTrack(listData->m_agentTrack.m_sectors, input, *trackInfoOut);
        }
        case hkpSerializedAgentNnEntry::LIST_AGENT3:
        case hkpSerializedAgentNnEntry::BV_TREE_AGENT3:
        case hkpSerializedAgentNnEntry::COLLECTION_COLLECTION_AGENT3:
        case hkpSerializedAgentNnEntry::COLLECTION_AGENT3: {
            // These agents' data is their track of sub-agents.
            sizeOfThisEntryOut = entry->m_size;
            const hkpAgent1nTrack* track = static_cast<const hkpAgent1nTrack*>(agentData);
            agent1nTrackOut = track;
            return serialize1nTrack(track->m_sectors, input, *trackInfoOut);
        }
        default: return HK_FAILURE;
    }
}

// Havok SDK: destroys the entry's agent through the dispatcher's destroy function when the entry is an agent call
// of the saved type; returns 1 (HK_FAILURE) otherwise.
hkResult hkpSaveContactPointsUtil::destroyOldEntry(
    const hkpSerializedAgentNnEntry::SerializedAgentType agentType,
    const hkpProcessCollisionInput* input,
    hkpDynamicsContactMgr* mgr,
    hkpConstraintOwner* constraintOwner,
    hkpAgentNnEntry* nnEntry
) {
    // The stream is untyped memory: the agent data sits a header's size in bytes after the entry.
    uint8_t* entryBytes = reinterpret_cast<uint8_t*>(nnEntry);
    hkpAgentData* agentData;
    if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL) {
        agentData = entryBytes + sizeof(hkpAgentNnMachinePaddedEntry);
    }
    else if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL_WITH_TIM) {
        agentData = entryBytes + sizeof(hkpAgentNnMachineTimEntry);
    }
    else {
        return HK_FAILURE;
    }
    hkpCollisionDispatcher* dispatcher = input->m_dispatcher;
    const hkpCollisionDispatcher::Agent3Funcs& funcs = dispatcher->m_agent3Func[nnEntry->m_agentType];
    if (getSerializedAgentType(funcs.m_processFunc) != agentType ||
        agentType == hkpSerializedAgentNnEntry::INVALID_AGENT_TYPE) {
        return HK_FAILURE;
    }
    funcs.m_destroyFunc(nnEntry, agentData, mgr, *constraintOwner, dispatcher);
    return HK_SUCCESS;
}

// Havok SDK: rebuilds the entry's agent from the saved Nn entry when the entry's agent is of the saved type: the
// agent data after the header copied back, then for an agent with a 1n track (a convex list agent in stream mode,
// a list, bv tree or collection agent) the track rebuilt from serializedTrack.
hkResult hkpSaveContactPointsUtil::deserializeEntry(
    const hkpSerializedAgentNnEntry& serializedEntryIn,
    const hkpSerializedAgentNnEntry::SerializedAgentType agentType,
    const hkpSerializedTrack1nInfo& serializedTrack,
    const hkpProcessCollisionInput* input,
    hkpAgentNnEntry* nnEntry
) {
    // The stream is untyped memory: the agent data sits a header's size in bytes after the entry.
    uint8_t* entryBytes = reinterpret_cast<uint8_t*>(nnEntry);
    hkpAgentData* agentData;
    if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL) {
        agentData = entryBytes + sizeof(hkpAgentNnMachinePaddedEntry);
    }
    else if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL_WITH_TIM) {
        agentData = entryBytes + sizeof(hkpAgentNnMachineTimEntry);
    }
    else {
        return HK_FAILURE;
    }
    if (getSerializedAgentType(input->m_dispatcher->m_agent3Func[nnEntry->m_agentType].m_processFunc) != agentType) {
        return HK_FAILURE;
    }
    // Saved entries have the x64 agent size, so data saved by the exe does not load (DEPARTURES.md, native layouts in
    // written data).
    const hkpAgentNnEntry* savedEntry = reinterpret_cast<const hkpAgentNnEntry*>(serializedEntryIn.m_nnEntryData);
    const int32_t iAgentBytes = HK_AGENT3_AGENT_SIZE - static_cast<int32_t>(sizeof(hkpAgentNnEntry));
    hkpAgent1nTrack* track;
    switch (agentType) {
        case hkpSerializedAgentNnEntry::BOX_BOX_AGENT3:
        case hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3:
        case hkpSerializedAgentNnEntry::PRED_GSK_AGENT3:
        case hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3:
            hkString::memCpy(
                entryBytes + sizeof(hkpAgentNnEntry),
                serializedEntryIn.m_nnEntryData + sizeof(hkpAgentNnEntry),
                iAgentBytes
            );
            nnEntry->m_numContactPoints = savedEntry->m_numContactPoints;
            nnEntry->m_size = savedEntry->m_size;
            return HK_SUCCESS;
        case hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3: {
            hkString::memCpy(
                entryBytes + sizeof(hkpAgentNnEntry),
                serializedEntryIn.m_nnEntryData + sizeof(hkpAgentNnEntry),
                iAgentBytes
            );
            nnEntry->m_numContactPoints = savedEntry->m_numContactPoints;
            nnEntry->m_size = savedEntry->m_size;
            // A convex list agent in GSK mode has no track of sub-agents; in stream mode its data, after the
            // hkpGskCache, holds one (hkConvexListAgent3::getAgent1nTrack).
            hkpGskCache* gskCache = static_cast<hkpGskCache*>(agentData);
            if (gskCache->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
                return HK_SUCCESS;
            }
            track = &static_cast<hkConvexListAgent3Data*>(static_cast<void*>(gskCache + 1))->m_agentTrack;
            break;
        }
        case hkpSerializedAgentNnEntry::LIST_AGENT3:
        case hkpSerializedAgentNnEntry::BV_TREE_AGENT3:
        case hkpSerializedAgentNnEntry::COLLECTION_COLLECTION_AGENT3:
        case hkpSerializedAgentNnEntry::COLLECTION_AGENT3:
            hkString::memCpy(
                entryBytes + sizeof(hkpAgentNnEntry),
                serializedEntryIn.m_nnEntryData + sizeof(hkpAgentNnEntry),
                iAgentBytes
            );
            // These agents' data is their track of sub-agents.
            track = static_cast<hkpAgent1nTrack*>(agentData);
            break;
        default: return HK_FAILURE;
    }
    ::new (track) hkpAgent1nTrack;
    return deserialize1nTrack(serializedTrack, input, track->m_sectors);
}

// trackInfoOut's sector array is reserved for sectorsIn; each sector's entries are serialized (as 1n entries) into a
// new sub track, kept with its sector index and byte offset when it saved any sectors, else destroyed; then a copy of
// the sector is appended. A failed entry destroys its sub track and returns 1 (HK_FAILURE).
hkResult hkpSaveContactPointsUtil::serialize1nTrack(
    const hkArray<hkpAgent1nSector*>& sectorsIn,
    const hkpProcessCollisionInput* input,
    hkpSerializedTrack1nInfo& trackInfoOut
) {
    trackInfoOut.m_sectors.reserve(sectorsIn.m_size);
    for (int32_t i = 0; i < sectorsIn.m_size; ++i) {
        hkpAgent1nSector* pSector = sectorsIn.m_data[i];
        // A sector's bytes are an untyped stream of variable-sized entries, walked as raw memory.
        const uint8_t* pEntry = pSector->m_data;
        const uint8_t* pEnd = pSector->m_data + pSector->m_bytesAllocated;
        if (pEntry < pEnd) {
            hkBool isNnEntry;
            isNnEntry.m_bool = 0;
            do {
                const hkpAgent1nTrack* pTrack = nullptr;
                int32_t iSizeOfEntry = 0;
                hkpSerializedSubTrack1nInfo* pSubTrack = new hkpSerializedSubTrack1nInfo;
                hkpSerializedAgentNnEntry::SerializedAgentType eAgentType;
                if (serializeEntry(
                        reinterpret_cast<const hkpAgentEntry*>(pEntry),
                        isNnEntry,
                        input,
                        iSizeOfEntry,
                        eAgentType,
                        pTrack,
                        pSubTrack
                    ) != 0) {
                    delete pSubTrack;
                    return HK_FAILURE;
                }
                if (pSubTrack->m_sectors.m_size != 0) {
                    pSubTrack->m_sectorIndex = i;
                    pSubTrack->m_offsetInSector = static_cast<int32_t>(
                        reinterpret_cast<const uint8_t*>(pTrack) - pSector->m_data
                    );
                    trackInfoOut.m_subTracks.pushBack(pSubTrack);
                }
                else {
                    delete pSubTrack;
                }
                pEntry += iSizeOfEntry;
            }
            while (pEntry < pEnd);
        }
        trackInfoOut.m_sectors.expandOne();
        trackInfoOut.m_sectors.m_data[i] = new hkpAgent1nSector;
        *trackInfoOut.m_sectors.m_data[i] = *pSector;
    }
    return HK_SUCCESS;
}

// The collision output is built inside the loop, with no constraint owner. The added events' loop rereads the id count
// each pass; the contact point events' loop reads it once. The added event leaves m_callbackFiredFrom unwritten:
// fireContactPointAdded clears it first. The flag test is the properties' m_flags & CONTACT_IS_NEW.
hkResult hkpSaveContactPointsUtil::deserializeCollisionEntry(
    const LoadPointsInput& input,
    const hkpSerializedAgentNnEntry& serializedEntryIn,
    hkpEntity* entityA,
    hkpEntity* entityB,
    const hkpProcessCollisionInput* collisionInput,
    hkpAgentNnEntry* entry
) {
    hkpSimulationIsland* pIsland = entityA->m_simulationIsland;
    hkpSimulationIsland* pIslandB = entityB->m_simulationIsland;
    hkpSimpleConstraintContactMgr* pMgr = static_cast<hkpSimpleConstraintContactMgr*>(entry->m_contactMgr);
    if (pIsland != pIslandB) {
        if (entityA->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
            pIsland = pIslandB;
        }
        else if (entityB->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
            pIsland = hkpWorldAgentUtil::getIslandFromAgentEntry(entry, pIsland, pIslandB);
        }
    }
    if (destroyOldEntry(
            static_cast<hkpSerializedAgentNnEntry::SerializedAgentType>(serializedEntryIn.m_agentType.m_storage),
            collisionInput,
            pMgr,
            pIsland,
            entry
        )) {
        return HK_FAILURE;
    }

    hkpLinkedCollidable* pCollidableA = entry->m_collidable[0];
    if (entityA != hkpGetRigidBody(pCollidableA)) {
        entry->m_collidable[0] = entry->m_collidable[1];
        entry->m_collidable[1] = pCollidableA;
        const uint16_t usIndex = entry->m_agentIndexOnCollidable[0];
        entry->m_agentIndexOnCollidable[0] = entry->m_agentIndexOnCollidable[1];
        entry->m_agentIndexOnCollidable[1] = usIndex;
        hkpEntity* pEntity1 = pMgr->m_constraint.m_entities[1];
        hkpEntity* pEntity0 = pMgr->m_constraint.m_entities[0];
        pMgr->m_constraint.m_entities[0] = pEntity1;
        pMgr->m_constraint.m_entities[1] = pEntity0;
        if (pMgr->m_constraint.m_internal) {
            hkConstraintInternal* pInternal = pMgr->m_constraint.m_internal;
            hkpEntity* pInternal1 = pInternal->m_entities[1];
            pInternal->m_entities[1] = pInternal->m_entities[0];
            pInternal->m_entities[0] = pInternal1;
            pMgr->m_constraint.m_internal->m_whoIsMaster = pMgr->m_constraint.m_internal->m_whoIsMaster == 0;
        }
    }
    if (deserializeEntry(
            serializedEntryIn,
            static_cast<hkpSerializedAgentNnEntry::SerializedAgentType>(serializedEntryIn.m_agentType.m_storage),
            serializedEntryIn.m_trackInfo,
            collisionInput,
            entry
        )) {
        return HK_FAILURE;
    }

    hkArray<uint8_t>& kIds = pMgr->m_contactConstraintData.m_idMgrA.m_values;
    kIds.setSize(serializedEntryIn.m_cpIdMgr.m_size);
    for (int32_t i = 0; i < kIds.m_size; ++i) {
        kIds.m_data[i] = serializedEntryIn.m_cpIdMgr.m_data[i];
    }

    if (pMgr->m_contactConstraintData.m_atom->m_numContactPoints == 0) {
        hkpWorld* pWorld = hkpGetRigidBody(entry->m_collidable[0])->m_world;
        pWorld->unlockCriticalOperations();
        pWorld->m_blockExecutingPendingOperations.m_bool = true;
        hkpWorldOperationUtil::addConstraintImmediately(
            pWorld,
            &pMgr->m_constraint,
            hkpWorldOperationUtil::DO_FIRE_CALLBACKS
        );
        pWorld->lockCriticalOperations();
        pWorld->m_blockExecutingPendingOperations.m_bool = false;
    }

    int32_t iOldPoints = pMgr->m_contactConstraintData.m_atom->m_numContactPoints;
    int32_t iSizeOfSchemas = (iOldPoints >> 1) * HK_SIZE_OF_JACOBIAN_PAIR_CONTACT_SCHEMA +
        (iOldPoints & 1) * HK_SIZE_OF_JACOBIAN_SINGLE_CONTACT_SCHEMA + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA +
        HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
    int32_t iNumSolverResults = iOldPoints + 2;
    int32_t iNumSolverElemTemps = iOldPoints + 3;
    if (iOldPoints >= 2) {
        iSizeOfSchemas += HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
        iNumSolverResults++;
        iNumSolverElemTemps++;
    }
    if (pMgr->m_constraint.m_internal) {
        hkConstraintInternal* pInternal = pMgr->m_constraint.m_internal;
        hkpSimulationIsland* pOwnerIsland = pInternal->m_entities[pInternal->m_whoIsMaster]->m_simulationIsland;
        pOwnerIsland->m_constraintInfo.m_sizeOfSchemas -= iSizeOfSchemas;
        pOwnerIsland->m_constraintInfo.m_numSolverResults -= iNumSolverResults;
        pOwnerIsland->m_constraintInfo.m_numSolverElemTemps -= iNumSolverElemTemps;
        pMgr->m_constraint.m_internal->m_numSolverResults -= iNumSolverResults;
        pMgr->m_constraint.m_internal->m_numSolverElemTemps -= iNumSolverElemTemps;
        pMgr->m_constraint.m_internal->m_sizeOfSchemas -= iSizeOfSchemas;
    }

    hkpSimpleContactConstraintAtomUtil::deallocateAtom(pMgr->m_contactConstraintData.m_atom);

    const int32_t iNumPoints = serializedEntryIn.m_atom.m_numContactPoints;
    hkpSimpleContactConstraintAtom* pNewAtom = hkpSimpleContactConstraintAtomUtil::allocateAtom(
        iNumPoints,
        serializedEntryIn.m_atom.m_numUserDatasForBodyA,
        serializedEntryIn.m_atom.m_numUserDatasForBodyB,
        serializedEntryIn.m_atom.m_maxNumContactPoints
    );
    pMgr->m_contactConstraintData.m_atom = pNewAtom;
    pNewAtom->m_numContactPoints = static_cast<uint16_t>(iNumPoints);
    pMgr->m_contactConstraintData.m_atom->m_info = serializedEntryIn.m_atom.m_info;
    pMgr->m_constraint.m_internal->m_atoms = pMgr->m_contactConstraintData.m_atom;
    pMgr->m_constraint.m_internal->m_atomsSize = pMgr->m_contactConstraintData.m_atom->m_sizeOfAllAtoms;

    const int32_t iNewPoints = pMgr->m_contactConstraintData.m_atom->m_numContactPoints;
    const int32_t iNewSchemas = (iNewPoints >> 1) * HK_SIZE_OF_JACOBIAN_PAIR_CONTACT_SCHEMA +
        (iNewPoints & 1) * HK_SIZE_OF_JACOBIAN_SINGLE_CONTACT_SCHEMA + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA +
        HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
    hkpConstraintInfo kInfo;
    kInfo.m_maxSizeOfSchema = iNewSchemas +
        (HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
    kInfo.m_sizeOfSchemas = iNewSchemas;
    kInfo.m_numSolverResults = iNewPoints + 2;
    kInfo.m_numSolverElemTemps = iNewPoints + 3;
    if (iNewPoints >= 2) {
        kInfo.m_numSolverResults++;
        kInfo.m_numSolverElemTemps++;
        kInfo.m_sizeOfSchemas = iNewSchemas +
            (HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
    }
    hkConstraintInternal* pInternal = pMgr->m_constraint.m_internal;
    pInternal->m_entities[pInternal->m_whoIsMaster]->m_simulationIsland->addConstraintInfo(&pMgr->m_constraint, kInfo);

    // The atom's contact points (32 bytes each) and then the reserved points' properties follow it in memory, so
    // their addresses are untyped arithmetic.
    hkpSimpleContactConstraintAtom* pAtom = pMgr->m_contactConstraintData.m_atom;
    hkString::memCpy(
        reinterpret_cast<uint8_t*>(pAtom + 1) + pAtom->m_numReservedContactPoints * 0x20,
        serializedEntryIn.m_propertiesStream.m_data,
        pAtom->m_contactPointPropertiesStriding * iNumPoints
    );
    hkString::memCpy(
        pMgr->m_contactConstraintData.m_atom + 1,
        serializedEntryIn.m_contactPoints.m_data,
        iNumPoints * sizeof(hkContactPoint)
    );
    if (input.m_zeroUserDataInContactPointProperties) {
        pAtom = pMgr->m_contactConstraintData.m_atom;
        const int32_t iStriding = pAtom->m_contactPointPropertiesStriding;
        uint8_t* pucProperties = reinterpret_cast<uint8_t*>(pAtom + 1) + pAtom->m_numReservedContactPoints * 0x20;
        for (int32_t i = 0; i < pMgr->m_contactConstraintData.m_atom->m_numContactPoints; ++i) {
            reinterpret_cast<hkpContactPointProperties*>(pucProperties)->m_userData = 0;
            pucProperties += iStriding;
        }
    }

    hkArray<uint16_t> kPointIds;
    hkpDynamicsContactMgr* pDynamicsMgr = pMgr;
    pDynamicsMgr->getAllContactPointIds(kPointIds);

    if (input.m_fireContactPointAddedCallbacks) {
        for (int32_t i = 0; i < kPointIds.m_size; ++i) {
            const uint16_t usId = kPointIds.m_data[i];
            alignas(16) hkpProcessCollisionOutput kOutput(nullptr);
            const hkpCdBody* pBodyA = nullptr;
            const hkpCdBody* pBodyB = nullptr;
            if (input.m_passCollidablePointersInCollisionCallbacks) {
                pBodyA = &entityA->m_collidable;
                pBodyB = &entityB->m_collidable;
            }
            hkpContactPointProperties* pProperties = pDynamicsMgr->getContactPointProperties(usId);
            hkContactPoint* pContactPoint = pDynamicsMgr->getContactPoint(usId);
            // The const type needs an aggregate: it also writes m_callbackFiredFrom (null), which the exe leaves
            // unwritten and fireContactPointAdded clears before any listener reads it.
            // The native struct has constructors, so no aggregate: its constructor takes the same delay (the simple
            // constraint contact manager's, else 0).
            hkpManifoldPointAddedEvent
                kEvent(usId, pMgr, collisionInput, &kOutput, pBodyA, pBodyB, pContactPoint, nullptr, pProperties, 0.0f);
            hkpWorldCallbackUtil::fireContactPointAdded(entityA->m_world, kEvent);
            if (entityA->m_contactListeners.m_size != 0) {
                hkpEntityCallbackUtil::fireContactPointAddedInternal(entityA, kEvent);
            }
            if (entityB->m_contactListeners.m_size != 0) {
                hkpEntityCallbackUtil::fireContactPointAddedInternal(entityB, kEvent);
            }
        }
    }

    if (input.m_fireContactPointCallbacks) {
        const int32_t iNumPointIds = kPointIds.m_size;
        for (int32_t i = 0; i < iNumPointIds; ++i) {
            const uint16_t usId = kPointIds.m_data[i];
            if (pDynamicsMgr->getContactPointProperties(usId)->m_flags & hkContactPointMaterial::CONTACT_IS_NEW) {
                continue;
            }
            hkpContactPointProperties* pProperties = pDynamicsMgr->getContactPointProperties(usId);
            hkContactPoint* pContactPoint = pDynamicsMgr->getContactPoint(usId);
            hkpContactPointEvent kEvent(
                hkpCollisionEvent::SOURCE_WORLD,
                static_cast<hkpRigidBody*>(entityA),
                static_cast<hkpRigidBody*>(entityB),
                pMgr,
                hkpContactPointEvent::TYPE_MANIFOLD_FROM_SAVED_CONTACT_POINT,
                pContactPoint,
                pProperties,
                nullptr,
                nullptr,
                false,
                false,
                false,
                // The shape keys are the extended user data that follows the properties in the atom's untyped stream.
                reinterpret_cast<uint32_t*>(
                    reinterpret_cast<uint8_t*>(pProperties) + sizeof(hkpContactPointProperties)
                ),
                nullptr,
                nullptr
            );
            hkpWorldCallbackUtil::fireContactPointCallback(entityA->m_world, kEvent);
            kEvent.m_source = hkpCollisionEvent::SOURCE_A;
            if (entityA->m_contactListeners.m_size != 0) {
                hkpEntityCallbackUtil::fireContactPointCallbackInternal(entityA, kEvent);
            }
            kEvent.m_source = hkpCollisionEvent::SOURCE_B;
            if (entityB->m_contactListeners.m_size != 0) {
                hkpEntityCallbackUtil::fireContactPointCallbackInternal(entityB, kEvent);
            }
        }
    }

    hkAgentNnMachine_InvalidateTimInAgent(entry, *collisionInput);
    return HK_SUCCESS;
}

// Loads the points of every entity, through a selector that accepts all.
void hkpSaveContactPointsUtil::loadContactPoints(
    const hkpSaveContactPointsUtil::LoadPointsInput& input,
    hkpPhysicsSystemWithContacts* sys,
    hkpWorld* world
) {
    hkEntitySelectorAll selector;
    loadContactPointsInternal(input, sys, world, selector);
}

void hkpSaveContactPointsUtil::loadContactPoints(
    const hkpSaveContactPointsUtil::LoadPointsInput& input,
    hkpPhysicsSystemWithContacts* sys,
    hkpEntity** entities,
    int32_t numEntities
) {
    hkpWorld* m_world = (*entities)->m_world;
    hkEntitySelectorListed selector(entities, numEntities);
    hkpSaveContactPointsUtil::loadContactPointsInternal(input, sys, m_world, selector);
}

// KEEP: the caller destroyOldEntry passes on the entry it loaded before the call without loading it again, relying on
// this function changing nothing but its return value; clang does not guarantee it.
hkpSerializedAgentNnEntry::SerializedAgentType hkpSaveContactPointsUtil::getSerializedAgentType(
    void* (*func)(const hkpAgent3ProcessInput&, hkpAgentEntry*, void*, hkVector4*, hkpProcessCollisionOutput&)
) {
    if (func == &hkBoxBoxAgent3::process) {
        return hkpSerializedAgentNnEntry::BOX_BOX_AGENT3;
    }
    if (func == &hkCapsuleTriangleAgent3::process) {
        return hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3;
    }
    if (func == &hkPredGskAgent3::process) {
        return hkpSerializedAgentNnEntry::PRED_GSK_AGENT3;
    }
    if (func == &hkPredGskCylinderAgent3::process) {
        return hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3;
    }
    if (func == &hkConvexListAgent3::process) {
        return hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3;
    }
    if (func == &hkListAgent3::process) {
        return hkpSerializedAgentNnEntry::LIST_AGENT3;
    }
    if (func == &hkBvTreeAgent3::process) {
        return hkpSerializedAgentNnEntry::BV_TREE_AGENT3;
    }
    if (func == &hkpCollectionCollectionAgent3::process) {
        return hkpSerializedAgentNnEntry::COLLECTION_COLLECTION_AGENT3;
    }
    return func == &hkpCollectionAgent3::process ? hkpSerializedAgentNnEntry::COLLECTION_AGENT3
                                                 : hkpSerializedAgentNnEntry::INVALID_AGENT_TYPE;
}

void hkpSaveContactPointsUtil::saveContactPoints(
    const hkpSaveContactPointsUtil::SavePointsInput& input,
    const hkpEntity** entities,
    int32_t numEntities,
    hkpPhysicsSystemWithContacts* sys
) {
    hkpWorld* world = entities[0]->m_world;
    hkpSaveContactPointsCollidableMap processedCollidables(numEntities);
    for (int32_t i = 0; i < numEntities; ++i) {
        processedCollidables.insert(&entities[i]->m_collidable, 0);
        hkArray<hkpLinkedCollidable::CollisionEntry> entries;
        entities[i]->m_collidable.getCollisionEntriesSorted(entries);
        for (int32_t j = 0; j < entries.m_size; ++j) {
            const hkpLinkedCollidable::CollisionEntry& entry = entries.m_data[j];
            if (!processedCollidables.isValid(processedCollidables.findKey(entry.m_partner))) {
                hkpSerializedAgentNnEntry* pSerialized = new hkpSerializedAgentNnEntry;
                if (serializeCollisionEntry(input, entry.m_agentEntry, world->m_collisionInput, *pSerialized) ==
                    HK_SUCCESS) {
                    sys->addContact(pSerialized);
                }
                pSerialized->removeReference();
            }
        }
    }
}

// sectorsOut gets a new copy of each saved sector, then each saved sub track is rebuilt in place, at its sector and
// byte offset, as an empty track filled by a recursive call.
hkResult hkpSaveContactPointsUtil::deserialize1nTrack(
    const hkpSerializedTrack1nInfo& serializedTrack,
    const hkpProcessCollisionInput* input,
    hkArray<hkpAgent1nSector*, hkContainerHeapAllocator>& sectorsOut
) {
    sectorsOut.setSize(serializedTrack.m_sectors.m_size);
    for (int32_t i = 0; i < serializedTrack.m_sectors.m_size; ++i) {
        sectorsOut.m_data[i] = new hkpAgent1nSector;
        *sectorsOut.m_data[i] = *serializedTrack.m_sectors.m_data[i];
    }
    for (int32_t i = 0; i < serializedTrack.m_subTracks.m_size; ++i) {
        const hkpSerializedSubTrack1nInfo* pSubTrack = serializedTrack.m_subTracks.m_data[i];
        // The sector's bytes are untyped agent data: the sub track sits at its byte offset there.
        hkpAgent1nTrack* pTrack = reinterpret_cast<hkpAgent1nTrack*>(
            sectorsOut.m_data[pSubTrack->m_sectorIndex]->m_data + pSubTrack->m_offsetInSector
        );
        ::new (pTrack) hkpAgent1nTrack;
        if (deserialize1nTrack(*pSubTrack, input, pTrack->m_sectors) == HK_FAILURE) {
            return HK_FAILURE;
        }
    }
    return HK_SUCCESS;
}

void hkpSaveContactPointsUtil::loadContactPointsInternal(
    const hkpSaveContactPointsUtil::LoadPointsInput& input,
    hkpPhysicsSystemWithContacts* sys,
    hkpWorld* world,
    hkpSaveContactPointsUtil::EntitySelector& selector
) {
    hkpWorld* v4 = world;
    world->lockCriticalOperations();
    int32_t v5 = sys->m_contacts.m_size - 1;
    for (int32_t i = v5; i >= 0; --i) {
        hkpEntity* m_bodyA;
        hkpEntity* m_bodyB;
        hkpSerializedAgentNnEntry* v6 = sys->m_contacts.m_data[v5];
        if (v6->m_useEntityIds.m_bool) {
            m_bodyA = input.m_getEntityFromId(v6->m_bodyAId);
            m_bodyB = input.m_getEntityFromId(v6->m_bodyBId);
        }
        else {
            m_bodyA = v6->m_bodyA;
            m_bodyB = v6->m_bodyB;
        }
        if (m_bodyA && m_bodyB) {
            if (selector.isEntityOk(m_bodyA) || selector.isEntityOk(m_bodyB)) {
                hkpAgentNnEntry* Agent = hkAgentNnMachine_FindAgent(&m_bodyA->m_collidable, &m_bodyB->m_collidable);
                if (Agent) {
                    if (v6->endianCheckUint32() != hkpSerializedAgentNnEntry::ENDIAN_CHECK_VALUE) {
                        hkpSaveContactPointsEndianUtil::swapEndianTypeInCollisionEntry(world->m_collisionInput, *v6);
                    }
                    hkResult v10 = hkpSaveContactPointsUtil::deserializeCollisionEntry(
                        input,
                        *v6,
                        m_bodyA,
                        m_bodyB,
                        world->m_collisionInput,
                        Agent
                    );
                    if (input.m_removeSerializedAgentsWhenLoaded && v10) {
                        sys->m_contacts.m_data[i]->removeReference();
                        int32_t v11 = --sys->m_contacts.m_size;
                        if (v11 != i) {
                            sys->m_contacts.m_data[i] = sys->m_contacts.m_data[v11];
                        }
                    }
                }
            }
            v4 = world;
        }

        v5 = i - 1;
    }
    v4->unlockAndAttemptToExecutePendingOperations();
}
