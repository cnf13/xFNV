#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Collide/Filter/Constraint/hkpConstraintCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpBodyOperation.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// Orders island merges by the storage index of the first entity's island, then of the second's (unsigned).
static hkBool islandLess(const hkWorldOperation::BiggestOperation& a, const hkWorldOperation::BiggestOperation& b) {
    // The pending island merges are kept as BiggestOperation storage holding MergeIslands operations.
    const hkWorldOperation::MergeIslands& mergeA = reinterpret_cast<const hkWorldOperation::MergeIslands&>(a);
    const hkWorldOperation::MergeIslands& mergeB = reinterpret_cast<const hkWorldOperation::MergeIslands&>(b);
    const uint32_t uiA = mergeA.m_entities[0]->m_simulationIsland->m_storageIndex;
    const uint32_t uiB = mergeB.m_entities[0]->m_simulationIsland->m_storageIndex;
    if (uiA < uiB ||
        (uiA == uiB &&
         mergeA.m_entities[1]->m_simulationIsland->m_storageIndex <
             mergeB.m_entities[1]->m_simulationIsland->m_storageIndex)) {
        return 1;
    }
    return 0;
}

int32_t hkAlgorithm::quickSortRecursive(
    hkWorldOperation::BiggestOperation* apArr,
    int32_t aiLow,
    int32_t aiHigh,
    hkBool (*cmpLess)(const hkWorldOperation::BiggestOperation&, const hkWorldOperation::BiggestOperation&)
) {
    for (;;) {
        int32_t i = aiLow;
        int32_t j = aiHigh;
        alignas(16) const hkWorldOperation::BiggestOperation str = apArr[(aiLow + aiHigh) >> 1];
        do {
            while (cmpLess(apArr[i], str).m_bool) {
                i++;
            }
            while (cmpLess(str, apArr[j]).m_bool) {
                j--;
            }
            if (i > j) {
                break;
            }
            if (i != j) {
                hkWorldOperation::BiggestOperation tmp = apArr[j];
                apArr[j] = apArr[i];
                apArr[i] = tmp;
            }
            i++;
            j--;
        }
        while (i <= j);
        int32_t iResult = aiLow;
        if (aiLow < j) {
            iResult = quickSortRecursive(apArr, aiLow, j, cmpLess);
        }
        if (i >= aiHigh) {
            return iResult;
        }
        aiLow = i;
    }
}

// Orders body operations by their entity's uid, and the operations on one entity by descending priority; both compares
// are unsigned.
bool hkpWorldBodyOperationQueueSortCmp(const hkpBodyOperationEntry& a, const hkpBodyOperationEntry& b) {
    if (a.m_entity == b.m_entity) {
        return b.m_priority < a.m_priority;
    }
    return a.m_entity->getUid() < b.m_entity->getUid();
}

hkpWorldOperationQueue::hkpWorldOperationQueue(hkpWorld* world) : m_world(world) {}

// The world counts the deferred operation, the entry takes the four words in order, and the operation keeps two
// references until the operation has run.
void hkpWorldOperationQueue::queueBodyOperation(
    hkpEntity* entity,
    hkpBodyOperation* operation,
    int32_t priority,
    hkpBodyOperation::ExecutionState hint
) {
    hkReferencedObject::lockAll();
    ++m_world->m_pendingBodyOperationsCount;
    hkpBodyOperationEntry& rEntry = m_pendingBodyOperations.expandOne();
    rEntry.m_entity = entity;
    rEntry.m_operation = operation;
    rEntry.m_priority = priority;
    rEntry.m_hint = hint;
    operation->addReference();
    operation->addReference();
    hkReferencedObject::unlockAll();
}

// Takes the queued body operations over into a local array, sorts them, runs each single-threaded, then drops the
// two references each entry holds, and repeats while running them queued more.
void hkpWorldOperationQueue::executeAllPendingBodyOperations() {
    while (m_pendingBodyOperations.m_size != 0) {
        hkArray<hkpBodyOperationEntry> kOperations;
        kOperations.swap(m_pendingBodyOperations);
        m_world->m_pendingBodyOperationsCount = 0;
        m_pendingBodyOperations.reserve(16);
        const int32_t iCount = kOperations.m_size;
        if (iCount > 1) {
            hkAlgorithm::quickSortRecursive<hkpBodyOperationEntry, hkpBodyOperationEntryLess>(
                kOperations.m_data,
                0,
                iCount - 1,
                &hkpWorldBodyOperationQueueSortCmp
            );
        }
        for (int32_t i = 0; i < iCount; ++i) {
            hkpBodyOperationEntry& rEntry = kOperations.m_data[i];
            hkpBodyOperation::UpdateInfo kInfo;
            kInfo.m_bodyIsDeleted = false;
            rEntry.m_operation->execute(static_cast<hkpRigidBody*>(rEntry.m_entity), kInfo);
        }
        hkReferencedObject::lockAll();
        for (int32_t i = 0; i < iCount; ++i) {
            hkpBodyOperationEntry& rEntry = kOperations.m_data[i];
            hkpBodyOperation* pOperation = rEntry.m_operation;
            rEntry.m_entity->removeReferenceLockUnchecked();
            pOperation->removeReferenceLockUnchecked();
        }
        hkReferencedObject::unlockAll();
    }
}

// Runs the operations queued while the world was locked: takes them over into a local array, merges the pending
// islands (sorted), then runs each operation in order, dropping the references the queue held and freeing the
// blocks it allocated, and runs what each operation queued in turn before the next one.
void hkpWorldOperationQueue::executeAllPending() {
    hkReferencedObject::lockAll();
    hkArray<hkWorldOperation::BiggestOperation> tmpOldPending;
    tmpOldPending.swap(m_pending);
    ++m_world->m_pendingOperationQueueCount;

    if (m_islandMerges.m_size > 1) {
        hkAlgorithm::quickSortRecursive(m_islandMerges.m_data, 0, m_islandMerges.m_size - 1, &islandLess);
    }
    while (m_islandMerges.m_size != 0) {
        hkWorldOperation::BiggestOperation kOperation = m_islandMerges.m_data[m_islandMerges.m_size - 1];
        m_islandMerges.popBack();
        // kOperation is raw, untyped storage: BiggestOperation bytes holding a MergeIslands operation.
        const hkWorldOperation::MergeIslands& rMerge = reinterpret_cast<const hkWorldOperation::MergeIslands&>(
            kOperation
        );
        hkpEntity* pEntityA = rMerge.m_entities[0];
        hkpEntity* pEntityB = rMerge.m_entities[1];
        hkpWorld* pWorld = m_world;
        if (pEntityA->m_world == pWorld &&
            pEntityB->m_world == pWorld &&
            pEntityA->m_motion.m_type != hkpMotion::MOTION_FIXED &&
            pEntityB->m_motion.m_type != hkpMotion::MOTION_FIXED &&
            pEntityA->m_simulationIsland != pEntityB->m_simulationIsland) {
            hkpWorldOperationUtil::mergeIslands(pEntityA->m_world, pEntityA, pEntityB);
        }
        pEntityA->removeReference();
        pEntityB->removeReference();
    }

    int32_t iRemaining = tmpOldPending.m_size;
    if (iRemaining > 0) {
        // Raw untyped storage: each entry is BiggestOperation bytes holding the operation its type byte names, which
        // each case reads through that operation's struct.
        hkWorldOperation::BiggestOperation* pOperation = tmpOldPending.m_data;
        do {
            switch (pOperation->m_type.m_storage) {
                case hkWorldOperation::ENTITY_ADD: {
                    const hkWorldOperation::AddEntity* pOp = reinterpret_cast<const hkWorldOperation::AddEntity*>(
                        pOperation
                    );
                    if (!pOp->m_entity->m_world) {
                        m_world->addEntity(pOp->m_entity, pOp->m_activation);
                    }
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::ENTITY_REMOVE: {
                    const hkWorldOperation::RemoveEntity* pOp = reinterpret_cast<const hkWorldOperation::RemoveEntity*>(
                        pOperation
                    );
                    hkpWorld* pWorld = m_world;
                    if (pOp->m_entity->m_world == pWorld) {
                        pWorld->removeEntity(pOp->m_entity);
                    }
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::ENTITY_UPDATE_BROAD_PHASE: {
                    const hkWorldOperation::UpdateEntityBP*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdateEntityBP*>(pOperation);
                    hkpWorld* pWorld = m_world;
                    if (pOp->m_entity->m_world == pWorld) {
                        hkpWorldOperationUtil::updateEntityBP(pWorld, pOp->m_entity);
                    }
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::RIGIDBODY_SET_MOTION_TYPE: {
                    const hkWorldOperation::SetRigidBodyMotionType*
                        pOp = reinterpret_cast<const hkWorldOperation::SetRigidBodyMotionType*>(pOperation);
                    pOp->m_rigidBody->setMotionType(
                        static_cast<hkpMotion::MotionType>(pOp->m_motionType),
                        static_cast<hkpEntityActivation>(pOp->m_activation),
                        static_cast<hkpUpdateCollisionFilterOnEntityMode>(pOp->m_collisionFilterUpdateMode)
                    );
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::WORLD_OBJECT_SET_SHAPE: {
                    const hkWorldOperation::SetWorldObjectShape*
                        pOp = reinterpret_cast<const hkWorldOperation::SetWorldObjectShape*>(pOperation);
                    pOp->m_worldObject->setShape(pOp->m_shape);
                    pOp->m_worldObject->removeReference();
                    pOp->m_shape->removeReference();
                    break;
                }
                case hkWorldOperation::ENTITY_BATCH_ADD: {
                    const hkWorldOperation::AddEntityBatch*
                        pOp = reinterpret_cast<const hkWorldOperation::AddEntityBatch*>(pOperation);
                    m_world->addEntityBatch(pOp->m_entities, pOp->m_numEntities, pOp->m_activation);
                    hkReferencedObject::removeReferences(pOp->m_entities, pOp->m_numEntities);
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_entities,
                        pOp->m_numEntities * static_cast<int32_t>(sizeof(hkpEntity*))
                    );
                    break;
                }
                case hkWorldOperation::ENTITY_BATCH_REMOVE: {
                    const hkWorldOperation::RemoveEntityBatch*
                        pOp = reinterpret_cast<const hkWorldOperation::RemoveEntityBatch*>(pOperation);
                    m_world->removeEntityBatch(pOp->m_entities, pOp->m_numEntities);
                    hkReferencedObject::removeReferences(pOp->m_entities, pOp->m_numEntities);
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_entities,
                        pOp->m_numEntities * static_cast<int32_t>(sizeof(hkpEntity*))
                    );
                    break;
                }
                case hkWorldOperation::CONSTRAINT_ADD: {
                    const hkWorldOperation::AddConstraint*
                        pOp = reinterpret_cast<const hkWorldOperation::AddConstraint*>(pOperation);
                    hkpConstraintInstance* pConstraint = pOp->m_constraint;
                    if (!pConstraint->m_owner) {
                        hkpWorld* pWorld = m_world;
                        if (pConstraint->m_entities[0]->m_world == pWorld &&
                            pConstraint->m_entities[1]->m_world == pWorld) {
                            pWorld->addConstraint(pConstraint);
                        }
                    }
                    pOp->m_constraint->removeReference();
                    break;
                }
                case hkWorldOperation::CONSTRAINT_REMOVE: {
                    const hkWorldOperation::RemoveConstraint*
                        pOp = reinterpret_cast<const hkWorldOperation::RemoveConstraint*>(pOperation);
                    if (pOp->m_constraint->m_owner) {
                        m_world->removeConstraint(pOp->m_constraint);
                    }
                    pOp->m_constraint->removeReference();
                    break;
                }
                case hkWorldOperation::ACTION_ADD: {
                    const hkWorldOperation::AddAction* pOp = reinterpret_cast<const hkWorldOperation::AddAction*>(
                        pOperation
                    );
                    hkpAction* pAction = pOp->m_action;
                    if (!pAction->m_world) {
                        // The action is added only while every entity it works on is still in this world.
                        hkArray<hkpEntity*> kEntities;
                        pAction->getEntities(kEntities);
                        bool bAllInWorld = true;
                        for (hkpEntity* pEntity : kEntities) {
                            if (pEntity->m_world != m_world) {
                                bAllInWorld = false;
                                break;
                            }
                        }
                        if (bAllInWorld) {
                            m_world->addAction(pOp->m_action);
                        }
                    }
                    pOp->m_action->removeReference();
                    break;
                }
                case hkWorldOperation::ACTION_REMOVE: {
                    const hkWorldOperation::RemoveAction* pOp = reinterpret_cast<const hkWorldOperation::RemoveAction*>(
                        pOperation
                    );
                    if (pOp->m_action->m_island) {
                        m_world->removeAction(pOp->m_action);
                    }
                    pOp->m_action->removeReference();
                    break;
                }
                case hkWorldOperation::PHANTOM_ADD: {
                    const hkWorldOperation::AddPhantom* pOp = reinterpret_cast<const hkWorldOperation::AddPhantom*>(
                        pOperation
                    );
                    if (!pOp->m_phantom->m_world) {
                        m_world->addPhantom(pOp->m_phantom);
                    }
                    pOp->m_phantom->removeReference();
                    break;
                }
                case hkWorldOperation::PHANTOM_REMOVE: {
                    const hkWorldOperation::RemovePhantom*
                        pOp = reinterpret_cast<const hkWorldOperation::RemovePhantom*>(pOperation);
                    hkpWorld* pWorld = m_world;
                    if (pOp->m_phantom->m_world == pWorld) {
                        pWorld->removePhantom(pOp->m_phantom);
                    }
                    pOp->m_phantom->removeReference();
                    break;
                }
                case hkWorldOperation::PHANTOM_BATCH_ADD: {
                    const hkWorldOperation::AddPhantomBatch*
                        pOp = reinterpret_cast<const hkWorldOperation::AddPhantomBatch*>(pOperation);
                    m_world->addPhantomBatch(pOp->m_phantoms, pOp->m_numPhantoms);
                    hkReferencedObject::removeReferences(pOp->m_phantoms, pOp->m_numPhantoms);
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_phantoms,
                        pOp->m_numPhantoms * static_cast<int32_t>(sizeof(hkpPhantom*))
                    );
                    break;
                }
                case hkWorldOperation::PHANTOM_BATCH_REMOVE: {
                    const hkWorldOperation::RemovePhantomBatch*
                        pOp = reinterpret_cast<const hkWorldOperation::RemovePhantomBatch*>(pOperation);
                    m_world->removePhantomBatch(pOp->m_phantoms, pOp->m_numPhantoms);
                    hkReferencedObject::removeReferences(pOp->m_phantoms, pOp->m_numPhantoms);
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_phantoms,
                        pOp->m_numPhantoms * static_cast<int32_t>(sizeof(hkpPhantom*))
                    );
                    break;
                }
                case hkWorldOperation::PHANTOM_UPDATE_BROAD_PHASE: {
                    const hkWorldOperation::UpdatePhantomBP*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdatePhantomBP*>(pOperation);
                    if (pOp->m_phantom->m_world == m_world) {
                        pOp->m_phantom->updateBroadPhase(*pOp->m_aabb);
                    }
                    pOp->m_phantom->removeReference();
                    if (pOp->m_aabb) {
                        hkMemoryRouter::getInstance().heap().blockFree(
                            pOp->m_aabb,
                            static_cast<int32_t>(sizeof(hkAabb))
                        );
                    }
                    break;
                }
                case hkWorldOperation::UPDATE_FILTER_ENTITY: {
                    const hkWorldOperation::UpdateFilterOnEntity*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdateFilterOnEntity*>(pOperation);
                    hkpWorld* pWorld = m_world;
                    if (pOp->m_entity->m_world == pWorld) {
                        pWorld->updateCollisionFilterOnEntity(
                            pOp->m_entity,
                            pOp->m_collisionFilterUpdateMode,
                            pOp->m_updateShapeCollections
                        );
                    }
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::UPDATE_FILTER_ENTITY_PAIR: {
                    const hkWorldOperation::UpdateFilterOnEntityPair*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdateFilterOnEntityPair*>(pOperation);
                    hkpWorld* pWorld = m_world;
                    if (pOp->m_entityA->m_world == pWorld && pOp->m_entityB->m_world == pWorld) {
                        pWorld->reenableCollisionBetweenEntityPair(pOp->m_entityA, pOp->m_entityB);
                    }
                    pOp->m_entityA->removeReference();
                    pOp->m_entityB->removeReference();
                    break;
                }
                case hkWorldOperation::UPDATE_FILTER_PHANTOM: {
                    const hkWorldOperation::UpdateFilterOnPhantom*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdateFilterOnPhantom*>(pOperation);
                    hkpWorld* pWorld = m_world;
                    if (pOp->m_phantom->m_world == pWorld) {
                        pWorld->updateCollisionFilterOnPhantom(pOp->m_phantom, pOp->m_updateShapeCollections);
                    }
                    pOp->m_phantom->removeReference();
                    break;
                }
                case hkWorldOperation::UPDATE_FILTER_WORLD: {
                    const hkWorldOperation::UpdateFilterOnWorld*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdateFilterOnWorld*>(pOperation);
                    m_world->updateCollisionFilterOnWorld(
                        pOp->m_collisionFilterUpdateMode,
                        pOp->m_updateShapeCollections
                    );
                    break;
                }
                case hkWorldOperation::UPDATE_MOVED_BODY_INFO: {
                    const hkWorldOperation::UpdateMovedBodyInfo*
                        pOp = reinterpret_cast<const hkWorldOperation::UpdateMovedBodyInfo*>(pOperation);
                    hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody(pOp->m_entity);
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::ENTITY_BATCH_REINTEGRATE_AND_RECOLLIDE: {
                    const hkWorldOperation::ReintegrateAndRecollideEntityBatch*
                        pOp = reinterpret_cast<const hkWorldOperation::ReintegrateAndRecollideEntityBatch*>(pOperation);
                    // Moves the entities no longer in this world to the end of the batch, so that only the first
                    // iNumEntities are reintegrated.
                    int32_t iNumEntities = pOp->m_numEntities;
                    hkpEntity** ppLast = pOp->m_entities + iNumEntities - 1;
                    for (hkpEntity** ppEntity = ppLast; ppEntity >= pOp->m_entities; --ppEntity) {
                        hkpEntity* pEntity = *ppEntity;
                        if (pEntity->m_world != m_world) {
                            *ppEntity = *ppLast;
                            *ppLast = pEntity;
                            --ppLast;
                            --iNumEntities;
                        }
                    }
                    if (iNumEntities) {
                        m_world->m_simulation
                            ->reintegrateAndRecollideEntities(pOp->m_entities, iNumEntities, m_world, pOp->m_mode);
                    }
                    hkpEntity** ppEnd = pOp->m_entities + pOp->m_numEntities;
                    for (hkpEntity** ppEntity = pOp->m_entities; ppEntity < ppEnd; ++ppEntity) {
                        (*ppEntity)->removeReference();
                    }
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_entities,
                        pOp->m_numEntities * static_cast<int32_t>(sizeof(hkpEntity*))
                    );
                    break;
                }
                case hkWorldOperation::RIGIDBODY_SET_POSITION_AND_ROTATION: {
                    const hkWorldOperation::SetRigidBodyPositionAndRotation*
                        pOp = reinterpret_cast<const hkWorldOperation::SetRigidBodyPositionAndRotation*>(pOperation);
                    // Raw untyped heap block: the position, then the rotation's four components.
                    hkVector4* pPositionAndRotation = pOp->m_positionAndRotation;
                    pOp->m_rigidBody->setPositionAndRotation(
                        pPositionAndRotation[0],
                        reinterpret_cast<const hkQuaternion&>(pPositionAndRotation[1])
                    );
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_positionAndRotation,
                        static_cast<int32_t>(2 * sizeof(hkVector4))
                    );
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::RIGIDBODY_SET_LINEAR_VELOCITY: {
                    const hkWorldOperation::SetRigidBodyLinearVelocity*
                        pOp = reinterpret_cast<const hkWorldOperation::SetRigidBodyLinearVelocity*>(pOperation);
                    hkVector4 kVelocity;
                    kVelocity.set(pOp->m_linearVelocity[0], pOp->m_linearVelocity[1], pOp->m_linearVelocity[2], 0.0f);
                    pOp->m_rigidBody->setLinearVelocity(kVelocity);
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::RIGIDBODY_SET_ANGULAR_VELOCITY: {
                    const hkWorldOperation::SetRigidBodyAngularVelocity*
                        pOp = reinterpret_cast<const hkWorldOperation::SetRigidBodyAngularVelocity*>(pOperation);
                    hkVector4 kVelocity;
                    kVelocity
                        .set(pOp->m_angularVelocity[0], pOp->m_angularVelocity[1], pOp->m_angularVelocity[2], 0.0f);
                    pOp->m_rigidBody->setAngularVelocity(kVelocity);
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::RIGIDBODY_APPLY_LINEAR_IMPULSE: {
                    const hkWorldOperation::ApplyRigidBodyLinearImpulse*
                        pOp = reinterpret_cast<const hkWorldOperation::ApplyRigidBodyLinearImpulse*>(pOperation);
                    hkVector4 kImpulse;
                    kImpulse.set(pOp->m_linearImpulse[0], pOp->m_linearImpulse[1], pOp->m_linearImpulse[2], 0.0f);
                    pOp->m_rigidBody->applyLinearImpulse(kImpulse);
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::RIGIDBODY_APPLY_POINT_IMPULSE: {
                    const hkWorldOperation::ApplyRigidBodyPointImpulse*
                        pOp = reinterpret_cast<const hkWorldOperation::ApplyRigidBodyPointImpulse*>(pOperation);
                    // The block holds the point, then the impulse.
                    hkVector4* pPointAndImpulse = pOp->m_pointAndImpulse;
                    pOp->m_rigidBody->applyPointImpulse(pPointAndImpulse[1], pPointAndImpulse[0]);
                    hkMemoryRouter::getInstance().heap().blockFree(
                        pOp->m_pointAndImpulse,
                        static_cast<int32_t>(2 * sizeof(hkVector4))
                    );
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::RIGIDBODY_APPLY_ANGULAR_IMPULSE: {
                    const hkWorldOperation::ApplyRigidBodyAngularImpulse*
                        pOp = reinterpret_cast<const hkWorldOperation::ApplyRigidBodyAngularImpulse*>(pOperation);
                    hkVector4 kImpulse;
                    kImpulse.set(pOp->m_angularImpulse[0], pOp->m_angularImpulse[1], pOp->m_angularImpulse[2], 0.0f);
                    pOp->m_rigidBody->applyAngularImpulse(kImpulse);
                    pOp->m_rigidBody->removeReference();
                    break;
                }
                case hkWorldOperation::ACTIVATE_REGION: {
                    const hkWorldOperation::ActivateRegion*
                        pOp = reinterpret_cast<const hkWorldOperation::ActivateRegion*>(pOperation);
                    m_world->activateRegion(*pOp->m_aabb);
                    hkMemoryRouter::getInstance().heap().blockFree(pOp->m_aabb, static_cast<int32_t>(sizeof(hkAabb)));
                    break;
                }
                case hkWorldOperation::ACTIVATE_ENTITY: {
                    const hkWorldOperation::ActivateEntity*
                        pOp = reinterpret_cast<const hkWorldOperation::ActivateEntity*>(pOperation);
                    pOp->m_entity->activate();
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::DEACTIVATE_ENTITY: {
                    const hkWorldOperation::DeactivateEntity*
                        pOp = reinterpret_cast<const hkWorldOperation::DeactivateEntity*>(pOperation);
                    if (pOp->m_entity->m_world == m_world) {
                        pOp->m_entity->deactivate();
                    }
                    pOp->m_entity->removeReference();
                    break;
                }
                case hkWorldOperation::CONSTRAINT_FILTER_BROKEN_EVENT: {
                    const hkWorldOperation::ConstraintCollisionFilterConstraintBroken*
                        pOp = reinterpret_cast<const hkWorldOperation::ConstraintCollisionFilterConstraintBroken*>(
                            pOperation
                        );
                    pOp->m_filter->_constraintBreakingCallbackImmediate(
                        pOp->m_constraintInstance,
                        pOp->m_constraintBroken
                    );
                    pOp->m_filter->removeReference();
                    pOp->m_constraintInstance->removeReference();
                    break;
                }
                case hkWorldOperation::USER_CALLBACK: {
                    const hkWorldOperation::UserCallbackOperation*
                        pOp = reinterpret_cast<const hkWorldOperation::UserCallbackOperation*>(pOperation);
                    pOp->m_userCallback->worldOperationUserCallback(pOp->m_userData);
                    pOp->m_userCallback->removeReference();
                    break;
                }
                default:
                    // ISLAND_MERGE, WORLD_OBJECT_ADD_REFERENCE and WORLD_OBJECT_REMOVE_REFERENCE do nothing here.
                    break;
            }
            // What the operation queued runs before the next one.
            if (m_pending.m_size != 0) {
                executeAllPending();
            }
            ++pOperation;
        }
        while (--iRemaining != 0);
    }

    tmpOldPending.swap(m_pending);
    m_pending.clear();
    --m_world->m_pendingOperationQueueCount;
    hkReferencedObject::unlockAll();
}

// Copies the operation into the pending queue and takes the references it holds until it runs: a batch gets a heap
// copy of its object list and a phantom's broad-phase update a heap copy of its box. An island merge moves on to
// m_islandMerges; a reference change takes effect at once and leaves the queue.
void hkpWorldOperationQueue::queueOperation(const hkWorldOperation::BaseOperation& operation) {
    hkReferencedObject::lockAll();
    hkWorldOperation::BiggestOperation& rOp = m_pending.expandOne();
    ++m_world->m_pendingOperationsCount;
    // The queue keeps every operation in BiggestOperation storage: the copy takes all of it whatever the type.
    rOp = static_cast<const hkWorldOperation::BiggestOperation&>(operation);
    // Each case reads the stored copy as the operation its type byte names, a downcast from BaseOperation.
    hkWorldOperation::BaseOperation& rBase = rOp;

    switch (rOp.m_type.m_storage) {
        case hkWorldOperation::ENTITY_ADD:
            static_cast<hkWorldOperation::AddEntity&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::ENTITY_REMOVE:
            static_cast<hkWorldOperation::RemoveEntity&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::ENTITY_UPDATE_BROAD_PHASE:
            static_cast<hkWorldOperation::UpdateEntityBP&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::RIGIDBODY_SET_MOTION_TYPE:
            static_cast<hkWorldOperation::SetRigidBodyMotionType&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::WORLD_OBJECT_SET_SHAPE: {
            hkWorldOperation::SetWorldObjectShape& rSetShape = static_cast<hkWorldOperation::SetWorldObjectShape&>(
                rBase
            );
            rSetShape.m_worldObject->addReference();
            rSetShape.m_shape->addReference();
            break;
        }
        case hkWorldOperation::ENTITY_BATCH_ADD: {
            hkWorldOperation::AddEntityBatch& rBatch = static_cast<hkWorldOperation::AddEntityBatch&>(rBase);
            const hkWorldOperation::AddEntityBatch& rSrc = static_cast<const hkWorldOperation::AddEntityBatch&>(
                operation
            );
            rBatch.m_entities = hkMemoryRouter::getInstance().heap()._blockAlloc<hkpEntity*>(rSrc.m_numEntities);
            hkString::memCpy(
                rBatch.m_entities,
                rSrc.m_entities,
                rSrc.m_numEntities * static_cast<int32_t>(sizeof(hkpEntity*))
            );
            hkpEntity** ppEnd = rBatch.m_entities + rBatch.m_numEntities;
            for (hkpEntity** ppEntity = rBatch.m_entities; ppEntity < ppEnd; ++ppEntity) {
                (*ppEntity)->addReference();
            }
            break;
        }
        case hkWorldOperation::ENTITY_BATCH_REMOVE: {
            hkWorldOperation::RemoveEntityBatch& rBatch = static_cast<hkWorldOperation::RemoveEntityBatch&>(rBase);
            const hkWorldOperation::RemoveEntityBatch& rSrc = static_cast<const hkWorldOperation::RemoveEntityBatch&>(
                operation
            );
            rBatch.m_entities = hkMemoryRouter::getInstance().heap()._blockAlloc<hkpEntity*>(rSrc.m_numEntities);
            hkString::memCpy(
                rBatch.m_entities,
                rSrc.m_entities,
                rSrc.m_numEntities * static_cast<int32_t>(sizeof(hkpEntity*))
            );
            hkpEntity** ppEnd = rBatch.m_entities + rBatch.m_numEntities;
            for (hkpEntity** ppEntity = rBatch.m_entities; ppEntity < ppEnd; ++ppEntity) {
                (*ppEntity)->addReference();
            }
            break;
        }
        case hkWorldOperation::CONSTRAINT_ADD:
            static_cast<hkWorldOperation::AddConstraint&>(rBase).m_constraint->addReference();
            break;
        case hkWorldOperation::CONSTRAINT_REMOVE:
            static_cast<hkWorldOperation::RemoveConstraint&>(rBase).m_constraint->addReference();
            break;
        case hkWorldOperation::ACTION_ADD:
            static_cast<hkWorldOperation::AddAction&>(rBase).m_action->addReference();
            break;
        case hkWorldOperation::ACTION_REMOVE:
            static_cast<hkWorldOperation::RemoveAction&>(rBase).m_action->addReference();
            break;
        case hkWorldOperation::ISLAND_MERGE: {
            hkWorldOperation::MergeIslands& rMerge = static_cast<hkWorldOperation::MergeIslands&>(rBase);
            rMerge.m_entities[0]->addReference();
            rMerge.m_entities[1]->addReference();
            m_islandMerges.pushBack(rOp);
            m_pending.popBack();
            break;
        }
        case hkWorldOperation::PHANTOM_ADD:
            static_cast<hkWorldOperation::AddPhantom&>(rBase).m_phantom->addReference();
            break;
        case hkWorldOperation::PHANTOM_REMOVE:
            static_cast<hkWorldOperation::RemovePhantom&>(rBase).m_phantom->addReference();
            break;
        case hkWorldOperation::PHANTOM_BATCH_ADD: {
            hkWorldOperation::AddPhantomBatch& rBatch = static_cast<hkWorldOperation::AddPhantomBatch&>(rBase);
            const hkWorldOperation::AddPhantomBatch& rSrc = static_cast<const hkWorldOperation::AddPhantomBatch&>(
                operation
            );
            rBatch.m_phantoms = hkMemoryRouter::getInstance().heap()._blockAlloc<hkpPhantom*>(rSrc.m_numPhantoms);
            hkString::memCpy(
                rBatch.m_phantoms,
                rSrc.m_phantoms,
                rSrc.m_numPhantoms * static_cast<int32_t>(sizeof(hkpPhantom*))
            );
            hkpPhantom** ppEnd = rBatch.m_phantoms + rBatch.m_numPhantoms;
            for (hkpPhantom** ppPhantom = rBatch.m_phantoms; ppPhantom < ppEnd; ++ppPhantom) {
                (*ppPhantom)->addReference();
            }
            break;
        }
        case hkWorldOperation::PHANTOM_BATCH_REMOVE: {
            hkWorldOperation::RemovePhantomBatch& rBatch = static_cast<hkWorldOperation::RemovePhantomBatch&>(rBase);
            const hkWorldOperation::RemovePhantomBatch& rSrc = static_cast<const hkWorldOperation::RemovePhantomBatch&>(
                operation
            );
            rBatch.m_phantoms = hkMemoryRouter::getInstance().heap()._blockAlloc<hkpPhantom*>(rSrc.m_numPhantoms);
            hkString::memCpy(
                rBatch.m_phantoms,
                rSrc.m_phantoms,
                rSrc.m_numPhantoms * static_cast<int32_t>(sizeof(hkpPhantom*))
            );
            hkpPhantom** ppEnd = rBatch.m_phantoms + rBatch.m_numPhantoms;
            for (hkpPhantom** ppPhantom = rBatch.m_phantoms; ppPhantom < ppEnd; ++ppPhantom) {
                (*ppPhantom)->addReference();
            }
            break;
        }
        case hkWorldOperation::PHANTOM_UPDATE_BROAD_PHASE: {
            hkWorldOperation::UpdatePhantomBP& rUpdate = static_cast<hkWorldOperation::UpdatePhantomBP&>(rBase);
            const hkWorldOperation::UpdatePhantomBP& rSrc = static_cast<const hkWorldOperation::UpdatePhantomBP&>(
                operation
            );
            rUpdate.m_phantom->addReference();
            hkAabb* pAabb = static_cast<hkAabb*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkAabb)));
            if (pAabb) {
                *pAabb = *rSrc.m_aabb;
            }
            rUpdate.m_aabb = pAabb;
            break;
        }
        case hkWorldOperation::UPDATE_FILTER_ENTITY:
            static_cast<hkWorldOperation::UpdateFilterOnEntity&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::UPDATE_FILTER_ENTITY_PAIR: {
            hkWorldOperation::UpdateFilterOnEntityPair&
                rPair = static_cast<hkWorldOperation::UpdateFilterOnEntityPair&>(rBase);
            rPair.m_entityA->addReference();
            rPair.m_entityB->addReference();
            break;
        }
        case hkWorldOperation::UPDATE_FILTER_PHANTOM:
            static_cast<hkWorldOperation::UpdateFilterOnPhantom&>(rBase).m_phantom->addReference();
            break;
        case hkWorldOperation::UPDATE_MOVED_BODY_INFO:
            static_cast<hkWorldOperation::UpdateMovedBodyInfo&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::ENTITY_BATCH_REINTEGRATE_AND_RECOLLIDE: {
            hkWorldOperation::ReintegrateAndRecollideEntityBatch&
                rBatch = static_cast<hkWorldOperation::ReintegrateAndRecollideEntityBatch&>(rBase);
            const hkWorldOperation::ReintegrateAndRecollideEntityBatch&
                rSrc = static_cast<const hkWorldOperation::ReintegrateAndRecollideEntityBatch&>(operation);
            rBatch.m_entities = hkMemoryRouter::getInstance().heap()._blockAlloc<hkpEntity*>(rSrc.m_numEntities);
            hkString::memCpy(
                rBatch.m_entities,
                rSrc.m_entities,
                rSrc.m_numEntities * static_cast<int32_t>(sizeof(hkpEntity*))
            );
            hkpEntity** ppEnd = rBatch.m_entities + rBatch.m_numEntities;
            for (hkpEntity** ppEntity = rBatch.m_entities; ppEntity < ppEnd; ++ppEntity) {
                (*ppEntity)->addReference();
            }
            break;
        }
        case hkWorldOperation::RIGIDBODY_SET_POSITION_AND_ROTATION:
            static_cast<hkWorldOperation::SetRigidBodyPositionAndRotation&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::RIGIDBODY_SET_LINEAR_VELOCITY:
            static_cast<hkWorldOperation::SetRigidBodyLinearVelocity&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::RIGIDBODY_SET_ANGULAR_VELOCITY:
            static_cast<hkWorldOperation::SetRigidBodyAngularVelocity&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::RIGIDBODY_APPLY_LINEAR_IMPULSE:
            static_cast<hkWorldOperation::ApplyRigidBodyLinearImpulse&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::RIGIDBODY_APPLY_POINT_IMPULSE:
            static_cast<hkWorldOperation::ApplyRigidBodyPointImpulse&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::RIGIDBODY_APPLY_ANGULAR_IMPULSE:
            static_cast<hkWorldOperation::ApplyRigidBodyAngularImpulse&>(rBase).m_rigidBody->addReference();
            break;
        case hkWorldOperation::WORLD_OBJECT_ADD_REFERENCE:
            static_cast<hkWorldOperation::AddReference&>(rBase).m_worldObject->addReference();
            --m_pending.m_size;
            --m_world->m_pendingOperationsCount;
            break;
        case hkWorldOperation::WORLD_OBJECT_REMOVE_REFERENCE:
            static_cast<hkWorldOperation::RemoveReference&>(rBase).m_worldObject->removeReference();
            --m_pending.m_size;
            --m_world->m_pendingOperationsCount;
            break;
        case hkWorldOperation::ACTIVATE_ENTITY:
            static_cast<hkWorldOperation::ActivateEntity&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::DEACTIVATE_ENTITY:
            static_cast<hkWorldOperation::DeactivateEntity&>(rBase).m_entity->addReference();
            break;
        case hkWorldOperation::CONSTRAINT_FILTER_BROKEN_EVENT: {
            hkWorldOperation::ConstraintCollisionFilterConstraintBroken&
                rBroken = static_cast<hkWorldOperation::ConstraintCollisionFilterConstraintBroken&>(rBase);
            rBroken.m_filter->addReference();
            rBroken.m_constraintInstance->addReference();
            break;
        }
        case hkWorldOperation::USER_CALLBACK:
            static_cast<hkWorldOperation::UserCallbackOperation&>(rBase).m_userCallback->addReference();
            break;
        default:
            // UPDATE_FILTER_WORLD and ACTIVATE_REGION hold no reference.
            break;
    }
    hkReferencedObject::unlockAll();
}
