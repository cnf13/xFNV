#include "BSHavok/bhkRagdollPenetrationUtil.h"

#include "BSCore/BSPrecisionTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkAabbPhantom.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Animation/Ragdoll/Penetration/hkaDetectRagdollPenetration.h"
#include "Havok/Source/Animation/Ragdoll/Utils/hkaRagdollUtils.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

// The [RagdollAnim] ini settings Update reads: the speed a penetrating bone is pushed out at along the penetration
// normal, and the bound on the impulse that does it.
static SettingT<INISettingCollection> gs_fDesiredVel_RagdollAnim("fDesiredVel:RagdollAnim", 15.0f);
static SettingT<INISettingCollection> gs_fImpulseLimit_RagdollAnim("fImpulseLimit:RagdollAnim", 1500.0f);
SettingT<INISettingCollection>
    bhkRagdollPenetrationUtil::fDetectionUpdateTimeSec("fDetectionUpdateTimeSec:RagdollAnim", 5.0f);

// Frees the skeleton, the ragdoll, the detection util and the bounding phantom, charged to the Havok memory context.
// The ragdoll is released before the skeleton, so the ragdoll instance's destructor releases its reference on a
// skeleton that is still alive; the same objects end up freed.
bhkRagdollPenetrationUtil::ConstrainedSystem::~ConstrainedSystem() {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if (pRagdollInstance) {
        pRagdollInstance->removeReference();
    }
    if (pSkel) {
        delete pSkel;
    }
    if (pDetectionUtil) {
        pDetectionUtil->removeReference();
    }
    if (pBoundingPhantom) {
        delete pBoundingPhantom;
    }
    pTLSData->eMemContext = eMemContext;
}

// DetectionTimer's own constructor clears it.
bhkRagdollPenetrationUtil::bhkRagdollPenetrationUtil() : bAttemptToInitializeNextUpdate(0), uiInitAttempts(0) {}

bhkRagdollPenetrationUtil::~bhkRagdollPenetrationUtil() {
    Clear();
}

// Casts the ray through the phantom's overlaps from arFromWS to arToWS, and when no hit counts, back from arToWS to
// arFromWS with the shape collection filter on. A hit counts when its root collidable is a rigid body on a layer other
// than LAYER_BIPED and LAYER_DEADBIP: its point and normal go to arHitPointWS and arHitNormalWS.
hkBool bhkRagdollPenetrationUtil::castRay(
    const hkpAabbPhantom* apPhantomIn,
    const hkVector4& arFromWS,
    const hkVector4& arToWS,
    hkVector4& arHitPointWS,
    hkVector4& arHitNormalWS
) {
    bool bFound = false;
    hkpWorldRayCastInput sinput;
    hkpAllRayHitCollector rayCollector;
    // The ray is filtered as a dead biped of the util's collision group.
    const uint32_t uiFilterInfo = (iGroup << 16) | L_DEADBIP;
    for (int32_t iPass = 0; iPass < 2 && !bFound; ++iPass) {
        // EXE-INLINED 0x4A3B70
        rayCollector.m_hits.m_size = 0;
        // EXE-INLINED 0x4A3B50
        rayCollector.m_earlyOutHitFraction = 1.0f;
        if (iPass == 0) {
            sinput.m_from.m_quad = arFromWS.m_quad;
            sinput.m_to.m_quad = arToWS.m_quad;
        }
        else {
            sinput.m_from.m_quad = arToWS.m_quad;
            sinput.m_to.m_quad = arFromWS.m_quad;
        }
        sinput.m_filterInfo = uiFilterInfo;
        if (iPass != 0) {
            sinput.m_enableShapeCollectionFilter.m_bool = true;
        }
        apPhantomIn->castRay(sinput, rayCollector);
        for (int32_t i = 0; i < rayCollector.m_hits.m_size && !bFound; ++i) {
            const hkpWorldRayCastOutput& rHit = rayCollector.m_hits.m_data[i];
            const hkpCollidable* pRoot = rHit.m_rootCollidable;
            // EXE-INLINED 0x4B59F0
            if (pRoot->m_broadPhaseHandle.m_type != hkpWorldObject::BROAD_PHASE_ENTITY) {
                continue;
            }
            const hkpRigidBody* pBody =
                static_cast<const hkpRigidBody*>(static_cast<hkpWorldObject*>(pRoot->getOwner()));
            if (!pBody) {
                continue;
            }
            const uint32_t uiLayer = pBody->getCollidable()->getCollisionFilterInfo() & 0x7F;
            if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
                continue;
            }
            hkSimdReal kFraction;
            kFraction.~hkSimdReal();
            ::new (&kFraction) hkSimdReal(rHit.m_hitFraction);
            arHitPointWS.setInterpolate4(sinput.m_from, sinput.m_to, kFraction);
            arHitNormalWS.m_quad = rHit.m_normal.m_quad;
            bFound = true;
        }
    }
    return bFound;
}

uint8_t bhkRagdollPenetrationUtil::QInitialOnNextUpdate() {
    return bAttemptToInitializeNextUpdate;
}

// Builds a ConstrainedSystem for each system root under apRoot whose bodies are joined by constraints: the systems
// already watched with the same root and body count are kept, the others get a skeleton, a ragdoll instance, a bounding
// phantom over the bodies' boxes and a detection util. The watched systems not kept are deleted, the new list replaces
// the old one, and the bones the last detection left without collision get it back.
bool bhkRagdollPenetrationUtil::Initialize(NiAVObject* apRoot, int32_t aiGroup) {
    iGroup = aiGroup;
    using SystemRootArray = BSSimpleArray<NiAVObject*, 1024>;
    SystemRootArray systemRoots;
    FindSystems(apRoot, systemRoots);
    using SystemArray = BSSimpleArray<ConstrainedSystem*, 1024>;
    SystemArray newSystems;
    bool bragdollFail = false;
    for (uint32_t ui = 0; ui < systemRoots.iSize; ++ui) {
        hkLocalArray<hkpRigidBody*> rbarray(64);
        hkLocalArray<hkpConstraintInstance*> constarray(64);
        FindBones(systemRoots.pBuffer[ui], rbarray, constarray);

        if (constarray.m_size != 0) {
            // A system already watched with the same root and body count is kept.
            bool bsystemMatch = false;
            for (uint32_t uj = 0; uj < ActiveSystems.iSize && !bsystemMatch; ++uj) {
                ConstrainedSystem** ppSystem = &ActiveSystems.pBuffer[uj];
                if (systemRoots.pBuffer[ui] == (*ppSystem)->pRoot3d &&
                    rbarray.m_size == (*ppSystem)->pRagdollInstance->m_rigidBodies.m_size) {
                    // EXE-INLINED none: BSSimpleArray<bhkRagdollPenetrationUtil::ConstrainedSystem *,1024>::Add
                    if (newSystems.iSize == newSystems.iReservedSize) {
                        if (newSystems.iReservedSize) {
                            uint32_t uiNewSize = newSystems.iReservedSize + 1024;
                            if (newSystems.iReservedSize <= 1024) {
                                uiNewSize = 2 * newSystems.iReservedSize;
                            }
                            newSystems._ReallocateBuffer(uiNewSize, newSystems.iSize);
                            newSystems.iReservedSize = uiNewSize;
                        }
                        else {
                            newSystems.pBuffer = newSystems._Allocate(4);
                            newSystems.iReservedSize = 4;
                        }
                    }
                    newSystems.pBuffer[newSystems.iSize++] = *ppSystem;
                    bsystemMatch = true;
                }
            }

            if (!bsystemMatch) {
                using LeafArray = BSSimpleArray<bhkRigidBody*, 1024>;
                LeafArray systemLeaves;
                FindLeaves(rbarray, systemLeaves);
                ConstrainedSystem* pconstrainedSystem =
                    static_cast<ConstrainedSystem*>(MemoryManager_Singleton.Allocate(sizeof(ConstrainedSystem)));
                if (pconstrainedSystem) {
                    // EXE-INLINED none: bhkRagdollPenetrationUtil::ConstrainedSystem::ConstrainedSystem
                    pconstrainedSystem->pRoot3d = nullptr;
                    pconstrainedSystem->pRagdollInstance = nullptr;
                    pconstrainedSystem->pSkel = nullptr;
                    pconstrainedSystem->pBoundingPhantom = nullptr;
                    pconstrainedSystem->pDetectionUtil = nullptr;
                    pconstrainedSystem->PenetratedBones.m_penetrations.m_data = nullptr;
                    pconstrainedSystem->PenetratedBones.m_penetrations.m_size = 0;
                    pconstrainedSystem->PenetratedBones.m_penetrations.m_capacityAndFlags =
                        hkArrayBase<hkaDetectRagdollPenetration::BonePenetration>::DONT_DEALLOCATE_FLAG;
                    pconstrainedSystem->bPenetrating = true;
                }
                if (hkaRagdollUtils::reorderForRagdoll(rbarray, constarray) == HK_FAILURE) {
                    bragdollFail = true;
                }
                else {
                    pconstrainedSystem->pRoot3d = systemRoots.pBuffer[ui];
                    pconstrainedSystem->pSkel = CreateSkeletonForRagdoll(rbarray, constarray, systemLeaves);
                    if (pconstrainedSystem->pSkel) {
                        pconstrainedSystem->pRagdollInstance = hkaRagdollUtils::createRagdollInstanceFromSkeleton(
                            pconstrainedSystem->pSkel, rbarray, constarray
                        );
                    }
                    if (pconstrainedSystem->pRagdollInstance) {
                        // The box around every body that has a shape, from the world's broadphase.
                        hkpRigidBody* pFirstBody = pconstrainedSystem->pRagdollInstance->m_rigidBodies.m_data[0];
                        hkpWorld* pworld = pFirstBody ? pFirstBody->m_world : nullptr;
                        hkVector4 maxExtent;
                        maxExtent.setAll(rdata_104E148);
                        hkVector4 minExtent;
                        minExtent.setAll(rdata_104DA48);
                        hkAabb ragdollAabb;
                        for (int32_t i = 0; i < rbarray.m_size; ++i) {
                            hkpRigidBody* prb = rbarray.m_data[i];
                            if (prb->m_collidable.getShape()) {
                                pworld->getBroadPhase()->getAabb(&prb->m_collidable.m_broadPhaseHandle, ragdollAabb);
                                maxExtent.setMax4(maxExtent, ragdollAabb.m_max);
                                minExtent.setMin4(minExtent, ragdollAabb.m_min);
                            }
                        }
                        ragdollAabb.m_min = minExtent;
                        ragdollAabb.m_max = maxExtent;

                        // The bounding phantom, in the ragdoll's system group on the char controller layer.
                        hkAabbPhantomCinfo cinfo;
                        cinfo.m_collisionFilterInfo = iGroup << 16 | L_CHARCONTROLLER;
                        cinfo.aabb.m_min = minExtent;
                        cinfo.aabb.m_max = maxExtent;
                        bhkAabbPhantom* pPhantom =
                            static_cast<bhkAabbPhantom*>(NiMemObject::operator new(sizeof(bhkAabbPhantom)));
                        if (pPhantom) {
                            pPhantom = ::new (pPhantom) bhkAabbPhantom(cinfo);
                        }
                        pconstrainedSystem->pBoundingPhantom = pPhantom;
                        hkpRigidBody* pWorldBody = pconstrainedSystem->pRagdollInstance->m_rigidBodies.m_data[0];
                        if (pWorldBody && pWorldBody->m_world) {
                            bhkWorld* pbhkWorld = nullptr;
                            if (pWorldBody && pWorldBody->m_world) {
                                pbhkWorld =
                                    static_cast<bhkWorld*>(static_cast<ahkpWorld*>(pWorldBody->m_world)->m_userData);
                            }
                            if (pPhantom) {
                                pPhantom->Add(pbhkWorld);
                            }
                        }

                        hkaDetectRagdollPenetration::Setup setup;
                        setup.m_ragdollSkeleton = pconstrainedSystem->pRagdollInstance->m_skeleton;
                        setup.m_raycastInterface = this;
                        setup.m_ragdollPhantom =
                            static_cast<hkpAabbPhantom*>(pconstrainedSystem->pBoundingPhantom->phkObject);
                        void* pMemory =
                            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkaDetectRagdollPenetration));
                        static_cast<hkaDetectRagdollPenetration*>(pMemory)->m_memSizeAndFlags =
                            sizeof(hkaDetectRagdollPenetration);
                        hkaDetectRagdollPenetration* pDetectionUtil =
                            ::new (pMemory) hkaDetectRagdollPenetration(&setup);
                        pconstrainedSystem->pDetectionUtil = pDetectionUtil;
                        pDetectionUtil->resetBoneStates();
                        // The interval in milliseconds is rounded to a float before it is scaled by the counter
                        // frequency.
                        float fIntervalMS = static_cast<float>(fDetectionUpdateTimeSec.uValue.f * 1000.0);
                        DetectionTimer.iStartTime = BSPrecisionTimer::GetTimer();
                        DetectionTimer.iTargetTime = static_cast<int64_t>(fIntervalMS * BSPrecisionTimer::fFrequencyMS);
                    }
                }
                if (pconstrainedSystem->pBoundingPhantom && pconstrainedSystem->pRoot3d &&
                    pconstrainedSystem->pDetectionUtil && pconstrainedSystem->pRagdollInstance &&
                    pconstrainedSystem->pSkel) {
                    uint32_t uiIndex = newSystems.AddUninitialized();
                    newSystems.pBuffer[uiIndex] = pconstrainedSystem;
                }
                else {
                    pconstrainedSystem->~ConstrainedSystem();
                    MemoryManager_Singleton.Deallocate(pconstrainedSystem);
                }
            }
        }
    }

    // The watched systems not kept are deleted.
    for (uint32_t ui = 0; ui < ActiveSystems.iSize; ++ui) {
        ConstrainedSystem** ppSystem = &ActiveSystems.pBuffer[ui];
        // EXE-INLINED none: BSSimpleArray<bhkRagdollPenetrationUtil::ConstrainedSystem *,1024>::Find
        int32_t iIndex = -1;
        for (uint32_t uk = 0; uk < newSystems.iSize; ++uk) {
            if (newSystems.pBuffer && newSystems.pBuffer[uk] == *ppSystem) {
                iIndex = static_cast<int32_t>(uk);
                break;
            }
        }
        if (iIndex == -1) {
            ConstrainedSystem* pSystem = *ppSystem;
            if (pSystem) {
                pSystem->~ConstrainedSystem();
                MemoryManager_Singleton.Deallocate(pSystem);
            }
        }
    }
    newSystems.CopyTo(&ActiveSystems);
    const bool bsystemsExist = ActiveSystems.iSize > 0;
    bAttemptToInitializeNextUpdate = !bsystemsExist && !bragdollFail;
    if (bAttemptToInitializeNextUpdate) {
        if (++uiInitAttempts > 5) {
            bAttemptToInitializeNextUpdate = 0;
            uiInitAttempts = 0;
        }
    }
    else {
        uiInitAttempts = 0;
    }

    // Every bone of the watched systems gets its collision back (the no-collision bit 0x4000 of the filter info).
    for (uint32_t ui = 0; ui < ActiveSystems.iSize; ++ui) {
        for (int32_t i = 0; i < ActiveSystems.pBuffer[ui]->pRagdollInstance->m_skeleton->m_bones.m_size; ++i) {
            hkaRagdollInstance* pInstance = ActiveSystems.pBuffer[ui]->pRagdollInstance;
            const int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[i];
            if (iBody < 0) {
                continue;
            }
            hkpRigidBody* prb = pInstance->m_rigidBodies.m_data[iBody];
            if (!prb) {
                continue;
            }
            bhkWorldObject* pbhkRB = prb->getUserData();
            if (!pbhkRB) {
                continue;
            }
            hkpWorldObject* pObject = static_cast<hkpWorldObject*>(pbhkRB->phkObject);
            // The collidable's address is tested for null at the read and at the store, as the game does. clang assumes
            // a member's address is non-null and drops those tests; the empty asm hides the pointer from it.
            uint32_t uiFilter = 0;
            if (pObject) {
                const hkpCollidable* pcol = pObject->getCollidable();
                __asm__("" : "+r"(pcol));
                if (pcol) {
                    uiFilter = pcol->getCollisionFilterInfo();
                }
            }
            if ((uiFilter & 0x4000) != 0) {
                uiFilter &= ~0x4000u;
                if (pObject) {
                    hkpCollidable* pCollidableRw = &pObject->m_collidable;
                    __asm__("" : "+r"(pCollidableRw));
                    if (pCollidableRw) {
                        pCollidableRw->m_broadPhaseHandle.m_collisionFilterInfo = uiFilter;
                    }
                }
                pbhkRB->UpdateCFilter();
            }
        }
    }

    return bsystemsExist;
}

// For each watched ragdoll with a body in a world and no keyframed or fixed body, while it penetrates or within the
// detection interval: the pose from the bodies, the bounding phantom moved to the bodies' world, then the detection.
// The bones found penetrating lose collision (the no-collision bit 0x4000 of the filter info, CFilter::GetNoCollision)
// and those of status HK_YESP are pushed along the penetration normal; once the ragdoll is out, collision comes back.
void bhkRagdollPenetrationUtil::Update() {
    for (uint32_t i = 0; i < ActiveSystems.iSize; ++i) {
        ConstrainedSystem* pSystem = ActiveSystems.pBuffer[i];
        hkaRagdollInstance* pRagdollInstance = pSystem->pRagdollInstance;
        const int32_t iNumBodies = pRagdollInstance->m_rigidBodies.m_size;
        bool bKeyframed = false;
        for (int32_t j = 0; j < iNumBodies && !bKeyframed; ++j) {
            const uint8_t ucType = pRagdollInstance->m_rigidBodies.m_data[j]->m_motion.m_type.m_storage;
            bKeyframed = ucType == hkpMotion::MOTION_KEYFRAMED || ucType == hkpMotion::MOTION_FIXED;
        }
        hkpRigidBody* pFirstBody = pRagdollInstance->m_rigidBodies.m_data[0];
        if (!pFirstBody || !pFirstBody->m_world || bKeyframed ||
            (!pSystem->bPenetrating &&
             BSPrecisionTimer::GetTimer() - DetectionTimer.iStartTime > DetectionTimer.iTargetTime)) {
            ConstrainedSystem* pIdleSystem = ActiveSystems.pBuffer[i];
            hkpRigidBody* pIdleBody = pIdleSystem->pRagdollInstance->m_rigidBodies.m_data[0];
            if (pIdleBody && pIdleBody->m_world) {
                pIdleSystem->pDetectionUtil->resetBoneStates();
            }
            continue;
        }
        if (BSPrecisionTimer::GetTimer() - DetectionTimer.iStartTime <= DetectionTimer.iTargetTime ||
            ActiveSystems.pBuffer[i]->bPenetrating) {
            ActiveSystems.pBuffer[i]->pDetectionUtil->resetBoneStates();
        }

        hkaPose kPose;
        // EXE-INLINED none: hkaPose::hkaPose(const hkaSkeleton*), the members its default constructor leaves unset
        kPose.m_skeleton = ActiveSystems.pBuffer[i]->pRagdollInstance->m_skeleton;
        kPose.m_modelInSync.m_bool = 0;
        kPose.m_localInSync.m_bool = 0;
        const int32_t iNumBones = kPose.m_skeleton->m_bones.m_size;
        // EXE-INLINED none: hkArray<hkQsTransform>::setSize
        kPose.m_modelPose.reserve(iNumBones);
        kPose.m_modelPose.setSizeUnchecked(iNumBones);
        // EXE-INLINED none: hkArray<hkQsTransform>::setSize
        kPose.m_localPose.reserve(iNumBones);
        kPose.m_localPose.setSizeUnchecked(iNumBones);
        // EXE-INLINED none: hkArray<uint32_t>::setSize
        kPose.m_boneFlags.reserve(iNumBones);
        kPose.m_boneFlags.setSizeUnchecked(iNumBones);
        // The float slots' storage rounded up to a multiple of four (HK_NEXT_MULTIPLE_OF), then the values zeroed.
        kPose.m_floatSlotValues.reserveExactly((kPose.m_skeleton->m_floatSlots.m_size + 3) & ~3);
        const int32_t iNumFloatSlots = kPose.m_skeleton->m_floatSlots.m_size;
        // EXE-INLINED none: hkArray<float>::setSize(int, const float&)
        kPose.m_floatSlotValues.reserve(iNumFloatSlots);
        for (int32_t k = kPose.m_floatSlotValues.m_size; k < iNumFloatSlots; ++k) {
            kPose.m_floatSlotValues.m_data[k] = 0.0f;
        }
        kPose.m_floatSlotValues.setSizeUnchecked(iNumFloatSlots);

        hkArray<hkQsTransform>& rModelPose = kPose.accessUnsyncedPoseModelSpace();
        ActiveSystems.pBuffer[i]->pRagdollInstance->getPoseWorldSpace(rModelPose.m_data);

        // The bounding phantom follows the bodies into their world: the bhkWorld the Havok world's user data names.
        ConstrainedSystem* pPhantomSystem = ActiveSystems.pBuffer[i];
        bhkAabbPhantom* pBoundingPhantom = pPhantomSystem->pBoundingPhantom;
        hkpRigidBody* pBody0 = pPhantomSystem->pRagdollInstance->m_rigidBodies.m_data[0];
        const hkpWorld* pBodyWorld = pBody0 ? pBody0->m_world : nullptr;
        if (pBodyWorld != pBoundingPhantom->GethkWorld()) {
            ConstrainedSystem* pAddSystem = ActiveSystems.pBuffer[i];
            hkpRigidBody* pAddBody = pAddSystem->pRagdollInstance->m_rigidBodies.m_data[0];
            bhkWorld* pWorld = nullptr;
            if (pAddBody && pAddBody->m_world) {
                pWorld = static_cast<bhkWorld*>(static_cast<ahkpWorld*>(pAddBody->m_world)->m_userData);
            }
            pAddSystem->pBoundingPhantom->Add(pWorld);
        }

        ConstrainedSystem* pDetectSystem = ActiveSystems.pBuffer[i];
        const bool bWasPenetrating = pDetectSystem->bPenetrating;
        const bool bPenetrating =
            pDetectSystem->pDetectionUtil->detectPenetration(kPose, pDetectSystem->PenetratedBones).m_bool != 0;
        ActiveSystems.pBuffer[i]->bPenetrating = bPenetrating;

        ConstrainedSystem* pResultSystem = ActiveSystems.pBuffer[i];
        if (pResultSystem->bPenetrating) {
            const hkArray<hkaDetectRagdollPenetration::BonePenetrationStatus>& rBoneStates =
                pResultSystem->pDetectionUtil->m_boneStates;
            hkaRagdollInstance* pInstance = pResultSystem->pRagdollInstance;
            for (int32_t iBone = 0; iBone < pInstance->m_skeleton->m_bones.m_size;
                 ++iBone, pInstance = ActiveSystems.pBuffer[i]->pRagdollInstance) {
                const int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[iBone];
                if (iBody < 0) {
                    continue;
                }
                hkpRigidBody* pBody = pInstance->m_rigidBodies.m_data[iBody];
                if (!pBody) {
                    continue;
                }
                bhkWorldObject* pWorldObject = pBody->getUserData();
                if (!pWorldObject) {
                    continue;
                }
                hkpWorldObject* pObject = static_cast<hkpWorldObject*>(pWorldObject->phkObject);
                const hkpCollidable* pCollidable;
                uint32_t uiFilter = (pObject && (pCollidable = pObject->getCollidable()) != nullptr)
                    ? pCollidable->getCollisionFilterInfo()
                    : 0;
                if (rBoneStates.m_data[iBone] == hkaDetectRagdollPenetration::HK_NOP) {
                    // A bone no longer penetrating gets its collision back.
                    if ((uiFilter & 0x4000) != 0) {
                        uiFilter &= ~0x4000u;
                        if (pObject) {
                            hkpCollidable* pCollidableRw = &pObject->m_collidable;
                            if (pCollidableRw) {
                                pCollidableRw->m_broadPhaseHandle.m_collisionFilterInfo = uiFilter;
                            }
                        }
                        pWorldObject->UpdateCFilter();
                    }
                    continue;
                }
                // A penetrating bone loses its collision.
                if ((uiFilter & 0x4000) == 0) {
                    uiFilter |= 0x4000;
                    if (pObject) {
                        hkpCollidable* pCollidableRw = &pObject->m_collidable;
                        if (pCollidableRw) {
                            pCollidableRw->m_broadPhaseHandle.m_collisionFilterInfo = uiFilter;
                        }
                    }
                    pWorldObject->UpdateCFilter();
                }
                if (rBoneStates.m_data[iBone] != hkaDetectRagdollPenetration::HK_YESP) {
                    continue;
                }
                // The push: at the penetration point found for the bone (the origin without one), along its normal (up
                // without one), by the impulse that brings the point's speed along the normal to gs_fDesiredVel,
                // bounded by gs_fImpulseLimit either way.
                hkVector4 kPoint;
                kPoint.set(0.0f, 0.0f, 0.0f, 0.0f);
                hkVector4 kNormal;
                kNormal.set(0.0f, 0.0f, 1.0f, 0.0f);
                ConstrainedSystem** ppSystem = &ActiveSystems.pBuffer[i];
                for (int32_t k = 0; k < (*ppSystem)->PenetratedBones.m_penetrations.m_size; ++k) {
                    if (iBone == (*ppSystem)->PenetratedBones.m_penetrations.m_data[k].m_boneBeginIndex) {
                        kPoint = ActiveSystems.pBuffer[i]->PenetratedBones.m_penetrations.m_data[k].m_penetrationPoint;
                        kNormal =
                            ActiveSystems.pBuffer[i]->PenetratedBones.m_penetrations.m_data[k].m_penetrationNormal;
                    }
                }
                hkVector4 kVelocity;
                pBody->getPointVelocity(kPoint, kVelocity);
                const float fSpeed = kVelocity.length3();
                float fSpeedAlongNormal;
                // HK_REAL_EPSILON; a NaN speed takes the zero.
                if (fSpeed > 1.1920929e-07f) {
                    kVelocity.normalize3();
                    fSpeedAlongNormal = kVelocity.dot3(kNormal);
                }
                else {
                    fSpeedAlongNormal = 0.0f;
                }
                const float fMass = pBody->m_motion.getMass();
                float fImpulse = (gs_fDesiredVel_RagdollAnim.uValue.f - fSpeed * fSpeedAlongNormal) * fMass;
                const float fLimit = gs_fImpulseLimit_RagdollAnim.uValue.f;
                if (fLimit < fImpulse) {
                    fImpulse = fLimit;
                }
                const float fNegativeLimit = -fLimit;
                if (fNegativeLimit > fImpulse) {
                    fImpulse = fNegativeLimit;
                }
                hkVector4 kImpulse;
                kImpulse.setAll(fImpulse);
                kImpulse.mul4(kNormal);
                pBody->applyPointImpulse(kImpulse, kPoint);
            }
        }
        else if (bWasPenetrating) {
            // Out of the geometry: every bone gets its collision back.
            pResultSystem->pDetectionUtil->resetBoneStates();
            hkaRagdollInstance* pInstance = ActiveSystems.pBuffer[i]->pRagdollInstance;
            for (int32_t iBone = 0; iBone < pInstance->m_skeleton->m_bones.m_size;
                 ++iBone, pInstance = ActiveSystems.pBuffer[i]->pRagdollInstance) {
                const int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[iBone];
                if (iBody < 0) {
                    continue;
                }
                hkpRigidBody* pBody = pInstance->m_rigidBodies.m_data[iBody];
                if (!pBody) {
                    continue;
                }
                bhkWorldObject* pWorldObject = pBody->getUserData();
                if (!pWorldObject) {
                    continue;
                }
                hkpWorldObject* pObject = static_cast<hkpWorldObject*>(pWorldObject->phkObject);
                const hkpCollidable* pCollidable;
                uint32_t uiFilter = (pObject && (pCollidable = pObject->getCollidable()) != nullptr)
                    ? pCollidable->getCollisionFilterInfo()
                    : 0;
                if ((uiFilter & 0x4000) != 0) {
                    uiFilter &= ~0x4000u;
                    if (pObject) {
                        hkpCollidable* pCollidableRw = &pObject->m_collidable;
                        if (pCollidableRw) {
                            pCollidableRw->m_broadPhaseHandle.m_collisionFilterInfo = uiFilter;
                        }
                    }
                    pWorldObject->UpdateCFilter();
                }
            }
        }
    }
}

// The interval in milliseconds is rounded to a float before it is scaled by the counter frequency.
void bhkRagdollPenetrationUtil::ResetSystems() {
    for (uint32_t i = 0; i < ActiveSystems.iSize; ++i) {
        ActiveSystems.pBuffer[i]->bPenetrating = true;
        ActiveSystems.pBuffer[i]->pDetectionUtil->resetBoneStates();
    }
    float fIntervalMS = static_cast<float>(fDetectionUpdateTimeSec.uValue.f * 1000.0);
    DetectionTimer.iStartTime = BSPrecisionTimer::GetTimer();
    DetectionTimer.iTargetTime = static_cast<int64_t>(BSPrecisionTimer::fFrequencyMS * fIntervalMS);
}

void bhkRagdollPenetrationUtil::Clear() {
    for (uint32_t ui = 0; ui < ActiveSystems.iSize; ++ui) {
        bhkRagdollPenetrationUtil::ConstrainedSystem** pBuffer = ActiveSystems.pBuffer;
        bhkRagdollPenetrationUtil::ConstrainedSystem* v4 = pBuffer[ui];
        if (v4) {
            pBuffer[ui]->~ConstrainedSystem();
            MemoryManager_Singleton.Deallocate(v4);
        }
    }
    if (ActiveSystems.pBuffer) {
        ActiveSystems._Deallocate(ActiveSystems.pBuffer);
        ActiveSystems.pBuffer = nullptr;
        ActiveSystems.iReservedSize = 0;
        ActiveSystems.iSize = 0;
    }
    DetectionTimer.iTargetTime = static_cast<int64_t>(
        (static_cast<uint64_t>(DetectionTimer.iTargetTime) & 0xFFFFFFFFull) |
        (static_cast<uint64_t>(static_cast<uint32_t>(0)) << 32)
    );
    DetectionTimer.iTargetTime = static_cast<int64_t>(
        (static_cast<uint64_t>(DetectionTimer.iTargetTime) & 0xFFFFFFFF00000000ull) |
        (static_cast<uint64_t>(static_cast<uint32_t>(0)))
    );
}

// Appends to arSystems every object under apObject, depth first, whose Havok body masters only contact constraints and
// is the slave of at least one constraint of another type.
void bhkRagdollPenetrationUtil::FindSystems(NiAVObject* apObject, BSSimpleArray<NiAVObject*>& arRoots) {
    BSSimpleArray<NiAVObject*>* apFound = &arRoots;
    if (!apObject) {
        return;
    }
    bool bMastersOnlyContacts = false;
    bool bSlaveOfOther = false;
    bhkNiCollisionObject* pNiCollisionObject = apObject->m_spCollisionObject.m_pObject;
    if (pNiCollisionObject) {
        bhkCollisionObject* pCollisionObject =
            static_cast<bhkCollisionObject*>(pNiCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
        if (pCollisionObject && pCollisionObject->spWorldObject) {
            hkpRigidBody* pRigidBody = static_cast<hkpRigidBody*>(pCollisionObject->spWorldObject->phkObject);
            if (pRigidBody) {
                bMastersOnlyContacts = true;
                for (int32_t i = 0; i < pRigidBody->m_constraintsMaster.m_size; ++i) {
                    if (!bMastersOnlyContacts) {
                        break;
                    }
                    bMastersOnlyContacts =
                        pRigidBody->m_constraintsMaster.m_data[i].m_constraint->getDataRw()->getType() ==
                        hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
                }
                for (int32_t i = 0; i < pRigidBody->m_constraintsSlave.m_size; ++i) {
                    if (bSlaveOfOther) {
                        break;
                    }
                    bSlaveOfOther = pRigidBody->m_constraintsSlave.m_data[i]->getDataRw()->getType() !=
                        hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
                }
            }
        }
    }
    if (bMastersOnlyContacts && bSlaveOfOther) {
        // EXE-INLINED 0x7CB2E0
        uint32_t uiReservedSize = apFound->iReservedSize;
        if (apFound->iSize == uiReservedSize) {
            if (!uiReservedSize) {
                NiAVObject** pNewBuffer = apFound->_Allocate(4);
                ++apFound->iSize;
                apFound->pBuffer = pNewBuffer;
                uint32_t uiSize = apFound->iSize;
                NiAVObject** pBuffer = apFound->pBuffer;
                apFound->iReservedSize = 4;
                pBuffer[uiSize - 1] = apObject;
            }
            else {
                uint32_t uiNewSize = uiReservedSize + 1024;
                if (uiReservedSize <= 1024) {
                    uiNewSize = 2 * uiReservedSize;
                }
                apFound->_ReallocateBuffer(uiNewSize, apFound->iSize);
                apFound->iReservedSize = uiNewSize;
                apFound->pBuffer[apFound->iSize++] = apObject;
            }
        }
        else {
            apFound->pBuffer[apFound->iSize++] = apObject;
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
            FindSystems(pNode->m_kChildren.m_pBase[i].m_pObject, arRoots);
        }
    }
}

void bhkRagdollPenetrationUtil::FindLeaves(
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
    BSSimpleArray<bhkRigidBody*, 1024>& arLeaves
) {
    for (int32_t i = 0; i < arRigidBodies.m_size; ++i) {
        bool bonlyMasterContact = true;
        for (int32_t j = 0; j < arRigidBodies.m_data[i]->m_constraintsMaster.m_size && bonlyMasterContact; ++j) {
            hkpConstraintInstance* pConstraint = arRigidBodies.m_data[i]->m_constraintsMaster.m_data[j].m_constraint;
            bonlyMasterContact = pConstraint->getDataRw()->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
        }
        bool bhasSlaveNonContact = false;
        for (int32_t j = 0; j < arRigidBodies.m_data[i]->m_constraintsSlave.m_size && !bhasSlaveNonContact; ++j) {
            hkpConstraintInstance* pConstraint = arRigidBodies.m_data[i]->m_constraintsSlave.m_data[j];
            bhasSlaveNonContact = pConstraint->getDataRw()->getType() != hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
        }
        if (!bonlyMasterContact && !bhasSlaveNonContact) {
            hkpRigidBody* pBody = arRigidBodies.m_data[i];
            bhkRigidBody* prb = pBody ? static_cast<bhkRigidBody*>(pBody->m_userData) : nullptr;
            // EXE-INLINED 0x7CB2E0
            BSSimpleArray<bhkRigidBody*, 1024>* pLeaves = &arLeaves;
            if (pLeaves->iSize == pLeaves->iReservedSize) {
                bhkRigidBody** ppNewBuffer;
                if (!pLeaves->iReservedSize) {
                    ppNewBuffer = pLeaves->_Allocate(4);
                    pLeaves->iReservedSize = 4;
                }
                else {
                    uint32_t uiNewSize =
                        pLeaves->iReservedSize > 1024 ? pLeaves->iReservedSize + 1024 : pLeaves->iReservedSize * 2;
                    if (pLeaves->pBuffer) {
                        ppNewBuffer = pLeaves->_Reallocate(pLeaves->pBuffer, uiNewSize);
                    }
                    else {
                        ppNewBuffer = pLeaves->_Allocate(uiNewSize);
                        pLeaves->iReservedSize = uiNewSize;
                    }
                    pLeaves->iReservedSize = uiNewSize;
                }
                pLeaves->pBuffer = ppNewBuffer;
            }
            ++pLeaves->iSize;
            pLeaves->pBuffer[pLeaves->iSize - 1] = prb;
        }
    }
}

// The list walk stops at the first empty item or once a breakable constraint is found.
void bhkRagdollPenetrationUtil::AddToArrays(
    bhkRigidBody* apRB,
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints
) {
    if (!apRB) {
        return;
    }
    // The property is keyed by the collision object's RTTI address. The property value is an untyped word whose low
    // part is read as the body's collision object.
    hkpPropertyValue kProperty;
    bhkNiCollisionObject* pcol = reinterpret_cast<bhkNiCollisionObject*>(static_cast<uintptr_t>(
        bhkWorldObject::GetProperty(
            &kProperty, static_cast<hkpWorldObject*>(apRB->phkObject), bhkPropertyKey(bhkNiCollisionObject::ms_RTTI)
        )
            ->m_data
    ));
    WeaponObject* pweap = nullptr;
    if (pcol) {
        pweap = static_cast<WeaponObject*>(pcol->DynamicCastMember(&WeaponObject::ms_RTTI));
    }
    bool bcontainsBreakable = false;
    hkArray<hkpConstraintInstance*> conA;
    for (BSSimpleList<NiPointer<bhkSerializable>>* plist = &apRB->kActionConstrants; plist; plist = plist->m_pkNext) {
        bhkSerializable* pItem = plist->m_item;
        if (!pItem || bcontainsBreakable) {
            break;
        }
        bhkConstraint* pcon = pItem->IsKindOf(&bhkConstraint::ms_RTTI) ? static_cast<bhkConstraint*>(pItem) : nullptr;
        if (!pcon) {
            continue;
        }
        // EXE-INLINED 0x887A10
        hkpConstraintData* pData;
        hkpConstraintInstance* pInstance = static_cast<hkpConstraintInstance*>(pcon->phkObject);
        if (pInstance) {
            pData = pInstance->getDataRw();
        }
        else if (pcon->pInfo) {
            pData = static_cast<hkConstraintCinfo*>(pcon->pInfo)->pData;
        }
        else {
            pData = nullptr;
        }
        if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE) {
            bcontainsBreakable = true;
            continue;
        }
        hkpConstraintInstance* pConstraint = static_cast<hkpConstraintInstance*>(pcon->phkObject);
        // EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        if (conA.m_size == (conA.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*pHeap, &conA, sizeof(hkpConstraintInstance*));
        }
        conA.m_data[conA.m_size] = pConstraint;
        conA.m_size += 1;
    }
    if ((!pweap || conA.m_size > 0) && !bcontainsBreakable) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(apRB->phkObject);
        // EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
        hkMemoryAllocator* pBodyHeap = hkMemoryRouter::getInstance().m_heap;
        if (arRigidBodies.m_size == (arRigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*pBodyHeap, &arRigidBodies, sizeof(hkpRigidBody*));
        }
        arRigidBodies.m_data[arRigidBodies.m_size] = pBody;
        arRigidBodies.m_size += 1;
        for (int32_t i = 0; i < conA.m_size; ++i) {
            hkpConstraintInstance** ppConstraint = &conA.m_data[i];
            // EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
            hkMemoryAllocator* pConstraintHeap = hkMemoryRouter::getInstance().m_heap;
            if (arConstraints.m_size ==
                (arConstraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*pConstraintHeap, &arConstraints, sizeof(hkpConstraintInstance*));
            }
            arConstraints.m_data[arConstraints.m_size] = *ppConstraint;
            arConstraints.m_size += 1;
        }
    }
}

// Adds to arRigidBodies and arConstraints the Havok body and constraints of every blended rigid body under apObject
// that is in a world, then recurses into the children of a node.
void bhkRagdollPenetrationUtil::FindBones(
    NiAVObject* apObject,
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints
) {
    if (!apObject) {
        return;
    }
    NiObject* pCollisionObject = apObject->m_spCollisionObject.m_pObject;
    if (pCollisionObject) {
        bhkBlendCollisionObject* pcol = static_cast<bhkBlendCollisionObject*>(
            pCollisionObject->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI)
        );
        if (pcol && pcol->spWorldObject) {
            bhkRigidBody* prb = static_cast<bhkRigidBody*>(
                static_cast<NiObject*>(pcol->spWorldObject)->DynamicCastMember(&bhkRigidBody::ms_RTTI)
            );
            if (prb && static_cast<bhkWorldObject*>(prb)->GethkWorld()) {
                AddToArrays(prb, arRigidBodies, arConstraints);
            }
        }
    }
    NiNode* pnode = static_cast<NiNode*>(apObject->DynamicCastMember(&NiNode::ms_RTTI));
    if (pnode) {
        for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
            FindBones(pnode->m_kChildren.m_pBase[i].m_pObject, arRigidBodies, arConstraints);
        }
    }
}

// A new skeleton with a bone per rigid body, then a "nub" bone per leaf body. A body's parent is the body the
// constraint before it attaches as its second entity, and its reference pose is its transform relative to the parent's
// (as hkaRagdollUtils::constructSkeletonForRagdoll). A nub's parent is the last body whose Havok body is the leaf's,
// its pose the identity moved out along the leaf's shape (the capsule's far end plus its radius along its axis; twice
// the box's, convex hull's or sphere's convex radius in x), its name the leaf body's (cut to 40 characters, else "No
// Name") with " Nub". The skeleton is named after its first bone.
hkaSkeleton* bhkRagdollPenetrationUtil::CreateSkeletonForRagdoll(
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints,
    BSSimpleArray<bhkRigidBody*, 1024>& arLeaves
) {
    // EXE-INLINED 0x56D280
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    hkaSkeleton* pskeleton = static_cast<hkaSkeleton*>(pRouter->m_heap->blockAlloc(sizeof(hkaSkeleton)));
    pskeleton->m_memSizeAndFlags = sizeof(hkaSkeleton);
    ::new (pskeleton) hkaSkeleton;
    const int32_t inumRbs = arRigidBodies.m_size;
    const int32_t numBones = static_cast<int32_t>(inumRbs + arLeaves.iSize);
    // EXE-INLINED none: hkArray<hkInt16>::setSize
    hkMemoryAllocator* pParentsHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iParentsCapacity = pskeleton->m_parentIndices.m_capacityAndFlags & hkArrayBase<int16_t>::CAPACITY_MASK;
    if (iParentsCapacity < numBones) {
        int32_t iNewCapacity = 2 * iParentsCapacity;
        if (numBones >= iNewCapacity) {
            iNewCapacity = numBones;
        }
        hkArrayUtil::_reserve(*pParentsHeap, &pskeleton->m_parentIndices, iNewCapacity, sizeof(int16_t));
    }
    pskeleton->m_parentIndices.m_size = numBones;
    // EXE-INLINED none: hkArray<hkQsTransform>::setSize
    hkMemoryAllocator* pPoseHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iPoseCapacity = pskeleton->m_referencePose.m_capacityAndFlags & hkArrayBase<hkQsTransform>::CAPACITY_MASK;
    if (iPoseCapacity < numBones) {
        int32_t iNewCapacity = 2 * iPoseCapacity;
        if (numBones >= iNewCapacity) {
            iNewCapacity = numBones;
        }
        hkArrayUtil::_reserve(*pPoseHeap, &pskeleton->m_referencePose, iNewCapacity, sizeof(hkQsTransform));
    }
    pskeleton->m_referencePose.m_size = numBones;
    // EXE-INLINED none: hkArray<hkaBone>::setSize
    hkMemoryAllocator* pBonesHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iBonesCapacity = pskeleton->m_bones.m_capacityAndFlags & hkArrayBase<hkaBone>::CAPACITY_MASK;
    if (iBonesCapacity < numBones) {
        int32_t iNewCapacity = 2 * iBonesCapacity;
        if (numBones >= iNewCapacity) {
            iNewCapacity = numBones;
        }
        hkArrayUtil::_reserve(*pBonesHeap, &pskeleton->m_bones, iNewCapacity, sizeof(hkaBone));
    }
    hkaBone* pDeadBone = &pskeleton->m_bones.m_data[numBones];
    if (pskeleton->m_bones.m_size - numBones > 0) {
        int32_t iDeadBones = pskeleton->m_bones.m_size - numBones;
        do {
            pDeadBone->m_name.~hkStringPtr();
            ++pDeadBone;
            --iDeadBones;
        }
        while (iDeadBones);
    }
    hkaBone* pNewBones = &pskeleton->m_bones.m_data[pskeleton->m_bones.m_size];
    const int32_t iNewBones = numBones - pskeleton->m_bones.m_size;
    for (int32_t i = 0; i < iNewBones; ++i) {
        hkaBone* pNewBone = &pNewBones[i];
        if (pNewBone) {
            ::new (&pNewBone->m_name) hkStringPtr;
            pNewBone->m_lockTranslation.m_bool = 0;
        }
    }
    pskeleton->m_bones.m_size = numBones;
    // The leaves' parent bones, one per leaf: an hkArray(int n) of plain ints (no constructor runs). _bufAlloc may
    // round the count up, and the array takes the rounded count as its size and capacity.
    int32_t iNumLeaves = static_cast<int32_t>(arLeaves.iSize);
    // EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::hkArray(int), the storage for its elements
    hkMemoryAllocator* pLeafHeap = hkMemoryRouter::getInstance().m_heap;
    hkArray<int32_t> leafIdxA;
    if (iNumLeaves) {
        leafIdxA.m_data = pLeafHeap->_bufAlloc<int32_t>(iNumLeaves);
    }
    leafIdxA.m_size = iNumLeaves;
    if (iNumLeaves) {
        leafIdxA.m_capacityAndFlags = iNumLeaves;
    }
    else {
        leafIdxA.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
    }
    int32_t b = 0;
    if (arRigidBodies.m_size > 0) {
        do {
            int32_t parentId = -1;
            if (b) {
                hkpEntity* pParentEntity = arConstraints.m_data[b - 1]->m_entities[1];
                for (int32_t p = 0; p < b; ++p) {
                    if (arRigidBodies.m_data[p] == pParentEntity) {
                        parentId = p;
                        break;
                    }
                }
            }
            pskeleton->m_parentIndices.m_data[b] = static_cast<int16_t>(parentId);
            hkpRigidBody* rbody = arRigidBodies.m_data[b];
            for (uint32_t ui = 0; ui < arLeaves.iSize; ++ui) {
                if (rbody == arLeaves.pBuffer[ui]->phkObject) {
                    leafIdxA.m_data[ui] = b;
                }
            }
            if (parentId < 0) {
                pskeleton->m_referencePose.m_data[b].setFromTransformNoScale(rbody->m_motion.m_motionState.m_transform);
            }
            else {
                hkpRigidBody* parentRb = arRigidBodies.m_data[parentId];
                hkQsTransform boneWorldTransform;
                boneWorldTransform.setFromTransformNoScale(rbody->m_motion.m_motionState.m_transform);
                hkQsTransform parentWorldTransform;
                parentWorldTransform.setFromTransformNoScale(parentRb->m_motion.m_motionState.m_transform);
                pskeleton->m_referencePose.m_data[b].setMulInverseMul(parentWorldTransform, boneWorldTransform);
            }
            const char* rbodyName = rbody->m_name.cString();
            if (rbodyName) {
                hkStringPtr* pBoneName = &pskeleton->m_bones.m_data[b].m_name;
                pBoneName->operator=(hkString::strDup(rbodyName));
            }
            else {
                {
                    char assertBuf[512];
                    hkErrStream ostr(assertBuf, 512);
                    ostr << "Rigid Body name could not be found";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBACC14), assertBuf,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkRagdollPenetrationUtil.cpp", 721
                    );
                }
                pskeleton->m_bones.m_data[b].m_name.operator=(static_cast<const char*>(nullptr));
            }
            const bool bLockTranslation = b != 0;
            pskeleton->m_bones.m_data[b].m_lockTranslation.m_bool = bLockTranslation;
            ++b;
        }
        while (b < arRigidBodies.m_size);
    }
    for (uint32_t ui = 0; ui < arLeaves.iSize; ++ui) {
        const int32_t iskeletonBoneIdx = inumRbs + static_cast<int32_t>(ui);
        hkpRigidBody* prb = static_cast<hkpRigidBody*>(arLeaves.pBuffer[ui]->phkObject);
        const int32_t inumbIdx = leafIdxA.m_data[ui];
        const hkpShape* pshape = prb->m_collidable.m_shape;
        if (!pshape) {
            continue;
        }
        const uint32_t eShapeType = pshape->m_type.m_storage;
        hkVector4 identityTranslation;
        identityTranslation.setZero4();
        alignas(16) hkVector4 boneTranslation;
        boneTranslation.setZero4();
        if (eShapeType == HK_SHAPE_CAPSULE) {
            const hkpCapsuleShape* pcap = static_cast<const hkpCapsuleShape*>(pshape);
            hkVector4 vertex1 = pcap->m_vertexA;
            hkVector4 vertex2 = pcap->m_vertexB;
            const float fLength1 = vertex1.length4();
            const float fLength2 = vertex2.length4();
            // Tests the second length below the first: an unordered compare takes vertex2.
            hkVector4 endVertex = (fLength2 < fLength1) ? vertex1 : vertex2;
            const float fRadius = pcap->m_radius;
            hkVector4 capsuleDiff;
            capsuleDiff.setSub4(vertex1, vertex2);
            capsuleDiff.normalize3();
            hkSimdReal kRadius;
            kRadius.~hkSimdReal();
            ::new (&kRadius) hkSimdReal(fRadius);
            boneTranslation.setAddMul4(endVertex, capsuleDiff, kRadius);
        }
        if (eShapeType == HK_SHAPE_BOX) {
            const hkpBoxShape* pbox = static_cast<const hkpBoxShape*>(pshape);
            const float fRadius = pbox->m_radius;
            boneTranslation(0) = static_cast<float>(fRadius * 2.0);
        }
        if (eShapeType == HK_SHAPE_CONVEX_VERTICES) {
            const hkpConvexVerticesShape* pcvs = static_cast<const hkpConvexVerticesShape*>(pshape);
            const float fRadius = pcvs->m_radius;
            boneTranslation(0) = static_cast<float>(fRadius * 2.0);
        }
        if (eShapeType == HK_SHAPE_SPHERE) {
            const hkpSphereShape* psphere = static_cast<const hkpSphereShape*>(pshape);
            const float fRadius = psphere->m_radius;
            boneTranslation(0) = static_cast<float>(2.0 * fRadius);
        }
        alignas(16) char boneName[45];
        const char* pcBodyName = prb->m_name.cString();
        if (pcBodyName) {
            strncpy_s(boneName, 41, pcBodyName, 40);
            boneName[40] = 0;
        }
        else {
            strcpy_s(boneName, 40, "No Name");
        }
        strcat_s(boneName, 45, " Nub");
        hkStringPtr* pNubName = &pskeleton->m_bones.m_data[iskeletonBoneIdx].m_name;
        pNubName->operator=(hkString::strDup(boneName));
        pskeleton->m_bones.m_data[iskeletonBoneIdx].m_lockTranslation.m_bool = 1;
        pskeleton->m_parentIndices.m_data[iskeletonBoneIdx] = static_cast<int16_t>(inumbIdx);
        hkQsTransform* pNubPose = &pskeleton->m_referencePose.m_data[iskeletonBoneIdx];
        // EXE-INLINED none: hkQsTransform::setIdentity
        pNubPose->m_translation = identityTranslation;
        pNubPose->m_rotation.setIdentity();
        pNubPose->m_scale.setAll(1.0f);
        pskeleton->m_referencePose.m_data[iskeletonBoneIdx].m_translation = boneTranslation;
    }
    if (pskeleton->m_bones.m_size == 0) {
        pskeleton->m_name.operator=(static_cast<const char*>(nullptr));
    }
    else {
        pskeleton->m_name.operator=(pskeleton->m_bones.m_data[0].m_name);
    }
    return pskeleton;
}

// The utility lives in a 16-byte aligned block.
void* bhkRagdollPenetrationUtil::operator new(size_t blocksize) {
    return ourMemalign(16, blocksize);
}

void bhkRagdollPenetrationUtil::operator delete(void* apMemory) {
    if (apMemory) {
        uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
        MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
    }
}

void hkaRagdollRaycastInterface::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
