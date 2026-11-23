#include "BSHavok/bhkBlendCollisionObject.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSUtilities.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cstdint>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkBlendCollisionObject::ms_RTTI("bhkBlendCollisionObject", &bhkCollisionObject::ms_RTTI);

void SetStoredWorldRecurse(NiAVObject* apObject, bhkWorld* apWorld) {
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            bhkCollisionObject* v3 = static_cast<bhkCollisionObject*>(
                m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI)
            );
            if (v3) {
                bhkBlendCollisionObject* v4 = v3->IsBhkBlendCollisionObject();
                if (v4) {
                    v4->pStoredWorld = apWorld;
                }
            }
        }
        NiNode* v5 = apObject->IsNode();
        if (v5) {
            uint32_t v7 = 0;
            if (v5->m_kChildren.m_usSize) {
                do {
                    SetStoredWorldRecurse(v5->m_kChildren.m_pBase[v7++].m_pObject, apWorld);
                }
                while (v7 < v5->m_kChildren.m_usSize);
            }
        }
    }
}

// The motion type before any blend is none (MOTION_MAX_ID).
bhkBlendCollisionObject::bhkBlendCollisionObject()
  : bRagdollUpdated(false), fHeirGain(0.0f), fVelGain(1.0f), ePrevType(hkpMotion::MOTION_MAX_ID), pStoredWorld(nullptr),
    iForceAddCount(0) {
    ++iCount;
    usFlags &= ~BLENDPOS_POS;
}

bhkBlendCollisionObject::~bhkBlendCollisionObject() {
    --iCount;
}

NiObject* bhkBlendCollisionObject::CreateClone(NiCloningProcess& arCloneData) {
    bhkBlendCollisionObject* pkClone = new bhkBlendCollisionObject;
    CopyMembers(pkClone, arCloneData);
    pkClone->fVelGain = fVelGain;
    pkClone->fHeirGain = fHeirGain;
    pkClone->pStoredWorld = pStoredWorld;
    return pkClone;
}

void bhkBlendCollisionObject::LoadBinary(NiStream& arStream) {
    bhkCollisionObject::LoadBinary(arStream);
    NiStreamLoadBinary(arStream, fHeirGain);
    NiStreamLoadBinary(arStream, fVelGain);
    // Older files carry two more floats that the object does not keep.
    if (arStream.m_BSStreamHeader.uiVersion < 8) {
        float fUnused;
        NiStreamLoadBinary(arStream, fUnused);
        NiStreamLoadBinary(arStream, fUnused);
    }
}

void bhkBlendCollisionObject::LinkObject(NiStream& arStream) {
    hkpLinkedCollidable* pCollidable;
    uint32_t v5;
    bhkCollisionObject::LinkObject(arStream);
    usFlags |= SETLOCAL_POS | USEVEL_POS;
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
    if (phkObject && (pCollidable = &phkObject->m_collidable) != nullptr) {
        v5 = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
    }
    else {
        v5 = 0;
    }
    int32_t v6 = v5 >> 8 & 31;
    fHeirGain = bhkBlendCollisionObject::pDefaultBlendA[v6].fGain;
    fVelGain = bhkBlendCollisionObject::pDefaultBlendA[v6].fVelGain;
}

bool bhkBlendCollisionObject::RegisterStreamables(NiStream& arStream) {
    return bhkCollisionObject::RegisterStreamables(arStream);
}

void bhkBlendCollisionObject::SaveBinary(NiStream& arStream) {
    bhkCollisionObject::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, fHeirGain);
    NiStreamSaveBinary(arStream, fVelGain);
}

bool bhkBlendCollisionObject::IsEqual(NiObject* apObject) {
    bool result = bhkCollisionObject::IsEqual(apObject);
    if (result) {
        bool v4 = fVelGain == static_cast<bhkBlendCollisionObject*>(apObject)->fVelGain;
        return fHeirGain == static_cast<bhkBlendCollisionObject*>(apObject)->fHeirGain && v4;
    }
    return result;
}

void bhkBlendCollisionObject::AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkCollisionObject::AddViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    pcString = NiGetViewerString("fHeirGain", fHeirGain);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("fVelGain", fVelGain);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bBlendPos", ((usFlags >> 8) & 1) != 0);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bAlwaysBlend", ((usFlags >> 9) & 1) != 0);
    apStrings->Add(pcString);
    char acBuffer[64];
    BSsprintf(acBuffer, 64, "0x%08X", pStoredWorld);
    pcString = NiGetViewerString("Stored World", static_cast<const char*>(acBuffer));
    apStrings->Add(pcString);
}

// While the thread synchronizes transforms: a ragdoll that was updated takes the Havok pose into the scene graph; a
// body flagged 0x40 or a 0x40 node flag does the same, and with SYNC_FROMHAVOK a 0x40 node flag instead copies the
// scene graph pose into Havok (clearing the bit). Otherwise a node flagged 1 lets the blend object transform the
// node, unless the body is un-flagged, the heir gain is at least 1.0 and the motion type is not 1.
void bhkBlendCollisionObject::UpdateWorldData(NiUpdateData& arUpdateData) {
    if (eHavokSyncMode == bhkNiCollisionObject::SYNC_NONE) {
        return;
    }
    if (bRagdollUpdated) {
        if (m_pkSceneObject) {
            m_pkSceneObject->UpdateWorldLoc();
        }
        bRagdollUpdated = false;
        return;
    }
    bool bUpdateWorldLoc;
    if (spWorldObject && ((spWorldObject->iFlags & bhkWorldObject::DISABLED) != 0)) {
        bUpdateWorldLoc = true;
    }
    else if ((usFlags & RESET_POS) != 0) {
        if (eHavokSyncMode != bhkNiCollisionObject::SYNC_FROMHAVOK) {
            bhkCollisionObject::NitohkTransform();
            usFlags &= ~RESET_POS;
            return;
        }
        bUpdateWorldLoc = true;
    }
    else {
        bUpdateWorldLoc = (usFlags & ACTIVE_POS) == 0;
    }
    if (!bUpdateWorldLoc) {
        if (!spWorldObject && !(fHeirGain < 1.0f) && (ePrevType != hkpMotion::MOTION_DYNAMIC)) {
            return;
        }
        hktoNiTransform();
        return;
    }
    if (m_pkSceneObject) {
        m_pkSceneObject->UpdateWorldLoc();
    }
}

void bhkBlendCollisionObject::hktoNiTransform() {
    uint32_t v23;
    hkVector4 kNiPos_8;
    hkVector4 pos;
    float fdelta = bhkWorld::fDeltaTime;
    bool v2 = 0.0f == this->fHeirGain;
    bhkRigidBody* prb = spWorldObject;
    NiAVObject* pobj = m_pkSceneObject;
    bool v4 = static_cast<uint8_t>(usFlags >> 8) & 1;
    hkpMotion::MotionType emt = hkpMotion::MOTION_DYNAMIC;
    if (v2) {
        if (eHavokSyncMode == bhkNiCollisionObject::SYNC_TOHAVOK) {
            pobj->UpdateWorldLoc();
            return;
        }
        v23 = 0.0f == this->fVelGain || v4;
    }
    else {
        int32_t v9;
        if (1.0f == this->fHeirGain) {
            bool v7 = eHavokSyncMode == bhkNiCollisionObject::SYNC_FROMHAVOK;
            emt = hkpMotion::MOTION_KEYFRAMED;
            v9 = !v7 + 1;
        }
        else {
            v9 = eHavokSyncMode == bhkNiCollisionObject::SYNC_FROMHAVOK;
        }
        v23 = v9;
    }
    hkpMotion::MotionType ePrevType = this->ePrevType;
    bool bAddToWorld = false;
    if (ePrevType != emt) {
        if (ePrevType == hkpMotion::MOTION_KEYFRAMED) {
            NitohkTransform();
        }
        else {
            if (prb) {
                prb->SetLinearVelocity(hkVector4Zero);
                prb->SetAngularVelocity(hkVector4Zero);
            }
        }
        bhkNiCollisionObject::kKeyframeLock.Lock(nullptr);
        bhkCollisionObject::SetMotionType(emt, prb, false);
        if (!--bhkNiCollisionObject::kKeyframeLock.uiLockCount) {
            bhkNiCollisionObject::kKeyframeLock.uiOwningThread = 0;
        }
        this->ePrevType = emt;
        bAddToWorld = true;
    }
    if (v23) {
        if (v23 == 1) {
            bhkCollisionObject::hktoNiTransform();
        }
        else {
            if (v23 == 2) {
                NitohkTransform();
            }
        }
    }
    else {
        double finvDelta;
        double fVelGain;
        hkQuaternion rot;
        hkQuaternion rbRot;
        hkQuaternion rbRot__rot;
        hkVector4 rbPos;
        hkRotation kFinalRot;
        hkVector4 kPos_8;
        hkRotation kInitialRot;
        pobj->UpdateWorldLoc();
        NiTransform* nit = &pobj->m_kWorld;
        XFNV_M128_F32(kNiPos_8.m_quad, 0) = pobj->m_kWorld.m_Translate.x * bhkConvert::fBS2HkScaleSC;
        XFNV_M128_F32(kNiPos_8.m_quad, 1) = pobj->m_kWorld.m_Translate.y * bhkConvert::fBS2HkScaleSC;
        XFNV_M128_F32(kNiPos_8.m_quad, 2) = bhkConvert::fBS2HkScaleSC * pobj->m_kWorld.m_Translate.z;
        XFNV_M128_F32(kNiPos_8.m_quad, 3) = 0.0f;
        NI2HK(kInitialRot, pobj->m_kWorld.m_Rotate);
        rbRot.set(kInitialRot);
        prb->GetPosition(rbPos);
        prb->GetRotation(rbRot__rot);
        __m128 v12 = _mm_shuffle_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(this->fHeirGain))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(this->fHeirGain))),
            0
        );
        float fHeirGain = this->fHeirGain;
        pos.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v12), rbPos.m_quad),
            _mm_mul_ps(kNiPos_8.m_quad, v12)
        );
        rot.setSlerp(rbRot__rot, rbRot, fHeirGain);
        rot.normalize();
        hkVector4* p_pos = &pos;
        if (!v4) {
            p_pos = &kNiPos_8;
        }
        kPos_8.m_quad = p_pos->m_quad;
        kFinalRot.set(rot);
        HK2NI(nit->m_Rotate, kFinalRot);
        HK2NI(pobj->m_kWorld.m_Translate, kPos_8);
        if (eHavokSyncMode == bhkNiCollisionObject::SYNC_BOTH) {
            SetTransform(*nit);
        }
        if (fdelta == 0.0f) {
            finvDelta = 1.0;
        }
        else {
            finvDelta = 1.0 / fdelta;
        }
        float finvDelta_2 = finvDelta;
        if (v4) {
            fVelGain = 1.0;
        }
        else {
            fVelGain = this->fVelGain;
        }
        float pHavokSyncModea = fVelGain;
        prb->ApplyKeyframe(pos, rot, finvDelta_2, pHavokSyncModea);
    }
    if (bAddToWorld) {
        bhkWorld* pStoredWorld = this->pStoredWorld;
        if (pStoredWorld && (!pStoredWorld->iBatchAdd)) {
            UpdateAddedToWorld();
        }
    }
}

// Sets the motion type of apObject's rigid body (else the collision object's own): a body on a blended layer (8 or 29)
// takes its layer's default gains, scaled by the starting gain (0 for a dynamic motion, else 1), and is woken and its
// blend controller's gains kept when it goes dynamic; any other body gets the motion type set.
void bhkBlendCollisionObject::SetMotionType(
    hkpMotion::MotionType aeType,
    bhkWorldObject* apObject,
    bool abAllowActivate
) {
    bhkRigidBody* prb;
    if (apObject) {
        prb = static_cast<bhkRigidBody*>(apObject->DynamicCastMember(&bhkRigidBody::ms_RTTI));
    }
    else {
        prb = nullptr;
    }
    float fBase;
    if (bhkRigidBody::IsMotionTypeDynamic(aeType)) {
        fBase = 0.0f;
    }
    else {
        fBase = 1.0f;
    }
    float fGain = fBase;
    float fVelGain = fBase;
    if (prb || (prb = this->spWorldObject) != nullptr) {
        hkpLinkedCollidable* pCollidable;
        uint32_t uiFilter;
        hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(prb->phkObject);
        if (phkObject && (pCollidable = &phkObject->m_collidable) != nullptr) {
            uiFilter = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
        }
        else {
            uiFilter = 0;
        }
        uint32_t uiLayer = uiFilter & 0x7F;
        if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
            uint32_t uiPart = (uiFilter >> 8) & 0x1F;
            fGain = bhkBlendCollisionObject::pDefaultBlendA[uiPart].fGain;
            if (!(fGain > 1.0f)) {
                fVelGain = bhkBlendCollisionObject::pDefaultBlendA[uiPart].fVelGain * fBase;
                fGain = fGain * fBase;
            }
            else {
                fGain = 1.0f;
                fVelGain = 1.0f;
            }
            usFlags &= ~ALWAYSBLEND_POS;
            if (bhkRigidBody::IsMotionTypeDynamic(aeType)) {
                if (abAllowActivate) {
                    hkpEntity* v12 = static_cast<hkpEntity*>(prb->phkObject);
                    if (v12) {
                        v12->activate();
                    }
                }
                NiAVObject* pav = m_pkSceneObject;
                if (pav) {
                    bhkBlendController* pcon = static_cast<bhkBlendController*>(
                        pav->GetController(&bhkBlendController::ms_RTTI)
                    );
                    if (pcon) {
                        pcon->m_usFlags &= ~(1u << bhkBlendController::RESTOREGAINS_POS);
                    }
                }
            }
        }
        else {
            if (!prb->SetMotionType(aeType) && bhkRigidBody::IsMotionTypeDynamic(aeType)) {
                fGain = 1.0f;
                fVelGain = 1.0f;
            }
        }
    }
    hkpMotion::MotionType ePrevType = this->ePrevType;
    this->fVelGain = fVelGain;
    fHeirGain = fGain;
    if (ePrevType != aeType && (usFlags & RESET_POS) == 0 && (prb->iFlags & bhkWorldObject::DISABLED) == 0) {
        ForceUpdateWorldData();
    }
    SynchronizeMotionType();
    bhkRigidBody* spWorldObject = this->spWorldObject;
    if (spWorldObject) {
        hkpLinkedCollidable* pCollidable;
        uint32_t uiFilter;
        hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
        if (phkBody && (pCollidable = &phkBody->m_collidable) != nullptr) {
            uiFilter = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
        }
        else {
            uiFilter = 0;
        }
        uint32_t uiLayer = uiFilter & 0x7F;
        if ((uiLayer == L_BIPED || uiLayer == L_DEADBIP) && pStoredWorld) {
            spWorldObject->Add(pStoredWorld);
        }
    }
}

void bhkBlendCollisionObject::CopyMembers(bhkBlendCollisionObject* apDest, NiCloningProcess& arCloneData) {
    bhkCollisionObject::CopyMembers(apDest, arCloneData);
}

void bhkBlendCollisionObject::UpdateAddedToWorld() {
    bhkRigidBody* pbody = spWorldObject;
    if (pbody) {
        hkpLinkedCollidable* pCollidable;
        uint32_t uiFilter;
        hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(pbody->phkObject);
        if (phkObject && (pCollidable = &phkObject->m_collidable) != nullptr) {
            uiFilter = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
        }
        else {
            uiFilter = 0;
        }
        uint32_t uiLayer = uiFilter & 0x7F;
        if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
            bhkWorld* pbw = pStoredWorld;
            if (pbw) {
                pbody->Add(pbw);
            }
        }
    }
}

void bhkBlendCollisionObject::ForceUpdateWorldData() {
    bhkNiCollisionObject::SYNCMODE eHavokSyncMode = bhkNiCollisionObject::eHavokSyncMode;
    if (eHavokSyncMode != bhkNiCollisionObject::SYNC_NONE) {
        if ((usFlags & RESET_POS) != 0) {
            if (eHavokSyncMode != bhkNiCollisionObject::SYNC_FROMHAVOK) {
                bhkCollisionObject::NitohkTransform();
                usFlags &= ~RESET_POS;
                return;
            }
        }
        else {
            if ((usFlags & ACTIVE_POS) != 0) {
                hktoNiTransform();
                return;
            }
        }
        NiAVObject* m_pkSceneObject = this->m_pkSceneObject;
        if (m_pkSceneObject) {
            m_pkSceneObject->UpdateWorldLoc();
        }
    }
}

void bhkBlendCollisionObject::SynchronizeMotionType() {
    bhkRigidBody* prb = spWorldObject;
    if (prb) {
        hkpRigidBody* phkObject;
        hkpRigidBody* v4;
        int32_t m_type;
        hkpMotion::MotionType ePrevType = this->ePrevType;
        if ((ePrevType == hkpMotion::MOTION_KEYFRAMED &&
             ((phkObject = static_cast<hkpRigidBody*>(prb->phkObject)) == nullptr ||
              phkObject->m_motion.m_type.m_storage != 4)) ||
            (ePrevType == hkpMotion::MOTION_DYNAMIC &&
             ((v4 = static_cast<hkpRigidBody*>(prb->phkObject)) == nullptr ||
              (((m_type = v4->m_motion.m_type.m_storage) == 4) || m_type == 5)))) {
            prb->SetMotionType(ePrevType);
        }
    }
}

// On the first count, UpdateAddedToWorld is performed.
void bhkBlendCollisionObject::IncForceAdd() {
    bool bFirst = iForceAddCount == 0;
    ++iForceAddCount;
    if (!bFirst) {
        return;
    }
    ForceUpdateWorldData();
    UpdateAddedToWorld();
}

// Once the count reaches zero, UpdateAddedToWorld is performed.
void bhkBlendCollisionObject::DecForceAdd() {
    --iForceAddCount;
    if (iForceAddCount > 0) {
        return;
    }
    iForceAddCount = 0;
    UpdateAddedToWorld();
}

// The class loader's factory.
NiObject* bhkBlendCollisionObject::CreateObject() {
    return new bhkBlendCollisionObject;
}

// The blend value of the blend collision object on apRoot, or on the child named apName; 1.0 when there is none.
float bhkBlendCollisionObject::GetBlendValue(NiAVObject* apRoot, const char* apName) {
    float fBlendValue = 1.0f;
    if (!apRoot) {
        return fBlendValue;
    }
    NiAVObject* pObject = nullptr;
    if (apName) {
        // The string's handle is AddString's result, with no default stored first.
        NiFixedString kName;
        kName.SetHandle(NiGlobalStringTable::AddString(apName));
        pObject = BSUtilities::GetObjectByName(apRoot, kName, false);
        // Releases the string's handle unless it is NULL_STRING. The reference count is stored 8 bytes before the
        // string's characters.
        if (kName.GetHandle() != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName.GetHandle() - 8)));
        }
    }
    if (!pObject) {
        pObject = apRoot;
    }
    bhkNiCollisionObject* pCollisionObject = pObject->m_spCollisionObject.m_pObject;
    if (!pCollisionObject) {
        return fBlendValue;
    }
    bhkBlendCollisionObject* pBlendObject = static_cast<bhkBlendCollisionObject*>(
        pCollisionObject->DynamicCastMember(&ms_RTTI)
    );
    if (!pBlendObject) {
        return fBlendValue;
    }
    fBlendValue = pBlendObject->fHeirGain;
    return fBlendValue;
}
