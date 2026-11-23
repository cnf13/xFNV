#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Algorithm/UnionFind/hkUnionFind.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpFixedRigidMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Internal/Collide/BroadPhase/3AxisSweep/hkp3AxisSweep.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Joins groups smaller than the world's minimum island size into sparse islands: each group's size is its entities'
// constraint count (at least one per entity), -1 for a group with no recently active entity (with deactivation), -2
// once merged into an earlier group.
void mergeSmallSubgroups(
    hkpWorld* world,
    hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    hkArray<int32_t, hkContainerHeapAllocator>& groupsSizes,
    hkFixedArray<int8_t>& groupSparse
) {
    const int32_t numGroups = groupsSizes.m_size;
    hkLocalBuffer<int32_t> groupSize(numGroups, nullptr);
    hkLocalBuffer<int32_t> reindex(numGroups, nullptr);
    for (int32_t g = 0; g < numGroups; ++g) {
        reindex[g] = 0;
        groupSize[g] = 0;
    }
    if (world->m_wantDeactivation.m_bool) {
        findInactiveGroupsAndEstimateSolverSize(
            checker,
            entities,
            reindex,
            groupSize,
            world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag
        );
    }
    else {
        estimateSolverSize(checker, groupSize, reindex, entities);
    }
    for (int32_t g = 0; g < numGroups; ++g) {
        if (!reindex[g]) {
            groupSize[g] = -1;
        }
    }
    int32_t numNewGroups = 0;
    for (int32_t g = 0; g < numGroups; ++g) {
        int32_t size = groupSize[g];
        if (size == -2) {
            continue;
        }
        const int32_t newGroup = numNewGroups;
        groupSparse[newGroup] = 0;
        ++numNewGroups;
        reindex[g] = newGroup;
        if (size < 0) {
            continue;
        }
        for (int32_t h = g + 1; h < numGroups; ++h) {
            // The world's minimum size is compared signed.
            const int32_t minSize = static_cast<int32_t>(world->m_minDesiredIslandSize);
            if (size + 10 >= minSize) {
                break;
            }
            if (groupSize[h] >= 0 && size + groupSize[h] < minSize) {
                groupSparse[newGroup] = 1;
                reindex[h] = newGroup;
                size += groupSize[h];
                groupSize[h] = -2;
            }
        }
    }
    checker.reindex(reindex, numNewGroups, groupsSizes);
}

void findInactiveGroupsAndEstimateSolverSize(
    const hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    hkFixedArray<int32_t>& isActive,
    hkFixedArray<int32_t>& sizes,
    const uint8_t* deactivationNumInactiveFramesSelectFlag
) {
    for (int32_t i = 0; i < entities.m_size; ++i) {
        hkpEntity* entity = entities.m_data[i];
        int32_t group = checker.m_parents->m_data.m_storage[i];
        // The SDK's hkpMotion::getNumInactiveFrames for both selectors, the larger one kept.
        int32_t frames1 = entity->m_motion.m_deactivationNumInactiveFrames[1];
        if (deactivationNumInactiveFramesSelectFlag[1] == frames1 >> 14) {
            frames1 >>= 7;
        }
        int32_t frames0 = entity->m_motion.m_deactivationNumInactiveFrames[0];
        if (deactivationNumInactiveFramesSelectFlag[0] == frames0 >> 14) {
            frames0 >>= 7;
        }
        int32_t numInactiveFrames = frames1 & 127;
        if ((frames0 & 127) > numInactiveFrames) {
            numInactiveFrames = frames0 & 127;
        }
        if (numInactiveFrames <= 5) {
            isActive.m_data.m_storage[group] = 1;
        }
        int32_t size = entity->m_constraintsMaster.m_size;
        if (size < 1) {
            size = 1;
        }
        sizes.m_data.m_storage[group] += size;
    }
}

void estimateSolverSize(
    const hkUnionFind& checker,
    hkFixedArray<int32_t>& sizes,
    hkFixedArray<int32_t>& isActive,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities
) {
    for (int32_t i = 0; i < entities.m_size; ++i) {
        hkpEntity* v5 = entities.m_data[i];
        int32_t v6 = checker.m_parents->m_data.m_storage[i];
        isActive.m_data.m_storage[v6] = 1;
        int32_t m_size = v5->m_constraintsMaster.m_size;
        if (m_size < 1) {
            m_size = 1;
        }
        sizes.m_data.m_storage[v6] += m_size;
    }
}

hkpSimulationIsland* findSmallSparseActiveIsland(
    hkpWorld* world,
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& islands,
    int32_t islandSizeAddon,
    hkpSimulationIsland* islandToIgnore
) {
    hkpSimulationIsland* result;
    int32_t m_minDesiredIslandSize = world->m_minDesiredIslandSize;
    if (islandSizeAddon >= m_minDesiredIslandSize || !world->m_wantSimulationIslands) {
        return nullptr;
    }
    int32_t m_size = islands.m_size;
    int32_t v8 = m_size - 20;
    int32_t v9 = m_size - 20 < 0;
    int32_t v10 = m_size - 1;
    int32_t v11 = v9 ? 0 : v8;
    if (v10 < v11) {
        return nullptr;
    }
    for (hkpSimulationIsland** i = &islands.m_data[v10];; --i) {
        result = *i;
        if (islandToIgnore != *i) {
            int32_t m_numConstraints = result->m_entities.m_size;
            if (m_numConstraints <= result->m_numConstraints) {
                m_numConstraints = result->m_numConstraints;
            }
            if (result->m_activeMark != 0 && islandSizeAddon + m_numConstraints < m_minDesiredIslandSize) {
                break;
            }
        }
        if (--v10 < v11) {
            return nullptr;
        }
    }
    result->m_sparseEnabled = 1;
    return result;
}

void findInactiveGroups(
    const hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    const hkpWorld* world,
    int32_t numGroups,
    hkFixedArray<int32_t>& isActive
) {
    for (int32_t i = 0; i < numGroups; ++i) {
        isActive.m_data.m_storage[i] = 0;
    }
    for (int32_t j = 0; j < entities.m_size; ++j) {
        hkpEntity* entity = entities.m_data[j];
        int32_t frames1 = entity->m_motion.m_deactivationNumInactiveFrames[1];
        if (world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[1] == frames1 >> 14) {
            frames1 >>= 7;
        }
        int32_t frames0 = entity->m_motion.m_deactivationNumInactiveFrames[0];
        if (world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[0] == frames0 >> 14) {
            frames0 >>= 7;
        }
        int32_t numInactiveFrames = frames1 & 127;
        if ((frames0 & 127) > numInactiveFrames) {
            numInactiveFrames = frames0 & 127;
        }
        if (numInactiveFrames <= 5) {
            isActive.m_data.m_storage[checker.m_parents->m_data.m_storage[j]] = 1;
        }
    }
}

// Group 0 gathers the groups with an entity active in the last five frames, as one sparse island; every other group
// gets its own. The groups are renumbered only when some group was inactive.
void identifyDeactivatedSubgroups(
    hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    const hkpWorld* world,
    hkArray<int32_t, hkContainerHeapAllocator>& groupsSizes,
    hkFixedArray<int8_t>& groupSparse
) {
    const int32_t numGroups = groupsSizes.m_size;
    hkLocalBuffer<int32_t> isActive(numGroups, nullptr);
    findInactiveGroups(checker, entities, world, numGroups, isActive);

    groupSparse[0] = 1;
    int32_t numNewGroups = 1;
    for (int32_t g = 0; g < numGroups; ++g) {
        if (isActive[g]) {
            isActive[g] = 0;
        }
        else {
            isActive[g] = numNewGroups;
            groupSparse[numNewGroups++] = 0;
        }
    }
    if (numNewGroups <= numGroups) {
        checker.reindex(isActive, numNewGroups, groupsSizes);
    }
    else {
        groupSparse[0] = 0;
    }
}

hkBool worldOperationUtil_less_hkSimulationIslandPtr(const hkpSimulationIsland* a, const hkpSimulationIsland* b) {
    if (b) {
        if (a) {
            return (*a->m_entities.begin())->getUid() < (*b->m_entities.begin())->getUid();
        }
        else {
            return 1;
        }
    }
    else {
        return 0;
    }
}

void hkpWorldOperationUtil::markIslandInactive(hkpWorld* world, hkpSimulationIsland* island) {
    island->m_activeMark = 0;
    if (island->m_dirtyListIndex == 0xFFFF) {
        island->m_dirtyListIndex = world->m_dirtySimulationIslands.m_size;
        world->m_dirtySimulationIslands.pushBack(island);
    }
}

// Havok SDK: splits the island into its connected parts. The new islands go into the array the island is in and take
// its array flag as both their array flag and their active mark; an active-array island whose own mark was cleared
// has each new island marked inactive. Their split frame counters run on from the island's.
void hkpWorldOperationUtil::splitSimulationIsland(hkpWorld* world, hkpSimulationIsland* currentIsland) {
    const uint8_t isInActiveIslandsArray = currentIsland->m_isInActiveIslandsArray;
    hkArray<hkpSimulationIsland*>& islands = isInActiveIslandsArray ? world->m_activeSimulationIslands
                                                                    : world->m_inactiveSimulationIslands;
    currentIsland->m_splitCheckRequested = 0;
    const int32_t oldSize = islands.m_size;
    splitSimulationIsland(currentIsland, world, islands, nullptr);
    const uint8_t activeMark = currentIsland->m_activeMark;
    for (int32_t i = oldSize; i < islands.m_size; ++i) {
        hkpSimulationIsland* newIsland = islands.m_data[i];
        newIsland->m_activeMark = isInActiveIslandsArray;
        newIsland->m_isInActiveIslandsArray = isInActiveIslandsArray;
        if (isInActiveIslandsArray && !activeMark) {
            markIslandInactive(world, newIsland);
        }
        newIsland->m_splitCheckFrameCounter = static_cast<uint8_t>(
            currentIsland->m_splitCheckFrameCounter - oldSize + i
        );
    }
}

// The constraint goes into the world's islands, then the world tells its constraint listeners. The caller already
// holds the critical-operations lock.
void hkpWorldOperationUtil::addConstraintToCriticalLockedIsland(hkpWorld* world, hkpConstraintInstance* constraint) {
    hkpWorldConstraintUtil::addConstraint(world, constraint);
    hkpWorldCallbackUtil::fireConstraintAdded(world, constraint);
}

// The same add under a lock this function takes itself: releasing it runs the operations the lock held back. Returns
// the constraint.
hkpConstraintInstance* hkpWorldOperationUtil::addConstraintImmediately(
    hkpWorld* world,
    hkpConstraintInstance* constraint,
    FireCallbacks fireCallbacks
) {
    world->lockCriticalOperations();
    hkpWorldConstraintUtil::addConstraint(world, constraint);
    if (fireCallbacks != 0) {
        hkpWorldCallbackUtil::fireConstraintAdded(world, constraint);
    }
    world->unlockAndAttemptToExecutePendingOperations();
    return constraint;
}

// The owner island is marked for a split check, then the constraint comes out: under the world's lock when it is
// already taken, else under a lock of this function's own whose release runs the held-back operations.
void hkpWorldOperationUtil::removeConstraintImmediately(
    hkpWorld* world,
    hkpConstraintInstance* constraint,
    FireCallbacks fireCallbacks
) {
    // The constraint's owner is the island it was added to.
    hkpSimulationIsland* island = static_cast<hkpSimulationIsland*>(constraint->m_owner);
    island->m_splitCheckRequested = 1;
    if (world->m_criticalOperationsLockCount != 0) {
        if (fireCallbacks != 0 && world->m_constraintListeners.m_size != 0) {
            hkpWorldCallbackUtil::fireConstraintRemoved(world, constraint);
        }
        hkpWorldConstraintUtil::removeConstraint(constraint);
        return;
    }
    world->lockCriticalOperations();
    if (fireCallbacks != 0 && world->m_constraintListeners.m_size != 0) {
        hkpWorldCallbackUtil::fireConstraintRemoved(world, constraint);
    }
    hkpWorldConstraintUtil::removeConstraint(constraint);
    world->unlockAndAttemptToExecutePendingOperations();
}

// The world's half of adding an entity: the entity gets its world, then joins the fixed island, an existing sparse
// active island, a fresh island, or the world's first active island when the world keeps no islands.
void hkpWorldOperationUtil::addEntitySI(
    hkpWorld* world,
    hkpEntity* entity,
    hkpEntityActivation initialActivationState
) {
    entity->m_world = world;
    if (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
        world->m_fixedIsland->internalAddEntity(entity);
        return;
    }
    hkpSimulationIsland* island;
    if (world->m_wantSimulationIslands) {
        bool activate = initialActivationState == HK_ENTITY_ACTIVATION_DO_ACTIVATE;
        hkArray<hkpSimulationIsland*>* islands;
        if (activate) {
            islands = &world->m_activeSimulationIslands;
            // The island is allowed to take four more entities before it counts as full.
            hkpSimulationIsland* sparseIsland = findSmallSparseActiveIsland(world, *islands, 4, nullptr);
            if (sparseIsland != nullptr) {
                sparseIsland->internalAddEntity(entity);
                return;
            }
        }
        else {
            islands = &world->m_inactiveSimulationIslands;
        }
        island = new hkpSimulationIsland(world);
        island->m_isInActiveIslandsArray = activate ? 1 : 0;
        island->m_activeMark = activate ? 1 : 0;
        island->m_storageIndex = static_cast<uint16_t>(islands->m_size);
        island->m_splitCheckFrameCounter = static_cast<uint8_t>(islands->m_size);
        islands->pushBack(island);
    }
    else {
        island = world->m_activeSimulationIslands.m_data[0];
    }
    island->internalAddEntity(entity);
    island->m_splitCheckFrameCounter = static_cast<uint8_t>(entity->m_uid);
}

void hkpWorldOperationUtil::removeIslandFromDirtyList(hkpWorld* world, hkpSimulationIsland* island) {
    uint16_t m_dirtyListIndex;
    m_dirtyListIndex = island->m_dirtyListIndex;
    if (m_dirtyListIndex != 0xffff) {
        world->m_dirtySimulationIslands.m_data[m_dirtyListIndex] = nullptr;
        island->m_dirtyListIndex = static_cast<uint16_t>(-1);
    }
}

void hkpWorldOperationUtil::removeIsland(hkpWorld* world, hkpSimulationIsland* island) {
    int32_t m_storageIndex;
    m_storageIndex = island->m_storageIndex;
    if (island->m_isInActiveIslandsArray != 0) {
        world->m_activeSimulationIslands
            .m_data[m_storageIndex] = world->m_activeSimulationIslands
                                          .m_data[world->m_activeSimulationIslands.m_size - 1];
        world->m_activeSimulationIslands.m_data[island->m_storageIndex]->m_storageIndex = island->m_storageIndex;
        --world->m_activeSimulationIslands.m_size;
    }
    else {
        world->m_inactiveSimulationIslands
            .m_data[m_storageIndex] = world->m_inactiveSimulationIslands
                                          .m_data[world->m_inactiveSimulationIslands.m_size - 1];
        world->m_inactiveSimulationIslands.m_data[island->m_storageIndex]->m_storageIndex = island->m_storageIndex;
        --world->m_inactiveSimulationIslands.m_size;
    }
    hkpWorldOperationUtil::removeIslandFromDirtyList(world, island);
}

void hkpWorldOperationUtil::markIslandActive(hkpWorld* world, hkpSimulationIsland* island) {
    if (island->m_activeMark == 0 && island->m_isInActiveIslandsArray != 0) {
        island->m_tryToIncreaseIslandSizeMark = 1;
    }
    island->m_activeMark = 1;
    if (island->m_dirtyListIndex == 0xffff) {
        island->m_dirtyListIndex = static_cast<uint16_t>(world->m_dirtySimulationIslands.m_size);
        world->m_dirtySimulationIslands.pushBack(island);
    }
}

// An entity with a shape has its TOI events reset, leaves the broad phase, and the pairs the broad phase reports are
// removed. The pair array is a stack-allocated hkLocalArray of m_broadPhaseQuerySize pairs.
void hkpWorldOperationUtil::removeEntityBP(hkpWorld* world, hkpEntity* entity) {
    hkpLinkedCollidable* pCollidable = &entity->m_collidable;
    if (!pCollidable->m_shape) {
        return;
    }
    world->m_simulation->resetCollisionInformationForEntities(&entity, 1, world, hkpSimulation::RESET_TOI);

    const int32_t iCapacity = world->m_broadPhaseQuerySize;
    hkLocalArray<hkpBroadPhaseHandlePair> delPairs(iCapacity);

    world->getBroadPhase()->removeObject(pCollidable->getBroadPhaseHandle(), delPairs);
    if (delPairs.m_size > 0) {
        world->m_broadPhaseDispatcher->removePairs(
            static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.m_data),
            delPairs.m_size
        );
    }
}

void hkpWorldOperationUtil::removeEntitySI(hkpWorld* world, hkpEntity* entity) {
    hkpSimulationIsland* m_simulationIsland = entity->getSimulationIsland();
    entity->m_world = nullptr;
    m_simulationIsland->internalRemoveEntity(entity);
    if (m_simulationIsland->m_storageIndex != 0xFFFF &&
        !m_simulationIsland->m_entities.m_size &&
        world->m_wantSimulationIslands) {
        hkpWorldOperationUtil::removeIsland(world, m_simulationIsland);
        delete m_simulationIsland;
    }
}

// An entity with a shape has its box recomputed and goes into the broad phase with it, and the pairs the broad phase
// reports are dispatched through the world's collision filter. The pair array is a stack-allocated hkLocalArray of
// m_broadPhaseQuerySize pairs.
void hkpWorldOperationUtil::addEntityBP(hkpWorld* world, hkpEntity* entity) {
    hkpLinkedCollidable* pCollidable = &entity->m_collidable;
    if (!pCollidable->m_shape) {
        return;
    }
    const int32_t iCapacity = world->m_broadPhaseQuerySize;
    hkLocalArray<hkpBroadPhaseHandlePair> newPairs(iCapacity);

    hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, &entity, 1);
    // Copies the bounding volume data's box, laid out as an hkAabbUint32, field by field.
    const hkpCollidable::BoundingVolumeData& rData = entity->m_collidable.m_boundingVolumeData;
    hkAabbUint32 aabb;
    aabb.m_min[0] = rData.m_min[0];
    aabb.m_min[1] = rData.m_min[1];
    aabb.m_min[2] = rData.m_min[2];
    aabb.m_expansionMin[0] = rData.m_expansionMin[0];
    aabb.m_expansionMin[1] = rData.m_expansionMin[1];
    aabb.m_expansionMin[2] = rData.m_expansionMin[2];
    aabb.m_expansionShift = rData.m_expansionShift;
    aabb.m_max[0] = rData.m_max[0];
    aabb.m_max[1] = rData.m_max[1];
    aabb.m_max[2] = rData.m_max[2];
    aabb.m_expansionMax[0] = rData.m_expansionMax[0];
    aabb.m_expansionMax[1] = rData.m_expansionMax[1];
    aabb.m_expansionMax[2] = rData.m_expansionMax[2];
    aabb.m_shapeKeyByte = rData.m_padding;
    world->getBroadPhase()->addObject(pCollidable->getBroadPhaseHandle(), aabb, newPairs);
    if (newPairs.m_size > 0) {
        const hkpCollidableCollidableFilter* pFilter = static_cast<const hkpCollidableCollidableFilter*>(
            world->m_collisionFilter
        );
        world->m_broadPhaseDispatcher
            ->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.m_data), newPairs.m_size, pFilter);
    }
}

// The phantom's box goes into the broad phase, the phantom takes it as its bounding volume data, and the pairs the
// broad phase reports are dispatched through the world's collision filter. The pair array is a stack-allocated
// hkLocalArray of m_broadPhaseQuerySize pairs.
void hkpWorldOperationUtil::addPhantomBP(hkpWorld* world, hkpPhantom* phantom) {
    const int32_t iCapacity = world->m_broadPhaseQuerySize;
    alignas(16) hkLocalArray<hkpBroadPhaseHandlePair> newPairs;
    newPairs.m_data = nullptr;
    newPairs.m_size = 0;
    newPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
    hkpBroadPhaseHandlePair* pLocal = hkAllocateStack<hkpBroadPhaseHandlePair>(iCapacity, nullptr);
    newPairs.m_data = pLocal;
    newPairs.m_capacityAndFlags = static_cast<int32_t>(
        static_cast<uint32_t>(iCapacity) | hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG
    );
    newPairs.m_localMemory = pLocal;

    // This function does not call hkAabb's default constructor for aabb.
    hkAabb aabb;
    phantom->calcAabb(aabb);
    world->getBroadPhase()->addObject(phantom->m_collidable.getBroadPhaseHandle(), aabb, newPairs);
    phantom->setBoundingVolumeData(aabb);
    if (newPairs.m_size != 0) {
        const hkpCollidableCollidableFilter* pFilter = static_cast<const hkpCollidableCollidableFilter*>(
            world->m_collisionFilter
        );
        world->m_broadPhaseDispatcher
            ->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.m_data), newPairs.m_size, pFilter);
    }
}

// The phantom leaves the broad phase and the pairs the broad phase reports are removed. The pair array is a
// stack-allocated hkLocalArray of m_broadPhaseQuerySize pairs.
void hkpWorldOperationUtil::removePhantomBP(hkpWorld* world, hkpPhantom* phantom) {
    const int32_t iCapacity = world->m_broadPhaseQuerySize;
    hkLocalArray<hkpBroadPhaseHandlePair> delPairs(iCapacity);

    world->getBroadPhase()->removeObject(phantom->m_collidable.getBroadPhaseHandle(), delPairs);
    if (delPairs.m_size != 0) {
        world->m_broadPhaseDispatcher->removePairs(
            static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.m_data),
            delPairs.m_size
        );
    }
}

// Sorts the dirty islands, then takes them off the list from the end one at a time: the island drops its removed
// actions when it is flagged for it, then either merges with a sparse active island or switches between the active
// and inactive arrays.
void hkpWorldOperationUtil::cleanupDirtyIslands(hkpWorld* world) {
    hkArray<hkpSimulationIsland*>& dirtyIslands = world->m_dirtySimulationIslands;
    const int32_t numDirty = dirtyIslands.m_size;
    if (numDirty == 0) {
        return;
    }
    if (numDirty > 1) {
        hkAlgorithm::quickSortRecursive<hkpSimulationIsland*, hkpSimulationIslandLess>(
            dirtyIslands.m_data,
            0,
            numDirty - 1,
            &worldOperationUtil_less_hkSimulationIslandPtr
        );
    }
    for (int32_t i = 0; i < dirtyIslands.m_size; ++i) {
        hkpSimulationIsland* pIsland = dirtyIslands.m_data[i];
        if (pIsland != nullptr) {
            pIsland->m_dirtyListIndex = static_cast<uint16_t>(i);
        }
    }
    while (dirtyIslands.m_size != 0) {
        hkpSimulationIsland* pIsland = dirtyIslands.m_data[dirtyIslands.m_size - 1];
        --dirtyIslands.m_size;
        if (pIsland == nullptr) {
            continue;
        }
        // The island leaves the list first; the flags then say whether its action list needs cleaning.
        const bool bCleanupActions = pIsland->m_actionListCleanupNeeded != 0;
        pIsland->m_dirtyListIndex = static_cast<uint16_t>(-1);
        if (bCleanupActions) {
            // Drops the null entries the removed actions left, keeping the order of the rest.
            hkArray<hkpAction*>& actions = pIsland->m_actions;
            const int32_t numActions = actions.m_size;
            int32_t i = 0;
            int32_t firstNull = -1;
            if (numActions > 0) {
                hkpAction** pAction = actions.m_data;
                while (i < numActions && *pAction != nullptr) {
                    ++i;
                    ++pAction;
                }
                if (i < numActions) {
                    firstNull = i++;
                }
            }
            if (i < numActions) {
                do {
                    if (actions.m_data[i] != nullptr) {
                        actions.m_data[firstNull++] = actions.m_data[i];
                    }
                    ++i;
                }
                while (i < actions.m_size);
            }
            if (firstNull != -1) {
                actions.setSize(firstNull);
            }
            pIsland->m_actionListCleanupNeeded = 0;
        }
        // m_activeMark against m_isInActiveIslandsArray decides the island's fate.
        const uint8_t activeMark = pIsland->m_activeMark;
        if (activeMark == pIsland->m_isInActiveIslandsArray) {
            if (pIsland->m_tryToIncreaseIslandSizeMark != 0) {
                const int32_t numConstraints = pIsland->m_numConstraints;
                pIsland->m_tryToIncreaseIslandSizeMark = 0;
                int32_t numEntities = pIsland->m_entities.m_size;
                if (numEntities <= numConstraints) {
                    numEntities = numConstraints;
                }
                hkpSimulationIsland* pSparseIsland = findSmallSparseActiveIsland(
                    world,
                    world->m_activeSimulationIslands,
                    numEntities,
                    pIsland
                );
                if (pSparseIsland != nullptr) {
                    mergeIslands(world, *pIsland->m_entities.m_data, *pSparseIsland->m_entities.m_data);
                }
            }
        }
        else if (activeMark != 0) {
            internalActivateIsland(world, pIsland, hkBool(true));
        }
        else {
            internalDeactivateIsland(world, pIsland);
        }
    }
    world->checkDeterminism();
}

void hkpWorldOperationUtil::internalActivateIsland(
    hkpWorld* world,
    hkpSimulationIsland* island,
    hkBool allowIslandMerges
) {
    world->m_inactiveSimulationIslands
        .m_data[island->m_storageIndex] = world->m_inactiveSimulationIslands
                                              .m_data[world->m_inactiveSimulationIslands.m_size - 1];
    world->m_inactiveSimulationIslands.m_data[island->m_storageIndex]->m_storageIndex = island->getStorageIndex();
    --world->m_inactiveSimulationIslands.m_size;
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>* activeIslands = &world->m_activeSimulationIslands;
    activeIslands->pushBack(island);
    int16_t m_size = activeIslands->m_size;
    island->m_timeSinceLastHighFrequencyCheck = 0.0f;
    island->m_timeSinceLastLowFrequencyCheck = 0.0f;
    island->m_isInActiveIslandsArray = 1;
    island->m_storageIndex = m_size - 1;
    for (hkpEntity* entity : island->m_entities) {
        hkSweptTransformUtil::setTimeInformation(0.0f, 0.0f, entity->m_motion.m_motionState);
        entity->m_motion.m_deactivationNumInactiveFrames[1] = 0;
        entity->m_motion.m_deactivationNumInactiveFrames[0] = 0;
        const hkpSolverInfo& solverInfo = world->m_dynamicsStepInfo.m_solverInfo;
        int32_t selectFlag0 = solverInfo.m_deactivationNumInactiveFramesSelectFlag[0];
        int32_t selectFlag1 = solverInfo.m_deactivationNumInactiveFramesSelectFlag[1];
        int32_t integrateCounter = solverInfo.m_deactivationIntegrateCounter;
        if ((integrateCounter & 3) < (entity->m_motion.m_deactivationIntegrateCounter & 3)) {
            selectFlag0 = ~selectFlag0;
        }
        if (integrateCounter < entity->m_motion.m_deactivationIntegrateCounter) {
            selectFlag1 = ~selectFlag1;
        }
        entity->m_motion.m_deactivationNumInactiveFrames[0] = static_cast<uint16_t>(selectFlag0 << 14);
        entity->m_motion.m_deactivationNumInactiveFrames[1] = static_cast<uint16_t>(selectFlag1 << 14);
    }
    hkpWorldAgentUtil::warpTime(
        island,
        island->m_timeOfDeactivation,
        world->m_dynamicsStepInfo.m_stepInfo.m_startTime,
        *world->m_collisionInput
    );
    hkpEntity* firstEntity = *island->m_entities.begin();
    firstEntity->addReference();
    hkpWorldCallbackUtil::fireIslandActivated(world, island);
    if (firstEntity->getSimulationIsland() != island) {
        firstEntity->removeReference();
        return;
    }
    firstEntity->removeReference();
    if (allowIslandMerges.m_bool) {
        int32_t numEntitiesToAdd = island->m_numConstraints + 3;
        if (island->m_entities.m_size + 1 > numEntitiesToAdd) {
            numEntitiesToAdd = island->m_entities.m_size + 1;
        }
        hkpSimulationIsland*
            sparseIsland = findSmallSparseActiveIsland(world, *activeIslands, numEntitiesToAdd, island);
        if (sparseIsland) {
            hkpWorldOperationUtil::internalMergeTwoIslands(world, sparseIsland, island);
            return;
        }
    }
    // Keep the largest active island first.
    int32_t storageIndex = island->getStorageIndex();
    if (island->m_storageIndex) {
        hkpSimulationIsland* firstIsland = *activeIslands->begin();
        if (island->m_entities.m_size > firstIsland->m_entities.m_size) {
            island->m_storageIndex = 0;
            firstIsland->m_storageIndex = storageIndex;
            *activeIslands->begin() = island;
            activeIslands->m_data[storageIndex] = firstIsland;
        }
    }
}

// The SDK's cast: the callee treats the entity array as untyped pointers, each plus the byte offset of m_motion.
void hkpWorldOperationUtil::internalDeactivateIsland(hkpWorld* world, hkpSimulationIsland* island) {
    if (hkRigidMotionUtilCanDeactivateFinal(
            world->m_dynamicsStepInfo.m_stepInfo,
            reinterpret_cast<hkpMotion* const*>(island->m_entities.begin()),
            island->m_entities.m_size,
            offsetof(hkpEntity, m_motion)
        )) {
        world->m_inactiveSimulationIslands.pushBack(island);
        world->m_activeSimulationIslands
            .m_data[island->m_storageIndex] = world->m_activeSimulationIslands
                                                  .m_data[world->m_activeSimulationIslands.m_size - 1];
        world->m_activeSimulationIslands.m_data[island->m_storageIndex]->m_storageIndex = island->getStorageIndex();
        --world->m_activeSimulationIslands.m_size;
        int16_t m_size = world->m_inactiveSimulationIslands.m_size;
        island->m_isInActiveIslandsArray = 0;
        island->m_storageIndex = m_size - 1;
        island->m_timeOfDeactivation = world->m_dynamicsStepInfo.m_stepInfo.m_startTime;
        for (hkpEntity* entity : island->m_entities) {
            hkSweptTransformUtil::freezeMotionState(island->m_timeOfDeactivation, entity->m_motion.m_motionState);
            // Both setters are called virtually through the motion pointer.
            hkpMotion* motion = &entity->m_motion;
            motion->setLinearVelocity(hkVector4Zero);
            motion->setAngularVelocity(hkVector4Zero);
        }
        hkpWorldCallbackUtil::fireIslandDeactivated(world, island);
    }
    else {
        island->m_activeMark = 1;
    }
}

void hkpWorldOperationUtil::mergeIslands(hkpWorld* world, hkpEntity* entityA, hkpEntity* entityB) {
    if (world->m_criticalOperationsLockCount) {
        hkWorldOperation::MergeIslands op;
        op.m_entities[0] = entityA;
        op.m_type.m_storage = hkWorldOperation::ISLAND_MERGE;
        op.m_entities[1] = entityB;
        world->queueOperation(op);
    }
    else {
        hkpWorldOperationUtil::internalMergeTwoIslands(
            world,
            entityA->getSimulationIsland(),
            entityB->getSimulationIsland()
        );
    }
}

// One inplace array of 16 entities serves every action, and it is deliberately never emptied between actions:
// each getEntities call appends to what the earlier ones left. An action moves when every entity in that
// list is fixed or the entity itself; the island is then flagged for action list cleanup and put on the world's dirty
// list.
void hkpWorldOperationUtil::removeAttachedActionsFromDynamicIsland(
    hkpWorld* world,
    hkpEntity* entity,
    hkArray<hkpAction*>& actionsToBeMoved
) {
    hkpSimulationIsland* island = entity->m_simulationIsland;
    hkInplaceArray<hkpEntity*, 16> entities;
    for (int32_t i = 0; i < entity->m_actions.m_size; ++i) {
        hkpAction* action = entity->m_actions.m_data[i];
        action->getEntities(entities);
        bool allOthersFixed = true;
        for (hkpEntity* other : entities) {
            if (other->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED && other != entity) {
                allOthersFixed = false;
                break;
            }
        }
        if (!allOthersFixed) {
            continue;
        }
        actionsToBeMoved.pushBack(action);
        action->addReference();
        island->removeAction(action);
        island->m_actionListCleanupNeeded = 1;
        putIslandOnDirtyList(world, island);
    }
}

// The island is the entity's at entry. An action landing on the fixed island is only added back; otherwise each of
// its other entities that is not fixed, while the entity is not fixed either, merges its island with the entity's
// current one.
void hkpWorldOperationUtil::addActionsToEntitysIsland(
    hkpWorld* world,
    hkpEntity* entity,
    hkArray<hkpAction*>& actionsToBeMoved
) {
    hkpSimulationIsland* island = entity->m_simulationIsland;
    hkInplaceArray<hkpEntity*, 16> entities;
    for (int32_t i = 0; i < actionsToBeMoved.m_size; ++i) {
        hkpAction* action = actionsToBeMoved.m_data[i];
        if (island == world->m_fixedIsland) {
            island->addAction(action);
            action->removeReference();
            continue;
        }
        island->addAction(action);
        action->removeReference();
        entities.m_size = 0;
        action->getEntities(entities);
        for (hkpEntity* other : entities) {
            if (other != entity &&
                other->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
                entity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
                other->m_simulationIsland != entity->m_simulationIsland) {
                mergeIslands(other->m_world, other, entity);
            }
        }
    }
}

void hkpWorldOperationUtil::validateWorld(hkpWorld* world) {
    int32_t i;
    hkpSimulationIsland* v2;
    int32_t j;
    hkpSimulationIsland* v4;
    hkpSimulationIsland* m_fixedIsland;
    for (i = world->m_activeSimulationIslands.m_size - 1; i >= 0; --i) {
        v2 = world->m_activeSimulationIslands.m_data[i];
        v2->isValid();
    }
    for (j = world->m_inactiveSimulationIslands.m_size - 1; j >= 0; --j) {
        v4 = world->m_inactiveSimulationIslands.m_data[j];
        v4->isValid();
    }
    m_fixedIsland = world->m_fixedIsland;
    m_fixedIsland->isValid();
}

void hkpWorldOperationUtil::removeAttachedActionsFromFixedIsland(
    hkpWorld* world,
    hkpEntity* entity,
    hkArray<hkpAction*, hkContainerHeapAllocator>& actionsToBeMoved
) {
    hkpEntity* v3 = entity;
    int32_t v4 = 0;
    int32_t v9 = 0;
    if (entity->m_actions.m_size) {
        do {
            hkpAction* v5 = v3->m_actions.m_data[v4];
            if (v5->m_island == world->m_fixedIsland) {
                actionsToBeMoved.pushBack(v5);
                v5->addReference();
                world->m_fixedIsland->removeAction(v5);
                world->m_fixedIsland->m_actionListCleanupNeeded = 1;
                hkpSimulationIsland* m_fixedIsland = world->m_fixedIsland;
                if (m_fixedIsland->m_dirtyListIndex == 0xFFFF) {
                    m_fixedIsland->m_dirtyListIndex = world->m_dirtySimulationIslands.m_size;
                    world->m_dirtySimulationIslands.pushBack(m_fixedIsland);
                }
            }
            v3 = entity;
            v4 = ++v9;
        }
        while (v9 < entity->m_actions.m_size);
    }
}

// While the world's critical operations are locked, an UpdateEntityBP operation is queued; otherwise, under the world's
// lock, an entity with a shape has its box recomputed where it is invalid, the broad phase moves its expanded box, and
// the pairs it reports are dispatched. The two pair arrays are stack-allocated hkLocalArrays of m_broadPhaseQuerySize
// pairs each. hkpWorldOperationQueue::queueOperation copies a whole BiggestOperation from the UpdateEntityBP local,
// whose padding after m_type and the storage past its end are never written.
void hkpWorldOperationUtil::updateEntityBP(hkpWorld* world, hkpEntity* entity) {
    if (world->m_criticalOperationsLockCount) {
        hkWorldOperation::UpdateEntityBP op;
        op.m_type = hkWorldOperation::ENTITY_UPDATE_BROAD_PHASE;
        op.m_entity = entity;
        world->queueOperation(op);
        return;
    }
    world->lockCriticalOperations();

    hkpLinkedCollidable* pCollidable = &entity->m_collidable;
    if (pCollidable->m_shape) {
        const int32_t iNewCapacity = world->m_broadPhaseQuerySize;
        alignas(16) hkLocalArray<hkpBroadPhaseHandlePair> newPairs(iNewCapacity);

        const int32_t iDelCapacity = world->m_broadPhaseQuerySize;
        alignas(16) hkLocalArray<hkpBroadPhaseHandlePair> delPairs(iDelCapacity);

        if (!pCollidable->m_boundingVolumeData.isValid()) {
            hkpEntityAabbUtil::entityBatchRecalcAabb(world->m_collisionInput, &entity, 1);
        }
        // The broad phase gets the box grown by its expansion; the box's expansion bytes are deliberately left
        // unwritten.
        hkAabbUint32 aabb;
        hkVector4Util::uncompressExpandedAabbUint32(
            reinterpret_cast<const hkAabbUint32&>(pCollidable->m_boundingVolumeData),
            aabb
        );
        hkpBroadPhaseHandle* pHandle = pCollidable->getBroadPhaseHandle();
        world->getBroadPhase()->updateAabbsUint32(&pHandle, &aabb, 1, newPairs, delPairs);
        if (newPairs.m_size + delPairs.m_size > 0) {
            hkpTypedBroadPhaseDispatcher::removeDuplicates(newPairs, delPairs);
            world->m_broadPhaseDispatcher->removePairs(
                static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.m_data),
                delPairs.m_size
            );
            const hkpCollidableCollidableFilter* pFilter = static_cast<const hkpCollidableCollidableFilter*>(
                world->m_collisionFilter
            );
            world->m_broadPhaseDispatcher
                ->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.m_data), newPairs.m_size, pFilter);
        }
    }
    world->unlockAndAttemptToExecutePendingOperations();
}

void hkpWorldOperationUtil::removeConstraintFromCriticalLockedIsland(
    hkpWorld* world,
    hkpConstraintInstance* constraint
) {
    if (world->m_constraintListeners.m_size) {
        hkpWorldCallbackUtil::fireConstraintRemoved(world, constraint);
    }
    hkpWorldConstraintUtil::removeConstraint(constraint);
}

// Moves the agents linking the entity to a fixed partner, and, when the entity becomes fixed, those whose island is
// the entity's, out of trackToScan: into agentsRemoved, or into the partner's island track.
void hkpWorldOperationUtil::removeAttachedAgentsConnectingTheEntityAndAFixedPartnerEntityPlus(
    hkpAgentNnTrack& trackToScan,
    hkpEntity* entity,
    hkpAgentNnTrack& agentsRemoved,
    hkpMotion::MotionType newMotionType
) {
    const hkArray<hkpLinkedCollidable::CollisionEntry, hkContainerHeapAllocator>&
        collisionEntries = entity->m_collidable.getCollisionEntriesDeterministicUnchecked();
    for (int32_t i = 0; i < collisionEntries.m_size; ++i) {
        const hkpLinkedCollidable::CollisionEntry& entry = collisionEntries.m_data[i];
        hkpEntity* partner = static_cast<hkpEntity*>(entry.m_partner->getOwner());
        if (partner->isFixed().m_bool) {
            hkpAgentNnEntry* agentEntry = entry.m_agentEntry;
            hkAgentNnMachine_CopyAndRelinkAgentEntry(agentsRemoved, agentEntry);
            hkAgentNnMachine_InternalDeallocateEntry(trackToScan, agentEntry);
        }
        else if (
            newMotionType == hkpMotion::MOTION_FIXED &&
            hkpWorldAgentUtil::getIslandFromAgentEntry(
                entry.m_agentEntry,
                entity->getSimulationIsland(),
                partner->getSimulationIsland()
            ) == entity->getSimulationIsland()
        ) {
            hkpAgentNnEntry* agentEntry = entry.m_agentEntry;
            hkAgentNnMachine_CopyAndRelinkAgentEntry(partner->getSimulationIsland()->m_agentTrack, agentEntry);
            hkAgentNnMachine_InternalDeallocateEntry(trackToScan, agentEntry);
        }
    }
}

// Merges the smaller island into the larger one (on equal sizes into the one at the lower storage index) and deletes
// the smaller one.
void hkpWorldOperationUtil::internalMergeTwoIslands(
    hkpWorld* world,
    hkpSimulationIsland* islandA,
    hkpSimulationIsland* islandB
) {
    if (islandA->m_entities.m_size <= islandB->m_entities.m_size &&
        (islandA->m_entities.m_size < islandB->m_entities.m_size ||
         islandA->m_storageIndex > islandB->m_storageIndex)) {
        hkpSimulationIsland* swap = islandA;
        islandA = islandB;
        islandB = swap;
    }
    world->lockCriticalOperations();
    bool isActive = islandA->m_isInActiveIslandsArray != 0 || islandB->m_isInActiveIslandsArray != 0;
    bool activeMark = islandA->m_activeMark != 0 || islandB->m_activeMark != 0;
    if (isActive) {
        if (islandA->m_isInActiveIslandsArray == 0) {
            islandA->m_activeMark = 1;
            internalActivateIsland(world, islandA, hkBool(false));
        }
        else if (islandB->m_isInActiveIslandsArray == 0) {
            islandB->m_activeMark = 1;
            internalActivateIsland(world, islandB, hkBool(false));
        }
    }
    islandA->m_sparseEnabled |= islandB->m_sparseEnabled;
    islandA->markForWrite();
    islandB->markForWrite();
    world->m_fixedIsland->markForWrite();
    hkAgentNnMachine_AppendTrack(islandA->m_agentTrack, islandB->m_agentTrack);

    // The entities: the destination index is kept as an hkObjectIndex (16 bits).
    uint16_t entityIndex = static_cast<uint16_t>(islandA->m_entities.m_size);
    int32_t numEntities = islandA->m_entities.m_size + islandB->m_entities.m_size;
    islandA->m_entities.setSize(numEntities);
    for (int32_t i = 0; i < islandB->m_entities.m_size; ++i, ++entityIndex) {
        islandA->m_entities.m_data[entityIndex] = islandB->m_entities.m_data[i];
        islandB->m_entities.m_data[i]->m_simulationIsland = islandA;
        islandB->m_entities.m_data[i]->m_storageIndex = entityIndex;
    }

    // The actions, without the null entries: setSize to the sum, then back to the count copied.
    int32_t numActions = islandA->m_actions.m_size;
    int32_t maxActions = numActions + islandB->m_actions.m_size;
    hkArray<hkpAction*, hkContainerHeapAllocator>* actions = &islandA->m_actions;
    actions->setSize(maxActions);
    for (hkpAction* action : islandB->m_actions) {
        if (action) {
            actions->m_data[numActions] = action;
            actions->m_data[numActions]->m_island = islandA;
            ++numActions;
        }
    }
    actions->setSize(numActions);

    // The constraints the moved entities master now belong to islandA.
    for (hkpEntity* entity : islandB->m_entities) {
        hkConstraintInternal* constraint = entity->m_constraintsMaster.begin();
        for (int32_t j = entity->m_constraintsMaster.m_size - 1; j >= 0; --j, ++constraint) {
            constraint->m_constraint->m_owner = islandA;
        }
    }
    islandA->mergeConstraintInfo(*islandB);
    islandA->m_numConstraints += islandB->m_numConstraints;

    // Take islandB out of its array.
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>* islands = islandB->m_isInActiveIslandsArray != 0
        ? &world->m_activeSimulationIslands
        : &world->m_inactiveSimulationIslands;
    int32_t storageIndex = islandB->m_storageIndex;
    islandB->m_numConstraints = 0;
    int32_t last = islands->m_size - 1;
    if (storageIndex < last) {
        islands->m_data[storageIndex] = islands->m_data[last];
        islands->m_data[storageIndex]->m_storageIndex = static_cast<uint16_t>(storageIndex);
    }
    --islands->m_size;

    islandA->m_splitCheckRequested = islandA->m_splitCheckRequested != 0 || islandB->m_splitCheckRequested != 0;
    islandA->m_actionListCleanupNeeded = islandA->m_actionListCleanupNeeded != 0 ||
        islandB->m_actionListCleanupNeeded != 0;
    islandA->m_activeMark = activeMark;
    if (islandB->m_dirtyListIndex != 0xFFFF && islandA->m_dirtyListIndex == 0xFFFF) {
        islandA->m_dirtyListIndex = world->m_dirtySimulationIslands.m_size;
        world->m_dirtySimulationIslands.pushBack(islandA);
    }
    hkpWorldOperationUtil::removeIslandFromDirtyList(world, islandB);
    delete islandB;

    // Keep the largest active island first.
    if (islandA->m_isInActiveIslandsArray != 0) {
        int32_t indexA = islandA->getStorageIndex();
        if (indexA) {
            hkpSimulationIsland* firstIsland = *world->m_activeSimulationIslands.begin();
            if (islandA->m_entities.m_size > firstIsland->m_entities.m_size) {
                islandA->m_storageIndex = 0;
                firstIsland->m_storageIndex = indexA;
                *world->m_activeSimulationIslands.begin() = islandA;
                world->m_activeSimulationIslands.m_data[indexA] = firstIsland;
            }
        }
    }
    world->unlockAndAttemptToExecutePendingOperations();
}

void hkpWorldOperationUtil::removeAttachedConstraints(
    hkpEntity* entity,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& removedConstraints
) {
    hkpEntity* v2 = entity;
    int32_t v3 = entity->m_constraintsMaster.m_size - 1;
    hkpWorld* world = entity->m_world;
    if (v3 >= 0) {
        int32_t v4 = v3;
        do {
            hkpConstraintInstance* m_constraint = v2->m_constraintsMaster.m_data[v4].m_constraint;
            m_constraint->addReference();
            hkpWorldOperationUtil::removeConstraintImmediately(
                world,
                m_constraint,
                hkpWorldOperationUtil::DO_NOT_FIRE_CALLBACKS_AND_SUPPRESS_EXECUTION_OF_PENDING_OPERATIONS
            );
            removedConstraints.pushBack(m_constraint);
            v2 = entity;
            --v3;
            --v4;
        }
        while (v3 >= 0);
    }
    int32_t v7 = v2->m_constraintsSlave.m_size - 1;
    if (v7 >= 0) {
        while (true) {
            hkpConstraintInstance* v8 = v2->m_constraintsSlave.m_data[v7];
            v8->addReference();
            hkpWorldOperationUtil::removeConstraintImmediately(
                world,
                v8,
                hkpWorldOperationUtil::DO_NOT_FIRE_CALLBACKS_AND_SUPPRESS_EXECUTION_OF_PENDING_OPERATIONS
            );
            removedConstraints.pushBack(v8);
            if (--v7 < 0) {
                break;
            }
            v2 = entity;
        }
    }
}

void hkpWorldOperationUtil::markIslandInactiveMt(hkpWorld* world, hkpSimulationIsland* island) {
    island->m_activeMark = 0;
    EnterCriticalSection(&world->m_islandDirtyListCriticalSection->m_section);
    if (island->m_dirtyListIndex == 0xFFFF) {
        island->m_dirtyListIndex = world->m_dirtySimulationIslands.m_size;
        world->m_dirtySimulationIslands.pushBack(island);
    }
    LeaveCriticalSection(&world->m_islandDirtyListCriticalSection->m_section);
}

// Splits the island into one island per connected group of entities (the island keeps the biggest group), moving
// entities, constraints, actions and agents into the new islands; the new islands are appended to newIslandsOut and the
// island's old entity array goes to oldEntitiesOut when given.
void hkpWorldOperationUtil::splitSimulationIsland(
    hkpSimulationIsland* currentIsland,
    hkpWorld* world,
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& newIslandsOut,
    hkArray<hkpEntity*, hkContainerHeapAllocator>* oldEntitiesOut
) {
    hkLocalBuffer<int32_t> entityStatus(currentIsland->m_entities.m_size, nullptr);
    hkUnionFind checkConnectivity(&entityStatus, currentIsland->m_entities.m_size);
    if (currentIsland->isFullyConnected(checkConnectivity)) {
        currentIsland->m_sparseEnabled = 0;
        return;
    }

    hkInplaceArray<int32_t, 32, hkContainerHeapAllocator> elementsPerGroup;
    checkConnectivity.assignGroups(elementsPerGroup);

    // Per group, whether its island is sparse; one byte more than there are groups.
    hkLocalBuffer<int8_t> isSparse(elementsPerGroup.m_size + 1, nullptr);

    bool bMergeSmallGroups = false;
    bool bSplit = true;
    if (!currentIsland->m_sparseEnabled) {
        if (world->m_minDesiredIslandSize == 0) {
            hkString::memSet(isSparse.m_data.m_storage, 0, elementsPerGroup.m_size + 1);
        }
        else {
            bMergeSmallGroups = true;
        }
    }
    else if (!world->m_wantDeactivation.m_bool) {
        bMergeSmallGroups = true;
    }
    else {
        // Group 0 gathers the groups with an entity active in the last five frames, as one sparse island; every
        // other group gets its own.
        identifyDeactivatedSubgroups(checkConnectivity, currentIsland->m_entities, world, elementsPerGroup, isSparse);
        if (elementsPerGroup.m_size == 1) {
            bSplit = false;
        }
    }

    if (bMergeSmallGroups) {
        // Joins groups smaller than the world's minimum island size into sparse islands: each group's size is its
        // entities' constraint count (at least one per entity), -1 for a group with no recently active entity (with
        // deactivation), -2 once merged into an earlier group.
        mergeSmallSubgroups(world, checkConnectivity, currentIsland->m_entities, elementsPerGroup, isSparse);
    }

    if (bSplit) {
        const int32_t biggestGroup = checkConnectivity.moveBiggestGroupToIndexZero(elementsPerGroup);
        const int8_t biggestSparse = isSparse[biggestGroup];
        const int8_t firstSparse = isSparse[0];
        isSparse[0] = biggestSparse;
        isSparse[biggestGroup] = firstSparse;

        const int32_t numEntities = currentIsland->m_entities.m_size;
        const int32_t numEntitiesStaying = elementsPerGroup.m_data[0];
        const int32_t numGroups = elementsPerGroup.m_size;
        if (numGroups != 1) {
            // The island of each group: the current island keeps group 0.
            hkLocalBuffer<hkpSimulationIsland*> islands(numGroups, nullptr);
            currentIsland->m_sparseEnabled = isSparse[0] != 0;
            islands[0] = currentIsland;
            for (int32_t g = 1; g < numGroups; ++g) {
                hkpSimulationIsland* newIsland = new hkpSimulationIsland(world);
                newIsland->m_isInActiveIslandsArray = 1;
                newIsland->m_activeMark = 1;
                newIsland->m_sparseEnabled = isSparse[g] != 0;
                newIsland->m_storageIndex = static_cast<uint16_t>(newIslandsOut.m_size);
                newIslandsOut.pushBack(newIsland);
                islands[g] = newIsland;
                // Reserves exactly the group's size (no doubling).
                newIsland->m_entities.reserveExactly(elementsPerGroup.m_data[g]);
            }

            // Entities: each goes to its group's island, its constraints with it.
            hkpEntity** oldEntities = currentIsland->m_entities.m_data;
            const int32_t numOldEntities = currentIsland->m_entities.m_size;
            if (oldEntitiesOut) {
                currentIsland->m_entities.swap(*oldEntitiesOut);
                currentIsland->m_entities.clear();
                currentIsland->m_entities.reserveExactly(elementsPerGroup.m_data[0]);
            }
            else {
                currentIsland->m_entities.clear();
            }
            for (int32_t e = 0; e < numOldEntities; ++e) {
                hkpSimulationIsland* newIsland = islands[entityStatus[e]];
                hkpEntity* entity = oldEntities[e];
                entity->m_simulationIsland = newIsland;
                entity->m_storageIndex = static_cast<uint16_t>(newIsland->m_entities.m_size);
                newIsland->m_entities.pushBackUnchecked(entity);
                if (currentIsland == newIsland) {
                    continue;
                }
                hkConstraintInternal* ci = entity->m_constraintsMaster.m_data;
                for (int32_t c = 0; c < entity->m_constraintsMaster.m_size; ++c, ++ci) {
                    hkpConstraintInfo delta;
                    delta.m_maxSizeOfSchema = currentIsland->m_constraintInfo.m_maxSizeOfSchema;
                    delta.m_sizeOfSchemas = ci->m_sizeOfSchemas;
                    delta.m_numSolverResults = ci->m_numSolverResults;
                    delta.m_numSolverElemTemps = ci->m_numSolverElemTemps;
                    --currentIsland->m_numConstraints;
                    currentIsland->subConstraintInfo(ci->m_constraint, delta);
                    ++newIsland->m_numConstraints;
                    ci->m_constraint->m_owner = newIsland;
                    newIsland->addConstraintInfo(ci->m_constraint, delta);
                }
            }

            // Actions: each goes to the island of its first entity that is not in the fixed island (the last
            // entity's island when all are).
            hkpAction** oldActions = currentIsland->m_actions.m_data;
            hkpSimulationIsland* fixedIsland = world->getFixedIsland();
            const int32_t numOldActions = currentIsland->m_actions.m_size;
            currentIsland->m_actions.clear();
            for (int32_t a = 0; a < numOldActions; ++a) {
                hkpAction* action = oldActions[a];
                if (!action) {
                    continue;
                }
                hkInplaceArray<hkpEntity*, 16, hkContainerHeapAllocator> entities;
                action->getEntities(entities);
                hkpSimulationIsland* actionIsland = nullptr;
                for (hkpEntity* entity : entities) {
                    actionIsland = entity->m_simulationIsland;
                    if (actionIsland != fixedIsland) {
                        break;
                    }
                }
                action->m_island = actionIsland;
                actionIsland->m_actions.pushBack(action);
            }

            // Agents: when an eighth or more of the entities leave, every entry is relinked into its island's track and
            // the old sectors are freed; otherwise only the leaving entries are moved out of the track. A sector's
            // entries are untyped bytes walked entry by entry, m_size bytes on each time, hence the reinterpret_casts
            // between the sector's bytes and the entries.
            world->lockForIslandSplit(currentIsland);
            hkpAgentNnTrack& track = currentIsland->m_agentTrack;
            if ((numEntities - numEntitiesStaying) * 8 >= numEntities) {
                hkpAgentNnTrack oldTrack;
                oldTrack.m_bytesUsedInLastSector = sizeof(hkpAgentNnSector);
                // Takes the island's sectors: a single one is copied into the inline storage, more are swapped in
                // after the local array is moved to the heap.
                if (track.m_sectors.m_size == 1) {
                    oldTrack.m_sectors.m_data[0] = track.m_sectors.m_data[0];
                    oldTrack.m_sectors.m_size = 1;
                    track.m_sectors.m_size = 0;
                }
                else if (track.m_sectors.m_size > 1) {
                    oldTrack.m_sectors.reserveExactly(2);
                    oldTrack.m_sectors.swap(track.m_sectors);
                }
                const uint32_t bytesUsedInLastSector = track.m_bytesUsedInLastSector;
                track.m_bytesUsedInLastSector = oldTrack.m_bytesUsedInLastSector;
                oldTrack.m_bytesUsedInLastSector = bytesUsedInLastSector;
                for (int32_t s = 0; s < oldTrack.m_sectors.m_size; ++s) {
                    hkpAgentNnSector* sector = oldTrack.m_sectors.m_data[s];
                    const int32_t sectorSize = oldTrack.getSectorSize(s);
                    hkpAgentNnEntry* end = reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[sectorSize]);
                    for (hkpAgentNnEntry* entry = sector->getBegin(); entry < end;) {
                        hkpSimulationIsland* island = static_cast<hkpEntity*>(entry->m_collidable[0]->getOwner())
                                                          ->m_simulationIsland;
                        if (island->isFixed().m_bool) {
                            island = static_cast<hkpEntity*>(entry->m_collidable[1]->getOwner())->m_simulationIsland;
                        }
                        hkAgentNnMachine_CopyAndRelinkAgentEntry(island->m_agentTrack, entry);
                        entry = hkAddByteOffset(entry, entry->m_size);
                    }
                    hkMemoryRouter::getInstance().m_heap->blockFree(sector, sizeof(hkpAgentNnSector));
                }
            }
            else {
                for (int32_t s = 0; s < track.m_sectors.m_size; ++s) {
                    hkpAgentNnSector* sector = track.m_sectors.m_data[s];
                    hkpAgentNnEntry* entry = sector->getBegin();
                    while (entry < reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[track.getSectorSize(s)])) {
                        hkpSimulationIsland* island = static_cast<hkpEntity*>(entry->m_collidable[0]->getOwner())
                                                          ->m_simulationIsland;
                        if (island->isFixed().m_bool) {
                            island = static_cast<hkpEntity*>(entry->m_collidable[1]->getOwner())->m_simulationIsland;
                        }
                        if (island == currentIsland) {
                            entry = hkAddByteOffset(entry, entry->m_size);
                            continue;
                        }
                        // Deallocating moves the track's last entry into this one's place: look at the same offset
                        // again, in the sector now at this index, unless the sector went.
                        const int32_t offset = static_cast<int32_t>(reinterpret_cast<uint8_t*>(entry) - sector->m_data);
                        hkAgentNnMachine_CopyAndRelinkAgentEntry(island->m_agentTrack, entry);
                        hkAgentNnMachine_InternalDeallocateEntry(track, entry);
                        if (s >= track.m_sectors.m_size) {
                            break;
                        }
                        sector = track.m_sectors.m_data[s];
                        entry = reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[offset]);
                    }
                }
            }
            world->unlockForIslandSplit(currentIsland);
            track.m_sectors.optimizeCapacity(4);
            for (int32_t g = 1; g < numGroups; ++g) {
                islands[g]->m_splitCheckFrameCounter = static_cast<uint8_t>(
                    currentIsland->m_splitCheckFrameCounter + g
                );
            }
        }
    }
}

// Havok SDK: changes the body's motion type. In a world, a body entering or leaving the fixed state has its
// constraints, actions and agents taken off, moves between the fixed and a dynamic island, and gets them back;
// otherwise the motion is replaced in place.
void hkpWorldOperationUtil::setRigidBodyMotionType(
    hkpRigidBody* body,
    hkpMotion::MotionType newState,
    hkpEntityActivation initialActivationState,
    hkpUpdateCollisionFilterOnEntityMode collisionFilterUpdateMode
) {
    const int32_t oldState = body->m_motion.m_type.m_storage;
    if (oldState == newState) {
        return;
    }
    if (oldState == hkpMotion::MOTION_FIXED) {
        body->m_collidable.sortEntries();
        body->sortConstraintsSlavesDeterministically();
    }
    hkBool newStateNeedsInertia;
    newStateNeedsInertia.m_bool = newState != hkpMotion::MOTION_FIXED && newState != hkpMotion::MOTION_KEYFRAMED;
    hkBool oldStateNeedsInertia;
    oldStateNeedsInertia.m_bool = oldState != hkpMotion::MOTION_FIXED && oldState != hkpMotion::MOTION_KEYFRAMED;
    // A fixed or keyframed body becomes dynamic only with a saved motion to take its inertia from.
    if (newStateNeedsInertia.m_bool && !oldStateNeedsInertia.m_bool && body->m_motion.m_savedMotion == nullptr) {
        return;
    }

    body->addReference();
    hkpWorld* world = body->m_world;
    if (world == nullptr || (oldState == hkpMotion::MOTION_FIXED) == (newState == hkpMotion::MOTION_FIXED)) {
        replaceMotionObject(body, newState, newStateNeedsInertia, oldStateNeedsInertia, world);
        if (world != nullptr) {
            world->updateCollisionFilterOnEntity(
                body,
                collisionFilterUpdateMode,
                HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
            );
        }
        body->removeReference();
        return;
    }

    hkInplaceArray<hkpConstraintInstance*, 16> removedConstraints;
    hkInplaceArray<hkpAction*, 16> actionsToBeMoved;
    world->m_blockExecutingPendingOperations.m_bool = 1;
    world->m_criticalOperationsAllowed.m_bool = 0;
    hkpAgentNnTrack agentsRemoved;
    agentsRemoved.m_bytesUsedInLastSector = HK_AGENT3_SECTOR_SIZE;

    removeAttachedConstraints(body, removedConstraints);
    if (newState == hkpMotion::MOTION_FIXED) {
        removeAttachedActionsFromDynamicIsland(world, body, actionsToBeMoved);
    }
    else {
        removeAttachedActionsFromFixedIsland(world, body, actionsToBeMoved);
    }
    removeAttachedAgentsConnectingTheEntityAndAFixedPartnerEntityPlus(
        body->getSimulationIsland()->m_agentTrack,
        body,
        agentsRemoved,
        newState
    );
    if (oldState != hkpMotion::MOTION_FIXED) {
        hkpSimulationIsland* island = body->getSimulationIsland();
        if (island->m_entities.m_size > 2) {
            island->m_splitCheckRequested = 1;
        }
    }
    removeEntitySI(world, body);
    replaceMotionObject(body, newState, newStateNeedsInertia, oldStateNeedsInertia, world);
    addEntitySI(world, body, initialActivationState);
    addActionsToEntitysIsland(world, body, actionsToBeMoved);
    world->m_criticalOperationsAllowed.m_bool = 1;

    hkArray<hkpLinkedCollidable::CollisionEntry> entries;
    for (hkpConstraintInstance* constraint : removedConstraints) {
        addConstraintImmediately(world, constraint, DO_NOT_FIRE_CALLBACKS_AND_SUPPRESS_EXECUTION_OF_PENDING_OPERATIONS);
        constraint->removeReference();
    }
    // A body that is not fixed now merges its island with each non-fixed partner's.
    if (newState != hkpMotion::MOTION_FIXED) {
        body->m_collidable.getCollisionEntriesSorted(entries);
        for (const hkpLinkedCollidable::CollisionEntry& entry : entries) {
            hkpEntity* partner = static_cast<hkpEntity*>(entry.m_partner->getOwner());
            if (body->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
                partner->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED &&
                body->getSimulationIsland() != partner->getSimulationIsland()) {
                mergeIslands(body->m_world, body, partner);
            }
        }
    }
    hkAgentNnMachine_AppendTrack(body->getSimulationIsland()->m_agentTrack, agentsRemoved);
    if (newState == hkpMotion::MOTION_FIXED) {
        float startTime = world->m_dynamicsStepInfo.m_stepInfo.m_startTime;
        hkSweptTransformUtil::freezeMotionState(startTime, body->m_motion.m_motionState);
        hkpSimulation* simulation = world->m_simulation;
        world->lockCriticalOperations();
        hkpEntity* entity = body;
        simulation->resetCollisionInformationForEntities(&entity, 1, world, hkpSimulation::RESET_ALL);
        hkpSimulation::collideEntitiesBroadPhaseDiscrete(&entity, 1, world);
        hkpWorldCallbackUtil::fireInactiveEntityMoved(body->m_world, body);
        world->unlockCriticalOperations();
    }
    world->updateCollisionFilterOnEntity(
        body,
        collisionFilterUpdateMode,
        HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
    );
    world->m_blockExecutingPendingOperations.m_bool = 0;
    world->attemptToExecutePendingOperations();

    body->removeReference();
}

// The SDK's replaceMotionObject: switches the body's motion in place between a motion that needs inertia (dynamic)
// and one that does not (fixed, keyframed). A fixed or keyframed motion keeps the old dynamic motion as its saved
// motion (a copy, or the old motion's own saved one); going back to a dynamic motion restores the saved one first and
// then builds a dynamic motion of the new type from it where the type changes.
void hkpWorldOperationUtil::replaceMotionObject(
    hkpRigidBody* body,
    hkpMotion::MotionType newState,
    hkBool newStateNeedsInertia,
    hkBool oldStateNeedsInertia,
    hkpWorld* world
) {
    hkpMaxSizeMotion* motion = &body->m_motion;
    if (newStateNeedsInertia.m_bool) {
        if (!oldStateNeedsInertia.m_bool) {
            hkpMaxSizeMotion* savedMotion = motion->m_savedMotion;
            motion->getMotionStateAndVelocitiesAndDeactivationType(savedMotion);
            savedMotion->m_deactivationNumInactiveFrames[0] = 0;
            savedMotion->m_deactivationNumInactiveFrames[1] = 0;
            body->m_collidable.setQualityType(hkpCollidableQualityType(motion->m_savedQualityTypeIndex));
            hkString::memCpy16NonEmpty(motion, savedMotion, sizeof(hkpMaxSizeMotion) >> 4);
            // Clears the bytes after the last 16-byte block of the motion.
            hkString::memSet(motion + 1, 0, sizeof(hkpMaxSizeMotion) & 15);
            savedMotion->removeReference();
            body->setCachedShapeData(world, body->m_collidable.m_shape);
        }
        if (motion->m_type.m_storage != newState && newState != hkpMotion::MOTION_DYNAMIC) {
            // Destroyed at the branch's end by hkpKeyframedRigidMotion's destructor.
            hkpMaxSizeMotion oldMotion;
            hkString::memCpy16NonEmpty(&oldMotion, motion, sizeof(hkpMaxSizeMotion) >> 4);
            hkMatrix3 inertiaLocal;
            motion->getInertiaLocal(inertiaLocal);
            float maxAngularVelocity = oldMotion.m_motionState.m_maxAngularVelocity;
            float maxLinearVelocity = oldMotion.m_motionState.m_maxLinearVelocity;
            hkpRigidBody::createDynamicRigidMotion(
                newState,
                oldMotion.getPosition(),
                oldMotion.getRotation(),
                oldMotion.getMass(),
                inertiaLocal,
                oldMotion.getCenterOfMassLocal(),
                maxLinearVelocity,
                maxAngularVelocity,
                motion
            );
            oldMotion.hkpMotion::getMotionStateAndVelocitiesAndDeactivationType(motion);
            motion->m_motionState.m_linearDamping = oldMotion.m_motionState.m_linearDamping;
            motion->m_motionState.m_angularDamping = oldMotion.m_motionState.m_angularDamping;
        }
    }
    else {
        // Destroyed at the branch's end by hkpKeyframedRigidMotion's destructor.
        hkpMaxSizeMotion oldMotion;
        hkString::memCpy16NonEmpty(&oldMotion, motion, sizeof(hkpMaxSizeMotion) >> 4);
        hkpKeyframedRigidMotion* newMotion;
        if (newState == hkpMotion::MOTION_FIXED) {
            // The SDK's placement new, with its null test.
            newMotion = motion ? ::new (motion) hkpFixedRigidMotion(motion->getPosition(), motion->getRotation())
                               : nullptr;
            newMotion->m_motionState.Copy(&oldMotion.m_motionState);
            newMotion->m_deactivationIntegrateCounter = oldMotion.m_deactivationIntegrateCounter;
            // The swept transform's w lanes: the inverse delta time (centre of mass 1) and the base time (0).
            const hkSweptTransform& sweptTransform = oldMotion.m_motionState.m_sweptTransform;
            float invDeltaTime = XFNV_M128_F32(sweptTransform.m_centerOfMass1.m_quad, 3);
            if (invDeltaTime != 0.0f) {
                float time = world ? world->getCurrentTime()
                                   : 1.0f / invDeltaTime + XFNV_M128_F32(sweptTransform.m_centerOfMass0.m_quad, 3);
                hkSweptTransformUtil::freezeMotionState(time, newMotion->m_motionState);
            }
        }
        else {
            // The SDK's placement new, with its null test.
            newMotion = motion ? ::new (motion) hkpKeyframedRigidMotion(motion->getPosition(), motion->getRotation())
                               : nullptr;
            oldMotion.hkpMotion::getMotionStateAndVelocitiesAndDeactivationType(newMotion);
        }
        if (oldStateNeedsInertia.m_bool) {
            hkpMaxSizeMotion* savedMotion = new hkpMaxSizeMotion;
            hkString::memCpy16NonEmpty(savedMotion, &oldMotion, sizeof(hkpMaxSizeMotion) >> 4);
            savedMotion->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpMaxSizeMotion));
            savedMotion->m_referenceCount = 1;
            newMotion->m_savedMotion = savedMotion;
            newMotion->m_savedQualityTypeIndex = static_cast<uint16_t>(body->m_collidable.getQualityType());
            body->setCachedShapeData(world, body->m_collidable.m_shape);
        }
        else {
            newMotion->setStoredMotion(oldMotion.m_savedMotion);
            newMotion->m_savedQualityTypeIndex = oldMotion.m_savedQualityTypeIndex;
        }
        body->m_solverData = 0;
        body->m_collidable.setQualityType(
            newState == hkpMotion::MOTION_FIXED ? HK_COLLIDABLE_QUALITY_FIXED : HK_COLLIDABLE_QUALITY_KEYFRAMED
        );
    }
    body->m_collidable.m_motion.pState = motion->getMotionState();
    if (world) {
        motion->setWorldSelectFlagsNeg(
            world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[0],
            world->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[1],
            world->m_dynamicsStepInfo.m_solverInfo.m_deactivationIntegrateCounter
        );
    }
}

void hkpWorldOperationUtil::splitSimulationIslands(hkpWorld* world) {
    if (world->m_wantSimulationIslands) {
        for (int32_t i = world->m_activeSimulationIslands.m_size - 1; i >= 0; --i) {
            if (world->m_activeSimulationIslands.m_data[i]->m_splitCheckRequested != 0) {
                hkpWorldOperationUtil::splitSimulationIsland(world, world->m_activeSimulationIslands.m_data[i]);
            }
        }
    }
}

// The entry slot lies in the sector's untyped storage (hkpAgentNnSector::getBegin's form), at the used bytes less one
// slot.
hkpAgentNnEntry* getLastAgentEntry(hkpAgentNnTrack* track) {
    hkpAgentNnSector* lastSector = track->m_sectors.m_data[track->m_sectors.m_size - 1];
    return reinterpret_cast<hkpAgentNnEntry*>(
        &lastSector->m_data[static_cast<int32_t>(track->m_bytesUsedInLastSector) - HK_AGENT3_AGENT_SIZE]
    );
}
