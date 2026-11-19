#include "BSHavok/bhkCharacterController.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/ahkpCharacterProxy.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCachingShapePhantom.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCharControllerShape.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "BSHavok/bhkCharacterState.h"
#include "BSHavok/bhkCharacterStateClimbing.h"
#include "BSHavok/bhkCharacterStateFlying.h"
#include "BSHavok/bhkCharacterStateInAir.h"
#include "BSHavok/bhkCharacterStateJumping.h"
#include "BSHavok/bhkCharacterStateOnGround.h"
#include "BSHavok/bhkCharacterStateProjectile.h"
#include "BSHavok/bhkCharacterStateSwimming.h"
#include "BSHavok/bhkCollisionBox.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkListShape.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideCapsuleUtil.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Havok/Source/Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyListener.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpSurfaceConstraintInfo.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterContext.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpSurfaceInfo.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/ehvec.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <x86intrin.h>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert
// The bodies here name the game-to-Havok scale unqualified.
using bhkConvert::fBS2HkScaleSC;

static hkQuadReal endAdj{};

// processConstraintsCallback's two guarded statics, the ray's offsets above and below the contact, and their shared
// guard: bit 0 for startAdj, bit 1 for endAdj.
static hkQuadReal startAdj{};
static int32_t startAdj_guard;

// The game-to-Havok scale times 32, computed at start-up.
static float fBS2HkScaleSC32 = fBS2HkScaleSC * 32.0;
SettingT<INISettingCollection> gs_fCharControllerWarpDistSqr_HAVOK("fCharControllerWarpDistSqr:HAVOK", 6000000.0f);
NiPointer<bhkShape> bhkCharacterController::spDefaultConvexShape;

// The triangle collection apShape is, or the one a MOPP shape wraps (the SDK's getShapeCollection(), the child read
// as the collection); null for any other shape.
const hkpShapeCollection* GetTriCollectionFromShape(const hkpShape* apShape) {
    if (!apShape) {
        return nullptr;
    }
    const hkpShape* pShape = apShape;
    uint32_t eType = apShape->m_type.m_storage;
    if (eType == HK_SHAPE_MOPP) {
        pShape = static_cast<const hkpMoppBvTreeShape*>(apShape)->m_child.m_childShape;
        eType = pShape->m_type.m_storage;
    }
    if (eType == HK_SHAPE_TRIANGLE_COLLECTION || eType == HK_SHAPE_USER0 || eType == HK_SHAPE_USER1) {
        return static_cast<const hkpShapeCollection*>(pShape);
    }
    return nullptr;
}

bhkShape* MakeConvex(const NiPoint3& arTop, const NiPoint3& arBottom, float afRadius, float afStepHeight) {
    using a2_t = hkArray<hkVector4, hkContainerHeapAllocator>;
    __m128 v27;
    hkVector4 v28;
    TLSData* v4 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v4->eMemContext;
    v4->eMemContext = MEM_CONTEXT_HAVOK;
    a2_t a2;
    float v24 = fBS2HkScaleSC * afRadius;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    hkArrayUtil::_reserve(*m_heap, &a2, 18, sizeof(hkVector4));
    v27[0] = arBottom.x * fBS2HkScaleSC;
    v27[1] = arBottom.y * fBS2HkScaleSC;
    v27[2] = fBS2HkScaleSC * arBottom.z;
    v27[3] = 0.0f;
    v27[2] = v27[2] - v24;
    hkMemoryAllocator* v6 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_size = a2.m_size;
    if (a2.m_size == (a2.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v6, &a2, sizeof(hkVector4));
        m_size = a2.m_size;
    }
    double v8 = afStepHeight + 0.10000000149011612 + v27[2];
    a2.m_data[m_size].m_quad = v27;
    ++a2.m_size;
    uint32_t v9 = 0;
    v27[2] = v8;
    do {
        double v10 = fCircleX[v9];
        v28.m_quad = v27;
        v28.m_quad[0] = v10 * v24 + v27[0];
        v28.m_quad[1] = v24 * fCircleY[v9] + v27[1];
        hkMemoryAllocator* v11 = (&hkMemoryRouter::getInstance())->m_heap;
        int32_t v12 = a2.m_size;
        if (a2.m_size == (a2.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*v11, &a2, sizeof(hkVector4));
            v12 = a2.m_size;
        }
        ++v9;
        a2.m_data[v12].m_quad = v28.m_quad;
        ++a2.m_size;
    }
    while (v9 < 8);
    uint32_t v13 = 0;
    v27[0] = arTop.x * fBS2HkScaleSC;
    v27[1] = arTop.y * fBS2HkScaleSC;
    v27[2] = fBS2HkScaleSC * arTop.z;
    v27[3] = 0.0f;
    double v14 = v24;
    double v26 = 0.5 * v24;
    v27[2] = v26 + v27[2];
    while (true) {
        double v15 = fCircleX[v13] * v14;
        v28.m_quad = v27;
        v28.m_quad[0] = v15;
        v28.m_quad[1] = v14 * fCircleY[v13];
        hkMemoryAllocator* v16 = (&hkMemoryRouter::getInstance())->m_heap;
        int32_t v17 = a2.m_size;
        if (a2.m_size == (a2.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*v16, &a2, sizeof(hkVector4));
            v17 = a2.m_size;
        }
        ++v13;
        a2.m_data[v17].m_quad = v28.m_quad;
        ++a2.m_size;
        if (v13 >= 8) {
            break;
        }
        v14 = v24;
    }
    v27[2] = v27[2] + v26;
    hkMemoryAllocator* v18 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v19 = a2.m_size;
    if (a2.m_size == (a2.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v18, &a2, sizeof(hkVector4));
        v19 = a2.m_size;
    }
    a2.m_data[v19].m_quad = v27;
    ++a2.m_size;
    bhkCharControllerShape* v20 = bhkCharControllerShape::CreateFromHull(a2);
    v4->eMemContext = eMemContext;
    return v20;
}

// The top of a stilt capsule: the capsule's first end point raised by its radius, placed by arTransf into
// arCapsuleTopOut.
void ExtractStiltCapsuleInfo(
    const hkpCapsuleShape& arCapShape,
    const hkTransform& arTransf,
    hkVector4& arCapsuleTopOut
) {
    hkVector4 temppt;
    temppt.m_quad = arCapShape.m_vertexA.m_quad;
    temppt(2) = arCapShape.m_radius + temppt(2);
    arCapsuleTopOut.setTransformedPos(arTransf, temppt);
}

// The surface info is set up by hkpSurfaceInfo's constructor; the shapes array, the bumped pointers and the damage
// impact list start empty through their own constructors. The manifold reserves five points.
bhkCharacterController::bhkCharacterController(bhkCharacterControllerCinfo& arInfo)
  : bhkCharacterProxy(), hkpCharacterContext(arInfo.spManager, arInfo.eStartState),
    bhkCharacterListener(
        hkVector4(0.0f, 0.0f, 1.0f, 0.0f),
        static_cast<float>(bhkCharacterController::fMaxSlopeDefault * 0.01745329238474369),
        arInfo.fScale * arInfo.fStepHeight
    ),
    hkpSurfaceInfo(), eWantState(HK_CHARACTER_ON_GROUND), fVelocityTime(0.0f), fRotMod(0.0f), fRotModTime(0.0f),
    fCalculatePitchTimer(0.0f), fAcrobatics(0.0f), fCenter(0.0f), fFallStartHeight(0.0f), fFallTime(0.0f),
    fGravity(1.0f), fPitchAngle(0.0f), fRollAngle(0.0f), fPitchMult(1.0f), spShapePhantom(nullptr),
    spCollisionBox(nullptr), eShapeType(bhkCharacterController::ST_NUMTYPES),
    eSizedShapeType(bhkCharacterController::ST_NUMTYPES), fRadius(2.0f), fHeight(10.0f), fDestRadius(2.0f),
    fLODDistance(-1.0f), eSize(bhkCharacterController::CS_NORMAL), iPriority(0), iSupportCount(0),
    spSupportBody(nullptr), bFakeSupport(false), fBumpedForce(0.0f) {
    UpVec.set(0.0f, 0.0f, 1.0f, 0.0f);
    ForwardVec.set(1.0f, 0.0f, 0.0f, 0.0f);
    VelocityMod.m_quad = hkVector4Zero.m_quad;
    RotCenter.m_quad = hkVector4Zero.m_quad;
    FakeSupportStart.m_quad = hkVector4Zero.m_quad;
    LastManifold._reserveExactly(hkMemoryRouter::getInstance().heap(), 5);
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eOldContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    iFlags = 0;
    hkSupportNorm.m_quad = _mm_setzero_ps();
    LastManifold.m_size = 0;
    bhkCharacterController::Init(arInfo);
    pTLS->eMemContext = eOldContext;
}

// Builds the controller's creation info from the proxy when it has none yet, and answers in arCreated whether it made
// one. It is allocated from the Havok heap (the class's QCinfoSize) inside the Havok heap context, and filled from the
// proxy when the proxy already has its Havok object.
hkSerializableCinfo& bhkCharacterController::CreateCinfo(bool& arCreated) {
    AutoMemContext memContext(MEM_CONTEXT_HAVOK);
    const bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        const int32_t iCinfoSize = static_cast<int32_t>(sizeof(bhkCharacterControllerCinfo));
        bhkCharacterControllerCinfo* pCinfo = static_cast<bhkCharacterControllerCinfo*>(heap.blockAlloc(iCinfoSize));
        if (pCinfo != nullptr) {
            pCinfo = ::new (pCinfo) bhkCharacterControllerCinfo(true);
        }
        const bool bHasObject = phkObject != nullptr;
        pInfo = pCinfo;
        if (bHasObject) {
            bhkCharacterProxy::GetCinfo(pCinfo);
        }
    }
    return *pInfo;
}

// Builds the controller from its creation info: the step height, the shapes, a caching shape phantom when there is none
// yet (added to the world with its debug colour, and the debug geometry when the world shows it), the collision box,
// the state and tuning values, then the proxy itself, the pitch/roll flags and this controller as the proxy's listener.
// All of it runs under the Havok heap context.
void bhkCharacterController::Init(bhkCharacterControllerCinfo& rInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    float fStep = rInfo.fScale * rInfo.fStepHeight;
    fStepHeight = fStep;
    InitShapes(rInfo);
    if (!spShapePhantom) {
        hkShapePhantomCinfo kCinfo;
        kCinfo.m_collisionFilterInfo = rInfo.iColFilter.iFilter;
        bhkShape* pShape = GetShape();
        kCinfo.m_shape = pShape ? static_cast<hkpShape*>(pShape->phkObject) : nullptr;
        void* pMemory = NiMemObject::operator new(sizeof(bhkCachingShapePhantom));
        bhkCachingShapePhantom* pPhantom = pMemory ? ::new (pMemory) bhkCachingShapePhantom(&kCinfo) : nullptr;
        // EXE-INLINED none: NiPointer<bhkCachingShapePhantom>::operator=
        bhkCachingShapePhantom* pOld = spShapePhantom;
        if (pOld != pPhantom) {
            if (pOld) {
                pOld->DecRefCount();
            }
            spShapePhantom = pPhantom;
            if (pPhantom) {
                pPhantom->IncRefCount();
            }
        }
    }
    if (rInfo.pWorld) {
        if (spShapePhantom) {
            bhkWorldObject* pObject = spShapePhantom;
            pObject->Add(rInfo.pWorld);
            bhkWorld* pWorld = rInfo.pWorld;
            bhkCachingShapePhantom* pPhantom = spShapePhantom;
            pWorld->SetDebugObjectColor(pPhantom, hkColor::rgbFromFloats(1.0f, 1.0f, 0.0f, 0.75f));
            if (rInfo.pWorld->bDebugDisplay) {
                BuildDisplayGeometry(nullptr);
            }
        }
        if (spCollisionBox) {
            spCollisionBox->Add(rInfo.pWorld);
        }
    }
    bhkCachingShapePhantom* pPhantom = spShapePhantom;
    eWantState = rInfo.eStartState;
    fAcrobatics = rInfo.fAcrobatics;
    fGravity = rInfo.fGravity;
    iPriority = rInfo.iPriority;
    // EXE-INLINED 0x4AE750
    rInfo.m_shapePhantom = pPhantom ? static_cast<hkpShapePhantom*>(pPhantom->phkObject) : nullptr;
    bhkCharacterProxy::Init(rInfo);
    static_cast<hkpCharacterProxy*>(phkObject)->m_refreshManifoldInCheckSupport = true;
    bhkCharacterListener* pListener = this;
    // EXE-INLINED none: bhkCharacterListener::SetFlag
    if (rInfo.bCanPitch) {
        pListener->iFlags |= CAN_PITCH;
    }
    else {
        pListener->iFlags &= ~CAN_PITCH;
    }
    // EXE-INLINED none: bhkCharacterListener::SetFlag
    if (rInfo.bCanRoll) {
        pListener->iFlags |= CAN_ROLL;
    }
    else {
        pListener->iFlags &= ~CAN_ROLL;
    }
    hkpCharacterProxy* pProxy = static_cast<hkpCharacterProxy*>(phkObject);
    if (pProxy) {
        UnMarkWrite();
        pProxy->addCharacterProxyListener(pListener);
        UnMarkWrite();
    }
}

// Moves the character by one frame's request. A flying character is moved by the displacement directly, its state
// updated, and its velocity set to the displacement per second in Havok units. Otherwise (with a world and a time step
// above 1e-4) the hit flags, hurtful hits and bumps are cleared, the size eased, the rotation applied, the wanted
// direction set from the displacement, the forward vector turned; then a projectile moves as one, and anything else
// checks its support (unless it is standing still on the ground), updates its state and integrates the proxy unless
// standing still with the same contact points as last frame, rides an elevator, and keeps the first five contact
// points; the pitch and roll, the velocity modifier's and the pitch timer's countdowns end the move. Returns MOVE_HIT
// when a HITFLAGS bit is set at the end of a full move, else MOVE_OK. The monitor stream's buffer is raw memory written
// untyped, hence the casts in the timer blocks.
MOVE_RESULTS bhkCharacterController::Move(MoveData& arData) {
    MOVE_RESULTS eResult = MOVE_OK;
    if (hkpCharacterContext::getState() == HK_CHARACTER_FLYING) {
        HK_TIMER_BEGIN("update character state", nullptr);
        hkVector4 kHkPos;
        GetPosition_hk(kHkPos);
        alignas(16) NiPoint3 kNewPos;
        HK2NI(kNewPos, kHkPos);
        kNewPos.x = arData.Displacement.x + kNewPos.x;
        kNewPos.y = arData.Displacement.y + kNewPos.y;
        kNewPos.z = arData.Displacement.z + kNewPos.z;
        SetPosition(kNewPos);
        // The game's states are all bhkCharacterStates, so the state is cast to call its Update.
        hkpCharacterState* pState = hkpCharacterContext::m_stateManager->getState(hkpCharacterContext::m_currentState);
        static_cast<bhkCharacterState*>(pState)->Update(*this);
        // The velocity is zero unless 0 < fTime (NaN gives zero).
        const NiPoint3* pVelocity;
        if (0.0f < arData.fTime) {
            const float fInvTime = 1.0f / arData.fTime;
            kNewPos.x = fInvTime * arData.Displacement.x;
            kNewPos.y = arData.Displacement.y * fInvTime;
            kNewPos.z = fInvTime * arData.Displacement.z;
            pVelocity = &kNewPos;
        }
        else {
            pVelocity = &NiPoint3_ZERO;
        }
        const float fScale = fBS2HkScaleSC;
        OutVelocity.m_quad[0] = pVelocity->x * fScale;
        OutVelocity.m_quad[1] = pVelocity->y * fScale;
        OutVelocity.m_quad[2] = fScale * pVelocity->z;
        OutVelocity.m_quad[3] = 0.0f;
        HK_TIMER_END();
        return MOVE_OK;
    }

    iFlags &= ~HITFLAGS;
    ClearHurtfulHits();
    if (spBumpedBody.m_pntr) {
        spBumpedBody.m_pntr->removeReference();
    }
    spBumpedBody.m_pntr = nullptr;
    fBumpedForce = 0.0f;
    if (spBumpedCharProxy.m_pntr) {
        spBumpedCharProxy.m_pntr->removeReference();
    }
    spBumpedCharProxy.m_pntr = nullptr;
    fSpeedPct = arData.fCurrMoveSpeed / fGravFrictionDampSpeed;
    const float fDeltaTime = arData.fTime;
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    // A NaN time step goes on.
    if (arData.fTime <= 9.999999747378752e-05 || !pWorld) {
        return MOVE_OK;
    }
    if ((iFlags >> 22) & 1) {
        if (GetWorld()->iEntityBatchCount != 0) {
            return MOVE_OK;
        }
        iFlags &= ~WAIT_FOR_BATCH_ADD;
    }
    const bool bStill = BSUtilities::ComparePoints(&arData.Displacement, &NiPoint3_ZERO, 0.05f);
    const uint32_t uiMoveFlags = iFlags;
    const uint8_t bQuickSimulate = (uiMoveFlags >> 20) & 1;
    const uint8_t bProjectile = (uiMoveFlags >> 19) & 1;
    uint8_t bNoMove = bStill;
    if (!bStill) {
        ClearPush();
        fRotMod = 0.0f;
        fRotModTime = 0.0f;
    }
    bNoMove &= VelocityMod.equals3(hkVector4Zero, 0.001f);
    if (bNoMove) {
        iFlags &= ~NO_FRICTION;
    }
    else {
        iFlags |= NO_FRICTION;
    }
    HK_TIMER_BEGIN("Character movement", nullptr);
    ChangeSize(fDeltaTime);
    // The support material each move starts from (BHK_MATERIAL_MAX: no material).
    eSupportMaterial = BHK_MATERIAL_MAX;
    eSupportLayer = L_UNIDENTIFIED;
    const float fRotation = ApplyRotationModifier(arData.Angle.z, arData.fTime);
    arData.Angle.z = fRotation;
    hkQuaternion kRotation;
    kRotation.setAxisAngle(UpVec, -fRotation);
    StepInfo.m_deltaTime = arData.fTime;
    StepInfo.m_invDeltaTime = 1.0f / arData.fTime;
    NiPoint3* pDisplacement = &arData.Displacement;
    alignas(16) NiPoint3 kVelocity = arData.Displacement;
    if (!QIsSwimming() && !((iFlags >> 11) & 1)) {
        kVelocity.z = 0.0f;
    }
    const float fInvDeltaTime = StepInfo.m_invDeltaTime;
    kVelocity.x = kVelocity.x * fInvDeltaTime;
    kVelocity.y = fInvDeltaTime * kVelocity.y;
    kVelocity.z = fInvDeltaTime * kVelocity.z;
    NI2HK(Direction, kVelocity);
    // A NaN pitch turns the direction too.
    if (GetUseZ() && 0.0f != arData.Angle.x) {
        hkVector4 kAxis;
        _mm_store_ps(&reinterpret_cast<float*>(&kAxis.m_quad)[0], _mm_setr_ps(-1.0f, 0.0f, 0.0f, 0.0f));
        hkQuaternion kPitch;
        kPitch.setAxisAngle(kAxis, arData.Angle.x);
        Direction.setRotatedDir(kPitch, Direction);
    }
    RotCenter.m_quad[2] = fCenter;
    const uint32_t uiPitchFlags = iFlags;
    hkVector4 kForward;
    _mm_store_ps(&reinterpret_cast<float*>(&kForward.m_quad)[0], _mm_setr_ps(0.0f, -1.0f, 0.0f, 0.0f));
    hkVector4 kDown;
    kDown.setNeg4(UpVec);
    if (((uiPitchFlags >> 25) & 1) || ((uiPitchFlags >> 26) & 1) || (iFlags & QUADRUPED)) {
        ApplyPitchAndRoll(kRotation);
    }
    hkVector4 kNewForward;
    kNewForward.setRotatedDir(kRotation, kForward);
    if (!bNoMove || !kNewForward.equals3(ForwardVec, 0.001f)) {
        fCalculatePitchTimer = fCalculatePitchTimeOut;
    }
    // The Havok proxy, read once here.
    ahkpCharacterProxy* pCharProxy = static_cast<ahkpCharacterProxy*>(phkObject);
    ForwardVec = kNewForward;
    if (bProjectile) {
        OutVelocity = GethkLinearVelocity();
        m_surfaceVelocity = hkVector4Zero;
        HK_TIMER_BEGIN("update character state", nullptr);
        GetCurrentState()->Update(*this);
        HK_TIMER_END();
        SetLinearVelocity(OutVelocity);
        MoveAsProjectile(StepInfo.m_deltaTime);
    }
    else {
        gElevator = nullptr;
        OutVelocity = GethkLinearVelocity();
        if (bNoMove && hkpCharacterContext::getState() != HK_CHARACTER_IN_AIR) {
            OutVelocity.m_quad[2] = 0.0f;
        }
        if (arData.bVisible || hkpCharacterContext::getState() != HK_CHARACTER_ON_GROUND ||
            !(*pDisplacement == NiPoint3_ZERO) || hkSupportNorm.equals4(hkVector4Zero, 0.001f)) {
            iFlags &= ~(SUPPORT2 | CANJUMP);
            iFlags |= CHECKSUPPORT;
            InitFrontBack();
            CheckSupport(kDown, *static_cast<hkpSurfaceInfo*>(this));
            iFlags &= ~CHECKSUPPORT;
            uint8_t bSupport = m_supportedState == hkpSurfaceInfo::SUPPORTED && !bAllSurfacesStopped;
            const uint32_t uiSupportFlags = iFlags;
            if (uiSupportFlags & JUMPING) {
                if (!bSupport && !((uiSupportFlags >> 9) & 1)) {
                    iFlags &= ~JUMPING;
                }
                else if (OutVelocity.m_quad[2] <= 0.0f) {
                    // A NaN z fails the zero test and goes on to the speed, as the game does.
                    iFlags &= ~JUMPING;
                }
                else {
                    const __m128 kVel = _mm_load_ps(&reinterpret_cast<float*>(&OutVelocity.m_quad)[0]);
                    const float fSpeed = OutVelocity.length3();
                    // A NaN speed stops here.
                    if (fSpeed > 0.0f) {
                        const float fInvSpeed = 1.0f / fSpeed;
                        // Tests fMaxSlope <= the normal's z; a NaN fails the test.
                        if (bSupport && ((uiSupportFlags >> 9) & 1) && fMaxSlope <= hkSupportNorm.m_quad[2]) {
                            const __m128 kDir = _mm_mul_ps(
                                _mm_mul_ps(_mm_set1_ps(fInvSpeed), kVel),
                                _mm_load_ps(&reinterpret_cast<float*>(&hkSupportNorm.m_quad)[0])
                            );
                            const __m128 kDot = _mm_add_ps(
                                _mm_shuffle_ps(kDir, kDir, 0xAA), _mm_add_ps(_mm_shuffle_ps(kDir, kDir, 0x55), kDir)
                            );
                            const float fDot = _mm_cvtss_f32(kDot);
                            if (fDot <= 0.05000000074505806) {
                                iFlags &= ~JUMPING;
                            }
                        }
                    }
                }
                bSupport = 0;
            }
            else {
                bSupport |= (uiSupportFlags >> 9) & 1;
            }
            if (bSupport) {
                iFlags |= SUPPORT;
            }
            else {
                iFlags &= ~SUPPORT;
            }
        }
        HK_TIMER_BEGIN("update character state", nullptr);
        hkpCharacterState* pState = hkpCharacterContext::m_stateManager->getState(hkpCharacterContext::m_currentState);
        static_cast<bhkCharacterState*>(pState)->Update(*this);
        HK_TIMER_END();
        if (phkObject) {
            static_cast<hkpCharacterProxy*>(phkObject)->setVelocity(&OutVelocity);
        }
        // Standing still on the ground with the same contact points as last frame (all four lanes within 0.001f, the
        // same root collidables) skips the integration and stops the proxy. In the swimming state (user state 0) a NaN
        // z integrates.
        uint8_t bIntegrate = 1;
        if (bNoMove && hkpCharacterContext::getState() != HK_CHARACTER_IN_AIR &&
            (hkpCharacterContext::getState() != HK_CHARACTER_USER_STATE_0 || 0.0f == OutVelocity.m_quad[2]) &&
            pCharProxy->GetCdPoints()->m_size <= LastManifold.m_size) {
            uint8_t bChanged = 0;
            int32_t iPoint = 0;
            while (iPoint < pCharProxy->GetCdPoints()->m_size) {
                const hkpRootCdPoint* pPoints = pCharProxy->GetCdPoints()->m_data;
                if (!LastManifold.m_data[iPoint].m_contact.m_position.equals4(
                        pPoints[iPoint].m_contact.m_position, 0.001f
                    )) {
                    bChanged = 1;
                }
                else {
                    pPoints = pCharProxy->GetCdPoints()->m_data;
                    const hkpRootCdPoint* pLast = &LastManifold.m_data[iPoint];
                    if (!pLast->m_contact.m_separatingNormal.equals4(
                            pPoints[iPoint].m_contact.m_separatingNormal, 0.001f
                        )) {
                        bChanged = 1;
                    }
                    else if (pLast->m_rootCollidableA != pCharProxy->GetCdPoints()->m_data[iPoint].m_rootCollidableA) {
                        bChanged = 1;
                    }
                    else {
                        pLast = &LastManifold.m_data[iPoint];
                        if (pLast->m_rootCollidableB != pCharProxy->GetCdPoints()->m_data[iPoint].m_rootCollidableB) {
                            bChanged = 1;
                        }
                    }
                }
                ++iPoint;
                if (bChanged) {
                    break;
                }
            }
            bIntegrate = bChanged;
        }
        if (bIntegrate) {
            pCharProxy->m_maxCastIterations = bQuickSimulate ? 1 : 4;
            Integrate(StepInfo);
            pCharProxy->m_maxCastIterations = 4;
        }
        else {
            hkReferencedObject* pProxy = phkObject;
            OutVelocity = hkVector4Zero;
            if (pProxy) {
                static_cast<hkpCharacterProxy*>(pProxy)->setVelocity(&OutVelocity);
            }
        }
        if (gElevator && !(iFlags & JUMPING)) {
            UnMarkWrite();
            hkpCharacterProxy* pProxy = operator*();
            hkVector4 kPosition = pProxy->getPosition();
            const float fRise = gElevator->m_motion.m_linearVelocity.m_quad[2] * StepInfo.m_deltaTime;
            kPosition.m_quad[2] = RotCenter.m_quad[2] + gHeightOfFloor + fRise;
            pProxy->setPosition(kPosition);
            UnMarkWrite();
        }
        eWantState = HK_CHARACTER_MAX_STATE_ID;
        int32_t iCount = pCharProxy->GetCdPoints()->m_size;
        if (iCount > 5) {
            iCount = 5;
        }
        for (int32_t iPoint = 0; iPoint < iCount; ++iPoint) {
            if (iPoint < LastManifold.m_size) {
                const hkpRootCdPoint& rSource = pCharProxy->GetCdPoints()->m_data[iPoint];
                LastManifold.m_data[iPoint].Assign(rSource);
            }
            else {
                LastManifold.pushBack(pCharProxy->GetCdPoints()->m_data[iPoint]);
            }
        }
    }
    const uint32_t uiEndFlags = iFlags;
    if (((uiEndFlags >> 25) & 1) || ((uiEndFlags >> 26) & 1)) {
        CalculatePitchAndRoll();
    }
    if (iFlags & HITFLAGS) {
        eResult = MOVE_HIT;
    }
    HK_TIMER_END();
    // The countdowns run only while 0 < the time left (NaN stops them).
    if (0.0f < fVelocityTime) {
        const float fTimeLeft = fVelocityTime - fDeltaTime;
        fVelocityTime = fTimeLeft;
        if (fTimeLeft <= 0.0f) {
            fVelocityTime = 0.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&VelocityMod.m_quad)[0], _mm_setzero_ps());
        }
    }
    if (0.0f < fCalculatePitchTimer) {
        fCalculatePitchTimer = fCalculatePitchTimer - fDeltaTime;
    }
    iFlags &= ~FOLLOWRAGDOLL;
    return eResult;
}

void bhkCharacterController::UpdateCollisionBox() {
    bhkCollisionBox* v1 = static_cast<bhkCollisionBox*>(spCollisionBox);
    if (v1) {
        v1->Update();
    }
}

// Builds the collision display under apNode, else under the shape phantom's 3D (its first child when it has children):
// a "bhkColDisp" node at kRotCenter in the parent's scale, holding the shape's display geometry in white, and sets
// COLLISION_DEBUG. Nothing when a display node exists already. The work runs in the Havok memory context.
void bhkCharacterController::BuildDisplayGeometry(NiNode* apNode) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if (GetCollisionDebugNode()) {
        pTLSData->eMemContext = eMemContext;
        return;
    }
    NiNode* pParent = apNode;
    if (!pParent) {
        if (!spShapePhantom) {
            pTLSData->eMemContext = eMemContext;
            return;
        }
        NiAVObject* pav = spShapePhantom->GetAVObject(0);
        if (!pav) {
            pTLSData->eMemContext = eMemContext;
            return;
        }
        pParent = pav->IsNode();
        if (!pParent) {
            pTLSData->eMemContext = eMemContext;
            return;
        }
        if (pParent->m_kChildren.m_usSize) {
            NiAVObject* pChild = pParent->GetAtChecked(0);
            if (!pChild) {
                pTLSData->eMemContext = eMemContext;
                return;
            }
            pParent = static_cast<NiNode*>(pChild->DynamicCastMember(&NiNode::ms_RTTI));
        }
        if (!pParent) {
            pTLSData->eMemContext = eMemContext;
            return;
        }
    }
    if (GetShape()) {
        NiNode* pMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
        NiNode* pchar = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pParent->AttachChild(pchar, 1);
        pchar->SetName("bhkColDisp");
        const float fInvScale = 1.0 / pParent->m_kWorld.m_fScale;
        NiPoint3 nip;
        const NiPoint3& rCenter = HK2NI(nip, RotCenter);
        pchar->m_kLocal.m_Translate.x = rCenter.x * fInvScale;
        pchar->m_kLocal.m_Translate.y = rCenter.y * fInvScale;
        pchar->m_kLocal.m_Translate.z = fInvScale * rCenter.z;
        if (0.0f != pParent->m_kWorld.m_fScale) {
            float fScale = 1.0 / pParent->m_kWorld.m_fScale;
            fScale = std::fabs(fScale);
            pchar->m_kLocal.m_fScale = fScale;
        }
        bhkShape* pShape = GetShape();
        pShape->BuildDisplayGeometry(pchar, NiColor_WHITE);
        iFlags |= COLLISION_DEBUG;
    }
    pTLSData->eMemContext = eMemContext;
}

// The listener's constraint callback: for each contact of the manifold (as many as the solver input has constraints) it
// records a hurtful body's hit, looks for an elevator floor under a keyframed body the character may step onto, adjusts
// the contact's surface constraint by the layer it belongs to (frictions, velocities, planes, a flattened wall plane
// added as a new constraint), sets the support, jump and step flags, and sums and normalises the support normal.
void bhkCharacterController::processConstraintsCallback(
    const hkpCharacterProxy* apProxy,
    const hkArray<hkpRootCdPoint, hkContainerHeapAllocator>& arManifold,
    hkpSimplexSolverInput& arInput
) {
    // Switches to Havok's memory context for the call; the old one is restored at the end.
    TLSData* pTLS = GetThreadTLSData();
    const MEM_CONTEXT eOldContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    iFlags &= ~TRY_STEP;
    SupportHeight[3] = 1.1754944e-38f;
    SupportHeight[2] = 1.1754944e-38f;
    SupportHeight[1] = 1.1754944e-38f;
    SupportHeight[0] = 1.1754944e-38f;
    hkSupportNorm.m_quad = _mm_setzero_ps();
    hkVector4 kZero;
    kZero.m_quad = _mm_setzero_ps();
    if (iFlags & CHECKSUPPORT) {
        if (spSupportBody) {
            spSupportBody->removeReference();
        }
        spSupportBody = nullptr;
    }
    float fcheckradSqr = 0.0f;
    iSupportCount = 0;
    int32_t inewConstraintsAdded = 0;
    int32_t inumInvalid = 0;
    bool bnormalSet = false;
    hkSupportNorm.m_quad = UpVec.m_quad;
    bool bcheckspheres = false;
    hkVector4 quadStiltSphereTop1;
    hkVector4 quadStiltSphereTop2;
    // A biped's stilts: the tops of the second and third capsules of a three-capsule list shape, and a third of the
    // third capsule's radius, squared, as the distance at which a contact counts as stuck between them.
    if (!(iFlags & QUADRUPED) && !(iFlags & STUCK_QUAD) && spShapePhantom) {
        hkpShapePhantom* pp = static_cast<hkpShapePhantom*>(spShapePhantom->phkObject);
        if (pp && spShapes[0]) {
            const hkpShape* pshape = static_cast<const hkpShape*>(spShapes[0]->phkObject);
            if (pshape && pshape->m_type.m_storage == HK_SHAPE_LIST) {
                const hkpListShape* plistshape = static_cast<const hkpListShape*>(pshape);
                if (static_cast<const hkpShapeContainer*>(plistshape)->getNumChildShapes() == 3) {
                    const hkTransform& rtransf = pp->m_motionState.m_transform;
                    // EXE-INLINED 0x4D8610
                    const hkpShape* pchild = plistshape->m_childInfo.m_data[1].m_shape;
                    if (pchild && pchild->m_type.m_storage == HK_SHAPE_CAPSULE) {
                        ExtractStiltCapsuleInfo(
                            *static_cast<const hkpCapsuleShape*>(pchild), rtransf, quadStiltSphereTop1
                        );
                        // EXE-INLINED 0x4D8610
                        pchild = plistshape->m_childInfo.m_data[2].m_shape;
                        if (pchild && pchild->m_type.m_storage == HK_SHAPE_CAPSULE) {
                            const hkpCapsuleShape* rcap = static_cast<const hkpCapsuleShape*>(pchild);
                            ExtractStiltCapsuleInfo(*rcap, rtransf, quadStiltSphereTop2);
                            bcheckspheres = true;
                            fcheckradSqr = rcap->m_radius * 0.3330000042915344;
                            fcheckradSqr = fcheckradSqr * fcheckradSqr;
                        }
                    }
                }
            }
        }
    }
    // The contacts the solver input has constraints for, read once: the constraints this callback adds are not
    // visited.
    int32_t iorigConstraints = arManifold.m_size;
    if (arInput.m_numConstraints < iorigConstraints) {
        iorigConstraints = arInput.m_numConstraints;
    }
    bool bpathObstacle = false;
    for (int32_t i = 0; i < iorigConstraints; ++i) {
        hkpSurfaceConstraintInfo& rsurface = arInput.m_constraints[i];
        const hkpRootCdPoint& rpnt = arManifold.m_data[i];
        const hkpCollidable* pcollB = rpnt.m_rootCollidableB;
        hkpRigidBody* pbody = hkpGetRigidBody(pcollB);
        const uint32_t ifilter = pcollB->m_broadPhaseHandle.m_collisionFilterInfo;
        // A hurtful body moving into the character is recorded as a hit, with its velocity less the part along the
        // character's own velocity when that part points the same way.
        if ((iFlags & CHECKSUPPORT) && (iFlags & RECORD_HITS) && IsHurtfulBody(pbody) && pbody) {
            bhkRigidBody* pbhkBody = static_cast<bhkRigidBody*>(pbody->getUserData());
            if (pbhkBody) {
                NiPoint3 objectVelocity;
                // EXE-INLINED 0x560D80
                hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pbhkBody->phkObject);
                const hkVector4* pvelocity = phkBody ? &phkBody->getLinearVelocity() : &hkVector4Zero;
                HK2NI(objectVelocity, *pvelocity);
                const double x = objectVelocity.x;
                const double y = objectVelocity.y;
                const double z = objectVelocity.z;
                float objectSpeed = z * z + (x * x + y * y);
                if (objectSpeed > 0.0f) {
                    NiPoint3 currentVelocity;
                    currentVelocity.x = 0.0f;
                    currentVelocity.y = 0.0f;
                    currentVelocity.z = 0.0f;
                    hkpCharacterProxy* pproxy = static_cast<hkpCharacterProxy*>(phkObject);
                    if (pproxy) {
                        bhkCharacterProxy::GethkWorldRw();
                        HK2NI(currentVelocity, pproxy->getLinearVelocity());
                        bhkCharacterProxy::GethkWorldRw();
                    }
                    const float fval = currentVelocity.x * x + currentVelocity.y * y + currentVelocity.z * z;
                    float fprojX = fval * x;
                    float fprojY = fval * y;
                    float fprojZ = fval * z;
                    const float fInvSpeed = 1.0 / objectSpeed;
                    fprojX = fprojX * fInvSpeed;
                    fprojY = fprojY * fInvSpeed;
                    fprojZ = fInvSpeed * fprojZ;
                    const float fdot = fprojY * y + fprojX * x + fprojZ * z;
                    if (fdot > 0.0f) {
                        objectVelocity.x = x - fprojX;
                        objectVelocity.y = y - fprojY;
                        objectVelocity.z = z - fprojZ;
                        objectSpeed = objectVelocity.x * objectVelocity.x + objectVelocity.y * objectVelocity.y +
                            objectVelocity.z * objectVelocity.z;
                    }
                    if (objectSpeed > 0.0f) {
                        void* pmemory = MemoryManager_Singleton.Allocate(sizeof(DamageImpactData));
                        DamageImpactData* phit = pmemory ? ::new (pmemory) DamageImpactData : nullptr;
                        phit->spBody = pbhkBody;
                        HK2NI(phit->kLocation, rpnt.m_contact.m_position);
                        HK2NI(phit->kNormal, rpnt.m_contact.m_separatingNormal);
                        phit->kVelocity.x = objectVelocity.x;
                        phit->kVelocity.y = objectVelocity.y;
                        phit->kVelocity.z = objectVelocity.z;
                        DamageImpacts.AddHead(phit);
                    }
                }
            }
        }
        // EXE-INLINED 0x43B4D0
        const COL_LAYER elayer = static_cast<COL_LAYER>(ifilter & 0x7F);
        bpathObstacle |= elayer == L_ANIMSTATIC;
        bool bblockWall = (iFlags & CHASEBIP) == 0;
        float fsurfaceVerticalComponent = _mm_cvtss_f32(
            _mm_shuffle_ps(rpnt.m_contact.m_separatingNormal.m_quad, rpnt.m_contact.m_separatingNormal.m_quad, 0xAA)
        );
        // A nearly flat contact on a keyframed body, with no elevator found yet: casts a ray down through the bodies
        // the phantom overlaps, from above the contact, and keeps the highest hit on a keyframed elevator's surface
        // as the floor and that body as the elevator.
        if (pbody && fsurfaceVerticalComponent > 0.9850000143051147 &&
            pbody->m_motion.m_type.m_storage == hkpMotion::MOTION_KEYFRAMED && !gElevator) {
            hkVector4 stepPos;
            GetPosition_hk(stepPos);
            stepPos.m_quad[2] = fStepHeight + stepPos.m_quad[2];
            const float fcontactZ =
                _mm_cvtss_f32(_mm_shuffle_ps(rpnt.m_contact.m_position.m_quad, rpnt.m_contact.m_position.m_quad, 0xAA));
            const float fstepZ = _mm_cvtss_f32(_mm_shuffle_ps(stepPos.m_quad, stepPos.m_quad, 0xAA));
            if (fstepZ >= fcontactZ) {
                hkpClosestRayHitCollector theCollector;
                alignas(16) hkArray<hkpSimpleShapePhantom::CollisionDetail, hkContainerHeapAllocator> theDetails;
                theDetails.m_data = nullptr;
                theDetails.m_size = 0;
                theDetails.m_capacityAndFlags =
                    hkArrayBase<hkpSimpleShapePhantom::CollisionDetail>::DONT_DEALLOCATE_FLAG;
                // The hkpShapePhantom is cast to hkpSimpleShapePhantom and its collision details are read whatever its
                // actual class, as the game does.
                hkpSimpleShapePhantom* pp =
                    static_cast<hkpSimpleShapePhantom*>(static_cast<hkpShapePhantom*>(spShapePhantom->phkObject));
                theDetails = pp->m_collisionDetails;
                // The ray's offsets from the contact, two guarded statics (guard bits 1 and 2), each built with the
                // SDK's inline hkVector4::set.
                int32_t iguard = startAdj_guard;
                if (!(iguard & 1)) {
                    iguard |= 1;
                    startAdj_guard = iguard;
                    hkVector4 kOffset;
                    kOffset.set(0.0f, 0.0f, 36.0f, 0.0f);
                    startAdj = kOffset.m_quad;
                }
                if (!(iguard & 2)) {
                    iguard |= 2;
                    startAdj_guard = iguard;
                    hkVector4 kOffset;
                    kOffset.set(0.0f, 0.0f, 54.0f, 0.0f);
                    endAdj = kOffset.m_quad;
                }
                gHeightOfFloor = -3.4028235e+38f;
                hkVector4 worldStart;
                worldStart.m_quad = _mm_add_ps(rpnt.m_contact.m_position.m_quad, startAdj);
                hkVector4 worldEnd;
                worldEnd.m_quad = _mm_sub_ps(worldStart.m_quad, endAdj);
                for (int32_t j = 0; j < theDetails.m_size; ++j) {
                    hkpCollidable* pcl = theDetails.m_data[j].m_collidable;
                    hkpRigidBody* phrb = hkpGetRigidBody(pcl);
                    if (!phrb || phrb->m_motion.m_type.m_storage != hkpMotion::MOTION_KEYFRAMED) {
                        continue;
                    }
                    hkpShapeRayCastInput theInput;
                    theInput.m_filterInfo = 0;
                    theInput.m_rayShapeCollectionFilter = nullptr;
                    hkpShapeRayCastOutput theOutput;
                    theOutput.m_hitFraction = 1.0f;
                    theOutput.m_extraInfo = -1;
                    theOutput.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
                    theOutput.m_shapeKeyIndex = 0;
                    theInput.m_from.setTransformedInversePos(phrb->m_motion.m_motionState.m_transform, worldStart);
                    theInput.m_to.setTransformedInversePos(phrb->m_motion.m_motionState.m_transform, worldEnd);
                    pcl->m_shape->castRayImpl(theInput, theOutput);
                    if (!(theOutput.m_hitFraction < 1.0f)) {
                        continue;
                    }
                    // EXE-INLINED 0x43B4D0
                    const COL_LAYER iflt =
                        static_cast<COL_LAYER>(phrb->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0x7F);
                    eSupportLayer = iflt;
                    // The key of the deepest shape the ray hit, if any.
                    uint32_t outputShapeKey = HK_INVALID_SHAPE_KEY;
                    for (int32_t k = 0; k < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; ++k) {
                        if (theOutput.m_shapeKeys[k] == HK_INVALID_SHAPE_KEY) {
                            break;
                        }
                        outputShapeKey = theOutput.m_shapeKeys[k];
                    }
                    // The material is the low five bits of the wrapper's.
                    bool bElevatorMaterial;
                    if (outputShapeKey == HK_INVALID_SHAPE_KEY) {
                        const hkpShape* pshape = pcl->m_shape;
                        if (!pshape) {
                            continue;
                        }
                        bhkShape* ps1 = pshape->m_userData;
                        if (!ps1) {
                            continue;
                        }
                        bElevatorMaterial = (ps1->eMaterial & 0x1F) == BHK_MATERIAL_ELEVATOR;
                    }
                    else {
                        if (iflt == L_GROUND) {
                            continue;
                        }
                        bhkShapeCollection* psc = bhkUtilFunctions::FindbhkShapeCollection(pcl);
                        if (!psc) {
                            continue;
                        }
                        bElevatorMaterial = (psc->eMaterial & 0x1F) == BHK_MATERIAL_ELEVATOR;
                    }
                    if (bElevatorMaterial) {
                        const __m128 kStartZ = _mm_shuffle_ps(worldStart.m_quad, worldStart.m_quad, 0xAA);
                        const __m128 kEndZ = _mm_shuffle_ps(worldEnd.m_quad, worldEnd.m_quad, 0xAA);
                        const float newHeight = _mm_cvtss_f32(_mm_add_ss(
                            kStartZ, _mm_mul_ss(_mm_sub_ss(kEndZ, kStartZ), _mm_load_ss(&theOutput.m_hitFraction))
                        ));
                        if (gHeightOfFloor < newHeight) {
                            gHeightOfFloor = newHeight;
                            gElevator = phrb;
                        }
                    }
                }
            }
        }
        hkVector4 tempSupportNormal;
        tempSupportNormal.m_quad = rsurface.m_plane.m_quad;
        if (pbody) {
            switch (elayer) {
                case L_ANIMSTATIC:
                    if (pbody->m_motion.m_type.m_storage == hkpMotion::MOTION_KEYFRAMED ||
                        pbody->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
                        rsurface.m_extraDownStaticFriction = 0.577f;
                        break;
                    }
                    [[fallthrough]];
                case L_PROPS:
                    if (!(iFlags & CHECKSUPPORT)) {
                        fsurfaceVerticalComponent = 1.0f;
                        rsurface.m_dynamicFriction = 0.0f;
                        const hkpMotion::MotionType emotion =
                            static_cast<hkpMotion::MotionType>(pbody->m_motion.m_type.m_storage);
                        tempSupportNormal.set(0.0f, 0.0f, 1.0f, 0.0f);
                        if (bhkRigidBody::IsMotionTypeDynamic(emotion)) {
                            rsurface.m_velocity.m_quad = kZero.m_quad;
                        }
                    }
                    bblockWall = false;
                    break;
                case L_CLUTTER:
                case L_WEAPON:
                case L_PROJECTILE:
                    if (!(iFlags & CHECKSUPPORT)) {
                        const float fmass = pbody->m_motion.getMass();
                        if (bhkCharacterListener::fMoveLimitMass > fmass) {
                            rsurface.m_velocity.m_quad = kZero.m_quad;
                        }
                        fsurfaceVerticalComponent = 1.0f;
                        tempSupportNormal.set(0.0f, 0.0f, 1.0f, 0.0f);
                    }
                    bblockWall = false;
                    break;
                default:
                    // A contact closer to either stilt top than the check radius: the character is stuck between its
                    // stilts.
                    if (bcheckspheres && !(iFlags & STUCK_QUAD)) {
                        const __m128 kPosition = rpnt.m_contact.m_position.m_quad;
                        const __m128 kDiff1 = _mm_sub_ps(kPosition, quadStiltSphereTop1.m_quad);
                        const __m128 kSq1 = _mm_mul_ps(kDiff1, kDiff1);
                        const float fdist1 = _mm_cvtss_f32(_mm_add_ss(
                            _mm_shuffle_ps(kSq1, kSq1, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq1, kSq1, 0x55), kSq1)
                        ));
                        if (fcheckradSqr > fdist1) {
                            iFlags |= STUCK_QUAD;
                        }
                        else {
                            const __m128 kDiff2 = _mm_sub_ps(kPosition, quadStiltSphereTop2.m_quad);
                            const __m128 kSq2 = _mm_mul_ps(kDiff2, kDiff2);
                            const float fdist2 = _mm_cvtss_f32(_mm_add_ss(
                                _mm_shuffle_ps(kSq2, kSq2, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSq2, kSq2, 0x55), kSq2)
                            ));
                            if (fdist2 < fcheckradSqr) {
                                iFlags |= STUCK_QUAD;
                            }
                        }
                    }
                    break;
            }
            if (fsurfaceVerticalComponent >= 0.0f) {
                bool bsupported = true;
                if (!(fMaxSlope > fsurfaceVerticalComponent)) {
                    // Ground the character can stand on.
                    if (!(iFlags & CHECKSUPPORT) && (iFlags & QUADRUPED)) {
                        static_cast<bhkCharacterListener*>(this)->AddSupportHeight2(
                            FrontBack, rpnt.m_contact.m_position
                        );
                    }
                    iFlags |= CANJUMP;
                    if (!(iFlags & NO_FRICTION)) {
                        rsurface.m_extraDownStaticFriction = 5.7700005f;
                    }
                }
                else if (IsStep(rpnt)) {
                    // A step: its plane turned upright.
                    if (!(iFlags & (CHASEBIP | FOLLOWRAGDOLL))) {
                        rsurface.m_dynamicFriction = 0.0f;
                    }
                    tempSupportNormal.m_quad[2] = 1.0f;
                    iFlags |= TRY_STEP | CANJUMP;
                    tempSupportNormal.normalize3();
                    rsurface.m_plane.m_quad = tempSupportNormal.m_quad;
                }
                else {
                    // A wall: too steep to support a blocking character, and its flattened plane added as a new
                    // constraint, at most four times.
                    if ((bblockWall || (iFlags & CHASEBIP)) && fsurfaceVerticalComponent < 0.20000000298023224) {
                        bsupported = false;
                    }
                    if (inewConstraintsAdded < 4 && bblockWall) {
                        hkpSurfaceConstraintInfo* newConstraint = &arInput.m_constraints[arInput.m_numConstraints];
                        hkpSurfaceConstraintInfo::Assign(newConstraint, rsurface);
                        newConstraint->m_velocity.m_quad = kZero.m_quad;
                        newConstraint->m_plane.m_quad[2] = 0.0f;
                        newConstraint->m_plane.normalize3();
                        ++inewConstraintsAdded;
                        ++arInput.m_numConstraints;
                        if (bsupported) {
                            ++inumInvalid;
                        }
                    }
                }
                if (getState() == HK_CHARACTER_ON_GROUND) {
                    rsurface.m_dynamicFriction = fSpeedPct * rsurface.m_dynamicFriction;
                }
                if (bsupported) {
                    iFlags |= SUPPORT2;
                    ++iSupportCount;
                    SetSupportMaterial(arManifold.m_data[i]);
                    if (bnormalSet) {
                        tempSupportNormal.m_quad = _mm_add_ps(tempSupportNormal.m_quad, hkSupportNorm.m_quad);
                    }
                    else {
                        bnormalSet = true;
                    }
                    hkSupportNorm.m_quad = tempSupportNormal.m_quad;
                    if (iFlags & CHECKSUPPORT) {
                        pbody->addReference();
                        if (spSupportBody) {
                            spSupportBody->removeReference();
                        }
                        spSupportBody = pbody;
                    }
                }
            }
        }
        else {
            switch (elayer) {
                case L_TRIGGER:
                case L_CLOUDTRAP:
                case L_ACTORZONE:
                case L_PROJECTILEZONE:
                case L_GASTRAP:
                case L_SPELLEXPLOSION: rsurface.m_velocity.m_quad = kZero.m_quad; break;
                case L_TRAP:
                    if (iFlags & CHECKSUPPORT) {
                        rsurface.m_velocity.m_quad = kZero.m_quad;
                    }
                    break;
                case L_CHARCONTROLLER: {
                    // Another character: no plane at all when it does not collide with characters, else its plane
                    // flattened.
                    const hkpCollidable* pcollPhantom = rpnt.m_rootCollidableB;
                    // EXE-INLINED 0x4B5950
                    hkpPhantom* pphantom =
                        pcollPhantom->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM
                        ? static_cast<hkpPhantom*>(static_cast<hkpWorldObject*>(pcollPhantom->getOwner()))
                        : nullptr;
                    bhkCharacterController* potherController = Getbhk(pphantom);
                    if (potherController->iFlags & NO_CHARACTER_COLLISIONS) {
                        rsurface.m_plane.m_quad = kZero.m_quad;
                        rsurface.m_velocity.m_quad = kZero.m_quad;
                    }
                    else {
                        rsurface.m_plane.m_quad[2] = 0.0f;
                        rsurface.m_plane.normalize3IfNotZero();
                    }
                    break;
                }
                default: break;
            }
        }
    }
    if (bpathObstacle) {
        iFlags |= POSSIBLE_PATH_OBSTACLE;
    }
    else {
        iFlags &= ~POSSIBLE_PATH_OBSTACLE;
    }
    bAllSurfacesStopped = iSupportCount == static_cast<uint32_t>(inumInvalid);
    hkSupportNorm.normalize3IfNotZero();
    pTLS->eMemContext = eOldContext;
}

// Bumping into another character: holds the other proxy as spBumpedCharProxy and, when the other controller (the
// ahkpCharacterProxy's user data, its owning bhkCharacterProxy) ranks no higher and is pushable, pushes it along the
// contact's flattened normal, scaled by this character's horizontal speed. A quadruped is also turned away from the
// contact by how fast this character moves against the contact normal.
void bhkCharacterController::characterInteractionCallback(
    hkpCharacterProxy* apProxy,
    hkpCharacterProxy* apOtherProxy,
    const hkContactPoint& arContact
) {
    bhkCharacterController* pcc = static_cast<bhkCharacterController*>(
        static_cast<bhkCharacterProxy*>(static_cast<ahkpCharacterProxy*>(apOtherProxy)->m_userData)
    );
    apOtherProxy->addReference();
    hkpCharacterProxy* pOldBumped = spBumpedCharProxy.val();
    if (pOldBumped) {
        pOldBumped->removeReference();
    }
    spBumpedCharProxy.m_pntr = apOtherProxy;
    if (!pcc) {
        return;
    }
    const bhkCharacterController::CHARACTER_SIZE eMySize = eSize;
    if (pcc->iPriority > iPriority || (pcc->iFlags & NOTPUSHABLE) != 0) {
        return;
    }
    // The contact's normal, flattened and normalised when its squared length is above zero.
    hkVector4 kNormal;
    kNormal.m_quad = arContact.m_separatingNormal.m_quad;
    kNormal.m_quad[2] = 0.0f;
    if (kNormal.dot3(kNormal) > 0.0f) {
        kNormal.normalize3();
    }
    // This character's horizontal speed, 20 more while it is resizing; the push is a quarter of it, backwards.
    hkVector4 kVelocity;
    kVelocity.m_quad = GethkLinearVelocity().m_quad;
    kVelocity.m_quad[2] = 0.0f;
    const __m128 sqV = _mm_mul_ps(kVelocity.m_quad, kVelocity.m_quad);
    float fSpeed = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqV, sqV, 0xAA), _mm_add_ss(_mm_shuffle_ps(sqV, sqV, 0x55), sqV)))
    );
    if (eMySize == CS_SIZING) {
        fSpeed = fSpeed + 20.0;
    }
    const float fScale = fSpeed * -0.25;
    kNormal.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0), kNormal.m_quad);
    alignas(16) NiPoint3 kPush;
    HK2NI(kPush, kNormal);
    pcc->PushDelta.m_quad = _mm_add_ps(pcc->PushDelta.m_quad, kNormal.m_quad);
    ++pcc->iPushCount;
    pcc->SetVelocityModifier(kPush, 0.01f);
    if ((pcc->iFlags & QUADRUPED) == 0) {
        return;
    }
    // The contact's horizontal offset from the other character, and its projection on that one's flattened
    // forward direction.
    hkVector4 kDiff;
    kDiff.m_quad = arContact.m_position.m_quad;
    kDiff.m_quad = _mm_sub_ps(kDiff.m_quad, pcc->GethkPosition().m_quad);
    kDiff.m_quad[2] = 0.0f;
    hkVector4 kProj;
    kProj.m_quad = pcc->ForwardVec.m_quad;
    kProj.m_quad[2] = 0.0f;
    kProj.normalize3();
    const float fAlong = kProj.dot3(kDiff);
    kProj.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fAlong), _mm_set_ss(fAlong), 0), kProj.m_quad);
    const __m128 sqP = _mm_mul_ps(kProj.m_quad, kProj.m_quad);
    const float fProjLen = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sqP, sqP, 0xAA), _mm_add_ss(_mm_shuffle_ps(sqP, sqP, 0x55), sqP)))
    );
    const float fRatio = fProjLen / (pcc->fHeight * 0.5) - 0.05000000074505806;
    // The contact's flattened normal, normalised.
    hkVector4 kSepNormal;
    kSepNormal.m_quad = arContact.m_separatingNormal.m_quad;
    kSepNormal.m_quad[2] = 0.0f;
    kSepNormal.normalize3();
    // How fast this character moves against the contact normal, horizontally (0 when it stands still).
    kVelocity.m_quad = GethkLinearVelocity().m_quad;
    kVelocity.m_quad[2] = 0.0f;
    float fApproach = 0.0f;
    const __m128 sqW = _mm_mul_ps(kVelocity.m_quad, kVelocity.m_quad);
    const float fLen2 =
        _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sqW, sqW, 0xAA), _mm_add_ss(_mm_shuffle_ps(sqW, sqW, 0x55), sqW)));
    if (fLen2 != 0.0f) {
        const float fLen = std::sqrt(fLen2);
        const float fInv = 1.0 / fLen;
        const __m128 t = _mm_mul_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fInv), _mm_set_ss(fInv), 0), kVelocity.m_quad), kSepNormal.m_quad
        );
        fApproach = _mm_cvtss_f32(_mm_sub_ss(
            _mm_setzero_ps(), _mm_add_ss(_mm_shuffle_ps(t, t, 0xAA), _mm_add_ss(_mm_shuffle_ps(t, t, 0x55), t))
        ));
    }
    const float fTurn = fApproach * fRatio;
    const float fClamped = BSMAX(0.0f, BSMIN(1.0f, fTurn));
    if (fClamped > 0.0f) {
        // The turn's sign: the z of the offset crossed with the projection.
        const __m128 cross = _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(kProj.m_quad, kProj.m_quad, 0xD2), _mm_shuffle_ps(kDiff.m_quad, kDiff.m_quad, 0xC9)
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(kProj.m_quad, kProj.m_quad, 0xC9), _mm_shuffle_ps(kDiff.m_quad, kDiff.m_quad, 0xD2)
            )
        );
        float fTurnSpeed = fClamped;
        if (cross[2] > 0.0f) {
            fTurnSpeed = fTurnSpeed * -1.0;
        }
        const float fRotation = fTurnSpeed + fTurnSpeed;
        pcc->SetRotationModifier(fRotation, 0.2f);
    }
}

void bhkCharacterController::objectInteractionCallback(
    hkpCharacterProxy* apProxy,
    const hkpCharacterObjectInteractionEvent& arInput,
    hkpCharacterObjectInteractionResult& arOutput
) {
    uint32_t v7;
    double v8;
    double v9;
    float v22;
    float v25;
    float Mass = arInput.m_body->m_motion.getMass();
    __m128 v5 = _mm_mul_ps(arOutput.m_objectImpulse.m_quad, arOutput.m_objectImpulse.m_quad);
    float v6 = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v5, v5, 170)[0] + (_mm_shuffle_ps(v5, v5, 85)[0] + v5[0]))))
    );
    float v23 = v6;
    if (!(Mass > 0.0f) || !(bhkCharacterListener::fMoveLimitMass > static_cast<double>(Mass))) {
        arOutput.m_objectImpulse.m_quad = _mm_setzero_ps();
        goto LABEL_19;
    }
    v7 = arInput.m_body->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo &
        hkpTypedBroadPhaseHandle::OFFSET_INVALID;
    if (v7 == 6) {
        v9 = 0.0f;
        v8 = Mass;
        goto LABEL_11;
    }
    if (v7 == 10) {
        v8 = Mass;
        if (!(bhkCharacterListener::fMoveLimitMass * 0.5 < Mass)) {
            v9 = 3.0f;
            goto LABEL_11;
        }
        goto LABEL_8;
    }
    v8 = Mass;
    if (v7 == 14) {
    LABEL_8:;
        v9 = 0.1f;
        goto LABEL_11;
    }
    v9 = 2.0f;
LABEL_11:;
    v22 = v9;
    v25 = v6 / v8;
    if (v25 > 1000.0) {
        v22 = 1000.0 / v25 * v22;
    }
    if ((iFlags & 1) == 0 && (iFlags & 8) != 0) {
        hkVector4 v26;
        GetPosition_hk(v26);
        v26.m_quad = _mm_sub_ps(arInput.m_position.m_quad, v26.m_quad);
        if (fHeight * 0.8999999761581421 < v26.m_quad[2]) {
            hkVector4 v10;
            __m128 v15 = _mm_undefined_ps();
            v10.m_quad = OutVelocity.m_quad;
            __m128 v11 = _mm_mul_ps(v10.m_quad, v10.m_quad);
            float v12 = _mm_shuffle_ps(v11, v11, 85)[0] + v11[0];
            __m128 v13 = _mm_shuffle_ps(v11, v11, 170);
            v13[0] = v13[0] + v12;
            float v14 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v13[0])));
            v15[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
            __m128 v16 = _mm_cmpeq_ss(v13, v15);
            v15[0] = 3.0f - ((v13[0] * v14) * v14);
            __m128 v17 = _mm_setzero_ps();
            v17[0] = (0.5f * v14) * v15[0];
            __m128 v18 = _mm_andnot_ps(v16, v17);
            arOutput.m_objectImpulse.m_quad = _mm_mul_ps(
                _mm_mul_ps(_mm_shuffle_ps(v18, v18, 0), v10.m_quad), _mm_shuffle_ps(_mm_set_ss(v23), _mm_set_ss(v23), 0)
            );
        }
    }
    v23 = v6 * v22;
    arOutput.m_objectImpulse.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v22), _mm_set_ss(v22), 0), arOutput.m_objectImpulse.m_quad);
LABEL_19:;
    if (fBumpedForce < static_cast<double>(v23)) {
        fBumpedForce = v23;
        hkpRigidBody* m_body = arInput.m_body;
        if (m_body) {
            m_body->addReference();
        }
        hkpRigidBody* m_pntr = spBumpedBody.val();
        if (m_pntr) {
            m_pntr->removeReference();
        }
        spBumpedBody.m_pntr = m_body;
    }
}

// The bumped and support bodies are released in the destructor body itself, inside the Havok heap context, before the
// members' destructors (the damage impact list's RemoveAll, the manifold's free); the shape phantom and collision box
// are plain pointers holding a reference, released here.
bhkCharacterController::~bhkCharacterController() {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_HAVOK;
    LastManifold.m_size = 0;
    ahkpCharacterProxy* pProxy = static_cast<ahkpCharacterProxy*>(phkObject);
    hkpCachingShapePhantom* pShapePhantom =
        pProxy ? static_cast<hkpCachingShapePhantom*>(pProxy->GetShapePhantom()) : nullptr;
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(pShapePhantom->m_world);
    if (pWorld && pWorld->m_userData) {
        hkpCharacterProxy* pCharProxy = static_cast<hkpCharacterProxy*>(phkObject);
        if (pCharProxy) {
            UnMarkWrite();
            pCharProxy->removeCharacterProxyListener(this);
            UnMarkWrite();
        }
    }
    if (spBumpedBody.val()) {
        spBumpedBody.val()->removeReference();
    }
    spBumpedBody.m_pntr = nullptr;
    if (spBumpedCharProxy.val()) {
        spBumpedCharProxy.val()->removeReference();
    }
    spBumpedCharProxy.m_pntr = nullptr;
    if (spSupportBody) {
        spSupportBody->removeReference();
    }
    spSupportBody = nullptr;
    pTLS->eMemContext = eMemContext;
    if (spCollisionBox) {
        NiDecRefCount(spCollisionBox);
    }
    if (spShapePhantom) {
        NiDecRefCount(spShapePhantom);
    }
}

// The bumped proxy's controller: the ahkpCharacterProxy's user data, its owning bhkCharacterProxy, as
// characterInteractionCallback reads it.
bhkCharacterController* bhkCharacterController::GetBumpedCharacterController() {
    hkpCharacterProxy* pProxy = spBumpedCharProxy.val();
    if (pProxy) {
        return static_cast<bhkCharacterController*>(
            static_cast<bhkCharacterProxy*>(static_cast<ahkpCharacterProxy*>(pProxy)->m_userData)
        );
    }

    return nullptr;
}

bhkRigidBody* bhkCharacterController::GetBumpedBody() {
    hkpRigidBody* m_pntr = spBumpedBody.val();
    if (m_pntr) {
        return static_cast<bhkRigidBody*>(m_pntr->m_userData);
    }

    return nullptr;
}

void bhkCharacterController::SetShapeType(bhkCharacterController::SHAPE_TYPES aeType) {
    if (eShapeType != aeType && aeType < bhkCharacterController::ST_NUMTYPES) {
        if (spShapes[aeType]) {
            bool v3 = (iFlags & bhkCharacterController::COLLISION_DEBUG) != 0;
            if ((iFlags & bhkCharacterController::COLLISION_DEBUG) != 0) {
                RemoveCollisionDebug();
            }
            eShapeType = aeType;
            const hkVector4* v5 = &GethkPosition();
            bhkShape* Shape = GetShape();
            SetShape(Shape, *v5);
            if (v3) {
                BuildDisplayGeometry(nullptr);
            }
        }
    }
}

bhkCachingShapePhantom* bhkCharacterController::GetShapePhantom() {
    return spShapePhantom;
}

BSSimpleList<DamageImpactData*>* bhkCharacterController::GetHurtfulHits() {
    return &DamageImpacts;
}

float bhkCharacterController::QPitchAngle() {
    return fPitchAngle;
}

float bhkCharacterController::QRollAngle() {
    return fRollAngle;
}

float bhkCharacterController::GetLODDistance() {
    return fLODDistance;
}

float bhkCharacterController::GetRadius() {
    bhkCapsuleShape* v3;
    hkpCapsuleShape* phkObject;
    NiObject* v2 = spShapes[0];
    if (v2 && ((v3 = static_cast<bhkCapsuleShape*>(v2->DynamicCastMember(&bhkCapsuleShape::ms_RTTI))) != nullptr)) {
        phkObject = static_cast<hkpCapsuleShape*>(v3->phkObject);
        if (phkObject) {
            return phkObject->m_radius;
        }

        return 0.10000000149011612f;
    }

    return fRadius;
}

void bhkCharacterController::SetWantState(hkpCharacterStateType aeType) {
    eWantState = aeType;
}

void bhkCharacterController::SetScale(float afScale) {
    fScale = afScale;
}

void bhkCharacterController::SetPriority(uint32_t aiVal) {
    iPriority = aiVal;
}

void bhkCharacterController::UpdateState() {
    if (eWantState != HK_CHARACTER_MAX_STATE_ID) {
        m_currentState = eWantState;
        eWantState = HK_CHARACTER_MAX_STATE_ID;
    }
}

hkpCharacterStateType bhkCharacterController::GetState() {
    return getState();
}

bool bhkCharacterController::QIsSwimming() {
    return GetState() == HK_CHARACTER_USER_STATE_0;
}

bool bhkCharacterController::QIsFlying() {
    return (bool)((uint8_t)(((int32_t)GetState()) == 4));
}

bool bhkCharacterController::GetUseZ() {
    const hkpCharacterStateType eState = getState();
    return eState == HK_CHARACTER_USER_STATE_0 || eState == HK_CHARACTER_USER_STATE_1 || eState == HK_CHARACTER_FLYING;
}

bool bhkCharacterController::QChaseBip() {
    return GetFlag(CHASEBIP) != 0;
}

bool bhkCharacterController::QHitDamage() {
    return GetFlag(HITDAMAGE) != 0;
}

bool bhkCharacterController::QCanPitch() {
    return GetFlag(CAN_PITCH) != 0;
}

bool bhkCharacterController::QCanRoll() {
    return GetFlag(CAN_ROLL) != 0;
}

void bhkCharacterController::SetWaitForBatchAdd(bool abTF) {
    SetFlag(WAIT_FOR_BATCH_ADD, abTF);
}

void bhkCharacterController::SetQuickSimulate(bool abTF) {
    SetFlag(QUICK_SIMULATE, abTF);
}

void bhkCharacterController::SetWaterHeight(float afHeight) {
    fWaterHeight = NI2HK(afHeight);
}

void bhkCharacterController::SetCollisionGroup(uint32_t aiGroup) {
    if (spShapePhantom) {
        spShapePhantom->SetCollisionGroup(aiGroup);
    }
    if (spCollisionBox) {
        spCollisionBox->SetCollisionGroup(aiGroup);
    }
}

void bhkCharacterController::ClearManifold() {
    if (this) {
        if (auto* pProxy = static_cast<hkpCharacterProxy*>(phkObject)) {
            pProxy->m_manifold.m_size = 0;
        }
    }
}

// Writes the shape phantom's collision filter (0 without a phantom) to apResult and returns apResult.
CFilter* bhkCharacterController::GetCollisionFilter(CFilter* apResult) const {
    CFilter kFilter;
    const CFilter* pFilter;
    if (spShapePhantom) {
        bhkWorldObject* pPhantom = spShapePhantom;
        pFilter = pPhantom->GetCollisionFilter(&kFilter);
    }
    else {
        kFilter.iFilter = 0;
        pFilter = &kFilter;
    }
    apResult->iFilter = pFilter->iFilter;
    return apResult;
}

// The 3D of the shape phantom's game object.
NiAVObject* bhkCharacterController::GetNiObject() {
    if (!spShapePhantom) {
        return nullptr;
    }
    bhkWorldObject* pPhantom = spShapePhantom;
    return pPhantom->GetAVObject(0);
}

// Sets the shape phantom's collision filter.
void bhkCharacterController::SetCollisionFilter(CFilter aiFilter) {
    if (spShapePhantom) {
        bhkWorldObject* pPhantom = spShapePhantom;
        pPhantom->SetCollisionFilter(aiFilter);
    }
}

// Whether the listener's flag 0x200 is set.
bool bhkCharacterController::QSupported2() {
    return bhkCharacterListener::GetFlag(0x200) != 0;
}

// Asks for the jumping state with a jump of afHeight game units.
void bhkCharacterController::Jump(float afHeight) {
    eWantState = HK_CHARACTER_JUMPING;
    fJumpHeight = fBS2HkScaleSC * afHeight;
}

void bhkCharacterController::SetSize(float afRadius) {
    double v3;
    double fRadius;
    float newSizea;
    if (((int32_t)(iFlags & 1)) == 0) {
        v3 = fHeight * 0.5;
        if (afRadius > v3) {
            afRadius = (float)v3;
        }
        fRadius = (double)afRadius;
        if (afRadius <= 0.0) {
            fRadius = (double)this->fRadius;
        }
        newSizea = (float)fRadius;
        if (fDestRadius != newSizea) {
            if (((int32_t)eSizedShapeType) == 2) {
                eSizedShapeType = static_cast<bhkCharacterController::SHAPE_TYPES>((int32_t)eShapeType);
                SetShapeType(bhkCharacterController::ST_CAPSULE);
            }
            eSize = bhkCharacterController::CS_SIZING;
            fDestRadius = newSizea;
        }
    }
}

// Moves the controller's phantom and collision box out of their old world and into apWorld. Leaving the old world
// cleans the proxy up, removes both objects and clears the proxy's contact points; entering the new one re-sizes the
// capsule for a walking character, adds both objects and gives the phantom a debug colour. The phantom's world is read
// without testing the phantom, as the game does: the proxy is always there by the time a world is set.
void bhkCharacterController::SetWorld(bhkWorld* apWorld) {
    ahkpCharacterProxy* pProxy = static_cast<ahkpCharacterProxy*>(phkObject);
    hkpShapePhantom* pShapePhantom = pProxy != nullptr ? pProxy->GetShapePhantom() : nullptr;
    // The Havok world the phantom is in, and the bhkWorld it belongs to, kept as ahkpWorld's user data at +0x350.
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(pShapePhantom->m_world);
    bhkWorld* pOldWorld = pWorld != nullptr ? static_cast<bhkWorld*>(pWorld->m_userData) : nullptr;
    if (pOldWorld == apWorld) {
        return;
    }
    if (pOldWorld != nullptr) {
        CleanUp();
        bhkCachingShapePhantom* pPhantom = spShapePhantom;
        if (pPhantom != nullptr) {
            pPhantom->Remove();
        }
        bhkWorldObject* pCollisionBox = spCollisionBox;
        if (pCollisionBox != nullptr) {
            pCollisionBox->Remove();
        }
    }
    ahkpCharacterProxy* pOldProxy = static_cast<ahkpCharacterProxy*>(phkObject);
    if (pOldProxy != nullptr) {
        pOldProxy->GetCdPoints()->m_size = 0;
    }
    if (apWorld == nullptr) {
        return;
    }
    bhkCachingShapePhantom* pPhantom = spShapePhantom;
    if (pPhantom != nullptr) {
        if ((iFlags & QUADRUPED) == 0 && (iFlags & QUICK_SIMULATE) == 0) {
            // The radius is computed in double and rounded to a float before it is passed.
            float fHkRadius = static_cast<float>(fBS2HkScaleSC * 0.009999999776482582);
            SetSize(fHkRadius);
            fHkRadius = static_cast<float>(fBS2HkScaleSC * 0.009999999776482582);
            SetCapsuleRadius(fHkRadius);
            SetSize(0.0f);
        }
        pPhantom->Add(apWorld);
        // Re-reads the phantom from the field after Add.
        apWorld->SetDebugObjectColor(spShapePhantom, hkColor::rgbFromFloats(1.0f, 1.0f, 0.0f, 0.75f));
    }
    bhkWorldObject* pCollisionBox = spCollisionBox;
    if (pCollisionBox != nullptr) {
        pCollisionBox->Add(apWorld);
    }
}

// Gives the controller's capsule the radius afNewRadius at the same overall extent: the end points move apart along z
// by the radius's shrinking (A up, B down), each stored with the capsule's old radius in w, then the radius. Without
// a Havok capsule the old radius is the cinfo default 0.1 and the end points are zero.
void bhkCharacterController::SetCapsuleRadius(float afNewRadius) {
    bhkShape* pShape = spShapes[0];
    if (!pShape) {
        return;
    }
    bhkCapsuleShape* pCapsuleShape =
        static_cast<bhkCapsuleShape*>(pShape->DynamicCastMember(&bhkCapsuleShape::ms_RTTI));
    if (!pCapsuleShape) {
        return;
    }
    hkpCapsuleShape* pCapsule = static_cast<hkpCapsuleShape*>(pCapsuleShape->phkObject);
    const float fOldRadius = pCapsule ? pCapsule->m_radius : 0.1f;
    const float fShrink = fOldRadius - afNewRadius;
    hkVector4 kVertexA;
    kVertexA.m_quad = (pCapsule ? pCapsule->m_vertexA : hkVector4::getZero()).m_quad;
    hkVector4 kVertexB;
    kVertexB.m_quad = (pCapsule ? pCapsule->m_vertexB : hkVector4::getZero()).m_quad;
    kVertexA(2) = kVertexA(2) + fShrink;
    kVertexB(2) = kVertexB(2) - fShrink;
    if (pCapsule) {
        pCapsule->m_vertexA.m_quad = kVertexA.m_quad;
        pCapsule->m_vertexA(3) = pCapsule->m_radius;
    }
    pCapsule = static_cast<hkpCapsuleShape*>(pCapsuleShape->phkObject);
    if (pCapsule) {
        pCapsule->m_vertexB.m_quad = kVertexB.m_quad;
        pCapsule->m_vertexB(3) = pCapsule->m_radius;
    }
    pCapsule = static_cast<hkpCapsuleShape*>(pCapsuleShape->phkObject);
    if (pCapsule) {
        pCapsule->m_radius = afNewRadius;
    }
}

void bhkCharacterController::SetNiObject(NiAVObject* apObject) {
    bhkCachingShapePhantom* spShapePhantom;
    bhkWorldObject* spCollisionBox;
    spShapePhantom = this->spShapePhantom;
    if (spShapePhantom) {
        spShapePhantom->bhkWorldObject::SetAVObject(apObject, 0);
    }
    spCollisionBox = this->spCollisionBox;
    if (spCollisionBox) {
        spCollisionBox->SetAVObject(apObject, 0);
    }
}

// The detached child is held in a NiPointer local and released after the flag is cleared.
void bhkCharacterController::RemoveCollisionDebug() {
    NiAVObject* pDebugNode = GetCollisionDebugNode();
    NiPointer<NiAVObject> spDetached;
    if (pDebugNode) {
        NiNode* pParent = pDebugNode->m_pkParent;
        if (pParent) {
            pParent->DetachChild(pDebugNode, spDetached);
        }
    }
    iFlags &= ~COLLISION_DEBUG;
}

void bhkCharacterController::SetStartingHeight(float afHeight) {
    fFallStartHeight = afHeight;
}

// Moves the proxy to arPos, given in game units.
void bhkCharacterController::SetPosition(const NiPoint3& arPos) {
    hkVector4 kPos;
    SetPosition(NI2HK(kPos, arPos));
}

// Moves the proxy to arPos, in Havok units, offset by kRotCenter turned by the shape phantom's rotation.
void bhkCharacterController::SetPosition(const hkVector4& arPos) {
    // EXE-INLINED 0x4AE750
    hkpShapePhantom* pp = spShapePhantom ? static_cast<hkpShapePhantom*>(spShapePhantom->phkObject) : nullptr;
    hkVector4 cntr;
    cntr.setRotatedDir(pp->m_motionState.m_transform.m_rotation, RotCenter);
    hkVector4 hkp;
    hkp.m_quad = _mm_add_ps(arPos.m_quad, cntr.m_quad);
    // EXE-INLINED none: bhkCharacterProxy::SetPosition
    hkpCharacterProxy* pProxy = static_cast<hkpCharacterProxy*>(this->phkObject);
    if (pProxy) {
        UnMarkWrite();
        pProxy->setPosition(hkp);
        pProxy->m_oldDisplacement.setZero4();
        UnMarkWrite();
    }
}

// The hkVector4 local is left uninitialised by its constructor, and the proxy comes from the null-safe getter.
void bhkCharacterController::SetLinearVelocityNi(const NiPoint3& arVelocity) {
    hkVector4 kVelocity;
    hkpCharacterProxy* pProxy = operator*();
    if (pProxy != nullptr) {
        pProxy->setVelocity(&NI2HK(kVelocity, arVelocity));
    }
}

void bhkCharacterController::SetVelocityModifier(const NiPoint3& arVelocity, float afTime) {
    uint32_t v8;
    __m128 v9;
    v9[0] = arVelocity.x * fBS2HkScaleSC;
    v9[1] = arVelocity.y * fBS2HkScaleSC;
    v9[2] = fBS2HkScaleSC * arVelocity.z;
    v9[3] = 0.0f;
    *reinterpret_cast<float*>(&v8) = 1.0 / afTime;
    __m128 v3 =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v8)), _mm_set_ss(std::bit_cast<float>(v8)), 0), v9);
    __m128 v4 = _mm_mul_ps(v3, v3);
    float v5 = _mm_shuffle_ps(v4, v4, 85)[0] + v4[0];
    float v6 = _mm_shuffle_ps(v4, v4, 170)[0];
    __m128 v7 = _mm_mul_ps(VelocityMod.m_quad, VelocityMod.m_quad);
    if ((_mm_shuffle_ps(v7, v7, 170)[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0])) < static_cast<double>((v6 + v5))) {
        fVelocityTime = afTime;
        VelocityMod.m_quad = v3;
    }
}

// The "bhkColDisp" node the collision debug display was given: the child of the shape phantom's game object's first
// child node that carries that name. Without one the request is forgotten; a found node keeps it. The string's
// reference is dropped before either answer.
NiAVObject* bhkCharacterController::GetCollisionDebugNode() {
    if ((iFlags & COLLISION_DEBUG) == 0) {
        return nullptr;
    }
    NiAVObject* pFound = nullptr;
    if (spShapePhantom != nullptr) {
        bhkWorldObject* pPhantom = spShapePhantom;
        NiAVObject* pObject = pPhantom->GetAVObject(0);
        if (pObject != nullptr) {
            NiNode* pNode = pObject->IsNode();
            if (pNode != nullptr && pNode->m_kChildren.m_usSize > 0) {
                NiAVObject* pFirstChild = pNode->m_kChildren.m_pBase[0].m_pObject;
                if (pFirstChild != nullptr) {
                    NiNode* pChildNode = static_cast<NiNode*>(pFirstChild->DynamicCastMember(&NiNode::ms_RTTI));
                    if (pChildNode != nullptr) {
                        NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString("bhkColDisp");
                        const uint32_t uiCount = pChildNode->m_kChildren.m_usSize;
                        for (uint32_t i = 0; i < uiCount && pFound == nullptr; ++i) {
                            NiAVObject* pChild = pChildNode->m_kChildren.m_pBase[i].m_pObject;
                            if (pChild != nullptr && pChild->m_kName.GetHandle() == kName) {
                                pFound = pChild;
                            }
                        }
                        // Releases the string by decrementing the untyped reference count 8 bytes before the
                        // characters, unless the handle is NULL_STRING.
                        if (kName != NiGlobalStringTable::NULL_STRING) {
                            InterlockedDecrementUInt32(
                                reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName - 8))
                            );
                        }
                    }
                }
            }
        }
    }
    if (pFound == nullptr) {
        // Clears the bit with a 32-bit write, which rewrites all four bytes.
        uint32_t& riFlags = iFlags;
        riFlags &= ~COLLISION_DEBUG;
    }
    return pFound;
}

// Reads the proxy's position in game units into arPos.
void bhkCharacterController::GetPosition(NiPoint3& arPos) {
    hkVector4 kPos;
    kPos.~hkVector4();
    ::new (&kPos) hkVector4;
    GetPosition_hk(kPos);
    HK2NI(arPos, kPos);
}

// The proxy's position less kRotCenter turned by the shape phantom's rotation, read under the world's write marks;
// the zero vector less that offset without a proxy.
void bhkCharacterController::GetPosition_hk(hkVector4& arPos) const {
    // EXE-INLINED 0x4AE750
    hkpShapePhantom* pPhantom = spShapePhantom ? static_cast<hkpShapePhantom*>(spShapePhantom->phkObject) : nullptr;
    hkVector4 kOffset;
    kOffset.setRotatedDir(pPhantom->m_motionState.m_transform.m_rotation, RotCenter);
    hkpCharacterProxy* pProxy = static_cast<hkpCharacterProxy*>(this->phkObject);
    const hkVector4* pPosition = &hkVector4Zero;
    if (pProxy) {
        // This getter is const and GethkWorldRw is non-const; GethkWorldRw is called directly for its write marks.
        bhkCharacterController* pThis = const_cast<bhkCharacterController*>(this);
        pThis->bhkCharacterProxy::GethkWorldRw();
        pPosition = &pProxy->getPosition();
        pThis->bhkCharacterProxy::GethkWorldRw();
    }
    arPos.m_quad = _mm_sub_ps(pPosition->m_quad, kOffset.m_quad);
}

float bhkCharacterController::QFallStartHeight() {
    return fFallStartHeight;
}

// Binds apObject to aiKey as a property of the shape phantom's Havok object, under the world's write marks.
void bhkCharacterController::SetBSReference(uint32_t aiKey, void* apObject) {
    bhkWorldObject* pPhantom = spShapePhantom;
    if (pPhantom == nullptr) {
        return;
    }
    if (pPhantom->HasProperty(aiKey)) {
        return;
    }
    hkpWorldObject* pObject = static_cast<hkpWorldObject*>(pPhantom->phkObject);
    if (pObject == nullptr) {
        return;
    }
    // The object pointer is sign-extended into the 64-bit property value. intptr_t is the 32-bit pointer on 32 bits and
    // the whole pointer on 64, so GetBSReference's getPtr() gets the address back on both widths.
    hkpPropertyValue value;
    value.m_data = static_cast<uint64_t>(static_cast<int64_t>(reinterpret_cast<intptr_t>(apObject)));
    pPhantom->MarkWrite();
    pObject->addProperty(aiKey, value);
    pPhantom->MarkWrite();
}

bhkCharacterController::SHAPE_TYPES bhkCharacterController::QShapeType() const {
    return eShapeType;
}

bhkCharacterController::CHARACTER_SIZE bhkCharacterController::GetSize() const {
    return eSize;
}

void bhkCharacterController::SetIsProjectile(bool abTF) {
    static_cast<bhkCharacterListener*>(this)->SetFlag(0x80000u, abTF);
}

float bhkCharacterController::QFallTime() const {
    return fFallTime;
}

hkVector4* bhkCharacterController::GetRotCenter() {
    return &RotCenter;
}

bool bhkCharacterController::QSupported() {
    return GetFlag(256u) != 0;
}

void bhkCharacterController::UpdateLODDistance(const NiPoint3& aReferencePoint, NiAVObject* pobject) {
    if (pobject) {
        NiBound* m_kWorldBound = pobject->m_kWorldBound;
        if (!m_kWorldBound) {
            m_kWorldBound = &NiAVObject::NullBoundS;
        }
        float v4 = aReferencePoint.x - m_kWorldBound->m_kCenter.x;
        float v5 = aReferencePoint.y - m_kWorldBound->m_kCenter.y;
        float v6 = aReferencePoint.z - m_kWorldBound->m_kCenter.z;
        float nodea = v4 * v4 + v5 * v5 + v6 * v6;
        float nodeb = std::sqrt((double)nodea);
        float node = nodeb - m_kWorldBound->m_fRadius;
        if (!(node > 0.0)) {
            fLODDistance = 0.0;
        }
        else {
            fLODDistance = node;
        }
    }
}

void bhkCharacterController::SetFallTime(float afFallTime) {
    fFallTime = afFallTime;
}

void bhkCharacterController::SetFollowRagdoll(bool abTF) {
    static_cast<bhkCharacterListener*>(this)->SetFlag(4096, abTF);
}

void bhkCharacterController::SetSwimFloatHeight(float afHeight) {
    fSwimFloatHeight = afHeight;
}

void bhkCharacterController::SetTrueState(hkpCharacterStateType aeType) {
    m_currentState = static_cast<hkpCharacterStateType>((int32_t)aeType);
}

void bhkCharacterController::SetPitchMult(float afPitchMult) {
    fPitchMult = afPitchMult;
}

void bhkCharacterController::ResetCalculatePitchTimer() {
    fCalculatePitchTimer = bhkCharacterController::fCalculatePitchTimeOut;
}

void bhkCharacterController::UpdateFalling() {
    fFallTime = (StepInfo.m_deltaTime + fFallTime);
}

// The cosine and sine results are each rounded to float. The negated sine is -0.0f minus the sine. Without a shape
// phantom the transform is still read through the null phantom pointer, as the game does.
void bhkCharacterController::ApplyPitchAndRoll(const hkQuaternion& arRot) {
    hkQuaternion kRotation = arRot;
    kRotation.normalize();
    bhkCachingShapePhantom* pPhantom = spShapePhantom;
    hkpShapePhantom* phkPhantom = pPhantom ? static_cast<hkpShapePhantom*>(pPhantom->phkObject) : nullptr;
    alignas(16) hkTransform kTransform = phkPhantom->m_motionState.m_transform;
    kTransform.m_rotation.set(kRotation);
    if (0.0f != fPitchAngle && (bhkCharacterListener::iFlags >> 25) & 1) {
        float fCos = std::cos(fPitchAngle);
        float fSin = std::sin(fPitchAngle);
        alignas(16) hkMatrix3 kPitch;
        kPitch.m_col0.m_quad = {1.0f, 0.0f, 0.0f, 0.0f};
        kPitch.m_col1.m_quad = {0.0f, fCos, (-0.0f - fSin), 0.0f};
        kPitch.m_col2.m_quad = {0.0f, fSin, fCos, 0.0f};
        kTransform.m_rotation.mul(kPitch);
    }
    if (0.0f != fRollAngle && (bhkCharacterListener::iFlags >> 26) & 1) {
        float fCos = std::cos(fRollAngle);
        float fSin = std::sin(fRollAngle);
        alignas(16) hkMatrix3 kRoll;
        kRoll.m_col0.m_quad = {fCos, 0.0f, fSin, 0.0f};
        kRoll.m_col1.m_quad = {0.0f, 1.0f, 0.0f, 0.0f};
        kRoll.m_col2.m_quad = {(-0.0f - fSin), 0.0f, fCos, 0.0f};
        kTransform.m_rotation.mul(kRoll);
    }
    if (pPhantom) {
        hkpShapePhantom* phkObject = static_cast<hkpShapePhantom*>(pPhantom->phkObject);
        if (phkObject) {
            pPhantom->MarkWrite();
            phkObject->setTransform(kTransform);
            pPhantom->UnMarkWrite();
        }
    }
}

int32_t bhkCharacterController::GetOnStairs() {
    return (int32_t)((uint8_t)(static_cast<bhkCharacterListener*>(this)->GetFlag(16777216) != 0));
}

int32_t bhkCharacterController::CanJump() {
    return (int32_t)((uint8_t)(static_cast<bhkCharacterListener*>(this)->GetFlag(1024) != 0));
}

int32_t bhkCharacterController::IsOnElevator() {
    return (int32_t)((uint8_t)(static_cast<bhkCharacterListener*>(this)->GetFlag(131072) != 0));
}

bhkCharacterState* bhkCharacterController::GetCurrentState() {
    return static_cast<bhkCharacterState*>(m_stateManager->getState(m_currentState));
}

float bhkCharacterController::ApplyRotationModifier(float afOldRotation, float afDeltaTime) {
    float fRotModTime;
    float v6;
    double v3 = (double)afOldRotation;
    if (!(this->fRotModTime > 0.0)) {
        fRotMod = (float)0.0;
        return afOldRotation;
    }

    fRotModTime = this->fRotModTime;
    if (fRotModTime > ((double)afDeltaTime)) {
        fRotModTime = afDeltaTime;
    }
    v6 = (float)(v3 + (fRotMod * fRotModTime));
    this->fRotModTime = (this->fRotModTime - afDeltaTime);
    fRotMod = (float)(fRotMod - 0.05000000074505806);
    return v6;
}

bhkRigidBody* bhkCharacterController::GetSupportBody() const {
    hkpRigidBody* spSupportBody;
    spSupportBody = this->spSupportBody;
    if (spSupportBody) {
        return static_cast<bhkRigidBody*>(spSupportBody->m_userData);
    }

    return nullptr;
}

uint32_t bhkCharacterController::GetFilterGroup() {
    CFilter a2;
    if (!spShapePhantom) {
        return (uint32_t)0;
    }
    bhkCachingShapePhantom* v1 = spShapePhantom;
    return v1->bhkWorldObject::GetCollisionFilter(&a2)->GetGroup();
}

float bhkCharacterController::QWaterHeight() const {
    return fWaterHeight;
}

float bhkCharacterController::QSwimFloatHeight() const {
    return fSwimFloatHeight;
}

// Eases the pitch and roll angles towards the slope under the character. On the ground (QUADRUPED) the slope comes
// from the support heights in front/behind (pitch) and left/right (roll), each found with FindSupportPoint while its
// slot is still empty (FLT_MIN), clamped to fMaxSlopeDefault degrees; otherwise from the support normal turned into
// the phantom's frame. The asm keeps the clamp limits and the 0 / 1 constants on the x87 stack (80-bit) and rounds
// every intermediate it stores to float; the double locals here hold the same values.
void bhkCharacterController::CalculatePitchAndRoll() {
    uint32_t uiFlags = iFlags;
    if ((uiFlags >> 18) & 1) {
        return;
    }
    if (!(fCalculatePitchTimer > 0.0f)) {
        return;
    }
    if (uiFlags & QUADRUPED) {
        if ((uiFlags >> 25) & 1) {
            float fBack = SupportHeight[0];
            float fFront = SupportHeight[1];
            if (1.1754944e-38f == fBack) {
                alignas(16) hkVector4 kPoint;
                FindSupportPoint(0, kPoint);
                fBack = kPoint.m_quad[2];
            }
            if (1.1754944e-38f == fFront) {
                alignas(16) hkVector4 kPoint;
                FindSupportPoint(1, kPoint);
                fFront = kPoint.m_quad[2];
            }
            float fDelta = fFront - fBack;
            float fAngle;
            if (fDelta == 0.0f) {
                fAngle = 0.0f;
            }
            else {
                float fSlope = fDelta / fHeight;
                fAngle = std::atan(fSlope);
                if (fAngle > 0.0f) {
                    double dLimit = fMaxSlopeDefault * 0.01745329238474369;
                    if (dLimit < fAngle) {
                        fAngle = (float)dLimit;
                    }
                }
                else {
                    double dLimit = fMaxSlopeDefault * -0.01745329238474369;
                    if (dLimit > fAngle) {
                        fAngle = (float)dLimit;
                    }
                }
            }
            float fDiff = fAngle - fPitchAngle;
            if (!(fDiff == 0.0f)) {
                if (1.0f < fPitchMult) {
                    fPitchMult = 1.0f;
                }
                fPitchAngle = (float)((double)fDiff * fPitchMult + fPitchAngle);
            }
        }
        if (!((iFlags >> 26) & 1)) {
            return;
        }
        float fLeft = SupportHeight[2];
        float fRight = SupportHeight[3];
        if (1.1754944e-38f == fLeft) {
            alignas(16) hkVector4 kPoint;
            FindSupportPoint(2, kPoint);
            fLeft = kPoint.m_quad[2];
        }
        if (1.1754944e-38f == fRight) {
            alignas(16) hkVector4 kPoint;
            FindSupportPoint(3, kPoint);
            fRight = kPoint.m_quad[2];
        }
        float fDelta = fRight - fLeft;
        float fAngle;
        if (fDelta == 0.0f) {
            fAngle = 0.0f;
        }
        else {
            float fSlope = fDelta / fHeight;
            fAngle = std::atan(fSlope);
            if (fAngle > 0.0f) {
                double dLimit = fMaxSlopeDefault * 0.01745329238474369;
                if (dLimit < fAngle) {
                    fAngle = (float)dLimit;
                }
            }
            else {
                double dLimit = fMaxSlopeDefault * -0.01745329238474369;
                if (dLimit > fAngle) {
                    fAngle = (float)dLimit;
                }
            }
        }
        float fDiff = fAngle - fRollAngle;
        if (!(fDiff == 0.0f)) {
            if (1.0f < fPitchMult) {
                fPitchMult = 1.0f;
            }
            fRollAngle = (float)((double)fDiff * fPitchMult + fRollAngle);
        }
        return;
    }
    // EXE-INLINED 0x4AE750
    hkpShapePhantom* pPhantom = spShapePhantom ? static_cast<hkpShapePhantom*>(spShapePhantom->phkObject) : nullptr;
    alignas(16) hkVector4 kNormal;
    kNormal.setRotatedInverseDir(pPhantom->m_motionState.m_transform.m_rotation, hkSupportNorm);
    float fLimit = (float)(fMaxSlopeDefault * 0.01745329238474369);
    uint32_t uiFlags2 = iFlags;
    if ((uiFlags2 >> 26) & 1) {
        float fTilt = (float)(kNormal.m_quad[0] * -1.0);
        float fClamped = (fTilt < fLimit) ? fTilt : fLimit;
        float fNegLimit = -fLimit;
        fClamped = (fNegLimit < fClamped) ? fClamped : fNegLimit;
        float fDiff = fClamped - fRollAngle;
        if (!(fDiff == 0.0f)) {
            if (1.0f < fPitchMult) {
                fPitchMult = 1.0f;
            }
            fRollAngle = (float)((double)fDiff * fPitchMult + fRollAngle);
        }
    }
    if ((uiFlags2 >> 25) & 1) {
        float fTilt = kNormal.m_quad[1];
        float fClamped = (fTilt < fLimit) ? fTilt : fLimit;
        float fNegLimit = -fLimit;
        fClamped = (fNegLimit < fClamped) ? fClamped : fNegLimit;
        float fDiff = fClamped - fPitchAngle;
        if (!(fDiff == 0.0f)) {
            if (1.0f < fPitchMult) {
                fPitchMult = 1.0f;
            }
            fPitchAngle = (float)((double)fDiff * fPitchMult + fPitchAngle);
        }
    }
}

void bhkCharacterController::FindSupportPoint(uint32_t aiFrontBack, hkVector4& arPos) {
    hkVector4 v4;
    hkVector4 v6;
    hkpCachingShapePhantom* ShapePhantom;
    bhkWorld* pw;
    CFilter iflt;
    bhkPickData pickRes;
    hkpClosestRayHitCollector v13;
    pickRes.m_hitFraction = 1.0f;
    pickRes.m_enableShapeCollectionFilter.m_bool = 0;
    pickRes.m_filterInfo = 0;
    pickRes.m_extraInfo = -1;
    pickRes.m_shapeKeyIndex = 0;
    pickRes.m_shapeKeys[0] = -1;
    pickRes.m_rootCollidable = nullptr;
    pickRes.pCache = nullptr;
    pickRes.bPickFailed = false;
    v4.m_quad = FrontBack[aiFrontBack].m_quad;
    arPos.m_quad = v4.m_quad;
    double v5 = arPos.m_quad[2] - 25.0;
    pickRes.m_from.m_quad = v4.m_quad;
    arPos.m_quad[2] = v5;
    pickRes.hkLength.m_quad = hkVector4Zero.m_quad;
    v6.m_quad = arPos.m_quad;
    pickRes.pCloseCollector = static_cast<SpecificItemCollector*>(&v13);
    pickRes.pAllCollector = nullptr;
    pickRes.m_to.m_quad = v6.m_quad;
    GetCollisionFilter(&iflt);
    ahkpCharacterProxy* phkObject = static_cast<ahkpCharacterProxy*>(this->phkObject);
    pickRes.m_filterInfo = iflt.iFilter;
    if (phkObject) {
        ShapePhantom = static_cast<hkpCachingShapePhantom*>(phkObject->GetShapePhantom());
    }
    else {
        ShapePhantom = nullptr;
    }
    ahkpWorld* m_world = static_cast<ahkpWorld*>(ShapePhantom->m_world);
    if (m_world) {
        pw = static_cast<bhkWorld*>(m_world->m_userData);
    }
    else {
        pw = nullptr;
    }
    if (pw->PickObject(pickRes)) {
        *reinterpret_cast<float*>(&iflt.iFilter) = 1.0 - v13.m_rayHit.m_hitFraction;
        arPos.m_quad[2] = *reinterpret_cast<float*>(&iflt.iFilter) * 25.0 + arPos.m_quad[2];
    }
}

// KEEP: when arInfo brings its own shape (spShape set), the exe reads the half-height slot (v129, [esp+0x78]) that
// only the shape-building paths write (fld at 0xC724F9) and stores twice it, scaled, into fActorHeight: an
// uninitialised read. The exe's cleanup-only C++ EH frame is left out.
void bhkCharacterController::InitShapes(bhkCharacterControllerCinfo& arInfo) {
    bhkShape* v4;
    bhkConvexShape* pcs;
    hkpConvexShape* v6;
    double v7;
    bhkCharacterControllerCinfo* v8;
    double v9;
    double v10;
    double v11;
    bhkCharacterController::SHAPE_TYPES v81;
    float v97;
    float v98;
    float v114;
    NiPoint3 top;
    NiPoint3 bottom;
    hkListShapeCinfo lsi;
    void* v125;
    float v129;
    NiPoint3 top2;
    alignas(16) NiPoint3 bottom2;
    bhkCharacterController* v2 = this;
    eSizedShapeType = bhkCharacterController::ST_NUMTYPES;
    bhkShape* m_pObject = arInfo.spShape.m_pObject;
    bhkCharacterControllerCinfo* stSizeInBytes = &arInfo;
    if (!m_pObject) {
        double v23;
        float v127;
        float v128;
        if (arInfo.pObject) {
            if (!arInfo.bUseDefaultShape) {
                BSBound* pb;
                float x;
                float y;
                float v19;
                float v20;
                BSNiBound* m_kWorldBound;
                double m_fRadius;
                float v101;
                pb = BSBound::Find(arInfo.pObject);
                if (pb) {
                    double fScale;
                    double v16;
                    double v17;
                    double v18;
                    bhkCharacterController* v27;
                    double v29;
                    double v65;
                    double v66;
                    double v70;
                    double v71;
                    float v99;
                    float v100;
                    float frad;
                    float v116;
                    float v117;
                    float v121;
                    float v130;
                    float v131;
                    x = NiPoint3_ZERO.x;
                    y = NiPoint3_ZERO.y;
                    fScale = arInfo.fScale;
                    v121 = fScale * pb->Extents.x;
                    v99 = pb->Extents.y * fScale;
                    v116 = fScale * pb->Extents.z;
                    v130 = v121;
                    v127 = v121;
                    v131 = v99;
                    v128 = v99;
                    v16 = arInfo.fScale;
                    v129 = v116;
                    v121 = v16;
                    v17 = v121;
                    v121 = pb->Center.x * v121;
                    v100 = pb->Center.y * v17;
                    v18 = v17 * pb->Center.z;
                    top.z = NiPoint3_ZERO.z;
                    v117 = v18;
                    top2.x = v121;
                    v19 = v121;
                    top2.y = v100;
                    v20 = v100;
                    top2.z = v117;
                LABEL_28:;
                    double v24 = v127;
                    v130 = v19;
                    double v25 = v129;
                    v131 = v20;
                    top.x = x;
                    top.y = y;
                    double v26 = v128;
                    if (!(v129 < static_cast<double>(v127)) && !(v26 > v25)) {
                        v27 = this;
                    }
                    else {
                        v27 = this;
                        this->iFlags |= 1u;
                    }
                    if ((v27->iFlags & 1) != 0) {
                        double v31;
                        double v33;
                        bhkShape* pcs0;
                        bhkShape* pcs1;
                        bhkShape* pcs2;
                        hkpShape* phkObject;
                        hkpShape* v53;
                        hkpShape* v55;
                        bhkListShape* v62;
                        bool v63;
                        bhkCharacterController::SHAPE_TYPES eShapeType;
                        float v118;
                        top.z = 0.0;
                        v29 = 0.0;
                        top.x = v19;
                        top.y = v20;
                        if (!(v24 > v26)) {
                            double v37;
                            double v38;
                            v98 = v26 * 2.0;
                            if (!(v24 < v25)) {
                                frad = v25;
                                v118 = v24;
                                v37 = v25;
                                v38 = 2.0;
                            }
                            else {
                                frad = v24;
                                v37 = v25;
                                v38 = 2.0;
                                v118 = v37;
                            }
                            bottom.x = v19;
                            bottom.z = top.z;
                            top.y = v26 - frad + top.y;
                            bottom.y = top.y * -1.0;
                            v10 = frad;
                            double v41 = v38;
                            v33 = v37;
                            v31 = v41;
                        }
                        else {
                            double v34;
                            v98 = v24 * 2.0;
                            v31 = 2.0;
                            if (!(v26 < v25)) {
                                frad = v25;
                                v118 = v26;
                                double v35 = v24;
                                v33 = v25;
                                v34 = v35;
                            }
                            else {
                                frad = v26;
                                double v32 = v24;
                                v33 = v25;
                                v34 = v32;
                                v118 = v33;
                            }
                            bottom.y = v20;
                            bottom.z = top.z;
                            double v36 = v34 - frad + top.x;
                            v10 = frad;
                            top.x = v36;
                            bottom.x = top.x * -1.0;
                        }
                        if (!(0.75 * v118 > v10)) {
                            double v79;
                            v71 = v33;
                            v70 = fBS2HkScaleSC;
                        LABEL_83:;
                            v2 = this;
                            v8 = stSizeInBytes;
                            NiPointer<bhkShape>* spShapes = this->spShapes;
                            this->RotCenter.m_quad[0] = v130 * v70;
                            v2->RotCenter.m_quad[1] = v131 * v70;
                            if (*spShapes) {
                                v79 = v70;
                            }
                            else {
                                bhkShape* Convex;
                                NiPointer<bhkShape>* v76;
                                float v106 = top.z + v10;
                                float v107 = v106 * v70;
                                float v108 = v107 + (v8->m_keepDistance + v8->m_keepDistance);
                                v2->fCenter = v108;
                                v2->RotCenter.m_quad[2] = v108 + v2->RotCenter.m_quad[2];
                                if (v29 == v8->m_contactAngleSensitivity) {
                                    v8->m_contactAngleSensitivity = v70 * v10;
                                }
                                if ((v2->iFlags & 1) != 0) {
                                    v8->eShapeType = bhkCharacterController::ST_CAPSULE;
                                    bhkCapsuleShape* v74 = static_cast<bhkCapsuleShape*>(
                                        NiMemObject::operator new(sizeof(bhkCapsuleShape))
                                    );
                                    v125 = (void*)v74;
                                    if (v74) {
                                        Convex = ::new (v74) bhkCapsuleShape(top, bottom, frad);
                                    }
                                    else {
                                        Convex = nullptr;
                                    }
                                    v63 = v8->spShape.m_pObject == nullptr;
                                    if (v63) {
                                        v76 = &v2->spShapes[v8->eShapeType];
                                    }
                                    else {
                                        v76 = v2->spShapes;
                                    }
                                }
                                else {
                                    bhkCapsuleShape* v78;
                                    bhkCapsuleShape* v77 = static_cast<bhkCapsuleShape*>(
                                        NiMemObject::operator new(sizeof(bhkCapsuleShape))
                                    );
                                    v125 = (void*)v77;
                                    if (v77) {
                                        v78 = ::new (v77) bhkCapsuleShape(top, bottom, frad);
                                    }
                                    else {
                                        v78 = nullptr;
                                    }
                                    *spShapes = v78;
                                    if (v8->bUseDefaultShape) {
                                        if (!bhkCharacterController::spDefaultConvexShape.m_pObject) {
                                            bhkCharacterController::spDefaultConvexShape.operator=(
                                                MakeConvex(top, bottom, frad, v2->fStepHeight)
                                            );
                                        }
                                        v2->spShapes[1] = bhkCharacterController::spDefaultConvexShape;
                                        v79 = fBS2HkScaleSC;
                                        v10 = frad;
                                        v71 = v129;
                                        goto LABEL_107;
                                    }
                                    Convex = MakeConvex(top, bottom, frad, v2->fStepHeight);
                                    v76 = &v2->spShapes[1];
                                }
                                *v76 = Convex;
                                v79 = fBS2HkScaleSC;
                                v10 = frad;
                                v71 = v129;
                            }
                        LABEL_107:;
                            double v80 = v71;
                            v11 = v79;
                            v9 = v80;
                            goto LABEL_108;
                        }
                        float v119 = v118 - v10;
                        top.z = v119 * v31 + v10 + 0.0;
                        bottom.z = top.z;
                        v121 = v31 * stSizeInBytes->m_keepDistance;
                        double v44 = v121;
                        this->fCenter = v121;
                        this->RotCenter.m_quad[2] = v44 + this->RotCenter.m_quad[2];
                        if (0.0 == stSizeInBytes->m_contactAngleSensitivity) {
                            stSizeInBytes->m_contactAngleSensitivity = v10 * fBS2HkScaleSC;
                        }
                        // EXE-INLINED none: hkListShapeCinfo::hkListShapeCinfo
                        // EXE-INLINED 0x4B5020
                        lsi.m_userData = 0;
                        lsi.m_childShapes.m_data = nullptr;
                        lsi.m_childShapes.m_size = 0;
                        lsi.m_childShapes.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
                        lsi.m_childShapeFilterInfo.m_data = nullptr;
                        lsi.m_childShapeFilterInfo.m_size = 0;
                        lsi.m_childShapeFilterInfo.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
                        bhkCapsuleShape* v45 =
                            static_cast<bhkCapsuleShape*>(NiMemObject::operator new(sizeof(bhkCapsuleShape)));
                        if (v45) {
                            pcs0 = (bhkShape*)::new (v45) bhkCapsuleShape(top, bottom, frad);
                        }
                        else {
                            pcs0 = nullptr;
                        }
                        top2.x = top.x;
                        top.y = top.y - 8.0;
                        top2.y = top.y;
                        v121 = v119 + 1.0;
                        top.z = v121;
                        top2.z = v119 - 1.0;
                        bhkCapsuleShape* v47 =
                            static_cast<bhkCapsuleShape*>(NiMemObject::operator new(sizeof(bhkCapsuleShape)));
                        v125 = (void*)v47;
                        if (v47) {
                            float v104 = v119 - 1.0;
                            pcs1 = (bhkShape*)::new (v47) bhkCapsuleShape(top, top2, v104);
                        }
                        else {
                            pcs1 = nullptr;
                        }
                        bottom2.x = bottom.x;
                        bottom.y = bottom.y + 8.0;
                        bottom2.y = bottom.y;
                        bottom.z = v121;
                        bottom2.z = v119 - 1.0;
                        bhkCapsuleShape* v49 =
                            static_cast<bhkCapsuleShape*>(NiMemObject::operator new(sizeof(bhkCapsuleShape)));
                        v125 = (void*)v49;
                        if (v49) {
                            float v105 = v119 - 1.0;
                            pcs2 = (bhkShape*)::new (v49) bhkCapsuleShape(bottom, bottom2, v105);
                        }
                        else {
                            pcs2 = nullptr;
                        }
                        if (pcs0) {
                            phkObject = static_cast<hkpShape*>(pcs0->phkObject);
                        }
                        else {
                            phkObject = nullptr;
                        }
                        hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
                        if (lsi.m_childShapes.m_size ==
                            (lsi.m_childShapes.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*m_heap, &lsi.m_childShapes, sizeof(const hkpShape*));
                        }
                        lsi.m_childShapes.m_data[lsi.m_childShapes.m_size++] = phkObject;
                        if (pcs1) {
                            v53 = static_cast<hkpShape*>(pcs1->phkObject);
                        }
                        else {
                            v53 = nullptr;
                        }
                        hkMemoryAllocator* v54 = (&hkMemoryRouter::getInstance())->m_heap;
                        if (lsi.m_childShapes.m_size ==
                            (lsi.m_childShapes.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*v54, &lsi.m_childShapes, sizeof(const hkpShape*));
                        }
                        lsi.m_childShapes.m_data[lsi.m_childShapes.m_size++] = v53;
                        if (pcs2) {
                            v55 = static_cast<hkpShape*>(pcs2->phkObject);
                        }
                        else {
                            v55 = nullptr;
                        }
                        hkMemoryAllocator* v56 = (&hkMemoryRouter::getInstance())->m_heap;
                        if (lsi.m_childShapes.m_size ==
                            (lsi.m_childShapes.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*v56, &lsi.m_childShapes, sizeof(const hkpShape*));
                        }
                        lsi.m_childShapes.m_data[lsi.m_childShapes.m_size++] = v55;
                        hkMemoryAllocator* v57 = (&hkMemoryRouter::getInstance())->m_heap;
                        if (lsi.m_childShapeFilterInfo.m_size ==
                            (lsi.m_childShapeFilterInfo.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*v57, &lsi.m_childShapeFilterInfo, sizeof(uint32_t));
                        }
                        lsi.m_childShapeFilterInfo.m_data[lsi.m_childShapeFilterInfo.m_size++] = 0;
                        hkMemoryAllocator* v58 = (&hkMemoryRouter::getInstance())->m_heap;
                        if (lsi.m_childShapeFilterInfo.m_size ==
                            (lsi.m_childShapeFilterInfo.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*v58, &lsi.m_childShapeFilterInfo, sizeof(uint32_t));
                        }
                        lsi.m_childShapeFilterInfo.m_data[lsi.m_childShapeFilterInfo.m_size++] = 0;
                        hkMemoryAllocator* v59 = (&hkMemoryRouter::getInstance())->m_heap;
                        if (lsi.m_childShapeFilterInfo.m_size ==
                            (lsi.m_childShapeFilterInfo.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
                            hkArrayUtil::_reserveMore(*v59, &lsi.m_childShapeFilterInfo, sizeof(uint32_t));
                        }
                        lsi.m_childShapeFilterInfo.m_data[lsi.m_childShapeFilterInfo.m_size++] = 0;
                        stSizeInBytes->eShapeType = bhkCharacterController::ST_CAPSULE;
                        bhkListShape* v61 = static_cast<bhkListShape*>(NiMemObject::operator new(sizeof(bhkListShape)));
                        v125 = (void*)v61;
                        if (v61) {
                            v62 = ::new (v61) bhkListShape(lsi);
                        }
                        else {
                            v62 = nullptr;
                        }
                        v63 = stSizeInBytes->spShape.m_pObject == nullptr;
                        if (v63) {
                            eShapeType = stSizeInBytes->eShapeType;
                        }
                        else {
                            eShapeType = bhkCharacterController::ST_CAPSULE;
                        }
                        this->spShapes[eShapeType] = v62;
                        v65 = frad;
                        v25 = v129;
                        v66 = fBS2HkScaleSC;
                        v29 = 0.0;
                    }
                    else {
                        bottom.x = x;
                        bottom.y = y;
                        v98 = v25 + v25;
                        v29 = 0.0;
                        frad = (v26 + v24) * 0.5;
                        v65 = frad;
                        top.z = v25 - frad + top.z;
                        bottom.z = top.z * -1.0;
                        float v109 = 0.75 * frad;
                        v66 = fBS2HkScaleSC;
                        float v110 = v109 * fBS2HkScaleSC;
                        if (v110 > static_cast<double>(this->fStepHeight)) {
                            this->fStepHeight = v110;
                        }
                    }
                    v10 = v65;
                    double v69 = v25;
                    v70 = v66;
                    v71 = v69;
                    goto LABEL_83;
                }
                m_kWorldBound = arInfo.pObject->m_kWorldBound;
                if (!m_kWorldBound) {
                    m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
                }
                m_fRadius = static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
                if ((v2->iFlags & 1) != 0) {
                    v128 = static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
                    v101 = arInfo.fScale * 25.0;
                    v129 = v101;
                }
                else {
                    v129 = static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
                    v101 = arInfo.fScale * 25.0;
                    v128 = v101;
                }
                v127 = v101;
            LABEL_27:;
                v20 = NiPoint3_ZERO.y;
                v19 = NiPoint3_ZERO.x;
                top.z = NiPoint3_ZERO.z;
                y = NiPoint3_ZERO.y;
                x = NiPoint3_ZERO.x;
                goto LABEL_28;
            }
        }
        else {
            if (!arInfo.bUseDefaultShape) {
                v23 = arInfo.fScale;
                goto LABEL_26;
            }
        }
        v23 = 1.0;
    LABEL_26:;
        float v102 = v23;
        v127 = 23.0 * v102;
        v128 = 17.5 * v102;
        v129 = v102 * 64.0;
        goto LABEL_27;
    }
    v4 = this->spShapes[0].m_pObject;
    if (v4 != m_pObject) {
        if (v4) {
            NiDecRefCount(v4);
        }
        v2->spShapes[0].m_pObject = m_pObject;
        NiIncRefCount(m_pObject);
    }
    pcs = static_cast<bhkConvexShape*>(v2->GetShape());
    if (pcs && (v6 = static_cast<hkpConvexShape*>(pcs->phkObject)) != nullptr) {
        v7 = v6->m_radius;
    }
    else {
        v7 = 0.1f;
    }
    v8 = stSizeInBytes;
    v97 = v7;
    v114 = v97 * bhkConvert::fHk2BSScaleSC;
    v98 = 0.0f;
    v9 = v129;
    v10 = v114;
    v11 = fBS2HkScaleSC;
LABEL_108:;
    float v111 = v10 * v11;
    v2->fRadius = v111;
    v2->fDestRadius = v111;
    v2->fHeight = v11 * v98;
    float v112 = v9 + v9;
    v2->fActorHeight = v11 * v112;
    if (v8->spShape.m_pObject) {
        v81 = bhkCharacterController::ST_CAPSULE;
    }
    else {
        v81 = v8->eShapeType;
    }
    v2->SetShapeType(v81);
    if (v8->bAvoidBox) {
        hkMemoryRouter::getInstance();
    }
    if (v8->bCollisionBox) {
        float v103;
        hkShapePhantomCinfo sspci;
        uint32_t iFilter = v8->iColFilter.iFilter;
        NiAVObject* pObject = v8->pObject;
        sspci.m_collisionFilterInfo = (iFilter & 0xFFFFFF80u) | 32;
        sspci.m_shape = nullptr;
        if (pObject) {
            NiBound* v84 = (NiBound*)pObject->m_kWorldBound;
            if (!v84) {
                v84 = &NiAVObject::NullBoundS;
            }
            float v85 = v84->m_kCenter.x;
            bottom2.y = v84->m_kCenter.y;
            float a1 = v84->m_fRadius;
            v103 = a1;
            bottom2.x = v85;
            bottom2.z = v84->m_kCenter.z;
            if (a1 > 0.0f) {
                goto LABEL_119;
            }
        }
        else {
            hkpCharacterProxy* v96 = static_cast<hkpCharacterProxy*>(v2->phkObject);
            if (v96) {
                v2->bhkCharacterProxy::GethkWorldRw();
                v96->getPosition();
                v2->bhkCharacterProxy::GethkWorldRw();
            }
        }
        v103 = 1.0f;
    LABEL_119:;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkpSphereShape* v87 = static_cast<hkpSphereShape*>(Instance->m_heap->blockAlloc(sizeof(hkpSphereShape)));
        v87->m_memSizeAndFlags = sizeof(hkpSphereShape);
        v125 = (void*)v87;
        float afRadius = v103 * fBS2HkScaleSC;
        hkpSphereShape* pshpere = ::new (v87) hkpSphereShape(afRadius);
        sspci.m_shape = (hkpShape*)pshpere;
        // The 16-byte aligned new: 16 bytes more than the box, its start rounded up to 16 and the padding's size kept
        // in the byte just before it.
        uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkCollisionBox) + 0x10));
        const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        pMemory += ucPadding;
        pMemory[-1] = ucPadding;
        void* v91 = pMemory;
        v125 = v91;
        bhkWorldObject* v92 = (bhkWorldObject*)::new (v91) bhkCollisionBox(&sspci, v2);
        bhkWorldObject* spCollisionBox = v2->spCollisionBox;
        if (spCollisionBox != v92) {
            if (spCollisionBox) {
                NiDecRefCount(spCollisionBox);
            }
            v2->spCollisionBox = v92;
            if (v92) {
                NiIncRefCount(v92);
            }
        }
    }
}

void bhkCharacterController::ClearPush() {
    if (iPushCount) {
        PushDelta.m_quad = _mm_setzero_ps();
        iPushCount = 0;
    }
}

// Fills kFrontBack, the quadruped's support probe points: the shape's points at half height less the radius to
// the front and back, then to either side, at the depth frad, carried into the phantom's transform and raised by
// fStepHeight.
void bhkCharacterController::InitFrontBack() {
    if ((iFlags & QUADRUPED) != 0) {
        ahkpCharacterProxy* phkObject = static_cast<ahkpCharacterProxy*>(this->phkObject);
        hkpShapePhantom* ShapePhantom;
        if (phkObject) {
            ShapePhantom = phkObject->GetShapePhantom();
        }
        else {
            ShapePhantom = nullptr;
        }
        float frad = fRadius;
        // Computed on the x87 stack and stored as floats.
        float fHalfHeight = fHeight * 0.5 - frad;
        float fNegHalfHeight = fHalfHeight * -1.0;
        if (ShapePhantom->m_collidable.m_shape->m_type.m_storage != HK_SHAPE_LIST) {
            frad = 0.5 * -fRadius;
        }
        const hkTransform& rTransform = ShapePhantom->m_motionState.m_transform;
        hkVector4 kPoint;
        kPoint.set(0.0f, fHalfHeight, frad, 0.0f);
        for (int32_t i = 0; i < 2; ++i) {
            FrontBack[i].setTransformedPos(rTransform, kPoint);
            FrontBack[i].m_quad[2] = fStepHeight + FrontBack[i].m_quad[2];
            kPoint.m_quad[1] = fNegHalfHeight;
        }
        kPoint.set(fNegHalfHeight, 0.0f, frad, 0.0f);
        for (int32_t i = 2; i < 4; ++i) {
            FrontBack[i].setTransformedPos(rTransform, kPoint);
            FrontBack[i].m_quad[2] = FrontBack[i].m_quad[2] + fStepHeight;
            kPoint.m_quad[0] = fHalfHeight;
        }
    }
}

void bhkCharacterController::ChangeSize(float afTime) {
    if (eSize == bhkCharacterController::CS_SIZING) {
        bhkCapsuleShape* pcs;
        double m_radius;
        double fcurRad;
        float fcurRad_2;
        hkVector4 v21;
        bhkShape* v3 = spShapes[0];
        if (!v3 || (pcs = static_cast<bhkCapsuleShape*>(v3->DynamicCastMember(&bhkCapsuleShape::ms_RTTI))) == nullptr) {
            fcurRad_2 = fDestRadius;
        LABEL_26:;
            if (fDestRadius == fcurRad_2) {
                if (fRadius == fcurRad_2) {
                    if (eSizedShapeType != bhkCharacterController::ST_NUMTYPES) {
                        SetShapeType(eSizedShapeType);
                        eSizedShapeType = bhkCharacterController::ST_NUMTYPES;
                    }
                    eSize = bhkCharacterController::CS_NORMAL;
                }
                else {
                    eSize = bhkCharacterController::CS_SIZED;
                }
            }
            return;
        }
        if ((iFlags & bhkCharacterController::QUICK_SIMULATE) != 0) {
            afTime = 100.0f;
        }
        hkpCapsuleShape* phkObject = static_cast<hkpCapsuleShape*>(pcs->phkObject);
        if (phkObject) {
            m_radius = phkObject->m_radius;
        }
        else {
            m_radius = 0.1f;
        }
        float v19 = m_radius;
        float fraddiff = afTime * 5.0;
        double v7 = v19;
        if (!(fDestRadius < static_cast<double>(v19))) {
            float v17 = fraddiff + v7;
            if (!(fDestRadius <= static_cast<double>(v17))) {
                fcurRad = v17;
                goto LABEL_15;
            }
        }
        else {
            float v16 = v7 - fraddiff;
            if (!(fDestRadius >= static_cast<double>(v16))) {
                fcurRad = v16;
                fraddiff = -fraddiff;
                goto LABEL_15;
            }
        }
        fraddiff = fDestRadius - v7;
        fcurRad = fDestRadius;
    LABEL_15:;
        fcurRad_2 = fcurRad;
        hkVector4* p_m_vertexA = &phkObject->m_vertexA;
        if (!phkObject) {
            p_m_vertexA = &hkVector4Zero;
        }
        v21.m_quad = p_m_vertexA->m_quad;
        hkVector4* p_m_vertexB = &phkObject->m_vertexB;
        if (!phkObject) {
            p_m_vertexB = &hkVector4Zero;
        }
        __m128 m_quad = p_m_vertexB->m_quad;
        v21.m_quad[2] = v21.m_quad[2] - fraddiff;
        m_quad[2] = fraddiff + p_m_vertexB->m_quad[2];
        if (phkObject) {
            double v11 = phkObject->m_radius;
            phkObject->m_vertexA.m_quad = v21.m_quad;
            phkObject->m_vertexA.m_quad[3] = v11;
        }
        hkpCapsuleShape* v12 = static_cast<hkpCapsuleShape*>(pcs->phkObject);
        if (v12) {
            double v13 = v12->m_radius;
            v12->m_vertexB.m_quad = m_quad;
            v12->m_vertexB.m_quad[3] = v13;
        }
        hkpCapsuleShape* v14 = static_cast<hkpCapsuleShape*>(pcs->phkObject);
        if (v14) {
            v14->m_radius = fcurRad_2;
        }
        goto LABEL_26;
    }
}

void bhkCharacterController::ClearHurtfulHits() {
    while (DamageImpacts.GetNext() || DamageImpacts.m_item) {
        DamageImpactData* pImpact = DamageImpacts.m_item;
        if (pImpact) {
            pImpact->~DamageImpactData();
            MemoryManager_Singleton.Deallocate(pImpact);
        }
        BSSimpleList<DamageImpactData*>* m_pkNext = DamageImpacts.GetNext();
        if (m_pkNext) {
            DamageImpacts.m_pkNext = m_pkNext->GetNext();
            DamageImpacts.m_item = m_pkNext->m_item;
            m_pkNext->m_pkNext = nullptr;
            m_pkNext->RemoveAll();
            MemoryManager_Singleton.Deallocate(m_pkNext);
        }
        else {
            DamageImpacts.m_item = 0;
        }
    }
}

// Without the projectile flag or a shape phantom, moves the character by its velocity over the step. Otherwise the
// phantom is cast linearly to that point first, and every hit, sorted by fraction, is reported to the listener before
// the character is moved.
void bhkCharacterController::MoveAsProjectile(float afTimeDelta) {
    if ((iFlags & PROJECTILE) != 0 && spShapePhantom) {
        hkpAllCdPointCollector collector;
        hkpLinearCastInput castInput;
        hkVector4 startPos;
        GetPosition_hk(startPos);
        castInput.m_to.m_quad = _mm_add_ps(
            startPos.m_quad,
            _mm_mul_ps(OutVelocity.m_quad, _mm_shuffle_ps(_mm_set_ss(afTimeDelta), _mm_set_ss(afTimeDelta), 0))
        );
        spShapePhantom->SetPositionAndLinearCast(startPos, castInput, collector, nullptr);
        // EXE-INLINED 0x621210
        if (collector.m_hits.m_size > 0) {
            collector.sortHits();
            // The listener's callback is called through its vtable.
            hkpCharacterProxyListener* pListener = this;
            for (int32_t i = 0; i < collector.m_hits.m_size; ++i) {
                pListener->contactPointAddedCallback(nullptr, collector.m_hits[i]);
            }
        }
        SetPosition(castInput.m_to);
    }
    else {
        hkVector4 kPos;
        GetPosition_hk(kPos);
        kPos.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(afTimeDelta), _mm_set_ss(afTimeDelta), 0), OutVelocity.m_quad),
            kPos.m_quad
        );
        SetPosition(kPos);
    }
}

bhkShape* bhkCharacterController::GetShape() {
    bhkShape* const* v2;
    bhkShape* v4;
    v4 = nullptr;
    bhkCharacterController::SHAPE_TYPES eShapeType = this->eShapeType;
    if (eShapeType >= bhkCharacterController::ST_NUMTYPES) {
        v4 = nullptr;
        v2 = &v4;
    }
    else {
        v2 = &spShapes[eShapeType].m_pObject;
    }
    return *v2;
}

void bhkCharacterController::UnMarkWrite() {
    GethkWorldRw();
}

int32_t bhkCharacterController::QFarAway() {
    return GetFlag(0x40000u) != 0;
}

void bhkCharacterController::ApplyVelocityMod() {
    if (!(fVelocityTime > 0.0f)) {
        VelocityMod.m_quad = hkVector4Zero.m_quad;
    }
    else {
        if ((iFlags & (bhkCharacterController::CHASEBIP | bhkCharacterController::FOLLOWRAGDOLL)) == 0) {
            OutVelocity.m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fVelocityTime), _mm_set_ss(fVelocityTime), 0), VelocityMod.m_quad),
                OutVelocity.m_quad
            );
        }
    }
}

// Whether the contact arPoint of the character's phantom with a rigid body is a step the character can climb. A contact
// on a triangle collection (through a MOPP) whose stair callback does not call the sub-shape's material a stair is one
// at once. Otherwise the contact must lie no higher than a third of the step height above the feet (for a quadruped,
// above the nearer of its front and back support points, kFrontBack, which this fills); then the body's triangle under
// the contact is tested against a capsule cast down along the contact's horizontal direction (both capsule-triangle
// distances non-negative: a step), or, without a triangle, a ray from the feet raised by the step height is cast at the
// body's shape: no hit is a step, a hit is one unless it is penetrating. A step of a quadruped adds the contact's
// height to the nearer support point's.
bool bhkCharacterController::IsStep(const hkpRootCdPoint& arPoint) {
    // EXE-INLINED 0x4B5950
    hkpShapePhantom* pp = nullptr;
    if (arPoint.m_rootCollidableA->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        pp = static_cast<hkpShapePhantom*>(static_cast<hkpWorldObject*>(arPoint.m_rootCollidableA->getOwner()));
    }
    hkpRigidBody* prb = hkpGetRigidBody(arPoint.m_rootCollidableB);
    bool bstep = false;
    uint32_t uiclosestFBIdx = 0;
    if (prb) {
        // EXE-INLINED 0xC6EAD0
        const hkpShape* psc = nullptr;
        const hkpShape* pcurShape = arPoint.m_rootCollidableB->m_shape;
        if (pcurShape) {
            uint32_t eType = pcurShape->m_type.m_storage;
            if (eType == HK_SHAPE_MOPP) {
                pcurShape = static_cast<const hkpMoppBvTreeShape*>(pcurShape)->m_child.m_childShape;
                eType = pcurShape->m_type.m_storage;
            }
            if (eType == HK_SHAPE_TRIANGLE_COLLECTION || eType == HK_SHAPE_USER0 || eType == HK_SHAPE_USER1) {
                psc = pcurShape;
            }
        }
        const uint32_t uiShapeKey = arPoint.m_shapeKeyB;
        // EXE-INLINED 0xC6D7E0
        bool bPenetrating = true;
        if (psc) {
            bhkShape* pShape = psc->m_userData;
            if (pShape) {
                HavokMaterialType eMaterial;
                if (uiShapeKey != HK_INVALID_SHAPE_KEY) {
                    eMaterial = pShape->GetSubshapeMaterial(uiShapeKey);
                }
                else {
                    // EXE-INLINED 0x6205A0
                    eMaterial = static_cast<HavokMaterialType>(pShape->eMaterial & BHK_MATERIAL_RUBBERBALL);
                }
                bool bStairMaterial = false;
                if (pfGetMaterialStairCallback) {
                    bStairMaterial = pfGetMaterialStairCallback(eMaterial);
                }
                bPenetrating = !bStairMaterial;
            }
        }
        bstep = !bPenetrating;
    }
    if (!bstep && pp && prb) {
        const hkpShape* pShapeB = arPoint.m_rootCollidableB->m_shape;
        float frad = fRadius;
        const hkpShape* pShapeA = arPoint.m_rootCollidableA->m_shape;
        const hkpCapsuleShape* pc = nullptr;
        if (pShapeA->m_type.m_storage == HK_SHAPE_CAPSULE) {
            frad = static_cast<const hkpCapsuleShape*>(pShapeA)->m_radius;
            pc = static_cast<const hkpCapsuleShape*>(pShapeA);
        }
        hkVector4 contactPos;
        contactPos.m_quad = arPoint.m_contact.m_position.m_quad;
        const float fhitz = contactPos.m_quad[2];
        hkVector4 rayFrom;
        GetPosition_hk(rayFrom);
        if (iFlags & QUADRUPED) {
            // The front and back support points: the capsule's end points (else the controller's half height
            // ahead and behind at its radius) in the phantom's space, raised by the step height.
            hkVector4 diffs[2];
            const hkTransform& rTransform = pp->m_motionState.m_transform;
            for (uint32_t ui = 0; ui < 2; ++ui) {
                hkVector4 vert;
                if (pc) {
                    vert.m_quad = (&pc->m_vertexA)[ui].m_quad;
                }
                else {
                    const float fheight = fHeight * 0.5;
                    vert.m_quad = _mm_unpacklo_ps(
                        _mm_unpacklo_ps(_mm_set_ss(zero), _mm_set_ss(frad)),
                        _mm_unpacklo_ps(_mm_set_ss(fheight), _mm_set_ss(zero))
                    );
                    if (ui == 1) {
                        vert.m_quad[1] = vert.m_quad[1] * -1.0;
                    }
                }
                FrontBack[ui].setTransformedPos(rTransform, vert);
                FrontBack[ui].m_quad[2] = FrontBack[ui].m_quad[2] + fStepHeight;
                diffs[ui].m_quad = _mm_sub_ps(contactPos.m_quad, FrontBack[ui].m_quad);
            }
            const float dist0 = diffs[0].dot3(diffs[0]);
            const float dist1 = diffs[1].dot3(diffs[1]);
            // Picks the back point on a tie or NaN.
            uiclosestFBIdx = (dist1 > dist0) ? 0 : 1;
            rayFrom.m_quad[0] = FrontBack[uiclosestFBIdx].m_quad[0];
            rayFrom.m_quad[1] = FrontBack[uiclosestFBIdx].m_quad[1];
        }
        // The contact's horizontal direction from the feet, normalised.
        hkVector4 contactNorm;
        contactNorm.m_quad = _mm_sub_ps(rayFrom.m_quad, contactPos.m_quad);
        contactNorm.m_quad[2] = 0.0f;
        rayFrom.m_quad[2] = rayFrom.m_quad[2] + fStepHeight;
        contactNorm.normalize3();
        contactPos.m_quad[2] = rayFrom.m_quad[2];
        // How far the contact lies above the raised feet; a contact above them counts only by stair materials.
        float fhitHeight = fhitz - rayFrom.m_quad[2];
        bool bstairMaterialOnly = fhitHeight > 0.0f;
        const float fstepHeightAdjust = fStepHeight / 3.0;
        if (fstepHeightAdjust < fhitHeight) {
            if (!(iFlags & QUADRUPED)) {
                goto end;
            }
            fhitHeight = fhitz - FrontBack[uiclosestFBIdx].m_quad[2] + fStepHeight;
            if (fhitHeight > 0.0f) {
                if (fstepHeightAdjust < fhitHeight) {
                    goto end;
                }
                bstairMaterialOnly = true;
            }
        }
        const uint32_t uiKey = arPoint.m_shapeKeyB;
        const hkpShapeCollection* ps = nullptr;
        if (uiKey != HK_INVALID_SHAPE_KEY) {
            ps = GetTriCollectionFromShape(pShapeB);
        }
        if (ps) {
            // The triangle under the contact in world space against a capsule from the contact back along the
            // direction by the cast depth.
            alignas(16) char buf[512];
            const hkpTriangleShape* psub = static_cast<const hkpTriangleShape*>(ps->getChildShape(uiKey, buf));
            const hkTransform& rBody = prb->getTransform();
            hkVector4 triVertices[3];
            for (int32_t i = 2; i >= 0; --i) {
                triVertices[i].setTransformedPos(rBody, psub->getVertex(i));
            }
            hkpCollideTriangleUtil::PointTriangleDistanceCache cache;
            hkpCollideTriangleUtil::setupPointTriangleDistanceCache(triVertices, cache);
            hkVector4 endPoints[2];
            endPoints[1].m_quad = contactPos.m_quad;
            endPoints[0].m_quad = contactPos.m_quad;
            endPoints[1].m_quad[0] = endPoints[1].m_quad[0] - contactNorm.m_quad[0] * fCastDepth;
            endPoints[1].m_quad[1] = endPoints[1].m_quad[1] - contactNorm.m_quad[1] * fCastDepth;
            hkContactPoint points[3];
            hkCollideCapsuleUtilCapsVsTri(
                endPoints, 0.05f, triVertices, psub->m_radius, cache, 0.0f, 0, points, nullptr
            );
            // Each distance must be at least zero (NaN is no step).
            bstep = points[0].m_separatingNormal.m_quad[3] >= 0.0f && points[1].m_separatingNormal.m_quad[3] >= 0.0f;
        }
        else {
            // A ray from the raised feet back along the direction by the radius and cast depth, in the body's space.
            const float fcd = fCastDepth + frad;
            hkVector4 rayTo;
            rayTo.m_quad = rayFrom.m_quad;
            rayTo.m_quad[0] = rayTo.m_quad[0] - contactNorm.m_quad[0] * fcd;
            rayTo.m_quad[1] = rayTo.m_quad[1] - contactNorm.m_quad[1] * fcd;
            hkpShapeRayCastInput rci;
            rci.m_filterInfo = 0;
            rci.m_rayShapeCollectionFilter = nullptr;
            hkpShapeRayCastOutput rco;
            rco.m_hitFraction = 1.0f;
            rco.m_shapeKeyIndex = 0;
            rco.m_extraInfo = -1;
            rco.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
            const hkTransform hkt = prb->getTransform();
            rci.m_from.setTransformedInversePos(hkt, rayFrom);
            rci.m_to.setTransformedInversePos(hkt, rayTo);
            pShapeB->castRay(rci, rco);
            // No hit below 1 (NaN included) is a step.
            if (!(rco.m_hitFraction < 1.0f)) {
                bstep = true;
            }
            else {
                uint32_t outputShapeKey = HK_INVALID_SHAPE_KEY;
                for (int32_t j = 0; j < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; ++j) {
                    if (rco.m_shapeKeys[j] == HK_INVALID_SHAPE_KEY) {
                        break;
                    }
                    outputShapeKey = rco.m_shapeKeys[j];
                }
                bstep = !IsShapeConsideredPenetrating(pShapeB, true, bstairMaterialOnly, outputShapeKey);
            }
        }
    }
end:
    if ((iFlags & QUADRUPED) && bstep) {
        AddSupportHeight(uiclosestFBIdx, arPoint.m_contact.m_position.m_quad[2]);
    }
    return bstep;
}

float bhkCharacterController::GetFallDistance() const {
    float fdistance = 0.0f;
    if (0.0f != fFallStartHeight) {
        NiPoint3 a1;
        hkVector4 a2;
        GetPosition_hk(a2);
        HK2NI(a1, a2);
        fdistance = fFallStartHeight - a1.z;
    }
    return fdistance;
}

void bhkCharacterController::MoveImmobile(MoveData& arData, bool abAllowFalling) {
    bool v4 = getState() == HK_CHARACTER_IN_AIR;
    if (abAllowFalling && v4) {
        arData.Displacement = NiPoint3_ZERO;
        Move(arData);
    }
    else {
        ChangeSize(arData.fTime);
    }
}

bool bhkCharacterController::SetShape(bhkShape* apShape, const hkVector4& a2) {
    hkpShape* v11;
    ahkpCharacterProxy* phkObject = static_cast<ahkpCharacterProxy*>(this->phkObject);
    if (!phkObject) {
        return false;
    }
    hkpCachingShapePhantom* ShapePhantom = static_cast<hkpCachingShapePhantom*>(phkObject->GetShapePhantom());
    hkpPhantom* v6 = ShapePhantom;
    if (!ShapePhantom) {
        return false;
    }
    const hkpShape* m_shape = ShapePhantom->m_collidable.m_shape;
    hkReferencedObject* v8 = apShape ? apShape->phkObject : nullptr;
    if (m_shape == v8) {
        return false;
    }
    hkpCharacterProxy* v9 = static_cast<hkpCharacterProxy*>(this->phkObject);
    hkpWorld* m_world = v6->m_world;
    hkpWorld* v13 = m_world;
    if (v9) {
        bhkCharacterProxy::GethkWorldRw();
        v9->getPosition();
        bhkCharacterProxy::GethkWorldRw();
        m_world = v13;
    }
    if (m_world) {
        m_world->removePhantom(v6);
    }
    if (apShape) {
        v11 = static_cast<hkpShape*>(apShape->phkObject);
    }
    else {
        v11 = nullptr;
    }
    v6->setShape(v11);
    if (v13) {
        v13->addPhantom(v6);
    }
    return true;
}

// The controller that owns a character proxy's phantom: the proxy (the phantom's PROPERTY_KEY_CHARACTER_PROXY property)
// keeps it in its user data. Null without a phantom or the property.
bhkCharacterController* bhkCharacterController::Getbhk(hkpPhantom* apPhantom) {
    if (!apPhantom) {
        return nullptr;
    }
    // EXE-INLINED none: hkpWorldObject::hasProperty
    bool bHasProperty = false;
    for (int32_t i = 0; i < apPhantom->m_properties.m_size; ++i) {
        if (apPhantom->m_properties.m_data[i].m_key == hkpCharacterProxy::PROPERTY_KEY_CHARACTER_PROXY) {
            bHasProperty = true;
            break;
        }
    }
    if (!bHasProperty) {
        return nullptr;
    }
    ahkpCharacterProxy* pProxy =
        static_cast<ahkpCharacterProxy*>(apPhantom
                                             ->getProperty(
                                                 hkpCharacterProxy::PROPERTY_KEY_CHARACTER_PROXY,
                                                 hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE
                                             )
                                             .getPtr());
    return static_cast<bhkCharacterController*>(static_cast<bhkCharacterProxy*>(pProxy->m_userData));
}

// Takes the support surface's layer, velocity and material from the manifold's collidable B, unless the
// controller already stands on an elevator, and sets the stairs and elevator flags from the material.
void bhkCharacterController::SetSupportMaterial(const hkpRootCdPoint& arManifold) {
    if (eSupportMaterial != BHK_MATERIAL_ELEVATOR) {
        const hkpCdBody* pb = arManifold.m_rootCollidableB;
        HavokMaterialType econtactMaterial = BHK_MATERIAL_MAX;
        if (pb) {
            // EXE-INLINED 0x43B4D0
            COL_LAYER v7 =
                static_cast<COL_LAYER>(pb->getRootCollidable()->m_broadPhaseHandle.m_collisionFilterInfo & 0x7F);
            eSupportLayer = v7;
            hkpRigidBody* pBody = hkpGetRigidBody(pb->getRootCollidable());
            const hkVector4* v12 = pBody ? &pBody->getLinearVelocity() : &hkVector4Zero;
            hkSupportVelocity.m_quad = v12->m_quad;
            if (arManifold.m_shapeKeyB == -1) {
                const hkpShape* m_shape = pb->m_shape;
                if (m_shape) {
                    bhkShape* m_userData = m_shape->m_userData;
                    if (m_userData) {
                        // The material is the low five bits of eMaterial.
                        econtactMaterial = static_cast<HavokMaterialType>(m_userData->eMaterial & 0x1F);
                    }
                }
            }
            else {
                if (v7 != L_GROUND) {
                    bhkShapeCollection* psc = bhkUtilFunctions::FindbhkShapeCollection(pb);
                    if (psc) {
                        econtactMaterial = psc->QSubMaterial(arManifold.m_shapeKeyB);
                    }
                }
            }
            if (eSupportMaterial == BHK_MATERIAL_MAX || econtactMaterial == BHK_MATERIAL_ELEVATOR) {
                eSupportMaterial = econtactMaterial;
                bool MaterialStairCallback = false;
                if (bhkCharacterController::pfGetMaterialStairCallback) {
                    MaterialStairCallback = bhkCharacterController::pfGetMaterialStairCallback(econtactMaterial);
                }
                if (MaterialStairCallback) {
                    iFlags |= bhkCharacterController::ON_STAIRS;
                }
                else {
                    iFlags &= ~bhkCharacterController::ON_STAIRS;
                }
                if ((iFlags & bhkCharacterController::CHECKSUPPORT) != 0 || econtactMaterial == BHK_MATERIAL_ELEVATOR) {
                    if (eSupportMaterial == BHK_MATERIAL_ELEVATOR) {
                        iFlags |= bhkCharacterController::ON_ELEVATOR;
                    }
                    else {
                        iFlags &= ~bhkCharacterController::ON_ELEVATOR;
                    }
                }
            }
        }
    }
}

// Whether the character is inside a shape: for each collidable the shape phantom caches, skipping the layers below,
// whose shape is a BV tree, a convex-vertices shape or a MOPP, a ray is cast in the shape's space from the
// character's position to that position plus the radius and 32 units along the phantom's turned y axis, up to three
// times, raised by a quarter of the actor's height each time. When none of those hits, the reversed ray is cast the
// same way; a hit of it means inside.
bool bhkCharacterController::CheckInsideOfObject() {
    bhkCachingShapePhantom* pp = spShapePhantom;
    hkpCachingShapePhantom* pPhantom = pp ? static_cast<hkpCachingShapePhantom*>(pp->phkObject) : nullptr;
    const float fhghtAdj = fActorHeight * 0.25;
    const float fOffset = GetRadius() + fBS2HkScaleSC * 32.0;
    hkVector4 posOffset;
    posOffset.set(0.0f, fOffset, 0.0f, 0.0f);
    posOffset.setRotatedDir(pPhantom->m_motionState.m_transform.m_rotation, posOffset);
    hkVector4 worldStart;
    GetPosition_hk(worldStart);
    hkVector4 worldEnd;
    worldEnd.m_quad = _mm_add_ps(worldStart.m_quad, posOffset.m_quad);
    for (int32_t icols = 0; icols < pPhantom->m_collisionDetails.m_size; ++icols) {
        hkpCollidable* pcollidable = pPhantom->m_collisionDetails.m_data[icols].m_collidable;
        if (!pcollidable) {
            continue;
        }
        // EXE-INLINED 0x43B4D0
        const COL_LAYER elayer = static_cast<COL_LAYER>(pcollidable->m_broadPhaseHandle.m_collisionFilterInfo & 0x7F);
        switch (elayer) {
            case L_CLUTTER:
            case L_WEAPON:
            case L_PROJECTILE:
            case L_SPELL:
            case L_BIPED:
            case L_PROPS:
            case L_WATER:
            case L_TRIGGER:
            case L_CLOUDTRAP:
            case L_GROUND:
            case L_ACTORZONE:
            case L_PROJECTILEZONE:
            case L_GASTRAP:
            case L_DEADBIP:
            case L_CHARCONTROLLER: continue;
            default: break;
        }
        const hkpShape* ps = pcollidable->m_shape;
        const uint32_t etype = ps->m_type.m_storage;
        if (etype != HK_SHAPE_BV_TREE && etype != HK_SHAPE_CONVEX_VERTICES && etype != HK_SHAPE_MOPP) {
            continue;
        }
        hkpShapeRayCastInput theInput;
        theInput.m_filterInfo = 0;
        theInput.m_rayShapeCollectionFilter = nullptr;
        hkpShapeRayCastOutput theOutput;
        theInput.m_from.setTransformedInversePos(pcollidable->m_motion.pState->m_transform, worldStart);
        theInput.m_to.setTransformedInversePos(pcollidable->m_motion.pState->m_transform, worldEnd);
        bool bhit = false;
        for (uint32_t I = 0; I < 3; ++I) {
            // EXE-INLINED 0x62A2D0
            theOutput.m_hitFraction = 1.0f;
            theOutput.m_extraInfo = -1;
            theOutput.m_shapeKeys[0] = 0xFFFFFFFFu;
            theInput.m_from.m_quad[2] = theInput.m_from.m_quad[2] + fhghtAdj;
            theInput.m_to.m_quad[2] = fhghtAdj + theInput.m_to.m_quad[2];
            theOutput.m_shapeKeyIndex = 0;
            ps->castRayImpl(theInput, theOutput);
            if (theOutput.m_hitFraction < 1.0f) {
                bhit = true;
                break;
            }
        }
        theInput.m_from.setTransformedInversePos(pcollidable->m_motion.pState->m_transform, worldEnd);
        theInput.m_to.setTransformedInversePos(pcollidable->m_motion.pState->m_transform, worldStart);
        if (!bhit) {
            for (uint32_t I = 0; I < 3; ++I) {
                // EXE-INLINED 0x62A2D0
                theOutput.m_hitFraction = 1.0f;
                theOutput.m_extraInfo = -1;
                theOutput.m_shapeKeys[0] = 0xFFFFFFFFu;
                theInput.m_from.m_quad[2] = theInput.m_from.m_quad[2] + fhghtAdj;
                theInput.m_to.m_quad[2] = fhghtAdj + theInput.m_to.m_quad[2];
                theOutput.m_shapeKeyIndex = 0;
                ps->castRayImpl(theInput, theOutput);
                if (theOutput.m_hitFraction < 1.0f) {
                    return true;
                }
            }
        }
    }
    return false;
}

void bhkCharacterController::SetRotationModifier(float a1, float a2) {
    double v3 = a1;
    float a1a = std::fabs((double)a1);
    double v4 = a1a;
    float a1b = std::fabs((double)fRotMod);
    if (a1b < v4) {
        fRotMod = v3;
        fRotModTime = a2;
    }
}

void bhkCharacterController::SetMinHurtfulMass(float afValue) {
    fMinHurtfulMass = afValue;
}

void bhkCharacterController::SetMultipleStepSpeed(float afValue) {
    fMultipleStepSpeedSquared = afValue;
}

void bhkCharacterController::SetGravFrictDampSpeed(float afSpeed) {
    if (afSpeed > 0.0) {
        fGravFrictionDampSpeed = afSpeed;
    }
}

void bhkCharacterController::ReleaseDefaultShape() {
    bhkCharacterController::spDefaultConvexShape = nullptr;
}

void* bhkCharacterController::FindBSReference(uint32_t aiKey, hkpPhantom* apPhantom) {
    void* result = nullptr;
    if (apPhantom) {
        hkpPropertyValue a2 = apPhantom->getProperty(aiKey, hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE);
        return a2.getPtr();
    }
    return result;
}

void bhkCharacterController::operator delete(void* apMemory) {
    ourMemalignFree(apMemory);
}

// Whether a contact counts as penetrating. A non-penetrating one does only when only stair materials count; a
// penetrating one does unless the stair callback calls its material (the sub-shape auiShapeKey's, the whole shape's
// for HK_INVALID_SHAPE_KEY) a stair material. A shape without a game shape keeps abPenetrating.
bool bhkCharacterController::IsShapeConsideredPenetrating(
    const hkpShape* apShape,
    bool abPenetrating,
    bool abStairsMaterialOnly,
    uint32_t auiShapeKey
) {
    if (!abPenetrating) {
        return abStairsMaterialOnly;
    }
    if (!apShape) {
        return abPenetrating;
    }
    bhkShape* pShape = apShape->m_userData;
    if (!pShape) {
        return abPenetrating;
    }
    HavokMaterialType eMaterial;
    if (auiShapeKey != HK_INVALID_SHAPE_KEY) {
        eMaterial = pShape->GetSubshapeMaterial(auiShapeKey);
    }
    else {
        // EXE-INLINED 0x6205A0
        eMaterial = static_cast<HavokMaterialType>(pShape->eMaterial & BHK_MATERIAL_RUBBERBALL);
    }
    bool bStairMaterial = false;
    if (pfGetMaterialStairCallback) {
        bStairMaterial = pfGetMaterialStairCallback(eMaterial);
    }
    return !bStairMaterial;
}

// Whether a body hurts the character it hits: a dynamic body heavier than fMinHurtfulMass, or any body on the
// projectile layer.
bool bhkCharacterController::IsHurtfulBody(hkpRigidBody* apBody) {
    if (!apBody) {
        return false;
    }
    const hkpMotion::MotionType eMotionType = apBody->m_motion.m_type;
    if (bhkRigidBody::IsMotionTypeDynamic(eMotionType)) {
        const float fMass = apBody->m_motion.getMass();
        if (fMinHurtfulMass < fMass) {
            return true;
        }
    }
    // EXE-INLINED 0x43B4D0
    return static_cast<COL_LAYER>(apBody->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0x7F) == L_PROJECTILE;
}

// Builds the cinfo: the Havok proxy cinfo, the game's defaults, a fresh collision group, and with abDefaultStates a
// reference on the shared default state manager, built with its seven states on first use. The default state manager
// and its states come from the Havok heap (newObject), inside the Havok heap context, which is put back on both paths.
bhkCharacterControllerCinfo::bhkCharacterControllerCinfo(bool abDefaultStates)
  : pObject(nullptr), iColFilter(0), pWorld(nullptr), fAcrobatics(0.0f), fGravity(1.0f), bCollisionBox(false),
    bUseDefaultShape(false), bCanPitch(false), bCanRoll(false), eStartState(HK_CHARACTER_ON_GROUND), spManager(nullptr),
    fStepHeight(static_cast<float>(static_cast<double>(fBS2HkScaleSC) * 31.0)), fScale(1.0f),
    eShapeType(bhkCharacterController::ST_CONVEX), iPriority(0) {
    int32_t& riMemContext = GetThreadMemContext();
    int32_t iOldContext = riMemContext;
    riMemContext = MEM_CONTEXT_HAVOK;
    uint32_t uiGroup = (bhkCollisionFilter::uiNextGroup + 1) & 0xFFFF;
    bhkCollisionFilter::uiNextGroup = uiGroup;
    if (!uiGroup) {
        uiGroup = 10;
        bhkCollisionFilter::uiNextGroup = 10;
    }
    m_maxCharacterSpeedForSolver = 100.0f;
    iColFilter.iFilter = uiGroup << 16 | 0x1E;
    m_userPlanes = 4;
    m_position.m_quad = hkVector4Zero.m_quad;
    m_velocity.m_quad = hkVector4Zero.m_quad;
    m_staticFriction = 0.0f;
    m_extraUpStaticFriction = 0.0f;
    m_extraDownStaticFriction = 0.0f;
    m_shapePhantom = nullptr;
    m_dynamicFriction = 1.0f;
    if (!abDefaultStates) {
        riMemContext = iOldContext;
        return;
    }
    if (!pDefaultManager) {
        pDefaultManager =
            new (hkReferencedObject::newObject(sizeof(hkpCharacterStateManager))) hkpCharacterStateManager();
        hkpCharacterState* pState =
            new (hkReferencedObject::newObject(sizeof(bhkCharacterStateOnGround))) bhkCharacterStateOnGround();
        pDefaultManager->registerState(pState, HK_CHARACTER_ON_GROUND);
        pState->removeReference();
        pState = new (hkReferencedObject::newObject(sizeof(bhkCharacterStateSwimming))) bhkCharacterStateSwimming();
        pDefaultManager->registerState(pState, HK_CHARACTER_USER_STATE_0);
        pState->removeReference();
        pState = new (hkReferencedObject::newObject(sizeof(bhkCharacterStateJumping))) bhkCharacterStateJumping();
        pDefaultManager->registerState(pState, HK_CHARACTER_JUMPING);
        pState->removeReference();
        pState = new (hkReferencedObject::newObject(sizeof(bhkCharacterStateInAir))) bhkCharacterStateInAir();
        pDefaultManager->registerState(pState, HK_CHARACTER_IN_AIR);
        pState->removeReference();
        // Climbing and flying have no constructor of their own: default-initialised (no `()`), as value-initialisation
        // would zero newObject's m_memSizeAndFlags and the states would never be counted or freed.
        pState = new (hkReferencedObject::newObject(sizeof(bhkCharacterStateClimbing))) bhkCharacterStateClimbing;
        pDefaultManager->registerState(pState, HK_CHARACTER_CLIMBING);
        pState->removeReference();
        pState = new (hkReferencedObject::newObject(sizeof(bhkCharacterStateFlying))) bhkCharacterStateFlying;
        pDefaultManager->registerState(pState, HK_CHARACTER_FLYING);
        pState->removeReference();
        pState = new (hkReferencedObject::newObject(sizeof(bhkCharacterStateProjectile))) bhkCharacterStateProjectile();
        pDefaultManager->registerState(pState, HK_CHARACTER_USER_STATE_1);
        pState->removeReference();
    }
    pDefaultManager->addReference();
    spManager = pDefaultManager;
    riMemContext = iOldContext;
}

// The state manager is a plain pointer holding a reference: released here. The shape's NiPointer releases itself.
bhkCharacterControllerCinfo::~bhkCharacterControllerCinfo() {
    if (spManager) {
        spManager->removeReference();
    }
    spManager = nullptr;
}

void bhkCharacterControllerCinfo::SetGravity(float afValue) {
    fGravity = afValue;
}

void bhkCharacterControllerCinfo::SetScale(float afScale) {
    fScale = afScale;
}

void bhkCharacterControllerCinfo::SetRoll(bool abValue) {
    bCanRoll = (bool)((uint8_t)abValue);
}

void bhkCharacterControllerCinfo::SetAcrobatics(float afValue) {
    fAcrobatics = afValue;
}

void bhkCharacterControllerCinfo::SetWorld(bhkWorld* apWorld) {
    pWorld = apWorld;
}

void bhkCharacterControllerCinfo::SetPitch(bool abValue) {
    bCanPitch = (bool)((uint8_t)abValue);
}

void bhkCharacterControllerCinfo::SetUseDefaultShape(bool abSetDefault) {
    bUseDefaultShape = (bool)((uint8_t)abSetDefault);
}

void bhkCharacterControllerCinfo::SetAvoidBox(bool abTF) {
    bAvoidBox = (bool)((int8_t)abTF);
}

void bhkCharacterControllerCinfo::SetColFilter(COL_LAYER aeLayer, int32_t aiGroup) {
    (&iColFilter)->Init(aeLayer, aiGroup, P_OTHER);
}

// Drops the reference the cinfos hold on the default state manager, with the allocation charged to the Havok heap
// context: saves the thread's context, stores MEM_CONTEXT_HAVOK and restores the saved one at the end.
void bhkCharacterControllerCinfo::ReleaseDefaultManager() {
    AutoMemContext memContext(MEM_CONTEXT_HAVOK);
    if (pDefaultManager != nullptr) {
        pDefaultManager->removeReference();
    }
}

DamageImpactData::DamageImpactData() : kLocation(NiPoint3_ZERO), kNormal(NiPoint3_ZERO), kVelocity(NiPoint3_ZERO) {}

hkCachingShapePhantomCinfo::hkCachingShapePhantomCinfo() {}

void bhkCharacterProxy::CheckSupport(const hkVector4& arDirection, hkpSurfaceInfo& arGround) {
    if (this) {
        hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(this->phkObject);
        if (phkObject) {
            static_cast<bhkCharacterController*>(this)->UnMarkWrite();
            phkObject->checkSupport(arDirection, arGround);
            static_cast<bhkCharacterController*>(this)->UnMarkWrite();
        }
    }
}

void bhkCharacterProxy::Integrate(hkStepInfo& arInfo) {
    static_cast<bhkCharacterController*>(this)->bhkCharacterController::UnMarkWrite();
    if (this) {
        hkpCharacterProxy* phkObject = static_cast<hkpCharacterProxy*>(this->phkObject);
        if (phkObject) {
            bhkCharacterPointCollector castCollector(&m_ignoredCollisionStartCollector);
            const hkpWorld* pWorld = GethkWorld();
            phkObject->integrateWithCollectors(
                arInfo, pWorld->getGravity(), castCollector, m_ignoredCollisionStartCollector
            );
            m_ignoredCollisionStartCollector.handleCollisions();
        }
    }
    static_cast<bhkCharacterController*>(this)->bhkCharacterController::UnMarkWrite();
}

// The members are destroyed in reverse order: the filter infos are freed first, then the child shapes.
hkListShapeCinfo::~hkListShapeCinfo() {}

hkpSurfaceConstraintInfo*
hkpSurfaceConstraintInfo::Assign(hkpSurfaceConstraintInfo* apThis, const hkpSurfaceConstraintInfo& arOther) {
    *apThis = arOther;
    return apThis;
}

void hkpCharacterProxyCinfo::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        Instance->m_heap->blockFree(apMemory, sizeof(hkpCharacterProxyCinfo));
    }
}
