#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbQueryUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Null/hkpNullAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpNullContactMgr.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpNullContactMgrFactory.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/DefaultConvexList/hkpDefaultConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Filter/Null/hkpNullCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpSimpleWorldRayCaster.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldLinearCaster.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCaster.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobQueueUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpWorldGetClosestPointsJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpWorldLinearCastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobQueueUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/Util/hkpCollisionCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpReportContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstanceAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpBodyOperation.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomBroadPhaseListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/BroadPhaseBorder/hkpBroadPhaseBorder.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandActivationListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandPostCollideListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandPostIntegrateListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostCollideListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostIntegrateListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/Maintenance/Default/hkpDefaultWorldMaintenanceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Maintenance/hkpWorldMaintenanceMgr.h"
#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpJobQueueUtils.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadConfig.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/AabbTree/hkpAabbTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/Util/BroadPhase/hkpBroadPhaseBorderListener.h"
#include "Havok/Source/Physics/Dynamics/World/Util/BroadPhase/hkpEntityEntityBroadPhaseListener.h"
#include "Havok/Source/Physics/Dynamics/World/Util/KdTree/hkpKdTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldMemoryUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <new>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpWorld(void* p);
static const void* getVtablehkpWorld();
// The SDK's hkAabbTreeQueryUtils<hkAabbTreeAabb16, hkUint16>::queryAabb
// with an array (hkAabbTreeQueryUtils.h): queryAabbInternal (queryAabbInternal) with no collector and no write
// iterator.
static int32_t queryAabbToArray(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabb& aabb,
    hkArray<void*>* hitArray
);

// The reflected enums and members of hkpWorld.
using hkpWorldReintegrationRecollideModeEnumItemsArray = const hkClassEnum::Item[4];
static hkpWorldReintegrationRecollideModeEnumItemsArray hkpWorldReintegrationRecollideModeEnumItems{
    {hkpWorld::RR_MODE_REINTEGRATE, "RR_MODE_REINTEGRATE"},
    {hkpWorld::RR_MODE_RECOLLIDE_BROADPHASE, "RR_MODE_RECOLLIDE_BROADPHASE"},
    {hkpWorld::RR_MODE_RECOLLIDE_NARROWPHASE, "RR_MODE_RECOLLIDE_NARROWPHASE"},
    {hkpWorld::RR_MODE_ALL, "RR_MODE_ALL"}
};
using hkpWorldMtAccessCheckingEnumItemsArray = const hkClassEnum::Item[2];
static hkpWorldMtAccessCheckingEnumItemsArray hkpWorldMtAccessCheckingEnumItems{
    {hkpWorld::MT_ACCESS_CHECKING_ENABLED, "MT_ACCESS_CHECKING_ENABLED"},
    {hkpWorld::MT_ACCESS_CHECKING_DISABLED, "MT_ACCESS_CHECKING_DISABLED"}
};
using hkpWorldCachedAabbUpdateEnumItemsArray = const hkClassEnum::Item[2];
static hkpWorldCachedAabbUpdateEnumItemsArray hkpWorldCachedAabbUpdateEnumItems{
    {hkpWorld::SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS, "SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS"},
    {hkpWorld::SHIFT_BROADPHASE_IGNORE_ENTITY_AABBS, "SHIFT_BROADPHASE_IGNORE_ENTITY_AABBS"}
};
using hkpWorldEnumsArray = const hkClassEnum[3];
static hkpWorldEnumsArray hkpWorldEnums{
    {"ReintegrationRecollideMode", hkpWorldReintegrationRecollideModeEnumItems, 4, nullptr, {0}},
    {"MtAccessChecking", hkpWorldMtAccessCheckingEnumItems, 2, nullptr, {0}},
    {"CachedAabbUpdate", hkpWorldCachedAabbUpdateEnumItems, 2, nullptr, {0}}
};
using hkpWorldClass_MembersArray = const hkClassMember[83];
static hkpWorldClass_MembersArray hkpWorldClass_Members{
    {"simulation",
     &hkpSimulationClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x8,
     nullptr},
    {"gravity", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"fixedIsland",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x20,
     nullptr},
    {"fixedRigidBody",
     &hkpRigidBodyClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x24,
     nullptr},
    {"activeSimulationIslands",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x28,
     nullptr},
    {"inactiveSimulationIslands",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x34,
     nullptr},
    {"dirtySimulationIslands",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x40,
     nullptr},
    {"maintenanceMgr",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x4C,
     nullptr},
    {"memoryWatchDog",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x50,
     nullptr},
    {"assertOnRunningOutOfSolverMemory",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x54,
     nullptr},
    {"broadPhase",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x58,
     nullptr},
    {"kdTreeManager",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x5C,
     nullptr},
    {"autoUpdateTree", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x60, nullptr},
    {"broadPhaseDispatcher",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x64,
     nullptr},
    {"phantomBroadPhaseListener",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x68,
     nullptr},
    {"entityEntityBroadPhaseListener",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x6C,
     nullptr},
    {"broadPhaseBorderListener",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x70,
     nullptr},
    {"multithreadedSimulationJobData",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x74,
     nullptr},
    {"collisionInput",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x78,
     nullptr},
    {"collisionFilter",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x7C,
     nullptr},
    {"collisionDispatcher",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x80,
     nullptr},
    {"convexListFilter",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x84,
     nullptr},
    {"pendingOperations",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x88,
     nullptr},
    {"pendingOperationsCount",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8C,
     nullptr},
    {"pendingBodyOperationsCount",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x90,
     nullptr},
    {"criticalOperationsLockCount",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x94,
     nullptr},
    {"criticalOperationsLockCountForPhantoms",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x98,
     nullptr},
    {"blockExecutingPendingOperations",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x9C,
     nullptr},
    {"criticalOperationsAllowed",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x9D,
     nullptr},
    {"pendingOperationQueues",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xA0,
     nullptr},
    {"pendingOperationQueueCount",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xA4,
     nullptr},
    {"multiThreadCheck",
     &hkMultiThreadCheckClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xA8,
     nullptr},
    {"processActionsInSingleThread",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB0,
     nullptr},
    {"allowIntegrationOfIslandsWithoutConstraintsInASeparateJob",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB1,
     nullptr},
    {"minDesiredIslandSize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xB4,
     nullptr},
    {"modifyConstraintCriticalSection",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xB8,
     nullptr},
    {"isLocked", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xBC, nullptr},
    {"islandDirtyListCriticalSection",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC0,
     nullptr},
    {"propertyMasterLock",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xC4,
     nullptr},
    {"wantSimulationIslands",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC8,
     nullptr},
    {"snapCollisionToConvexEdgeThreshold",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xCC,
     nullptr},
    {"snapCollisionToConcaveEdgeThreshold",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD0,
     nullptr},
    {"enableToiWeldRejection",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD4,
     nullptr},
    {"wantDeactivation",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD5,
     nullptr},
    {"shouldActivateOnRigidBodyTransformChange",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD6,
     nullptr},
    {"deactivationReferenceDistance",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xD8,
     nullptr},
    {"toiCollisionResponseRotateNormal",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xDC,
     nullptr},
    {"maxSectorsPerCollideTask",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xE0,
     nullptr},
    {"processToisMultithreaded",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xE4,
     nullptr},
    {"maxEntriesPerToiCollideTask",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xE8,
     nullptr},
    {"maxNumToiCollisionPairsSinglethreaded",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xEC,
     nullptr},
    {"simulationType",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT32},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0xF0,
     nullptr},
    {"numToisTillAllowedPenetrationSimplifiedToi",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xF4,
     nullptr},
    {"numToisTillAllowedPenetrationToi",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xF8,
     nullptr},
    {"numToisTillAllowedPenetrationToiHigher",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xFC,
     nullptr},
    {"numToisTillAllowedPenetrationToiForced",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x100,
     nullptr},
    {"lastEntityUid",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x104,
     nullptr},
    {"lastIslandUid",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x108,
     nullptr},
    {"lastConstraintUid",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10C,
     nullptr},
    {"phantoms",
     &hkpPhantomClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x110,
     nullptr},
    {"actionListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x11C,
     nullptr},
    {"entityListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x128,
     nullptr},
    {"phantomListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x134,
     nullptr},
    {"constraintListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x140,
     nullptr},
    {"worldDeletionListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x14C,
     nullptr},
    {"islandActivationListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x158,
     nullptr},
    {"worldPostSimulationListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x164,
     nullptr},
    {"worldPostIntegrateListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x170,
     nullptr},
    {"worldPostCollideListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x17C,
     nullptr},
    {"islandPostIntegrateListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x188,
     nullptr},
    {"islandPostCollideListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x194,
     nullptr},
    {"contactListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1A0,
     nullptr},
    {"contactImpulseLimitBreachedListeners",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1AC,
     nullptr},
    {"violatedConstraintArray",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1B8,
     nullptr},
    {"broadPhaseBorder",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1BC,
     nullptr},
    {"collisionCallbackUtil",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1C0,
     nullptr},
    {"destructionWorld",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1C4,
     nullptr},
    {"broadPhaseExtents",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     2,
     {0},
     0x310,
     nullptr},
    {"broadPhaseNumMarkers",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x330,
     nullptr},
    {"sizeOfToiEventQueue",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x334,
     nullptr},
    {"broadPhaseQuerySize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x338,
     nullptr},
    {"broadPhaseUpdateSize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x33C,
     nullptr},
    {"contactPointGeneration",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x340,
     nullptr}
};

// The dynamics jobs' process functions: registerWithJobQueue fills all 17 entries and passes the table as
// hkJobHandlerFuncs::m_processJobFuncs with m_numProcessJobFuncs 17.
using s_dynamicsProcessFuncs_t = hkJobQueue::ProcessJobFunc[17];
static s_dynamicsProcessFuncs_t s_dynamicsProcessFuncs{};
hkClass hkpWorldClass(
    "hkpWorld",
    &hkReferencedObjectClass,
    848,
    nullptr,
    0,
    hkpWorldEnums,
    3,
    hkpWorldClass_Members,
    83,
    nullptr,
    nullptr,
    1,
    0
);
hkTypeInfo hkpWorldTypeInfo = {
    "hkpWorld",
    finishLoadedObjecthkpWorld,
    cleanupLoadedObjecthkpWorld,
    getVtablehkpWorld(),
    &typeid(hkpWorld),
    sizeof(hkpWorld)
};

static const void* getVtablehkpWorld() {
    return hkGetVtableOf<hkpWorld>();
}

static void cleanupLoadedObjecthkpWorld(void* p) {
    static_cast<hkpWorld*>(p)->~hkpWorld();
}

static int32_t queryAabbToArray(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabb& aabb,
    hkArray<void*>* hitArray
) {
    if (tree) {
        return queryAabbInternal(tree, aabb, nullptr, hitArray, nullptr, 0);
    }
    return 0;
}

// The finish constructor: the arrays start empty; the loaded fields are kept.
hkpWorld::hkpWorld(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject() {}

// m_snapCollisionToConcaveEdgeThreshold deliberately gets info's convex threshold. When
// m_forceMultithreadedSimulation is set, info itself is switched to the multithreaded type.
hkpWorld::hkpWorld(const hkpWorldCinfo& info, uint32_t sdkversion)
  : hkReferencedObject(), m_gravity(info.m_gravity), m_assertOnRunningOutOfSolverMemory(true),
    m_pendingOperationsCount(0), m_pendingBodyOperationsCount(0), m_criticalOperationsLockCount(0),
    m_criticalOperationsLockCountForPhantoms(0), m_blockExecutingPendingOperations(false),
    m_criticalOperationsAllowed(true), m_pendingOperationQueues(nullptr), m_pendingOperationQueueCount(1),
    m_processActionsInSingleThread(info.m_processActionsInSingleThread),
    m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob(false), m_minDesiredIslandSize(0),
    m_modifyConstraintCriticalSection(nullptr), m_isLocked(0), m_islandDirtyListCriticalSection(nullptr),
    m_propertyMasterLock(nullptr), m_wantSimulationIslands(info.m_enableSimulationIslands),
    m_snapCollisionToConvexEdgeThreshold(info.m_snapCollisionToConvexEdgeThreshold),
    m_snapCollisionToConcaveEdgeThreshold(info.m_snapCollisionToConvexEdgeThreshold),
    m_enableToiWeldRejection(info.m_enableToiWeldRejection),
    // Deactivation needs islands.
    m_wantDeactivation(info.m_enableDeactivation.m_bool && info.m_enableSimulationIslands.m_bool),
    m_shouldActivateOnRigidBodyTransformChange(info.m_shouldActivateOnRigidBodyTransformChange),
    m_deactivationReferenceDistance(info.m_deactivationReferenceDistance),
    m_toiCollisionResponseRotateNormal(info.m_toiCollisionResponseRotateNormal),
    m_maxSectorsPerCollideTask(info.m_maxSectorsPerCollideTask),
    m_processToisMultithreaded(info.m_processToisMultithreaded),
    m_maxEntriesPerToiCollideTask(info.m_maxEntriesPerToiCollideTask),
    m_maxNumToiCollisionPairsSinglethreaded(info.m_maxNumToiCollisionPairsSinglethreaded),
    m_numToisTillAllowedPenetrationSimplifiedToi(info.m_numToisTillAllowedPenetrationSimplifiedToi),
    m_numToisTillAllowedPenetrationToi(info.m_numToisTillAllowedPenetrationToi),
    m_numToisTillAllowedPenetrationToiHigher(info.m_numToisTillAllowedPenetrationToiHigher),
    m_numToisTillAllowedPenetrationToiForced(info.m_numToisTillAllowedPenetrationToiForced),
    m_lastEntityUid(0xFFFFFFFF), m_lastConstraintUid(0xFFFFFFFF), m_collisionCallbackUtil(nullptr),
    m_destructionWorld(nullptr), m_broadPhaseNumMarkers(info.m_broadPhaseNumMarkers),
    m_sizeOfToiEventQueue(info.m_sizeOfToiEventQueue), m_broadPhaseQuerySize(info.m_broadPhaseQuerySize),
    m_broadPhaseUpdateSize(info.m_broadPhaseQuerySize / 2) {
    m_violatedConstraintArray = new hkpViolatedConstraintArray;
    m_pendingOperations = new hkpWorldOperationQueue(this);

    m_broadPhaseExtents[0] = info.m_broadPhaseWorldAabb.m_min;
    m_broadPhaseExtents[1] = info.m_broadPhaseWorldAabb.m_max;
    m_broadPhase = hkpBroadPhase::s_createSweepAndPruneBroadPhaseFunction(
        m_broadPhaseExtents[0],
        m_broadPhaseExtents[1],
        info.m_broadPhaseNumMarkers
    );
    // 70100 is the HAVOK_SDK_VERSION_NUMBER of the Havok libraries the game is built with.
    if (sdkversion != 70100) {
        char acBuffer[0x200];
        hkErrStream kStream(acBuffer, sizeof(acBuffer));
        kStream << "** Havok libs built with version [" << static_cast<int32_t>(70100)
                << "], used with code built with [" << sdkversion << "]. **";
        if (hkError::s_instance
                ->message(hkError::MESSAGE_ERROR, 0x53C94B42, acBuffer, ".\\World\\hkpWorld.cpp", 0xBB5)) {
            xfnv::DebugTrap();
        }
    }
    hkpWeldingUtility::initWeldingTable(
        info.m_snapCollisionToConvexEdgeThreshold,
        info.m_snapCollisionToConcaveEdgeThreshold
    );

    hkpSolverInfo& rSolverInfo = m_dynamicsStepInfo.m_solverInfo;
    rSolverInfo.m_one = 1.0f;
    rSolverInfo.setTauAndDamping(info.m_solverTau, info.m_solverDamp);
    rSolverInfo.m_contactRestingVelocity = info.m_contactRestingVelocity;
    rSolverInfo.m_numSteps = static_cast<int32_t>(info.m_solverIterations);
    rSolverInfo.m_invNumSteps = 1.0f / static_cast<int32_t>(info.m_solverIterations);
    rSolverInfo.m_numMicroSteps = static_cast<int32_t>(info.m_solverMicrosteps);
    rSolverInfo.m_maxConstraintViolationSqrd = info.m_maxConstraintViolation * info.m_maxConstraintViolation;
    rSolverInfo.m_invNumMicroSteps = 1.0f / static_cast<int32_t>(info.m_solverMicrosteps);
    rSolverInfo.m_forceCoherentConstraintOrderingInSolver = info.m_forceCoherentConstraintOrderingInSolver;
    rSolverInfo.m_deactivationNumInactiveFramesSelectFlag[0] = info.m_deactivationNumInactiveFramesSelectFlag0;
    rSolverInfo.m_deactivationNumInactiveFramesSelectFlag[1] = info.m_deactivationNumInactiveFramesSelectFlag1;
    rSolverInfo.m_deltaTime = 0.0f;
    rSolverInfo.m_invDeltaTime = 0.0f;
    rSolverInfo.m_deactivationIntegrateCounter = info.m_deactivationIntegrateCounter;

    // The deactivation thresholds scale with the gravity; a zero gravity sizes them for the earth's.
    const hkQuadReal& kInfoGravity = info.m_gravity.m_quad;
    float fGravity = std::sqrtf(
        XFNV_M128_F32(kInfoGravity, 0) * XFNV_M128_F32(kInfoGravity, 0) +
        XFNV_M128_F32(kInfoGravity, 1) * XFNV_M128_F32(kInfoGravity, 1) +
        XFNV_M128_F32(kInfoGravity, 2) * XFNV_M128_F32(kInfoGravity, 2)
    );
    if (fGravity == 0.0f) {
        fGravity = 9.81f;
    }
    const float fAverageObjectSize = 0.1f * fGravity;
    static const float kRelVelocities[6] = {HK_REAL_EPSILON, HK_REAL_EPSILON, 0.01f, 0.017f, 0.02f, 0.025f};
    static const float kFactors[6] = {0.0f, 0.0f, 0.08f, 0.2f, 0.3f, 0.4f};
    for (int32_t i = 0; i < 6; ++i) {
        hkpSolverInfo::DeactivationInfo& rInfo = rSolverInfo.m_deactivationInfo[i];
        const float fFactor = kFactors[i];
        const float fRelVelocityThreshold = kRelVelocities[i] * fGravity;
        const float fGravityFactor = fFactor * fGravity / fRelVelocityThreshold;
        rInfo.m_slowObjectVelocityMultiplier = 1.0f - fGravityFactor * (rSolverInfo.m_invNumSteps * 0.016f);
        rInfo.m_linearVelocityThresholdInv = 1.0f / fRelVelocityThreshold;
        rInfo.m_angularVelocityThresholdInv = 1.0f / (fRelVelocityThreshold * fAverageObjectSize);
        rInfo.m_relativeSleepVelocityThreshold = fFactor > 0.0f ? rSolverInfo.m_invNumSteps * 0.016f / fFactor
                                                                : 2.126762511484785e+37f;
        const float fDistance = info.m_deactivationReferenceDistance;
        rInfo.m_maxDistSqrd[0] = fDistance * fDistance;
        rInfo.m_maxDistSqrd[1] = (4.0f * fDistance) * (4.0f * fDistance);
        rInfo.m_maxRotSqrd[0] = hkHalf((fDistance + fDistance) * (fDistance + fDistance));
        rInfo.m_maxRotSqrd[1] = hkHalf((fDistance * 8.0f) * (fDistance * 8.0f));
    }

    hkWorldMemoryAvailableWatchDog* pWatchDog = static_cast<hkWorldMemoryAvailableWatchDog*>(info.m_memoryWatchDog);
    if (pWatchDog) {
        pWatchDog->addReference();
    }
    m_memoryWatchDog.m_pntr = pWatchDog;

    // The broad phase dispatcher and its listeners per pair of broad phase handle types.
    m_broadPhaseDispatcher = new hkpTypedBroadPhaseDispatcher();
    // The two listeners without a constructor of their own are default-initialised (no `()`): value-initialisation
    // would zero the object after operator new, the m_memSizeAndFlags it stores with it, and delete would then free
    // the block as 0 bytes.
    m_phantomBroadPhaseListener = new hkpPhantomBroadPhaseListener;
    m_entityEntityBroadPhaseListener = new hkpEntityEntityBroadPhaseListener(this);
    m_broadPhaseBorderListener = new hkpBroadPhaseBorderListener;
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_phantomBroadPhaseListener,
        hkpWorldObject::BROAD_PHASE_ENTITY,
        hkpWorldObject::BROAD_PHASE_PHANTOM
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_phantomBroadPhaseListener,
        hkpWorldObject::BROAD_PHASE_PHANTOM,
        hkpWorldObject::BROAD_PHASE_ENTITY
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_phantomBroadPhaseListener,
        hkpWorldObject::BROAD_PHASE_PHANTOM,
        hkpWorldObject::BROAD_PHASE_PHANTOM
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_entityEntityBroadPhaseListener,
        hkpWorldObject::BROAD_PHASE_ENTITY,
        hkpWorldObject::BROAD_PHASE_ENTITY
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_broadPhaseBorderListener,
        hkpWorldObject::BROAD_PHASE_ENTITY,
        hkpWorldObject::BROAD_PHASE_BORDER
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_broadPhaseBorderListener,
        hkpWorldObject::BROAD_PHASE_BORDER,
        hkpWorldObject::BROAD_PHASE_ENTITY
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_broadPhaseBorderListener,
        hkpWorldObject::BROAD_PHASE_PHANTOM,
        hkpWorldObject::BROAD_PHASE_BORDER
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_broadPhaseBorderListener,
        hkpWorldObject::BROAD_PHASE_BORDER,
        hkpWorldObject::BROAD_PHASE_PHANTOM
    );
    m_broadPhaseDispatcher->setBroadPhaseListener(
        m_broadPhaseBorderListener,
        hkpWorldObject::BROAD_PHASE_BORDER,
        hkpWorldObject::BROAD_PHASE_BORDER
    );

    // The collision dispatcher holds the only reference to its default contact manager factory.
    hkpSimpleConstraintContactMgr::Factory* pFactory = new hkpSimpleConstraintContactMgr::Factory(this);
    m_collisionDispatcher = new hkpCollisionDispatcher(hkpNullAgent::createNullAgent, pFactory);
    pFactory->removeReference();

    m_collisionFilter = static_cast<hkpCollisionFilter*>(info.m_collisionFilter);
    if (m_collisionFilter) {
        m_collisionFilter->addReference();
    }
    else {
        m_collisionFilter = new hkpNullCollisionFilter();
    }
    m_convexListFilter = static_cast<hkpConvexListFilter*>(info.m_convexListFilter);
    if (m_convexListFilter) {
        m_convexListFilter->addReference();
    }
    else {
        // Default-initialised (no `()`), as the listeners above: value-initialisation would zero the m_memSizeAndFlags
        // operator new stores and the filter would be freed as 0 bytes.
        m_convexListFilter = new hkpDefaultConvexListFilter;
    }

    m_collisionInput = new hkpProcessCollisionInput();
    hkpProcessCollisionInput* pInput = m_collisionInput;
    pInput->m_weldClosestPoints.m_storage = 0;
    pInput->m_dispatcher = m_collisionDispatcher;
    pInput->m_tolerance = info.m_collisionTolerance;
    pInput->m_filter = m_collisionFilter;
    pInput->m_convexListFilter = m_convexListFilter;
    m_contactPointGeneration.m_storage = info.m_contactPointGeneration.m_storage;
    pInput->m_allowToSkipConfirmedCallbacks = info.m_allowToSkipConfirmedCallbacks;
    pInput->m_config = new hkpCollisionAgentConfig;
    pInput->m_config->m_iterativeLinearCastEarlyOutDistance = info.m_iterativeLinearCastEarlyOutDistance;
    pInput->m_config->m_iterativeLinearCastMaxIterations = static_cast<int32_t>(
        info.m_iterativeLinearCastMaxIterations
    );
    pInput->m_enableDeprecatedWelding = info.m_enableDeprecatedWelding;
    pInput->m_createPredictiveAgents.m_storage = 0;
    // The PSI quality (COLLISION_QUALITY_PSI, 1) of the input's dispatcher.
    pInput->m_collisionQualityInfo = &pInput->m_dispatcher->m_collisionQualityInfo[1];
    pInput->m_stepInfo.m_startTime = 0.0f;
    pInput->m_stepInfo.m_endTime = 0.0f;
    pInput->m_stepInfo.m_deltaTime = 0.0f;
    pInput->m_stepInfo.m_invDeltaTime = 0.0f;

    // The world AABB packed into 32 bits.
    const hkQuadReal& kMin = info.m_broadPhaseWorldAabb.m_min.m_quad;
    const hkQuadReal& kMax = info.m_broadPhaseWorldAabb.m_max.m_quad;
    hkpCollisionInput::Aabb32Info& rAabb32 = pInput->m_aabb32Info;
    for (int32_t i = 0; i < 3; ++i) {
        const float fExtent = XFNV_M128_F32(kMax, i) - XFNV_M128_F32(kMin, i);
        XFNV_M128_F32(rAabb32.m_bitScale.m_quad, i) = (1.0f / fExtent) * 2147352576.0f;
        XFNV_M128_F32(rAabb32.m_bitOffsetLow.m_quad, i) = -XFNV_M128_F32(kMin, i);
        XFNV_M128_F32(rAabb32.m_bitOffsetHigh.m_quad, i) = 4.656897090171697e-10f * fExtent - XFNV_M128_F32(kMin, i);
    }
    XFNV_M128_F32(rAabb32.m_bitScale.m_quad, 3) = 0.0f;
    XFNV_M128_F32(rAabb32.m_bitOffsetLow.m_quad, 3) = 0.0f;
    XFNV_M128_F32(rAabb32.m_bitOffsetHigh.m_quad, 3) = 0.0f;
    m_broadPhase->set32BitOffsetAndScale(rAabb32.m_bitOffsetLow, rAabb32.m_bitOffsetHigh, rAabb32.m_bitScale);
    hkWorld_setupContactMgrFactories(this, m_collisionDispatcher);

    if (hkpWorld::m_forceMultithreadedSimulation) {
        const_cast<hkpWorldCinfo&>(info).m_simulationType.m_storage = hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED;
    }
    m_simulationType = static_cast<hkpWorldCinfo::SimulationType>(info.m_simulationType.m_storage);
    switch (m_simulationType) {
        case hkpWorldCinfo::SIMULATION_TYPE_DISCRETE: m_simulation = new hkpSimulation(this); break;
        case hkpWorldCinfo::SIMULATION_TYPE_CONTINUOUS: m_simulation = new hkpContinuousSimulation(this); break;
        case hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED:
            m_modifyConstraintCriticalSection = new hkCriticalSection(4000);
            m_propertyMasterLock = new hkCriticalSection(4000);
            m_simulation = new hkpMultiThreadedSimulation(this);
            m_minDesiredIslandSize = info.m_minDesiredIslandSize;
            break;
        default: break;
    }
    m_islandDirtyListCriticalSection = new hkCriticalSection(4000);
    m_simulation->m_frameMarkerPsiSnap = info.m_frameMarkerPsiSnap;

    // The collision qualities, from the world's gravity this time.
    const hkQuadReal& kWorldGravity = m_gravity.m_quad;
    fGravity = std::sqrtf(
        XFNV_M128_F32(kWorldGravity, 0) * XFNV_M128_F32(kWorldGravity, 0) +
        XFNV_M128_F32(kWorldGravity, 1) * XFNV_M128_F32(kWorldGravity, 1) +
        XFNV_M128_F32(kWorldGravity, 2) * XFNV_M128_F32(kWorldGravity, 2)
    );
    if (fGravity == 0.0f) {
        fGravity = 9.81f;
    }
    hkpCollisionDispatcher::InitCollisionQualityInfo kQualityInfo;
    kQualityInfo.m_gravityLength = fGravity;
    kQualityInfo.m_collisionTolerance = m_collisionInput->m_tolerance;
    kQualityInfo.m_minDeltaTime = info.m_expectedMinPsiDeltaTime;
    kQualityInfo.m_maxLinearVelocity = info.m_expectedMaxLinearVelocity;
    kQualityInfo.m_numToisTillAllowedPenetrationSimplifiedToi = info.m_numToisTillAllowedPenetrationSimplifiedToi;
    kQualityInfo.m_numToisTillAllowedPenetrationToi = info.m_numToisTillAllowedPenetrationToi;
    kQualityInfo.m_numToisTillAllowedPenetrationToiHigher = info.m_numToisTillAllowedPenetrationToiHigher;
    kQualityInfo.m_numToisTillAllowedPenetrationToiForced = info.m_numToisTillAllowedPenetrationToiForced;
    kQualityInfo.m_defaultConstraintPriority = 1;
    kQualityInfo.m_toiConstraintPriority = 3;
    kQualityInfo.m_toiHigherConstraintPriority = 4;
    kQualityInfo.m_toiForcedConstraintPriority = 5;
    kQualityInfo.m_wantContinuousCollisionDetection.m_bool = info.m_simulationType.m_storage >=
        hkpWorldCinfo::SIMULATION_TYPE_CONTINUOUS;
    kQualityInfo.m_enableNegativeManifoldTims.m_bool = info.m_contactPointGeneration.m_storage ==
        hkpWorldCinfo::CONTACT_POINT_REJECT_MANY;
    kQualityInfo.m_enableNegativeToleranceToCreateNon4dContacts.m_bool = info.m_contactPointGeneration.m_storage >=
        hkpWorldCinfo::CONTACT_POINT_REJECT_DUBIOUS;
    kQualityInfo.m_enableToiWeldRejection = info.m_enableToiWeldRejection;
    m_collisionDispatcher->initCollisionQualityInfo(kQualityInfo);
    m_collisionInput->m_collisionQualityInfo = &m_collisionDispatcher->m_collisionQualityInfo[1];

    // The fixed island; without islands every entity lives in one active island.
    m_fixedIsland = new hkpSimulationIsland(this);
    m_fixedIsland->m_storageIndex = 0xFFFF;
    m_fixedIsland->m_activeMark = 0;
    m_fixedIsland->m_isInActiveIslandsArray = 0;
    if (!m_wantSimulationIslands.m_bool) {
        hkpSimulationIsland* pIsland = new hkpSimulationIsland(this);
        m_activeSimulationIslands.pushBack(pIsland);
        pIsland->m_storageIndex = 0;
    }

    // The fixed rigid body: the world holds the only reference.
    hkpRigidBodyCinfo kBodyInfo;
    kBodyInfo.m_mass = 0.0f;
    kBodyInfo.m_motionType.m_storage = hkpMotion::MOTION_FIXED;
    m_fixedRigidBody = new hkpRigidBody(kBodyInfo);
    addEntity(m_fixedRigidBody, HK_ENTITY_ACTIVATION_DO_ACTIVATE);
    m_fixedRigidBody->removeReference();

    m_dynamicsStepInfo.m_stepInfo.set(0.0f, 1.0f / 60.0f);
    m_collisionInput->m_dynamicsInfo = &m_dynamicsStepInfo;

    if (info.m_broadPhaseBorderBehaviour.m_storage != hkpWorldCinfo::BROADPHASE_BORDER_DO_NOTHING) {
        hkBool bPostponeAndSort;
        bPostponeAndSort.m_bool = info.m_mtPostponeAndSortBroadPhaseBorderCallbacks.m_bool;
        m_broadPhaseBorder = new hkpBroadPhaseBorder(
            this,
            static_cast<hkpWorldCinfo::BroadPhaseBorderBehaviour>(info.m_broadPhaseBorderBehaviour.m_storage),
            bPostponeAndSort
        );
    }
    else {
        m_broadPhaseBorder = nullptr;
    }
    m_maintenanceMgr = new hkpDefaultWorldMaintenanceMgr();
    m_maintenanceMgr->init(this);
    if (info.m_fireCollisionCallbacks.m_bool) {
        hkpCollisionCallbackUtil::addCollisionCallbackUtil(this);
    }
    if (info.m_useKdTree.m_bool) {
        m_kdTreeManager = new hkpKdTreeWorldManager(this, hkpKdTreeWorldManager::MARK_TREE_AS_DIRTY);
        m_autoUpdateTree = info.m_autoUpdateKdTree;
    }
    else if (info.m_useMultipleTree.m_bool) {
        m_kdTreeManager = new hkpAabbTreeWorldManager(
            this,
            static_cast<hkpWorldCinfo::TreeUpdateType>(info.m_treeUpdateType.m_storage),
            0.0f
        );
        m_autoUpdateTree = info.m_autoUpdateKdTree;
    }
    else {
        m_kdTreeManager = nullptr;
    }
    if (info.m_simulationType.m_storage != hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED) {
        m_multiThreadCheck.disableChecks();
    }
    m_multithreadedSimulationJobData = new hkpMtThreadStructure;
}

// The arrays and m_memoryWatchDog are freed by their own destructors; what the world owns through pointers is
// released or deleted here.
hkpWorld::~hkpWorld() {
    delete m_multithreadedSimulationJobData;
    m_maintenanceMgr->removeReference();
    m_maintenanceMgr = nullptr;
    hkCriticalSection** const kSections[] =
        {&m_modifyConstraintCriticalSection, &m_propertyMasterLock, &m_islandDirtyListCriticalSection};
    for (hkCriticalSection** ppSection : kSections) {
        if (*ppSection) {
            delete *ppSection;
            *ppSection = nullptr;
        }
    }
    if (m_memoryWatchDog.m_pntr) {
        m_memoryWatchDog.m_pntr->removeReference();
    }
    m_memoryWatchDog.m_pntr = nullptr;

    removeAll();
    removeEntity(m_fixedRigidBody);
    m_fixedRigidBody = nullptr;
    hkpWorldOperationUtil::removeIslandFromDirtyList(this, m_fixedIsland);
    delete m_fixedIsland;
    m_fixedIsland = nullptr;
    hkpWorldCallbackUtil::fireWorldDeleted(this);

    m_broadPhase->removeReference();
    m_broadPhase = nullptr;
    m_collisionDispatcher->removeReference();
    m_collisionDispatcher = nullptr;
    delete m_broadPhaseDispatcher;
    if (m_broadPhaseBorder) {
        m_broadPhaseBorder->removeReference();
    }
    delete m_phantomBroadPhaseListener;
    delete m_entityEntityBroadPhaseListener;
    delete m_broadPhaseBorderListener;
    m_collisionFilter->removeReference();
    m_convexListFilter->removeReference();
    delete m_collisionInput->m_config;
    delete m_collisionInput;
    m_simulation->removeReference();
    delete m_pendingOperations;
    if (m_kdTreeManager) {
        m_kdTreeManager->removeReference();
    }
    delete m_violatedConstraintArray;
}

void hkpWorld::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->m_clientData = this->m_collisionInput;
    int32_t m_capacityAndFlags = m_activeSimulationIslands.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_activeSimulationIslands.m_data,
            static_cast<int32_t>(sizeof(*m_activeSimulationIslands.m_data)) * m_capacityAndFlags
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "activeIslands",
            m_activeSimulationIslands.m_data,
            static_cast<int32_t>(sizeof(*m_activeSimulationIslands.m_data)) * m_activeSimulationIslands.m_size,
            AllocatedSize
        );
    }
    for (int32_t i = m_activeSimulationIslands.m_size - 1; i >= 0; --i) {
        collector->addReferencedObject("Active", m_activeSimulationIslands.m_data[i], 0);
    }
    int32_t v7 = m_inactiveSimulationIslands.m_capacityAndFlags;
    if (v7 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v8 = hkStatisticsCollector::getAllocatedSize(
            m_inactiveSimulationIslands.m_data,
            static_cast<int32_t>(sizeof(*m_inactiveSimulationIslands.m_data)) * v7
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "inactiveIslands",
            m_inactiveSimulationIslands.m_data,
            static_cast<int32_t>(sizeof(*m_inactiveSimulationIslands.m_data)) * m_inactiveSimulationIslands.m_size,
            v8
        );
    }
    for (int32_t j = m_inactiveSimulationIslands.m_size - 1; j >= 0; --j) {
        collector->addReferencedObject("Inactive", m_inactiveSimulationIslands.m_data[j], 0);
    }
    collector->addReferencedObject("Fixed", m_fixedIsland, 0);
    int32_t v10 = m_phantoms.m_capacityAndFlags;
    if (v10 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v11 = hkStatisticsCollector::getAllocatedSize(
            m_phantoms.m_data,
            static_cast<int32_t>(sizeof(*m_phantoms.m_data)) * v10
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PhantomPtrs",
            m_phantoms.m_data,
            static_cast<int32_t>(sizeof(*m_phantoms.m_data)) * m_phantoms.m_size,
            v11
        );
    }
    for (hkpPhantom* phantom : m_phantoms) {
        collector->addReferencedObject("Phantoms", phantom, 0);
    }
    hkCriticalSection* m_modifyConstraintCriticalSection = this->m_modifyConstraintCriticalSection;
    if (m_modifyConstraintCriticalSection) {
        collector->addNormalChunk(
            "modifyConstraintCriticalSection",
            m_modifyConstraintCriticalSection,
            static_cast<int32_t>(sizeof(*m_modifyConstraintCriticalSection)),
            0
        );
    }
    collector->addNormalChunk(
        "islandDirtyListCriticalSection",
        m_islandDirtyListCriticalSection,
        static_cast<int32_t>(sizeof(*m_islandDirtyListCriticalSection)),
        0
    );
    hkCriticalSection* m_propertyMasterLock = this->m_propertyMasterLock;
    if (m_propertyMasterLock) {
        collector->addNormalChunk(
            "propertyMasterLock",
            m_propertyMasterLock,
            static_cast<int32_t>(sizeof(*m_propertyMasterLock)),
            0
        );
    }
    collector->addNormalChunk(
        "multithreadedSimulationJobData",
        m_multithreadedSimulationJobData,
        static_cast<int32_t>(sizeof(*m_multithreadedSimulationJobData)),
        0
    );
    if (this->m_pendingOperationQueues) {
        hkpDebugInfoOnPendingOperationQueues* m_pendingOperationQueues = this->m_pendingOperationQueues;
        do {
            collector->addNormalChunk(
                "pendingOperationQueues",
                this->m_pendingOperationQueues,
                static_cast<int32_t>(sizeof(*this->m_pendingOperationQueues)),
                0
            );
            hkpDebugInfoOnPendingOperationQueues* v16 = this->m_pendingOperationQueues;
            if (v16->m_pending) {
                hkArray<hkWorldOperation::BiggestOperation, hkContainerHeapAllocator>* m_pending = v16->m_pending;
                int32_t v18 = v16->m_pending->m_capacityAndFlags;
                if (v18 >= 0 || collector->m_includeUnmanagedArrayMemory) {
                    int32_t v19 = hkStatisticsCollector::getAllocatedSize(
                        m_pending->begin(),
                        static_cast<int32_t>(sizeof(*m_pending->begin())) * (v18 & 0x3FFFFFFF)
                    );
                    collector->addChunk(
                        hkStatisticsCollector::MEM_ARRAY,
                        "pendingOperationQueuesArray",
                        m_pending->begin(),
                        static_cast<int32_t>(sizeof(*m_pending->begin())) * m_pending->m_size,
                        v19
                    );
                }
            }
            m_pendingOperationQueues = m_pendingOperationQueues->m_nextQueue;
        }
        while (m_pendingOperationQueues);
    }
    collector->pushDir("Internal");
    collector->addReferencedObject("Simulation", m_simulation, 0);
    int32_t v20 = m_dirtySimulationIslands.m_capacityAndFlags;
    if (v20 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v21 = hkStatisticsCollector::getAllocatedSize(
            m_dirtySimulationIslands.m_data,
            static_cast<int32_t>(sizeof(*m_dirtySimulationIslands.m_data)) * v20
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "DirtyIslPtr",
            m_dirtySimulationIslands.m_data,
            static_cast<int32_t>(sizeof(*m_dirtySimulationIslands.m_data)) * m_dirtySimulationIslands.m_size,
            v21
        );
    }
    collector->addReferencedObject("MaintnceMgr", m_maintenanceMgr, 0);
    collector->addNormalChunk(
        "OpQueue",
        this->m_pendingOperations,
        static_cast<int32_t>(sizeof(*this->m_pendingOperations)),
        0
    );
    collector->pushDir("OpQueue");
    hkpWorldOperationQueue* m_pendingOperations = this->m_pendingOperations;
    int32_t v23 = m_pendingOperations->m_islandMerges.m_capacityAndFlags;
    hkArray<hkWorldOperation::BiggestOperation, hkContainerHeapAllocator>* p_m_islandMerges = &m_pendingOperations
                                                                                                   ->m_islandMerges;
    if (v23 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v25 = hkStatisticsCollector::getAllocatedSize(
            p_m_islandMerges->begin(),
            static_cast<int32_t>(sizeof(*p_m_islandMerges->begin())) * (v23 & 0x3FFFFFFF)
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "IslandMerges",
            p_m_islandMerges->begin(),
            static_cast<int32_t>(sizeof(*p_m_islandMerges->begin())) * p_m_islandMerges->m_size,
            v25
        );
    }
    hkpWorldOperationQueue* v26 = this->m_pendingOperations;
    int32_t v27 = v26->m_pendingBodyOperations.m_capacityAndFlags;
    hkArray<hkpBodyOperationEntry, hkContainerHeapAllocator>* p_m_pendingBodyOperations = &v26->m_pendingBodyOperations;
    if (v27 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v29 = hkStatisticsCollector::getAllocatedSize(
            p_m_pendingBodyOperations->begin(),
            static_cast<int32_t>(sizeof(*p_m_pendingBodyOperations->begin())) * v27
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "BodyOperations",
            p_m_pendingBodyOperations->begin(),
            static_cast<int32_t>(sizeof(*p_m_pendingBodyOperations->begin())) * p_m_pendingBodyOperations->m_size,
            v29
        );
    }
    collector->popDir();
    collector->addReferencedObject("broadPhaseBorder", m_broadPhaseBorder, 0);
    collector->pushDir("Collide");
    collector->addReferencedObject("Broadphase", m_broadPhase, 0);
    collector->addNormalChunk(
        "BpDispatch",
        m_broadPhaseDispatcher,
        static_cast<int32_t>(sizeof(*m_broadPhaseDispatcher)),
        0
    );
    collector->addNormalChunk(
        "BpDispatch",
        m_entityEntityBroadPhaseListener,
        static_cast<int32_t>(sizeof(*m_entityEntityBroadPhaseListener)),
        0
    );
    collector->addNormalChunk(
        "BpDispatch",
        m_phantomBroadPhaseListener,
        static_cast<int32_t>(sizeof(*m_phantomBroadPhaseListener)),
        0
    );
    collector->addNormalChunk(
        "BpDispatch",
        m_broadPhaseBorderListener,
        static_cast<int32_t>(sizeof(*m_broadPhaseBorderListener)),
        0
    );
    collector
        ->addNormalChunk("CollInput", this->m_collisionInput, static_cast<int32_t>(sizeof(*this->m_collisionInput)), 0);
    hkpProcessCollisionInput* m_collisionInput = this->m_collisionInput;
    if (m_collisionInput) {
        collector->addNormalChunk(
            "collisionAgentConfig",
            m_collisionInput->m_config,
            static_cast<int32_t>(sizeof(*m_collisionInput->m_config)),
            0
        );
    }
    collector->addReferencedObject("Filter", m_collisionFilter, 0);
    collector->addReferencedObject("convexListFilter", m_convexListFilter, 0);
    collector->addReferencedObject("Dispatcher", m_collisionDispatcher, 0);
    collector->popDir();
    collector->popDir();
    collector->pushDir("Listeners");
    int32_t v31 = m_entityListeners.m_capacityAndFlags;
    if (v31 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v32 = hkStatisticsCollector::getAllocatedSize(
            m_entityListeners.m_data,
            static_cast<int32_t>(sizeof(*m_entityListeners.m_data)) * v31
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "EntLisPtr",
            m_entityListeners.m_data,
            static_cast<int32_t>(sizeof(*m_entityListeners.m_data)) * m_entityListeners.m_size,
            v32
        );
    }
    int32_t v33 = m_phantomListeners.m_capacityAndFlags;
    if (v33 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v34 = hkStatisticsCollector::getAllocatedSize(
            m_phantomListeners.m_data,
            static_cast<int32_t>(sizeof(*m_phantomListeners.m_data)) * v33
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PhantLisPtr",
            m_phantomListeners.m_data,
            static_cast<int32_t>(sizeof(*m_phantomListeners.m_data)) * m_phantomListeners.m_size,
            v34
        );
    }
    int32_t v35 = m_constraintListeners.m_capacityAndFlags;
    if (v35 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v36 = hkStatisticsCollector::getAllocatedSize(
            m_constraintListeners.m_data,
            static_cast<int32_t>(sizeof(*m_constraintListeners.m_data)) * v35
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "ConstrLisPtr",
            m_constraintListeners.m_data,
            static_cast<int32_t>(sizeof(*m_constraintListeners.m_data)) * m_constraintListeners.m_size,
            v36
        );
    }
    int32_t v37 = m_worldDeletionListeners.m_capacityAndFlags;
    if (v37 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v38 = hkStatisticsCollector::getAllocatedSize(
            m_worldDeletionListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldDeletionListeners.m_data)) * v37
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "WldDelLisPtr",
            m_worldDeletionListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldDeletionListeners.m_data)) * m_worldDeletionListeners.m_size,
            v38
        );
    }
    int32_t v39 = m_islandActivationListeners.m_capacityAndFlags;
    if (v39 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v40 = hkStatisticsCollector::getAllocatedSize(
            m_islandActivationListeners.m_data,
            static_cast<int32_t>(sizeof(*m_islandActivationListeners.m_data)) * v39
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "IslActLisPtr",
            m_islandActivationListeners.m_data,
            static_cast<int32_t>(sizeof(*m_islandActivationListeners.m_data)) * m_islandActivationListeners.m_size,
            v40
        );
    }
    int32_t v41 = m_worldPostSimulationListeners.m_capacityAndFlags;
    if (v41 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v42 = hkStatisticsCollector::getAllocatedSize(
            m_worldPostSimulationListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldPostSimulationListeners.m_data)) * v41
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PstSimLisPtr",
            m_worldPostSimulationListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldPostSimulationListeners.m_data)) *
                m_worldPostSimulationListeners.m_size,
            v42
        );
    }
    int32_t v43 = m_worldPostIntegrateListeners.m_capacityAndFlags;
    if (v43 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v44 = hkStatisticsCollector::getAllocatedSize(
            m_worldPostIntegrateListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldPostIntegrateListeners.m_data)) * v43
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PstIntLisPtr",
            m_worldPostIntegrateListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldPostIntegrateListeners.m_data)) * m_worldPostIntegrateListeners.m_size,
            v44
        );
    }
    int32_t v45 = m_worldPostCollideListeners.m_capacityAndFlags;
    if (v45 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v46 = hkStatisticsCollector::getAllocatedSize(
            m_worldPostCollideListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldPostCollideListeners.m_data)) * v45
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PstColLisPtr",
            m_worldPostCollideListeners.m_data,
            static_cast<int32_t>(sizeof(*m_worldPostCollideListeners.m_data)) * m_worldPostCollideListeners.m_size,
            v46
        );
    }
    int32_t v47 = m_islandPostIntegrateListeners.m_capacityAndFlags;
    if (v47 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v48 = hkStatisticsCollector::getAllocatedSize(
            m_islandPostIntegrateListeners.m_data,
            static_cast<int32_t>(sizeof(*m_islandPostIntegrateListeners.m_data)) * v47
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "PstIntLisPtr",
            m_islandPostIntegrateListeners.m_data,
            static_cast<int32_t>(sizeof(*m_islandPostIntegrateListeners.m_data)) *
                m_islandPostIntegrateListeners.m_size,
            v48
        );
    }
    int32_t v49 = m_contactListeners.m_capacityAndFlags;
    if (v49 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v50 = hkStatisticsCollector::getAllocatedSize(
            m_contactListeners.m_data,
            static_cast<int32_t>(sizeof(*m_contactListeners.m_data)) * v49
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "CollLisPtr",
            m_contactListeners.m_data,
            static_cast<int32_t>(sizeof(*m_contactListeners.m_data)) * m_contactListeners.m_size,
            v50
        );
    }
    collector->popDir();
    collector->m_clientData = nullptr;
}

void hkpWorld::queueOperation(const hkWorldOperation::BaseOperation& operation) {
    m_pendingOperations->queueOperation(operation);
}

void hkpWorld::unlockIslandForConstraintUpdate(hkpSimulationIsland* island) {
    if (m_modifyConstraintCriticalSection) {
        LeaveCriticalSection(&m_modifyConstraintCriticalSection->m_section);
    }
}

void hkpWorld::addEntityListener(hkpEntityListener* worldListener) {
    m_entityListeners.pushBack(worldListener);
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removeEntityListener(hkpEntityListener* worldListener) {
    int32_t m_size = m_entityListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpEntityListener** m_data = m_entityListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_entityListeners.m_data[v3] = 0;
}

void hkpWorld::addIslandActivationListener(hkpIslandActivationListener* worldListener) {
    m_islandActivationListeners.pushBack(worldListener);
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removeIslandActivationListener(hkpIslandActivationListener* worldListener) {
    int32_t m_size = m_islandActivationListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpIslandActivationListener** m_data = m_islandActivationListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_islandActivationListeners.m_data[v3] = 0;
}

void hkpWorld::addContactListener(hkpContactListener* collisionListener) {
    m_contactListeners.pushBack(collisionListener);
}

hkpEntity* hkpWorld::addEntity(hkpEntity* entity, hkpEntityActivation initialActivationState) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::AddEntity operation;
        operation.m_type.m_storage = hkWorldOperation::ENTITY_ADD;
        operation.m_entity = entity;
        operation.m_activation = initialActivationState;
        queueOperation(operation);
        return nullptr;
    }
    entity->setCachedShapeData(this, entity->m_collidable.m_shape);
    // An entity that carries its own motion state keeps it; the rest take the one their class builds.
    if (!entity->m_collidable.m_motion.pState) {
        entity->m_collidable.m_motion.pState = entity->getMotionState();
    }
    hkSweptTransformUtil::setTimeInformation(0.0f, 0.0f, entity->m_motion.m_motionState);
    entity->m_motion.m_deactivationNumInactiveFrames[0] = 0;
    entity->m_motion.m_deactivationNumInactiveFrames[1] = 0;
    m_criticalOperationsAllowed.m_bool = false;
    m_lastEntityUid++;
    entity->m_uid = m_lastEntityUid;
    entity->addReference();
    hkpWorldOperationUtil::addEntitySI(this, entity, initialActivationState);
    entity->m_motion.setWorldSelectFlagsNeg(
        m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[0],
        m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[1],
        m_dynamicsStepInfo.m_solverInfo.m_deactivationIntegrateCounter
    );
    lockCriticalOperations();
    m_criticalOperationsAllowed.m_bool = true;
    hkpWorldOperationUtil::addEntityBP(this, entity);
    hkpWorldCallbackUtil::fireEntityAdded(this, entity);
    hkpEntityCallbackUtil::fireEntityAdded(entity);
    unlockAndAttemptToExecutePendingOperations();
    return entity;
}

hkpPhantom* hkpWorld::addPhantom(hkpPhantom* phantom) {
    // Either lock count queues the add instead, and the queued path returns no phantom.
    if (m_criticalOperationsLockCount + m_criticalOperationsLockCountForPhantoms) {
        hkWorldOperation::AddPhantom operation;
        operation.m_type.m_storage = hkWorldOperation::PHANTOM_ADD;
        operation.m_phantom = phantom;
        queueOperation(operation);
        return nullptr;
    }
    lockCriticalOperations();
    // A phantom that carries its own motion state keeps it; the rest take the one their class builds.
    if (!phantom->m_collidable.m_motion.pState) {
        phantom->m_collidable.m_motion.pState = phantom->getMotionState();
    }
    phantom->m_world = this;
    phantom->addReference();
    m_phantoms.pushBack(phantom);
    hkpWorldOperationUtil::addPhantomBP(this, phantom);
    hkpWorldCallbackUtil::firePhantomAdded(this, phantom);
    phantom->firePhantomAdded();
    unlockAndAttemptToExecutePendingOperations();
    return phantom;
}

// KEEP (partcopy:): on the locked path only the queued AddPhantomBatch local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
void hkpWorld::addPhantomBatch(hkpPhantom* const* phantomBatch, int32_t numPhantoms) {
    // Either lock count queues the batch instead.
    if (m_criticalOperationsLockCount + m_criticalOperationsLockCountForPhantoms) {
        hkWorldOperation::AddPhantomBatch operation;
        operation.m_type.m_storage = hkWorldOperation::PHANTOM_BATCH_ADD;
        operation.m_phantoms = const_cast<hkpPhantom**>(phantomBatch);
        operation.m_numPhantoms = static_cast<uint16_t>(numPhantoms);
        queueOperation(operation);
        return;
    }
    lockCriticalOperations();
    hkLocalArray<hkpBroadPhaseHandle*> addObjectList(numPhantoms);
    hkLocalArray<hkAabb> addAabbList(numPhantoms);
    addAabbList.setSizeUnchecked(numPhantoms);
    addObjectList.setSizeUnchecked(numPhantoms);
    for (int32_t i = 0; i < numPhantoms; ++i) {
        hkpPhantom* phantom = phantomBatch[i];
        // A phantom without a motion state of its collidable's gets its own.
        if (!phantom->m_collidable.m_motion.pState) {
            phantom->m_collidable.m_motion.pState = phantom->getMotionState();
        }
        phantom->m_world = this;
        addObjectList.m_data[i] = phantom->m_collidable.getBroadPhaseHandle();
        phantom->calcAabb(addAabbList.m_data[i]);
        phantom->setBoundingVolumeData(addAabbList.m_data[i]);
        phantom->addReference();
        m_phantoms.pushBack(phantom);
        hkpWorldCallbackUtil::firePhantomAdded(this, phantom);
        phantom->firePhantomAdded();
    }
    hkLocalArray<hkpBroadPhaseHandlePair> newPairs(m_broadPhaseQuerySize);
    getBroadPhase()->addObjectBatch(addObjectList, addAabbList, newPairs);
    m_broadPhaseDispatcher->addPairs(
        static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.begin()),
        newPairs.m_size,
        static_cast<hkpCollidableCollidableFilter*>(m_collisionFilter)
    );
    unlockAndAttemptToExecutePendingOperations();
}

// KEEP (partcopy): hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole
// BiggestOperation from the RemovePhantom local, whose padding after m_type and the storage past its end nothing wrote.
void hkpWorld::removePhantom(hkpPhantom* phantom) {
    // Either lock count queues the removal instead; only the two together turn it away.
    if (m_criticalOperationsLockCount + m_criticalOperationsLockCountForPhantoms) {
        hkWorldOperation::RemovePhantom operation;
        operation.m_type.m_storage = hkWorldOperation::PHANTOM_REMOVE;
        operation.m_phantom = phantom;
        queueOperation(operation);
        return;
    }
    lockCriticalOperations();
    hkpWorldCallbackUtil::firePhantomRemoved(this, phantom);
    phantom->firePhantomRemoved();
    hkpWorldOperationUtil::removePhantomBP(this, phantom);
    // The index is -1 when the phantom is not there, and the last element moves into the hole.
    int32_t index = -1;
    for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
        if (m_phantoms.m_data[i] == phantom) {
            index = i;
            break;
        }
    }
    uint32_t newSize = --m_phantoms.m_size;
    if (static_cast<int32_t>(newSize) != index) {
        m_phantoms.m_data[index] = m_phantoms.m_data[newSize];
    }
    bool kDeallocate = phantom->m_memSizeAndFlags == 0;
    phantom->m_world = nullptr;
    if (kDeallocate) {
        phantom->deallocateInternalArrays();
    }
    phantom->removeReference();
    unlockAndAttemptToExecutePendingOperations();
}

// Locked: queues the add and returns null. Otherwise a breakable constraint whose child sizes are still unset takes
// them from its wrapped constraint's runtime info; then the constraint gets the fixed body for its null entities and
// the next uid and is added. A chain also gets its action added to the world (unless the action is in one already).
// Pending operations run at the end unless the world is locked.
hkpConstraintInstance* hkpWorld::addConstraint(hkpConstraintInstance* constraint) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::AddConstraint operation;
        operation.m_type.m_storage = hkWorldOperation::CONSTRAINT_ADD;
        operation.m_constraint = constraint;
        queueOperation(operation);
        return nullptr;
    }
    if (constraint->m_data->getType() == hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE) {
        hkpBreakableConstraintData* data = static_cast<hkpBreakableConstraintData*>(constraint->m_data);
        if (!data->m_childRuntimeSize) {
            hkpConstraintData::RuntimeInfo info;
            hkBool wantRuntime;
            wantRuntime.m_bool = 1;
            data->m_constraintData->getRuntimeInfo(wantRuntime, info);
            data->m_childRuntimeSize = static_cast<uint16_t>(info.m_sizeOfExternalRuntime);
            data->m_childNumSolverResults = static_cast<uint16_t>(info.m_numSolverResults);
        }
    }
    m_blockExecutingPendingOperations.m_bool = true;
    constraint->pointNullsToFixedRigidBody();
    constraint->m_uid = ++m_lastConstraintUid;
    hkpConstraintInstance* result = hkpWorldOperationUtil::addConstraintImmediately(
        this,
        constraint,
        hkpWorldOperationUtil::DO_FIRE_CALLBACKS
    );
    if (constraint->getType() == hkpConstraintInstance::TYPE_CHAIN) {
        hkpConstraintChainInstanceAction* action = static_cast<hkpConstraintChainInstance*>(constraint)->m_action;
        if (!action->m_world) {
            addAction(action);
        }
        // Checks the chain's entity count against the data's constraint infos; the call is kept and its result is
        // discarded.
        static_cast<hkpConstraintChainData*>(constraint->m_data)->getNumConstraintInfos();
    }
    m_blockExecutingPendingOperations.m_bool = false;
    attemptToExecutePendingOperations(false);
    return result;
}

// Locked: queues the re-enable. Otherwise, when the two broad phase handles' AABBs overlap, the filter lets the
// collidables collide and no agent links them yet, hands the pair to the broad phase dispatcher.
void hkpWorld::reenableCollisionBetweenEntityPair(hkpEntity* entityA, hkpEntity* entityB) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::UpdateFilterOnEntityPair operation;
        operation.m_type.m_storage = hkWorldOperation::UPDATE_FILTER_ENTITY_PAIR;
        operation.m_entityA = entityA;
        operation.m_entityB = entityB;
        queueOperation(operation);
        return;
    }
    hkpTypedBroadPhaseHandle* handleA = &entityA->m_collidable.m_broadPhaseHandle;
    hkpTypedBroadPhaseHandle* handleB = &entityB->m_collidable.m_broadPhaseHandle;
    if (m_broadPhase->areAabbsOverlapping(handleA, handleB)) {
        const hkpLinkedCollidable* collA = &entityA->m_collidable;
        const hkpLinkedCollidable* collB = &entityB->m_collidable;
        if (m_collisionFilter->isCollisionEnabled(*collA, *collB).m_bool &&
            (!hkAgentNnMachine_FindAgent(collA, collB))) {
            hkpTypedBroadPhaseHandlePair pair;
            pair.m_a = handleA;
            pair.m_b = handleB;
            // A null filter stays null; otherwise its hkpCollidableCollidableFilter base (+8).
            m_broadPhaseDispatcher->addPairs(&pair, 1, m_collisionFilter);
        }
    }
}

// Takes a reference on filter, or builds an hkpNullCollisionFilter when there is none (the new filter's own reference
// is the world's), releases the old filter and installs the new one in the world and its collision input, then
// refilters the world when asked.
void hkpWorld::setCollisionFilter(
    hkpCollisionFilter* filter,
    hkBool runUpdateCollisionFilterOnWorld,
    hkpUpdateCollisionFilterOnWorldMode updateMode,
    hkpUpdateCollectionFilterMode updateShapeCollectionFilter
) {
    if (!filter) {
        filter = new hkpNullCollisionFilter;
    }
    else {
        filter->addReference();
    }
    m_collisionFilter->removeReference();
    m_collisionFilter = filter;
    m_collisionInput->m_filter = filter;
    if (runUpdateCollisionFilterOnWorld.m_bool) {
        updateCollisionFilterOnWorld(updateMode, updateShapeCollectionFilter);
    }
}

// KEEP (partcopy): on the locked path the queued UpdateFilterOnWorld local is fully written, but
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the storage past its end reaches the queue unwritten.
// A full check re-runs the filter on every entity of every island and on every phantom. Otherwise each island's
// agents are filtered in place: an agent the filter or the dispatcher's quality table now disables is collected and
// removed, its island asked for a split check; the others get the shape collection filter when asked. A locked
// world queues the update instead.
void hkpWorld::updateCollisionFilterOnWorld(
    hkpUpdateCollisionFilterOnWorldMode updateMode,
    hkpUpdateCollectionFilterMode updateShapeCollectionFilter
) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::UpdateFilterOnWorld operation;
        operation.m_type.m_storage = hkWorldOperation::UPDATE_FILTER_WORLD;
        operation.m_collisionFilterUpdateMode.m_storage = static_cast<uint8_t>(updateMode);
        operation.m_updateShapeCollections.m_storage = static_cast<uint8_t>(updateShapeCollectionFilter);
        queueOperation(operation);
        return;
    }

    m_blockExecutingPendingOperations.m_bool = 1;
    HK_TIMER_BEGIN("UpdateFilterOnWorld", nullptr);

    if (updateMode == HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK) {
        for (int32_t i = 0; i < m_activeSimulationIslands.m_size; ++i) {
            hkpSimulationIsland* island = m_activeSimulationIslands.m_data[i];
            for (int32_t j = 0; j < island->m_entities.m_size; ++j) {
                updateCollisionFilterOnEntity(
                    island->m_entities.m_data[j],
                    HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
                    updateShapeCollectionFilter
                );
            }
        }
        for (int32_t i = 0; i < m_inactiveSimulationIslands.m_size; ++i) {
            hkpSimulationIsland* island = m_inactiveSimulationIslands.m_data[i];
            for (int32_t j = 0; j < island->m_entities.m_size; ++j) {
                updateCollisionFilterOnEntity(
                    island->m_entities.m_data[j],
                    HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
                    updateShapeCollectionFilter
                );
            }
        }
        for (int32_t i = 0; i < m_phantoms.m_size; ++i) {
            updateCollisionFilterOnPhantom(m_phantoms.m_data[i], updateShapeCollectionFilter);
        }
    }
    else {
        lockCriticalOperations();
        hkArray<hkpSimulationIsland*>* islandArrays[2] = {&m_activeSimulationIslands, &m_inactiveSimulationIslands};
        for (int32_t arrayIndex = 0; arrayIndex < 2; ++arrayIndex) {
            hkArray<hkpSimulationIsland*>* islands = islandArrays[arrayIndex];
            hkInplaceArray<hkpAgentNnEntry*, 32> agentsToRemove;
            for (int32_t i = 0; i < islands->m_size; ++i) {
                hkpSimulationIsland* island = islands->m_data[i];
                agentsToRemove.m_size = 0;
                // The SDK's HK_FOR_ALL_AGENT_ENTRIES_BEGIN walk over the island's track: every sector is full but
                // the last, which holds m_bytesUsedInLastSector bytes, and each entry's m_size steps to the next.
                hkpAgentNnTrack& track = island->m_agentTrack;
                for (int32_t sectorIndex = 0; sectorIndex < track.m_sectors.m_size;) {
                    hkpAgentNnSector* currentSector = track.m_sectors.m_data[sectorIndex];
                    hkpAgentNnEntry* entry = currentSector->getBegin();
                    sectorIndex++;
                    hkpAgentNnEntry* endEntry = (sectorIndex == track.m_sectors.m_size)
                        ? hkAddByteOffset(currentSector->getBegin(), track.m_bytesUsedInLastSector)
                        : currentSector->getEnd();
                    for (; entry < endEntry; entry = hkAddByteOffset(entry, entry->m_size)) {
                        if (m_collisionFilter->isCollisionEnabled(*entry->m_collidable[0], *entry->m_collidable[1])
                                .m_bool &&
                            m_collisionDispatcher->m_collisionQualityTable
                                [entry->m_collidable[0]->m_broadPhaseHandle.m_objectQualityType]
                                [entry->m_collidable[1]->m_broadPhaseHandle.m_objectQualityType]) {
                            if (updateShapeCollectionFilter == HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS) {
                                hkAgentNnMachine_UpdateShapeCollectionFilter(entry, *m_collisionInput, *island);
                            }
                        }
                        else {
                            agentsToRemove.pushBack(entry);
                            island->m_splitCheckRequested = 1;
                        }
                    }
                }
                while (agentsToRemove.m_size) {
                    hkpAgentNnEntry* agent = agentsToRemove.m_data[agentsToRemove.m_size - 1];
                    agentsToRemove.m_size--;
                    hkpWorldAgentUtil::removeAgentAndItsToiEvents(agent);
                }
            }
        }
        unlockCriticalOperations();
    }

    m_blockExecutingPendingOperations.m_bool = 0;
    attemptToExecutePendingOperations();
    HK_TIMER_END();
}

// Re-runs the collision filter between the phantom and everything its broad-phase handle overlaps, then, when asked,
// the phantom's shape collection filter; a locked world queues the update instead. On the locked path the queued
// UpdateFilterOnPhantom is only partly written before it is copied into the queue, so unwritten bytes reach the queue.
void hkpWorld::updateCollisionFilterOnPhantom(
    hkpPhantom* phantom,
    hkpUpdateCollectionFilterMode updateShapeCollectionFilter
) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::UpdateFilterOnPhantom operation;
        operation.m_type.m_storage = hkWorldOperation::UPDATE_FILTER_PHANTOM;
        operation.m_phantom = phantom;
        operation.m_updateShapeCollections.m_storage = static_cast<uint8_t>(updateShapeCollectionFilter);
        queueOperation(operation);
        return;
    }
    lockCriticalOperations();
    HK_TIMER_BEGIN_LIST("UpdateFilterOnPhantom", "broadphase");

    hkLocalArray<hkpBroadPhaseHandlePair> pairsOut(m_broadPhaseQuerySize);
    hkpTypedBroadPhaseHandle* pPhantomHandle = phantom->m_collidable.getBroadPhaseHandle();
    m_broadPhase->reQuerySingleObject(pPhantomHandle, pairsOut);

    HK_TIMER_SPLIT_LIST("UpdateOverlaps");
    for (const hkpBroadPhaseHandlePair& pair : pairsOut) {
        if (pair.m_b != pPhantomHandle) {
            hkpTypedBroadPhaseHandle* pHandle = static_cast<hkpTypedBroadPhaseHandle*>(pair.m_b);
            hkpCollidable* pCollidable = static_cast<hkpCollidable*>(pHandle->getOwner());
            hkWorld_updateFilterOnSinglePhantom(m_collisionFilter, phantom, pCollidable);
        }
    }

    if (updateShapeCollectionFilter) {
        HK_TIMER_SPLIT_LIST("collectionFilter");
        phantom->updateShapeCollectionFilter();
    }

    unlockAndAttemptToExecutePendingOperations();
    HK_TIMER_END_LIST();
}

// Re-runs the collision filter on everything the entity touches: with a full check, the pairs its broad-phase handle
// overlaps now (phantoms updated at once, pairs that already have an agent dropped) become new agents; every existing
// agent the filter or the quality table now rejects is removed, and, when asked, the others' shape collection filters
// are re-run. A locked world queues the update instead. Only the queued UpdateFilterOnEntity local's members are
// written, yet hkpWorldOperationQueue::queueOperation copies a whole BiggestOperation into the queue, so its padding
// and the storage past its end reach the queue unwritten.
void hkpWorld::updateCollisionFilterOnEntity(
    hkpEntity* entity,
    hkpUpdateCollisionFilterOnEntityMode updateMode,
    hkpUpdateCollectionFilterMode updateShapeCollectionFilter
) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::UpdateFilterOnEntity operation;
        operation.m_entity = entity;
        operation.m_collisionFilterUpdateMode.m_storage = static_cast<uint8_t>(updateMode);
        operation.m_type.m_storage = hkWorldOperation::UPDATE_FILTER_ENTITY;
        operation.m_updateShapeCollections.m_storage = static_cast<uint8_t>(updateShapeCollectionFilter);
        queueOperation(operation);
        return;
    }
    HK_TIMER_BEGIN_LIST("UpdateFilterOnEntity", "init");
    lockCriticalOperations();

    if (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
        entity->m_collidable.sortEntries();
    }

    {
        hkInplaceArray<hkpBroadPhaseHandlePair, 128> newPairs;

        if (updateMode == HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK) {
            HK_TIMER_SPLIT_LIST("broadphase");
            m_broadPhase->reQuerySingleObject(entity->m_collidable.getBroadPhaseHandle(), newPairs);

            // The phantoms among the overlaps are updated here and leave the list.
            HK_TIMER_SPLIT_LIST("phantom");
            for (int32_t i = 0; i < newPairs.m_size; ++i) {
                hkpTypedBroadPhaseHandle* pHandle = static_cast<hkpTypedBroadPhaseHandle*>(newPairs.m_data[i].m_b);
                hkpCollidable* pCollidable = static_cast<hkpCollidable*>(pHandle->getOwner());
                hkpPhantom* phantom = hkpGetPhantom(pCollidable);
                if (phantom) {
                    hkWorld_updateFilterOnSinglePhantom(m_collisionFilter, phantom, &entity->m_collidable);
                    if (updateShapeCollectionFilter) {
                        phantom->updateShapeCollectionFilter();
                    }
                    newPairs.removeAt(i);
                    --i;
                }
            }

            // The pairs that already have an agent leave the list.
            {
                hkArray<hkpLinkedCollidable::CollisionEntry>&
                    entries = entity->m_collidable.getCollisionEntriesDeterministicUnchecked();
                hkInplaceArray<hkpBroadPhaseHandlePair, 128> existingPairs;
                for (int32_t i = 0; i < entries.m_size; ++i) {
                    hkpBroadPhaseHandlePair& pair = existingPairs.expandOne();
                    pair.m_a = entries.m_data[i].m_agentEntry->m_collidable[0]->getBroadPhaseHandle();
                    pair.m_b = entries.m_data[i].m_agentEntry->m_collidable[1]->getBroadPhaseHandle();
                }
                hkpTypedBroadPhaseDispatcher::removeDuplicates(existingPairs, newPairs);
            }
        }

        // The existing agents: removed where the filter or the quality table now rejects the pair, else their shape
        // collection filters re-run when asked.
        HK_TIMER_SPLIT_LIST("checkAgts");
        {
            hkpLinkedCollidable* collidable = &entity->m_collidable;
            const int32_t qualityA = collidable->m_broadPhaseHandle.m_objectQualityType;
            hkArray<hkpLinkedCollidable::CollisionEntry>& entries = collidable
                                                                        ->getCollisionEntriesDeterministicUnchecked();
            for (int32_t i = 0; i < entries.m_size; ++i) {
                hkpLinkedCollidable::CollisionEntry& entry = entries.m_data[i];
                if (m_collisionFilter->isCollisionEnabled(*collidable, *entry.m_partner).m_bool &&
                    m_collisionDispatcher->m_collisionQualityTable[qualityA][entry.m_partner->m_broadPhaseHandle
                                                                                 .m_objectQualityType] != 0) {
                    if (updateShapeCollectionFilter == HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS) {
                        // A fixed entity's agents run in its partner's island.
                        hkpSimulationIsland* island;
                        if (entity->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                            island = static_cast<hkpEntity*>(entry.m_partner->getOwner())->m_simulationIsland;
                        }
                        else {
                            island = entity->m_simulationIsland;
                        }
                        hkAgentNnMachine_UpdateShapeCollectionFilter(entry.m_agentEntry, *m_collisionInput, *island);
                    }
                }
                else {
                    hkpWorldAgentUtil::removeAgentAndItsToiEvents(entry.m_agentEntry);
                    --i;
                    entity->m_simulationIsland->m_splitCheckRequested = 1;
                }
            }
        }

        if (newPairs.m_size > 0) {
            HK_TIMER_SPLIT_LIST("addAgts");
            hkpCollidableCollidableFilter* filter = m_collisionFilter;
            m_broadPhaseDispatcher
                ->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.m_data), newPairs.m_size, filter);
        }

        unlockAndAttemptToExecutePendingOperations();
    }
    HK_TIMER_END_LIST();
}

// KEEP (partcopy:): on the locked path only the queued RemoveConstraint local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
hkBool hkpWorld::removeConstraint(hkpConstraintInstance* constraint) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::RemoveConstraint operation;
        operation.m_type.m_storage = hkWorldOperation::CONSTRAINT_REMOVE;
        operation.m_constraint = constraint;
        queueOperation(operation);
        return 0;
    }
    lockCriticalOperations();
    if (constraint->getType() == hkpConstraintInstance::TYPE_CHAIN) {
        hkpConstraintChainInstance* chain = static_cast<hkpConstraintChainInstance*>(constraint);
        if (chain->m_action->m_world == this) {
            removeActionImmediately(chain->m_action);
        }
    }
    constraint->addReference();
    hkpWorldOperationUtil::removeConstraintImmediately(this, constraint, hkpWorldOperationUtil::DO_FIRE_CALLBACKS);
    constraint->setFixedRigidBodyPointersToZero(this);
    constraint->removeReference();
    unlockAndAttemptToExecutePendingOperations();
    return 1;
}

void hkpWorld::addPhantomListener(hkpPhantomListener* worldListener) {
    m_phantomListeners.pushBack(worldListener);
}

// Havok SDK: moves the broad phase by shiftDistance (the shift it made goes to effectiveShiftDistanceOut), moves the
// world's extents and the border's phantoms with it, adds the pairs the move created and, when asked, recomputes the
// AABBs of every entity with a shape.
void hkpWorld::shiftBroadPhase(
    const hkVector4& shiftDistance,
    hkVector4& effectiveShiftDistanceOut,
    hkpWorld::CachedAabbUpdate updateAabbs
) {
    HK_TIMER_BEGIN("Broadphase Shift", nullptr);
    hkArray<hkpBroadPhaseHandlePair> newPairs;
    getBroadPhase()->shiftBroadPhase(shiftDistance, effectiveShiftDistanceOut, newPairs);
    getBroadPhase()->getOffsetLowHigh32bit(
        m_collisionInput->m_aabb32Info.m_bitOffsetLow,
        m_collisionInput->m_aabb32Info.m_bitOffsetHigh
    );
    m_broadPhaseExtents[0].add4(effectiveShiftDistanceOut);
    m_broadPhaseExtents[1].add4(effectiveShiftDistanceOut);
    lockCriticalOperations();
    m_broadPhaseDispatcher->addPairs(
        static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.begin()),
        newPairs.m_size,
        static_cast<hkpCollidableCollidableFilter*>(m_collisionFilter)
    );
    // The border's six phantoms move with the broad phase: an AABB phantom's box, a shape phantom's translation.
    hkpBroadPhaseBorder* border = m_broadPhaseBorder;
    if (border) {
        for (hkpPhantom* phantom : border->m_phantoms) {
            switch (phantom->getType()) {
                case HK_PHANTOM_AABB: {
                    hkAabb& aabb = static_cast<hkpAabbPhantom*>(phantom)->m_aabb;
                    aabb.m_min.m_quad = _mm_add_ps(aabb.m_min.m_quad, effectiveShiftDistanceOut.m_quad);
                    // The max is added the other way round (shift + max).
                    aabb.m_max.m_quad = _mm_add_ps(effectiveShiftDistanceOut.m_quad, aabb.m_max.m_quad);
                    break;
                }
                case HK_PHANTOM_SIMPLE_SHAPE:
                case HK_PHANTOM_CACHING_SHAPE:
                    static_cast<hkpShapePhantom*>(phantom)->m_motionState.m_transform.m_translation.add4(
                        effectiveShiftDistanceOut
                    );
                    break;
                default: break;
            }
        }
    }
    if (updateAabbs == hkpWorld::SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS) {
        hkArray<hkpEntity*> entities;
        // Every island: the fixed one (when there is one), then the active and the inactive ones.
        hkpSimulationIsland* fixedIsland = getFixedIsland();
        const int32_t numIslands = m_inactiveSimulationIslands.m_size + m_activeSimulationIslands.m_size + 1;
        hkArray<hkpSimulationIsland*> islands(numIslands);
        hkpSimulationIsland** dst = islands.m_data;
        if (fixedIsland) {
            *dst++ = fixedIsland;
        }
        for (hkpSimulationIsland* island : m_activeSimulationIslands) {
            *dst++ = island;
        }
        for (hkpSimulationIsland* island : m_inactiveSimulationIslands) {
            *dst++ = island;
        }
        // The entities with a shape, island by island.
        for (hkpSimulationIsland* island : islands) {
            for (hkpEntity* entity : island->m_entities) {
                if (entity->m_collidable.m_shape) {
                    entities.pushBack(entity);
                }
            }
        }
        hkpEntityAabbUtil::entityBatchRecalcAabb(m_collisionInput, entities.m_data, entities.m_size);
    }
    unlockAndAttemptToExecutePendingOperations();
    HK_TIMER_END();
}

// KEEP (partcopy:): on the locked path only the queued AddEntityBatch local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
void hkpWorld::addEntityBatch(
    hkpEntity* const* entityBatch,
    int32_t numEntities,
    hkpEntityActivation initialActivationState
) {
    if (numEntities == 0) {
        return;
    }
    // Either lock queues the batch instead.
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::AddEntityBatch operation;
        operation.m_type = hkWorldOperation::ENTITY_BATCH_ADD;
        operation.m_entities = const_cast<hkpEntity**>(entityBatch);
        operation.m_numEntities = static_cast<uint16_t>(numEntities);
        operation.m_activation = initialActivationState;
        queueOperation(operation);
        return;
    }
    HK_TIMER_BEGIN_LIST("AddEntities", "init");
    lockCriticalOperations();
    // The SDK's hkArray<T>::Temp lists: the temp allocator's storage, reserved for the whole batch.
    hkArrayBase<hkpBroadPhaseHandle*> addObjectList;
    addObjectList.m_data = nullptr;
    addObjectList.m_size = 0;
    addObjectList.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandle*>::DONT_DEALLOCATE_FLAG;
    addObjectList._reserveExactly(*hkMemoryRouter::getInstance().m_temp, numEntities);
    hkArrayBase<hkAabb> addAabbList;
    addAabbList.m_data = nullptr;
    addAabbList.m_size = 0;
    addAabbList.m_capacityAndFlags = hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG;
    addAabbList._reserveExactly(*hkMemoryRouter::getInstance().m_temp, numEntities);
    // With islands the batch's movable entities go into one new island; without them into the one active island.
    hkpSimulationIsland* pIsland;
    bool bIslandUsed = false;
    if (m_wantSimulationIslands.m_bool) {
        pIsland = new hkpSimulationIsland(this);
        pIsland->m_splitCheckRequested = 1;
        if (initialActivationState == HK_ENTITY_ACTIVATION_DO_ACTIVATE) {
            pIsland->m_storageIndex = static_cast<uint16_t>(m_activeSimulationIslands.m_size);
            pIsland->m_isInActiveIslandsArray = 1;
            pIsland->m_activeMark = 1;
        }
        else {
            pIsland->m_storageIndex = static_cast<uint16_t>(m_inactiveSimulationIslands.m_size);
            pIsland->m_isInActiveIslandsArray = 0;
            pIsland->m_activeMark = 0;
        }
    }
    else {
        pIsland = m_activeSimulationIslands.m_data[0];
        pIsland->m_entities.reserve(pIsland->m_entities.m_size + numEntities);
    }
    const float tolerance = m_collisionInput->m_tolerance * 0.5;
    for (int32_t i = 0; i < numEntities; ++i) {
        hkpEntity* entity = entityBatch[i];
        entity->m_uid = ++m_lastEntityUid;
        entity->addReference();
        // An entity without a motion state of its collidable's gets its own.
        if (!entity->m_collidable.m_motion.pState) {
            entity->m_collidable.m_motion.pState = entity->getMotionState();
        }
        hkSweptTransformUtil::setTimeInformation(0.0f, 0.0f, entity->m_motion.m_motionState);
        entity->m_motion.m_deactivationNumInactiveFrames[1] = 0;
        entity->m_motion.m_deactivationNumInactiveFrames[0] = 0;
        entity->m_world = this;
        const hkpSolverInfo& solverInfo = m_dynamicsStepInfo.m_solverInfo;
        entity->m_motion.setWorldSelectFlagsNeg(
            solverInfo.m_deactivationNumInactiveFramesSelectFlag[0],
            solverInfo.m_deactivationNumInactiveFramesSelectFlag[1],
            solverInfo.m_deactivationIntegrateCounter
        );
        if (entity->isFixed().m_bool) {
            m_fixedIsland->internalAddEntity(entity);
        }
        else {
            bIslandUsed = true;
            pIsland->internalAddEntity(entity);
        }
        const hkpShape* shape = entity->m_collidable.m_shape;
        entity->m_collidable.m_boundingVolumeData.invalidate();
        if (shape) {
            hkAabb* out = &addAabbList.m_data[addAabbList.m_size++];
            entity->m_collidable.m_shape
                ->getAabbImpl(entity->m_collidable.m_motion.pState->m_transform, tolerance, *out);
            addObjectList.m_data[addObjectList.m_size++] = entity->m_collidable.getBroadPhaseHandle();
            entity->setCachedShapeData(this, shape);
        }
    }
    if (m_wantSimulationIslands.m_bool) {
        if (bIslandUsed) {
            pIsland->m_splitCheckFrameCounter = static_cast<uint8_t>(pIsland->m_entities[0]->getUid());
            hkArray<hkpSimulationIsland*>& islands = initialActivationState == HK_ENTITY_ACTIVATION_DO_ACTIVATE
                ? m_activeSimulationIslands
                : m_inactiveSimulationIslands;
            islands.pushBack(pIsland);
        }
        else {
            delete pIsland;
        }
    }
    hkLocalArray<hkpBroadPhaseHandlePair> newPairs(m_broadPhaseQuerySize);
    HK_TIMER_SPLIT_LIST("Broadphase");
    getBroadPhase()->addObjectBatch(addObjectList, addAabbList, newPairs);
    HK_TIMER_SPLIT_LIST("CreateAgents");
    m_broadPhaseDispatcher->addPairs(
        static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.begin()),
        newPairs.m_size,
        static_cast<hkpCollidableCollidableFilter*>(m_collisionFilter)
    );
    HK_TIMER_SPLIT_LIST("AddedCb");
    for (int32_t i = 0; i < numEntities; ++i) {
        hkpEntity* entity = entityBatch[i];
        hkpWorldCallbackUtil::fireEntityAdded(this, entity);
        hkpEntityCallbackUtil::fireEntityAdded(entity);
    }
    unlockAndAttemptToExecutePendingOperations();
    HK_TIMER_END_LIST();
    // The two temp lists' ~hkArray, in reverse order of construction.
    {
        hkMemoryAllocator& temp = *hkMemoryRouter::getInstance().m_temp;
        addAabbList.m_size = 0;
        if ((addAabbList.m_capacityAndFlags & hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG) == 0) {
            temp._bufFree<hkAabb>(addAabbList.m_data, addAabbList.getCapacity());
        }
        addAabbList.m_data = nullptr;
        addAabbList.m_capacityAndFlags = hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG;
    }
    addObjectList._clearAndDeallocate(*hkMemoryRouter::getInstance().m_temp);
}

hkBool hkpWorld::removeEntity(hkpEntity* entity) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::RemoveEntity operation;
        operation.m_type.m_storage = hkWorldOperation::ENTITY_REMOVE;
        operation.m_entity = entity;
        queueOperation(operation);
        return 0;
    }
    lockCriticalOperations();
    hkpWorldOperationUtil::removeEntityBP(this, entity);
    hkpWorldCallbackUtil::fireEntityRemoved(this, entity);
    hkpEntityCallbackUtil::fireEntityRemoved(entity);
    m_criticalOperationsAllowed.m_bool = false;
    hkpWorldOperationUtil::removeEntitySI(this, entity);
    // This is hkpCollidable::BoundingVolumeData::hasAllocations: the test is on m_childShapeAabbs alone.
    if (entity->m_collidable.m_boundingVolumeData.m_childShapeAabbs) {
        entity->m_collidable.m_boundingVolumeData.deallocate();
    }
    if (!entity->m_memSizeAndFlags) {
        entity->deallocateInternalArrays();
    }
    entity->removeReference();
    bool kLastLock = m_criticalOperationsLockCount-- == 1;
    m_criticalOperationsAllowed.m_bool = true;
    if (kLastLock && !m_blockExecutingPendingOperations) {
        if (m_pendingOperationsCount) {
            internal_executePendingOperations();
        }
        if (m_pendingOperationQueueCount == 1 && m_pendingBodyOperationsCount) {
            internal_executePendingBodyOperations();
        }
    }
    return 1;
}

void hkpWorld::findInitialContactPoints(hkpEntity** entities, int32_t numEntities) {
    double v3;
    alignas(16) hkStepInfo v5;
    v5.m_startTime = this->m_simulation->m_currentPsiTime;
    v5.m_endTime = v5.m_startTime;
    v5.m_deltaTime = v5.m_startTime - v5.m_startTime;
    if (v5.m_deltaTime == 0.0) {
        v3 = 0.0;
    }
    else {
        v3 = 1.0 / v5.m_deltaTime;
    }
    hkpSimulation* m_simulation = this->m_simulation;
    v5.m_invDeltaTime = v3;
    m_simulation->collideEntitiesDiscrete(entities, numEntities, this, v5, hkpSimulation::FIND_CONTACTS_EXTRA);
}

// KEEP (partcopy:): on the locked path only the queued RemoveEntityBatch local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
void hkpWorld::removeEntityBatch(hkpEntity* const* entityBatch, int32_t numEntities) {
    if (numEntities < 1) {
        return;
    }
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::RemoveEntityBatch operation;
        operation.m_type.m_storage = hkWorldOperation::ENTITY_BATCH_REMOVE;
        operation.m_entities = const_cast<hkpEntity**>(entityBatch);
        operation.m_numEntities = static_cast<uint16_t>(numEntities);
        queueOperation(operation);
        return;
    }
    lockCriticalOperations();
    HK_TIMER_BEGIN_LIST("RemEntities", "Init+CallBck");
    m_simulation->resetCollisionInformationForEntities(
        const_cast<hkpEntity**>(entityBatch),
        numEntities,
        this,
        hkpSimulation::RESET_TOI
    );
    hkpEntity* const* const entityBatchEnd = entityBatch + numEntities;
    {
        // The broad-phase handles of the entities that have a shape, from the thread's temp allocator.
        hkArray<hkpBroadPhaseHandle*, hkContainerTempAllocator> removeList;
        removeList.reserveExactly(numEntities);
        for (hkpEntity* const* pEntity = entityBatch; pEntity != entityBatchEnd; ++pEntity) {
            if ((*pEntity)->m_collidable.m_shape) {
                removeList.pushBackUnchecked((*pEntity)->m_collidable.getBroadPhaseHandle());
            }
        }
        if (removeList.m_size) {
            HK_TIMER_SPLIT_LIST("BroadPhase");
            hkLocalArray<hkpBroadPhaseHandlePair> delPairs(m_broadPhaseQuerySize);
            getBroadPhase()->removeObjectBatch(removeList, delPairs);
            HK_TIMER_SPLIT_LIST("DelAgents");
            m_broadPhaseDispatcher->removePairs(
                static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.begin()),
                delPairs.m_size
            );
        }
    }
    HK_TIMER_SPLIT_LIST("RemoveCb");
    for (hkpEntity* const* pEntity = entityBatch; pEntity != entityBatchEnd; ++pEntity) {
        hkpWorldCallbackUtil::fireEntityRemoved(this, *pEntity);
        hkpEntityCallbackUtil::fireEntityRemoved(*pEntity);
        hkpWorldOperationUtil::removeEntitySI(this, *pEntity);
        if ((*pEntity)->m_collidable.m_boundingVolumeData.m_childShapeAabbs) {
            (*pEntity)->m_collidable.m_boundingVolumeData.deallocate();
        }
        if (!(*pEntity)->m_memSizeAndFlags) {
            (*pEntity)->deallocateInternalArrays();
        }
        (*pEntity)->removeReference();
    }
    HK_TIMER_END_LIST();
    unlockAndAttemptToExecutePendingOperations();
}

// KEEP (partcopy:): on the locked path only the queued RemovePhantomBatch local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
void hkpWorld::removePhantomBatch(hkpPhantom* const* phantomBatch, int32_t numPhantoms) {
    // Either lock count queues the batch instead.
    if (m_criticalOperationsLockCount + m_criticalOperationsLockCountForPhantoms) {
        hkWorldOperation::RemovePhantomBatch operation;
        operation.m_type.m_storage = hkWorldOperation::PHANTOM_BATCH_REMOVE;
        operation.m_phantoms = const_cast<hkpPhantom**>(phantomBatch);
        operation.m_numPhantoms = static_cast<uint16_t>(numPhantoms);
        queueOperation(operation);
        return;
    }
    lockCriticalOperations();
    hkLocalArray<hkpBroadPhaseHandle*> removeObjectList(numPhantoms);
    for (int32_t i = 0; i < numPhantoms; ++i) {
        hkpPhantom* phantom = phantomBatch[i];
        removeObjectList.pushBackUnchecked(phantom->m_collidable.getBroadPhaseHandle());
        hkpWorldCallbackUtil::firePhantomRemoved(this, phantom);
        phantom->firePhantomRemoved();
    }
    if (removeObjectList.m_size) {
        hkLocalArray<hkpBroadPhaseHandlePair> delPairs(m_broadPhaseQuerySize);
        getBroadPhase()->removeObjectBatch(removeObjectList, delPairs);
        m_broadPhaseDispatcher->removePairs(
            static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.begin()),
            delPairs.m_size
        );
    }
    for (int32_t i = 0; i < numPhantoms; ++i) {
        hkpPhantom* phantom = phantomBatch[i];
        phantom->m_world = nullptr;
        // The last element moves into the hole.
        const int32_t index = m_phantoms.indexOf(phantom);
        m_phantoms.removeAt(index);
        if (!phantom->m_memSizeAndFlags) {
            phantom->deallocateInternalArrays();
        }
        phantom->removeReference();
    }
    unlockAndAttemptToExecutePendingOperations();
}

// KEEP (partcopy:): on the locked path only the queued AddAction local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten. Also, when getEntities returns
// no entity, entities[0] is read from inline storage nothing wrote.
hkpAction* hkpWorld::addAction(hkpAction* action) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::AddAction operation;
        operation.m_type.m_storage = hkWorldOperation::ACTION_ADD;
        operation.m_action = action;
        queueOperation(operation);
        return nullptr;
    }
    action->addReference();
    lockCriticalOperations();
    hkInplaceArray<hkpEntity*, 4> entities;
    action->getEntities(entities);
    action->m_world = this;
    // The first entity on a movable island: the action goes to its island, and every other movable island is merged
    // into it.
    hkpEntity* mainEntity = nullptr;
    for (int32_t i = 0; i < entities.m_size; ++i) {
        entities.m_data[i]->m_actions.pushBack(action);
        hkpSimulationIsland* island = entities.m_data[i]->m_simulationIsland;
        if (!island->isFixed().m_bool) {
            if (!mainEntity) {
                mainEntity = entities.m_data[i];
                island->addAction(action);
            }
            else if (mainEntity->m_simulationIsland != island) {
                hkpWorldOperationUtil::mergeIslands(this, mainEntity, entities.m_data[i]);
            }
        }
    }
    if (!mainEntity) {
        entities.m_data[0]->m_simulationIsland->addAction(action);
    }
    unlockAndAttemptToExecutePendingOperations();
    hkpWorldCallbackUtil::fireActionAdded(this, action);
    return action;
}

// KEEP (partcopy:): on the locked path only the queued RemoveAction local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
void hkpWorld::removeAction(hkpAction* action) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::RemoveAction operation;
        operation.m_type.m_storage = hkWorldOperation::ACTION_REMOVE;
        operation.m_action = action;
        queueOperation(operation);
        return;
    }
    removeActionImmediately(action);
}

void hkpWorld::removeActionImmediately(hkpAction* action) {
    action->addReference();
    lockCriticalOperations();
    hkpWorldCallbackUtil::fireActionRemoved(this, action);
    hkInplaceArray<hkpEntity*, 4> entities;
    action->getEntities(entities);
    for (int32_t i = 0; i < entities.m_size; ++i) {
        hkpEntity* entity = entities.m_data[i];
        entity->m_actions.removeAt(entity->m_actions.indexOf(action));
    }
    hkpSimulationIsland* island = action->m_island;
    island->removeAction(action);
    action->m_world = nullptr;
    action->removeReference();
    hkpWorldOperationUtil::putIslandOnDirtyList(island->getWorld(), island);
    unlockAndAttemptToExecutePendingOperations();
    action->removeReference();
}

// The entity drops the action (the last action moves into its slot; an action the entity does not hold overwrites slot
// -1, deliberately) and asks its island for a split check. The action then moves to the island of its
// first other entity that is not in the fixed island (else the last other entity's), when that differs from its own.
// The entity list is a four-entry inplace array.
void hkpWorld::detachActionFromEntity(hkpAction* action, hkpEntity* entity) {
    int32_t index = -1;
    for (int32_t i = 0; i < entity->m_actions.m_size; ++i) {
        if (entity->m_actions.m_data[i] == action) {
            index = i;
            break;
        }
    }
    entity->m_actions.removeAt(index);
    hkpSimulationIsland* entityIsland = entity->m_simulationIsland;
    entityIsland->m_splitCheckRequested = 1;

    hkInplaceArray<hkpEntity*, 4> entities;
    action->getEntities(entities);
    hkpSimulationIsland* newIsland = nullptr;
    for (int32_t i = 0; i < entities.m_size; ++i) {
        if (entities.m_data[i] != entity) {
            newIsland = entities.m_data[i]->m_simulationIsland;
            if (!newIsland->isFixed().m_bool) {
                break;
            }
        }
    }
    hkpSimulationIsland* oldIsland = action->m_island;
    if (newIsland != oldIsland) {
        oldIsland->removeAction(action);
        newIsland->addAction(action);
    }
}

void hkpWorld::getMemUsageForIntegration(hkpWorld::MemUsageInfo& infoOut) {
    infoOut.m_maxRuntimeBlockSize = 0;
    infoOut.m_sumRuntimeBlockSize = 0;
    infoOut.m_largestSimulationIsland = nullptr;
    for (int32_t i = m_activeSimulationIslands.m_size - 1; i >= 0; --i) {
        hkpSimulationIsland* island = m_activeSimulationIslands.m_data[i];
        const int32_t size = island->getMemUsageForIntegration();
        infoOut.m_sumRuntimeBlockSize += size;
        if (size > infoOut.m_maxRuntimeBlockSize) {
            infoOut.m_maxRuntimeBlockSize = size;
            infoOut.m_largestSimulationIsland = island;
        }
    }
}

void hkpWorld::internal_executePendingOperations() {
    m_pendingOperationsCount = 0;
    m_pendingOperations->executeAllPending();
}

void hkpWorld::internal_executePendingBodyOperations() {
    m_pendingBodyOperationsCount = 0;
    m_pendingOperations->executeAllPendingBodyOperations();
}

float hkpWorld::getCurrentTime() const {
    return m_simulation->m_currentTime;
}

float hkpWorld::getCurrentPsiTime() const {
    return m_simulation->m_currentPsiTime;
}

void hkpWorld::setFrameTimeMarker(float frameDeltaTime) {
    m_simulation->hkpSimulation::setFrameTimeMarker(frameDeltaTime);
}

hkpStepResult hkpWorld::advanceTime() {
    lock();
    uint32_t stepResult = m_simulation->advanceTime();
    unlock();
    return static_cast<hkpStepResult>(stepResult);
}

bool hkpWorld::isSimulationAtMarker() const {
    return m_simulation->isSimulationAtMarker();
}

hkpStepResult hkpWorld::stepDeltaTime(float physicsDeltaTime) {
    lock();
    hkpWorldMemoryUtil::watchHeapMemory(this);
    uint32_t stepResult = m_simulation->stepDeltaTime(physicsDeltaTime);
    if (m_kdTreeManager) {
        if (m_autoUpdateTree) {
            updateKdTree(nullptr, nullptr);
            unlock();
            return static_cast<hkpStepResult>(stepResult);
        }
        markTreeDirty();
    }
    unlock();
    return static_cast<hkpStepResult>(stepResult);
}

hkpStepResult hkpWorld::initMtStep(hkJobQueue* jobQueue, float physicsDeltaTime) {
    if (m_memoryWatchDog) {
        lock();
        hkpWorldMemoryUtil::watchHeapMemory(this);
        unlock();
    }
    return static_cast<hkpMultiThreadedSimulation*>(m_simulation)->stepBeginSt(jobQueue, physicsDeltaTime);
}

hkpStepResult hkpWorld::stepMultithreaded(hkJobQueue* jobQueue, hkJobThreadPool* threadPool, float physicsDeltaTime) {
    hkpStepResult result = initMtStep(jobQueue, physicsDeltaTime);
    if (result == HK_STEP_RESULT_SUCCESS) {
        threadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
        jobQueue->processAllJobs();
        threadPool->waitForCompletion();
        return finishMtStep(jobQueue, threadPool);
    }
    return result;
}

void hkpWorld::findInitialContactPointsOfAllEntities() {
    int32_t i;
    hkpSimulationIsland* v3;
    int32_t j;
    hkpSimulationIsland* v5;
    for (i = 0; i < m_activeSimulationIslands.m_size; ++i) {
        v3 = m_activeSimulationIslands.m_data[i];
        findInitialContactPoints(v3->m_entities.m_data, v3->m_entities.m_size);
    }
    for (j = 0; j < m_inactiveSimulationIslands.m_size; ++j) {
        v5 = m_inactiveSimulationIslands.m_data[j];
        findInitialContactPoints(v5->m_entities.m_data, v5->m_entities.m_size);
    }
}

void hkpWorld::lock() {
    hkReferencedObject::lockAll();
    ++m_isLocked;
}

void hkpWorld::unlock() {
    --m_isLocked;
    hkReferencedObject::unlockAll();
}

void hkpWorld::lockReadOnly() {
    ;
}

void hkpWorld::unlockReadOnly() {
    ;
}

bool hkpWorld::checkUnmarked() {
    return true;
}

hkWorldMemoryAvailableWatchDog* hkpWorld::getMemoryWatchDog() const {
    return m_memoryWatchDog;
}

void hkpWorld::checkDeterminism() {
    ;
}

void hkpWorld::checkDeterminismOfIslandBroadPhase(const hkpSimulationIsland* island) {
    ;
}

hkpStepResult hkpWorld::collide() {
    lock();
    int32_t v2 = m_simulation->collide();
    unlock();
    return static_cast<hkpStepResult>(v2);
}

void hkpWorld::addPhysicsSystem(const hkpPhysicsSystem* sys) {
    // An array is added as one batch only when none of its slots is empty; one empty slot makes it add the objects one
    // at a time.
    if (sys->m_rigidBodies.m_size > 0) {
        int32_t i;
        for (i = 0; i < sys->m_rigidBodies.m_size; ++i) {
            if (!sys->m_rigidBodies.m_data[i]) {
                break;
            }
        }
        if (i == sys->m_rigidBodies.m_size) {
            // The rigid body array is handed to the entity batch call unchanged: an hkpRigidBody is an hkpEntity and
            // both arrays hold pointers, so the same address is passed. The untyped local is that hand-over.
            void* pBatch = sys->m_rigidBodies.m_data;
            addEntityBatch(
                static_cast<hkpEntity* const*>(pBatch),
                sys->m_rigidBodies.m_size,
                sys->m_active.m_bool ? HK_ENTITY_ACTIVATION_DO_ACTIVATE : HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE
            );
        }
        else {
            for (int32_t j = 0; j < sys->m_rigidBodies.m_size; ++j) {
                hkpRigidBody* body = sys->m_rigidBodies.m_data[j];
                if (body) {
                    addEntity(
                        body,
                        sys->m_active.m_bool ? HK_ENTITY_ACTIVATION_DO_ACTIVATE : HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE
                    );
                }
            }
        }
    }
    if (sys->m_phantoms.m_size > 0) {
        int32_t i;
        for (i = 0; i < sys->m_phantoms.m_size; ++i) {
            if (!sys->m_phantoms.m_data[i]) {
                break;
            }
        }
        if (i == sys->m_phantoms.m_size) {
            addPhantomBatch(sys->m_phantoms.m_data, sys->m_phantoms.m_size);
        }
        else {
            for (int32_t j = 0; j < sys->m_phantoms.m_size; ++j) {
                hkpPhantom* phantom = sys->m_phantoms.m_data[j];
                if (phantom) {
                    addPhantom(phantom);
                }
            }
        }
    }
    for (int32_t i = 0; i < sys->m_actions.m_size; ++i) {
        hkpAction* action = sys->m_actions.m_data[i];
        if (action) {
            addAction(action);
        }
    }
    // A constraint needs at least one entity; one to the world alone is skipped.
    for (int32_t i = 0; i < sys->m_constraints.m_size; ++i) {
        hkpConstraintInstance* constraint = sys->m_constraints.m_data[i];
        if (constraint && (constraint->m_entities[0] || constraint->m_entities[1])) {
            addConstraint(constraint);
        }
    }
}

// Builds a system for the inactive entities when there are any, then one for everything else.
void hkpWorld::getWorldAsSystems(hkArray<hkpPhysicsSystem*>& systemsInOut) const {
    hkpPhysicsSystem* pSystem = new hkpPhysicsSystem;
    if (enumerateAllInactiveEntitiesInWorld(this, pSystem).m_bool) {
        pSystem->m_active.m_bool = false;
        systemsInOut.pushBack(pSystem);
        pSystem = nullptr;
    }
    if (!pSystem) {
        pSystem = new hkpPhysicsSystem;
    }
    systemsInOut.pushBack(pSystem);
    enumerateAllActiveEntitiesInWorld(this, pSystem);
    enumerateAllPhantomsInWorld(this, pSystem);
    enumerateAllConstraintsInWorld(this, pSystem);
    enumerateAllActionsInWorld(this, pSystem);
}

void hkpWorld::markTreeDirty() {
    if (m_kdTreeManager) {
        m_kdTreeManager->setUpToDate(false);
    }
}

void hkpWorld::updateKdTree(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
    m_kdTreeManager->updateFromWorld(jobQueue, jobThreadPool);
}

void hkpWorld::lockIslandForConstraintUpdate(hkpSimulationIsland* island) {
    if (m_modifyConstraintCriticalSection) {
        EnterCriticalSection(&m_modifyConstraintCriticalSection->m_section);
    }
}

void hkpWorld::lockForIslandSplit(hkpSimulationIsland* island) {
    if (m_modifyConstraintCriticalSection) {
        EnterCriticalSection(&m_modifyConstraintCriticalSection->m_section);
    }
}

void hkpWorld::addBodyOperation(
    hkpRigidBody* breakingBody,
    hkpBodyOperation* operation,
    int32_t priority,
    hkpBodyOperation::ExecutionState hint
) {
    m_pendingOperations->queueBodyOperation(breakingBody, operation, priority, hint);
}

void hkpWorld::castRay(const hkpWorldRayCastInput& input, hkpRayHitCollector& collector) const {
    HK_TIMER_BEGIN("worldCastRayCollector", nullptr);
    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        m_kdTreeManager->castRay(input, m_collisionFilter, collector);
    }
    else {
        hkpCollisionFilter* filter = m_collisionFilter;
        hkpBroadPhase* broadPhase = m_broadPhase;
        hkpWorldRayCaster rayCaster;
        rayCaster.castRay(*broadPhase, input, filter, nullptr, collector);
    }
    HK_TIMER_END();
}

void hkpWorld::castRay(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput& output) const {
    HK_TIMER_BEGIN("worldCastRay", nullptr);
    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        m_kdTreeManager->castRay(input, m_collisionFilter, output);
    }
    else {
        hkpCollisionFilter* filter = m_collisionFilter;
        hkpBroadPhase* broadPhase = m_broadPhase;
        hkpSimpleWorldRayCaster rayCaster;
        rayCaster.castRay(*broadPhase, input, filter, nullptr, output);
    }
    HK_TIMER_END();
}

void hkpWorld::linearCast(
    const hkpCollidable* collA,
    const hkpLinearCastInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) const {
    HK_TIMER_BEGIN("worldLinCast", nullptr);
    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        const hkpCollidableCollidableFilter* filter = static_cast<hkpCollidableCollidableFilter*>(m_collisionFilter);
        m_kdTreeManager->linearCast(
            collA,
            input,
            filter,
            *m_collisionInput,
            m_collisionInput->m_config,
            castCollector,
            startCollector
        );
    }
    else {
        hkpWorldLinearCaster caster;
        hkpCollisionFilter* collisionFilter = m_collisionFilter;
        const hkpCollidableCollidableFilter* filter = static_cast<hkpCollidableCollidableFilter*>(collisionFilter);
        caster.linearCast(
            *m_broadPhase,
            collA,
            input,
            filter,
            *m_collisionInput,
            m_collisionInput->m_config,
            nullptr,
            castCollector,
            startCollector
        );
    }
    HK_TIMER_END();
}

// The SDK's getWorldAsOneSystem: a new system (0x44 bytes from the thread heap) holding all of the world's entities,
// phantoms, constraints and actions. The helpers take the world and the system in registers.
hkpPhysicsSystem* hkpWorld::getWorldAsOneSystem() const {
    hkpPhysicsSystem* sys = new hkpPhysicsSystem;
    enumerateAllEntitiesInWorld(this, sys);
    enumerateAllPhantomsInWorld(this, sys);
    enumerateAllConstraintsInWorld(this, sys);
    enumerateAllActionsInWorld(this, sys);
    return sys;
}

// KEEP (partcopy:): queueOperation hands the operation to hkpWorldOperationQueue::queueOperation, which copies a whole
// BiggestOperation whatever its type; only m_type and m_aabb are written, so the local's padding and the storage past
// its end reach the queue unwritten.
void hkpWorld::activateRegion(const hkAabb& aabb) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::ActivateRegion operation;
        operation.m_type.m_storage = hkWorldOperation::ACTIVATE_REGION;
        operation.m_aabb = hkAllocateChunk<hkAabb>(1, HK_MEMORY_CLASS_DYNAMICS);
        hkString::memCpy(operation.m_aabb, &aabb, sizeof(hkAabb));
        queueOperation(operation);
        return;
    }
    hkArray<hkpBroadPhaseHandlePair, hkContainerHeapAllocator> pairsOut;
    m_broadPhase->querySingleAabb(aabb, pairsOut);
    for (const hkpBroadPhaseHandlePair& pair : pairsOut) {
        hkpTypedBroadPhaseHandle* handle = static_cast<hkpTypedBroadPhaseHandle*>(pair.m_b);
        hkpCollidable* collidable = static_cast<hkpCollidable*>(handle->getOwner());
        hkpEntity* entity = hkpGetRigidBody(collidable);
        if (entity) {
            entity->activate();
        }
    }
}

void hkpWorld::getPenetrations(
    const hkpCollidable* collA,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) const {
    HK_TIMER_BEGIN_LIST("hkpWorld::getPenetrations", "broadphase");
    hkAabb aabb;
    // The tolerance is stored as a float, then clamped at 0 (a NaN is kept).
    float tolerance = input.m_tolerance - m_collisionInput->m_tolerance * 0.5;
    if (tolerance < 0.0) {
        tolerance = 0.0f;
    }
    collA->m_shape->getAabbImpl(collA->m_motion.pState->m_transform, tolerance, aabb);

    alignas(16) hkInplaceArray<hkpBroadPhaseHandlePair, 128> hits;
    m_broadPhase->querySingleAabb(aabb, hits);

    hkpShapeType typeA = static_cast<hkpShapeType>(collA->m_shape->m_type.m_storage);
    HK_TIMER_SPLIT_LIST("narrowphase");
    hkpBroadPhaseHandlePair* pair = hits.m_data;
    for (int32_t i = hits.m_size - 1; i >= 0; ++pair, --i) {
        hkpTypedBroadPhaseHandle* handleB = static_cast<hkpTypedBroadPhaseHandle*>(pair->m_b);
        const hkpCollidable* collB = static_cast<hkpCollidable*>(handleB->getOwner());
        if (collA == collB) {
            continue;
        }
        if (!m_collisionFilter->isCollisionEnabled(*collA, *collB).m_bool) {
            continue;
        }
        const hkpShape* shapeB = collB->m_shape;
        if (!shapeB) {
            continue;
        }
        hkpShapeType typeB = static_cast<hkpShapeType>(shapeB->m_type.m_storage);
        hkpCollisionDispatcher::GetPenetrationsFunc getPenetrationsFunc = input.m_dispatcher->getGetPenetrationsFunc(
            typeA,
            typeB
        );
        getPenetrationsFunc(*collA, *collB, input, collector);
        if (collector.m_earlyOut.m_bool) {
            break;
        }
    }
    HK_TIMER_END_LIST();
}

void hkpWorld::setBroadPhaseBorder(hkpBroadPhaseBorder* b) {
    hkpBroadPhaseBorder* m_broadPhaseBorder = this->m_broadPhaseBorder;
    if (m_broadPhaseBorder) {
        m_broadPhaseBorder->deactivate();
        this->m_broadPhaseBorder->removeReference();
    }
    this->m_broadPhaseBorder = b;
    if (b) {
        b->addReference();
    }
}

hkpBroadPhaseBorder* hkpWorld::getBroadPhaseBorder() const {
    return m_broadPhaseBorder;
}

// KEEP (partcopy:): on the locked path only the queued ReintegrateAndRecollideEntityBatch local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
void hkpWorld::reintegrateAndRecollideEntities(
    hkpEntity** entityBatch,
    int32_t numEntities,
    hkpWorld::ReintegrationRecollideMode mode
) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::ReintegrateAndRecollideEntityBatch operation;
        operation.m_type.m_storage = hkWorldOperation::ENTITY_BATCH_REINTEGRATE_AND_RECOLLIDE;
        operation.m_entities = entityBatch;
        operation.m_numEntities = static_cast<uint16_t>(numEntities);
        operation.m_mode = static_cast<uint8_t>(mode);
        queueOperation(operation);
    }
    else {
        m_simulation->reintegrateAndRecollideEntities(entityBatch, numEntities, this, mode);
    }
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removeActionListener(hkpActionListener* worldListener) {
    int32_t m_size = m_actionListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpActionListener** m_data = m_actionListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_actionListeners.m_data[v3] = 0;
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removeConstraintListener(hkpConstraintListener* worldListener) {
    int32_t m_size = m_constraintListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpConstraintListener** m_data = m_constraintListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_constraintListeners.m_data[v3] = 0;
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removePhantomListener(hkpPhantomListener* worldListener) {
    int32_t m_size = m_phantomListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpPhantomListener** m_data = m_phantomListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_phantomListeners.m_data[v3] = 0;
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removeWorldPostSimulationListener(hkpWorldPostSimulationListener* worldListener) {
    int32_t m_size = m_worldPostSimulationListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpWorldPostSimulationListener** m_data = m_worldPostSimulationListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_worldPostSimulationListeners.m_data[v3] = 0;
}

// A listener not in the array gives index -1, and the entry before the buffer is then cleared, deliberately.
void hkpWorld::removeContactListener(hkpContactListener* collisionListener) {
    int32_t m_size = m_contactListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpContactListener** m_data = m_contactListeners.begin();
        while (*m_data != collisionListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_contactListeners.m_data[v3] = 0;
}

// The index is -1 when the listener is absent, and the slot before the array's first element is then cleared,
// deliberately, as Havok does.
void hkpWorld::removeWorldDeletionListener(hkpWorldDeletionListener* worldListener) {
    const int32_t i = m_worldDeletionListeners.indexOf(worldListener);
    m_worldDeletionListeners.m_data[i] = nullptr;
}

hkpStepResult hkpWorld::integrate(float physicsDeltaTime) {
    lock();
    hkpStepResult v3 = m_simulation->integrate(physicsDeltaTime);
    unlock();
    return v3;
}

hkpStepResult hkpWorld::finishMtStep(hkJobQueue* jobQueue, hkJobThreadPool* threadPool) {
    hkpStepResult result = static_cast<hkpMultiThreadedSimulation*>(m_simulation)
                               ->hkpMultiThreadedSimulation::finishMtStep(jobQueue, threadPool);
    if (result == HK_STEP_RESULT_SUCCESS) {
        if (m_kdTreeManager) {
            if (m_autoUpdateTree) {
                updateKdTree(jobQueue, threadPool);
                return HK_STEP_RESULT_SUCCESS;
            }
            markTreeDirty();
        }
        return HK_STEP_RESULT_SUCCESS;
    }
    return result;
}

// Both snap thresholds are stored into m_snapCollisionToConvexEdgeThreshold, the concave one last.
void hkpWorld::getCinfo(hkpWorldCinfo& info) const {
    hkBool* p_m_postponeAndSortCallbacks;
    hkBool v13;
    info.m_gravity = m_gravity;
    info.m_broadPhaseQuerySize = m_broadPhaseQuerySize;
    info.m_broadPhaseWorldAabb.m_min = m_broadPhaseExtents[0];
    info.m_broadPhaseWorldAabb.m_max = m_broadPhaseExtents[1];
    info.m_collisionTolerance = m_collisionInput->m_tolerance;
    hkpCollisionFilter* m_collisionFilter = this->m_collisionFilter;
    if (m_collisionFilter) {
        this->m_collisionFilter->addReference();
    }
    hkReferencedObject* m_collisionFilter_1 = info.m_collisionFilter;
    if (m_collisionFilter_1) {
        m_collisionFilter_1->removeReference();
    }
    info.m_collisionFilter = m_collisionFilter;
    hkpConvexListFilter* m_convexListFilter = this->m_convexListFilter;
    if (m_convexListFilter) {
        this->m_convexListFilter->addReference();
    }
    hkReferencedObject* m_convexListFilter_1 = info.m_convexListFilter;
    if (m_convexListFilter_1) {
        m_convexListFilter_1->removeReference();
    }
    info.m_convexListFilter = m_convexListFilter;
    info.m_broadPhaseBorderBehaviour.m_storage = m_broadPhaseBorder ? m_broadPhaseBorder->m_type
                                                                    : hkpWorldCinfo::BROADPHASE_BORDER_DO_NOTHING;
    hkpBroadPhaseBorder* m_broadPhaseBorder_1 = this->m_broadPhaseBorder;
    if (m_broadPhaseBorder_1) {
        p_m_postponeAndSortCallbacks = &m_broadPhaseBorder_1->m_postponeAndSortCallbacks;
    }
    else {
        v13.m_bool = 0;
        p_m_postponeAndSortCallbacks = &v13;
    }
    info.m_mtPostponeAndSortBroadPhaseBorderCallbacks.m_bool = p_m_postponeAndSortCallbacks->m_bool;
    info.m_useKdTree.m_bool = m_kdTreeManager != nullptr;
    info.m_autoUpdateKdTree.m_bool = m_autoUpdateTree.m_bool;
    info.m_expectedMaxLinearVelocity = m_collisionDispatcher->m_expectedMaxLinearVelocity;
    info.m_expectedMinPsiDeltaTime = m_collisionDispatcher->m_expectedMinPsiDeltaTime;
    info.m_snapCollisionToConvexEdgeThreshold = m_snapCollisionToConvexEdgeThreshold;
    info.m_snapCollisionToConvexEdgeThreshold = m_snapCollisionToConcaveEdgeThreshold;
    info.m_enableToiWeldRejection.m_bool = m_enableToiWeldRejection.m_bool;
    hkWorldMemoryAvailableWatchDog* m_pntr = this->m_memoryWatchDog.val();
    if (m_pntr) {
        m_pntr->addReference();
    }
    hkReferencedObject* m_memoryWatchDog = info.m_memoryWatchDog;
    if (m_memoryWatchDog) {
        m_memoryWatchDog->removeReference();
    }
    info.m_memoryWatchDog = this->m_memoryWatchDog.val();
    info.m_broadPhaseNumMarkers = m_broadPhaseNumMarkers;
    info.m_sizeOfToiEventQueue = m_sizeOfToiEventQueue;
    info.m_contactPointGeneration.m_storage = m_contactPointGeneration.m_storage;
    info.m_allowToSkipConfirmedCallbacks.m_bool = m_collisionInput->m_allowToSkipConfirmedCallbacks.m_bool;
    info.m_contactRestingVelocity = m_dynamicsStepInfo.m_solverInfo.m_contactRestingVelocity;
    info.m_solverTau = m_dynamicsStepInfo.m_solverInfo.m_tau;
    info.m_solverDamp = m_dynamicsStepInfo.m_solverInfo.m_damping;
    info.m_solverIterations = m_dynamicsStepInfo.m_solverInfo.m_numSteps;
    info.m_solverMicrosteps = m_dynamicsStepInfo.m_solverInfo.m_numMicroSteps;
    float m_maxConstraintViolation = std::sqrt(
        static_cast<double>(m_dynamicsStepInfo.m_solverInfo.m_maxConstraintViolationSqrd)
    );
    info.m_maxConstraintViolation = m_maxConstraintViolation;
    info.m_forceCoherentConstraintOrderingInSolver.m_bool = m_dynamicsStepInfo.m_solverInfo
                                                                .m_forceCoherentConstraintOrderingInSolver.m_bool;
    info.m_deactivationNumInactiveFramesSelectFlag0 = m_dynamicsStepInfo.m_solverInfo
                                                          .m_deactivationNumInactiveFramesSelectFlag[0];
    info.m_deactivationNumInactiveFramesSelectFlag1 = m_dynamicsStepInfo.m_solverInfo
                                                          .m_deactivationNumInactiveFramesSelectFlag[1];
    info.m_deactivationIntegrateCounter = m_dynamicsStepInfo.m_solverInfo.m_deactivationIntegrateCounter;
    info.m_iterativeLinearCastEarlyOutDistance = m_collisionInput->m_config->m_iterativeLinearCastEarlyOutDistance;
    info.m_iterativeLinearCastMaxIterations = m_collisionInput->m_config->m_iterativeLinearCastMaxIterations;
    info.m_enableDeprecatedWelding.m_bool = m_collisionInput->m_enableDeprecatedWelding.m_bool;
    info.m_shouldActivateOnRigidBodyTransformChange.m_bool = m_shouldActivateOnRigidBodyTransformChange.m_bool;
    info.m_toiCollisionResponseRotateNormal = m_toiCollisionResponseRotateNormal;
    info.m_maxSectorsPerCollideTask = m_maxSectorsPerCollideTask;
    info.m_processToisMultithreaded.m_bool = m_processToisMultithreaded.m_bool;
    info.m_maxEntriesPerToiCollideTask = m_maxEntriesPerToiCollideTask;
    info.m_maxNumToiCollisionPairsSinglethreaded = m_maxNumToiCollisionPairsSinglethreaded;
    info.m_deactivationReferenceDistance = m_deactivationReferenceDistance;
    info.m_numToisTillAllowedPenetrationSimplifiedToi = m_numToisTillAllowedPenetrationSimplifiedToi;
    info.m_numToisTillAllowedPenetrationToi = m_numToisTillAllowedPenetrationToi;
    info.m_numToisTillAllowedPenetrationToiHigher = m_numToisTillAllowedPenetrationToiHigher;
    info.m_numToisTillAllowedPenetrationToiForced = m_numToisTillAllowedPenetrationToiForced;
    info.m_enableDeactivation.m_bool = m_wantDeactivation.m_bool;
    info.m_simulationType.m_storage = m_simulationType;
    info.m_frameMarkerPsiSnap = m_simulation->m_frameMarkerPsiSnap;
    info.m_enableSimulationIslands.m_bool = m_wantSimulationIslands.m_bool;
    info.m_processActionsInSingleThread.m_bool = m_processActionsInSingleThread.m_bool;
    info.m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob
        .m_bool = m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob.m_bool;
    info.m_minDesiredIslandSize = m_minDesiredIslandSize;
    info.m_fireCollisionCallbacks.m_bool = m_collisionCallbackUtil != nullptr;
}

// KEEP (partcopy:): the job (a 64-byte local in the realigned frame) has bytes 3 (hkJob's padding03), 8-15 (hkJob's
// alignment padding) and 0x3C-0x3F (the job's tail padding) unwritten; each sub-job is a whole copy of it, and
// hkJobQueue::addJob copies m_size (64) bytes of it into the queue.
void hkpWorld::getClosestPointsMt(
    hkpWorldGetClosestPointsCommand* commandArray,
    int32_t numCommands,
    hkJobQueue* jobQueue,
    hkJobThreadPool* jobThreadPool,
    hkSemaphore* semaphore,
    int32_t numCommandsPerJob
) {
    hkpCollisionQueryJobHeader* jobHeader = static_cast<hkpCollisionQueryJobHeader*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCollisionQueryJobHeader))
    );

    hkpWorldGetClosestPointsJob job(
        m_collisionInput,
        jobHeader,
        commandArray,
        numCommands,
        m_broadPhase,
        m_collisionInput->m_tolerance,
        semaphore
    );
    job.setRunsOnSpuOrPpu();

    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        job.m_aabbTreeInt16 = m_kdTreeManager->m_inactiveTree;
        job.m_kdTree = m_kdTreeManager->m_kdTree;
        job.m_broadphase = nullptr;
    }

    while (job.m_numCommands > 0) {
        hkpWorldGetClosestPointsJob subJob = job;
        subJob.m_numCommands = job.m_numCommands < numCommandsPerJob ? job.m_numCommands : numCommandsPerJob;
        job.m_commandArray += numCommandsPerJob;
        job.m_numCommands -= numCommandsPerJob;
        jobQueue->addJob(subJob, hkJobQueue::JOB_LOW_PRIORITY);
    }

    jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
    jobThreadPool->waitForCompletion();
    semaphore->acquire();
    hkMemoryRouter::getInstance().heap().blockFree(jobHeader, sizeof(hkpCollisionQueryJobHeader));
}

// KEEP (partcopy:): on the locked path only the queued UserCallbackOperation local's members are written;
// hkpWorldOperationQueue::queueOperation (through hkpWorld::queueOperation) copies a whole BiggestOperation into the
// queue, so the local's padding and the storage past its end reach the queue unwritten.
hkWorldOperation::UserCallback* hkpWorld::queueCallback(hkWorldOperation::UserCallback* callback, uint32_t userData) {
    if (m_criticalOperationsLockCount) {
        hkWorldOperation::UserCallbackOperation operation;
        operation.m_type.m_storage = hkWorldOperation::USER_CALLBACK;
        operation.m_userCallback = callback;
        operation.m_userData = userData;
        queueOperation(operation);
    }
    else {
        callback->worldOperationUserCallback(userData);
    }
    return callback;
}

void hkpWorld::attachActionToEntity(hkpAction* action, hkpEntity* entity) {
    entity->m_actions.pushBack(action);
    hkpSimulationIsland* m_island = action->m_island;
    if (m_island->m_storageIndex != 0xFFFF || entity->m_motion.m_type.m_storage == 5) {
        if (entity->getSimulationIsland() != m_island && entity->m_motion.m_type.m_storage != 5) {
            hkpWorldOperationUtil::mergeIslands(this, entity, *m_island->m_entities.begin());
        }
    }
    else {
        m_island->removeAction(action);
        entity->getSimulationIsland()->addAction(action);
    }
}

void hkpWorld::removeAll() {
    while (m_phantoms.m_size > 0) {
        removePhantom(*m_phantoms.begin());
    }
    while (m_activeSimulationIslands.m_size > 0) {
        hkpSimulationIsland** m_data = m_activeSimulationIslands.begin();
        if (!(*m_data)->m_entities.m_size) {
            break;
        }
        removeEntity(*(*m_data)->m_entities.begin());
    }
    while (m_inactiveSimulationIslands.m_size > 0) {
        removeEntity(*(*m_inactiveSimulationIslands.begin())->m_entities.begin());
    }
    m_fixedRigidBody->addReference();
    removeEntity(m_fixedRigidBody);
    for (hkpSimulationIsland* i = m_fixedIsland; i->m_entities.m_size > 0; i = m_fixedIsland) {
        removeEntity(*i->m_entities.begin());
    }
    addEntity(m_fixedRigidBody, HK_ENTITY_ACTIVATION_DO_ACTIVATE);
    m_fixedRigidBody->removeReference();
    if (!m_wantSimulationIslands.m_bool) {
        hkpWorldOperationUtil::removeIslandFromDirtyList(
            this,
            m_activeSimulationIslands.m_data[m_activeSimulationIslands.m_size - 1]
        );
        hkpSimulationIsland* v4 = m_activeSimulationIslands.m_data[m_activeSimulationIslands.m_size - 1];
        if (v4) {
            delete v4;
        }
        --m_activeSimulationIslands.m_size;
    }
    hkpWorldCallbackUtil::fireWorldRemoveAll(this);
}

void hkpWorld::addActionListener(hkpActionListener* worldListener) {
    m_actionListeners.pushBack(worldListener);
}

void hkpWorld::addConstraintListener(hkpConstraintListener* worldListener) {
    m_constraintListeners.pushBack(worldListener);
}

void hkpWorld::addWorldPostSimulationListener(hkpWorldPostSimulationListener* worldListener) {
    m_worldPostSimulationListeners.pushBack(worldListener);
}

void hkpWorld::addWorldDeletionListener(hkpWorldDeletionListener* worldListener) {
    m_worldDeletionListeners.pushBack(worldListener);
}

// collA's box, grown by input's tolerance less half the world's, is queried in the trees (the inactive AABB tree, then
// the kd-tree) when the tree manager is up to date, else in the broadphase; each other collidable the filter accepts
// and that has a shape is handed to the dispatcher's closest-points function. The tree hit arrays end before the timer
// list, the broadphase one after it.
void hkpWorld::getClosestPoints(
    const hkpCollidable* collA,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    hkAabb aabb;
    {
        const float tolerance = input.m_tolerance - m_collisionInput->m_tolerance * 0.5;
        collA->getShape()->getAabb(*collA->m_motion.pMotion, tolerance, aabb);
    }
    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        HK_TIMER_BEGIN_LIST("hkpWorld::getClosestPoints(Trees)", "AabbTreeBp");
        hkpTreeWorldManager* treeManager = m_kdTreeManager;
        const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* inactiveTree = treeManager->m_inactiveTree;
        const hkKdTree* kdTree = treeManager->m_kdTree;
        if (inactiveTree) {
            hkInplaceArray<void*, 128> hits;
            queryAabbToArray(inactiveTree, aabb, &hits);
            HK_TIMER_SPLIT_LIST("AabbTreeNp");
            const hkpShapeType typeA = collA->getShape()->m_type;
            for (void* hit : hits) {
                const hkpCollidable* collB = static_cast<const hkpCollidable*>(hit);
                if (collA == collB) {
                    continue;
                }
                if (!m_collisionFilter->isCollisionEnabled(*collA, *collB).m_bool) {
                    continue;
                }
                const hkpShape* shapeB = collB->getShape();
                if (!shapeB) {
                    continue;
                }
                input.m_dispatcher->getGetClosestPointsFunc(typeA, shapeB->m_type)(*collA, *collB, input, collector);
            }
        }
        if (kdTree) {
            HK_TIMER_SPLIT_LIST("KdTreeBp");
            hkArray<hkUlong, hkContainerTempAllocator> hits;
            hkKdTreeAabbQueryUtils::queryAabbIterative(kdTree, aabb, hits);
            HK_TIMER_SPLIT_LIST("KdTreeNp");
            const hkpShapeType typeA = collA->getShape()->m_type;
            for (hkUlong hit : hits) {
                // The SDK's cast: a kd-tree hit is an hkPrimitiveId, the collidable's address.
                const hkpCollidable* collB = reinterpret_cast<const hkpCollidable*>(hit);
                if (collA == collB) {
                    continue;
                }
                if (!m_collisionFilter->isCollisionEnabled(*collA, *collB).m_bool) {
                    continue;
                }
                const hkpShape* shapeB = collB->getShape();
                if (!shapeB) {
                    continue;
                }
                input.m_dispatcher->getGetClosestPointsFunc(typeA, shapeB->m_type)(*collA, *collB, input, collector);
            }
        }
        HK_TIMER_END_LIST();
    }
    else {
        HK_TIMER_BEGIN_LIST("hkpWorld::getClosestPoints(Broadphase)", "3AxisBp");
        hkInplaceArray<hkpBroadPhaseHandlePair, 128> hits;
        m_broadPhase->querySingleAabb(aabb, hits);
        hkpBroadPhaseHandlePair* pair = hits.m_data;
        const hkpShapeType typeA = collA->getShape()->m_type;
        HK_TIMER_SPLIT_LIST("3AxisNp");
        for (int32_t i = hits.m_size - 1; i >= 0; --i, ++pair) {
            hkpTypedBroadPhaseHandle* handle = static_cast<hkpTypedBroadPhaseHandle*>(pair->m_b);
            const hkpCollidable* collB = static_cast<const hkpCollidable*>(handle->getOwner());
            if (collA == collB) {
                continue;
            }
            if (!m_collisionFilter->isCollisionEnabled(*collA, *collB).m_bool) {
                continue;
            }
            const hkpShape* shapeB = collB->getShape();
            if (!shapeB) {
                continue;
            }
            input.m_dispatcher->getGetClosestPointsFunc(typeA, shapeB->m_type)(*collA, *collB, input, collector);
        }
        HK_TIMER_END_LIST();
    }
}

hkpConstraintInstance*
hkpWorld::createAndAddConstraintInstance(hkpRigidBody* bodyA, hkpRigidBody* bodyB, hkpConstraintData* constraintData) {
    hkpConstraintInstance*
        pConstraint = new hkpConstraintInstance(bodyA, bodyB, constraintData, hkpConstraintInstance::PRIORITY_PSI);
    pConstraint->m_userData = constraintData->getUserData();
    addConstraint(pConstraint);
    return pConstraint;
}

void hkpWorld::removePhysicsSystem(const hkpPhysicsSystem* sys) {
    for (int32_t i = 0; i < sys->m_constraints.m_size; ++i) {
        if (sys->m_constraints.m_data[i]) {
            removeConstraint(sys->m_constraints.m_data[i]);
        }
    }
    for (int32_t i = 0; i < sys->m_actions.m_size; ++i) {
        if (sys->m_actions.m_data[i]) {
            removeAction(sys->m_actions.m_data[i]);
        }
    }
    if (sys->m_rigidBodies.indexOf(nullptr) == -1) {
        // The rigid body array is handed to the entity batch call unchanged: an hkpRigidBody is an hkpEntity and both
        // arrays hold pointers, so the same address is passed. The untyped local is that hand-over.
        void* pBatch = sys->m_rigidBodies.m_data;
        removeEntityBatch(static_cast<hkpEntity* const*>(pBatch), sys->m_rigidBodies.m_size);
    }
    else {
        for (int32_t i = 0; i < sys->m_rigidBodies.m_size; ++i) {
            if (sys->m_rigidBodies.m_data[i]) {
                removeEntity(sys->m_rigidBodies.m_data[i]);
            }
        }
    }
    if (sys->m_phantoms.indexOf(nullptr) == -1) {
        removePhantomBatch(sys->m_phantoms.m_data, sys->m_phantoms.m_size);
    }
    else {
        for (int32_t i = 0; i < sys->m_phantoms.m_size; ++i) {
            if (sys->m_phantoms.m_data[i]) {
                removePhantom(sys->m_phantoms.m_data[i]);
            }
        }
    }
}

void hkpWorld::setGravity(const hkVector4& gravity) {
    m_gravity = gravity;
}

hkpWorld::MtAccessChecking hkpWorld::getMultithreadedAccessChecking() {
    return static_cast<hkpWorld::MtAccessChecking>(m_multiThreadCheck.m_markCount >> 15);
}

// KEEP (partcopy:): the job (0x40 bytes) has bytes 3 (hkJob's padding03), 8-15 (hkJob's alignment padding) and
// 0x3C-0x3F (the job's tail padding) unwritten; each sub-job is a rep movsd copy of it, and hkJobQueue::addJob
// copies m_size (0x40) bytes of it into the queue.
void hkpWorld::linearCastMt(
    hkpWorldLinearCastCommand* commandArray,
    int32_t numCommands,
    hkJobQueue* jobQueue,
    hkJobThreadPool* jobThreadPool,
    hkSemaphore* semaphore,
    int32_t numCommandsPerJob
) const {
    hkpCollisionQueryJobHeader* jobHeader = static_cast<hkpCollisionQueryJobHeader*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCollisionQueryJobHeader))
    );

    hkpWorldLinearCastJob job(m_collisionInput, jobHeader, commandArray, numCommands, m_broadPhase, semaphore);

    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        job.m_broadphase = nullptr;
        job.m_aabbTreeInt16 = m_kdTreeManager->m_inactiveTree;
        job.m_kdTree = m_kdTreeManager->m_kdTree;
    }
    // The job runs on the SPU.
    job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;

    while (job.m_numCommands > 0) {
        hkpWorldLinearCastJob subJob = job;
        subJob.m_numCommands = job.m_numCommands < numCommandsPerJob ? job.m_numCommands : numCommandsPerJob;
        job.m_commandArray += numCommandsPerJob;
        job.m_numCommands -= numCommandsPerJob;
        jobQueue->addJob(subJob, hkJobQueue::JOB_LOW_PRIORITY);
    }

    jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
    jobThreadPool->waitForCompletion();
    semaphore->acquire();
    hkMemoryRouter::getInstance().heap().blockFree(jobHeader, sizeof(hkpCollisionQueryJobHeader));
}

void hkpWorld::unlockForIslandSplit(hkpSimulationIsland* island) {
    if (m_modifyConstraintCriticalSection) {
        LeaveCriticalSection(&m_modifyConstraintCriticalSection->m_section);
    }
}

void hkpWorld::setMemoryWatchDog(hkWorldMemoryAvailableWatchDog* watchDog) {
    if (watchDog) {
        watchDog->addReference();
    }
    hkWorldMemoryAvailableWatchDog* m_pntr = m_memoryWatchDog.val();
    if (m_pntr) {
        m_pntr->removeReference();
    }
    m_memoryWatchDog.m_pntr = watchDog;
}

// KEEP (partcopy:): the job (an 80-byte local in the realigned frame) has bytes 3 (hkJob's padding03), 8-15 (hkJob's
// alignment padding) and 0x44-0x4F (the job's tail padding) unwritten; each sub-job is a whole copy of it, and
// hkJobQueue::addJob copies m_size (80) bytes of it into the queue.
void hkpWorld::castRayMt(
    hkpWorldRayCastCommand* commandArray,
    int32_t numCommands,
    hkJobQueue* jobQueue,
    hkJobThreadPool* jobThreadPool,
    hkSemaphore* semaphore,
    int32_t numCommandsPerJob
) {
    hkpCollisionQueryJobHeader* jobHeader = static_cast<hkpCollisionQueryJobHeader*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCollisionQueryJobHeader))
    );

    hkpWorldRayCastJob job(m_collisionInput, jobHeader, commandArray, numCommands, m_broadPhase, semaphore);

    if (m_kdTreeManager && m_kdTreeManager->isUpToDate()) {
        job.m_aabbTreeInt16 = m_kdTreeManager->m_inactiveTree;
        job.m_kdTree = m_kdTreeManager->m_kdTree;
        job.m_broadphase = nullptr;
    }

    while (job.m_numCommands > 0) {
        hkpWorldRayCastJob subJob = job;
        subJob.m_numCommands = job.m_numCommands < numCommandsPerJob ? job.m_numCommands : numCommandsPerJob;
        job.m_commandArray += numCommandsPerJob;
        job.m_numCommands -= numCommandsPerJob;
        jobQueue->addJob(subJob, hkJobQueue::JOB_LOW_PRIORITY);
    }

    jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
    jobThreadPool->waitForCompletion();
    semaphore->acquire();
    hkMemoryRouter::getInstance().heap().blockFree(jobHeader, sizeof(hkpCollisionQueryJobHeader));
}

void hkpWorld::addWorldPostCollideListener(hkpWorldPostCollideListener* worldListener) {
    m_worldPostCollideListeners.pushBack(worldListener);
}

void hkpWorld::addWorldPostIntegrateListener(hkpWorldPostIntegrateListener* worldListener) {
    m_worldPostIntegrateListeners.pushBack(worldListener);
}

void hkpWorld::addIslandPostCollideListener(hkpIslandPostCollideListener* islandListener) {
    m_islandPostCollideListeners.pushBack(islandListener);
}

void hkpWorld::addIslandPostIntegrateListener(hkpIslandPostIntegrateListener* islandListener) {
    m_islandPostIntegrateListeners.pushBack(islandListener);
}

void hkpWorld::addContactImpulseLimitBreachedListener(hkpContactImpulseLimitBreachedListener* listener) {
    m_contactImpulseLimitBreachedListeners.pushBack(listener);
}

// When the listener is not found, the entry at index -1 (the indexOf result) is cleared, deliberately.
void hkpWorld::removeWorldPostCollideListener(hkpWorldPostCollideListener* worldListener) {
    int32_t m_size = m_worldPostCollideListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    not_found:;
        m_worldPostCollideListeners.begin()[-1] = 0;
    }
    else {
        hkpWorldPostCollideListener** m_data = m_worldPostCollideListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto not_found;
            }
        }
        m_worldPostCollideListeners.m_data[v3] = 0;
    }
}

// When the listener is not found, the entry at index -1 (the indexOf result) is cleared, deliberately.
void hkpWorld::removeWorldPostIntegrateListener(hkpWorldPostIntegrateListener* worldListener) {
    int32_t m_size = m_worldPostIntegrateListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    not_found:;
        m_worldPostIntegrateListeners.begin()[-1] = 0;
    }
    else {
        hkpWorldPostIntegrateListener** m_data = m_worldPostIntegrateListeners.begin();
        while (*m_data != worldListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto not_found;
            }
        }
        m_worldPostIntegrateListeners.m_data[v3] = 0;
    }
}

// When the listener is not found, the entry at index -1 (the indexOf result) is cleared, deliberately.
void hkpWorld::removeIslandPostCollideListener(hkpIslandPostCollideListener* islandListener) {
    int32_t m_size = m_islandPostCollideListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    not_found:;
        m_islandPostCollideListeners.begin()[-1] = 0;
    }
    else {
        hkpIslandPostCollideListener** m_data = m_islandPostCollideListeners.begin();
        while (*m_data != islandListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto not_found;
            }
        }
        m_islandPostCollideListeners.m_data[v3] = 0;
    }
}

// When the listener is not found, the entry at index -1 (the indexOf result) is cleared, deliberately.
void hkpWorld::removeIslandPostIntegrateListener(hkpIslandPostIntegrateListener* islandListener) {
    int32_t m_size = m_islandPostIntegrateListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    not_found:;
        m_islandPostIntegrateListeners.begin()[-1] = 0;
    }
    else {
        hkpIslandPostIntegrateListener** m_data = m_islandPostIntegrateListeners.begin();
        while (*m_data != islandListener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto not_found;
            }
        }
        m_islandPostIntegrateListeners.m_data[v3] = 0;
    }
}

// When the listener is not found, the entry at index -1 (the indexOf result) is cleared, deliberately.
void hkpWorld::removeContactImpulseLimitBreachedListener(hkpContactImpulseLimitBreachedListener* listener) {
    int32_t m_size = m_contactImpulseLimitBreachedListeners.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
    not_found:;
        m_contactImpulseLimitBreachedListeners.begin()[-1] = 0;
    }
    else {
        hkpContactImpulseLimitBreachedListener** m_data = m_contactImpulseLimitBreachedListeners.begin();
        while (*m_data != listener) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                goto not_found;
            }
        }
        m_contactImpulseLimitBreachedListeners.m_data[v3] = 0;
    }
}

void hkpWorld::setMultithreadedAccessChecking(hkpWorld::MtAccessChecking accessCheckState) {
    hkMultiThreadCheck* p_m_multiThreadCheck = &m_multiThreadCheck;
    if (accessCheckState) {
        p_m_multiThreadCheck->disableChecks();
        m_broadPhase->m_multiThreadCheck.disableChecks();
    }
    else {
        p_m_multiThreadCheck->enableChecks();
        m_broadPhase->m_multiThreadCheck.enableChecks();
    }
}

void hkpWorld::registerWithJobQueue(hkJobQueue* jobQueue) {
    s_dynamicsProcessFuncs[0] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[1] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[2] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[3] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[4] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[5] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[6] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[7] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[8] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[9] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[10] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[11] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[12] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[13] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[14] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[15] = &hkpMultiThreadedSimulation::processNextJob;
    s_dynamicsProcessFuncs[16] = &hkpMultiThreadedSimulation::processNextJob;
    if (jobQueue->m_hwSetup.m_numCpuThreads < 2) {
        jobQueue->m_hwSetup.m_numCpuThreads = 2;
    }
    jobQueue->registerJobWithCpuThread(HK_JOB_TYPE_DYNAMICS, hkpDynamicsJob::DYNAMICS_JOB_BROADPHASE, 1);
    // Both handlers share the 17-entry process table; each hkJobHandlerFuncs is written whole (16 bytes).
    hkJobQueue::hkJobHandlerFuncs dynamicsFuncs;
    dynamicsFuncs.m_processJobFuncs = s_dynamicsProcessFuncs;
    dynamicsFuncs.m_numProcessJobFuncs = 17;
    dynamicsFuncs.m_popJobFunc = &hkpJobQueueUtils::popIntegrateJob;
    dynamicsFuncs.m_finishJobFunc = &hkpJobQueueUtils::finishIntegrateJob;
    jobQueue->registerJobHandler(HK_JOB_TYPE_DYNAMICS, dynamicsFuncs);
    hkJobQueue::hkJobHandlerFuncs collideFuncs;
    collideFuncs.m_processJobFuncs = s_dynamicsProcessFuncs;
    collideFuncs.m_numProcessJobFuncs = 17;
    collideFuncs.m_popJobFunc = &hkpJobQueueUtils::popCollideJob;
    collideFuncs.m_finishJobFunc = &hkpJobQueueUtils::finishCollideJob;
    jobQueue->registerJobHandler(HK_JOB_TYPE_COLLIDE, collideFuncs);
    hkpCollisionQueryJobQueueUtils::registerWithJobQueue(jobQueue);
    hkpRayCastQueryJobQueueUtils::registerWithJobQueue(jobQueue);
}

const hkClass& hkpWorld::staticClass() {
    return hkpWorldClass;
}

hkBool enumerateAllInactiveEntitiesInWorld(const hkpWorld* apWorld, hkpPhysicsSystem* apSystem) {
    const hkpWorld* v3 = apWorld;
    hkpSimulationIsland* m_fixedIsland = apWorld->m_fixedIsland;
    bool v10 = false;
    if (m_fixedIsland) {
        for (int32_t i = 0; i < m_fixedIsland->m_entities.m_size; ++i) {
            if (i || (*m_fixedIsland->m_entities.begin())->m_collidable.m_shape) {
                apSystem->addRigidBody(static_cast<hkpRigidBody*>(m_fixedIsland->m_entities.m_data[i]));
                v3 = apWorld;
            }
        }
        v10 = m_fixedIsland->m_entities.m_size > 0;
    }
    hkpSimulationIsland** m_data = v3->m_inactiveSimulationIslands.m_data;
    if (m_data != &m_data[v3->m_inactiveSimulationIslands.m_size]) {
        do {
            hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &(*m_data)->m_entities;
            int32_t v8 = 0;
            if ((*m_data)->m_entities.m_size > 0) {
                do {
                    apSystem->addRigidBody(static_cast<hkpRigidBody*>(p_m_entities->m_data[v8++]));
                }
                while (v8 < p_m_entities->m_size);
                v3 = apWorld;
            }
            ++m_data;
        }
        while (m_data != &v3->m_inactiveSimulationIslands.m_data[v3->m_inactiveSimulationIslands.m_size]);
    }
    if (v3->m_inactiveSimulationIslands.m_size > 0 || v10) {
        return 1;
    }

    return 0;
}

hkBool enumerateAllActiveEntitiesInWorld(const hkpWorld* apWorld, hkpPhysicsSystem* apSystem) {
    hkpSimulationIsland** m_data = apWorld->m_activeSimulationIslands.m_data;
    if (m_data != &m_data[apWorld->m_activeSimulationIslands.m_size]) {
        do {
            hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &(*m_data)->m_entities;
            int32_t v5 = 0;
            if ((*m_data)->m_entities.m_size > 0) {
                do {
                    apSystem->addRigidBody(static_cast<hkpRigidBody*>(p_m_entities->m_data[v5++]));
                }
                while (v5 < p_m_entities->m_size);
            }
            ++m_data;
        }
        while (m_data != &apWorld->m_activeSimulationIslands.m_data[apWorld->m_activeSimulationIslands.m_size]);
    }
    return apWorld->m_activeSimulationIslands.m_size > 0;
}

void enumerateAllPhantomsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys) {
    hkpBroadPhaseBorder* BroadPhaseBorder = world->getBroadPhaseBorder();
    if (BroadPhaseBorder) {
        for (hkpPhantom* phantom : world->m_phantoms) {
            if (phantom != BroadPhaseBorder->m_phantoms[0] &&
                phantom != BroadPhaseBorder->m_phantoms[1] &&
                phantom != BroadPhaseBorder->m_phantoms[2] &&
                phantom != BroadPhaseBorder->m_phantoms[3] &&
                phantom != BroadPhaseBorder->m_phantoms[4] &&
                phantom != BroadPhaseBorder->m_phantoms[5]) {
                sys->addPhantom(phantom);
            }
        }
    }
    else {
        for (hkpPhantom* phantom : world->m_phantoms) {
            sys->addPhantom(phantom);
        }
    }
}

// Each island pointer is compared with the array's end, so an end that wraps to the start skips the loop. A pointer
// compare lets clang test the size in place of the addresses, so the addresses are integers here, with the end reloaded
// each pass.
void enumerateAllConstraintsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys) {
    hkpSimulationIsland** pActive = world->m_activeSimulationIslands.m_data;
    while (reinterpret_cast<uintptr_t>(pActive) !=
           reinterpret_cast<uintptr_t>(world->m_activeSimulationIslands.m_data) +
               static_cast<uintptr_t>(world->m_activeSimulationIslands.m_size) * sizeof(hkpSimulationIsland*)) {
        enumerateAllConstraintsInIsland(*pActive++, sys);
    }
    hkpSimulationIsland** pInactive = world->m_inactiveSimulationIslands.m_data;
    while (reinterpret_cast<uintptr_t>(pInactive) !=
           reinterpret_cast<uintptr_t>(world->m_inactiveSimulationIslands.m_data) +
               static_cast<uintptr_t>(world->m_inactiveSimulationIslands.m_size) * sizeof(hkpSimulationIsland*)) {
        enumerateAllConstraintsInIsland(*pInactive++, sys);
    }
}

void enumerateAllActionsInWorld(const hkpWorld* world, hkpPhysicsSystem* sys) {
    hkpSimulationIsland** m_data = world->m_activeSimulationIslands.m_data;
    if (m_data != &m_data[world->m_activeSimulationIslands.m_size]) {
        do {
            hkArray<hkpAction*, hkContainerHeapAllocator>* p_m_actions = &(*m_data)->m_actions;
            int32_t v4 = 0;
            if ((*m_data)->m_actions.m_size > 0) {
                do {
                    sys->addAction(p_m_actions->m_data[v4++]);
                }
                while (v4 < p_m_actions->m_size);
            }
            ++m_data;
        }
        while (m_data != &world->m_activeSimulationIslands.m_data[world->m_activeSimulationIslands.m_size]);
    }
    hkpSimulationIsland** v5 = world->m_inactiveSimulationIslands.m_data;
    if (v5 != &v5[world->m_inactiveSimulationIslands.m_size]) {
        do {
            hkArray<hkpAction*, hkContainerHeapAllocator>* v6 = &(*v5)->m_actions;
            int32_t v7 = 0;
            if ((*v5)->m_actions.m_size > 0) {
                do {
                    sys->addAction(v6->m_data[v7++]);
                }
                while (v7 < v6->m_size);
            }
            ++v5;
        }
        while (v5 != &world->m_inactiveSimulationIslands.m_data[world->m_inactiveSimulationIslands.m_size]);
    }
}

// The SDK's file-local hkWorld_setupContactMgrFactories: registers the simple constraint, report and null contact
// manager factories with dis for every response pair, then drops this function's references.
void hkWorld_setupContactMgrFactories(hkpWorld* world, hkpCollisionDispatcher* dis) {
    hkpContactMgrFactory* simple = new hkpSimpleConstraintContactMgr::Factory(world);
    hkpContactMgrFactory* report = new hkpReportContactMgr::Factory(world);
    hkpNullContactMgrFactory* none = new hkpNullContactMgrFactory;
    dis->registerContactMgrFactoryWithAll(simple, hkpMaterial::RESPONSE_SIMPLE_CONTACT);
    dis->registerContactMgrFactoryWithAll(report, hkpMaterial::RESPONSE_REPORTING);
    dis->registerContactMgrFactoryWithAll(none, hkpMaterial::RESPONSE_NONE);
    simple->removeReference();
    report->removeReference();
    none->removeReference();
}

// The SDK's file-local helper: adds to sys every constraint the island's entities master, except those whose
// terminal atom (the one under all modifiers) is a contact atom.
void enumerateAllConstraintsInIsland(hkpSimulationIsland* island, hkpPhysicsSystem* sys) {
    for (int32_t e = 0; e < island->m_entities.m_size; ++e) {
        hkpEntity* entity = island->m_entities.m_data[e];
        const hkSmallArray<hkConstraintInternal>& constraintMasters = entity->m_constraintsMaster;
        for (int32_t c = 0; c < constraintMasters.m_size; ++c) {
            const hkConstraintInternal* ci = &constraintMasters.m_data[c];
            hkpConstraintAtom* atom = hkpWorldConstraintUtil::getTerminalAtom(ci);
            if (atom->m_type.m_storage != hkpConstraintAtom::TYPE_CONTACT) {
                sys->addConstraint(ci->m_constraint);
            }
        }
    }
}

// The SDK's file-local hkWorld_updateFilterOnSinglePhantom: adds or removes collidable from phantom's overlaps as
// filter now says, and, where collidable is itself a phantom's, phantom from that phantom's.
void hkWorld_updateFilterOnSinglePhantom(hkpCollisionFilter* filter, hkpPhantom* phantom, hkpCollidable* collidable) {
    hkBool oldOverlapping = phantom->isOverlappingCollidableAdded(collidable);
    hkBool otherOverlapping;
    if (filter->isCollisionEnabled(phantom->m_collidable, *collidable).m_bool) {
        if (!oldOverlapping.m_bool) {
            phantom->addOverlappingCollidable(collidable);
        }
        if (collidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
            otherOverlapping = static_cast<hkpPhantom*>(collidable->getOwner())
                                   ->isOverlappingCollidableAdded(&phantom->m_collidable);
            if (!otherOverlapping.m_bool) {
                static_cast<hkpPhantom*>(collidable->getOwner())->addOverlappingCollidable(&phantom->m_collidable);
            }
        }
    }
    else {
        if (oldOverlapping.m_bool) {
            phantom->removeOverlappingCollidable(collidable);
        }
        if (collidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
            otherOverlapping = static_cast<hkpPhantom*>(collidable->getOwner())
                                   ->isOverlappingCollidableAdded(&phantom->m_collidable);
            if (otherOverlapping.m_bool) {
                static_cast<hkpPhantom*>(collidable->getOwner())->removeOverlappingCollidable(&phantom->m_collidable);
            }
        }
    }
}

// Adds the inactive, then the active islands' entities to sys, and makes sys active when the world has active islands.
void enumerateAllEntitiesInWorld(const hkpWorld* world, hkpPhysicsSystem* sys) {
    enumerateAllInactiveEntitiesInWorld(world, sys);
    sys->m_active = enumerateAllActiveEntitiesInWorld(world, sys);
}

// Copies every member but m_spareAgentSector, which is left as it was.
hkpProcessCollisionInput& hkpProcessCollisionInput::operator=(const hkpProcessCollisionInput& other) {
    hkpCollisionInput::operator=(other);
    m_stepInfo = other.m_stepInfo;
    m_collisionQualityInfo = other.m_collisionQualityInfo;
    m_dynamicsInfo = other.m_dynamicsInfo;
    m_enableDeprecatedWelding = other.m_enableDeprecatedWelding;
    m_allowToSkipConfirmedCallbacks = other.m_allowToSkipConfirmedCallbacks;
    m_config = other.m_config;
    return *this;
}

// Both indices are read before the first store.
hkpAgentNnEntry* updateCollidableEntryPointers(hkpAgentNnEntry* entry) {
    uint16_t uiIndexA = entry->m_agentIndexOnCollidable[0];
    uint16_t uiIndexB = entry->m_agentIndexOnCollidable[1];
    entry->m_collidable[0]->m_collisionEntries.m_data[uiIndexA].m_agentEntry = entry;
    entry->m_collidable[1]->m_collisionEntries.m_data[uiIndexB].m_agentEntry = entry;
    return entry;
}

hkpNullContactMgr::~hkpNullContactMgr() {}

uint16_t hkpNullContactMgr::addContactPointImpl(
    const hkpCdBody& a,
    const hkpCdBody& b,
    const hkpProcessCollisionInput& env,
    hkpProcessCollisionOutput& output,
    const hkpGskCache* contactCache,
    hkContactPoint& cp
) {
    return 0;
}

hkResult hkpNullContactMgr::reserveContactPointsImpl(int32_t numPoints) {
    return HK_SUCCESS;
}

hkpSimpleWorldRayCaster::~hkpSimpleWorldRayCaster() {}

void hkpSimpleWorldRayCaster::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkpWorldLinearCaster::~hkpWorldLinearCaster() {}

void hkpWorldLinearCaster::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void hkpSolverInfo::setTauAndDamping(float tau, float damping) {
    m_tau = tau;
    float v3 = 0.5 * tau;
    m_frictionTau = v3;
    m_damping = damping;
    m_dampDivTau = damping / tau;
    m_dampDivFrictionTau = damping / v3;
    m_tauDivDamp = tau / damping;
    m_integrateVelocityFactor.m_quad = _mm_shuffle_ps(_mm_set_ss(m_tauDivDamp), _mm_set_ss(m_tauDivDamp), 0);
    m_invIntegrateVelocityFactor.m_quad = _mm_shuffle_ps(_mm_set_ss(m_dampDivTau), _mm_set_ss(m_dampDivTau), 0);
    m_frictionTauDivDamp = v3 / damping;
}

hkpMultithreadConfig::hkpMultithreadConfig() : m_maxNumConstraintsSolvedSingleThreaded(70) {}

// The member manager is destroyed by its own destructor.
hkpNullContactMgrFactory::~hkpNullContactMgrFactory() {}

// The shared manager, with no reference added.
hkpContactMgr*
hkpNullContactMgrFactory::createContactMgr(const hkpCollidable&, const hkpCollidable&, const hkpCollisionInput&) {
    return &m_contactMgr;
}

hkpPhantomBroadPhaseListener::~hkpPhantomBroadPhaseListener() {}

hkpBroadPhaseBorderListener::~hkpBroadPhaseBorderListener() {}

void hkpWorldGetClosestPointsJob::setRunsOnSpuOrPpu() {}

hkpConvexListFilter::~hkpConvexListFilter() {}
