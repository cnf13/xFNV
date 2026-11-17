#include "Fallout/AI/CombatController.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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

// The actor move mode flags of each combat move mode, indexed by COMBAT_MOVE_MODE.
using CombatMoveModeFlagsArray = uint32_t[COMBAT_MOVE_MODE_COUNT];
static CombatMoveModeFlagsArray iCombatMoveMode{256, 256, 1024, 512, 512};

using CombatModeNameArray = const char* [6];
static CombatModeNameArray CombatLowStateNames{"None",          "Attacking", "Pursuing",
                                               "Investigating", "Searching", "Fleeing"};
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
    return ((apThis->PackData.iPackageSpecificFlags & 4) != 0);
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
    float ftimeDelay = static_cast<double>(iStaggerPeriodicUpdateCounter % 10) * 0.10000000149011612;
    ++iStaggerPeriodicUpdateCounter;
    pCombatState = new CombatState(this, ftimeDelay);
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
    char cactionType;
    uint32_t isaveGameCount;
    uint32_t FormHealth_0;
    char apData;
    char v9;
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pAttacker, 0);
    apSaveGameBuffer->SaveFormID(pTarget, 0);
    FormHealth_0 = pCombatGroup->iGroupID;
    apSaveGameBuffer->SaveDataEndian(&FormHealth_0, 4u, 0);
    pCombatState->SaveGame(apSaveGameBuffer);
    SaveProcedure(pActionProcedure, apSaveGameBuffer);
    SaveProcedure(pMovementProcedure, apSaveGameBuffer);
    isaveGameCount = BackgroundProcedures.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SaveProcedure(BackgroundProcedures.operator[](i), apSaveGameBuffer);
    }
    apData = pCombatPlan != nullptr;
    apSaveGameBuffer->SaveDataEndian(&apData, 1u, 0);
    if (apData) {
        pCombatPlan->SaveGame(apSaveGameBuffer);
    }
    if (pCurrentAction) {
        cactionType = pCurrentAction->eType;
    }
    else {
        cactionType = -1;
    }
    v9 = cactionType;
    apSaveGameBuffer->SaveDataEndian(&v9, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iCurrentActionValue, 4u, 0);
    PlanTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bForceSneak, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iMoveMode, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fWalkSpeedMult, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fRunSpeedMult, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bFirstUpdate, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bStopCombat, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bStoppingCombat, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&eLowState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fLowMovementDelta, 4u, 0);
    for (uint32_t i_2 = 0; i_2 < 2; ++i_2) {
        apSaveGameBuffer->SaveDataEndian(&kLookAtTargets[i_2], 12u, 0);
        apSaveGameBuffer->SaveDataEndian(&ucLookAtTargetPriorities[i_2], 1u, 0);
    }
    apSaveGameBuffer->SaveDataEndian(&ucIgnoreLookAtTarget, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cUseIronSights, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bUpdatedDetection, 1u, 0);
}

void CombatController::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t icombatGroupID;
    bool bhasPlan;
    char cactionType;
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pAttacker);
    apLoadGameBuffer->LoadFormID(pTarget);
    icombatGroupID = 0;
    apLoadGameBuffer->LoadData(&icombatGroupID, 4u);
    pCombatGroup = pCombatManager->GetCombatGroup(icombatGroupID);
    pCombatState->LoadGame(apLoadGameBuffer);
    LoadProcedure(pActionProcedure, apLoadGameBuffer);
    LoadProcedure(pMovementProcedure, apLoadGameBuffer);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    BackgroundProcedures.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        LoadProcedure(BackgroundProcedures.operator[](i), apLoadGameBuffer);
    }
    bhasPlan = false;
    apLoadGameBuffer->LoadData(&bhasPlan, 1u);
    if (bhasPlan) {
        CombatPlan* v2;
        CombatPlan* apMemory = static_cast<CombatPlan*>(ourMalloc(sizeof(CombatPlan)));
        if (apMemory) {
            v2 = ::new (apMemory) CombatPlan(this);
        }
        else {
            v2 = nullptr;
        }
        pCombatPlan = v2;
        pCombatPlan->LoadGame(apLoadGameBuffer);
        pCurrentGoal = pCombatPlan->pGoal;
    }
    cactionType = -1;
    apLoadGameBuffer->LoadData(&cactionType, 1u);
    if (cactionType != -1) {
        pCurrentAction = CombatPlan::GetCombatAction(cactionType);
    }
    apLoadGameBuffer->LoadData(&iCurrentActionValue, 4u);
    PlanTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&bForceSneak, 1u);
    apLoadGameBuffer->LoadData(&iMoveMode, 4u);
    apLoadGameBuffer->LoadData(&fWalkSpeedMult, 4u);
    apLoadGameBuffer->LoadData(&fRunSpeedMult, 4u);
    apLoadGameBuffer->LoadData(&bFirstUpdate, 1u);
    apLoadGameBuffer->LoadData(&bStopCombat, 1u);
    apLoadGameBuffer->LoadData(&bStoppingCombat, 1u);
    apLoadGameBuffer->LoadData(&eLowState, 4u);
    apLoadGameBuffer->LoadData(&fLowMovementDelta, 4u);
    for (uint32_t i_2 = 0; i_2 < 2; ++i_2) {
        apLoadGameBuffer->LoadData(&kLookAtTargets[i_2], 12u);
        apLoadGameBuffer->LoadData(&ucLookAtTargetPriorities[i_2], 1u);
    }
    apLoadGameBuffer->LoadData(&ucIgnoreLookAtTarget, 1u);
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        apLoadGameBuffer->LoadData(&cUseIronSights, 1u);
        apLoadGameBuffer->LoadData(&bUpdatedDetection, 1u);
    }
}

void CombatController::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* v4;
    Actor* v5;
    TESPackage::InitLoadGame(apLoadGameBuffer);
    if (pAttacker) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttacker)));
        v5 = dynamic_cast<Actor*>(FormByNumericID);
    }
    else {
        v5 = nullptr;
    }
    pAttacker = v5;
    if (pTarget) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        v4 = dynamic_cast<Actor*>(v3);
    }
    else {
        v4 = nullptr;
    }
    pTarget = v4;
    if (pTarget) {
        pCombatManager->pTargetMap->AddTargetAttacker(pTarget, pAttacker);
    }
    if (pActionProcedure) {
        pActionProcedure->InitLoadGame(apLoadGameBuffer);
    }
    if (pMovementProcedure) {
        pMovementProcedure->InitLoadGame(apLoadGameBuffer);
    }
    uint32_t FormFlags = BackgroundProcedures.iSize;
    for (uint32_t auiIndex = 0; auiIndex < FormFlags; ++auiIndex) {
        CombatProcedure** pbackgroundProcedure = &BackgroundProcedures.operator[](auiIndex);
        (*pbackgroundProcedure)->InitLoadGame(apLoadGameBuffer);
    }
    pCombatState->InitLoadGame(apLoadGameBuffer);
    if (pCombatPlan) {}
    if ((pCombatGroup) && (!pCombatGroup->IsMember(pAttacker) && !pCombatGroup->AddMember(pAttacker))) {
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
    CombatProcedureSwitchWeapon* pswitchWeapon = static_cast<CombatProcedureSwitchWeapon*>(GetBackgroundProcedure(5));
    if (pswitchWeapon) {
        pCurrentWeapon = pswitchWeapon->pWeapon;
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
    bAttackerCanSneak = (bool)((int32_t)pAttacker->CanSneak());
    TESActorBase* actorBase = pAttacker->GetActorBase();
    bAttackerCanMove = (bool)((uint8_t)(!actorBase->IsImmobile()));
}

float CombatController::GetPowerAttackForwardDistance(ANIM_GROUP_ENUM aePowerAttack) {
    float fDistance = 0.0f;
    NiPoint3 Delta = NiPoint3_ZERO;
    if (GetPowerAttackDelta(aePowerAttack, Delta)) {
        fDistance = Delta.y;
    }
    return fDistance;
}

void CombatController::ClearMoveMode() {
    iMoveMode = (uint32_t)0;
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
    apLoadGameBuffer->LoadData(&cType, 1u);
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
    apSaveGameBuffer->SaveDataEndian(&cType, 1u, 0);
    if (arpProcedure) {
        arpProcedure->SaveGame(apSaveGameBuffer);
    }
}

bool CombatController::IsFleeing() {
    return (bool)((uint8_t)(CombatPlanContainsAction(21) || eLowState == COMBAT_LOW_STATE_FLEEING));
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
    Actor* packageOwner;
    Actor* v3;
    float yDistanceForAnimGroup;
    int32_t i;
    TESCombatStyle* pCombatStyle = GetCurrentCombatStyle();
    uint8_t bCanPowerAttack = (uint8_t)(((double)pCombatStyle->QPowerAttackChance()) > 0.0);
    bHasForwardAttacks = false;
    for (i = 0; i < 5; ++i) {
        yDistanceForAnimGroup = GetPowerAttackForwardDistance(static_cast<ANIM_GROUP_ENUM>(i + 92));
        if (bCanPowerAttack && (yDistanceForAnimGroup > 0.0)) {
            packageOwner = GetAttacker();
            fPowerAttackRanges[i] = (packageOwner->GetScale() * yDistanceForAnimGroup);
            bHasForwardAttacks = true;
        }
        else {
            fPowerAttackRanges[i] = (float)0.0;
        }
        v3 = GetAttacker();
        fPowerAttackHitTimes[i] = Actor::GetAnimGroupDuration(v3, static_cast<ANIM_GROUP_ENUM>(i + 92));
    }
    bDeterminedPowerAttackDeltas = true;
}

bool CombatController::IsActoraCombatTarget(Actor* apActor) {
    return (bool)((uint8_t)(pCombatGroup && pCombatGroup->IsTarget(apActor)));
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
    CombatPlan* v3 = kPlanner.CreatePlan_0(this, true);
    if (v3) {
        delete v3;
    }
}

void CombatController::ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel) {
    if (aeNewLevel == PROCESS_TYPE_HIGH) {
        bFirstUpdate = false;
        ForcePlanUpdate();
        bool bfleeing = pCombatState->cFleeState != 0;
        BaseProcess* CurrentAIProcess = pAttacker->pCurrentProcess;
        CurrentAIProcess->SetWantWeaponDrawn(!bfleeing);
        if (bfleeing) {
            float afValue = CombatTimer::GetTimer();
            pCombatState->SetLastFleeDoorTimeStamp(afValue);
        }
        if (pAttacker->Get3D()) {
            TESObjectWEAP* pcurrentWeapon;
            TESObjectWEAP* pweapon = pCombatState->GetBestCombatWeapon();
            BaseProcess* v8 = pAttacker->pCurrentProcess;
            ItemChange* pcurrentWeaponItem = v8->GetCurrentWeapon();
            if (pcurrentWeaponItem) {
                pcurrentWeapon = static_cast<TESObjectWEAP*>(pcurrentWeaponItem->pContainerObj);
            }
            else {
                pcurrentWeapon = nullptr;
            }
            if (!pweapon || pweapon == pcurrentWeapon) {
                if ((!pweapon) && (pcurrentWeapon)) {
                    uint32_t FormHealth_0 = pcurrentWeaponItem->iNumber;
                    pAttacker->UnEquipObject(pcurrentWeapon, FormHealth_0, nullptr, false, false, true);
                }
            }
            else {
                int32_t count;
                if (pweapon->IsThrownWeapon()) {
                    count = pAttacker->GetInventoryObjectCount(pweapon);
                }
                else {
                    count = 1;
                }
                pAttacker->EquipObject(pweapon, count, nullptr, true, false, true);
            }
        }
        else {
            BaseProcess* v5 = pAttacker->pCurrentProcess;
            v5->SetWeaponDrawn(pAttacker, !bfleeing);
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
    if ((pCombatStyle) && (pCombatStyle != pCurrentCombatStyle)) {
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
        uiFlags |= 0x400;
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
    for (uint32_t auiIndex = 0; auiIndex < BackgroundProcedures.iSize; ++auiIndex) {
        CombatProcedure* v4 = BackgroundProcedures.operator[](auiIndex);
        if (v4->GetType() == aiProcedureType) {
            return v4;
        }
    }
    return nullptr;
}

bool CombatController::CombatPlanContainsAction(int32_t aiAction) {
    if (pCombatPlan) {
        return (bool)pCombatPlan->HasAction(aiAction);
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
    TESPackage* ppackage;
    if (GetPretendInCombat() || GetContinueDuringCombat()) {
        ppackage = nullptr;
        BaseProcess* pProcess = pAttacker->pCurrentProcess;
        if (pProcess->ShouldUpdateCombatControllerOrEndCombat(pAttacker, ppackage)) {
            ForceStopCombat();
        }
        else {
            if (ppackage) {
                UpdatePackageData(ppackage);
            }
            else {
                SetPretendInCombat(false);
                SetContinueDuringCombat(false);
            }
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
    return (bool)((uint8_t)bAttackerCanSneak);
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
    if (!(fCombatPosition < 0.0)) {
        return fCombatPosition;
    }

    return (float)0.0;
}

void CombatController::SetUpdatedDetection(bool abValue) {
    bUpdatedDetection = (bool)((uint8_t)abValue);
}

void CombatController::ForcePlanUpdate() {
    (&PlanTimer)->ForceTrigger();
}

void CombatController::CheckCurrentWeapon() {
    COMBAT_WEAPON_TYPE combatWeaponType;
    if (pCurrentWeapon) {
        combatWeaponType = pCurrentWeapon->GetCombatWeaponType();
        if (pCombatState->GetWeaponByCombatType(combatWeaponType) != pCurrentWeapon) {
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
    return (bool)((uint8_t)(pMovementProcedure && pMovementProcedure->GetFailed()));
}

bool CombatController::GetProcedureFinished() {
    if (pActionProcedure && pActionProcedure->GetFinished()) {
        return true;
    }
    if (pMovementProcedure && pMovementProcedure->GetFinished()) {
        return true;
    }
    return (bool)((uint8_t)((!pActionProcedure) && (!pMovementProcedure)));
}

void CombatController::StartCalmed() {
    pAttacker->StopMoving();
}

bool CombatController::ShouldStopCombat() {
    BaseProcess* currentProcess;
    uint8_t v4;
    if (bStopCombat) {
        return true;
    }
    if (pTarget || (pCombatGroup->GetTargetCount() || TESPackage::GetPretendInCombat())) {
        bStoppingCombat = false;
        return false;
    }

    if (pAttacker->GetAnimAction() == (-1)) {
        currentProcess = pAttacker->MobileObject::GetCurrentAIProcess();
        v4 = (uint8_t)(currentProcess->GetRunOncePackage() == nullptr);
    }
    else {
        v4 = 0;
    }
    if (v4) {
        return true;
    }

    bStoppingCombat = true;
    return false;
}

void CombatController::SetForceSneak(bool abValue) {
    if ((bAttackerCanSneak) && (bForceSneak != ((uint8_t)abValue))) {
        bForceSneak = (bool)((uint8_t)abValue);
        SetActualMoveMode(iMoveMode);
    }
}

uint32_t CombatController::GetLookAtTargetIndex(CombatProcedure* apCombatProcedure) {
    return (uint32_t)((uint8_t)((apCombatProcedure == pActionProcedure) || (apCombatProcedure != pMovementProcedure)));
}

void CombatController::SetLookAtTarget(uint8_t acIndex, const NiPoint3& arLookAtTarget, uint8_t acPriority) {
    NiPoint3* v4 = &kLookAtTargets[acIndex];
    v4->x = (&arLookAtTarget)->x;
    v4->y = (&arLookAtTarget)->y;
    v4->z = (&arLookAtTarget)->z;
    ucLookAtTargetPriorities[acIndex] = (uint8_t)((int8_t)acPriority);
}

void CombatController::ClearLookAtTarget(uint8_t acIndex) {
    ucLookAtTargetPriorities[acIndex] = (uint8_t)0;
}

int32_t CombatController::IsNotDetectingTarget() {
    return (int32_t)((uint8_t)(!pCombatState->GetHasBufferedDetection()));
}

bool CombatController::IsSwitchingWeapons() {
    return (GetBackgroundProcedure(5) != nullptr);
}

uint8_t CombatController::GetForceSneak() {
    return bForceSneak;
}

bool CombatController::GetUpdatedDetection() {
    return (bool)((uint8_t)bUpdatedDetection);
}

void CombatController::SetCombatPosition(float afCombatPosition) {
    fCombatPosition = afCombatPosition;
}

double CombatController::GetPowerAttackRange(int32_t a2) {
    return fPowerAttackRanges[a2];
}

double CombatController::GetPowerAttackHitTime(int32_t a2) {
    return fPowerAttackHitTimes[a2];
}

char CombatController::ShouldInitPowerAttackDistances() {
    return (int8_t)((uint8_t)bDeterminedPowerAttackDeltas);
}

void CombatController::FirstUpdate() {
    UpdateCurrentWeapon();
    pCombatState->DoInitialUpdate();
    if (((int32_t)pAttacker->MobileObject::GetCurrentProcessType()) == 0) {
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

void CombatController::SetIsBlocking(char a2) {
    reinterpret_cast<hkEnum<hkpWorldObject::BroadPhaseType, int8_t>*>(pCombatState)
        ->operator=(static_cast<hkpWorldObject::BroadPhaseType>(a2));
}

char CombatController::IsBlocking() {
    return pCombatState->GetBlocking();
}

bool CombatController::IsPlanUpdateForced() {
    return (bool)((uint8_t)((&PlanTimer)->GetTargetTime() == 0.0));
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
            Actor* Attacker = pAttacker;
            pCombatManager->pTargetMap->RemoveTargetAttacker(pTarget, Attacker);
        }
        Actor* pPrevTarget = pTarget;
        pTarget = apTarget;
        if (!bDestructing && !pAttacker->IsDead(false)) {
            if (!pPrevTarget || pTarget) {
                if ((pTarget) && (!pPrevTarget && pCombatGroup->GetDetectionValue(apTarget) > 0)) {
                    Script::SetActionFlag(pTarget, &pAttacker->m_Extra, SCRIPT_ACTION_ONSTARTCOMBAT);
                    pCombatDialogueManager->StartDialogue(this, DT_DETECTION, 4, true);
                }
            }
            else {
                if (!pCombatGroup->iSearchCount) {
                    pCombatDialogueManager->StartDialogue(this, DT_DETECTION, 6, false);
                }
            }
        }
        if (pTarget) {
            pCombatGroup->IncrementAttackerCount(pTarget);
            Actor* v3 = pAttacker;
            pCombatManager->pTargetMap->AddTargetAttacker(pTarget, v3);
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
    Actor* bestTarget;
    Actor* attacker;
    uint8_t v7 = 0;
    if (apTarget) {
        RemoveTarget(apTarget);
        if (pTarget == apTarget) {
            bestTarget = pCombatGroup->GetBestTarget(pAttacker);
            SetTarget(bestTarget);
            if (!pTarget) {
                ClearProcedures();
                ClearCombatPlan();
                ForcePlanUpdate();
            }
        }
    }
    else {
        v7 = 1;
    }
    Actor* pOwner = GetAttacker();
    if (pOwner->GetBlocked()) {
        attacker = GetAttacker();
        attacker->SetBlock(false);
    }
    return (bool)v7;
}

void CombatController::TakeHit(Actor* apAttacker, ActiveEffect* apEffect) {
    Actor* attacker;
    BaseProcess* currentAIProcess;
    int32_t a3a;
    int32_t a4[2];
    int32_t v8;
    if (!bFirstUpdate) {
        FirstUpdate();
        bFirstUpdate = true;
    }
    if (pCombatGroup) {
        pCombatGroup->NotifyMemberHit(pAttacker, apAttacker);
    }
    if (((apAttacker != GetTarget()) && (apAttacker != GetAttacker())) &&
        (apAttacker->MobileObject::GetCurrentAIProcess())) {
        v8 = 0;
        if (!IsActoraCombatTarget(apAttacker)) {
            a4[1] = 0;
            a4[0] = 0;
            attacker = GetAttacker();
            if (attacker->GetShouldAttackActor(apAttacker, true, *reinterpret_cast<FIGHT_REACTION*>(a4), false)) {
                v8 = 100;
            }
            if (v8 > 0) {
                currentAIProcess = apAttacker->MobileObject::GetCurrentAIProcess();
                a3a = (int32_t)currentAIProcess->GetPotentialThreatDamage(apAttacker, pAttacker);
                AddTarget(apAttacker, a3a, false, 0.0, 0.0);
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
        fLowMovementDelta = (float)0.0;
    }
}

void CombatController::Update() {
    if (CombatController::bProcessCombat && !bStoppingCombat) {
        Actor* pattacker = pAttacker;
        if ((pattacker) && (pattacker->pCurrentProcess)) {
            if (!ShouldStopCombat()) {
                Animation* panimation = pattacker->GetAnimation();
                if (!pattacker->Get3D()) {
                    return;
                }
                if (!panimation) {
                    return;
                }
                BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
                int32_t eattackerFadeState = CurrentAIProcess->GetFadeState();
                if ((eattackerFadeState) && (eattackerFadeState != 2 && eattackerFadeState != 1)) {
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
                    Actor* ptarget__pattacker = GetTarget();
                    if (ptarget__pattacker &&
                        ((ptarget__pattacker->IsDead(false) && !ptarget__pattacker->IsDying()) ||
                         (ptarget__pattacker->GetDisabled() || ptarget__pattacker->GetDelete()))) {
                        pattacker->StopCombat(ptarget__pattacker);
                    }
                    else {
                        int16_t sanimAction = pattacker->pCurrentProcess->GetAnimAction();
                        bool bTemporarilyImmobile =
                            sanimAction == ANIM_ACTION_RECOIL || sanimAction == ANIM_ACTION_STAGGER;
                        if (!bTemporarilyImmobile && !pattacker->IsKnockedOut() && !pattacker->IsParalyzed()) {
                            pCombatState->Update();
                            if (!bStopCombat && !pCombatState->bInitializing && !panimation->GetAnimsLoading() &&
                                !pattacker->IsKnockedOut() && !pattacker->IsSurfacing()) {
                                bool bneedNewPlan = false;
                                if (GetProcedureFailed()) {
                                    bneedNewPlan = true;
                                    if ((pActionProcedure) && (pActionProcedure->GetFailed())) {
                                        AddFailedCombatProcedure(pActionProcedure);
                                    }

                                    if ((pMovementProcedure) && (pMovementProcedure->GetFailed())) {
                                        AddFailedCombatProcedure(pMovementProcedure);
                                    }
                                }
                                if (PlanTimer.Check()) {
                                    bneedNewPlan = true;
                                }
                                if ((!bneedNewPlan && GetProcedureFinished()) && (pCombatPlan)) {
                                    if (pCombatPlan->IsFinished()) {
                                        pCombatGroup->NotifyMemberPlanFinished(this, pCombatPlan->pGoal);
                                        bneedNewPlan = true;
                                    }
                                    else {
                                        pCombatPlan->ExecuteNextAction(this);
                                    }
                                }

                                if (bneedNewPlan) {
                                    CombatGoal* CastingType;
                                    ClearCombatPlan();
                                    ClearBackgroundProcedures();
                                    Actor* BestTarget = pCombatGroup->GetBestTarget(pAttacker);
                                    SetTarget(BestTarget);
                                    if (pMovementProcedure) {
                                        pMovementProcedure->StartedPlanning();
                                    }
                                    if (pActionProcedure) {
                                        pActionProcedure->StartedPlanning();
                                    }
                                    CombatAI_Planning.Start();
                                    CombatPlanner planner;
                                    pCombatPlan = planner.CreatePlan_0(this, false);
                                    if (pCombatPlan) {
                                        CastingType = pCombatPlan->pGoal;
                                    }
                                    else {
                                        CastingType = nullptr;
                                    }
                                    pCurrentGoal = CastingType;
                                    InitializeCombatPlanData();
                                    planner.CleanUp();
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
                                        CombatProcedure* pbackgroundProcedure = BackgroundProcedures.operator[](i);
                                        pbackgroundProcedure->Update();
                                        if (pbackgroundProcedure->GetFinished() || pbackgroundProcedure->GetFailed()) {
                                            if (pbackgroundProcedure->GetFailed()) {
                                                AddFailedCombatProcedure(pbackgroundProcedure);
                                            }
                                            pbackgroundProcedure->Shutdown();
                                            if (pbackgroundProcedure) {
                                                delete pbackgroundProcedure;
                                            }
                                            BackgroundProcedures.RemoveFast(i--, 1u);
                                        }
                                    }
                                    UpdateLookAtTarget();
                                    bool banimBlendCheck = false;
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
    int32_t icombatLocation;
    if (pCombatPlan) {
        icombatLocation = pCombatPlan->iCombatLocation;
    }
    else {
        icombatLocation = -1;
    }
    bool v8 = false;
    bool v7 = false;
    if (icombatLocation != -1) {
        CombatLocation* pcombatLocation = pCombatPlannerData->GetLocation(icombatLocation);
        if (pcombatLocation) {
            v8 = true;
            pCombatState->SetCombatLocation(pcombatLocation);
            CombatCoverLocation* pcombatCoverLocation = pCombatPlannerData->GetCombatCoverLocation(icombatLocation);
            if (pcombatCoverLocation) {
                v7 = true;
                pCombatState->SetCoverLocation(pcombatCoverLocation);
            }
            pCombatPlannerData->ClearLocation(icombatLocation);
        }
        else {
            CombatLocation* pcombatLocation_2 = pCombatState->pCombatLocation;
            if (pcombatLocation_2) {
                v8 = pcombatLocation_2->iLocation == icombatLocation;
            }
            CombatCoverLocation* pcombatCoverLocation_2 = pCombatState->GetCoverLocation();
            if ((pcombatCoverLocation_2) && (pcombatCoverLocation_2->iLocation == icombatLocation)) {
                v7 = true;
            }
        }
    }
    if (!v8) {
        pCombatState->ClearCombatLocation();
    }
    if (v7) {
        pCombatState->ReserveCoverLocation();
    }
    else {
        pCombatState->ClearCoverLocation(false);
    }
}

void CombatController::UpdateLookAtTarget() {
    NiPoint3 apResult;
    Actor* Attacker = GetAttacker();
    int32_t v7 = -1;
    uint32_t v8 = 1;
    for (uint32_t i = 0; i < 2; ++i) {
        if (ucLookAtTargetPriorities[i] >= v8) {
            v8 = ucLookAtTargetPriorities[i];
            v7 = i;
        }
    }
    if (v7 == -1 || v8 < static_cast<uint32_t>(ucIgnoreLookAtTarget) + 1) {
        if (fCombatPathingLookAheadDelta.Float() > 0.0 && (Attacker->IsPathing() && Attacker->IsPathValid())) {
            NiPoint3 v4;
            float* v1 = &fCombatPathingLookAheadDelta.Float();
            v4 = Attacker->CalculateAnticipatedLocation(*v1);
            CombatUtilities::GetAttackerFiringLocationSimple(&apResult, Attacker);
            const NiPoint3* v2 = &Attacker->GetLocationOnReference();
            v4.z = apResult.z - v2->z + v4.z;
            Attacker->SetLookAtTarget(v4);
        }
        else {
            Attacker->ClearLookAtTarget();
        }
    }
    else {
        Attacker->SetLookAtTarget(kLookAtTargets[v7]);
    }
}

void CombatController::UpdateLow() {
    BGSWorldLocation attackerWorldLoc_2;
    CombatLocation targetLocation_2;
    CombatLocation targetLocation;
    CombatLocation fleeLocation;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation targetWorldLoc;
    if (!bFirstUpdate) {
        FirstUpdate();
        bFirstUpdate = true;
    }
    if (fLowMovementDelta < 0.0) {
        PlanTimer.Reset(1.0f);
        fLowMovementDelta = 0.0f;
        if ((pTarget) && (bEncounterZoneTargetRestrict.Bool())) {
            BGSEncounterZone* LocationEncounterZone = pTarget->GetLocationEncounterZone();
            if (LocationEncounterZone != pAttacker->GetLocationEncounterZone()) {
                SetTarget(nullptr);
            }
        }
    }
    if (PlanTimer.Check()) {
        bool bproceduresFinished = false;
        if (GetProcedureFailed()) {
            if ((pActionProcedure) && (pActionProcedure->GetFailed())) {
                AddFailedCombatProcedure(pActionProcedure);
            }

            if ((pMovementProcedure) && (pMovementProcedure->GetFailed())) {
                AddFailedCombatProcedure(pMovementProcedure);
            }

            SetTarget(nullptr);
        }
        else {
            if (GetProcedureFinished()) {
                ClearProcedures();
                eLowState = COMBAT_LOW_STATE_NONE;
                bproceduresFinished = true;
            }
        }
        if (!pTarget) {
            Actor* BestTarget = pCombatGroup->GetBestTarget(pAttacker);
            SetTarget(BestTarget);
            ClearProcedures();
            eLowState = COMBAT_LOW_STATE_NONE;
        }
        if (pTarget) {
            pCombatState->Update();
            COMBAT_LOW_STATE enewState = COMBAT_LOW_STATE_NONE;
            pAttacker->GetWorldLocation(&attackerWorldLoc);
            pTarget->GetWorldLocation(&targetWorldLoc);
            if (pCombatState->CheckShouldFlee()) {
                enewState = COMBAT_LOW_STATE_FLEEING;
            }
            else {
                if (pCombatState->GetHasBufferedDetection()) {
                    double DistanceSquared = attackerWorldLoc.GetDistanceSquared(targetWorldLoc);
                    float* v4 = &fCombatLowMaxAttackDistance.Float();
                    double fmaxDistanceSquared = *v4 * fCombatLowMaxAttackDistance.Float();
                    if (!(fmaxDistanceSquared > DistanceSquared)) {
                        enewState = COMBAT_LOW_STATE_PURSUING;
                    }
                    else {
                        enewState = COMBAT_LOW_STATE_ATTACKING;
                    }
                }
                else {
                    if (pCombatState->IsDetectionLost()) {
                        if (pCombatGroup->IsSearching()) {
                            enewState = COMBAT_LOW_STATE_SEARCHING;
                        }
                    }
                    else {
                        enewState = COMBAT_LOW_STATE_INVESTIGATING;
                    }
                }
            }
            if (enewState != eLowState) {
                ClearProcedures();
                uint8_t coldFleeState = pCombatState->cFleeState;
                pCombatState->cFleeState = 0;
                switch (enewState) {
                    case COMBAT_LOW_STATE_ATTACKING: {
                        CombatProcedureAttackLow* v9;
                        CombatProcedureAttackLow* apMemory =
                            static_cast<CombatProcedureAttackLow*>(ourMalloc(sizeof(CombatProcedureAttackLow)));
                        if (apMemory) {
                            v9 = ::new (apMemory) CombatProcedureAttackLow;
                        }
                        else {
                            v9 = nullptr;
                        }
                        SetActionProcedure(v9);
                        break;
                    }
                    case COMBAT_LOW_STATE_PURSUING: {
                        targetLocation.~CombatLocation();
                        ::new (&targetLocation) CombatLocation;
                        targetLocation.SetRef(pTarget);
                        if (pCombatState->CheckMovement(*&targetLocation.WorldLoc, true, true)) {
                            CombatProcedure* pmovementProcedure_4;
                            CombatProcedureMove* v17 =
                                static_cast<CombatProcedureMove*>(ourMalloc(sizeof(CombatProcedureMove)));
                            if (v17) {
                                pmovementProcedure_4 = (CombatProcedure*)::new (v17)
                                    CombatProcedureMove(&targetLocation, kCombatLocationTypeData[0].fRadius, 0);
                            }
                            else {
                                pmovementProcedure_4 = nullptr;
                            }
                            SetMovementProcedure(pmovementProcedure_4);
                        }
                        break;
                    }
                    case COMBAT_LOW_STATE_INVESTIGATING: {
                        enewState = COMBAT_LOW_STATE_NONE;
                        targetLocation_2.~CombatLocation();
                        ::new (&targetLocation_2) CombatLocation;
                        BGSWorldLocation lastDetectedLocation;
                        if (pCombatGroup->GetTargetLastDetectedLocation(pTarget, lastDetectedLocation) &&
                            pCombatState->CheckMovement(lastDetectedLocation, true, true)) {
                            pAttacker->GetWorldLocation(&attackerWorldLoc_2);
                            if (attackerWorldLoc_2.GetDistanceSquared(lastDetectedLocation) > 100.0) {
                                CombatProcedure* pmovementProcedure_5;
                                targetLocation_2.SetLocation(lastDetectedLocation);
                                CombatProcedureMove* v16 =
                                    static_cast<CombatProcedureMove*>(ourMalloc(sizeof(CombatProcedureMove)));
                                if (v16) {
                                    pmovementProcedure_5 = (CombatProcedure*)::new (v16)
                                        CombatProcedureMove(&targetLocation_2, kCombatLocationTypeData[0].fRadius, 15);
                                }
                                else {
                                    pmovementProcedure_5 = nullptr;
                                }
                                SetMovementProcedure(pmovementProcedure_5);
                                enewState = COMBAT_LOW_STATE_INVESTIGATING;
                            }
                        }
                        break;
                    }
                    case COMBAT_LOW_STATE_SEARCHING: {
                        CombatProcedureSearch* v6;
                        CombatProcedureSearch* v15 =
                            static_cast<CombatProcedureSearch*>(ourMalloc(sizeof(CombatProcedureSearch)));
                        if (v15) {
                            v6 = ::new (v15) CombatProcedureSearch;
                        }
                        else {
                            v6 = nullptr;
                        }
                        SetMovementProcedure(v6);
                        break;
                    }
                    case COMBAT_LOW_STATE_FLEEING: {
                        float* v5;
                        double v12;
                        if ((bproceduresFinished && coldFleeState) ||
                            (v12 = attackerWorldLoc.GetDistanceSquared(targetWorldLoc),
                             (v5 = &fCombatLowMaxAttackDistance.Float(),
                              !(*v5 * fCombatLowMaxAttackDistance.Float() > v12)))) {
                            pCombatState->cFleeState = 2;
                        }
                        else {
                            pCombatState->cFleeState = 1;
                            fleeLocation.~CombatLocation();
                            ::new (&fleeLocation) CombatLocation;
                            if (pCombatState->FindBestFleeLocation(nullptr, &fleeLocation, 12)) {
                                CombatProcedure* pmovementProcedure_2;
                                if (fleeLocation.GetFlag(1u)) {
                                    CombatProcedure* pmovementProcedure;
                                    CombatProcedureEvade* v20 =
                                        static_cast<CombatProcedureEvade*>(ourMalloc(sizeof(CombatProcedureEvade)));
                                    if (v20) {
                                        pmovementProcedure = (CombatProcedure*)::new (v20) CombatProcedureEvade(12);
                                    }
                                    else {
                                        pmovementProcedure = nullptr;
                                    }
                                    pmovementProcedure_2 = pmovementProcedure;
                                }
                                else {
                                    CombatProcedure* pmovementProcedure_3;
                                    CombatProcedureMove* v19 =
                                        static_cast<CombatProcedureMove*>(ourMalloc(sizeof(CombatProcedureMove)));
                                    if (v19) {
                                        pmovementProcedure_3 = (CombatProcedure*)::new (v19)
                                            CombatProcedureMove(&fleeLocation, kCombatLocationTypeData[12].fRadius, 12);
                                    }
                                    else {
                                        pmovementProcedure_3 = nullptr;
                                    }
                                    pmovementProcedure_2 = pmovementProcedure_3;
                                }
                                SetMovementProcedure(pmovementProcedure_2);
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
                eLowState = enewState;
            }
            if (pMovementProcedure) {
                pMovementProcedure->Update();
            }
            if (pActionProcedure) {
                pActionProcedure->Update();
            }
            if (pAttacker->IsPathing()) {
                fLowMovementDelta = PlanTimer.ElapsedTime();
                pAttacker->SetAngleOnReferenceX(3.4028235e+38f);
            }
            PlanTimer.Reset(1.0f);
        }
    }
}

int32_t CombatController::GetAttackSkill() {
    int32_t iresult = 0;
    Actor* Attacker = pAttacker;
    if (Attacker->IsCreature()) {
        Creature* pcreature = dynamic_cast<Creature*>(pAttacker);
        if (pcreature) {
            return static_cast<int32_t>(pcreature->GetWeaponSkill());
        }
    }
    else {
        if (GetCurrentWeapon()) {
            ActorValue::Index WeaponSkill = GetCurrentWeapon()->GetWeaponSkill();
            return pAttacker->GetClampedActorValue(WeaponSkill);
        }

        return pAttacker->GetClampedActorValue(ActorValue::UNARMED);
    }
    return iresult;
}

void CombatController::AddBackgroundProcedure(CombatProcedure* apBackgroundProcedure) {
    for (uint32_t i = 0; i < BackgroundProcedures.iSize; ++i) {
        CombatProcedure* pbackgroundProcedure = BackgroundProcedures.operator[](i);
        CombatProcedure::COMBAT_PROCEDURES v2 = pbackgroundProcedure->GetType();
        if (v2 == apBackgroundProcedure->GetType()) {
            pbackgroundProcedure->Shutdown();
            if (pbackgroundProcedure) {
                delete pbackgroundProcedure;
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
        CombatLocationTarget* plocationTarget = pCombatState->pLocationTarget;
        if ((plocationTarget) && (plocationTarget->cType == 1 || !plocationTarget->cType)) {
            pCombatState->ClearLocationTarget();
        }

        pCombatState->RemoveTargetedObject(2);
    }
    SetPretendInCombat(false);
    SetContinueDuringCombat(false);
    pCombatStyle = nullptr;
    if (apCurrentPackage) {
        pCombatStyle = apCurrentPackage->pCombatStyle;
        uint32_t ipackageFlags = apCurrentPackage->PackData.iPackFlags;
        if ((ipackageFlags & (kPackageFlag_PretendInCombat | kPackageFlag_ContinueDuringCombat)) != 0) {
            SetPretendInCombat((ipackageFlags & kPackageFlag_PretendInCombat) != 0);
            SetContinueDuringCombat((ipackageFlags & kPackageFlag_ContinueDuringCombat) != 0);
            BaseProcess* pproc = pAttacker->pCurrentProcess;
            bool bhasRefForLocation = true;
            if (apCurrentPackage->pPackLoc) {
                PACK_LOC_TYPE LocType = apCurrentPackage->pPackLoc->GetLocType();
                if (LocType > PLT_REFERENCE) {
                    bhasRefForLocation = LocType > PLT_EDITORLOC;
                }
            }
            TESObjectREFR* pref = pproc->GetCurrentDestinationReference(pAttacker, false);
            // The destination radius and the package target's value share one float: the default case passes the radius
            // unless the package target replaced it.
            float fradius = pproc->GetCurrentDestinationRadius(pAttacker);
            if (!pref || pref == pAttacker) {
                pref = pproc->GetTarget();
                if ((!pref) && (apCurrentPackage->GetPackageTarget()) &&
                    (apCurrentPackage->GetPackageTarget()->GetTargReference())) {
                    pref = apCurrentPackage->GetPackageTarget()->GetTargReference();
                    fradius = static_cast<float>(apCurrentPackage->GetPackageTarget()->iValue);
                }
            }
            switch (apCurrentPackage->GetPackType()) {
                case PACKAGE_TYPE_EXPLORE:
                case PACKAGE_TYPE_FIND: {
                    TESBoundObject* apObject;
                    if (pref) {
                        apObject = pref->GetObjectReference();
                    }
                    else {
                        apObject = nullptr;
                    }
                    pCombatState->AddObjectToTarget(2u, pref, apObject);
                    break;
                }
                case PACKAGE_TYPE_FOLLOW:
                case PACKAGE_TYPE_ACCOMPANY: {
                    float fbase;
                    float fmin;
                    float fmult;
                    bool bteammate = pAttacker->IsPlayerTeammate();
                    if (bteammate) {
                        fmult = fCombatTeammateFollowRadiusMult.Float();
                    }
                    else {
                        fmult = fCombatFollowRadiusMult.Float();
                    }
                    float v35 = fmult;
                    if (bteammate) {
                        fmin = fCombatTeammateFollowRadiusMin.Float();
                    }
                    else {
                        fmin = fCombatFollowRadiusMin.Float();
                    }
                    if (bteammate) {
                        fbase = fCombatTeammateFollowRadiusBase.Float();
                    }
                    else {
                        fbase = fCombatFollowRadiusBase.Float();
                    }
                    float fguardDistanceMin_4 = static_cast<float>(apCurrentPackage->GetPackageTarget()->iValue);
                    float fguardDistanceMax_3 = fguardDistanceMin_4 * v35 + fbase;
                    float fguardDistanceMax_4 = BSMAX(fguardDistanceMax_3, fmin);
                    if (bhasRefForLocation) {
                        pCombatState->SetLocationTargetRef(
                            pref, LOCATION_TARGET_GUARD, fguardDistanceMin_4, fguardDistanceMax_4
                        );
                    }
                    else {
                        TESObjectCELL* pcell_2 = pproc->GetCurrentDestinationCell(pAttacker);
                        TESForm* pspace_4 = nullptr;
                        if (pcell_2) {
                            TESForm* pspace_5;
                            if (pcell_2->GetInterior()) {
                                pspace_5 = pcell_2;
                            }
                            else {
                                pspace_5 = pcell_2->GetWorldSpace();
                            }
                            pspace_4 = pspace_5;
                        }
                        else {
                            pspace_4 = pproc->GetCurrentDestinationWorldSpace(pAttacker);
                        }
                        BGSWorldLocation worldLoc_2(pproc->GetCurrentDestinationCoordinate(pAttacker, true), pspace_4);
                        pCombatState->SetLocationTargetLocation(
                            &worldLoc_2, 1, fguardDistanceMin_4, fguardDistanceMax_4
                        );
                    }
                    break;
                }
                case PACKAGE_TYPE_GUARD: {
                    float fguardDistanceMin_2;
                    if (IsPackageFlag4Set(apCurrentPackage, nullptr)) {
                        bhasRefForLocation = true;
                    }
                    PackageTarget* ppackTarg = apCurrentPackage->GetPackageTarget();
                    if (apCurrentPackage->SandboxGetAllowConversation() && apCurrentPackage->pPackLoc) {
                        fguardDistanceMin_2 = static_cast<float>(apCurrentPackage->GetLocationRadius(pAttacker));
                    }
                    else {
                        float fguardDistanceMin;
                        if (ppackTarg) {
                            fguardDistanceMin = static_cast<float>(ppackTarg->iValue);
                        }
                        else {
                            fguardDistanceMin = 30.0f;
                        }
                        fguardDistanceMin_2 = fguardDistanceMin;
                    }
                    float fguardDistanceMax = fguardDistanceMin_2 * fCombatGuardRadiusMult.Float();
                    float* v2 = &fCombatGuardRadiusMin.Float();
                    float fguardDistanceMax_2 = BSMAX(fguardDistanceMax, *v2);
                    if (bhasRefForLocation) {
                        pCombatState->SetLocationTargetRef(
                            pref, LOCATION_TARGET_GUARD, fguardDistanceMin_2, fguardDistanceMax_2
                        );
                    }
                    else {
                        TESObjectCELL* pcell = pproc->GetCurrentDestinationCell(pAttacker);
                        TESForm* pspace = nullptr;
                        if (pcell) {
                            TESForm* pspace_2;
                            if (pcell->GetInterior()) {
                                pspace_2 = pcell;
                            }
                            else {
                                pspace_2 = pcell->GetWorldSpace();
                            }
                            pspace = pspace_2;
                        }
                        else {
                            pspace = pproc->GetCurrentDestinationWorldSpace(pAttacker);
                        }
                        BGSWorldLocation worldLoc(pproc->GetCurrentDestinationCoordinate(pAttacker, true), pspace);
                        pCombatState->SetLocationTargetLocation(&worldLoc, 1, fguardDistanceMin_2, fguardDistanceMax_2);
                    }
                    break;
                }
                default: {
                    if (bhasRefForLocation) {
                        pCombatState->SetLocationTargetRef(pref, LOCATION_TARGET_DESTINATION, fradius, fradius);
                    }
                    else {
                        TESObjectCELL* pcell_3 = pproc->GetCurrentDestinationCell(pAttacker);
                        TESForm* pspace_7 = nullptr;
                        if (pcell_3) {
                            TESForm* pspace_8;
                            if (pcell_3->GetInterior()) {
                                pspace_8 = pcell_3;
                            }
                            else {
                                pspace_8 = pcell_3->GetWorldSpace();
                            }
                            pspace_7 = pspace_8;
                        }
                        else {
                            pspace_7 = pproc->GetCurrentDestinationWorldSpace(pAttacker);
                        }
                        BGSWorldLocation worldLoc_3(pproc->GetCurrentDestinationCoordinate(pAttacker, true), pspace_7);
                        pCombatState->SetLocationTargetLocation(&worldLoc_3, 0, fradius, fradius);
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
    return (bool)((uint8_t)CombatController::bShowVerboseMessages);
}

void CombatController::SetShowVerboseMessages(bool abShowVerboseMessages) {
    CombatController::bShowVerboseMessages = (bool)((int8_t)abShowVerboseMessages);
}

bool CombatController::CheckHasEquip(Actor* apAttacker, ItemChange* apWeapon) {
    bool v8 = false;
    uint16_t usGroup = apAttacker->GetAnimGroup(ANIM_GROUP_WEAPONS_START, apWeapon, apWeapon == nullptr, nullptr);
    if (usGroup) {
        Animation* v6 = apAttacker->GetAnimation();
        if (v6) {
            AnimSequenceBase* val;
            NiTPointerMap<uint16_t, AnimSequenceBase*>* MapAnimSequenceBase = v6->GetMapAnimSequenceBase();
            if (MapAnimSequenceBase->GetAt(usGroup, val)) {
                return true;
            }
        }
    }
    return v8;
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
