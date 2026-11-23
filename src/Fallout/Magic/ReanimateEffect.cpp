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
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::REANIMATE,
    &ReanimateEffect::Instantiate
);

ReanimateEffect::ReanimateEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect), pBody(nullptr), iState(0), fSaveTime(0.0f), StartPos(0.0f, 0.0f, 0.0f),
    StartRot(NiQuaternion::IDENTITY) {}

uint16_t ReanimateEffect::GetSaveSize(Actor* apActor) {
    return static_cast<uint16_t>(ActiveEffect::GetSaveSize(apActor) + 36);
}

void ReanimateEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&iState, sizeof(iState));
    pSaveLoadGameOLD->SaveGameDataOLD(&fSaveTime, sizeof(fSaveTime));
    pSaveLoadGameOLD->SaveGameDataOLD(&StartPos, sizeof(StartPos));
    pSaveLoadGameOLD->SaveGameDataOLD(&StartRot, sizeof(StartRot));
}

void ReanimateEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&iState, sizeof(iState));
    pSaveLoadGameOLD->LoadGameDataOLD(&fSaveTime, sizeof(fSaveTime));
    pSaveLoadGameOLD->LoadGameDataOLD(&StartPos, sizeof(StartPos));
    pSaveLoadGameOLD->LoadGameDataOLD(&StartRot, sizeof(StartRot));
}

void ReanimateEffect::Start() {
    if (!GetCaster()) {
        Dispel(true);
        return;
    }
    Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
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
    float fHealthDelta = static_cast<double>(pValues->GetBaseActorValue(ActorValue::HEALTH)) -
        static_cast<double>(pValues->GetActorFloatValue(ActorValue::HEALTH));
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
        hkSimdReal kT(fT);
        kNewPos.setInterpolate4(kStartVec, kRaisedPos, kT);
        NiPoint3 kBodyPos;
        pBody->GetPosition(kBodyPos);
        NiPoint3 kToCaster = pCasterActor->GetLocationOnReference() - kBodyPos;
        kToCaster.z = 0.0f;
        float fZAngle = -NiATan2(kToCaster.y, kToCaster.x);
        float fXAngle = NI_HALF_PI;
        float fYAngle = 0.5f;
        NiMatrix3 kNiRot;
        kNiRot.FromEulerAnglesZYX(fZAngle, fYAngle, fXAngle);
        hkRotation kHkRot;
        NI2HK(kHkRot, kNiRot);
        hkQuaternion kStartQuat;
        NI2HKMIGRATION_QUATERNION(kStartQuat, StartRot);
        hkQuaternion kEndQuat(kHkRot);
        hkQuaternion kNewRot;
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
            pAnimation->PlayGroup(0, ACTION_NORMAL, -1, AGS_NONE);
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
    ReanimateEffect* pCopy = new ReanimateEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    pCopy->pBody = pBody;
    pCopy->iState = iState;
    pCopy->fSaveTime = fSaveTime;
    pCopy->StartPos = StartPos;
    pCopy->StartRot = StartRot;
    return pCopy;
}

void ReanimateEffect::FinishInitLoadGame(Actor* apActor) {
    ActiveEffect::FinishInitLoadGame(apActor);
    NiAVObject* pAV = apActor->Get3D();
    if (pAV) {
        BSFaceGenAnimationData* pAnimData = apActor->GetFaceAnimationData();
        if (pAnimData) {
            if (iState >= 40) {
                pAnimData->SetDead(false, false);
            }
            else {
                pAnimData->SetDead(true, true);
            }
        }
        if (iState < 30) {
            bhkWorld::SetMotion(pAV, hkpMotion::MOTION_DYNAMIC, true, false, true);
        }
        NiAVObject* pMove = pAV->GetObjectByName("Bip01 Spine2");
        if (pMove) {
            bhkCollisionObject* pCol = bhkCollisionObject::GetbhkCollisionObject(pMove);
            if (pCol) {
                pBody = pCol->GetRigidBody();
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
    if (iState < 50 && apActor && apActor->Get3D() && pBody) {
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
    TESObjectREFR* pRef = apTarget->GetTargetStatsObject();
    Character* pCharacter = dynamic_cast<Character*>(pRef);
    return pCharacter &&
        pCharacter->IsDead(false) &&
        !pCharacter->IsReanimated() &&
        pCharacter->GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN;
}

void ReanimateEffect::Finish() {
    if (!pDataHandler->GetClearingData()) {
        Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
        if (pTargetActor) {
            if (iState >= 50) {
                if (!pTargetActor->IsDead(false)) {
                    pTargetActor->Kill(nullptr, 0.0f);
                }
            }
            else {
                pTargetActor->SetGhost(false);
                if (pBody) {
                    pBody->SetMotionType(hkpMotion::MOTION_DYNAMIC);
                }
                pTargetActor->Kill(nullptr, 0.0f);
            }
            Actor* pCasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
            if (pCasterActor) {
                CommandEffect::CleanupCommandedActor(pTargetActor, pCasterActor, false);
            }
        }
    }
}

int32_t ReanimateEffect::GetState() {
    return iState;
}

ActiveEffect* ReanimateEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new ReanimateEffect(apCaster, apSpell, apEffect);
}
