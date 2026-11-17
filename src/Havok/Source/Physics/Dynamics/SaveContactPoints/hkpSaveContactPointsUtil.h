#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"

struct hkpAgent1nSector;
struct hkpAgent1nTrack;
class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpAgentNnEntry;
class hkpConstraintOwner;
class hkpDynamicsContactMgr;
class hkpEntity;
class hkpPhysicsSystemWithContacts;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpWorld;

// Havok's static helpers that save a world's contact points and load them back (saveContactPoints, loadContactPoints
// and the private serialize/deserialize helpers for collision entries, agent entries and 1n tracks). No instances and
// no members.
class hkpSaveContactPointsUtil {
public:
    // The id saveContactPoints records for an entity when SavePointsInput asks for ids.
    using GetIdForEntityFunc = hkUlong (*)(const hkpEntity* entity);

    // How saveContactPoints names the entities: by id through m_getIdForEntity, else by pointer.
    struct SavePointsInput {
        bool m_useEntityIds;
        GetIdForEntityFunc m_getIdForEntity;
    };

    // Chooses the entities whose contact points are saved or loaded. The hkBool32 result of isEntityOk is spelled
    // int32_t.
    class EntitySelector {
    public:
        virtual ~EntitySelector();

        virtual int32_t isEntityOk(const hkpEntity* entity) = 0;

        // DeletingDestruct's free (ourFree).
        static void operator delete(void* apMemory);
    };

    // The entity loadContactPoints finds for a saved id.
    using GetEntityFromIdFunc = hkpEntity* (*)(hkUlong id);

    // How loadContactPoints restores the points: whether it removes the saved entries, clears the properties' user
    // data, finds the entities by id and which callbacks it fires.
    struct LoadPointsInput {
        bool m_removeSerializedAgentsWhenLoaded;
        bool m_zeroUserDataInContactPointProperties;
        GetEntityFromIdFunc m_getEntityFromId;
        bool m_fireContactPointAddedCallbacks;
        bool m_fireContactPointCallbacks;
        bool m_passCollidablePointersInCollisionCallbacks;
    };

    hkpSaveContactPointsUtil() = delete;

    // The saveContactPoints overload that saves every agent entry of the world's active, then inactive, simulation
    // islands into sys.
    static void
    saveContactPoints(const SavePointsInput& input, const hkpWorld* world, hkpPhysicsSystemWithContacts* sys);
    // The overload that saves the numEntries agent entries at entries into sys: each one serialized
    // (serializeCollisionEntry) into a new hkpSerializedAgentNnEntry, added to sys when that succeeds.
    static void saveContactPoints(
        const SavePointsInput& input,
        const hkpAgentNnEntry** entries,
        int32_t numEntries,
        hkpPhysicsSystemWithContacts* sys
    );

protected:
    // Saves one agent entry into serializedEntryOut; returns 0 (HK_SUCCESS) on success.
    static hkResult serializeCollisionEntry(
        const SavePointsInput& input,
        const hkpAgentNnEntry* entry,
        const hkpProcessCollisionInput* collisionInput,
        hkpSerializedAgentNnEntry& serializedEntryOut
    );
    // Saves one agent entry (an Nn entry when isNnEntry is set): its size, its serialized agent type and, for agents
    // with a 1n track, the track and its sectors in trackInfoOut; returns 0 on success. trackInfoOut is a pointer
    // because serialize1nTrack passes its unchecked new result.
    static hkResult serializeEntry(
        const hkpAgentEntry* entry,
        hkBool isNnEntry,
        const hkpProcessCollisionInput* input,
        int32_t& sizeOfThisEntryOut,
        hkpSerializedAgentNnEntry::SerializedAgentType& agentTypeOut,
        const hkpAgent1nTrack*& agent1nTrackOut,
        hkpSerializedTrack1nInfo* trackInfoOut
    );
    // Destroys the agent the entry holds before a saved one replaces it; returns an hkResult
    // spelled int32_t, 0 on success.
    static hkResult destroyOldEntry(
        const hkpSerializedAgentNnEntry::SerializedAgentType agentType,
        const hkpProcessCollisionInput* input,
        hkpDynamicsContactMgr* mgr,
        hkpConstraintOwner* constraintOwner,
        hkpAgentNnEntry* nnEntry
    );
    // Rebuilds the entry's agent from the saved entry and its 1n track; returns an hkResult
    // spelled int32_t, 0 on success.
    static hkResult deserializeEntry(
        const hkpSerializedAgentNnEntry& serializedEntryIn,
        const hkpSerializedAgentNnEntry::SerializedAgentType agentType,
        const hkpSerializedTrack1nInfo& serializedTrack,
        const hkpProcessCollisionInput* input,
        hkpAgentNnEntry* nnEntry
    );
    // Copies each sector of sectorsIn into trackInfoOut and saves a sub track for every entry with a nested 1n
    // track; returns an hkResult spelled int32_t, 0 on success.
    static hkResult serialize1nTrack(
        const hkArray<hkpAgent1nSector*>& sectorsIn,
        const hkpProcessCollisionInput* input,
        hkpSerializedTrack1nInfo& trackInfoOut
    );

public:
    // Restores a saved entry into entry, the agent of entityA and entityB: the agent, the contact constraint's atom,
    // its points and properties, then the callbacks input asks for; returns an hkResult spelled int32_t, 0 on
    // success.
    static hkResult deserializeCollisionEntry(
        const LoadPointsInput& input,
        const hkpSerializedAgentNnEntry& serializedEntryIn,
        hkpEntity* entityA,
        hkpEntity* entityB,
        const hkpProcessCollisionInput* collisionInput,
        hkpAgentNnEntry* entry
    );
    static void loadContactPoints(
        const hkpSaveContactPointsUtil::LoadPointsInput& input,
        hkpPhysicsSystemWithContacts* sys,
        hkpWorld* world
    );
    static void loadContactPoints(
        const hkpSaveContactPointsUtil::LoadPointsInput& input,
        hkpPhysicsSystemWithContacts* sys,
        hkpEntity** entities,
        int32_t numEntities
    );
    // The serialized type of the agent whose process function func is, or INVALID_AGENT_TYPE.
    static hkpSerializedAgentNnEntry::SerializedAgentType getSerializedAgentType(
        void* (*func)(const hkpAgent3ProcessInput&, hkpAgentEntry*, void*, hkVector4*, hkpProcessCollisionOutput&)
    );
    static void saveContactPoints_00DCDC00(
        const hkpSaveContactPointsUtil::SavePointsInput& input,
        const hkpEntity** entities,
        int32_t numEntities,
        hkpPhysicsSystemWithContacts* sys
    );
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    static hkResult deserialize1nTrack(
        const hkpSerializedTrack1nInfo& serializedTrack,
        const hkpProcessCollisionInput* input,
        hkArray<hkpAgent1nSector*, hkContainerHeapAllocator>& sectorsOut
    );
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    static void loadContactPointsInternal(
        const hkpSaveContactPointsUtil::LoadPointsInput& input,
        hkpPhysicsSystemWithContacts* sys,
        hkpWorld* world,
        hkpSaveContactPointsUtil::EntitySelector& selector
    );
};
