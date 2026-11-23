#include "Havok/Source/Physics/Dynamics/Constraint/Setup/hkpConstraintSolverSetup.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverElemTemp.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "xfnv/Compiler.h"

#include <cstdint>
#include <xmmintrin.h>

// The step hkpConstraintSolverSetup's solve takes after a successful solve, as a block of its own: the solver's
// velocities (after the fixed body's accumulator, the first) applied to the entities' motions, then their AABBs
// recomputed. The entities array is handed over as the motions, each motion at m_motion in its entity.
int32_t hkConstraintSolverSetup_integrate2(
    const hkStepInfo* info,
    const hkpSolverInfo* solverInfo,
    const hkpVelocityAccumulator* accumulators,
    hkpEntity* const* entities,
    int32_t numEntities
) {
    const int32_t result = hkRigidMotionUtilApplyAccumulators(
        *solverInfo,
        *info,
        accumulators + 1,
        reinterpret_cast<hkpMotion* const*>(entities),
        numEntities,
        offsetof(hkpEntity, m_motion)
    );
    hkpEntityAabbUtil::entityBatchRecalcAabb(entities[0]->m_world->m_collisionInput, entities, numEntities);
    return result;
}

// Splits the 16-aligned buffer: 10/64 of it for accumulators, 4/64 for their backups, then the two schema ranges (0.6
// and 0.4 of 40/64 of it, through a float), the rest for element temps. The float/double products are truncated. The
// buffer is raw memory carved by address, hence the reinterpret_casts.
void hkpConstraintSolverSetup::initializeSolverState(
    hkStepInfo& stepInfo,
    hkpSolverInfo& solverInfo,
    hkpConstraintQueryIn& constraintQueryIn,
    char* buffer,
    int32_t bufferSize,
    hkpConstraintSolverResources& s
) {
    s.m_solverInfo = &solverInfo;
    s.m_stepInfo = &stepInfo;
    s.m_constraintQueryInput = &constraintQueryIn;
    int32_t size = bufferSize - bufferSize % 16;
    uintptr_t accumulators = (reinterpret_cast<uintptr_t>(buffer) + 15) & ~static_cast<uintptr_t>(15);
    uintptr_t backups = (accumulators + size * 10 / 64) & ~static_cast<uintptr_t>(15);
    uintptr_t schemas0 = (backups + size * 4 / 64) & ~static_cast<uintptr_t>(15);
    float schemaSize = static_cast<float>(size * 40);
    int32_t schemaSize0 = static_cast<int32_t>(static_cast<int64_t>(schemaSize * -0.6000000238418579 * 0.015625));
    s.m_schemas[0].m_begin = reinterpret_cast<hkpJacobianSchema*>(schemas0);
    s.m_accumulatorsBackupEnd = reinterpret_cast<hkpConstraintSolverResources::VelocityAccumTransformBackup*>(schemas0);
    s.m_accumulatorsEnd = reinterpret_cast<hkpVelocityAccumulator*>(backups);
    s.m_accumulatorsBackup = reinterpret_cast<hkpConstraintSolverResources::VelocityAccumTransformBackup*>(backups);
    s.m_accumulators = reinterpret_cast<hkpVelocityAccumulator*>(accumulators);
    uintptr_t schemas1 = (schemas0 - schemaSize0) & ~static_cast<uintptr_t>(15);
    s.m_schemas[1].m_begin = reinterpret_cast<hkpJacobianSchema*>(schemas1);
    s.m_schemas[0].m_end = reinterpret_cast<hkpJacobianSchema*>(schemas1);
    int32_t schemaSize1 = static_cast<int32_t>(static_cast<int64_t>(0.015625 * (schemaSize * -0.3999999761581421)));
    uintptr_t elemTemp = (schemas1 - schemaSize1) & ~static_cast<uintptr_t>(15);
    s.m_accumulatorsCurrent = reinterpret_cast<hkpVelocityAccumulator*>(accumulators);
    s.m_elemTempEnd = reinterpret_cast<hkpSolverElemTemp*>(buffer + size);
    s.m_schemas[0].m_current = s.m_schemas[0].m_begin;
    s.m_schemas[0].m_lastProcessed = s.m_schemas[0].m_begin;
    s.m_schemas[1].m_end = reinterpret_cast<hkpJacobianSchema*>(elemTemp);
    s.m_elemTemp = reinterpret_cast<hkpSolverElemTemp*>(elemTemp);
    s.m_elemTempCurrent = reinterpret_cast<hkpSolverElemTemp*>(elemTemp);
    s.m_elemTempLastProcessed = reinterpret_cast<hkpSolverElemTemp*>(elemTemp);
    s.m_schemas[1].m_current = s.m_schemas[1].m_begin;
    s.m_schemas[1].m_lastProcessed = s.m_schemas[1].m_begin;
}

// Adds the accumulators of numEntities entities: on the first call a fixed accumulator (index 0) and its zeroed backup,
// then each entity's m_solverData is its accumulator's byte offset, and the built accumulators' scratch0 and scratch123
// column 0 move into the backups. The entity array goes to the util as motions with m_motion's offset, as Havok passes
// it.
void hkpConstraintSolverSetup::internalAddAccumulators(
    hkpConstraintSolverResources& s,
    hkpEntity* const* entities,
    int32_t numEntities
) {
    // The compares are unsigned.
    uint32_t uiNumEntities = static_cast<uint32_t>(numEntities);
    if (uiNumEntities == 0) {
        return;
    }
    if (s.m_accumulatorsCurrent == s.m_accumulators) {
        hkpVelocityAccumulator* fixedAcc = s.m_accumulatorsCurrent;
        fixedAcc->setFixed();
        s.m_accumulatorsCurrent = fixedAcc + 1;
        hkpConstraintSolverResources::VelocityAccumTransformBackup* fixedBackup = s.m_accumulatorsBackup;
        const hkQuadReal zero = {0.0f, 0.0f, 0.0f, 0.0f};
        fixedBackup->m_angular.m_quad = zero;
        fixedBackup->m_linear.m_quad = zero;
    }
    for (uint32_t i = 0; i < uiNumEntities; ++i) {
        // The accumulator buffer is raw memory: offsets in bytes from its start.
        entities[i]->m_solverData = static_cast<uint32_t>(
            i * sizeof(hkpVelocityAccumulator) - reinterpret_cast<uintptr_t>(s.m_accumulators) +
            reinterpret_cast<uintptr_t>(s.m_accumulatorsCurrent)
        );
    }
    hkpVelocityAccumulator* firstAcc = s.m_accumulatorsCurrent;
    hkpConstraintSolverResources::VelocityAccumTransformBackup* backup = s.m_accumulatorsBackup +
        (entities[0]->m_solverData >> 7);
    // Havok's motionsOffset convention: the entity pointers stand for their motions at +0xE0.
    s.m_accumulatorsCurrent = hkRigidMotionUtilApplyForcesAndBuildAccumulators(
        *s.m_stepInfo,
        reinterpret_cast<hkpMotion* const*>(entities),
        uiNumEntities,
        offsetof(hkpEntity, m_motion),
        firstAcc
    );
    const hkQuadReal zero = {0.0f, 0.0f, 0.0f, 0.0f};
    for (hkpVelocityAccumulator* acc = firstAcc; uiNumEntities != 0; --uiNumEntities, ++acc, ++backup) {
        backup->m_linear.m_quad = acc->m_scratch0.m_quad;
        backup->m_angular.m_quad = acc->m_scratch123.m_col0.m_quad;
        acc->m_scratch0.m_quad = zero;
        acc->m_scratch123.m_col0.m_quad = zero;
    }
    s.m_accumulatorsCurrent->m_type = hkpVelocityAccumulator::HK_END;
}

// Builds one constraint's Jacobian at a time: it points the query in at the constraint's two accumulators (restoring
// their scratch columns from the backups), at the instance, the runtime and the two bodies' transforms, appends the
// constraint, its first schema slot and the smaller of its bodies' allowed penetration depths to cinfo, and calls the
// builder of the constraint's first atom kind (the contact builder for a contact atom, the atoms builder otherwise).
// A contact constraint fires its contact callbacks first, and a constraint whose internal record asks for it runs the
// data's buildJacobianCallback. A forced-TOI constraint builds into the second schema range. The accumulators' scratch
// columns are zeroed again afterwards, and both ranges end with a zero schema.
hkpJacobianSchema* hkpConstraintSolverSetup::internalAddJacobianSchemas(
    hkpConstraintSolverResources& s,
    hkpConstraintInstance** constraints,
    int32_t numConstraints,
    hkArray<hkpConstraintSchemaInfo>& constraintStatus
) {
    hkpConstraintQueryIn* pQueryIn = s.m_constraintQueryInput;
    // The end is computed once, with pointer-width wrap, and compared unsigned: it is built through uintptr_t so that
    // clang does not fold the entry test into a sign test of numConstraints.
    hkpConstraintInstance** const end = reinterpret_cast<hkpConstraintInstance**>(
        reinterpret_cast<uintptr_t>(constraints) + static_cast<uintptr_t>(numConstraints) * sizeof(*constraints)
    );
    for (hkpConstraintInstance** c = constraints; c < end; ++c) {
        const hkQuadReal zero = {0.0f, 0.0f, 0.0f, 0.0f};
        hkpConstraintInstance* pInstance = *c;
        hkConstraintInternal* pInternal = pInstance->m_internal;
        hkpEntity* pEntityA = pInternal->m_entities[0];
        hkpEntity* pEntityB = pInternal->m_entities[1];
        const int32_t iSchemaSet = pInstance->m_priority.m_storage == hkpConstraintInstance::PRIORITY_TOI_FORCED ? 1
                                                                                                                 : 0;
        // The accumulator buffer is raw memory: the entities' solver data are byte offsets from its start.
        hkpVelocityAccumulator* pAccumulatorA = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<uintptr_t>(s.m_accumulators) + pEntityA->m_solverData
        );
        hkpVelocityAccumulator* pAccumulatorB = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<uintptr_t>(s.m_accumulators) + pEntityB->m_solverData
        );
        pQueryIn->m_bodyA = pAccumulatorA;
        pQueryIn->m_bodyB = pAccumulatorB;
        hkpConstraintSolverResources::VelocityAccumTransformBackup* pBackupA = s.m_accumulatorsBackup +
            (pEntityA->m_solverData >> 7);
        hkpConstraintSolverResources::VelocityAccumTransformBackup* pBackupB = s.m_accumulatorsBackup +
            (pEntityB->m_solverData >> 7);
        pAccumulatorA->m_scratch0.m_quad = pBackupA->m_linear.m_quad;
        pAccumulatorA->m_scratch123.m_col0.m_quad = pBackupA->m_angular.m_quad;
        pAccumulatorB->m_scratch0.m_quad = pBackupB->m_linear.m_quad;
        pAccumulatorB->m_scratch123.m_col0.m_quad = pBackupB->m_angular.m_quad;
        pQueryIn->m_constraintInstance = pInstance;
        pQueryIn->m_transformA = &pEntityA->m_motion.m_motionState.m_transform;
        pQueryIn->m_transformB = &pEntityB->m_motion.m_motionState.m_transform;
        pQueryIn->m_constraintRuntime.m_storage = pInternal->m_runtime;
        pQueryIn->m_accumulatorAIndex.m_storage = static_cast<uint16_t>(pEntityA->m_solverData >> 7);
        pQueryIn->m_accumulatorBIndex.m_storage = static_cast<uint16_t>(pEntityB->m_solverData >> 7);
        pQueryIn->m_constraintRuntimeInMainMemory.m_storage = pInternal->m_runtime;
        hkpConstraintSchemaInfo& rInfo = constraintStatus.expandOne();
        rInfo.m_constraint = pInstance;
        rInfo.m_schema = s.m_schemas[iSchemaSet].m_current;
        const float afDepthA = pEntityA->m_collidable.m_allowedPenetrationDepth;
        const float afDepthB = pEntityB->m_collidable.m_allowedPenetrationDepth;
        rInfo.m_allowedPenetrationDepth = afDepthA < afDepthB ? afDepthA : afDepthB;
        hkpConstraintQueryOut out;
        out.m_jacobianSchemas = s.m_schemas[iSchemaSet].m_current;
        hkpConstraintData* pData = pInstance->m_data;
        if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT) {
            hkpSimpleContactConstraintData* pContactData = static_cast<hkpSimpleContactConstraintData*>(pData);
            hkSimpleContactConstraintData_fireCallbacks(
                pContactData,
                pQueryIn,
                pContactData->m_atom,
                hkpContactPointEvent::TYPE_EXPAND_MANIFOLD
            );
        }
        if ((pInternal->m_callbackRequest & hkpConstraintAtom::CALLBACK_REQUEST_SETUP_CALLBACK) != 0) {
            pData->buildJacobianCallback(*pQueryIn);
        }
        hkpConstraintAtom* pAtoms = pInternal->m_atoms;
        const int32_t iAtomsSize = pInternal->m_atomsSize;
        if (pAtoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
            hkBool contact = {true};
            hkSimpleContactConstraintDataBuildJacobian(
                static_cast<hkpSimpleContactConstraintAtom*>(pAtoms),
                *pQueryIn,
                contact,
                out
            );
        }
        else {
            hkSolverBuildJacobianFromAtomsNotContact(pAtoms, iAtomsSize, *pQueryIn, out);
        }
        s.m_schemas[iSchemaSet].m_current = out.m_jacobianSchemas;
        s.m_elemTempCurrent += 4 * pInternal->m_numSolverElemTemps;
        pAccumulatorA->m_scratch0.m_quad = zero;
        pAccumulatorA->m_scratch123.m_col0.m_quad = zero;
        pAccumulatorB->m_scratch0.m_quad = zero;
        pAccumulatorB->m_scratch123.m_col0.m_quad = zero;
    }
    hkpJacobianSchema* pResult = s.m_schemas[0].m_current;
    *reinterpret_cast<uint32_t*>(pResult) = 0;
    *reinterpret_cast<uint32_t*>(s.m_schemas[1].m_current) = 0;
    return pResult;
}

// Lays the solver buffer out in preallocatedBuffer, else in a buffer from the solver allocator: the fixed accumulator
// and one per entity, the schemas with their end schema, then the element temps. What does not fit moves to a temp
// block: the whole layout when not even the schemas start inside the buffer; the element temps alone, onto the thread's
// stack, when only they overflow; else the schemas past the buffer's end (behind a goto schema written when the first
// range is full) together with the element temps. It then builds the accumulators and the Jacobians (the constraints of
// priority TOI_HIGHER and above after all others), solves, and on success exports the impulses, integrates, recomputes
// the aabbs and reports the breached impulse limits; returns hkRigidMotionUtilApplyAccumulators' result, 0 when the
// solve fails. The buffers are raw memory carved by address with 32-bit sizes and filled with schemas of many layouts,
// hence the address arithmetic and casts. The stack block for the element temps is freed through the temp allocator
// with tempBufferSize (still 0 there), and a whole-layout move sizes its block from the solver buffer's address, null
// for a preallocated buffer: both deliberately.
int32_t hkpConstraintSolverSetup::solve(
    const hkStepInfo& stepInfo,
    const hkpSolverInfo& solverInfo,
    hkpConstraintQueryIn& constraintQueryIn,
    hkpSimulationIsland& island,
    void* preallocatedBuffer,
    int32_t preallocatedBufferSize,
    hkpEntity* const* bodies,
    int32_t numBodies
) {
    // HK_NEXT_MULTIPLE_OF(16, the element temps and two more); the schemas and HK_SIZE_OF_JACOBIAN_END_SCHEMA; the
    // fixed accumulator, one per entity and 16 bytes of alignment.
    const int32_t elemTempSize = ((island.m_constraintInfo.m_numSolverElemTemps + 2) *
                                      static_cast<int32_t>(sizeof(hkpSolverElemTemp)) +
                                  15) &
        ~15;
    const int32_t schemaSize = island.m_constraintInfo.m_sizeOfSchemas + 16;
    const int32_t accumSize = (numBodies + 1) * static_cast<int32_t>(sizeof(hkpVelocityAccumulator)) + 16;
    int32_t solverBufferSize = elemTempSize + schemaSize + accumSize;
    int32_t result = 0;
    char* tempBuffer = nullptr;
    int32_t tempBufferSize = 0;
    char* secondSchemas = nullptr;
    char* solverBuffer;
    char* layoutBase;
    char* bufferEnd;
    if (preallocatedBuffer == nullptr) {
        solverBuffer = static_cast<char*>(hkMemoryRouter::getInstance().solver().bufAlloc(solverBufferSize));
        layoutBase = solverBuffer;
        bufferEnd = solverBuffer + solverBufferSize;
    }
    else {
        solverBuffer = nullptr;
        layoutBase = static_cast<char*>(preallocatedBuffer);
        bufferEnd = layoutBase + preallocatedBufferSize;
    }

    hkpVelocityAccumulator* accumulators;
    char* schemaBase;
    char* schemasLimit;
    char* elemTempBegin;
    for (;;) {
        // Raw buffer memory: the accumulators start it.
        accumulators = reinterpret_cast<hkpVelocityAccumulator*>(layoutBase);
        schemaBase = layoutBase + accumSize;
        elemTempBegin = schemaBase + schemaSize;
        schemasLimit = elemTempBegin;
        bool relayout = false;
        while (elemTempBegin + elemTempSize > bufferEnd) {
            if (schemaBase >= bufferEnd) {
                // Nothing past the accumulators fits: lay everything out again in a temp block.
                tempBufferSize = static_cast<int32_t>(
                    reinterpret_cast<uintptr_t>(elemTempBegin + elemTempSize) -
                    reinterpret_cast<uintptr_t>(solverBuffer)
                );
                tempBuffer = static_cast<char*>(hkMemoryRouter::getInstance().temp().blockAlloc(tempBufferSize));
                bufferEnd = tempBuffer + tempBufferSize;
                layoutBase = tempBuffer;
                relayout = true;
                break;
            }
            const int32_t maxSchemaSize = island.m_constraintInfo.m_maxSizeOfSchema + 16;
            if (schemasLimit < bufferEnd) {
                // Only the element temps overflow: they go on the thread's stack.
                tempBuffer = static_cast<char*>(
                    hkMemoryRouter::getInstance().m_stack.allocateStackRoundSizeTo128(elemTempSize, nullptr)
                );
                elemTempBegin = tempBuffer;
                bufferEnd = tempBuffer + elemTempSize;
                continue;
            }
            // The schemas run past the buffer: the ones that do not fit before room for one maximal schema, and the
            // element temps, go to a temp block.
            char* schemasFit = bufferEnd - maxSchemaSize;
            const int32_t spare = static_cast<int32_t>(schemasFit - schemaBase);
            const int32_t movedSize = island.m_constraintInfo.m_sizeOfSchemas - (spare <= 0 ? 0 : spare) +
                maxSchemaSize;
            tempBufferSize = movedSize + elemTempSize;
            tempBuffer = static_cast<char*>(hkMemoryRouter::getInstance().temp().blockAlloc(tempBufferSize));
            bufferEnd = tempBuffer + tempBufferSize;
            if (spare < 0) {
                // Not even one schema fits: all of them go to the temp block.
                schemaBase = tempBuffer;
                secondSchemas = nullptr;
                elemTempBegin = tempBuffer + island.m_constraintInfo.m_sizeOfSchemas + 16;
                schemasLimit = elemTempBegin;
                continue;
            }
            secondSchemas = tempBuffer;
            schemasLimit = schemasFit;
            elemTempBegin = tempBuffer + movedSize;
        }
        if (!relayout) {
            break;
        }
    }

    hkpVelocityAccumulator* fixedAcc = accumulators;
    fixedAcc->setFixed();
    // Each entity's m_solverData is its accumulator's byte offset from the fixed one; stored only when it changes.
    uint32_t uiSolverData = sizeof(hkpVelocityAccumulator);
    for (int32_t i = 0; i < numBodies; ++i, uiSolverData += sizeof(hkpVelocityAccumulator)) {
        hkpEntity* pEntity = bodies[i];
        if (uiSolverData != pEntity->m_solverData) {
            pEntity->m_solverData = uiSolverData;
        }
    }
    hkpVelocityAccumulator* firstAcc = accumulators + 1;
    // Havok's motionsOffset convention: the entity pointers stand for their motions at +0xE0.
    hkRigidMotionUtilApplyForcesAndBuildAccumulators(
        stepInfo,
        reinterpret_cast<hkpMotion* const*>(bodies),
        numBodies,
        offsetof(hkpEntity, m_motion),
        firstAcc
    )
        ->m_type = hkpVelocityAccumulator::HK_END;
    // Raw buffer memory: the element temps follow the schemas.
    hkpSolverElemTemp* elemTemp = reinterpret_cast<hkpSolverElemTemp*>(elemTempBegin);
    for (int32_t i = 0; i < island.m_constraintInfo.m_numSolverElemTemps; ++i) {
        elemTemp[i].m_impulseApplied = 0.0f;
    }

    hkpConstraintQueryOut out;
    // Raw buffer memory: the schema stream, schemas of many layouts behind one another.
    out.m_jacobianSchemas = reinterpret_cast<hkpJacobianSchema*>(schemaBase);
    // Past this the schemas continue in the second range; all ones once the goto is written.
    uintptr_t uiSchemasLimit = reinterpret_cast<uintptr_t>(schemasLimit);
    // The constraints of priority TOI_HIGHER and above, built after all others.
    hkInplaceArray<hkConstraintInternal*, 256> deferred;
    hkpEntity* const* entitiesEnd = bodies + numBodies;
    for (hkpEntity* const* ppEntity = bodies; ppEntity < entitiesEnd; ++ppEntity) {
        hkSmallArray<hkConstraintInternal>& masters = (*ppEntity)->m_constraintsMaster;
        hkConstraintInternal* pEnd = masters.m_data + masters.m_size;
        for (hkConstraintInternal* ci = masters.m_data; ci < pEnd; ++ci) {
            if ((ci->m_callbackRequest &
                 (hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
                  hkpConstraintAtom::CALLBACK_REQUEST_SETUP_CALLBACK |
                  hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK)) != 0) {
                constraintQueryIn.m_constraintInstance = ci->m_constraint;
                constraintQueryIn.m_constraintRuntime.m_storage = ci->m_runtime;
                hkpEntity* pEntityA = ci->m_constraint->m_entities[0];
                hkpEntity* pEntityB = ci->m_constraint->m_entities[1];
                // The accumulator buffer is raw memory: the entities' solver data are byte offsets from its start.
                constraintQueryIn.m_bodyA = reinterpret_cast<hkpVelocityAccumulator*>(
                    reinterpret_cast<uintptr_t>(accumulators) + pEntityA->m_solverData
                );
                constraintQueryIn.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(
                    reinterpret_cast<uintptr_t>(accumulators) + pEntityB->m_solverData
                );
                constraintQueryIn.m_transformA = pEntityA->m_collidable.m_motion.pMotion;
                constraintQueryIn.m_transformB = pEntityB->m_collidable.m_motion.pMotion;
                if ((ci->m_callbackRequest &
                     (hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
                      hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK)) != 0) {
                    hkpSimpleContactConstraintData* pContactData = static_cast<hkpSimpleContactConstraintData*>(
                        ci->m_constraint->m_data
                    );
                    hkSimpleContactConstraintData_fireCallbacks(
                        pContactData,
                        &constraintQueryIn,
                        pContactData->m_atom,
                        hkpContactPointEvent::TYPE_MANIFOLD
                    );
                }
                if ((ci->m_callbackRequest & hkpConstraintAtom::CALLBACK_REQUEST_SETUP_CALLBACK) != 0) {
                    ci->m_constraint->m_data->buildJacobianCallback(constraintQueryIn);
                }
            }
            if (ci->m_priority >= hkpConstraintInstance::PRIORITY_TOI_HIGHER) {
                deferred.pushBack(ci);
                continue;
            }
            if (reinterpret_cast<uintptr_t>(out.m_jacobianSchemas) >= uiSchemasLimit) {
                // The first range is full: a goto schema to the second range (the schema stream is raw memory, each
                // schema its own layout).
                hkpJacobianSchema* pGoto = out.m_jacobianSchemas;
                reinterpret_cast<hkpJacobianGotoSchema*>(pGoto)->initOffset(
                    static_cast<int32_t>(secondSchemas - reinterpret_cast<char*>(pGoto))
                );
                uiSchemasLimit = ~static_cast<uintptr_t>(0);
                out.m_jacobianSchemas = reinterpret_cast<hkpJacobianSchema*>(secondSchemas);
            }
            // The SDK's forcePrefetch<256> of the atoms, of the next 256 bytes when there are more and of the 256
            // after those when there are more still.
            const char* pAtomBytes = reinterpret_cast<const char*>(ci->m_atoms);
            const uint16_t atomsSize = ci->m_atomsSize;
            for (int32_t iLine = 0; iLine < 256; iLine += 64) {
                _mm_prefetch(pAtomBytes + iLine, _MM_HINT_NTA);
            }
            if (atomsSize > 256) {
                for (int32_t iLine = 256; iLine < 512; iLine += 64) {
                    _mm_prefetch(pAtomBytes + iLine, _MM_HINT_NTA);
                }
                if (atomsSize > 512) {
                    for (int32_t iLine = 512; iLine < 768; iLine += 64) {
                        _mm_prefetch(pAtomBytes + iLine, _MM_HINT_NTA);
                    }
                }
            }
            hkpEntity* pEntityA = ci->m_entities[0];
            hkpEntity* pEntityB = ci->m_entities[1];
            constraintQueryIn.m_bodyA = reinterpret_cast<hkpVelocityAccumulator*>(
                reinterpret_cast<uintptr_t>(accumulators) + pEntityA->m_solverData
            );
            constraintQueryIn.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(
                reinterpret_cast<uintptr_t>(accumulators) + pEntityB->m_solverData
            );
            constraintQueryIn.m_transformA = &pEntityA->m_motion.m_motionState.m_transform;
            constraintQueryIn.m_transformB = &pEntityB->m_motion.m_motionState.m_transform;
            constraintQueryIn.m_constraintInstance = ci->m_constraint;
            constraintQueryIn.m_constraintRuntime.m_storage = ci->m_runtime;
            constraintQueryIn.m_accumulatorAIndex.m_storage = static_cast<uint16_t>(pEntityA->m_solverData >> 7);
            constraintQueryIn.m_accumulatorBIndex.m_storage = static_cast<uint16_t>(pEntityB->m_solverData >> 7);
            constraintQueryIn.m_constraintRuntimeInMainMemory.m_storage = constraintQueryIn.m_constraintRuntime
                                                                              .m_storage;
            hkpConstraintAtom* pAtoms = ci->m_atoms;
            if (pAtoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
                hkBool writeHeaderSchema = {true};
                hkSimpleContactConstraintDataBuildJacobian(
                    static_cast<hkpSimpleContactConstraintAtom*>(pAtoms),
                    constraintQueryIn,
                    writeHeaderSchema,
                    out
                );
            }
            else {
                hkSolverBuildJacobianFromAtomsNotContact(pAtoms, ci->m_atomsSize, constraintQueryIn, out);
            }
        }
    }
    for (int32_t i = 0; i < deferred.m_size; ++i) {
        hkConstraintInternal* ci = deferred.m_data[i];
        if (reinterpret_cast<uintptr_t>(out.m_jacobianSchemas) >= uiSchemasLimit) {
            // The goto schema, as above.
            hkpJacobianSchema* pGoto = out.m_jacobianSchemas;
            reinterpret_cast<hkpJacobianGotoSchema*>(pGoto)->initOffset(
                static_cast<int32_t>(secondSchemas - reinterpret_cast<char*>(pGoto))
            );
            out.m_jacobianSchemas = reinterpret_cast<hkpJacobianSchema*>(secondSchemas);
            uiSchemasLimit = ~static_cast<uintptr_t>(0);
        }
        // The SDK's forcePrefetch<256>, as above.
        const char* pAtomBytes = reinterpret_cast<const char*>(ci->m_atoms);
        const uint16_t atomsSize = ci->m_atomsSize;
        for (int32_t iLine = 0; iLine < 256; iLine += 64) {
            _mm_prefetch(pAtomBytes + iLine, _MM_HINT_NTA);
        }
        if (atomsSize > 256) {
            for (int32_t iLine = 256; iLine < 512; iLine += 64) {
                _mm_prefetch(pAtomBytes + iLine, _MM_HINT_NTA);
            }
            if (atomsSize > 512) {
                for (int32_t iLine = 512; iLine < 768; iLine += 64) {
                    _mm_prefetch(pAtomBytes + iLine, _MM_HINT_NTA);
                }
            }
        }
        hkpEntity* pEntityA = ci->m_entities[0];
        hkpEntity* pEntityB = ci->m_entities[1];
        constraintQueryIn.m_bodyA = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<uintptr_t>(accumulators) + pEntityA->m_solverData
        );
        constraintQueryIn.m_bodyB = reinterpret_cast<hkpVelocityAccumulator*>(
            reinterpret_cast<uintptr_t>(accumulators) + pEntityB->m_solverData
        );
        constraintQueryIn.m_transformA = &pEntityA->m_motion.m_motionState.m_transform;
        constraintQueryIn.m_transformB = &pEntityB->m_motion.m_motionState.m_transform;
        constraintQueryIn.m_constraintInstance = ci->m_constraint;
        constraintQueryIn.m_constraintRuntime.m_storage = ci->m_runtime;
        constraintQueryIn.m_accumulatorAIndex.m_storage = static_cast<uint16_t>(pEntityA->m_solverData >> 7);
        constraintQueryIn.m_accumulatorBIndex.m_storage = static_cast<uint16_t>(pEntityB->m_solverData >> 7);
        constraintQueryIn.m_constraintRuntimeInMainMemory.m_storage = ci->m_runtime;
        hkpConstraintAtom* pAtoms = ci->m_atoms;
        if (pAtoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
            hkBool writeHeaderSchema = {true};
            hkSimpleContactConstraintDataBuildJacobian(
                static_cast<hkpSimpleContactConstraintAtom*>(pAtoms),
                constraintQueryIn,
                writeHeaderSchema,
                out
            );
        }
        else {
            hkSolverBuildJacobianFromAtomsNotContact(pAtoms, ci->m_atomsSize, constraintQueryIn, out);
        }
    }
    // The end schema: a zero first 32-bit word (SCHEMA_TYPE_END) in the raw schema stream.
    *reinterpret_cast<uint32_t*>(out.m_jacobianSchemas) = 0;

    hkpJacobianSchema* schemas = reinterpret_cast<hkpJacobianSchema*>(schemaBase);
    const int32_t solveResult = hkSolveConstraints(solverInfo, schemas, accumulators, elemTemp);
    HK_MONITOR_ADD_VALUE(
        "NumJacobians",
        static_cast<float>(island.m_constraintInfo.m_numSolverResults),
        HK_MONITOR_TYPE_INT
    );
    HK_MONITOR_ADD_VALUE("NumEntities", static_cast<float>(numBodies), HK_MONITOR_TYPE_INT);
    if (solveResult == 1) {
        hkExportImpulsesAndRhs(solverInfo, elemTemp, schemas, accumulators);
        result = hkRigidMotionUtilApplyAccumulators(
            solverInfo,
            stepInfo,
            firstAcc,
            reinterpret_cast<hkpMotion* const*>(bodies),
            numBodies,
            offsetof(hkpEntity, m_motion)
        );
        hkpEntityAabbUtil::entityBatchRecalcAabb(bodies[0]->m_world->m_collisionInput, bodies, numBodies);
        // The solver leaves the breached impulse limits at the start of the raw schema buffer: a header, then the
        // elements, which the SDK hands to the listeners as their infos.
        hkpImpulseLimitBreachedHeader* pBreached = reinterpret_cast<hkpImpulseLimitBreachedHeader*>(schemaBase);
        const int32_t numBreached = pBreached->m_numBreached;
        if (numBreached != 0) {
            hkpWorldCallbackUtil::fireContactImpulseLimitBreached(
                island.getWorld(),
                reinterpret_cast<const hkpContactImpulseLimitBreachedListenerInfo*>(&pBreached->getElem(0)),
                numBreached
            );
        }
    }
    if (tempBuffer != nullptr) {
        hkMemoryRouter::getInstance().temp().blockFree(tempBuffer, tempBufferSize);
    }
    if (solverBuffer != nullptr) {
        hkMemoryRouter::getInstance().solver().bufFree(solverBuffer, solverBufferSize);
    }
    return result;
}

// Integrates the entities one at a time: each from a copy of its accumulator whose sums (scratch0, scratch123 column 0)
// are its velocities, with the motion's deactivation counter set to 0xFF meanwhile, then recomputes its aabb.
void hkpConstraintSolverSetup::oneStepIntegrate(
    const hkpSolverInfo& si,
    const hkStepInfo& info,
    const hkpVelocityAccumulator* accumulatorsBase,
    hkpEntity* const* entities,
    int32_t numEntities
) {
    for (int32_t i = 0; i < numEntities; ++i) {
        hkpEntity* entity = entities[i];
        hkpMotion* motion = entity->getMotion();
        // The accumulator buffer is raw memory: the entity's solver data is a byte offset from its start.
        const hkpVelocityAccumulator* src = hkAddByteOffsetConst(accumulatorsBase, entity->m_solverData);
        hkpVelocityAccumulator accu;
        accu.m_type.m_storage = src->m_type.m_storage;
        accu.m_deactivationClass = src->m_deactivationClass;
        accu.m_gravityFactor = src->m_gravityFactor;
        accu.m_linearVel.m_quad = src->m_linearVel.m_quad;
        accu.m_angularVel.m_quad = src->m_angularVel.m_quad;
        accu.m_invMasses.m_quad = src->m_invMasses.m_quad;
        accu.m_scratch0.m_quad = src->m_scratch0.m_quad;
        accu.m_scratch123.m_col0.m_quad = src->m_scratch123.m_col0.m_quad;
        accu.m_scratch123.m_col1.m_quad = src->m_scratch123.m_col1.m_quad;
        accu.m_scratch123.m_col2.m_quad = src->m_scratch123.m_col2.m_quad;
        accu.m_scratch0.m_quad = accu.m_linearVel.m_quad;
        accu.m_scratch123.m_col0.m_quad = accu.m_angularVel.m_quad;
        const uint8_t deactivationCounter = motion->m_deactivationIntegrateCounter;
        motion->m_deactivationIntegrateCounter = 0xFF;
        hkRigidMotionUtilApplyAccumulators(si, info, &accu, &motion, 1, 0);
        hkpEntityAabbUtil::entityBatchRecalcAabb(entity->m_world->m_collisionInput, &entity, 1);
        motion->m_deactivationIntegrateCounter = deactivationCounter;
    }
}

// Lays the island's solver buffer out in taskHeader: the accumulators (with the fixed one and the end marker), the
// schemas (with the end schemas of the multithreaded build's extra tasks, 144 constraints a task), the element temps
// rounded up to 16 bytes, and the total size. With a buffer, it writes the fixed accumulator and the end marker. The
// buffer is raw memory carved by address, hence the uintptr_t arithmetic (bufferIn may be null).
int32_t hkpConstraintSolverSetup::calcBufferOffsetsForSolve(
    const hkpSimulationIsland& island,
    char* bufferIn,
    int32_t bufferSize,
    hkpBuildJacobianTaskHeader& taskHeader
) {
    taskHeader.m_buffer = bufferIn;
    const int32_t numEntities = island.getEntities().m_size;
    taskHeader.m_accumulatorsBase = reinterpret_cast<hkpVelocityAccumulator*>(bufferIn);
    // HK_SIZE_OF_JACOBIAN_END_SCHEMA.
    const int32_t iEndSchemaSize = 16;
    const int32_t sizeForAccumulators = island.getEntities().m_size *
            static_cast<int32_t>(sizeof(hkpVelocityAccumulator)) +
        static_cast<int32_t>(sizeof(hkpVelocityAccumulator)) + 16;
    const uintptr_t schemas = reinterpret_cast<uintptr_t>(bufferIn) + sizeForAccumulators;
    // A build task holds up to hkpBuildJacobianTask::MAX_NUM_ATOM_INFOS constraints (144 for 4-byte pointers, 80 for
    // 8-byte pointers), each task with its own end schema and element temps.
    const int32_t minNumConstraintsPerTask = hkpBuildJacobianTask::MAX_NUM_ATOM_INFOS;
    int32_t extraTasksDueToMt = (island.m_numConstraints / minNumConstraintsPerTask) + 1;
    extraTasksDueToMt += 3;
    extraTasksDueToMt *= 2;
    taskHeader.m_schemasBase = reinterpret_cast<hkpJacobianSchema*>(schemas);
    const int32_t sizeForElemTemp = island.m_constraintInfo.m_numSolverElemTemps *
            static_cast<int32_t>(sizeof(hkpSolverElemTemp)) +
        2 * static_cast<int32_t>(sizeof(hkpSolverElemTemp)) + extraTasksDueToMt * 12;
    const int32_t sizeForSchemas = island.m_constraintInfo.m_sizeOfSchemas + iEndSchemaSize * extraTasksDueToMt +
        iEndSchemaSize;
    const uintptr_t elemTemp = schemas + sizeForSchemas;
    // Reads the buffer back before the element temps' store.
    void* const buffer = taskHeader.m_buffer;
    taskHeader.m_solverTempBase = reinterpret_cast<hkpSolverElemTemp*>(elemTemp);
    // HK_NEXT_MULTIPLE_OF(16, sizeForElemTemp).
    const int32_t totalSize = static_cast<int32_t>(
        elemTemp + ((sizeForElemTemp + 15) & ~15) - reinterpret_cast<uintptr_t>(buffer)
    );
    taskHeader.m_bufferSize = totalSize;
    if (buffer != nullptr) {
        hkpVelocityAccumulator* fixedAcc = taskHeader.m_accumulatorsBase;
        fixedAcc->setFixed();
        fixedAcc[numEntities + 1].m_type = hkpVelocityAccumulator::HK_END;
    }
    return totalSize;
}

int32_t hkpConstraintSolverSetup::calcBufferSize(hkpSimulationIsland& island) {
    alignas(16) hkpBuildJacobianTaskHeader taskHeader;
    const int32_t size = calcBufferOffsetsForSolve(island, nullptr, 0, taskHeader);
    return size;
}

void hkpConstraintSolverSetup::subSolve(hkpConstraintSolverResources& s, hkpConstraintSolverSetup::SolverMode mode) {
    if (mode == hkpConstraintSolverSetup::SOLVER_MODE_PROCESS_ALL) {
        hkpJacobianSchema* m_begin = s.m_schemas[1].m_begin;
        s.m_schemas[0].m_lastProcessed = s.m_schemas[0].m_begin;
        s.m_schemas[1].m_lastProcessed = m_begin;
    }
    for (hkpSolverElemTemp* i = s.m_elemTempLastProcessed; i < s.m_elemTempCurrent; ++i) {
        i->m_deltaRhs = 0.0f;
    }
    hkpJacobianSchema* m_lastProcessed = s.m_schemas[0].m_lastProcessed;
    s.m_elemTempLastProcessed = s.m_elemTempCurrent;
    if (m_lastProcessed != s.m_schemas[0].m_current) {
        hkSolveStepJacobians(*s.m_solverInfo, m_lastProcessed, s.m_accumulators, s.m_elemTemp);
    }
    hkpJacobianSchema* schemas = s.m_schemas[1].m_lastProcessed;
    if (schemas != s.m_schemas[1].m_current) {
        hkSolveStepJacobians(*s.m_solverInfo, schemas, s.m_accumulators, s.m_elemTemp);
    }
    hkpJacobianSchema* m_current = s.m_schemas[1].m_current;
    s.m_schemas[0].m_lastProcessed = s.m_schemas[0].m_current;
    s.m_schemas[1].m_lastProcessed = m_current;
}

// Adds up the schema bytes (per range: the second for forced-TOI constraints) and the element temps the constraints
// need and tells whether they still fit the resources' buffer, each schema range keeping room for its end schema (the
// zero word internalAddJacobianSchemas writes).
hkBool hkpConstraintSolverSetup::internalIsMemoryOkForNewJacobianSchemas(
    hkpConstraintSolverResources& s,
    hkpConstraintInstance** constraints,
    int32_t numConstraints
) {
    // The schema ranges are raw memory: the sizes are in bytes.
    uint8_t* schemas[2] = {
        reinterpret_cast<uint8_t*>(s.m_schemas[0].m_current),
        reinterpret_cast<uint8_t*>(s.m_schemas[1].m_current)
    };
    hkpSolverElemTemp* elemTemp = s.m_elemTempCurrent;
    // The loop walks a pointer to an end computed with wrapping uintptr_t arithmetic, so that clang does not fold the
    // entry test into a sign test of numConstraints.
    hkpConstraintInstance** const end = reinterpret_cast<hkpConstraintInstance**>(
        reinterpret_cast<uintptr_t>(constraints) + static_cast<uintptr_t>(numConstraints) * sizeof(*constraints)
    );
    auto addConstraint = [&schemas, &elemTemp](hkpConstraintInstance* pInstance) {
        hkConstraintInternal* pInternal = pInstance->getInternal();
        const int32_t iSchemaSet = pInstance->m_priority >= hkpConstraintInstance::PRIORITY_TOI_FORCED ? 1 : 0;
        schemas[iSchemaSet] += pInternal->m_sizeOfSchemas;
        elemTemp += pInternal->m_numSolverElemTemps;
    };
    // The loop is unrolled by two so that every count, a garbage one included, takes the same passes as the game: a
    // signed trip count of the byte span divided by the element size, rounded up, then pairs while the cursor is below
    // the end's last element in a signed compare, then one pass while it is below the end in an unsigned compare. The
    // trip count divides by the pointer size, not the exe's 4: a span of one element would otherwise count two and read
    // past the array.
    hkpConstraintInstance** c = constraints;
    const uint32_t uiSpan = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end) - reinterpret_cast<uintptr_t>(c));
    constexpr uint32_t uiElem = sizeof(*c);
    if (static_cast<int32_t>(uiSpan + (uiElem - 1u)) / static_cast<int32_t>(uiElem) >= 2) {
        const intptr_t iPairEnd = static_cast<intptr_t>(reinterpret_cast<uintptr_t>(end) - sizeof(*end));
        do {
            addConstraint(c[0]);
            addConstraint(c[1]);
            c += 2;
        }
        while (reinterpret_cast<intptr_t>(c) < iPairEnd);
    }
    if (c < end) {
        addConstraint(*c);
    }
    const int32_t iEndSchemaSize = 4;
    if (elemTemp > s.m_elemTempEnd ||
        schemas[0] + iEndSchemaSize > reinterpret_cast<uint8_t*>(s.m_schemas[0].m_end) ||
        schemas[1] + iEndSchemaSize > reinterpret_cast<uint8_t*>(s.m_schemas[1].m_end)) {
        return 0;
    }
    return 1;
}

// The accumulator buffer's free bytes, less 16, against 128 bytes per accumulator; the compare is signed. The buffer is
// raw memory carved by byte (see solve), so its free space is a byte difference.
hkBool hkpConstraintSolverSetup::internalIsMemoryOkForNewAccumulators(
    hkpConstraintSolverResources& s,
    hkpEntity** entities,
    int32_t numEntities
) {
    int32_t numAccumulators = numEntities;
    if (s.m_accumulatorsCurrent == s.m_accumulators) {
        numAccumulators++;
    }
    const int32_t bytesLeft = static_cast<int32_t>(
        reinterpret_cast<char*>(s.m_accumulatorsEnd) - reinterpret_cast<char*>(s.m_accumulatorsCurrent)
    );
    return numAccumulators * static_cast<int32_t>(sizeof(hkpVelocityAccumulator)) <= bytesLeft - 16;
}

void hkpConstraintSolverSetup::shutdownSolver(hkpConstraintSolverResources& s) {}

void uploadOpenJobsVariable(
    const hkpBuildJacobianTaskHeader* localTaskHeader,
    const hkpBuildJacobianTaskHeader* taskHeaderInMainMemory
) {
    ;
}
