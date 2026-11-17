#pragma once

#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverElemTemp.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkStatisticsCollector;
class hkUnionFind;
class hkpAction;
class hkpEntity;
class hkpWorld;

extern hkClass hkpSimulationIslandClass;

// A group of entities the world simulates and deactivates together. Its code stays with the exe.
class hkpSimulationIsland : public hkpConstraintOwner {
public:
	hkpSimulationIsland(hkpWorld* apWorld);
	~hkpSimulationIsland() override;

	// The SDK's inline body; the exe's out-of-line copy is 0xD078D0.
	const hkClass* getClassType() const override { return &hkpSimulationIslandClass; }
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,string,vtable).
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

protected:
	void addConstraintToCriticalLockedIsland(hkpConstraintInstance*) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void removeConstraintFromCriticalLockedIsland(hkpConstraintInstance* constraint) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void addCallbackRequest(hkpConstraintInstance* constraint, int32_t request) override;

public:
	// Whether the entities' collision entries, constraints and actions join the island into one connected set, using
	// checkConnectivityOut.
	bool isFullyConnected(hkUnionFind& checkConnectivityOut);
	void internalRemoveEntity(hkpEntity* entity);

	// The SDK's inline getter. The exe's out-of-line copy is the shared one-field getter 0x624700 (this + 0x48).
	const hkArray<hkpEntity*>& getEntities() const { return m_entities; }
	const hkArray<hkpAction*>& getActions() const { return m_actions; }
	hkpWorld* getWorld() { return m_world; }
	int32_t getStorageIndex() { return m_storageIndex; }

	// Havok SDK (protected there).
	void internalAddEntity(hkpEntity* entity);
	void addAction(hkpAction* act);
	void removeAction(hkpAction* act);
	void markForWrite();
	void mergeConstraintInfo(hkpSimulationIsland& other);

	// The rest of the SDK's members. The hkBool results come back through a hidden pointer.
	// The SDK's inline bodies follow; the exe has no out-of-line copy of them.
	hkBool* wasActiveLastFrame(hkBool* arResult) const {
		arResult->m_bool = m_isInActiveIslandsArray != 0;
		return arResult;
	}

	// Whether the island is the world's fixed island (no storage index: HK_INVALID_OBJECT_INDEX).
	hkBool* isFixed(hkBool* arResult) const {
		arResult->m_bool = m_storageIndex == 0xFFFF;
		return arResult;
	}

	// The solver buffer the island's integration needs: the velocity accumulators, the element temps rounded up to
	// 16 bytes and the schemas, with the multithreaded build's extra tasks (each adding an end schema and three
	// element temps). The exe inlines it in hkpWorld::getMemUsageForIntegration (0xC92B50): 80 constraints per task,
	// 4-byte element temps, 0x80-byte accumulators, 16-byte end schemas.
	int32_t getMemUsageForIntegration() {
		// HK_SIZE_OF_JACOBIAN_END_SCHEMA.
		const int32_t iEndSchemaSize = 16;
		int32_t sizeForElemTemp = m_constraintInfo.m_numSolverElemTemps * int32_t(sizeof(hkpSolverElemTemp)) +
			2 * int32_t(sizeof(hkpSolverElemTemp));
		int32_t sizeForAccumulators = getEntities().m_size * int32_t(sizeof(hkpVelocityAccumulator)) +
			int32_t(sizeof(hkpVelocityAccumulator)) + 16;
		int32_t sizeForSchemas = m_constraintInfo.m_sizeOfSchemas + iEndSchemaSize;
		{
			const int32_t minNumConstraintsPerTask = 80;
			int32_t extraTasksDueToMt = (m_numConstraints / minNumConstraintsPerTask) + 1;
			extraTasksDueToMt += 3;
			extraTasksDueToMt *= 2;
			sizeForSchemas += iEndSchemaSize * extraTasksDueToMt;
			sizeForElemTemp += extraTasksDueToMt * 12;
		}
		// HK_NEXT_MULTIPLE_OF(16, sizeForElemTemp).
		return sizeForAccumulators + ((sizeForElemTemp + 15) & ~15) + sizeForSchemas;
	}

	bool isFullyConnected();
	// The SDK's m_multiThreadCheck.unmarkForWrite(): empty, as the game's Havok has no multithreading debug checks
	// (hkpWorld::unmarkForWrite is the empty function 0x483710).
	void unmarkForWrite() {}
	void isValid();
	// Empty without HK_DEBUG_MULTI_THREADING, as in the game.
	void markAllEntitiesReadOnly() const {}
	void unmarkAllEntitiesReadOnly() const {}
	hkMultiThreadCheck& getMultiThreadCheck() { return m_multiThreadCheck; }

	hkpWorld* m_world;
	int32_t m_numConstraints;
	// The island's index in the world's island array; 0xFFFF for an island not in one.
	uint16_t m_storageIndex;
	uint16_t m_dirtyListIndex;
	uint8_t m_splitCheckFrameCounter;
	// The PDB's `bool m_x : 2` bit fields, two bits each from bit 0 up. The exe reads and writes both bits of each
	// (tests `& 3`, stores 0 or 1 into the pair), so they are 2-bit unsigned fields here.
	uint8_t m_splitCheckRequested : 2;
	uint8_t m_sparseEnabled : 2;
	uint8_t m_actionListCleanupNeeded : 2;
	uint8_t m_allowIslandLocking : 2;
	uint8_t m_isInActiveIslandsArray : 2;
	uint8_t m_activeMark : 2;
	uint8_t m_tryToIncreaseIslandSizeMark : 2;
	uint8_t m_inIntegrateJob : 2;
	uint8_t padding27[1];
	hkMultiThreadCheck m_multiThreadCheck;
	float m_timeSinceLastHighFrequencyCheck;
	float m_timeSinceLastLowFrequencyCheck;
	hkArray<hkpAction*> m_actions;
	float m_timeOfDeactivation;
	hkInplaceArray<hkpEntity*, 1> m_entities;
	hkpAgentNnTrack m_agentTrack;
};

// A second exe copy of hkpConstraintSolverSetup::calcBufferSize (0xD889F0), with no caller in the exe (cdecl, ret 0).
int32_t calcBufferSizeForSolve(hkpSimulationIsland& island);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
void hkCpuSplitSimulationIslandJobImpl(hkpSimulationIsland* island,
	hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& newSplitIslands);

inline constexpr fnvd::Vtable hkpSimulationIsland_vftable{};
