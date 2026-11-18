#pragma once

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

class hkpAgentNnTrack;
class hkpConstraintInstance;
class hkpEntity;
class hkpPhantom;
class hkpRigidBody;
class hkpSimulationIsland;
class hkUnionFind;

// Havok SDK: joins the groups smaller than the world's minimum island size into sparse ones.
void mergeSmallSubgroups(
    hkpWorld* world,
    hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    hkArray<int32_t, hkContainerHeapAllocator>& groupsSizes,
    hkFixedArray<int8_t>& groupSparse
);
// Marks the group of each entity that has been inactive for at most 5 frames active, and adds the entity's solver
// size (its constraint count, at least 1) to its group's.
void findInactiveGroupsAndEstimateSolverSize(
    const hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    hkFixedArray<int32_t>& isActive,
    hkFixedArray<int32_t>& sizes,
    const uint8_t* deactivationNumInactiveFramesSelectFlag
);
// Marks each entity's group active and adds the entity's solver size (at least 1) to the group's.
void estimateSolverSize(
    const hkUnionFind& checker,
    hkFixedArray<int32_t>& sizes,
    hkFixedArray<int32_t>& isActive,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities
);
// Looks at the newest islands of the array, at most 21 of them: the first one that stays under the world's minimum
// island size with islandSizeAddon more entities is marked sparse and returned. islandToIgnore is compared with each
// island.
hkpSimulationIsland* findSmallSparseActiveIsland(
    hkpWorld* world,
    hkArray<hkpSimulationIsland*>& islands,
    int32_t islandSizeAddon,
    hkpSimulationIsland* islandToIgnore
);
void findInactiveGroups(
    const hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    const hkpWorld* world,
    int32_t numGroups,
    hkFixedArray<int32_t>& isActive
);
void identifyDeactivatedSubgroups(
    hkUnionFind& checker,
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& entities,
    const hkpWorld* world,
    hkArray<int32_t, hkContainerHeapAllocator>& groupsSizes,
    hkFixedArray<int8_t>& groupSparse
);
hkBool worldOperationUtil_less_hkSimulationIslandPtr(const hkpSimulationIsland* a, const hkpSimulationIsland* b);
int32_t getSectorSize(hkpAgentNnTrack* apThis, [[maybe_unused]] void* apEdx, int32_t a2);

// The dirty islands' comparator. The sort that takes it is hkAlgorithm::quickSortRecursive.
using hkpSimulationIslandLess = hkBool (*)(const hkpSimulationIsland*, const hkpSimulationIsland*);
namespace hkAlgorithm {}

// The world's island and entity bookkeeping.
class hkpWorldOperationUtil {
public:
    // Whether an immediate constraint add or remove fires the world's callbacks. addConstraintImmediately tests the
    // whole 32-bit value.
    enum FireCallbacks : int32_t {
        DO_NOT_FIRE_CALLBACKS_AND_SUPPRESS_EXECUTION_OF_PENDING_OPERATIONS = 0x0,
        DO_FIRE_CALLBACKS = 0x1,
    };

    // Havok SDK.
    static void markIslandInactive(hkpWorld* world, hkpSimulationIsland* island);
    // Havok SDK.
    static void splitSimulationIsland(hkpWorld* world, hkpSimulationIsland* currentIsland);

    // Adds the constraint to the world and fires the constraint-added callbacks.
    static void addConstraintToCriticalLockedIsland(hkpWorld* world, hkpConstraintInstance* constraint);
    // Adds the constraint under a critical-operations lock, fires the callbacks and runs the operations the unlock
    // left pending. Returns the constraint.
    static hkpConstraintInstance*
    addConstraintImmediately(hkpWorld* world, hkpConstraintInstance* constraint, FireCallbacks fireCallbacks);
    // Removes the constraint from its island; DO_FIRE_CALLBACKS fires the constraint-removed callbacks first.
    static void
    removeConstraintImmediately(hkpWorld* world, hkpConstraintInstance* constraint, FireCallbacks fireCallbacks);
    // Places entity in an island of world: the fixed island for a fixed motion, else an existing or a new island.
    static void addEntitySI(hkpWorld* world, hkpEntity* entity, hkpEntityActivation initialActivationState);
    static void removeIslandFromDirtyList(hkpWorld* world, hkpSimulationIsland* island);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static void removeIsland(hkpWorld* world, hkpSimulationIsland* island);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static void markIslandActive(hkpWorld* world, hkpSimulationIsland* island);
    // Removes apEntity's broad phase object and dispatches the removed pairs.
    static void removeEntityBP(hkpWorld* world, hkpEntity* entity);
    // Takes apEntity out of its simulation island.
    static void removeEntitySI(hkpWorld* world, hkpEntity* entity);
    // Adds apEntity's broad phase object and dispatches the new pairs.
    static void addEntityBP(hkpWorld* world, hkpEntity* entity);
    // Adds apPhantom's broad phase object and dispatches the new pairs.
    static void addPhantomBP(hkpWorld* world, hkpPhantom* phantom);
    // Removes apPhantom's broad phase object and dispatches the removed pairs.
    static void removePhantomBP(hkpWorld* world, hkpPhantom* phantom);
    // Sorts the world's dirty islands by their first entity's uid, then takes them off the list from the end,
    // dropping their removed actions and activating, deactivating or merging them as their flags ask.
    static void cleanupDirtyIslands(hkpWorld* world);
    // Moves the island into the world's active islands array and merges it with a sparse one.
    static void internalActivateIsland(hkpWorld* world, hkpSimulationIsland* island, hkBool allowIslandMerges);
    // Moves the island into the world's inactive islands array.
    static void internalDeactivateIsland(hkpWorld* world, hkpSimulationIsland* island);
    // Merges the islands of the two entities.
    static void mergeIslands(hkpWorld* world, hkpEntity* entityA, hkpEntity* entityB);
    // Moves the entity's actions whose other entities are all fixed off its island into actionsToBeMoved, each with a
    // reference taken.
    static void
    removeAttachedActionsFromDynamicIsland(hkpWorld* world, hkpEntity* entity, hkArray<hkpAction*>& actionsToBeMoved);
    // Puts the actions on the entity's island, drops the references removeAttached... took and merges the islands they
    // connect.
    static void addActionsToEntitysIsland(hkpWorld* world, hkpEntity* entity, hkArray<hkpAction*>& actionsToBeMoved);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    static void validateWorld(hkpWorld* world);
    static void removeAttachedActionsFromFixedIsland(
        hkpWorld* world,
        hkpEntity* entity,
        hkArray<hkpAction*, hkContainerHeapAllocator>& actionsToBeMoved
    );
    // Moves the entity's broad phase object to its current box and dispatches the pairs, or queues the operation while
    // the world is locked.
    static void updateEntityBP(hkpWorld* world, hkpEntity* entity);
    static void removeConstraintFromCriticalLockedIsland(hkpWorld* world, hkpConstraintInstance* constraint);
    static void removeAttachedAgentsConnectingTheEntityAndAFixedPartnerEntityPlus(
        hkpAgentNnTrack& trackToScan,
        hkpEntity* entity,
        hkpAgentNnTrack& agentsRemoved,
        hkpMotion::MotionType newMotionType
    );
    // Merges the smaller island into the larger one and deletes it.
    static void internalMergeTwoIslands(hkpWorld* world, hkpSimulationIsland* islandA, hkpSimulationIsland* islandB);
    static void removeAttachedConstraints(
        hkpEntity* entity,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& removedConstraints
    );
    static void markIslandInactiveMt(hkpWorld* world, hkpSimulationIsland* island);
    static void splitSimulationIsland(
        hkpSimulationIsland* currentIsland,
        hkpWorld* world,
        hkArray<hkpSimulationIsland*>& newIslandsOut,
        hkArray<hkpEntity*>* oldEntitiesOut
    );
    // Havok SDK.
    static void setRigidBodyMotionType(
        hkpRigidBody* body,
        hkpMotion::MotionType newState,
        hkpEntityActivation initialActivationState,
        hkpUpdateCollisionFilterOnEntityMode collisionFilterUpdateMode
    );

protected:
    // Switches the body's motion in place between a dynamic and a fixed or keyframed one.
    static void replaceMotionObject(
        hkpRigidBody* body,
        hkpMotion::MotionType newState,
        hkBool newStateNeedsInertia,
        hkBool oldStateNeedsInertia,
        hkpWorld* world
    );

public:
    static void splitSimulationIslands(hkpWorld* world);
};
