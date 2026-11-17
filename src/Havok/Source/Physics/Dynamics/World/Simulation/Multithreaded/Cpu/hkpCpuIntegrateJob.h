#pragma once

#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"

class hkConstraintInternal;
template <typename T> class hkLocalArray;
class hkpJacobianSchema;
struct hkpMtThreadStructure;
class hkpSolverElemTemp;

// A constraint going into a build-jacobians task and its two entities' indices (8-byte elements of the task's
// local array, 0xD94EC0). The PDB's `anonymous-namespace'::hkConstraintInternalInfo of hkpCpuIntegrateJob.cpp,
// named at namespace scope here (src has no file-local types).
struct hkConstraintInternalInfo {
	hkConstraintInternal* m_internal;
	uint16_t m_entityAIndex;
	uint16_t m_entityBIndex;
};

// A constraint whose callbacks fire after its task is built: the info and the task's atom info for the constraint
// (16-byte elements, 0xD94EC0 writes +0 to +8). The PDB's `anonymous-namespace'::hkConstraintInternalCallbackInfo
// (m_atomInfo is hkpBuildJacobianTask::AtomInfo*, sizeof 16).
// unk0C pads to the exe's 16-byte stride.
struct hkConstraintInternalCallbackInfo : public hkConstraintInternalInfo {
	hkpBuildJacobianTask::AtomInfo* m_atomInfo;
	uint8_t unk0C[4];
};

// Havok SDK (the exe's name ?createAndAppendNewTask@@YAXPBUhkpBuildJacobianTaskHeader@@...,
// the PDB's hkpCpuIntegrateJob.cpp, where it is defined). Usercall (0xD94EC0): outBuildCollection in eax (its first
// task and count at +0 and +4), the other 15 arguments on the stack in order, ret 0.
void createAndAppendNewTask(const hkpBuildJacobianTaskHeader* taskHeader,
	hkpBuildJacobianTaskCollection* outBuildCollection, hkpSolveJacobiansTaskCollection* outSolveCollection,
	hkpBuildJacobianTask*& prevBuildTask, hkpSolveConstraintBatchTask*& prevSolveTask,
	hkpSolveConstraintBatchTask*& firstTaskOfPreviousBatch, hkpSolveConstraintBatchTask*& firstTaskOfCurrentBatch,
	hkpJacobianSchema*& schemas, hkpSolverElemTemp*& elemTemp,
	hkLocalArray<hkConstraintInternalInfo>& constraintsForThisTask, int32_t& batchIndexOfLastTask,
	int32_t currentBatchIndex, uint16_t*& ptrToBatchSize, hkBool putOnPpuOnlyBuildJacobianTaskList,
	hkBool generateSolveTasks,
	hkArray<hkConstraintInternalCallbackInfo, hkContainerHeapAllocator>& callbackConstraints);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,string).
void createBuildJacobianAndSolveJacobianTaskCollection(hkpSimulationIsland* island,
	hkBool forceCoherentConstraintOrderingInSolver, hkpBuildJacobianTaskHeader* taskHeader,
	hkpBuildJacobianTaskCollection* outBuildCollection, hkpSolveJacobiansTaskCollection* outSolveCollection);
// The SDK's integrateJob: the integrate job of one island (KEEP body, 0xD95CF0). Defined in hkpCpuIntegrateJob.cpp.
hkJobQueue::JobStatus integrateJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& nextJobOut);
// Usercall (0xD94EB0): the hidden result pointer in eax, ret 0.
hkBool* canSolveOnSingleProcessor(hkBool* result);
