#pragma once

#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/hkpToiResourceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpAgentNnEntry;
struct hkpConstraintSchemaInfo;
struct hkpConstraintSolverResources;
class hkpContinuousSimulation;
class hkpDynamicsContactMgr;
class hkpEntity;
class hkpProcessCollisionOutput;
class hkpSimulationIsland;
struct hkpToiResources;

// A time of impact between two entities: when it happens, how fast the bodies separate, the pair, the contact
// manager, the contact point's properties and the contact point itself. Members, offsets and size (0x70) from the
// Havok 7.1 hkpdynamics.pdb and the exe (hkpMultiThreadedSimulation::addToiEventWithCriticalSectionLock, 0xD02E70,
// and handleSimpleToi, 0xD0E210, step the event array by 0x70). The PDB's hkTime and hkPadSpu members are plain
// floats here. The properties' 0x14 bytes end where m_extendedUserDatas starts; the exe passes the addresses of the
// properties (+0x18) and of the contact point (+0x50). Its constructor and operator= live in
// hkpMultithreadedSimulation.cpp.
struct hkpToiEvent {
	float m_time;
	float m_seperatingVelocity;
	hkBool m_useSimpleHandling;
	hkpEntity* m_entities[2];
	hkpDynamicsContactMgr* m_contactMgr;
	hkpContactPointProperties m_properties;
	// The contact point's extended user data words (the Havok PDB's unsigned[7]).
	uint32_t m_extendedUserDatas[7];
	// The contact point is 16-byte aligned (its hkVector4s), which puts it at 0x50; the PC release PDB
	// (hkpdynamics.pdb, sizeof 0x70) names no member between the user data words and it. The exe copies events whole
	// (handleAllToisTill's dword loop and rep movsd over 0x70 bytes, 0xD0EB00 and 0xD0EB30), these 8 bytes included.
	uint8_t unk48[8];
	hkContactPoint m_contactPoint;
};

// A static of hkpContinuousSimulation.cpp: appends to entriesOut the agent of each of entity's pairs whose collision
// quality index is at least minQualityIndex, skipping partners whose state in entityStates (by storage index) is 8 or
// more; a non-fixed partner not yet moved (state 0) is marked 1 and set to its transform at time (cdecl, 0xD0D2C0).
// The PDB names no parameters. minQualityIndex is the PDB's hkpConstraintInstance::ConstraintPriority, an enum src
// does not have.
void hkLs_collectAgentEntriesToProcess(const hkpEntity* entity,
	hkpConstraintInstance::ConstraintPriority minPriorityToProcess, float time, hkFixedArray<uint8_t>& entityState,
	hkLocalArray<hkpAgentNnEntry*>& entriesOut);
// A static of hkpContinuousSimulation.cpp: solves the event's contact point as a single collision; when its impulse
// exceeds the contact's limit the world's impulse limit listeners are told instead, otherwise the moving bodies that
// need activating are appended to toBeActivated (cdecl, 0xD0C480). The PDB names no parameters (the SDK's names).
void hkLs_doSimpleCollisionResponse(hkpWorld* world, const hkpToiEvent& event, float rotateNormal,
	hkArray<hkpEntity*>& toBeActivated);
// A quickSort comparator of hkLs_toiActivateEntitiesAndCheckConstraints: orders entities by uid.
hkBool* less_EntityPtr(hkBool* apResult, const hkpEntity* a, const hkpEntity* b);
// A quickSort comparator of hkLs_toiActivateEntitiesAndCheckConstraints: orders constraints by master entity uid.
hkBool* less_ConstraintPtr(hkBool* apResult, const hkpConstraintInstance* a, const hkpConstraintInstance* b);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
int32_t hkLs_areVelocitiesOk(float a1, const hkpConstraintSchemaInfo& a2, const float* a3, int32_t a4,
	const hkpProcessCollisionInput& a5);
// Havok SDK signature (hkpDynamics.lib, tied by bytes).
void hkLs_toiActivateConstraintsLinkingToFixedAndKeyframedEntities(hkArray<hkpConstraintSchemaInfo,
																	   hkContainerHeapAllocator>& constraintStatus,
	int32_t& firstNonActiveConstraintStatus, int32_t numOfConstraintsToCheckAtTheEndOfTheList);
// A static of hkpContinuousSimulation.cpp: sets each entity's accumulator velocities from the entity again, the
// angular one through the accumulator's backed-up core-from-world column.
void hkLs_toiResetVelocityAccumulatorsForEntities(hkpConstraintSolverResources& solverResources,
	const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities);
// A static of hkpContinuousSimulation.cpp: with the entities' accumulators set to the velocities of their swept
// transforms, checks each constraint from firstNonActiveConstraintStatus on whose priority is at least
// minPriorityToProcess; a constraint whose non-frozen entity is marked 4 already, or which its solved velocities
// violate (then the entity is marked 4 and appended to toBeActivated), is moved to the active front. The accumulator
// velocities are put back after (cdecl, ret 0, 0xD0CB00). The PDB's signature; it names no parameters (the SDK's).
void hkLs_toiCheckValidityOfConstraints(hkpConstraintSolverResources& solverResources,
	hkpProcessCollisionInput& processInput,
	hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>& constraintStatus,
	int32_t& firstNonActiveConstraintStatus, hkpConstraintInstance::ConstraintPriority minPriorityToProcess,
	hkFixedArray<uint8_t>& entityState, const hkArray<hkpEntity*, hkContainerHeapAllocator>& touchedEntities,
	hkArray<hkpEntity*, hkContainerHeapAllocator>& toBeActivated);
// A static of hkpContinuousSimulation.cpp: whether no forced (PRIORITY_TOI_FORCED) constraint is violated by its
// solved velocities. Without violatedConstraints it stops at the first violation; with it, each violation is
// appended there, with its contact point and properties for a contact constraint.
hkBool* hkLs_toiCheckFinalValidityOfCriticalConstraints(hkBool* apResult, hkpConstraintSolverResources& solverResources,
	const hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>& constraintStatus,
	hkpProcessCollisionInput& processInput,
	hkArray<hkpToiResourceMgr::ConstraintViolationInfo, hkContainerHeapAllocator>* violatedConstraints);
// Activates the entities of a time of impact solve and collects the constraints and partners they touch.
void hkLs_toiActivateEntitiesAndCheckConstraints(hkpProcessCollisionInput& collisionInput,
	hkpConstraintInstance::ConstraintPriority minPriorityToProcess, float time,
	const hkArray<hkpEntity*, hkContainerHeapAllocator>& toBeActivated, hkFixedArray<uint8_t>& entityState,
	hkArray<hkpEntity*, hkContainerHeapAllocator>& newTouchedEntities,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& newTouchedConstraints);
// A static of hkpContinuousSimulation.cpp: for the entities whose state is 1 or 2, sets the transform from the swept
// transform's end; for those whose state is 2 or 8, takes the velocities back from their accumulators.
void hkLs_restoreTransformOnBodiesWithUpdatedTransform(hkpSimulationIsland* island, hkFixedArray<uint8_t>& entityState,
	hkpConstraintSolverResources& solverResources);
// A static of hkpContinuousSimulation.cpp: backsteps each of the island's entities (+0x48 data, +0x4C size) to time
// and appends those whose state byte is not 16 to the array, marking them 16.
void hkLs_backstepAndFreezeEntireIsland(float time, hkpSimulationIsland* island, hkFixedArray<uint8_t>& entityState,
	hkArray<hkpEntity*, hkContainerHeapAllocator>& activeEntities);
// A static of hkpContinuousSimulation.cpp: moves the constraints from firstNonActiveConstraintStatus on whose two
// entities are both active (state 16) to the front, swapping each with the first non-active one.
void hkLs_toiActivateConstraintsLinkingActivatedEntities(hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>&
															 constraintStatus,
	int32_t& firstNonActiveConstraintStatus, hkFixedArray<uint8_t>& entityState);
// A static of hkpContinuousSimulation.cpp: solves the TOI event's constraints locally; HK_FAILURE when the contact
// manager activates nothing.
hkResult hkLs_localizedSolveToi(const hkpToiResources& toiResources, hkpConstraintSolverResources& solverResources,
	hkpToiEvent& event, hkpToiResourceMgr& toiResourceMgr, hkpWorld* world,
	hkArray<hkpEntity*, hkContainerHeapAllocator>& activeEntities, hkFixedArray<uint8_t>& entityState,
	float rotateNormal);

// The simulation with continuous collision detection: it collects time of impact events and solves them in order
// after each step. Its code stays with the exe.
class hkpContinuousSimulation : public hkpSimulation {
public:
	// Holds the world's critical operations locked while it lives (hkpdynamics.pdb, sizeof 4; IDA names no
	// out-of-line copy of its constructor or destructor).
	class LockCriticalOperationsScope {
	public:
		hkpWorld* m_world;
	};

	// The continuous simulation of world.
	hkpContinuousSimulation(hkpWorld* world);
	~hkpContinuousSimulation() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	hkpStepResult advanceTime() override;
	void resetCollisionInformationForEntities(hkpEntity** entities, int32_t numEntities, hkpWorld* world,
		hkpSimulation::ResetCollisionInformation resetInfo) override;
	void assertThereIsNoCollisionInformationForEntities(hkpEntity** entities, int32_t numEntities,
		hkpWorld* world) override;
	void removeCollisionInformationForAgent(hkpAgentNnEntry* agent) override;
	void assertThereIsNoCollisionInformationForAgent(hkpAgentNnEntry* agent) override;
	void reintegrateAndRecollideEntities(hkpEntity** entities, int32_t numEntities, hkpWorld* world,
		int32_t reintegrateRecollideMode) override;
	void collideInternal(const hkStepInfo& stepInfoIn) override;
	void warpTime(float warpDeltaTime) override;

protected:
	virtual void collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(hkpEntity** entities, int32_t numEntities,
		const hkpProcessCollisionInput& input,
		hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection);
	virtual void collideEntitiesNeedingPsiCollisionDetectionNarrowPhase_toiOnly(const hkpProcessCollisionInput& input,
		hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection);
	// Solves one TOI event localised to its island. The times are hkReal (a float).
	virtual void simulateToi(hkpWorld* world, hkpToiEvent& event, float physicsDeltaTime, float rotateNormal);

public:
	// Processes the entries' agents with continuous collision detection.
	virtual void processAgentNnEntries(hkpAgentNnEntry* const* entries, int32_t numEntries,
		const hkpProcessCollisionInput& collisionInput, hkpSimulationIsland* island, hkBool forceManifoldQualityType);

protected:
	// Handles a time of impact without the localized solver: the contact manager confirms it, each entity of simple
	// TOI quality is frozen at the event's time and loses its other pending TOIs, and the frozen entities are
	// recollided.
	void handleSimpleToi(hkpWorld* world, hkpToiEvent& event, float physicsDeltaTime, float rotateNormal);
	// Marks the event's contact point new, lets its contact manager fire the earliest-TOI callbacks and returns
	// whether the point is still enabled: hkBool through the hidden result pointer (thiscall, ret 0xC, 0xD0BC20;
	// handleAllToisTill reads the byte at the returned eax).
	hkBool* fireEarliestToiCallbacksAndCheckEnabled(hkBool* result, hkpToiEvent& event, float& rotateNormal);

	// Runs the continuous narrow phase over the island's agents;
	// the parameter names are the SDK's.
	void collideIslandNarrowPhaseContinuous(hkpSimulationIsland* isle, const hkpProcessCollisionInput& input);

public:
	// Runs the broad phase on the entities and updates the dispatcher's agents for the pairs that start and stop
	// overlapping; a non-null exportFinished is waited on first.
	void collideEntitiesBroadPhaseContinuous(hkpEntity** entities, int32_t numEntities, hkpWorld* world,
		char* exportFinished);
	// Updates the entities' broad phase AABBs (recomputing invalid ones) and collects the pairs that start and stop
	// overlapping, without the duplicates.
	void collideEntitiesBroadPhaseContinuousFindPairs(hkpEntity** entities, int32_t numEntities, hkpWorld* world,
		hkArray<hkpBroadPhaseHandlePair>& newPairs, hkArray<hkpBroadPhaseHandlePair>& delPairs);
	// IDA's hkpSimulation::collideEntitiesNarrowPhaseDiscrete, a second copy with three arguments that the PDB does
	// not name: the discrete narrow phase on the entities' agents (thiscall, ret 0xC, 0xD0F2E0).
	void collideEntitiesNarrowPhaseDiscrete(hkpEntity** entities, int32_t numEntities,
		const hkpProcessCollisionInput& input);
	// Appends a toi event to m_toiEvents from the output's toi record and the entry's pair, contact manager and
	// quality, or warns that the queue is full.
	void addToiEvent(const hkpProcessCollisionOutput& processOutput, const hkpAgentNnEntry& entry);
	// Removes the pending times of impact that involve any of the entities, telling each event's contact manager.
	void removeToiEventsOfEntities(hkpEntity** entities, int32_t numEntities);
	// Removes the pending times of impact that involve entity, telling each event's contact manager.
	void removeToiEventsOfEntity(hkpEntity* entity);
	void processAgentCollideContinuous(hkpAgentNnEntry* entry, const hkpProcessCollisionInput& processInput,
		hkpProcessCollisionOutput& processOutput);
	hkpStepResult handleAllToisTill(float minTime);
	hkpStepResult advanceTimeInternal();

	// IDA: UInt32 pad; the PDB has no member here. The base hkpSimulation is 0x2C bytes and m_toiEvents starts at
	// 0x30 in both the exe and the PC release PDB (hkpdynamics.pdb, sizeof 0x50), so these are the gap before it.
	uint8_t padding2C[4];
	// The pending times of impact (PDB: hkArray<hkpToiEvent,hkContainerHeapAllocator>).
	hkArray<hkpToiEvent> m_toiEvents;
	// The entities whose collisions are detected again after the time of impact events, by uid.
	hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator> m_entitiesNeedingPsiCollisionDetection;
	hkpToiResourceMgr* m_toiResourceMgr;
	int32_t m_toiCounter;
};

// The state of an entity during a time of impact solve, one byte per entity in the solve's hkFixedArray<uint8_t>
// (hkpdynamics.pdb's enum, a static of hkpContinuousSimulation.cpp; hkLs_collectAgentEntriesToProcess
// tests the byte below 8 and sets 0 to 1).
enum hkContinuousSimulationToiEntityState : uint8_t {
	HK_TOI_ENTITY_STATE_ZERO = 0,
	HK_TOI_ENTITY_STATE_TRANSFORM_UPDATED = 1,
	HK_TOI_ENTITY_STATE_TOUCHED = 2,
	HK_TOI_ENTITY_STATE_PENDING_FLAG = 4,
	HK_TOI_ENTITY_STATE_PENDING_AND_TOUCHED = 6,
	HK_TOI_ENTITY_STATE_MANIFOLD_UPDATED = 8,
	HK_TOI_ENTITY_STATE_PENDING_AND_MANIFOLD_UPDATED = 12,
	HK_TOI_ENTITY_STATE_ACTIVE = 16,
};

extern hkClass hkpSimulationClass;
extern hkTypeInfo hkpSimulationTypeInfo;
inline constexpr fnvd::Vtable hkpContinuousSimulation_vftable{};
