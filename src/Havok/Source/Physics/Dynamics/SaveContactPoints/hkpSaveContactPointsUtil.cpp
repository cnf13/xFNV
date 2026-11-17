#include "Havok/Source/Physics/Dynamics/SaveContactPoints/hkpSaveContactPointsUtil.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskCylinderAgent3/hkpPredGskCylinderAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpPhysicsSystemWithContacts.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSaveContactPointsEndianUtil.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/BoxBox/hkpBoxBoxAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Atom/hkpConstraintAtomUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/CapsuleTriangle/hkpCapsuleTriangleAgent3.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Collection3/hkpCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/ConvexList3/hkpConvexListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "SDK/MSVC/string.h"

using hkpSaveContactPointsUtil_saveWorld_t = void (*)(const hkpSaveContactPointsUtil::SavePointsInput&, const hkpWorld*,
	hkpPhysicsSystemWithContacts*);

namespace {

// The selector loadContactPoints builds over its entity list: the PDB's `anonymous namespace'::hkEntitySelectorListed
// (hkpSaveContactPointsUtil.cpp), with the PDB's m_map at +0x4. The exe's vtable 0x10D77CC is the deleting destructor
// (slot 0, 0xDCE8F0) and isEntityOk (slot 1, 0xDCE8D0).
class hkEntitySelectorListed : public hkpSaveContactPointsUtil::EntitySelector {
public:
	// The hybrid's implicit default constructor: the empty map.
	hkEntitySelectorListed() = default;
	// The constructor Construct stands for.
	hkEntitySelectorListed(hkpEntity** entities, int32_t numEntities);
	~hkEntitySelectorListed() override;

	// Not reconstructed yet. Whether entity is one of the listed ones: m_map's value for it, 0 when absent
	// (getWithDefault at 0xDCE8DA; thiscall, ret 4, 0xDCE8D0).
	int32_t isEntityOk(const hkpEntity* entity) override;

	// +0x4: the listed entities, each mapped to 1.
	hkPointerMap<const hkpEntity*, int32_t> m_map;
};

inline constexpr fnvd::Vtable hkEntitySelectorListed_vftable{};
inline constexpr fnvd::Vtable hkEntitySelectorAll_vftable{};

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

// The map frees its own slots (the exe's deleting destructor, 0xDCE8F0, inlines that).
hkEntitySelectorListed::~hkEntitySelectorListed() {}

// The selector of every entity: the PDB's `anonymous namespace'::hkEntitySelectorAll (hkpSaveContactPointsUtil.cpp).
// The exe's vtable 0x10D77C0 holds EntitySelector's deleting destructor (slot 0, 0xDCD4B0) and isEntityOk (slot 1);
// the exe inlines its constructor (the vtable store, 0xDCE7C4) and has no out-of-line copy of it.
class hkEntitySelectorAll : public hkpSaveContactPointsUtil::EntitySelector {
public:
	// Every entity is ok: 1.
	int32_t isEntityOk(const hkpEntity* entity) override;
};

int32_t hkEntitySelectorAll::isEntityOk(const hkpEntity* entity) {
	return 1;
}

} // namespace

// Havok SDK: saves the agent entries of the numEntities entities into sys, each pair once: an entity's collidable
// goes into a map before its entries are saved, and an entry whose partner is in the map already is skipped. The
// exe inlines the map's and each entry array's constructors and destructors.
using hkpSaveContactPointsCollidableMap = hkPointerMap<const hkpLinkedCollidable*, int32_t>;
using hkpSaveContactPointsCollidableMapBase = hkUlongMapBase;

hkpSaveContactPointsUtil::EntitySelector::~EntitySelector() {}

void hkpSaveContactPointsUtil::EntitySelector::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The islands are gathered into one array, active first; every agent entry of their agent tracks is saved into a
// new hkpSerializedAgentNnEntry, which sys keeps when the save succeeds. A sector holds 512 bytes of entries, the
// last one m_bytesUsedInLastSector.
void hkpSaveContactPointsUtil::saveContactPoints(const SavePointsInput& input, const hkpWorld* world,
	hkpPhysicsSystemWithContacts* sys) {
	FNVD_INLINED_LOCAL(hkArray<hkpSimulationIsland*>, islands, std::nullopt, std::nullopt);
	islands.m_data = nullptr;
	islands.m_size = 0;
	islands.m_capacityAndFlags = 0x80000000u;
	// EXE-INLINED none: hkArray<hkpSimulationIsland*>::operator=
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity =
		static_cast<int32_t>(islands.m_capacityAndFlags & hkArrayBase<hkpSimulationIsland*>::CAPACITY_MASK);
	if (iCapacity < world->m_activeSimulationIslands.m_size) {
		if (islands.m_capacityAndFlags >= 0) {
			pHeap->bufFree(islands.m_data, sizeof(hkpSimulationIsland*) * iCapacity);
		}
		int32_t iNumBytes = sizeof(hkpSimulationIsland*) * world->m_activeSimulationIslands.m_size;
		islands.m_data = static_cast<hkpSimulationIsland**>(pHeap->bufAlloc(iNumBytes));
		islands.m_capacityAndFlags = static_cast<uint32_t>(iNumBytes / 4);
	}
	islands.m_size = world->m_activeSimulationIslands.m_size;
	for (int32_t i = 0; i < islands.m_size; ++i) {
		islands.m_data[i] = world->m_activeSimulationIslands.m_data[i];
	}
	// EXE-INLINED none: hkArray<hkpSimulationIsland*>::insertAt
	islands._insertAt(*hkMemoryRouter::getInstance().m_heap, islands.m_size, world->m_inactiveSimulationIslands.m_data,
		world->m_inactiveSimulationIslands.m_size);
	for (int32_t i = 0; i < islands.m_size; ++i) {
		hkpSimulationIsland* pIsland = islands.m_data[i];
		for (int32_t iSector = 0; iSector < pIsland->m_agentTrack.m_sectors.m_size;) {
			hkpAgentNnTrack& track = islands.m_data[i]->m_agentTrack;
			uint8_t* pEntry = track.m_sectors.m_data[iSector]->m_data;
			++iSector;
			uint8_t* pEnd;
			if (iSector == track.m_sectors.m_size) {
				pEnd = pEntry + pIsland->m_agentTrack.m_bytesUsedInLastSector;
			} else {
				pEnd = pEntry + sizeof(hkpAgentNnSector);
			}
			// A sector's bytes are an untyped stream of variable-sized entries, each giving its size in m_size: the
			// exe walks them as raw memory, so src reads each entry through a cast of the byte pointer.
			for (; pEntry < pEnd; pEntry += reinterpret_cast<hkpAgentNnEntry*>(pEntry)->m_size) {
				// EXE-INLINED none: hkReferencedObject::operator new
				hkMemoryRouter& router = hkMemoryRouter::getInstance();
				void* pMemory = router.m_heap->blockAlloc(static_cast<int32_t>(sizeof(hkpSerializedAgentNnEntry)));
				static_cast<hkpSerializedAgentNnEntry*>(pMemory)->m_memSizeAndFlags =
					static_cast<uint16_t>(sizeof(hkpSerializedAgentNnEntry));
				hkpSerializedAgentNnEntry* pSerialized = ::new (pMemory) hkpSerializedAgentNnEntry;
				if (serializeCollisionEntry(input, reinterpret_cast<hkpAgentNnEntry*>(pEntry), world->m_collisionInput,
						*pSerialized) == 0) {
					sys->addContact(pSerialized);
				}
				pSerialized->removeReference();
			}
			pIsland = islands.m_data[i];
		}
	}
	// EXE-INLINED none: hkArray<hkpSimulationIsland*>::~hkArray
	pHeap = hkMemoryRouter::getInstance().m_heap;
	islands.m_size = 0;
	if (islands.m_capacityAndFlags >= 0) {
		pHeap->bufFree(islands.m_data,
			sizeof(hkpSimulationIsland*) *
				static_cast<int32_t>(islands.m_capacityAndFlags & hkArrayBase<hkpSimulationIsland*>::CAPACITY_MASK));
	}
}

// Havok SDK: saves the numEntries agent entries at entries into sys, each serialized into a new
// hkpSerializedAgentNnEntry with the collision input of the world of the first entry's first entity.
void hkpSaveContactPointsUtil::saveContactPoints(const hkpSaveContactPointsUtil::SavePointsInput& input,
	const hkpAgentNnEntry** entries, int32_t numEntries, hkpPhysicsSystemWithContacts* sys) {
	// EXE-INLINED 0x4B59F0
	const hkpLinkedCollidable* collidable = entries[0]->m_collidable[0];
	hkpEntity* entity = collidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
		? static_cast<hkpEntity*>(collidable->getOwner())
		: nullptr;
	hkpWorld* world = entity->m_world;
	for (int32_t i = 0; i < numEntries; ++i) {
		// EXE-INLINED none: hkpSerializedAgentNnEntry::operator new
		void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSerializedAgentNnEntry));
		static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpSerializedAgentNnEntry);
		hkpSerializedAgentNnEntry* pSerialized = ::new (pMemory) hkpSerializedAgentNnEntry;
		if (serializeCollisionEntry(input, entries[i], world->m_collisionInput, *pSerialized) == HK_SUCCESS) {
			sys->addContact(pSerialized);
		}
		pSerialized->removeReference();
	}
}

// KEEP (not registered): the exe builds serializeEntry's isNnEntry argument by writing 1 into the low byte of the
// stack slot that later receives agent1nTrack and pushes the whole dword, so its upper three bytes are stack garbage
// nothing wrote (0xDCD76A..0xDCD774). The callee reads only the low byte (0xDCD583, 0xDCD594).
hkResult hkpSaveContactPointsUtil::serializeCollisionEntry(const hkpSaveContactPointsUtil::SavePointsInput& input,
	const hkpAgentNnEntry* entry, const hkpProcessCollisionInput* collisionInput,
	hkpSerializedAgentNnEntry& serializedEntryOut) {
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
	if (serializeEntry(entry, hkBool(true), collisionInput, sizeOfThisEntry, agentType, agent1nTrack,
			&serializedEntryOut.m_trackInfo)) {
		return HK_FAILURE;
	}
	// EXE-INLINED none: hkEnum<hkpSerializedAgentNnEntry::SerializedAgentType, int8_t>::operator=
	serializedEntryOut.m_agentType.m_storage = static_cast<int8_t>(agentType);
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
	hkString::memCpy(serializedEntryOut.m_propertiesStream.m_data, atom->getContactPointPropertiesStream(0),
		atom->m_numContactPoints * atom->getContactPointPropertiesStriding());

	// EXE-INLINED none: hkArray<hkContactPoint>::setSize (an hkContactPoint's construction is empty)
	hkArray<hkContactPoint>& contactPoints = serializedEntryOut.m_contactPoints;
	const int32_t numContactPoints = atom->m_numContactPoints;
	contactPoints._reserve(hkContainerHeapAllocator().get(&contactPoints), numContactPoints);
	contactPoints.m_size = numContactPoints;
	hkString::memCpy(contactPoints.m_data, atom->getContactPoints(),
		atom->m_numContactPoints * static_cast<int32_t>(sizeof(hkContactPoint)));

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
	} else {
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
hkResult hkpSaveContactPointsUtil::serializeEntry(const hkpAgentEntry* entry, hkBool isNnEntry,
	const hkpProcessCollisionInput* input, int32_t& sizeOfThisEntryOut,
	hkpSerializedAgentNnEntry::SerializedAgentType& agentTypeOut, const hkpAgent1nTrack*& agent1nTrackOut,
	hkpSerializedTrack1nInfo* trackInfoOut) {
	agent1nTrackOut = nullptr;
	// The stream is untyped memory: the agent data sits a header's size in bytes after the entry.
	const uint8_t* entryBytes = static_cast<const uint8_t*>(static_cast<const void*>(entry));
	const hkpAgentData* agentData;
	switch (entry->m_streamCommand) {
	case hkAgent3::STREAM_NULL:
	case hkAgent3::STREAM_END:
		sizeOfThisEntryOut = 16;
		return HK_SUCCESS;
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
		agentData =
			isNnEntry ? entryBytes + sizeof(hkpAgentNnMachineTimEntry) : entryBytes + sizeof(hkpAgent1nMachineTimEntry);
		break;
	case hkAgent3::STREAM_CALL_AGENT:
	case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
		return HK_FAILURE;
	default:
		return HK_SUCCESS;
	}

	const hkpSerializedAgentNnEntry::SerializedAgentType agentType =
		getSerializedAgentType(input->m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc);
	agentTypeOut = agentType;
	switch (agentType) {
	case hkpSerializedAgentNnEntry::BOX_BOX_AGENT3:
	case hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3:
	case hkpSerializedAgentNnEntry::PRED_GSK_AGENT3:
	case hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3:
		sizeOfThisEntryOut = entry->m_size;
		return HK_SUCCESS;
	case hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3: {
		sizeOfThisEntryOut = entry->m_size;
		// A convex list agent in GSK mode has no track of sub-agents; in stream mode its data, after the
		// hkpGskCache, holds one (hkConvexListAgent3::getAgent1nTrack, the exe's trivial accessor 0xD32AD0).
		const hkpGskCache* gskCache = static_cast<const hkpGskCache*>(agentData);
		if (gskCache->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
			return HK_SUCCESS;
		}
		const hkConvexListAgent3Data* listData =
			static_cast<const hkConvexListAgent3Data*>(static_cast<const void*>(gskCache + 1));
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
	default:
		return HK_FAILURE;
	}
}

// Havok SDK: destroys the entry's agent through the dispatcher's destroy function when the entry is an agent call
// of the saved type; returns 1 (HK_FAILURE) otherwise.
hkResult hkpSaveContactPointsUtil::destroyOldEntry(const hkpSerializedAgentNnEntry::SerializedAgentType agentType,
	const hkpProcessCollisionInput* input, hkpDynamicsContactMgr* mgr, hkpConstraintOwner* constraintOwner,
	hkpAgentNnEntry* nnEntry) {
	// The stream is untyped memory: the agent data sits a header's size in bytes after the entry.
	uint8_t* entryBytes = reinterpret_cast<uint8_t*>(nnEntry);
	hkpAgentData* agentData;
	if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL) {
		agentData = entryBytes + sizeof(hkpAgentNnMachinePaddedEntry);
	} else if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL_WITH_TIM) {
		agentData = entryBytes + sizeof(hkpAgentNnMachineTimEntry);
	} else {
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
hkResult hkpSaveContactPointsUtil::deserializeEntry(const hkpSerializedAgentNnEntry& serializedEntryIn,
	const hkpSerializedAgentNnEntry::SerializedAgentType agentType, const hkpSerializedTrack1nInfo& serializedTrack,
	const hkpProcessCollisionInput* input, hkpAgentNnEntry* nnEntry) {
	// The stream is untyped memory: the agent data sits a header's size in bytes after the entry.
	uint8_t* entryBytes = reinterpret_cast<uint8_t*>(nnEntry);
	hkpAgentData* agentData;
	if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL) {
		agentData = entryBytes + sizeof(hkpAgentNnMachinePaddedEntry);
	} else if (nnEntry->m_streamCommand == hkAgent3::STREAM_CALL_WITH_TIM) {
		agentData = entryBytes + sizeof(hkpAgentNnMachineTimEntry);
	} else {
		return HK_FAILURE;
	}
	if (getSerializedAgentType(input->m_dispatcher->m_agent3Func[nnEntry->m_agentType].m_processFunc) != agentType) {
		return HK_FAILURE;
	}
	// The saved entry's bytes: an Nn entry's header, then its agent data (HK_AGENT3_AGENT_SIZE bytes in all: 128 for
	// 4-byte pointers).
	const hkpAgentNnEntry* savedEntry = reinterpret_cast<const hkpAgentNnEntry*>(serializedEntryIn.m_nnEntryData);
	const int32_t iAgentBytes = HK_AGENT3_AGENT_SIZE - static_cast<int32_t>(sizeof(hkpAgentNnEntry));
	hkpAgent1nTrack* track;
	switch (agentType) {
	case hkpSerializedAgentNnEntry::BOX_BOX_AGENT3:
	case hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3:
	case hkpSerializedAgentNnEntry::PRED_GSK_AGENT3:
	case hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3:
		hkString::memCpy(entryBytes + sizeof(hkpAgentNnEntry),
			serializedEntryIn.m_nnEntryData + sizeof(hkpAgentNnEntry), iAgentBytes);
		nnEntry->m_numContactPoints = savedEntry->m_numContactPoints;
		nnEntry->m_size = savedEntry->m_size;
		return HK_SUCCESS;
	case hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3: {
		hkString::memCpy(entryBytes + sizeof(hkpAgentNnEntry),
			serializedEntryIn.m_nnEntryData + sizeof(hkpAgentNnEntry), iAgentBytes);
		nnEntry->m_numContactPoints = savedEntry->m_numContactPoints;
		nnEntry->m_size = savedEntry->m_size;
		// A convex list agent in GSK mode has no track of sub-agents; in stream mode its data, after the
		// hkpGskCache, holds one (hkConvexListAgent3::getAgent1nTrack, the exe's trivial accessor 0xD32AC0).
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
		hkString::memCpy(entryBytes + sizeof(hkpAgentNnEntry),
			serializedEntryIn.m_nnEntryData + sizeof(hkpAgentNnEntry), iAgentBytes);
		// These agents' data is their track of sub-agents.
		track = static_cast<hkpAgent1nTrack*>(agentData);
		break;
	default:
		return HK_FAILURE;
	}
	if (track) {
		// EXE-INLINED none: hkpAgent1nTrack::hkpAgent1nTrack
		track->m_sectors.m_data = nullptr;
		track->m_sectors.m_size = 0;
		track->m_sectors.m_capacityAndFlags = hkArrayBase<hkpAgent1nSector*>::DONT_DEALLOCATE_FLAG;
	}
	return deserialize1nTrack(serializedTrack, input, track->m_sectors);
}

// Havok SDK: trackInfoOut's sector array is reserved for sectorsIn; each sector's entries are serialized (as 1n
// entries) into a new sub track, kept with its sector index and byte offset when it saved any sectors, else
// destroyed; then a copy of the sector is appended. A failed entry destroys its sub track and returns 1 (HK_FAILURE).
// The exe inlines the array reserve and pushes, the sub track and sector constructors and the sector copy (rep movsd).
// KEEP (not registered): the exe passes serializeEntry's isNnEntry as the whole dword at [esp+0x1C], of which only
// the low byte is written (0xDCDFE4); its upper three bytes are stack garbage nothing wrote (pushed at 0xDCE04F).
hkResult hkpSaveContactPointsUtil::serialize1nTrack(const hkArray<hkpAgent1nSector*>& sectorsIn,
	const hkpProcessCollisionInput* input, hkpSerializedTrack1nInfo& trackInfoOut) {
	// EXE-INLINED none: hkArray<hkpAgent1nSector*>::reserve
	int32_t iNumSectors = sectorsIn.m_size;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity =
		static_cast<int32_t>(trackInfoOut.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgent1nSector*>::CAPACITY_MASK);
	if (iCapacity < iNumSectors) {
		int32_t iDoubled = 2 * iCapacity;
		hkArrayUtil::_reserve(pHeap, &trackInfoOut.m_sectors, iNumSectors < iDoubled ? iDoubled : iNumSectors,
			sizeof(hkpAgent1nSector*));
	}
	for (int32_t i = 0; i < sectorsIn.m_size; ++i) {
		hkpAgent1nSector* pSector = sectorsIn.m_data[i];
		// A sector's bytes are an untyped stream of variable-sized entries: the exe walks them as raw memory.
		const uint8_t* pEntry = pSector->m_data;
		const uint8_t* pEnd = pSector->m_data + pSector->m_bytesAllocated;
		if (pEntry < pEnd) {
			hkBool isNnEntry;
			isNnEntry.m_bool = 0;
			do {
				const hkpAgent1nTrack* pTrack = nullptr;
				int32_t iSizeOfEntry = 0;
				// EXE-INLINED none: hkpSerializedSubTrack1nInfo::operator new
				void* pMemory = hkMemoryRouter::getInstance()
									.m_heap->blockAlloc(static_cast<int32_t>(sizeof(hkpSerializedSubTrack1nInfo)));
				hkpSerializedSubTrack1nInfo* pSubTrack;
				if (pMemory) {
					// EXE-INLINED none: hkpSerializedSubTrack1nInfo::hkpSerializedSubTrack1nInfo
					pSubTrack = static_cast<hkpSerializedSubTrack1nInfo*>(pMemory);
					pSubTrack->m_sectors.m_data = nullptr;
					pSubTrack->m_sectors.m_size = 0;
					pSubTrack->m_sectors.m_capacityAndFlags = 0x80000000u;
					pSubTrack->m_subTracks.m_data = nullptr;
					pSubTrack->m_subTracks.m_size = 0;
					pSubTrack->m_subTracks.m_capacityAndFlags = 0x80000000u;
					pSubTrack->m_sectorIndex = -1;
				} else {
					pSubTrack = nullptr;
				}
				hkpSerializedAgentNnEntry::SerializedAgentType eAgentType;
				if (serializeEntry(reinterpret_cast<const hkpAgentEntry*>(pEntry), isNnEntry, input, iSizeOfEntry,
						eAgentType, pTrack, pSubTrack) != 0) {
					if (pSubTrack) {
						FNVD_DESTRUCT(hkpSerializedTrack1nInfo)(pSubTrack);
						hkMemoryRouter::getInstance().m_heap->blockFree(pSubTrack,
							static_cast<int32_t>(sizeof(hkpSerializedSubTrack1nInfo)));
					}
					return HK_FAILURE;
				}
				if (pSubTrack->m_sectors.m_size != 0) {
					pSubTrack->m_sectorIndex = i;
					pSubTrack->m_offsetInSector =
						static_cast<int32_t>(reinterpret_cast<const uint8_t*>(pTrack) - pSector->m_data);
					// EXE-INLINED none: hkArray<hkpSerializedSubTrack1nInfo*>::pushBack
					hkArray<hkpSerializedSubTrack1nInfo*>& subTracks = trackInfoOut.m_subTracks;
					hkMemoryRouter& router = hkMemoryRouter::getInstance();
					if (subTracks.m_size ==
						(subTracks.m_capacityAndFlags & hkArrayBase<hkpSerializedSubTrack1nInfo*>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*router.m_heap, &subTracks, sizeof(hkpSerializedSubTrack1nInfo*));
					}
					subTracks.m_data[subTracks.m_size] = pSubTrack;
					++subTracks.m_size;
				} else {
					FNVD_DESTRUCT(hkpSerializedTrack1nInfo)(pSubTrack);
					hkMemoryRouter::getInstance().m_heap->blockFree(pSubTrack,
						static_cast<int32_t>(sizeof(hkpSerializedSubTrack1nInfo)));
				}
				pEntry += iSizeOfEntry;
			} while (pEntry < pEnd);
		}
		// EXE-INLINED none: hkArray<hkpAgent1nSector*>::expandOne
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (trackInfoOut.m_sectors.m_size ==
			(trackInfoOut.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgent1nSector*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &trackInfoOut.m_sectors, sizeof(hkpAgent1nSector*));
		}
		++trackInfoOut.m_sectors.m_size;
		// EXE-INLINED none: hkpAgent1nSector::operator new
		void* pSectorMemory =
			hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(hkpAgent1nSector)));
		hkpAgent1nSector* pCopy;
		if (pSectorMemory) {
			// EXE-INLINED none: hkpAgent1nSector::hkpAgent1nSector
			pCopy = static_cast<hkpAgent1nSector*>(pSectorMemory);
			pCopy->m_bytesAllocated = 0;
		} else {
			pCopy = nullptr;
		}
		trackInfoOut.m_sectors.m_data[i] = pCopy;
		*trackInfoOut.m_sectors.m_data[i] = *pSector;
	}
	return HK_SUCCESS;
}

// The exe inlines hkpGetRigidBody (the broad-phase type test and hkpCollidable::getOwner), the id manager's copy
// (hkArray::setSize: the capacity compared signed, grown to the larger of the size and twice the capacity),
// hkpConstraintOwner::subConstraintInfo, the atom's deallocation (blockFree, vtable slot 2, of the size 0xD92440
// rounds), the constraint info of the new atom's points and both events' constructors. Its constraint info for n
// points is (n >> 1) * 0x70 + (n & 1) * 0x30 + 0x80 bytes of schema (0x20 more for the maximum, and for the schemas
// when n >= 2; 0x90 and 0x30 for 8-byte pointers: hkpSolve.h's header and friction schemas), n + 2 solver results
// and n + 3 element temps (one more of each when n >= 2). The collision output
// is built inside the loop: only its constraint owner (null), m_toi.m_time (FLT_MAX, the float at 0x104DA48) and
// the TOI properties' solver results (fldz) are written. The added events' loop rereads the id count each pass; the
// contact point events' loop reads it once. The exe's added event leaves m_callbackFiredFrom unwritten:
// fireContactPointAdded clears it first. The flag test is properties byte 7 (m_flags) & CONTACT_IS_NEW.
hkResult hkpSaveContactPointsUtil::deserializeCollisionEntry(const LoadPointsInput& input,
	const hkpSerializedAgentNnEntry& serializedEntryIn, hkpEntity* entityA, hkpEntity* entityB,
	const hkpProcessCollisionInput* collisionInput, hkpAgentNnEntry* entry) {
	hkpSimulationIsland* pIsland = entityA->m_simulationIsland;
	hkpSimulationIsland* pIslandB = entityB->m_simulationIsland;
	hkpSimpleConstraintContactMgr* pMgr = static_cast<hkpSimpleConstraintContactMgr*>(entry->m_contactMgr);
	if (pIsland != pIslandB) {
		if (entityA->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
			pIsland = pIslandB;
		} else if (entityB->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
			pIsland = hkpWorldAgentUtil::getIslandFromAgentEntry(entry, pIsland, pIslandB);
		}
	}
	if (destroyOldEntry(static_cast<hkpSerializedAgentNnEntry::SerializedAgentType>(serializedEntryIn.m_agentType
								.m_storage),
			collisionInput, pMgr, pIsland, entry)) {
		return HK_FAILURE;
	}

	// EXE-INLINED none: hkpGetRigidBody
	hkpLinkedCollidable* pCollidableA = entry->m_collidable[0];
	hkpWorldObject* pOwnerA = nullptr;
	if (pCollidableA->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
		// EXE-INLINED 0x4B59D0
		// The owner is the collidable moved by its byte offset: untyped arithmetic, as in the exe.
		pOwnerA =
			reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(pCollidableA) + pCollidableA->m_ownerOffset);
	}
	if (static_cast<hkpWorldObject*>(entityA) != pOwnerA) {
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
	if (deserializeEntry(serializedEntryIn,
			static_cast<hkpSerializedAgentNnEntry::SerializedAgentType>(serializedEntryIn.m_agentType.m_storage),
			serializedEntryIn.m_trackInfo, collisionInput, entry)) {
		return HK_FAILURE;
	}

	// EXE-INLINED none: hkArray<uint8_t>::operator=
	hkArray<uint8_t>& kIds = pMgr->m_contactConstraintData.m_idMgrA.m_values;
	int32_t iNumIds = serializedEntryIn.m_cpIdMgr.m_size;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t iCapacity = kIds.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
	if (iCapacity < iNumIds) {
		int32_t iNewCapacity = iNumIds;
		if (iNumIds < iCapacity * 2) {
			iNewCapacity = iCapacity * 2;
		}
		hkArrayUtil::_reserve(pHeap, &kIds, iNewCapacity, sizeof(uint8_t));
	}
	kIds.m_size = iNumIds;
	for (int32_t i = 0; i < kIds.m_size; ++i) {
		kIds.m_data[i] = serializedEntryIn.m_cpIdMgr.m_data[i];
	}

	if (pMgr->m_contactConstraintData.m_atom->m_numContactPoints == 0) {
		// EXE-INLINED none: hkpGetRigidBody
		hkpLinkedCollidable* pCollidable = entry->m_collidable[0];
		hkpWorldObject* pOwner = nullptr;
		if (pCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
			// EXE-INLINED 0x4B59D0
			// The owner is the collidable moved by its byte offset: untyped arithmetic, as in the exe.
			pOwner =
				reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(pCollidable) + pCollidable->m_ownerOffset);
		}
		hkpWorld* pWorld = pOwner->m_world;
		pWorld->m_criticalOperationsLockCount--;
		pWorld->m_blockExecutingPendingOperations.m_bool = true;
		hkpWorldOperationUtil::addConstraintImmediately(pWorld, &pMgr->m_constraint,
			hkpWorldOperationUtil::DO_FIRE_CALLBACKS);
		pWorld->m_criticalOperationsLockCount++;
		pWorld->m_blockExecutingPendingOperations.m_bool = false;
	}

	// EXE-INLINED none: hkpConstraintOwner::subConstraintInfo
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

	// EXE-INLINED none: hkpSimpleContactConstraintAtomUtil::deallocateAtom
	hkpSimpleContactConstraintAtom* pOldAtom = pMgr->m_contactConstraintData.m_atom;
	const int32_t iAllocatedSize = hkpSimpleContactConstraintAtomUtil::getAllocatedAtomSize(pOldAtom->m_sizeOfAllAtoms);
	hkMemoryRouter::getInstance().m_heap->blockFree(pOldAtom, iAllocatedSize);

	const int32_t iNumPoints = serializedEntryIn.m_atom.m_numContactPoints;
	hkpSimpleContactConstraintAtom* pNewAtom =
		hkpSimpleContactConstraintAtomUtil::allocateAtom(iNumPoints, serializedEntryIn.m_atom.m_numUserDatasForBodyA,
			serializedEntryIn.m_atom.m_numUserDatasForBodyB, serializedEntryIn.m_atom.m_maxNumContactPoints);
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
	kInfo.m_maxSizeOfSchema =
		iNewSchemas + (HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
	kInfo.m_sizeOfSchemas = iNewSchemas;
	kInfo.m_numSolverResults = iNewPoints + 2;
	kInfo.m_numSolverElemTemps = iNewPoints + 3;
	if (iNewPoints >= 2) {
		kInfo.m_numSolverResults++;
		kInfo.m_numSolverElemTemps++;
		kInfo.m_sizeOfSchemas =
			iNewSchemas + (HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
	}
	hkConstraintInternal* pInternal = pMgr->m_constraint.m_internal;
	pInternal->m_entities[pInternal->m_whoIsMaster]->m_simulationIsland->addConstraintInfo(&pMgr->m_constraint, kInfo);

	// The atom's contact points (32 bytes each) and then the reserved points' properties follow it in memory, so
	// their addresses are untyped arithmetic.
	hkpSimpleContactConstraintAtom* pAtom = pMgr->m_contactConstraintData.m_atom;
	hkString::memCpy(reinterpret_cast<uint8_t*>(pAtom + 1) + pAtom->m_numReservedContactPoints * 0x20,
		serializedEntryIn.m_propertiesStream.m_data, pAtom->m_contactPointPropertiesStriding * iNumPoints);
	hkString::memCpy(pMgr->m_contactConstraintData.m_atom + 1, serializedEntryIn.m_contactPoints.m_data,
		iNumPoints * sizeof(hkContactPoint));
	if (input.m_zeroUserDataInContactPointProperties) {
		pAtom = pMgr->m_contactConstraintData.m_atom;
		const int32_t iStriding = pAtom->m_contactPointPropertiesStriding;
		uint8_t* pucProperties = reinterpret_cast<uint8_t*>(pAtom + 1) + pAtom->m_numReservedContactPoints * 0x20;
		for (int32_t i = 0; i < pMgr->m_contactConstraintData.m_atom->m_numContactPoints; ++i) {
			reinterpret_cast<hkpContactPointProperties*>(pucProperties)->m_userData = 0;
			pucProperties += iStriding;
		}
	}

	FNVD_INLINED_LOCAL(hkArray<uint16_t>, kPointIds, std::nullopt, std::nullopt);
	kPointIds.m_data = nullptr;
	kPointIds.m_size = 0;
	kPointIds.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
	hkpDynamicsContactMgr* pDynamicsMgr = pMgr;
	pDynamicsMgr->getAllContactPointIds(kPointIds);

	if (input.m_fireContactPointAddedCallbacks) {
		for (int32_t i = 0; i < kPointIds.m_size; ++i) {
			const uint16_t usId = kPointIds.m_data[i];
			// EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput
			alignas(16) hkpProcessCollisionOutput kOutput;
			kOutput.m_toi.m_time.m_storage = 3.40282347e+38f;
			kOutput.m_toi.m_properties.m_impulseApplied = 0.0f;
			kOutput.m_toi.m_properties.m_internalSolverData = 0.0f;
			kOutput.m_constraintOwner.m_storage = nullptr;
			const hkpCdBody* pBodyA = nullptr;
			const hkpCdBody* pBodyB = nullptr;
			if (input.m_passCollidablePointersInCollisionCallbacks) {
				pBodyA = &entityA->m_collidable;
				pBodyB = &entityB->m_collidable;
			}
			hkpContactPointProperties* pProperties = pDynamicsMgr->getContactPointProperties(usId);
			hkContactPoint* pContactPoint = pDynamicsMgr->getContactPoint(usId);
			uint16_t usCallbackDelay = 0;
			if (pMgr->m_type == hkpContactMgr::TYPE_SIMPLE_CONSTRAINT_CONTACT_MGR) {
				usCallbackDelay = pMgr->m_contactPointCallbackDelay;
			}
			// EXE-INLINED none: hkpManifoldPointAddedEvent::hkpManifoldPointAddedEvent
			// The const type needs an aggregate: it also writes m_callbackFiredFrom (null), which the exe leaves
			// unwritten and fireContactPointAdded clears before any listener reads it.
			// The native struct has constructors, so no aggregate: its constructor takes the same delay (the simple
			// constraint contact manager's, else 0).
			hkpManifoldPointAddedEvent kEvent(usId, pMgr, collisionInput, &kOutput, pBodyA, pBodyB, pContactPoint,
				nullptr, pProperties, 0.0f);
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
			// EXE-INLINED none: hkpContactPointEvent::hkpContactPointEvent
			hkpContactPointEvent kEvent;
			kEvent.m_contactPoint = pContactPoint;
			kEvent.m_contactPointProperties = pProperties;
			// The shape keys are the extended user data that follows the properties in the atom's untyped stream.
			kEvent.m_shapeKeyStorage = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pProperties) +
				sizeof(hkpContactPointProperties));
			kEvent.m_source = hkpCollisionEvent::SOURCE_WORLD;
			kEvent.m_contactMgr = pMgr;
			kEvent.m_bodies[0] = static_cast<hkpRigidBody*>(entityA);
			kEvent.m_bodies[1] = static_cast<hkpRigidBody*>(entityB);
			kEvent.m_type = hkpContactPointEvent::TYPE_MANIFOLD_FROM_SAVED_CONTACT_POINT;
			kEvent.m_firingCallbacksForFullManifold.m_bool = 0;
			kEvent.m_firstCallbackForFullManifold.m_bool = 0;
			kEvent.m_lastCallbackForFullManifold.m_bool = 0;
			kEvent.m_separatingVelocity = nullptr;
			kEvent.m_rotateNormal = nullptr;
			kEvent.m_accumulators[0] = nullptr;
			kEvent.m_accumulators[1] = nullptr;
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
	// EXE-INLINED none: hkArray<uint16_t>::~hkArray
	hkMemoryAllocator* pFreeHeap = hkMemoryRouter::getInstance().m_heap;
	kPointIds.m_size = 0;
	if (kPointIds.m_capacityAndFlags >= 0) {
		pFreeHeap->bufFree(kPointIds.m_data,
			(kPointIds.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK) * sizeof(uint16_t));
	}
	return HK_SUCCESS;
}

// Havok SDK: loads the points of every entity, through a selector that accepts all (its constructor inlined to
// the vtable store).
void hkpSaveContactPointsUtil::loadContactPoints(const hkpSaveContactPointsUtil::LoadPointsInput& input,
	hkpPhysicsSystemWithContacts* sys, hkpWorld* world) {
	FNVD_INLINED_LOCAL(hkEntitySelectorAll, selector, hkEntitySelectorAll_vftable, std::nullopt);
	loadContactPointsInternal(input, sys, world, selector);
}

void hkpSaveContactPointsUtil::loadContactPoints(const hkpSaveContactPointsUtil::LoadPointsInput& input,
	hkpPhysicsSystemWithContacts* sys, hkpEntity** entities, int32_t numEntities) {
	FNVD_INLINED_LOCAL(hkEntitySelectorListed, a4, std::nullopt, std::nullopt);
	hkpWorld* m_world = (*entities)->m_world;
	::new (&a4) hkEntitySelectorListed(entities, numEntities);
	hkpSaveContactPointsUtil::loadContactPointsInternal(input, sys, m_world, a4);
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	a4.m_map.m_map.clearAndDeallocate(*Instance->m_heap);
}

// KEEP: the exe body never writes edx, and its caller destroyOldEntry (0xDCD6A0) pushes edx (the entry it loaded
// before the call) at 0xDCD707 without loading it again; clang does not keep edx across the call.
hkpSerializedAgentNnEntry::SerializedAgentType
hkpSaveContactPointsUtil::getSerializedAgentType(void* (*func)(const hkpAgent3ProcessInput&, hkpAgentEntry*, void*,
	hkVector4*, hkpProcessCollisionOutput&)) {
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

void hkpSaveContactPointsUtil::saveContactPoints_00DCDC00(const hkpSaveContactPointsUtil::SavePointsInput& input,
	const hkpEntity** entities, int32_t numEntities, hkpPhysicsSystemWithContacts* sys) {
	hkpWorld* world = entities[0]->m_world;
	FNVD_INLINED_LOCAL(hkpSaveContactPointsCollidableMap, processedCollidables, std::nullopt, std::nullopt);
	// EXE-INLINED none: hkPointerMap<const hkpLinkedCollidable*, int>::hkPointerMap
	processedCollidables.m_map.m_elem = nullptr;
	processedCollidables.m_map.m_numElems = 0;
	processedCollidables.m_map.m_hashMod = -1;
	processedCollidables.m_map.hkpSaveContactPointsCollidableMapBase::reserve(hkMemoryRouter::getInstance().heap(),
		numEntities);
	for (int32_t i = 0; i < numEntities; ++i) {
		processedCollidables.insert(&entities[i]->m_collidable, 0);
		FNVD_INLINED_LOCAL(hkArray<hkpLinkedCollidable::CollisionEntry>, entries, std::nullopt, std::nullopt);
		// EXE-INLINED none: hkArray<hkpLinkedCollidable::CollisionEntry>::hkArray
		entries.m_data = nullptr;
		entries.m_size = 0;
		entries.m_capacityAndFlags = hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG;
		entities[i]->m_collidable.getCollisionEntriesSorted(entries);
		for (int32_t j = 0; j < entries.m_size; ++j) {
			const hkpLinkedCollidable::CollisionEntry& entry = entries.m_data[j];
			if (!processedCollidables.isValid(processedCollidables.findKey(entry.m_partner))) {
				// EXE-INLINED none: hkpSerializedAgentNnEntry::operator new
				void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSerializedAgentNnEntry));
				static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpSerializedAgentNnEntry);
				hkpSerializedAgentNnEntry* pSerialized = ::new (pMemory) hkpSerializedAgentNnEntry;
				if (serializeCollisionEntry(input, entry.m_agentEntry, world->m_collisionInput, *pSerialized) ==
					HK_SUCCESS) {
					sys->addContact(pSerialized);
				}
				pSerialized->removeReference();
			}
		}
		// EXE-INLINED none: hkArray<hkpLinkedCollidable::CollisionEntry>::~hkArray
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		entries.m_size = 0;
		if (entries.m_capacityAndFlags >= 0) {
			pHeap->bufFree(entries.m_data,
				static_cast<int32_t>(entries.m_capacityAndFlags &
					hkArrayBase<hkpLinkedCollidable::CollisionEntry>::CAPACITY_MASK) *
					static_cast<int32_t>(sizeof(hkpLinkedCollidable::CollisionEntry)));
		}
	}
	// EXE-INLINED none: hkPointerMap<const hkpLinkedCollidable*, int>::~hkPointerMap (the map's operations
	// destructor after it is the exe's empty function 0xA29680)
	processedCollidables.m_map.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
}

// Havok SDK: sectorsOut gets a new copy of each saved sector, then each saved sub track is rebuilt in place, at its
// sector and byte offset, as an empty track filled by a recursive call. The exe inlines the array reserve and the
// sector and track constructors, and copies each sector whole (rep movsd).
hkResult hkpSaveContactPointsUtil::deserialize1nTrack(const hkpSerializedTrack1nInfo& serializedTrack,
	const hkpProcessCollisionInput* input, hkArray<hkpAgent1nSector*, hkContainerHeapAllocator>& sectorsOut) {
	// EXE-INLINED none: hkArray<hkpAgent1nSector*>::setSize
	int32_t iNumSectors = serializedTrack.m_sectors.m_size;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity =
		static_cast<int32_t>(sectorsOut.m_capacityAndFlags & hkArrayBase<hkpAgent1nSector*>::CAPACITY_MASK);
	if (iCapacity < iNumSectors) {
		int32_t iDoubled = 2 * iCapacity;
		hkArrayUtil::_reserve(pHeap, &sectorsOut, iNumSectors < iDoubled ? iDoubled : iNumSectors,
			sizeof(hkpAgent1nSector*));
	}
	sectorsOut.m_size = iNumSectors;
	for (int32_t i = 0; i < serializedTrack.m_sectors.m_size; ++i) {
		// EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
		hkpAgent1nSector* pSector =
			static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpAgent1nSector)));
		if (pSector) {
			pSector->m_bytesAllocated = 0;
		}
		sectorsOut.m_data[i] = pSector;
		*sectorsOut.m_data[i] = *serializedTrack.m_sectors.m_data[i];
	}
	for (int32_t i = 0; i < serializedTrack.m_subTracks.m_size; ++i) {
		const hkpSerializedSubTrack1nInfo* pSubTrack = serializedTrack.m_subTracks.m_data[i];
		// The sector's bytes are untyped agent data: the sub track sits at its byte offset there.
		hkpAgent1nTrack* pTrack =
			reinterpret_cast<hkpAgent1nTrack*>(sectorsOut.m_data[pSubTrack->m_sectorIndex]->m_data +
				pSubTrack->m_offsetInSector);
		if (pTrack) {
			// EXE-INLINED none: hkpAgent1nTrack::hkpAgent1nTrack
			pTrack->m_sectors.m_data = nullptr;
			pTrack->m_sectors.m_size = 0;
			pTrack->m_sectors.m_capacityAndFlags = hkArrayBase<hkpAgent1nSector*>::DONT_DEALLOCATE_FLAG;
		}
		if (deserialize1nTrack(*pSubTrack, input, pTrack->m_sectors) == HK_FAILURE) {
			return HK_FAILURE;
		}
	}
	return HK_SUCCESS;
}

void hkpSaveContactPointsUtil::loadContactPointsInternal(const hkpSaveContactPointsUtil::LoadPointsInput& input,
	hkpPhysicsSystemWithContacts* sys, hkpWorld* world, hkpSaveContactPointsUtil::EntitySelector& selector) {
	hkpWorld* v4 = world;
	++world->m_criticalOperationsLockCount;
	int32_t v5 = sys->m_contacts.m_size - 1;
	for (int32_t i = v5; i >= 0; --i) {
		hkpEntity* m_bodyA;
		hkpEntity* m_bodyB;
		hkpSerializedAgentNnEntry* v6 = sys->m_contacts.m_data[v5];
		if (v6->m_useEntityIds.m_bool) {
			m_bodyA = input.m_getEntityFromId(v6->m_bodyAId);
			m_bodyB = input.m_getEntityFromId(v6->m_bodyBId);
		} else {
			m_bodyA = v6->m_bodyA;
			m_bodyB = v6->m_bodyB;
		}
		if (m_bodyA) {
			if (m_bodyB) {
				if (selector.isEntityOk(m_bodyA) || selector.isEntityOk(m_bodyB)) {
					hkpAgentNnEntry* Agent = hkAgentNnMachine_FindAgent(&m_bodyA->m_collidable, &m_bodyB->m_collidable);
					if (Agent) {
						if (v6->endianCheckUint32() != hkpSerializedAgentNnEntry::ENDIAN_CHECK_VALUE) {
							hkpSaveContactPointsEndianUtil::swapEndianTypeInCollisionEntry(world->m_collisionInput,
								*v6);
						}
						hkResult v10 = hkpSaveContactPointsUtil::deserializeCollisionEntry(input, *v6, m_bodyA, m_bodyB,
							world->m_collisionInput, Agent);
						if (input.m_removeSerializedAgentsWhenLoaded) {
							if (v10) {
								sys->m_contacts.m_data[i]->removeReference();
								int32_t v11 = --sys->m_contacts.m_size;
								if (v11 != i) {
									sys->m_contacts.m_data[i] = sys->m_contacts.m_data[v11];
								}
							}
						}
					}
				}
				v4 = world;
			}
		}
		v5 = i - 1;
	}
	if (v4->m_criticalOperationsLockCount-- == 1 && !v4->m_blockExecutingPendingOperations.m_bool) {
		if (v4->m_pendingOperationsCount) {
			v4->internal_executePendingOperations();
		}
		if (v4->m_pendingOperationQueueCount == 1) {
			if (v4->m_pendingBodyOperationsCount) {
				v4->internal_executePendingBodyOperations();
			}
		}
	}
}
