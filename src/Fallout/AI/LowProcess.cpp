#include "Fallout/AI/LowProcess.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkPickData.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/GuardPackage.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ModifierList.h"
#include "Fallout/AI/PatrolPackage.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SandBoxPackage.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>
#include <limits>

// Each low process's lock owner thread and count (37 buckets).
using LowProcessLockTable = NiTMap<LowProcess*, LP_LOCK_DATA>;
static LowProcessLockTable LowProcessLockMap(37);
static BSCriticalSection LowProcCritSection;

static BSPerformanceTimer LowUpdateTimer("AIProcessing", "LowUpdate", true, true);

uint32_t* LP_LOCK_DATA::Clear(int32_t aiClearValue) {
    iThreadID = 0;
    iLockCount = 0;
    return &iThreadID;
}

LowProcess::LowProcess()
  : pItemBeingUsed(nullptr), pTarget(nullptr), pGenericLocation(nullptr), pGenericSecondLocation(nullptr),
    pTargetFormList(nullptr), pPatrolLocation(nullptr), iNumberItemsActivate(0), pAcquireObject(nullptr),
    pSavedAcquireObject(nullptr), pObjecttoAcquire(nullptr), pObjectForLocation(nullptr), eFormType(POTYPE_NONE),
    pFactiontoAquire(nullptr), fDeathTime(0.0f), fTrackedDamage(0.0f), bSkippedTimeStampForPathing(false) {
    fHourLastProcessed = -1.0f;
    BaseProcess::ClearCurrentPackage();
    fSystemTimeClock = 0.0f;
    iDateLastProcessed = 0;
    SetField(0, 255, 0);
    eLevel = PROCESS_TYPE_LOW;
}

LowProcess::~LowProcess() {
    BaseProcess::ClearCurrentPackage();
    ClearData();
}

// A low process catches up on missed packages after an hour's gap (3600.0), then lets the actor process with no time
// step.
void LowProcess::Update(MobileObject* apRef, float fTimeDelta) {
    if (!apRef) {
        return;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRef);
    if (!pActor) {
        return;
    }
    if (fTimeDelta > 3600.0) {
        UpdateMissedPackages(apRef, true);
    }
    pActor->Process(0.0f);
}

// Runs one low-level step of apRef's package: switches to a new package when one applies, then runs the step of the
// current procedure, and once the procedure is done ends the package (its done flag, script event, run-once day,
// acquire lists) and picks the next one from the schedule. A non-actor only gets its time stamp. The closing perf-timer
// mark is skipped when apRef is null, when the pathing goal can't be set and when a finished travel package sends the
// actor back to its location.
void LowProcess::Update(MobileObject* apRef) {
    LowUpdateTimer.Start();
    if (!apRef) {
        return;
    }
    Actor* pActor = nullptr;
    if (apRef->IsActor()) {
        pActor = static_cast<Actor*>(apRef);
    }
    if (!pActor) {
        BaseProcess* pCurrProc = apRef->GetCurrentAIProcess();
        pCurrProc->ComputeLastTimeProcessed();
        LowUpdateTimer.Stop();
        return;
    }
    if (pActor->IsInCombat(true) && pPlayer->IsSleepingorResting()) {
        pActor->StopCombat(nullptr);
    }
    TESPackage* pCurrPackage = GetCurrentPackage();
    if (CheckforNewPackage(pActor, false)) {
        ClearCurrentDataforProcess(pActor);
        pCurrPackage = GetCurrentPackage();
        pActor->AlertAccompanyToPackageChange();
        SetLocationForPackage(pActor, false);
        TESPackage* pNewPackage = GetCurrentPackage();
        if (pNewPackage) {
            PackageTarget* pPackageTarget = pNewPackage->GetPackageTarget();
            if (pPackageTarget && pPackageTarget->GetTargType() && pPackageTarget->GetTargType() != PTT_LINKEDREF) {
                iNumberItemsActivate = pNewPackage->GetInitialTargetCount();
            }
        }
    }
    bool bPathingFailed = false;
    if (bSkippedTimeStampForPathing && pActor->DidPathingFail()) {
        bPathingFailed = true;
    }
    if (GetCurrentPackage() &&
        GetCurrentPackage()->GetProcedureType() != PACKAGE_PROCEDURE_NONE &&
        !GetCurrentPackage()->GetWaiting()) {
        UpdateTarget();
        TESObjectREFR* pGenericLoc = nullptr;
        PACKAGE_PROCEDURE_TYPE eType = GetCurrentPackage()->GetProcedureType();
        uint32_t uiIndex = GetProcedureIndexRunning();
        switch (Procedure::Action[eType][uiIndex]) {
            case PROCEDURE_TRAVEL: {
                pGenericLoc = pGenericLocation;
                if (GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false)) {
                    if (!GetLockedLocation()) {
                        // Reads the second point, then the first, then the cell. The actor pointer is passed as the
                        // filters' untyped int32 data, hence the pointer-to-integer cast.
                        if (GetCurrentPackage()->GetLockDoorsAtLocation()) {
                            const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                            const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                            TESObjectCELL* pParentCell = pActor->GetParentCell();
                            pDataHandler->EnumReferencesCloseToPoint(
                                pParentCell,
                                *pPoint1,
                                5000.0f,
                                *pPoint2,
                                5000.0f,
                                &LowProcess::LockDoorsatLocation,
                                pActor
                            );
                        }
                        SetLockedLocation(true);
                    }
                    if (GetCurrentPackage()->GetUnlockDoorsAtLocation()) {
                        const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                        const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                        TESObjectCELL* pParentCell = pActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pParentCell,
                            *pPoint1,
                            5000.0f,
                            *pPoint2,
                            5000.0f,
                            &LowProcess::UnLockDoorsatLocation,
                            pActor
                        );
                    }
                    if ((pGenericLoc && pGenericLoc->GetObjectReference() == pXMarkerHeading) ||
                        (GetCurrentPackage()->GetLocation() &&
                         GetCurrentPackage()->GetLocation()->GetLocType() == PLT_EDITORLOC)) {
                        float fZ = 0.0f;
                        if (pGenericLoc) {
                            fZ = pGenericLoc->GetAngleOnReference().z;
                        }
                        else {
                            fZ = pActor->GetStartingAngle().z;
                        }
                        pActor->SetHeading(fZ);
                    }
                    AddToProcedureIndexRunning(pActor, 1);
                    break;
                }
                if (pActor->IsPathingComplete()) {
                    TESWorldSpace* pWorld = GetCurrentPackage()->GetLocationWorld(pActor);
                    TESObjectCELL* pCell = GetCurrentPackage()->GetLocationCell(pActor);
                    NiPoint3 kCoord = GetCurrentPackage()->GetLocationCoord(pActor);
                    if (!pActor->SetPathfindingGoal(kCoord, pCell, pWorld, -1.0f, nullptr)) {
                        return;
                    }
                }
                if (pActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_LOW &&
                    GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false)) {
                    if (!GetLockedLocation()) {
                        if (GetCurrentPackage()->GetLockDoorsAtLocation()) {
                            const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                            const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                            TESObjectCELL* pParentCell = pActor->GetParentCell();
                            pDataHandler->EnumReferencesCloseToPoint(
                                pParentCell,
                                *pPoint1,
                                5000.0f,
                                *pPoint2,
                                5000.0f,
                                &LowProcess::LockDoorsatLocation,
                                pActor
                            );
                        }
                        SetLockedLocation(true);
                    }
                    if (GetCurrentPackage()->GetUnlockDoorsAtLocation()) {
                        const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                        const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                        TESObjectCELL* pParentCell = pActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pParentCell,
                            *pPoint1,
                            5000.0f,
                            *pPoint2,
                            5000.0f,
                            &LowProcess::UnLockDoorsatLocation,
                            pActor
                        );
                    }
                    // Here the procedure is advanced before the heading is set.
                    AddToProcedureIndexRunning(pActor, 1);
                    if ((pGenericLoc && pGenericLoc->GetObjectReference() == pXMarkerHeading) ||
                        (GetCurrentPackage()->GetLocation() &&
                         GetCurrentPackage()->GetLocation()->GetLocType() == PLT_EDITORLOC)) {
                        float fZ = 0.0f;
                        if (pGenericLoc) {
                            fZ = pGenericLoc->GetAngleOnReference().z;
                        }
                        else {
                            fZ = pActor->GetStartingAngle().z;
                        }
                        pActor->SetHeading(fZ);
                    }
                }
                break;
            }
            case PROCEDURE_WANDER:
                if (GetCurrentPackage()->GetProcedureType() == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                    AddToProcedureIndexRunning(pActor, static_cast<uint32_t>(-1));
                }
                break;
            case PROCEDURE_ACTIVATE: ProcessActivate(pActor, true); break;
            case PROCEDURE_ACQUIRE: ProcessAcquire(pActor); break;
            case PROCEDURE_SLEEP:
                if (Interface::GetConsoleSelection() == pActor) {
                    char acText[0x130];
                    PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                    const char* pPackName = NavMeshInfo::GetCellOrCoords(
                        const_cast<char*>(packageTypeToString[ePackType]),
                        0
                    );
                    const char* pFullName = pActor->GetFullName();
                    std::sprintf(acText, "%s is sleeping with %s", pFullName, pPackName);
                    Interface::PrintLine<>(acText);
                }
                break;
            case PROCEDURE_EAT:
                if (Interface::GetConsoleSelection() == pActor) {
                    char acText[0x130];
                    PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                    const char* pPackName = NavMeshInfo::GetCellOrCoords(
                        const_cast<char*>(packageTypeToString[ePackType]),
                        0
                    );
                    const char* pFullName = pActor->GetFullName();
                    std::sprintf(acText, "%s is eating with %s", pFullName, pPackName);
                    Interface::PrintLine<>(acText);
                }
                break;
            case PROCEDURE_FOLLOW:
                // 0x101 is the move mode.
                ProcessFollow(pActor, true, 0x101, false);
                break;
            case PROCEDURE_ESCORT:
                switch (GetCurrentPackage()->GetPackType()) {
                    case PACKAGE_TYPE_FOLLOW:
                    case PACKAGE_TYPE_ESCORT:
                    case PACKAGE_TYPE_ACCOMPANY:
                    case PACKAGE_TYPE_FLEE: ProcessEscort(pActor); break;
                    default: break;
                }
                break;
            case PROCEDURE_ALARM: ProcessAlarm(pActor); break;
            case PROCEDURE_FLEE: ProcessFlee(pActor); break;
            case PROCEDURE_DIALOGUE: pActor->EndInterruptPackage(false); break;
            case PROCEDURE_WAIT:
                if (!pTarget) {
                    SetTargetForPackage(pActor);
                }
                if (!pTarget || pTarget->GetDelete()) {
                    AddToProcedureIndexRunning(pActor, 1);
                    break;
                }
                // Tests the package read before the package change check, not the current one.
                if (pCurrPackage->GetWaiting()) {
                    if (!pPlayer->IsPlayerFollower(pActor)) {
                        int32_t iFollowers = pPlayer->GetNumberOfFollowers();
                        if (iFollowers < static_cast<int32_t>(iNumberActorsAllowedToFollowPlayer.UInt())) {
                            GetCurrentPackage()->SetWaiting(false);
                        }
                    }
                }
                else if (pCurrPackage->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE && !pCurrPackage->GetWaiting()) {
                    if (pCurrPackage->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                        float fRadius = static_cast<float>(pCurrPackage->GetPackageTarget()->iValue);
                        if (fRadius >= pActor->GetDistanceFromReference(pTarget, false, false)) {
                            AddToProcedureIndexRunning(pActor, 1);
                        }
                    }
                    else {
                        // The wait distance is worked out and never used.
                        float fWaitDistance = 0.0f;
                        int32_t iValue = pCurrPackage->GetPackageTarget()->iValue;
                        if (iValue <= 0) {
                            iValue = 200;
                        }
                        if (fWaitDistance == 0.0 && pActor->GetParentCell() && pActor->GetParentCell()->GetInterior()) {
                            fWaitDistance = fAIEscortWaitDistanceInterior.Float();
                        }
                        else if (fWaitDistance == 0.0) {
                            fWaitDistance = static_cast<float>(
                                fAIEscortWaitDistanceExterior.Float() * static_cast<double>(iValue)
                            );
                        }
                        AddToProcedureIndexRunning(pActor, 1);
                    }
                }
                break;
            case PROCEDURE_TRAVEL_TARGET: ProcessActivate(pActor, false); break;
            case PROCEDURE_PURSUE: ProcessPursue(pActor, false, true, 0x101); break;
            case PROCEDURE_CREATE_FOLLOW:
                if (!pTarget) {
                    SetTargetForPackage(pActor);
                }
                if (pTarget && pTarget != pPlayer && pTarget->IsActor()) {
                    CreateFollowforEscort(pActor, GetCurrentPackage(), true);
                }
                AddToProcedureIndexRunning(pActor, 1);
                break;
            case PROCEDURE_NOTIFY: ProcessNotify(pActor); break;
            case PROCEDURE_ACCOMPANY: ProcessAccompany(pActor); break;
            case PROCEDURE_USE_ITEM_AT: ProcessUseItemAt(pActor); break;
            case PROCEDURE_AMBUSH_WAIT: ProcessAmbushWait(pActor); break;
            case PROCEDURE_CHOOSE_CAST: ProcessChooseSpell(pActor); break;
            case PROCEDURE_FLEE_NON_COMBAT: ProcessFleeNonCombat(pActor); break;
            case PROCEDURE_REMOVE_WORN_ITEMS: ProcessRemoveWorn(pActor, true); break;
            case PROCEDURE_SUMMON_CREATURE_DEFEND: ProcessSummonCreatureDefend(pActor); break;
            case PROCEDURE_TAKE_BACK_ITEM: AddToProcedureIndexRunning(pActor, 2); break;
            case PROCEDURE_SANDBOX: ProcessSandBox(pActor); break;
            case PROCEDURE_PATROL: ProcessPatrol(pActor); break;
            case PROCEDURE_GUARD: ProcessGuard(pActor); break;
            case PROCEDURE_DIALOGUE_ACTIVATE: {
                Actor* pTargetActor = nullptr;
                if (pTarget && pTarget->IsActor()) {
                    pTargetActor = static_cast<Actor*>(pTarget);
                }
                if (pTargetActor &&
                    (!GetCurrentPackage()->GetPackageSecondLocation() ||
                     GetCurrentPackage()->IsActorAtSecondLocation(pTargetActor, pActor, false, -1.0f, true))) {
                    ProcessActivate(pActor, true);
                }
                break;
            }
            case PROCEDURE_USE_WEAPON: ProcessUseWeapon(pActor); break;
            case PROCEDURE_MOVEMENT_BLOCKED: ProcessMovementBlocked(pActor); break;
            default: break;
        }
        if (pActor->GetCurrentProcessType() == PROCESS_TYPE_LOW && GetCurrentPackage()) {
            PACKAGE_PROCEDURE_TYPE eDoneType = GetCurrentPackage()->GetProcedureType();
            uint32_t uiDoneIndex = GetProcedureIndexRunning();
            if (Procedure::Action[eDoneType][uiDoneIndex] == PROCEDURE_DONE) {
                if (!IsPackageDoneOnce()) {
                    ExtraDataList* pExtra = pActor->GetExtra();
                    // The package done action flag (0x400).
                    Script::SetActionFlag(GetCurrentPackage(), pExtra, SCRIPT_ACTION_ONPACKAGEEND);
                    PackageDone(pActor, GetCurrentPackage());
                    SetPackageDoneOnce(true);
                }
                SetIsAggressor(false);
                if (GetCurrentPackage() &&
                    GetCurrentPackage()->GetProcedureType() == PACKAGE_PROCEDURE_EXPLORE_TRAVEL &&
                    !GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false)) {
                    AddToProcedureIndexRunning(pActor, static_cast<uint32_t>(-1));
                    return;
                }
                if (GetCurrentPackage() && GetCurrentPackage()->GetOncePerDay()) {
                    char cDay = appCalendar.GetDay();
                    pActor->SetRunOnceExtraPackage(GetCurrentPackage(), cDay);
                }
                if (GetCurrentPackage() && GetCurrentPackage()->GetIsCreated()) {
                    pActor->EndInterruptPackage(false);
                }
                if (Interface::GetConsoleSelection() == pActor) {
                    char acText[0x130];
                    PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                    const char* pPackName = NavMeshInfo::GetCellOrCoords(
                        const_cast<char*>(packageTypeToString[ePackType]),
                        0
                    );
                    const char* pFullName = pActor->GetFullName();
                    std::sprintf(acText, "%s is done with %s", pFullName, pPackName);
                    Interface::PrintLine<>(acText);
                }
                ObjectstoAcquire* pAcquire = pAcquireObject;
                delete pAcquire;
                pAcquireObject = nullptr;
                while (!ObjectList.IsEmpty()) {
                    ObjectstoAcquire* pObject = ObjectList.GetItem();
                    delete pObject;
                    ObjectList.Remove(pObject);
                }
                if (GetCurrentPackage() && !GetCurrentPackage()->GetPackageSchedule()->GetDuration()) {
                    TESPackage* pOldPackage = GetCurrentPackage();
                    SetCurrentPackageFromSchedule(pActor);
                    pCurrPackage = GetCurrentPackage();
                    if (pOldPackage != GetCurrentPackage()) {
                        SetProcedureIndexRunning(0);
                    }
                }
            }
        }
    }
    if (pActor->IsWaitingOnPath() && !bPathingFailed) {
        bSkippedTimeStampForPathing = true;
    }
    else {
        BaseProcess* pCurrProc = apRef->GetCurrentAIProcess();
        pCurrProc->ComputeLastTimeProcessed();
        bSkippedTimeStampForPathing = false;
    }
    LowUpdateTimer.Stop();
}

// Catches the actor up on the packages it missed since the process last ran (unless the current one must complete, is a
// script package or must reach its location): each missed package whose conditions hold starts, moves the actor to its
// location or target and runs its door locking; at the end the actor moves to the last location found. When the loop
// stops on an empty node, the loop's node is deleted again after the list itself is gone.
void LowProcess::UpdateMissedPackages(MobileObject* apRef, bool abActionflag) {
    Actor* pActor = dynamic_cast<Actor*>(apRef);
    TESPackage* pCurrent = GetCurrentPackage();
    if ((pActor && !pCurrent) ||
        (!pCurrent->GetMustComplete() && !pCurrent->IsScriptPackage() && !pCurrent->GetMustReachLocation())) {
        BSSimpleList<TESPackage*>* pList = new BSSimpleList<TESPackage*>;
        // The date is widened as an unsigned value and rounded to float.
        pActor->GetMissedPacks(pList, static_cast<float>(GetLastDateProcessed()));
        BSSimpleList<TESPackage*>* pIter = pList;
        TESObjectCELL* pCell = nullptr;
        TESWorldSpace* pWorld = nullptr;
        bool bClearAngle = false;
        if (!pIter->IsEmpty()) {
            while (pIter && !pIter->IsEmpty()) {
                TESPackage* pPack = pIter->GetItem();
                TESObjectREFR* pTargetRef = nullptr;
                PackageTarget* pPackTarget = pPack->GetPackageTarget();
                if (pPackTarget) {
                    pTargetRef = pPackTarget->GetTargReference();
                }
                SetLockedLocation(false);
                if (pPack->GetConditions()->IsTrue(pActor, pTargetRef)) {
                    if (abActionflag) {
                        // The package start (0x200) and package done (0x400) action flags.
                        Script::SetActionFlag(pPack, pActor->GetExtra(), SCRIPT_ACTION_ONPACKAGESTART);
                        ExtraDataList* pExtra = pActor->GetExtra();
                        Script::SetActionFlag(GetCurrentPackage(), pExtra, SCRIPT_ACTION_ONPACKAGEEND);
                    }
                    SetCurrentPackage(pPack, pActor);
                    pActor->UnlockLockDoorsProcedure();
                    SetLocationForPackage(pActor, false);
                    switch (pPack->GetPackType()) {
                        case PACKAGE_TYPE_WANDER:
                        case PACKAGE_TYPE_TRAVEL: {
                            NiPoint3 kCoord = pPack->GetLocationCoord(pActor);
                            pActor->SetLocationOnReference(kCoord);
                            pCell = pPack->GetLocationCell(pActor);
                            pWorld = pPack->GetLocationWorld(pActor);
                            bClearAngle = pPack->GetLocation() && pPack->GetLocation()->GetLocType() == PLT_CELL;
                            break;
                        }
                        case PACKAGE_TYPE_EXPLORE:
                            if (!pPack->GetPackageTarget()) {
                                NiPoint3 kCoord = pPack->GetLocationCoord(pActor);
                                pActor->SetLocationOnReference(kCoord);
                                pCell = pPack->GetLocationCell(pActor);
                                pWorld = pPack->GetLocationWorld(pActor);
                                bClearAngle = false;
                            }
                            else {
                                TESObjectREFR* pRef = pPack->GetPackageTarget()->GetTargReference();
                                if (pRef && !pRef->GetDelete()) {
                                    pActor->SetLocationOnReference(pRef->GetLocationOnReference());
                                    bClearAngle = true;
                                    if (!pRef->IsActor() && !pRef->GetDisabled() && !pRef->GetDelete()) {
                                        pRef->Activate(pActor, false, nullptr, 1);
                                    }
                                    pCell = pRef->GetParentCell();
                                    pWorld = pRef->GetWorldSpace();
                                }
                                else {
                                    NiPoint3 kCoord = pPack->GetLocationCoord(pActor);
                                    pActor->SetLocationOnReference(kCoord);
                                    pCell = pPack->GetLocationCell(pActor);
                                    pWorld = pPack->GetLocationWorld(pActor);
                                    bClearAngle = false;
                                }
                            }
                            break;
                        case PACKAGE_TYPE_EAT:
                        case PACKAGE_TYPE_SLEEP: {
                            NiPoint3 kCoord = pPack->GetLocationCoord(pActor);
                            pActor->SetLocationOnReference(kCoord);
                            pCell = pPack->GetLocationCell(pActor);
                            pWorld = pPack->GetLocationWorld(pActor);
                            bClearAngle = pPack->GetLocation() && pPack->GetLocation()->GetLocReference();
                            break;
                        }
                        case PACKAGE_TYPE_FOLLOW:
                            if (pPack->GetPackageTarget() &&
                                (pPack->GetPackageTarget()->GetTargReference() ||
                                 pPack->GetPackageTarget()->GetTargObject() ||
                                 pPack->GetPackageTarget()->GetTargObjectType())) {
                                TESObjectREFR* pRef = pPack->GetPackageTarget()->GetTargReference();
                                if (pRef && !pRef->GetDelete() && pRef != pPlayer) {
                                    pActor->SetLocationOnReference(pRef->GetLocationOnReference());
                                    pCell = pRef->GetParentCell();
                                    pWorld = pRef->GetWorldSpace();
                                    bClearAngle = false;
                                }
                            }
                            break;
                        case PACKAGE_TYPE_GUARD: ProcessGuard(pActor); break;
                        default: break;
                    }
                    pActor->UnlockLockDoorsProcedure();
                    if (pPack->GetOncePerDay()) {
                        pActor->SetRunOnceExtraPackage(pPack, appCalendar.GetDay());
                    }
                    // Reads the second point, then the first, then the cell. Passes the actor as the filters' data.
                    if (pPack->GetLockDoorsAtEnd()) {
                        const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                        const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                        TESObjectCELL* pParentCell = pActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pParentCell,
                            *pPoint1,
                            5000.0f,
                            *pPoint2,
                            5000.0f,
                            &LowProcess::LockDoorsatLocation,
                            pActor
                        );
                    }
                    else if (pPack->GetUnlockDoorsAtEnd()) {
                        const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                        const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                        TESObjectCELL* pParentCell = pActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pParentCell,
                            *pPoint1,
                            5000.0f,
                            *pPoint2,
                            5000.0f,
                            &LowProcess::UnLockDoorsatLocation,
                            pActor
                        );
                    }
                }
                pIter = pIter->GetNext();
            }
            if (pList) {
                pList->RemoveAll();
            }
            if (pList) {
                delete pList;
            }
            if (pCell || pWorld) {
                if (bClearAngle) {
                    pActor->SetAngleOnReferenceX(std::numeric_limits<float>::max());
                }
                TESObjectREFR::MoveRefToNewSpace(pActor, pCell, pWorld);
                ClearCurrentPackage();
                ClearCurrentDataforProcess(pActor);
            }
        }
        if (pIter) {
            delete pIter;
        }
    }
    BaseProcess* pProcess = pActor->GetCurrentAIProcess();
    pProcess->SetLockedLocation(false);
}

NiPoint3 LowProcess::GetCurrentPackageLocation(Actor* apActor) {
    NiPoint3 kLocation;
    if (GetCurrentPackage()) {
        kLocation = GetCurrentPackage()->GetLocationCoord(apActor);
    }
    else {
        kLocation = NiPoint3::ZERO;
    }
    return kLocation;
}

bool LowProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
    float fHour = appCalendar.GetHour();
    TESPackage* ppack = GetCurrentPackage();
    if (!abFlag && (apActor->IsAlarmed() || apActor->IsInCombat())) {
        return false;
    }
    bool bflag = false;
    if (!ppack) {
        SetCurrentPackageFromSchedule(apActor);
        bflag = GetCurrentPackage() != nullptr;
        ppack = GetCurrentPackage();
        SetTarget(nullptr);
        SetTargetActivated(false);
    }
    else {
        if (ppack->GetPackageSchedule()->PSData.iDuration && ppack->GetMustComplete()) {
            bool bcompleteflag = false;
            bool bnonending = false;
            if (apActor->pCurrentProcess->GetCurrentActionComplete() ||
                Procedure::Action[ppack->ePROCEDURE_TYPE][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
                bcompleteflag = true;
            }
            else if (
                ppack->GetPackType() == PACKAGE_TYPE_WANDER ||
                ppack->GetPackType() == PACKAGE_TYPE_SLEEP ||
                ppack->GetPackType() == PACKAGE_TYPE_EAT ||
                ppack->GetPackType() == PACKAGE_TYPE_SANDBOX
            ) {
                bcompleteflag = true;
                bnonending = true;
            }
            if (bcompleteflag) {
                int32_t iPackageStartTime;
                if (ppack->GetPackageSchedule()->GetStartTime() == -1) {
                    iPackageStartTime = static_cast<int32_t>(CurrentPackage.GetPackageStartTime());
                }
                else {
                    iPackageStartTime = ppack->GetPackageSchedule()->GetStartTime();
                }
                int32_t iTimedone = ppack->GetPackageSchedule()->PSData.iDuration + iPackageStartTime;
                if (iTimedone <= 23) {
                    if (fHour < static_cast<double>(ppack->GetPackageSchedule()->GetStartTime())) {
                        bflag = true;
                    }
                }
                else {
                    iTimedone -= 23;
                }
                if (static_cast<int32_t>(fHour) >= iTimedone) {
                    bflag = true;
                }
                if (!bflag && abFlag && !apActor->GetEVPBuffered()) {
                    abFlag = false;
                }
                if (bnonending) {
                    Script::SetActionFlag(ppack, &apActor->m_Extra, SCRIPT_ACTION_ONPACKAGEEND);
                    if (ppack->GetOncePerDay()) {
                        char cDay = appCalendar.GetDay();
                        apActor->SetRunOnceExtraPackage(ppack, cDay);
                    }
                }
            }
        }
        else if (ppack->GetMustComplete()) {
            if (apActor->pCurrentProcess->GetCurrentActionComplete() ||
                (ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE &&
                 Procedure::Action[ppack->ePROCEDURE_TYPE][GetProcedureIndexRunning()] == PROCEDURE_DONE)) {
                if (ppack->GetOncePerDay()) {
                    char cDay = appCalendar.GetDay();
                    apActor->SetRunOnceExtraPackage(ppack, cDay);
                }
                bflag = true;
            }
            else {
                bflag = false;
                abFlag = false;
            }
        }
        else if (!ppack->GetMustReachLocation() || apActor->GetEVPBuffered()) {
            bflag = true;
        }
        else {
            if (ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE &&
                (ppack->IsActorAtLocation(apActor, false, -1.0f, false) ||
                 Procedure::Action[ppack->ePROCEDURE_TYPE][GetProcedureIndexRunning()] == PROCEDURE_DONE)) {
                if (Procedure::Action[ppack->ePROCEDURE_TYPE][GetProcedureIndexRunning()] == PROCEDURE_DONE &&
                    ppack->GetOncePerDay()) {
                    char cDay = appCalendar.GetDay();
                    apActor->SetRunOnceExtraPackage(ppack, cDay);
                }
                bflag = true;
            }
            else {
                abFlag = false;
            }
        }
        if (bflag || (abFlag && !ppack->IsScriptPackage())) {
            if (ppack->GetIsCreated()) {
                apActor->EndInterruptPackage(false);
                if (GetCurrentPackage() && ppack == GetCurrentPackage()) {
                    ClearCurrentDataforProcess(apActor);
                    ClearCurrentPackage();
                }

                ppack = nullptr;
            }
            if (!SetCurrentPackageFromSchedule(apActor)) {
                return false;
            }
            TESPackage* pcurrentPackage = GetCurrentPackage();
            if (!ppack || ppack == pcurrentPackage || (pcurrentPackage && pcurrentPackage->IsInterruptPackage())) {
                bflag = false;
            }
            else if (ppack->GetPCContinue()) {
                TESObjectCELL* pPlayerCell = pPlayer->GetParentCell();
                if (pPlayerCell == apActor->GetParentCell()) {
                    if (GetProcedureIndexRunning()) {
                        SetContinuingPackage(true);
                    }
                    SetCurrentPackage(ppack, apActor);
                    return false;
                }
            }
        }
    }
    if (bflag) {
        if (ppack && GetCurrentPackage() != ppack) {
            if (ppack->GetLockDoorsAtEnd()) {
                const NiPoint3& kPoint2 = apActor->GetLocationOnReference();
                const NiPoint3& kPoint1 = apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    kPoint1,
                    5000.0f,
                    kPoint2,
                    5000.0f,
                    &LowProcess::LockDoorsatLocation,
                    apActor
                );
            }
            else if (ppack->GetUnlockDoorsAtEnd()) {
                const NiPoint3& kPoint2 = apActor->GetLocationOnReference();
                const NiPoint3& kPoint1 = apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    kPoint1,
                    5000.0f,
                    kPoint2,
                    5000.0f,
                    &LowProcess::UnLockDoorsatLocation,
                    apActor
                );
            }
        }

        if (GetCurrentPackage() && GetCurrentPackage() != ppack) {
            if (GetCurrentPackage()->GetLockDoorsAtStart()) {
                const NiPoint3& kPoint2 = apActor->GetLocationOnReference();
                const NiPoint3& kPoint1 = apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    kPoint1,
                    5000.0f,
                    kPoint2,
                    5000.0f,
                    &LowProcess::LockDoorsatLocation,
                    apActor
                );
            }
            else if (GetCurrentPackage()->GetUnlockDoorsAtStart()) {
                const NiPoint3& kPoint2 = apActor->GetLocationOnReference();
                const NiPoint3& kPoint1 = apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    kPoint1,
                    5000.0f,
                    kPoint2,
                    5000.0f,
                    &LowProcess::UnLockDoorsatLocation,
                    apActor
                );
            }
        }

        if (GetCurrentPackage() && (ppack != GetCurrentPackage() || !apActor->GetCurrentTarget())) {
            if (GetCurrentPackage()->IsTargetAnActor(apActor)) {
                if (GetCurrentPackage()->GetPackType()) {
                    SetTarget(GetCurrentPackage()->GetEscortTargetForPackage(apActor, false));
                }
            }
        }

        apActor->UnlockLockDoorsProcedure();
        SetLockedLocation(false);
        float fAngleZ = apActor->GetAngleOnReference().z;
        const NiPoint3& kLocation = apActor->GetLocationOnReference();
        TESObjectCELL* pCell = apActor->GetParentCell();
        TESWorldSpace* pWorldSpace = apActor->GetWorldSpace();
        apActor->SetPackageStartLocation(pWorldSpace, pCell, kLocation, fAngleZ);
        SetContinuingPackage(false);
        SetTarget(nullptr);
        TESPackage* pRunOncePack = GetRunOncePackage();
        if (pRunOncePack && pRunOncePack->GetPackType() != PACKAGE_TYPE_GET_UP) {
            ClearRunOncePackage();
        }

        SetCurrentActionComplete(false);
        SetPackageIdleDone(false);
        SetTargetActivated(false);
        while (!ObjectList.IsEmpty()) {
            ObjectstoAcquire* pobj = ObjectList.m_item;
            delete pobj;
            ObjectList.Remove(pobj);
        }
    }
    if (bflag) {
        StartNewPackage(apActor, GetCurrentPackage());
        SetPackageDoneOnce(false);
    }
    return bflag;
}

// Stamps the current game hour and date; the date packs the month (shift 9), the day (a signed byte, sign-extended)
// and the year (shift 13).
void LowProcess::ComputeLastTimeProcessed() {
    SetLastTimeProcessed(fGameHour);
    SetLastDateProcessed(
        (static_cast<uint32_t>(LowProcess::eCurrentGameMonth) << 9) |
        static_cast<uint32_t>(LowProcess::cCurrentGameDay) | (iGameYear << 13)
    );
}

bool LowProcess::IsWandering() {
    if (!GetCurrentPackage()) {
        return false;
    }
    PACKAGE_PROCEDURE_TYPE eProcedureType = GetCurrentPackage()->TESPackage::GetProcedureType();
    if (Procedure::Action[eProcedureType][GetProcedureIndexRunning()] == PROCEDURE_WANDER) {
        return true;
    }
    return false;
}

int32_t LowProcess::GetProcessedDay() {
    return static_cast<int32_t>(iDateLastProcessed & 0x1FF);
}

int32_t LowProcess::GetProcessedMonth() {
    return static_cast<int32_t>((iDateLastProcessed & 0x1E00) >> 9);
}

int32_t LowProcess::GetProcessedYear() {
    return static_cast<int32_t>((iDateLastProcessed & 0x7FFE000) >> 13);
}

bool LowProcess::SetTargetForPackage(Actor* apActor) {
    SetTarget(GetTargetForPackage(apActor, GetCurrentPackage()));
    return pTarget != nullptr;
}

TESObjectREFR* LowProcess::SetLocationForPackage(Actor* apActor, bool abSecondLoc) {
    TESPackage* pPackage = GetCurrentPackage();
    if (!pPackage) {
        return nullptr;
    }
    TESObjectREFR* pLocation = GetLocationForPackage(apActor, pPackage, abSecondLoc);
    if (abSecondLoc) {
        pGenericSecondLocation = pLocation;
    }
    else {
        pGenericLocation = pLocation;
    }
    return pLocation;
}

TESObjectREFR* LowProcess::GetCurrentDestinationReference(Actor* apActor, bool abInterrupt) {
    TESPackage* pPackage = GetCurrentPackage();
    Actor* pTarget = static_cast<Actor*>(this->pTarget);
    int32_t iIndex = GetProcedureIndexRunning();
    if (!abInterrupt && (!pPackage || pPackage->IsInterruptPackage())) {
        pPackage = apActor->m_Extra.GetPackageExtra();
        iIndex = apActor->m_Extra.GetPackageExtraIndex();
        pTarget = static_cast<Actor*>(apActor->m_Extra.GetPackageExtraTarget());
    }
    if (!pPackage) {
        return nullptr;
    }
    if (pPackage->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
        return nullptr;
    }
    switch (Procedure::Action[pPackage->ePROCEDURE_TYPE][iIndex]) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT:
        case PROCEDURE_ESCORT:
        case PROCEDURE_USE_WEAPON: {
            TESObjectREFR* pLocReference = nullptr;
            if (pPackage->pPackLoc) {
                pLocReference = pPackage->pPackLoc->GetLocReference();
            }
            pTarget = static_cast<Actor*>(pLocReference);
            if (!pLocReference) {
                TESObjectREFR* pLocation = nullptr;
                if (apActor->pCurrentProcess) {
                    pLocation = apActor->pCurrentProcess->GetGenericLocation();
                }
                pTarget = static_cast<Actor*>(pLocation);
            }
            if (!pTarget) {
                pTarget = apActor;
            }
            break;
        }
        case PROCEDURE_WANDER:
        case PROCEDURE_ACTIVATE:
        case PROCEDURE_ACQUIRE:
        case PROCEDURE_FOLLOW:
        case PROCEDURE_ALARM:
        case PROCEDURE_COMBAT:
        case PROCEDURE_WAIT:
        case PROCEDURE_TRAVEL_TARGET:
        case PROCEDURE_PURSUE:
        case PROCEDURE_USE_ITEM_AT:
        case PROCEDURE_AMBUSH_WAIT:
        case PROCEDURE_SUMMON_CREATURE_DEFEND:
        case PROCEDURE_SANDBOX:
        case PROCEDURE_PATROL: {
            if (!pTarget) {
                pTarget = apActor;
            }
            break;
        }
        case PROCEDURE_ACCOMPANY: {
            if (pTarget && pTarget->IsActor()) {
                TESPackage* pTargetPackage = pTarget->GetCurrentPackage();
                if (pTargetPackage) {
                    pTarget = static_cast<Actor*>(pTargetPackage->GetLocationReference(pTarget));
                }
            }

            break;
        }
        case PROCEDURE_GUARD: {
            if (pPackage->SandboxGetAllowConversation() && pPackage->pPackLoc) {
                pTarget = static_cast<Actor*>(pPackage->pPackLoc->GetLocReference());
            }
            else {
                BaseProcess* pProcess = apActor->pCurrentProcess;
                if (pProcess) {
                    pTarget = static_cast<Actor*>(pProcess->GetTarget());
                    if (!pTarget) {
                        pTarget = static_cast<Actor*>(pProcess->GetGenericLocation());
                    }
                    if (!pTarget) {
                        pTarget = apActor;
                    }
                }
            }
            break;
        }
        case PROCEDURE_DONE: {
            if (pPackage->pPackLoc) {
                pTarget = static_cast<Actor*>(pPackage->pPackLoc->GetLocReference());
            }
            if (!pTarget) {
                TESObjectREFR* pLocation = nullptr;
                if (apActor->pCurrentProcess) {
                    pLocation = apActor->pCurrentProcess->GetGenericLocation();
                }
                pTarget = static_cast<Actor*>(pLocation);
            }
            if (!pTarget) {
                pTarget = apActor;
            }
            break;
        }
        default: {
            break;
        }
    }
    return pTarget;
}

NiPoint3 LowProcess::GetCurrentDestinationCoordinate(Actor* apActor, bool abInterrupt) {
    NiPoint3 ppoint = NiPoint3::ZERO;
    TESPackage* pPack = GetCurrentPackage();
    Actor* ptargAct = static_cast<Actor*>(pTarget);
    int32_t iIndex = GetProcedureIndexRunning();
    if (!abInterrupt && pPack && pPack->IsInterruptPackage()) {
        pPack = apActor->m_Extra.GetPackageExtra();
        iIndex = apActor->m_Extra.GetPackageExtraIndex();
        ptargAct = static_cast<Actor*>(apActor->m_Extra.GetPackageExtraTarget());
    }

    if (pPack) {
        if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
            return ppoint;
        }

        switch (Procedure::Action[pPack->ePROCEDURE_TYPE][iIndex]) {
            case PROCEDURE_TRAVEL:
            case PROCEDURE_SLEEP:
            case PROCEDURE_EAT:
            case PROCEDURE_ESCORT:
            case PROCEDURE_USE_WEAPON: {
                if (pPack->pPackLoc) {
                    ppoint = pPack->GetLocationCoord(apActor);
                }
                else {
                    ppoint = apActor->GetLocationOnReference();
                }
                break;
            }
            case PROCEDURE_WANDER:
            case PROCEDURE_ACTIVATE:
            case PROCEDURE_ACQUIRE:
            case PROCEDURE_FOLLOW:
            case PROCEDURE_ALARM:
            case PROCEDURE_COMBAT:
            case PROCEDURE_WAIT:
            case PROCEDURE_TRAVEL_TARGET:
            case PROCEDURE_PURSUE:
            case PROCEDURE_USE_ITEM_AT:
            case PROCEDURE_AMBUSH_WAIT:
            case PROCEDURE_SUMMON_CREATURE_DEFEND:
            case PROCEDURE_SANDBOX:
            case PROCEDURE_PATROL: {
                if (ptargAct) {
                    ppoint = ptargAct->GetLocationOnReference();
                }
                else {
                    ppoint = apActor->GetLocationOnReference();
                }
                break;
            }
            case PROCEDURE_ACCOMPANY: {
                ppoint = apActor->GetLocationOnReference();
                if (ptargAct && ptargAct->IsActor()) {
                    TESPackage* ptargpack = ptargAct->GetCurrentPackage();
                    if (ptargpack) {
                        ppoint = ptargpack->GetLocationCoord(ptargAct);
                    }
                }

                break;
            }
            case PROCEDURE_GUARD: {
                if (pPack->SandboxGetAllowConversation() && pPack->pPackLoc) {
                    ppoint = pPack->GetLocationCoord(apActor);
                }
                else if (pPack->GetPackageTarget() && pPack->GetPackageTarget()->GetTargReference()) {
                    Actor* pTargetRef = static_cast<Actor*>(pPack->GetPackageTarget()->GetTargReference());
                    ppoint = pTargetRef->GetLocationOnReference();
                }
                else {
                    ppoint = apActor->GetLocationOnReference();
                }
                break;
            }
            case PROCEDURE_DONE: {
                if (abInterrupt) {
                    if (ptargAct) {
                        ppoint = ptargAct->GetLocationOnReference();
                    }
                    else {
                        ppoint = apActor->GetLocationOnReference();
                    }
                }
                else if (pPack->GetPackType() == PACKAGE_TYPE_TRAVEL) {
                    if (pPack->pPackLoc) {
                        ppoint = pPack->GetLocationCoord(apActor);
                    }
                    else {
                        ppoint = apActor->GetPackageStartLocationCoord();
                    }
                }
                else if (ptargAct) {
                    ppoint = ptargAct->GetLocationOnReference();
                }
                else {
                    ppoint = apActor->GetLocationOnReference();
                }
                break;
            }
            default: {
                break;
            }
        }
        return ppoint;
    }

    return ppoint;
}

TESObjectCELL* LowProcess::GetCurrentDestinationCell(Actor* apActor) {
    TESObjectCELL* pCell = nullptr;
    if (apActor) {
        pCell = apActor->GetParentCell();
    }
    if (!GetCurrentPackage()) {
        return pCell;
    }
    uint32_t uiProcedureType = GetCurrentPackage()->GetProcedureType();
    switch (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()]) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_USE_WEAPON: {
            if (GetCurrentPackage()->pPackLoc) {
                pCell = GetCurrentPackage()->GetLocationCell(apActor);
            }
            else {
                pCell = apActor->GetParentCell();
            }
            break;
        }
        case PROCEDURE_WANDER:
        case PROCEDURE_ACTIVATE:
        case PROCEDURE_ACQUIRE:
        case PROCEDURE_ALARM:
        case PROCEDURE_TRAVEL_TARGET:
        case PROCEDURE_PURSUE: {
            if (pTarget) {
                pCell = pTarget->GetParentCell();
            }
            else {
                pCell = apActor->GetParentCell();
            }
            break;
        }
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT: {
            if (GetCurrentPackage()->pPackLoc) {
                pCell = GetCurrentPackage()->GetLocationCell(apActor);
            }
            else {
                pCell = apActor->GetParentCell();
            }
            break;
        }
        case PROCEDURE_FOLLOW:
        case PROCEDURE_WAIT:
        case PROCEDURE_AMBUSH_WAIT: {
            if (pTarget) {
                pCell = pTarget->GetParentCell();
            }
            else {
                pCell = apActor->GetParentCell();
            }
            break;
        }
        case PROCEDURE_ESCORT: {
            if (GetCurrentPackage()->pPackLoc) {
                pCell = GetCurrentPackage()->GetLocationCell(apActor);
            }
            else {
                pCell = apActor->GetParentCell();
            }
            break;
        }
        case PROCEDURE_ACCOMPANY: {
            pCell = apActor->GetParentCell();
            break;
        }
        case PROCEDURE_SANDBOX: {
            SandBoxActorPackageData* pdata = static_cast<SandBoxActorPackageData*>(
                BaseProcess::GetInstanceDataThatIsRunning()
            );
            if (pdata) {
                TESObjectREFR* ptarg = pdata->pCurrentTargetObject;
                if (ptarg) {
                    pCell = ptarg->GetParentCell();
                }
            }
            break;
        }
        case PROCEDURE_GUARD: {
            if (GetCurrentPackage()->SandboxGetAllowConversation() && GetCurrentPackage()->pPackLoc) {
                pCell = GetCurrentPackage()->GetLocationCell(apActor);
            }
            else if (GetTarget()) {
                pCell = GetTarget()->GetParentCell();
            }
            else {
                pCell = apActor->GetParentCell();
            }
            break;
        }
        case PROCEDURE_DONE: {
            pCell = apActor->GetParentCell();
            break;
        }
        default: {
            break;
        }
    }
    return pCell;
}

TESWorldSpace* LowProcess::GetCurrentDestinationWorldSpace(Actor* apActor) {
    TESWorldSpace* pWorld = nullptr;
    if (apActor) {
        pWorld = apActor->GetWorldSpace();
    }
    TESPackage* ppack = GetCurrentPackage();
    if (!ppack) {
        return pWorld;
    }
    uint32_t uiProcedureType = ppack->GetProcedureType();
    switch (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()]) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_USE_WEAPON: {
            if (ppack->pPackLoc) {
                pWorld = ppack->GetLocationWorld(apActor);
            }
            else {
                pWorld = apActor->GetWorldSpace();
            }
            break;
        }
        case PROCEDURE_WANDER:
        case PROCEDURE_ACTIVATE:
        case PROCEDURE_ACQUIRE:
        case PROCEDURE_ALARM:
        case PROCEDURE_TRAVEL_TARGET:
        case PROCEDURE_PURSUE: {
            if (pTarget) {
                pWorld = pTarget->GetWorldSpace();
            }
            else {
                pWorld = apActor->GetWorldSpace();
            }
            break;
        }
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT: {
            if (ppack->pPackLoc) {
                pWorld = ppack->GetLocationWorld(apActor);
            }
            else {
                pWorld = apActor->GetWorldSpace();
            }
            break;
        }
        case PROCEDURE_FOLLOW:
        case PROCEDURE_WAIT:
        case PROCEDURE_AMBUSH_WAIT: {
            if (pTarget) {
                pWorld = pTarget->GetWorldSpace();
            }
            else {
                pWorld = apActor->GetWorldSpace();
            }
            break;
        }
        case PROCEDURE_ESCORT: {
            if (ppack->pPackLoc) {
                pWorld = ppack->GetLocationWorld(apActor);
            }
            else {
                pWorld = apActor->GetWorldSpace();
            }
            break;
        }
        case PROCEDURE_ACCOMPANY: {
            pWorld = apActor->GetWorldSpace();
            break;
        }
        case PROCEDURE_SANDBOX: {
            SandBoxActorPackageData* pdata = static_cast<SandBoxActorPackageData*>(
                BaseProcess::GetInstanceDataThatIsRunning()
            );
            if (pdata) {
                TESObjectREFR* ptarg = pdata->pCurrentTargetObject;
                if (ptarg) {
                    pWorld = ptarg->GetWorldSpace();
                }
            }
            break;
        }
        case PROCEDURE_GUARD: {
            if (ppack->SandboxGetAllowConversation() && ppack->pPackLoc) {
                pWorld = ppack->GetLocationWorld(apActor);
            }
            else if (GetTarget()) {
                pWorld = GetTarget()->GetWorldSpace();
            }
            else {
                pWorld = apActor->GetWorldSpace();
            }
            break;
        }
        case PROCEDURE_DONE: {
            pWorld = apActor->GetWorldSpace();
            break;
        }
        default: {
            break;
        }
    }
    return pWorld;
}

float LowProcess::GetCurrentDestinationRadius(Actor* apActor) {
    float fradius = 128.0f;
    if (GetCurrentPackage()) {
        uint32_t uiProcedureType = GetCurrentPackage()->ePROCEDURE_TYPE;
        switch (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()]) {
            case PROCEDURE_TRAVEL:
            case PROCEDURE_USE_WEAPON: {
                if (GetCurrentPackage()->pPackLoc) {
                    fradius = GetCurrentPackage()->GetPackageRadiusActorToLocation(apActor, false);
                }
                break;
            }
            case PROCEDURE_WANDER:
            case PROCEDURE_ACTIVATE:
            case PROCEDURE_ACQUIRE:
            case PROCEDURE_ALARM:
            case PROCEDURE_TRAVEL_TARGET:
            case PROCEDURE_PURSUE: {
                if (GetCurrentPackage()->GetPackageTarget()) {
                    fradius = GetCurrentPackage()->GetPackageRadiusActorToRefTarget(apActor, false);
                }
                break;
            }
            case PROCEDURE_FOLLOW:
            case PROCEDURE_ESCORT:
            case PROCEDURE_WAIT:
            case PROCEDURE_AMBUSH_WAIT: {
                if (GetCurrentPackage()->GetPackageTarget()) {
                    fradius = GetCurrentPackage()->GetPackageRadiusActorToRefTarget(apActor, false);
                }
                break;
            }
            case PROCEDURE_GUARD: {
                PackageTarget* ppackTarg = GetCurrentPackage()->GetPackageTarget();
                if (GetCurrentPackage()->SandboxGetAllowConversation() && GetCurrentPackage()->pPackLoc) {
                    GetCurrentPackage()->GetLocationRadius(apActor);
                }
                else if (ppackTarg) {}
                break;
            }
            default: {
                break;
            }
        }
        return fradius;
    }

    return fradius;
}

bool LowProcess::CheckAndStartCombatControllerForPackage(
    Actor* apActor,
    TESPackage* apPackage,
    bool abCheckContinueInCombat
) {
    if (!apPackage || !ShouldUseCombatControllerForPackage(apPackage, abCheckContinueInCombat)) {
        return false;
    }
    BaseProcess* pProcess = this;
    pProcess->SetTargetForPackage(apActor);
    pProcess->SetLocationForPackage(apActor, false);
    pProcess->EnterCombat(
        apActor,
        nullptr,
        false,
        false,
        nullptr,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        apPackage
    );
    return true;
}

// The procedure index of the running package ran past its last step: end the package in the extra data and, when the
// actor's current package can still drive the combat controller, hand that package back to the caller. Returns true
// when the process now ignores combat.
bool LowProcess::ShouldUpdateCombatControllerOrEndCombat(Actor* apActor, TESPackage*& apRetPackage) {
    ExtraDataList* pExtra = apActor->TESObjectREFR::GetExtra();
    TESPackage* pPackage = pExtra->GetPackageExtra();
    if (!pPackage) {
        apRetPackage = nullptr;
        return false;
    }
    int32_t iPackageIndex = apActor->TESObjectREFR::GetExtra()->GetPackageExtraIndex();
    if (ShouldStopCombatControllerToFinishPackage(pPackage, iPackageIndex)) {
        // Slot 202: BaseProcess::SetFinishingCombatPackage.
        SetFinishingCombatPackage(true);
        apRetPackage = nullptr;
        return true;
    }
    if (UpdatePackageTargetForCombat(apActor, pPackage)) {
        apRetPackage = pPackage;
        return false;
    }
    int32_t iLastProcedure = Procedure::GetProcedureCount(pPackage->TESPackage::GetProcedureType()) - 1;
    apActor->TESObjectREFR::GetExtra()->SetPackageExtraIndex(iLastProcedure);
    Script::SetActionFlag(pPackage, apActor->TESObjectREFR::GetExtra(), SCRIPT_ACTION_ONPACKAGEEND);
    PackageDone(apActor, pPackage);
    SetPackageDoneOnce(true);
    SetTargetActivated(true);
    SetCurrentActionComplete(true);
    apActor->TESObjectREFR::GetExtra()->SetPackageExtraActionComplete(true);
    TESPackage* pCurrentPackage = apActor->GetPackage();
    if (pCurrentPackage && pCurrentPackage != pPackage && ShouldUseCombatControllerForPackage(pCurrentPackage, true)) {
        TESObjectREFR* pTarget = GetTargetForPackage(apActor, pCurrentPackage);
        if (!pTarget) {
            pTarget = GetLocationForPackage(apActor, pCurrentPackage, false);
        }
        SetPackageDoneOnce(false);
        SetTargetActivated(false);
        SetCurrentActionComplete(false);
        ExtraDataList* pNewExtra = apActor->TESObjectREFR::GetExtra();
        pNewExtra->SetPackageExtra(
            pCurrentPackage,
            0,
            pTarget,
            GetCurrentActionComplete(),
            IsTargetActivated(),
            IsPackageDoneOnce()
        );
        ActorPackageData* pData = ActorPackageData::CreateActorPackageData(pCurrentPackage->GetPackType());
        if (pData) {
            pNewExtra->SetActorPackageData(pData);
        }
        StartNewPackage(apActor, pCurrentPackage);
        UpdatePackageTargetForCombat(apActor, pCurrentPackage);
        apActor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        apRetPackage = pCurrentPackage;
        return false;
    }
    apRetPackage = nullptr;
    return false;
}

void LowProcess::ProcessRemoveWorn(Actor* apActor, bool abInterateIndex) {
    TESBoundObject* pObject = apActor->TESObjectREFR::GetObjectReference();
    TESNPC* pNPC = dynamic_cast<TESNPC*>(pObject);
    pObject = apActor->TESObjectREFR::GetObjectReference();
    TESCreature* pCreature = dynamic_cast<TESCreature*>(pObject);
    TESPackage* pPackage = GetCurrentPackage();
    uint8_t bAwake = 1;
    uint8_t bWeapon = 1;
    if (pPackage) {
        if (pPackage->GetPackType() == PACKAGE_TYPE_SLEEP) {
            bAwake = 0;
            bWeapon = 0;
        }
        else {
            bWeapon = !pPackage->GetForceUnequipWeapon();
        }
    }
    if (pNPC) {
        BipedAnim* pBiped = apActor->GetBiped();
        pNPC->InitParts(apActor, pBiped, true);
    }
    else if (pCreature) {
        pCreature->InitDefaultWorn(apActor, bAwake, bWeapon, true);
    }
    if (abInterateIndex) {
        AddToProcedureIndexRunning(apActor, 1u);
    }
}

void LowProcess::CreateFollowforEscort(Actor* apActor, TESPackage* apEscort, bool abCheck) {
    Actor* pTarget = dynamic_cast<Actor*>(this->pTarget);
    if (pTarget != apActor && pTarget && pTarget != pPlayer) {
        if (apActor->GetSitSleepState()) {
            apActor->InitiateGetUpPackage();
        }
        TESPackage* pPackage = TESPackage::CreatePackage(PACKAGE_TYPE_FOLLOW);
        pPackage->SetPackType(PACKAGE_TYPE_FOLLOW);
        pPackage->SetMustReachLocation(true);
        pPackage->SetMustComplete(true);
        pPackage->SetPackageLocation(GetCurrentPackage()->GetLocation());
        uint32_t uiEscortFollowDistance = apEscort->GetEscortFollowDistance();
        pPackage->GetLocation()->SetRadius(uiEscortFollowDistance);
        pPackage->SetPackageTarget(GetCurrentPackage()->GetPackageTarget());
        pPackage->SetProcedureType(PACKAGE_PROCEDURE_FOLLOW_WITH_ESCORT);
        pPackage->GetPackageTarget()->SetTargType(PTT_REFERENCE);
        pPackage->GetPackageTarget()->SetTargReference(apActor);
        uint32_t uiTargetValue = apEscort->GetPackageTarget()->iValue;
        pPackage->GetPackageTarget()->iValue = uiTargetValue;
        pPackage->CopyAllFlags(apEscort);
        BaseProcess* pTargetProcess = pTarget->GetCurrentAIProcess();
        pTargetProcess->ComputeLastTimeProcessed();
        pTargetProcess->SavePackageToExtraData(pTarget);
        apActor->AddFollower(pTarget);
        pTarget->PutCreatedPackage(pPackage, false, true);
        pTargetProcess->SetTarget(apActor);
    }
}

void LowProcess::ModEssentialDownTimer() {
    fEssentialDownTimer = fEssentialDownTimer - appTimer.GetSeconds();
}

void LowProcess::SetEssentialDownTimer(float afvalue) {
    fEssentialDownTimer = afvalue;
}

void LowProcess::SetCurrentActionComplete(bool abValue) {
    SetBit(abValue, kFlagCurrentActionComplete);
}

bool LowProcess::GetCurrentActionComplete() {
    return GetBit(kFlagCurrentActionComplete);
}

void LowProcess::SetTarget(TESObjectREFR* apRef) {
    if (apRef) {
        apRef->SetTargeted(true);
    }
    pTarget = apRef;
    BaseProcess* pProcess = this;
    pProcess->GetActorPackageThatIsRunning()->SetTarget(apRef);
}

bool LowProcess::GetLockedLocation() {
    return GetBit(kFlagLockedLocation);
}

void LowProcess::SetLockedLocation(bool bflag) {
    SetBit(bflag, kFlagLockedLocation);
}

float LowProcess::GetPotentialThreatDamage(Actor*, Actor*) {
    return 0.0f;
}

bool LowProcess::IsPackageDone(Actor* apActor) {
    TESPackage* pPackage = GetCurrentPackage();
    if (!pPackage) {
        return true;
    }
    PACKAGE_PROCEDURE_TYPE eProcedureType = pPackage->TESPackage::GetProcedureType();
    return Procedure::Action[eProcedureType][GetProcedureIndexRunning()] == PROCEDURE_DONE;
}

void LowProcess::AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb) {
    TESPackage* pPackage = GetCurrentPackage();
    AddToCurrentProcedureIndex(ainumb);
    if ((GetCurrentProcedureIndex() & 0x80000000u) != 0) {
        SetCurrentProcedureIndex(0);
    }
    if (pPackage) {
        PACKAGE_PROCEDURE_TYPE eProcedureType = pPackage->TESPackage::GetProcedureType();
        int32_t iProcedureCount = Procedure::GetProcedureCount(eProcedureType);
        if (GetCurrentProcedureIndex() >= iProcedureCount) {
            SetCurrentProcedureIndex(iProcedureCount - 1);
        }
        eProcedureType = pPackage->TESPackage::GetProcedureType();
        if (Procedure::Action[eProcedureType][GetCurrentProcedureIndex()] == PROCEDURE_DONE && !IsPackageDoneOnce()) {
            Script::SetActionFlag(pPackage, apActor->TESObjectREFR::GetExtra(), SCRIPT_ACTION_ONPACKAGEEND);
            PackageDone(static_cast<Actor*>(apActor), pPackage);
            SetPackageDoneOnce(true);
        }
    }
}

void LowProcess::ProcessFollow(Actor* apActor, bool abflag, int32_t emovemode, bool abpursuit) {
    if (GetCurrentPackage() && GetCurrentPackage()->GetWaiting()) {
        if (pPlayer->IsPlayerFollower(apActor)) {
            return;
        }
        int32_t iNumberOfFollowers;
        iNumberOfFollowers = pPlayer->GetNumberOfFollowers();
        if (iNumberOfFollowers >= iNumberActorsAllowedToFollowPlayer.Int()) {
            return;
        }
        GetCurrentPackage()->SetWaiting(false);
    }
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget ||
        (GetCurrentPackage()->pPackLoc &&
         GetCurrentPackage()->IsActorAtLocation(apActor, false, -1.0f, false) &&
         GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_FOLLOW)) {
        AddToProcedureIndexRunning(apActor, 1);
        if (GetCurrentPackage()->GetIsCreated()) {
            ClearCurrentDataforProcess(apActor);
            ClearCurrentPackage();
            CheckforNewPackage(apActor, false);
        }
    }
    else {
        GetTimeDelta(apActor);
        float fDistance = GetCurrentPackage()->GetPackageRadiusActorToRefTarget(apActor, false);
        TESWorldSpace* pws1 = apActor->GetWorldSpace();
        TESWorldSpace* pws2 = pTarget->GetWorldSpace();
        if (apActor->IsPathingComplete() &&
            pTarget &&
            (pws1 != pws2 ||
             static_cast<double>(fDistance) < apActor->GetDistanceFromReference(pTarget, false, false))) {
            TESWorldSpace* pWorldSpace = pTarget->GetWorldSpace();
            TESObjectCELL* pCell = pTarget->GetParentCell();
            const NiPoint3& kLocation = pTarget->GetLocationOnReference();
            apActor->SetPathfindingGoal(kLocation, pCell, pWorldSpace, fDistance, nullptr);
        }
    }
}

void LowProcess::ProcessPursue(Actor* apActor, bool abNewpackage, bool abInterateIndex, int32_t emovemode) {
    if (!this->pTarget) {
        SetTargetForPackage(apActor);
    }
    if (this->pTarget) {
        Actor* pTarget = static_cast<Actor*>(this->pTarget);
        if (pTarget->GetCurrentProcessType() < PROCESS_TYPE_MIDDLE_HIGH && !apActor->IsAlarmed()) {
            AddToProcedureIndexRunning(apActor, 2);
            pTarget->RemoveFleeTarget(apActor);
        }
        if (apActor->GetDistanceFromReference(this->pTarget, false, false) > 200.0) {
            ProcessFollow(apActor, false, emovemode, true);
        }
        else if (!apActor->IsActoraGuard() || GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_COMBAT_LOW) {
            AddToProcedureIndexRunning(apActor, 1);
        }
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
    }
}

void LowProcess::ProcessFlee(Actor* apActor) {
    if (GetCurrentPackage()->IsActorAtLocation(apActor, false, -1.0f, false)) {
        apActor->EndInterruptPackage(false);
    }
    else {
        GetTimeDelta(apActor);
    }
}

// Says aptopic's matching info to the actor's dialogue target (taken and cleared) and runs its begin script; with
// abSayCallBack the actor records the topic and info and the say-to callback runs at once. The current package is read
// and discarded.
void LowProcess::ProcessGreet(Actor* apActor, TESTopic* apGreetTopic, bool, bool, bool, bool abSayCallBack) {
    BaseProcess* pProcess = this;
    pProcess->GetCurrentPackage();
    MobileObject* pTarget = apActor->GetDialogueItemTarget();
    apActor->SetDialogueItemTarget(nullptr);
    if (!pTarget) {
        pTarget = apActor->GetDialogueItemTarget();
    }
    if (!apGreetTopic->GetEmpty()) {
        return;
    }
    DialogueItem* pItem = apGreetTopic->CreateDialogueItem(apActor, pTarget, nullptr, nullptr, true);
    if (!pItem) {
        return;
    }
    pItem->FirstResponse();
    if (!pItem->GetCurrentResponse()) {
        return;
    }
    pItem->RunResult(TIRS_BEGIN);
    if (abSayCallBack) {
        apActor->SetSayToTopic(apGreetTopic);
        apActor->SetSayToTopicInfo(pItem->GetInfo());
        MobileObject::SayToCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())), true);
        apActor->UpdateSoundCallBack();
    }
}

void LowProcess::UpdateFollowers(Actor* apActor, float afDelta) {
    Actor* ptarget;
    bool bremoveall = false;
    if (apActor->IsDead(false) || apActor->GetDisabled()) {
        bremoveall = true;
    }
    ExtraFollower* pfollow = apActor->m_Extra.GetFollowerExtra();
    if (pfollow) {
        BSSimpleList<Actor*>* pTempList = new BSSimpleList<Actor*>;
        BSSimpleList<Actor*>* pHead = new BSSimpleList<Actor*>;
        BSSimpleList<Actor*>* plist = pHead;
        for (BSSimpleList<Actor*>* ptemp = pfollow->pActorlist; ptemp && ptemp->m_item; ptemp = ptemp->m_pkNext) {
            pHead->AddTail(ptemp->m_item);
        }
        while (plist && plist->m_item) {
            ptarget = plist->m_item;
            if (bremoveall) {
                pTempList->AddHead(ptarget);
            }
            else if (ptarget && ptarget != pPlayer) {
                bool bremove = false;
                TESPackage* pactpack = apActor->GetPackageSetAsPcurrent();
                TESPackage* pactEdit = apActor->m_Extra.GetPackageExtra();
                if ((!pactpack || pactpack->IsInterruptPackage()) && pactEdit) {
                    pactpack = pactEdit;
                }

                if (pactpack && pactpack->GetPackType() == PACKAGE_TYPE_ESCORT) {
                    bremove = true;
                }

                TESPackage* ptargpack = ptarget->GetPackageSetAsPcurrent();
                TESPackage* pEditorpack = ptarget->m_Extra.GetPackageExtra();
                if (!ptargpack || (ptargpack->IsInterruptPackage() && pEditorpack)) {
                    ptargpack = pEditorpack;
                }
                if (!bremove &&
                    (!ptargpack ||
                     (ptargpack->GetPackType() != PACKAGE_TYPE_FOLLOW &&
                      ptargpack->GetPackType() != PACKAGE_TYPE_ACCOMPANY))) {
                    pTempList->AddHead(ptarget);
                }
                else if (ptarget->GetCurrentProcessType()) {
                    ptarget->Process(afDelta);
                    ptarget->RunScript();
                }
            }
            plist = plist->m_pkNext;
        }
        for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
            apActor->m_Extra.RemoveFollower(plist->m_item);
        }
        pHead->RemoveAllPointerList();
        if (pHead) {
            delete pHead;
        }
        pTempList->RemoveAllPointerList();
        if (pTempList) {
            delete pTempList;
        }
        if (pfollow->pActorlist->IsEmpty()) {
            apActor->m_Extra.RemoveFollowerExtra();
        }
    }
}

bool LowProcess::GetAlert() {
    return GetBit(kFlagAlerted);
}

void LowProcess::SetAlert(bool abFlag) {
    SetBit(abFlag, kFlagAlerted);
}

bool LowProcess::EnterCombat(
    Actor* apActor,
    Actor* apTarget,
    bool abCheckForAmbush,
    bool absendalarm,
    CombatGroup* apGroup,
    bool abfleeing,
    bool abForceFlee,
    bool abIsAggressor,
    char acTeam,
    bool bforcecombat,
    bool bIgnoreDefensivePackage,
    bool abCastSpell,
    TESPackage* apPackage
) {
    apActor->StartCombat(apTarget, apGroup, bforcecombat, abIsAggressor, abfleeing, 0, abCastSpell, apPackage);
    return true;
}

bool LowProcess::IsAFollower() {
    return GetBit(kFlagFollower);
}

// The base value plus the actor's permanent and this process's modifiers, truncated; an enumerated value is the base
// value alone. The int base value and the float permanent modifier are summed as a double, then the process modifier is
// added and the result truncated. The modifier lookups take the index as a byte.
int32_t LowProcess::GetActorValue(TESActorBase*, ActorValue::Index aiWhich, Actor* apActor) {
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner = apActor;
        return pOwner->GetBaseActorValue(aiWhich);
    }
    float fModifier = 0.0f;
    if (apActor) {
        fModifier = apActor->pPermanentModifiers.GetModifier(static_cast<uint8_t>(aiWhich));
    }
    ActorValueOwner* pOwner = apActor;
    const double dValue = static_cast<double>(pOwner->GetBaseActorValue(aiWhich)) + fModifier;
    return static_cast<int32_t>(pDamageModifiers.GetModifier(static_cast<uint8_t>(aiWhich)) + dValue);
}

// As GetActorValue for the float value: the base value plus the permanent modifier is summed as a double and the final
// sum is rounded to a float; an enumerated value returns the base value unchanged.
float LowProcess::GetActorFloatValue(TESActorBase*, ActorValue::Index aiWhich, Actor* apActor) {
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        ActorValueOwner* pOwner = apActor;
        return pOwner->GetBaseActorFloatValue(aiWhich);
    }
    float fModifier = 0.0f;
    if (apActor) {
        fModifier = apActor->pPermanentModifiers.GetModifier(static_cast<uint8_t>(aiWhich));
    }
    ActorValueOwner* pOwner = apActor;
    const double dValue = static_cast<double>(pOwner->GetBaseActorFloatValue(aiWhich)) + fModifier;
    return static_cast<float>(pDamageModifiers.GetModifier(static_cast<uint8_t>(aiWhich)) + dValue);
}

void LowProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION) && aiWhich == ActorValue::HEALTH) {
        double dClamped = Actor::CheckClampDamageModifier(
            static_cast<ActorValueOwner*>(apActor),
            ActorValue::HEALTH,
            afValue
        );
        pDamageModifiers.AdjustModifier(16, static_cast<float>(dClamped), ModifierList::LE_ZERO);
    }
}

void LowProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) {
    DamageModActorValue(apActor, aiWhich, static_cast<float>(aiValue));
}

void LowProcess::ClearObjectFromAcquireList(TESObjectREFR* apRef) {
    if (pAcquireObject && pAcquireObject->pRef == apRef) {
        delete pAcquireObject;
        pAcquireObject = nullptr;
    }

    if (pSavedAcquireObject && pSavedAcquireObject->pRef == apRef) {
        delete pSavedAcquireObject;
        pSavedAcquireObject = nullptr;
    }

    BSSimpleList<ObjectstoAcquire*>* plist = &ObjectList;
    while (plist && plist->m_item) {
        ObjectstoAcquire* pobj = plist->m_item;
        if (pobj->pRef == apRef) {
            ObjectList.Remove(pobj);
            delete pobj;
            plist = &ObjectList;
        }
        else {
            plist = plist->m_pkNext;
        }
    }
    if (pGenericLocation == apRef) {
        pGenericLocation = nullptr;
    }
    BSSimpleList<TESObjectREFR*>* preflist = &GenericLocationsList;
    BSSimpleList<TESObjectREFR*>* plastlist = nullptr;
    while (preflist && preflist->m_item) {
        TESObjectREFR* pref = preflist->m_item;
        if (pref == apRef) {
            if (plastlist) {
                plastlist->Remove(pref);
                preflist = plastlist->m_pkNext;
            }
            else {
                preflist->RemoveHead();
            }
        }
        else {
            plastlist = preflist;
            preflist = preflist->m_pkNext;
        }
    }
}

void LowProcess::ClearObjectFromAcquireList(uint32_t aID) {
    if (pAcquireObject && pAcquireObject->pRef->iFormID == aID) {
        delete pAcquireObject;
        pAcquireObject = nullptr;
    }

    if (pSavedAcquireObject && pSavedAcquireObject->pRef->iFormID == aID) {
        delete pSavedAcquireObject;
        pSavedAcquireObject = nullptr;
    }

    BSSimpleList<ObjectstoAcquire*>* plist = &ObjectList;
    while (plist && plist->m_item) {
        ObjectstoAcquire* pobj = plist->m_item;
        if (pobj->pRef->iFormID == aID) {
            ObjectList.Remove(pobj);
            delete pobj;
            plist = &ObjectList;
        }
        else {
            plist = plist->m_pkNext;
        }
    }
    if (pGenericLocation && pGenericLocation->iFormID == aID) {
        pGenericLocation = nullptr;
    }

    BSSimpleList<TESObjectREFR*>* preflist = &GenericLocationsList;
    BSSimpleList<TESObjectREFR*>* plastlist = nullptr;
    while (preflist && preflist->m_item) {
        TESObjectREFR* pref = preflist->m_item;
        if (pref->iFormID == aID) {
            if (plastlist) {
                plastlist->Remove(pref);
                preflist = plastlist->m_pkNext;
            }
            else {
                preflist->RemoveHead();
            }
        }
        else {
            plastlist = preflist;
            preflist = preflist->m_pkNext;
        }
    }
}

bool LowProcess::IsTargetActivated() {
    return GetBit(kFlagTargetActivated);
}

void LowProcess::SetTargetActivated(bool abFlag) {
    SetBit(abFlag, kFlagTargetActivated);
}

void LowProcess::SetSecondGenericLocation(TESObjectREFR* apLoc) {
    pGenericSecondLocation = apLoc;
}

void LowProcess::ClearCurrentDataforProcess(Actor* apActor) {
    SetCurrentActionComplete(false);
    SetPackageIdleDone(false);
    SetTargetActivated(false);
    SetTarget(nullptr);
    pGenericLocation = nullptr;
    if (pAcquireObject) {
        delete pAcquireObject;
        pAcquireObject = nullptr;
    }
    if (pSavedAcquireObject) {
        ObjectList.Remove(pSavedAcquireObject);
        delete pSavedAcquireObject;
        pSavedAcquireObject = nullptr;
    }
    while (!ObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj = ObjectList.m_item;
        delete pobj;
        ObjectList.Remove(pobj);
    }
    GenericLocationsList.RemoveAll();
}

uint16_t LowProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    uint16_t uiSaveSize = BaseProcess::GetSaveSize(aiFlags, pmob);
    uint16_t uiBaseSize = uiSaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uiSaveSize += 6;
    }
    int16_t iDataSize = m_uFlags + uiSaveSize;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 75u) {
        iDataSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 79u) {
        iDataSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 86u) {
        iDataSize += 4;
    }
    uint16_t isize = iDataSize + 8;
    if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
        isize += pDamageModifiers.GetSaveSize();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 116u) {
        isize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize - uiBaseSize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                3802,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize - uiBaseSize,
                3802,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
    }
    return isize;
}

void LowProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    BaseProcess::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&m_uFlags)), sizeof(m_uFlags), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fEssentialDownTimer)), sizeof(fEssentialDownTimer), 0);
    apSaveGameBuffer->SaveFormID(pItemBeingUsed, 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&iNumberItemsActivate)),
        sizeof(iNumberItemsActivate),
        0
    );
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fDeathTime)), sizeof(fDeathTime), 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fTrackedDamage)), sizeof(fTrackedDamage), 0);
    apSaveGameBuffer->SaveDataEndian(
        static_cast<char*>(static_cast<void*>(&bSkippedTimeStampForPathing)),
        sizeof(bSkippedTimeStampForPathing),
        0
    );
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&sFormListNumb)), sizeof(sFormListNumb), 0);
    fCombatDelayTimer.SaveGame(apSaveGameBuffer);
    TESObjectREFR* pSavedTarget = pAcquireObject ? nullptr : pTarget;
    apSaveGameBuffer->SaveFormID(pSavedTarget, 0);
    apSaveGameBuffer->SaveFormID(pGenericLocation, 0);
    apSaveGameBuffer->SaveFormID(pGenericSecondLocation, 0);
    apSaveGameBuffer->SaveFormID(pTargetFormList, 0);
    apSaveGameBuffer->SaveFormID(pPatrolLocation, 0);
    uint32_t uiVariableValue = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<TESObjectREFR*>* plist = &GenericLocationsList; plist; plist = plist->m_pkNext) {
        TESObjectREFR* pref = plist->m_item;
        if (pref) {
            apSaveGameBuffer->SaveFormID(pref, 0);
            ++uiVariableValue;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiVariableValue, uiPosition);
    BGSChangeFlags kChangeFlags = apSaveGameBuffer->GetChangeFlags();
    if (kChangeFlags.CheckFlags(CHANGE_ACTOR_DAMAGE_MODIFIERS)) {
        pDamageModifiers.SaveGame(apSaveGameBuffer);
    }
}

void LowProcess::SaveGame(uint32_t aiFlags, MobileObject* pmob) {
    uint32_t iitemBeingUsedID;
    uint32_t iblockHeader;
    uint16_t iblockSize;
    uint32_t itargetID;
    uint32_t ilocationID;
    BaseProcess::SaveGame(aiFlags, pmob);
    iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK'
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&m_uFlags, sizeof(m_uFlags));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 75u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fEssentialDownTimer, sizeof(fEssentialDownTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 79u) {
        iitemBeingUsedID = 0;
        if (pItemBeingUsed) {
            iitemBeingUsedID = pItemBeingUsed->iFormID;
        }
        pSaveLoadGameOLD->SaveNumericID(&iitemBeingUsedID, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 86u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&iNumberItemsActivate, sizeof(iNumberItemsActivate));
    }
    itargetID = 0;
    if (pTarget && !pAcquireObject) {
        itargetID = pTarget->iFormID;
    }

    pSaveLoadGameOLD->SaveNumericID(&itargetID, 4u);
    ilocationID = 0;
    if (pGenericLocation) {
        ilocationID = pGenericLocation->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&ilocationID, 4u);
    if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
        pDamageModifiers.SaveGame();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 116u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fDeathTime, sizeof(fDeathTime));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&fTrackedDamage, sizeof(fTrackedDamage));
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                pBufferPointer - itempStartPosition,
                pheader->iFormID,
                pEditorID,
                iFlags,
                3872,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                pBufferPointer - itempStartPosition,
                3872,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pbufferEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(static_cast<void*>(pblockSize));
        // The addresses are compared unsigned.
        if (pbufferEnd > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                3872
            );
        }
        *pblockSize = static_cast<uint16_t>(pbufferEnd - pblockStart);
    }
}

void LowProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t uiTempData;
    BaseProcess::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() < 6u) {
        char ptempData[4];
        apLoadGameBuffer->LoadData(ptempData, sizeof(ptempData));
        apLoadGameBuffer->LoadData(&uiTempData, sizeof(uiTempData));
    }
    apLoadGameBuffer->LoadData(&m_uFlags, sizeof(m_uFlags));
    apLoadGameBuffer->LoadData(&fEssentialDownTimer, sizeof(fEssentialDownTimer));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pLoadedForm = TESForm::GetFormByNumericID(uiFormID);
    pItemBeingUsed = dynamic_cast<TESBoundObject*>(pLoadedForm);
    apLoadGameBuffer->LoadData(&iNumberItemsActivate, sizeof(iNumberItemsActivate));
    apLoadGameBuffer->LoadData(&fDeathTime, sizeof(fDeathTime));
    apLoadGameBuffer->LoadData(&fTrackedDamage, sizeof(fTrackedDamage));
    apLoadGameBuffer->LoadData(&bSkippedTimeStampForPathing, sizeof(bSkippedTimeStampForPathing));
    apLoadGameBuffer->LoadData(&sFormListNumb, sizeof(sFormListNumb));
    fCombatDelayTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pTarget);
    apLoadGameBuffer->LoadFormID(pGenericLocation);
    apLoadGameBuffer->LoadFormID(pGenericSecondLocation);
    uiFormID = apLoadGameBuffer->LoadFormID();
    pLoadedForm = TESForm::GetFormByNumericID(uiFormID);
    pTargetFormList = dynamic_cast<BGSListForm*>(pLoadedForm);
    apLoadGameBuffer->LoadFormID(pPatrolLocation);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        TESObjectREFR* pForm = nullptr;
        apLoadGameBuffer->LoadFormID(pForm);
        GenericLocationsList.AddHead(pForm);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_DAMAGE_MODIFIERS)) {
        pDamageModifiers.LoadGame(apLoadGameBuffer);
    }
}

void LowProcess::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) {
    uint32_t iitemBeingUsedID;
    float ftemp;
    uint32_t iblockHeader;
    uint16_t iblockSize;
    uint32_t itargetID;
    uint32_t ilocationID;
    BaseProcess::LoadGame(aiFlags, aiCurrentFlags, pmob);
    iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                    3885,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                    3885,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 54u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&ftemp, sizeof(ftemp));
        int32_t ivalue = static_cast<int32_t>(std::fmod(ftemp, 100.0f));
        double dMonthValue = std::fmod(ftemp, 10000.0f);
        int32_t imonth = static_cast<int32_t>(dMonthValue) / 100;
        int32_t iyear = static_cast<int32_t>(ftemp) / 0x2710;
        iDateLastProcessed = iyear << 13 | (ivalue | imonth << 9);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&m_uFlags, sizeof(m_uFlags));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 75u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fEssentialDownTimer, sizeof(fEssentialDownTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 79u) {
        pSaveLoadGameOLD->LoadNumericID(&iitemBeingUsedID, 4u);
        TESForm* pform = TESForm::GetFormByNumericID(iitemBeingUsedID);
        pItemBeingUsed = dynamic_cast<TESBoundObject*>(pform);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 86u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iNumberItemsActivate, sizeof(iNumberItemsActivate));
    }
    pSaveLoadGameOLD->LoadNumericID(&itargetID, 4u);
    SetTarget(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(itargetID)));
    pSaveLoadGameOLD->LoadNumericID(&ilocationID, 4u);
    pGenericLocation = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(ilocationID));
    if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
        pDamageModifiers.LoadGameTES();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 51u && (aiFlags & CHANGE_ACTOR_PERMANENT_MODIFIERS) != 0) {
        uint16_t icount;
        pSaveLoadGameOLD->LoadGameDataOLD(&icount, sizeof(icount));
        if (icount) {
            pSaveLoadGameOLD->AdvanceBuffer(8 * icount);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 116u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fDeathTime, sizeof(fDeathTime));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&fTrackedDamage, sizeof(fTrackedDamage));
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t uiFlags = pheader->iFlags;
                    int32_t iVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                        3959,
                        pheader->iFormID,
                        pEditorID,
                        iVersion,
                        uiFlags
                    );
                }
            }
            else {
                uint32_t uiFlags = pheader->iFlags;
                int32_t iVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                    3959,
                    pheader->iFormID,
                    pEditorID,
                    iVersion,
                    uiFlags
                );
            }
        }
        else if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
            if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                    3959,
                    iVersion
                );
            }
        }
        else {
            int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                icurrentBufferPosition - &istartBufferPosition[iblockSize],
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp",
                3959,
                iVersion
            );
        }
    }
}

void LowProcess::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    BaseProcess::InitLoadGame(apLoadGameBuffer);
    TESObjectREFR* ptempTarget = nullptr;
    if (pTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        ptempTarget = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pTarget = nullptr;
    SetTarget(ptempTarget);
    TESObjectREFR* pLocation = nullptr;
    if (pGenericLocation) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGenericLocation))
        );
        pLocation = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pGenericLocation = pLocation;
    TESObjectREFR* pSecondLocation = nullptr;
    if (pGenericSecondLocation) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGenericSecondLocation))
        );
        pSecondLocation = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pGenericSecondLocation = pSecondLocation;
    TESObjectREFR* pPatrolRef = nullptr;
    if (pPatrolLocation) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPatrolLocation))
        );
        pPatrolRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pPatrolLocation = pPatrolRef;
    BSSimpleList<TESObjectREFR*>* plist = &GenericLocationsList;
    BSSimpleList<TESObjectREFR*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        TESObjectREFR* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* pRefForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<TESObjectREFR*>(pRefForm);
            if (pForm) {
                plist->SetItem(pForm);
            }
            else {
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastList) {
                plastList->Remove(plist->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
}

// Resolves the saved target and generic location form IDs back into references and marks the target targeted. Calls the
// base body directly, not through the vtable.
void LowProcess::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) {
    BaseProcess::InitLoadGame(aiFlags, aiOldFlags, pmob);
    TESObjectREFR* pSavedTarget = pTarget;
    if (pSavedTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSavedTarget)));
        TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
        // Calls the override through virtual dispatch, not directly.
        static_cast<BaseProcess*>(this)->SetTarget(pRef);
        if (pTarget) {
            pTarget->SetTargeted(true);
        }
    }
    TESObjectREFR* pSavedLocation = pGenericLocation;
    if (pSavedLocation) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSavedLocation))
        );
        pGenericLocation = dynamic_cast<TESObjectREFR*>(pForm);
    }
}

void LowProcess::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    BaseProcess::Revert(apLoadGameBuffer);
    ClearData();
    iNumberItemsActivate = 0;
    SetTarget(nullptr);
    fEssentialDownTimer = 0.0f;
    pItemBeingUsed = nullptr;
    fCombatDelayTimer.Clear();
    fDeathTime = 0.0f;
    m_uFlags = 0;
    fTrackedDamage = 0.0f;
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_DAMAGE_MODIFIERS)) {
        pDamageModifiers.Revert();
    }
}

void LowProcess::Revert(uint32_t aiFlags, MobileObject* pmob) {
    BaseProcess::Revert(aiFlags, pmob);
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        iNumberItemsActivate = 0;
        while (!ObjectList.IsEmpty()) {
            ObjectstoAcquire* pobj = ObjectList.m_item;
            ObjectList.Remove(pobj);
            delete pobj;
        }
        GenericLocationsList.RemoveAll();
        pAcquireObject = nullptr;
        pSavedAcquireObject = nullptr;
        SetTarget(nullptr);
        pGenericLocation = nullptr;
        fEssentialDownTimer = 0.0f;
        pItemBeingUsed = nullptr;
        fDeathTime = 0.0f;
        m_uFlags = 0;
        fTrackedDamage = 0.0f;
    }
    if (!pmob->GetDelete()) {
        pSaveLoadGameOLD->IsUnk048Zero();
    }
    if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
        pDamageModifiers.Revert();
    }
}

void LowProcess::StartNewPackage(Actor* apActor, TESPackage* apPack) {
    if (apPack) {
        Script* pScript = apPack->GetOnBeginScript();
        if (pScript) {
            ScriptLocals* pLocals = apActor->GetExtra()->GetScriptLocals();
            pScript->Run(apActor, pLocals, nullptr, true);
        }
        TESTopic* pTopic = apPack->GetOnBeginTopicInfo();
        if (pTopic) {
            BaseProcess* pProcess = this;
            pProcess->ProcessGreet(apActor, pTopic, false, false, true, false);
        }
    }
    if (apActor) {
        apActor->SetWalkRunFlagDirty();
    }
}

void LowProcess::ChangePackage(Actor* apActor, TESPackage* apPack) {
    if (apPack) {
        Script* pScript = apPack->GetOnChangeScript();
        if (pScript && (pScript->GetCompiledData() || pScript->GetText())) {
            ScriptLocals* pLocals = apActor->TESObjectREFR::GetExtra()->GetScriptLocals();
            pScript->Run(apActor, pLocals, nullptr, true);
        }

        TESTopic* pTopic = apPack->GetOnChangeTopicInfo();
        if (pTopic) {
            ProcessGreet(apActor, pTopic, false, false, true, false);
        }
    }
}

void LowProcess::PackageDone(Actor* apActor, TESPackage* apPack) {
    if (apPack) {
        Script* pScript = apPack->GetOnEndScript();
        if (pScript) {
            ScriptLocals* pLocals = apActor->GetExtra()->GetScriptLocals();
            pScript->Run(apActor, pLocals, nullptr, true);
        }
        TESTopic* pTopic = apPack->GetOnEndTopic();
        if (pTopic) {
            ProcessGreet(apActor, pTopic, false, false, true, false);
        }
    }
}

bool LowProcess::IsPackageDoneOnce() {
    return GetBit(kFlagPackageDoneOnce);
}

void LowProcess::SetPackageDoneOnce(bool abflag) {
    SetBit(abflag, kFlagPackageDoneOnce);
}

float LowProcess::GetDamageModifier(ActorValue::Index aiWhich) {
    return pDamageModifiers.GetModifier(aiWhich);
}

void LowProcess::SetDeathTime(float afDeathTime) {
    fDeathTime = afDeathTime;
}

// Saves the running package (unless it was created at run time) into the actor's package extra data, then hands the
// package's per-actor data over to the extra data list.
void LowProcess::SavePackageToExtraData(MobileObject* apMob) {
    if (!GetCurrentPackage() || GetCurrentPackage()->GetIsCreated()) {
        return;
    }
    ExtraDataList* pExtra = apMob->TESObjectREFR::GetExtra();
    // The arguments are evaluated last to first.
    bool bDoneOnce = IsPackageDoneOnce();
    bool bActivated = IsTargetActivated();
    bool bComplete = GetCurrentActionComplete();
    TESObjectREFR* pRef = pTarget;
    int32_t iIndex = GetCurrentProcedureIndex();
    pExtra->SetPackageExtra(GetCurrentPackage(), iIndex, pRef, bComplete, bActivated, bDoneOnce);
    apMob->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    ActorPackageData* pData = BaseProcess::GetInstanceDataThatIsRunning();
    if (!pData) {
        return;
    }
    if (apMob->GetCurrentAIProcess() && apMob->GetCurrentAIProcess()->GetActorPackageThatIsRunning()) {
        apMob->GetCurrentAIProcess()->GetActorPackageThatIsRunning()->GetTarget();
    }
    pExtra->SetActorPackageData(pData);
    CurrentPackage.NullInstanceData();
}

void LowProcess::LoadPackageFromExtraData(MobileObject* apMob) {
    ExtraDataList* pextra = &apMob->m_Extra;
    if (pextra->GetPackageExtra()) {
        if (apMob->IsActor()) {
            ClearCurrentDataforProcess(static_cast<Actor*>(apMob));
        }
        TESObjectREFR* pExtraTarget = pextra->GetPackageExtraTarget();
        CurrentPackage.InitFromPackage(pextra->GetPackageExtra(), pExtraTarget);
        // The current package is only handed to an empty function on apMob, so its result is unused and only the
        // virtual call stays.
        GetCurrentPackage();
        int32_t iindex = pextra->GetPackageExtraIndex();
        PACKAGE_PROCEDURE_TYPE eprocType = CurrentPackage.pPackage->ePROCEDURE_TYPE;
        if (eprocType != PACKAGE_PROCEDURE_NONE && iindex >= Procedure::GetProcedureCount(eprocType)) {
            iindex = 0;
        }
        SetCurrentProcedureIndex(iindex);
        SetTarget(pextra->GetPackageExtraTarget());
        SetCurrentActionComplete(pextra->GetPackageExtraComplete());
        SetPackageDoneOnce(pextra->GetPackageExtraDoneOnce());
        SetTargetActivated(pextra->GetPackageExtraActivate());
        pextra->RemovePackageExtra();
        ActorPackageData* pactorPackData = pextra->GetActorPackageData();
        if (pactorPackData) {
            CurrentPackage.pData = pactorPackData;
            pextra->RemoveActorPackageData();
        }
    }
    else {
        if (apMob->IsActor()) {
            ClearCurrentDataforProcess(static_cast<Actor*>(apMob));
        }
        ClearCurrentPackage();
        SetTarget(nullptr);
        SetCurrentActionComplete(false);
        SetPackageIdleDone(false);
        SetPackageDoneOnce(false);
        SetTargetActivated(false);
    }
}

void LowProcess::ModTrackedDamage(Actor* apAggressor, float afAmount) {
    if (apAggressor && (apAggressor->IsPc() || apAggressor->Actor::IsPlayerTeammate())) {
        fTrackedDamage = fTrackedDamage + afAmount;
    }
}

void LowProcess::ProcessEscort(Actor* apActor) {
    NiPoint3 destcoord;
    float ftargdistance;
    float fMydistance;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    int32_t inumb = 0;
    ItemChange* pitem = apActor->GetItemBeingEscorted();
    if (pitem ||
        (pTarget && (pTarget->IsActor() || apActor->HasObjectId(pTarget->iFormID))) ||
        ((SetTargetForPackage(apActor), pTarget) && (pTarget->IsActor() || apActor->HasObjectId(pTarget->iFormID)))) {
        if (!pitem && pAcquireObject && pAcquireObject->pRef == apActor) {
            pitem = apActor->GetInventoryItem(pAcquireObject->pObject, 0);
        }

        if (GetCurrentPackage()->GetPackageTarget()) {
            inumb = GetCurrentPackage()->GetInitialTargetCount();
        }
        if ((pTarget && (pTarget->IsActor() || apActor->HasObjectId(pTarget->iFormID))) ||
            CheckIfHasObject(apActor, inumb)) {
            if (GetCurrentPackage()->IsActorAtLocation(apActor, false, -1.0f, false)) {
                if ((!pTarget || pTarget->IsActor()) && pTarget) {
                    if (GetCurrentPackage()
                            ->IsActorAtLocation(static_cast<MobileObject*>(pTarget), false, -1.0f, false)) {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else {
                    if (pitem) {
                        ExtraDataList* plist = nullptr;
                        if (pitem->pExtraObjectList) {
                            plist = pitem->pExtraObjectList->m_item;
                        }
                        uint32_t uiID = 0;
                        if (plist && plist->GetReferencePointer()) {
                            uiID = plist->GetReferencePointer()->iFormID;
                        }

                        TESObjectREFR* pcont = pPack->pPackLoc->GetLocReference();
                        if (!pcont) {
                            pcont = pGenericLocation;
                        }
                        if (pcont && pcont->HasContainer()) {
                            int32_t iInitialTargetCount = pPack->GetInitialTargetCount();
                            apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, iInitialTargetCount, uiID);
                        }
                        else {
                            if (pitem->pExtraObjectList) {
                                plist = pitem->pExtraObjectList->m_item;
                            }
                            const NiPoint3* ppoint = nullptr;
                            PackageLocation* ppackloc = pPack->pPackLoc;
                            PackageTarget* ppacktarg = pPack->GetPackageTarget();
                            if (ppackloc) {
                                TESObjectREFR* pref = ppackloc->GetLocReference();
                                if (!pref) {
                                    pref = pGenericLocation;
                                }
                                if (pref) {
                                    TESBoundObject* pObject = pref->GetObjectReference();
                                    if (pObject == pXMarker || pref->GetObjectReference() == pXMarkerHeading) {
                                        ppoint = &pref->GetLocationOnReference();
                                    }
                                }
                            }
                            int32_t inumber = 1;
                            if (!ppacktarg->GetTargReference()) {
                                inumber = pPack->GetInitialTargetCount();
                            }
                            apActor->DropObject(pitem->pContainerObj, plist, inumber, ppoint, nullptr);
                        }
                    }
                    AddToProcedureIndexRunning(apActor, 1);
                }
            }
            else {
                int32_t iDistance = static_cast<int32_t>(
                    GetCurrentPackage()->GetPackageRadiusActorToRefTarget(apActor, false)
                );
                fMydistance = 0.0f;
                ftargdistance = 0.0f;
                if (pTarget && pTarget->IsActor()) {
                    destcoord = pPack->GetLocationCoord(apActor);
                    TESObjectCELL* pdestcell = pPack->GetLocationCell(apActor);
                    TESWorldSpace* pdestworld = pPack->GetLocationWorld(apActor);
                    PathingLocation pl1(static_cast<TESObjectREFR*>(apActor));
                    PathingLocation pl2(&destcoord, pdestcell, pdestworld);
                    Pathing::ComputeLowPathDistance(pl1, pl2, &fMydistance);
                    Actor* ptargactor = static_cast<Actor*>(pTarget);
                    PathingLocation pl3(static_cast<TESObjectREFR*>(ptargactor));
                    Pathing::ComputeLowPathDistance(pl3, pl2, &ftargdistance);
                }

                if ((!pTarget ||
                     !pTarget->IsActor() ||
                     static_cast<double>(iDistance) > apActor->GetDistanceFromReference(pTarget, false, false) ||
                     ftargdistance < static_cast<double>(fMydistance)) &&
                    apActor->IsPathingComplete()) {
                    TESWorldSpace* pWorldSpace = GetCurrentPackage()->GetLocationWorld(apActor);
                    TESObjectCELL* pCell = GetCurrentPackage()->GetLocationCell(apActor);
                    NiPoint3 kLocation = GetCurrentPackage()->GetLocationCoord(apActor);
                    apActor->SetPathfindingGoal(kLocation, pCell, pWorldSpace, -1.0f, nullptr);
                }
            }
            if (pitem) {
                delete pitem;
            }
        }
        else {
            AddToProcedureIndexRunning(apActor, 1);
            if (pitem) {
                delete pitem;
            }
        }
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
    }
}

void LowProcess::ProcessCombat(Actor* apActor) {
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget || GetCurrentPackage()->IsActorAtRefTarget(apActor, 0)) {
        if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
            pPlayer->SetSleeping(0, true);
            AddToProcedureIndexRunning(apActor, -1);
        }
        else {
            int32_t iRounds = static_cast<int32_t>(fCombatRoundAmount.Float() * 4.0);
            Actor* pTargetActor = nullptr;
            char cFought = 0;
            while (iRounds) {
                if (apActor->IsDead(false)) {
                    break;
                }
                apActor->PerformOneRoundofCombat();
                if (!apActor->IsInCombat()) {
                    break;
                }
                pTargetActor = dynamic_cast<Actor*>(pTarget);
                if (pTargetActor) {
                    pTargetActor->SetCurrentTarget(apActor);
                    if (!pTargetActor->IsDead(false)) {
                        if (!pTargetActor->IsInCombat()) {
                            pTargetActor->GetCurrentAIProcess()->EnterCombat(
                                pTargetActor,
                                apActor,
                                false,
                                false,
                                nullptr,
                                false,
                                false,
                                false,
                                0,
                                false,
                                false,
                                true,
                                nullptr
                            );
                        }
                        pTargetActor->PerformOneRoundofCombat();
                        cFought = 1;
                    }
                }
                --iRounds;
            }
            if (cFought) {
                pTargetActor->GetCurrentAIProcess()->ComputeLastTimeProcessed();
            }
        }
    }
    else {
        ProcessFollow(apActor, false, 257, false);
    }
}

void LowProcess::ProcessAlarm(Actor* apActor) {
    TESPackage* pPackage = GetCurrentPackage();
    AlarmPackage* ppack = dynamic_cast<AlarmPackage*>(pPackage);
    Actor* pactor = nullptr;
    if (ppack) {
        if (apActor->GetClass()->IsGuard()) {
            AddToProcedureIndexRunning(apActor, 1);
            ProcessPursue(apActor, false, true, 257);
            if (apActor->GetDistanceFromReference(pPlayer, false, false) > 3500.0 && pPlayer->GetWorldSpace()) {
                TESWorldSpace* pPlayerWorld = pPlayer->GetWorldSpace();
                if (pPlayerWorld == apActor->GetWorldSpace()) {
                    apActor->EndInterruptPackage(false);
                }
            }
        }
        else {
            BSSimpleList<Crime*>* plist = &ppack->Crimes;
            bool bcombatflag = false;
            while (plist && plist->m_item && !bcombatflag) {
                Crime* pcrime = plist->m_item;
                int32_t pcrimeAct = 0;
                Actor* pcriminal = pcrime->pcriminal;
                FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                if (!bcombatflag) {
                    if (pcrime->ecrime <= PackageNS::CRIME_TYPE_TRESPASS) {
                        if (apActor->GetShouldAttackActor(pcriminal, false, ereaction, false)) {
                            bcombatflag = true;
                        }
                    }
                    else if (pcrimeAct && apActor->GetShouldAttackActor(pcriminal, false, ereaction, false)) {
                        bcombatflag = true;
                    }
                }
                plist = plist->m_pkNext;
            }
            if (bcombatflag) {
                ProcessFollow(apActor, false, 257, false);
                EnterCombat(
                    apActor,
                    pactor,
                    false,
                    false,
                    nullptr,
                    false,
                    false,
                    false,
                    0,
                    false,
                    false,
                    true,
                    nullptr
                );
            }
            else {
                AddToProcedureIndexRunning(apActor, 3);
            }
        }
    }
}

// Runs the activate procedure: paths apActor to the package target, then activates it (taking the acquire object, or
// attacking or stealing from an actor target) and runs the target's script. Returns false while still on the way.
bool LowProcess::ProcessActivate(Actor* apActor, bool abInterateindex) {
    TESPackage* pPackage = GetCurrentPackage();
    if (!pTarget || pTarget->GetDelete()) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget) {
        if (abInterateindex) {
            AddToProcedureIndexRunning(apActor, 2);
        }
        return false;
    }
    PackageTarget* pPackTarget = pPackage->GetPackageTarget();
    if (pPackTarget->GetTargType() == PTT_REFERENCE || pPackTarget->GetTargType() == PTT_LINKEDREF) {
        iNumberItemsActivate = 0;
    }
    TESObjectREFR* pLocRef = pPackage->GetLocationReference(apActor);
    if (pLocRef &&
        pLocRef->IsFurniture() &&
        (pTarget->GetObjectReference() == pXMarkerHeading || pTarget->GetObjectReference() == pXMarker)) {
        return true;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_AMBUSH && !pPackage->IsActorAtLocation(apActor, false, 300.0f, false)) {
        AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-2));
    }
    bool bAtTarget = false;
    float fRadius = static_cast<float>(GetCurrentPackage()->GetPackageRadiusActorToRefTarget(apActor, false));
    if (pTarget && fRadius < apActor->GetDistanceFromReference(pTarget, false, false) && apActor->IsPathingComplete()) {
        PROCESS_TYPE eOldLevel = apActor->GetCurrentProcessType();
        if (!apActor->SetPathfindingGoal(pTarget, -1.0f, nullptr)) {
            return false;
        }
        if (apActor->GetCurrentProcessType() != eOldLevel) {
            return true;
        }
    }
    bAtTarget = pPackage->IsActorAtRefTarget(apActor, 0);
    if (!bAtTarget) {
        return false;
    }
    TESObjectREFR* pActivated = pTarget;
    if (abInterateindex) {
        TESForm* pObject = nullptr;
        uint32_t iCount = 1;
        bool bRefDead = false;
        if (pAcquireObject) {
            if (!pAcquireObject->pRef || pAcquireObject->pRef->IsDead(false) || !pAcquireObject->pRef->IsNpc()) {
                pObject = pAcquireObject->pObject;
                iCount = pAcquireObject->iamount;
                iNumberItemsActivate -= iCount;
                if (pAcquireObject->pRef->IsDead(false)) {
                    bRefDead = true;
                }
            }
            if (iNumberItemsActivate <= 0) {
                AddToProcedureIndexRunning(apActor, 2);
            }
        }
        if (!pTarget->IsActor() || bRefDead) {
            pTarget->Activate(apActor, true, static_cast<TESBoundObject*>(pObject), iCount);
            ObjectstoAcquire* pOld = pAcquireObject;
            delete pOld;
            pAcquireObject = nullptr;
            SetTarget(nullptr);
            AddToProcedureIndexRunning(apActor, 2);
        }
        else if (pTarget->IsActor()) {
            if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
                pPlayer->SetSleeping(0, false);
                return false;
            }
            if (pAcquireObject) {
                if (pAcquireObject->bKill) {
                    EnterCombat(
                        apActor,
                        static_cast<Actor*>(pTarget),
                        false,
                        false,
                        nullptr,
                        false,
                        true,
                        false,
                        true,
                        false,
                        false,
                        true,
                        nullptr
                    );
                }
                else if (pAcquireObject->bSteal) {
                    pTarget->Activate(apActor, true, pAcquireObject->pObject, iCount);
                    ObjectstoAcquire* pOld = pAcquireObject;
                    delete pOld;
                    pAcquireObject = nullptr;
                    SetTarget(nullptr);
                }
            }
            else if (pTarget != pPlayer) {
                AddToProcedureIndexRunning(apActor, 2);
                return false;
            }
        }
    }
    else if (!pTarget->IsActor()) {
        pTarget->Activate(apActor, false, nullptr, 1);
    }
    else if (pPackage->GetPackType() == PACKAGE_TYPE_ESCORT) {
        apActor->GetCurrentAIProcess()->AddToProcedureIndexRunning(apActor, 2);
    }
    else {
        if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
            pPlayer->SetSleeping(0, true);
            return false;
        }
        return true;
    }
    if (pActivated) {
        pActivated->RunScript();
    }
    return true;
}

void LowProcess::ProcessAccompany(Actor* apActor) {
    ProcessFollow(apActor, true, 0x101u, false);
}

void LowProcess::ProcessAcquire(Actor* apActor) {
    PackageTarget* pPackTarget = nullptr;
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (pTarget) {
        pPackTarget = GetCurrentPackage()->GetPackageTarget();
    }
    if (pTarget &&
        (GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT ||
         GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_USE_WEAPON)) {
        switch (pTarget->GetObjectReference()->GetFormType()) {
            case ACTI_ID:
            case CONT_ID:
            case DOOR_ID:
            case STAT_ID:
            case SCOL_ID:
            case GRAS_ID:
            case TREE_ID:
            case FLOR_ID:
            case FURN_ID:
            case NPC__ID:
            case CREA_ID:
            case LVLC_ID:
            case LVLN_ID:
            case ACHR_ID:
            case ACRE_ID: {
                AddToProcedureIndexRunning(apActor, 1);
                return;
            }
            case LIGH_ID: {
                TESObjectLIGH* plight = dynamic_cast<TESObjectLIGH*>(pTarget->GetObjectReference());
                if (plight && !plight->GetCanCarry()) {
                    AddToProcedureIndexRunning(apActor, 1);
                }

                return;
            }
            default: {
                break;
            }
        }
    }
    if ((pPackTarget && pPackTarget->GetTargReference()) || GetCurrentPackage()->GetMustComplete()) {
        int32_t inumb = 1;
        if (GetCurrentPackage()->GetPackageTarget()) {
            inumb = GetCurrentPackage()->GetInitialTargetCount();
        }
        if (CheckIfHasObject(apActor, inumb)) {
            if (GetCurrentPackage()->IsActorAtLocation(apActor, false, -1.0f, false) ||
                GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_EAT) {
                if (GetCurrentPackage()->GetProcedureType() != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC ||
                    GetCurrentPackage()->GetPackageSchedule()->PSData.iDuration) {
                    AddToProcedureIndexRunning(apActor, 1);
                }
                else {
                    AddToProcedureIndexRunning(apActor, 2);
                }
            }
            else {
                SetProcedureIndexRunning(0);
            }
            if (GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ESCORT) {
                ProcessEscort(apActor);
            }
        }
        else if (apActor->GetCurrentProcessType() >= PROCESS_TYPE_MIDDLE_LOW) {
            ProcessActivate(apActor, true);
        }
        else {
            ProcessActivateOneHour(apActor, 3600.0f);
        }
    }
    else if (GetCurrentPackage()->GetProcedureType() == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
        AddToProcedureIndexRunning(apActor, 2);
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
    }
}

void LowProcess::ProcessAmbushWait(Actor* apActor) {
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (pTarget && !pTarget->GetDisabled()) {
        if (!pTarget->GetDelete()) {
            if (!pTarget->IsDead(true) || pAcquireObject) {
                if (pPackage->IsActorAtLocation(apActor, false, -1.0f, false)) {
                    float fDistance = apActor->GetDistanceFromReference(pTarget, false, false);
                    if (fDistance != 3.4028234663852886e+38 && pPackage->IsTargetAtLocation(apActor, 0)) {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else {
                    AddToProcedureIndexRunning(apActor, -1);
                }
            }
            else {
                pPackage->SetNeverRun(pTarget, true);
            }
            return;
        }
        if (pTarget->GetDelete()) {
            pPackage->SetNeverRun(pTarget, true);
        }
    }
    AddToProcedureIndexRunning(apActor, 2);
}

// Returns false on every path.
bool LowProcess::ProcessUseItemAt(Actor* apActor) {
    if (GetCurrentPackage()) {
        TESPackage* pPackage = GetCurrentPackage();
        if (!pPackage->IsActorAtLocation(apActor, false, -1.0f, false)) {
            AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-1));
            return false;
        }
    }
    if (GetCurrentPackage()->GetMustComplete()) {
        AddToProcedureIndexRunning(apActor, 1);
    }
    return false;
}

// Returns false on every path.
bool LowProcess::ProcessUseWeapon(Actor* apActor) {
    if (GetCurrentPackage()) {
        TESPackage* pPackage = GetCurrentPackage();
        if (!pPackage->IsActorAtLocation(apActor, false, -1.0f, false)) {
            AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-1));
            return false;
        }
    }
    if (GetCurrentPackage()->GetMustComplete()) {
        AddToProcedureIndexRunning(apActor, 1);
    }
    return false;
}

void LowProcess::ProcessFleeNonCombat(Actor* apActor) {
    TESPackage* pPackage = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    SetLocationForPackage(apActor, false);
    float fdistance = 0.0f;
    if (pPackage->GetPackageTarget()) {
        fdistance = static_cast<float>(static_cast<double>(pPackage->GetPackageTarget()->iValue));
    }
    if ((!pTarget || !(static_cast<double>(fdistance) < apActor->GetDistanceFromReference(pTarget, false, false))) &&
        !pTarget) {
        pPackage->IsActorAtLocation(apActor, false, -1.0f, false);
    }
}

void LowProcess::ProcessAvoidArea(Actor* apActor) {
    AddToProcedureIndexRunning(apActor, 1);
}

void LowProcess::ProcessSandBox(Actor* apActor) {
    SandBoxActorPackageData* pPackData = static_cast<SandBoxActorPackageData*>(
        BaseProcess::GetInstanceDataThatIsRunning()
    );
    if (pPackData) {
        switch (pPackData->eState) {
            case SandBoxActorPackageData::SANDBOX_STATE_INIT: {
                TESPackage* pPackage = GetPackageThatIsRunning();
                pPackData->eState = SandBoxActorPackageData::SANDBOX_STATE_MOVING;
                pPackData->InitLocation(apActor, pPackage->GetLocation());
                break;
            }
            case SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP:
            case SandBoxActorPackageData::SANDBOX_STATE_MOVING: {
                pPackData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                [[fallthrough]];
            }
            case SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF: {
                if (apActor->IsPathingComplete()) {
                    TESObjectREFR* pOldLocation = pPackData->pLocation;
                    if (pPackData->MoveToNextLocation()) {
                        TESObjectREFR* pNewLocation = pPackData->pLocation;
                        if (pNewLocation && pNewLocation != pOldLocation) {
                            TESObjectCELL* pOldCell = pOldLocation->GetParentCell();
                            TESObjectCELL* pNewCell = pNewLocation->GetParentCell();
                            bool bNeitherLoaded = (!pOldCell || !TES::pInstance->IsCellLoaded(pOldCell, false)) &&
                                (!pNewCell || !TES::pInstance->IsCellLoaded(pNewCell, false));
                            bool bSameCell = pOldCell && pOldCell == pNewCell;
                            if (bNeitherLoaded) {
                                apActor->SetPosition(pNewLocation->GetLocationOnReference());
                                if (!bSameCell) {
                                    TESObjectREFR::MoveRefToNewSpace(apActor, pNewCell, pNewLocation->GetWorldSpace());
                                }
                            }
                            else {
                                float fLocationRadius = static_cast<float>(
                                    GetPackageThatIsRunning()->GetLocationRadius(apActor)
                                );
                                float fRadius = BSMAX(fLocationRadius, 10.0f);
                                apActor->SetPathfindingGoal(
                                    pNewLocation,
                                    fRadius,
                                    static_cast<PathingAvoidNodeArray*>(nullptr)
                                );
                            }
                        }
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
    }
}

void LowProcess::ProcessMovementBlocked(Actor* apActor) {
    double dHour = appCalendar.GetHour();
    float fTimeDiff = dHour - CurrentPackage.GetPackageStartTime();
    float fAbsTimeDiff = std::fabs(fTimeDiff);
    if (!(fAbsTimeDiff >= 0.10000000149011612)) {
        apActor->StopMoving();
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
        TESPackage* pCurrPackage = CurrentPackage.pPackage;
        pCurrPackage->SetIsCreated(true);
    }
}

void LowProcess::ProcessGuard(Actor* apActor) {
    GuardActorPackageData* pdata = static_cast<GuardActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
    if (GetRunOncePackage()) {
        if (GetRunOncePackage()->GetPackType() == PACKAGE_TYPE_GUARD) {
            pdata = static_cast<GuardActorPackageData*>(GetRunOncePackageInstanceData());
        }
    }
    if (pdata) {
        TESPackage* ppack = GetPackageThatIsRunning();
        TESObjectREFR* pguardedRef = ppack->ComputeInitialTarget(apActor);
        if (!pguardedRef) {
            pguardedRef = apActor;
        }
        TESObjectREFR* pholder = pguardedRef->m_Extra.GetReferencePointer();
        if (pholder) {
            pguardedRef = pholder;
        }
        if (GetTarget() != pguardedRef) {
            SetTarget(pguardedRef);
            pguardedRef->m_Extra.AddGuard(apActor);
            pguardedRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }
        if (!IsRunningRunOnce() &&
            (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
             apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
            pdata->eState = GuardActorPackageData::GUARD_STATE_INIT;
            apActor->InitiateGetUpPackage();
        }
        else if (!apActor->IsImmobile()) {
            bool bAtLoc;
            if (ppack->pPackLoc) {
                bAtLoc = ppack->IsActorAtLocation(apActor, false, -1.0f, false);
                ppack->GetLocationRadius(apActor);
            }
            else {
                float ftargRadius;
                PackageTarget* ppackTarg = ppack->GetPackageTarget();
                if (ppackTarg) {
                    ftargRadius = static_cast<float>(ppackTarg->iValue);
                }
                else {
                    ftargRadius = 30.0f;
                }
                bAtLoc = static_cast<double>(ftargRadius) >
                    apActor->GetDistanceFromReference(pguardedRef, false, false);
            }
            if ((apActor->IsPathingComplete() && !bAtLoc) || apActor->DidPathingFail()) {
                ProcessGuardStartTravelToLocation(apActor, pdata);
            }
        }
    }
    else {
        SetProcedureIndexRunning(-1);
    }
}

void LowProcess::ProcessGuardStartTravelToLocation(Actor* apActor, GuardActorPackageData* apData) {
    float fguardRadius;
    NiPoint3 location;
    TESObjectREFR* preferenceForLocation = nullptr;
    TESObjectCELL* pcell = nullptr;
    TESWorldSpace* pworldspace = nullptr;
    TESPackage* ppack = GetPackageThatIsRunning();
    PackageLocation* ploc = ppack->pPackLoc;
    PackageTarget* ptarg = ppack->GetPackageTarget();
    bool buseTargetAsDefault = true;
    if (ploc) {
        if (ploc->GetLocType() == PLT_EDITORLOC) {
            location = apActor->GetEditorLocationCoord();
            pcell = apActor->GetEditorLocationInteriorCell();
            pworldspace = apActor->GetEditorLocationWorld();
            buseTargetAsDefault = false;
        }
        else if (ploc->GetLocType() == PLT_CURRLOC) {
            preferenceForLocation = apActor;
        }
        else if (ploc->GetLocType() == PLT_LINKEDREF) {
            preferenceForLocation = apActor->GetLinkedRef();
        }
        else {
            preferenceForLocation = ploc->GetLocReference();
            if (!preferenceForLocation && !GetRunOncePackage()) {
                preferenceForLocation = pGenericLocation;
            }
        }
    }
    if (!preferenceForLocation && buseTargetAsDefault) {
        preferenceForLocation = GetTarget();
    }
    if (ploc) {
        fguardRadius = static_cast<float>(ppack->GetLocationRadius(apActor));
    }
    else if (ptarg) {
        fguardRadius = static_cast<float>(ptarg->iValue);
    }
    else {
        fguardRadius = 30.0f;
    }
    float fhalfRadius = fguardRadius * 0.5;
    fguardRadius = BSMAX(fhalfRadius, 15.0f);
    if (preferenceForLocation) {
        PathingRequest request;
        TESWorldSpace* pWorldSpace = preferenceForLocation->GetWorldSpace();
        TESObjectCELL* pCell = preferenceForLocation->GetParentCell();
        const NiPoint3& kLocation = preferenceForLocation->GetLocationOnReference();
        apActor->BuildRequest(&request, kLocation, pCell, pWorldSpace, fguardRadius, nullptr);
        request.SetSoftGoalRadius(false);
        apActor->SetPathfindingGoal(&request);
        apData->eState = GuardActorPackageData::GUARD_STATE_IDLE;
    }
    else if (pcell || pworldspace) {
        PathingRequest request;
        apActor->BuildRequest(&request, location, pcell, pworldspace, fguardRadius, nullptr);
        request.SetSoftGoalRadius(false);
        apActor->SetPathfindingGoal(&request);
        apData->eState = GuardActorPackageData::GUARD_STATE_IDLE;
    }
}

void LowProcess::ProcessPatrol(Actor* apActor) {
    PatrolActorPackageData* pPackageData = static_cast<PatrolActorPackageData*>(
        BaseProcess::GetInstanceDataThatIsRunning()
    );
    if (pPackageData) {
        if (apActor->GetSitSleepState()) {
            apActor->InitiateGetUpPackage();
        }
        else {
            float fTimeDelta = GetTimeDelta(apActor);
            while (fTimeDelta > 0.0) {
                uint32_t eState = pPackageData->eState;
                if (eState) {
                    if (eState == 1) {
                        fTimeDelta = Patrol_TravelingToNextRefLow(apActor, pPackageData, fTimeDelta);
                    }
                    else if (eState == 2) {
                        fTimeDelta = Patrol_AtRefLow(apActor, pPackageData, fTimeDelta);
                    }
                    else {
                        fTimeDelta = 0.0;
                    }
                }
                else if (!Patrol_StartLow(apActor, pPackageData)) {
                    fTimeDelta = 0.0;
                }
            }
        }
    }
    else {
        SetProcedureIndexRunning(-1);
    }
}

void LowProcess::ProcessSummonCreatureDefend(Actor* apActor) {
    ProcessFollow(apActor, false, 0x101u, false);
}

void LowProcess::ProcessNotify(Actor* apActor) {
    AddToProcedureIndexRunning(apActor, 1);
}

bool LowProcess::CheckIfHasObject(Actor* apActor, int32_t ainumb) {
    PACKAGE_OBJECT_TYPE eUseItem;
    TESPackage* pPack = GetCurrentPackage();
    PackageTarget* ptarg = pPack->GetPackageTarget();
    if (!ptarg) {
        return false;
    }
    PACK_TARG_TYPE etype = ptarg->GetTargType();
    bool bdone = false;
    switch (etype) {
        case PTT_REFERENCE:
        case PTT_LINKEDREF: {
            TESObjectREFR* pref = CurrentPackage.pTarget;
            if (pref) {
                uint32_t uiFormID = pref->iFormID;
                bdone = apActor->HasObjects(pref->GetObjectReference(), POTYPE_NONE, 1, uiFormID, eUseItem);
            }
            else {
                bdone = false;
            }
            break;
        }
        case PTT_OBJECT: {
            TESBoundObject* pTargObject = static_cast<TESBoundObject*>(ptarg->GetTargObject());
            bdone = apActor->HasObjects(pTargObject, POTYPE_NONE, ainumb, 0, eUseItem);
            break;
        }
        case PTT_TYPE: {
            PACKAGE_OBJECT_TYPE eTargObjectType = ptarg->GetTargObjectType();
            bdone = apActor->HasObjects(nullptr, eTargObjectType, ainumb, 0, eUseItem);
            break;
        }
        default: {
            break;
        }
    }
    return bdone;
}

TESObjectREFR* LowProcess::GetTargetForPackage(Actor* apActor, TESPackage* apPackage) {
    if (!apPackage) {
        return nullptr;
    }
    TESObjectREFR* preturntarget = nullptr;
    bool bpackagesmatch = apPackage == GetCurrentPackage();
    if (apPackage->GetPackageTarget()) {
        PackageTarget* ptarg = apPackage->GetPackageTarget();
        if (ptarg->GetTargType()) {
            if (ptarg->GetTargType() == PTT_LINKEDREF) {
                preturntarget = apActor->GetLinkedRef();
            }
            else if (apPackage->GetMustComplete()) {
                if (ObjectList.IsEmpty()) {
                    switch (apPackage->GetPackType()) {
                        case PACKAGE_TYPE_FOLLOW:
                        case PACKAGE_TYPE_ESCORT:
                        case PACKAGE_TYPE_ACCOMPANY: {
                            if (apPackage->IsTargetAnActor(apActor)) {
                                preturntarget = apPackage->GetEscortTargetForPackage(apActor, false);
                                break;
                            }
                            [[fallthrough]];
                        }
                        case PACKAGE_TYPE_PATROL: {
                            if (bpackagesmatch) {
                                PatrolActorPackageData* pdata = static_cast<PatrolActorPackageData*>(
                                    BaseProcess::GetInstanceDataThatIsRunning()
                                );
                                if (pdata) {
                                    preturntarget = pdata->GetCurrentReference();
                                }
                            }
                            if (!preturntarget) {
                                preturntarget = PatrolActorPackageData::GetInitialReferenceForSetup(apActor, apPackage);
                            }
                            break;
                        }
                        default: {
                            TESObjectCELL* pCell = apActor->GetParentCell();
                            NiPoint3 pointB = apActor->GetLocationOnReference();
                            PackageTarget* ptarget = apPackage->GetPackageTarget();
                            PACK_TARG_TYPE etype = ptarget->GetTargType();
                            if (etype == PTT_OBJECT) {
                                TESForm* pForm = ptarget->GetTargObject();
                                if (pForm->GetFormType() == FLST_ID) {
                                    if (pTargetFormList) {
                                        ++sFormListNumb;
                                    }
                                    else {
                                        pTargetFormList = static_cast<BGSListForm*>(pForm);
                                        sFormListNumb = 0;
                                    }
                                    BSSimpleList<TESForm*>* plist = &pTargetFormList->ListOfForms;
                                    if (!plist->IsEmpty()) {
                                        for (int32_t inum = 0; plist && !plist->IsEmpty() && sFormListNumb <= inum;
                                             ++inum) {
                                            plist = plist->m_pkNext;
                                        }
                                    }
                                    if (pForm) {
                                        pObjecttoAcquire = static_cast<TESBoundObject*>(pForm);
                                    }
                                    eFormType = POTYPE_NONE;
                                }
                                else if (pForm->GetFormType() == FACT_ID) {
                                    pFactiontoAquire = static_cast<TESFaction*>(pForm);
                                    eFormType = POTYPE_ACTOR;
                                }
                                else {
                                    pObjecttoAcquire = static_cast<TESBoundObject*>(ptarget->GetTargObject());
                                    eFormType = POTYPE_NONE;
                                }
                            }
                            else if (etype == PTT_TYPE) {
                                pObjecttoAcquire = nullptr;
                                eFormType = ptarget->GetTargObjectType();
                            }
                            float facquireDist = 0.0f;
                            if (apPackage->GetPackageSearchLocation()) {
                                float fsearchRadius;
                                uint32_t uiradius = apPackage->GetSearchLocationRadius(apActor);
                                pCell = apPackage->GetSearchLocationCell(apActor);
                                pointB = apPackage->GetSearchLocationCoord(apActor);
                                if (uiradius) {
                                    fsearchRadius = static_cast<float>(uiradius);
                                }
                                else if (pCell->GetInterior()) {
                                    fsearchRadius = std::numeric_limits<float>::max();
                                }
                                else {
                                    fsearchRadius = fAIAcquireObjectDistance.Float();
                                }
                                facquireDist = fsearchRadius;
                                pDataHandler->EnumReferencesCloseToPoint(
                                    pCell,
                                    pointB,
                                    fsearchRadius,
                                    pointB,
                                    fsearchRadius,
                                    &LowProcess::EvaluateReferenceforAcquire,
                                    apActor
                                );
                            }
                            else {
                                if (!apPackage->GetAcquireRadius(apActor, facquireDist)) {
                                    facquireDist = fAIAcquireObjectDistance.Float();
                                }
                                float fRadius2 = facquireDist;
                                const NiPoint3& kLocation = apActor->GetLocationOnReference();
                                pDataHandler->EnumReferencesCloseToPoint(
                                    pCell,
                                    pointB,
                                    facquireDist,
                                    kLocation,
                                    fRadius2,
                                    &LowProcess::EvaluateReferenceforAcquire,
                                    apActor
                                );
                            }
                            eFormType = POTYPE_NONE;
                            pObjecttoAcquire = nullptr;
                            EvaluateOrderAcquireList(apActor);
                            ptarget = apPackage->GetPackageTarget();
                            if (ptarget && ptarget->GetTargType()) {
                                iNumberItemsActivate = apPackage->GetInitialTargetCount();
                            }

                            break;
                        }
                    }
                }
                if (!ObjectList.IsEmpty()) {
                    pAcquireObject = ObjectList.m_item;
                    if (pAcquireObject->eType == ACQUIRE_TYPE_BUY_NPC) {
                        if (pAcquireObject->pRef->IsActor()) {
                            preturntarget = pAcquireObject->pRef;
                        }
                        else {
                            TESForm* pOwner = pAcquireObject->pRef->GetOwner();
                            TESNPC* pnpc = dynamic_cast<TESNPC*>(pOwner);
                            if (pnpc) {
                                preturntarget = ReferenceProcessLists.GetActorRefInHigh(pnpc, false);
                            }
                        }
                    }
                    else {
                        preturntarget = pAcquireObject->pRef;
                    }
                    ObjectList.Remove(pAcquireObject);
                }
            }
        }
        else {
            if (!ptarg->GetTargReference()) {
                return preturntarget;
            }
            if (ptarg->GetTargReference()->IsDead(true)) {
                apPackage->SetNeverRun(ptarg->GetTargReference(), true);
                return preturntarget;
            }
            switch (apPackage->GetPackType()) {
                case PACKAGE_TYPE_FOLLOW:
                case PACKAGE_TYPE_ESCORT:
                case PACKAGE_TYPE_ACCOMPANY: {
                    if (apPackage->IsTargetAnActor(apActor)) {
                        preturntarget = apPackage->GetEscortTargetForPackage(apActor, false);
                        break;
                    }
                    [[fallthrough]];
                }
                case PACKAGE_TYPE_PATROL: {
                    PatrolActorPackageData* pdata = static_cast<PatrolActorPackageData*>(
                        BaseProcess::GetInstanceDataThatIsRunning()
                    );
                    if (pdata) {
                        preturntarget = pdata->GetCurrentReference();
                    }
                    if (!pTarget) {
                        preturntarget = PatrolActorPackageData::GetInitialReferenceForSetup(apActor, apPackage);
                    }
                    break;
                }
                default: {
                    preturntarget = ptarg->GetTargReference();
                    break;
                }
            }
            if (preturntarget && preturntarget->GetDelete() && !preturntarget->IsActor()) {
                // Tests this address (&preturntarget->m_Extra), which is never null.
                preturntarget = preturntarget->m_Extra.GetReferencePointer();
            }
        }
    }
    if (preturntarget && preturntarget->GetDelete() && preturntarget->m_Extra.GetReferencePointer()) {
        return preturntarget->m_Extra.GetReferencePointer();
    }

    return preturntarget;
}

TESObjectREFR* LowProcess::GetLocationForPackage(Actor* apActor, TESPackage* apPackage, bool abSecondLoc) {
    TESObjectREFR* preturnloc = nullptr;
    PackageLocation* pLoc = nullptr;
    if (abSecondLoc) {
        pLoc = apPackage->GetPackageSecondLocation();
    }
    else {
        pLoc = apPackage->pPackLoc;
    }
    if (pLoc) {
        if (pLoc->GetLocType() == PLT_REFERENCE) {
            preturnloc = pLoc->GetLocReference();
        }
        if (pLoc->GetLocType() == PLT_LINKEDREF) {
            return apActor->GetLinkedRef();
        }

        if (pLoc->IsLocationGeneric() && apPackage->GetMustComplete()) {
            if (GenericLocationsList.IsEmpty()) {
                TESObjectCELL* pCell = apActor->GetParentCell();
                NiPoint3 pointB = apActor->GetLocationOnReference();
                PACK_LOC_TYPE etype = pLoc->GetLocType();
                if (etype == PLT_OBJECT) {
                    pObjectForLocation = static_cast<TESBoundObject*>(pLoc->GetLocObject());
                    eFormType = POTYPE_NONE;
                }
                else if (etype == PLT_TYPE) {
                    pObjectForLocation = nullptr;
                    eFormType = pLoc->GetLocObjectType();
                }
                float facquireDist = 0.0f;
                if (!apPackage || !apPackage->GetAcquireRadius(apActor, facquireDist)) {
                    facquireDist = fAIAcquireObjectDistance.Float();
                }
                float fRadius2 = facquireDist;
                const NiPoint3& kLocation = apActor->GetLocationOnReference();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    pointB,
                    facquireDist,
                    kLocation,
                    fRadius2,
                    &EvaluateGenericLocation,
                    apActor
                );
                eFormType = POTYPE_NONE;
                pObjectForLocation = nullptr;
                for (BSSimpleList<TESObjectREFR*>* plist = &TempLocList; plist->m_item; plist->RemoveHead()) {
                    GenericLocationsList.AddHead(plist->m_item);
                }
            }
            if (!GenericLocationsList.IsEmpty()) {
                preturnloc = GenericLocationsList.m_item;
                GenericLocationsList.RemoveHead();
            }
        }
    }
    return preturnloc;
}

void LowProcess::EvaluateOrderAcquireList(Actor* apActor) {
    BSSimpleList<ObjectstoAcquire*>* pRefItem = &TempObjectList;
    TESPackage* ppack = GetPackageThatIsRunning();
    bool ballowBuying = ppack ? ppack->SearchGetAllowBuying() : false;
    bool ballowStealing = ppack ? ppack->SearchGetAllowStealing() : false;
    bool ballowKilling = ppack ? ppack->SearchGetAllowKilling() : false;
    while (pRefItem && pRefItem->m_item) {
        ObjectstoAcquire* pObject = pRefItem->m_item;
        TESObjectREFR* pRef = pObject->pRef;
        TESBoundObject* pObj = pObject->pObject;
        Actor* pbuyit = pRef->IsActor() ? static_cast<Actor*>(pRef) : nullptr;
        char cBuying = 0;
        bool badd = true;
        if (pbuyit) {
            badd = !pbuyit->IsPlayerTeammate();
        }
        if ((pRef->GetOwner() && pRef->GetOwner() != apActor->GetObjectReference()) || pRef->IsActor()) {
            bool bsteal = false;
            bool bkill = false;
            int32_t ivalue = TESValueForm::GetFormValue(pObj);
            TESNPC* pnpc = nullptr;
            if (!pRef->IsDead(false)) {
                if (apActor->GetGoldAmount() >= ivalue) {
                    Actor* pbuyactor = nullptr;
                    if (pRef->IsNpc()) {
                        pbuyactor = static_cast<Actor*>(pRef);
                    }
                    else {
                        TESForm* pOwnerForm = pRef->GetOwner();
                        pnpc = dynamic_cast<TESNPC*>(pOwnerForm);
                        if (pnpc) {
                            pbuyactor = ReferenceProcessLists.GetActorRefInHigh(pnpc, false);
                        }
                    }
                    if (pbuyactor && ballowBuying && pbuyactor->GetActorBase()->SellBuysItem(pObj)) {
                        if (apActor->IsInCombat()) {
                            badd = false;
                        }
                        else {
                            pObject->eType = ACQUIRE_TYPE_BUY_NPC;
                            pObject->iWantValue = static_cast<int32_t>(
                                apActor->GetDistanceFromReference(pRef, false, false)
                            );
                            cBuying = 1;
                        }
                    }
                }
            }
            if (!cBuying && (!pbuyit || !pbuyit->IsPlayerTeammate())) {
                if (pRef->IsActor()) {
                    if (pRef->IsDead(false)) {
                        badd = true;
                    }
                    else if (pRef->IsNpc()) {
                        if (ballowStealing) {
                            bsteal = true;
                        }
                        if (ballowKilling) {
                            bkill = true;
                        }
                    }
                }
                else if (ballowStealing) {
                    bsteal = true;
                }
                if ((!bsteal && !bkill) || (pbuyit && pbuyit->IsPlayerTeammate())) {
                    if (apActor->IsCreature()) {
                        badd = false;
                    }
                    else if (
                        !pRef->IsActor() ||
                        pRef->IsNpc() ||
                        pRef->GetOwner() == apActor->GetObjectReference() ||
                        apActor->bPlayerTeammate
                    ) {
                        badd = false;
                    }
                    else if (pbuyit && !pbuyit->IsPlayerTeammate()) {
                        pObject->eType = ACQUIRE_TYPE_KILL;
                        badd = true;
                    }
                }
                else {
                    if (pRef->IsActor()) {
                        if (bsteal) {
                            if (apActor->IsInCombat()) {
                                badd = false;
                            }
                            else {
                                pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                            }
                        }
                        else if (bkill) {
                            if (apActor->IsInCombat() || apActor->bPlayerTeammate) {
                                badd = false;
                            }
                            else if (!pbuyit || !pbuyit->IsPlayerTeammate()) {
                                pObject->eType = ACQUIRE_TYPE_KILL;
                            }
                        }
                    }
                    else if (pObject->eType == ACQUIRE_TYPE_NOOWNED_CONTAINER) {
                        PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
                        pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                        if (!apActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem) && pRef->GetLock()) {
                            badd = false;
                        }
                    }
                    else {
                        pObject->eType = ACQUIRE_TYPE_STEALGROUND;
                    }
                    pObject->bSteal = bsteal;
                    pObject->bKill = bkill;
                }
            }
        }
        else {
            pObject->iWantValue = static_cast<int32_t>(apActor->GetDistanceFromReference(pRef, false, false));
        }
        if (badd) {
            pObject->pRef->SetTargeted(true);
            ObjectList.AddTail(pObject);
        }
        else {
            delete pObject;
        }
        pRefItem = pRefItem->m_pkNext;
    }
    TempObjectList.RemoveAll();
}

void LowProcess::SetBit(bool bVal, uint8_t uMask) {
    if (bVal != 0) {
        m_uFlags |= uMask;
    }
    else {
        m_uFlags &= ~uMask;
    }
}

bool LowProcess::GetBit(uint8_t uMask) {
    return (m_uFlags & uMask) != 0;
}

void LowProcess::SetPackageIdleDone(bool abFlag) {
    SetBit(abFlag, kFlagPackageIdleDone);
}

bool LowProcess::ShouldUseCombatControllerForPackage(TESPackage* apPackage, bool abCheckContinueInCombat) {
    switch (apPackage->GetPackType()) {
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_FOLLOW:
        case PACKAGE_TYPE_TRAVEL:
        case PACKAGE_TYPE_ACCOMPANY:
        case PACKAGE_TYPE_PATROL:
        case PACKAGE_TYPE_GUARD:
        case PACKAGE_TYPE_FIND: {
            uint32_t uiFlags = kPackageFlag_PretendInCombat;
            if (abCheckContinueInCombat) {
                uiFlags |= kPackageFlag_ContinueDuringCombat;
            }
            return (apPackage->GetPackageFlags() & uiFlags) != 0 && GetProcessLevel() <= PROCESS_TYPE_MIDDLE_HIGH;
        }
        default: return false;
    }
}

// Takes over apProcess's low-level state; the acquire objects move (apProcess loses them), the rest is copied.
void LowProcess::LowCopy(LowProcess* apProcess) {
    SetLastTimeProcessed(apProcess->GetLastTimeProcessed());
    apProcess->CurrentPackage.Copy(&CurrentPackage);
    m_uFlags = apProcess->m_uFlags;
    pAcquireObject = apProcess->pAcquireObject;
    apProcess->pAcquireObject = nullptr;
    fCombatDelayTimer = apProcess->fCombatDelayTimer;
    pItemBeingUsed = apProcess->pItemBeingUsed;
    if (apProcess->pSavedAcquireObject) {
        pSavedAcquireObject = apProcess->pSavedAcquireObject;
        apProcess->pSavedAcquireObject = nullptr;
    }
    iDateLastProcessed = apProcess->GetLastDateProcessed();
    BaseProcess* pProcess = this;
    pProcess->SetTarget(apProcess->pTarget);
    iNumberItemsActivate = apProcess->iNumberItemsActivate;
    pDamageModifiers = apProcess->pDamageModifiers;
    fEssentialDownTimer = apProcess->fEssentialDownTimer;
    fDeathTime = apProcess->fDeathTime;
    fTrackedDamage = apProcess->fTrackedDamage;
}

void LowProcess::StartAtPatrolRefBehaviorLow(Actor* apActor, PatrolActorPackageData* apData) {
    SetupNewPackage();
    apData->fCurrentTimeAtReference = 0.0f;
    apData->eState = PatrolActorPackageData::PATROL_STATE_AT_REF;
}

bool LowProcess::UpdatePackageTargetForCombat(Actor* apActor, TESPackage* apPackage) {
    if (apPackage->GetPackType() != PACKAGE_TYPE_PATROL) {
        return false;
    }
    ExtraDataList* pExtra = apActor->TESObjectREFR::GetExtra();
    PatrolActorPackageData* pData = static_cast<PatrolActorPackageData*>(pExtra->GetActorPackageData());
    if (!pData) {
        return false;
    }
    if (pData->eState) {
        if (pData->SetNextReferenceForPatrol(true)) {
            TESObjectREFR* pRef = pData->GetCurrentReference();
            apActor->TESObjectREFR::GetExtra()->SetPackageExtraTarget(pRef);
            return true;
        }
    }
    else if (pData->Init(apActor, apPackage)) {
        pData->eState = PatrolActorPackageData::PATROL_STATE_TRAVELING_TO_NEXT_REF;
        TESObjectREFR* pRef = pData->GetCurrentReference();
        apActor->TESObjectREFR::GetExtra()->SetPackageExtraTarget(pRef);
        return true;
    }
    return false;
}

float LowProcess::GetTimeDelta(Actor* apActor) {
    TESGlobal* pTimeScale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58));
    float fHour = appCalendar.GetHour();
    if (fHour < BaseProcess::GetLastTimeProcessed()) {
        fHour = static_cast<float>(fHour + 24.0);
    }
    float fElapsed = fHour - BaseProcess::GetLastTimeProcessed();
    return static_cast<float>(3600.0 / pTimeScale->TESGlobal::GetValue() * fElapsed);
}

void LowProcess::Patrol_StartTravelToRefLow(Actor* apActor, PatrolActorPackageData* apData) {
    TESObjectREFR* pRef = apData->GetCurrentReference();
    if (pRef) {
        SetTarget(pRef);
        bool bSkipPathing = false;
        if (pPlayer->IsSleepingorResting()) {
            bSkipPathing = true;
        }
        else if (!apActor->Get3D()) {
            TESObjectCELL* pActorCell = apActor->GetParentCell();
            TESObjectCELL* pRefCell = pRef->GetParentCell();
            bSkipPathing = (!pActorCell || !TES::pInstance->IsCellLoaded(pActorCell, false)) &&
                (!pRefCell || !TES::pInstance->IsCellLoaded(pRefCell, false));
        }
        if (!bSkipPathing) {
            float fRadius = PathingRequest::GetDefaultRadius();
            TESPackage* pPackage = GetPackageThatIsRunning();
            if (pPackage) {
                float fLocationRadius = static_cast<float>(pPackage->GetLocationRadius(apActor));
                fRadius = BSMAX(fRadius, fLocationRadius);
            }
            PathingRequest request;
            TESWorldSpace* pWorldSpace = pRef->GetWorldSpace();
            TESObjectCELL* pCell = pRef->GetParentCell();
            const NiPoint3& kLocation = pRef->GetLocationOnReference();
            apActor->BuildRequest(&request, kLocation, pCell, pWorldSpace, fRadius, nullptr);
            request.SetSoftGoalRadius(false);
            TESBoundObject* pObject = pRef->GetObjectReference();
            if (pObject->GetFormType() == IDLM_ID || pObject == pXMarkerHeading) {
                request.SetGoalAngleToFace(pRef->GetAngleOnReference().z);
            }
            apActor->SetPathfindingGoal(&request);
        }
        apData->eState = PatrolActorPackageData::PATROL_STATE_TRAVELING_TO_NEXT_REF;
    }
    else {
        SetProcedureIndexRunning(-1);
    }
}

char LowProcess::Patrol_StartLow(Actor* apActor, PatrolActorPackageData* apData) {
    if (apData->Init(apActor, GetCurrentPackage())) {
        Patrol_StartTravelToRefLow(apActor, apData);
        return 1;
    }

    SetProcedureIndexRunning(0xFFFFFFFFu);
    return 0;
}

void LowProcess::UpdateTarget() {
    if (GetCurrentPackage()) {
        if (GetCurrentPackage()->GetPackageTarget()) {
            if (GetCurrentPackage()->GetPackageTarget()->GetTargReference()) {
                TESObjectREFR* ptargReference = GetCurrentPackage()->GetPackageTarget()->GetTargReference();
                if (!ptargReference->GetDelete() && ptargReference != pTarget) {
                    SetTarget(ptargReference);
                }
            }
        }
    }
}

bool LowProcess::SetCurrentPackageFromSchedule(Actor* apActor) {
    TESPackage* pNewPackage = apActor->GetPackage();
    if (pNewPackage == GetCurrentPackage()) {
        return false;
    }
    SetCurrentPackage(pNewPackage, apActor);
    SetupNewPackage();
    apActor->ClearLookAtTarget();
    if (GetCurrentPackage()) {
        if (!GetCurrentPackage()->pPackLoc || GetCurrentPackage()->pPackLoc->GetLocType() == PLT_CURRLOC) {
            float fAngleZ = apActor->GetAngleOnReference().z;
            const NiPoint3& kLocation = apActor->GetLocationOnReference();
            TESObjectCELL* pCell = apActor->GetParentCell();
            TESWorldSpace* pWorldSpace = apActor->GetWorldSpace();
            apActor->SetPackageStartLocation(pWorldSpace, pCell, kLocation, fAngleZ);
        }
    }
    CombatController* pCombatController = apActor->GetCombatController();
    if (pCombatController) {
        pCombatController->UpdatePackageData(pNewPackage);
    }
    CheckAndStartCombatControllerForPackage(apActor, pNewPackage, false);
    return true;
}

bool LowProcess::ShouldStopCombatControllerToFinishPackage(TESPackage* apPackage, int32_t aiIndex) {
    uint8_t bStop;
    switch (apPackage->GetPackType()) {
        case 0:
        case 1:
        case 6:
        case 7:
        case 13:
        case 14:
        case 17: {
            bStop = 0;
            break;
        }
        default: {
            bStop = apPackage->TESPackage::GetProcedureType() == PACKAGE_PROCEDURE_NONE ||
                Procedure::Action[apPackage->TESPackage::GetProcedureType()][aiIndex] != PROCEDURE_DONE;
            break;
        }
    }
    return bStop;
}

void LowProcess::ProcessKill(Actor* apActor) {
    if (pTarget) {
        if (pTarget->IsDead(false)) {
            ProcessActivate(apActor, false);
        }
        else {
            EnterCombat(
                apActor,
                static_cast<Actor*>(pTarget),
                false,
                false,
                nullptr,
                false,
                true,
                true,
                0,
                false,
                false,
                true,
                nullptr
            );
        }
    }
}

void LowProcess::ClearData() {
    if (pAcquireObject) {
        if (pSavedAcquireObject == pAcquireObject) {
            pSavedAcquireObject = nullptr;
        }
        delete pAcquireObject;
        pAcquireObject = nullptr;
    }
    if (pSavedAcquireObject) {
        delete pSavedAcquireObject;
        pSavedAcquireObject = nullptr;
    }
    while (!ObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj = ObjectList.m_item;
        ObjectList.Remove(pobj);
        delete pobj;
    }
    GenericLocationsList.RemoveAll();
    pGenericLocation = nullptr;
    while (!TempObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj = TempObjectList.m_item;
        TempObjectList.Remove(pobj);
        delete pobj;
    }
    TempLocList.RemoveAll();
    if (pObjecttoAcquire) {
        delete pObjecttoAcquire;
    }
    pObjecttoAcquire = nullptr;
}

void LowProcess::BuildMulitTargetObjectList(Actor* apActor, TESObjectREFR* pref, PACKAGE_OBJECT_TYPE eType) {
    TESContainer* pCont = nullptr;
    if (pref) {
        pCont = pref->HasContainer();
    }
    InventoryChanges* pchange = nullptr;
    if (pCont) {
        pchange = InventoryChanges::GetInventoryChanges(pref);
    }
    if (pchange) {
        switch (eType) {
            case POTYPE_WEAP: {
                int32_t icount = pchange->GetCount(true);
                ItemChange* pitem = nullptr;
                for (int32_t i = 0; i < icount; ++i) {
                    pitem = pchange->GetItembyIndex(i);
                    if (pitem) {
                        if (dynamic_cast<TESObjectWEAP*>(pitem->pContainerObj)) {
                            ObjectstoAcquire* pacquire = new ObjectstoAcquire;
                            pacquire->pObject = pitem->pContainerObj;
                            pacquire->iamount = pitem->iNumber;
                            pacquire->pRef = pref;
                            pacquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire);
                        }
                        if (pitem) {
                            delete pitem;
                        }
                        pitem = nullptr;
                    }
                }
                break;
            }
            case POTYPE_COMBATWEAR: {
                int32_t icount = pchange->GetCount(true);
                ItemChange* pitem = nullptr;
                for (int32_t i = 0; i < icount; ++i) {
                    pitem = pchange->GetItembyIndex(i);
                    if (pitem) {
                        TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(pitem->pContainerObj);
                        TESObjectWEAP* pweap = dynamic_cast<TESObjectWEAP*>(pitem->pContainerObj);
                        TESAmmo* pammo = dynamic_cast<TESAmmo*>(pitem->pContainerObj);
                        if (parmo || pweap || pammo) {
                            ObjectstoAcquire* pacquire = new ObjectstoAcquire;
                            pacquire->pObject = pitem->pContainerObj;
                            pacquire->iamount = pitem->iNumber;
                            pacquire->pRef = pref;
                            pacquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire);
                        }
                        if (pitem) {
                            delete pitem;
                        }
                        pitem = nullptr;
                    }
                }
                break;
            }
            case POTYPE_WEAR: {
                int32_t icount = pchange->GetCount(true);
                ItemChange* pitem = nullptr;
                for (int32_t i = 0; i < icount; ++i) {
                    pitem = pchange->GetItembyIndex(i);
                    if (pitem) {
                        TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(pitem->pContainerObj);
                        TESObjectWEAP* pweap = dynamic_cast<TESObjectWEAP*>(pitem->pContainerObj);
                        TESAmmo* pammo = dynamic_cast<TESAmmo*>(pitem->pContainerObj);
                        TESObjectCLOT* pcloth = dynamic_cast<TESObjectCLOT*>(pitem->pContainerObj);
                        if (parmo || pweap || pammo || pcloth) {
                            ObjectstoAcquire* pacquire = new ObjectstoAcquire;
                            pacquire->pObject = pitem->pContainerObj;
                            pacquire->iamount = pitem->iNumber;
                            pacquire->pRef = pref;
                            pacquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire);
                        }
                        if (pitem) {
                            delete pitem;
                        }
                        pitem = nullptr;
                    }
                }
                break;
            }
            case POTYPE_WEAP_MELEE: {
                int32_t icount = pchange->GetCount(true);
                ItemChange* pitem = nullptr;
                for (int32_t i = 0; i < icount; ++i) {
                    pitem = pchange->GetItembyIndex(i);
                    if (pitem) {
                        TESObjectWEAP* pweap = dynamic_cast<TESObjectWEAP*>(pitem->pContainerObj);
                        if (pweap && pweap->IsMeleeWeapon()) {
                            ObjectstoAcquire* pacquire = new ObjectstoAcquire;
                            pacquire->pObject = pitem->pContainerObj;
                            pacquire->iamount = pitem->iNumber;
                            pacquire->pRef = pref;
                            pacquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire);
                        }

                        if (pitem) {
                            delete pitem;
                        }
                        pitem = nullptr;
                    }
                }
                break;
            }
            case POTYPE_WEAP_RANGED: {
                int32_t icount = pchange->GetCount(true);
                ItemChange* pitem = nullptr;
                for (int32_t i = 0; i < icount; ++i) {
                    pitem = pchange->GetItembyIndex(i);
                    if (pitem) {
                        TESObjectWEAP* pweap = dynamic_cast<TESObjectWEAP*>(pitem->pContainerObj);
                        if (pweap && pweap->IsRangedWeapon()) {
                            ObjectstoAcquire* pacquire = new ObjectstoAcquire;
                            pacquire->pObject = pitem->pContainerObj;
                            pacquire->iamount = pitem->iNumber;
                            pacquire->pRef = pref;
                            pacquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire);
                        }

                        if (pitem) {
                            delete pitem;
                        }
                        pitem = nullptr;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    else {
        TESObjectCELL* pCell = apActor->GetParentCell();
        NiPoint3 pointB = apActor->GetLocationOnReference();
        eFormType = eType;
        float facquireDist = 0.0f;
        TESPackage* pPack = GetPackageThatIsRunning();
        if (pCell && GetCurrentPackage()->GetPackageSearchLocation()) {
            uint32_t iradius = pPack->GetSearchLocationRadius(apActor);
            pCell = pPack->GetSearchLocationCell(apActor);
            pointB = pPack->GetSearchLocationCoord(apActor);
            if (pCell) {
                float fsearchRadius;
                if (iradius) {
                    fsearchRadius = static_cast<float>(iradius);
                }
                else if (pCell->GetInterior()) {
                    fsearchRadius = std::numeric_limits<float>::max();
                }
                else {
                    fsearchRadius = fAIAcquireObjectDistance.Float();
                }
                facquireDist = fsearchRadius;
            }
            else {
                Warning("Package on %s is using an invalid Search Location\n", apActor->GetFullName());
            }
            pDataHandler->EnumReferencesCloseToPoint(
                pCell,
                pointB,
                facquireDist,
                pointB,
                facquireDist,
                &LowProcess::EvaluateReferenceforAcquire,
                apActor
            );
        }
        else {
            if (!GetCurrentPackage() || !GetCurrentPackage()->GetAcquireRadius(apActor, facquireDist)) {
                facquireDist = fAIAcquireObjectDistance.Float();
            }
            float fRadius2 = facquireDist;
            const NiPoint3& kLocation = apActor->GetLocationOnReference();
            pDataHandler->EnumReferencesCloseToPoint(
                pCell,
                pointB,
                facquireDist,
                kLocation,
                fRadius2,
                &LowProcess::EvaluateReferenceforAcquire,
                apActor
            );
        }
        eFormType = POTYPE_NONE;
        pObjecttoAcquire = nullptr;
        pFactiontoAquire = nullptr;
        EvaluateOrderAcquireList(apActor);
    }
}

double LowProcess::Patrol_TravelingToNextRefLow(Actor* apActor, PatrolActorPackageData* apData, float afTimeRemaining) {
    TESObjectREFR* pRef = apData->GetCurrentReference();
    if (pRef) {
        float fTimeLeft = afTimeRemaining;
        char cTeleport = 0;
        TESObjectCELL* pActorCell = apActor->GetParentCell();
        TESObjectCELL* pRefCell = pRef->GetParentCell();
        bool bSameCell = pActorCell && pActorCell == pRefCell;
        if (pPlayer->IsSleepingorResting()) {
            cTeleport = 1;
        }
        else {
            cTeleport = (!pActorCell || !TES::pInstance->IsCellLoaded(pActorCell, false)) &&
                (!pRefCell || !TES::pInstance->IsCellLoaded(pRefCell, false));
        }
        if (cTeleport || !apActor->DidPathingFail()) {
            int32_t iMoveMode = 257;
            TESPackage* pPackage = GetPackageThatIsRunning();
            bool bAlwaysRun = pPackage->GetAlwaysRun();
            bool bCheckAlwaysRun = false;
            if (bAlwaysRun) {
                float fDistance = apActor->GetDistanceFromReference(pRef, false, false);
                bCheckAlwaysRun = 64.0f < static_cast<double>(fDistance);
                if (64.0f < static_cast<double>(fDistance)) {
                    float fRadius = pPackage->GetPackageRadiusActorToLocation(apActor, false);
                    float fDoubleRadius = fRadius + fRadius;
                    iMoveMode = CalculateMoveMode(apActor, fDistance, fRadius, fDoubleRadius, false, false);
                }
            }
            SetActorsAnimation(apActor, iMoveMode, bCheckAlwaysRun);
            if (cTeleport) {
                float fPathLength;
                float fSpeed = apActor->GetCurrentSpeed();
                fSpeed = BSMAX(fSpeed, static_cast<float>(1.0));
                if (bSameCell) {
                    NiPoint3 kOffset = pRef->GetLocationOnReference();
                    kOffset -= apActor->GetLocationOnReference();
                    fPathLength = kOffset.Length();
                }
                else {
                    PathingLocation aOrigin(apActor);
                    PathingLocation aDestination(pRef);
                    fPathLength = Pathing::ComputeTeleportDoorPathLength(
                        aOrigin,
                        aDestination,
                        4096.0f,
                        nullptr,
                        Pathing::DOOR_LOCK_DISPOSITION_NORMAL
                    );
                }
                fTimeLeft = fTimeLeft - fPathLength / fSpeed;
                apActor->MoveActorAndFollowers(pRef, true, false);
                StartAtPatrolRefBehaviorLow(apActor, apData);
            }
            else if (apActor->IsPathingComplete()) {
                StartAtPatrolRefBehaviorLow(apActor, apData);
            }
            else {
                return static_cast<float>(0.0);
            }
            return fTimeLeft;
        }

        Patrol_StartTravelToRefLow(apActor, apData);
        return fTimeLeft;
    }

    SetProcedureIndexRunning(-1);
    return 0.0;
}

double LowProcess::Patrol_AtRefLow(Actor* apActor, PatrolActorPackageData* apData, float afTimeRemaining) {
    TESObjectREFR* pRef = apData->GetCurrentReference();
    if (!pRef) {
        SetProcedureIndexRunning(-1);
        return 0.0;
    }
    float fTimeAtRef = 0.0;
    if (pRef->GetExtra()->GetPatrolRefData()) {
        fTimeAtRef = pRef->GetExtra()->GetPatrolRefData()->GetTimeAtRef();
    }
    float fTimeLeftAtRef = fTimeAtRef - apData->fCurrentTimeAtReference;
    fTimeAtRef = BSMAX(static_cast<float>(0.0), fTimeLeftAtRef);
    float fTimeLeft = afTimeRemaining - fTimeAtRef;
    if (fTimeLeft > 0.0) {
        if (!apData->SetNextReferenceForPatrol(true)) {
            AddToProcedureIndexRunning(apActor, 1);
            return 0.0;
        }
        if (apData->GetCurrentReference() == pRef) {
            fTimeLeft = 0.0;
        }
        else {
            NiPoint3 kOffset = apData->GetCurrentReference()->GetLocationOnReference();
            kOffset -= pRef->GetLocationOnReference();
            if (kOffset.SqrLength() < 0.0010000000474974513) {
                fTimeLeft = 0.0;
            }
        }
        Patrol_StartTravelToRefLow(apActor, apData);
    }
    else {
        apData->fCurrentTimeAtReference = apData->fCurrentTimeAtReference + afTimeRemaining;
    }
    return fTimeLeft;
}

// Locks apRef when it is a locked door the living actor aiFilterData owns; through a teleport door into a worldspace,
// or from an exterior (or no) cell, faction ownership counts too. The connected cells stop being public.
bool LowProcess::LockDoorsatLocation(TESObjectREFR* apRef, void* aiFilterData) {
    Actor* pActor = static_cast<Actor*>(aiFilterData);
    if (!pActor || pActor->IsDead(false) || !apRef) {
        return false;
    }
    if (apRef->GetObjectReference()->GetFormType() != DOOR_ID) {
        return false;
    }
    bool bCheckFaction = false;
    DoorTeleportData* pTeleport = apRef->GetTeleport();
    if (pTeleport &&
        (pTeleport->GetTeleportWorldSpace() || !pActor->GetParentCell() || !pActor->GetParentCell()->GetInterior())) {
        bCheckFaction = true;
    }
    if (!apRef->IsAnOwner(pActor, bCheckFaction)) {
        return false;
    }
    if (!apRef->GetLock()) {
        return false;
    }
    apRef->Lock();
    DoorTeleportData* pDoorTeleport = apRef->GetTeleport();
    if (pDoorTeleport) {
        pDoorTeleport->SetConnectedCellsPublic(apRef, false);
    }
    return false;
}

// Unlocks apRef when it is a locked door the living actor aiFilterData owns (faction ownership counts for a teleport
// door); the connected cells become public.
bool LowProcess::UnLockDoorsatLocation(TESObjectREFR* apRef, void* aiFilterData) {
    Actor* pActor = static_cast<Actor*>(aiFilterData);
    if (!pActor || pActor->IsDead(false) || !apRef) {
        return false;
    }
    if (apRef->GetObjectReference()->GetFormType() != DOOR_ID) {
        return false;
    }
    bool bCheckFaction = false;
    DoorTeleportData* pTeleport = apRef->GetTeleport();
    if (pTeleport) {
        bCheckFaction = true;
    }
    if (!apRef->IsAnOwner(pActor, bCheckFaction)) {
        return false;
    }
    if (!apRef->GetLock()) {
        return false;
    }
    apRef->UnLock();
    if (pTeleport) {
        pTeleport->SetConnectedCellsPublic(apRef, true);
    }
    return false;
}

void LowProcess::SetField(uint8_t auiValue, char acMask, char acShift) {
    m_uFlags = static_cast<uint8_t>((auiValue << acShift) | (~acMask & m_uFlags));
}

bool LowProcess::EvaluateReferenceforAcquire(TESObjectREFR* apRef, void* aiFilterData) {
    // The filter data is the searching actor.
    Actor* pAcquirer = static_cast<Actor*>(aiFilterData);
    if (!apRef || apRef == pAcquirer || apRef->GetDelete() || apRef->GetTemporary() || apRef->GetDisabled()) {
        return false;
    }
    if (!pAcquirer) {
        return true;
    }
    ObjectstoAcquire* pAcquire = nullptr;
    bool bAcquire = false;
    HighProcess* pProcess = nullptr;
    bool bIsActor = apRef->IsActor();
    TESForm* pForm = apRef->GetObjectReference();
    int32_t iFormType = pForm->GetFormType();
    if (pAcquirer->GetCurrentAIProcess()) {
        // The level is tested signed against both ends.
        PROCESS_TYPE eLevel = pAcquirer->GetCurrentAIProcess()->GetProcessLevel();
        if (eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_LOW) {
            pProcess = static_cast<HighProcess*>(pAcquirer->GetCurrentAIProcess());
        }
    }
    if (!pProcess) {
        return true;
    }
    if (pProcess->eFormType && TESPackage::FormMatchesPackageObjectType(pForm, pProcess->eFormType)) {
        if (pProcess->pFactiontoAquire && bIsActor) {
            if (static_cast<Actor*>(apRef)->Actor::IsInFaction(pProcess->pFactiontoAquire)) {
                bAcquire = true;
            }
        }
        else {
            bAcquire = true;
        }
    }
    else if (pProcess->pObjecttoAcquire) {
        if (pProcess->pObjecttoAcquire == pForm) {
            bAcquire = true;
        }
        else if (bIsActor && apRef->m_Extra.GetLevCreaOriginalBase() == pProcess->pObjecttoAcquire) {
            bAcquire = true;
        }
    }
    else if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjecttoAcquire) {
        bAcquire = true;
    }
    if (iFormType == 25) {
        if (static_cast<TESObjectBOOK*>(pForm)->TESObjectBOOK::GetCantTake()) {
            bAcquire = false;
        }
    }
    else if (iFormType == 30 && !static_cast<TESObjectLIGH*>(pForm)->TESObjectLIGH::GetCanCarry()) {
        bAcquire = false;
    }
    char cInCombat = 0;
    if (iFormType == 40) {
        // The weapon's TESHealthForm base, null-tested as a base cast.
        TESHealthForm* pHealthForm = static_cast<TESObjectWEAP*>(pForm);
        if (pAcquirer->IsInCombat()) {
            cInCombat = 1;
        }
        if (pHealthForm && apRef->GetHealth() <= 0.0 && cInCombat) {
            bAcquire = false;
        }
    }
    if (bAcquire) {
        pAcquire = new ObjectstoAcquire;
        pAcquire->pObject = apRef->GetObjectReference();
        pAcquire->pRef = apRef;
        pAcquire->iamount = apRef->m_Extra.GetCount();
        pAcquire->eType = ACQUIRE_TYPE_NOOWNED_GROUND;
        pAcquire->pExtra = &apRef->m_Extra;
        pProcess->TempObjectList.AddTail(pAcquire);
    }
    bool bValidItem = false;
    switch (pProcess->eFormType) {
        case POTYPE_NONE:
        case POTYPE_ARMO:
        case POTYPE_BOOK:
        case POTYPE_CLOT:
        case POTYPE_INGR:
        case POTYPE_LIGH:
        case POTYPE_MISC:
        case POTYPE_FLOR:
        case POTYPE_WEAP:
        case POTYPE_AMMO:
        case POTYPE_KEYM:
        case POTYPE_ALCH:
        case POTYPE_FOOD:
        case POTYPE_COMBATWEAR:
        case POTYPE_WEAR:
        case POTYPE_WEAP_MELEE:
        case POTYPE_WEAP_RANGED: {
            bValidItem = true;
            break;
        }
        default: {
            break;
        }
    }
    if ((bValidItem && bIsActor && apRef != pPlayer) || iFormType == 27) {
        InventoryChanges* pChanges = apRef->m_Extra.GetContainerChanges();
        if (pChanges) {
            BSSimpleList<ContainerObject*>* plist = nullptr;
            if (pChanges->GetOriginalContainer()) {
                plist = &pChanges->GetOriginalContainer()->objectList;
            }
            while (plist && plist->m_item) {
                char cMatch = 0;
                ContainerObject* pContObj = plist->m_item;
                if (pContObj) {
                    ItemChange* pItem = pChanges->GetInventoryItem(pContObj->pObj, 0);
                    if (pItem) {
                        delete pItem;
                    }
                    else {
                        if (pProcess->eFormType &&
                            TESPackage::FormMatchesPackageObjectType(pContObj->pObj, pProcess->eFormType)) {
                            cMatch = 1;
                        }
                        else if (pProcess->pObjecttoAcquire && pProcess->pObjecttoAcquire == pContObj->pObj) {
                            cMatch = 1;
                        }
                        else if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjecttoAcquire) {
                            cMatch = 1;
                        }
                        if (cMatch) {
                            TESForm* pSpace = apRef->GetSpace();
                            if (pSpace == pAcquirer->GetSpace()) {
                                pAcquire = new ObjectstoAcquire;
                                pAcquire->pObject = pContObj->pObj;
                                pAcquire->pRef = apRef;
                                pAcquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                                if (apRef->HasContainer()) {
                                    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(apRef);
                                    if (pInventory) {
                                        pAcquire->iamount = pInventory->GetObjectCount(pContObj->pObj);
                                    }
                                }
                                else {
                                    pAcquire->iamount = 1;
                                }
                                pProcess->TempObjectList.AddTail(pAcquire);
                            }
                        }
                    }
                }
                plist = plist->m_pkNext;
            }
        }
        int32_t iInventoryCount = apRef->GetInventoryCount(false, true);
        for (int32_t iIndex = 0; iIndex < iInventoryCount; ++iIndex) {
            ItemChange* pInvItem = apRef->GetInventoryItem(iIndex, false);
            if (pInvItem) {
                if (!pInvItem->GetWorn(false)) {
                    char cMatch = 0;
                    if (pProcess->eFormType &&
                        TESPackage::FormMatchesPackageObjectType(pInvItem->pContainerObj, pProcess->eFormType)) {
                        cMatch = 1;
                    }
                    else if (pProcess->pObjecttoAcquire && pProcess->pObjecttoAcquire == pInvItem->pContainerObj) {
                        cMatch = 1;
                    }
                    else if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjecttoAcquire) {
                        cMatch = 1;
                    }
                    if (cMatch) {
                        TESForm* pSpace = apRef->GetSpace();
                        if (pSpace == pAcquirer->GetSpace()) {
                            pAcquire = new ObjectstoAcquire;
                            pAcquire->pObject = pInvItem->pContainerObj;
                            pAcquire->pRef = apRef;
                            pAcquire->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            if (apRef->HasContainer()) {
                                InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(apRef);
                                if (pInventory) {
                                    pAcquire->iamount = pInventory->GetObjectCount(pAcquire->pObject);
                                }
                            }
                            else {
                                pAcquire->iamount = 1;
                            }
                            pProcess->TempObjectList.AddTail(pAcquire);
                            if (pInvItem->pExtraObjectList) {
                                pAcquire->pExtra = pInvItem->pExtraObjectList->m_item;
                            }
                        }
                    }
                }
                delete pInvItem;
            }
        }
    }
    return false;
}

// aiFilterData is the searching actor.
bool EvaluateGenericLocation(TESObjectREFR* apRef, void* aiFilterData) {
    Actor* pActor = static_cast<Actor*>(aiFilterData);
    if (!apRef ||
        apRef->GetDelete() ||
        apRef->GetTemporary() ||
        apRef->GetDisabled() ||
        (pActor->IsChild(false) && !pActor->CanChildUseObject(apRef))) {
        return false;
    }
    if (!pActor) {
        return true;
    }
    LowProcess* pProcess = dynamic_cast<LowProcess*>(pActor->pCurrentProcess);
    if (!pProcess) {
        return true;
    }
    bool bIsActor = apRef->IsActor();
    int32_t iFormType = apRef->GetObjectReference()->GetFormType();
    if (pProcess->eFormType &&
        TESPackage::FormMatchesPackageObjectType(apRef->GetObjectReference(), pProcess->eFormType)) {
        if (!apRef->IsDead(false)) {
            apRef->SetTargeted(true);
            pProcess->TempLocList.AddTail(apRef);
        }
    }
    else if (pProcess->pObjectForLocation && pProcess->pObjectForLocation == apRef->GetObjectReference()) {
        if (!apRef->IsDead(false)) {
            apRef->SetTargeted(true);
            pProcess->TempLocList.AddTail(apRef);
        }
    }
    else if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjectForLocation) {
        apRef->SetTargeted(true);
        pProcess->TempLocList.AddTail(apRef);
    }
    if ((bIsActor && apRef != pPlayer) || iFormType == 27) {
        int32_t iInventoryCount = apRef->GetInventoryCount(false, true);
        for (int32_t iIndex = 0; iIndex < iInventoryCount; ++iIndex) {
            ItemChange* pInvItem = apRef->GetInventoryItem(iIndex, false);
            if (pInvItem) {
                if (!pInvItem->GetWorn(false)) {
                    char cMatch = 0;
                    if (pProcess->eFormType &&
                        TESPackage::FormMatchesPackageObjectType(pInvItem->pContainerObj, pProcess->eFormType)) {
                        cMatch = 1;
                    }
                    else if (pProcess->pObjectForLocation == pInvItem->pContainerObj) {
                        cMatch = 1;
                    }
                    else if (
                        pProcess->eFormType == POTYPE_NONE &&
                        !pProcess->pObjecttoAcquire &&
                        !pProcess->pObjectForLocation
                    ) {
                        cMatch = 1;
                    }
                    if (cMatch) {
                        apRef->SetTargeted(true);
                        pProcess->TempLocList.AddTail(apRef);
                    }
                }
                delete pInvItem;
            }
        }
    }
    return false;
}

float PatrolRefData::GetTimeAtRef() {
    return fTimeAtRef;
}

CombatTimer::CombatTimer() : fStartTime(0.0f), fTargetTime(0.0f) {}

void CombatTimer::Clear() {
    fTargetTime = 0.0f;
}
