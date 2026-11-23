#include "Fallout/AI/Character.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMorphData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

Character::Character() : Character(true) {}

Character::Character(bool abadd)
  : Actor(abadd), pBiped(nullptr), bTrespassing(false), bGuard(false), fequippedweight(-1.0f) {
    SetFormType(59u);
    Character::ResetArmorRating();
    bAttackOnNextTheft = false;
    iThiefCrimeStamp = 0;
    iMinorCrimes = 0;
    iMajorCrimes = 0;
    bIgnoreCrime = false;
}

Character::~Character() {
    if (!GetTemporary()) {
        TESObjectREFR::Set3D(nullptr, false);
    }
}

uint16_t Character::GetSaveSize(uint32_t aiFlags) {
    return Actor::GetSaveSize(aiFlags);
}

void Character::SaveGame(uint32_t aiFlags) {
    Actor::SaveGame(aiFlags);
}

void Character::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Actor::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTrespassing), sizeof(bTrespassing), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bGuard), sizeof(bGuard), 0);
}

void Character::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    Actor::LoadGame(aiFlags, aiCurrentFlags);
}

void Character::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    char cSkipped;
    Actor::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() < 9u) {
        char cSkippedData[7];
        apLoadGameBuffer->LoadData(&cSkipped, sizeof(cSkipped));
        apLoadGameBuffer->LoadData(cSkippedData, 4u);
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTrespassing), sizeof(bTrespassing));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bGuard), sizeof(bGuard));
    if (apLoadGameBuffer->GetVersion() < 9u) {
        char cSkippedWord[4];
        char cSkippedData[7];
        apLoadGameBuffer->LoadData(&cSkipped, sizeof(cSkipped));
        apLoadGameBuffer->LoadData(cSkippedData, 4u);
        apLoadGameBuffer->LoadData(cSkippedWord, sizeof(cSkippedWord));
    }
}

void Character::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    Actor::InitLoadGame(aiFlags, aiCurrentFlags);
}

void Character::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor::InitLoadGame(apLoadGameBuffer);
}

// Reverts the actor's TES data, then forgets the cached armor rating and threshold unless the game is loading a save
// (stores -1.0f into both).
void Character::Revert(uint32_t aiFlags) {
    Actor::Revert(aiFlags);
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        fArmorRating = -1.0f;
        fArmorThreshold = -1.0f;
    }
}

void Character::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor::Revert(apLoadGameBuffer);
    fArmorRating = -1.0f;
    fArmorThreshold = -1.0f;
    fequippedweight = -1.0f;
    bTrespassing = false;
    bGuard = false;
}

void Character::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor::FinishLoadGame(apLoadGameBuffer);
}

// A child race, a race whose first three body-part models have "child" in their path, or (unless abIgnoreHeight) a
// loaded actor less than 110 units tall at scale 1. The height is stored as a double, divided by the scale and the
// quotient rounded to a float before comparing it with the double 110.0.
bool Character::IsChild(bool abIgnoreHeight) {
    bool bIsChild = false;
    TESRace* pRace = GetRace();
    if (pRace) {
        bIsChild = pRace->GetChildRace();
        if (!bIsChild) {
            bool bChildModel = false;
            for (int32_t i = 0; !bChildModel && i < 3; ++i) {
                TESModel* pModel = pRace->GetBodyPartModel(GetSex(), static_cast<RACE_BODY_PART>(i));
                if (pModel) {
                    bChildModel = BSstristr(pModel->GetModel(), "child") != nullptr;
                }
            }
            bIsChild = bChildModel;
        }
    }
    if (!abIgnoreHeight && !bIsChild && Get3D()) {
        double dHeight = GetHeight();
        float fScaledHeight = static_cast<float>(dHeight / GetScale());
        if (fScaledHeight < 110.0) {
            bIsChild = true;
        }
    }
    return bIsChild;
}

BipedAnim* Character::GetBiped() {
    return pBiped;
}

// The player's old biped is deleted at once; any other character's goes to the garbage collector.
void Character::SetBiped(BipedAnim* apBiped) {
    if (pBiped) {
        if (pBiped == apBiped) {
            return;
        }
        if (this == pPlayer) {
            BipedAnim* pOldBiped = pBiped;
            if (pOldBiped) {
                delete pOldBiped;
            }
        }
        else {
            GarbageCollector::Add(pBiped);
        }
    }
    pBiped = apBiped;
}

void Character::Update(float afDelta) {
    Actor::Update(afDelta);
    if (this != pPlayer || pPlayer->Is3rdPerson()) {
        NiAVObject* proot = Get3D();
        if (proot && pCurrentProcess) {
            BSFaceGenNiNode* pfacenode = pCurrentProcess->GetFaceNode();
            BSFaceGenNiNode* pfacenodeSkinned = pCurrentProcess->GetFaceSkinnedNode();
            NiAVObject* panimFace = pCurrentProcess->GetHeadAnims();
            if (!pfacenode) {
                NiAVObject* pObject = BSUtilities::GetObjectByName(proot, FixedStrings::QRaceHeadBiped());
                pfacenode = static_cast<BSFaceGenNiNode*>(
                    NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pObject)
                );
                pCurrentProcess->SetFaceNode(pfacenode);
            }
            if (!pfacenodeSkinned) {
                NiAVObject* pObject = BSUtilities::GetObjectByName(proot, FixedStrings::QRaceHeadSkinned());
                pfacenodeSkinned = static_cast<BSFaceGenNiNode*>(
                    NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pObject)
                );
                pCurrentProcess->SetFaceSkinnedNode(pfacenodeSkinned);
            }
            if (!panimFace) {
                panimFace = BSUtilities::GetObjectByName(proot, FixedStrings::QHeadAnims());
                pCurrentProcess->SetHeadAnims(panimFace);
            }
            if ((!pfacenode || pfacenode->GetSelectiveUpdate()) &&
                (!pfacenodeSkinned || !pfacenodeSkinned->GetAppCulled()) &&
                panimFace &&
                pfacenode &&
                pfacenodeSkinned) {
                bool bheadAnimsCulled = panimFace->m_pkParent->GetAppCulled();
                pfacenode->GetAnimationData()->SetAnimHeadCulled(bheadAnimsCulled);
                pfacenodeSkinned->GetAnimationData()->SetAnimHeadCulled(bheadAnimsCulled);
                if (!bheadAnimsCulled) {
                    NiGeomMorpherController* panimMorph = static_cast<NiGeomMorpherController*>(
                        panimFace->GetController(&NiGeomMorpherController::ms_RTTI)
                    );
                    uint32_t icount = panimMorph->GetNumTargets();
                    for (uint32_t n = 0; n < icount; ++n) {
                        NiMorphData::MorphTarget* ptarget = panimMorph->GetTarget(n);
                        NiInterpolator* pinterp = panimMorph->GetInterpolator(n);
                        NiBlendFloatInterpolator* pfloatInterp = nullptr;
                        if (NiObject::IsExactKindOf(&NiBlendFloatInterpolator::ms_RTTI, pinterp)) {
                            pfloatInterp = static_cast<NiBlendFloatInterpolator*>(pinterp);
                        }
                        if (pfloatInterp) {
                            float fvalue = pfloatInterp->m_fFloatValue;
                            NiFixedString pname(ptarget->GetName());
                            if (fvalue >= 0.0 && *pname.GetHandleString()) {
                                BS_FACEGEN_EXPRESSION eexp = BS_FG_EXPRESSION_NONE;
                                BS_FACEGEN_MODIFIER emod = BS_FG_MODIFIER_NONE;
                                BS_FACEGEN_PHONEME ephon = BS_FG_PHONEME_NONE;
                                switch (*pname.GetHandleString()) {
                                    case 65: {
                                        if (pname.GetHandleString()[1] == 110) {
                                            eexp = BS_FG_EXPRESSION_ANGER;
                                        }
                                        else {
                                            ephon = BS_FG_PHONEME_AAH;
                                        }
                                        break;
                                    }
                                    case 66: {
                                        switch (pname.GetHandleString()[1]) {
                                            case 77: {
                                                ephon = BS_FG_PHONEME_B_M_P;
                                                break;
                                            }
                                            case 105: {
                                                ephon = BS_FG_PHONEME_BIG_AAH;
                                                break;
                                            }
                                            case 108: {
                                                emod = static_cast<BS_FACEGEN_MODIFIER>(
                                                    pname.GetHandleString()[5] != 76
                                                );
                                                break;
                                            }
                                            case 114: {
                                                switch (pname.GetHandleString()[4]) {
                                                    case 68: {
                                                        if (pname.GetHandleString()[8] == 76) {
                                                            emod = BS_FG_MODIFIER_BROW_DOWN_LEFT;
                                                        }
                                                        else {
                                                            emod = BS_FG_MODIFIER_BROW_DOWN_RIGHT;
                                                        }
                                                        break;
                                                    }
                                                    case 73: {
                                                        if (pname.GetHandleString()[6] == 76) {
                                                            emod = BS_FG_MODIFIER_BROW_IN_LEFT;
                                                        }
                                                        else {
                                                            emod = BS_FG_MODIFIER_BROW_IN_RIGHT;
                                                        }
                                                        break;
                                                    }
                                                    case 85: {
                                                        if (pname.GetHandleString()[6] == 76) {
                                                            emod = BS_FG_MODIFIER_BROW_UP_LEFT;
                                                        }
                                                        else {
                                                            emod = BS_FG_MODIFIER_BROW_UP_RIGHT;
                                                        }
                                                        break;
                                                    }
                                                }
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 67: {
                                        if (pname.GetHandleString()[1] == 111) {
                                            eexp = BS_FG_EXPRESSION_COMBAT_ANGRY;
                                        }
                                        else {
                                            ephon = BS_FG_PHONEME_CH_J_SH;
                                        }
                                        break;
                                    }
                                    case 68: {
                                        ephon = BS_FG_PHONEME_D_S_T;
                                        break;
                                    }
                                    case 69: {
                                        if (pname.GetHandleString()[1] == 101) {
                                            ephon = BS_FG_PHONEME_EEE;
                                        }
                                        else {
                                            ephon = BS_FG_PHONEME_EH;
                                        }
                                        break;
                                    }
                                    case 70: {
                                        if (pname.GetHandleString()[1] == 101) {
                                            eexp = BS_FG_EXPRESSION_FEAR;
                                        }
                                        else {
                                            ephon = BS_FG_PHONEME_F_V;
                                        }
                                        break;
                                    }
                                    case 72: {
                                        switch (pname.GetHandleString()[4]) {
                                            case 80: {
                                                emod = BS_FG_MODIFIER_HEAD_PITCH;
                                                break;
                                            }
                                            case 82: {
                                                emod = BS_FG_MODIFIER_HEAD_ROLL;
                                                break;
                                            }
                                            case 89: {
                                                emod = BS_FG_MODIFIER_HEAD_YAW;
                                                break;
                                            }
                                            case 121: {
                                                eexp = BS_FG_EXPRESSION_HAPPY;
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 73: {
                                        ephon = BS_FG_PHONEME_I;
                                        break;
                                    }
                                    case 75: {
                                        ephon = BS_FG_PHONEME_K;
                                        break;
                                    }
                                    case 76: {
                                        switch (pname.GetHandleString()[4]) {
                                            case 68: {
                                                emod = BS_FG_MODIFIER_LOOK_DOWN;
                                                break;
                                            }
                                            case 76: {
                                                emod = BS_FG_MODIFIER_LOOK_LEFT;
                                                break;
                                            }
                                            case 82: {
                                                emod = BS_FG_MODIFIER_LOOK_RIGHT;
                                                break;
                                            }
                                            case 85: {
                                                emod = BS_FG_MODIFIER_LOOK_UP;
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 77: {
                                        switch (pname.GetHandleString()[5]) {
                                            case 97: {
                                                eexp = BS_FG_EXPRESSION_MOOD_SAD;
                                                break;
                                            }
                                            case 101: {
                                                eexp = BS_FG_EXPRESSION_MOOD_NEUTRAL;
                                                break;
                                            }
                                            case 102: {
                                                eexp = BS_FG_EXPRESSION_MOOD_AFRAID;
                                                break;
                                            }
                                            case 108: {
                                                eexp = BS_FG_EXPRESSION_MOOD_PLEASANT;
                                                break;
                                            }
                                            case 110: {
                                                if (pname.GetHandleString()[6] == 110) {
                                                    eexp = BS_FG_EXPRESSION_MOOD_ANNOYED;
                                                }
                                                else {
                                                    eexp = BS_FG_EXPRESSION_MOOD_ANGRY;
                                                }
                                                break;
                                            }
                                            case 111: {
                                                eexp = BS_FG_EXPRESSION_MOOD_COCKY;
                                                break;
                                            }
                                            case 114: {
                                                eexp = BS_FG_EXPRESSION_MOOD_DRUGGED;
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 78: {
                                        ephon = BS_FG_PHONEME_N;
                                        break;
                                    }
                                    case 79: {
                                        if (pname.GetHandleString()[1] == 104) {
                                            ephon = BS_FG_PHONEME_OH;
                                        }
                                        else {
                                            ephon = BS_FG_PHONEME_OOH_Q;
                                        }
                                        break;
                                    }
                                    case 80: {
                                        eexp = BS_FG_EXPRESSION_MOOD_PAINED;
                                        break;
                                    }
                                    case 82: {
                                        ephon = BS_FG_PHONEME_R;
                                        break;
                                    }
                                    case 83: {
                                        switch (pname.GetHandleString()[1]) {
                                            case 97: {
                                                eexp = BS_FG_EXPRESSION_SAD;
                                                break;
                                            }
                                            case 113: {
                                                if (pname.GetHandleString()[6] == 76) {
                                                    emod = BS_FG_MODIFIER_SQUINT_LEFT;
                                                }
                                                else {
                                                    emod = BS_FG_MODIFIER_SQUINT_RIGHT;
                                                }
                                                break;
                                            }
                                            case 117: {
                                                eexp = BS_FG_EXPRESSION_SURPRISE;
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 84: {
                                        ephon = BS_FG_PHONEME_TH;
                                        break;
                                    }
                                    case 87: {
                                        ephon = BS_FG_PHONEME_W;
                                        break;
                                    }
                                    default: {
                                        break;
                                    }
                                }
                                if (eexp != BS_FG_EXPRESSION_NONE) {
                                    pfacenode->GetAnimationData()->SetAnimExpressionValue(eexp, fvalue);
                                    pfacenodeSkinned->GetAnimationData()->SetAnimExpressionValue(eexp, fvalue);
                                }
                                else if (emod != BS_FG_MODIFIER_NONE) {
                                    pfacenode->GetAnimationData()->SetAnimModifierValue(emod, fvalue);
                                    pfacenodeSkinned->GetAnimationData()->SetAnimModifierValue(emod, fvalue);
                                }
                                else if (ephon != BS_FG_PHONEME_NONE) {
                                    pfacenode->GetAnimationData()->SetAnimPhonemeValue(ephon, fvalue);
                                    pfacenodeSkinned->GetAnimationData()->SetAnimPhonemeValue(ephon, fvalue);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

bool Character::IsActoraGuard() {
    return bGuard;
}

void Character::SetActorGuard(bool abGuard) {
    bGuard = abGuard;
}

void Character::SetEquippedWeight(float afvalue) {
    fequippedweight = afvalue;
}

float Character::GetTurningSpeed() {
    return fCharacterDefaultTurningSpeed.Float();
}

void Character::InitiateSandmanPackage(
    Actor* apTarget,
    TESObjectREFR* apfurniture,
    FurnitureMark* aMarker,
    uint8_t auiIndex
) {
    if (!IsInCombat()) {
        pCurrentProcess->SetRunOncePackage(nullptr, this);
        pCurrentProcess->ClearActionHeadTrackTarget(false);
        pCurrentProcess->GetCurrentPackage();
        if (GetCurrentPackage()) {
            if (GetCurrentPackage()->IsInterruptPackage()) {
                EndDialogue();
            }
        }
        TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_SANDMAN);
        ppack->SetMustReachLocation(true);
        ppack->SetMustComplete(true);
        PackageLocation* ploc = new PackageLocation;
        ploc->SetLocType(PLT_REFERENCE);
        ploc->SetLocReference(this);
        ppack->SetPackageLocation(ploc);
        if (ploc) {
            delete ploc;
        }
        PackageTarget* pnew = new PackageTarget;
        ppack->SetPackageTarget(pnew);
        if (pnew) {
            delete pnew;
        }
        ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SANDMAN;
        ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        ppack->GetPackageTarget()->SetTargReference(apTarget);
        ppack->GetPackageTarget()->iValue = 0;
        pCurrentProcess->ComputeLastTimeProcessed();
        if (pCurrentProcess->GetCurrentPackage()) {
            bool bDoneOnce = pCurrentProcess->IsPackageDoneOnce();
            bool bTargetActivated = pCurrentProcess->IsTargetActivated();
            bool bActionComplete = pCurrentProcess->GetCurrentActionComplete();
            TESObjectREFR* pTarget = pCurrentProcess->GetTarget();
            int32_t iProcedureIndex = pCurrentProcess->GetCurrentProcedureIndex();
            TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
            m_Extra.SetPackageExtra(pPackage, iProcedureIndex, pTarget, bActionComplete, bTargetActivated, bDoneOnce);
        }
        pCurrentProcess->SetFurniture(apfurniture, auiIndex, aMarker);
        PutCreatedPackage(ppack, false, true);
        apTarget->InitiateDoNothingPackage();
    }
}

TESRace* Character::GetRace() {
    TESNPC* pBaseForm = static_cast<TESNPC*>(GetObjectReference());
    return pBaseForm->GetFormRace();
}

void Character::ResetArmorRating() {
    fArmorRating = -1.0f;
    fArmorThreshold = -1.0f;
}

bool Character::InitiateTresPassPackage(TrespassPackage* apPack) {
    if (Actor::IsInCombat() ||
        IsDead(false) ||
        Actor::IsAlarmed() ||
        Actor::IsFleeing(false) ||
        IsTresPassing() ||
        (apPack && static_cast<int32_t>(apPack->GetNumberActors()) > 1)) {
        return false;
    }
    if (GetSitSleepState() &&
        GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
        GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
        return false;
    }
    if (!apPack) {
        return false;
    }
    pCurrentProcess->ClearRunOncePackage();
    MobileObject::GetCurrentAIProcess()->ClearActionHeadTrackTarget(true);
    if (MobileObject::GetCurrentPackage()) {
        if (MobileObject::GetCurrentPackage()->IsInterruptPackage()) {
            Actor::EndInterruptPackage(false);
        }
    }
    MobileObject::GetCurrentAIProcess()->ComputeLastTimeProcessed();
    MobileObject::GetCurrentAIProcess()->SavePackageToExtraData(this);
    PutCreatedPackage(apPack, false, false);
    apPack->ModNumberActors(1);
    pCurrentProcess->SetProcedureIndexRunning(0);
    return true;
}

void Character::InitiateSpectator() {
    if ((!GetCurrentPackage() || GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_FLEE) &&
        !IsInCombat() &&
        (!IsinDialogue() || GetCurrentTarget() != pPlayer) &&
        (GetSitSleepState() == SIT_SLEEP_NORMAL ||
         GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
         GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) &&
        !ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_OBSERVE_COMBAT_BEHAVIOR)) {
        BaseProcess* pproc = pCurrentProcess;
        if (pproc) {
            TESPackage* ppack = pproc->GetRunOncePackage();
            if (!ppack ||
                (ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_OBSERVE_COMBAT &&
                 ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE &&
                 ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE_NON_COMBAT)) {
                ppack = pproc->GetCurrentPackage();
                if (!ppack ||
                    (ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_OBSERVE_COMBAT &&
                     ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE &&
                     ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE_NON_COMBAT)) {
                    pCurrentProcess->ClearRunOncePackage();
                    pCurrentProcess->ClearActionHeadTrackTarget(true);
                    if (pCurrentProcess->GetSitSleepState() == SIT_SLEEP_IS_SITTING || IsSleeping()) {
                        InitiateGetUpPackage();
                    }
                    SpectatorPackage* pspecPack = new SpectatorPackage;
                    pCurrentProcess->ComputeLastTimeProcessed();
                    if (GetCurrentPackage()) {
                        pspecPack->CopyAllFlags(GetCurrentPackage());
                    }
                    if (pCurrentProcess) {
                        if (pCurrentProcess->GetPackageThatIsRunning()) {
                            if (pCurrentProcess->GetPackageThatIsRunning()->IsInterruptPackage()) {
                                EndInterruptPackage(false);
                            }
                        }
                    }
                    pCurrentProcess->SavePackageToExtraData(this);
                    PutCreatedPackage(pspecPack, false, true);
                }
            }
        }
    }
}

float Character::GetArmorRating() {
    TESObjectARMO* parmorFound[20];
    int32_t iarmorFoundCount = 0;
    parmorFound[0] = nullptr;
    if (fArmorRating < 0.0) {
        fArmorRating = 0.0f;
        InventoryChanges* pinventory = m_Extra.GetContainerChanges();
        if (pinventory) {
            for (BIPED_OBJECT i = BO_HEAD; i < BIPED_OBJECT_COUNT; i = static_cast<BIPED_OBJECT>(i + 1)) {
                ItemChange* pchange = pinventory->GetWornItem(i, false);
                if (pchange) {
                    TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(pchange->pContainerObj);
                    if (parmo) {
                        bool balreadyCounted = false;
                        if (iarmorFoundCount) {
                            for (int32_t j = 0; j < iarmorFoundCount && !balreadyCounted; ++j) {
                                balreadyCounted = parmorFound[j] == parmo;
                            }
                        }
                        if (!balreadyCounted) {
                            ActorValueOwner* pOwner;
                            if (this) {
                                pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(*this));
                            }
                            else {
                                pOwner = nullptr;
                            }
                            fArmorRating = pchange->GetConditionScaledDR(pOwner) + fArmorRating;
                            parmorFound[iarmorFoundCount++] = parmo;
                        }
                    }
                    delete pchange;
                }
            }
        }
    }
    if (fMaxArmorRating.Float() > 0.0) {
        return BSMIN(fArmorRating, fMaxArmorRating.Float());
    }

    return fArmorRating;
}

float Character::GetArmorThreshold() {
    TESObjectARMO* parmorFound[20];
    int32_t iarmorFoundCount = 0;
    parmorFound[0] = nullptr;
    if (fArmorThreshold < 0.0) {
        fArmorThreshold = 0.0f;
        InventoryChanges* pinventory = m_Extra.GetContainerChanges();
        if (pinventory) {
            for (int32_t i = 0; i < 20; ++i) {
                ItemChange* pchange = pinventory->GetWornItem(static_cast<BIPED_OBJECT>(i), false);
                if (pchange) {
                    TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(pchange->pContainerObj);
                    if (parmo) {
                        bool balreadyCounted = false;
                        if (iarmorFoundCount) {
                            for (int32_t j = 0; j < iarmorFoundCount && !balreadyCounted; ++j) {
                                balreadyCounted = parmorFound[j] == parmo;
                            }
                        }
                        if (!balreadyCounted) {
                            ActorValueOwner* pOwner;
                            if (this) {
                                pOwner = &static_cast<ActorValueOwner&>(static_cast<Actor&>(*this));
                            }
                            else {
                                pOwner = nullptr;
                            }
                            fArmorThreshold = pchange->GetModifiedDamageThreshold(pOwner) + fArmorThreshold;
                            parmorFound[iarmorFoundCount++] = parmo;
                        }
                    }
                    delete pchange;
                }
            }
        }
    }
    return fArmorThreshold;
}

int32_t Character::GetAttackDamage() {
    float fDamage = 0.0f;
    float fFatigueDamage = 0.0f;
    if (this) {
        CombatFormulas::CalcHandToHandDamage(static_cast<ActorValueOwner*>(this), false, fDamage, fFatigueDamage);
    }
    else {
        CombatFormulas::CalcHandToHandDamage(nullptr, false, fDamage, fFatigueDamage);
    }
    return static_cast<int32_t>(fDamage);
}

void Character::SetTresPassing() {
    TESObjectCELL* lpParentCell = GetParentCell();
    bool lbTrespassing = false;
    if (lpParentCell) {
        lbTrespassing = lpParentCell->CheckifTrespassing(this);
    }
    bTrespassing = lbTrespassing;
}

void Character::PrecacheData() {
    Actor::PrecacheData();
    GetArmorThreshold();
    GetArmorRating();
    NiNode* pScene = Get3D();
    if (pScene && pCurrentProcess) {
        if (!pCurrentProcess->GetFaceNode()) {
            NiAVObject* pObject = BSUtilities::GetObjectByName(pScene, FixedStrings::QRaceHeadBiped());
            BSFaceGenNiNode* pFaceNode = static_cast<BSFaceGenNiNode*>(
                NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pObject)
            );
            pCurrentProcess->SetFaceNode(pFaceNode);
        }
        if (!pCurrentProcess->GetFaceSkinnedNode()) {
            NiAVObject* pObject = BSUtilities::GetObjectByName(pScene, FixedStrings::QRaceHeadSkinned());
            BSFaceGenNiNode* pFaceNode = static_cast<BSFaceGenNiNode*>(
                NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, pObject)
            );
            pCurrentProcess->SetFaceSkinnedNode(pFaceNode);
        }
        if (!pCurrentProcess->GetHeadAnims()) {
            NiAVObject* pObject = BSUtilities::GetObjectByName(pScene, FixedStrings::QHeadAnims());
            pCurrentProcess->SetHeadAnims(pObject);
        }
    }
}

void Character::InitiateCannibal(Actor* apTarget) {
    if (!IsInCombat()) {
        pCurrentProcess->SetRunOncePackage(nullptr, this);
        pCurrentProcess->ClearActionHeadTrackTarget(false);
        pCurrentProcess->GetCurrentPackage();
        if (GetCurrentPackage()) {
            if (GetCurrentPackage()->IsInterruptPackage()) {
                EndDialogue();
            }
        }
        TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_CANNIBAL);
        ppack->SetMustReachLocation(true);
        ppack->SetMustComplete(true);
        PackageLocation* ploc = new PackageLocation;
        ploc->SetLocType(PLT_REFERENCE);
        ploc->SetLocReference(this);
        ppack->SetPackageLocation(ploc);
        if (ploc) {
            delete ploc;
        }
        PackageTarget* pnew = new PackageTarget;
        ppack->SetPackageTarget(pnew);
        if (pnew) {
            delete pnew;
        }
        ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_CANNIBAL;
        ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        ppack->GetPackageTarget()->SetTargReference(apTarget);
        ppack->GetPackageTarget()->iValue = 0;
        pCurrentProcess->ComputeLastTimeProcessed();
        if (pCurrentProcess->GetCurrentPackage()) {
            bool bDoneOnce = pCurrentProcess->IsPackageDoneOnce();
            bool bTargetActivated = pCurrentProcess->IsTargetActivated();
            bool bActionComplete = pCurrentProcess->GetCurrentActionComplete();
            TESObjectREFR* pTarget = pCurrentProcess->GetTarget();
            int32_t iProcedureIndex = pCurrentProcess->GetCurrentProcedureIndex();
            TESPackage* pPackage = pCurrentProcess->GetCurrentPackage();
            m_Extra.SetPackageExtra(pPackage, iProcedureIndex, pTarget, bActionComplete, bTargetActivated, bDoneOnce);
        }
        PutCreatedPackage(ppack, false, true);
        apTarget->InitiateDoNothingPackage();
    }
}

void Character::Reset3D() {
    if (TESObjectREFR::Get3D()) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueCharacterReset3D(this);
        }
        else {
            if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
                Interface::GetPipboy()->Shutdown();
            }
            MobileObject::GetCurrentAIProcess()->Set3DUpdateFlag(RESET_MODEL);
            MobileObject::GetCurrentAIProcess()->Set3DUpdateFlag(RESET_SKIN);
            MobileObject::GetCurrentAIProcess()->Set3DUpdateFlag(RESET_HEAD);
            MobileObject::GetCurrentAIProcess()->Set3DUpdateFlag(RESET_SCALE);
            MobileObject::GetCurrentAIProcess()->Update3dModel(this);
            if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
                Interface::GetPipboy()->Initialize();
            }
            if (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                static_cast<HighProcess*>(pCurrentProcess)->SetActorHeight(0.0f);
            }
        }
    }
}

float Character::GetClothingValue() {
    float fClothingValue = 0.0f;
    InventoryChanges* pInventory = m_Extra.GetContainerChanges();
    if (pInventory) {
        for (BIPED_OBJECT part = BO_HEAD; part < BIPED_OBJECT_COUNT; part = static_cast<BIPED_OBJECT>(part + 1)) {
            ItemChange* pWornItem = pInventory->GetWornItem(part, false);
            if (pWornItem) {
                char* pArmor = reinterpret_cast<char*>(dynamic_cast<TESObjectARMO*>(pWornItem->pContainerObj));
                char* pClothing = reinterpret_cast<char*>(dynamic_cast<TESObjectCLOT*>(pWornItem->pContainerObj));
                if (pArmor) {
                    double dItemValue = pWornItem->GetItemValue();
                    double dScaled = fClothingArmorScale.Float() * dItemValue;
                    fClothingValue = fClothingArmorBase.Float() + dScaled + fClothingValue;
                }
                else if (pClothing) {
                    double dItemValue = pWornItem->GetItemValue();
                    double dScaled = fClothingClassScale.Float() * dItemValue;
                    fClothingValue = fClothingBase.Float() + dScaled + fClothingValue;
                }
                delete pWornItem;
            }
        }
    }
    if (fClothingValue > 100.0) {
        return 100.0f;
    }
    return fClothingValue;
}

bool TESRace::GetChildRace() const {
    return (Data.iFlags & RACE_DATA::RACE_FLAG_CHILD) != 0;
}

uint32_t TrespassPackage::GetNumberActors() {
    return iNumberActors;
}

NiMorphData::MorphTarget* NiGeomMorpherController::GetTarget(uint32_t uiIndex) {
    return m_spMorphData->GetTarget(uiIndex);
}
