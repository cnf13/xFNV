#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"

#include <stdint.h>

class hkdWorld;
class hkJobThreadPool;
class hkpAction;
class hkpActionListener;
class hkpBroadPhase;
class hkpBroadPhaseBorder;
class hkpBroadPhaseBorderListener;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollidable;
class hkpCollisionCallbackUtil;
class hkpCollisionDispatcher;
class hkpCollisionFilter;
class hkpConstraintData;
class hkpConstraintInstance;
class hkpConstraintListener;
class hkpContactImpulseLimitBreachedListener;
class hkpContactListener;
class hkpConvexListFilter;
class hkpEntity;
class hkpEntityEntityBroadPhaseListener;
class hkpEntityListener;
class hkpIslandActivationListener;
class hkpIslandPostCollideListener;
class hkpIslandPostIntegrateListener;
class hkpLinearCastInput;
struct hkpMtThreadStructure;
class hkpMultiThreadedSimulation;
class hkpPhantom;
class hkpPhantomBroadPhaseListener;
class hkpPhantomListener;
class hkpPhysicsSystem;
class hkpRayHitCollector;
class hkpRigidBody;
class hkpSimulation;
class hkpSimulationIsland;
class hkpTreeWorldManager;
class hkpTypedBroadPhaseDispatcher;
class hkpWorldDeletionListener;
struct hkpWorldGetClosestPointsCommand;
struct hkpWorldLinearCastCommand;
class hkpWorldMaintenanceMgr;
class hkpWorldPostCollideListener;
class hkpWorldPostIntegrateListener;
class hkpWorldPostSimulationListener;
struct hkpWorldRayCastCommand;
struct hkpWorldRayCastInput;
class hkStatisticsCollector;
class hkWorldMemoryAvailableWatchDog;

namespace hkWorldOperation {
    struct BaseOperation;
}

// The step information the world's dynamics run with: the step's times and the solver's settings.
struct hkpWorldDynamicsStepInfo {
    hkStepInfo m_stepInfo;
    hkpSolverInfo m_solverInfo;
};

// Whether an entity added to a world starts awake. The underlying type is fixed at int32_t so that
// hkpWorldOperationQueue.h can declare it ahead.
enum hkpEntityActivation : int32_t {
    HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE = 0x0,
    HK_ENTITY_ACTIVATION_DO_ACTIVATE = 0x1,
};

// How a simulation step ended.
enum hkpStepResult : int32_t {
    HK_STEP_RESULT_SUCCESS = 0x0,
    HK_STEP_RESULT_MEMORY_FAILURE_BEFORE_INTEGRATION = 0x1,
    HK_STEP_RESULT_MEMORY_FAILURE_DURING_COLLIDE = 0x2,
    HK_STEP_RESULT_MEMORY_FAILURE_DURING_TOI_SOLVE = 0x3,
};

// How hkpWorld::updateCollisionFilterOnWorld rechecks the world's pairs.
enum hkpUpdateCollisionFilterOnWorldMode : int32_t {
    HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK = 0x0,
    HK_UPDATE_FILTER_ON_WORLD_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY = 0x1,
};

// How a filter update on one entity rechecks its pairs; it is the fourth parameter of
// hkpWorldOperationUtil::setRigidBodyMotionType.
enum hkpUpdateCollisionFilterOnEntityMode : int32_t {
    HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK = 0x0,
    HK_UPDATE_FILTER_ON_ENTITY_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY = 0x1,
};

// Whether a filter update also refilters the shape collections' children (the SDK's enum; passed like the one above).
enum hkpUpdateCollectionFilterMode : int32_t {
    HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS = 0x0,
    HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS = 0x1,
};

// The Havok simulation world. Only the calls the game makes into it are declared here.
class hkpWorld : public hkReferencedObject {
public:
    // The runtime memory the world's islands use: the largest block, the sum and the island with the largest one.
    struct MemUsageInfo {
        int32_t m_maxRuntimeBlockSize;
        int32_t m_sumRuntimeBlockSize;
        hkpSimulationIsland* m_largestSimulationIsland;
    };

    // Values from the SDK.
    enum ReintegrationRecollideMode : int32_t {
        RR_MODE_REINTEGRATE = 0x1,
        RR_MODE_RECOLLIDE_BROADPHASE = 0x2,
        RR_MODE_RECOLLIDE_NARROWPHASE = 0x4,
        RR_MODE_ALL = 0x7,
    };

    // Values from the SDK.
    enum MtAccessChecking : int32_t {
        MT_ACCESS_CHECKING_ENABLED = 0x0,
        MT_ACCESS_CHECKING_DISABLED = 0x1,
    };

    // Values from the SDK.
    enum CachedAabbUpdate : int32_t {
        SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS = 0x0,
        SHIFT_BROADPHASE_IGNORE_ENTITY_AABBS = 0x1,
    };

    // The finish constructor of a loaded world.
    hkpWorld(hkFinishLoadedObjectFlag aFlag);
    // Builds the world from info: the broad phase, the dispatchers and filters, the simulation for info's type, the
    // fixed island and rigid body and the managers.
    hkpWorld(const hkpWorldCinfo& info, uint32_t sdkversion);
    ~hkpWorld() override;

    // Havok SDK signature (hkpDynamics.lib, tied by string,vtable).
    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

    const hkArray<hkpPhantom*>& getPhantoms() const {
        return m_phantoms;
    }
    hkpSimulationIsland* getFixedIsland() {
        return m_fixedIsland;
    }
    hkpBroadPhase* getBroadPhase() {
        return m_broadPhase;
    }
    // The threading-check marks; they are empty because the game is built without HK_DEBUG_MULTI_THREADING.
    void markForWrite() {}
    void unmarkForWrite() {}
    // The read marks, the same shared empty function.
    void markForRead() const {}
    void unmarkForRead() const {}
    const hkVector4& getGravity() const {
        return m_gravity;
    }
    const hkpBroadPhase* getBroadPhase() const {
        return m_broadPhase;
    }
    const hkpSimulationIsland* getFixedIsland() const {
        return m_fixedIsland;
    }
    // Queues arOperation to run once the critical operations are unlocked.
    void queueOperation(const hkWorldOperation::BaseOperation& operation);
    // The unlock matching lockIslandForConstraintUpdate.
    void unlockIslandForConstraintUpdate(hkpSimulationIsland* island);

    // Havok SDK.
    void addEntityListener(hkpEntityListener* worldListener);
    // Havok SDK.
    void removeEntityListener(hkpEntityListener* worldListener);
    // Havok SDK.
    void addIslandActivationListener(hkpIslandActivationListener* worldListener);
    // Havok SDK.
    void removeIslandActivationListener(hkpIslandActivationListener* worldListener);

    // Havok SDK.
    void addContactListener(hkpContactListener* collisionListener);

    // Havok SDK: the entity, or null when the add was queued. The game ignores the result.
    hkpEntity* addEntity(hkpEntity* entity, hkpEntityActivation initialActivationState);

    // Havok SDK: the phantom, or null when the add was queued.
    hkpPhantom* addPhantom(hkpPhantom* phantom);

    // Havok SDK.
    void addPhantomBatch(hkpPhantom* const* phantomBatch, int32_t numPhantoms);

    // Havok SDK.
    void removePhantom(hkpPhantom* phantom);

    // Havok SDK: adds the constraint, or queues the add while the world is locked (and then returns
    // null).
    hkpConstraintInstance* addConstraint(hkpConstraintInstance* constraint);

    // Havok SDK: lets the two entities collide again, creating their agent when their AABBs overlap,
    // the filter allows it and they have none; queued while the world is locked.
    void reenableCollisionBetweenEntityPair(hkpEntity* entityA, hkpEntity* entityB);

    // Havok SDK: installs filter (a new hkpNullCollisionFilter when null), then optionally
    // refilters the world with it.
    void setCollisionFilter(
        hkpCollisionFilter* filter,
        hkBool runUpdateCollisionFilterOnWorld,
        hkpUpdateCollisionFilterOnWorldMode updateMode,
        hkpUpdateCollectionFilterMode updateShapeCollectionFilter
    );
    void updateCollisionFilterOnWorld(
        hkpUpdateCollisionFilterOnWorldMode updateMode,
        hkpUpdateCollectionFilterMode updateShapeCollectionFilter
    );
    // Rechecks phantom's broadphase overlaps against the collision filter, and with updateShapeCollectionFilter set its
    // shape collection filter too; queued while the world is locked.
    void updateCollisionFilterOnPhantom(hkpPhantom* phantom, hkpUpdateCollectionFilterMode updateShapeCollectionFilter);
    // Rechecks the entity's pairs against the collision filter; queued while the world is locked.
    void updateCollisionFilterOnEntity(
        hkpEntity* entity,
        hkpUpdateCollisionFilterOnEntityMode updateMode,
        hkpUpdateCollectionFilterMode updateShapeCollectionFilter
    );

    hkBool removeConstraint(hkpConstraintInstance* constraint);

    // Havok SDK.
    void addPhantomListener(hkpPhantomListener* worldListener);

    // Havok SDK: moves the broadphase by shiftDistance, rounded to its
    // grid; the shift it made goes to effectiveShiftDistanceOut. The game passes
    // SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS.
    void
    shiftBroadPhase(const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut, CachedAabbUpdate updateAabbs);

    // Havok SDK.
    void addEntityBatch(hkpEntity* const* entityBatch, int32_t numEntities, hkpEntityActivation initialActivationState);

    hkBool removeEntity(hkpEntity* entity);

    // Collides the entities discretely at the current time.
    void findInitialContactPoints(hkpEntity** entities, int32_t numEntities);

    // Havok SDK.
    void removeEntityBatch(hkpEntity* const* entityBatch, int32_t numEntities);

    // Havok SDK.
    void removePhantomBatch(hkpPhantom* const* phantomBatch, int32_t numPhantoms);

    // Havok SDK: the action, or null when the add was queued. The returned
    // action is unused by the game.
    hkpAction* addAction(hkpAction* action);

    // Havok SDK.
    void removeAction(hkpAction* action);

    // Havok SDK.
    void removeActionImmediately(hkpAction* action);
    // Takes the action off the entity and moves it to the island of its other entities.
    void detachActionFromEntity(hkpAction* action, hkpEntity* entity);

    // Fills infoOut with the runtime memory the active islands need to integrate.
    void getMemUsageForIntegration(MemUsageInfo& infoOut);

    void internal_executePendingOperations();
    void internal_executePendingBodyOperations();

    // The SDK's inlines: while the critical operations are locked, operations on the world are queued instead of run.
    void lockCriticalOperations() {
        ++m_criticalOperationsLockCount;
    }
    void unlockCriticalOperations() {
        --m_criticalOperationsLockCount;
    }
    void unlockAndAttemptToExecutePendingOperations() {
        --m_criticalOperationsLockCount;
        attemptToExecutePendingOperations();
    }
    void lockCriticalOperationsForPhantoms() {
        ++m_criticalOperationsLockCountForPhantoms;
    }
    void unlockCriticalOperationsForPhantoms() {
        --m_criticalOperationsLockCountForPhantoms;
    }

    // The SDK's inline: runs the pending operations unless the world is locked against them.
    void attemptToExecutePendingOperations(bool executeBodyOperations = true) {
        if (!m_criticalOperationsLockCount && !m_blockExecutingPendingOperations) {
            if (m_pendingOperationsCount) {
                internal_executePendingOperations();
            }
            if (m_pendingOperationQueueCount == 1 && executeBodyOperations && m_pendingBodyOperationsCount) {
                internal_executePendingBodyOperations();
            }
        }
    }

    float getCurrentTime() const;
    float getCurrentPsiTime() const;

    // Havok SDK.
    void setFrameTimeMarker(float frameDeltaTime);

    hkpStepResult advanceTime();
    bool isSimulationAtMarker() const;
    hkpStepResult stepDeltaTime(float physicsDeltaTime);
    // The multithreaded simulation's step start: the step result.
    hkpStepResult initMtStep(hkJobQueue* jobQueue, float physicsDeltaTime);
    // The multithreaded simulation's step: the step result.
    hkpStepResult stepMultithreaded(hkJobQueue* jobQueue, hkJobThreadPool* threadPool, float physicsDeltaTime);
    void findInitialContactPointsOfAllEntities();
    void lock();
    void unlock();
    void lockReadOnly();
    void unlockReadOnly();
    bool checkUnmarked();
    hkWorldMemoryAvailableWatchDog* getMemoryWatchDog() const;
    void checkDeterminism();
    // Empty in this build; the parameter is the SDK's.
    void checkDeterminismOfIslandBroadPhase(const hkpSimulationIsland* island);
    hkpStepResult collide();
    // Adds the system's entities, phantoms, actions and constraints to the world.
    void addPhysicsSystem(const hkpPhysicsSystem* sys);
    // Fills systemsOut with one system of the inactive entities (when there are any, marked inactive) and one of the
    // active entities, the phantoms, the constraints and the actions.
    void getWorldAsSystems(hkArray<hkpPhysicsSystem*>& systemsInOut) const;
    // Marks the AABB tree dirty so the next step moves it.
    void markTreeDirty();
    // Has the kd tree manager move the tree; the game passes two nulls.
    void updateKdTree(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);
    void lockIslandForConstraintUpdate(hkpSimulationIsland* a1);
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void lockForIslandSplit(hkpSimulationIsland* island);
    void addBodyOperation(hkpRigidBody* a1, hkpBodyOperation* a2, int32_t a3, hkpBodyOperation::ExecutionState a4);
    void castRay(const hkpWorldRayCastInput& rayCaster, hkpRayHitCollector& assertBuf) const;
    // The closest hit of a ray.
    void castRay(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput& output) const;
    void linearCast(
        const hkpCollidable* collA,
        const hkpLinearCastInput& input,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* a4
    ) const;
    hkpPhysicsSystem* getWorldAsOneSystem() const;
    void activateRegion(const hkAabb& aabb);
    void getPenetrations(
        const hkpCollidable* collA,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    ) const;
    void setBroadPhaseBorder(hkpBroadPhaseBorder* b);
    hkpBroadPhaseBorder* getBroadPhaseBorder() const;
    void reintegrateAndRecollideEntities(
        hkpEntity** entityBatch,
        int32_t numEntities,
        hkpWorld::ReintegrationRecollideMode mode
    );
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removeActionListener(hkpActionListener* worldListener);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removeConstraintListener(hkpConstraintListener* worldListener);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removePhantomListener(hkpPhantomListener* worldListener);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removeWorldPostSimulationListener(hkpWorldPostSimulationListener* worldListener);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removeContactListener(hkpContactListener* collisionListener);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removeWorldDeletionListener(hkpWorldDeletionListener* worldListener);
    hkpStepResult integrate(float physicsDeltaTime);
    hkpStepResult finishMtStep(hkJobQueue* jobQueue, hkJobThreadPool* threadPool);
    void getCinfo(hkpWorldCinfo& info) const;
    void getClosestPointsMt(
        hkpWorldGetClosestPointsCommand* commandArray,
        int32_t numCommands,
        hkJobQueue* jobQueue,
        hkJobThreadPool* jobThreadPool,
        hkSemaphore* semaphore,
        int32_t numCommandsPerJob
    );
    hkWorldOperation::UserCallback* queueCallback(hkWorldOperation::UserCallback* callback, uint32_t userData);
    void attachActionToEntity(hkpAction* action, hkpEntity* entity);
    void removeAll();
    void addActionListener(hkpActionListener* worldListener);
    void addConstraintListener(hkpConstraintListener* worldListener);
    void addWorldPostSimulationListener(hkpWorldPostSimulationListener* worldListener);
    void addWorldDeletionListener(hkpWorldDeletionListener* worldListener);
    void getClosestPoints(const hkpCollidable* collA, const hkpCollisionInput& input, hkpCdPointCollector& collector);
    hkpConstraintInstance*
    createAndAddConstraintInstance(hkpRigidBody* bodyA, hkpRigidBody* bodyB, hkpConstraintData* constraintData);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    void removePhysicsSystem(const hkpPhysicsSystem* sys);
    void setGravity(const hkVector4& gravity);
    hkpWorld::MtAccessChecking getMultithreadedAccessChecking();
    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    void linearCastMt(
        hkpWorldLinearCastCommand* commandArray,
        int32_t numCommands,
        hkJobQueue* jobQueue,
        hkJobThreadPool* jobThreadPool,
        hkSemaphore* semaphore,
        int32_t numCommandsPerJob
    ) const;
    // Mirror of lockForIslandSplit.
    void unlockForIslandSplit(hkpSimulationIsland* island);
    void setMemoryWatchDog(hkWorldMemoryAvailableWatchDog* watchDog);
    void castRayMt(
        hkpWorldRayCastCommand* commandArray,
        int32_t numCommands,
        hkJobQueue* jobQueue,
        hkJobThreadPool* jobThreadPool,
        hkSemaphore* semaphore,
        int32_t numCommandsPerJob
    );
    void addWorldPostCollideListener(hkpWorldPostCollideListener* worldListener);
    void addWorldPostIntegrateListener(hkpWorldPostIntegrateListener* worldListener);
    void addIslandPostCollideListener(hkpIslandPostCollideListener* islandListener);
    void addIslandPostIntegrateListener(hkpIslandPostIntegrateListener* islandListener);
    void addContactImpulseLimitBreachedListener(hkpContactImpulseLimitBreachedListener* listener);
    // Havok SDK: clears the listener's slot in its array (indexOf, then null).
    void removeWorldPostCollideListener(hkpWorldPostCollideListener* worldListener);
    void removeWorldPostIntegrateListener(hkpWorldPostIntegrateListener* worldListener);
    void removeIslandPostCollideListener(hkpIslandPostCollideListener* islandListener);
    void removeIslandPostIntegrateListener(hkpIslandPostIntegrateListener* islandListener);
    void removeContactImpulseLimitBreachedListener(hkpContactImpulseLimitBreachedListener* listener);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    void setMultithreadedAccessChecking(hkpWorld::MtAccessChecking accessCheckState);

    static void registerWithJobQueue(hkJobQueue* jobQueue);
    static const hkClass& staticClass();

    // An hkpSimulation, hkpContinuousSimulation or hkpMultiThreadedSimulation, by the cinfo's simulation type.
    hkpSimulation* m_simulation;
    hkVector4 m_gravity;
    hkpSimulationIsland* m_fixedIsland;
    hkpRigidBody* m_fixedRigidBody;
    hkArray<hkpSimulationIsland*> m_activeSimulationIslands;
    hkArray<hkpSimulationIsland*> m_inactiveSimulationIslands;
    hkArray<hkpSimulationIsland*> m_dirtySimulationIslands;
    hkpWorldMaintenanceMgr* m_maintenanceMgr;
    hkRefPtr<hkWorldMemoryAvailableWatchDog> m_memoryWatchDog;
    hkBool m_assertOnRunningOutOfSolverMemory;
    uint8_t padding55[3];
    hkpBroadPhase* m_broadPhase;
    hkpTreeWorldManager* m_kdTreeManager;
    hkBool m_autoUpdateTree;
    uint8_t padding61[3];
    hkpTypedBroadPhaseDispatcher* m_broadPhaseDispatcher;
    hkpPhantomBroadPhaseListener* m_phantomBroadPhaseListener;
    hkpEntityEntityBroadPhaseListener* m_entityEntityBroadPhaseListener;
    hkpBroadPhaseBorderListener* m_broadPhaseBorderListener;
    hkpMtThreadStructure* m_multithreadedSimulationJobData;
    hkpProcessCollisionInput* m_collisionInput;
    hkpCollisionFilter* m_collisionFilter;
    hkpCollisionDispatcher* m_collisionDispatcher;
    hkpConvexListFilter* m_convexListFilter;
    hkpWorldOperationQueue* m_pendingOperations;
    int32_t m_pendingOperationsCount;
    int32_t m_pendingBodyOperationsCount;
    int32_t m_criticalOperationsLockCount;
    int32_t m_criticalOperationsLockCountForPhantoms;
    hkBool m_blockExecutingPendingOperations;
    hkBool m_criticalOperationsAllowed;
    uint8_t padding9E[2];
    hkpDebugInfoOnPendingOperationQueues* m_pendingOperationQueues;
    int32_t m_pendingOperationQueueCount;
    hkMultiThreadCheck m_multiThreadCheck;
    hkBool m_processActionsInSingleThread;
    hkBool m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob;
    uint8_t paddingB2[2];
    uint32_t m_minDesiredIslandSize;
    hkCriticalSection* m_modifyConstraintCriticalSection;
    int32_t m_isLocked;
    hkCriticalSection* m_islandDirtyListCriticalSection;
    hkCriticalSection* m_propertyMasterLock;
    hkBool m_wantSimulationIslands;
    uint8_t paddingC9[3];
    float m_snapCollisionToConvexEdgeThreshold;
    float m_snapCollisionToConcaveEdgeThreshold;
    hkBool m_enableToiWeldRejection;
    hkBool m_wantDeactivation;
    hkBool m_shouldActivateOnRigidBodyTransformChange;
    uint8_t paddingD7[1];
    float m_deactivationReferenceDistance;
    float m_toiCollisionResponseRotateNormal;
    int32_t m_maxSectorsPerCollideTask;
    hkBool m_processToisMultithreaded;
    uint8_t paddingE5[3];
    int32_t m_maxEntriesPerToiCollideTask;
    int32_t m_maxNumToiCollisionPairsSinglethreaded;
    hkpWorldCinfo::SimulationType m_simulationType;
    float m_numToisTillAllowedPenetrationSimplifiedToi;
    float m_numToisTillAllowedPenetrationToi;
    float m_numToisTillAllowedPenetrationToiHigher;
    float m_numToisTillAllowedPenetrationToiForced;
    uint32_t m_lastEntityUid;
    uint32_t m_lastIslandUid;
    uint32_t m_lastConstraintUid;
    hkArray<hkpPhantom*> m_phantoms;
    hkArray<hkpActionListener*> m_actionListeners;
    hkArray<hkpEntityListener*> m_entityListeners;
    hkArray<hkpPhantomListener*> m_phantomListeners;
    hkArray<hkpConstraintListener*> m_constraintListeners;
    hkArray<hkpWorldDeletionListener*> m_worldDeletionListeners;
    hkArray<hkpIslandActivationListener*> m_islandActivationListeners;
    hkArray<hkpWorldPostSimulationListener*> m_worldPostSimulationListeners;
    hkArray<hkpWorldPostIntegrateListener*> m_worldPostIntegrateListeners;
    hkArray<hkpWorldPostCollideListener*> m_worldPostCollideListeners;
    hkArray<hkpIslandPostIntegrateListener*> m_islandPostIntegrateListeners;
    hkArray<hkpIslandPostCollideListener*> m_islandPostCollideListeners;
    hkArray<hkpContactListener*> m_contactListeners;
    hkArray<hkpContactImpulseLimitBreachedListener*> m_contactImpulseLimitBreachedListeners;
    hkpViolatedConstraintArray* m_violatedConstraintArray;
    hkpBroadPhaseBorder* m_broadPhaseBorder;
    hkpCollisionCallbackUtil* m_collisionCallbackUtil;
    hkdWorld* m_destructionWorld;
    alignas(16) hkpWorldDynamicsStepInfo m_dynamicsStepInfo;
    hkVector4 m_broadPhaseExtents[2];
    int32_t m_broadPhaseNumMarkers;
    int32_t m_sizeOfToiEventQueue;
    int32_t m_broadPhaseQuerySize;
    int32_t m_broadPhaseUpdateSize;
    hkEnum<hkpWorldCinfo::ContactPointGeneration, int8_t> m_contactPointGeneration;

    // The SDK's static hkBool: every new world simulates multithreaded while it is set.
    static inline bool m_forceMultithreadedSimulation;
};

// Adds the inactive islands' entities to apSystem and returns whether there were any, through apResult.
hkBool enumerateAllInactiveEntitiesInWorld(const hkpWorld* apWorld, hkpPhysicsSystem* apSystem);
// Adds the active islands' entities to apSystem; the hkBool result is written to apResult, which is returned.
hkBool enumerateAllActiveEntitiesInWorld(const hkpWorld* apWorld, hkpPhysicsSystem* apSystem);
// Adds the world's phantoms to sys.
void enumerateAllPhantomsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);
// Adds the world's constraints to sys.
void enumerateAllConstraintsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);
// Adds the world's actions to apSystem.
void enumerateAllActionsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);
// Registers the simple constraint, report and null contact manager factories with dis.
void hkWorld_setupContactMgrFactories(hkpWorld* world, hkpCollisionDispatcher* dis);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
void enumerateAllConstraintsInIsland(hkpSimulationIsland* island, hkpPhysicsSystem* sys);
void hkWorld_updateFilterOnSinglePhantom(hkpCollisionFilter* filter, hkpPhantom* phantom, hkpCollidable* collidable);
void enumerateAllEntitiesInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);

// The per-thread data of the multithreaded simulation's jobs; the world allocates one and leaves it to the simulation
// to fill.
struct hkpMtThreadStructure {
    hkpWorld* m_world;
    hkpConstraintQueryIn m_constraintQueryIn;
    hkpProcessCollisionInput m_collisionInput;
    // The multithreaded simulation the jobs run under (the jobs cast it to hkpMultiThreadedSimulation).
    hkPadSpu<hkpMultiThreadedSimulation*> m_simulation;
    hkpWorldDynamicsStepInfo* m_dynamicsStepInfo;
    float m_tolerance;
    hkPadSpu<void*> m_weldingTable;
};

extern hkClass hkpWorldClass;
extern hkTypeInfo hkpWorldTypeInfo;
