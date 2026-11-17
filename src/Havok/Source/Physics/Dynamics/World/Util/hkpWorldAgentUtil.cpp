#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"

hkpSimulationIsland* getAnyNonFixedIsland(hkpSimulationIsland* a1, hkpSimulationIsland* r_eax) {
	if (r_eax->m_storageIndex == 0xFFFF && a1->m_storageIndex != 0xFFFF) {
		return a1;
	}
	return r_eax;
}

void* getAgentData(hkpAgentNnEntry* entry) {
	if (entry->m_streamCommand == 4) {
		return &entry[2];
	} else {
		return &entry[1].m_contactMgr;
	}
}

void hkpWorldAgentUtil::invalidateTim(hkpEntity* entity, const hkpCollisionInput& collisionInput) {
	hkpLinkedCollidable* collidable = &entity->m_collidable;
	// Havok's getSize() is an int, so the compare is signed.
	for (int32_t i = 0; i < collidable->m_collisionEntries.m_size; i++) {
		hkAgentNnMachine_InvalidateTimInAgent(collidable->m_collisionEntries.m_data[i].m_agentEntry, collisionInput);
	}
}

// Merges the two entities' islands unless one is fixed, picks the non-fixed island of the two, resolves transform
// shapes on either side and creates the agent, with the pair flipped where the dispatcher says so.
hkpAgentNnEntry* hkpWorldAgentUtil::addAgent(hkpLinkedCollidable* collA, hkpLinkedCollidable* collB,
	const hkpProcessCollisionInput& input) {
	hkpEntity* entityA = static_cast<hkpEntity*>(collA->getOwner());
	hkpEntity* entityB = static_cast<hkpEntity*>(collB->getOwner());
	if (entityA->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
		entityB->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
		entityA->getSimulationIsland() != entityB->getSimulationIsland()) {
		hkpWorldOperationUtil::mergeIslands(entityA->m_world, entityA, entityB);
	}
	hkpSimulationIsland* islandA = entityA->getSimulationIsland();
	hkpSimulationIsland* islandB = entityB->getSimulationIsland();
	// EXE-INLINED 0xCC0E00
	hkpSimulationIsland* theIsland =
		(islandA->m_storageIndex == 0xFFFF && islandB->m_storageIndex != 0xFFFF) ? islandB : islandA;

	uint8_t cdBodyHasTransformFlag = 0;
	alignas(16) hkpCdBody newCdBodiesA[4];
	hkMotionState newMotionStatesA[4];
	alignas(16) hkpCdBody newCdBodiesB[4];
	hkMotionState newMotionStatesB[4];
	const hkpCdBody* firstNonTransformBodyA = collA;
	const hkpCdBody* firstNonTransformBodyB = collB;
	if (collA->m_shape->m_type == HK_SHAPE_TRANSFORM) {
		firstNonTransformBodyA =
			hkAgentMachine_processTransformedShapes(collA, newCdBodiesA, newMotionStatesA, 4, cdBodyHasTransformFlag);
	}
	if (collB->m_shape->m_type == HK_SHAPE_TRANSFORM) {
		firstNonTransformBodyB =
			hkAgentMachine_processTransformedShapes(collB, newCdBodiesB, newMotionStatesB, 4, cdBodyHasTransformFlag);
	}

	int32_t agentType;
	int32_t isFlipped;
	hkAgentNnMachine_GetAgentType(firstNonTransformBodyA, firstNonTransformBodyB, input, agentType, isFlipped);
	if (isFlipped) {
		hkpLinkedCollidable* collTmp = collA;
		collA = collB;
		collB = collTmp;
		const hkpCdBody* bodyTmp = firstNonTransformBodyA;
		firstNonTransformBodyA = firstNonTransformBodyB;
		firstNonTransformBodyB = bodyTmp;
	}
	// The factory is picked by the entities' response types in their original order.
	hkpContactMgrFactory* factory =
		input.m_dispatcher->m_contactMgrFactory[entityA->m_material.m_responseType.m_storage]
											   [entityB->m_material.m_responseType.m_storage];
	hkpContactMgr* mgr = factory->createContactMgr(*collA, *collB, input);
	return hkAgentNnMachine_CreateAgent(theIsland->m_agentTrack, collA, firstNonTransformBodyA, collB,
		firstNonTransformBodyB, cdBodyHasTransformFlag, agentType, input, mgr);
}

// The world reached through the first entity's island has its simulation drop the agent's collision information
// (its time of impact events); then the agent goes.
void hkpWorldAgentUtil::removeAgentAndItsToiEvents(hkpAgentNnEntry* agent) {
	hkpEntity* entity = static_cast<hkpEntity*>(agent->m_collidable[0]->getOwner());
	entity->getSimulationIsland()->getWorld()->m_simulation->removeCollisionInformationForAgent(agent);
	hkpWorldAgentUtil::removeAgent(agent);
}

// The agent's island: the common one, flagged for a split check; else the island of the entity that is not fixed; else
// the one whose track holds the entry, both islands flagged. The second entity's island is read again after
// getIslandFromAgentEntry. The contact manager, read before the agent goes, is cleaned up last.
void hkpWorldAgentUtil::removeAgent(hkpAgentNnEntry* agent) {
	hkpEntity* entityA = static_cast<hkpEntity*>(agent->m_collidable[0]->getOwner());
	hkpEntity* entityB = static_cast<hkpEntity*>(agent->m_collidable[1]->getOwner());
	hkpSimulationIsland* islandA = entityA->getSimulationIsland();
	hkpSimulationIsland* theIsland;
	if (islandA == entityB->getSimulationIsland()) {
		theIsland = islandA;
		theIsland->m_splitCheckRequested = 1;
	} else if (entityA->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
		theIsland = entityB->getSimulationIsland();
	} else if (entityB->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
		theIsland = islandA;
	} else {
		theIsland = hkpWorldAgentUtil::getIslandFromAgentEntry(agent, islandA, entityB->getSimulationIsland());
		islandA->m_splitCheckRequested = 1;
		entityB->getSimulationIsland()->m_splitCheckRequested = 1;
	}
	hkpContactMgr* mgr = agent->m_contactMgr;
	hkAgentNnMachine_DestroyAgent(theIsland->m_agentTrack, agent, theIsland->getWorld()->m_collisionDispatcher,
		*theIsland);
	mgr->cleanup();
}

// Searches the sectors of the island with fewer of them for the one holding the entry: the entry belongs to that
// island when found there, else to the other. The pointer compares are unsigned, the counts signed.
hkpSimulationIsland* hkpWorldAgentUtil::getIslandFromAgentEntry(hkpAgentNnEntry* entry, hkpSimulationIsland* candidateA,
	hkpSimulationIsland* candidateB) {
	const bool searchIslandA = candidateA->m_agentTrack.m_sectors.m_size <= candidateB->m_agentTrack.m_sectors.m_size;
	hkpSimulationIsland* islandToSearch = searchIslandA ? candidateA : candidateB;
	bool found = false;
	for (int32_t i = 0; i < islandToSearch->m_agentTrack.m_sectors.m_size; i++) {
		hkpAgentNnSector* sector = islandToSearch->m_agentTrack.m_sectors.m_data[i];
		if (sector->getBegin() <= entry && entry < sector->getEnd()) {
			found = true;
			break;
		}
	}
	return (searchIslandA != found) ? candidateB : candidateA;
}

// The sorted copy of the entity's agent list starts empty (no storage, not ours). Each agent with an update-filter
// function gets the island of its first entity, or of its second when the first is fixed.
void hkpWorldAgentUtil::updateEntityShapeCollectionFilter(hkpEntity* entity, hkpCollisionInput& collisionInput) {
	FNVD_INLINED_LOCAL(hkArray<hkpLinkedCollidable::CollisionEntry>, collisionEntries, std::nullopt, std::nullopt);
	collisionEntries.m_data = nullptr;
	collisionEntries.m_size = 0;
	collisionEntries.m_capacityAndFlags = hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG;
	entity->m_collidable.getCollisionEntriesSorted(collisionEntries);
	for (int32_t i = 0; i < collisionEntries.m_size; ++i) {
		hkpAgentNnEntry* entry = collisionEntries.m_data[i].m_agentEntry;
		hkAgent3::UpdateFilterFunc func =
			collisionInput.m_dispatcher->m_agent3Func[entry->m_agentType].m_updateFilterFunc;
		if (!func)
			continue;
		hkpLinkedCollidable* collA = entry->m_collidable[0];
		hkpLinkedCollidable* collB = entry->m_collidable[1];
		// The owner sits m_ownerOffset bytes from the collidable, as raw memory.
		// EXE-INLINED 0x4B59D0
		hkpEntity* entityA =
			static_cast<hkpEntity*>(reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(collA) +
				collA->m_ownerOffset));
		hkpSimulationIsland* island;
		if (entityA->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
			// EXE-INLINED 0x4B59D0
			hkpEntity* entityB =
				static_cast<hkpEntity*>(reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(collB) +
					collB->m_ownerOffset));
			island = entityB->m_simulationIsland;
		} else {
			island = entityA->m_simulationIsland;
		}
		hkpAgentData* agentData;
		if (entry->m_streamCommand == hkAgent3::STREAM_CALL_WITH_TIM)
			agentData = static_cast<hkpAgentNnMachineTimEntry*>(entry) + 1;
		else
			agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
		func(entry, agentData, *collA, *collB, collisionInput, entry->m_contactMgr, *island);
	}

	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	const int32_t capacityAndFlags = collisionEntries.m_capacityAndFlags;
	collisionEntries.m_size = 0;
	if (capacityAndFlags >= 0) {
		heap.bufFree(collisionEntries.m_data,
			(capacityAndFlags & hkArrayBase<hkpLinkedCollidable::CollisionEntry>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(hkpLinkedCollidable::CollisionEntry)));
	}
}

// The SDK's HK_FOR_ALL_AGENT_ENTRIES_BEGIN walk over the island's track: every sector is full but the last, which holds
// m_bytesUsedInLastSector bytes, and each entry's m_size steps to the next.
void hkpWorldAgentUtil::warpTime(hkpSimulationIsland* island, float oldTime, float newTime,
	const hkpCollisionInput& collisionInput) {
	float oldTimeCopy = oldTime;
	float newTimeCopy = newTime;
	hkpAgentNnTrack& track = island->m_agentTrack;
	for (int32_t sectorIndex = 0; sectorIndex < track.m_sectors.m_size;) {
		hkpAgentNnSector* currentSector = track.m_sectors.m_data[sectorIndex];
		hkpAgentNnEntry* entry = currentSector->getBegin();
		sectorIndex++;
		// The sector's bytes are untyped storage the entries are carved from.
		// EXE-INLINED none: hkAddByteOffset<hkpAgentNnEntry>
		hkpAgentNnEntry* endEntry = (sectorIndex == track.m_sectors.m_size)
			? reinterpret_cast<hkpAgentNnEntry*>(currentSector->m_data + track.m_bytesUsedInLastSector)
			: currentSector->getEnd();
		// Each entry's m_size is its byte length in that untyped storage.
		// EXE-INLINED none: hkAddByteOffset<hkpAgentNnEntry>
		for (; entry < endEntry;
			entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size)) {
			hkAgentNnMachine_WarpTimeInAgent(entry, oldTimeCopy, newTimeCopy, collisionInput);
		}
	}
}
