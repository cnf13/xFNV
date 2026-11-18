#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/SimpleConstraints/hkpSimpleConstraintUtil.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Response/hkpSimpleCollisionResponse.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Setup/hkpConstraintSolverSetup.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/Default/hkpDefaultToiResourceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/hkpToiResourceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/ToiResourceMgr/hkpToiResources.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

namespace {
    // An agent entry to recollide and the island whose constraints its contacts go to. Plain data: its hkArray destroys
    // nothing.
    struct EntryAndIsland {
        HK_DECLARE_POD_TYPE();

        hkpAgentNnEntry* m_entry;
        hkpSimulationIsland* m_island;
    };
} // namespace

// The sorted copy of the entity's agent list starts empty (no storage, not ours). The partner's motion type is tested
// twice, the second time inside the update of its transform, and entityStates' storage is read again for the update.
// The pushBack allocates from the heap first, then tests the capacity.
void hkLs_collectAgentEntriesToProcess(
    const hkpEntity* entity,
    hkpConstraintInstance::ConstraintPriority minPriorityToProcess,
    float time,
    hkFixedArray<uint8_t>& entityState,
    hkLocalArray<hkpAgentNnEntry*>& entriesOut
) {
    hkArray<hkpLinkedCollidable::CollisionEntry> collisionEntriesTmp;
    entity->m_collidable.getCollisionEntriesSorted(collisionEntriesTmp);
    for (int32_t i = 0; i < collisionEntriesTmp.m_size; ++i) {
        hkpLinkedCollidable::CollisionEntry& entry = collisionEntriesTmp.m_data[i];
        // The compare is signed.
        if (minPriorityToProcess > static_cast<int32_t>(entry.m_agentEntry->m_collisionQualityIndex)) {
            continue;
        }
        hkpLinkedCollidable* partner = entry.m_partner;
        // The owner sits m_ownerOffset bytes from the collidable, as raw memory.
        // EXE-INLINED 0x4B59D0
        hkpEntity* otherEntity = static_cast<hkpEntity*>(
            reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(partner) + partner->m_ownerOffset)
        );
        if (otherEntity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
            if (entityState.m_data.m_storage[otherEntity->m_storageIndex] >= 8) {
                continue;
            }
            // EXE-INLINED none: hkLs_updateTransformIfNeeded
            if (otherEntity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
                uint8_t* state = &entityState.m_data.m_storage[otherEntity->m_storageIndex];
                if (*state == 0) {
                    *state = 1;
                    hkMotionState& ms = otherEntity->m_motion.m_motionState;
                    hkSweptTransformUtil::lerp2(ms.m_sweptTransform, time, ms.m_transform);
                }
            }
        }
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        if (entriesOut.m_size == (entriesOut.m_capacityAndFlags & hkArrayBase<hkpAgentNnEntry*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(heap, &entriesOut, sizeof(hkpAgentNnEntry*));
        }
        entriesOut.m_data[entriesOut.m_size] = entry.m_agentEntry;
        ++entriesOut.m_size;
    }
}

// The material values are the event properties'; a zero max impulse means no limit (HK_REAL_MAX as a float constant).
// The compares of the keyframed velocities keep the game's NaN paths deliberately: a NaN velocity ends in the last
// choice.
void hkLs_doSimpleCollisionResponse(
    hkpWorld* world,
    const hkpToiEvent& event,
    float rotateNormal,
    hkArray<hkpEntity*>& toBeActivated
) {
    hkpEntity* pEntityA = event.m_entities[0];
    hkpEntity* pEntityB = event.m_entities[1];
    hkpSimpleConstraintUtilCollideParams params;
    params.m_externalSeperatingVelocity = event.m_seperatingVelocity;
    params.m_extraSeparatingVelocity = 0.1f;
    params.m_extraSlope = rotateNormal;
    // EXE-INLINED 0x577BF0
    params.m_friction = hkUFloat8_intToReal[event.m_properties.m_friction.m_value];
    params.m_restitution = static_cast<float>(static_cast<int32_t>(event.m_properties.m_restitution) * 0.0078125);
    uint8_t ucMaxImpulse = event.m_properties.m_maxImpulse.m_value;
    if (ucMaxImpulse == 0) {
        params.m_maxImpulse = 3.40282e+38f;
    }
    else {
        // EXE-INLINED 0x577BF0
        params.m_maxImpulse = hkUFloat8_intToReal[ucMaxImpulse];
    }
    hkpSimpleCollisionResponse::SolveSingleOutput output;
    hkpSimpleCollisionResponse::solveSingleContact(
        event.m_contactPoint, event.m_time, params, &pEntityA->m_motion, &pEntityB->m_motion, event.m_contactMgr, output
    );

    if (params.m_contactImpulseLimitBreached.m_bool) {
        // EXE-INLINED none: hkpContactImpulseLimitBreachedListenerInfo::set
        hkpContactImpulseLimitBreachedListenerInfo info;
        info.m_data.m_single.m_isContact = 1;
        info.m_data.m_single.m_type = 2;
        // The event is taken as const; the listener info holds non-const pointers into it.
        info.m_data.m_single.m_contactPoint = const_cast<hkContactPoint*>(&event.m_contactPoint);
        info.m_data.m_single.m_constraintInstance =
            &static_cast<hkpSimpleConstraintContactMgr*>(event.m_contactMgr)->m_constraint;
        info.m_data.m_single.m_properties = const_cast<hkpContactPointProperties*>(&event.m_properties);
        hkpWorldCallbackUtil::fireContactImpulseLimitBreached(world, &info, 1);
        return;
    }

    float fVelocityA = output.m_velocityKeyframedA;
    float fVelocityB = output.m_velocityKeyframedB;
    if (fVelocityA < 0.0f) {
        if (fVelocityB < 0.0f) {
            // EXE-INLINED 0xC761F0
            uint8_t ucTypeA = pEntityA->m_motion.m_type.m_storage;
            if (ucTypeA != hkpMotion::MOTION_FIXED && ucTypeA != hkpMotion::MOTION_KEYFRAMED) {
                // EXE-INLINED none: hkArray::pushBackUnchecked
                toBeActivated.m_data[toBeActivated.m_size] = pEntityA;
                toBeActivated.m_size++;
            }
            // EXE-INLINED 0xC761F0
            uint8_t ucTypeB = pEntityB->m_motion.m_type.m_storage;
            if (ucTypeB != hkpMotion::MOTION_FIXED && ucTypeB != hkpMotion::MOTION_KEYFRAMED) {
                // EXE-INLINED none: hkArray::pushBackUnchecked
                toBeActivated.m_data[toBeActivated.m_size] = pEntityB;
                toBeActivated.m_size++;
            }
            return;
        }
        if (fVelocityB >= 0.0f) {
            // EXE-INLINED 0xC761F0
            uint8_t ucTypeA = pEntityA->m_motion.m_type.m_storage;
            hkpEntity* pActivate =
                (ucTypeA == hkpMotion::MOTION_FIXED || ucTypeA == hkpMotion::MOTION_KEYFRAMED) ? pEntityB : pEntityA;
            // EXE-INLINED none: hkArray::pushBackUnchecked
            toBeActivated.m_data[toBeActivated.m_size] = pActivate;
            toBeActivated.m_size++;
            return;
        }
    }
    else if ((fVelocityA >= 0.0f) && (fVelocityB < 0.0f)) {
        hkpEntity* pActivate = pEntityB->isFixedOrKeyframed().m_bool ? pEntityA : pEntityB;
        // EXE-INLINED none: hkArray::pushBackUnchecked
        toBeActivated.m_data[toBeActivated.m_size] = pActivate;
        toBeActivated.m_size++;
        return;
    }

    // A velocity is NaN, or both are at least zero: the moving body, else the slower one (B when strictly slower).
    hkpEntity* pActivate;
    // EXE-INLINED 0xC761F0
    uint8_t ucTypeA = pEntityA->m_motion.m_type.m_storage;
    if (ucTypeA == hkpMotion::MOTION_FIXED || ucTypeA == hkpMotion::MOTION_KEYFRAMED) {
        pActivate = pEntityB;
    }
    else {
        // EXE-INLINED 0xC761F0
        uint8_t ucTypeB = pEntityB->m_motion.m_type.m_storage;
        if (ucTypeB == hkpMotion::MOTION_FIXED || ucTypeB == hkpMotion::MOTION_KEYFRAMED) {
            pActivate = pEntityA;
        }
        else if (fVelocityB < fVelocityA) {
            pActivate = pEntityB;
        }
        else {
            pActivate = pEntityA;
        }
    }
    // EXE-INLINED none: hkArray::pushBackUnchecked
    toBeActivated.m_data[toBeActivated.m_size] = pActivate;
    toBeActivated.m_size++;
}

hkBool less_EntityPtr(const hkpEntity* a, const hkpEntity* b) {
    return a->getUid() < b->getUid();
}

// Orders constraints by their master entity's uid, then by their internal's address.
hkBool less_ConstraintPtr(const hkpConstraintInstance* a, const hkpConstraintInstance* b) {
    if (a->getMasterEntity()->getUid() < b->getMasterEntity()->getUid()) {
        return true;
    }
    if (a->getMasterEntity()->getUid() == b->getMasterEntity()->getUid()) {
        return a->m_internal < b->m_internal;
    }
    return false;
}

// The collision quality index is a signed byte (hkpCollisionDispatcher's hkChar index): the constraint's priority is
// sign-extended before indexing the dispatcher's quality infos.
int32_t hkLs_areVelocitiesOk(
    float a1,
    const hkpConstraintSchemaInfo& a2,
    const float* a3,
    int32_t a4,
    const hkpProcessCollisionInput& a5
) {
    int32_t v5 = -1;
    int32_t v6 = 0;
    float a2a = a5.m_dispatcher->m_collisionQualityInfo[static_cast<int8_t>(a2.m_constraint->m_priority.m_storage)]
                    .m_maxContraintViolation *
        a2.m_allowedPenetrationDepth;
    double v7 = a1;
    if (a4 >= 4) {
        const float* v8 = a3 + 2;
        do {
            float a5f = std::fabs((double)*(v8 - 2));
            float a5a = a5f * v7;
            if (a2a < static_cast<double>(a5a)) {
                a2a = a5a;
                v5 = v6;
            }
            float a5g = std::fabs((double)*(v8 - 1));
            float a5b = a5g * v7;
            if (a2a < static_cast<double>(a5b)) {
                a2a = a5b;
                v5 = v6 + 1;
            }
            float a5h = std::fabs((double)*v8);
            float a5c = a5h * v7;
            if (a2a < static_cast<double>(a5c)) {
                a2a = a5c;
                v5 = v6 + 2;
            }
            float a5i = std::fabs((double)v8[1]);
            float a5d = a5i * v7;
            if (a2a < static_cast<double>(a5d)) {
                a2a = a5d;
                v5 = v6 + 3;
            }
            v6 += 4;
            v8 += 4;
        }
        while (v6 < a4 - 3);
    }
    for (; v6 < a4; ++v6) {
        float a5j = std::fabs((double)a3[v6]);
        float a5e = a5j * v7;
        if (a2a < static_cast<double>(a5e)) {
            a2a = a5e;
            v5 = v6;
        }
    }
    return v5;
}

// Moves each of the last numNew schema infos whose constraint links a fixed or keyframed entity to the front part
// [0, numMoved), swapping it with the entry at numMoved.
void hkLs_toiActivateConstraintsLinkingToFixedAndKeyframedEntities(
    hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>& constraintStatus,
    int32_t& firstNonActiveConstraintStatus,
    int32_t numOfConstraintsToCheckAtTheEndOfTheList
) {
    for (int32_t i = constraintStatus.m_size - numOfConstraintsToCheckAtTheEndOfTheList; i < constraintStatus.m_size;
         ++i) {
        hkConstraintInternal* internal = constraintStatus.m_data[i].m_constraint->getInternal();
        if (internal->m_entities[0]->isFixedOrKeyframed().m_bool ||
            internal->m_entities[1]->isFixedOrKeyframed().m_bool) {
            int32_t j = firstNonActiveConstraintStatus++;
            hkpConstraintSchemaInfo tmp = constraintStatus.m_data[j];
            constraintStatus.m_data[j] = constraintStatus.m_data[i];
            constraintStatus.m_data[i] = tmp;
        }
    }
}

// The SDK's static. Each entity's accumulator (at the byte offset m_solverData in the accumulator buffer, the SDK's
// inline hkAddByteOffset) takes the entity's linear velocity, and its angular velocity rotated into the accumulator's
// space: for the rotation, column 0 of m_scratch123 is the backed-up one, and the column it held is put back after.
void hkLs_toiResetVelocityAccumulatorsForEntities(
    hkpConstraintSolverResources& solverResources,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities
) {
    for (int32_t i = 0; i < entities.m_size; i++) {
        hkpEntity* entity = entities.m_data[i];
        // Adds the byte offset to the buffer's address: raw memory, typed as the accumulator it holds.
        hkpVelocityAccumulator* accumulator = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<char*>(solverResources.m_accumulators) + entity->m_solverData
        );
        accumulator->m_linearVel = entity->m_motion.m_linearVelocity;
        const hkVector4 savedColumn = accumulator->m_scratch123.m_col0;
        accumulator->m_scratch123.m_col0 =
            solverResources.m_accumulatorsBackup[entity->m_solverData / sizeof(hkpVelocityAccumulator)].m_angular;
        accumulator->m_angularVel.setRotatedDir(accumulator->m_scratch123, entity->m_motion.m_angularVelocity);
        accumulator->m_scratch123.m_col0 = savedColumn;
    }
}

// The accumulators' velocities are kept in a stack buffer, two vectors per entity, while each accumulator holds its
// entity's velocities over the swept transform (the inverse delta time is w of the second centre of mass). A
// constraint's checked entity is its first one unless that one is frozen (state 16).
void hkLs_toiCheckValidityOfConstraints(
    hkpConstraintSolverResources& solverResources,
    hkpProcessCollisionInput& processInput,
    hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>& constraintStatus,
    int32_t& firstNonActiveConstraintStatus,
    hkpConstraintInstance::ConstraintPriority minPriorityToProcess,
    hkFixedArray<uint8_t>& entityState,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& touchedEntities,
    hkArray<hkpEntity*, hkContainerHeapAllocator>& toBeActivated
) {
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    // The stack allocator hands out raw bytes, so the buffer is cast from char.
    // EXE-INLINED none: hkStackMemory::allocateStack
    char* velocitiesBuffer = router.m_stack.m_next.m_storage;
    router.m_stack.m_next.m_storage =
        velocitiesBuffer + ((touchedEntities.m_size * static_cast<int32_t>(2 * sizeof(hkVector4)) + 127) & ~127);
    hkVector4* savedVelocities = reinterpret_cast<hkVector4*>(velocitiesBuffer);
    for (int32_t i = 0; i < touchedEntities.m_size; i++) {
        hkpEntity* entity = touchedEntities.m_data[i];
        // Adds the byte offset to the buffer's address: raw memory, typed as the accumulator it holds.
        hkpVelocityAccumulator* accumulator = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<char*>(solverResources.m_accumulators) + entity->m_solverData
        );
        savedVelocities[2 * i] = accumulator->m_linearVel;
        savedVelocities[2 * i + 1] = accumulator->m_angularVel;
        // EXE-INLINED none: hkSweptTransformUtil::getVelocity
        const hkMotionState& ms = entity->m_motion.m_motionState;
        accumulator->m_linearVel.m_quad =
            _mm_sub_ps(ms.m_sweptTransform.m_centerOfMass1.m_quad, ms.m_sweptTransform.m_centerOfMass0.m_quad);
        accumulator->m_linearVel.m_quad =
            _mm_mul_ps(_mm_set1_ps(ms.m_sweptTransform.m_centerOfMass1(3)), accumulator->m_linearVel.m_quad);
        accumulator->m_angularVel.m_quad =
            _mm_mul_ps(_mm_set1_ps(ms.m_sweptTransform.m_centerOfMass1(3)), ms.m_deltaAngle.m_quad);
    }
    for (int32_t i = firstNonActiveConstraintStatus; i < constraintStatus.m_size; i++) {
        const hkpConstraintSchemaInfo& info = constraintStatus.m_data[i];
        if (info.m_constraint->getPriority() < minPriorityToProcess) {
            continue;
        }
        hkConstraintInternal* internal = info.m_constraint->getInternal();
        hkpEntity* entity = internal->m_entities[entityState[internal->m_entities[0]->m_storageIndex] == 16];
        if (!(entityState[entity->m_storageIndex] & 4)) {
            alignas(16) float velocities[256];
            const int32_t numVelocities = hkSolveGetToiViolatingConstraintVelocity(
                *solverResources.m_solverInfo, info.m_schema, solverResources.m_accumulators, 256, velocities
            );
            if (hkLs_areVelocitiesOk(
                    solverResources.m_stepInfo->m_deltaTime, info, velocities, numVelocities, processInput
                ) == -1) {
                continue;
            }
            entityState[entity->m_storageIndex] |= 4;
            // EXE-INLINED none: hkArray<hkpEntity*>::pushBack (the router is fetched even when it fits)
            hkMemoryRouter& pushRouter = hkMemoryRouter::getInstance();
            if (toBeActivated.m_size == (toBeActivated.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*pushRouter.m_heap, &toBeActivated, sizeof(hkpEntity*));
            }
            toBeActivated.m_data[toBeActivated.m_size++] = entity;
        }
        // EXE-INLINED none: hkAlgorithm::swap<hkpConstraintSchemaInfo>
        const int32_t j = firstNonActiveConstraintStatus++;
        const hkpConstraintSchemaInfo tmp = constraintStatus.m_data[j];
        constraintStatus.m_data[j] = constraintStatus.m_data[i];
        constraintStatus.m_data[i] = tmp;
    }
    for (int32_t i = 0; i < touchedEntities.m_size; i++) {
        // Raw memory again: the accumulator at the entity's byte offset.
        hkpVelocityAccumulator* accumulator = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<char*>(solverResources.m_accumulators) + touchedEntities.m_data[i]->m_solverData
        );
        accumulator->m_linearVel = savedVelocities[2 * i];
        accumulator->m_angularVel = savedVelocities[2 * i + 1];
    }
    // EXE-INLINED none: hkStackMemory::shrinkAllocatedStack
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = velocitiesBuffer;
}

// A contact constraint's violation names the violating contact point and its properties (the SDK's atom accessors;
// the properties index is taken as 16 bits, as the SDK's HK_HINT_SIZE16 makes it).
hkBool hkLs_toiCheckFinalValidityOfCriticalConstraints(
    hkpConstraintSolverResources& solverResources,
    const hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>& constraintStatus,
    hkpProcessCollisionInput& processInput,
    hkArray<hkpToiResourceMgr::ConstraintViolationInfo, hkContainerHeapAllocator>* violatedConstraints
) {
    bool allOk = true;
    for (int32_t i = 0; i < constraintStatus.m_size; i++) {
        const hkpConstraintSchemaInfo& info = constraintStatus.m_data[i];
        if (info.m_constraint->getPriority() != hkpConstraintInstance::PRIORITY_TOI_FORCED) {
            continue;
        }
        float velocities[256];
        const int32_t numVelocities = hkSolveGetToiViolatingConstraintVelocity(
            *solverResources.m_solverInfo, info.m_schema, solverResources.m_accumulators, 256, velocities
        );
        const int32_t violatedIndex = hkLs_areVelocitiesOk(
            solverResources.m_stepInfo->m_deltaTime, info, velocities, numVelocities, processInput
        );
        if (violatedIndex < 0) {
            continue;
        }
        if (!violatedConstraints) {
            return false;
        }
        allOk = false;
        // EXE-INLINED none: hkArray<hkpToiResourceMgr::ConstraintViolationInfo>::expandOne (the router is fetched
        // even when it fits)
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (violatedConstraints->m_size ==
            (violatedConstraints->m_capacityAndFlags &
             hkArrayBase<hkpToiResourceMgr::ConstraintViolationInfo>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                *router.m_heap, violatedConstraints, sizeof(hkpToiResourceMgr::ConstraintViolationInfo)
            );
        }
        hkpToiResourceMgr::ConstraintViolationInfo& violation =
            violatedConstraints->m_data[violatedConstraints->m_size++];
        violation.m_constraint = info.m_constraint;
        if (info.m_constraint->getData()->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT) {
            // The SDK's downcast of a contact constraint's data.
            const hkpSimpleContactConstraintData* data =
                static_cast<const hkpSimpleContactConstraintData*>(info.m_constraint->getData());
            violation.m_contactPoint = &data->m_atom->getContactPoints()[violatedIndex];
            // The properties stream is the atom's untyped memory after its contact points; the point's properties
            // are read from it (the SDK's getContactPointProperties casts the same way).
            violation.m_contactPointProperties = reinterpret_cast<const hkpContactPointProperties*>(
                data->m_atom->getContactPointPropertiesStream(static_cast<int16_t>(violatedIndex))
            );
        }
        else {
            violation.m_contactPoint = nullptr;
            violation.m_contactPointProperties = nullptr;
        }
    }
    return allOk;
}

// Activates the entities of a time of impact solve: each is moved to its transform at time (if not yet moved), marked
// manifold-updated, and its agents of at least minPriorityToProcess are collected and processed by the world's
// continuous simulation at the step info cut down to end at time. Then each entity is marked active, and every
// constraint of enough priority between it and a fixed or not yet active partner is appended to
// newTouchedConstraints; a partner not yet touched is appended to newTouchedEntities, moved to time and marked
// touched. The new entries of each entity's slave constraints are sorted.
void hkLs_toiActivateEntitiesAndCheckConstraints(
    hkpProcessCollisionInput& collisionInput,
    hkpConstraintInstance::ConstraintPriority minPriorityToProcess,
    float time,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& toBeActivated,
    hkFixedArray<uint8_t>& entityState,
    hkArray<hkpEntity*, hkContainerHeapAllocator>& newTouchedEntities,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& newTouchedConstraints
) {
    // The capacity is 0x800003E8 (1000 with the flag).
    const int32_t kNumLocalEntries = 1000;
    alignas(16) hkLocalArray<hkpAgentNnEntry*> entriesOut;
    {
        // EXE-INLINED none: hkLocalArray<hkpAgentNnEntry*>::hkLocalArray(1000) (hkArray's empty constructor, then
        // hkAllocateStack)
        entriesOut.m_data = nullptr;
        entriesOut.m_size = 0;
        entriesOut.m_capacityAndFlags = hkArrayBase<hkpAgentNnEntry*>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + ((kNumLocalEntries * sizeof(hkpAgentNnEntry*) + 127) & ~size_t(127));
        // The stack allocator hands out raw bytes.
        entriesOut.m_data = reinterpret_cast<hkpAgentNnEntry**>(pBlock);
        entriesOut.m_capacityAndFlags = kNumLocalEntries | hkArrayBase<hkpAgentNnEntry*>::DONT_DEALLOCATE_FLAG;
        entriesOut.m_localMemory = entriesOut.m_data;
    }
    for (int32_t i = 0; i < toBeActivated.m_size; ++i) {
        hkpEntity* entity = toBeActivated.m_data[i];
        if (entityState.m_data.m_storage[entity->m_storageIndex] == HK_TOI_ENTITY_STATE_ZERO) {
            entityState.m_data.m_storage[entity->m_storageIndex] = HK_TOI_ENTITY_STATE_TRANSFORM_UPDATED;
            hkSweptTransformUtil::lerp2(
                entity->m_motion.m_motionState.m_sweptTransform, time, entity->m_motion.m_motionState.m_transform
            );
        }
        entityState.m_data.m_storage[entity->m_storageIndex] = HK_TOI_ENTITY_STATE_MANIFOLD_UPDATED;
        hkLs_collectAgentEntriesToProcess(entity, minPriorityToProcess, time, entityState, entriesOut);
    }

    // The agents are processed with the step ending at time; the step info is put back afterwards.
    const hkStepInfo originalStepInfo = collisionInput.m_stepInfo;
    collisionInput.m_stepInfo.m_endTime = time;
    // The difference is rounded to a float before it is stored and tested.
    float fDeltaTime = time - collisionInput.m_stepInfo.m_startTime;
    const float deltaTime = fDeltaTime;
    collisionInput.m_stepInfo.m_deltaTime = deltaTime;
    collisionInput.m_stepInfo.m_invDeltaTime = (deltaTime > 0.0f) ? 1.0f / deltaTime : 0.0f;
    {
        // The SDK's cast: a world running times of impact has a continuous simulation.
        hkpContinuousSimulation* simulation =
            static_cast<hkpContinuousSimulation*>(toBeActivated.m_data[0]->m_world->m_simulation);
        hkBool processContinuous;
        processContinuous.m_bool = true;
        simulation->processAgentNnEntries(
            entriesOut.m_data, entriesOut.m_size, collisionInput, toBeActivated.m_data[0]->getSimulationIsland(),
            processContinuous
        );
    }
    collisionInput.m_stepInfo = originalStepInfo;

    for (int32_t i = 0; i < toBeActivated.m_size; ++i) {
        hkpEntity* entity = toBeActivated.m_data[i];
        entityState.m_data.m_storage[entity->m_storageIndex] = HK_TOI_ENTITY_STATE_ACTIVE;
        for (int32_t j = 0; j < entity->m_constraintsMaster.m_size; ++j) {
            hkConstraintInternal* ci = &entity->m_constraintsMaster.m_data[j];
            if (minPriorityToProcess > ci->m_constraint->m_priority.m_storage) {
                continue;
            }
            hkpEntity* otherEntity = ci->getOtherEntity(entity);
            if ((otherEntity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) &&
                (entityState.m_data.m_storage[otherEntity->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE)) {
                continue;
            }

            {
                // EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (newTouchedConstraints.m_size ==
                    (newTouchedConstraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(
                        router.heap(), &newTouchedConstraints, static_cast<int32_t>(sizeof(hkpConstraintInstance*))
                    );
                }
                newTouchedConstraints.m_data[newTouchedConstraints.m_size++] = ci->m_constraint;
            }
            if (otherEntity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                continue;
            }
            if (entityState.m_data.m_storage[otherEntity->m_storageIndex] >= HK_TOI_ENTITY_STATE_TOUCHED) {
                continue;
            }
            {
                // EXE-INLINED none: hkArray<hkpEntity*>::pushBack
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (newTouchedEntities.m_size ==
                    (newTouchedEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(
                        router.heap(), &newTouchedEntities, static_cast<int32_t>(sizeof(hkpEntity*))
                    );
                }
                newTouchedEntities.m_data[newTouchedEntities.m_size++] = otherEntity;
            }
            if (entityState.m_data.m_storage[otherEntity->m_storageIndex] == HK_TOI_ENTITY_STATE_ZERO) {
                entityState.m_data.m_storage[otherEntity->m_storageIndex] = HK_TOI_ENTITY_STATE_TRANSFORM_UPDATED;
                hkSweptTransformUtil::lerp2(
                    otherEntity->m_motion.m_motionState.m_sweptTransform, time,
                    otherEntity->m_motion.m_motionState.m_transform
                );
            }
            entityState.m_data.m_storage[otherEntity->m_storageIndex] = HK_TOI_ENTITY_STATE_TOUCHED;
        }

        // The asm takes both sizes here, after the master constraints: only the entries the slave constraints add
        // are sorted.
        const int32_t firstNewConstraint = newTouchedConstraints.m_size;
        const int32_t firstNewEntity = newTouchedEntities.m_size;
        for (int32_t j = 0; j < entity->m_constraintsSlave.m_size; ++j) {
            hkConstraintInternal* ci = entity->m_constraintsSlave.m_data[j]->m_internal;
            if (minPriorityToProcess > ci->m_constraint->m_priority.m_storage) {
                continue;
            }
            hkpEntity* otherEntity = ci->getOtherEntity(entity);
            if ((otherEntity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) &&
                (entityState.m_data.m_storage[otherEntity->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE)) {
                continue;
            }

            {
                // EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (newTouchedConstraints.m_size ==
                    (newTouchedConstraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(
                        router.heap(), &newTouchedConstraints, static_cast<int32_t>(sizeof(hkpConstraintInstance*))
                    );
                }
                newTouchedConstraints.m_data[newTouchedConstraints.m_size++] = ci->m_constraint;
            }
            if (otherEntity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                continue;
            }
            if (entityState.m_data.m_storage[otherEntity->m_storageIndex] >= HK_TOI_ENTITY_STATE_TOUCHED) {
                continue;
            }
            {
                // EXE-INLINED none: hkArray<hkpEntity*>::pushBack
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (newTouchedEntities.m_size ==
                    (newTouchedEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(
                        router.heap(), &newTouchedEntities, static_cast<int32_t>(sizeof(hkpEntity*))
                    );
                }
                newTouchedEntities.m_data[newTouchedEntities.m_size++] = otherEntity;
            }
            if (entityState.m_data.m_storage[otherEntity->m_storageIndex] == HK_TOI_ENTITY_STATE_ZERO) {
                entityState.m_data.m_storage[otherEntity->m_storageIndex] = HK_TOI_ENTITY_STATE_TRANSFORM_UPDATED;
                hkSweptTransformUtil::lerp2(
                    otherEntity->m_motion.m_motionState.m_sweptTransform, time,
                    otherEntity->m_motion.m_motionState.m_transform
                );
            }
            entityState.m_data.m_storage[otherEntity->m_storageIndex] = HK_TOI_ENTITY_STATE_TOUCHED;
        }

        // EXE-INLINED none: hkAlgorithm::quickSort (only more than one element is sorted)
        const int32_t numNewConstraints = newTouchedConstraints.m_size - firstNewConstraint;
        if (numNewConstraints > 1) {
            hkAlgorithm::quickSortRecursive<
                hkpConstraintInstance*, hkBool (*)(const hkpConstraintInstance*, const hkpConstraintInstance*)>(
                newTouchedConstraints.m_data + firstNewConstraint, 0, numNewConstraints - 1, &less_ConstraintPtr
            );
        }
        // EXE-INLINED none: hkAlgorithm::quickSort (only more than one element is sorted)
        const int32_t numNewEntities = newTouchedEntities.m_size - firstNewEntity;
        if (numNewEntities > 1) {
            hkAlgorithm::quickSortRecursive<hkpEntity*, hkBool (*)(const hkpEntity*, const hkpEntity*)>(
                newTouchedEntities.m_data + firstNewEntity, 0, numNewEntities - 1, &less_EntityPtr
            );
        }
    }
}

// The SDK's static. A body whose transform was updated (state 1 or 2) gets the transform at the end of its swept
// transform again. A touched or manifold-updated body (state 2 or 8) takes its velocities back from its accumulator
// (at the byte offset m_solverData in the accumulator buffer, the SDK's inline hkAddByteOffset), the angular one
// rotated back out of the accumulator's space once column 0 of m_scratch123 is the backed-up one again.
void hkLs_restoreTransformOnBodiesWithUpdatedTransform(
    hkpSimulationIsland* island,
    hkFixedArray<uint8_t>& entityState,
    hkpConstraintSolverResources& solverResources
) {
    for (int32_t i = 0; i < island->m_entities.m_size; i++) {
        const int32_t state = entityState[i];
        if (state == HK_TOI_ENTITY_STATE_TRANSFORM_UPDATED || state == HK_TOI_ENTITY_STATE_TOUCHED) {
            // EXE-INLINED none: hkSweptTransformUtil::calcTransAtT1
            hkMotionState* motionState = island->m_entities.m_data[i]->m_motion.getMotionState();
            const hkSweptTransform& sweptTransform = motionState->m_sweptTransform;
            hkTransform& transform = motionState->m_transform;
            transform.m_rotation.set(sweptTransform.m_rotation1);
            // EXE-INLINED 0xC7F700
            const hkQuadReal local = sweptTransform.m_centerOfMassLocal.m_quad;
            const hkQuadReal centerShift = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(local, local, _MM_SHUFFLE(1, 1, 1, 1)), transform.m_rotation.m_col1.m_quad
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(local, local, _MM_SHUFFLE(0, 0, 0, 0)), transform.m_rotation.m_col0.m_quad
                    )
                ),
                _mm_mul_ps(_mm_shuffle_ps(local, local, _MM_SHUFFLE(2, 2, 2, 2)), transform.m_rotation.m_col2.m_quad)
            );
            transform.m_translation.m_quad = _mm_sub_ps(sweptTransform.m_centerOfMass1.m_quad, centerShift);
        }
        if (state == HK_TOI_ENTITY_STATE_MANIFOLD_UPDATED || state == HK_TOI_ENTITY_STATE_TOUCHED) {
            hkpEntity* entity = island->m_entities.m_data[i];
            // Adds the byte offset to the buffer's address: raw memory, typed as the accumulator it holds.
            hkpVelocityAccumulator* accumulator = reinterpret_cast<hkpVelocityAccumulator*>(
                reinterpret_cast<char*>(solverResources.m_accumulators) + entity->m_solverData
            );
            accumulator->m_scratch123.m_col0 =
                solverResources.m_accumulatorsBackup[entity->m_solverData / sizeof(hkpVelocityAccumulator)].m_angular;
            entity->m_motion.m_linearVelocity = accumulator->m_linearVel;
            entity->m_motion.m_angularVelocity._setRotatedInverseDir(
                accumulator->m_scratch123, accumulator->m_angularVel
            );
        }
    }
}

void hkLs_backstepAndFreezeEntireIsland(
    float time,
    hkpSimulationIsland* island,
    hkFixedArray<uint8_t>& entityState,
    hkArray<hkpEntity*, hkContainerHeapAllocator>& activeEntities
) {
    int32_t v4 = 0;
    if (island->m_entities.m_size > 0) {
        double v5 = time;
        do {
            hkpEntity* v9 = island->m_entities.m_data[v4];
            hkSweptTransformUtil::backStepMotionState(v5, v9->m_motion.m_motionState);
            if (entityState.m_data.m_storage[v4] != 16) {
                hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                if (activeEntities.m_size ==
                    (activeEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*Instance->m_heap, &activeEntities, sizeof(hkpEntity*));
                }
                activeEntities.m_data[activeEntities.m_size++] = v9;
                entityState.m_data.m_storage[v4] = 16;
            }
            v9->m_motion.m_motionState.m_sweptTransform.m_centerOfMass0 =
                v9->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1;
            v9->m_motion.m_motionState.m_sweptTransform.m_rotation0 =
                v9->m_motion.m_motionState.m_sweptTransform.m_rotation1;
            v9->m_motion.m_motionState.m_sweptTransform.m_centerOfMass0.m_quad[3] = time;
            ++v4;
            double v7 = 1.0 / (island->getWorld()->getCurrentPsiTime() - time);
            v5 = time;
            v9->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad[3] = v7;
        }
        while (v4 < island->m_entities.m_size);
    }
}

// The SDK's static: each constraint from firstNonActiveConstraintStatus on whose two entities are both active is
// swapped with the first non-active entry, which then counts as active.
void hkLs_toiActivateConstraintsLinkingActivatedEntities(
    hkArray<hkpConstraintSchemaInfo, hkContainerHeapAllocator>& constraintStatus,
    int32_t& firstNonActiveConstraintStatus,
    hkFixedArray<uint8_t>& entityState
) {
    for (int32_t i = firstNonActiveConstraintStatus; i < constraintStatus.m_size; i++) {
        hkConstraintInternal* internal = constraintStatus.m_data[i].m_constraint->getInternal();
        if (entityState[internal->m_entities[0]->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE &&
            entityState[internal->m_entities[1]->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE) {
            // EXE-INLINED none: hkAlgorithm::swap<hkpConstraintSchemaInfo>
            hkpConstraintSchemaInfo& firstNonActive = constraintStatus.m_data[firstNonActiveConstraintStatus++];
            const hkpConstraintSchemaInfo tmp = firstNonActive;
            firstNonActive = constraintStatus.m_data[i];
            constraintStatus.m_data[i] = tmp;
        }
    }
}

// Static: solves a time of impact locally. The event's non-fixed entities are moved to the event's time and touched;
// the contact manager confirms the TOI and names the entities to activate (none: their transforms go back to the swept
// transforms' ends and the solve fails). Then, for up to m_numToiSolverIterations rounds, the entities and constraints
// the activation touches get accumulators and schemas and are solved incrementally while resources last; when they run
// out, the resource manager may ask for a backstep of the whole island. Last, the critical constraints are checked (and
// solved again up to m_numForcedToiFinalSolverIterations times; the resource manager decides when they stay violated),
// the active entities are backstepped, integrated one step and their TIMs invalidated. The hkLocalArray's storage comes
// from the thread's stack allocator.
hkResult hkLs_localizedSolveToi(
    const hkpToiResources& toiResources,
    hkpConstraintSolverResources& solverResources,
    hkpToiEvent& event,
    hkpToiResourceMgr& toiResourceMgr,
    hkpWorld* world,
    hkArray<hkpEntity*, hkContainerHeapAllocator>& activeEntities,
    hkFixedArray<uint8_t>& entityState,
    float rotateNormal
) {
    alignas(16) hkInplaceArray<hkpConstraintSchemaInfo, 64> constraintStatus;
    int32_t firstNonActiveConstraintStatus = 0;

    hkpSimulationIsland* island = event.m_entities[0]->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED
        ? event.m_entities[1]->getSimulationIsland()
        : event.m_entities[0]->getSimulationIsland();
    const int32_t numIslandEntities = island->m_entities.m_size;
    alignas(16) hkLocalArray<hkpEntity*> touchedEntities;
    {
        // EXE-INLINED none: hkLocalArray<hkpEntity*>::hkLocalArray(numIslandEntities) (hkArray's empty constructor,
        // then hkAllocateStack)
        touchedEntities.m_data = nullptr;
        touchedEntities.m_size = 0;
        touchedEntities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + ((numIslandEntities * static_cast<int32_t>(sizeof(hkpEntity*)) + 127) & ~127);
        touchedEntities.m_capacityAndFlags = numIslandEntities | hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
        // The stack allocator hands out raw bytes.
        touchedEntities.m_data = reinterpret_cast<hkpEntity**>(pBlock);
        touchedEntities.m_localMemory = touchedEntities.m_data;
    }

    if (event.m_entities[0]->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
        // EXE-INLINED none: hkArray<hkpEntity*>::pushBackUnchecked
        touchedEntities.m_data[touchedEntities.m_size++] = event.m_entities[0];
        hkSweptTransformUtil::lerp2(
            event.m_entities[0]->m_motion.m_motionState.m_sweptTransform, event.m_time,
            event.m_entities[0]->m_motion.m_motionState.m_transform
        );
        entityState[event.m_entities[0]->m_storageIndex] = HK_TOI_ENTITY_STATE_TOUCHED;
    }
    if (event.m_entities[1]->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
        // EXE-INLINED none: hkArray<hkpEntity*>::pushBackUnchecked
        touchedEntities.m_data[touchedEntities.m_size++] = event.m_entities[1];
        hkSweptTransformUtil::lerp2(
            event.m_entities[1]->m_motion.m_motionState.m_sweptTransform, event.m_time,
            event.m_entities[1]->m_motion.m_motionState.m_transform
        );
        entityState[event.m_entities[1]->m_storageIndex] = HK_TOI_ENTITY_STATE_TOUCHED;
    }

    alignas(16) hkInplaceArray<hkpEntity*, 64> toBeActivated;
    toBeActivated.m_data = toBeActivated.m_storage;
    toBeActivated.m_size = 0;
    toBeActivated.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG | 64;
    event.m_contactMgr->confirmToi(event, rotateNormal, toBeActivated);
    if (!toBeActivated.m_size) {
        for (int32_t e = 0; e < 2; e++) {
            hkpEntity* entity = event.m_entities[e];
            if (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                continue;
            }
            // EXE-INLINED none: hkSweptTransformUtil::calcTransAtT1
            const hkSweptTransform& sweptTransform = entity->m_motion.m_motionState.m_sweptTransform;
            hkTransform& transform = entity->m_motion.m_motionState.m_transform;
            transform.m_rotation.set(sweptTransform.m_rotation1);
            // EXE-INLINED 0xC7F700
            const hkQuadReal local = sweptTransform.m_centerOfMassLocal.m_quad;
            const hkQuadReal centerShift = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(local, local, _MM_SHUFFLE(1, 1, 1, 1)), transform.m_rotation.m_col1.m_quad
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(local, local, _MM_SHUFFLE(0, 0, 0, 0)), transform.m_rotation.m_col0.m_quad
                    )
                ),
                _mm_mul_ps(_mm_shuffle_ps(local, local, _MM_SHUFFLE(2, 2, 2, 2)), transform.m_rotation.m_col2.m_quad)
            );
            transform.m_translation.m_quad = _mm_sub_ps(sweptTransform.m_centerOfMass1.m_quad, centerShift);
        }
        {
            // EXE-INLINED none: hkInplaceArray<hkpEntity*,64>::~hkInplaceArray
            hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
            toBeActivated.m_size = 0;
            if (toBeActivated.m_capacityAndFlags >= 0) {
                heap.bufFree(
                    toBeActivated.m_data,
                    (toBeActivated.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK) *
                        static_cast<int32_t>(sizeof(hkpEntity*))
                );
            }
            toBeActivated.m_data = nullptr;
            toBeActivated.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
        }
        return HK_FAILURE;
    }
    // The entities the solve activates when nothing else is active.
    hkpEntity* firstToActivate = toBeActivated.m_data[0];
    hkpEntity* secondToActivate = toBeActivated.m_size > 1 ? toBeActivated.m_data[1] : nullptr;

    hkSolveUpload();
    hkpConstraintSolverSetup::internalAddAccumulators(solverResources, touchedEntities.m_data, touchedEntities.m_size);

    alignas(16) hkInplaceArray<hkpEntity*, 64> newTouchedEntities;
    newTouchedEntities.m_data = newTouchedEntities.m_storage;
    newTouchedEntities.m_size = 0;
    newTouchedEntities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG | 64;
    alignas(16) hkInplaceArray<hkpConstraintInstance*, 64> newTouchedConstraints;
    newTouchedConstraints.m_data = newTouchedConstraints.m_storage;
    newTouchedConstraints.m_size = 0;
    newTouchedConstraints.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG | 64;

    bool firstIteration = true;
    int32_t iterationsLeft = toiResources.m_numToiSolverIterations;
    // A round that touched new entities or constraints is followed by another without using up an iteration.
    while (iterationsLeft > 0) {
        newTouchedConstraints.m_size = 0;
        newTouchedEntities.m_size = 0;
        int32_t nextFirstNonActiveConstraintStatus = firstNonActiveConstraintStatus;
        bool expand = true;
        if (!firstIteration) {
            expand = false;
            if (nextFirstNonActiveConstraintStatus < constraintStatus.m_size) {
                toBeActivated.m_size = 0;
                hkLs_toiCheckValidityOfConstraints(
                    solverResources, *world->m_collisionInput, constraintStatus, nextFirstNonActiveConstraintStatus,
                    toiResources.m_minPriorityToProcess, entityState, touchedEntities, toBeActivated
                );
                expand = toBeActivated.m_size != 0;
            }
        }
        if (expand) {
            hkLs_toiActivateEntitiesAndCheckConstraints(
                *world->m_collisionInput, toiResources.m_minPriorityToProcess, event.m_time, toBeActivated, entityState,
                newTouchedEntities, newTouchedConstraints
            );
            firstIteration = false;
            if (hkpConstraintSolverSetup::internalIsMemoryOkForNewAccumulators(
                    solverResources, newTouchedEntities.m_data, newTouchedEntities.m_size
                )
                    .m_bool &&
                hkpConstraintSolverSetup::internalIsMemoryOkForNewJacobianSchemas(
                    solverResources, newTouchedConstraints.m_data, newTouchedConstraints.m_size
                )
                    .m_bool &&
                toiResources.m_maxNumConstraints >= constraintStatus.m_size + newTouchedConstraints.m_size &&
                toiResources.m_maxNumEntities >=
                    newTouchedEntities.m_size + activeEntities.m_size + touchedEntities.m_size &&
                toiResources.m_maxNumActiveEntities >= toBeActivated.m_size + activeEntities.m_size) {
                hkpConstraintSolverSetup::internalAddAccumulators(
                    solverResources, newTouchedEntities.m_data, newTouchedEntities.m_size
                );
                hkpConstraintSolverSetup::internalAddJacobianSchemas(
                    solverResources, newTouchedConstraints.m_data, newTouchedConstraints.m_size, constraintStatus
                );
                firstNonActiveConstraintStatus = nextFirstNonActiveConstraintStatus;
                // The touched entities that are active now leave the touched list.
                for (int32_t i = 0; i < touchedEntities.m_size; i++) {
                    if (entityState[touchedEntities.m_data[i]->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE) {
                        touchedEntities.removeAt(i);
                        i--;
                    }
                }
                activeEntities._insertAt(
                    hkMemoryRouter::getInstance().heap(), activeEntities.m_size, toBeActivated.m_data,
                    toBeActivated.m_size
                );
                touchedEntities._reserve(
                    hkMemoryRouter::getInstance().heap(), touchedEntities.m_size + newTouchedEntities.m_size
                );
                for (int32_t i = 0; i < newTouchedEntities.m_size; i++) {
                    hkpEntity* entity = newTouchedEntities.m_data[i];
                    if (entity->m_motion.m_type.m_storage != hkpMotion::MOTION_KEYFRAMED) {
                        // EXE-INLINED none: hkArray<hkpEntity*>::pushBackUnchecked
                        touchedEntities.m_data[touchedEntities.m_size++] = entity;
                    }
                }
                hkLs_toiActivateConstraintsLinkingToFixedAndKeyframedEntities(
                    constraintStatus, firstNonActiveConstraintStatus, newTouchedConstraints.m_size
                );
                hkLs_toiActivateConstraintsLinkingActivatedEntities(
                    constraintStatus, firstNonActiveConstraintStatus, entityState
                );
                hkpConstraintSolverSetup::subSolve(
                    solverResources, hkpConstraintSolverSetup::SOLVER_MODE_INCREMENTAL_CONTINUE
                );
                if (newTouchedConstraints.m_size || newTouchedEntities.m_size) {
                    continue;
                }
            }
            else {
                // Out of resources: the entities to activate stay manifold-updated, the new ones transform-updated.
                for (int32_t i = 0; i < toBeActivated.m_size; i++) {
                    entityState[toBeActivated.m_data[i]->m_storageIndex] = HK_TOI_ENTITY_STATE_MANIFOLD_UPDATED;
                }
                for (int32_t i = 0; i < newTouchedEntities.m_size; i++) {
                    entityState[newTouchedEntities.m_data[i]->m_storageIndex] = HK_TOI_ENTITY_STATE_TRANSFORM_UPDATED;
                }
                if (toiResourceMgr.resourcesDepleted() == HK_TOI_RESOURCE_MGR_RESPONSE_BACKSTEP) {
                    hkLs_backstepAndFreezeEntireIsland(event.m_time, island, entityState, activeEntities);
                    for (int32_t i = 0; i < activeEntities.m_size; i++) {
                        hkpWorldAgentUtil::invalidateTim(activeEntities.m_data[i], *world->m_collisionInput);
                    }
                    activeEntities.m_size = 0;
                    goto cleanup;
                }
            }
        }
        hkpConstraintSolverSetup::subSolve(solverResources, hkpConstraintSolverSetup::SOLVER_MODE_PROCESS_ALL);
        iterationsLeft--;
    }

    {
        if (solverResources.m_schemas[1].m_begin != solverResources.m_schemas[1].m_current) {
            hkLs_toiResetVelocityAccumulatorsForEntities(solverResources, touchedEntities);
            hkBool criticalOk = hkLs_toiCheckFinalValidityOfCriticalConstraints(
                solverResources, constraintStatus, *world->m_collisionInput, nullptr
            );
            if (!criticalOk.m_bool) {
                bool solved = false;
                for (int32_t i = toiResources.m_numForcedToiFinalSolverIterations; --i >= 0;) {
                    hkSolveStepJacobians(
                        *solverResources.m_solverInfo, solverResources.m_schemas[1].m_begin,
                        solverResources.m_accumulators, solverResources.m_elemTemp
                    );
                    hkLs_toiResetVelocityAccumulatorsForEntities(solverResources, touchedEntities);
                    if (hkLs_toiCheckFinalValidityOfCriticalConstraints(
                            solverResources, constraintStatus, *world->m_collisionInput, nullptr
                        )
                            .m_bool) {
                        solved = true;
                        break;
                    }
                }
                if (!solved) {
                    alignas(16) hkInplaceArray<hkpToiResourceMgr::ConstraintViolationInfo, 32> violatedConstraints;
                    violatedConstraints.m_data = violatedConstraints.m_storage;
                    violatedConstraints.m_size = 0;
                    violatedConstraints.m_capacityAndFlags =
                        hkArrayBase<hkpToiResourceMgr::ConstraintViolationInfo>::DONT_DEALLOCATE_FLAG | 32;
                    hkLs_toiCheckFinalValidityOfCriticalConstraints(
                        solverResources, constraintStatus, *world->m_collisionInput, &violatedConstraints
                    );
                    const bool backstep =
                        toiResourceMgr.cannotSolve(violatedConstraints) == HK_TOI_RESOURCE_MGR_RESPONSE_BACKSTEP;
                    if (backstep) {
                        hkLs_backstepAndFreezeEntireIsland(event.m_time, island, entityState, activeEntities);
                        for (int32_t i = 0; i < activeEntities.m_size; i++) {
                            hkpWorldAgentUtil::invalidateTim(activeEntities.m_data[i], *world->m_collisionInput);
                        }
                        activeEntities.m_size = 0;
                    }
                    {
                        // EXE-INLINED none:
                        // hkInplaceArray<hkpToiResourceMgr::ConstraintViolationInfo,32>::~hkInplaceArray
                        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                        violatedConstraints.m_size = 0;
                        if (violatedConstraints.m_capacityAndFlags >= 0) {
                            heap.bufFree(
                                violatedConstraints.m_data,
                                (violatedConstraints.m_capacityAndFlags &
                                 hkArrayBase<hkpToiResourceMgr::ConstraintViolationInfo>::CAPACITY_MASK) *
                                    static_cast<int32_t>(sizeof(hkpToiResourceMgr::ConstraintViolationInfo))
                            );
                        }
                        violatedConstraints.m_data = nullptr;
                        violatedConstraints.m_capacityAndFlags =
                            hkArrayBase<hkpToiResourceMgr::ConstraintViolationInfo>::DONT_DEALLOCATE_FLAG;
                    }
                    if (backstep) {
                        goto cleanup;
                    }
                }
            }
        }
        if (!activeEntities.m_size) {
            activeEntities._insertAt(hkMemoryRouter::getInstance().heap(), 0, firstToActivate);
            entityState[activeEntities.m_data[0]->m_storageIndex] = HK_TOI_ENTITY_STATE_ACTIVE;
            if (secondToActivate) {
                activeEntities._insertAt(hkMemoryRouter::getInstance().heap(), 1, secondToActivate);
                entityState[activeEntities.m_data[1]->m_storageIndex] = HK_TOI_ENTITY_STATE_ACTIVE;
            }
        }
        for (int32_t i = 0; i < activeEntities.m_size; i++) {
            hkSweptTransformUtil::backStepMotionState(event.m_time, activeEntities.m_data[i]->m_motion.m_motionState);
        }
        hkpConstraintSolverSetup::oneStepIntegrate(
            *solverResources.m_solverInfo, *solverResources.m_stepInfo, solverResources.m_accumulators,
            activeEntities.m_data, activeEntities.m_size
        );
        hkLs_restoreTransformOnBodiesWithUpdatedTransform(island, entityState, solverResources);
        for (int32_t i = 0; i < activeEntities.m_size; i++) {
            hkpWorldAgentUtil::invalidateTim(activeEntities.m_data[i], *world->m_collisionInput);
        }
    }

cleanup:
    {
        // EXE-INLINED none: hkInplaceArray<hkpConstraintInstance*,64>::~hkInplaceArray
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        newTouchedConstraints.m_size = 0;
        if (newTouchedConstraints.m_capacityAndFlags >= 0) {
            heap.bufFree(
                newTouchedConstraints.m_data,
                (newTouchedConstraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkpConstraintInstance*))
            );
        }
        newTouchedConstraints.m_data = nullptr;
        newTouchedConstraints.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
    }
    {
        // EXE-INLINED none: hkInplaceArray<hkpEntity*,64>::~hkInplaceArray
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        newTouchedEntities.m_size = 0;
        if (newTouchedEntities.m_capacityAndFlags >= 0) {
            heap.bufFree(
                newTouchedEntities.m_data,
                (newTouchedEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkpEntity*))
            );
        }
        newTouchedEntities.m_data = nullptr;
        newTouchedEntities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
    }
    {
        // EXE-INLINED none: hkInplaceArray<hkpEntity*,64>::~hkInplaceArray
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        toBeActivated.m_size = 0;
        if (toBeActivated.m_capacityAndFlags >= 0) {
            heap.bufFree(
                toBeActivated.m_data,
                (toBeActivated.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkpEntity*))
            );
        }
        toBeActivated.m_data = nullptr;
        toBeActivated.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
    }
    return HK_SUCCESS;
}

hkpContinuousSimulation::hkpContinuousSimulation(hkpWorld* world)
  : hkpSimulation(world), m_toiResourceMgr(new hkpDefaultToiResourceMgr()), m_toiCounter(0) {
    m_toiEvents.reserve(world->m_sizeOfToiEventQueue);
}

// The arrays and the map free themselves; the resource manager is owned.
hkpContinuousSimulation::~hkpContinuousSimulation() {
    delete m_toiResourceMgr;
}

void hkpContinuousSimulation::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_toiEvents.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_toiEvents.m_data, static_cast<int32_t>(sizeof(*m_toiEvents.m_data)) * (m_capacityAndFlags & 0x3FFFFFFF)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "toiEvents", m_toiEvents.m_data,
            static_cast<int32_t>(sizeof(*m_toiEvents.m_data)) * m_toiEvents.m_size, AllocatedSize
        );
    }
    collector->addReferencedObject("toiResourceManager", m_toiResourceMgr, 0);
    int32_t v6 = m_entitiesNeedingPsiCollisionDetection.m_map.getMemSize();
    int32_t v7 = hkStatisticsCollector::getAllocatedSize(m_entitiesNeedingPsiCollisionDetection.m_map.m_elem, v6);
    collector->addChunk(
        hkStatisticsCollector::MEM_NORMAL, "entitiesNeedingPsiCollisionDetection",
        m_entitiesNeedingPsiCollisionDetection.m_map.m_elem, v6, v7
    );
    hkReferencedObject::calcContentStatistics(collector, &hkpSimulationClass);
}

hkpStepResult hkpContinuousSimulation::advanceTime() {
    hkpStepResult result = advanceTimeInternal();
    if (m_simulateUntilTime <= m_currentTime) {
        if ((m_entitiesNeedingPsiCollisionDetection.m_map.m_numElems & hkUlongMapBase::NUM_ELEMS_MASK) != 0) {
            collideEntitiesNeedingPsiCollisionDetectionNarrowPhase_toiOnly(
                *m_world->m_collisionInput, m_entitiesNeedingPsiCollisionDetection
            );
        }
        if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
            return HK_STEP_RESULT_MEMORY_FAILURE_DURING_TOI_SOLVE;
        }
        if (m_world->m_worldPostSimulationListeners.m_size) {
            HK_TIMER_BEGIN("PostSimCb", nullptr);
            hkpWorldCallbackUtil::firePostSimulationCallback(m_world);
            HK_TIMER_END();
        }
    }
    return result;
}

void hkpContinuousSimulation::resetCollisionInformationForEntities(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    hkpSimulation::ResetCollisionInformation resetInfo
) {
    hkpSimulation::resetCollisionInformationForEntities(entities, numEntities, world, resetInfo);
    if ((resetInfo & 1) != 0) {
        if (numEntities == 1) {
            removeToiEventsOfEntity(*entities);
        }
        else {
            removeToiEventsOfEntities(entities, numEntities);
        }
    }
}

void hkpContinuousSimulation::assertThereIsNoCollisionInformationForEntities(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world
) {
    ;
}

// The SDK's: removes the events of the agent's contact manager, from the last one down.
void hkpContinuousSimulation::removeCollisionInformationForAgent(hkpAgentNnEntry* agent) {
    for (int32_t i = m_toiEvents.m_size - 1; i >= 0; i--) {
        if (m_toiEvents.m_data[i].m_contactMgr == agent->m_contactMgr) {
            m_toiEvents.removeAt(i);
        }
    }
}

void hkpContinuousSimulation::assertThereIsNoCollisionInformationForAgent(hkpAgentNnEntry* agent) {
    ;
}

// Steps the entities again from the current time to the psi time and recollides them, as the mode asks: the world's
// critical operations stay locked throughout, and the collision input runs on that step and gets its own step info back
// at the end. The step info local is 16-aligned.
void hkpContinuousSimulation::reintegrateAndRecollideEntities(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    int32_t reintegrateRecollideMode
) {
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++world->m_criticalOperationsLockCount;
    hkpProcessCollisionInput* input = world->m_collisionInput;
    const hkStepInfo originalStepInfo = input->m_stepInfo;
    alignas(16) hkStepInfo physicsStepInfo;
    const float startTime = m_currentTime;
    const float endTime = m_currentPsiTime;
    physicsStepInfo.m_startTime = startTime;
    physicsStepInfo.m_endTime = endTime;
    physicsStepInfo.m_deltaTime = endTime - startTime;
    physicsStepInfo.m_invDeltaTime = physicsStepInfo.m_deltaTime == 0.0f ? 0.0f : 1.0f / physicsStepInfo.m_deltaTime;
    input->m_stepInfo = physicsStepInfo;
    const int32_t reintegrate = reintegrateRecollideMode & hkpWorld::RR_MODE_REINTEGRATE;
    if (reintegrate) {
        for (int32_t i = 0; i < numEntities; ++i) {
            hkpEntity* entity = entities[i];
            if (entity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
                hkSweptTransformUtil::backStepMotionState(m_currentTime, entity->m_motion.m_motionState);
            }
        }
        // The callee adds the offset to each array element, so it treats the entity pointers as untyped.
        hkRigidMotionUtilStep(
            physicsStepInfo, reinterpret_cast<hkpMotion* const*>(entities), numEntities,
            static_cast<int32_t>(offsetof(hkpEntity, m_motion))
        );
        hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, entities, numEntities);
    }
    resetCollisionInformationForEntities(entities, numEntities, world, RESET_TOI);
    if (reintegrateRecollideMode & hkpWorld::RR_MODE_RECOLLIDE_BROADPHASE) {
        collideEntitiesBroadPhaseContinuous(entities, numEntities, world, nullptr);
    }
    if (reintegrateRecollideMode & hkpWorld::RR_MODE_RECOLLIDE_NARROWPHASE) {
        collideEntitiesNarrowPhaseDiscrete(entities, numEntities, *world->m_collisionInput);
    }
    else if (reintegrate) {
        resetCollisionInformationForEntities(entities, numEntities, world, RESET_TIM);
    }
    // EXE-INLINED none: hkpWorld::unlockCriticalOperations
    if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
        if (world->m_pendingOperationsCount) {
            world->internal_executePendingOperations();
        }
        if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
            world->internal_executePendingBodyOperations();
        }
    }
    world->m_collisionInput->m_stepInfo = originalStepInfo;
}

// After a failed step the pending times of impact are dropped first, each removed from its contact manager (the
// island of the first entity, or of the second when the first is fixed, owns the contact constraint). The step info
// goes to the world's dynamics and collision input, the solver's step times follow it, then the active islands run
// the continuous broad phase and the continuous narrow phase, each pass under the world's critical-operations lock;
// running out of memory ends either pass early. The island post-collide listeners hear about each island. The
// timer entries go into the monitor stream's raw buffer as in hkpSimulation::advanceTime.
void hkpContinuousSimulation::collideInternal(const hkStepInfo& stepInfoIn) {
    HK_TIMER_BEGIN("Collide", nullptr);
    if (m_previousStepResult) {
        // Every pending event leaves its contact manager and the list is emptied.
        for (int32_t i = m_toiEvents.m_size - 1; i >= 0; --i) {
            hkpToiEvent& event = m_toiEvents.m_data[i];
            hkpEntity* entity = event.m_entities[0];
            if (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                entity = event.m_entities[1];
            }
            event.m_contactMgr->removeToi(*entity->m_simulationIsland, event.m_properties);
        }
        m_toiEvents.m_size = 0;
    }
    m_world->m_dynamicsStepInfo.m_stepInfo = stepInfoIn;
    m_world->m_collisionInput->m_stepInfo = stepInfoIn;
    // Multiplies in double and rounds once to float.
    hkpSolverInfo& solverInfo = m_world->m_dynamicsStepInfo.m_solverInfo;
    solverInfo.m_deltaTime =
        static_cast<float>(static_cast<double>(solverInfo.m_invNumSteps) * static_cast<double>(stepInfoIn.m_deltaTime));
    solverInfo.m_invDeltaTime =
        static_cast<float>(static_cast<double>(solverInfo.m_numSteps) * static_cast<double>(stepInfoIn.m_invDeltaTime));
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++m_world->m_criticalOperationsLockCount;
    bool bOutOfMemory = false;
    {
        hkArray<hkpSimulationIsland*>& islands = m_world->m_activeSimulationIslands;
        for (int32_t i = 0; i < islands.m_size; ++i) {
            hkpSimulationIsland* island = islands.m_data[i];
            collideEntitiesBroadPhaseContinuous(island->m_entities.m_data, island->m_entities.m_size, m_world, nullptr);
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                bOutOfMemory = true;
                break;
            }
        }
    }
    if (!bOutOfMemory) {
        // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
        hkpWorld* world = m_world;
        if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
            if (world->m_pendingOperationsCount) {
                world->internal_executePendingOperations();
            }
            if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
                world->internal_executePendingBodyOperations();
            }
        }
        // EXE-INLINED none: hkpWorld::lockCriticalOperations
        ++m_world->m_criticalOperationsLockCount;
        hkArray<hkpSimulationIsland*>& islands = m_world->m_activeSimulationIslands;
        for (int32_t i = 0; i < islands.m_size; ++i) {
            hkpSimulationIsland* island = islands.m_data[i];
            collideIslandNarrowPhaseContinuous(island, *m_world->m_collisionInput);
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                break;
            }
            if (m_world->m_islandPostCollideListeners.m_size) {
                HK_TIMER_BEGIN("IslandPostCollideCb", nullptr);
                hkpWorldCallbackUtil::fireIslandPostCollideCallback(m_world, island, stepInfoIn);
                HK_TIMER_END();
            }
        }
    }
    // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
    hkpWorld* world = m_world;
    if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
        if (world->m_pendingOperationsCount) {
            world->internal_executePendingOperations();
        }
        if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
            world->internal_executePendingBodyOperations();
        }
    }
    HK_TIMER_END();
}

void hkpContinuousSimulation::warpTime(float warpDeltaTime) {
    hkpContinuousSimulation* m_simulation = static_cast<hkpContinuousSimulation*>(m_world->m_simulation);
    int32_t v3 = 0;
    if (m_simulation->m_toiEvents.m_size > 0) {
        int32_t v4 = 0;
        do {
            hkpToiEvent* m_data = m_simulation->m_toiEvents.begin();
            double m_time = m_data[v4].m_time;
            float* p_m_time = &m_data[v4].m_time;
            ++v3;
            ++v4;
            *p_m_time = m_time + warpDeltaTime;
        }
        while (v3 < m_simulation->m_toiEvents.m_size);
    }
}

// Runs the continuous agents of the entities' pairs once each: a pair is skipped when the partner is in the same
// island and was handled already (the visited flags are one byte per island entity, on the stack allocator). A pair
// whose quality is not continuous, or uses simple toi handling, instead adds the entity (one reference taken) to
// entitiesNeedingPsiCollisionDetection. Out of memory after an agent stops the walk. The output's constraint owner is
// the first entity's island.
void hkpContinuousSimulation::collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(
    hkpEntity** entities,
    int32_t numEntities,
    const hkpProcessCollisionInput& input,
    hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection
) {
    hkpSimulationIsland* island = entities[0]->getSimulationIsland();
    const int32_t numEntitiesInIsland = island->m_entities.m_size;
    hkLocalArray<uint8_t> entityVisited(numEntitiesInIsland);
    hkString::memSet(entityVisited.m_data, 0, numEntitiesInIsland);

    // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput(island) (the toi record's time and its
    // properties' solver results are initialised, then the owner)
    hkpProcessCollisionOutput processOutput;
    processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
    processOutput.m_toi.m_properties.m_impulseApplied = 0.0f;
    processOutput.m_toi.m_properties.m_internalSolverData = 0.0f;
    processOutput.m_constraintOwner.m_storage = island;

    HK_TIMER_BEGIN("NarrowPhaseTOI", nullptr);
    bool outOfMemory = false;
    for (int32_t i = 0; i < numEntities; ++i) {
        hkpEntity* entity = entities[i];
        entityVisited.m_data[entity->m_storageIndex] = 1;
        hkArray<hkpLinkedCollidable::CollisionEntry> entries;
        entity->m_collidable.getCollisionEntriesSorted(entries);
        for (int32_t j = 0; j < entries.m_size; ++j) {
            hkpEntity* otherEntity = static_cast<hkpEntity*>(entries.m_data[j].m_partner->getOwner());
            if (otherEntity->m_simulationIsland == island && entityVisited.m_data[otherEntity->m_storageIndex]) {
                continue;
            }
            hkpAgentNnEntry* entry = entries.m_data[j].m_agentEntry;
            const hkpCollisionQualityInfo* qualityInfo =
                m_world->m_collisionDispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex);
            if (!qualityInfo->m_useContinuousPhysics.m_storage || qualityInfo->m_useSimpleToiHandling.m_bool) {
                if (!entitiesNeedingPsiCollisionDetection.getWithDefault(entity->getUid(), nullptr)) {
                    entity->addReference();
                    entitiesNeedingPsiCollisionDetection.insert(entity->getUid(), entity);
                }
                continue;
            }
            // EXE-INLINED 0xD0DAA0
            // EXE-INLINED none: hkpProcessCollisionOutput::reset
            processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
            processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
            processOutput.m_potentialContacts.m_storage = nullptr;
            hkpCollisionQualityInfo* processQualityInfo =
                input.m_dispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex);
            input.m_collisionQualityInfo = processQualityInfo;
            input.m_createPredictiveAgents.m_storage = processQualityInfo->m_useContinuousPhysics.m_storage;
            hkAgentNnMachine_ProcessAgent(entry, input, processOutput, entry->m_contactMgr);
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                outOfMemory = true;
                break;
            }
            if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
                entry->m_contactMgr->processContact(
                    *entry->m_collidable[0], *entry->m_collidable[1], input, processOutput
                );
            }
            // The time is compared as a double with the double HK_REAL_MAX.
            if (processOutput.m_toi.m_time.m_storage != 3.402820018375656e+38) {
                addToiEvent(processOutput, *entry);
            }
        }
        if (outOfMemory) {
            break;
        }
    }
    HK_TIMER_END();
}

// Recollides at the PSI time the agents of the entities the TOI phase moved: every agent entry of every entity in the
// map once (the reference the map holds on each entity is dropped), each processed with the island of its entity (a
// fixed entity's partner's island) as the constraint owner, its contacts handed to the contact manager.
void hkpContinuousSimulation::collideEntitiesNeedingPsiCollisionDetectionNarrowPhase_toiOnly(
    const hkpProcessCollisionInput& input,
    hkPointerMap<uint32_t, hkpEntity*, hkContainerHeapAllocator>& entitiesNeedingPsiCollisionDetection
) {
    HK_TIMER_BEGIN("Recollide PSI", nullptr);
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++m_world->m_criticalOperationsLockCount;

    hkPointerMap<hkpAgentNnEntry*, int32_t> entriesSet;
    hkArray<EntryAndIsland> entriesAndIslands;

    for (int32_t it = entitiesNeedingPsiCollisionDetection.getIterator();
         entitiesNeedingPsiCollisionDetection.isValid(it); it = entitiesNeedingPsiCollisionDetection.getNext(it)) {
        hkpEntity* entity = entitiesNeedingPsiCollisionDetection.getValue(it);
        hkArray<hkpLinkedCollidable::CollisionEntry>& collisionEntries =
            entity->m_collidable.getCollisionEntriesNonDeterministic();
        const int32_t numCollisionEntries = collisionEntries.m_size;
        for (int32_t i = 0; i < numCollisionEntries; ++i) {
            hkpAgentNnEntry* entry = collisionEntries.m_data[i].m_agentEntry;
            if (entriesSet.getWithDefault(entry, 0)) {
                continue;
            }
            const hkpCollisionQualityInfo* quality = m_world->m_collisionDispatcher->getCollisionQualityInfo(
                static_cast<hkpCollisionDispatcher::CollisionQualityIndex>(entry->m_collisionQualityIndex)
            );
            // EXE-INLINED none: hkSweptTransform::getInvDeltaTime
            const float invDeltaTime = entity->getMotion()->m_motionState.m_sweptTransform.m_centerOfMass1(3);
            if (quality->m_useContinuousPhysics.m_storage &&
                !(quality->m_useSimpleToiHandling.m_bool && 0.0f == invDeltaTime)) {
                continue;
            }
            hkpSimulationIsland* island = entity->getSimulationIsland();
            if (entity->getMotion()->getType() == hkpMotion::MOTION_FIXED) {
                hkpEntity* partner = hkSelectOther(
                    entity, static_cast<hkpEntity*>(entry->m_collidable[0]->getOwner()),
                    static_cast<hkpEntity*>(entry->m_collidable[1]->getOwner())
                );
                island = partner->getSimulationIsland();
            }
            // Inserts the value 0, which getWithDefault above cannot tell from an absent key, as the game does.
            entriesSet.insert(entry, 0);
            {
                // EXE-INLINED none: hkArray<EntryAndIsland>::expandOne
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (entriesAndIslands.m_size == entriesAndIslands.getCapacity()) {
                    hkArrayUtil::_reserveMore(router.heap(), &entriesAndIslands, sizeof(EntryAndIsland));
                }
                EntryAndIsland& entryAndIsland = entriesAndIslands.m_data[entriesAndIslands.m_size++];
                entryAndIsland.m_island = island;
                entryAndIsland.m_entry = entry;
            }
        }
        entity->removeReference();
    }
    entitiesNeedingPsiCollisionDetection.m_map.clear();

    for (int32_t i = 0; i < entriesAndIslands.m_size; ++i) {
        hkpAgentNnEntry* entry = entriesAndIslands.m_data[i].m_entry;
        // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput(island) (the toi record's time and
        // its properties' solver results are initialised, then the owner; then reset)
        hkpProcessCollisionOutput output;
        output.m_toi.m_properties.m_impulseApplied = 0.0f;
        output.m_toi.m_properties.m_internalSolverData = 0.0f;
        output.m_constraintOwner.m_storage = entriesAndIslands.m_data[i].m_island;
        output.m_toi.m_time.m_storage = 3.40282e+38f;
        output.m_firstFreeContactPoint.m_storage = output.m_contactPoints;
        output.m_potentialContacts.m_storage = nullptr;
        hkpCollisionQualityInfo* qualityInfo = input.m_dispatcher->getCollisionQualityInfo(
            static_cast<hkpCollisionDispatcher::CollisionQualityIndex>(entry->m_collisionQualityIndex)
        );
        input.m_collisionQualityInfo = qualityInfo;
        input.m_createPredictiveAgents.m_storage = qualityInfo->m_useContinuousPhysics.m_storage;
        hkAgentNnMachine_ProcessAgent(entry, input, output, entry->m_contactMgr);
        if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
            break;
        }
        if (output.m_firstFreeContactPoint.m_storage != output.m_contactPoints) {
            entry->m_contactMgr->processContact(*entry->m_collidable[0], *entry->m_collidable[1], input, output);
        }
    }

    // EXE-INLINED none: hkpWorld::unlockCriticalOperations
    hkpWorld* world = m_world;
    if (--world->m_criticalOperationsLockCount == 0 && !world->m_blockExecutingPendingOperations) {
        if (world->m_pendingOperationsCount) {
            world->internal_executePendingOperations();
        }
        if (world->m_pendingOperationQueueCount == 1 && world->m_pendingBodyOperationsCount) {
            world->internal_executePendingBodyOperations();
        }
    }
    HK_TIMER_END();
}

// Solves one time of impact event localised to its island: the toi resource manager sets up the solver's limits, the
// solve runs on the stack allocator's entity state bytes, and every other pending event that names an entity the solve
// made active in that island is removed. The active entities (one reference each, held over the step) are then
// collided again, broad phase and continuous narrow phase, after the ones that left the world are dropped. Each stage
// runs under the "TOI" timer list; the world's critical operations are locked from the solve to the end.
void hkpContinuousSimulation::simulateToi(
    hkpWorld* world,
    hkpToiEvent& event,
    float physicsDeltaTime,
    float rotateNormal
) {
    alignas(16) hkpToiResources toiResources;
    toiResources.m_maxNumEntities = 1000;
    toiResources.m_maxNumActiveEntities = 1000;
    toiResources.m_maxNumConstraints = 1000;
    toiResources.m_numToiSolverIterations = 4;
    toiResources.m_numForcedToiFinalSolverIterations = 4;
    toiResources.m_scratchpad = nullptr;
    toiResources.m_scratchpadSize = 0;
    toiResources.m_minPriorityToProcess = hkpConstraintInstance::PRIORITY_TOI;
    if (m_toiResourceMgr->beginToiAndSetupResources(event, m_toiEvents, toiResources) == HK_FAILURE) {
        return;
    }
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++m_world->m_criticalOperationsLockCount;

    // EXE-INLINED none: hkStepInfo::hkStepInfo(startTime, endTime)
    alignas(16) hkStepInfo stepInfo;
    stepInfo.m_startTime = event.m_time;
    alignas(16) hkInplaceArray<hkpEntity*, 64> activeEntities;
    stepInfo.m_endTime = world->getCurrentPsiTime();
    stepInfo.m_deltaTime = stepInfo.m_endTime - stepInfo.m_startTime;
    stepInfo.m_invDeltaTime = 1.0f / stepInfo.m_deltaTime;

    alignas(16) hkpSolverInfo solverInfo;
    solverInfo.operator=(world->m_dynamicsStepInfo.m_solverInfo);
    solverInfo.m_invNumSteps = 1.0f;
    solverInfo.m_deltaTime = physicsDeltaTime;
    solverInfo.m_numSteps = 1;
    solverInfo.m_numMicroSteps = 1;
    solverInfo.m_globalAccelerationPerSubStep.setZero4();
    solverInfo.m_globalAccelerationPerStep.setZero4();
    solverInfo.m_invDeltaTime = 1.0f / physicsDeltaTime;
    solverInfo.setTauAndDamping(0.5f, 1.4f);

    alignas(16) hkpConstraintQueryIn queryIn;
    queryIn.set(solverInfo, stepInfo, world->m_violatedConstraintArray);
    alignas(16) hkpConstraintSolverResources solverResources;
    hkpConstraintSolverSetup::initializeSolverState(
        stepInfo, solverInfo, queryIn, toiResources.m_scratchpad, toiResources.m_scratchpadSize, solverResources
    );

    HK_TIMER_BEGIN_LIST("TOI", "SolveToi");
    hkpEntity* islandEntity = event.m_entities[0];
    if (islandEntity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
        islandEntity = event.m_entities[1];
    }
    hkpSimulationIsland* island = islandEntity->m_simulationIsland;
    const int32_t numEntitiesInIsland = island->m_entities.m_size;
    hkLocalBuffer<uint8_t> entityState;
    // EXE-INLINED none: hkLocalBuffer<hkUchar>::hkLocalBuffer(numEntitiesInIsland) (hkAllocateStack)
    hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
    // Keeps its own copy of the block's address: it indexes the states through it after the solve and gives it back to
    // the stack at the end, never reloading the buffer's field, which hkLs_localizedSolveToi gets by reference.
    char* pBlock = stack.m_next.m_storage;
    stack.m_next.m_storage = pBlock + ((numEntitiesInIsland + 127) & ~127);
    // The stack allocator hands out raw bytes.
    uint8_t* pStates = reinterpret_cast<uint8_t*>(pBlock);
    entityState.m_data.m_storage = pStates;
    hkString::memSet(pStates, 0, numEntitiesInIsland);
    const hkResult result = hkLs_localizedSolveToi(
        toiResources, solverResources, event, *m_toiResourceMgr, world, activeEntities, entityState, rotateNormal
    );
    hkpConstraintSolverSetup::shutdownSolver(solverResources);

    hkpEntity* owner = event.m_entities[0];
    if (owner->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
        owner = event.m_entities[1];
    }
    event.m_contactMgr->removeToi(*owner->m_simulationIsland, event.m_properties);

    if (result == HK_SUCCESS) {
        // Every other pending event with an entity of the island the solve made active goes (removeAt moves the last
        // event into the hole).
        for (int32_t i = m_toiEvents.m_size - 1; i >= 0; --i) {
            hkpToiEvent& other = m_toiEvents.m_data[i];
            hkpEntity* entity0 = other.m_entities[0];
            hkpEntity* entity1;
            if ((entity0->m_simulationIsland == island &&
                 pStates[entity0->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE) ||
                ((entity1 = other.m_entities[1])->m_simulationIsland == island &&
                 pStates[entity1->m_storageIndex] == HK_TOI_ENTITY_STATE_ACTIVE)) {
                hkpEntity* otherOwner = other.m_entities[0];
                if (otherOwner->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                    otherOwner = other.m_entities[1];
                }
                other.m_contactMgr->removeToi(*otherOwner->m_simulationIsland, other.m_properties);
                // EXE-INLINED none: hkArrayBase<hkpToiEvent>::removeAt
                --m_toiEvents.m_size;
                if (m_toiEvents.m_size != i) {
                    m_toiEvents.m_data[i] = m_toiEvents.m_data[m_toiEvents.m_size];
                }
            }
        }
        hkReferencedObject::addReferences(activeEntities.m_data, activeEntities.m_size);
    }
    else {
        // EXE-INLINED none: hkArray<hkpEntity*>::clear
        activeEntities.m_size = 0;
    }
    // EXE-INLINED none: hkLocalBuffer<hkUchar>::~hkLocalBuffer (hkDeallocateStack)
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBlock;
    m_toiResourceMgr->endToiAndFreeResources(event, m_toiEvents, toiResources);

    HK_TIMER_SPLIT_LIST("PendingOperations");
    HK_TIMER_SPLIT_LIST("Collide");
    if (activeEntities.m_size) {
        // The entities that left the world during the solve are dropped (the last one moves into the hole) and
        // their reference released.
        for (int32_t i = activeEntities.m_size - 1; i >= 0; --i) {
            hkpEntity* entity = activeEntities.m_data[i];
            if (!entity->m_world) {
                // EXE-INLINED none: hkArrayBase<hkpEntity*>::removeAt
                --activeEntities.m_size;
                if (activeEntities.m_size != i) {
                    activeEntities.m_data[i] = activeEntities.m_data[activeEntities.m_size];
                }
                entity->removeReference();
            }
        }
        const hkpProcessCollisionInput* collisionInput = world->m_collisionInput;
        hkpEntity** entities = activeEntities.m_data;
        const int32_t numEntities = activeEntities.m_size;
        if (numEntities) {
            collideEntitiesBroadPhaseContinuous(entities, numEntities, world, nullptr);
            if (hkOutOfMemoryState != hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(
                    entities, numEntities, *collisionInput, m_entitiesNeedingPsiCollisionDetection
                );
            }
        }
        hkReferencedObject::removeReferences(activeEntities.m_data, activeEntities.m_size);
    }

    // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
    hkpWorld* lockedWorld = m_world;
    if (--lockedWorld->m_criticalOperationsLockCount == 0 && !lockedWorld->m_blockExecutingPendingOperations) {
        if (lockedWorld->m_pendingOperationsCount) {
            lockedWorld->internal_executePendingOperations();
        }
        if (lockedWorld->m_pendingOperationQueueCount == 1 && lockedWorld->m_pendingBodyOperationsCount) {
            lockedWorld->internal_executePendingBodyOperations();
        }
    }
    HK_TIMER_END_LIST();
}

// Processes the given agent entries with continuous collision detection, the island as the constraint owner of each
// entry's output; with collisionQualityOverride the input takes the dispatcher's last quality level, while
// m_createPredictiveAgents still follows the entry's own level, as the game does.
void hkpContinuousSimulation::processAgentNnEntries(
    hkpAgentNnEntry* const* entries,
    int32_t numEntries,
    const hkpProcessCollisionInput& collisionInput,
    hkpSimulationIsland* island,
    hkBool forceManifoldQualityType
) {
    HK_TIMER_BEGIN("NarrowPhaseTOI", nullptr);
    for (int32_t i = 0; i < numEntries; ++i) {
        hkpAgentNnEntry* entry = entries[i];
        // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput
        hkpProcessCollisionOutput processOutput;
        processOutput.m_toi.m_properties.m_impulseApplied = 0.0f;
        processOutput.m_toi.m_properties.m_internalSolverData = 0.0f;
        processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
        processOutput.m_constraintOwner.m_storage = island;
        processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
        processOutput.m_potentialContacts.m_storage = nullptr;
        hkpCollisionDispatcher* dispatcher = collisionInput.m_dispatcher;
        if (forceManifoldQualityType.m_bool) {
            collisionInput.m_collisionQualityInfo =
                &dispatcher->m_collisionQualityInfo[hkpCollisionDispatcher::COLLISION_QUALITY_TMP_EXPAND_MANIFOLD];
        }
        else {
            collisionInput.m_collisionQualityInfo = dispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex);
        }
        collisionInput.m_createPredictiveAgents.m_storage =
            dispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex)->m_useContinuousPhysics.m_storage;
        hkAgentNnMachine_ProcessAgent(entry, collisionInput, processOutput, entry->m_contactMgr);
        if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
            entry->m_contactMgr->processContact(
                *entry->m_collidable[0], *entry->m_collidable[1], collisionInput, processOutput
            );
        }
    }
    HK_TIMER_END();
}

// The world's critical operations stay locked throughout. The contact manager confirms the event; each of its
// entities of simple TOI quality (HK_COLLIDABLE_QUALITY_DEBRIS_SIMPLE_TOI, 3) is frozen at the event's time, its
// TIMs are invalidated and every other pending event it is in is removed from its contact manager and from the list
// (the last event moved into its place), and it is recollided. The event's own TOI is removed last. The fixed-motion
// test and the removal follow collideInternal.
void hkpContinuousSimulation::handleSimpleToi(
    hkpWorld* world,
    hkpToiEvent& event,
    float physicsDeltaTime,
    float rotateNormal
) {
    HK_TIMER_BEGIN("SimpleTOI", nullptr);
    // EXE-INLINED none: hkpWorld::lockCriticalOperations
    ++m_world->m_criticalOperationsLockCount;
    hkInplaceArray<hkpEntity*, 2> toBeActivated;
    event.m_contactMgr->confirmToi(event, rotateNormal, toBeActivated);
    hkInplaceArray<hkpEntity*, 2> entitiesToRecollide;
    for (int32_t e = 0; e < 2; e++) {
        hkpEntity* entity = event.m_entities[e];
        if (entity->m_collidable.m_broadPhaseHandle.m_objectQualityType != 3) {
            continue;
        }
        hkSweptTransformUtil::freezeMotionState(event.m_time, entity->m_motion.m_motionState);
        hkpWorldAgentUtil::invalidateTim(entity, *world->m_collisionInput);
        // Every other pending event that names the frozen entity leaves the list, removed from its contact manager
        // first (removeAt moves the last event into the hole).
        for (int32_t i = m_toiEvents.m_size - 1; i >= 0; --i) {
            hkpToiEvent& other = m_toiEvents.m_data[i];
            hkpEntity* owner = other.m_entities[0];
            if (owner == entity || other.m_entities[1] == entity) {
                if (owner->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                    owner = other.m_entities[1];
                }
                other.m_contactMgr->removeToi(*owner->m_simulationIsland, other.m_properties);
                // EXE-INLINED none: hkArrayBase<hkpToiEvent>::removeAt
                --m_toiEvents.m_size;
                if (m_toiEvents.m_size != i) {
                    m_toiEvents.m_data[i] = m_toiEvents.m_data[m_toiEvents.m_size];
                }
            }
        }
        // EXE-INLINED none: hkArray<hkpEntity*>::pushBackUnchecked
        entitiesToRecollide.m_data[entitiesToRecollide.m_size++] = entity;
    }
    hkpEntity* owner = event.m_entities[0];
    if (owner->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
        owner = event.m_entities[1];
    }
    event.m_contactMgr->removeToi(*owner->m_simulationIsland, event.m_properties);
    if (entitiesToRecollide.m_size) {
        collideEntitiesOfOneIslandNarrowPhaseContinuous_toiOnly(
            entitiesToRecollide.m_data, entitiesToRecollide.m_size, *world->m_collisionInput,
            m_entitiesNeedingPsiCollisionDetection
        );
    }
    // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
    hkpWorld* lockedWorld = m_world;
    if (--lockedWorld->m_criticalOperationsLockCount == 0 && !lockedWorld->m_blockExecutingPendingOperations) {
        if (lockedWorld->m_pendingOperationsCount) {
            lockedWorld->internal_executePendingOperations();
        }
        if (lockedWorld->m_pendingOperationQueueCount == 1 && lockedWorld->m_pendingBodyOperationsCount) {
            lockedWorld->internal_executePendingBodyOperations();
        }
    }
    HK_TIMER_END();
}

hkBool hkpContinuousSimulation::fireEarliestToiCallbacksAndCheckEnabled(hkpToiEvent& event, float& rotateNormal) {
    event.m_properties.m_flags |= hkContactPointMaterial::CONTACT_IS_NEW;
    event.m_contactMgr->fireCallbacksForEarliestToi(event, rotateNormal);
    return (event.m_properties.m_flags & hkContactPointMaterial::CONTACT_IS_DISABLED) == 0;
}

// Runs every agent of the island's track with continuous collision detection (the island as the output's constraint
// owner), under the NarrowPhase timer; stops at the first agent after which the memory system is out of memory.
void hkpContinuousSimulation::collideIslandNarrowPhaseContinuous(
    hkpSimulationIsland* isle,
    const hkpProcessCollisionInput& input
) {
    HK_TIMER_BEGIN("NarrowPhase", nullptr);
    // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput
    hkpProcessCollisionOutput output;
    output.m_toi.m_time.m_storage = 3.40282e+38f;
    output.m_toi.m_properties.m_impulseApplied = 0.0f;
    output.m_toi.m_properties.m_internalSolverData = 0.0f;
    output.m_constraintOwner.m_storage = isle;
    hkpAgentNnTrack& track = isle->m_agentTrack;
    for (int32_t sectorIndex = 0; sectorIndex < track.m_sectors.m_size;) {
        hkpAgentNnSector* currentSector = track.m_sectors.m_data[sectorIndex];
        hkpAgentNnEntry* entry = currentSector->getBegin();
        sectorIndex++;
        // The sector's bytes are untyped storage the entries are carved from.
        // EXE-INLINED none: hkAddByteOffset<hkpAgentNnEntry>
        hkpAgentNnEntry* endEntry = (sectorIndex == track.m_sectors.m_size)
            ? reinterpret_cast<hkpAgentNnEntry*>(currentSector->m_data + track.m_bytesUsedInLastSector)
            : currentSector->getEnd();
        // Each entry's m_size is its byte length in that untyped storage.
        // EXE-INLINED none: hkAddByteOffset<hkpAgentNnEntry>
        for (; entry < endEntry;
             entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size)) {
            // Cache hints on the contact manager and the sector's bytes 128 past the entry; both are treated as plain
            // addresses.
            _mm_prefetch(reinterpret_cast<const char*>(entry->m_contactMgr), _MM_HINT_NTA);
            // The pair is read before the agent runs, and the contact manager again after it.
            hkpLinkedCollidable* collA = entry->m_collidable[0];
            hkpLinkedCollidable* collB = entry->m_collidable[1];
            _mm_prefetch(reinterpret_cast<const char*>(entry) + 128, _MM_HINT_NTA);
            // EXE-INLINED 0xD0DAA0
            hkpCollisionQualityInfo* qualityInfo =
                input.m_dispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex);
            input.m_collisionQualityInfo = qualityInfo;
            output.m_toi.m_time.m_storage = 3.40282e+38f;
            input.m_createPredictiveAgents.m_storage = qualityInfo->m_useContinuousPhysics.m_storage;
            output.m_firstFreeContactPoint.m_storage = output.m_contactPoints;
            output.m_potentialContacts.m_storage = nullptr;
            hkAgentNnMachine_ProcessAgent(entry, input, output, entry->m_contactMgr);
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                HK_TIMER_END();
                return;
            }
            if (output.m_firstFreeContactPoint.m_storage != output.m_contactPoints) {
                entry->m_contactMgr->processContact(*collA, *collB, input, output);
            }
            // The time is compared as a double with the double HK_REAL_MAX.
            if (output.m_toi.m_time.m_storage != 3.402820018375656e+38) {
                addToiEvent(output, *entry);
            }
        }
    }
    HK_TIMER_END();
}

// The SDK's broad phase step: the AABBs (32 bytes each) and handles go on the thread's stack allocator as in
// collideEntitiesBroadPhaseContinuousFindPairs, the pair arrays are local arrays on it too. The pairs found are
// removed and added under the first entity's island lock; the added ones only while the heap can take 1 KB per pair.
// A non-null exportFinished is waited on (spinning on the byte until it is 1 or 2) before the agents are touched.
void hkpContinuousSimulation::collideEntitiesBroadPhaseContinuous(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    char* exportFinished
) {
    if (!numEntities) {
        return;
    }
    HK_TIMER_BEGIN_LIST("BroadPhase", "GatherAabbs");
    hkLocalArray<hkpBroadPhaseHandlePair> newPairs(world->m_broadPhaseUpdateSize);
    hkLocalArray<hkpBroadPhaseHandlePair> delPairs(world->m_broadPhaseUpdateSize);
    {
        // The stack allocator hands out raw bytes, so both blocks are cast from char.
        // EXE-INLINED none: hkStackMemory::allocateStack
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        char* aabbsBuffer = router.m_stack.m_next.m_storage;
        router.m_stack.m_next.m_storage =
            aabbsBuffer + ((numEntities * static_cast<int32_t>(sizeof(hkAabbUint32)) + 127) & ~127);
        hkAabbUint32* aabbs = reinterpret_cast<hkAabbUint32*>(aabbsBuffer);
        // EXE-INLINED none: hkStackMemory::allocateStack
        hkMemoryRouter& router2 = hkMemoryRouter::getInstance();
        char* handlesBuffer = router2.m_stack.m_next.m_storage;
        router2.m_stack.m_next.m_storage =
            handlesBuffer + ((numEntities * static_cast<int32_t>(sizeof(hkpBroadPhaseHandle*)) + 127) & ~127);
        hkpBroadPhaseHandle** handles = reinterpret_cast<hkpBroadPhaseHandle**>(handlesBuffer);
        hkpEntity** entityIt = entities;
        hkpBroadPhaseHandle** handleIt = handles;
        hkAabbUint32* aabbIt = aabbs;
        for (int32_t i = numEntities - 1; i >= 0; i--) {
            hkpEntity* entity = *entityIt;
            if (i >= 4) {
                __builtin_prefetch(&entityIt[4]->m_collidable, 0, 0);
            }
            *handleIt = &entity->m_collidable.m_broadPhaseHandle;
            const hkpCollidable::BoundingVolumeData& bv = entity->m_collidable.m_boundingVolumeData;
            if (bv.m_min[0] > bv.m_max[0]) {
                hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, &entity, 1);
            }
            // Only the min and max words of each AABB are written, as the game does.
            const int32_t shift = bv.m_expansionShift;
            aabbIt->m_min[1] = bv.m_min[1] - (uint32_t(bv.m_expansionMin[1]) << shift);
            aabbIt->m_min[2] = bv.m_min[2] - (uint32_t(bv.m_expansionMin[2]) << shift);
            aabbIt->m_min[0] = bv.m_min[0] - (uint32_t(bv.m_expansionMin[0]) << shift);
            aabbIt->m_max[1] = bv.m_max[1] + (uint32_t(bv.m_expansionMax[1]) << shift);
            aabbIt->m_max[0] = bv.m_max[0] + (uint32_t(bv.m_expansionMax[0]) << shift);
            aabbIt->m_max[2] = bv.m_max[2] + (uint32_t(bv.m_expansionMax[2]) << shift);
            ++entityIt;
            ++handleIt;
            ++aabbIt;
        }
        HK_TIMER_SPLIT_LIST("3AxisSweep");
        // EXE-INLINED none: hkpBroadPhase::lock
        hkpBroadPhase* broadPhase = world->m_broadPhase;
        if (broadPhase->m_criticalSection) {
            broadPhase->lockImplementation();
        }
        world->m_broadPhase->updateAabbsUint32(handles, aabbs, numEntities, newPairs, delPairs);
        // EXE-INLINED none: hkpBroadPhase::unlock
        broadPhase = world->m_broadPhase;
        if (broadPhase->m_criticalSection) {
            broadPhase->unlockImplementation();
        }
        // EXE-INLINED none: hkStackMemory::shrinkAllocatedStack
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = handlesBuffer;
        // EXE-INLINED none: hkStackMemory::shrinkAllocatedStack
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = aabbsBuffer;
    }
    if (newPairs.m_size + delPairs.m_size > 0) {
        HK_TIMER_SPLIT_LIST("RemoveDup");
        hkpTypedBroadPhaseDispatcher::removeDuplicates(newPairs, delPairs);
    }
    if (exportFinished) {
        HK_TIMER_BEGIN("WaitForExport", nullptr);
        // Another thread sets the flag, so the byte is read again for each compare.
        volatile char* pFinished = exportFinished;
        while (*pFinished != 1 && *pFinished != 2) {}
        HK_TIMER_END();
    }
    if (newPairs.m_size + delPairs.m_size > 0) {
        HK_TIMER_SPLIT_LIST("RemoveAgt");
        world->lockIslandForConstraintUpdate(entities[0]->getSimulationIsland());
        world->m_broadPhaseDispatcher->removePairs(
            static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.m_data), delPairs.m_size
        );
        const int32_t numNewPairs = newPairs.m_size;
        if (hkMemoryRouter::getInstance().heap().canAllocTotal(numNewPairs * 1024)) {
            HK_TIMER_SPLIT_LIST("AddAgt");
            hkpCollisionFilter* filter = world->m_collisionFilter;
            world->m_broadPhaseDispatcher->addPairs(
                static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.m_data), newPairs.m_size,
                filter ? static_cast<hkpCollidableCollidableFilter*>(filter) : nullptr
            );
        }
        world->unlockIslandForConstraintUpdate(entities[0]->getSimulationIsland());
    }
    HK_TIMER_END_LIST();
}

// The AABBs (32 bytes each) and handles go on the thread's stack allocator, each block rounded up to 128 bytes, and
// are handed back in reverse order. Each AABB is the collidable's expanded by its expansion bytes; only the min and
// max words are written. An invalid AABB (min x above max x, unsigned) is recomputed first. The timer entries go into
// the monitor stream's raw buffer as in collideInternal; the first is a 16-byte list entry.
void hkpContinuousSimulation::collideEntitiesBroadPhaseContinuousFindPairs(
    hkpEntity** entities,
    int32_t numEntities,
    hkpWorld* world,
    hkArray<hkpBroadPhaseHandlePair>& newPairs,
    hkArray<hkpBroadPhaseHandlePair>& delPairs
) {
    if (!numEntities) {
        return;
    }
    HK_TIMER_BEGIN_LIST("BroadPhase", "GatherAabbs");
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    // The stack allocator hands out raw bytes, so both blocks are cast from char.
    // EXE-INLINED none: hkStackMemory::allocateStack
    char* aabbsBuffer = router.m_stack.m_next.m_storage;
    router.m_stack.m_next.m_storage = aabbsBuffer + ((numEntities * 32 + 127) & ~127);
    hkAabbUint32* aabbs = reinterpret_cast<hkAabbUint32*>(aabbsBuffer);
    hkMemoryRouter& router2 = hkMemoryRouter::getInstance();
    // EXE-INLINED none: hkStackMemory::allocateStack
    char* handlesBuffer = router2.m_stack.m_next.m_storage;
    router2.m_stack.m_next.m_storage =
        handlesBuffer + ((numEntities * static_cast<int32_t>(sizeof(hkpBroadPhaseHandle*)) + 127) & ~127);
    hkpBroadPhaseHandle** handles = reinterpret_cast<hkpBroadPhaseHandle**>(handlesBuffer);
    hkpEntity** entityIt = entities;
    hkpBroadPhaseHandle** handleIt = handles;
    hkAabbUint32* aabbIt = aabbs;
    for (int32_t i = numEntities - 1; i >= 0; i--) {
        hkpEntity* entity = *entityIt;
        if (i >= 4) {
            __builtin_prefetch(&entityIt[4]->m_collidable, 0, 0);
        }
        *handleIt = &entity->m_collidable.m_broadPhaseHandle;
        const hkpCollidable::BoundingVolumeData& bv = entity->m_collidable.m_boundingVolumeData;
        if (bv.m_min[0] > bv.m_max[0]) {
            hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, &entity, 1);
        }
        const int32_t shift = bv.m_expansionShift;
        aabbIt->m_min[1] = bv.m_min[1] - (uint32_t(bv.m_expansionMin[1]) << shift);
        aabbIt->m_min[2] = bv.m_min[2] - (uint32_t(bv.m_expansionMin[2]) << shift);
        aabbIt->m_min[0] = bv.m_min[0] - (uint32_t(bv.m_expansionMin[0]) << shift);
        aabbIt->m_max[1] = bv.m_max[1] + (uint32_t(bv.m_expansionMax[1]) << shift);
        aabbIt->m_max[0] = bv.m_max[0] + (uint32_t(bv.m_expansionMax[0]) << shift);
        aabbIt->m_max[2] = bv.m_max[2] + (uint32_t(bv.m_expansionMax[2]) << shift);
        ++entityIt;
        ++handleIt;
        ++aabbIt;
    }
    HK_TIMER_SPLIT_LIST("3AxisSweep");
    hkpBroadPhase* broadPhase = world->m_broadPhase;
    if (broadPhase->m_criticalSection) {
        broadPhase->lockImplementation();
    }
    world->m_broadPhase->updateAabbsUint32(handles, aabbs, numEntities, newPairs, delPairs);
    broadPhase = world->m_broadPhase;
    if (broadPhase->m_criticalSection) {
        broadPhase->unlockImplementation();
    }
    // EXE-INLINED none: hkStackMemory::shrinkAllocatedStack
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = handlesBuffer;
    // EXE-INLINED none: hkStackMemory::shrinkAllocatedStack
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = aabbsBuffer;
    if ((delPairs.m_size + newPairs.m_size) > 0) {
        HK_TIMER_SPLIT_LIST("RemoveDup");
        hkpTypedBroadPhaseDispatcher::removeDuplicates(newPairs, delPairs);
    }
    HK_TIMER_END_LIST();
}

void hkpContinuousSimulation::collideEntitiesNarrowPhaseDiscrete(
    hkpEntity** entities,
    int32_t numEntities,
    const hkpProcessCollisionInput& input
) {
    processAgentsOfEntities(
        entities, numEntities, input, &xfnv::callMember<&hkpContinuousSimulation::processAgentCollideContinuous>,
        hkpSimulation::FIND_CONTACTS_DEFAULT
    );
}

// Appends a time of impact event from the output's toi record and the entry's pair, contact manager and quality, or
// warns when the queue is full. The x87 copies of m_time and m_seperatingVelocity are plain
// float copies here.
void hkpContinuousSimulation::addToiEvent(
    const hkpProcessCollisionOutput& processOutput,
    const hkpAgentNnEntry& entry
) {
    if (m_toiEvents.m_size == (m_toiEvents.m_capacityAndFlags & hkArrayBase<hkpToiEvent>::CAPACITY_MASK)) {
        alignas(16) char buf[512];
        hkErrStream errStream(buf, 512);
        errStream << "Toi event queue full, consider using HK_COLLIDABLE_QUALITY_DEBRIS for some objects or increase "
                     "hkpWorldCinfo::m_sizeOfToiEventQueue";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xF0323454u), buf,
            ".\\World\\Simulation\\Continuous\\hkpContinuousSimulation.cpp", 1083
        );
    }
    else {
        hkpToiEvent* newEvents = m_toiEvents.expandByUnchecked(1);
        // EXE-INLINED none: hkArrayUtil::construct<hkpToiEvent>
        // The placement new of each new event: its properties' solver results start at zero.
        for (int32_t i = 0; i < 1; ++i) {
            hkpToiEvent* newEvent = &newEvents[i];
            if (newEvent != nullptr) {
                newEvent->m_properties.m_impulseApplied = 0.0f;
                newEvent->m_properties.m_internalSolverData = 0.0f;
            }
        }
        hkpToiEvent& event = *newEvents;
        event.m_time = processOutput.m_toi.m_time.m_storage;
        event.m_useSimpleHandling.m_bool =
            m_world->m_collisionDispatcher->m_collisionQualityInfo[static_cast<int8_t>(entry.m_collisionQualityIndex)]
                .m_useSimpleToiHandling.m_bool;
        event.m_seperatingVelocity = processOutput.m_toi.m_seperatingVelocity.m_storage;
        event.m_contactPoint = processOutput.m_toi.m_contactPoint;
        event.m_entities[0] = static_cast<hkpEntity*>(entry.m_collidable[0]->getOwner());
        event.m_entities[1] = static_cast<hkpEntity*>(entry.m_collidable[1]->getOwner());
        event.m_properties = static_cast<const hkpContactPointProperties&>(processOutput.m_toi.m_properties);
        event.m_contactMgr = static_cast<hkpDynamicsContactMgr*>(entry.m_contactMgr);
        hkString::memCpy4(
            event.m_extendedUserDatas, processOutput.m_toi.m_properties.m_extendedUserDatas,
            sizeof(event.m_extendedUserDatas) >> 2
        );
    }
}

// The SDK's: the entities go into a pointer map, then every event with either of its entities in the map is
// removed, its contact manager told through the island of its first non-fixed entity.
void hkpContinuousSimulation::removeToiEventsOfEntities(hkpEntity** entities, int32_t numEntities) {
    if (m_toiEvents.m_size == 0) {
        return;
    }
    hkPointerMap<hkpEntity*, int32_t> entitiesToRemove(numEntities);
    for (int32_t i = 0; i < numEntities; i++) {
        entitiesToRemove.insert(entities[i], 0);
    }
    for (int32_t i = 0; i < m_toiEvents.m_size; i++) {
        hkpToiEvent& event = m_toiEvents.m_data[i];
        const int32_t it0 = entitiesToRemove.findKey(event.m_entities[0]);
        const int32_t it1 = entitiesToRemove.findKey(event.m_entities[1]);
        if (entitiesToRemove.isValid(it0) || entitiesToRemove.isValid(it1)) {
            // EXE-INLINED none: hkpEntity::isFixed
            hkpSimulationIsland* island = event.m_entities[0]->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED
                ? event.m_entities[1]->getSimulationIsland()
                : event.m_entities[0]->getSimulationIsland();
            event.m_contactMgr->removeToiImpl(*island, event.m_properties);
            m_toiEvents.removeAt(i--);
        }
    }
}

// The SDK's: removes every event of entity, its contact manager told through the island of the event's first
// non-fixed entity.
void hkpContinuousSimulation::removeToiEventsOfEntity(hkpEntity* entity) {
    for (int32_t i = 0; i < m_toiEvents.m_size; i++) {
        hkpToiEvent& event = m_toiEvents.m_data[i];
        if (event.m_entities[0] == entity || event.m_entities[1] == entity) {
            // EXE-INLINED none: hkpEntity::isFixed
            hkpEntity* islandEntity = event.m_entities[0]->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED
                ? event.m_entities[1]
                : event.m_entities[0];
            event.m_contactMgr->removeToiImpl(*islandEntity->getSimulationIsland(), event.m_properties);
            m_toiEvents.removeAt(i--);
        }
    }
}

// Processes one agent entry with continuous collision detection: resets the output, sets the input's quality from the
// entry's quality index, runs the agent, hands new contact points to the entry's contact manager and queues a time of
// impact event when the agent found one; nothing after the agent when the memory system is out of memory.
void hkpContinuousSimulation::processAgentCollideContinuous(
    hkpAgentNnEntry* entry,
    const hkpProcessCollisionInput& processInput,
    hkpProcessCollisionOutput& processOutput
) {
    // EXE-INLINED none: hkpProcessCollisionOutput::reset
    processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
    processOutput.m_potentialContacts.m_storage = nullptr;
    processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
    hkpCollisionQualityInfo* qualityInfo =
        processInput.m_dispatcher->getCollisionQualityInfo(entry->m_collisionQualityIndex);
    processInput.m_collisionQualityInfo = qualityInfo;
    processInput.m_createPredictiveAgents.m_storage = qualityInfo->m_useContinuousPhysics.m_storage;
    hkAgentNnMachine_ProcessAgent(entry, processInput, processOutput, entry->m_contactMgr);
    if (hkOutOfMemoryState != hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
        if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
            entry->m_contactMgr->processContact(
                *entry->m_collidable[0], *entry->m_collidable[1], processInput, processOutput
            );
        }
        // The time is compared as a double with the double HK_REAL_MAX.
        if (processOutput.m_toi.m_time.m_storage != 3.402820018375656e+38) {
            addToiEvent(processOutput, *entry);
        }
    }
}

// Handles the pending times of impact up to minTime, earliest first (equal times ordered by the entities' uids), each
// under the world's critical-operations lock. An event its callbacks disable is dropped and the search starts again
// under the same lock. An enabled event is taken out of the list, the step from its time to the PSI time goes to the
// world's dynamics and collision input, and it is solved by handleSimpleToi or simulateToi. Running out of memory
// ends the loop.
hkpStepResult hkpContinuousSimulation::handleAllToisTill(float minTime) {
    while (true) {
        hkpWorld* world = m_world;
        // EXE-INLINED none: hkpWorld::lockCriticalOperations
        ++world->m_criticalOperationsLockCount;
        int32_t earliestIndex;
        float rotateNormal;
        while (true) {
            float earliestTime = minTime;
            rotateNormal = m_world->m_toiCollisionResponseRotateNormal;
            earliestIndex = -1;
            for (int32_t i = 0; i < m_toiEvents.m_size; i++) {
                const hkpToiEvent& event = m_toiEvents.m_data[i];
                if (earliestTime > event.m_time) {
                    earliestIndex = i;
                    earliestTime = event.m_time;
                }
                else if (event.m_time == earliestTime && earliestIndex != -1) {
                    const hkpToiEvent& earliest = m_toiEvents.m_data[earliestIndex];
                    const uint32_t uid0 = event.m_entities[0]->getUid();
                    const uint32_t uid1 = event.m_entities[1]->getUid();
                    const uint32_t earliestUid0 = earliest.m_entities[0]->getUid();
                    const uint32_t earliestUid1 = earliest.m_entities[1]->getUid();
                    if (uid0 < earliestUid0 || (uid0 == earliestUid0 && uid1 < earliestUid1)) {
                        earliestIndex = i;
                        earliestTime = event.m_time;
                    }
                }
            }
            if (earliestIndex < 0) {
                // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
                --world->m_criticalOperationsLockCount;
                world->attemptToExecutePendingOperations();
                return HK_STEP_RESULT_SUCCESS;
            }
            if (fireEarliestToiCallbacksAndCheckEnabled(m_toiEvents.m_data[earliestIndex], rotateNormal).m_bool) {
                break;
            }
            // EXE-INLINED none: hkArrayBase<hkpToiEvent>::removeAt
            --m_toiEvents.m_size;
            if (m_toiEvents.m_size != earliestIndex) {
                m_toiEvents.m_data[earliestIndex] = m_toiEvents.m_data[m_toiEvents.m_size];
            }
        }
        alignas(16) hkpToiEvent event = m_toiEvents.m_data[earliestIndex];
        // EXE-INLINED none: hkArrayBase<hkpToiEvent>::removeAt
        --m_toiEvents.m_size;
        if (m_toiEvents.m_size != earliestIndex) {
            m_toiEvents.m_data[earliestIndex] = m_toiEvents.m_data[m_toiEvents.m_size];
        }
        const float eventTime = event.m_time;
        m_currentTime = eventTime;
        const float psiTime = m_currentPsiTime;
        // EXE-INLINED none: hkStepInfo::hkStepInfo
        hkStepInfo stepInfo;
        stepInfo.m_startTime = eventTime;
        stepInfo.m_endTime = psiTime;
        // The difference is rounded to a float before it is tested.
        float fDeltaTime = psiTime - eventTime;
        const float deltaTime = fDeltaTime;
        stepInfo.m_deltaTime = deltaTime;
        stepInfo.m_invDeltaTime = (deltaTime == 0.0f) ? 0.0f : 1.0f / deltaTime;
        m_world->m_dynamicsStepInfo.m_stepInfo = stepInfo;
        m_world->m_collisionInput->m_stepInfo = stepInfo;
        const float physicsDeltaTime = m_physicsDeltaTime;
        const float eventRotateNormal = rotateNormal;
        if (event.m_useSimpleHandling.m_bool) {
            handleSimpleToi(m_world, event, physicsDeltaTime, eventRotateNormal);
        }
        else {
            simulateToi(m_world, event, physicsDeltaTime, eventRotateNormal);
        }
        ++m_toiCounter;
        if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
            // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
            --world->m_criticalOperationsLockCount;
            world->attemptToExecutePendingOperations();
            return HK_STEP_RESULT_MEMORY_FAILURE_DURING_TOI_SOLVE;
        }
        // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
        --world->m_criticalOperationsLockCount;
        world->attemptToExecutePendingOperations();
    }
}

hkpStepResult hkpContinuousSimulation::advanceTimeInternal() {
    HK_TIMER_BEGIN("Physics", nullptr);
    if (m_previousStepResult != HK_STEP_RESULT_SUCCESS) {
        HK_TIMER_BEGIN("reCollideAfterStepFailure", nullptr);
        hkpStepResult result = reCollideAfterStepFailure();
        if (result != HK_STEP_RESULT_SUCCESS) {
            HK_TIMER_END();
            HK_TIMER_END();
            return result;
        }
        HK_TIMER_END();
    }
    float timeToAdvanceTo = snapSimulateTimeAndGetTimeToAdvanceTo();
    if (m_toiEvents.m_size) {
        HK_TIMER_BEGIN("TOIs", nullptr);
        hkpStepResult result = handleAllToisTill(timeToAdvanceTo);
        if (result != HK_STEP_RESULT_SUCCESS) {
            m_previousStepResult = result;
            HK_TIMER_END();
            HK_TIMER_END();
            return result;
        }
        HK_TIMER_END();
    }
    m_previousStepResult = HK_STEP_RESULT_SUCCESS;
    m_currentTime = timeToAdvanceTo;
    HK_TIMER_END();
    return HK_STEP_RESULT_SUCCESS;
}

hkpSolverInfo& hkpSolverInfo::operator=(const hkpSolverInfo& other) {
    m_one = other.m_one;
    m_tau = other.m_tau;
    m_damping = other.m_damping;
    m_frictionTau = other.m_frictionTau;
    m_globalAccelerationPerSubStep = other.m_globalAccelerationPerSubStep;
    m_globalAccelerationPerStep = other.m_globalAccelerationPerStep;
    m_integrateVelocityFactor = other.m_integrateVelocityFactor;
    m_invIntegrateVelocityFactor = other.m_invIntegrateVelocityFactor;
    m_dampDivTau = other.m_dampDivTau;
    m_tauDivDamp = other.m_tauDivDamp;
    m_dampDivFrictionTau = other.m_dampDivFrictionTau;
    m_frictionTauDivDamp = other.m_frictionTauDivDamp;
    m_contactRestingVelocity = other.m_contactRestingVelocity;
    rt::memcpy(m_deactivationInfo, other.m_deactivationInfo, sizeof(hkpSolverInfo::DeactivationInfo) * 1);
    m_deactivationInfo[1] = other.m_deactivationInfo[1];
    m_deactivationInfo[2] = other.m_deactivationInfo[2];
    m_deactivationInfo[3] = other.m_deactivationInfo[3];
    m_deactivationInfo[4] = other.m_deactivationInfo[4];
    rt::memcpy(
        &m_deactivationInfo[5], &other.m_deactivationInfo[5],
        offsetof(hkpSolverInfo, m_maxConstraintViolationSqrd) + sizeof(hkpSolverInfo::m_maxConstraintViolationSqrd) -
            offsetof(hkpSolverInfo, m_deactivationInfo[5])
    );
    return *this;
}
