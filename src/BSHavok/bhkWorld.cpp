#include "BSHavok/bhkWorld.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkAabbPhantom.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkAngularDashpotAction.h"
#include "BSHavok/bhkBallAndSocketConstraint.h"
#include "BSHavok/bhkBallSocketConstraintChain.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkBoxShape.h"
#include "BSHavok/bhkBreakableConstraint.h"
#include "BSHavok/bhkCachingShapePhantom.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkConvexListShape.h"
#include "BSHavok/bhkConvexSweepShape.h"
#include "BSHavok/bhkConvexTransformShape.h"
#include "BSHavok/bhkConvexVerticesShape.h"
#include "BSHavok/bhkCylinderShape.h"
#include "BSHavok/bhkDashpotAction.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkExtendedMeshShape.h"
#include "BSHavok/bhkExtraData.h"
#include "BSHavok/bhkFixedConstraint.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkHingeLimitsConstraint.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkLiquidAction.h"
#include "BSHavok/bhkListShape.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkMemoryAllocator.h"
#include "BSHavok/bhkMoppBvTreeShape.h"
#include "BSHavok/bhkMotorAction.h"
#include "BSHavok/bhkMouseSpringAction.h"
#include "BSHavok/bhkMultiSphereShape.h"
#include "BSHavok/bhkMultithreadedSystem.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkNiTriStripsShape.h"
#include "BSHavok/bhkOnStartupCallback.h"
#include "BSHavok/bhkOrientHingedBodyAction.h"
#include "BSHavok/bhkPCollisionObject.h"
#include "BSHavok/bhkPackedNiTriStripsShape.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkPhantomOverlapListener.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkPlaneShape.h"
#include "BSHavok/bhkPrismaticConstraint.h"
#include "BSHavok/bhkRagdollConstraint.h"
#include "BSHavok/bhkRagdollLimitsConstraint.h"
#include "BSHavok/bhkRagdollTemplate.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyT.h"
#include "BSHavok/bhkSPCollisionObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkSpringAction.h"
#include "BSHavok/bhkStiffSpringConstraint.h"
#include "BSHavok/bhkTaskletManagerCallback.h"
#include "BSHavok/bhkTransformShape.h"
#include "BSHavok/bhkTriangleShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWaterListener.h"
#include "BSHavok/bhkWheelConstraint.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSHavok/hkPackedNiTriStripsShape.h"
#include "BSMain/BSXFlags.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSAcousticSpaceListener.h"
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/TES.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/System/FreeList/hkFreeListMemorySystem.h"
#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/hkBaseSystem.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplay.h"
#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpAgentRegisterUtil.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCaster.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpMotorAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Dashpot/hkpDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Reorient/hkpReorientAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"
#include "Havok/Source/Physics/Utilities/Collide/Filter/ConstrainedSystem/hkpConstrainedSystemFilter.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Deprecated/H1Group/hkpGroupCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpPhysicsSystemWithContacts.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarker.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarkerList.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpBallGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpGravityGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpMountedBallGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpProjectileGun.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

#include <bit>
#include <xmmintrin.h>

#if FNVD_SYSTEM_ALLOC
#include "xfnv/debugheap/SystemAlloc.h"
#endif

namespace bhkConvert {} // namespace bhkConvert

// Set while Update runs.
static bool bIsUpdating;

// Set from the debugger to serve the world to the visual debugger at the next update.
static bool b_hack_dump_world;

// SetDeltaTime's function-local static: simulate one step every frame whatever the caller asks.
static bool balwaysSimulateEveryFrame;

// ScaleSolverInfo's function-local static: this frame's copy of fTauRatio, which is written and read back.
static float fratio;

// ourAllocatorS's guard (bit 0).
static uint32_t uiOurAllocatorSGuard;
bhkTaskletManagerCallback::bhkThreadState bhkTaskletManagerCallback::sbhkThreadState;
static bhkOnStartupCallback sOnStartup;
static bhkOnExitCallback sOnExit;
SettingT<INISettingCollection> bhkWorld::fTimePerSubStep("fTimePerSubStep:HAVOK", 0.008f);
SettingT<INISettingCollection> bhkWorld::iMinNumSubSteps("iMinNumSubSteps:HAVOK", 8);
SettingT<INISettingCollection> bhkWorld::bHavokDebug("bHavokDebug:HAVOK", false);
SettingT<INISettingCollection> bhkWorld::fMaxStepTime("fMaxTime:HAVOK", 0.016666668f);
SettingT<INISettingCollection> bhkWorld::bFindContactPointsOnAddS("bFindContactPointsOnAdd:HAVOK", true);

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> iHWThread1("iHWThread1:General", 4);
SettingT<INISettingCollection> iHWThread2("iHWThread2:General", 4);
SettingT<INISettingCollection> iHWThread3("iHWThread3:General", 4);
SettingT<INISettingCollection> iHWThread4("iHWThread4:General", 5);
SettingT<INISettingCollection> iHWThread5("iHWThread5:General", 5);
SettingT<INISettingCollection> iHWThread6("iHWThread6:General", 5);
NiRTTI bhkWorld::ms_RTTI("bhkWorld", &bhkRefObject::ms_RTTI);

// Runs once, while hkBaseSystemIsInitialized is clear, under the Havok memory context. The FrameInfo goes to mainInit
// with its stack size cleared.
void bhkWorld::InitSystem() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    // EXE-INLINED none: hkBaseSystem::isInitialized
    if (!hkBaseSystemIsInitialized.m_bool) {
        static bhkMemoryAllocator ourAllocatorS;
        hkMemorySystem::FrameInfo kFrameInfo;
        kFrameInfo.~FrameInfo();
        ::new (&kFrameInfo) hkMemorySystem::FrameInfo;
        kFrameInfo.m_stackSize = 0;
        hkFreeListMemorySystem* pSystem = ::new (&hkFreeListMemorySystem_Instance.m_system) hkFreeListMemorySystem;
        pSystem->setAllocator(&ourAllocatorS);
        hkFreeListMemorySystem::pSingleton = pSystem;
        hkMemorySystem::replaceInstance(pSystem);
        hkMemorySystem* pMemorySystem = hkFreeListMemorySystem::pSingleton;
        hkMemoryRouter* pRouter = pMemorySystem->mainInit(kFrameInfo, hkMemorySystem::FLAG_ROUTER);
        hkMemoryAllocator* pHeap = pRouter->m_heap;
        pRouter->m_temp = pHeap;
        pRouter->m_solver = pHeap;
        // The stack is the address of the static buffer pStack, moved up to the next 16-byte boundary with its size cut
        // to match. The address's low bits are tested as an integer.
        char* pStackStart = bhkWorld::pStack;
#if FNVD_SYSTEM_ALLOC
        // The stack is a pass-through allocation (never freed, as the static buffer is not), so that with guarded
        // allocation a guard page follows its end. Its blocks are bumped and popped inline in ~70 files, so they are
        // not tracked one by one.
        pStackStart = static_cast<char*>(fnvd::sysalloc::AlignedAllocate(0x60000, 16));
#endif
        int32_t iStackSize = 0x60000;
        uint32_t uiMisalign = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pStackStart)) & 0xF;
        if (uiMisalign) {
            pStackStart += 0x10 - uiMisalign;
            iStackSize -= static_cast<int32_t>(0x10 - uiMisalign);
        }
        pRouter->m_stack.m_start.m_storage = pStackStart;
        pRouter->m_stack.m_next.m_storage = pStackStart;
        pRouter->m_stack.m_end.m_storage = pStackStart + iStackSize;
        hkBaseSystem::init(pRouter, &BSprintf_ErrorReport, nullptr);
        bhkCollisionFilter::InitFlags();
        void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(bhkCollisionFilter));
        static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(bhkCollisionFilter);
        bhkCollisionFilter* pFilter = ::new (pMemory) bhkCollisionFilter;
        bhkWorld::pFilter = pFilter;
        NiStream::RegisterLoader("bhkBoxShape", &bhkBoxShape::CreateObject);
        NiStream::RegisterLoader("bhkConvexListShape", &bhkConvexListShape::CreateObject);
        NiStream::RegisterLoader("bhkConvexSweepShape", &bhkConvexSweepShape::CreateObject);
        NiStream::RegisterLoader("bhkConvexTransformShape", &bhkConvexTransformShape::CreateObject);
        NiStream::RegisterLoader("bhkConvexVerticesShape", &bhkConvexVerticesShape::CreateObject);
        NiStream::RegisterLoader("bhkCylinderShape", &bhkCylinderShape::CreateObject);
        NiStream::RegisterLoader("bhkExtendedMeshShapeData", &bhkExtendedMeshShapeData::CreateObject);
        NiStream::RegisterLoader("bhkExtendedMeshShape", &bhkExtendedMeshShape::CreateObject);
        NiStream::RegisterLoader("bhkMultiSphereShape", &bhkMultiSphereShape::CreateObject);
        NiStream::RegisterLoader("bhkpMeshShape", &bhkNiTriStripsShape::CreateObject);
        NiStream::RegisterLoader("bhkNiTriStripsShape", &bhkNiTriStripsShape::CreateObject);
        NiStream::RegisterLoader("bhkPackedNiTriStripsShape", &bhkPackedNiTriStripsShape::CreateObject);
        NiStream::RegisterLoader("hkPackedNiTriStripsData", &hkPackedNiTriStripsData::CreateObject);
        NiStream::RegisterLoader("bhkPlaneShape", &bhkPlaneShape::CreateObject);
        NiStream::RegisterLoader("bhkSphereShape", &bhkSphereShape::CreateObject);
        NiStream::RegisterLoader("bhkTriangleShape", &bhkTriangleShape::CreateObject);
        NiStream::RegisterLoader("bhkMoppBvTreeShape", &bhkMoppBvTreeShape::CreateObject);
        NiStream::RegisterLoader("bhkTransformShape", &bhkTransformShape::CreateObject);
        NiStream::RegisterLoader("bhkCapsuleShape", &bhkCapsuleShape::CreateObject);
        NiStream::RegisterLoader("bhkListShape", &bhkListShape::CreateObject);
        NiStream::RegisterLoader("bhkBallAndSocketConstraint", &bhkBallAndSocketConstraint::CreateObject);
        NiStream::RegisterLoader("bhkBallSocketConstraintChain", &bhkBallSocketConstraintChain::CreateObject);
        NiStream::RegisterLoader("bhkHingeConstraint", &bhkHingeConstraint::CreateObject);
        NiStream::RegisterLoader("bhkHingeLimitsConstraint", &bhkHingeLimitsConstraint::CreateObject);
        NiStream::RegisterLoader("bhkFixedConstraint", &bhkFixedConstraint::CreateObject);
        NiStream::RegisterLoader("bhkLimitedHingeConstraint", &bhkLimitedHingeConstraint::CreateObject);
        NiStream::RegisterLoader("bhkPrismaticConstraint", &bhkPrismaticConstraint::CreateObject);
        NiStream::RegisterLoader("bhkRagdollConstraint", &bhkRagdollConstraint::CreateObject);
        NiStream::RegisterLoader("bhkRagdollLimitsConstraint", &bhkRagdollLimitsConstraint::CreateObject);
        NiStream::RegisterLoader("bhkStiffSpringConstraint", &bhkStiffSpringConstraint::CreateObject);
        NiStream::RegisterLoader("bhkWheelConstraint", &bhkWheelConstraint::CreateObject);
        NiStream::RegisterLoader("bhkBreakableConstraint", &bhkBreakableConstraint::CreateObject);
        NiStream::RegisterLoader("bhkMalleableConstraint", &bhkMalleableConstraint::CreateObject);
        NiStream::RegisterLoader("bhkAngularDashpotAction", &bhkAngularDashpotAction::CreateObject);
        NiStream::RegisterLoader("bhkDashpotAction", &bhkDashpotAction::CreateObject);
        NiStream::RegisterLoader("bhkLiquidAction", &bhkLiquidAction::CreateObject);
        NiStream::RegisterLoader("bhkMouseSpringAction", &bhkMouseSpringAction::CreateObject);
        NiStream::RegisterLoader("bhkMotorAction", &bhkMotorAction::CreateObject);
        NiStream::RegisterLoader("bhkOrientHingedBodyAction", &bhkOrientHingedBodyAction::CreateObject);
        NiStream::RegisterLoader("bhkSpringAction", &bhkSpringAction::CreateObject);
        NiStream::RegisterLoader("bhkAabbPhantom", &bhkAabbPhantom::CreateObject);
        NiStream::RegisterLoader("bhkCollisionObject", &bhkCollisionObject::CreateObject);
        NiStream::RegisterLoader("bhkPCollisionObject", &bhkPCollisionObject::CreateObject);
        NiStream::RegisterLoader("bhkSPCollisionObject", &bhkSPCollisionObject::CreateObject);
        NiStream::RegisterLoader("bhkBlendCollisionObject", &bhkBlendCollisionObject::CreateObject);
        NiStream::RegisterLoader("bhkBlendController", &bhkBlendController::CreateObject);
        NiStream::RegisterLoader("bhkRigidBody", &bhkRigidBody::CreateObject);
        NiStream::RegisterLoader("bhkRigidBodyT", &bhkRigidBodyT::CreateObject);
        NiStream::RegisterLoader("bhkSimpleShapePhantom", &bhkSimpleShapePhantom::CreateObject);
        NiStream::RegisterLoader("bhkCachingShapePhantom", &bhkCachingShapePhantom::CreateObject);
        NiStream::RegisterLoader("bhkExtraData", &bhkExtraData::CreateObject);
        NiStream::RegisterLoader("bhkRagdollTemplate", &bhkRagdollTemplate::CreateObject);
        NiStream::RegisterLoader("bhkRagdollTemplateData", &bhkRagdollTemplateData::CreateObject);
        NiStream::RegisterLoader("bhkPoseArray", &bhkPoseArray::CreateObject);
        if (!bhkWorld::pMultithreadedSystem) {
            void* pSystemMemory = MemoryManager_Singleton.Allocate(sizeof(bhkMultithreadedSystem));
            bhkMultithreadedSystem* pMultithreaded =
                pSystemMemory ? ::new (pSystemMemory) bhkMultithreadedSystem : nullptr;
            bhkWorld::pMultithreadedSystem = pMultithreaded;
        }
    }
}

// Sets hkpWorldCinfo's defaults, with the game's own values.
bhkWorldCinfo::bhkWorldCinfo() {
    m_mtPostponeAndSortBroadPhaseBorderCallbacks.m_bool = 0;
    m_collisionTolerance = 0.1f;
    m_contactRestingVelocity = 3.40282e+38f;
    m_gravity.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(zero), _mm_set_ss(zero)),
        _mm_unpacklo_ps(_mm_set_ss(fDefaultGravityZ), _mm_set_ss(zero))
    );
    m_simulationType.m_storage = eDefaultSimType;
    m_shouldActivateOnRigidBodyTransformChange.m_bool = 0;
    m_broadPhaseBorderBehaviour.m_storage = 3;
    m_expectedMinPsiDeltaTime = bhkWorld::fMaxStepTime.uValue.f;
}

bhkWorldCinfo::~bhkWorldCinfo() {}

void bhkWorldCinfo::SetSimulationType(uint32_t aiType) {
    if (!aiType) {
        bhkWorldCinfo::eDefaultSimType = (int32_t)1;
    }
    else {
        if (((int32_t)aiType) == 4) {
            bhkWorldCinfo::eDefaultSimType = (int32_t)3;
        }
        else {
            bhkWorldCinfo::eDefaultSimType = (int32_t)2;
        }
    }
}

bhkWorld::bhkWorld(const bhkWorldCinfo& arInfo)
  : pDebugger(nullptr), bAdded(false), bEnabled(false), bDebugDisplay(false), iBatchAdd(0), iBatchRemove(0),
    pTrapListener(nullptr), pWaterListener(nullptr), pAcousticSpaceListener(nullptr), iEntityBatchCount(0),
    iPhantomBatchCount(0), iActionBatchCount(0), iConstraintInstanceBatchCount(0), iEntityRemoveBatchCount(0) {
    Origin.m_quad = hkVector4Zero.m_quad;
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    ++iCount;
    pEntityBatch = static_cast<hkpEntity**>(MemoryManager_Singleton.Allocate(3000 * sizeof(hkpEntity*)));
    std::memset(pEntityBatch, 0, 3000 * sizeof(hkpEntity*));
    pPhantomBatch = static_cast<hkpPhantom**>(MemoryManager_Singleton.Allocate(200 * sizeof(hkpPhantom*)));
    std::memset(pPhantomBatch, 0, 200 * sizeof(hkpPhantom*));
    pActionBatch = static_cast<hkpAction**>(MemoryManager_Singleton.Allocate(100 * sizeof(hkpAction*)));
    std::memset(pActionBatch, 0, 100 * sizeof(hkpAction*));
    pConstraintInstanceBatch =
        static_cast<hkpConstraintInstance**>(MemoryManager_Singleton.Allocate(200 * sizeof(hkpConstraintInstance*)));
    std::memset(pConstraintInstanceBatch, 0, 200 * sizeof(hkpConstraintInstance*));
    pEntityRemoveBatch = static_cast<hkpEntity**>(MemoryManager_Singleton.Allocate(3000 * sizeof(hkpEntity*)));
    std::memset(pEntityRemoveBatch, 0, 3000 * sizeof(hkpEntity*));
    double f = 1.0;
    if (fMaxStepTime.uValue.f < 1.0f) {
        f = fMaxStepTime.uValue.f;
    }
    fMaxStepTime.uValue.f = static_cast<float>(f);
    ActionList.RemoveAll();
    // The serializable tag base sits after the Havok cinfo (+0xF0).
    bhkWorld::Init(static_cast<const hkSerializableCinfo&>(arInfo));
    fOriginalTau = GethkWorld()->m_dynamicsStepInfo.m_solverInfo.m_tau;
    fOriginalDamping = GethkWorld()->m_dynamicsStepInfo.m_solverInfo.m_damping;
    pTLSData->eMemContext = eMemContext;
}

// Releases, under the thread's Havok memory context, the debugger, the Havok world and the five batch pointers, the
// entity listeners (each destroyed through its vtable) and the action list; the context is restored before the members'
// destructors run (the list, then the two hkArrays).
bhkWorld::~bhkWorld() {
    int32_t iOldMemContext = GetThreadMemContext();
    --iCount;
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    if (pDebugger) {
        GethkWorld();
        pDebugger->removeReference();
        pDebugger = nullptr;
    }
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    GethkWorld();
    bhkSerializable::Kill();
    MemoryManager_Singleton.Deallocate(pEntityBatch);
    MemoryManager_Singleton.Deallocate(pPhantomBatch);
    MemoryManager_Singleton.Deallocate(pActionBatch);
    MemoryManager_Singleton.Deallocate(pConstraintInstanceBatch);
    MemoryManager_Singleton.Deallocate(pEntityRemoveBatch);
    for (int32_t i = 0; i < pEntityListenerA.m_size; ++i) {
        hkpEntityListener* pListener = pEntityListenerA.m_data[i];
        if (pListener) {
            delete pListener;
        }
    }
    pEntityListenerA.m_size = 0;
    ActionList.RemoveAll();
    GetThreadMemContext() = iOldMemContext;
}

void bhkWorld::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkWorld::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v35 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v35);
    char* ViewerString_0 = NiGetViewerString("Enabled", bEnabled);
    uint32_t v9 = apStrings->m_usSize;
    v35 = ViewerString_0;
    if (v9 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v9 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v9, v35);
    char* v10 = NiGetViewerString("Vis Debug", pDebugger != nullptr);
    uint32_t v11 = apStrings->m_usSize;
    uint32_t v12 = apStrings->m_usMaxSize;
    v35 = v10;
    if (v11 >= v12) {
        apStrings->SetSize(v11 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v11, v35);
    char* ViewerString_6 = NiGetViewerString("Worlds", bhkWorld::iCount);
    uint32_t v14 = apStrings->m_usSize;
    v35 = ViewerString_6;
    if (v14 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v14 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v14, v35);
    char* v15 = NiGetViewerString("Actions", bhkAction::iCount);
    uint32_t v16 = apStrings->m_usSize;
    uint32_t v17 = apStrings->m_usMaxSize;
    v35 = v15;
    if (v16 >= v17) {
        apStrings->SetSize(v16 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v16, v35);
    char* v18 = NiGetViewerString("Contraints", bhkConstraint::iCount);
    uint32_t v19 = apStrings->m_usSize;
    uint32_t v20 = apStrings->m_usMaxSize;
    v35 = v18;
    if (v19 >= v20) {
        apStrings->SetSize(v19 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v19, v35);
    char* v21 = NiGetViewerString("Entities", bhkEntity::iCount);
    uint32_t v22 = apStrings->m_usSize;
    v35 = v21;
    if (v22 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v22 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v22, v35);
    char* v23 = NiGetViewerString("RigidBodies", bhkRigidBody::iCount);
    uint32_t v24 = apStrings->m_usSize;
    uint32_t v25 = apStrings->m_usMaxSize;
    v35 = v23;
    if (v24 >= v25) {
        apStrings->SetSize(v24 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v24, v35);
    char* v26 = NiGetViewerString("Shapes", bhkShape::iCount);
    uint32_t v27 = apStrings->m_usSize;
    uint32_t v28 = apStrings->m_usMaxSize;
    v35 = v26;
    if (v27 >= v28) {
        apStrings->SetSize(v27 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v27, v35);
    char* v29 = NiGetViewerString("Phantoms", bhkPhantom::iCount);
    uint32_t v30 = apStrings->m_usSize;
    v35 = v29;
    if (v30 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v30 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v30, v35);
    char* v31 = NiGetViewerString("MoppBVTreeShapes", bhkMoppBvTreeShape::iCount);
    uint32_t v32 = apStrings->m_usSize;
    uint32_t v33 = apStrings->m_usMaxSize;
    v35 = v31;
    if (v32 >= v33) {
        apStrings->SetSize(v32 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v32, v35);
    v3->eMemContext = eMemContext;
}

void bhkWorld::SethkObject(hkReferencedObject* aphkObject) {
    bhkRefObject::SethkObject(aphkObject);
}

// Points the Havok world back at this wrapper while linked, and clears it when unlinked.
void bhkWorld::Link(bool abAttach) {
    if (this) {
        ahkpWorld* pWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pWorld) {
            pWorld->setUserData(abAttach ? this : nullptr);
        }
    }
    bhkRefObject::Link(abAttach);
}

const hkpWorld* bhkWorld::GethkWorld() const {
    return static_cast<hkpWorld*>(phkObject);
}

// The created info is a bhkWorldCinfo, whose hkSerializableCinfo base pInfo points at sits 0xF0 bytes in.
void bhkWorld::KillCinfo(bool abCreated) {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        bhkWorldCinfo* pi = static_cast<bhkWorldCinfo*>(pInfo);
        if (pi) {
            delete pi;
        }
        pInfo = nullptr;
    }
}

// Builds the Havok world from the cinfo, gives it the world's collision filter, wraps it, finishes the initialisation
// and registers every collision agent.
void bhkWorld::Init(const hkSerializableCinfo& arInfo) {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    const hkSerializableCinfo* pInfo = &arInfo;
    const bhkWorldCinfo* pCinfo = pInfo ? static_cast<const bhkWorldCinfo*>(pInfo) : nullptr;
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    void* pMemory = pRouter->m_heap->blockAlloc(sizeof(ahkpWorld));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(ahkpWorld);
    ahkpWorld* pWorld = ::new (pMemory) ahkpWorld(*pCinfo, 70100);
    if (bhkWorld::pFilter) {
        pWorld->setCollisionFilter(
            bhkWorld::pFilter, hkBool(true), HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK,
            HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
        );
    }
    SethkObject(pWorld);
    pWorld->removeReference();
    PostInit(arInfo);
    const ahkpWorld* pCreated = static_cast<const ahkpWorld*>(GethkWorld());
    if (pCreated) {
        hkpAgentRegisterUtil::registerAllAgents(pCreated->m_collisionDispatcher);
    }
    pTLS->eMemContext = eMemContext;
}

hkSerializableCinfo& bhkWorld::CreateCinfo(bool& arCreated) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    bool bCreate = pInfo == nullptr;
    arCreated = bCreate;
    if (bCreate) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        bhkWorldCinfo* pNewInfo = static_cast<bhkWorldCinfo*>(Instance->m_heap->blockAlloc(sizeof(bhkWorldCinfo)));
        pNewInfo->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(bhkWorldCinfo));
        bhkWorldCinfo* pCinfo = ::new (pNewInfo) bhkWorldCinfo;
        // The serializable tag base sits after the Havok cinfo (+0xF0); a null cinfo stays null.
        hkSerializableCinfo* pSerializableInfo = static_cast<hkSerializableCinfo*>(pCinfo);
        bool bNoObject = phkObject == nullptr;
        pInfo = pSerializableInfo;
        if (!bNoObject) {
            bhkWorldCinfo* pWorldInfo = static_cast<bhkWorldCinfo*>(pSerializableInfo);
            const hkpWorld* pWorld = GethkWorld();
            if (pWorld) {
                pWorld->getCinfo(*pWorldInfo);
            }
        }
    }
    hkSerializableCinfo* result = pInfo;
    pTLSData->eMemContext = eMemContext;
    return *result;
}

// Pending adds get one frame's share of the entities (once per frame) and the constraints and actions. An enabled world
// with time to run (`0.0 < fBSDeltaTime`) then steps to the frame's marker in steps of fMaxStepTime scaled by the
// global time multiplier, across the job threads for a multithreaded world. The returned current and psi times are
// unused, as the game does. Pending entity removals follow either way.
bool bhkWorld::Update() {
    fCurrentPickTime = 0.0f;
    bIsUpdating = true;
    iCurrentPickCount = 0;
    bool bres = false;
    if (iEntityBatchCount) {
        if (!bFrameBatchProcessed) {
            if (iEntityBatchCount >= 3000) {
                iEntityBatchCount = 3000;
            }
            const uint32_t uiNum = iEntityBatchCount < iEntityBatchAddRate ? iEntityBatchCount : iEntityBatchAddRate;
            const uint32_t uiRest = iEntityBatchCount - uiNum;
            AddEntities(&pEntityBatch[uiRest], uiNum);
            iEntityBatchCount = uiRest;
            bFrameBatchProcessed = 1;
        }
        AddBatchedConstraints();
        AddBatchedActions();
    }
    if (bEnabled && 0.0 < fBSDeltaTime) {
        ahkpWorld* phkWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        const bool bMultithreaded = phkWorld->m_simulationType == hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED;
        if (bMultithreaded) {
            pMultithreadedSystem->AcquireMTResources(bHavokDebug.uValue.b);
        }
        fStepTime = fMaxStepTime.uValue.f * BSTimer::fGlobalTimeMultiplier;
        UpdateActions(fDeltaTime);
        ScaleSolverInfo();
        // The write mark, empty in the game's build, leaves only its world lookup.
        GethkWorld();
        phkWorld->attemptToExecutePendingOperations();
        GethkWorld();
        phkWorld->getCurrentTime();
        phkWorld->setFrameTimeMarker(fDeltaTime);
        phkWorld->advanceTime();
        while (!phkWorld->isSimulationAtMarker()) {
            phkWorld->getCurrentPsiTime();
            if (bMultithreaded) {
                phkWorld->stepMultithreaded(
                    pMultithreadedSystem->pJobQueue, pMultithreadedSystem->pJobThreadPool, fStepTime
                );
            }
            else {
                phkWorld->stepDeltaTime(fStepTime);
            }
        }
        UpdateListeners(fBSDeltaTime);
        UpdateDebugger();
        bres = true;
    }
    if (iEntityRemoveBatchCount) {
        RemoveBatchedEntitiesPartial();
    }
    if (b_hack_dump_world) {
        SetVisualDebugger(true);
        b_hack_dump_world = false;
    }
    bIsUpdating = false;
    return bres;
}

// Casts the pick's ray into the Havok world, into its all-hits collector, its closest-hit collector or its own output,
// through the broad phase cache when it has one. Out of pick time (no override, and the current pick time past the
// limit), the pick fails at once.
bool bhkWorld::PickObject(bhkPickData& arPickData) {
    if (!bhkWorld::bPickCountOverride) {
        // EXE-INLINED 0x9C8EF0
        float fMaxTime = bhkWorld::bVatsReplay ? bhkWorld::fMaxPickTimeVATS : bhkWorld::fMaxPickTime;
        if (fMaxTime < bhkWorld::fCurrentPickTime) {
            arPickData.bPickFailed = true;
            // EXE-INLINED 0x62A2B0
            arPickData.m_hitFraction = 1.0f;
            arPickData.m_extraInfo = -1;
            arPickData.m_shapeKeys[0] = -1;
            arPickData.m_rootCollidable = nullptr;
            arPickData.m_shapeKeyIndex = 0;
            return false;
        }
    }
    HK_TIMER_BEGIN("PickObject", nullptr);
    bool bres = false;
    const hkpWorld* pWorld = GethkWorld();
    if (pWorld) {
        arPickData.CalcTo();
        // EXE-INLINED 0x62A2B0
        arPickData.m_hitFraction = 1.0f;
        arPickData.m_rootCollidable = nullptr;
        arPickData.m_extraInfo = -1;
        arPickData.m_shapeKeyIndex = 0;
        arPickData.m_shapeKeys[0] = -1;
        // A reference conversion: the output base's address is taken with no null test.
        hkpWorldRayCastOutput& rOutput = arPickData;
        hkpAllRayHitCollector* pAllCollector = arPickData.pAllCollector;
        if (pAllCollector) {
            char* pCache = arPickData.pCache;
            if (pCache) {
                FNVD_INLINED_LOCAL(hkpWorldRayCaster, kCaster, hkpWorldRayCaster_vftable, std::nullopt);
                kCaster.m_shapeInput.m_filterInfo = 0;
                kCaster.m_shapeInput.m_rayShapeCollectionFilter = nullptr;
                kCaster.castRay(*pWorld->m_broadPhase, arPickData, pWorld->m_collisionFilter, pCache, *pAllCollector);
            }
            else {
                pWorld->castRay(arPickData, *pAllCollector);
            }
            if (pAllCollector->m_hits.m_size) {
                pAllCollector->sortHits();
                hkpWorldRayCastOutput* pHit = pAllCollector->m_hits.m_data;
                rOutput.hkpShapeRayCastOutput::operator=(*pHit);
                rOutput.m_rootCollidable = pHit->m_rootCollidable;
                bres = true;
            }
        }
        else {
            SpecificItemCollector* pClosestCollector = arPickData.pCloseCollector;
            if (pClosestCollector) {
                // EXE-INLINED 0x62A290
                // EXE-INLINED 0x62A2B0
                pClosestCollector->m_rayHit.m_hitFraction = 1.0f;
                pClosestCollector->m_rayHit.m_rootCollidable = nullptr;
                pClosestCollector->m_rayHit.m_extraInfo = -1;
                pClosestCollector->m_rayHit.m_shapeKeyIndex = 0;
                pClosestCollector->m_rayHit.m_shapeKeys[0] = -1;
                // EXE-INLINED 0x4A3B50
                pClosestCollector->m_earlyOutHitFraction = 1.0f;
                char* pCache = arPickData.pCache;
                if (pCache) {
                    // As above: the caster's constructor and destructor are inline.
                    FNVD_INLINED_LOCAL(hkpWorldRayCaster, kCaster, hkpWorldRayCaster_vftable, std::nullopt);
                    kCaster.m_shapeInput.m_filterInfo = 0;
                    kCaster.m_shapeInput.m_rayShapeCollectionFilter = nullptr;
                    kCaster.castRay(
                        *pWorld->m_broadPhase, arPickData, pWorld->m_collisionFilter, pCache, *pClosestCollector
                    );
                }
                else {
                    pWorld->castRay(arPickData, *pClosestCollector);
                }
                if (pClosestCollector->m_rayHit.m_rootCollidable) {
                    rOutput = pClosestCollector->m_rayHit;
                    bres = true;
                }
            }
            else {
                pWorld->castRay(arPickData, rOutput);
                bres = arPickData.m_rootCollidable != nullptr;
            }
        }
    }
    HK_TIMER_END();
    return bres;
}

bool bhkWorld::HasActiveItems() {
    ahkpWorld* v1 = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    return (v1 && (v1->m_activeSimulationIslands.m_size != 0));
}

// Walks the subtree with the add handler (uDataA: notify, group, count, reset). Without abForce only a model flagged
// HAVOK is added; with bNoFixedObjects its fixed bodies are made keyframed first. Notify needs a collision object on
// the root itself.
bool bhkWorld::AddObjects(NiAVObject* apObject, bool abRecurse, bool abNotify, uint32_t aiGroup, bool abForce) {
    if (!apObject) {
        return false;
    }
    if (!abForce) {
        BSXFlags* pFlags = BSXFlags::Find(apObject);
        if (!pFlags || !(pFlags->m_iValue & BSXFlags::HAVOK)) {
            return false;
        }
    }
    bAdded = true;
    ObjectRecData arData;
    arData.pWorld = this;
    arData.bRecurse = abRecurse;
    if (bNoFixedObjects) {
        arData.eAction = OA_FIXEDTOKEYFRAMED;
        if (pRecFunctionA[OA_FIXEDTOKEYFRAMED]) {
            DoObjectRec(apObject, arData, pRecFunctionA[OA_FIXEDTOKEYFRAMED]);
        }
    }
    bool bnotify = abNotify;
    if (!apObject->m_spCollisionObject.m_pObject) {
        bnotify = false;
    }
    arData.uDataA[0].i = bnotify;
    arData.eAction = OA_ADD;
    arData.uDataA[1].i = aiGroup;
    arData.uDataA[2].i = 0;
    arData.uDataA[3].i = 1;
    if (pRecFunctionA[OA_ADD]) {
        DoObjectRec(apObject, arData, pRecFunctionA[OA_ADD]);
    }
    return true;
}

// Moves apObject's collision object up to apRoot (a model flagged HAVOK) and adds the subtree. A non-keyframed body
// that is not a bhkRigidBodyT is first wrapped in one, its current transform kept as the local offset and the body put
// at the origin. The local offset is then carried through apObject's local transform: the offset position is rotated by
// the offset rotation, transformed by apObject's transform and rotated back, and the rotation is multiplied by the
// transform's. The quaternion rotations use hkVector4::setRotatedDir and setRotatedInverseDir (see hkQuaternion.h),
// which round lane by lane. Without a body to move, apObject's local transform is reset instead. True once a
// non-keyframed body was found.
bool bhkWorld::AddObjectsAndMoveController(NiAVObject* apObject, NiAVObject* apRoot) {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    bool bres = false;
    if (!apObject || !apRoot) {
        return bres;
    }
    BSXFlags* pFlags = BSXFlags::Find(apRoot);
    if (!pFlags || !(pFlags->m_iValue & BSXFlags::HAVOK)) {
        return bres;
    }
    uint32_t inotify = 0;
    NiCollisionObject* pCollision = apObject->m_spCollisionObject.m_pObject;
    bhkNiCollisionObject* pcol;
    if (pCollision && (pcol = pCollision->IsBhkNiCollisionObject()) != nullptr) {
        bhkWorldObject* pwo = pcol->spWorldObject;
        inotify = 1;
        if (pwo && !pcol->QKeyframe()) {
            bAdded = true;
            bres = true;
            if (apRoot != apObject) {
                bhkRigidBodyT* pbody = nullptr;
                bool bRigidBodyT;
                bRigidBodyT = NiObject::IsKindOf(&bhkRigidBodyT::ms_RTTI, pwo);
                if (bRigidBodyT) {
                    pbody = static_cast<bhkRigidBodyT*>(pwo);
                }
                else {
                    NiPointer<bhkRigidBody> spbhkBody(pwo->IsBhkRigidBody());
                    if (spbhkBody.m_pObject) {
                        pcol->SetWorldObject(nullptr);
                        void* pMemory = MemoryManager::AlignedAllocate16(sizeof(bhkRigidBodyT));
                        pbody = pMemory ? ::new (pMemory) bhkRigidBodyT(spbhkBody.m_pObject) : nullptr;
                        hkVector4 localPos;
                        spbhkBody.m_pObject->GetPosition(localPos);
                        hkQuaternion localRot;
                        spbhkBody.m_pObject->GetRotation(localRot);
                        pbody->kLocalPos = localPos;
                        pbody->kLocalRot = localRot;
                        pbody->SetPosition(hkVector4Zero);
                        pbody->SetRotation(hkQuaternionIdentity);
                        pcol->SetWorldObject(pbody);
                        spbhkBody = nullptr;
                    }
                }
                if (pbody) {
                    hkTransform objectTrans;
                    NI2HK(objectTrans, apObject->m_kLocal);
                    hkVector4 tmp;
                    tmp.setRotatedDir(pbody->kLocalRot, pbody->kLocalPos);
                    hkVector4 pos;
                    pos.setTransformedPos(objectTrans, tmp);
                    hkQuaternion rot;
                    rot.set(objectTrans.m_rotation);
                    hkQuaternion tmpRot;
                    tmpRot.setMul(pbody->kLocalRot, rot);
                    tmp.setRotatedInverseDir(pbody->kLocalRot, pos);
                    pbody->kLocalPos = tmp;
                    pbody->kLocalRot = tmpRot;
                    apRoot->SetCollisionObject(pcol);
                    apObject->SetCollisionObject(nullptr);
                    apObject->m_uFlags = static_cast<NiAVObject::NiFlags>(
                        (apObject->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE_RIGID) | NiAVObject::SELECTIVE_UPDATE |
                        NiAVObject::SELECTIVE_UPDATE_TRANSFORMS
                    );
                }
                else {
                    apRoot->SetCollisionObject(pcol);
                    apObject->SetCollisionObject(nullptr);
                    apObject->m_kLocal.m_Rotate = NiMatrix3_IDENTITY;
                    apObject->m_kLocal.m_Translate = NiPoint3_ZERO;
                    apObject->m_uFlags = static_cast<NiAVObject::NiFlags>(
                        (apObject->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE_RIGID) | NiAVObject::SELECTIVE_UPDATE |
                        NiAVObject::SELECTIVE_UPDATE_TRANSFORMS
                    );
                }
            }
        }
    }
    ObjectRecData recData;
    recData.pWorld = this;
    recData.bRecurse = true;
    if (bNoFixedObjects) {
        recData.eAction = OA_FIXEDTOKEYFRAMED;
        DoObjects(apRoot, recData);
    }
    recData.eAction = OA_ADD;
    recData.uDataA[0].i = inotify;
    recData.uDataA[1].i = 0;
    recData.uDataA[2].i = 0;
    recData.uDataA[3].i = 1;
    if (pRecFunctionA[OA_ADD]) {
        DoObjectRec(apRoot, recData, pRecFunctionA[OA_ADD]);
    }
    return bres;
}

void bhkWorld::SetDebugDisplay(NiAVObject* apRoot, bool abTF) {
    uint8_t v3;
    bhkWorld::ObjectRecData v4;
    if (((uint8_t)bDebugDisplay) != ((uint8_t)abTF)) {
        bDebugDisplay = (bool)((uint8_t)abTF);
    }
    if (apRoot) {
        v3 = (uint8_t)(!bDebugDisplay);
        v4.pWorld = nullptr;
        v4.bRecurse = true;
        v4.eAction = bhkWorld::OA_DEBUGDISPLAY;
        v4.uDataA[0].i = (int32_t)((uint8_t)(!v3));
        if (bhkWorld::pRecFunctionA[6]) {
            bhkWorld::DoObjectRec(apRoot, v4, bhkWorld::pRecFunctionA[6]);
        }
    }
}

int8_t bhkWorld::RemoveConstraint(hkpConstraintInstance* apConstraint) {
    // Calls the const getter and removes through its result.
    hkpWorld* v3 = const_cast<hkpWorld*>(GethkWorld());
    if (!v3) {
        return 0;
    }
    hkpConstraintInstance* v4 = apConstraint;
    if (!apConstraint) {
        return 0;
    }
    if (iBatchRemove) {
        if (iConstraintInstanceBatchCount >= 200u) {
            RemoveBatchedConstraints();
            RemoveBatchedActions();
            RemoveBatchedPhantoms();
            RemoveBatchedEntitiesPartial();
        }
        v4->addReference();
        pConstraintInstanceBatch[iConstraintInstanceBatchCount++] = v4;
        return 1;
    }
    else {
        hkBool bResult;
        v3->removeConstraint(&bResult, apConstraint);
        return 1;
    }
}

// Queues apEntity's removal while removals are batched (flushing the batches when the entity batch is full), else
// removes it from the Havok world at once. An entity whose world object is already removed or queued (flags 0x30)
// counts as removed.
bool bhkWorld::RemoveEntity(hkpEntity* apEntity) {
    bool bRemoved = false;
    const hkpWorld* pWorld = GethkWorld();
    if (!pWorld) {
        return bRemoved;
    }
    if (!apEntity) {
        return bRemoved;
    }
    bhkWorldObject* pWorldObject = apEntity->m_userData;
    if (!iBatchRemove) {
        if (!pWorldObject || (pWorldObject->iFlags & 0x30) != 0) {
            return true;
        }
        hkBool kResult;
        return const_cast<hkpWorld*>(pWorld)->removeEntity(&kResult, apEntity)->m_bool != 0;
    }
    if (!pWorldObject) {
        return true;
    }
    uint32_t iFlags = pWorldObject->iFlags;
    if ((iFlags & 0x30) != 0) {
        return true;
    }
    pWorldObject->iFlags = iFlags | 0x20;
    if (iEntityRemoveBatchCount >= 3000) {
        RemoveBatchedConstraints();
        RemoveBatchedActions();
        RemoveBatchedPhantoms();
        RemoveBatchedEntitiesPartial();
    }
    apEntity->addReference();
    pEntityRemoveBatch[iEntityRemoveBatchCount++] = apEntity;
    return true;
}

// Adds apAction to the Havok world: while additions are batched it queues the action (first flushing the batches when
// the action batch is full, the entity batch at most iEntityBatchAddRate entities once a frame), else adds it at once;
// then puts the bhkAction wrapping it (the action's user data) at the head of the game's action list.
void bhkWorld::Add(hkpAction* apAction) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if (this) {
        const hkpWorld* pWorld = this->GethkWorld();
        if (pWorld) {
            if (iBatchAdd) {
                if (iActionBatchCount >= 100) {
                    if (iEntityBatchCount && !bhkWorld::bFrameBatchProcessed) {
                        if (iEntityBatchCount >= 3000) {
                            iEntityBatchCount = 3000;
                        }
                        uint32_t uiCount = iEntityBatchCount;
                        uint32_t uiNumToAdd = bhkWorld::iEntityBatchAddRate;
                        if (uiCount < uiNumToAdd) {
                            uiNumToAdd = uiCount;
                        }
                        uint32_t uiLeft = uiCount - uiNumToAdd;
                        AddEntities(&pEntityBatch[uiLeft], uiNumToAdd);
                        iEntityBatchCount = uiLeft;
                        bhkWorld::bFrameBatchProcessed = 1;
                    }
                    AddBatchedPhantoms();
                    AddBatchedActions();
                    AddBatchedConstraints();
                }
                apAction->addReference();
                pActionBatch[iActionBatchCount] = apAction;
                ++iActionBatchCount;
            }
            else {
                const_cast<hkpWorld*>(pWorld)->addAction(apAction);
            }
            if (apAction) {
                // The Havok action's user data is an untyped hkUlong, which the game fills with the bhkAction wrapping
                // it (bhkAction::Link stores it), so it is read back as that pointer.
                bhkAction* pAction = reinterpret_cast<bhkAction*>(apAction->m_userData);
                if (pAction) {
                    NiPointer<bhkAction> spAction(pAction);
                    ActionList.AddHead(spAction);
                }
            }
        }
    }
    pTLSData->eMemContext = eMemContext;
}

// Takes apAction out of the game's action list and, while removals are batched, queues its removal from the Havok world
// (flushing the batches when the action batch is full), else removes it at once.
bool bhkWorld::RemoveAction(hkpAction* apAction) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    const hkpWorld* pWorld = this->GethkWorld();
    if (pWorld && apAction) {
        // The Havok action's user data is an untyped hkUlong, which the game fills with the bhkAction wrapping it
        // (bhkAction::Link stores it), so it is read back as that pointer.
        bhkAction* pAction = reinterpret_cast<bhkAction*>(apAction->m_userData);
        if (pAction) {
            NiPointer<bhkAction> spAction(pAction);
            ActionList.Remove(spAction);
        }
        if (iBatchRemove) {
            if (iActionBatchCount >= 100) {
                RemoveBatchedConstraints();
                RemoveBatchedActions();
                RemoveBatchedPhantoms();
                RemoveBatchedEntitiesPartial();
            }
            apAction->addReference();
            pActionBatch[iActionBatchCount] = apAction;
            ++iActionBatchCount;
            pTLSData->eMemContext = eMemContext;
            return true;
        }
        else {
            const_cast<hkpWorld*>(pWorld)->removeAction(apAction);
            pTLSData->eMemContext = eMemContext;
            return true;
        }
    }
    else {
        pTLSData->eMemContext = eMemContext;
        return false;
    }
}

// The substepped update (type 2) runs ceil(frame / fTimePerSubStep) substeps, at least iMinNumSubSteps, and scales tau
// and damping by how far the substeps fall short of the frame, keeping the unscaled values in fOriginalTau and
// fOriginalDamping. iMinNumSubSteps (an int setting) is deliberately read as a float, so its bits, not its value, are
// compared and divided by, as the game does. Any other update type takes 4 substeps per fMaxStepTime of step time (at
// least 2) and blends the default tau of 0.6 towards the time dilation by fTauRatio. The comparisons keep the game's
// NaN behaviour.
void bhkWorld::ScaleSolverInfo() {
    ahkpWorld* phkWorld = this ? static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld())) : nullptr;
    hkpSolverInfo& kSolverInfo = phkWorld->m_dynamicsStepInfo.m_solverInfo;
    if (iUpdateType == 2) {
        float numSubsteps =
            static_cast<float>(std::ceil(static_cast<double>((fBSDeltaTime / fTimePerSubStep.uValue.f))));
        float timePerSubstep = fTimePerSubStep.uValue.f;
        if (iMinNumSubSteps.uValue.f > numSubsteps) {
            numSubsteps = iMinNumSubSteps.uValue.f;
            timePerSubstep = fBSDeltaTime / numSubsteps;
        }
        fOriginalTau = kSolverInfo.m_tau;
        const float scaledTau = fOriginalTau + (fBSDeltaTime / (timePerSubstep * numSubsteps) - 1.0);
        fOriginalDamping = kSolverInfo.m_damping;
        const float scaledDamping = (fBSDeltaTime / (timePerSubstep * numSubsteps) - 1.0) + fOriginalDamping;
        kSolverInfo.m_tau = scaledTau;
        kSolverInfo.m_damping = scaledDamping;
        const int32_t iNumSteps = hkMath::hkFloatToInt(numSubsteps);
        kSolverInfo.m_numSteps = iNumSteps;
        kSolverInfo.m_invNumSteps = 1.0 / iNumSteps;
    }
    else {
        const float timeDilation = fStepTime / fMaxStepTime.uValue.f;
        float fNumSteps = 4.0 * timeDilation;
        if (fNumSteps < 2.0) {
            fNumSteps = 2.0f;
        }
        const int32_t iNumSteps = static_cast<int32_t>(fNumSteps);
        kSolverInfo.m_numSteps = iNumSteps;
        kSolverInfo.m_invNumSteps = 1.0 / iNumSteps;
        fratio = fTauRatio;
        const float fTau = timeDilation * fratio * 0.6000000238418579 + (1.0 - fratio) * 0.6000000238418579;
        kSolverInfo.m_tau = fTau;
        kSolverInfo.m_dampDivTau = kSolverInfo.m_damping / fTau;
        kSolverInfo.m_tauDivDamp = fTau / kSolverInfo.m_damping;
    }
}

// Each listener is a bhkEntityListener; none is ticked by a zero delta (a NaN delta still ticks them).
void bhkWorld::UpdateListeners(float afDelta) {
    if (!phkObject) {
        return;
    }
    for (int32_t i = 0; i < pEntityListenerA.m_size; ++i) {
        bhkEntityListener* pListener = static_cast<bhkEntityListener*>(pEntityListenerA.m_data[i]);
        if (pListener && afDelta != 0.0) {
            pListener->Update(afDelta);
        }
    }
}

// Each action in the world is ticked; one that left its Havok world is taken off the list, held by a smart pointer for
// the removal. Removing the head node moves the next node's item into it, so the head is visited again.
void bhkWorld::UpdateActions(float afDelta) {
    BSSimpleList<NiPointer<bhkAction>>* pList = &ActionList;
    BSSimpleList<NiPointer<bhkAction>>* pNode = pList;
    do {
        if (!pNode->m_pkNext && !pNode->m_item) {
            break;
        }
        bhkAction* pAction = pNode->m_item.m_pObject;
        BSSimpleList<NiPointer<bhkAction>>* pNext = pNode->m_pkNext;
        if (pAction) {
            if (pAction->GethkWorld()) {
                pAction->Update(afDelta);
            }
            else {
                if (pNode == pList) {
                    pNext = pNode;
                }
                pList->Remove(NiPointer<bhkAction>(pAction));
            }
        }
        pNode = pNext;
    }
    while (pNode);
}

// The visual debugger serves a frame and the timers of this thread and the job threads are cleared; the queued debug
// colors follow.
void bhkWorld::UpdateDebugger() {
    if (pDebugger) {
        pDebugger->step(0.0f);
        hkMonitorStream::getInstance().reset();
        hkJobThreadPool* pThreadPool = pMultithreadedSystem->pJobThreadPool;
        if (pThreadPool) {
            pThreadPool->clearTimerData();
        }
    }
    UpdateDebugColor();
}

// The queued colors go to the debug display last first, while the visual debugger runs; the queue is emptied either
// way.
void bhkWorld::UpdateDebugColor() {
    if (pDebugger) {
        for (int32_t i = DebugColorA.m_size; i != 0;) {
            --i;
            DebugColor& rColor = DebugColorA.m_data[i];
            // Havok's geometry id is the collidable's address.
            hkDebugDisplay::s_instance->setGeometryColor(
                static_cast<int32_t>(rColor.uiColor), reinterpret_cast<hkUlong>(rColor.pCollidable), 0
            );
        }
    }
    DebugColorA.m_size = 0;
}

// Each entity the world let go is marked no longer being removed (bhkWorldObject flag 0x20), and the batch drops
// its reference.
void bhkWorld::RemoveBatchedEntities() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iEntityRemoveBatchCount) {
        if (iEntityRemoveBatchCount >= 3000) {
            iEntityRemoveBatchCount = 3000;
        }
        ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pw) {
            pw->removeEntityBatch(pEntityRemoveBatch, iEntityRemoveBatchCount);
            for (uint32_t i = 0; i < iEntityRemoveBatchCount; ++i) {
                hkpCollidable* pc = &pEntityRemoveBatch[i]->m_collidable;
                if (pc->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
                    hkpWorldObject* pOwner =
                        reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(pc) + pc->m_ownerOffset);
                    if (pOwner) {
                        bhkWorldObject* pwo = pOwner->m_userData;
                        if (pwo) {
                            pwo->iFlags &= ~0x20;
                        }
                    }
                }
                pEntityRemoveBatch[i]->removeReference();
            }
            std::memset(pEntityRemoveBatch, 0, sizeof(hkpEntity*) * iEntityRemoveBatchCount);
            iEntityRemoveBatchCount = 0;
        }
    }
}

// Each phantom with a wrapper is marked no longer being removed (bhkPhantom flag 0x20); the batch drops every
// reference.
void bhkWorld::RemoveBatchedPhantoms() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iPhantomBatchCount) {
        if (static_cast<uint32_t>(iPhantomBatchCount) >= 200) {
            iPhantomBatchCount = 200;
        }
        ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pw) {
            pw->removePhantomBatch(pPhantomBatch, iPhantomBatchCount);
            for (uint32_t i = 0; i < static_cast<uint32_t>(iPhantomBatchCount); ++i) {
                hkpPhantom* pPhantom = pPhantomBatch[i];
                if (pPhantom) {
                    bhkPhantom* pphantom = static_cast<bhkPhantom*>(pPhantom->m_userData);
                    if (pphantom) {
                        pphantom->ucFlags &= ~0x20;
                    }
                }
                pPhantomBatch[i]->removeReference();
            }
            std::memset(pPhantomBatch, 0, sizeof(hkpPhantom*) * iPhantomBatchCount);
            iPhantomBatchCount = 0;
        }
    }
}

// Only an action still in a world is removed from it; the batch drops every reference it holds.
void bhkWorld::RemoveBatchedActions() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iActionBatchCount) {
        if (iActionBatchCount >= 100) {
            iActionBatchCount = 100;
        }
        ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pw) {
            for (uint32_t i = 0; i < iActionBatchCount; ++i) {
                if (pActionBatch[i] && pActionBatch[i]->m_world) {
                    pw->removeAction(pActionBatch[i]);
                }
                if (pActionBatch[i]) {
                    pActionBatch[i]->removeReference();
                }
            }
            std::memset(pActionBatch, 0, sizeof(hkpAction*) * iActionBatchCount);
            iActionBatchCount = 0;
        }
    }
}

// The batch counts are unsigned and the batch is cleared with memset.
void bhkWorld::RemoveBatchedConstraints() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (!iConstraintInstanceBatchCount) {
        return;
    }
    if (static_cast<uint32_t>(iConstraintInstanceBatchCount) >= 200) {
        iConstraintInstanceBatchCount = 200;
    }
    bhkSerializable* pSerializable = this;
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(pSerializable->GethkWorld()));
    if (!pw) {
        return;
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(iConstraintInstanceBatchCount); ++i) {
        hkpConstraintInstance* pConstraint = pConstraintInstanceBatch[i];
        if (pConstraint && pConstraint->m_owner) {
            hkBool op;
            pw->removeConstraint(&op, pConstraint);
        }
        if (pConstraintInstanceBatch[i]) {
            pConstraintInstanceBatch[i]->removeReference();
        }
    }
    std::memset(pConstraintInstanceBatch, 0, sizeof(hkpConstraintInstance*) * iConstraintInstanceBatchCount);
    iConstraintInstanceBatchCount = 0;
}

// Removes one frame's share of the queued entity removals: iEntityBatchRemoveRate entities off the end of the batch
// (the whole batch when it is smaller), after clamping the count to 3000. Each removed entity whose broad phase type is
// an entity clears flag 0x20 of the game object it belongs to, drops its reference and empties its slot; the count
// keeps the part not yet removed.
void bhkWorld::RemoveBatchedEntitiesPartial() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (!iEntityRemoveBatchCount) {
        return;
    }
    if (iEntityRemoveBatchCount >= 3000) {
        iEntityRemoveBatchCount = 3000;
    }
    const uint32_t uiCount = iEntityRemoveBatchCount;
    uint32_t uiNum = static_cast<uint32_t>(iEntityBatchRemoveRate);
    if (uiCount < uiNum) {
        uiNum = uiCount;
    }
    const uint32_t uiRest = uiCount - uiNum;
    // GethkWorld is called virtually through the base, so the call goes through bhkSerializable.
    bhkSerializable* pSerializable = this;
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(pSerializable->GethkWorld()));
    if (!pw) {
        return;
    }
    pw->removeEntityBatch(&pEntityRemoveBatch[uiRest], static_cast<int32_t>(uiNum));
    for (uint32_t i = uiRest; i < iEntityRemoveBatchCount; ++i) {
        hkpCollidable* pc = &pEntityRemoveBatch[i]->m_collidable;
        hkpWorldObject* pOwner;
        if (pc->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY &&
            (pOwner = reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(pc) + pc->m_ownerOffset)) !=
                nullptr &&
            pOwner->m_userData != nullptr) {
            // Clears the bit with a 32-bit `and`, which rewrites all four bytes.
            uint32_t& riFlags = pOwner->m_userData->iFlags;
            riFlags &= ~0x20u;
        }
        pEntityRemoveBatch[i]->removeReference();
        pEntityRemoveBatch[i] = nullptr;
    }
    iEntityRemoveBatchCount = uiRest;
}

bool bhkWorld::QDebugDisplay() {
    return bDebugDisplay;
}

void bhkWorld::SetEnabled(bool abTF) {
    bEnabled = abTF;
}

// Appends the pair inline (hkArray::pushBack): the router is fetched before the capacity check.
void bhkWorld::SetDebugObjectColor(const bhkWorldObject* apObject, uint32_t aiColor) {
    const hkpCollidable* pc = nullptr;
    if (apObject->phkObject) {
        pc = &static_cast<hkpWorldObject*>(apObject->phkObject)->m_collidable;
    }
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    if (DebugColorA.m_size == (DebugColorA.m_capacityAndFlags & hkArrayBase<DebugColor>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(
            kRouter.heap(), reinterpret_cast<hkArrayBase<void>*>(&DebugColorA), sizeof(DebugColor)
        );
    }
    DebugColor* ent = &DebugColorA.m_data[DebugColorA.m_size];
    if (ent) {
        ent->pCollidable = const_cast<hkpCollidable*>(pc);
        ent->uiColor = aiColor;
    }
    ++DebugColorA.m_size;
}

// Turning it on serves a visual debugger with one physics context watching this world on port 25001; turning it off
// drops the debugger. The context and the debugger are allocated the way Havok's operator new does (heap blockAlloc,
// then the size stored in m_memSizeAndFlags), and the contexts array is pushed and freed inline.
void bhkWorld::SetVisualDebugger(bool abTF) {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (!abTF) {
        if (pDebugger) {
            GethkWorld();
            pDebugger->removeReference();
            pDebugger = nullptr;
        }
        return;
    }
    if (pDebugger) {
        return;
    }
    FNVD_INLINED_LOCAL(hkArray<hkProcessContext*>, contexts, std::nullopt, std::nullopt);
    contexts.m_data = nullptr;
    contexts.m_size = 0;
    contexts.m_capacityAndFlags = hkArrayBase<hkProcessContext*>::DONT_DEALLOCATE_FLAG;
    hkpPhysicsContext::registerAllPhysicsProcesses();
    void* pContextMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpPhysicsContext));
    static_cast<hkReferencedObject*>(pContextMemory)->m_memSizeAndFlags = sizeof(hkpPhysicsContext);
    hkpPhysicsContext* pContext = ::new (pContextMemory) hkpPhysicsContext;
    pContext->addWorld(const_cast<hkpWorld*>(GethkWorld()));
    hkProcessContext* pProcessContext = pContext;
    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
    if (contexts.m_size == (contexts.m_capacityAndFlags & hkArrayBase<hkProcessContext*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(kHeap, reinterpret_cast<hkArrayBase<void>*>(&contexts), sizeof(hkProcessContext*));
    }
    contexts.m_data[contexts.m_size] = pProcessContext;
    ++contexts.m_size;
    void* pDebuggerMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkVisualDebugger));
    static_cast<hkReferencedObject*>(pDebuggerMemory)->m_memSizeAndFlags = sizeof(hkVisualDebugger);
    hkVisualDebugger* pVisualDebugger = ::new (pDebuggerMemory) hkVisualDebugger(&contexts, nullptr);
    pDebugger = pVisualDebugger;
    pVisualDebugger->serve(25001);
    hkMemoryAllocator& kFreeHeap = hkMemoryRouter::getInstance().heap();
    contexts.m_size = 0;
    if (!(contexts.m_capacityAndFlags & hkArrayBase<hkProcessContext*>::DONT_DEALLOCATE_FLAG)) {
        kFreeHeap.bufFree(
            contexts.m_data,
            (contexts.m_capacityAndFlags & hkArrayBase<hkProcessContext*>::CAPACITY_MASK) * sizeof(hkProcessContext*)
        );
    }
}

// Writes the world to the file: an hkOstream over the file name, then the snapshot writer, which gets the world (called
// through the vtable), the stream's writer, a false binary flag, no target layout and no contact points. The stream is
// destroyed when it goes out of scope, after the result is read.
bool bhkWorld::SaveSnapshot(const char* apFileName) {
    hkOstream kStream(apFileName);
    hkBool bBinary = {false};
    bhkSerializable* pSerializable = this;
    hkBool bResult;
    hkpHavokSnapshot::save(&bResult, pSerializable->GethkWorld(), kStream.m_writer, bBinary, nullptr, false);
    return bResult.m_bool != 0;
}

void bhkWorld::StartBatchingAdds() {
    ++iBatchAdd;
}

void bhkWorld::StartBatchingRemovals() {
    ++iBatchRemove;
}

// The last level closes the batch: a partial flush adds one frame's share of the entities (once per frame), a full
// one all of them; phantoms, actions and constraints follow. With batching already closed, a full stop still
// flushes leftover entities.
void bhkWorld::StopBatchingAdds(bool abAllowPartial) {
    if (iBatchAdd) {
        if (--iBatchAdd) {
            return;
        }
        if (abAllowPartial) {
            if (iEntityBatchCount && !bFrameBatchProcessed) {
                if (iEntityBatchCount >= 3000) {
                    iEntityBatchCount = 3000;
                }
                const uint32_t uiNum =
                    iEntityBatchCount < iEntityBatchAddRate ? iEntityBatchCount : iEntityBatchAddRate;
                const uint32_t uiRest = iEntityBatchCount - uiNum;
                AddEntities(&pEntityBatch[uiRest], uiNum);
                iEntityBatchCount = uiRest;
                bFrameBatchProcessed = 1;
            }
        }
        else {
            AddBatchedEntities();
        }
        AddBatchedPhantoms();
        AddBatchedActions();
        AddBatchedConstraints();
    }
    else if (iEntityBatchCount && !abAllowPartial) {
        AddBatchedEntities();
    }
}

// Closing the outermost batch flushes constraints, actions and phantoms before the entities.
void bhkWorld::StopBatchingRemovals(bool abAllowPartial) {
    if (iBatchRemove) {
        if (--iBatchRemove) {
            return;
        }
        RemoveBatchedConstraints();
        RemoveBatchedActions();
        RemoveBatchedPhantoms();
        if (abAllowPartial) {
            RemoveBatchedEntitiesPartial();
        }
        else {
            RemoveBatchedEntities();
        }
    }
    else if (iEntityRemoveBatchCount && !abAllowPartial) {
        RemoveBatchedEntities();
    }
}

bool bhkWorld::IsBatchingAdds() {
    return iBatchAdd != 0;
}

// The entities go in asleep. Each one the world took is marked no longer pending (bhkWorldObject flag 0x10); one
// without a wrapper is taken back out. The first entity already in a stored island has its island split, after it
// collides at once when bFindContactPointsOnAdd is set. A body moving faster than 0.001 on any of x, y, z, linear or
// angular, is woken. The velocities are read straight from the Havok body's motion.
void bhkWorld::AddEntities(hkpEntity** apEntityA, uint32_t auiNumEntities) {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (!pw) {
        return;
    }
    pw->addEntityBatch(apEntityA, auiNumEntities, HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE);
    bool bSplit = false;
    for (uint32_t i = 0; i < auiNumEntities; ++i) {
        hkpEntity* pEntity = apEntityA[i];
        hkpCollidable* pc = &pEntity->m_collidable;
        bhkWorldObject* pwo = nullptr;
        hkpWorldObject* pOwner;
        if (pc->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY &&
            (pOwner = reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(pc) + pc->m_ownerOffset)) !=
                nullptr &&
            (pwo = pOwner->m_userData) != nullptr) {
            pwo->iFlags &= ~0x10;
        }
        else {
            hkBool bRemoved;
            pw->removeEntity(&bRemoved, apEntityA[i]);
        }
        if (!bSplit) {
            hkpSimulationIsland* pIsland = apEntityA[i]->m_simulationIsland;
            if (pIsland && pIsland->m_storageIndex != 0xFFFF) {
                if (bFindContactPointsOnAddS.uValue.b) {
                    pw->findInitialContactPoints(pIsland->m_entities.m_data, pIsland->m_entities.m_size);
                }
                hkpWorldOperationUtil::splitSimulationIsland(pw, pIsland);
                bSplit = true;
            }
        }
        const hkVector4& kLinear = (pwo && pwo->phkObject)
            ? static_cast<hkpRigidBody*>(pwo->phkObject)->m_motion.m_linearVelocity
            : hkVector4Zero;
        bool bAtRest = false;
        if (__builtin_fabsf(kLinear.m_quad[0] - hkVector4Zero.m_quad[0]) <= 0.001f &&
            __builtin_fabsf(kLinear.m_quad[1] - hkVector4Zero.m_quad[1]) <= 0.001f &&
            __builtin_fabsf(kLinear.m_quad[2] - hkVector4Zero.m_quad[2]) <= 0.001f) {
            const hkVector4& kAngular = (pwo && pwo->phkObject)
                ? static_cast<hkpRigidBody*>(pwo->phkObject)->m_motion.m_angularVelocity
                : hkVector4Zero;
            bAtRest = __builtin_fabsf(kAngular.m_quad[0] - hkVector4Zero.m_quad[0]) <= 0.001f &&
                __builtin_fabsf(kAngular.m_quad[1] - hkVector4Zero.m_quad[1]) <= 0.001f &&
                __builtin_fabsf(kAngular.m_quad[2] - hkVector4Zero.m_quad[2]) <= 0.001f;
        }
        if (!bAtRest && pwo && pwo->phkObject) {
            static_cast<hkpEntity*>(pwo->phkObject)->activate();
        }
        apEntityA[i]->removeReference();
    }
    std::memset(apEntityA, 0, sizeof(hkpEntity*) * auiNumEntities);
}

// At most 3000 entities go in at once.
void bhkWorld::AddBatchedEntities() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iEntityBatchCount) {
        if (iEntityBatchCount >= 3000) {
            iEntityBatchCount = 3000;
        }
        AddEntities(pEntityBatch, iEntityBatchCount);
        iEntityBatchCount = 0;
    }
}

// Each phantom the world took is marked no longer pending (bhkPhantom flag 0x10); one without a wrapper is taken
// back out. The phantom's collision object then updates its world data, and the batch drops its reference.
void bhkWorld::AddBatchedPhantoms() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iPhantomBatchCount) {
        if (static_cast<uint32_t>(iPhantomBatchCount) >= 200) {
            iPhantomBatchCount = 200;
        }
        ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pw) {
            pw->addPhantomBatch(pPhantomBatch, iPhantomBatchCount);
            for (uint32_t i = 0; i < static_cast<uint32_t>(iPhantomBatchCount); ++i) {
                hkpPhantom* pPhantom = pPhantomBatch[i];
                bhkPhantom* pphantom;
                if (pPhantom && (pphantom = static_cast<bhkPhantom*>(pPhantom->m_userData)) != nullptr) {
                    pphantom->ucFlags &= ~0x10;
                }
                else {
                    pw->removePhantom(pPhantom);
                }
                hkpPropertyValue kProperty;
                bhkNiCollisionObject* pcol = reinterpret_cast<bhkNiCollisionObject*>(static_cast<uintptr_t>(
                    bhkWorldObject::GetProperty(
                        &kProperty, pPhantomBatch[i], bhkPropertyKey(bhkNiCollisionObject::ms_RTTI)
                    )
                        ->m_data
                ));
                if (pcol) {
                    NiUpdateData UpdateData;
                    UpdateData.fTime = 0.0f;
                    UpdateData.bUpdateControllers = false;
                    UpdateData.bParallelUpdate = false;
                    UpdateData.bFoundParticles = false;
                    UpdateData.bFoundMorphController = false;
                    UpdateData.bSceneGraphChange = false;
                    pcol->UpdateWorldData(UpdateData);
                }
                pPhantomBatch[i]->removeReference();
            }
            std::memset(pPhantomBatch, 0, sizeof(hkpPhantom*) * iPhantomBatchCount);
            iPhantomBatchCount = 0;
        }
    }
}

// An action not yet in a world goes in only when every entity it acts on is in this world. The entity list is a
// local hkArray, constructed empty and freed inline (heap bufFree unless it never allocated).
void bhkWorld::AddBatchedActions() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iActionBatchCount) {
        if (iActionBatchCount >= 100) {
            iActionBatchCount = 100;
        }
        ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pw) {
            for (uint32_t i = 0; i < iActionBatchCount; ++i) {
                hkpAction* pAction = pActionBatch[i];
                if (pAction && !pAction->m_world) {
                    FNVD_INLINED_LOCAL(hkArray<hkpEntity*>, entities, std::nullopt, std::nullopt);
                    entities.m_data = nullptr;
                    entities.m_size = 0;
                    entities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
                    pAction->getEntities(entities);
                    bool bSameWorld = true;
                    for (int32_t j = 0; j < entities.m_size; ++j) {
                        if (!bSameWorld) {
                            break;
                        }
                        bSameWorld = entities.m_data[j]->m_world == pw;
                    }
                    if (bSameWorld) {
                        pw->addAction(pActionBatch[i]);
                    }
                    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
                    entities.m_size = 0;
                    if (!(entities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG)) {
                        kHeap.bufFree(
                            entities.m_data,
                            (entities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK) * sizeof(hkpEntity*)
                        );
                    }
                    entities.m_data = nullptr;
                    entities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
                }
                if (pActionBatch[i]) {
                    pActionBatch[i]->removeReference();
                }
            }
            std::memset(pActionBatch, 0, sizeof(hkpAction*) * iActionBatchCount);
            iActionBatchCount = 0;
        }
    }
}

// Constraints wait while entities are still batched; a constraint already owned, or whose entities have no island
// yet, is dropped from the batch without being added.
void bhkWorld::AddBatchedConstraints() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    if (iConstraintInstanceBatchCount && !iEntityBatchCount) {
        if (static_cast<uint32_t>(iConstraintInstanceBatchCount) >= 200) {
            iConstraintInstanceBatchCount = 200;
        }
        ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
        if (pw) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(iConstraintInstanceBatchCount); ++i) {
                hkpConstraintInstance* pConstraint = pConstraintInstanceBatch[i];
                if (pConstraint && !pConstraint->m_owner &&
                    (!pConstraint->m_entities[0] || pConstraint->m_entities[0]->m_simulationIsland) &&
                    (!pConstraint->m_entities[1] || pConstraint->m_entities[1]->m_simulationIsland)) {
                    pw->addConstraint(pConstraint);
                }
                if (pConstraintInstanceBatch[i]) {
                    pConstraintInstanceBatch[i]->removeReference();
                }
            }
            std::memset(pConstraintInstanceBatch, 0, sizeof(hkpConstraintInstance*) * iConstraintInstanceBatchCount);
            iConstraintInstanceBatchCount = 0;
        }
    }
}

ahkpWorld* bhkWorld::GetahkpWorld() const {
    return static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
}

void bhkWorld::GetOrigin(NiPoint3& arOrigin) {
    HK2NI(arOrigin, Origin);
}

void bhkWorld::SetOrigin(const NiPoint3& arOrigin) {
    hkVector4 kOrigin;
    kOrigin.m_quad[0] = arOrigin.x * bhkConvert::fBS2HkScaleSC;
    kOrigin.m_quad[1] = arOrigin.y * bhkConvert::fBS2HkScaleSC;
    kOrigin.m_quad[2] = bhkConvert::fBS2HkScaleSC * arOrigin.z;
    kOrigin.m_quad[3] = 0.0f;
    SethkOrigin(kOrigin);
}

// The broadphase moves by its grid, so the origin advances by the shift Havok actually made. The vectors are subtracted
// and added per lane.
void bhkWorld::SethkOrigin(const hkVector4& arOrigin) {
    hkVector4 kShift;
    kShift.m_quad[0] = arOrigin.m_quad[0] - Origin.m_quad[0];
    kShift.m_quad[1] = arOrigin.m_quad[1] - Origin.m_quad[1];
    kShift.m_quad[2] = arOrigin.m_quad[2] - Origin.m_quad[2];
    kShift.m_quad[3] = arOrigin.m_quad[3] - Origin.m_quad[3];
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    // The write mark, empty in the game's build, leaves only its world lookup.
    GethkWorld();
    hkVector4 kEffectiveShift;
    pw->shiftBroadPhase(kShift, kEffectiveShift, hkpWorld::SHIFT_BROADPHASE_UPDATE_ENTITY_AABBS);
    GethkWorld();
    Origin.m_quad[0] = Origin.m_quad[0] + kEffectiveShift.m_quad[0];
    Origin.m_quad[1] = Origin.m_quad[1] + kEffectiveShift.m_quad[1];
    Origin.m_quad[2] = Origin.m_quad[2] + kEffectiveShift.m_quad[2];
    Origin.m_quad[3] = Origin.m_quad[3] + kEffectiveShift.m_quad[3];
}

ahkpWorld* bhkWorld::operator*() const {
    if (this) {
        return GetahkpWorld();
    }
    else {
        return nullptr;
    }
}

bool bhkWorld::Add(bhkSerializable* apObj) {
    if (apObj) {
        return apObj->Add(this);
    }
    return false;
}

// A full batch flushes every batch first. Outside batching a constraint goes in only when it has no owner yet and
// each of its entities is in an island.
void bhkWorld::Add(hkpConstraintInstance* apConstraint) {
    if (!this) {
        return;
    }
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (!pw) {
        return;
    }
    if (iBatchAdd) {
        if (static_cast<uint32_t>(iConstraintInstanceBatchCount) >= 200) {
            AddBatchedEntities();
            AddBatchedPhantoms();
            AddBatchedActions();
            AddBatchedConstraints();
        }
        apConstraint->addReference();
        pConstraintInstanceBatch[iConstraintInstanceBatchCount++] = apConstraint;
    }
    else if (
        apConstraint && !apConstraint->m_owner &&
        (!apConstraint->m_entities[0] || apConstraint->m_entities[0]->m_simulationIsland) &&
        (!apConstraint->m_entities[1] || apConstraint->m_entities[1]->m_simulationIsland)
    ) {
        pw->addConstraint(apConstraint);
    }
}

// The phantom's wrapper flag is one byte; mark it before flushing a full batch.
bool bhkWorld::Remove(hkpPhantom* apPhantom) {
    bhkSerializable* pSerializable = this;
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(pSerializable->GethkWorld()));
    if (!pw || !apPhantom) {
        return false;
    }
    bhkPhantom* pPhantom = static_cast<bhkPhantom*>(apPhantom->m_userData);
    if (!pPhantom || (pPhantom->ucFlags & 0x30)) {
        return true;
    }
    if (iBatchRemove) {
        pPhantom->ucFlags |= 0x20;
        if (static_cast<uint32_t>(iPhantomBatchCount) >= 200) {
            RemoveBatchedConstraints();
            RemoveBatchedActions();
            RemoveBatchedPhantoms();
            RemoveBatchedEntitiesPartial();
        }
        apPhantom->addReference();
        pPhantomBatch[iPhantomBatchCount] = apPhantom;
        ++iPhantomBatchCount;
    }
    else {
        pw->removePhantom(apPhantom);
    }
    return true;
}

// While adds are batched the object joins its batch (a full batch is flushed first) marked pending (flag 0x10).
// Otherwise it goes straight in, unless its wrapper is pending or being removed (flags 0x30). With
// bhkRigidBody::bStartInactive, a non-biped body whose linear and angular velocities are within 0.001 of zero is added
// asleep (|v - hkVector4Zero| <= 0.001f on x, y and z). The phantom's wrapper is found through the collidable's owner
// (hkpCollidable::getOwner).
void bhkWorld::Add(hkpWorldObject* apObject) {
    if (!this) {
        return;
    }
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (!pw) {
        return;
    }
    if (apObject->m_collidable.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
        bhkWorldObject* pwo = apObject->m_userData;
        if (iBatchAdd) {
            if (!pwo || (pwo->iFlags & 0x30)) {
                return;
            }
            if (iEntityBatchCount >= 3000) {
                if (iEntityBatchCount) {
                    iEntityBatchCount = 3000;
                    const uint32_t uiNum =
                        iEntityBatchCount < iEntityBatchAddRate ? iEntityBatchCount : iEntityBatchAddRate;
                    const uint32_t uiRest = iEntityBatchCount - uiNum;
                    AddEntities(&pEntityBatch[uiRest], uiNum);
                    iEntityBatchCount = uiRest;
                    bFrameBatchProcessed = 1;
                }
                AddBatchedPhantoms();
                AddBatchedActions();
                AddBatchedConstraints();
            }
            pwo->iFlags |= 0x10;
            apObject->addReference();
            pEntityBatch[iEntityBatchCount++] = static_cast<hkpEntity*>(apObject);
        }
        else {
            if (!pwo || (pwo->iFlags & 0x30)) {
                return;
            }
            hkpEntityActivation eActivation = HK_ENTITY_ACTIVATION_DO_ACTIVATE;
            if (bhkRigidBody::bStartInactive) {
                CFilter kFilter;
                if ((pwo->GetCollisionFilter(&kFilter)->iFilter & 0x7F) != L_BIPED) {
                    bhkRigidBody* pbody = static_cast<bhkRigidBody*>(pwo);
                    const hkVector4& kLinear = pbody->GetLinearVelocity();
                    if (__builtin_fabsf(kLinear.m_quad[0] - hkVector4Zero.m_quad[0]) <= 0.001f &&
                        __builtin_fabsf(kLinear.m_quad[1] - hkVector4Zero.m_quad[1]) <= 0.001f &&
                        __builtin_fabsf(kLinear.m_quad[2] - hkVector4Zero.m_quad[2]) <= 0.001f) {
                        const hkVector4& kAngular = pbody->GetAngularVelocity();
                        if (__builtin_fabsf(kAngular.m_quad[0] - hkVector4Zero.m_quad[0]) <= 0.001f &&
                            __builtin_fabsf(kAngular.m_quad[1] - hkVector4Zero.m_quad[1]) <= 0.001f &&
                            __builtin_fabsf(kAngular.m_quad[2] - hkVector4Zero.m_quad[2]) <= 0.001f) {
                            eActivation = HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE;
                        }
                    }
                }
            }
            pw->addEntity(static_cast<hkpEntity*>(apObject), eActivation);
        }
        return;
    }
    hkpCollidable* pc = &apObject->m_collidable;
    if (pc->m_broadPhaseHandle.m_type != hkpWorldObject::BROAD_PHASE_PHANTOM) {
        return;
    }
    hkpWorldObject* pOwner = reinterpret_cast<hkpWorldObject*>(reinterpret_cast<uint8_t*>(pc) + pc->m_ownerOffset);
    if (!pOwner) {
        return;
    }
    bhkPhantom* pphantom = static_cast<bhkPhantom*>(pOwner->m_userData);
    if (!pphantom || (pphantom->ucFlags & 0x30)) {
        return;
    }
    if (iBatchAdd) {
        if (static_cast<uint32_t>(iPhantomBatchCount) >= 200) {
            if (iEntityBatchCount && !bFrameBatchProcessed) {
                if (iEntityBatchCount >= 3000) {
                    iEntityBatchCount = 3000;
                }
                const uint32_t uiNum =
                    iEntityBatchCount < iEntityBatchAddRate ? iEntityBatchCount : iEntityBatchAddRate;
                const uint32_t uiRest = iEntityBatchCount - uiNum;
                AddEntities(&pEntityBatch[uiRest], uiNum);
                iEntityBatchCount = uiRest;
                bFrameBatchProcessed = 1;
            }
            AddBatchedPhantoms();
            AddBatchedActions();
            AddBatchedConstraints();
        }
        pphantom->ucFlags |= 0x10;
        apObject->addReference();
        pPhantomBatch[iPhantomBatchCount++] = static_cast<hkpPhantom*>(apObject);
    }
    else {
        pw->addPhantom(static_cast<hkpPhantom*>(apObject));
    }
}

// The listener is handed to the world as it is: the engine passes the same pointer for both uses.
void bhkWorld::AddWaterListener(bhkWaterListener* apListener) {
    AddEntityListener(apListener);
    pWaterListener = static_cast<TESWaterListener*>(apListener);
}

void bhkWorld::AddAcousticSpaceListener(BGSAcousticSpaceListener* apListener) {
    AddEntityListener(apListener);
    pAcousticSpaceListener = apListener;
}

bhkEntityListener* bhkWorld::QAcousticSpaceListener() {
    return pAcousticSpaceListener;
}

bhkWaterListener* bhkWorld::QWaterListener() {
    return pWaterListener;
}

void bhkWorld::ProcessBatchRemovals(bool abAllowPartial) {
    RemoveBatchedConstraints();
    RemoveBatchedActions();
    RemoveBatchedPhantoms();
    if (abAllowPartial) {
        RemoveBatchedEntitiesPartial();
    }
    else {
        RemoveBatchedEntities();
    }
}

char bhkWorld::RemoveBody(bhkSerializable* apObject) {
    if (apObject) {
        return (int8_t)apObject->Remove();
    }
    else {
        return 0;
    }
}

void* bhkWorld::GetWaterListener() {
    return pWaterListener;
}

// Puts every active island whose entities all have (near) zero linear and angular velocity to sleep, working on a copy
// of the active island list. GethkWorld is called around each world operation and the results are dropped, as the game
// does.
void bhkWorld::DeactivateAllIslands() {
    hkpWorld* pWorld = const_cast<hkpWorld*>(GethkWorld());
    if (pWorld) {
        GethkWorld();
        hkpWorldOperationUtil::cleanupDirtyIslands(pWorld);
        GethkWorld();
        // EXE-INLINED none: hkArray<hkpSimulationIsland*>::hkArray
        FNVD_INLINED_LOCAL(hkArray<hkpSimulationIsland*>, kIslands, std::nullopt, std::nullopt);
        kIslands.m_data = nullptr;
        kIslands.m_size = 0;
        kIslands.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkpSimulationIsland*>::DONT_DEALLOCATE_FLAG);
        GethkWorld();
        // EXE-INLINED none: hkArray<hkpSimulationIsland*>::operator=
        kIslands.copyFromArray(hkMemoryRouter::getInstance().heap(), pWorld->m_activeSimulationIslands, hkTypeIsPod());
        GethkWorld();
        for (int32_t i = 0; i < kIslands.m_size; ++i) {
            hkpSimulationIsland* pIsland = kIslands.m_data[i];
            int32_t iNumEntities = pIsland->m_entities.m_size;
            bool bActive = false;
            for (int32_t j = 0; j < iNumEntities && !bActive; ++j) {
                hkpEntity* pEntity = pIsland->m_entities.m_data[j];
                if (!pEntity->m_motion.m_linearVelocity.equals3(hkVector4::getZero(), 0.001f) ||
                    !pEntity->m_motion.m_angularVelocity.equals3(hkVector4::getZero(), 0.001f)) {
                    bActive = true;
                }
            }
            if (!bActive) {
                GethkWorld();
                hkpWorldOperationUtil::markIslandInactive(pWorld, pIsland);
                GethkWorld();
            }
        }
        GethkWorld();
        hkpWorldOperationUtil::cleanupDirtyIslands(pWorld);
        GethkWorld();
        // EXE-INLINED none: hkArray<hkpSimulationIsland*>::~hkArray
        hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
        if ((kIslands.m_capacityAndFlags & hkArrayBase<hkpSimulationIsland*>::DONT_DEALLOCATE_FLAG) == 0) {
            rHeap._bufFree<hkpSimulationIsland*>(kIslands.m_data, kIslands.getCapacity());
        }
    }
}

// Tears Havok down inside the Havok heap context: the phantom overlap listener, the character controllers' default
// state manager and shape, the shared collision filter and the multithreaded system (bhkWorld::pMultithreadedSystem is
// freed but not cleared), then hkBaseSystem, then the free-list memory system, which is quit, destroyed and
// uninstalled.
void bhkWorld::QuitSystem() {
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    bhkPhantomOverlapListener::KillInstance();
    bhkCharacterControllerCinfo::ReleaseDefaultManager();
    bhkCharacterController::ReleaseDefaultShape();
    if (pFilter) {
        pFilter->removeReference();
        pFilter = nullptr;
    }
    bhkMultithreadedSystem* pSystem = pMultithreadedSystem;
    if (pSystem) {
        // EXE-INLINED none: bhkMultithreadedSystem's scalar deleting destructor
        pSystem->~bhkMultithreadedSystem();
        MemoryManager_Singleton.Deallocate(pSystem);
    }
    hkBaseSystem::quit();
    hkFreeListMemorySystem::pSingleton->mainQuit(hkMemorySystem::FLAG_ALL);
    static_cast<hkMemorySystem*>(hkFreeListMemorySystem::pSingleton)->~hkMemorySystem();
    hkFreeListMemorySystem::pSingleton = nullptr;
    hkMemorySystem::replaceInstance(nullptr);
}

bool bhkWorld::IsHavokDebug() {
    return bHavokDebug;
}

uint8_t bhkWorld::QPickCountOverride() {
    return bPickCountOverride;
}

void bhkWorld::SetPickCountOverride(uint8_t abVal) {
    bPickCountOverride = abVal;
}

void bhkWorld::ResetBatchProcessed() {
    bFrameBatchProcessed = 0;
}

void bhkWorld::SetUpdateType(uint32_t aiType) {
    iUpdateType = aiType;
}

void bhkWorld::SetAddBipedWhenKeyframed(bool ab) {
    bAddBipedWhenKeyframed = ab;
}

void bhkWorld::SetEntityBatchRemoveRate(uint32_t aiRate) {
    iEntityBatchRemoveRate = aiRate;
}

void bhkWorld::SetChaseDeltaMult(float afMult) {
    fChaseDeltaMultPerFrame = afMult;
}

void bhkWorld::SetDebrisMaxVelocity(float af) {
    fDebrisMaxVelocity = af;
}

// Without abForce only a model flagged HAVOK is changed. The walk has no world; uDataA[0] is the motion type and
// uDataA[1] whether the body may be activated. True once the walk was due, even without a handler.
bool bhkWorld::SetMotion(
    NiAVObject* apObject,
    hkpMotion::MotionType aeType,
    bool abRecurse,
    bool abForce,
    bool abAllowActivate
) {
    bool bres = false;
    if (!apObject) {
        return bres;
    }
    if (!abForce) {
        BSXFlags* pFlags = BSXFlags::Find(apObject);
        if (!pFlags || !(pFlags->m_iValue & BSXFlags::HAVOK)) {
            return bres;
        }
    }
    ObjectRecData arData;
    arData.bRecurse = abRecurse;
    bres = true;
    arData.pWorld = nullptr;
    arData.eAction = OA_MOTION;
    arData.uDataA[0].i = aeType;
    arData.uDataA[1].i = abAllowActivate;
    if (pRecFunctionA[OA_MOTION]) {
        DoObjectRec(apObject, arData, pRecFunctionA[OA_MOTION]);
    }
    return bres;
}

void bhkWorld::SetDebrisMinExtent(float af) {
    fDebrisMinExtent = af;
}

void bhkWorld::SetMaxPickTimeNormal(float afTime) {
    fMaxPickTime = afTime;
}

void bhkWorld::SetMaxPickTimeVATS(float afTime) {
    fMaxPickTimeVATS = afTime;
}

void bhkWorld::SetRecFunction(
    bhkWorldNS::OBJECTACTION aeAction,
    void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&)
) {
    pRecFunctionA[aeAction] = apFunc;
}

bool bhkWorld::Activate(NiAVObject* apObject, bool abActivate, bool abRecurse, bool abForce) {
    BSXFlags* v5;
    bhkWorld::ObjectRecData arData;
    uint8_t v4 = 0;
    if (apObject) {
        if (abForce || (((v5 = BSXFlags::Find(apObject)) != nullptr) && ((v5->m_iValue & 2) != 0))) {
            arData.bRecurse = (bool)((uint8_t)abRecurse);
            v4 = 1;
            arData.pWorld = nullptr;
            arData.eAction = bhkWorld::OA_ACTIVATE;
            arData.uDataA[0].i = (uint32_t)((uint8_t)abActivate);
            if (bhkWorld::pRecFunctionA[12]) {
                bhkWorld::DoObjectRec(apObject, arData, bhkWorld::pRecFunctionA[12]);
            }
        }
    }
    return (bool)v4;
}

bool bhkWorld::RemoveObjects(NiAVObject* apObject, bool abRecurse, bool abForce) {
    BSXFlags* v4;
    bhkWorld::ObjectRecData arData;
    uint8_t result = 0;
    if (apObject) {
        if (abForce || (((v4 = BSXFlags::Find(apObject)) != nullptr) && ((v4->m_iValue & 2) != 0))) {
            arData.bRecurse = (bool)((uint8_t)abRecurse);
            arData.pWorld = nullptr;
            arData.eAction = bhkWorld::OA_REMOVE;
            if (bhkWorld::pRecFunctionA[1]) {
                bhkWorld::DoObjectRec(apObject, arData, bhkWorld::pRecFunctionA[1]);
            }
            return (bool)((uint8_t)abForce);
        }
        else {
            return false;
        }
    }
    return (bool)result;
}

// uDataA[0] bit 0 marks the object keyframed for this add, uDataA[2] counts the objects added (a blend object added
// first leaves its 0x100 flag set), and uDataA[3] asks for the 0x40 reset flag when the body joins the world.
void bhkWorld::AddObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pwo = apCollision->spWorldObject;
    if (pwo) {
        bhkBlendCollisionObject* pbc = apCollision->IsBhkBlendCollisionObject();
        if (pbc) {
            if (arData.uDataA[2].i) {
                pbc->usFlags &= ~0x100;
            }
            else {
                pbc->usFlags |= 0x100;
            }
        }
        if (arData.uDataA[0].i & 1) {
            apCollision->usFlags |= 4;
            apCollision->usFlags |= 8;
        }
        else {
            apCollision->usFlags &= ~4;
        }
        if (bInitGroupDuringAdd) {
            SetCollisionGroup(pwo, arData);
        }
        ahkpWorld* pWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(arData.pWorld->GethkWorld()));
        if (pwo->GethkWorld() != pWorld) {
            if (arData.uDataA[3].i) {
                apCollision->usFlags |= 0x40;
            }
            else {
                apCollision->usFlags &= ~0x40;
            }
            pwo->Add(arData.pWorld);
        }
        NiUpdateData UpdateData;
        UpdateData.fTime = 0.0f;
        UpdateData.bUpdateControllers = false;
        UpdateData.bParallelUpdate = false;
        UpdateData.bFoundParticles = false;
        UpdateData.bFoundMorphController = false;
        UpdateData.bSceneGraphChange = false;
        NiAVObject* pav = apCollision->m_pkSceneObject;
        pav->UpdateWorldData(UpdateData);
        if (arData.pWorld->bDebugDisplay) {
            apCollision->SetDebugDisplay(true);
        }
    }
    arData.uDataA[0].i &= ~1u;
    ++arData.uDataA[2].i;
}

// An add (OA_ADD) flags every visited node for a selective rigid-free transform update first.
void bhkWorld::DoObjectRec(
    NiAVObject* apObject,
    bhkWorldNS::ObjectRecData& arData,
    void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&)
) {
    if (!apObject) {
        return;
    }
    NiCollisionObject* pCollision = apObject->m_spCollisionObject.m_pObject;
    bhkNiCollisionObject* pcol = pCollision ? pCollision->IsBhkNiCollisionObject() : nullptr;
    if (arData.eAction == OA_ADD) {
        apObject->m_uFlags = static_cast<NiAVObject::NiFlags>(
            (apObject->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE_RIGID) | NiAVObject::SELECTIVE_UPDATE |
            NiAVObject::SELECTIVE_UPDATE_TRANSFORMS
        );
    }
    if (pcol) {
        apFunc(pcol, arData);
    }
    if (arData.bRecurse) {
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            for (uint32_t I = 0; I < pnode->m_kChildren.m_usSize; ++I) {
                NiAVObject* pchild = pnode->m_kChildren.m_pBase[I].m_pObject;
                DoObjectRec(pchild, arData, apFunc);
            }
        }
    }
}

// A forced walk bypasses BSXFlags; a missing handler still reports that the walk was requested.
bool bhkWorld::UpdatePosition(NiAVObject* apObject, bool abRecurse, bool abForce) {
    if (!apObject) {
        return false;
    }
    if (!abForce) {
        BSXFlags* pFlags = BSXFlags::Find(apObject);
        if (!pFlags || !(pFlags->m_iValue & BSXFlags::HAVOK)) {
            return false;
        }
    }
    ObjectRecData arData;
    arData.bRecurse = abRecurse;
    arData.pWorld = nullptr;
    arData.eAction = OA_NI2HK;
    RecFunction apFunc = pRecFunctionA[OA_NI2HK];
    if (apFunc) {
        DoObjectRec(apObject, arData, apFunc);
    }
    return true;
}

// The kill walk uses the same flag gate as position updates, with the OA_KILLHAVOK handler.
bool bhkWorld::KillHavok(NiAVObject* apObject, bool abRecurse, bool abForce) {
    if (!apObject) {
        return false;
    }
    if (!abForce) {
        BSXFlags* pFlags = BSXFlags::Find(apObject);
        if (!pFlags || !(pFlags->m_iValue & BSXFlags::HAVOK)) {
            return false;
        }
    }
    ObjectRecData arData;
    arData.bRecurse = abRecurse;
    arData.pWorld = nullptr;
    arData.eAction = OA_KILLHAVOK;
    RecFunction apFunc = pRecFunctionA[OA_KILLHAVOK];
    if (apFunc) {
        DoObjectRec(apObject, arData, apFunc);
    }
    return true;
}

void bhkWorld::DoObjects(NiAVObject* apObject, bhkWorldNS::ObjectRecData& arData) {
    if (bhkWorld::pRecFunctionA[(int32_t)(&arData)->eAction]) {
        bhkWorld::DoObjectRec(apObject, arData, bhkWorld::pRecFunctionA[(int32_t)(&arData)->eAction]);
    }
}

bool bhkWorld::DoObjects(
    NiAVObject* apObject,
    bhkWorldNS::ObjectRecData& arData,
    void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&)
) {
    bool bResult = false;
    if (apObject) {
        BSXFlags* pFlags = BSXFlags::Find(apObject);
        if (pFlags && (pFlags->m_iValue & BSXFlags::HAVOK)) {
            bResult = true;
            bhkWorld::DoObjectRec(apObject, arData, apFunc);
        }
    }
    return bResult;
}

void bhkWorld::NI2HKObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData&) {
    apCollision->usFlags |= 0x40;
}

void bhkWorld::SetMotionOnObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    apCollision->SetMotionType(
        static_cast<hkpMotion::MotionType>(arData.uDataA[0].i), nullptr, arData.uDataA[1].i != 0
    );
}

void bhkWorld::AdjustForceAddCount(bhkNiCollisionObject* apObject, bhkWorldNS::ObjectRecData& apData) {
    if (!apObject) {
        return;
    }
    bhkBlendCollisionObject* pBlendCollision = apObject->IsBhkBlendCollisionObject();
    if (!pBlendCollision) {
        return;
    }
    if (apData.uDataA[0].i != 0) {
        pBlendCollision->IncForceAdd();
    }
    else {
        pBlendCollision->DecForceAdd();
    }
}

void bhkWorld::EnableObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (arData.uDataA[0].i) {
        apCollision->usFlags |= 1;
    }
    else {
        apCollision->usFlags &= 0xFFFE;
    }
}

void bhkWorld::RemoveObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkRigidBody* pWorldObject = apCollision->spWorldObject;
    if (pWorldObject) {
        pWorldObject->Remove();
    }
}

void bhkWorld::DoEnableCollision(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkRigidBody* pWorldObject = apCollision->spWorldObject;
    if (pWorldObject) {
        pWorldObject->EnableCollision(arData.uDataA[0].i != 0);
    }
}

void bhkWorld::ActivateObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (apCollision) {
        bhkRigidBody* pWorldObject = apCollision->spWorldObject;
        if (pWorldObject) {
            bhkRigidBody* pRigidBody = pWorldObject->IsBhkRigidBody();
            if (pRigidBody) {
                pRigidBody->SetActive(arData.uDataA[0].i != 0);
            }
        }
    }
}

void bhkWorld::DoDisable(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkRigidBody* pWorldObject = apCollision->spWorldObject;
    if (pWorldObject) {
        bhkRigidBody* pRigidBody = pWorldObject->IsBhkRigidBody();
        if (pRigidBody) {
            if (arData.uDataA[0].i) {
                pRigidBody->iFlags |= 0x40;
            }
            else {
                pRigidBody->iFlags &= ~0x40u;
            }
        }
    }
}

bool bhkWorld::AdjustForceAddCount(NiAVObject* apObject, bool abIncrement, bool abRecurse, bool abForce) {
    bool v4 = false;
    if (apObject) {
        BSXFlags* v5;
        if (abForce || ((v5 = BSXFlags::Find(apObject)) != nullptr && (v5->m_iValue & 2) != 0)) {
            bhkWorld::ObjectRecData arData;
            arData.bRecurse = abRecurse;
            v4 = true;
            arData.pWorld = nullptr;
            arData.eAction = bhkWorld::OA_FORCEADD;
            arData.uDataA[0].i = abIncrement;
            if (bhkWorld::pRecFunctionA[13]) {
                bhkWorld::DoObjectRec(apObject, arData, bhkWorld::pRecFunctionA[13]);
            }
        }
    }
    return v4;
}

// The step count is capped at 1 for a quick simulation and 3 otherwise. Update type 0 steps at the setting's step time
// scaled by the global time multiplier and carries the rest over; 2 runs one step of the frame's own time (afDeltaTime,
// not the capped fDeltaTime); 10 runs one step of a smoothed (chased) time; any other splits the time into equal steps.
// The comparisons keep the game's NaN behaviour.
void bhkWorld::SetDeltaTime(float afDeltaTime, bool abQuickSimulate, bool abSimulateEveryFrame) {
    fBSDeltaTime = afDeltaTime;
    const bool bEveryFrame = abSimulateEveryFrame || balwaysSimulateEveryFrame;
    fDeltaTime = fTimeRemainder + afDeltaTime;
    if (fDeltaTime > 166.666672f) {
        fDeltaTime = 166.666672f;
    }
    else if (fDeltaTime <= 0.0f) {
        iSteps = 0;
        fStepTime = 0.0f;
        return;
    }
    const float fDelta = fDeltaTime;
    const uint32_t imaxSteps = 2 * !abQuickSimulate + 1;
    switch (iUpdateType) {
        case 0:
            fStepTime = fMaxStepTime.uValue.f * BSTimer::fGlobalTimeMultiplier;
            iSteps = static_cast<uint32_t>(fDelta / fStepTime);
            if (iSteps > imaxSteps) {
                iSteps = imaxSteps;
            }
            if (iSteps) {
                fTimeRemainder = fDelta - fStepTime * iSteps;
                fDeltaTime = fDelta - fTimeRemainder;
                fTimeRemainder = BSMIN(fTimeRemainder, fMaxStepTime.Float());
            }
            else if (bEveryFrame || !(fStepTime * 0.5 > fDelta)) {
                fStepTime = fDelta;
                iSteps = 1;
                fTimeRemainder = 0.0f;
            }
            else {
                fTimeRemainder = fDelta;
                fDeltaTime = 0.0f;
                fStepTime = 0.0f;
            }
            break;
        case 2:
            iSteps = 1;
            fStepTime = afDeltaTime;
            break;
        case 10:
            fChaseDelta = (fDelta - fChaseDelta) * fChaseDeltaMultPerFrame + fChaseDelta;
            fStepTime = fChaseDelta;
            iSteps = 1;
            fTimeRemainder = 0.0f;
            break;
        default:
            if (bEveryFrame || !(0.00833333377f > fDelta)) {
                fTimeRemainder = 0.0f;
                iSteps = static_cast<uint32_t>(fDelta / fMaxStepTime.uValue.f);
                if (iSteps > imaxSteps) {
                    iSteps = imaxSteps;
                }
                if (iSteps) {
                    fStepTime = fDelta / iSteps;
                }
                else {
                    iSteps = 1;
                    fStepTime = fDelta;
                }
            }
            else {
                iSteps = 0;
                fTimeRemainder = fDelta;
                fDeltaTime = 0.0f;
                fStepTime = 0.0f;
            }
            break;
    }
}

void bhkWorld::ClearPickData() {
    bhkWorld::iCurrentPickCount = 0;
    bhkWorld::fCurrentPickTime = 0.0f;
}

void bhkWorld::SetVatsReplay(bool abValue) {
    bhkWorld::bVatsReplay = (bool)((uint8_t)abValue);
}

bool bhkWorld::Enable(NiAVObject* apObject, bool abEnable, bool abRecurse, bool abForce) {
    BSXFlags* v5;
    bhkWorld::ObjectRecData v7;
    int8_t v4 = 0;
    if (!apObject) {
        return (bool)v4;
    }
    if ((((uint8_t)abForce) == 0) && (((v5 = BSXFlags::Find(apObject)) == nullptr) || ((v5->m_iValue & 2) == 0))) {
        return (bool)v4;
    }
    v7.bRecurse = (bool)((uint8_t)abRecurse);
    v4 = 1;
    v7.pWorld = nullptr;
    v7.eAction = bhkWorld::OA_ENABLE;
    v7.uDataA[0].i = (int32_t)((uint8_t)abEnable);
    if (bhkWorld::pRecFunctionA[3]) {
        bhkWorld::DoObjectRec(apObject, v7, bhkWorld::pRecFunctionA[3]);
    }
    return (bool)v4;
}

bool bhkWorld::EnableCollision(NiAVObject* apObject, bool abEnable, bool abRecurse, bool abForce) {
    BSXFlags* v5;
    bhkWorld::ObjectRecData v7;
    int8_t v4 = 0;
    if (!apObject) {
        return (bool)v4;
    }
    if ((((uint8_t)abForce) == 0) && (((v5 = BSXFlags::Find(apObject)) == nullptr) || ((v5->m_iValue & 2) == 0))) {
        return (bool)v4;
    }
    v7.bRecurse = (bool)((uint8_t)abRecurse);
    v4 = 1;
    v7.pWorld = nullptr;
    v7.eAction = bhkWorld::OA_ENABLECOLLISION;
    v7.uDataA[0].i = (int32_t)((uint8_t)abEnable);
    if (bhkWorld::pRecFunctionA[14]) {
        bhkWorld::DoObjectRec(apObject, v7, bhkWorld::pRecFunctionA[14]);
    }
    return (bool)v4;
}

bool bhkWorld::SetDisabled(NiAVObject* apObject, bool abDisabled, bool abRecurse, bool abForce) {
    BSXFlags* v5;
    bhkWorld::ObjectRecData v7;
    int8_t v4 = 0;
    if (!apObject) {
        return (bool)v4;
    }
    if ((((uint8_t)abForce) == 0) && (((v5 = BSXFlags::Find(apObject)) == nullptr) || ((v5->m_iValue & 2) == 0))) {
        return (bool)v4;
    }
    v7.bRecurse = (bool)((uint8_t)abRecurse);
    v4 = 1;
    v7.pWorld = nullptr;
    v7.eAction = bhkWorld::OA_DISABLE;
    v7.uDataA[0].i = (int32_t)((uint8_t)abDisabled);
    if (bhkWorld::pRecFunctionA[17]) {
        bhkWorld::DoObjectRec(apObject, v7, bhkWorld::pRecFunctionA[17]);
    }
    return (bool)v4;
}

bool bhkWorld::SetGroup(NiAVObject* apObject, uint32_t aiGroup, bool abRecurse, bool abForce) {
    BSXFlags* v5;
    bhkWorld::ObjectRecData kData;
    uint8_t bResult = 0;
    if (apObject) {
        if (abForce || (((v5 = BSXFlags::Find(apObject)) != nullptr) && ((v5->m_iValue & 2) != 0))) {
            kData.bRecurse = (bool)((uint8_t)abRecurse);
            bResult = 1;
            kData.pWorld = nullptr;
            kData.eAction = bhkWorld::OA_GROUP;
            kData.uDataA[0].i = aiGroup;
            if (bhkWorld::pRecFunctionA[7]) {
                bhkWorld::DoObjectRec(apObject, kData, bhkWorld::pRecFunctionA[7]);
            }
        }
    }
    return (bool)bResult;
}

bool bhkWorld::IsActive(NiAVObject* apObject, bool abRecurse, bool abForce) {
    BSXFlags* v3;
    bhkWorld::ObjectRecData v5;
    if (!apObject) {
        return false;
    }
    if (!abForce) {
        v3 = BSXFlags::Find(apObject);
        if ((!v3) || ((v3->m_iValue & 2) == 0)) {
            return false;
        }
    }
    v5.bRecurse = (bool)((uint8_t)abRecurse);
    v5.pWorld = nullptr;
    v5.eAction = bhkWorld::OA_QACTIVE;
    v5.uDataA[0].i = 0;
    if (bhkWorld::pRecFunctionA[8]) {
        bhkWorld::DoObjectRec(apObject, v5, bhkWorld::pRecFunctionA[8]);
    }
    return (v5.uDataA[0].i != 0);
}

bool bhkWorld::QPickTimeOverMax() {
    return (bool)((uint8_t)((!bhkWorld::QPickCountOverride()) &&
                            (bhkWorld::fCurrentPickTime > bhkWorld::QMaxPickTime())));
}

bool bhkWorld::ChangeLayer(NiAVObject* apObject, COL_LAYER aeTo, COL_LAYER aeFrom, bool abRecurse, bool abForce) {
    BSXFlags* v6;
    bhkWorld::ObjectRecData v8;
    int8_t v5 = 0;
    if (apObject) {
        if (abForce || (((v6 = BSXFlags::Find(apObject)) != nullptr) && ((v6->m_iValue & 2) != 0))) {
            v8.bRecurse = (bool)((uint8_t)abRecurse);
            v5 = 1;
            v8.pWorld = nullptr;
            v8.eAction = bhkWorld::OA_CHANGELAYER;
            v8.uDataA[0].i = (uint32_t)((int32_t)aeFrom);
            v8.uDataA[1].i = (uint32_t)((int32_t)aeTo);
            if (bhkWorld::pRecFunctionA[9]) {
                bhkWorld::DoObjectRec(apObject, v8, bhkWorld::pRecFunctionA[9]);
            }
        }
    }
    return (bool)v5;
}

bool bhkWorld::SetRestitution(NiAVObject* apObject, float afRestitution, bool abRecurse, bool abForce) {
    bool bres = false;
    if (apObject) {
        BSXFlags* v5;
        if (abForce || ((v5 = BSXFlags::Find(apObject)) != nullptr && (v5->m_iValue & 2) != 0)) {
            bhkWorldNS::ObjectRecData recData;
            recData.bRecurse = abRecurse;
            recData.uDataA[0].f = afRestitution;
            bres = true;
            recData.pWorld = nullptr;
            recData.eAction = bhkWorldNS::OA_SETRESTITUTION;
            if (bhkWorld::pRecFunctionA[15]) {
                bhkWorld::DoObjectRec(apObject, recData, bhkWorld::pRecFunctionA[15]);
            }
        }
    }
    return bres;
}

bool bhkWorld::SetFriction(NiAVObject* apObject, float afFriction, bool abRecurse, bool abForce) {
    bool bres = false;
    if (apObject) {
        BSXFlags* v5;
        if (abForce || ((v5 = BSXFlags::Find(apObject)) != nullptr && (v5->m_iValue & 2) != 0)) {
            bhkWorldNS::ObjectRecData recData;
            recData.bRecurse = abRecurse;
            recData.uDataA[0].f = afFriction;
            bres = true;
            recData.pWorld = nullptr;
            recData.eAction = bhkWorldNS::OA_SETFRICTION;
            if (bhkWorld::pRecFunctionA[16]) {
                bhkWorld::DoObjectRec(apObject, recData, bhkWorld::pRecFunctionA[16]);
            }
        }
    }
    return bres;
}

bool bhkWorld::SetCollisionGroup(bhkWorldObject* apObject, bhkWorldNS::ObjectRecData& arData) {
    hkpLinkedCollidable* p_m_collidable;
    uint32_t m_collisionFilterInfo;
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(apObject->phkObject);
    bool bres = false;
    if (phkObject && (p_m_collidable = &phkObject->m_collidable) != nullptr) {
        m_collisionFilterInfo = p_m_collidable->m_broadPhaseHandle.m_collisionFilterInfo;
    }
    else {
        m_collisionFilterInfo = 0;
    }
    uint32_t igroup = static_cast<uint16_t>(m_collisionFilterInfo >> 16);
    if (arData.uDataA[1].i) {
        apObject->SetCollisionGroup(arData.uDataA[1].i);
        return true;
    }
    else {
        if (!igroup) {
            igroup = bhkCollisionFilter::iLayerGroupA[m_collisionFilterInfo & 127];
            if (!igroup) {
                igroup = static_cast<uint16_t>(bhkCollisionFilter::uiNextGroup + 1);
                bhkCollisionFilter::uiNextGroup = igroup;
                if (!igroup) {
                    igroup = 10;
                    bhkCollisionFilter::uiNextGroup = 10;
                }
            }
            bres = true;
        }
        arData.uDataA[1].i = igroup;
        if (bres) {
            apObject->SetCollisionGroup(igroup);
        }
        return bres;
    }
}

void bhkWorld::DebugDisplayObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    apCollision->SetDebugDisplay(arData.uDataA[0u].i != 0);
}

void bhkWorld::DoSetRestitution(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pwo = (bhkWorldObject*)apCollision->spWorldObject;
    if (pwo) {
        bhkRigidBody* prb = pwo->IsBhkRigidBody();
        if (prb) {
            static_cast<hkpRigidBody*>(prb->phkObject)->m_material.m_restitution = arData.uDataA[0u].f;
        }
    }
}

void bhkWorld::DoSetFriction(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pwo = (bhkWorldObject*)apCollision->spWorldObject;
    if (pwo) {
        bhkRigidBody* prb = pwo->IsBhkRigidBody();
        if (prb) {
            static_cast<hkpRigidBody*>(prb->phkObject)->m_material.m_friction = arData.uDataA[0u].f;
        }
    }
}

void bhkWorld::SetCollisionGroup_00C67240(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pwo = apCollision->spWorldObject;
    if (pwo) {
        pwo->SetCollisionGroup(arData.uDataA[0u].i);
    }
}

void bhkWorld::QActiveObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pwo = apCollision->spWorldObject;
    if (pwo) {
        bhkRigidBody* prb = pwo->IsBhkRigidBody();
        if (prb) {
            hkpEntity* pEntity = static_cast<hkpEntity*>(prb->phkObject);
            if (pEntity && pEntity->isActive()) {
                ++arData.uDataA[0u].i;
            }
            arData.bRecurse = false;
        }
    }
}

// The layer is the low 7 bits of the collidable's filter info (0 when there is no Havok object).
void bhkWorld::ChangeLayerObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkWorldObject* pwo = apCollision->spWorldObject;
    uint32_t uiTo = arData.uDataA[1].i;
    uint32_t uiFrom = arData.uDataA[0u].i;
    hkpWorldObject* phkObject = static_cast<hkpWorldObject*>(pwo->phkObject);
    uint32_t uiFilterInfo = 0;
    if (phkObject) {
        const hkpCollidable* pCollidable = phkObject->getCollidable();
        if (pCollidable) {
            uiFilterInfo = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
        }
    }
    uint32_t uiCurrent = uiFilterInfo & 0x7F;
    if (uiTo != uiCurrent && (uiFrom == L_UNIDENTIFIED || uiFrom == uiCurrent)) {
        uint32_t uiNewFilterInfo = ((uiFilterInfo ^ uiTo) & 0x7F) ^ uiFilterInfo;
        if (phkObject) {
            hkpLinkedCollidable* pCollidable = &phkObject->m_collidable;
            if (pCollidable) {
                pCollidable->m_broadPhaseHandle.m_collisionFilterInfo = uiNewFilterInfo;
            }
        }
        pwo->UpdateCFilter();
    }
}

void bhkWorld::ShiftPosition(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    bhkBlendCollisionObject* pbc = apCollision->IsBhkBlendCollisionObject();
    bhkWorldObject* pwo = apCollision->spWorldObject;
    if (pwo) {
        bhkRigidBody* prb = pwo->IsBhkRigidBody();
        if (prb) {
            hkVector4* poffset = static_cast<hkVector4*>(arData.uDataA[0u].p);
            if (poffset) {
                __m128 m_quad = poffset->m_quad;
                __m128 v10 = poffset->m_quad;
                if (!pbc ||
                    (!(pbc->fHeirGain < 1.0f) ||
                     (0.0f != pbc->fHeirGain &&
                      (v10 = _mm_mul_ps(
                           _mm_shuffle_ps(
                               _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(pbc->fVelGain))),
                               _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(pbc->fVelGain))), 0
                           ),
                           m_quad
                       ),
                       (_mm_movemask_ps(_mm_cmple_ps(
                            _mm_and_ps(_mm_sub_ps(v10, hkVector4Zero.m_quad), hkVector4_setAbs4_absMask),
                            _mm_shuffle_ps(
                                _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)),
                                _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), 0
                            )
                        )) &
                        7) != 7)))) {
                    hkVector4 v9;
                    prb->GetPosition(v9);
                    v9.m_quad = _mm_add_ps(v9.m_quad, v10);
                    prb->SetPosition(v9);
                }
            }
        }
    }
}

void bhkWorld::KillHavokOnObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& a2) {
    NiAVObject* pSceneObject = apCollision->m_pkSceneObject;
    if (pSceneObject) {
        pSceneObject->m_spCollisionObject = nullptr;
        bhkNiCollisionObject* v4 = pSceneObject->m_spCollisionObject.m_pObject;
        if (v4) {
            if (v4->m_pkSceneObject != pSceneObject) {
                v4->SetSceneGraphObject(pSceneObject);
            }
        }
    }
}

// Turns a fixed rigid body (or one with no Havok body) keyframed: a handler of the recursive walk.
void bhkWorld::FixedToKeyframedObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& a2) {
    bhkWorldObject* pwo = apCollision->spWorldObject;
    if (pwo) {
        bhkRigidBody* prb = pwo->IsBhkRigidBody();
        if (prb) {
            hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(prb->phkObject);
            if (!phkObject || phkObject->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                prb->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
            }
        }
    }
}

// The block goes back to the thread's Havok heap allocator with the size in m_memSizeAndFlags (zero-extended).
bhkCollisionFilter::~bhkCollisionFilter() {}

void bhkCollisionFilter::operator delete(void* apMemory) {
    hkReferencedObject::deleteObject(apMemory);
}

// Only the base's destructor runs; destroying a bhkMemoryAllocator frees nothing.
bhkMemoryAllocator::~bhkMemoryAllocator() {}

// Allocates nbytes plus 16 from the game's heap and returns the first 16-aligned address past the block's start (1 to
// 16 bytes in), with that distance in the byte before it; blockFree reads it back. There is deliberately no null check,
// as the game does.
void* bhkMemoryAllocator::blockAlloc(int32_t nbytes) {
    char* pBlock = static_cast<char*>(MemoryManager_Singleton.Allocate(static_cast<size_t>(nbytes + 16)));
    uint8_t ucPad = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pBlock) & 15));
    char* p = pBlock + ucPad;
    p[-1] = static_cast<char>(ucPad);
    return p;
}

void bhkMemoryAllocator::blockFree(void* p, int32_t __formal) {
    if (p) {
        MemoryManager_Singleton.Deallocate(static_cast<char*>(p) - static_cast<uint8_t>(*(static_cast<char*>(p) - 1)));
    }
}

int32_t bhkMemoryAllocator::getAllocatedSize(const void* __formal, int32_t nbytes) {
    return nbytes;
}

// Neither callback adds anything to destroy.
bhkOnStartupCallback::~bhkOnStartupCallback() {}

void bhkOnStartupCallback::operator()() {
    bhkTaskletManagerCallback::sbhkThreadState.CreateThreadMemory();
}

bhkOnExitCallback::~bhkOnExitCallback() {}

// Deletes the singleton through its deleting destructor inside the Havok heap context; pListener itself is left as it
// is.
void bhkPhantomOverlapListener::KillInstance() {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    if (bhkPhantomOverlapListener::pListener) {
        delete static_cast<hkpPhantomOverlapListener*>(bhkPhantomOverlapListener::pListener);
    }
    pTLS->eMemContext = eMemContext;
}

NiAVObject* bhkPickData::GetNiAVObject() {
    if (m_rootCollidable) {
        return bhkUtilFunctions::GetNiAVObject(m_rootCollidable);
    }
    else {
        return nullptr;
    }
}

// The rigid body owning the hit's root collidable, or null when there is none or its owner is not an entity.
hkpRigidBody* bhkPickData::GethkRigidBody() {
    hkpCollidable* pCollidable = m_rootCollidable;
    // EXE-INLINED 0x4B59F0
    if (pCollidable && pCollidable->m_broadPhaseHandle.m_type == 1) {
        return static_cast<hkpRigidBody*>(pCollidable->getOwner());
    }
    return nullptr;
}

bhkTaskletManagerCallback::bhkThreadStateElem::bhkThreadStateElem() : ThreadID(0) {}

bhkTaskletManagerCallback::bhkThreadState::bhkThreadState() : iNumElem(0) {
    InitializeCriticalSection(&StateLock);
}

void bhkTaskletManagerCallback::bhkThreadState::CreateThreadMemory() {
    EnterCriticalSection(&StateLock);
    // iNumElem is compared signed.
    if (static_cast<int32_t>(iNumElem) < 16) {
        int32_t& riMemContext = GetThreadMemContext();
        int32_t iOldContext = riMemContext;
        riMemContext = MEM_CONTEXT_HAVOK;
        bhkTaskletManagerCallback::bhkThreadStateElem* pElem = &Elem[iNumElem];
        hkMemorySystem& rSystem = hkMemorySystem::getInstance();
        rSystem.threadInit(pElem->LocalRouter, "bhkTaskletThread", 1);
        hkBaseSystem::initThread(&pElem->LocalRouter);
        void* pStack = pElem->LocalRouter.m_heap->blockAlloc(0x5000);
        pElem->pStackBuffer = pStack;
        pElem->LocalRouter.m_stack.m_start.m_storage = pStack;
        // EXE-INLINED none: hkPadSpu<char*>::operator=
        pElem->LocalRouter.m_stack.m_next.m_storage = static_cast<char*>(pStack);
        pElem->LocalRouter.m_stack.m_end.m_storage = static_cast<char*>(pStack) + 0x5000;
        pElem->ThreadID = GetCurrentThreadId();
        ++iNumElem;
        riMemContext = iOldContext;
    }
    LeaveCriticalSection(&StateLock);
}

void bhkTaskletManagerCallback::bhkThreadState::Destruct(bhkThreadState* apThis) {
    rt::eh_vector_destructor_iterator(
        apThis->Elem, sizeof(bhkThreadStateElem), 16,
        &fnvd::destructElement<bhkTaskletManagerCallback::bhkThreadStateElem>
    );
    DeleteCriticalSection(&apThis->StateLock);
}

hkMemoryAllocator::~hkMemoryAllocator() {}

void hkpBroadPhaseCastCollector::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

hkpCollidableCollidableFilter::~hkpCollidableCollidableFilter() {}

void hkpCollidableCollidableFilter::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

hkpRayCollidableFilter::~hkpRayCollidableFilter() {}

void hkpRayCollidableFilter::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

hkpRayShapeCollectionFilter::~hkpRayShapeCollectionFilter() {}

void hkpRayShapeCollectionFilter::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

hkpShapeCollectionFilter::~hkpShapeCollectionFilter() {}

void hkpShapeCollectionFilter::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

const void* getVtablehkpMotorAction() {
    return hkGetVtableOf<hkpMotorAction>();
}

void finishLoadedObjecthkpMotorAction(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!p) {
        return;
    }
    ::new (p) hkpMotorAction(aFlag);
}

void cleanupLoadedObjecthkpMotorAction(void* p) {
    static_cast<hkpMotorAction*>(p)->~hkpMotorAction();
}

// As above, for an angular dashpot action.
void finishLoadedObjecthkpAngularDashpotAction(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!p) {
        return;
    }
    ::new (p) hkpAngularDashpotAction(aFlag);
}

const void* getVtablehkpDashpotAction() {
    return hkGetVtableOf<hkpDashpotAction>();
}

// As above, for a dashpot action.
void finishLoadedObjecthkpDashpotAction(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!p) {
        return;
    }
    ::new (p) hkpDashpotAction(aFlag);
}

void cleanupLoadedObjecthkpDashpotAction(void* p) {
    static_cast<hkpDashpotAction*>(p)->~hkpDashpotAction();
}

// The class registry's finish-loaded-object callback for a mouse spring action: the null check, then the action's
// finish constructor.
void finishLoadedObjecthkpMouseSpringAction(void* apMemory, int32_t aiFinishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = aiFinishing;
    if (!apMemory) {
        return;
    }
    ::new (apMemory) hkpMouseSpringAction(aFlag);
}

const void* getVtablehkpMouseSpringAction() {
    return hkGetVtableOf<hkpMouseSpringAction>();
}

const void* getVtablehkpReorientAction() {
    return hkGetVtableOf<hkpReorientAction>();
}

void finishLoadedObjecthkpReorientAction(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!p) {
        return;
    }
    ::new (p) hkpReorientAction(aFlag);
}

void cleanupLoadedObjecthkpReorientAction(void* p) {
    static_cast<hkpReorientAction*>(p)->~hkpReorientAction();
}

const void* getVtablehkpSpringAction() {
    return hkGetVtableOf<hkpSpringAction>();
}

// The class registry's finish-loaded-object callback for a spring action: the null check, then the action's finish
// constructor.
void finishLoadedObjecthkpSpringAction(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!p) {
        return;
    }
    ::new (p) hkpSpringAction(aFlag);
}

void cleanupLoadedObjecthkpSpringAction(void* p) {
    static_cast<hkpSpringAction*>(p)->~hkpSpringAction();
}

// The type info's vtable getter for hkpConstrainedSystemFilter: returns the class's primary vtable.
const void* getVtablehkpConstrainedSystemFilter() {
    return hkGetVtableOf<hkpConstrainedSystemFilter>();
}

// The class registry's finish-loaded-object callback for a constrained-system collision filter: the null check, then
// the filter's finish constructor.
void finishLoadedObjecthkpConstrainedSystemFilter(void* p, int32_t finishing) {
    hkpConstrainedSystemFilter* apFilter = static_cast<hkpConstrainedSystemFilter*>(p);
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apFilter) {
        return;
    }
    ::new (apFilter) hkpConstrainedSystemFilter(aFlag);
}

const hkClass& hkpConstrainedSystemFilter::staticClass() {
    return hkpConstrainedSystemFilterClass;
}

// The class registry's finish-loaded-object callback for a disable-entity collision filter: the null check, then the
// filter's finish constructor.
void finishLoadedObjecthkpDisableEntityCollisionFilter(void* p, int32_t finishing) {
    hkpDisableEntityCollisionFilter* apFilter = static_cast<hkpDisableEntityCollisionFilter*>(p);
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apFilter) {
        return;
    }
    ::new (apFilter) hkpDisableEntityCollisionFilter(aFlag);
}

// The class registry's finish-loaded-object callback for a group collision filter: the null check, then the filter's
// finish constructor.
void finishLoadedObjecthkpGroupCollisionFilter(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpGroupCollisionFilter(kFlag);
    }
}

const void* getVtablehkpGroupCollisionFilter() {
    return hkGetVtableOf<hkpGroupCollisionFilter>();
}

const void* getVtablehkpPhysicsSystemWithContacts() {
    return hkGetVtableOf<hkpPhysicsSystemWithContacts>();
}

// The class registry's finish-loaded-object callback for a physics system with contacts: the null check, then the
// system's finish constructor.
void finishLoadedObjecthkpPhysicsSystemWithContacts(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!p) {
        return;
    }
    ::new (p) hkpPhysicsSystemWithContacts(aFlag);
}

// The type info's finish-loaded-object callback for hkpSerializedDisplayMarker: its finish constructor.
void finishLoadedObjecthkpSerializedDisplayMarker(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpSerializedDisplayMarker(kFlag);
    }
}

const void* getVtablehkpSerializedDisplayMarker() {
    return hkGetVtableOf<hkpSerializedDisplayMarker>();
}

// The marker adds nothing to destroy.
hkpSerializedDisplayMarker::~hkpSerializedDisplayMarker() {}

const hkClass& hkpSerializedDisplayMarker::staticClass() {
    return hkpSerializedDisplayMarkerClass;
}

// The type info's finish-loaded-object callback for hkpSerializedDisplayMarkerList: its finish constructor.
void finishLoadedObjecthkpSerializedDisplayMarkerList(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpSerializedDisplayMarkerList(kFlag);
    }
}

const hkClass& hkpSerializedDisplayMarkerList::staticClass() {
    return hkpSerializedDisplayMarkerListClass;
}

// The type info's finish-loaded-object callback for hkpDisplayBindingDataRigidBody: its finish constructor.
void finishLoadedObjecthkpDisplayBindingDataRigidBody(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpDisplayBindingData::RigidBody(kFlag);
    }
}

const void* getVtablehkpDisplayBindingDataRigidBody() {
    return hkGetVtableOf<hkpDisplayBindingData::RigidBody>();
}

// The type info's finish-loaded-object callback for hkpDisplayBindingDataPhysicsSystem: its finish constructor.
void finishLoadedObjecthkpDisplayBindingDataPhysicsSystem(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpDisplayBindingData::PhysicsSystem(kFlag);
    }
}

const void* getVtablehkpDisplayBindingDataPhysicsSystem() {
    return hkGetVtableOf<hkpDisplayBindingData::PhysicsSystem>();
}

// The type info's finish-loaded-object callback for hkpDisplayBindingData: its finish constructor.
void finishLoadedObjecthkpDisplayBindingData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpDisplayBindingData(kFlag);
    }
}

const void* getVtablehkpDisplayBindingData() {
    return hkGetVtableOf<hkpDisplayBindingData>();
}

// The type info's vtable getter for hkpBallGun: returns the class's vtable.
const void* getVtablehkpBallGun() {
    return hkGetVtableOf<hkpBallGun>();
}

void finishLoadedObjecthkpBallGun(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag f;
    f.m_finishing = finishing;
    if (p) {
        ::new (p) hkpBallGun(f);
    }
}

void cleanupLoadedObjecthkpBallGun(void* p) {
    static_cast<hkpBallGun*>(p)->~hkpBallGun();
}

// The type info's vtable getter for hkpFirstPersonGun: returns the class's vtable.
const void* getVtablehkpFirstPersonGun() {
    return hkGetVtableOf<hkpFirstPersonGun>();
}

// The class registry's finish-loaded-object callback for a first person gun: the null check, then the gun's finish
// constructor.
void finishLoadedObjecthkpFirstPersonGun(void* p, int32_t finishing) {
    hkpFirstPersonGun* apGun = static_cast<hkpFirstPersonGun*>(p);
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apGun) {
        return;
    }
    ::new (apGun) hkpFirstPersonGun(aFlag);
}

void cleanupLoadedObjecthkpFirstPersonGun(void* p) {
    static_cast<hkpFirstPersonGun*>(p)->~hkpFirstPersonGun();
}

// The type info's vtable getter for hkpGravityGun: runs the finish constructor, flag clear, on a 16-aligned local that
// is never destroyed, and returns the vtable pointer the constructor stored.
const void* getVtablehkpGravityGun() {
    return hkGetVtableOf<hkpGravityGun>();
}

// The class registry's finish-loaded-object callback for a gravity gun: the null check, then the gun's finish
// constructor.
void finishLoadedObjecthkpGravityGun(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (p) {
        ::new (p) hkpGravityGun(aFlag);
    }
}

void cleanupLoadedObjecthkpGravityGun(void* p) {
    static_cast<hkpGravityGun*>(p)->~hkpGravityGun();
}

// Runs hkpMountedBallGun's finish constructor (flag 0) on a 16-aligned stack object and returns the object's first
// word, the vptr that constructor stores last.
const void* getVtablehkpMountedBallGun() {
    return hkGetVtableOf<hkpMountedBallGun>();
}

// The class registry's finish-loaded-object callback for a mounted ball gun: the null check, then the gun's finish
// constructor.
void finishLoadedObjecthkpMountedBallGun(void* p, int32_t finishing) {
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (p) {
        ::new (p) hkpMountedBallGun(aFlag);
    }
}

void cleanupLoadedObjecthkpMountedBallGun(void* p) {
    static_cast<hkpMountedBallGun*>(p)->~hkpMountedBallGun();
}

const void* getVtablehkpProjectileGun() {
    return hkGetVtableOf<hkpProjectileGun>();
}

// The class registry's finish-loaded-object callback for a projectile gun: the null check, then the gun's finish
// constructor.
void finishLoadedObjecthkpProjectileGun(void* p, int32_t finishing) {
    hkpProjectileGun* apGun = static_cast<hkpProjectileGun*>(p);
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apGun) {
        return;
    }
    ::new (apGun) hkpProjectileGun(aFlag);
}

void cleanupLoadedObjecthkpProjectileGun(void* p) {
    static_cast<hkpProjectileGun*>(p)->~hkpProjectileGun();
}

ahkpWorld::~ahkpWorld() {}

void* ahkpWorld::getUserData() {
    return m_userData;
}
