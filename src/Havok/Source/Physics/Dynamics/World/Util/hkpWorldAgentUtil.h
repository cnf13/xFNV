#pragma once

#include "Reconstruction.hpp"

class hkpAgentNnEntry;
class hkpCollisionInput;
class hkpEntity;
class hkpLinkedCollidable;
class hkpProcessCollisionInput;
class hkpSimulationIsland;

hkpSimulationIsland* getAnyNonFixedIsland(hkpSimulationIsland* a1, hkpSimulationIsland* r_eax);
// Havok SDK (the exe's name ?getAgentData@@YAPAXPAUhkpAgentNnEntry@@@Z): the agent data
// after the entry's header, at +0x30 for a type-4 entry, else at +0x20. Usercall (0xCC0E90): entry in eax, ret 0.
void* getAgentData(hkpAgentNnEntry* entry);

// The world's helpers for the agents of an entity's collision pairs.
class hkpWorldAgentUtil {
public:
	// Invalidates the cached time of impact of every agent of the entity's pairs.
	static void invalidateTim(hkpEntity* entity, const hkpCollisionInput& collisionInput);
	// Creates the agent of a pair of collidables, or returns null.
	static hkpAgentNnEntry* addAgent(hkpLinkedCollidable* collA, hkpLinkedCollidable* collB,
		const hkpProcessCollisionInput& input);
	// Removes the agent and the time of impact events of its pair.
	static void removeAgentAndItsToiEvents(hkpAgentNnEntry* agent);
	// Removes the agent of a pair.
	static void removeAgent(hkpAgentNnEntry* agent);
	// The island that holds the agent entry's pair: one of the two islands passed.
	static hkpSimulationIsland* getIslandFromAgentEntry(hkpAgentNnEntry* entry, hkpSimulationIsland* candidateA,
		hkpSimulationIsland* candidateB);
	// Runs each agent's update-filter function on the entity's sorted agent list.
	static void updateEntityShapeCollectionFilter(hkpEntity* entity, hkpCollisionInput& collisionInput);
	// Moves the island's agents in time.
	static void warpTime(hkpSimulationIsland* island, float oldTime, float newTime,
		const hkpCollisionInput& collisionInput);
};
