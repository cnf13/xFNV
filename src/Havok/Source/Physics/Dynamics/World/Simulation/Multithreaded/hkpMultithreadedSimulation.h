#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommandQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadConfig.h"

#include <stdint.h>

class hkJobThreadPool;
class hkpAgentNnEntry;
class hkpSimulationIsland;

// One island's slice of the agent entries handed to processAgentNnEntriesFromMultipleIslands.
struct hkpIslandsAgentEntriesInfo {
	hkpSimulationIsland* m_island;
	int32_t m_numEntries;
	int32_t m_firstEntryIdx;
	int32_t m_nextEntryIdx;
	hkpAgentNnEntryJob* m_ppuAgentNnEntryJob;
	hkpAgentNnEntryJob* m_spuAgentNnEntryJob;
};

// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
bool pairLessThan(const hkpTypedBroadPhaseHandlePair& a1, const hkpTypedBroadPhaseHandlePair& a2);
hkBool* less_hkSimulationIslandPtr(hkBool* apResult, const hkpSimulationIsland* a, const hkpSimulationIsland* b);
void sortPairList(hkArray<hkpTypedBroadPhaseHandlePair, hkContainerHeapAllocator>& pairs);
// Finds the entry of island in the array, appending a zeroed one keyed by island when there is none, and returns it.
hkpIslandsAgentEntriesInfo* getIslandEntriesInfo(hkpSimulationIsland* island,
	hkArray<hkpIslandsAgentEntriesInfo>* r_esi);

// An agent entry to recollide and the island whose job collides it.
struct EntryAndIsland {
	hkpAgentNnEntry* m_entry;
	hkpSimulationIsland* m_island;
};

// The continuous simulation stepped by several threads through a job queue. It overrides the destructor, the two assert
// functions and the two toi narrow phase functions, and adds getMultithreadConfig and setMultithreadConfig. Only some
// of the fields are declared.
class hkpMultiThreadedSimulation : public hkpContinuousSimulation {
public:
	// The listener the broad phase tells about new and lost entity-entity pairs: it creates or removes the pair's
	// agent, or queues the pair while cross-island pairs are being collected. It uses hkpBroadPhaseListener's
	// destructor and overrides both callbacks. The constructor of the simulation stores m_simulation.
	class MtEntityEntityBroadPhaseListener : public hkpBroadPhaseListener {
	public:
		void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
		void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

		hkpMultiThreadedSimulation* m_simulation;
	};

	// The listener the broad phase tells about pairs that involve a phantom; it guards the phantoms with
	// m_criticalSection (the simulation's m_phantomCriticalSection, stored by its constructor). It uses
	// hkpBroadPhaseListener's destructor and overrides both callbacks.
	class MtPhantomBroadPhaseListener : public hkpBroadPhaseListener {
	public:
		void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
		void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

		hkCriticalSection* m_criticalSection;
	};

	// The listener the broad phase tells about pairs with the world's border phantoms. The callbacks ignore this and
	// m_criticalSection, which the simulation's constructor sets to null.
	class MtBroadPhaseBorderListener : public hkpBroadPhaseListener {
	public:
		void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
		void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

		hkCriticalSection* m_criticalSection;
	};

	// The multithreaded simulation of world.
	hkpMultiThreadedSimulation(hkpWorld* world);
	~hkpMultiThreadedSimulation() override;

	void assertThereIsNoCollisionInformationForEntities(hkpEntity** entities, int32_t numEntities,
		hkpWorld* world) override;
	void assertThereIsNoCollisionInformationForAgent(hkpAgentNnEntry* agent) override;

protected:
	void collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(hkpEntity** entities, int32_t numEntities,
		const hkpProcessCollisionInput& input,
		hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection) override;
	void collideEntitiesNeedingPsiCollisionDetectionNarrowPhase_toiOnly(const hkpProcessCollisionInput& input,
		hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection) override;

public:
	virtual void getMultithreadConfig(hkpMultithreadConfig& config) const;
	virtual void setMultithreadConfig(const hkpMultithreadConfig& config, hkJobQueue* queue);

	// KEEP (DS832): the exe builds the integrate job on its stack writing only some of its 0x30 bytes (the word at
	// +4 says 0x30; bytes 3, 8-0xF, 0x12-0x13 and 0x24-0x2F are never written) and hkJobQueue::addJob (0xD04B90)
	// copies all of them into the queue: an uninitialised read src cannot reproduce.
	// Starts the step in the job queue and returns the step result.
	hkpStepResult stepBeginSt(hkJobQueue* jobQueue, float physicsDeltaTime);
	// Appends a toi event from the output's toi record under criticalSection.
	void addToiEventWithCriticalSectionLock(const hkpProcessCollisionOutput& result, const hkpAgentNnEntry& entry,
		hkCriticalSection* criticalSection);
	void processAgentNnEntriesFromMultipleIslands(hkpAgentNnEntry** allEntries, int32_t numAllEntries,
		hkpIslandsAgentEntriesInfo* islandEntriesInfos, int32_t numInfos, const hkpProcessCollisionInput& input,
		hkBool forceManifoldQualityType);
	hkpStepResult finishMtStep(hkJobQueue* jobQueue, hkJobThreadPool* threadPool);
	// Collides the entries in agent-sector jobs on the world's job queue, or through
	// hkpContinuousSimulation::processAgentNnEntries when there is no queue or too few entries. It is a plain public
	// member, not a virtual override: it hides the base's.
	void processAgentNnEntries(hkpAgentNnEntry** entries, int32_t numEntries,
		const hkpProcessCollisionInput& collisionInput, hkpSimulationIsland* island, hkBool forceManifoldQualityType);

	// Runs a dynamics job by its sub type.
	static hkJobQueue::JobStatus processNextJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& job);

	MtEntityEntityBroadPhaseListener m_entityEntityBroadPhaseListener;
	MtPhantomBroadPhaseListener m_phantomBroadPhaseListener;
	MtBroadPhaseBorderListener m_broadPhaseBorderListener;
	hkBool m_crossIslandPairsCollectingActive;
	hkArray<hkpTypedBroadPhaseHandlePair> m_addedCrossIslandPairs;
	hkCriticalSection m_addCrossIslandPairCriticalSection;
	hkArray<hkpTypedBroadPhaseHandlePair> m_removedCrossIslandPairs;
	hkCriticalSection m_removeCrossIslandPairCriticalSection;
	hkpMultithreadConfig m_multithreadConfig;
	// Written by stepBeginSt, read by finishMtStep.
	int32_t m_numActiveIslandsAtBeginningOfStep;
	int32_t m_numInactiveIslandsAtBeginningOfStep;
	// Cleared by the constructor, tested and read by processAgentNnEntriesFromMultipleIslands.
	hkJobQueue* m_jobQueueHandleForToiSolve;
	// Both critical sections sit at 64-byte boundaries.
	alignas(64) hkCriticalSection m_toiQueueCriticalSection;
	alignas(64) hkCriticalSection m_phantomCriticalSection;
};

// The post-collide job of one island: the constraint info and the queued physics commands of the pairs its narrow phase
// collided, one JobInfo block per task.
class hkpPostCollideJob : public hkpDynamicsJob {
public:
	hkpAgentSectorHeader* m_header;

	// The job is padded to the base hkpDynamicsJob's 16-byte alignment; no member follows m_header.
};
