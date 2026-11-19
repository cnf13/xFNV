#include "Fallout/Magic/ReanimateEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/CommandEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "SDK/MSVC/rtti.h"

// Whether the reanimate archetype took ReanimateEffect::Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::REANIMATE, &ReanimateEffect::Instantiate);

ReanimateEffect::ReanimateEffect(MagicCaster* a1, MagicItem* a2, EffectItem* a3)
  : ActiveEffect(a1, a2, a3), pBody(nullptr), iState(0), fSaveTime(0.0f), StartPos(0.0f, 0.0f, 0.0f),
    StartRot(NiQuaternion_IDENTITY) {}

uint16_t ReanimateEffect::GetSaveSize(Actor* apActor) {
    return (uint16_t)(((int16_t)ActiveEffect::GetSaveSize(apActor)) + 36);
}

void ReanimateEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&iState, 4);
    pSaveLoadGameOLD->SaveGameDataOLD(&fSaveTime, 4);
    pSaveLoadGameOLD->SaveGameDataOLD(&StartPos, 12);
    pSaveLoadGameOLD->SaveGameDataOLD(&StartRot, 16);
}

void ReanimateEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&iState, 4);
    pSaveLoadGameOLD->LoadGameDataOLD(&fSaveTime, 4);
    pSaveLoadGameOLD->LoadGameDataOLD(&StartPos, 12);
    pSaveLoadGameOLD->LoadGameDataOLD(&StartRot, 16);
}

void ReanimateEffect::Start() {
    if (!GetCaster()) {
        Dispel(true);
        return;
    }
    Actor* pTargetActor;
    if (GetTarget()) {
        pTargetActor = GetTarget()->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    Actor* pActor = pTargetActor;
    if (!pActor || !pActor->IsDead(false)) {
        Dispel(false);
        return;
    }
    pActor->SetHavokDeath(false);
    pActor->SetLifeState(ACTOR_LIFE_STATE_REANIMATE);
    if (pActor->IsInCombat()) {
        pActor->StopCombat(nullptr);
    }
    BaseProcess* pProcess = pActor->GetCurrentAIProcess();
    if (!pProcess->GetProcessLevel()) {
        pProcess->SetCurrentlyReanimating(true);
    }
    pActor->SetGhost(true);
    MagicCaster* pCasterObj = GetCaster();
    pActor->SetHeading(pCasterObj->GetCasterStatsObject());
    if (pActor->Get3D()) {
        pActor->SetLocationOnReference(pActor->Get3D()->GetWorldTranslate());
    }
    pActor->InitHavok();
    BaseProcess* pHeadTrackProcess = pActor->GetCurrentAIProcess();
    pHeadTrackProcess->SetActionHeadTrackTarget(nullptr);
    pActor->SetWantWeaponDrawn(false);
    ActorValueOwner* pValues = pActor;
    // The base value is subtracted at double precision before the result is rounded to float.
    float fHealthDelta = (double)pValues->GetBaseActorValue(ActorValue::HEALTH) -
        (double)pValues->GetActorFloatValue(ActorValue::HEALTH);
    pActor->DamageModActorValue(ActorValue::HEALTH, fHealthDelta, nullptr);
    if (pActor->Get3D()) {
        pActor->InitAnimation();
        pActor->InitLightingPropertyPtr(pActor->Get3D());
    }
    TESPackage* pPackage = TESPackage::CreatePackage(PACKAGE_TYPE_DO_NOTHING);
    pPackage->SetPackType(PACKAGE_TYPE_DO_NOTHING);
    pPackage->SetMustReachLocation(true);
    pPackage->SetMustComplete(true);
    pPackage->SetProcedureType(PACKAGE_PROCEDURE_DO_NOTHING);
    pPackage->SetSelectiveFalloutBehavior(true);
    pActor->PutCreatedPackage(pPackage, true, true);
    NiNode* p3D = pActor->Get3D();
    if (!p3D) {
        return;
    }
    bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
    NiFixedString kBoneName("Bip01 Spine2");
    NiAVObject* pBone = p3D->GetObjectByName(kBoneName);
    if (pBone) {
        bhkCollisionObject* pCollision = bhkCollisionObject::GetbhkCollisionObject(pBone);
        if (pCollision) {
            pBody = pCollision->GetRigidBody();
            pBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
            pBody->GetPosition(StartPos);
            pBody->GetRotation(StartRot);
        }
    }
    else {
        Warning("ANIMATION: No Bip01 Spine2 bone for reanimation. Need a backup bone!");
    }
}

// The elapsed-time differences are taken in double before the double compares.
void ReanimateEffect::Update(float afSecondsElapsed) {
    Actor* pTarget = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
    Actor* pCasterActor = GetCaster() ? GetCaster()->GetCasterAsActor() : nullptr;
    if (!pCasterActor) {
        Dispel(true);
        return;
    }
    if (!pBody || !pTarget) {
        return;
    }
    if (pTarget->IsDead(false)) {
        Dispel(false);
        return;
    }
    if (!iState) {
        fSaveTime = 0.0f;
        iState = 10;
    }
    if (iState == 10) {
        // Lift the body over its start position and turn it toward the caster.
        BaseProcess* pProcess = pTarget->GetCurrentAIProcess();
        if (pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
            pProcess->SetCurrentlyReanimating(true);
        }
        // Only strictly above 5 passes; NaN does not.
        if (static_cast<double>(fElapsedSeconds) - fSaveTime > 5.0) {
            iState = 20;
        }
        float fRatio = static_cast<float>((static_cast<double>(fElapsedSeconds) - fSaveTime) / 4.0);
        float fT = BSMIN(1.0f, fRatio);
        hkVector4 kRaisedPos;
        hkVector4 kStartVec;
        NI2HK(kRaisedPos, StartPos);
        NI2HK(kStartVec, StartPos);
        float* pZ = &kRaisedPos(2);
        *pZ = NI2HK(100.0f) + *pZ;
        hkVector4 kNewPos;
        hkSimdReal kT;
        kT.~hkSimdReal();
        ::new (&kT) hkSimdReal(fT);
        kNewPos.setInterpolate4(kStartVec, kRaisedPos, kT);
        NiPoint3 kBodyPos;
        kBodyPos.~NiPoint3();
        ::new (&kBodyPos) NiPoint3;
        pBody->GetPosition(kBodyPos);
        NiPoint3 kToCaster = pCasterActor->GetLocationOnReference() - kBodyPos;
        kToCaster.z = 0.0f;
        float fZAngle = -NiATan2(kToCaster.y, kToCaster.x);
        float fXAngle = NI_HALF_PI;
        float fYAngle = 0.5f;
        NiMatrix3 kNiRot;
        kNiRot.~NiMatrix3();
        ::new (&kNiRot) NiMatrix3;
        kNiRot.FromEulerAnglesZYX(fZAngle, fYAngle, fXAngle);
        hkRotation kHkRot;
        kHkRot.~hkRotation();
        ::new (&kHkRot) hkRotation;
        NI2HK(kHkRot, kNiRot);
        hkQuaternion kStartQuat;
        kStartQuat.~hkQuaternion();
        ::new (&kStartQuat) hkQuaternion;
        NI2HKMIGRATION_QUATERNION(kStartQuat, StartRot);
        hkQuaternion kEndQuat;
        kEndQuat.~hkQuaternion();
        ::new (&kEndQuat) hkQuaternion(kHkRot);
        hkQuaternion kNewRot;
        kNewRot.~hkQuaternion();
        ::new (&kNewRot) hkQuaternion;
        kNewRot.setSlerp(kStartQuat, kEndQuat, fT);
        float fInvDeltaTime = 1.0f / afSecondsElapsed;
        pBody->ApplyKeyframe(kNewPos, kNewRot, fInvDeltaTime, 1.0f);
        pTarget->SetHeading(GetCaster()->GetCasterStatsObject());
    }
    else if (iState == 20) {
        // Play the get-up once the actor has its animation and 3D.
        Animation* pAnimation = pTarget->GetAnimation();
        NiNode* p3D = pTarget->Get3D();
        if (p3D && pAnimation) {
            iState = 30;
            pTarget->ReloadTargets(true);
            // Group 0 with no flags, looping without end, in the group's own section.
            pAnimation->PlayGroup(0, static_cast<ACTION_FLAGS>(0), -1, AGS_NONE);
            bhkBlendController::DoGetUp(p3D, 0.0f);
            BaseProcess* pProcess = pTarget->GetCurrentAIProcess();
            if (pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                pProcess->SetCurrentlyReanimating(false);
            }
            pTarget->GetExtra()->SetRagDollData(nullptr);
            fSaveTime = fElapsedSeconds;
        }
    }
    else if (iState == 30) {
        // Only strictly above 2 passes; NaN does not.
        if (static_cast<double>(fElapsedSeconds) - fSaveTime > 2.0) {
            fSaveTime = fElapsedSeconds;
            iState = 40;
            NiNode* p3D = pTarget->Get3D();
            if (p3D) {
                bhkWorld::SetMotion(p3D, hkpMotion::MOTION_KEYFRAMED, true, false, true);
            }
            TESNPC* pNPC = dynamic_cast<TESNPC*>(pTarget->GetObjectReference());
            if (pNPC) {
                pNPC->SwapEyes(pReanimateEyes);
            }
            BSFaceGenAnimationData* pFaceData = pTarget->GetFaceAnimationData();
            if (pFaceData) {
                pFaceData->SetDead(false, false);
            }
        }
    }
    else if (iState == 40) {
        // Only a difference above 1 passes; NaN does not.
        if (static_cast<double>(fElapsedSeconds) - fSaveTime > 1.0) {
            iState = 50;
            pTarget->SetLifeState(ACTOR_LIFE_STATE_REANIMATE);
            pTarget->SetGhost(false);
            CommandEffect::InitCommandedActor(pTarget, pCasterActor);
        }
    }
    else if (iState == 50) {
        pTarget->GetCurrentAIProcess()->ClearActionHeadTrackTarget(false);
        CommandEffect::UpdateCommandedActor(pTarget, pCasterActor);
    }
}

ActiveEffect* ReanimateEffect::Copy() {
    ReanimateEffect* v2;
    void* v4 = ourMalloc(sizeof(ReanimateEffect));
    if (v4) {
        v2 = ::new (v4) ReanimateEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    CopyData(v2);
    v2->pBody = pBody;
    v2->iState = iState;
    v2->fSaveTime = fSaveTime;
    v2->StartPos.x = StartPos.x;
    v2->StartPos.y = StartPos.y;
    v2->StartPos.z = StartPos.z;
    v2->StartRot.m_fW = StartRot.m_fW;
    v2->StartRot.m_fX = StartRot.m_fX;
    v2->StartRot.m_fY = StartRot.m_fY;
    v2->StartRot.m_fZ = StartRot.m_fZ;
    return v2;
}

void ReanimateEffect::FinishInitLoadGame(Actor* apActor) {
    ActiveEffect::FinishInitLoadGame(apActor);
    NiAVObject* pav = apActor->Get3D();
    if (pav) {
        BSFaceGenAnimationData* panimData = apActor->GetFaceAnimationData();
        if (panimData) {
            if (iState >= 40) {
                panimData->SetDead(false, false);
            }
            else {
                panimData->SetDead(true, true);
            }
        }
        if (iState < 30) {
            bhkWorld::SetMotion(pav, hkpMotion::MOTION_DYNAMIC, true, false, true);
        }
        NiAVObject* pmove = pav->GetObjectByName("Bip01 Spine2");
        if (pmove) {
            bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(pmove);
            if (pcol) {
                pBody = pcol->GetRigidBody();
                pBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
            }
        }
        else {
            Warning("SAVELOAD: No Bip01 Spine2 bone for reanimation. Need a backup bone!");
        }
    }
}

void ReanimateEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    if ((iState < 50) && (apActor) && (apActor->Get3D()) && (pBody)) {
        pBody->SetMotionType(hkpMotion::MOTION_DYNAMIC);
    }
}

bool ReanimateEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

bool ReanimateEffect::CheckTarget(MagicTarget* apTarget) {
    TESObjectREFR* v2 = apTarget->GetTargetStatsObject();
    Character* v4 = dynamic_cast<Character*>(v2);
    return v4 && (v4->IsDead(false) && (!v4->IsReanimated() && v4->GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN));
}

void ReanimateEffect::Finish() {
    if (!pDataHandler->GetClearingData()) {
        Actor* ptarget = pTarget ? pTarget->GetTargetAsActor() : nullptr;
        if (ptarget) {
            if (iState >= 50) {
                if (ptarget->IsDead(false)) {
                    Actor* pcaster;
                LABEL_12:;
                    if (pCaster) {
                        pcaster = pCaster->GetCasterAsActor();
                    }
                    else {
                        pcaster = nullptr;
                    }
                    if (pcaster) {
                        CommandEffect::CleanupCommandedActor(ptarget, pcaster, false);
                    }
                    return;
                }
            }
            else {
                ptarget->SetGhost(false);
                if (pBody) {
                    pBody->SetMotionType(hkpMotion::MOTION_DYNAMIC);
                }
            }
            ptarget->Kill(nullptr, 0.0f);
            goto LABEL_12;
        }
    }
}

int32_t ReanimateEffect::GetState() {
    return iState;
}

ActiveEffect* ReanimateEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
    void* v5 = ourMalloc(sizeof(ReanimateEffect));
    if (v5) {
        return ::new (v5) ReanimateEffect(a1, a2, a3);
    }

    return nullptr;
}
