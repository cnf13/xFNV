#include "Fallout/AI/CombatController.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AnimSequenceBase.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatProcedureAttackLow.h"
#include "Fallout/AI/Combat/CombatProcedureEvade.h"
#include "Fallout/AI/Combat/CombatProcedureMove.h"
#include "Fallout/AI/Combat/CombatProcedureSearch.h"
#include "Fallout/AI/Combat/CombatProcedureSwitchWeapon.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/FailedCombatProcedure.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cstdint>
#include <limits>

// The actor move mode flags of each combat move mode, indexed by COMBAT_MOVE_MODE.
using CombatMoveModeFlagsArray = uint32_t[COMBAT_MOVE_MODE_COUNT];
static CombatMoveModeFlagsArray iCombatMoveMode{256, 256, 1024, 512, 512};

kCombatLocationTypeData_t kCombatLocationTypeData{
    {true, true, true, true, 64.0f},    {true, true, true, true, 128.0f},  {true, true, true, true, 10.0f},
    {false, true, false, false, 10.0f}, {true, true, true, false, 10.0f},  {true, true, true, true, 10.0f},
    {true, true, true, true, 10.0f},    {false, true, true, true, 10.0f},  {false, true, true, false, 10.0f},
    {false, true, true, true, 10.0f},   {false, true, true, true, 10.0f},  {false, true, true, true, 10.0f},
    {false, false, false, true, 10.0f}, {false, true, true, true, 128.0f}, {false, true, true, true, 10.0f},
    {true, false, true, true, 64.0f},   {false, true, true, true, 10.0f},  {false, true, true, true, 128.0f},
    {false, true, true, true, 128.0f},  {false, true, true, true, 128.0f}, {false, true, true, true, 128.0f},
    {false, true, true, true, 128.0f}
};
BSPerformanceTimer CombatAI_Planning("Combat AI", "Planning", true, true);

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bDebugCombatAttackerText("bDebugCombatAttackerText:Combat", false);
SettingT<INISettingCollection> bDebugCombatGuardRadius("bDebugCombatGuardRadius:Combat", false);
SettingT<INISettingCollection> bDebugCombatArea("bDebugCombatArea:Combat", false);
SettingT<INISettingCollection> bDebugCombatUnreachableLocations("bDebugCombatUnreachableLocations:Combat", false);

bool IsPackageFlag4Set(TESPackage* apThis, [[maybe_unused]] void* apEdx) {
    return (apThis->PackData.iPackageSpecificFlags & kPackSpecificFlag_GuardRemainNearReference) != 0;
}

CombatController::CombatController(Actor* apActor, CombatGroup* apGroup, TESPackage* apPackage)
  : pCombatGroup(apGroup), pActionProcedure(nullptr), pMovementProcedure(nullptr), pCombatPlan(nullptr),
    pCombatPlannerData(nullptr), pCurrentAction(nullptr), pCurrentGoal(nullptr), iCurrentActionValue(0),
    pAttacker(apActor), pTarget(nullptr), bForceCombat(false), bUpdatedDetection(false), cUseIronSights(-1),
    bForceSneak(0), iMoveMode(0), fAttackerHeight(128.0f), fAttackerSneakHeight(64.0f), bAttackerCanSneak(true),
    bAttackerCanMove(true), fWalkSpeedMult(1.0f), fRunSpeedMult(1.0f), fCombatPosition(-1.0f), bFirstUpdate(false),
    spDebugGeometry(nullptr), spCellDebugGeometry(nullptr), pCurrentWeapon(nullptr), pCurrentCombatStyle(nullptr),
    bDestructing(false), bDeterminedPowerAttackDeltas(false), fPowerAttackRanges{}, fPowerAttackHitTimes{},
    bHasForwardAttacks(false), bStopCombat(false), bStoppingCombat(false), ucLookAtTargetPriorities{},
    ucIgnoreLookAtTarget(0), eLowState(COMBAT_LOW_STATE_NONE), fLowMovementDelta(0.0f) {
    float fTimeDelay = static_cast<double>(iStaggerPeriodicUpdateCounter % 10) * 0.10000000149011612;
    ++iStaggerPeriodicUpdateCounter;
    pCombatState = new CombatState(this, fTimeDelay);
    PlanTimer.Reset(0.1f);
    ourMemset(FailedCombatProcedures, 0, sizeof(FailedCombatProcedures));
    SetPackType(PACKAGE_TYPE_COMBAT);
    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_COMBAT;
    ourMemset(kLookAtTargets, 0, sizeof(kLookAtTargets));
    if (pAttacker) {
        if (!apPackage) {
            apPackage = pAttacker->pCurrentProcess->GetCurrentPackage();
        }
        UpdatePackageData(apPackage);
    }
}

CombatController::~CombatController() {
    bDestructing = true;
    if (pSaveLoadGame->GetSaveGameLoading() && !pSaveLoadGame->GetInitingForms()) {
        pCombatGroup = nullptr;
    }
    if (!pDataHandler->GetClearingData()) {
        SetTarget(nullptr);
        SetMovementProcedure(nullptr);
        pAttacker->ClearLookAtTarget();
        pAttacker->SetIronSights(false, false, false);
        SetActionProcedure(nullptr);
    }
    ClearBackgroundProcedures();
    ClearCombatPlan();
    if (pCombatGroup) {
        pCombatGroup->RemoveMember(pAttacker);
    }
    if (pCombatState) {
        delete pCombatState;
    }
    UpdateDebugGeometry(true);
    if (!pSaveLoadGameOLD->GetLoading() && pAttacker && pAttacker->GetBlocked()) {
        pAttacker->SetBlock(false);
    }
    if (--iStaggerPeriodicUpdateCounter < 0) {
        iStaggerPeriodicUpdateCounter = 0;
    }
}

void CombatController::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pAttacker, 0);
    apSaveGameBuffer->SaveFormID(pTarget, 0);
    uint32_t uiCombatGroupID = pCombatGroup->iGroupID;
    apSaveGameBuffer->SaveDataEndian(&uiCombatGroupID, sizeof(uiCombatGroupID), 0);
    pCombatState->SaveGame(apSaveGameBuffer);
    SaveProcedure(pActionProcedure, apSaveGameBuffer);
    SaveProcedure(pMovementProcedure, apSaveGameBuffer);
    uint32_t uiSaveGameCount = BackgroundProcedures.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        SaveProcedure(BackgroundProcedures[i], apSaveGameBuffer);
    }
    char cHasPlan = pCombatPlan != nullptr;
    apSaveGameBuffer->SaveDataEndian(&cHasPlan, sizeof(cHasPlan), 0);
    if (cHasPlan) {
        pCombatPlan->SaveGame(apSaveGameBuffer);
    }
    char cActionType;
    if (pCurrentAction) {
        cActionType = pCurrentAction->eType;
    }
    else {
        cActionType = -1;
    }
    apSaveGameBuffer->SaveDataEndian(&cActionType, sizeof(cActionType), 0);
    apSaveGameBuffer->SaveDataEndian(&iCurrentActionValue, sizeof(iCurrentActionValue), 0);
    PlanTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bForceSneak, sizeof(bForceSneak), 0);
    apSaveGameBuffer->SaveDataEndian(&iMoveMode, sizeof(iMoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(&fWalkSpeedMult, sizeof(fWalkSpeedMult), 0);
    apSaveGameBuffer->SaveDataEndian(&fRunSpeedMult, sizeof(fRunSpeedMult), 0);
    apSaveGameBuffer->SaveDataEndian(&bFirstUpdate, sizeof(bFirstUpdate), 0);
    apSaveGameBuffer->SaveDataEndian(&bStopCombat, sizeof(bStopCombat), 0);
    apSaveGameBuffer->SaveDataEndian(&bStoppingCombat, sizeof(bStoppingCombat), 0);
    apSaveGameBuffer->SaveDataEndian(&eLowState, sizeof(eLowState), 0);
    apSaveGameBuffer->SaveDataEndian(&fLowMovementDelta, sizeof(fLowMovementDelta), 0);
    for (uint32_t i = 0; i < 2; ++i) {
        apSaveGameBuffer->SaveDataEndian(&kLookAtTargets[i], sizeof(kLookAtTargets[i]), 0);
        apSaveGameBuffer->SaveDataEndian(&ucLookAtTargetPriorities[i], sizeof(ucLookAtTargetPriorities[i]), 0);
    }
    apSaveGameBuffer->SaveDataEndian(&ucIgnoreLookAtTarget, sizeof(ucIgnoreLookAtTarget), 0);
    apSaveGameBuffer->SaveDataEndian(&cUseIronSights, sizeof(cUseIronSights), 0);
    apSaveGameBuffer->SaveDataEndian(&bUpdatedDetection, sizeof(bUpdatedDetection), 0);
}

void CombatController::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pAttacker);
    apLoadGameBuffer->LoadFormID(pTarget);
    uint32_t uiCombatGroupID = 0;
    apLoadGameBuffer->LoadData(&uiCombatGroupID, sizeof(uiCombatGroupID));
    pCombatGroup = pCombatManager->GetCombatGroup(uiCombatGroupID);
    pCombatState->LoadGame(apLoadGameBuffer);
    LoadProcedure(pActionProcedure, apLoadGameBuffer);
    LoadProcedure(pMovementProcedure, apLoadGameBuffer);
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    BackgroundProcedures.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        LoadProcedure(BackgroundProcedures[i], apLoadGameBuffer);
    }
    bool bHasPlan = false;
    apLoadGameBuffer->LoadData(&bHasPlan, sizeof(bHasPlan));
    if (bHasPlan) {
        CombatPlan* pPlan = new CombatPlan(this);
        pCombatPlan = pPlan;
        pCombatPlan->LoadGame(apLoadGameBuffer);
        pCurrentGoal = pCombatPlan->pGoal;
    }
    char cActionType = -1;
    apLoadGameBuffer->LoadData(&cActionType, sizeof(cActionType));
    if (cActionType != -1) {
        pCurrentAction = CombatPlan::GetCombatAction(cActionType);
    }
    apLoadGameBuffer->LoadData(&iCurrentActionValue, sizeof(iCurrentActionValue));
    PlanTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&bForceSneak, sizeof(bForceSneak));
    apLoadGameBuffer->LoadData(&iMoveMode, sizeof(iMoveMode));
    apLoadGameBuffer->LoadData(&fWalkSpeedMult, sizeof(fWalkSpeedMult));
    apLoadGameBuffer->LoadData(&fRunSpeedMult, sizeof(fRunSpeedMult));
    apLoadGameBuffer->LoadData(&bFirstUpdate, sizeof(bFirstUpdate));
    apLoadGameBuffer->LoadData(&bStopCombat, sizeof(bStopCombat));
    apLoadGameBuffer->LoadData(&bStoppingCombat, sizeof(bStoppingCombat));
    apLoadGameBuffer->LoadData(&eLowState, sizeof(eLowState));
    apLoadGameBuffer->LoadData(&fLowMovementDelta, sizeof(fLowMovementDelta));
    for (uint32_t i = 0; i < 2; ++i) {
        apLoadGameBuffer->LoadData(&kLookAtTargets[i], sizeof(kLookAtTargets[i]));
        apLoadGameBuffer->LoadData(&ucLookAtTargetPriorities[i], sizeof(ucLookAtTargetPriorities[i]));
    }
    apLoadGameBuffer->LoadData(&ucIgnoreLookAtTarget, sizeof(ucIgnoreLookAtTarget));
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        apLoadGameBuffer->LoadData(&cUseIronSights, sizeof(cUseIronSights));
        apLoadGameBuffer->LoadData(&bUpdatedDetection, sizeof(bUpdatedDetection));
    }
}

void CombatController::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
    Actor* pLoadedAttacker = nullptr;
    if (pAttacker) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttacker)));
        pLoadedAttacker = dynamic_cast<Actor*>(pForm);
    }
    pAttacker = pLoadedAttacker;
    Actor* pLoadedTarget = nullptr;
    if (pTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        pLoadedTarget = dynamic_cast<Actor*>(pForm);
    }
    pTarget = pLoadedTarget;
    if (pTarget) {
        pCombatManager->pTargetMap->AddTargetAttacker(pTarget, pAttacker);
    }
    if (pActionProcedure) {
        pActionProcedure->InitLoadGame(apLoadGameBuffer);
    }
    if (pMovementProcedure) {
        pMovementProcedure->InitLoadGame(apLoadGameBuffer);
    }
    uint32_t uiCount = BackgroundProcedures.iSize;
    for (uint32_t i = 0; i < uiCount; ++i) {
        BackgroundProcedures[i]->InitLoadGame(apLoadGameBuffer);
    }
    pCombatState->InitLoadGame(apLoadGameBuffer);
    if (pCombatPlan) {}
    if (pCombatGroup && !pCombatGroup->IsMember(pAttacker) && !pCombatGroup->AddMember(pAttacker)) {
        pCombatGroup = nullptr;
    }

    if (!pCombatGroup) {
        pCombatGroup = pCombatManager->CreateCombatGroup();
        pCombatGroup->AddMember(pAttacker);
        if (pTarget) {
            pCombatGroup->AddTarget(pTarget);
            pCombatGroup->IncrementAttackerCount(pTarget);
        }
    }
}

void CombatController::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    reinterpret_cast<TESActorBase*>(this)->TESActorBase::Revert(apLoadGameBuffer);
    UpdateCurrentWeapon();
    UpdateCurrentCombatStyle();
    CalculateAttackerInfo();
    InitPowerAttackDistances();
}

void CombatController::UpdateCurrentWeapon() {
    pCurrentWeapon = nullptr;
    CombatProcedureSwitchWeapon* pSwitchWeapon = static_cast<CombatProcedureSwitchWeapon*>(GetBackgroundProcedure(5));
    if (pSwitchWeapon) {
        pCurrentWeapon = pSwitchWeapon->pWeapon;
    }
    else {
        pCurrentWeapon = pAttacker->GetCurrentWeapon();
    }
}

void CombatController::ClearReferencePointers(TESObjectREFR* apRef) {
    if (pCombatState) {
        pCombatState->ClearReferencePointer(apRef, false);
    }
    if (pActionProcedure) {
        pActionProcedure->ClearReferencePointers(apRef);
    }
    if (pMovementProcedure) {
        pMovementProcedure->ClearReferencePointers(apRef);
    }
    if (pTarget == apRef) {
        StopCombat(pTarget);
    }
}

void CombatController::CalculateAttackerInfo() {
    fAttackerHeight = pAttacker->GetHeight();
    fAttackerSneakHeight = pAttacker->GetSneakHeight(false);
    bAttackerCanSneak = pAttacker->CanSneak();
    TESActorBase* pActorBase = pAttacker->GetActorBase();
    bAttackerCanMove = !pActorBase->IsImmobile();
}

float CombatController::GetPowerAttackForwardDistance(ANIM_GROUP_ENUM aePowerAttack) {
    float fDistance = 0.0f;
    NiPoint3 kDelta = NiPoint3::ZERO;
    if (GetPowerAttackDelta(aePowerAttack, kDelta)) {
        fDistance = kDelta.y;
    }
    return fDistance;
}

void CombatController::ClearMoveMode() {
    iMoveMode = 0;
    SetActualMoveMode(iMoveMode);
}

float CombatController::GetRunSpeedMult() {
    return fRunSpeedMult;
}

// Reads the procedure kind the save data names and rebuilds the procedure: created, initialised with this controller,
// then loaded. A kind of -1 means no procedure was saved.
void CombatController::LoadProcedure(CombatProcedure*& arpProcedure, BGSLoadFormBuffer* apLoadGameBuffer) {
    // One byte is read and sign-extended, so the local is a plain (signed) char.
    char cType = -1;
    apLoadGameBuffer->LoadData(&cType, sizeof(cType));
    if (cType != -1) {
        arpProcedure = CombatProcedure::CreateProcedure(cType);
        arpProcedure->Initialize(this);
        arpProcedure->LoadGame(apLoadGameBuffer);
    }
}

// Writes the procedure's kind and then the procedure's own save data; a null procedure writes -1 for the kind.
void CombatController::SaveProcedure(CombatProcedure*& arpProcedure, BGSSaveFormBuffer* apSaveGameBuffer) {
    char cType;
    if (arpProcedure) {
        cType = static_cast<char>(arpProcedure->GetType());
    }
    else {
        cType = -1;
    }
    apSaveGameBuffer->SaveDataEndian(&cType, sizeof(cType), 0);
    if (arpProcedure) {
        arpProcedure->SaveGame(apSaveGameBuffer);
    }
}

bool CombatController::IsFleeing() {
    return CombatPlanContainsAction(CombatAction::COMBAT_ACTION_FLEE) || eLowState == COMBAT_LOW_STATE_FLEEING;
}

bool CombatController::QSwimAllowed() {
    // Swimming is never allowed in combat in this build.
    return false;
}

void CombatController::SetMovementProcedure(CombatProcedure* apMovementProcedure) {
    if (pMovementProcedure) {
        pMovementProcedure->Shutdown();
        if (pMovementProcedure) {
            delete pMovementProcedure;
        }
    }
    pMovementProcedure = apMovementProcedure;
    if (pMovementProcedure) {
        pMovementProcedure->Initialize(this);
    }
}

void CombatController::SetActionProcedure(CombatProcedure* apActionProcedure) {
    if (pActionProcedure) {
        pActionProcedure->Shutdown();
        if (pActionProcedure) {
            delete pActionProcedure;
        }
    }
    pActionProcedure = apActionProcedure;
    if (pActionProcedure) {
        pActionProcedure->Initialize(this);
    }
}

void CombatController::ReloadingWeapon(TESObjectWEAP* apWeapon, bool abForced) {
    pCombatState->ReloadingWeapon(apWeapon, abForced);
}

uint8_t CombatController::IsSearching() {
    return pCombatGroup && pCombatGroup->IsSearching();
}

void CombatController::SetTargetSegmentInView(ACTOR_SEGMENT_INVIEW aeInView) {
    pCombatState->eTargetSegmentInView = aeInView;
}

void CombatController::InitPowerAttackDistances() {
    TESCombatStyle* pCombatStyle = GetCurrentCombatStyle();
    uint8_t bCanPowerAttack = pCombatStyle->QPowerAttackChance() > 0.0;
    bHasForwardAttacks = false;
    for (int32_t i = 0; i < 5; ++i) {
        float fForwardDistance = GetPowerAttackForwardDistance(static_cast<ANIM_GROUP_ENUM>(i + 92));
        if (bCanPowerAttack && fForwardDistance > 0.0) {
            fPowerAttackRanges[i] = GetAttacker()->GetScale() * fForwardDistance;
            bHasForwardAttacks = true;
        }
        else {
            fPowerAttackRanges[i] = 0.0f;
        }
        fPowerAttackHitTimes[i] = Actor::GetAnimGroupDuration(GetAttacker(), static_cast<ANIM_GROUP_ENUM>(i + 92));
    }
    bDeterminedPowerAttackDeltas = true;
}

bool CombatController::IsActoraCombatTarget(Actor* apActor) {
    return pCombatGroup && pCombatGroup->IsTarget(apActor);
}

TESObjectWEAP* CombatController::GetCurrentWeapon() {
    return pCurrentWeapon;
}

TESCombatStyle* CombatController::GetCurrentCombatStyle() {
    return pCurrentCombatStyle;
}

Actor* CombatController::GetAttacker() {
    return pAttacker;
}

bool CombatController::GetAimLocation(NiPoint3* arOut, const NiPoint3* arStartLocation) {
    return pActionProcedure && pActionProcedure->GetTargetedPoint(*arOut, const_cast<NiPoint3&>(*arStartLocation));
}

void CombatController::DebugCombatPlan() {
    CombatPlanner kPlanner;
    CombatPlan* pPlan = kPlanner.CreatePlan_0(this, true);
    if (pPlan) {
        delete pPlan;
    }
}

void CombatController::ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel) {
    if (aeNewLevel == PROCESS_TYPE_HIGH) {
        bFirstUpdate = false;
        ForcePlanUpdate();
        bool bFleeing = pCombatState->cFleeState != 0;
        pAttacker->pCurrentProcess->SetWantWeaponDrawn(!bFleeing);
        if (bFleeing) {
            float fTime = CombatTimer::GetTimer();
            pCombatState->SetLastFleeDoorTimeStamp(fTime);
        }
        if (pAttacker->Get3D()) {
            TESObjectWEAP* pWeapon = pCombatState->GetBestCombatWeapon();
            ItemChange* pcurrentWeaponItem = pAttacker->pCurrentProcess->GetCurrentWeapon();
            TESObjectWEAP* pcurrentWeapon = nullptr;
            if (pcurrentWeaponItem) {
                pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
            }
            if (!pWeapon || pWeapon == pcurrentWeapon) {
                if (!pWeapon && pcurrentWeapon) {
                    uint32_t uiCount = pcurrentWeaponItem->iNumber;
                    pAttacker->UnEquipObject(pcurrentWeapon, uiCount, nullptr, false, false, true);
                }
            }
            else {
                int32_t iCount = pWeapon->IsThrownWeapon() ? pAttacker->GetInventoryObjectCount(pWeapon) : 1;
                pAttacker->EquipObject(pWeapon, iCount, nullptr, true, false, true);
            }
        }
        else {
            pAttacker->pCurrentProcess->SetWeaponDrawn(pAttacker, !bFleeing);
        }
    }
    pCombatState->ChangeProcessLevel(aeOldLevel, aeNewLevel);
    if (aeOldLevel == PROCESS_TYPE_HIGH || aeNewLevel == PROCESS_TYPE_HIGH) {
        ClearCombatPlan();
        ClearProcedures();
        ClearBackgroundProcedures();
        fLowMovementDelta = -1.0f;
    }
}

void CombatController::UpdateCurrentCombatStyle() {
    TESCombatStyle* pCombatStyle = pCurrentCombatStyle;
    pCurrentCombatStyle = pAttacker->GetCombatStyle();
    if (pCombatStyle && pCombatStyle != pCurrentCombatStyle) {
        pCombatState->SetInventoryDirty(true);
        ForcePlanUpdate();
    }
}

void CombatController::ActivatedReference(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    int32_t aiCount,
    bool abActivatedScriptOnly
) {
    pCombatState->ActivatedReference(apRef, apObject, aiCount, abActivatedScriptOnly);
    if (pActionProcedure) {
        pActionProcedure->ActivatedReference(apRef, apObject, aiCount, abActivatedScriptOnly);
    }
    if (pMovementProcedure) {
        pMovementProcedure->ActivatedReference(apRef, apObject, aiCount, abActivatedScriptOnly);
    }
}

bool CombatController::GetPowerAttackDelta(ANIM_GROUP_ENUM aePowerAttack, NiPoint3& aNewPos) {
    return Actor::GetDistanceForAnimGroup(GetAttacker(), aePowerAttack, &aNewPos);
}

// bForceSneak is a uint8_t: the whole byte is tested.
void CombatController::SetActualMoveMode(uint32_t aiMoveMode) {
    uint32_t uiFlags = aiMoveMode;
    if (bForceSneak) {
        uiFlags |= ActorMover::MF_SNEAKING;
    }
    if (uiFlags) {
        pAttacker->SetMoveMode(uiFlags);
    }
    else {
        pAttacker->ClearMoveMode(iMoveMode);
    }
    iMoveMode = aiMoveMode;
}

CombatProcedure* CombatController::GetBackgroundProcedure(int32_t aiProcedureType) {
    for (CombatProcedure* pProcedure : BackgroundProcedures) {
        if (pProcedure->GetType() == aiProcedureType) {
            return pProcedure;
        }
    }
    return nullptr;
}

bool CombatController::CombatPlanContainsAction(int32_t aiAction) {
    if (pCombatPlan) {
        return pCombatPlan->HasAction(aiAction);
    }

    return false;
}

void CombatController::SetMoveMode(COMBAT_MOVE_MODE aeCombatMoveMode) {
    uint32_t uiFlags = iCombatMoveMode[aeCombatMoveMode];
    fWalkSpeedMult = 1.0f;
    fRunSpeedMult = 1.0f;
    pAttacker->SetWalkRunFlagDirty();
    if (aeCombatMoveMode == COMBAT_MOVE_MODE_FAST_WALK) {
        fWalkSpeedMult = 1.5f;
    }
    else if (aeCombatMoveMode == COMBAT_MOVE_MODE_SLOW_RUN) {
        fRunSpeedMult = 0.67f;
    }
    SetActualMoveMode(uiFlags);
}

void CombatController::FinishedPackageTask() {
    if (GetPretendInCombat() || GetContinueDuringCombat()) {
        TESPackage* pPackage = nullptr;
        BaseProcess* pProcess = pAttacker->pCurrentProcess;
        if (pProcess->ShouldUpdateCombatControllerOrEndCombat(pAttacker, pPackage)) {
            ForceStopCombat();
        }
        else if (pPackage) {
            UpdatePackageData(pPackage);
        }
        else {
            SetPretendInCombat(false);
            SetContinueDuringCombat(false);
        }
    }
}

// Records apTarget as a target of the combat group; for the player the compass first shows the attacker as hostile,
// and a group still shared with the player is split off.
void CombatController::AddTarget(
    Actor* apTarget,
    int32_t iPriority,
    bool abHighPriority,
    float afDamageDone,
    float afDamageTaken
) {
    if (!apTarget) {
        return;
    }
    if (apTarget == pPlayer) {
        pPlayer->ChangePerceivedActorHostileStatus(pAttacker, true);
        // The group is read through the Actor base pointer so that the call stays virtual.
        if (pCombatGroup == static_cast<Actor*>(pPlayer)->GetCombatGroup()) {
            pCombatGroup = pCombatManager->SplitPlayerCombatGroup(pAttacker);
        }
    }
    pCombatGroup->AddTarget(apTarget);
}

void CombatController::RemoveTarget(Actor* apActor) {
    if (apActor == pPlayer) {
        pPlayer->ChangePerceivedActorHostileStatus(pAttacker, false);
    }
    if (pCombatGroup) {
        pCombatGroup->RemoveTarget(apActor);
    }
}

bool CombatController::GetAttackerCanSneak() {
    return bAttackerCanSneak;
}

void CombatController::ClearProcedures() {
    SetActionProcedure(nullptr);
    SetMovementProcedure(nullptr);
}

void CombatController::ClearBackgroundProcedures() {
    for (uint32_t i = 0; i < BackgroundProcedures.GetSize(); ++i) {
        CombatProcedure* pProcedure = BackgroundProcedures[i];
        pProcedure->Shutdown();
        delete pProcedure;
    }
    BackgroundProcedures.Clear(true);
}

void CombatController::ForceCombat() {
    bForceCombat = true;
}

float CombatController::GetCombatPosition() {
    if (fCombatPosition < 0.0) {
        return 0.0f;
    }
    return fCombatPosition;
}

void CombatController::SetUpdatedDetection(bool abValue) {
    bUpdatedDetection = abValue;
}

void CombatController::ForcePlanUpdate() {
    PlanTimer.ForceTrigger();
}

void CombatController::CheckCurrentWeapon() {
    if (pCurrentWeapon) {
        COMBAT_WEAPON_TYPE eCombatWeaponType = pCurrentWeapon->GetCombatWeaponType();
        if (pCombatState->GetWeaponByCombatType(eCombatWeaponType) != pCurrentWeapon) {
            pCurrentWeapon = nullptr;
        }
    }
}

void CombatController::PerformedMeleeAttack(Actor* apTarget) {
    pCombatState->PerformedMeleeAttack(apTarget);
}

void CombatController::DamagedByAttacker(Actor* apAttacker, float afDamage) {
    pCombatState->DamagedByAttacker(apAttacker, afDamage);
}

bool CombatController::GetProcedureFailed() {
    if (pActionProcedure && pActionProcedure->GetFailed()) {
        return true;
    }
    return pMovementProcedure && pMovementProcedure->GetFailed();
}

bool CombatController::GetProcedureFinished() {
    if (pActionProcedure && pActionProcedure->GetFinished()) {
        return true;
    }
    if (pMovementProcedure && pMovementProcedure->GetFinished()) {
        return true;
    }
    return !pActionProcedure && !pMovementProcedure;
}

void CombatController::StartCalmed() {
    pAttacker->StopMoving();
}

bool CombatController::ShouldStopCombat() {
    if (bStopCombat) {
        return true;
    }
    if (pTarget || pCombatGroup->GetTargetCount() || TESPackage::GetPretendInCombat()) {
        bStoppingCombat = false;
        return false;
    }

    if (pAttacker->GetAnimAction() == -1 &&
        pAttacker->MobileObject::GetCurrentAIProcess()->GetRunOncePackage() == nullptr) {
        return true;
    }

    bStoppingCombat = true;
    return false;
}

void CombatController::SetForceSneak(bool abValue) {
    if (bAttackerCanSneak && bForceSneak != abValue) {
        bForceSneak = abValue;
        SetActualMoveMode(iMoveMode);
    }
}

uint32_t CombatController::GetLookAtTargetIndex(CombatProcedure* apCombatProcedure) {
    return apCombatProcedure == pActionProcedure || apCombatProcedure != pMovementProcedure;
}

void CombatController::SetLookAtTarget(uint8_t acIndex, const NiPoint3& arLookAtTarget, uint8_t acPriority) {
    NiPoint3& kLookAtTarget = kLookAtTargets[acIndex];
    kLookAtTarget = arLookAtTarget;
    ucLookAtTargetPriorities[acIndex] = acPriority;
}

void CombatController::ClearLookAtTarget(uint8_t acIndex) {
    ucLookAtTargetPriorities[acIndex] = 0;
}

int32_t CombatController::IsNotDetectingTarget() {
    return !pCombatState->GetHasBufferedDetection();
}

bool CombatController::IsSwitchingWeapons() {
    return GetBackgroundProcedure(5) != nullptr;
}

uint8_t CombatController::GetForceSneak() {
    return bForceSneak;
}

bool CombatController::GetUpdatedDetection() {
    return bUpdatedDetection;
}

void CombatController::SetCombatPosition(float afCombatPosition) {
    fCombatPosition = afCombatPosition;
}

double CombatController::GetPowerAttackRange(int32_t aiIndex) {
    return fPowerAttackRanges[aiIndex];
}

double CombatController::GetPowerAttackHitTime(int32_t aiIndex) {
    return fPowerAttackHitTimes[aiIndex];
}

char CombatController::ShouldInitPowerAttackDistances() {
    return bDeterminedPowerAttackDeltas;
}

void CombatController::FirstUpdate() {
    UpdateCurrentWeapon();
    pCombatState->DoInitialUpdate();
    if (pAttacker->MobileObject::GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
        CalculateAttackerInfo();
    }
}

void CombatController::TargetChanged() {
    pCombatState->TargetChanged();
    if (pActionProcedure) {
        pActionProcedure->TargetChanged();
    }
    if (pMovementProcedure) {
        pMovementProcedure->TargetChanged();
    }
}

void CombatController::UnInitializeCombatPlanData() {
    pCombatState->UnreserveCoverLocation();
}

ACTOR_SEGMENT_INVIEW CombatController::GetTargetSegmentInView() {
    return pCombatState->GetTargetSegmentInView();
}

uint8_t CombatController::HasLineOfSight() {
    return pCombatState->bTargetLineOfSight;
}

void CombatController::SetIsBlocking(char acBlocking) {
    *reinterpret_cast<hkEnum<hkpWorldObject::BroadPhaseType, int8_t>*>(
        pCombatState
    ) = static_cast<hkpWorldObject::BroadPhaseType>(acBlocking);
}

char CombatController::IsBlocking() {
    return pCombatState->GetBlocking();
}

bool CombatController::IsPlanUpdateForced() {
    return PlanTimer.GetTargetTime() == 0.0;
}

void CombatController::SetInventoryDirty(bool abValue) {
    pCombatState->SetInventoryDirty(abValue);
}

void CombatController::Disarmed(Actor* apOpponent) {
    bDeterminedPowerAttackDeltas = false;
    pCombatState->Disarmed();
    if (pActionProcedure) {
        pActionProcedure->Disarmed(apOpponent);
    }
}

void CombatController::ClearCombatPlan() {
    if (pCombatPlan) {
        UnInitializeCombatPlanData();
        if (pCombatPlan) {
            delete pCombatPlan;
        }
        pCombatPlan = nullptr;
    }
}

void CombatController::SetTarget(Actor* apTarget) {
    if (apTarget != pTarget) {
        if (pTarget) {
            if (pCombatGroup) {
                pCombatGroup->DecrementAttackerCount(pTarget);
            }
            pCombatManager->pTargetMap->RemoveTargetAttacker(pTarget, pAttacker);
        }
        Actor* pPrevTarget = pTarget;
        pTarget = apTarget;
        if (!bDestructing && !pAttacker->IsDead(false)) {
            if (!pPrevTarget || pTarget) {
                if (pTarget && !pPrevTarget && pCombatGroup->GetDetectionValue(apTarget) > 0) {
                    Script::SetActionFlag(pTarget, &pAttacker->m_Extra, SCRIPT_ACTION_ONSTARTCOMBAT);
                    pCombatDialogueManager->StartDialogue(this, DT_DETECTION, 4, true);
                }
            }
            else if (!pCombatGroup->iSearchCount) {
                pCombatDialogueManager->StartDialogue(this, DT_DETECTION, 6, false);
            }
        }
        if (pTarget) {
            pCombatGroup->IncrementAttackerCount(pTarget);
            pCombatManager->pTargetMap->AddTargetAttacker(pTarget, pAttacker);
        }
        if (!bDestructing) {
            if (!pCombatState->bInitializing && pAttacker->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                pCombatState->UpdateImmediateData();
            }
            TargetChanged();
        }
    }
}

bool CombatController::StopCombat(Actor* apTarget) {
    uint8_t bStopAll = 0;
    if (apTarget) {
        RemoveTarget(apTarget);
        if (pTarget == apTarget) {
            SetTarget(pCombatGroup->GetBestTarget(pAttacker));
            if (!pTarget) {
                ClearProcedures();
                ClearCombatPlan();
                ForcePlanUpdate();
            }
        }
    }
    else {
        bStopAll = 1;
    }
    Actor* pOwner = GetAttacker();
    if (pOwner->GetBlocked()) {
        GetAttacker()->SetBlock(false);
    }
    return bStopAll;
}

void CombatController::TakeHit(Actor* apAttacker, ActiveEffect* apEffect) {
    if (!bFirstUpdate) {
        FirstUpdate();
        bFirstUpdate = true;
    }
    if (pCombatGroup) {
        pCombatGroup->NotifyMemberHit(pAttacker, apAttacker);
    }
    if (apAttacker != GetTarget() && apAttacker != GetAttacker() && apAttacker->MobileObject::GetCurrentAIProcess()) {
        int32_t iHostility = 0;
        if (!IsActoraCombatTarget(apAttacker)) {
            int32_t iFightReaction[2];
            iFightReaction[1] = 0;
            iFightReaction[0] = 0;
            if (GetAttacker()->GetShouldAttackActor(
                    apAttacker,
                    true,
                    *reinterpret_cast<FIGHT_REACTION*>(iFightReaction),
                    false
                )) {
                iHostility = 100;
            }
            if (iHostility > 0) {
                BaseProcess* pProcess = apAttacker->MobileObject::GetCurrentAIProcess();
                int32_t iThreatDamage = static_cast<int32_t>(pProcess->GetPotentialThreatDamage(apAttacker, pAttacker));
                AddTarget(apAttacker, iThreatDamage, false, 0.0, 0.0);
            }
        }
    }
}

void CombatController::FiredProjectile(Projectile* apProjectile) {
    pCombatState->FiredProjectile(apProjectile);
    if (pActionProcedure) {
        pActionProcedure->FiredProjectile(apProjectile);
    }
    if (pMovementProcedure) {
        pMovementProcedure->FiredProjectile(apProjectile);
    }
}

void CombatController::UpdateLowMovement() {
    if (fLowMovementDelta > 0.0) {
        if (pAttacker->IsPathing()) {
            pAttacker->UpdateActorMover(fLowMovementDelta);
            pAttacker->UpdateMovement(fLowMovementDelta, false);
        }
        fLowMovementDelta = 0.0f;
    }
}

void CombatController::Update() {
    if (CombatController::bProcessCombat && !bStoppingCombat) {
        Actor* pattacker = pAttacker;
        if (pattacker && pattacker->pCurrentProcess) {
            if (!ShouldStopCombat()) {
                Animation* panimation = pattacker->GetAnimation();
                if (!pattacker->Get3D()) {
                    return;
                }
                if (!panimation) {
                    return;
                }
                int32_t eAttackerFadeState = pattacker->pCurrentProcess->GetFadeState();
                if (eAttackerFadeState && eAttackerFadeState != 2 && eAttackerFadeState != 1) {
                    return;
                }

                if (pattacker->GetSitSleepState()) {
                    return;
                }
                if (!bFirstUpdate) {
                    FirstUpdate();
                    bFirstUpdate = true;
                }
                if (pattacker->IsDying()) {
                    return;
                }
                if (!pattacker->IsDead(false)) {
                    UpdateCurrentWeapon();
                    Actor* pCombatTarget = GetTarget();
                    if (pCombatTarget &&
                        ((pCombatTarget->IsDead(false) && !pCombatTarget->IsDying()) ||
                         pCombatTarget->GetDisabled() ||
                         pCombatTarget->GetDelete())) {
                        pattacker->StopCombat(pCombatTarget);
                    }
                    else {
                        int16_t sAnimAction = pattacker->pCurrentProcess->GetAnimAction();
                        bool bTemporarilyImmobile = sAnimAction == ANIM_ACTION_RECOIL ||
                            sAnimAction == ANIM_ACTION_STAGGER;
                        if (!bTemporarilyImmobile && !pattacker->IsKnockedOut() && !pattacker->IsParalyzed()) {
                            pCombatState->Update();
                            if (!bStopCombat &&
                                !pCombatState->bInitializing &&
                                !panimation->GetAnimsLoading() &&
                                !pattacker->IsKnockedOut() &&
                                !pattacker->IsSurfacing()) {
                                bool bNeedNewPlan = false;
                                if (GetProcedureFailed()) {
                                    bNeedNewPlan = true;
                                    if (pActionProcedure && pActionProcedure->GetFailed()) {
                                        AddFailedCombatProcedure(pActionProcedure);
                                    }

                                    if (pMovementProcedure && pMovementProcedure->GetFailed()) {
                                        AddFailedCombatProcedure(pMovementProcedure);
                                    }
                                }
                                if (PlanTimer.Check()) {
                                    bNeedNewPlan = true;
                                }
                                if (!bNeedNewPlan && GetProcedureFinished() && pCombatPlan) {
                                    if (pCombatPlan->IsFinished()) {
                                        pCombatGroup->NotifyMemberPlanFinished(this, pCombatPlan->pGoal);
                                        bNeedNewPlan = true;
                                    }
                                    else {
                                        pCombatPlan->ExecuteNextAction(this);
                                    }
                                }

                                if (bNeedNewPlan) {
                                    ClearCombatPlan();
                                    ClearBackgroundProcedures();
                                    SetTarget(pCombatGroup->GetBestTarget(pAttacker));
                                    if (pMovementProcedure) {
                                        pMovementProcedure->StartedPlanning();
                                    }
                                    if (pActionProcedure) {
                                        pActionProcedure->StartedPlanning();
                                    }
                                    CombatAI_Planning.Start();
                                    CombatPlanner kPlanner;
                                    pCombatPlan = kPlanner.CreatePlan_0(this, false);
                                    pCurrentGoal = pCombatPlan ? pCombatPlan->pGoal : nullptr;
                                    InitializeCombatPlanData();
                                    kPlanner.CleanUp();
                                    pCombatPlannerData = nullptr;
                                    CombatAI_Planning.Stop();
                                    PlanTimer.Reset(1.0f);
                                    if (pCombatPlan) {
                                        pCombatPlan->ExecuteNextAction(this);
                                    }
                                    else {
                                        ClearProcedures();
                                        pCurrentAction = nullptr;
                                    }
                                    if (pMovementProcedure) {
                                        pMovementProcedure->FinishedPlanning();
                                    }
                                    if (pActionProcedure) {
                                        pActionProcedure->FinishedPlanning();
                                    }
                                    pCombatState->ClearCovers();
                                    UpdateCurrentWeapon();
                                }
                                if (pMovementProcedure) {
                                    pMovementProcedure->Update();
                                }
                                if (pActionProcedure) {
                                    pActionProcedure->Update();
                                }
                                if (pattacker->IsInCombat()) {
                                    for (int32_t i = 0; i < BackgroundProcedures.iSize; ++i) {
                                        CombatProcedure* pBackgroundProcedure = BackgroundProcedures[i];
                                        pBackgroundProcedure->Update();
                                        if (pBackgroundProcedure->GetFinished() || pBackgroundProcedure->GetFailed()) {
                                            if (pBackgroundProcedure->GetFailed()) {
                                                AddFailedCombatProcedure(pBackgroundProcedure);
                                            }
                                            pBackgroundProcedure->Shutdown();
                                            if (pBackgroundProcedure) {
                                                delete pBackgroundProcedure;
                                            }
                                            BackgroundProcedures.RemoveFast(i--, 1u);
                                        }
                                    }
                                    UpdateLookAtTarget();
                                    pattacker->SetIronSights(cUseIronSights == 1, false, false);
                                    eLowState = COMBAT_LOW_STATE_NONE;
                                }
                            }
                        }
                    }
                    return;
                }
            }
            pattacker->StopCombat(nullptr);
        }
    }
}

void CombatController::AddFailedCombatProcedure(CombatProcedure* apCombatProcedure) {
    ourMemcpy(FailedCombatProcedures, &FailedCombatProcedures[1], sizeof(FailedCombatProcedure) * 4);
    FailedCombatProcedures[4].iProcedureType = apCombatProcedure->GetType();
    FailedCombatProcedures[4].pFailureReason = apCombatProcedure->pFailureReason;
    FailedCombatProcedures[4].fTime = CombatTimer::GetTimer();
}

void CombatController::InitializeCombatPlanData() {
    int32_t iCombatLocation = pCombatPlan ? pCombatPlan->iCombatLocation : -1;
    bool bHasCombatLocation = false;
    bool bHasCoverLocation = false;
    if (iCombatLocation != -1) {
        CombatLocation* pCombatLocation = pCombatPlannerData->GetLocation(iCombatLocation);
        if (pCombatLocation) {
            bHasCombatLocation = true;
            pCombatState->SetCombatLocation(pCombatLocation);
            CombatCoverLocation* pCombatCoverLocation = pCombatPlannerData->GetCombatCoverLocation(iCombatLocation);
            if (pCombatCoverLocation) {
                bHasCoverLocation = true;
                pCombatState->SetCoverLocation(pCombatCoverLocation);
            }
            pCombatPlannerData->ClearLocation(iCombatLocation);
        }
        else {
            CombatLocation* pCurrentLocation = pCombatState->pCombatLocation;
            if (pCurrentLocation) {
                bHasCombatLocation = pCurrentLocation->iLocation == iCombatLocation;
            }
            CombatCoverLocation* pCoverLocation = pCombatState->GetCoverLocation();
            if (pCoverLocation && pCoverLocation->iLocation == iCombatLocation) {
                bHasCoverLocation = true;
            }
        }
    }
    if (!bHasCombatLocation) {
        pCombatState->ClearCombatLocation();
    }
    if (bHasCoverLocation) {
        pCombatState->ReserveCoverLocation();
    }
    else {
        pCombatState->ClearCoverLocation(false);
    }
}

void CombatController::UpdateLookAtTarget() {
    Actor* pOwner = GetAttacker();
    int32_t iBestIndex = -1;
    uint32_t uiBestPriority = 1;
    for (uint32_t i = 0; i < 2; ++i) {
        if (ucLookAtTargetPriorities[i] >= uiBestPriority) {
            uiBestPriority = ucLookAtTargetPriorities[i];
            iBestIndex = i;
        }
    }
    if (iBestIndex == -1 || uiBestPriority < static_cast<uint32_t>(ucIgnoreLookAtTarget) + 1) {
        if (fCombatPathingLookAheadDelta.Float() > 0.0 && pOwner->IsPathing() && pOwner->IsPathValid()) {
            NiPoint3 kLookAtTarget;
            kLookAtTarget = pOwner->CalculateAnticipatedLocation(fCombatPathingLookAheadDelta.Float());
            NiPoint3 kFiringLocation = CombatUtilities::GetAttackerFiringLocationSimple(pOwner);
            kLookAtTarget.z = kFiringLocation.z - pOwner->GetLocationOnReference().z + kLookAtTarget.z;
            pOwner->SetLookAtTarget(kLookAtTarget);
        }
        else {
            pOwner->ClearLookAtTarget();
        }
    }
    else {
        pOwner->SetLookAtTarget(kLookAtTargets[iBestIndex]);
    }
}

void CombatController::UpdateLow() {
    BGSWorldLocation kInvestigatorWorldLoc;
    BGSWorldLocation kAttackerWorldLoc;
    BGSWorldLocation kTargetWorldLoc;
    if (!bFirstUpdate) {
        FirstUpdate();
        bFirstUpdate = true;
    }
    if (fLowMovementDelta < 0.0) {
        PlanTimer.Reset(1.0f);
        fLowMovementDelta = 0.0f;
        if (pTarget && bEncounterZoneTargetRestrict.Bool()) {
            BGSEncounterZone* pTargetZone = pTarget->GetLocationEncounterZone();
            if (pTargetZone != pAttacker->GetLocationEncounterZone()) {
                SetTarget(nullptr);
            }
        }
    }
    if (PlanTimer.Check()) {
        bool bProceduresFinished = false;
        if (GetProcedureFailed()) {
            if (pActionProcedure && pActionProcedure->GetFailed()) {
                AddFailedCombatProcedure(pActionProcedure);
            }

            if (pMovementProcedure && pMovementProcedure->GetFailed()) {
                AddFailedCombatProcedure(pMovementProcedure);
            }

            SetTarget(nullptr);
        }
        else if (GetProcedureFinished()) {
            ClearProcedures();
            eLowState = COMBAT_LOW_STATE_NONE;
            bProceduresFinished = true;
        }
        if (!pTarget) {
            SetTarget(pCombatGroup->GetBestTarget(pAttacker));
            ClearProcedures();
            eLowState = COMBAT_LOW_STATE_NONE;
        }
        if (pTarget) {
            pCombatState->Update();
            COMBAT_LOW_STATE eNewState = COMBAT_LOW_STATE_NONE;
            pAttacker->GetWorldLocation(&kAttackerWorldLoc);
            pTarget->GetWorldLocation(&kTargetWorldLoc);
            if (pCombatState->CheckShouldFlee()) {
                eNewState = COMBAT_LOW_STATE_FLEEING;
            }
            else if (pCombatState->GetHasBufferedDetection()) {
                double dDistanceSquared = kAttackerWorldLoc.GetDistanceSquared(kTargetWorldLoc);
                double dMaxDistanceSquared = fCombatLowMaxAttackDistance.Float() * fCombatLowMaxAttackDistance.Float();
                if (dMaxDistanceSquared > dDistanceSquared) {
                    eNewState = COMBAT_LOW_STATE_ATTACKING;
                }
                else {
                    eNewState = COMBAT_LOW_STATE_PURSUING;
                }
            }
            else if (pCombatState->IsDetectionLost()) {
                if (pCombatGroup->IsSearching()) {
                    eNewState = COMBAT_LOW_STATE_SEARCHING;
                }
            }
            else {
                eNewState = COMBAT_LOW_STATE_INVESTIGATING;
            }
            if (eNewState != eLowState) {
                ClearProcedures();
                uint8_t cOldFleeState = pCombatState->cFleeState;
                pCombatState->cFleeState = 0;
                switch (eNewState) {
                    case COMBAT_LOW_STATE_ATTACKING: {
                        CombatProcedureAttackLow* pAttackLow = new CombatProcedureAttackLow;
                        SetActionProcedure(pAttackLow);
                        break;
                    }
                    case COMBAT_LOW_STATE_PURSUING: {
                        CombatLocation kPursueLocation;
                        kPursueLocation.SetRef(pTarget);
                        if (pCombatState->CheckMovement(kPursueLocation.WorldLoc, true, true)) {
                            CombatProcedure* pMoveProcedure = new CombatProcedureMove(
                                &kPursueLocation,
                                kCombatLocationTypeData[0].fRadius,
                                0
                            );
                            SetMovementProcedure(pMoveProcedure);
                        }
                        break;
                    }
                    case COMBAT_LOW_STATE_INVESTIGATING: {
                        eNewState = COMBAT_LOW_STATE_NONE;
                        CombatLocation kInvestigateLocation;
                        BGSWorldLocation lastDetectedLocation;
                        if (pCombatGroup->GetTargetLastDetectedLocation(pTarget, lastDetectedLocation) &&
                            pCombatState->CheckMovement(lastDetectedLocation, true, true)) {
                            pAttacker->GetWorldLocation(&kInvestigatorWorldLoc);
                            if (kInvestigatorWorldLoc.GetDistanceSquared(lastDetectedLocation) > 100.0) {
                                kInvestigateLocation.SetLocation(lastDetectedLocation);
                                CombatProcedure* pMoveProcedure = new CombatProcedureMove(
                                    &kInvestigateLocation,
                                    kCombatLocationTypeData[0].fRadius,
                                    15
                                );
                                SetMovementProcedure(pMoveProcedure);
                                eNewState = COMBAT_LOW_STATE_INVESTIGATING;
                            }
                        }
                        break;
                    }
                    case COMBAT_LOW_STATE_SEARCHING: {
                        CombatProcedureSearch* pSearch = new CombatProcedureSearch;
                        SetMovementProcedure(pSearch);
                        break;
                    }
                    case COMBAT_LOW_STATE_FLEEING: {
                        double dDistanceSquared;
                        if ((bProceduresFinished && cOldFleeState) ||
                            (dDistanceSquared = kAttackerWorldLoc.GetDistanceSquared(kTargetWorldLoc),
                             !(fCombatLowMaxAttackDistance.Float() * fCombatLowMaxAttackDistance.Float() >
                               dDistanceSquared))) {
                            pCombatState->cFleeState = 2;
                        }
                        else {
                            pCombatState->cFleeState = 1;
                            CombatLocation kFleeLocation;
                            if (pCombatState->FindBestFleeLocation(nullptr, &kFleeLocation, 12)) {
                                CombatProcedure* pFleeProcedure;
                                if (kFleeLocation.GetFlag(CombatLocation::FLAG_EVADE)) {
                                    pFleeProcedure = new CombatProcedureEvade(12);
                                }
                                else {
                                    pFleeProcedure = new CombatProcedureMove(
                                        &kFleeLocation,
                                        kCombatLocationTypeData[12].fRadius,
                                        12
                                    );
                                }
                                SetMovementProcedure(pFleeProcedure);
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
                eLowState = eNewState;
            }
            if (pMovementProcedure) {
                pMovementProcedure->Update();
            }
            if (pActionProcedure) {
                pActionProcedure->Update();
            }
            if (pAttacker->IsPathing()) {
                fLowMovementDelta = PlanTimer.ElapsedTime();
                pAttacker->SetAngleOnReferenceX(std::numeric_limits<float>::max());
            }
            PlanTimer.Reset(1.0f);
        }
    }
}

int32_t CombatController::GetAttackSkill() {
    int32_t iResult = 0;
    if (pAttacker->IsCreature()) {
        Creature* pCreature = dynamic_cast<Creature*>(pAttacker);
        if (pCreature) {
            return static_cast<int32_t>(pCreature->GetWeaponSkill());
        }
    }
    else {
        if (GetCurrentWeapon()) {
            ActorValue::Index eWeaponSkill = GetCurrentWeapon()->GetWeaponSkill();
            return pAttacker->GetClampedActorValue(eWeaponSkill);
        }

        return pAttacker->GetClampedActorValue(ActorValue::UNARMED);
    }
    return iResult;
}

void CombatController::AddBackgroundProcedure(CombatProcedure* apBackgroundProcedure) {
    for (uint32_t i = 0; i < BackgroundProcedures.iSize; ++i) {
        CombatProcedure* pBackgroundProcedure = BackgroundProcedures[i];
        CombatProcedure::COMBAT_PROCEDURES eType = pBackgroundProcedure->GetType();
        if (eType == apBackgroundProcedure->GetType()) {
            pBackgroundProcedure->Shutdown();
            if (pBackgroundProcedure) {
                delete pBackgroundProcedure;
            }
            BackgroundProcedures.RemoveFast(i, 1u);
            break;
        }
    }
    BackgroundProcedures.Add(apBackgroundProcedure);
    apBackgroundProcedure->Initialize(this);
}

void CombatController::UpdatePackageData(TESPackage* apCurrentPackage) {
    if (GetPretendInCombat() || GetContinueDuringCombat()) {
        CombatLocationTarget* pLocationTarget = pCombatState->pLocationTarget;
        if (pLocationTarget && (pLocationTarget->cType == 1 || !pLocationTarget->cType)) {
            pCombatState->ClearLocationTarget();
        }

        pCombatState->RemoveTargetedObject(2);
    }
    SetPretendInCombat(false);
    SetContinueDuringCombat(false);
    pCombatStyle = nullptr;
    if (apCurrentPackage) {
        pCombatStyle = apCurrentPackage->pCombatStyle;
        uint32_t uiPackageFlags = apCurrentPackage->PackData.iPackFlags;
        if ((uiPackageFlags & (kPackageFlag_PretendInCombat | kPackageFlag_ContinueDuringCombat)) != 0) {
            SetPretendInCombat((uiPackageFlags & kPackageFlag_PretendInCombat) != 0);
            SetContinueDuringCombat((uiPackageFlags & kPackageFlag_ContinueDuringCombat) != 0);
            BaseProcess* pProc = pAttacker->pCurrentProcess;
            bool bHasRefForLocation = true;
            if (apCurrentPackage->pPackLoc) {
                PACK_LOC_TYPE eLocType = apCurrentPackage->pPackLoc->GetLocType();
                if (eLocType > PLT_REFERENCE) {
                    bHasRefForLocation = eLocType > PLT_EDITORLOC;
                }
            }
            TESObjectREFR* pRef = pProc->GetCurrentDestinationReference(pAttacker, false);
            // The destination radius and the package target's value share one float: the default case passes the radius
            // unless the package target replaced it.
            float fRadius = pProc->GetCurrentDestinationRadius(pAttacker);
            if (!pRef || pRef == pAttacker) {
                pRef = pProc->GetTarget();
                if (!pRef &&
                    apCurrentPackage->GetPackageTarget() &&
                    apCurrentPackage->GetPackageTarget()->GetTargReference()) {
                    pRef = apCurrentPackage->GetPackageTarget()->GetTargReference();
                    fRadius = static_cast<float>(apCurrentPackage->GetPackageTarget()->iValue);
                }
            }
            switch (apCurrentPackage->GetPackType()) {
                case PACKAGE_TYPE_EXPLORE:
                case PACKAGE_TYPE_FIND: {
                    TESBoundObject* pBaseObject = pRef ? pRef->GetObjectReference() : nullptr;
                    pCombatState->AddObjectToTarget(2u, pRef, pBaseObject);
                    break;
                }
                case PACKAGE_TYPE_FOLLOW:
                case PACKAGE_TYPE_ACCOMPANY: {
                    bool bTeammate = pAttacker->IsPlayerTeammate();
                    float fMult = bTeammate ? fCombatTeammateFollowRadiusMult.Float() : fCombatFollowRadiusMult.Float();
                    float fMin = bTeammate ? fCombatTeammateFollowRadiusMin.Float() : fCombatFollowRadiusMin.Float();
                    float fBase = bTeammate ? fCombatTeammateFollowRadiusBase.Float() : fCombatFollowRadiusBase.Float();
                    float fGuardDistanceMin = static_cast<float>(apCurrentPackage->GetPackageTarget()->iValue);
                    float fGuardDistanceMax = fGuardDistanceMin * fMult + fBase;
                    fGuardDistanceMax = BSMAX(fGuardDistanceMax, fMin);
                    if (bHasRefForLocation) {
                        pCombatState
                            ->SetLocationTargetRef(pRef, LOCATION_TARGET_GUARD, fGuardDistanceMin, fGuardDistanceMax);
                    }
                    else {
                        TESObjectCELL* pCell = pProc->GetCurrentDestinationCell(pAttacker);
                        TESForm* pSpace;
                        if (pCell) {
                            if (pCell->GetInterior()) {
                                pSpace = pCell;
                            }
                            else {
                                pSpace = pCell->GetWorldSpace();
                            }
                        }
                        else {
                            pSpace = pProc->GetCurrentDestinationWorldSpace(pAttacker);
                        }
                        BGSWorldLocation kWorldLoc(pProc->GetCurrentDestinationCoordinate(pAttacker, true), pSpace);
                        pCombatState->SetLocationTargetLocation(&kWorldLoc, 1, fGuardDistanceMin, fGuardDistanceMax);
                    }
                    break;
                }
                case PACKAGE_TYPE_GUARD: {
                    float fGuardDistanceMin;
                    if (IsPackageFlag4Set(apCurrentPackage, nullptr)) {
                        bHasRefForLocation = true;
                    }
                    PackageTarget* ppackTarg = apCurrentPackage->GetPackageTarget();
                    if (apCurrentPackage->SandboxGetAllowConversation() && apCurrentPackage->pPackLoc) {
                        fGuardDistanceMin = static_cast<float>(apCurrentPackage->GetLocationRadius(pAttacker));
                    }
                    else if (ppackTarg) {
                        fGuardDistanceMin = static_cast<float>(ppackTarg->iValue);
                    }
                    else {
                        fGuardDistanceMin = 30.0f;
                    }
                    float fGuardDistanceMax = fGuardDistanceMin * fCombatGuardRadiusMult.Float();
                    fGuardDistanceMax = BSMAX(fGuardDistanceMax, fCombatGuardRadiusMin.Float());
                    if (bHasRefForLocation) {
                        pCombatState
                            ->SetLocationTargetRef(pRef, LOCATION_TARGET_GUARD, fGuardDistanceMin, fGuardDistanceMax);
                    }
                    else {
                        TESObjectCELL* pCell = pProc->GetCurrentDestinationCell(pAttacker);
                        TESForm* pSpace;
                        if (pCell) {
                            if (pCell->GetInterior()) {
                                pSpace = pCell;
                            }
                            else {
                                pSpace = pCell->GetWorldSpace();
                            }
                        }
                        else {
                            pSpace = pProc->GetCurrentDestinationWorldSpace(pAttacker);
                        }
                        BGSWorldLocation kWorldLoc(pProc->GetCurrentDestinationCoordinate(pAttacker, true), pSpace);
                        pCombatState->SetLocationTargetLocation(&kWorldLoc, 1, fGuardDistanceMin, fGuardDistanceMax);
                    }
                    break;
                }
                default: {
                    if (bHasRefForLocation) {
                        pCombatState->SetLocationTargetRef(pRef, LOCATION_TARGET_DESTINATION, fRadius, fRadius);
                    }
                    else {
                        TESObjectCELL* pCell = pProc->GetCurrentDestinationCell(pAttacker);
                        TESForm* pSpace;
                        if (pCell) {
                            if (pCell->GetInterior()) {
                                pSpace = pCell;
                            }
                            else {
                                pSpace = pCell->GetWorldSpace();
                            }
                        }
                        else {
                            pSpace = pProc->GetCurrentDestinationWorldSpace(pAttacker);
                        }
                        BGSWorldLocation kWorldLoc(pProc->GetCurrentDestinationCoordinate(pAttacker, true), pSpace);
                        pCombatState->SetLocationTargetLocation(&kWorldLoc, 0, fRadius, fRadius);
                    }
                    ForcePlanUpdate();
                    pAttacker->ResumeMoving();
                    break;
                }
            }
        }
        UpdateCurrentCombatStyle();
    }
    else {
        UpdateCurrentCombatStyle();
    }
}

bool CombatController::GetShowVerboseMessages() {
    return CombatController::bShowVerboseMessages;
}

void CombatController::SetShowVerboseMessages(bool abShowVerboseMessages) {
    CombatController::bShowVerboseMessages = abShowVerboseMessages;
}

bool CombatController::CheckHasEquip(Actor* apAttacker, ItemChange* apWeapon) {
    uint16_t usGroup = apAttacker->GetAnimGroup(ANIM_GROUP_WEAPONS_START, apWeapon, apWeapon == nullptr, nullptr);
    if (usGroup) {
        Animation* pAnimation = apAttacker->GetAnimation();
        if (pAnimation) {
            AnimSequenceBase* pSequence;
            if (pAnimation->GetMapAnimSequenceBase()->GetAt(usGroup, pSequence)) {
                return true;
            }
        }
    }
    return false;
}

TESForm* BGSWorldLocation::GetSpace() {
    return pSpace;
}

CombatLocation::CombatLocation() : pRef(nullptr), iLocation(-1), fRadius(0.0f), iFlags(0) {}

void CombatState::UnreserveCoverLocation() {
    UnreserveCoverLocation(pCoverLocation);
}

CombatTimeStamp CombatState::GetLastTargetDetectedTimeStamp() {
    return fLastTargetDetectedTimeStamp;
}

CombatTimeStamp CombatState::GetLastTarget360LineOfSightTimeStamp() {
    return fLastTarget360LineOfSightTimeStamp;
}

CombatTimeStamp CombatState::GetLastAttackTimeStamp() {
    return fLastAttackTimeStamp;
}

bool CombatState::GetBlocking() {
    return bBlocking;
}

bool CombatState::GetLocationUnreachable(int32_t aiLocation) {
    return (iLocationUnreachable & aiLocation) != 0;
}

float CombatTimer::GetTargetTime() {
    return fTargetTime;
}
