#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Setup/hkpConstraintSolverSetup.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Maintenance/hkpWorldMaintenanceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuSolveConstraintsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldMemoryUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"

#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpSimulation(void* p);
static void finishLoadedObjecthkpSimulation(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpSimulation();

// The reflected enums of hkpSimulation.
using hkpSimulationFindContactsEnumItemsArray = const hkClassEnum::Item[2];
static hkpSimulationFindContactsEnumItemsArray hkpSimulationFindContactsEnumItems{
    {hkpSimulation::FIND_CONTACTS_DEFAULT, "FIND_CONTACTS_DEFAULT"},
    {hkpSimulation::FIND_CONTACTS_EXTRA, "FIND_CONTACTS_EXTRA"}
};
using hkpSimulationResetCollisionInformationEnumItemsArray = const hkClassEnum::Item[4];
static hkpSimulationResetCollisionInformationEnumItemsArray hkpSimulationResetCollisionInformationEnumItems{
    {hkpSimulation::RESET_TOI, "RESET_TOI"},
    {hkpSimulation::RESET_TIM, "RESET_TIM"},
    {hkpSimulation::RESET_AABB, "RESET_AABB"},
    {hkpSimulation::RESET_ALL, "RESET_ALL"}
};
using hkpSimulationLastProcessingStepEnumItemsArray = const hkClassEnum::Item[2];
static hkpSimulationLastProcessingStepEnumItemsArray hkpSimulationLastProcessingStepEnumItems{
    {hkpSimulation::INTEGRATE, "INTEGRATE"},
    {hkpSimulation::COLLIDE, "COLLIDE"}
};
using hkpSimulationEnumsArray = const hkClassEnum[3];
static hkpSimulationEnumsArray hkpSimulationEnums{
    {"FindContacts", hkpSimulationFindContactsEnumItems, 2, nullptr, {0}},
    {"ResetCollisionInformation", hkpSimulationResetCollisionInformationEnumItems, 4, nullptr, {0}},
    {"LastProcessingStep", hkpSimulationLastProcessingStepEnumItems, 2, nullptr, {0}}
};

static const hkClassEnum* hkpSimulationFindContactsEnum = &hkpSimulationEnums[0];
static const hkClassEnum* hkpSimulationResetCollisionInformationEnum = &hkpSimulationEnums[1];
static const hkClassEnum* hkpSimulationLastProcessingStepEnum = &hkpSimulationEnums[2];
hkpSimulation::MembersArray hkpSimulation::Members{
    {"determinismCheckFrameCounter",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8,
     nullptr},
    {"world",
     &hkpWorldClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0xC,
     nullptr},
    {"lastProcessingStep",
     nullptr,
     hkpSimulationLastProcessingStepEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x10,
     nullptr},
    {"currentTime", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"currentPsiTime", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
    {"physicsDeltaTime",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x1C,
     nullptr},
    {"simulateUntilTime",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"frameMarkerPsiSnap",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x24,
     nullptr},
    {"previousStepResult",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x28,
     nullptr}
};
hkClass hkpSimulationClass(
    "hkpSimulation",
    &hkReferencedObjectClass,
    44,
    nullptr,
    0,
    hkpSimulationEnums,
    3,
    hkpSimulation::Members,
    9,
    nullptr,
    nullptr,
    1,
    0
);
hkTypeInfo hkpSimulationTypeInfo = {
    "hkpSimulation",          finishLoadedObjecthkpSimulation, cleanupLoadedObjecthkpSimulation,
    getVtablehkpSimulation(), &typeid(hkpSimulation),          sizeof(hkpSimulation)
};

static void cleanupLoadedObjecthkpSimulation(void* p) {
    static_cast<hkpSimulation*>(p)->~hkpSimulation();
}

// The type info's finish-loaded-object callback for hkpSimulation: runs its finish constructor.
static void finishLoadedObjecthkpSimulation(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpSimulation(flag);
    }
}

static const void* getVtablehkpSimulation() {
    return hkGetVtableOf<hkpSimulation>();
}

void setRotationAroundCentreOfMass(hkpRigidBody* rb, hkQuaternion& newRotation) {
    rb->m_motion.m_motionState.m_transform.m_rotation.set(newRotation);
    rb->m_motion.m_motionState.m_transform.m_translation.m_quad = _mm_sub_ps(
        rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                        rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 85
                    ),
                    rb->m_motion.m_motionState.m_transform.m_rotation.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                        rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 0
                    ),
                    rb->m_motion.m_motionState.m_transform.m_rotation.m_col0.m_quad
                )
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(
                    rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad,
                    rb->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal.m_quad, 170
                ),
                rb->m_motion.m_motionState.m_transform.m_rotation.m_col2.m_quad
            )
        )
    );
}

hkClass* getSimulationClassType() {
    return &hkpSimulationClass;
}

hkpSimulation::hkpSimulation(hkpWorld* world)
  : m_determinismCheckFrameCounter(0), m_world(world), m_currentTime(0.0f), m_currentPsiTime(0.0f),
    m_simulateUntilTime(-1.0f), m_frameMarkerPsiSnap(0.0001f), m_previousStepResult(0) {
    m_lastProcessingStep = COLLIDE;
}

hkpSimulation::~hkpSimulation() {}

hkpStepResult hkpSimulation::stepDeltaTime(float physicsDeltaTime) {
    if (this->m_previousStepResult <= 1) {
        integrate(physicsDeltaTime);
    }
    uint32_t m_previousStepResult = this->m_previousStepResult;
    if (!m_previousStepResult || m_previousStepResult == 2) {
        collide();
    }
    uint32_t v4 = this->m_previousStepResult;
    if (!v4 || v4 == 3) {
        advanceTime();
    }
    if (this->m_previousStepResult) {
        if (m_world->getMemoryWatchDog()) {
            hkpWorldMemoryUtil::tryToRecoverFromMemoryErrors(m_world);
        }
    }
    return static_cast<hkpStepResult>(this->m_previousStepResult);
}

// The step's times go to the world's dynamics step info and to its collision input, the maintenance manager runs on
// them, then the islands integrate. The step info local is 16-aligned.
hkpStepResult hkpSimulation::integrate(float physicsDeltaTime) {
    // HK_TIMER_BEGIN writes into the stream's raw buffer, as in advanceTime.
    HK_TIMER_BEGIN("Physics", nullptr);
    m_physicsDeltaTime = physicsDeltaTime;
    alignas(16) hkStepInfo stepInfo;
    float endTime = physicsDeltaTime + m_currentPsiTime;
    stepInfo.m_startTime = m_currentPsiTime;
    stepInfo.m_endTime = endTime;
    stepInfo.m_deltaTime = endTime - stepInfo.m_startTime;
    stepInfo.m_invDeltaTime = stepInfo.m_deltaTime == 0.0f ? 0.0f : 1.0f / stepInfo.m_deltaTime;
    m_world->m_dynamicsStepInfo.m_stepInfo = stepInfo;
    m_world->m_collisionInput->m_stepInfo = stepInfo;
    m_world->m_maintenanceMgr->performMaintenance(m_world, stepInfo);
    hkpStepResult stepResult = integrateInternal(stepInfo);
    m_previousStepResult = stepResult;
    if (!stepResult) {
        m_lastProcessingStep.m_storage = INTEGRATE;
    }
    HK_TIMER_END();
    return stepResult;
}

// Collides the islands over the next psi step, or collides again after a failed step. Running out of memory fails the
// step; otherwise the psi time advances and the post-collide listeners hear about it.
hkpStepResult hkpSimulation::collide() {
    HK_TIMER_BEGIN("Physics", nullptr);
    if (m_previousStepResult) {
        const hkpStepResult stepResult = reCollideAfterStepFailure();
        HK_TIMER_END();
        return stepResult;
    }
    alignas(16) hkStepInfo stepInfo;
    const float startTime = m_currentPsiTime;
    const float endTime = m_currentPsiTime + m_physicsDeltaTime;
    stepInfo.m_startTime = startTime;
    stepInfo.m_endTime = endTime;
    stepInfo.m_deltaTime = endTime - startTime;
    stepInfo.m_invDeltaTime = stepInfo.m_deltaTime == 0.0f ? 0.0f : 1.0f / stepInfo.m_deltaTime;
    collideInternal(stepInfo);
    if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
        m_previousStepResult = HK_STEP_RESULT_MEMORY_FAILURE_DURING_COLLIDE;
        HK_TIMER_END();
        return HK_STEP_RESULT_MEMORY_FAILURE_DURING_COLLIDE;
    }
    m_currentPsiTime = m_currentPsiTime + m_physicsDeltaTime;
    if (m_world->m_worldPostCollideListeners.m_size) {
        HK_TIMER_BEGIN("PostCollideCB", nullptr);
        hkpWorldCallbackUtil::firePostCollideCallback(m_world, stepInfo);
        HK_TIMER_END();
    }
    m_lastProcessingStep.m_storage = COLLIDE;
    m_previousStepResult = HK_STEP_RESULT_SUCCESS;
    HK_TIMER_END();
    return HK_STEP_RESULT_SUCCESS;
}

// The time to advance to becomes the current time; when the world's marker is reached the world's post-simulation
// listeners hear about it, between two monitor-stream timer entries.
hkpStepResult hkpSimulation::advanceTime() {
    float TimeToAdvanceTo = snapSimulateTimeAndGetTimeToAdvanceTo();
    m_currentTime = TimeToAdvanceTo;
    // The callbacks are skipped when the marker is past the time to advance to, or the comparison is unordered.
    if (m_simulateUntilTime <= TimeToAdvanceTo && m_world->m_worldPostSimulationListeners.m_size) {
        HK_TIMER_BEGIN("PostSimCb", nullptr);
        hkpWorldCallbackUtil::firePostSimulationCallback(m_world);
        HK_TIMER_END();
    }
    // Writes the step result and returns zero.
    m_previousStepResult = 0;
    return HK_STEP_RESULT_SUCCESS;
}

void hkpSimulation::collideEntitiesDiscrete(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    const hkStepInfo& stepInfo,
    hkpSimulation::FindContacts findExtraContacts
) {
    hkpProcessCollisionInput* m_collisionInput;
    float m_endTime;
    float m_deltaTime;
    float m_invDeltaTime;
    hkMemoryRouter* instance;
    hkpProcessCollisionInput apMemory;
    m_collisionInput = world->m_collisionInput;
    // Copies the hkpCollisionInput base's bytes before the copy constructor, as the game does; the base holds only
    // plain pointers, scalars and vectors, so the copy is harmless.
    __builtin_memcpy(static_cast<void*>(&apMemory), m_collisionInput, 0x50u);
    apMemory.m_spareAgentSector = nullptr;
    apMemory.~hkpProcessCollisionInput();
    ::new (&apMemory) hkpProcessCollisionInput(*m_collisionInput);
    m_endTime = (&stepInfo)->m_endTime;
    apMemory.m_stepInfo.m_startTime = (&stepInfo)->m_startTime;
    m_deltaTime = (&stepInfo)->m_deltaTime;
    apMemory.m_stepInfo.m_endTime = m_endTime;
    m_invDeltaTime = (&stepInfo)->m_invDeltaTime;
    ++world->m_criticalOperationsLockCount;
    apMemory.m_stepInfo.m_deltaTime = m_deltaTime;
    apMemory.m_stepInfo.m_invDeltaTime = m_invDeltaTime;
    hkpSimulation::collideEntitiesBroadPhaseDiscrete(entities, numEntities, world);
    collideEntitiesNarrowPhaseDiscrete(entities, numEntities, apMemory, findExtraContacts);
    if (((world->m_criticalOperationsLockCount--) == 1) && (!world->m_blockExecutingPendingOperations)) {
        if (world->m_pendingOperationsCount) {
            world->internal_executePendingOperations();
        }
        if (world->m_pendingOperationQueueCount == 1) {
            if (world->m_pendingBodyOperationsCount) {
                world->internal_executePendingBodyOperations();
            }
        }
    }
    if (apMemory.m_spareAgentSector) {
        instance = &hkMemoryRouter::getInstance();
        instance->m_heap->blockFree(apMemory.m_spareAgentSector, sizeof(hkpAgent1nSector));
    }
}

void hkpSimulation::resetCollisionInformationForEntities(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    hkpSimulation::ResetCollisionInformation resetInfo
) {
    if (resetInfo & RESET_TIM) {
        processAgentsOfEntities(
            entities, numEntities, *world->m_collisionInput,
            &fnvd::callMember<&hkpSimulation::processAgentResetCollisionInformation>, FIND_CONTACTS_DEFAULT
        );
    }
    if (resetInfo & RESET_AABB) {
        hkpEntityAabbUtil::entityBatchInvalidateAabb(entities, numEntities);
    }
}

void hkpSimulation::reintegrateAndRecollideEntities(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    int32_t reintegrateRecollideMode
) {
    double v5;
    hkpEntity** v9;
    alignas(16) hkStepInfo info;
    ++world->m_criticalOperationsLockCount;
    float CurrentTime = world->getCurrentTime();
    float CurrentPsiTime = world->getCurrentPsiTime();
    info.m_startTime = CurrentTime;
    info.m_endTime = CurrentPsiTime;
    info.m_deltaTime = CurrentPsiTime - CurrentTime;
    if (info.m_deltaTime == 0.0f) {
        v5 = 0.0;
    }
    else {
        v5 = 1.0 / info.m_deltaTime;
    }
    hkpProcessCollisionInput* m_collisionInput = world->m_collisionInput;
    info.m_invDeltaTime = v5;
    m_collisionInput->m_stepInfo = info;
    if ((reintegrateRecollideMode & 1) != 0) {
        for (int32_t i = 0; i < numEntities; ++i) {
            hkpEntity* v8 = entities[i];
            if (v8->m_motion.m_type.m_storage != 5) {
                hkMotionState* p_m_motionState = v8->m_motion.getMotionState();
                float v11 = world->getCurrentTime();
                hkSweptTransformUtil::backStepMotionState(v11, *p_m_motionState);
            }
        }
        // The callee adds the offset to each array element, so it treats the entity pointers as untyped.
        hkRigidMotionUtilStep(
            info, reinterpret_cast<hkpMotion* const*>(entities), numEntities,
            static_cast<int32_t>(offsetof(hkpEntity, m_motion))
        );
        v9 = entities;
        hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, entities, numEntities);
    }
    else {
        v9 = entities;
    }
    if ((reintegrateRecollideMode & 2) != 0) {
        hkpSimulation::collideEntitiesBroadPhaseDiscrete(v9, numEntities, world);
    }
    if ((reintegrateRecollideMode & 4) != 0) {
        collideEntitiesNarrowPhaseDiscrete(
            v9, numEntities, *world->m_collisionInput, hkpSimulation::FIND_CONTACTS_DEFAULT
        );
    }
    else {
        if ((reintegrateRecollideMode & 1) != 0) {
            resetCollisionInformationForEntities(v9, numEntities, world, hkpSimulation::RESET_TIM);
        }
    }
    if (world->m_criticalOperationsLockCount-- == 1 && !world->m_blockExecutingPendingOperations.m_bool) {
        if (world->m_pendingOperationsCount) {
            world->internal_executePendingOperations();
        }
        if (world->m_pendingOperationQueueCount == 1) {
            if (world->m_pendingBodyOperationsCount) {
                world->internal_executePendingBodyOperations();
            }
        }
    }
}

// Copies the step info into the world and its collision input, derives the solver's substep times from it, then, with
// the world's critical operations locked, runs the broad phase over every active island and, after the pending
// operations had their chance, the narrow phase per island with its post-collide callbacks. Running out of memory stops
// either phase.
void hkpSimulation::collideInternal(const hkStepInfo& stepInfoIn) {
    HK_TIMER_BEGIN("Collide", nullptr);
    m_world->m_dynamicsStepInfo.m_stepInfo = stepInfoIn;
    m_world->m_collisionInput->m_stepInfo = stepInfoIn;
    hkpSolverInfo& solverInfo = m_world->m_dynamicsStepInfo.m_solverInfo;
    solverInfo.m_deltaTime = solverInfo.m_invNumSteps * stepInfoIn.m_deltaTime;
    // The step count is converted to double, not rounded to float, before the product.
    solverInfo.m_invDeltaTime = static_cast<double>(solverInfo.m_numSteps) * stepInfoIn.m_invDeltaTime;
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++m_world->m_criticalOperationsLockCount;
    {
        const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& activeIslands =
            m_world->m_activeSimulationIslands;
        for (int32_t i = 0; i < activeIslands.m_size; ++i) {
            hkpSimulationIsland* activeIsland = activeIslands.m_data[i];
            collideEntitiesBroadPhaseDiscrete(
                activeIsland->m_entities.m_data, activeIsland->m_entities.m_size, m_world
            );
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                hkpWorld* world = m_world;
                // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
                --world->m_criticalOperationsLockCount;
                world->attemptToExecutePendingOperations();
                HK_TIMER_END();
                return;
            }
        }
    }
    {
        hkpWorld* world = m_world;
        // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
        --world->m_criticalOperationsLockCount;
        world->attemptToExecutePendingOperations();
    }
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++m_world->m_criticalOperationsLockCount;
    {
        const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& activeIslands =
            m_world->m_activeSimulationIslands;
        for (int32_t i = 0; i < activeIslands.m_size; ++i) {
            hkpSimulationIsland* activeIsland = activeIslands.m_data[i];
            hkpProcessCollisionInput* input = m_world->m_collisionInput;
            HK_TIMER_BEGIN("NarrowPhase", nullptr);
            input->m_collisionQualityInfo =
                input->m_dispatcher->getCollisionQualityInfo(hkpCollisionDispatcher::COLLISION_QUALITY_PSI);
            // EXE-INLINED none: hkPadSpu<int>::operator=
            input->m_createPredictiveAgents.m_storage = 0;
            hkAgentNnMachine_ProcessTrack(activeIsland, activeIsland->m_agentTrack, *input);
            HK_TIMER_END();
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                break;
            }
            if (m_world->m_islandPostCollideListeners.m_size) {
                HK_TIMER_BEGIN("IslandPostCollideCb", nullptr);
                hkpWorldCallbackUtil::fireIslandPostCollideCallback(m_world, activeIsland, stepInfoIn);
                HK_TIMER_END();
            }
        }
    }
    {
        hkpWorld* world = m_world;
        // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
        --world->m_criticalOperationsLockCount;
        world->attemptToExecutePendingOperations();
    }
    HK_TIMER_END();
}

void hkpSimulation::collideEntitiesNarrowPhaseDiscrete(
    hkpEntity** entities,
    int32_t numEntities,
    const hkpProcessCollisionInput& input,
    FindContacts findExtraContacts
) {
    processAgentsOfEntities(
        entities, numEntities, input, &fnvd::callMember<&hkpSimulation::processAgentCollideDiscrete>, findExtraContacts
    );
}

bool hkpSimulation::isSimulationAtMarker() {
    return (bool)((uint8_t)(m_simulateUntilTime == m_currentTime));
}

void hkpSimulation::setFrameTimeMarker(float a1) {
    m_simulateUntilTime = (m_currentTime + a1);
}

void hkpSimulation::processAgentResetCollisionInformation(
    hkpAgentNnEntry* a1,
    const hkpProcessCollisionInput& a2,
    hkpProcessCollisionOutput& a3
) {
    hkAgentNnMachine_InvalidateTimInAgent(a1, a2);
}

void hkpSimulation::checkDeltaTimeIsOk(float deltaTime) {
    ;
}

void hkpSimulation::applyActions() {
    // Both bounds are reloaded every iteration, as the game does: the island count from the world's array and the
    // action count from the island's.
    for (int32_t i = 0; i < m_world->m_activeSimulationIslands.m_size; ++i) {
        hkpSimulationIsland* island = m_world->m_activeSimulationIslands.m_data[i];
        for (int32_t j = 0; j < island->m_actions.m_size; ++j) {
            island->m_actions.m_data[j]->applyAction(m_world->m_dynamicsStepInfo.m_stepInfo);
        }
    }
}

// Rechecks the world's filter, then collides again over the failed step: after a TOI solve failure from the current
// time to the psi time, otherwise over the next psi step. Out of memory again keeps the old result; otherwise a failed
// collide advances the psi time and tells the post-collide listeners, as collide does. The step info local is
// 16-aligned.
hkpStepResult hkpSimulation::reCollideAfterStepFailure() {
    m_world->updateCollisionFilterOnWorld(
        HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK, HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
    );
    alignas(16) hkStepInfo stepInfo;
    if (m_previousStepResult == HK_STEP_RESULT_MEMORY_FAILURE_DURING_TOI_SOLVE) {
        const float startTime = m_currentTime;
        const float endTime = m_currentPsiTime;
        stepInfo.m_startTime = startTime;
        stepInfo.m_endTime = endTime;
    }
    else {
        const float startTime = m_currentPsiTime;
        const float endTime = m_physicsDeltaTime + m_currentPsiTime;
        stepInfo.m_startTime = startTime;
        stepInfo.m_endTime = endTime;
    }
    stepInfo.m_deltaTime = stepInfo.m_endTime - stepInfo.m_startTime;
    stepInfo.m_invDeltaTime = stepInfo.m_deltaTime == 0.0f ? 0.0f : 1.0f / stepInfo.m_deltaTime;
    collideInternal(stepInfo);
    if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
        // HK_TIMER_END writes into the stream's raw buffer, as in advanceTime.
        HK_TIMER_END();
        return static_cast<hkpStepResult>(m_previousStepResult);
    }
    if (m_previousStepResult == HK_STEP_RESULT_MEMORY_FAILURE_DURING_COLLIDE) {
        m_currentPsiTime = m_physicsDeltaTime + m_currentPsiTime;
        if (m_world->m_worldPostCollideListeners.m_size) {
            HK_TIMER_BEGIN("PostCollideCB", nullptr);
            hkpWorldCallbackUtil::firePostCollideCallback(m_world, stepInfo);
            HK_TIMER_END();
        }
        m_lastProcessingStep.m_storage = COLLIDE;
    }
    m_previousStepResult = HK_STEP_RESULT_SUCCESS;
    return HK_STEP_RESULT_SUCCESS;
}

float hkpSimulation::snapSimulateTimeAndGetTimeToAdvanceTo() {
    if (-1.0f != this->m_simulateUntilTime) {
        float v3 = this->m_simulateUntilTime - this->m_currentPsiTime;
        float v4 = std::fabs((double)v3);
        if (m_frameMarkerPsiSnap > static_cast<double>(v4)) {
            this->m_simulateUntilTime = this->m_currentPsiTime;
        }
    }
    float m_currentPsiTime = this->m_currentPsiTime;
    if (-1.0f == this->m_simulateUntilTime) {
        return m_currentPsiTime;
    }
    float m_simulateUntilTime = this->m_simulateUntilTime;
    if (!(m_simulateUntilTime > static_cast<double>(m_currentPsiTime))) {
        return m_simulateUntilTime;
    }
    return m_currentPsiTime;
}

// Sets up the world's step and solver info for the step, checks the solver allocator can hold the integration (or has
// the memory watchdog make room; without one the step fails), applies the actions, advances the deactivation counters,
// then integrates each active island from the last: islands without constraints step their motions directly, the others
// go through the constraint solver. Islands inactive long enough are marked inactive; the island and world
// post-integrate listeners are told.
hkpStepResult hkpSimulation::integrateInternal(const hkStepInfo& stepInfoIn) {
    // HK_TIMER_BEGIN_LIST writes a 16-byte entry with the first split's name. Every timer entry here goes into the
    // stream's raw buffer, which is treated as untyped (hence the casts to the command layouts).
    HK_TIMER_BEGIN_LIST("Integrate", "Init");
    m_world->m_dynamicsStepInfo.m_stepInfo = stepInfoIn;
    hkpSolverInfo& solverInfo = m_world->m_dynamicsStepInfo.m_solverInfo;
    solverInfo.m_deltaTime = solverInfo.m_invNumSteps * stepInfoIn.m_deltaTime;
    solverInfo.m_invDeltaTime = static_cast<float>(solverInfo.m_numSteps) * stepInfoIn.m_invDeltaTime;
    // EXE-INLINED 0x630AE0
    {
        const float subStepDeltaTime = solverInfo.m_deltaTime;
        const hkQuadReal& gravity = m_world->m_gravity.m_quad;
        solverInfo.m_globalAccelerationPerSubStep.m_quad[0] = subStepDeltaTime * gravity[0];
        solverInfo.m_globalAccelerationPerSubStep.m_quad[1] = subStepDeltaTime * gravity[1];
        solverInfo.m_globalAccelerationPerSubStep.m_quad[2] = subStepDeltaTime * gravity[2];
        solverInfo.m_globalAccelerationPerSubStep.m_quad[3] = subStepDeltaTime * gravity[3];
    }
    // EXE-INLINED 0x630AE0
    {
        const float stepDeltaTime = stepInfoIn.m_deltaTime;
        const hkQuadReal& gravity = m_world->m_gravity.m_quad;
        solverInfo.m_globalAccelerationPerStep.m_quad[0] = stepDeltaTime * gravity[0];
        solverInfo.m_globalAccelerationPerStep.m_quad[1] = stepDeltaTime * gravity[1];
        solverInfo.m_globalAccelerationPerStep.m_quad[2] = stepDeltaTime * gravity[2];
        solverInfo.m_globalAccelerationPerStep.m_quad[3] = stepDeltaTime * gravity[3];
    }
    hkSolveUpload();
    hkpWorldOperationUtil::cleanupDirtyIslands(m_world);
    hkpWorld::MemUsageInfo memUsage;
    m_world->getMemUsageForIntegration(memUsage);
    if (!hkMemoryRouter::getInstance().solver().canAllocSingleBlock(memUsage.m_maxRuntimeBlockSize)) {
        if (!m_world->getMemoryWatchDog()) {
            m_previousStepResult = HK_STEP_RESULT_MEMORY_FAILURE_BEFORE_INTEGRATION;
            return HK_STEP_RESULT_MEMORY_FAILURE_BEFORE_INTEGRATION;
        }
        // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
        {
            hkpWorld* world = m_world;
            if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
                if (world->m_pendingOperationsCount) {
                    world->internal_executePendingOperations();
                }
                if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
                    world->internal_executePendingBodyOperations();
                }
            }
        }
        hkpWorldMemoryUtil::checkMemoryForIntegration(m_world);
        ++m_world->m_criticalOperationsLockCount;
    }
    ++m_world->m_criticalOperationsLockCount;
    --m_world->m_criticalOperationsLockCountForPhantoms;
    HK_TIMER_SPLIT_LIST("Actions");
    applyActions();
    ++m_world->m_criticalOperationsLockCountForPhantoms;
    // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
    {
        hkpWorld* world = m_world;
        if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
            if (world->m_pendingOperationsCount) {
                world->internal_executePendingOperations();
            }
            if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
                world->internal_executePendingBodyOperations();
            }
        }
    }
    ++m_world->m_criticalOperationsLockCount;
    // The deactivation's frame counter: the two select flags of the inactive-frame counters toggle on their frames.
    {
        hkpSolverInfo& info = m_world->m_dynamicsStepInfo.m_solverInfo;
        const uint8_t counter = ++info.m_deactivationIntegrateCounter;
        if (((counter - 4) & 7) == 0) {
            info.m_deactivationNumInactiveFramesSelectFlag[0] ^= 1;
        }
        if ((counter & 7) == 0) {
            info.m_deactivationNumInactiveFramesSelectFlag[0] ^= 2;
        }
        if ((counter & 0xF) == 0) {
            const uint8_t flag = static_cast<uint8_t>(1 - info.m_deactivationNumInactiveFramesSelectFlag[1]);
            info.m_deactivationIntegrateCounter = 0;
            info.m_deactivationNumInactiveFramesSelectFlag[1] = flag;
        }
    }
    alignas(16) hkpConstraintQueryIn queryIn;

    queryIn.set(m_world->m_dynamicsStepInfo.m_solverInfo, stepInfoIn, m_world->m_violatedConstraintArray);
    HK_TIMER_SPLIT_LIST("Integrate");
    // Takes the island array's address once and reloads its data each iteration, last island first.
    hkArray<hkpSimulationIsland*>& activeIslands = m_world->m_activeSimulationIslands;
    for (int32_t i = activeIslands.m_size - 1; i >= 0; --i) {
        hkpSimulationIsland* activeIsland = activeIslands.m_data[i];
        hkpWorldDynamicsStepInfo& stepInfo = m_world->m_dynamicsStepInfo;
        int32_t numInactiveFrames;
        if (activeIsland->m_constraintInfo.m_sizeOfSchemas == 0) {
            HK_TIMER_BEGIN("SingleObj", nullptr);
            // Each entity pointer stands for its motion, m_motion bytes further on.
            numInactiveFrames = hkRigidMotionUtilApplyForcesAndStep(
                stepInfo.m_solverInfo, stepInfo.m_stepInfo, stepInfo.m_solverInfo.m_globalAccelerationPerStep,
                reinterpret_cast<hkpMotion* const*>(activeIsland->m_entities.m_data), activeIsland->m_entities.m_size,
                offsetof(hkpEntity, m_motion)
            );
            hkpEntityAabbUtil::entityBatchRecalcAabb(
                activeIsland->m_world->m_collisionInput, activeIsland->m_entities.m_data,
                activeIsland->m_entities.m_size
            );
            HK_TIMER_END();
        }
        else {
            numInactiveFrames = hkpConstraintSolverSetup::solve(
                stepInfo.m_stepInfo, stepInfo.m_solverInfo, queryIn, *activeIsland, nullptr, 0,
                activeIsland->m_entities.m_data, activeIsland->m_entities.m_size
            );
        }
        if (numInactiveFrames > 5 && activeIsland->m_activeMark) {
            hkpWorld* world = activeIsland->m_world;
            if (world->m_wantDeactivation) {
                hkpWorldOperationUtil::markIslandInactive(world, activeIsland);
            }
        }
        if (m_world->m_islandPostIntegrateListeners.m_size) {
            HK_TIMER_SPLIT_LIST("IslandPostIntegrateCb");
            hkpWorldCallbackUtil::fireIslandPostIntegrateCallback(m_world, activeIsland, stepInfoIn);
        }
    }
    // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
    {
        hkpWorld* world = m_world;
        if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
            if (world->m_pendingOperationsCount) {
                world->internal_executePendingOperations();
            }
            if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
                world->internal_executePendingBodyOperations();
            }
        }
    }
    if (m_world->m_worldPostIntegrateListeners.m_size) {
        HK_TIMER_BEGIN("WorldPostIntegrateCb", nullptr);
        hkpWorldCallbackUtil::firePostIntegrateCallback(m_world, stepInfoIn);
        HK_TIMER_END();
    }
    HK_TIMER_END_LIST();
    return HK_STEP_RESULT_SUCCESS;
}

// Runs processFunc once on every agent of the entities: an agent whose partner was handled before (an entity earlier in
// the list) is skipped. With FIND_CONTACTS_EXTRA, an agent that found contacts is run three more times with the entity
// turned about its centre of mass (by the angle that brings the first contact's arm to the tolerance about the normal x
// arm axis, then by the same angle both ways about the axis perpendicular to that axis and the normal) under twenty
// times the input's tolerance, and once more at its own transform with the tolerances restored. The input's tolerance
// and the quality info's create4dContact and createContact are saved and restored as plain float copies.
void hkpSimulation::processAgentsOfEntities(
    hkpEntity** entities,
    int32_t numEntities,
    const hkpProcessCollisionInput& input,
    hkpSimulation::AgentEntityCallback processFunc,
    hkpSimulation::FindContacts findExtraContacts
) {
    // The entities handled so far, by their collidables.
    using entitiesProcessed_t = hkPointerMap<hkpLinkedCollidable*, int32_t>;
    FNVD_INLINED_LOCAL(entitiesProcessed_t, entitiesProcessed, std::nullopt, std::nullopt);
    // EXE-INLINED 0x626630
    entitiesProcessed.m_map.m_elem = nullptr;
    entitiesProcessed.m_map.m_numElems = 0;
    entitiesProcessed.m_map.m_hashMod = -1;
    entitiesProcessed.m_map.hkUlongMapBase::reserve(hkMemoryRouter::getInstance().heap(), numEntities);
    // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput
    hkpProcessCollisionOutput processOutput;
    processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
    processOutput.m_constraintOwner.m_storage = nullptr;
    processOutput.m_toi.m_properties.m_impulseApplied = 0.0f;
    processOutput.m_toi.m_properties.m_internalSolverData = 0.0f;
    FNVD_INLINED_LOCAL(hkArray<hkpLinkedCollidable::CollisionEntry>, collisionEntries, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkpLinkedCollidable::CollisionEntry>::hkArray
    collisionEntries.m_data = nullptr;
    collisionEntries.m_size = 0;
    collisionEntries.m_capacityAndFlags = hkArrayBase<hkpLinkedCollidable::CollisionEntry>::DONT_DEALLOCATE_FLAG;
    for (int32_t i = 0; i < numEntities; ++i) {
        hkpEntity* entity = entities[i];
        hkpLinkedCollidable* lColl = &entity->m_collidable;
        entitiesProcessed.insert(lColl, 0);
        lColl->getCollisionEntriesSorted(collisionEntries);
        for (int32_t j = 0; j < collisionEntries.m_size; ++j) {
            const hkpLinkedCollidable::CollisionEntry& centry = collisionEntries.m_data[j];
            if (entitiesProcessed.isValid(entitiesProcessed.findKey(centry.m_partner))) {
                continue;
            }
            hkpAgentNnEntry* agentEntry = centry.m_agentEntry;
            hkpEntity* entityA = static_cast<hkpEntity*>(agentEntry->m_collidable[0]->getOwner());
            // EXE-INLINED none: hkpEntity::isFixed
            hkpSimulationIsland* island = entityA->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED
                ? static_cast<hkpEntity*>(agentEntry->m_collidable[1]->getOwner())->getSimulationIsland()
                : entityA->getSimulationIsland();
            processOutput.m_constraintOwner.m_storage = island;
            hkAgentNnMachine_InvalidateTimInAgent(agentEntry, input);
            processFunc(this, agentEntry, input, processOutput);
            if (findExtraContacts == FIND_CONTACTS_EXTRA &&
                processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
                hkMotionState& ms = entity->m_motion.m_motionState;
                const hkQuaternion origRotation = ms.m_sweptTransform.m_rotation1;
                const hkTransform origTransform = ms.m_transform;
                const hkContactPoint& contact = processOutput.m_contactPoints[0].m_contact;
                // The contact normal, pointing away from the entity's side of the agent.
                hkVector4 normal = contact.m_separatingNormal;
                if (centry.m_agentEntry->m_collidable[0] != lColl) {
                    // EXE-INLINED none: hkVector4::setNeg4
                    normal.m_quad = _mm_xor_ps(contact.m_separatingNormal.m_quad, hkQuadRealNegateMask);
                }
                // EXE-INLINED 0x458A10
                const __m128 arm = _mm_sub_ps(ms.m_sweptTransform.m_centerOfMass1.m_quad, contact.m_position.m_quad);
                const __m128 n = normal.m_quad;
                // The axis normal x arm.
                // EXE-INLINED 0x630DF0
                hkVector4 axis;
                axis.m_quad = _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(n, n, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(arm, arm, _MM_SHUFFLE(3, 1, 0, 2))
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(n, n, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(arm, arm, _MM_SHUFFLE(3, 0, 2, 1))
                    )
                );
                // The lengths and the dot product: the SDK's length3 and dot3 (the lanes summed y + x, then z + that;
                // the scalar adds as _ps, their upper lanes unread).
                const __m128 axisSq = _mm_mul_ps(axis.m_quad, axis.m_quad);
                float axisLength;
                _mm_store_ss(
                    &axisLength,
                    _mm_sqrt_ss(_mm_add_ps(
                        _mm_shuffle_ps(axisSq, axisSq, _MM_SHUFFLE(2, 2, 2, 2)),
                        _mm_add_ps(_mm_shuffle_ps(axisSq, axisSq, _MM_SHUFFLE(1, 1, 1, 1)), axisSq)
                    ))
                );
                const __m128 armDotNormal = _mm_mul_ps(arm, n);
                float dot;
                _mm_store_ss(
                    &dot,
                    _mm_add_ps(
                        _mm_shuffle_ps(armDotNormal, armDotNormal, _MM_SHUFFLE(2, 2, 2, 2)),
                        _mm_add_ps(_mm_shuffle_ps(armDotNormal, armDotNormal, _MM_SHUFFLE(1, 1, 1, 1)), armDotNormal)
                    )
                );
                const __m128 armSq = _mm_mul_ps(arm, arm);
                float armLength;
                _mm_store_ss(
                    &armLength,
                    _mm_sqrt_ss(_mm_add_ps(
                        _mm_shuffle_ps(armSq, armSq, _MM_SHUFFLE(2, 2, 2, 2)),
                        _mm_add_ps(_mm_shuffle_ps(armSq, armSq, _MM_SHUFFLE(1, 1, 1, 1)), armSq)
                    ))
                );
                // Rounded to float before the compare, as the game does.
                float cosAngle = dot / armLength;
                // The 0.9999 limit is a double constant.
                if (axisLength > 1e-5f && cosAngle < 0.9998999834060669) {
                    hkpProcessCollisionInput& processInput = const_cast<hkpProcessCollisionInput&>(input);
                    const float savedTolerance = input.m_tolerance;
                    float extraTolerance = input.m_tolerance * 20.0;
                    const float tolerance = extraTolerance;
                    processInput.m_tolerance = tolerance;
                    hkpCollisionQualityInfo* qualityInfo = input.m_collisionQualityInfo;
                    const float savedCreate4dContact = qualityInfo->m_create4dContact;
                    const float savedCreateContact = qualityInfo->m_createContact;
                    qualityInfo->m_create4dContact = tolerance;
                    input.m_collisionQualityInfo->m_createContact = tolerance;
                    float cosTarget = (dot - tolerance * 0.05) / armLength;
                    const float cosTargetValue = cosTarget;
                    const float cosLimited = cosTargetValue > 0.0f ? cosTargetValue : 0.0f;
                    float asinAngle = hkMath::asin(cosAngle);
                    const float angle = asinAngle - hkMath::asin(cosLimited);
                    const float invAxisLength = 1.0f / axisLength;
                    // EXE-INLINED 0x627920
                    axis.m_quad = _mm_mul_ps(_mm_set1_ps(invAxisLength), axis.m_quad);
                    hkQuaternion turn;
                    turn.setAxisAngle(axis, angle);
                    hkQuaternion rotation;
                    rotation.setMul(turn, ms.m_sweptTransform.m_rotation1);
                    // EXE-INLINED 0xCF8B60
                    ms.m_transform.m_rotation.set(rotation);
                    __m128 comLocal = ms.m_sweptTransform.m_centerOfMassLocal.m_quad;
                    ms.m_transform.m_translation.m_quad = _mm_sub_ps(
                        ms.m_sweptTransform.m_centerOfMass1.m_quad,
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(1, 1, 1, 1)),
                                    ms.m_transform.m_rotation.m_col1.m_quad
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(0, 0, 0, 0)),
                                    ms.m_transform.m_rotation.m_col0.m_quad
                                )
                            ),
                            _mm_mul_ps(
                                _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(2, 2, 2, 2)),
                                ms.m_transform.m_rotation.m_col2.m_quad
                            )
                        )
                    );
                    hkAgentNnMachine_InvalidateTimInAgent(agentEntry, input);
                    processFunc(this, agentEntry, input, processOutput);

                    // The axis perpendicular to the first one and the normal, normalized: the inverse length is an
                    // estimate refined by one Newton step, and is 0 for a zero length.
                    // EXE-INLINED 0x630DF0
                    const __m128 a = axis.m_quad;
                    const __m128 perp = _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(n, n, _MM_SHUFFLE(3, 1, 0, 2))
                        ),
                        _mm_mul_ps(
                            _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(n, n, _MM_SHUFFLE(3, 0, 2, 1))
                        )
                    );
                    // EXE-INLINED 0x62AEA0
                    const __m128 perpSq = _mm_mul_ps(perp, perp);
                    const __m128 len2 = _mm_add_ps(
                        _mm_shuffle_ps(perpSq, perpSq, _MM_SHUFFLE(2, 2, 2, 2)),
                        _mm_add_ps(_mm_shuffle_ps(perpSq, perpSq, _MM_SHUFFLE(1, 1, 1, 1)), perpSq)
                    );
                    const __m128 approx = _mm_rsqrt_ss(len2);
                    const __m128 len2IsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
                    const __m128 refined = _mm_mul_ps(
                        _mm_mul_ps(_mm_set_ss(0.5f), approx),
                        _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(len2, approx), approx))
                    );
                    const __m128 invLength = _mm_andnot_ps(len2IsZero, refined);
                    hkVector4 perpAxis;
                    perpAxis.m_quad = _mm_mul_ps(_mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)), perp);
                    hkQuaternion perpTurn;
                    perpTurn.setAxisAngle(perpAxis, angle);
                    hkQuaternion perpRotation;
                    perpRotation.setMul(perpTurn, origRotation);
                    // EXE-INLINED 0xCF8B60
                    ms.m_transform.m_rotation.set(perpRotation);
                    comLocal = ms.m_sweptTransform.m_centerOfMassLocal.m_quad;
                    ms.m_transform.m_translation.m_quad = _mm_sub_ps(
                        ms.m_sweptTransform.m_centerOfMass1.m_quad,
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(1, 1, 1, 1)),
                                    ms.m_transform.m_rotation.m_col1.m_quad
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(0, 0, 0, 0)),
                                    ms.m_transform.m_rotation.m_col0.m_quad
                                )
                            ),
                            _mm_mul_ps(
                                _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(2, 2, 2, 2)),
                                ms.m_transform.m_rotation.m_col2.m_quad
                            )
                        )
                    );
                    hkAgentNnMachine_InvalidateTimInAgent(agentEntry, input);
                    processFunc(this, agentEntry, input, processOutput);

                    perpRotation.setInverseMul(perpTurn, origRotation);
                    // EXE-INLINED 0xCF8B60
                    ms.m_transform.m_rotation.set(perpRotation);
                    comLocal = ms.m_sweptTransform.m_centerOfMassLocal.m_quad;
                    ms.m_transform.m_translation.m_quad = _mm_sub_ps(
                        ms.m_sweptTransform.m_centerOfMass1.m_quad,
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(1, 1, 1, 1)),
                                    ms.m_transform.m_rotation.m_col1.m_quad
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(0, 0, 0, 0)),
                                    ms.m_transform.m_rotation.m_col0.m_quad
                                )
                            ),
                            _mm_mul_ps(
                                _mm_shuffle_ps(comLocal, comLocal, _MM_SHUFFLE(2, 2, 2, 2)),
                                ms.m_transform.m_rotation.m_col2.m_quad
                            )
                        )
                    );
                    hkAgentNnMachine_InvalidateTimInAgent(agentEntry, input);
                    processFunc(this, agentEntry, input, processOutput);

                    processInput.m_tolerance = savedTolerance;
                    input.m_collisionQualityInfo->m_create4dContact = savedCreate4dContact;
                    input.m_collisionQualityInfo->m_createContact = savedCreateContact;
                    hkpMotion* motion = entity->getMotion();
                    motion->setTransform(origTransform);
                    hkAgentNnMachine_InvalidateTimInAgent(agentEntry, input);
                    processFunc(this, agentEntry, input, processOutput);
                }
            }
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                goto done;
            }
        }
    }
done:
    // EXE-INLINED none: hkArray<hkpLinkedCollidable::CollisionEntry>::~hkArray
    collisionEntries.clearAndDeallocate();
    // EXE-INLINED 0x6264F0
    entitiesProcessed.m_map.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
    entitiesProcessed.m_map.destructOperations();
}

bool hkpSimulation::isSimulationAtPsi() const {
    return m_currentTime == m_currentPsiTime;
}

void hkpSimulation::processAgentCollideDiscrete(
    hkpAgentNnEntry* entry,
    const hkpProcessCollisionInput& processInput,
    hkpProcessCollisionOutput& processOutput
) {
    // EXE-INLINED none: hkpProcessCollisionOutput::reset
    processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
    processOutput.m_potentialContacts.m_storage = 0;
    processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
    hkpCollisionDispatcher* dispatcher = processInput.m_dispatcher;
    processInput.m_collisionQualityInfo =
        dispatcher->getCollisionQualityInfo(hkpCollisionDispatcher::COLLISION_QUALITY_PSI);
    processInput.m_createPredictiveAgents.m_storage =
        dispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex)->m_useContinuousPhysics.m_storage;
    hkAgentNnMachine_ProcessAgent(entry, processInput, processOutput, entry->m_contactMgr);
    if (hkOutOfMemoryState != hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY &&
        processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
        entry->m_contactMgr->processContactImpl(
            *entry->m_collidable[0], *entry->m_collidable[1], processInput, processOutput
        );
    }
}

// The broadphase pass of a discrete step: the entities' boxes go to the broadphase, then the overlaps it found new
// and gone are handed to the dispatcher (agents removed, then added when the heap can take them).
void hkpSimulation::collideEntitiesBroadPhaseDiscrete(hkpEntity** delPairs, int32_t newPairs, hkpWorld* assertBuf) {
    hkpEntity** entities = delPairs;
    const int32_t numEntities = newPairs;
    hkpWorld* world = assertBuf;

    HK_TIMER_BEGIN_LIST("BroadPhase", "InitMem");

    const int32_t iAddedCapacity = world->m_broadPhaseUpdateSize;
    FNVD_INLINED_LOCAL(hkLocalArray<hkpBroadPhaseHandlePair>, addedPairs, std::nullopt, std::nullopt);
    addedPairs.m_data = nullptr;
    addedPairs.m_size = 0;
    addedPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkAllocateStack<hkpBroadPhaseHandlePair>
    hkStackMemory& rAddedStack = hkMemoryRouter::getInstance().m_stack;
    void* pAddedBlock = rAddedStack.m_next.m_storage;
    rAddedStack.m_next.m_storage = static_cast<char*>(pAddedBlock) +
        ((iAddedCapacity * static_cast<int32_t>(sizeof(hkpBroadPhaseHandlePair)) + 127) & ~127);
    addedPairs.m_capacityAndFlags = static_cast<int32_t>(
        static_cast<uint32_t>(iAddedCapacity) | hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG
    );
    hkpBroadPhaseHandlePair* pAddedLocal = static_cast<hkpBroadPhaseHandlePair*>(pAddedBlock);
    addedPairs.m_data = pAddedLocal;
    addedPairs.m_localMemory = pAddedLocal;

    const int32_t iRemovedCapacity = world->m_broadPhaseUpdateSize;
    FNVD_INLINED_LOCAL(hkLocalArray<hkpBroadPhaseHandlePair>, removedPairs, std::nullopt, std::nullopt);
    removedPairs.m_data = nullptr;
    removedPairs.m_size = 0;
    removedPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkAllocateStack<hkpBroadPhaseHandlePair>
    hkStackMemory& rRemovedStack = hkMemoryRouter::getInstance().m_stack;
    void* pRemovedBlock = rRemovedStack.m_next.m_storage;
    rRemovedStack.m_next.m_storage = static_cast<char*>(pRemovedBlock) +
        ((iRemovedCapacity * static_cast<int32_t>(sizeof(hkpBroadPhaseHandlePair)) + 127) & ~127);
    hkpBroadPhaseHandlePair* pRemovedLocal = static_cast<hkpBroadPhaseHandlePair*>(pRemovedBlock);
    removedPairs.m_data = pRemovedLocal;
    removedPairs.m_capacityAndFlags = static_cast<int32_t>(
        static_cast<uint32_t>(iRemovedCapacity) | hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG
    );
    removedPairs.m_localMemory = pRemovedLocal;

    {
        // hkLocalBuffer<hkAabbUint32> aabbs(numEntities), keeping only its pointer.
        // EXE-INLINED none: hkAllocateStack<hkAabbUint32>
        hkStackMemory& rAabbStack = hkMemoryRouter::getInstance().m_stack;
        void* pAabbBlock = rAabbStack.m_next.m_storage;
        rAabbStack.m_next.m_storage =
            static_cast<char*>(pAabbBlock) + ((numEntities * static_cast<int32_t>(sizeof(hkAabbUint32)) + 127) & ~127);
        hkAabbUint32* aabbs = static_cast<hkAabbUint32*>(pAabbBlock);
        // hkLocalBuffer<hkpBroadPhaseHandle*> phandles(numEntities), keeping only its pointer.
        // EXE-INLINED none: hkAllocateStack<hkpBroadPhaseHandle *>
        hkStackMemory& rHandleStack = hkMemoryRouter::getInstance().m_stack;
        void* pHandleBlock = rHandleStack.m_next.m_storage;
        rHandleStack.m_next.m_storage = static_cast<char*>(pHandleBlock) +
            ((numEntities * static_cast<int32_t>(sizeof(hkpBroadPhaseHandle*)) + 127) & ~127);
        hkpBroadPhaseHandle** phandles = static_cast<hkpBroadPhaseHandle**>(pHandleBlock);

        HK_TIMER_SPLIT_LIST("CalcAabbs");
        for (int32_t i = 0; i < numEntities; i++) {
            // Its address goes to entityBatchRecalcAabb.
            hkpEntity* entity = entities[i];
            hkpCollidable* collidable = &entity->m_collidable;
            phandles[i] = collidable->getBroadPhaseHandle();
            if (!collidable->m_boundingVolumeData.isValid()) {
                hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, &entity, 1);
            }
            // The bounding volume's first 32 bytes are the box: hkAabbUint32::operator= over them.
            // EXE-INLINED 0xCF8690
            hkString::memCpy16NonEmpty(
                &aabbs[i], &collidable->m_boundingVolumeData, static_cast<int32_t>(sizeof(hkAabbUint32)) / 16
            );
        }

        HK_TIMER_SPLIT_LIST("3AxisSweep");
        world->getBroadPhase()->updateAabbsUint32(phandles, aabbs, numEntities, addedPairs, removedPairs);

        // EXE-INLINED none: hkDeallocateStack<hkpBroadPhaseHandle *>
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = static_cast<char*>(pHandleBlock);
        // EXE-INLINED none: hkDeallocateStack<hkAabbUint32>
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = static_cast<char*>(pAabbBlock);
    }

    if (addedPairs.m_size + removedPairs.m_size > 0) {
        HK_TIMER_SPLIT_LIST("RemoveDup");
        hkpTypedBroadPhaseDispatcher::removeDuplicates(addedPairs, removedPairs);
        HK_TIMER_SPLIT_LIST("RemoveAgt");
        world->m_broadPhaseDispatcher->removePairs(
            static_cast<hkpTypedBroadPhaseHandlePair*>(removedPairs.m_data), removedPairs.m_size
        );
        // The SDK's check that the heap can take the new pairs' agents: 640 bytes for each.
        if (!hkMemoryRouter::getInstance().m_heap->canAllocTotal(addedPairs.m_size * 640)) {
            hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY);
        }
        else {
            HK_TIMER_SPLIT_LIST("AddAgt");
            hkpCollisionFilter* pFilter = world->m_collisionFilter;
            world->m_broadPhaseDispatcher->addPairs(
                static_cast<hkpTypedBroadPhaseHandlePair*>(addedPairs.m_data), addedPairs.m_size,
                static_cast<const hkpCollidableCollidableFilter*>(pFilter)
            );
        }
    }
    HK_TIMER_END_LIST();

    removedPairs.~hkLocalArray<hkpBroadPhaseHandlePair>();
    addedPairs.~hkLocalArray<hkpBroadPhaseHandlePair>();
}

void setLocalTranslationsToWorld(
    const hkpConstraintQueryIn* apIn,
    const hkpSetLocalTranslationsConstraintAtom* apAtom,
    hkTransform* apOutA,
    hkTransform* apOutB
) {
    hkVector4 v4;
    v4.m_quad = apAtom->m_translationA.m_quad;
    apOutA->m_translation.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v4.m_quad, v4.m_quad, 85), apIn->m_transformA->m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v4.m_quad, v4.m_quad, 0), apIn->m_transformA->m_rotation.m_col0.m_quad)
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v4.m_quad, v4.m_quad, 170), apIn->m_transformA->m_rotation.m_col2.m_quad),
            apIn->m_transformA->m_translation.m_quad
        )
    );
    const hkTransform* m_transformB = apIn->m_transformB;
    apOutB->m_translation.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(apAtom->m_translationB.m_quad, apAtom->m_translationB.m_quad, 85),
                m_transformB->m_rotation.m_col1.m_quad
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(apAtom->m_translationB.m_quad, apAtom->m_translationB.m_quad, 0),
                m_transformB->m_rotation.m_col0.m_quad
            )
        ),
        _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(apAtom->m_translationB.m_quad, apAtom->m_translationB.m_quad, 170),
                m_transformB->m_rotation.m_col2.m_quad
            ),
            m_transformB->m_translation.m_quad
        )
    );
}
