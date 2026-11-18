#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuIntegrateJob.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverElemTemp.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Setup/hkpConstraintSolverSetup.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "SDK/Windows/winbase.h"

// Creates a new build-jacobian task from the constraints gathered for it and appends it to the build collection (to its
// head unless the task goes on the PPU-only list), filling in each constraint's atom info and adding the ones whose
// callbacks fire after the build to callbackConstraints; then, when asked, appends its solve task to the solve tasks,
// which closes the previous batch when this task starts a new one. schemas and elemTemp move past the task's schemas
// (and its end schema) and solver temps.
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
) {
    hkpJacobianSchema* const schemaStart = schemas;
    hkpSolverElemTemp* const elemTempStart = elemTemp;
    // EXE-INLINED none: hkpBuildJacobianTask::operator new and hkpBuildJacobianTask::hkpBuildJacobianTask
    hkpBuildJacobianTask* task = static_cast<hkpBuildJacobianTask*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpBuildJacobianTask))
    );
    if (task) {
        task->m_numAtomInfos = 0;
        task->m_schemasOfNextTask = nullptr;
        task->m_next = nullptr;
    }
    if (prevBuildTask) {
        prevBuildTask->m_next = task;
        prevBuildTask->m_schemasOfNextTask = schemas;
    }
    else if (!putOnPpuOnlyBuildJacobianTaskList.m_bool) {
        outBuildCollection->m_buildJacobianTasks = task;
        outBuildCollection->m_numBuildJacobianTasks = 0;
    }
    ++outBuildCollection->m_numBuildJacobianTasks;
    // The header parameter is const and the task's member is not, hence the cast.
    task->m_taskHeader = const_cast<hkpBuildJacobianTaskHeader*>(taskHeader);
    task->m_numAtomInfos = constraintsForThisTask.m_size;
    task->m_accumulators = taskHeader->m_accumulatorsBase;
    task->m_schemas = schemaStart;
    const int32_t numConstraints = constraintsForThisTask.m_size;
    for (int32_t i = 0; i < numConstraints; ++i) {
        const hkConstraintInternalInfo& info = constraintsForThisTask.m_data[i];
        const hkConstraintInternal* ci = info.m_internal;
        hkpBuildJacobianTask::AtomInfo& atomInfo = task->m_atomInfos[i];
        if (ci->m_callbackRequest &
            (hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
             hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY |
             hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK)) {
            hkConstraintInternalCallbackInfo* callbackInfo = callbackConstraints.expandByUnchecked(1);
            static_cast<hkConstraintInternalInfo&>(*callbackInfo) = info;
            callbackInfo->m_atomInfo = &atomInfo;
        }
        atomInfo.m_atoms = ci->m_atoms;
        atomInfo.m_atomsSize = ci->m_atomsSize;
        atomInfo.m_instance = ci->m_constraint;
        atomInfo.m_runtime = ci->m_runtime;
        atomInfo.m_runtimeSize = ci->m_runtimeSize;
        atomInfo.m_accumulatorIndexA = static_cast<uint16_t>(info.m_entityAIndex + 1);
        atomInfo.m_accumulatorIndexB = static_cast<uint16_t>(info.m_entityBIndex + 1);
        atomInfo.m_transformA = &ci->m_entities[0]->getMotion()->getTransform();
        atomInfo.m_transformB = &ci->m_entities[1]->getMotion()->getTransform();
        // The schema stream is untyped bytes: each constraint's schemas take m_sizeOfSchemas of them.
        schemas = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(schemas) + ci->m_sizeOfSchemas);
        elemTemp += ci->m_numSolverElemTemps;
    }
    constraintsForThisTask.clear();
    // The task's end schema.
    ++schemas;
    task->m_schemasOfNextTask = schemas;
    // HK_NEXT_MULTIPLE_OF(16, elemTemp): the address rounded up, untyped.
    elemTemp =
        reinterpret_cast<hkpSolverElemTemp*>((reinterpret_cast<hkUlong>(elemTemp) + 15) & ~static_cast<hkUlong>(15));
    prevBuildTask = task;
    if (generateSolveTasks.m_bool) {
        // EXE-INLINED none: hkpSolveConstraintBatchTask::operator new and
        // hkpSolveConstraintBatchTask::hkpSolveConstraintBatchTask
        hkpSolveConstraintBatchTask* solveTask = static_cast<hkpSolveConstraintBatchTask*>(
            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpSolveConstraintBatchTask))
        );
        if (solveTask) {
            solveTask->m_next = nullptr;
            solveTask->m_isLastTaskInBatch.m_bool = 0;
            solveTask->m_sizeOfNextBatch = 0;
            solveTask->m_firstTaskInNextBatch = nullptr;
        }
        if (prevSolveTask) {
            prevSolveTask->m_next = solveTask;
        }
        if (currentBatchIndex != batchIndexOfLastTask) {
            if (currentBatchIndex == 0) {
                outSolveCollection->m_firstSolveJacobiansTask = solveTask;
            }
            batchIndexOfLastTask = currentBatchIndex;
            if (prevSolveTask) {
                // The previous batch's tasks learn the size and first task of the batch that ends here.
                const uint16_t batchSize = *ptrToBatchSize;
                for (hkpSolveConstraintBatchTask* t = firstTaskOfPreviousBatch; t && t != firstTaskOfCurrentBatch;
                     t = t->m_next) {
                    t->m_sizeOfNextBatch = batchSize;
                    t->m_firstTaskInNextBatch = firstTaskOfCurrentBatch;
                }
                prevSolveTask->m_isLastTaskInBatch.m_bool = 1;
                ptrToBatchSize = &prevSolveTask->m_sizeOfNextBatch;
            }
            firstTaskOfPreviousBatch = firstTaskOfCurrentBatch;
            firstTaskOfCurrentBatch = solveTask;
        }
        ++*ptrToBatchSize;
        solveTask->m_taskHeader = const_cast<hkpBuildJacobianTaskHeader*>(taskHeader);
        solveTask->m_accumulators = taskHeader->m_accumulatorsBase;
        solveTask->m_schemas = schemaStart;
        solveTask->m_solverElemTemp = elemTempStart;
        solveTask->m_sizeOfSchemaBuffer =
            static_cast<int32_t>(reinterpret_cast<char*>(schemas) - reinterpret_cast<char*>(schemaStart));
        solveTask->m_sizeOfSolverElemTempBuffer =
            static_cast<int32_t>(reinterpret_cast<char*>(elemTemp) - reinterpret_cast<char*>(elemTempStart));
        prevSolveTask = solveTask;
    }
}

// Sorts the island's constraints into normal and high priority ones (with the island indices of their entities, 0xFFFF
// for an entity outside the island), then, pass by pass, packs them into build-jacobian tasks of at most
// MAX_NUM_ATOM_INFOS atoms, each followed by a solve task unless the island must be solved in a single thread. A pass
// takes a constraint only while neither of its entities belongs to another task of the same batch (entityGroup); the
// rest wait for the next pass, which starts a new batch. The constraints whose callbacks fire after the build are
// handed to outBuildCollection. Tasks are created in two places: for a full task inside the pass and for the pass's
// last task.
void createBuildJacobianAndSolveJacobianTaskCollection(
    hkpSimulationIsland* island,
    hkBool forceCoherentConstraintOrderingInSolver,
    hkpBuildJacobianTaskHeader* taskHeader,
    hkpBuildJacobianTaskCollection* outBuildCollection,
    hkpSolveJacobiansTaskCollection* outSolveCollection
) {
    HK_TIMER_BEGIN("BuildJacTask", nullptr);

    const int32_t numBodies = island->m_entities.getSize();
    hkpEntity* const* bodies = island->m_entities.begin();
    hkpEntity* const* const bodiesEnd = bodies + numBodies;

    // EXE-INLINED none: hkArray<hkConstraintInternalInfo, hkContainerTempAllocator>::hkArray
    using normalPriorityConstraints_t = hkArray<hkConstraintInternalInfo, hkContainerTempAllocator>;
    XFNV_INLINED_LOCAL(normalPriorityConstraints_t, normalPriorityConstraints, std::nullopt, std::nullopt);
    normalPriorityConstraints.m_data = nullptr;
    normalPriorityConstraints.m_size = 0;
    normalPriorityConstraints.m_capacityAndFlags = hkArrayBase<hkConstraintInternalInfo>::DONT_DEALLOCATE_FLAG;
    normalPriorityConstraints._reserve(hkMemoryRouter::getInstance().temp(), island->m_numConstraints);
    // EXE-INLINED none: hkArray<hkConstraintInternalInfo, hkContainerTempAllocator>::hkArray
    using highPriorityConstraints_t = hkArray<hkConstraintInternalInfo, hkContainerTempAllocator>;
    XFNV_INLINED_LOCAL(highPriorityConstraints_t, highPriorityConstraints, std::nullopt, std::nullopt);
    highPriorityConstraints.m_data = nullptr;
    highPriorityConstraints.m_size = 0;
    highPriorityConstraints.m_capacityAndFlags = hkArrayBase<hkConstraintInternalInfo>::DONT_DEALLOCATE_FLAG;
    highPriorityConstraints._reserve(hkMemoryRouter::getInstance().temp(), island->m_numConstraints);

    // The stack allocator's bytes are untyped memory; each local array keeps its elements there.
    XFNV_INLINED_LOCAL(hkLocalArray<hkConstraintInternalCallbackInfo>, callbackConstraints, std::nullopt, std::nullopt);
    {
        // EXE-INLINED none: hkLocalArray<hkConstraintInternalCallbackInfo>::hkLocalArray(island->m_numConstraints)
        // (hkAllocateStack)
        const int32_t capacity = island->m_numConstraints;
        callbackConstraints.m_data = nullptr;
        callbackConstraints.m_size = 0;
        callbackConstraints.m_capacityAndFlags = hkArrayBase<hkConstraintInternalCallbackInfo>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((capacity * static_cast<int32_t>(sizeof(hkConstraintInternalCallbackInfo)) + 127) & ~127);
        callbackConstraints.m_data = reinterpret_cast<hkConstraintInternalCallbackInfo*>(pBlock);
        callbackConstraints.m_capacityAndFlags =
            capacity | hkArrayBase<hkConstraintInternalCallbackInfo>::DONT_DEALLOCATE_FLAG;
        callbackConstraints.m_localMemory = callbackConstraints.m_data;
    }

    // Whether every constraint is of the normal type; else the island is solved in a single thread.
    int32_t canSolveInParallel = 1;
    {
        // The map from each entity to its index in the island, over a temp buffer of four words per slot.
        int32_t numSlots = 64;
        while (numSlots < numBodies) {
            numSlots *= 2;
        }
        const int32_t pointerBufferSize = 4 * numSlots;
        hkUlong* pointerBuffer = hkMemoryRouter::getInstance().temp()._blockAlloc<hkUlong>(pointerBufferSize);
        using entityPtrToIndex_t = hkUlongMapBase;
        XFNV_INLINED_LOCAL(entityPtrToIndex_t, entityPtrToIndex, std::nullopt, std::nullopt);
        // EXE-INLINED none: hkMap<hkUlong, hkUlong>::hkMap(pointerBuffer, size)
        entityPtrToIndex.init(pointerBuffer, pointerBufferSize * static_cast<int32_t>(sizeof(hkUlong)));
        // The map keys an entity by its address converted to the map's 32-bit storage integer (hkPointerMap::ToMapInt's
        // conversion).
        for (int32_t i = 0; i < numBodies; ++i) {
            entityPtrToIndex.insert(
                hkMemoryRouter::getInstance().heap(), static_cast<uint32_t>(reinterpret_cast<hkUlong>(bodies[i])),
                static_cast<uint32_t>(i)
            );
        }

        // Each entity's solver data is the byte offset of its accumulator (the first accumulator is the fixed one).
        uint32_t solverData = sizeof(hkpVelocityAccumulator);
        int32_t bodyIndex = 0;
        for (hkpEntity* const* pBody = bodies; pBody < bodiesEnd; ++pBody, ++bodyIndex) {
            hkpEntity* body = *pBody;
            if (body->m_solverData != solverData) {
                body->m_solverData = solverData;
            }
            solverData += sizeof(hkpVelocityAccumulator);
            hkConstraintInternal* ci = body->m_constraintsMaster.begin();
            hkConstraintInternal* const ciEnd = ci + body->m_constraintsMaster.getSize();
            for (; ci < ciEnd; ++ci) {
                // The body is the constraint's master; its slave's index comes from the map.
                const int32_t whoIsMaster = ci->m_whoIsMaster;
                hkConstraintInternalInfo info;
                info.m_internal = ci;
                const uint16_t slaveIndex = static_cast<uint16_t>(entityPtrToIndex.getWithDefault(
                    static_cast<uint32_t>(reinterpret_cast<hkUlong>(ci->m_entities[1 - whoIsMaster])), 0xFFFF
                ));
                if (whoIsMaster == 0) {
                    info.m_entityAIndex = static_cast<uint16_t>(bodyIndex);
                    info.m_entityBIndex = slaveIndex;
                }
                else {
                    info.m_entityBIndex = static_cast<uint16_t>(bodyIndex);
                    info.m_entityAIndex = slaveIndex;
                }
                canSolveInParallel &= (ci->m_isNormalType.m_bool != 0);
                hkArray<hkConstraintInternalInfo, hkContainerTempAllocator>& priorityConstraints =
                    (ci->m_priority.m_storage < hkpConstraintInstance::PRIORITY_TOI_HIGHER) ? normalPriorityConstraints
                                                                                            : highPriorityConstraints;
                // EXE-INLINED none: hkArray<hkConstraintInternalInfo>::pushBackUnchecked (its placement copy
                // tests the slot)
                hkConstraintInternalInfo* slot = priorityConstraints.m_data + priorityConstraints.m_size;
                if (slot) {
                    *slot = info;
                }
                ++priorityConstraints.m_size;
            }
        }
        // EXE-INLINED none: hkMap<hkUlong, hkUlong>::~hkMap (clearAndDeallocate through the heap, then the base's
        // empty destructor)
        entityPtrToIndex.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
        entityPtrToIndex.destructOperations();
        hkMemoryRouter::getInstance().temp()._blockFree(pointerBuffer, pointerBufferSize);
    }

    HK_MONITOR_ADD_VALUE("NumEntities", static_cast<float>(numBodies), HK_MONITOR_TYPE_INT);
    HK_MONITOR_ADD_VALUE("NumConstraints", static_cast<float>(island->m_numConstraints), HK_MONITOR_TYPE_INT);
    HK_MONITOR_ADD_VALUE(
        "NumJacobians", static_cast<float>(island->m_constraintInfo.m_numSolverResults), HK_MONITOR_TYPE_INT
    );

    taskHeader->m_solveInSingleThreadOnPpuOnly.m_bool = (canSolveInParallel == 0);
    taskHeader->m_solveInSingleThread.m_bool = (taskHeader->m_solveInSingleThreadOnPpuOnly.m_bool != 0);
    hkBool generateSolveTasks;
    generateSolveTasks.m_bool = (taskHeader->m_solveInSingleThread.m_bool == 0);

    // HK_SIZE_OF_JACOBIAN_END_SCHEMA: each task's schemas end with a 16-byte end schema.
    const int32_t endSchemaSize = 16;
    // The schema and solver temp buffers are untyped memory treated as bytes: the schemas are a byte stream of records
    // of varying sizes, advanced by byte counts, and a task's share of each buffer is measured in bytes; the solver
    // temps of a task start at a 16-byte aligned address.
    hkpJacobianSchema* schemas = taskHeader->m_schemasBase;
    hkpSolverElemTemp* elemTemp = taskHeader->m_solverTempBase;
    int32_t currentBatchIndex = -1;
    int32_t batchIndexOfLastTask = -1;
    uint16_t* ptrToBatchSize = &outSolveCollection->m_firstBatchSize;
    *ptrToBatchSize = 0;
    hkpBuildJacobianTask* prevBuildTask = nullptr;
    hkpSolveConstraintBatchTask* prevSolveTask = nullptr;
    hkpSolveConstraintBatchTask* firstTaskOfPreviousBatch = nullptr;
    hkpSolveConstraintBatchTask* firstTaskOfCurrentBatch = nullptr;

    XFNV_INLINED_LOCAL(hkLocalArray<hkConstraintInternalInfo>, constraintsForThisTask, std::nullopt, std::nullopt);
    {
        // EXE-INLINED none: hkLocalArray<hkConstraintInternalInfo>::hkLocalArray(MAX_NUM_ATOM_INFOS) (hkAllocateStack)
        const int32_t capacity = hkpBuildJacobianTask::MAX_NUM_ATOM_INFOS;
        constraintsForThisTask.m_data = nullptr;
        constraintsForThisTask.m_size = 0;
        constraintsForThisTask.m_capacityAndFlags = hkArrayBase<hkConstraintInternalInfo>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((capacity * static_cast<int32_t>(sizeof(hkConstraintInternalInfo)) + 127) & ~127);
        constraintsForThisTask.m_data = reinterpret_cast<hkConstraintInternalInfo*>(pBlock);
        constraintsForThisTask.m_capacityAndFlags =
            capacity | hkArrayBase<hkConstraintInternalInfo>::DONT_DEALLOCATE_FLAG;
        constraintsForThisTask.m_localMemory = constraintsForThisTask.m_data;
    }

    // Per entity, the group of the task of the current batch that holds one of its constraints (0: none yet).
    int32_t* entityGroup = hkMemoryRouter::getInstance().temp()._blockAlloc<int32_t>(numBodies);
    for (int32_t il = 0; il < 2; ++il) {
        hkArray<hkConstraintInternalInfo, hkContainerTempAllocator>* sourceConstraints;
        if (il) {
            sourceConstraints = &highPriorityConstraints;
        }
        else {
            sourceConstraints = &normalPriorityConstraints;
            prevBuildTask = nullptr;
        }
        while (sourceConstraints->getSize()) {
            for (int32_t i = 0; i < numBodies; ++i) {
                entityGroup[i] = 0;
            }
            ++currentBatchIndex;
            int32_t currentGroup = 1;
            hkConstraintInternalInfo* nextConstraintLeftOut = sourceConstraints->begin();
            hkConstraintInternalInfo* info = sourceConstraints->begin();
            hkConstraintInternalInfo* const infoEnd = info + sourceConstraints->getSize();
            for (; info < infoEnd; ++info) {
                // An entity outside the island (the index 0xFFFF) never blocks a constraint.
                int32_t fixedEntityGroup = 0;
                int32_t* groupA =
                    (info->m_entityAIndex == 0xFFFF) ? &fixedEntityGroup : &entityGroup[info->m_entityAIndex];
                int32_t* groupB =
                    (info->m_entityBIndex == 0xFFFF) ? &fixedEntityGroup : &entityGroup[info->m_entityBIndex];
                if (constraintsForThisTask.getSize() == hkpBuildJacobianTask::MAX_NUM_ATOM_INFOS) {
                    // EXE-INLINED 0xD94EC0
                    hkpJacobianSchema* const taskSchemas = schemas;
                    hkpSolverElemTemp* const taskElemTemp = elemTemp;
                    // EXE-INLINED none: hkpBuildJacobianTask::operator new and
                    // hkpBuildJacobianTask::hkpBuildJacobianTask
                    hkpBuildJacobianTask* task = static_cast<hkpBuildJacobianTask*>(
                        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpBuildJacobianTask))
                    );
                    if (task) {
                        task->m_next = nullptr;
                        task->m_schemasOfNextTask = nullptr;
                        task->m_numAtomInfos = 0;
                    }
                    if (prevBuildTask) {
                        prevBuildTask->m_next = task;
                        prevBuildTask->m_schemasOfNextTask = schemas;
                    }
                    else {
                        outBuildCollection->m_buildJacobianTasks = task;
                        outBuildCollection->m_numBuildJacobianTasks = 0;
                    }
                    ++outBuildCollection->m_numBuildJacobianTasks;
                    task->m_taskHeader = taskHeader;
                    task->m_numAtomInfos = constraintsForThisTask.getSize();
                    task->m_accumulators = taskHeader->m_accumulatorsBase;
                    task->m_schemas = schemas;
                    for (int32_t i = 0; i < constraintsForThisTask.getSize(); ++i) {
                        const hkConstraintInternalInfo& taskInfo = constraintsForThisTask.m_data[i];
                        const hkConstraintInternal* taskCi = taskInfo.m_internal;
                        hkpBuildJacobianTask::AtomInfo& atomInfo = task->m_atomInfos[i];
                        if (taskCi->m_callbackRequest &
                            (hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
                             hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY |
                             hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK)) {
                            hkConstraintInternalCallbackInfo* callbackInfo = callbackConstraints.expandByUnchecked(1);
                            static_cast<hkConstraintInternalInfo&>(*callbackInfo) = taskInfo;
                            callbackInfo->m_atomInfo = &atomInfo;
                        }
                        atomInfo.m_atoms = taskCi->m_atoms;
                        atomInfo.m_atomsSize = taskCi->m_atomsSize;
                        atomInfo.m_instance = taskCi->m_constraint;
                        atomInfo.m_runtime = taskCi->m_runtime;
                        atomInfo.m_runtimeSize = taskCi->m_runtimeSize;
                        atomInfo.m_accumulatorIndexA = static_cast<uint16_t>(taskInfo.m_entityAIndex + 1);
                        atomInfo.m_accumulatorIndexB = static_cast<uint16_t>(taskInfo.m_entityBIndex + 1);
                        atomInfo.m_transformA = &taskCi->m_entities[0]->getMotion()->getTransform();
                        atomInfo.m_transformB = &taskCi->m_entities[1]->getMotion()->getTransform();
                        schemas = reinterpret_cast<hkpJacobianSchema*>(
                            reinterpret_cast<char*>(schemas) + taskCi->m_sizeOfSchemas
                        );
                        elemTemp += taskCi->m_numSolverElemTemps;
                    }
                    constraintsForThisTask.clear();
                    schemas = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(schemas) + endSchemaSize);
                    task->m_schemasOfNextTask = schemas;
                    // HK_NEXT_MULTIPLE_OF(16, elemTemp).
                    elemTemp = reinterpret_cast<hkpSolverElemTemp*>(
                        (reinterpret_cast<hkUlong>(elemTemp) + 15) & ~static_cast<hkUlong>(15)
                    );
                    prevBuildTask = task;
                    if (generateSolveTasks.m_bool) {
                        // EXE-INLINED none: hkpSolveConstraintBatchTask::operator new and
                        // hkpSolveConstraintBatchTask::hkpSolveConstraintBatchTask
                        hkpSolveConstraintBatchTask* solveTask = static_cast<hkpSolveConstraintBatchTask*>(
                            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpSolveConstraintBatchTask))
                        );
                        if (solveTask) {
                            solveTask->m_next = nullptr;
                            solveTask->m_isLastTaskInBatch.m_bool = 0;
                            solveTask->m_sizeOfNextBatch = 0;
                            solveTask->m_firstTaskInNextBatch = nullptr;
                        }
                        if (prevSolveTask) {
                            prevSolveTask->m_next = solveTask;
                        }
                        if (currentBatchIndex != batchIndexOfLastTask) {
                            if (currentBatchIndex == 0) {
                                outSolveCollection->m_firstSolveJacobiansTask = solveTask;
                            }
                            batchIndexOfLastTask = currentBatchIndex;
                            if (prevSolveTask) {
                                // The previous batch's tasks learn the size and first task of the batch that ends here.
                                const uint16_t batchSize = *ptrToBatchSize;
                                for (hkpSolveConstraintBatchTask* t = firstTaskOfPreviousBatch;
                                     t && t != firstTaskOfCurrentBatch; t = t->m_next) {
                                    t->m_sizeOfNextBatch = batchSize;
                                    t->m_firstTaskInNextBatch = firstTaskOfCurrentBatch;
                                }
                                prevSolveTask->m_isLastTaskInBatch.m_bool = 1;
                                ptrToBatchSize = &prevSolveTask->m_sizeOfNextBatch;
                            }
                            firstTaskOfPreviousBatch = firstTaskOfCurrentBatch;
                            firstTaskOfCurrentBatch = solveTask;
                        }
                        ++*ptrToBatchSize;
                        solveTask->m_taskHeader = taskHeader;
                        solveTask->m_accumulators = taskHeader->m_accumulatorsBase;
                        solveTask->m_schemas = taskSchemas;
                        solveTask->m_solverElemTemp = taskElemTemp;
                        solveTask->m_sizeOfSchemaBuffer = static_cast<int32_t>(
                            reinterpret_cast<char*>(schemas) - reinterpret_cast<char*>(taskSchemas)
                        );
                        solveTask->m_sizeOfSolverElemTempBuffer = static_cast<int32_t>(
                            reinterpret_cast<char*>(elemTemp) - reinterpret_cast<char*>(taskElemTemp)
                        );
                        prevSolveTask = solveTask;
                    }
                    // The next task's constraints are a new group of the batch.
                    if (generateSolveTasks.m_bool) {
                        ++currentGroup;
                    }
                }
                if (forceCoherentConstraintOrderingInSolver.m_bool && currentGroup > 1) {
                    break;
                }
                if ((*groupA == 0 || *groupA == currentGroup) && (*groupB == 0 || *groupB == currentGroup)) {
                    // EXE-INLINED none: hkLocalArray<hkConstraintInternalInfo>::pushBackUnchecked (its placement
                    // copy tests the slot)
                    hkConstraintInternalInfo* slot = constraintsForThisTask.expandByUnchecked(1);
                    if (slot) {
                        *slot = *info;
                    }
                    *groupA = currentGroup;
                    *groupB = currentGroup;
                }
                else {
                    *nextConstraintLeftOut++ = *info;
                }
            }
            // A forced coherent order stops the pass at its second group: the rest waits for the next pass.
            for (; info < infoEnd; ++info) {
                *nextConstraintLeftOut++ = *info;
            }

            if (constraintsForThisTask.getSize()) {
                // EXE-INLINED 0xD94EC0
                hkpJacobianSchema* const taskSchemas = schemas;
                hkpSolverElemTemp* const taskElemTemp = elemTemp;
                // EXE-INLINED none: hkpBuildJacobianTask::operator new and hkpBuildJacobianTask::hkpBuildJacobianTask
                hkpBuildJacobianTask* task = static_cast<hkpBuildJacobianTask*>(
                    hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpBuildJacobianTask))
                );
                if (task) {
                    task->m_next = nullptr;
                    task->m_schemasOfNextTask = nullptr;
                    task->m_numAtomInfos = 0;
                }
                if (prevBuildTask) {
                    prevBuildTask->m_next = task;
                    prevBuildTask->m_schemasOfNextTask = schemas;
                }
                else {
                    outBuildCollection->m_buildJacobianTasks = task;
                    outBuildCollection->m_numBuildJacobianTasks = 0;
                }
                ++outBuildCollection->m_numBuildJacobianTasks;
                task->m_taskHeader = taskHeader;
                task->m_numAtomInfos = constraintsForThisTask.getSize();
                task->m_accumulators = taskHeader->m_accumulatorsBase;
                task->m_schemas = schemas;
                for (int32_t i = 0; i < constraintsForThisTask.getSize(); ++i) {
                    const hkConstraintInternalInfo& taskInfo = constraintsForThisTask.m_data[i];
                    const hkConstraintInternal* taskCi = taskInfo.m_internal;
                    hkpBuildJacobianTask::AtomInfo& atomInfo = task->m_atomInfos[i];
                    if (taskCi->m_callbackRequest &
                        (hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
                         hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY |
                         hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK)) {
                        hkConstraintInternalCallbackInfo* callbackInfo = callbackConstraints.expandByUnchecked(1);
                        static_cast<hkConstraintInternalInfo&>(*callbackInfo) = taskInfo;
                        callbackInfo->m_atomInfo = &atomInfo;
                    }
                    atomInfo.m_atoms = taskCi->m_atoms;
                    atomInfo.m_atomsSize = taskCi->m_atomsSize;
                    atomInfo.m_instance = taskCi->m_constraint;
                    atomInfo.m_runtime = taskCi->m_runtime;
                    atomInfo.m_runtimeSize = taskCi->m_runtimeSize;
                    atomInfo.m_accumulatorIndexA = static_cast<uint16_t>(taskInfo.m_entityAIndex + 1);
                    atomInfo.m_accumulatorIndexB = static_cast<uint16_t>(taskInfo.m_entityBIndex + 1);
                    atomInfo.m_transformA = &taskCi->m_entities[0]->getMotion()->getTransform();
                    atomInfo.m_transformB = &taskCi->m_entities[1]->getMotion()->getTransform();
                    schemas = reinterpret_cast<hkpJacobianSchema*>(
                        reinterpret_cast<char*>(schemas) + taskCi->m_sizeOfSchemas
                    );
                    elemTemp += taskCi->m_numSolverElemTemps;
                }
                constraintsForThisTask.clear();
                schemas = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(schemas) + endSchemaSize);
                task->m_schemasOfNextTask = schemas;
                // HK_NEXT_MULTIPLE_OF(16, elemTemp).
                elemTemp = reinterpret_cast<hkpSolverElemTemp*>(
                    (reinterpret_cast<hkUlong>(elemTemp) + 15) & ~static_cast<hkUlong>(15)
                );
                prevBuildTask = task;
                if (generateSolveTasks.m_bool) {
                    // EXE-INLINED none: hkpSolveConstraintBatchTask::operator new and
                    // hkpSolveConstraintBatchTask::hkpSolveConstraintBatchTask
                    hkpSolveConstraintBatchTask* solveTask = static_cast<hkpSolveConstraintBatchTask*>(
                        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpSolveConstraintBatchTask))
                    );
                    if (solveTask) {
                        solveTask->m_next = nullptr;
                        solveTask->m_isLastTaskInBatch.m_bool = 0;
                        solveTask->m_sizeOfNextBatch = 0;
                        solveTask->m_firstTaskInNextBatch = nullptr;
                    }
                    if (prevSolveTask) {
                        prevSolveTask->m_next = solveTask;
                    }
                    if (currentBatchIndex != batchIndexOfLastTask) {
                        if (currentBatchIndex == 0) {
                            outSolveCollection->m_firstSolveJacobiansTask = solveTask;
                        }
                        batchIndexOfLastTask = currentBatchIndex;
                        if (prevSolveTask) {
                            // The previous batch's tasks learn the size and first task of the batch that ends here.
                            const uint16_t batchSize = *ptrToBatchSize;
                            for (hkpSolveConstraintBatchTask* t = firstTaskOfPreviousBatch;
                                 t && t != firstTaskOfCurrentBatch; t = t->m_next) {
                                t->m_sizeOfNextBatch = batchSize;
                                t->m_firstTaskInNextBatch = firstTaskOfCurrentBatch;
                            }
                            prevSolveTask->m_isLastTaskInBatch.m_bool = 1;
                            ptrToBatchSize = &prevSolveTask->m_sizeOfNextBatch;
                        }
                        firstTaskOfPreviousBatch = firstTaskOfCurrentBatch;
                        firstTaskOfCurrentBatch = solveTask;
                    }
                    ++*ptrToBatchSize;
                    solveTask->m_taskHeader = taskHeader;
                    solveTask->m_accumulators = taskHeader->m_accumulatorsBase;
                    solveTask->m_schemas = taskSchemas;
                    solveTask->m_solverElemTemp = taskElemTemp;
                    solveTask->m_sizeOfSchemaBuffer =
                        static_cast<int32_t>(reinterpret_cast<char*>(schemas) - reinterpret_cast<char*>(taskSchemas));
                    solveTask->m_sizeOfSolverElemTempBuffer =
                        static_cast<int32_t>(reinterpret_cast<char*>(elemTemp) - reinterpret_cast<char*>(taskElemTemp));
                    prevSolveTask = solveTask;
                }
            }
            sourceConstraints->setSizeUnchecked(
                static_cast<int32_t>(nextConstraintLeftOut - sourceConstraints->begin())
            );
        }
    }
    hkMemoryRouter::getInstance().temp()._blockFree(entityGroup, numBodies);

    if (generateSolveTasks.m_bool) {
        // The last batch has no next batch: its tasks get the last batch's size and the last task's successor.
        const uint16_t batchSize = *ptrToBatchSize;
        for (hkpSolveConstraintBatchTask* t = firstTaskOfPreviousBatch; t && t != firstTaskOfCurrentBatch;
             t = t->m_next) {
            t->m_sizeOfNextBatch = batchSize;
            t->m_firstTaskInNextBatch = firstTaskOfCurrentBatch;
        }
        prevSolveTask->m_isLastTaskInBatch.m_bool = 1;
        // The accumulators: the fixed one, then one per entity; a gravity or integrate-velocities job handles 128.
        const int32_t numAccumulators = taskHeader->m_numAllEntities + 1;
        taskHeader->m_accumulatorsEnd = taskHeader->m_accumulatorsBase + numAccumulators;
        const uint16_t numJobs = static_cast<uint16_t>((static_cast<uint32_t>(numAccumulators - 1) >> 7) + 1);
        taskHeader->m_numApplyGravityJobs = numJobs;
        taskHeader->m_numIntegrateVelocitiesJobs = numJobs;
    }

    outBuildCollection->m_numCallbackConstraints = 0;
    outBuildCollection->m_callbackConstraints = nullptr;
    const int32_t numCallbackConstraints = callbackConstraints.getSize();
    if (numCallbackConstraints) {
        hkpBuildJacobianTaskCollection::CallbackPair* callbackPairs =
            hkMemoryRouter::getInstance().heap()._blockAlloc<hkpBuildJacobianTaskCollection::CallbackPair>(
                numCallbackConstraints
            );
        outBuildCollection->m_numCallbackConstraints = numCallbackConstraints;
        outBuildCollection->m_callbackConstraints = callbackPairs;
        for (int32_t i = 0; i < numCallbackConstraints; ++i) {
            callbackPairs[i].m_callbackConstraints = callbackConstraints.m_data[i].m_internal;
            callbackPairs[i].m_atomInfo = callbackConstraints.m_data[i].m_atomInfo;
        }
    }

    // Destroys the two local arrays (~hkLocalArray) and the two priority arrays (clearAndDeallocate through the temp
    // allocator), the last constructed first. The local arrays' elements are not plain data: their clear calls
    // hkArrayUtil::destruct. The two local arrays are raw storage that C++ never destroys, so their destructors are
    // called here: without them their stack blocks are never given back.
    constraintsForThisTask.~hkLocalArray<hkConstraintInternalInfo>();
    callbackConstraints.~hkLocalArray<hkConstraintInternalCallbackInfo>();
    highPriorityConstraints._clearAndDeallocate(hkMemoryRouter::getInstance().temp());
    normalPriorityConstraints._clearAndDeallocate(hkMemoryRouter::getInstance().temp());

    HK_TIMER_END();
}

// The SDK's integrateJob: allocates the island's solver buffer (running another job while the solver allocator has
// none), applies the island's actions, creates the island's task header and decides whether to split the island. An
// island without constraints is integrated here or in integrate-motion jobs; one with few constraints is solved here;
// otherwise the job becomes the island's build-accumulators job and a create-Jacobian-tasks job is finished in its
// place. The island then goes on to the broad phase.
// KEEP: the split-island job is built in a 0x80-byte queue entry of which the exe writes bytes 0..2, 4..7 and
// 0x10..0x11, 0x14..0x1F; addJobQueueLocked (0xD04B00) enqueues it at high priority with
// hkQueue::enqueueInFront (0xD057F0), which copies the whole entry (rep movsd of 0x20 dwords at 0xD0582D), so bytes
// 3, 8..0xF, 0x12..0x13 and 0x20..0x7F of uninitialised stack reach the job queue.
hkJobQueue::JobStatus
integrateJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    // The queue entry holds the job: both are hkJobs.
    hkJob& baseJob = nextJobOut;
    hkpIntegrateJob& job = static_cast<hkpIntegrateJob&>(baseJob);
    hkpSimulationIsland* island = job.m_island;

    int32_t bufferRequested = 0;
    int32_t bufferSize = 0;
    char* buffer = nullptr;
    if (island->m_constraintInfo.m_sizeOfSchemas) {
        bufferRequested = hkpConstraintSolverSetup::calcBufferSize(*island);
        bufferSize = bufferRequested;
        buffer = static_cast<char*>(hkMemoryRouter::getInstance().solver().bufAlloc(bufferSize));
        // No solver memory: take another job without waiting, running as thread 1. When there is one, this job goes
        // back to the queue at low priority, a dummy job is finished in its place and the other job is returned;
        // otherwise spin and try the allocation again.
        while (!buffer) {
            hkJobQueue::JobQueueEntry otherJob;
            void* savedThreadNumber = TlsGetValue(hkThreadNumber);
            // The thread-local slot holds the thread number as a pointer-sized value.
            TlsSetValue(hkThreadNumber, reinterpret_cast<void*>(static_cast<uintptr_t>(1)));
            hkJobQueue::JobStatus status = jobQueue.getNextJob(otherJob, hkJobQueue::DO_NOT_WAIT_FOR_NEXT_JOB);
            TlsSetValue(hkThreadNumber, savedThreadNumber);
            if (status == hkJobQueue::GOT_NEXT_JOB) {
                jobQueue.addJob(nextJobOut, hkJobQueue::JOB_LOW_PRIORITY);
                hkJobQueue::JobQueueEntry dummyJob;
                dummyJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_DUMMY;
                // EXE-INLINED none: hkEnum<E, S>::operator=
                dummyJob.m_jobType.m_storage = HK_JOB_TYPE_DYNAMICS;
                jobQueue.finishJob(&dummyJob, hkJobQueue::FINISH_FLAG_NORMAL);
                nextJobOut = otherJob;
                return hkJobQueue::GOT_NEXT_JOB;
            }
            // A busy wait that stores the counter on each pass.
            volatile int32_t spin;
            for (int32_t i = 0; i < 4000; ++i) {
                spin = i;
            }
            buffer = static_cast<char*>(hkMemoryRouter::getInstance().solver().bufAlloc(bufferSize));
        }
    }

    HK_TIMER_BEGIN_LIST("Integrate", "Init");
    hkpWorldDynamicsStepInfo& stepInfo = tl.m_world->m_dynamicsStepInfo;
    if (!tl.m_world->m_processActionsInSingleThread.m_bool) {
        HK_TIMER_SPLIT_LIST("Actions");
        for (int32_t i = 0; i < island->m_actions.m_size; ++i) {
            hkpAction* action = island->m_actions.m_data[i];
            if (action) {
                action->applyAction(stepInfo.m_stepInfo);
            }
        }
    }

    // EXE-INLINED none: hkpBuildJacobianTaskHeader::hkpBuildJacobianTaskHeader
    hkpBuildJacobianTaskHeader* taskHeader = static_cast<hkpBuildJacobianTaskHeader*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpBuildJacobianTaskHeader))
    );
    if (taskHeader) {
        taskHeader->m_buffer = nullptr;
        taskHeader->m_bufferSize = 0;
        taskHeader->m_bufferCapacity = 0;
        taskHeader->m_referenceCount = 1;
        taskHeader->m_numIslandsAfterSplit = 1;
        // EXE-INLINED none: hkArray<hkpSimulationIsland*>::hkArray
        taskHeader->m_newSplitIslands.m_data = nullptr;
        taskHeader->m_newSplitIslands.m_size = 0;
        taskHeader->m_newSplitIslands.m_capacityAndFlags = hkArrayBase<hkpSimulationIsland*>::DONT_DEALLOCATE_FLAG;
        taskHeader->m_tasks.m_buildJacobianTasks = nullptr;
        taskHeader->m_tasks.m_numBuildJacobianTasks = 0;
        taskHeader->m_tasks.m_callbackConstraints = nullptr;
        taskHeader->m_tasks.m_numCallbackConstraints = 0;
        taskHeader->m_solveTasks.m_firstSolveJacobiansTask = nullptr;
        taskHeader->m_solveTasks.m_firstBatchSize = 0;
    }

    HK_TIMER_SPLIT_LIST("Island Splitting");
    const int32_t numEntities = island->m_entities.m_size;
    hkpEntity** entities = island->m_entities.m_data;
    taskHeader->m_numAllEntities = static_cast<uint16_t>(numEntities);
    taskHeader->m_allEntities = entities;
    taskHeader->m_entitiesCapacity = static_cast<uint16_t>(island->m_entities.m_capacityAndFlags);
    taskHeader->m_numUnfinishedJobsForBroadphase = 0;
    taskHeader->m_islandShouldBeDeactivated = 1;
    taskHeader->m_buffer = nullptr;
    taskHeader->m_bufferSize = 0;
    taskHeader->m_impulseLimitsBreached = nullptr;
    taskHeader->m_tasks.m_buildJacobianTasks = nullptr;
    taskHeader->m_accumulatorsBase = nullptr;
    // 1 when the island is split after this step; it is an int because it is added to a job count.
    int32_t splitIsland = 0;
    HK_TIMER_SPLIT_LIST("SplitIslands");
    ++island->m_splitCheckFrameCounter;
    if (island->m_sparseEnabled) {
        int32_t islandSize = island->m_entities.m_size;
        if (islandSize <= island->m_numConstraints) {
            islandSize = island->m_numConstraints;
        }
        // The compare is signed although m_minDesiredIslandSize is unsigned, as the game does.
        if (islandSize < static_cast<int32_t>(tl.m_world->m_minDesiredIslandSize)) {
            if ((island->m_splitCheckFrameCounter & 7) == 0) {
                splitIsland = 1;
            }
        }
        else {
            island->m_sparseEnabled = 0;
            if ((island->m_splitCheckFrameCounter & 3) == 0) {
                splitIsland = 1;
            }
        }
    }
    else if (
        island->m_splitCheckRequested && (island->m_splitCheckFrameCounter & 7) == 0 &&
        tl.m_world->m_wantSimulationIslands.m_bool
    ) {
        splitIsland = 1;
    }

    if (!island->m_constraintInfo.m_sizeOfSchemas) {
        if (tl.m_world->m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob.m_bool) {
            taskHeader->m_buffer = nullptr;
            taskHeader->m_bufferCapacity = 0;
            taskHeader->m_accumulatorsBase = nullptr;
            taskHeader->m_schemasBase = nullptr;
            taskHeader->m_solverTempBase = nullptr;
            taskHeader->m_bufferSize = 0;
            taskHeader->m_numSolverResults = island->m_constraintInfo.m_numSolverResults;
            taskHeader->m_numSolverElemTemps = island->m_constraintInfo.m_numSolverElemTemps;
            taskHeader->m_bufferCapacity = bufferSize;
            taskHeader->m_splitCheckRequested = static_cast<int8_t>(splitIsland);
            taskHeader->m_solveTasks.m_firstSolveJacobiansTask = nullptr;
            taskHeader->m_constraintQueryIn = &tl.m_constraintQueryIn;
            // One integrate-motion job per 128 entities; the shift is unsigned.
            const int32_t numIntegrateJobs = static_cast<int32_t>((static_cast<uint32_t>(numEntities - 1) >> 7) + 1);
            taskHeader->m_numUnfinishedJobsForBroadphase = numIntegrateJobs + splitIsland;

            // The job becomes the island's integrate-motion job in place. The SDK's constructor also copies the
            // dynamics job's members onto themselves, which changes no byte.
            hkpIntegrateMotionJob& motionJob = static_cast<hkpIntegrateMotionJob&>(baseJob);
            motionJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION;
            // EXE-INLINED none: hkEnum<E, S>::operator=
            motionJob.m_jobType.m_storage = HK_JOB_TYPE_DYNAMICS;
            // EXE-INLINED none: hkEnum<E, S>::operator=
            motionJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
            motionJob.m_threadAffinity = -1;
            motionJob.m_size = sizeof(hkpIntegrateMotionJob);
            motionJob.m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
            motionJob.m_firstEntityIdx = 0;
            motionJob.m_numEntities = taskHeader->m_numAllEntities;
            motionJob.m_buffer = taskHeader->m_buffer;
            motionJob.m_applyForcesAndStepMotionOnly = false;
            motionJob.m_taskHeader = taskHeader;
            motionJob.m_applyForcesAndStepMotionOnly = true;
            // EXE-INLINED none: hkEnum<E, S>::operator=
            motionJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
            taskHeader->m_exportFinished = 1;
            taskHeader->m_openJobs = numIntegrateJobs;
            HK_TIMER_END_LIST();

            hkJobQueue::DynamicData lockData;
            // lockQueue takes its preallocated buffer as untyped bytes.
            hkJobQueue::DynamicData* data = jobQueue.lockQueue(reinterpret_cast<char*>(&lockData));
            if (splitIsland) {
                hkJobQueue::JobQueueEntry splitEntry;
                hkJob& splitBase = splitEntry;
                hkpSplitSimulationIslandJob& splitJob = static_cast<hkpSplitSimulationIslandJob&>(splitBase);
                splitJob.m_size = sizeof(hkpSplitSimulationIslandJob);
                splitJob.m_islandIndex = motionJob.m_islandIndex;
                splitJob.m_threadAffinity = -1;
                splitJob.m_taskHeader = motionJob.m_taskHeader;
                splitJob.m_island = motionJob.m_island;
                splitJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_SPLIT_ISLAND;
                // EXE-INLINED none: hkEnum<E, S>::operator=
                splitJob.m_jobType.m_storage = HK_JOB_TYPE_DYNAMICS;
                // EXE-INLINED none: hkEnum<E, S>::operator=
                splitJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
                splitJob.m_mtThreadStructure = motionJob.m_mtThreadStructure;
                jobQueue.addJobQueueLocked(data, splitEntry, hkJobQueue::JOB_HIGH_PRIORITY);
            }
            jobQueue.unlockQueue(data);
            return jobQueue.finishAddAndGetNextJob(
                HK_JOB_TYPE_DYNAMICS, hkJobQueue::JOB_HIGH_PRIORITY, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB
            );
        }

        HK_TIMER_SPLIT_LIST("SingleObj");
        // Havok's motionsOffset convention: the callee reads the entity pointers as untyped bases of their motions.
        const int32_t numInactiveFrames = hkRigidMotionUtilApplyForcesAndStep(
            stepInfo.m_solverInfo, stepInfo.m_stepInfo, stepInfo.m_solverInfo.m_globalAccelerationPerStep,
            reinterpret_cast<hkpMotion* const*>(island->m_entities.m_data), island->m_entities.m_size,
            offsetof(hkpEntity, m_motion)
        );
        hkpEntityAabbUtil::entityBatchRecalcAabb(
            island->getWorld()->m_collisionInput, island->m_entities.m_data, island->m_entities.m_size
        );
        if (numInactiveFrames <= 5) {
            taskHeader->m_islandShouldBeDeactivated = 0;
        }
    }
    else {
        int32_t islandSize = island->m_entities.m_size;
        if (island->m_numConstraints > islandSize) {
            islandSize = island->m_numConstraints;
        }
        // The compare is unsigned although both are ints, as the game does.
        // EXE-INLINED none: hkPadSpu<hkpMultiThreadedSimulation*>::operator hkpMultiThreadedSimulation*
        if (static_cast<uint32_t>(islandSize) >=
            static_cast<uint32_t>(
                tl.m_simulation.m_storage->m_multithreadConfig.m_maxNumConstraintsSolvedSingleThreaded
            )) {
            HK_TIMER_SPLIT_LIST("SetupJobs");
            hkpConstraintSolverSetup::calcBufferOffsetsForSolve(*island, buffer, bufferRequested, *taskHeader);
            taskHeader->m_numSolverResults = island->m_constraintInfo.m_numSolverResults;
            taskHeader->m_bufferCapacity = bufferSize;
            taskHeader->m_numSolverElemTemps = island->m_constraintInfo.m_numSolverElemTemps;
            taskHeader->m_splitCheckRequested = static_cast<int8_t>(splitIsland);
            taskHeader->m_constraintQueryIn = &tl.m_constraintQueryIn;
            // One build-accumulators job per 128 entities. The shift is unsigned.
            const int32_t numBatchesLess1 = static_cast<int32_t>(static_cast<uint32_t>(numEntities - 1) >> 7);
            taskHeader->m_numUnfinishedJobsForBroadphase = numBatchesLess1 + splitIsland + 1;
            taskHeader->m_exportFinished = 0;
            taskHeader->m_openJobs = numBatchesLess1 + 2;

            // The job becomes the island's build-accumulators job in place.
            hkpBuildAccumulatorsJob& accumulatorsJob = static_cast<hkpBuildAccumulatorsJob&>(baseJob);
            accumulatorsJob.m_firstEntityIdx = 0;
            accumulatorsJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_BUILD_ACCUMULATORS;
            accumulatorsJob.m_taskHeader = taskHeader;
            accumulatorsJob.m_islandEntitiesArray = entities;
            accumulatorsJob.m_numEntities = static_cast<uint16_t>(numEntities);
            // EXE-INLINED none: hkEnum<E, S>::operator=
            accumulatorsJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
            jobQueue.addJob(nextJobOut, hkJobQueue::JOB_HIGH_PRIORITY);

            hkpBuildJacobianTaskCollection buildCollection;
            buildCollection.m_buildJacobianTasks = nullptr;
            buildCollection.m_numBuildJacobianTasks = 0;
            buildCollection.m_callbackConstraints = nullptr;
            buildCollection.m_numCallbackConstraints = 0;
            hkpSolveJacobiansTaskCollection solveCollection;
            solveCollection.m_firstSolveJacobiansTask = nullptr;
            solveCollection.m_firstBatchSize = 0;
            createBuildJacobianAndSolveJacobianTaskCollection(
                island, stepInfo.m_solverInfo.m_forceCoherentConstraintOrderingInSolver, taskHeader, &buildCollection,
                &solveCollection
            );
            HK_TIMER_END_LIST();
            taskHeader->m_tasks = buildCollection;
            taskHeader->m_solveTasks = solveCollection;

            // The create-Jacobian-tasks job is finished in place of this one, which queues the island's next jobs.
            hkJobQueue::JobQueueEntry createEntry;
            hkJob& createBase = createEntry;
            hkpCreateJacobianTasksJob& createJob = static_cast<hkpCreateJacobianTasksJob&>(createBase);
            createJob.m_size = sizeof(hkpCreateJacobianTasksJob);
            createJob.m_islandIndex = accumulatorsJob.m_islandIndex;
            createJob.m_threadAffinity = -1;
            createJob.m_island = accumulatorsJob.m_island;
            createJob.m_mtThreadStructure = accumulatorsJob.m_mtThreadStructure;
            createJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_CREATE_JACOBIAN_TASKS;
            // EXE-INLINED none: hkEnum<E, S>::operator=
            createJob.m_jobType.m_storage = HK_JOB_TYPE_DYNAMICS;
            // EXE-INLINED none: hkEnum<E, S>::operator=
            createJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
            createJob.m_taskHeader = taskHeader;
            return jobQueue.finishJobAndGetNextJob(&createEntry, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
        }

        HK_TIMER_SPLIT_LIST("Solver 1Cpu");
        hkpConstraintQueryIn constraintQueryIn = tl.m_constraintQueryIn;
        const int32_t numInactiveFrames = hkpConstraintSolverSetup::solve(
            stepInfo.m_stepInfo, stepInfo.m_solverInfo, constraintQueryIn, *island, buffer, bufferRequested, entities,
            numEntities
        );
        if (numInactiveFrames <= 5) {
            taskHeader->m_islandShouldBeDeactivated = 0;
        }
        hkMemoryRouter::getInstance().solver().bufFree(buffer, bufferSize);
    }

    if (splitIsland) {
        hkCpuSplitSimulationIslandJobImpl(island, taskHeader->m_newSplitIslands);
    }
    HK_TIMER_END_LIST();
    taskHeader->m_exportFinished = 1;
    // The job becomes the island's broad-phase job in place.
    hkpBroadPhaseJob& broadPhaseJob = static_cast<hkpBroadPhaseJob&>(baseJob);
    broadPhaseJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_BROADPHASE;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    broadPhaseJob.m_jobType.m_storage = HK_JOB_TYPE_DYNAMICS;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    broadPhaseJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
    broadPhaseJob.m_size = sizeof(hkpBroadPhaseJob);
    broadPhaseJob.m_numIslands = 1;
    broadPhaseJob.m_threadAffinity = -1;
    broadPhaseJob.m_taskHeader = taskHeader;
    jobQueue.finishAddAndGetNextJob(
        HK_JOB_TYPE_DYNAMICS, hkJobQueue::JOB_HIGH_PRIORITY, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB
    );
    return hkJobQueue::GOT_NEXT_JOB;
}

hkBool canSolveOnSingleProcessor() {
    return 0;
}
