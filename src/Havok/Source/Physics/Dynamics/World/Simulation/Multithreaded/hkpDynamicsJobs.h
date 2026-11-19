#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommandQueue.h"

#include <stdint.h>

class hkConstraintInternal;
struct hkpBuildJacobianTaskHeader;
struct hkpConstraintAtom;
class hkpConstraintInstance;
class hkpConstraintQueryIn;
class hkpEntity;
struct hkpImpulseLimitBreachedHeader;
class hkpJacobianSchema;
struct hkpMtThreadStructure;
class hkpSimulationIsland;
class hkpSolverElemTemp;
class hkpVelocityAccumulator;
class hkTransform;

// One island's Jacobians to build: the accumulators, the schemas the builders write into, and one entry per constraint
// to build.
struct hkpBuildJacobianTask {
    // 144 for 4-byte pointers, 80 for 8-byte pointers.
    enum : int32_t {
#if XFNV_64BIT
        MAX_NUM_ATOM_INFOS = 80,
#else
        MAX_NUM_ATOM_INFOS = 0x90,
#endif
    };

    // One constraint to build: its atoms and instance, the runtime the builders write, the two accumulators'
    // transforms and the accumulator slots they live in.
    struct AtomInfo {
        hkpConstraintAtom* m_atoms;
        hkpConstraintInstance* m_instance;
        void* m_runtime;
        const hkTransform* m_transformA;
        const hkTransform* m_transformB;
        uint16_t m_atomsSize;
        uint16_t m_runtimeSize;
        uint16_t m_accumulatorIndexA;
        uint16_t m_accumulatorIndexB;
    };

    hkpBuildJacobianTask* m_next;
    hkpBuildJacobianTaskHeader* m_taskHeader;
    hkpVelocityAccumulator* m_accumulators;
    hkpJacobianSchema* m_schemas;
    hkpJacobianSchema* m_schemasOfNextTask;
    int32_t m_numAtomInfos;
    AtomInfo m_atomInfos[MAX_NUM_ATOM_INFOS];
    // The struct is 4096 bytes: m_atomInfos ends at 0xFD8 and no member follows it, so these are its tail padding. Only
    // the fields above are written.
    uint8_t paddingFD8[40];
};

// What every dynamics job adds to hkJob: the island it runs on, the task header it shares with the island's other jobs,
// and the thread structure of the thread that runs it. It has no vtable: the job queue dispatches a job through the
// handler table of its type. The class is 16-byte aligned, so the jobs built on it have sizes that are multiples of 16.
class alignas(16) hkpDynamicsJob : public hkJob {
public:
    // The m_jobSubType values of the dynamics jobs (hkpdynamics.pdb), the indices of the job type's process
    // function table.
    enum JobSubType : int32_t {
        DYNAMICS_JOB_INTEGRATE = 0,
        DYNAMICS_JOB_BUILD_ACCUMULATORS = 1,
        DYNAMICS_JOB_CREATE_JACOBIAN_TASKS = 2,
        DYNAMICS_JOB_FIRE_JACOBIAN_SETUP_CALLBACK = 3,
        DYNAMICS_JOB_BUILD_JACOBIANS = 4,
        DYNAMICS_JOB_SPLIT_ISLAND = 5,
        DYNAMICS_JOB_SOLVE_CONSTRAINTS = 6,
        DYNAMICS_JOB_SOLVE_APPLY_GRAVITY = 7,
        DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH = 8,
        DYNAMICS_JOB_SOLVE_INTEGRATE_VELOCITIES = 9,
        DYNAMICS_JOB_SOLVE_EXPORT_RESULTS = 10,
        DYNAMICS_JOB_INTEGRATE_MOTION = 11,
        DYNAMICS_JOB_BROADPHASE = 12,
        DYNAMICS_JOB_AGENT_SECTOR = 13,
        DYNAMICS_JOB_POST_COLLIDE = 14,
        DYNAMICS_JOB_AGENT_NN_ENTRY = 15,
        DYNAMICS_JOB_DUMMY = 16,
        DYNAMICS_JOB_END = 17,
    };

    // The tag of the constructors that build a job from no source job (hkpdynamics.pdb).
    enum NoJob : int32_t {
        NO_SRC_JOB = 0,
    };

    uint16_t m_islandIndex;
    uint8_t padding12[2];
    hkpSimulationIsland* m_island;
    hkpBuildJacobianTaskHeader* m_taskHeader;
    hkpMtThreadStructure* m_mtThreadStructure;
};

// One batch of constraints to solve: the accumulators, schemas and solver temporaries it owns. The struct is 16-byte
// aligned so that its size is 48 bytes while the members end at 0x24, because the task is allocated and freed as a
// 48-byte block.
struct alignas(16) hkpSolveConstraintBatchTask {
    hkpSolveConstraintBatchTask* m_next;
    hkpBuildJacobianTaskHeader* m_taskHeader;
    hkpVelocityAccumulator* m_accumulators;
    hkpJacobianSchema* m_schemas;
    hkpSolverElemTemp* m_solverElemTemp;
    int32_t m_sizeOfSchemaBuffer;
    int32_t m_sizeOfSolverElemTempBuffer;
    hkBool m_isLastTaskInBatch;
    uint16_t m_sizeOfNextBatch;
    hkpSolveConstraintBatchTask* m_firstTaskInNextBatch;
};

// The scratch types of the code that sorts an island's constraints into batches of constraints sharing no body
// (hkpdynamics.pdb: a namespace, no class record; the SDK ships no header for it).
namespace hkBuildConstraintBatches {

    // One constraint to place: its internal, its two bodies' indices and the batch it went to.
    struct Constraint {
        hkConstraintInternal* m_internal;
        uint16_t m_bodyA;
        uint16_t m_bodyB;
        uint8_t m_batchIndex;
    };

    // One batch: whether it starts a new batch, how many constraints it holds and where they are.
    struct Header {
        hkBool m_startNewBatch;
        int32_t m_numConstraints;
        Constraint* m_firstConstraint;
        Constraint* m_currentConstraint;
    };

} // namespace hkBuildConstraintBatches

// Solves one task's constraint batch over the solver's micro steps.
class hkpSolveConstraintBatchJob : public hkpDynamicsJob {
public:
    // Unsigned because hkpJobQueueUtils::finishIntegrateJob compares the micro steps unsigned.
    uint32_t m_currentSolverStep;
    uint32_t m_numSolverMicroSteps;
    uint32_t m_currentSolverMicroStep;
    hkpSolveConstraintBatchTask* m_solveConstraintBatchTask;
};

// Builds the Jacobians of one island's constraint batch. m_constraintQueryIn is a pointer, of the same type as
// hkpBuildJacobianTaskHeader::m_constraintQueryIn, which spawnBuildJacobiansJobs copies into it.
class hkpBuildJacobiansJob : public hkpDynamicsJob {
public:
    hkpBuildJacobianTask* m_buildJacobianTask;
    const hkpConstraintQueryIn* m_constraintQueryIn;
    hkBool m_finishSchemasWithGoto;
};

// Integrates the motion of an island's entities: the job carries how many islands it still has to do. Its size is
// padded to the base's 16-byte alignment, and no member follows m_numIslands.
class hkpIntegrateJob : public hkpDynamicsJob {
public:
    int32_t m_numIslands;
};

// The island's Jacobian tasks and the constraints whose builders report to a callback: the list the build job walks.
struct hkpBuildJacobianTaskCollection {
    // One callback constraint and the atom info its builder fills.
    struct CallbackPair {
        const hkConstraintInternal* m_callbackConstraints;
        hkpBuildJacobianTask::AtomInfo* m_atomInfo;
    };

    hkpBuildJacobianTask* m_buildJacobianTasks;
    int32_t m_numBuildJacobianTasks;
    CallbackPair* m_callbackConstraints;
    int32_t m_numCallbackConstraints;
};

// The first of the island's batch tasks to solve and how many constraints that batch holds.
struct hkpSolveJacobiansTaskCollection {
    hkpSolveConstraintBatchTask* m_firstSolveJacobiansTask;
    uint16_t m_firstBatchSize;
};

// What every job of one island shares: the open-job counters, the buffers the builder and the solver carve up, the
// accumulator, schema and solver-temporary bases, the query the builders read, the split islands and the two task
// lists. m_accumulatorsBase is 16-byte aligned.
struct hkpBuildJacobianTaskHeader {
    int32_t m_openJobs;
    int32_t m_numUnfinishedJobsForBroadphase;
    int32_t m_islandShouldBeDeactivated;
    int32_t m_referenceCount;
    int32_t m_numIslandsAfterSplit;
    void* m_buffer;
    int32_t m_bufferSize;
    int32_t m_bufferCapacity;
    // hkChar (signed char).
    int8_t m_exportFinished;
    int8_t m_splitCheckRequested;
    hkpImpulseLimitBreachedHeader* m_impulseLimitsBreached;
    alignas(16) hkpVelocityAccumulator* m_accumulatorsBase;
    hkpVelocityAccumulator* m_accumulatorsEnd;
    uint16_t m_numApplyGravityJobs;
    uint16_t m_numIntegrateVelocitiesJobs;
    hkpJacobianSchema* m_schemasBase;
    hkpSolverElemTemp* m_solverTempBase;
    int32_t m_numSolverResults;
    int32_t m_numSolverElemTemps;
    const hkpConstraintQueryIn* m_constraintQueryIn;
    hkpEntity* const* m_allEntities;
    uint16_t m_numAllEntities;
    uint16_t m_entitiesCapacity;
    hkBool m_solveInSingleThread;
    hkBool m_solveInSingleThreadOnPpuOnly;
    hkArray<hkpSimulationIsland*> m_newSplitIslands;
    hkpBuildJacobianTaskCollection m_tasks;
    hkpSolveJacobiansTaskCollection m_solveTasks;
};

void uploadOpenJobsVariable(
    const hkpBuildJacobianTaskHeader* localTaskHeader,
    const hkpBuildJacobianTaskHeader* taskHeaderInMainMemory
);

// The tasks of one agent job, in one allocation: the header, then m_numTotalTasks pointers to the tasks' JobInfo
// blocks. Allocation sets m_numTotalTasks and m_openJobs to numTasks; deallocation frees m_numTotalTasks blocks of
// m_sizeOfJobInfo bytes and then the header.
struct hkpAgentSectorHeader {
    // One task's block: the constraint info of the pairs it collided and the queue of the physics commands they
    // asked for, whose commands follow the block's first 0x20 bytes (hkpdynamics.pdb, sizeof 32).
    struct JobInfo {
        hkpConstraintInfo m_constraintInfo;
        hkpPhysicsCommandQueue m_commandQueue;
        // No member follows m_commandQueue; this padding is neither written nor read, and allocate sets
        // m_commandQueue.m_start to just past the end of this struct.
        uint8_t padding1C[4];
    };

    // Frees every task block and then the header itself.
    void deallocate();

    // The task pointers that follow the header.
    JobInfo** getJobInfos() {
        return reinterpret_cast<JobInfo**>(this + 1);
    }

    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    static hkpAgentSectorHeader* allocate(int32_t numTasks, int32_t numAgentNnEntriesPerTask);

    int32_t m_numTotalTasks;
    int32_t m_openJobs;
    int32_t m_sizeOfJobInfo;
};

// What the agent jobs (sector and entry) share: the task header, which task this job is of how many, the elements
// (agent sectors or agent entries) it collides and the step.
class hkpAgentBaseJob : public hkpDynamicsJob {
public:
    hkpAgentSectorHeader* m_header;
    uint16_t m_taskIndex;
    uint16_t m_numTotalTasks;
    uint16_t m_numElements;
    uint16_t m_numElementsPerTask;
    void* const* m_elements;
    hkStepInfo m_stepInfo;
};

// Collides a range of an island's agent sectors. m_sectorIndex indexes the island's sectors.
class hkpAgentSectorJob : public hkpAgentBaseJob {
public:
    int32_t m_sectorIndex;
    uint16_t m_bytesUsedInLastSector;
};

// Collides a list of agent entries (hkpdynamics.pdb, sizeof 80; 0x41..0x50 is the base's 16-byte alignment).
class hkpAgentNnEntryJob : public hkpAgentBaseJob {
public:
    hkBool m_forceManifoldQualityType;
};

// Runs the broad phase of the islands a step integrated (hkpdynamics.pdb, sizeof 48).
class hkpBroadPhaseJob : public hkpDynamicsJob {
public:
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    hkJobQueue::JobPopFuncResult
    popJobTask(hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& islands, hkpBroadPhaseJob& out);

    uint16_t m_numIslands;
};

// Builds the velocity accumulators of a range of an island's entities (hkpdynamics.pdb, sizeof 48).
class hkpBuildAccumulatorsJob : public hkpDynamicsJob {
public:
    // The entities one job takes (hkpdynamics.pdb).
    enum {
        ACCUMULATORS_PER_JOB = 128,
    };

    hkpEntity* const* m_islandEntitiesArray;
    uint16_t m_firstEntityIdx;
    uint16_t m_numEntities;
};

// Splits an island's constraints into Jacobian build tasks; it adds no member to the accumulator job it follows
// (hkpdynamics.pdb, sizeof 48).
class hkpCreateJacobianTasksJob : public hkpBuildAccumulatorsJob {};

// Fires the constraints' Jacobian setup callbacks; it adds no member (hkpdynamics.pdb, sizeof 32).
class hkpFireJacobianSetupCallback : public hkpDynamicsJob {};

// Splits an island that the solver found to have fallen apart; it adds no member (hkpdynamics.pdb, sizeof 32).
class hkpSplitSimulationIslandJob : public hkpDynamicsJob {};

// Writes the solver's results back to the constraints and reports breached impulse limits.
class hkpSolveExportResultsJob : public hkpSplitSimulationIslandJob {
public:
    hkpSolveConstraintBatchTask* m_solveConstraintBatchTask;
    hkpImpulseLimitBreachedHeader* m_impulseLimitsBreachedPadding;
    int32_t m_numImpulseLimitsBreachedPadding;
};

// Integrates the motions of a range of an island's entities. The integrate-motion job reads m_numEntities,
// m_firstEntityIdx and m_applyForcesAndStepMotionOnly, and writes m_numInactiveFrames.
class hkpIntegrateMotionJob : public hkpSolveExportResultsJob {
public:
    // The entities one job takes (hkpdynamics.pdb).
    enum {
        ACCUMULATORS_PER_JOB = 128,
    };

    void* m_buffer;
    uint16_t m_numEntities;
    uint16_t m_firstEntityIdx;
    int32_t m_numInactiveFrames;
    hkBool m_applyForcesAndStepMotionOnly;
};

// Applies gravity to a range of an island's accumulators (hkpdynamics.pdb, sizeof 48).
class hkpSolveApplyGravityJob : public hkpDynamicsJob {
public:
    // The accumulators one job takes (hkpdynamics.pdb).
    enum {
        MAX_NUM_ACCUMULATORS_FOR_APPLY_GRAVITY_JOB = 128,
    };

    hkpVelocityAccumulator* m_accumulators;
    hkpVelocityAccumulator* m_accumulatorsEnd;
};

// Solves an island's constraints in one job: the buffer and the offsets of the accumulators, schemas and solver
// temporaries in it (hkpdynamics.pdb, sizeof 64).
class hkpSolveConstraintsJob : public hkpDynamicsJob {
public:
    void* m_buffer;
    uint32_t m_bufferSize;
    uint32_t m_accumulatorsOffset;
    uint32_t m_schemasOffset;
    uint32_t m_solverTempOffset;
    int32_t m_numSolverResults;
    int32_t m_numSolverElemTemps;
};

// Integrates the velocities of a range of an island's accumulators after a solver step.
class hkpSolveIntegrateVelocitiesJob : public hkpDynamicsJob {
public:
    // The accumulators one job takes (hkpdynamics.pdb).
    enum {
        MAX_NUM_ACCUMULATORS_FOR_INTEGRATE_VELOCITIES_JOB = 128,
    };

    hkpVelocityAccumulator* m_accumulators;
    hkpVelocityAccumulator* m_accumulatorsEnd;
    uint32_t m_currentSolverStep;
    hkBool m_solvingFinished;
};
