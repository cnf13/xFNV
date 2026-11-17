#pragma once

#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkJobThreadPool;
class hkStatisticsCollector;
class hkWorldMemoryAvailableWatchDog;
class hkdWorld;
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

namespace hkWorldOperation {
struct BaseOperation;
}

// The step information the world's dynamics run with: the step's times and the solver's settings.
struct hkpWorldDynamicsStepInfo {
	hkStepInfo m_stepInfo;
	hkpSolverInfo m_solverInfo;
};

// Whether an entity added to a world starts awake. Fixed at the exe's dword (the AddEntity operation stores it at
// +8, addEntity takes it as a dword) so that hkpWorldOperationQueue.h can declare it ahead.
enum hkpEntityActivation : int32_t {
	HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE = 0x0,
	HK_ENTITY_ACTIVATION_DO_ACTIVATE = 0x1,
};

// How a simulation step ended (the SDK's enum; the exe returns it as an int).
enum hkpStepResult : int32_t {
	HK_STEP_RESULT_SUCCESS = 0x0,
	HK_STEP_RESULT_MEMORY_FAILURE_BEFORE_INTEGRATION = 0x1,
	HK_STEP_RESULT_MEMORY_FAILURE_DURING_COLLIDE = 0x2,
	HK_STEP_RESULT_MEMORY_FAILURE_DURING_TOI_SOLVE = 0x3,
};

// How hkpWorld::updateCollisionFilterOnWorld rechecks the world's pairs (the SDK's enum; the exe passes it as a dword
// and reads its low byte).
enum hkpUpdateCollisionFilterOnWorldMode : int32_t {
	HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK = 0x0,
	HK_UPDATE_FILTER_ON_WORLD_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY = 0x1,
};

// How a filter update on one entity rechecks its pairs (the SDK's enum; the exe passes it as a dword, the fourth
// parameter of hkpWorldOperationUtil::setRigidBodyMotionType, 0xCB6F70).
enum hkpUpdateCollisionFilterOnEntityMode : int32_t {
	HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK = 0x0,
	HK_UPDATE_FILTER_ON_ENTITY_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY = 0x1,
};

// Whether a filter update also refilters the shape collections' children (the SDK's enum; passed like the one above).
enum hkpUpdateCollectionFilterMode : int32_t {
	HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS = 0x0,
	HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS = 0x1,
};

// The Havok simulation world. Its code stays with the exe: src only declares the calls the game makes into it.
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

	// The world's phantoms (the exe's copy is 0x6288A0).
	const hkArray<hkpPhantom*>& getPhantoms() const { return m_phantoms; }
	hkpSimulationIsland* getFixedIsland() { return m_fixedIsland; }
	// The exe reads it through its shared one-field getter 0x6286D0.
	hkpBroadPhase* getBroadPhase() { return m_broadPhase; }
	// The threading-check marks; the game's build (no HK_DEBUG_MULTI_THREADING) has them empty, compiled to the
	// engine's shared empty thiscall function (0x483710).
	void markForWrite() {}
	void unmarkForWrite() {}
	// The read marks, the same shared empty function.
	void markForRead() const {}
	void unmarkForRead() const {}
	// The exe reads it through its shared add-0x10 getter 0x460140.
	const hkVector4& getGravity() const { return m_gravity; }
	// The exe reads it through its shared +0x58 getter 0x6286D0.
	const hkpBroadPhase* getBroadPhase() const { return m_broadPhase; }
	// The exe reads it through its shared +0x20 getter 0x7AF430.
	const hkpSimulationIsland* getFixedIsland() const { return m_fixedIsland; }
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
	void setCollisionFilter(hkpCollisionFilter* filter, hkBool runUpdateCollisionFilterOnWorld,
		hkpUpdateCollisionFilterOnWorldMode updateMode, hkpUpdateCollectionFilterMode updateShapeCollectionFilter);
	// Havok SDK (thiscall, ret 8, 0xC96FF0; kept unregistered, the reason above its body).
	void updateCollisionFilterOnWorld(hkpUpdateCollisionFilterOnWorldMode updateMode,
		hkpUpdateCollectionFilterMode updateShapeCollectionFilter);
	// Havok SDK (thiscall, ret 8, 0xC944A0; kept unregistered, the reason above its body): rechecks phantom's
	// broadphase overlaps against the collision filter, and with updateShapeCollectionFilter set its shape collection
	// filter too (phantom vtable +0x30); queued while the world is locked.
	void updateCollisionFilterOnPhantom(hkpPhantom* phantom, hkpUpdateCollectionFilterMode updateShapeCollectionFilter);
	// Havok SDK (PDB hkpWorld::updateCollisionFilterOnEntity; thiscall, ret 0xC, 0xC946E0): rechecks entity's pairs
	// against the collision filter; queued while the world is locked.
	void updateCollisionFilterOnEntity(hkpEntity* entity, hkpUpdateCollisionFilterOnEntityMode updateMode,
		hkpUpdateCollectionFilterMode updateShapeCollectionFilter);

	// Havok SDK: the hkBool result comes back through the hidden pointer, as
	// removeEntity's does.
	hkBool* removeConstraint(hkBool* op, hkpConstraintInstance* constraint);

	// Havok SDK.
	void addPhantomListener(hkpPhantomListener* worldListener);

	// Havok SDK: moves the broadphase by shiftDistance, rounded to its
	// grid; the shift it made goes to effectiveShiftDistanceOut. The game passes
	// SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS.
	void shiftBroadPhase(const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut,
		CachedAabbUpdate updateAabbs);

	// Havok SDK.
	void addEntityBatch(hkpEntity* const* entityBatch, int32_t numEntities, hkpEntityActivation initialActivationState);

	// Havok SDK: the hkBool result comes back through the hidden pointer.
	hkBool* removeEntity(hkBool* result, hkpEntity* entity);

	// Havok SDK (thiscall, ret 8). IDA's hkpWorld::C92A80: it collides the entities
	// discretely at the current time; [INFERENCE] the PDB's findInitialContactPoints.
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
	// The SDK's name; the game's IDA name is _sub_C976A0. Takes the action off the entity and moves it to the island
	// of its other entities (thiscall, ret 8, 0xC976A0).
	void detachActionFromEntity(hkpAction* action, hkpEntity* entity);

	// Fills infoOut with the runtime memory the active islands need to integrate.
	void getMemUsageForIntegration(MemUsageInfo& infoOut);

	// Havok SDK (thiscall, no arguments; both end in a tail jump).
	void internal_executePendingOperations();
	void internal_executePendingBodyOperations();

	// The SDK's inline: runs the pending operations unless the world is locked against them.
	void attemptToExecutePendingOperations(bool executeBodyOperations = true) {
		if (!m_criticalOperationsLockCount && !m_blockExecutingPendingOperations) {
			if (m_pendingOperationsCount)
				internal_executePendingOperations();
			if (m_pendingOperationQueueCount == 1 && executeBodyOperations && m_pendingBodyOperationsCount)
				internal_executePendingBodyOperations();
		}
	}

	// Havok SDK (thiscall, ret 0): the times come back in ST(0).
	float getCurrentTime() const;
	float getCurrentPsiTime() const;

	// Havok SDK.
	void setFrameTimeMarker(float frameDeltaTime);

	// Havok SDK (thiscall): the step result comes back in eax.
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
	// Has the kd tree manager move the tree; the exe passes two nulls (thiscall, ret 8,
	// 0xC91270).
	void updateKdTree(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);
	void lockIslandForConstraintUpdate(hkpSimulationIsland* a1);
	// Havok SDK signature (hkpDynamics.lib, tied by reloc).
	void lockForIslandSplit(hkpSimulationIsland* island);
	// Inferred type: a4: hkpBodyOperation::ExecutionState: passed to hkpWorldOperationQueue::queueBodyOperation 3 at
	// 0xc91170
	void addBodyOperation(hkpRigidBody* a1, hkpBodyOperation* a2, int32_t a3, hkpBodyOperation::ExecutionState a4);
	void castRay(const hkpWorldRayCastInput& rayCaster, hkpRayHitCollector& assertBuf) const;
	// The closest hit of a ray (thiscall, ret 8, 0xC92040; nothing returned in eax).
	void castRay(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput& output) const;
	void linearCast(const hkpCollidable* collA, const hkpLinearCastInput& input, hkpCdPointCollector& castCollector,
		hkpCdPointCollector* a4) const;
	hkpPhysicsSystem* getWorldAsOneSystem() const;
	void activateRegion(const hkAabb& aabb);
	void getPenetrations(const hkpCollidable* collA, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) const;
	void setBroadPhaseBorder(hkpBroadPhaseBorder* b);
	hkpBroadPhaseBorder* getBroadPhaseBorder() const;
	void reintegrateAndRecollideEntities(hkpEntity** entityBatch, int32_t numEntities,
		hkpWorld::ReintegrationRecollideMode mode);
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
	void getClosestPointsMt(hkpWorldGetClosestPointsCommand* commandArray, int32_t numCommands, hkJobQueue* jobQueue,
		hkJobThreadPool* jobThreadPool, hkSemaphore* semaphore, int32_t numCommandsPerJob);
	hkWorldOperation::UserCallback* queueCallback(hkWorldOperation::UserCallback* callback, uint32_t userData);
	void attachActionToEntity(hkpAction* action, hkpEntity* entity);
	void removeAll();
	void addActionListener(hkpActionListener* worldListener);
	void addConstraintListener(hkpConstraintListener* worldListener);
	void addWorldPostSimulationListener(hkpWorldPostSimulationListener* worldListener);
	void addWorldDeletionListener(hkpWorldDeletionListener* worldListener);
	void getClosestPoints(const hkpCollidable* collA, const hkpCollisionInput& input, hkpCdPointCollector& collector);
	hkpConstraintInstance* createAndAddConstraintInstance(hkpRigidBody* bodyA, hkpRigidBody* bodyB,
		hkpConstraintData* constraintData);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	void removePhysicsSystem(const hkpPhysicsSystem* sys);
	void setGravity(const hkVector4& gravity);
	hkpWorld::MtAccessChecking getMultithreadedAccessChecking();
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	void linearCastMt(hkpWorldLinearCastCommand* commandArray, int32_t numCommands, hkJobQueue* jobQueue,
		hkJobThreadPool* jobThreadPool, hkSemaphore* semaphore, int32_t numCommandsPerJob) const;
	// The PDB's and the SDK's unlockForIslandSplit(hkpSimulationIsland*) (hkpDynamics.lib): lockForIslandSplit's
	// (0xC911F0) mirror, thiscall ret 4.
	void unlockForIslandSplit(hkpSimulationIsland* island);
	void setMemoryWatchDog(hkWorldMemoryAvailableWatchDog* watchDog);
	void castRayMt(hkpWorldRayCastCommand* commandArray, int32_t numCommands, hkJobQueue* jobQueue,
		hkJobThreadPool* jobThreadPool, hkSemaphore* semaphore, int32_t numCommandsPerJob);
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
	// The PDB has m_gravity at 0x10: the 4 bytes after m_simulation are its 16-byte alignment padding.
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
	// The listener arrays' element types are the PDB's (hkArray<hkpActionListener*> ... ); addPhantomListener
	// (0xC93860) stores an hkpPhantomListener* into m_phantomListeners.
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
	// The PDB's hkdWorld* m_destructionWorld; the PDB has the step info at 0x1D0, its 16-byte alignment padding
	// after this pointer.
	hkdWorld* m_destructionWorld;
	alignas(16) hkpWorldDynamicsStepInfo m_dynamicsStepInfo;
	hkVector4 m_broadPhaseExtents[2];
	int32_t m_broadPhaseNumMarkers;
	int32_t m_sizeOfToiEventQueue;
	int32_t m_broadPhaseQuerySize;
	int32_t m_broadPhaseUpdateSize;
	// The class's last member (the PDB's too), the 15 bytes to the exe's 0x350 being its 16-byte alignment padding.
	hkEnum<hkpWorldCinfo::ContactPointGeneration, int8_t> m_contactPointGeneration;

	// The SDK's static hkBool: every new world simulates multithreaded while it is set.
	static inline bool m_forceMultithreadedSimulation;
};

// [INFERENCE] the SDK's file-local enumerateAllInactiveEntitiesInWorld (its order in
// getWorldAsSystems): adds the inactive islands' entities to apSystem and returns whether there were any, through the
// hidden result pointer.
hkBool* enumerateAllInactiveEntitiesInWorld(hkBool* apResult, const hkpWorld* apWorld, hkpPhysicsSystem* apSystem);
// [INFERENCE] the SDK's file-local enumerateAllActiveEntitiesInWorld: adds the active
// islands' entities to apSystem; the hkBool comes back through the hidden result pointer.
hkBool* enumerateAllActiveEntitiesInWorld(hkBool* apResult, const hkpWorld* apWorld, hkpPhysicsSystem* apSystem);
// Adds the world's phantoms to apSystem; the world goes in edi, the system on the stack
// (cdecl-style, 0xC92950).
void enumerateAllPhantomsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);
// Adds the world's constraints to apSystem; the world goes in esi, the system on the stack
// (cdecl-style, 0xC92860).
void enumerateAllConstraintsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);
// Adds the world's actions to apSystem.
void enumerateAllActionsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);
// The SDK's file-local hkWorld_setupContactMgrFactories: registers the simple constraint, report and null contact
// manager factories with dis. The world goes in eax, the dispatcher in edi (no stack arguments, 0xC912E0).
void hkWorld_setupContactMgrFactories(hkpWorld* world, hkpCollisionDispatcher* dis);
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
void enumerateAllConstraintsInIsland(hkpSimulationIsland* island, hkpPhysicsSystem* sys);
// Usercall (0xC90F40, ret 0): filter on the stack (its +8 interface's isCollisionEnabled),
// phantom in edi (its vtable's +0x1C/+0x20/+0x24 overlap calls), collidable in esi (+0x10 owner offset, +0x18 type).
void hkWorld_updateFilterOnSinglePhantom(hkpCollisionFilter* filter, hkpPhantom* phantom, hkpCollidable* collidable);
// The SDK's file-local enumerateAllEntitiesInWorld (PDB name, no parameters recorded): the world goes in edi, the
// system in esi (0xC927C0, ret 0; eax is left holding the hkBool byte, which getWorldAsOneSystem ignores).
void enumerateAllEntitiesInWorld(const hkpWorld* world, hkpPhysicsSystem* sys);

// The per-thread data of the multithreaded simulation's jobs (PDB source hkpWorld.cpp); the world allocates one
// (0x120 bytes) and leaves it to the simulation to fill.
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
inline constexpr fnvd::Vtable hkpWorld_vftable{};
