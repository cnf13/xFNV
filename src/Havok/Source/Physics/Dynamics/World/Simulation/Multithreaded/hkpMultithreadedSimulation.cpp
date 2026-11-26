#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/Maintenance/hkpWorldMaintenanceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuAgentSectorJob.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuBuildJacobiansJob.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuFireJacobianSetupCallbackJob.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuIntegrateJob.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuPostCollideJob.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuSolveConstraintsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadConfig.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldMemoryUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"
#include "xfnv/Compiler.h"

static hkJobQueue::JobStatus
hkCpuSplitSimulationIslandJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
static hkJobQueue::JobStatus
hkCpuIntegrateMotionJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
static hkJobQueue::JobStatus
hkCpuBuildAccumulatorsJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);

// The SDK's hkCpuSplitSimulationIslandJob: splits the job's island into the task header's new islands.
static hkJobQueue::JobStatus
hkCpuSplitSimulationIslandJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut) {
    HK_TIMER_BEGIN("SplitIsle", nullptr);
    // The queue entry holds the job: both are hkJobs.
    hkJob& baseJob = jobInOut;
    hkpSplitSimulationIslandJob& job = static_cast<hkpSplitSimulationIslandJob&>(baseJob);
    hkCpuSplitSimulationIslandJobImpl(job.m_island, job.m_taskHeader->m_newSplitIslands);
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// The SDK's hkCpuIntegrateMotionJob: integrates the motions of the job's range of the island's entities, by forces
// and gravity alone or from the solver's accumulators (the first accumulator is the fixed one), keeps the fewest
// inactive frames in the job and recomputes the entities' AABBs.
static hkJobQueue::JobStatus
hkCpuIntegrateMotionJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut) {
    HK_TIMER_BEGIN_LIST("Integrate", "IntegrateMotions");
    // The queue entry holds the job: both are hkJobs.
    hkJob& baseJob = jobInOut;
    hkpIntegrateMotionJob& job = static_cast<hkpIntegrateMotionJob&>(baseJob);
    const int32_t numEntities = job.m_numEntities;
    hkpBuildJacobianTaskHeader* taskHeader = job.m_taskHeader;
    hkpEntity* const* entities = &taskHeader->m_allEntities[job.m_firstEntityIdx];
    // The SDK hands the entity array over as the motions, with the offset of each entity's m_motion: the callees
    // treat the array as untyped pointers and add the offset to each.
    hkpMotion* const* motions = reinterpret_cast<hkpMotion* const*>(entities);
    int32_t numInactiveFrames;
    if (job.m_applyForcesAndStepMotionOnly.m_bool) {
        numInactiveFrames = hkRigidMotionUtilApplyForcesAndStep(
            tl.m_world->m_dynamicsStepInfo.m_solverInfo,
            tl.m_world->m_dynamicsStepInfo.m_stepInfo,
            tl.m_world->m_dynamicsStepInfo.m_solverInfo.m_globalAccelerationPerStep,
            motions,
            numEntities,
            offsetof(hkpEntity, m_motion)
        );
    }
    else {
        numInactiveFrames = hkRigidMotionUtilApplyAccumulators(
            tl.m_world->m_dynamicsStepInfo.m_solverInfo,
            tl.m_world->m_dynamicsStepInfo.m_stepInfo,
            &taskHeader->m_accumulatorsBase[job.m_firstEntityIdx + 1],
            motions,
            numEntities,
            offsetof(hkpEntity, m_motion)
        );
    }
    job.m_numInactiveFrames = numInactiveFrames;
    hkpEntityAabbUtil::entityBatchRecalcAabb(tl.m_world->m_collisionInput, entities, numEntities);
    HK_TIMER_END_LIST();
    return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

static hkJobQueue::JobStatus
hkCpuBuildAccumulatorsJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN_LIST("Integrate", "BuildAccumulators");
    // The queue entry is untyped storage the job was written into (the SDK's cast).
    const hkpBuildAccumulatorsJob& job = reinterpret_cast<const hkpBuildAccumulatorsJob&>(nextJobOut);
    // Havok's motionsOffset convention: the callee reads the entity pointers as untyped bases of their motions at
    // +0xE0 (the SDK's cast).
    hkpMotion* const* motions = reinterpret_cast<hkpMotion* const*>(job.m_islandEntitiesArray + job.m_firstEntityIdx);
    // Accumulator 0 is the fixed one; the island's entities start at 1.
    hkpVelocityAccumulator* accumulators = job.m_taskHeader->m_accumulatorsBase + 1 + job.m_firstEntityIdx;
    hkRigidMotionUtilApplyForcesAndBuildAccumulators(
        tl.m_collisionInput.m_stepInfo,
        motions,
        job.m_numEntities,
        offsetof(hkpEntity, m_motion),
        accumulators
    );
    HK_TIMER_END_LIST();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// Orders the cross-island pairs by the uid of their A entities, then of their B entities. hkpEntity::m_uid is
// unsigned, but the uids are compared signed.
bool pairLessThan(const hkpTypedBroadPhaseHandlePair& pair1, const hkpTypedBroadPhaseHandlePair& pair2) {
    const hkpCollidable* pCollidableA1 = static_cast<const hkpCollidable*>(pair1.getElementA()->getOwner());
    const hkpCollidable* pCollidableA2 = static_cast<const hkpCollidable*>(pair2.getElementA()->getOwner());
    int32_t iUidA1 = static_cast<int32_t>(static_cast<const hkpEntity*>(pCollidableA1->getOwner())->getUid());
    int32_t iUidA2 = static_cast<int32_t>(static_cast<const hkpEntity*>(pCollidableA2->getOwner())->getUid());
    if (iUidA1 < iUidA2) {
        return true;
    }
    if (iUidA1 != iUidA2) {
        return false;
    }
    const hkpCollidable* pCollidableB1 = static_cast<const hkpCollidable*>(pair1.getElementB()->getOwner());
    const hkpCollidable* pCollidableB2 = static_cast<const hkpCollidable*>(pair2.getElementB()->getOwner());
    return static_cast<int32_t>(static_cast<const hkpEntity*>(pCollidableB1->getOwner())->getUid()) <
        static_cast<int32_t>(static_cast<const hkpEntity*>(pCollidableB2->getOwner())->getUid());
}

hkBool less_hkSimulationIslandPtr(const hkpSimulationIsland* a, const hkpSimulationIsland* b) {
    return (*a->m_entities.begin())->getUid() < (*b->m_entities.begin())->getUid();
}

// Puts each pair's lower broad phase id first, then sorts the pairs with pairLessThan. The ids are unsigned but are
// compared signed.
void sortPairList(hkArray<hkpTypedBroadPhaseHandlePair, hkContainerHeapAllocator>& pairs) {
    // The end address is computed as an integer and the addresses are compared unsigned, so an end that wraps skips the
    // loop. A pointer compare would let clang assume no wrap and run it anyway, so the addresses are
    // integers here.
    const uintptr_t endAddress = reinterpret_cast<uintptr_t>(pairs.m_data) +
        static_cast<uintptr_t>(pairs.m_size) * sizeof(hkpTypedBroadPhaseHandlePair);
    for (hkpTypedBroadPhaseHandlePair* pPair = pairs.m_data; reinterpret_cast<uintptr_t>(pPair) < endAddress; ++pPair) {
        hkpTypedBroadPhaseHandle* pHandleA = pPair->getElementA();
        hkpTypedBroadPhaseHandle* pHandleB = pPair->getElementB();
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        if (static_cast<int32_t>(pCollidableA->getBroadPhaseHandle()->m_id) >
            static_cast<int32_t>(pCollidableB->getBroadPhaseHandle()->m_id)) {
            pPair->m_a = pHandleB;
            pPair->m_b = pHandleA;
        }
    }
    hkAlgorithm::quickSort(pairs.m_data, pairs.m_size, &pairLessThan);
}

hkpIslandsAgentEntriesInfo* getIslandEntriesInfo(
    hkpSimulationIsland* island,
    hkArray<hkpIslandsAgentEntriesInfo, hkContainerHeapAllocator>* infos
) {
    int32_t v2 = 0;
    if (infos->m_size > 0) {
        hkpIslandsAgentEntriesInfo* m_data = infos->begin();
        do {
            if (island == m_data->m_island) {
                return &infos->m_data[v2];
            }
            ++v2;
            ++m_data;
        }
        while (v2 < infos->m_size);
    }
    hkpIslandsAgentEntriesInfo kInfo = {};
    kInfo.m_island = island;
    infos->pushBack(kInfo);
    return &infos->back();
}

// The broad phase's entity pair listener. While the simulation collects cross-island pairs, a pair of two movable
// entities in different islands is queued in m_addedCrossIslandPairs; any other pair gets its agent at once, with the
// predictive flag of the pair's collision quality, unless the quality table says the pair does not collide.
void hkpMultiThreadedSimulation::MtEntityEntityBroadPhaseListener::addCollisionPair(
    hkpTypedBroadPhaseHandlePair& pair
) {
    hkpMultiThreadedSimulation* pSimulation = m_simulation;
    hkpLinkedCollidable* collA = static_cast<hkpLinkedCollidable*>(
        static_cast<hkpCollidable*>(pair.getElementA()->getOwner())
    );
    hkpLinkedCollidable* collB = static_cast<hkpLinkedCollidable*>(
        static_cast<hkpCollidable*>(pair.getElementB()->getOwner())
    );
    hkpEntity* entityA = static_cast<hkpEntity*>(collA->getOwner());
    hkpEntity* entityB = static_cast<hkpEntity*>(collB->getOwner());
    if (!pSimulation->m_crossIslandPairsCollectingActive.m_bool ||
        entityA->isFixed() ||
        entityB->isFixed() ||
        entityA->getSimulationIsland() == entityB->getSimulationIsland()) {
        hkpProcessCollisionInput* input = pSimulation->m_world->m_collisionInput;
        int8_t quality = input->m_dispatcher->m_collisionQualityTable[collA->m_broadPhaseHandle.m_objectQualityType]
                                                                     [collB->m_broadPhaseHandle.m_objectQualityType];
        if (quality) {
            input->m_createPredictiveAgents.m_storage = input->m_dispatcher->m_collisionQualityInfo[quality]
                                                            .m_useContinuousPhysics.m_storage;
            hkpWorldAgentUtil::addAgent(collA, collB, *input);
        }
    }
    else {
        pSimulation->m_addedCrossIslandPairs.pushBack(pair);
    }
}

// Called by the broad phase when two entities' aabbs stop overlapping. While cross-island pairs are being collected, a
// pair of non-fixed entities in different islands is queued in m_removedCrossIslandPairs; any other pair has its agent,
// if one exists, removed at once.
void hkpMultiThreadedSimulation::MtEntityEntityBroadPhaseListener::removeCollisionPair(
    hkpTypedBroadPhaseHandlePair& pair
) {
    hkpMultiThreadedSimulation* pSimulation = m_simulation;
    hkpLinkedCollidable* collA = static_cast<hkpLinkedCollidable*>(
        static_cast<hkpCollidable*>(pair.getElementA()->getOwner())
    );
    hkpLinkedCollidable* collB = static_cast<hkpLinkedCollidable*>(
        static_cast<hkpCollidable*>(pair.getElementB()->getOwner())
    );
    hkpEntity* entityA = static_cast<hkpEntity*>(collA->getOwner());
    hkpEntity* entityB = static_cast<hkpEntity*>(collB->getOwner());
    if (!pSimulation->m_crossIslandPairsCollectingActive.m_bool ||
        entityA->isFixed() ||
        entityB->isFixed() ||
        entityA->getSimulationIsland() == entityB->getSimulationIsland()) {
        hkpAgentNnEntry* entry = hkAgentNnMachine_FindAgent(collA, collB);
        if (entry) {
            hkpWorldAgentUtil::removeAgent(entry);
        }
    }
    else {
        pSimulation->m_removedCrossIslandPairs.pushBack(pair);
    }
}

// The broad phase's phantom listener: when one handle of the pair is a phantom, the other handle's collidable is added
// to that phantom's overlaps, A's phantom first.
void hkpMultiThreadedSimulation::MtPhantomBroadPhaseListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pair.getElementB()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pair.getElementA()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableA);
    }
}

// The same as addCollisionPair, removing the other collidable from the phantom's overlaps.
void hkpMultiThreadedSimulation::MtPhantomBroadPhaseListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pair.getElementB()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pair.getElementA()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableA);
    }
}

// The broad phase's border listener: when one handle of the pair is a border phantom, the other handle's collidable is
// added to that border's overlaps, A's border first; a pair of two borders is left alone.
void hkpMultiThreadedSimulation::MtBroadPhaseBorderListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER &&
        pair.getElementB()->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        return;
    }
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pair.getElementB()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pair.getElementA()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableA);
    }
}

// The same as addCollisionPair, removing the other collidable from the border's overlaps.
void hkpMultiThreadedSimulation::MtBroadPhaseBorderListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER &&
        pair.getElementB()->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        return;
    }
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pair.getElementB()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pair.getElementA()->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableA);
    }
}

hkpMultiThreadedSimulation::hkpMultiThreadedSimulation(hkpWorld* world)
  : hkpContinuousSimulation(world), m_crossIslandPairsCollectingActive(false),
    m_addCrossIslandPairCriticalSection(4000), m_removeCrossIslandPairCriticalSection(4000),
    m_jobQueueHandleForToiSolve(nullptr), m_toiQueueCriticalSection(4000), m_phantomCriticalSection(4000) {
    m_world->m_broadPhase->enableMultiThreading(10000);
    m_entityEntityBroadPhaseListener.m_simulation = this;
    m_phantomBroadPhaseListener.m_criticalSection = &m_phantomCriticalSection;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[1][1] = &m_entityEntityBroadPhaseListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[2][1] = &m_phantomBroadPhaseListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[1][2] = &m_phantomBroadPhaseListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[2][2] = &m_phantomBroadPhaseListener;
    m_broadPhaseBorderListener.m_criticalSection = &m_phantomCriticalSection;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[1][3] = &m_broadPhaseBorderListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[3][1] = &m_broadPhaseBorderListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[2][3] = &m_broadPhaseBorderListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[3][2] = &m_broadPhaseBorderListener;
    world->m_broadPhaseDispatcher->m_broadPhaseListeners[3][3] = &m_broadPhaseBorderListener;
}

// The critical sections and the arrays release themselves.
hkpMultiThreadedSimulation::~hkpMultiThreadedSimulation() {}

void hkpMultiThreadedSimulation::assertThereIsNoCollisionInformationForEntities(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world
) {
    ;
}

void hkpMultiThreadedSimulation::assertThereIsNoCollisionInformationForAgent(hkpAgentNnEntry* agent) {
    ;
}

// The toi-only narrow phase of a group of one island's entities. Without a job queue handle for the toi solve it is the
// continuous simulation's, given the simulation's own map (m_entitiesNeedingPsiCollisionDetection, not the caller's).
// Otherwise each entity is flagged in a stack array of the island's entity count; each agent of an entity whose partner
// is not a flagged entity of this island goes to the agent array when its collision quality uses continuous physics
// without simple toi handling, else the entity goes into the map of entities needing psi collision detection (with a
// reference) unless it is there already. The agent array is then collided by processAgentNnEntries.
void hkpMultiThreadedSimulation::collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(
    hkpEntity** entities,
    int32_t numEntities,
    const hkpProcessCollisionInput& input,
    hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection
) {
    if (!m_jobQueueHandleForToiSolve) {
        hkpContinuousSimulation::collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(
            entities,
            numEntities,
            input,
            m_entitiesNeedingPsiCollisionDetection
        );
        return;
    }
    hkpSimulationIsland* island = entities[0]->getSimulationIsland();
    const int32_t numIslandEntities = island->m_entities.m_size;

    hkLocalArray<uint8_t> entityInfo(numIslandEntities);
    hkString::memSet(entityInfo.m_data, 0, numIslandEntities);

    hkLocalArray<hkpAgentNnEntry*> agentEntries(1000);

    hkArray<hkpLinkedCollidable::CollisionEntry> collisionEntries;

    for (int32_t e = 0; e < numEntities; e++) {
        hkpEntity* entity = entities[e];
        entityInfo.m_data[entity->m_storageIndex] = 1;
        entity->m_collidable.getCollisionEntriesSorted(collisionEntries);
        for (const hkpLinkedCollidable::CollisionEntry& entry : collisionEntries) {
            hkpEntity* partner = static_cast<hkpEntity*>(entry.m_partner->getOwner());
            if (partner->getSimulationIsland() == island && entityInfo.m_data[partner->m_storageIndex]) {
                continue;
            }
            const hkpCollisionQualityInfo* qualityInfo = m_world->m_collisionDispatcher->getCollisionQualityInfo(
                static_cast<int8_t>(entry.m_agentEntry->m_collisionQualityIndex)
            );
            if (qualityInfo->m_useContinuousPhysics.m_storage && !qualityInfo->m_useSimpleToiHandling.m_bool) {
                agentEntries.pushBack(entry.m_agentEntry);
            }
            else if (!entitiesNeedingPsiCollisionDetection.getWithDefault(entity->getUid(), nullptr)) {
                entity->addReference();
                entitiesNeedingPsiCollisionDetection.insert(entity->getUid(), entity);
            }
        }
    }

    if (agentEntries.m_size) {
        processAgentNnEntries(agentEntries.m_data, agentEntries.m_size, input, island, hkBool(false));
    }
}

// Recollides at the PSI time the agents of the entities the TOI phase moved: every agent entry of every entity in the
// map once (the reference the map holds on each entity is dropped), grouped by the island whose agent-sector job
// collides it (a fixed entity's partner's island). The entries go out island by island in one block of the thread's
// stack memory to processAgentNnEntriesFromMultipleIslands, with the world's collision input; input is never read.
void hkpMultiThreadedSimulation::collideEntitiesNeedingPsiCollisionDetectionNarrowPhase_toiOnly(
    [[maybe_unused]] const hkpProcessCollisionInput& input,
    hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection
) {
    HK_TIMER_BEGIN("Recollide PSI", nullptr);
    m_world->lockCriticalOperations();

    using entriesSet_t = hkPointerMap<hkpAgentNnEntry*, int32_t>;
    entriesSet_t entriesSet;
    hkArray<EntryAndIsland> entriesAndIslands;
    hkArray<hkpIslandsAgentEntriesInfo> islandEntriesInfos;
    int32_t numAllEntries = 0;

    for (int32_t it = entitiesNeedingPsiCollisionDetection.getIterator();
         entitiesNeedingPsiCollisionDetection.isValid(it);
         it = entitiesNeedingPsiCollisionDetection.getNext(it)) {
        hkpEntity* entity = entitiesNeedingPsiCollisionDetection.getValue(it);
        hkArray<hkpLinkedCollidable::CollisionEntry> collisionEntries;
        entity->m_collidable.getCollisionEntriesSorted(collisionEntries);
        const int32_t numCollisionEntries = collisionEntries.m_size;
        for (int32_t i = 0; i < numCollisionEntries; ++i) {
            hkpAgentNnEntry* entry = collisionEntries.m_data[i].m_agentEntry;
            if (entriesSet.getWithDefault(entry, 0)) {
                continue;
            }
            const hkpCollisionQualityInfo* quality = m_world->m_collisionDispatcher->getCollisionQualityInfo(
                static_cast<hkpCollisionDispatcher::CollisionQualityIndex>(entry->m_collisionQualityIndex)
            );
            const float invDeltaTime = entity->getMotion()->m_motionState.m_sweptTransform.getInvDeltaTime();
            if (quality->m_useContinuousPhysics.m_storage &&
                !(quality->m_useSimpleToiHandling.m_bool && 0.0f == invDeltaTime)) {
                continue;
            }
            hkpSimulationIsland* island = entity->getSimulationIsland();
            if (entity->getMotion()->getType() == hkpMotion::MOTION_FIXED) {
                hkpEntity* partner = hkSelectOther(
                    entity,
                    static_cast<hkpEntity*>(entry->m_collidable[0]->getOwner()),
                    static_cast<hkpEntity*>(entry->m_collidable[1]->getOwner())
                );
                island = partner->getSimulationIsland();
            }
            entriesSet.insert(entry, 1);
            {
                EntryAndIsland& entryAndIsland = entriesAndIslands.expandOne();
                entryAndIsland.m_entry = entry;
                entryAndIsland.m_island = island;
            }
            hkpIslandsAgentEntriesInfo* info = getIslandEntriesInfo(island, &islandEntriesInfos);
            info->m_numEntries++;
            numAllEntries++;
        }
        entity->removeReference();
    }
    entitiesNeedingPsiCollisionDetection.m_map.clear();

    int32_t firstEntryIdx = 0;
    for (hkpIslandsAgentEntriesInfo& info : islandEntriesInfos) {
        info.m_firstEntryIdx = firstEntryIdx;
        info.m_nextEntryIdx = firstEntryIdx;
        firstEntryIdx += info.m_numEntries;
    }

    hkpAgentNnEntry** allEntries = hkAllocateStack<hkpAgentNnEntry*>(numAllEntries, nullptr);

    for (int32_t i = 0; i < entriesAndIslands.m_size; ++i) {
        hkpAgentNnEntry* entry = entriesAndIslands.m_data[i].m_entry;
        hkpSimulationIsland* island = entriesAndIslands.m_data[i].m_island;
        hkpIslandsAgentEntriesInfo* info = getIslandEntriesInfo(island, &islandEntriesInfos);
        allEntries[info->m_nextEntryIdx++] = entry;
    }
    processAgentNnEntriesFromMultipleIslands(
        allEntries,
        numAllEntries,
        islandEntriesInfos.m_data,
        islandEntriesInfos.m_size,
        *m_world->m_collisionInput,
        hkBool(false)
    );
    m_world->unlockAndAttemptToExecutePendingOperations();
    HK_TIMER_END();
    hkDeallocateStack(allEntries);
}

void hkpMultiThreadedSimulation::getMultithreadConfig(hkpMultithreadConfig& config) const {
    int32_t result = m_multithreadConfig.m_maxNumConstraintsSolvedSingleThreaded;
    config.m_maxNumConstraintsSolvedSingleThreaded = result;
}

void hkpMultiThreadedSimulation::setMultithreadConfig(const hkpMultithreadConfig& config, hkJobQueue* queue) {
    m_multithreadConfig.m_maxNumConstraintsSolvedSingleThreaded = config.m_maxNumConstraintsSolvedSingleThreaded;
}

// Begins a multithreaded step on the stepping thread: opens the Physics timer list, locks the world, sets the step's
// time span in the world's dynamics and collision inputs, runs the maintenance and the dirty-island cleanup, and
// checks that the solver's memory holds the integration (else, without a memory watchdog, fails the step). It then
// sets the solver's per-step values, fills the thread structure the jobs share and, with active islands, runs the
// actions here when the world asks for it and queues one integrate job for all islands. Last the deactivation
// counters advance and the island counts are kept for the end of the step.
// KEEP: partcopy. Of the 48-byte integrate job only bytes 0x0-0x2 and 0x4-0x7 of the job header, the island index word,
// m_island, m_taskHeader, m_mtThreadStructure and m_numIslands are written; hkJobQueue::addJob copies all m_size bytes
// into the queue, so the job's bytes 0x3, 0x8-0xF, 0x12-0x13 and 0x24-0x2F reach it unwritten.
hkpStepResult hkpMultiThreadedSimulation::stepBeginSt(hkJobQueue* jobQueue, float physicsDeltaTime) {
    HK_TIMER_BEGIN_LIST("Physics", "Init");
    m_crossIslandPairsCollectingActive.m_bool = true;
    m_world->lockCriticalOperations();
    m_physicsDeltaTime = physicsDeltaTime;
    // Not hkStepInfo::set: the inverse is divided in double.
    hkStepInfo physicsStepInfo;
    {
        const float startTime = m_currentPsiTime;
        const float endTime = physicsDeltaTime + m_currentPsiTime;
        physicsStepInfo.m_startTime = startTime;
        physicsStepInfo.m_endTime = endTime;
        const float deltaTime = endTime - startTime;
        physicsStepInfo.m_deltaTime = deltaTime;
        physicsStepInfo.m_invDeltaTime = deltaTime == 0.0f ? 0.0f : static_cast<float>(1.0 / deltaTime);
    }
    m_world->m_dynamicsStepInfo.m_stepInfo = physicsStepInfo;
    m_world->m_collisionInput->m_stepInfo = physicsStepInfo;
    m_world->m_maintenanceMgr->performMaintenanceNoSplit(m_world, physicsStepInfo);
    hkpWorldOperationUtil::cleanupDirtyIslands(m_world);
    hkpWorld::MemUsageInfo memUsage;
    m_world->getMemUsageForIntegration(memUsage);
    if (!hkMemoryRouter::getInstance().solver().canAllocSingleBlock(memUsage.m_maxRuntimeBlockSize)) {
        if (!m_world->getMemoryWatchDog()) {
            if (m_world->m_assertOnRunningOutOfSolverMemory.m_bool) {
                xfnv::DebugTrap();
            }
            m_previousStepResult = HK_STEP_RESULT_MEMORY_FAILURE_BEFORE_INTEGRATION;
            return HK_STEP_RESULT_MEMORY_FAILURE_BEFORE_INTEGRATION;
        }
        m_world->unlockAndAttemptToExecutePendingOperations();
        hkpWorldMemoryUtil::checkMemoryForIntegration(m_world);
        m_world->lockCriticalOperations();
    }
    hkSolveUpload();
    m_world->m_dynamicsStepInfo.m_stepInfo = physicsStepInfo;
    {
        hkpSolverInfo& solverInfo = m_world->m_dynamicsStepInfo.m_solverInfo;
        solverInfo.m_deltaTime = solverInfo.m_invNumSteps * physicsStepInfo.m_deltaTime;
        // The step count is multiplied in double precision, unrounded.
        solverInfo.m_invDeltaTime = static_cast<float>(
            static_cast<double>(solverInfo.m_numSteps) * physicsStepInfo.m_invDeltaTime
        );
        solverInfo.m_globalAccelerationPerSubStep.setMul4(solverInfo.m_deltaTime, m_world->m_gravity);
        solverInfo.m_globalAccelerationPerStep.setMul4(physicsStepInfo.m_deltaTime, m_world->m_gravity);
    }
    m_world->m_violatedConstraintArray->m_nextFreeElement = 0;
    hkpMtThreadStructure* threadStructure = m_world->m_multithreadedSimulationJobData;
    threadStructure->m_dynamicsStepInfo = &m_world->m_dynamicsStepInfo;
    threadStructure->m_collisionInput = *m_world->m_collisionInput;
    threadStructure->m_constraintQueryIn.set(
        m_world->m_dynamicsStepInfo.m_solverInfo,
        m_world->m_dynamicsStepInfo.m_stepInfo,
        m_world->m_violatedConstraintArray
    );
    threadStructure->m_simulation.m_storage = this;
    threadStructure->m_weldingTable.m_storage = hkpWeldingUtility::m_sinCosTable;
    threadStructure->m_tolerance = m_world->m_collisionInput->m_tolerance;
    threadStructure->m_world = m_world;
    const int32_t numIslands = m_world->m_activeSimulationIslands.m_size;
    if (numIslands > 0) {
        if (m_world->m_processActionsInSingleThread.m_bool) {
            HK_TIMER_SPLIT_LIST("Actions");
            m_world->unlockCriticalOperationsForPhantoms();
            applyActions();
            m_world->lockCriticalOperationsForPhantoms();
        }
        hkpIntegrateJob job(numIslands);
        job.m_mtThreadStructure = m_world->m_multithreadedSimulationJobData;
        jobQueue->setQueueCapacityForJobType(HK_JOB_TYPE_DYNAMICS, numIslands);
        jobQueue->setQueueCapacityForJobType(HK_JOB_TYPE_COLLIDE, numIslands);
        jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
    }
    m_world->m_dynamicsStepInfo.m_solverInfo.incrementDeactivationFlags();
    m_numActiveIslandsAtBeginningOfStep = m_world->m_activeSimulationIslands.m_size;
    m_numInactiveIslandsAtBeginningOfStep = m_world->m_inactiveSimulationIslands.m_size;
    m_world->checkDeterminism();
    HK_TIMER_END_LIST();
    return HK_STEP_RESULT_SUCCESS;
}

// Appends a time of impact event for the entry's pair to m_toiEvents, from the collision output's toi record, under the
// toi queue's critical section; when the queue is full it only warns. The new event's constructor zeroes its
// properties' solver results.
void hkpMultiThreadedSimulation::addToiEventWithCriticalSectionLock(
    const hkpProcessCollisionOutput& result,
    const hkpAgentNnEntry& entry,
    hkCriticalSection* criticalSection
) {
    if (m_toiEvents.m_size == m_toiEvents.getCapacity()) {
        alignas(16) char assertBuf[512];
        hkErrStream ostr(assertBuf, 512);
        ostr << "Toi event queue full, consider using HK_COLLIDABLE_QUALITY_DEBRIS for some objects or increase "
                "hkpWorldCinfo::m_sizeOfToiEventQueue";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING,
            static_cast<int32_t>(0xF0323454),
            assertBuf,
            ".\\World\\Simulation\\Multithreaded\\hkpMultithreadedSimulation.cpp",
            1015
        );
        return;
    }
    HK_TIMER_BEGIN("AgentJob.addToi", nullptr);
    criticalSection->enter();
    hkpToiEvent& event = m_toiEvents.expandOneUnchecked();
    criticalSection->leave();
    HK_TIMER_END();

    const hkpProcessCollisionData::ToiInfo& toi = result.m_toi;
    event.m_time = toi.m_time.m_storage;
    event.m_useSimpleHandling.m_bool = m_world->m_collisionDispatcher
                                           ->getCollisionQualityInfo(entry.m_collisionQualityIndex)
                                           ->m_useSimpleToiHandling.m_bool;
    event.m_seperatingVelocity = toi.m_seperatingVelocity.m_storage;
    event.m_contactPoint = toi.m_contactPoint;
    event.m_entities[0] = static_cast<hkpEntity*>(entry.m_collidable[0]->getOwner());
    event.m_entities[1] = static_cast<hkpEntity*>(entry.m_collidable[1]->getOwner());
    event.m_properties = static_cast<const hkpContactPointProperties&>(toi.m_properties);
    event.m_contactMgr = static_cast<hkpDynamicsContactMgr*>(entry.m_contactMgr);
    for (int32_t i = 0; i < 7; i++) {
        event.m_extendedUserDatas[i] = toi.m_properties.m_extendedUserDatas[i];
    }
}

// The SDK's processAgentNnEntriesFromMultipleIslands: with a toi job queue and more entries than one thread takes,
// one agent-entry job per island (on the thread's stack allocator; the fixed island's entries are kept for this
// thread), the queue run to its end, each island's post-collide job run here with a thread structure of its own,
// then the fixed island's entries collided here. Otherwise each island's entries are collided here.
// KEEP: partcopy. Of each 80-byte agent-entry job only bytes 0x0-0x2, 0x4-0x7, 0x10-0x11, 0x14-0x25 and 0x28-0x40 are
// written; hkJobQueue::addJob copies all m_size bytes into the queue, so bytes 0x3, 0x8-0xF, 0x12-0x13, 0x26-0x27 and
// 0x41-0x4F reach it unwritten.
void hkpMultiThreadedSimulation::processAgentNnEntriesFromMultipleIslands(
    hkpAgentNnEntry** allEntries,
    int32_t numAllEntries,
    hkpIslandsAgentEntriesInfo* islandEntriesInfos,
    int32_t numInfos,
    const hkpProcessCollisionInput& input,
    hkBool forceManifoldQualityType
) {
    if (m_jobQueueHandleForToiSolve && numAllEntries > m_world->m_maxNumToiCollisionPairsSinglethreaded) {
        const int32_t numAgentNnEntriesPerTask = m_world->m_maxEntriesPerToiCollideTask;
        hkpIslandsAgentEntriesInfo* fixedIslandInfo = nullptr;
        hkpAgentNnEntryJob* jobs = hkAllocateStack<hkpAgentNnEntryJob>(numInfos, nullptr);
        for (int32_t i = 0; i < numInfos; ++i) {
            hkpIslandsAgentEntriesInfo& info = islandEntriesInfos[i];
            if (info.m_island == m_world->getFixedIsland()) {
                fixedIslandInfo = &info;
                continue;
            }
            hkpAgentNnEntry** entries = &allEntries[info.m_firstEntryIdx];
            const int32_t numEntries = info.m_numEntries;
            hkpAgentNnEntryJob* job = new (jobs + i) hkpAgentNnEntryJob(
                input.m_stepInfo,
                entries,
                numEntries,
                numAgentNnEntriesPerTask,
                forceManifoldQualityType
            );
            job->m_mtThreadStructure = m_world->m_multithreadedSimulationJobData;
            if (numEntries > 0) {
                job->m_header = hkpAgentSectorHeader::allocate(
                    (numEntries - 1) / numAgentNnEntriesPerTask + 1,
                    numAgentNnEntriesPerTask
                );
                job->m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
                job->m_islandIndex = static_cast<uint16_t>(info.m_island->getStorageIndex());
                m_jobQueueHandleForToiSolve->addJob(*job, hkJobQueue::JOB_LOW_PRIORITY);
                info.m_ppuAgentNnEntryJob = job;
            }
        }
        m_jobQueueHandleForToiSolve->processAllJobs();
        for (int32_t i = 0; i < numInfos; ++i) {
            hkpIslandsAgentEntriesInfo& info = islandEntriesInfos[i];
            hkpAgentNnEntryJob* job = info.m_ppuAgentNnEntryJob;
            if (!job) {
                continue;
            }
            alignas(16) hkpPostCollideJob postCollideJob(*job);
            postCollideJob.m_island = info.m_island;
            alignas(16) hkpMtThreadStructure tl;
            tl.m_dynamicsStepInfo = &m_world->m_dynamicsStepInfo;
            tl.m_collisionInput = *m_world->m_collisionInput;
            tl.m_constraintQueryIn.set(
                m_world->m_dynamicsStepInfo.m_solverInfo,
                m_world->m_dynamicsStepInfo.m_stepInfo,
                m_world->m_violatedConstraintArray
            );
            hkpWorld* world = m_world;
            tl.m_simulation.m_storage = this;
            float tolerance = world->m_collisionInput->m_tolerance;
            tl.m_tolerance = tolerance;
            tl.m_world = world;
            tl.m_weldingTable.m_storage = hkpWeldingUtility::m_sinCosTable;
            // The SDK's cast of the job to the queue entry the job functions take.
            hkCpuPostCollideJob(
                tl,
                nullptr,
                static_cast<hkJobQueue::JobQueueEntry&>(static_cast<hkJob&>(postCollideJob))
            );
        }
        if (fixedIslandInfo) {
            hkpContinuousSimulation::processAgentNnEntries(
                &allEntries[fixedIslandInfo->m_firstEntryIdx],
                fixedIslandInfo->m_numEntries,
                input,
                fixedIslandInfo->m_island,
                hkBool(false)
            );
        }
        hkDeallocateStack(jobs);
    }
    else {
        for (int32_t i = 0; i < numInfos; ++i) {
            const hkpIslandsAgentEntriesInfo& info = islandEntriesInfos[i];
            hkpContinuousSimulation::processAgentNnEntries(
                &allEntries[info.m_firstEntryIdx],
                info.m_numEntries,
                input,
                info.m_island,
                forceManifoldQualityType
            );
        }
    }
}

// Ends a multithreaded step: the master thread joins the job queue until the collide jobs are done, the islands the
// step added are sorted and renumbered, the cross-island pairs the broad phase collected are turned into agents (or
// removed), the violated constraints are reported, and the TOIs are solved after the post-collide callbacks.
hkpStepResult hkpMultiThreadedSimulation::finishMtStep(hkJobQueue* jobQueue, hkJobThreadPool* threadPool) {
    HK_TIMER_BEGIN("Physics", nullptr);
    m_world->lock();
    int32_t masterFlags = 0;
    if (jobQueue && threadPool) {
        masterFlags = jobQueue->getMasterThreadFinishingFlags();
        jobQueue->setWaitPolicy(hkJobQueue::WAIT_INDEFINITELY);
        jobQueue->setMasterThreadFinishingFlags(1 << HK_JOB_TYPE_COLLIDE);
        threadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
    }
    {
        hkpWorld* world = m_world;
        const int32_t first = m_numActiveIslandsAtBeginningOfStep;
        const int32_t numNew = world->m_activeSimulationIslands.m_size - first;
        hkAlgorithm::quickSort(&world->m_activeSimulationIslands.m_data[first], numNew, &less_hkSimulationIslandPtr);
    }
    for (int32_t i = m_numActiveIslandsAtBeginningOfStep; i < m_world->m_activeSimulationIslands.m_size; ++i) {
        m_world->m_activeSimulationIslands.m_data[i]->m_storageIndex = static_cast<uint16_t>(i);
    }
    m_world->checkDeterminism();
    if (m_addedCrossIslandPairs.m_size + m_removedCrossIslandPairs.m_size) {
        HK_TIMER_BEGIN_LIST("InterIsland", "duplicates");
        // The SDK's cast: the pairs are typed broad-phase handle pairs, of the same layout as the base pairs.
        hkpTypedBroadPhaseDispatcher::removeDuplicates(
            reinterpret_cast<hkArray<hkpBroadPhaseHandlePair>&>(m_addedCrossIslandPairs),
            reinterpret_cast<hkArray<hkpBroadPhaseHandlePair>&>(m_removedCrossIslandPairs)
        );
        HK_TIMER_SPLIT_LIST("sortPairs");
        sortPairList(m_addedCrossIslandPairs);
        sortPairList(m_removedCrossIslandPairs);
        HK_TIMER_SPLIT_LIST("addAgt");
        {
            hkpProcessCollisionOutput processOutput(nullptr);
            for (int32_t i = 0; i < m_addedCrossIslandPairs.m_size; ++i) {
                hkpTypedBroadPhaseHandlePair& pair = m_addedCrossIslandPairs.m_data[i];
                void* ownerB = pair.getElementB()->getOwner();
                void* ownerA = pair.getElementA()->getOwner();
                hkpLinkedCollidable* collB = static_cast<hkpLinkedCollidable*>(ownerB);
                hkpLinkedCollidable* collA = static_cast<hkpLinkedCollidable*>(ownerA);
                hkpEntity* entityB = static_cast<hkpEntity*>(collB->getOwner());
                hkpEntity* entityA = static_cast<hkpEntity*>(collA->getOwner());
                hkpSimulationIsland* islandB = entityB->getSimulationIsland();
                hkpSimulationIsland* islandA = entityA->getSimulationIsland();
                if (islandA != islandB) {
                    hkpWorldOperationUtil::internalMergeTwoIslands(m_world, islandA, islandB);
                    islandA = entityA->getSimulationIsland();
                }
                processOutput.m_constraintOwner.m_storage = islandA;
                hkpProcessCollisionInput* input = m_world->m_collisionInput;
                hkpCollisionDispatcher* dispatcher = input->m_dispatcher;
                const int8_t
                    qualityIndex = dispatcher->m_collisionQualityTable[collA->m_broadPhaseHandle.m_objectQualityType]
                                                                      [collB->m_broadPhaseHandle.m_objectQualityType];
                if (qualityIndex) {
                    input->m_createPredictiveAgents.m_storage = dispatcher->getCollisionQualityInfo(qualityIndex)
                                                                    ->m_useContinuousPhysics.m_storage;
                    hkpAgentNnEntry* entry = hkpWorldAgentUtil::addAgent(collA, collB, *input);
                    if (entry) {
                        hkpProcessCollisionInput* processInput = m_world->m_collisionInput;
                        hkpLinkedCollidable* entryCollA = entry->m_collidable[0];
                        hkpLinkedCollidable* entryCollB = entry->m_collidable[1];
                        hkpCollisionQualityInfo* qualityInfo = processInput->m_dispatcher->getCollisionQualityInfo(
                            static_cast<int8_t>(entry->m_collisionQualityIndex)
                        );
                        processInput->m_collisionQualityInfo = qualityInfo;
                        processInput->m_createPredictiveAgents.m_storage = qualityInfo->m_useContinuousPhysics
                                                                               .m_storage;
                        processOutput.reset();
                        hkAgentNnMachine_ProcessAgent(entry, *processInput, processOutput, entry->m_contactMgr);
                        if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
                            entry->m_contactMgr->processContact(*entryCollA, *entryCollB, *processInput, processOutput);
                        }
                        if (processOutput.m_toi.m_time.m_storage != 3.402820018375656e+38) {
                            addToiEventWithCriticalSectionLock(processOutput, *entry, &m_toiQueueCriticalSection);
                        }
                    }
                }
                if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                    break;
                }
            }
        }
        m_addedCrossIslandPairs.clear();
        HK_TIMER_SPLIT_LIST("removeAgt");
        for (int32_t i = 0; i < m_removedCrossIslandPairs.m_size; ++i) {
            hkpTypedBroadPhaseHandlePair& pair = m_removedCrossIslandPairs.m_data[i];
            void* ownerA = pair.getElementA()->getOwner();
            void* ownerB = pair.getElementB()->getOwner();
            hkpAgentNnEntry* entry = hkAgentNnMachine_FindAgent(
                static_cast<hkpLinkedCollidable*>(ownerA),
                static_cast<hkpLinkedCollidable*>(ownerB)
            );
            if (entry) {
                hkpWorldAgentUtil::removeAgentAndItsToiEvents(entry);
            }
        }
        m_removedCrossIslandPairs.clear();
        HK_TIMER_END_LIST();
    }
    m_crossIslandPairsCollectingActive.m_bool = false;
    // The count is compared signed with the capacity.
    int32_t numViolated = static_cast<int32_t>(m_world->m_violatedConstraintArray->m_nextFreeElement);
    if (numViolated >= 128) {
        numViolated = 128;
    }
    for (int32_t i = 0; i < numViolated; ++i) {
        hkpWorldCallbackUtil::fireConstraintViolated(m_world, m_world->m_violatedConstraintArray->m_constraints[i]);
    }
    m_currentPsiTime += m_physicsDeltaTime;
    m_world->unlockAndAttemptToExecutePendingOperations();
    hkpStepResult result = HK_STEP_RESULT_SUCCESS;
    if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
        result = HK_STEP_RESULT_MEMORY_FAILURE_DURING_COLLIDE;
        m_currentPsiTime -= m_physicsDeltaTime;
        m_world->unlock();
        HK_TIMER_END();
    }
    else {
        if (m_world->m_worldPostCollideListeners.m_size) {
            HK_TIMER_BEGIN("PostCollideCB", nullptr);
            // Both times are rounded to float before the step info is built.
            float startTime = m_currentPsiTime - m_physicsDeltaTime;
            float endTime = m_currentPsiTime;
            hkStepInfo stepInfo(startTime, endTime);
            hkpWorldCallbackUtil::firePostCollideCallback(m_world, stepInfo);
            HK_TIMER_END();
        }
        m_world->checkDeterminism();
        HK_TIMER_END();
        if (m_world->m_processToisMultithreaded.m_bool) {
            m_jobQueueHandleForToiSolve = jobQueue;
        }
        m_world->unlock();
        advanceTime();
        m_jobQueueHandleForToiSolve = nullptr;
        if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
            result = HK_STEP_RESULT_MEMORY_FAILURE_DURING_TOI_SOLVE;
        }
    }
    if (threadPool) {
        HK_TIMER_BEGIN("WaitForWorkerThreads", nullptr);
        jobQueue->setWaitPolicy(hkJobQueue::WAIT_UNTIL_ALL_WORK_COMPLETE);
        jobQueue->setMasterThreadFinishingFlags(masterFlags);
        threadPool->waitForCompletion();
        HK_TIMER_END();
    }
    m_previousStepResult = result;
    if (result != HK_STEP_RESULT_SUCCESS) {
        if (m_world->getMemoryWatchDog()) {
            hkpWorldMemoryUtil::tryToRecoverFromMemoryErrors(m_world);
        }
        return static_cast<hkpStepResult>(m_previousStepResult);
    }
    return result;
}

// With a toi job queue and more entries than one thread takes, the entries go to one agent-entry job on the queue, the
// queue is run to its end and the island's post-collide job is run here with a thread structure of its own; otherwise
// the base collides them here.
// KEEP: partcopy. Of the 80-byte agent-entry job only bytes 0x0-0x2, 0x4-0x7, 0x10-0x11, 0x14-0x25 and 0x28-0x40 are
// written; hkJobQueue::addJob copies all m_size bytes into the queue, so bytes 0x3, 0x8-0xF, 0x12-0x13, 0x26-0x27 and
// 0x41-0x4F reach it unwritten.
void hkpMultiThreadedSimulation::processAgentNnEntries(
    hkpAgentNnEntry** entries,
    int32_t numEntries,
    const hkpProcessCollisionInput& collisionInput,
    hkpSimulationIsland* island,
    hkBool forceManifoldQualityType
) {
    if (!m_jobQueueHandleForToiSolve || numEntries <= m_world->m_maxNumToiCollisionPairsSinglethreaded) {
        hkpContinuousSimulation::processAgentNnEntries(
            entries,
            numEntries,
            collisionInput,
            island,
            forceManifoldQualityType
        );
        return;
    }
    const int32_t numAgentNnEntriesPerTask = m_world->m_maxEntriesPerToiCollideTask;
    hkArray<hkpAgentNnEntry*> entriesArray(entries, numEntries, numEntries);
    alignas(16) hkpAgentNnEntryJob job(
        collisionInput.m_stepInfo,
        entriesArray.m_data,
        numEntries,
        numAgentNnEntriesPerTask,
        forceManifoldQualityType
    );
    job.m_mtThreadStructure = m_world->m_multithreadedSimulationJobData;
    if (numEntries > 0) {
        job.m_header = hkpAgentSectorHeader::allocate(
            (numEntries - 1) / numAgentNnEntriesPerTask + 1,
            numAgentNnEntriesPerTask
        );
        job.m_islandIndex = static_cast<uint16_t>(island->getStorageIndex());
        job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
        m_jobQueueHandleForToiSolve->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
    }
    m_jobQueueHandleForToiSolve->processAllJobs();
    if (numEntries > 0) {
        alignas(16) hkpPostCollideJob postCollideJob(job);
        postCollideJob.m_island = island;
        alignas(16) hkpMtThreadStructure tl;
        tl.m_dynamicsStepInfo = &m_world->m_dynamicsStepInfo;
        tl.m_collisionInput = *m_world->m_collisionInput;
        tl.m_constraintQueryIn.set(
            m_world->m_dynamicsStepInfo.m_solverInfo,
            m_world->m_dynamicsStepInfo.m_stepInfo,
            m_world->m_violatedConstraintArray
        );
        tl.m_simulation.m_storage = this;
        hkpWorld* world = m_world;
        float tolerance = world->m_collisionInput->m_tolerance;
        tl.m_tolerance = tolerance;
        tl.m_weldingTable.m_storage = hkpWeldingUtility::m_sinCosTable;
        tl.m_world = world;
        // The SDK's cast of the job to the queue entry the job functions take.
        hkCpuPostCollideJob(tl, nullptr, static_cast<hkJobQueue::JobQueueEntry&>(static_cast<hkJob&>(postCollideJob)));
    }
}

// Runs a dynamics job on this thread: the job's process function by its sub type, given the job's thread structure
// (the SDK's reinterpret_cast of the queue entry to its dynamics job is the hkJob downcast here). Sub types without
// a process function here (create Jacobian tasks, dummy, out of range) report no jobs available.
hkJobQueue::JobStatus hkpMultiThreadedSimulation::processNextJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& job) {
    hkpDynamicsJob& dynamicsJob = static_cast<hkpDynamicsJob&>(static_cast<hkJob&>(job));
    switch (dynamicsJob.m_jobSubType) {
        case hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE:
            return integrateJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_BUILD_ACCUMULATORS:
            return hkCpuBuildAccumulatorsJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_FIRE_JACOBIAN_SETUP_CALLBACK:
            return hkpSingleThreadedJobsOnIsland::cpuFireJacobianSetupCallbackJob(
                *dynamicsJob.m_mtThreadStructure,
                jobQueue,
                job
            );
        case hkpDynamicsJob::DYNAMICS_JOB_BUILD_JACOBIANS:
            return hkCpuBuildJacobiansJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_SPLIT_ISLAND:
            return hkCpuSplitSimulationIslandJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINTS:
            return hkCpuSolveConstraintsJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_APPLY_GRAVITY:
            return hkCpuSolveApplyGravityJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH:
            return hkCpuSolveConstraintBatchJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_INTEGRATE_VELOCITIES:
            return hkCpuSolveIntegrateVelocitiesJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_EXPORT_RESULTS:
            return hkCpuSolveExportResultsJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION:
            return hkCpuIntegrateMotionJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_BROADPHASE:
            return hkpSingleThreadedJobsOnIsland::cpuBroadPhaseJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_AGENT_SECTOR:
        case hkpDynamicsJob::DYNAMICS_JOB_AGENT_NN_ENTRY:
            return hkCpuAgentSectorJob(*dynamicsJob.m_mtThreadStructure, jobQueue, job);
        case hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE:
            return hkCpuPostCollideJob(*dynamicsJob.m_mtThreadStructure, &jobQueue, job);
        default: return hkJobQueue::NO_JOBS_AVAILABLE;
    }
}
