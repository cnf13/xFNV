#pragma once

#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"

class hkConstraintInternal;
class hkpJacobianSchema;
struct hkpMtThreadStructure;
class hkpSolverElemTemp;

template <typename T>
class hkLocalArray;

// A constraint going into a build-jacobians task and its two entities' indices, the elements of the task's local array.
struct hkConstraintInternalInfo {
    hkConstraintInternal* m_internal;
    uint16_t m_entityAIndex;
    uint16_t m_entityBIndex;
};

// A constraint whose callbacks fire after its task is built: the info and the task's atom info for the constraint.
struct hkConstraintInternalCallbackInfo : public hkConstraintInternalInfo {
    hkpBuildJacobianTask::AtomInfo* m_atomInfo;
    uint8_t unk0C[4];
};

void createAndAppendNewTask(
    const hkpBuildJacobianTaskHeader* taskHeader,
    hkpBuildJacobianTaskCollection* outBuildCollection,
    hkpSolveJacobiansTaskCollection* outSolveCollection,
    hkpBuildJacobianTask*& prevBuildTask,
    hkpSolveConstraintBatchTask*& prevSolveTask,
    hkpSolveConstraintBatchTask*& firstTaskOfPreviousBatch,
    hkpSolveConstraintBatchTask*& firstTaskOfCurrentBatch,
    hkpJacobianSchema*& schemas,
    hkpSolverElemTemp*& elemTemp,
    hkLocalArray<hkConstraintInternalInfo>& constraintsForThisTask,
    int32_t& batchIndexOfLastTask,
    int32_t currentBatchIndex,
    uint16_t*& ptrToBatchSize,
    hkBool putOnPpuOnlyBuildJacobianTaskList,
    hkBool generateSolveTasks,
    hkArray<hkConstraintInternalCallbackInfo, hkContainerHeapAllocator>& callbackConstraints
);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,string).
void createBuildJacobianAndSolveJacobianTaskCollection(
    hkpSimulationIsland* island,
    hkBool forceCoherentConstraintOrderingInSolver,
    hkpBuildJacobianTaskHeader* taskHeader,
    hkpBuildJacobianTaskCollection* outBuildCollection,
    hkpSolveJacobiansTaskCollection* outSolveCollection
);
// The integrate job of one island. Defined in hkpCpuIntegrateJob.cpp.
hkJobQueue::JobStatus
integrateJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
hkBool* canSolveOnSingleProcessor(hkBool* result);
