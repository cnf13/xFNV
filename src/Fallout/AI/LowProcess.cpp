#include "Fallout/AI/LowProcess.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
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

// Each low process's lock owner thread and count (37 buckets).
using LowProcessLockTable = NiTMap<LowProcess*, LP_LOCK_DATA>;
static LowProcessLockTable LowProcessLockMap(37);
static BSCriticalSection LowProcCritSection;

static BSPerformanceTimer LowUpdateTimer("AIProcessing", "LowUpdate", true, true);

uint32_t* LP_LOCK_DATA::Clear(int32_t a1) {
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
    LowProcess_calcByt030(this, nullptr, 0, 255, 0);
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
// actor back to its location, as the game does. The game hour it reads first and the travel time delta are stored and
// never read.
void LowProcess::Update(MobileObject* apRef) {
    LowUpdateTimer.Start();
    if (!apRef) {
        return;
    }
    [[maybe_unused]] float fHour = appCalendar.GetHour();
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
    if (GetCurrentPackage() && GetCurrentPackage()->GetProcedureType() != PACKAGE_PROCEDURE_NONE &&
        !GetCurrentPackage()->GetWaiting()) {
        UpdateTarget();
        TESObjectREFR* pGenericLoc = nullptr;
        PACKAGE_PROCEDURE_TYPE eType = GetCurrentPackage()->GetProcedureType();
        uint32_t uiIndex = GetProcedureIndexRunning();
        switch (Procedure_Action[eType][uiIndex]) {
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
                                pParentCell, *pPoint1, 5000.0f, *pPoint2, 5000.0f, &LowProcess::LockDoorsatLocation,
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
                            pParentCell, *pPoint1, 5000.0f, *pPoint2, 5000.0f, &LowProcess::UnLockDoorsatLocation,
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
                [[maybe_unused]] float fTimeDelta = GetTimeDelta(pActor);
                if (pActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_LOW &&
                    GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false)) {
                    if (!GetLockedLocation()) {
                        if (GetCurrentPackage()->GetLockDoorsAtLocation()) {
                            const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                            const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                            TESObjectCELL* pParentCell = pActor->GetParentCell();
                            pDataHandler->EnumReferencesCloseToPoint(
                                pParentCell, *pPoint1, 5000.0f, *pPoint2, 5000.0f, &LowProcess::LockDoorsatLocation,
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
                            pParentCell, *pPoint1, 5000.0f, *pPoint2, 5000.0f, &LowProcess::UnLockDoorsatLocation,
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
                    const char* pPackName =
                        NavMeshInfo::GetCellOrCoords(const_cast<char*>(packageTypeToString[ePackType]), 0);
                    const char* pFullName = pActor->GetFullName();
                    std::sprintf(acText, "%s is sleeping with %s", pFullName, pPackName);
                    Interface::PrintLine<>(acText);
                }
                break;
            case PROCEDURE_EAT:
                if (Interface::GetConsoleSelection() == pActor) {
                    char acText[0x130];
                    PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                    const char* pPackName =
                        NavMeshInfo::GetCellOrCoords(const_cast<char*>(packageTypeToString[ePackType]), 0);
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
                // Tests the package read before the package change check, not the current one, as the game does.
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
                        // The wait distance is worked out and never used, as the game does.
                        float fWaitDistance = 0.0f;
                        int32_t iValue = pCurrPackage->GetPackageTarget()->iValue;
                        if (iValue <= 0) {
                            iValue = 200;
                        }
                        if (fWaitDistance == 0.0 && pActor->GetParentCell() && pActor->GetParentCell()->GetInterior()) {
                            fWaitDistance = fAIEscortWaitDistanceInterior.Float();
                        }
                        else if (fWaitDistance == 0.0) {
                            fWaitDistance =
                                static_cast<float>(fAIEscortWaitDistanceExterior.Float() * static_cast<double>(iValue));
                        }
                        (void)fWaitDistance;
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
            if (Procedure_Action[eDoneType][uiDoneIndex] == PROCEDURE_DONE) {
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
                    const char* pPackName =
                        NavMeshInfo::GetCellOrCoords(const_cast<char*>(packageTypeToString[ePackType]), 0);
                    const char* pFullName = pActor->GetFullName();
                    std::sprintf(acText, "%s is done with %s", pFullName, pPackName);
                    Interface::PrintLine<>(acText);
                }
                ObjectstoAcquire* pAcquire = pAcquireObject;
                if (pAcquire) {
                    pAcquire->Delete(1);
                }
                pAcquireObject = nullptr;
                while (!ObjectList.IsEmpty()) {
                    ObjectstoAcquire* pObject = ObjectList.GetItem();
                    if (pObject) {
                        pObject->Delete(1);
                    }
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
// stops on an empty node, the loop's node is deleted again after the list itself is gone, as the game does.
void LowProcess::UpdateMissedPackages(MobileObject* apRef, bool abActionflag) {
    Actor* pActor = dynamic_cast<Actor*>(apRef);
    TESPackage* pCurrent = GetCurrentPackage();
    if ((pActor && !pCurrent) ||
        (!pCurrent->GetMustComplete() && !pCurrent->IsScriptPackage() && !pCurrent->GetMustReachLocation())) {
        void* pMemory = ourMalloc(sizeof(BSSimpleList<TESPackage*>));
        BSSimpleList<TESPackage*>* pNew;
        if (pMemory) {
            pNew = ::new (pMemory) BSSimpleList<TESPackage*>;
        }
        else {
            pNew = nullptr;
        }
        BSSimpleList<TESPackage*>* pList = pNew;
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
                            pParentCell, *pPoint1, 5000.0f, *pPoint2, 5000.0f, &LowProcess::LockDoorsatLocation, pActor
                        );
                    }
                    else if (pPack->GetUnlockDoorsAtEnd()) {
                        const NiPoint3* pPoint2 = &pActor->GetLocationOnReference();
                        const NiPoint3* pPoint1 = &pActor->GetLocationOnReference();
                        TESObjectCELL* pParentCell = pActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            pParentCell, *pPoint1, 5000.0f, *pPoint2, 5000.0f, &LowProcess::UnLockDoorsatLocation,
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
                    pActor->SetAngleOnReferenceX(3.4028234663852886e+38f);
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
    NiPoint3 result;
    if (GetCurrentPackage()) {
        TESPackage* v3 = GetCurrentPackage();
        result = v3->GetLocationCoord(apActor);
    }
    else {
        result = NiPoint3_ZERO;
    }
    return result;
}

bool LowProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
    float fHour = appCalendar.GetHour();
    TESPackage* ppack = GetCurrentPackage();
    if (!abFlag && (apActor->IsAlarmed() || apActor->IsInCombat())) {
        return false;
    }
    TESPackage* pcurrentPack = nullptr;
    if (GetCurrentPackage()) {
        TESPackage* v5;
        TESPackage* v4 = GetCurrentPackage();
        if (v4->GetLockDoorsAtEnd() || (v5 = GetCurrentPackage(), v5->GetUnlockDoorsAtEnd())) {
            pcurrentPack = GetCurrentPackage();
        }
    }
    bool bflag = false;
    if (!ppack) {
        SetCurrentPackageFromSchedule(apActor);
        bflag = GetCurrentPackage() != nullptr;
        ppack = GetCurrentPackage();
        SetTarget(nullptr);
        SetTargetActivated(false);
    LABEL_79:;
        if (bflag) {
            if ((ppack) && (GetCurrentPackage() != ppack)) {
                if (ppack->GetLockDoorsAtEnd()) {
                    const NiPoint3* v30 = &apActor->GetLocationOnReference();
                    const NiPoint3* v26 = &apActor->GetLocationOnReference();
                    TESObjectCELL* ParentCell = apActor->GetParentCell();
                    pDataHandler->EnumReferencesCloseToPoint(
                        ParentCell, *v26, 5000.0f, *v30, 5000.0f, &LowProcess::LockDoorsatLocation, apActor
                    );
                }
                else {
                    if (ppack->GetUnlockDoorsAtEnd()) {
                        const NiPoint3* v31 = &apActor->GetLocationOnReference();
                        const NiPoint3* v27 = &apActor->GetLocationOnReference();
                        TESObjectCELL* v23 = apActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            v23, *v27, 5000.0f, *v31, 5000.0f, &LowProcess::UnLockDoorsatLocation, apActor
                        );
                    }
                }
            }

            if ((GetCurrentPackage()) && (GetCurrentPackage() != ppack)) {
                if (GetCurrentPackage()->GetLockDoorsAtStart()) {
                    const NiPoint3* v32 = &apActor->GetLocationOnReference();
                    const NiPoint3* v28 = &apActor->GetLocationOnReference();
                    TESObjectCELL* v24 = apActor->GetParentCell();
                    pDataHandler->EnumReferencesCloseToPoint(
                        v24, *v28, 5000.0f, *v32, 5000.0f, &LowProcess::LockDoorsatLocation, apActor
                    );
                }
                else {
                    TESPackage* v15 = GetCurrentPackage();
                    if (v15->GetUnlockDoorsAtStart()) {
                        const NiPoint3* v33 = &apActor->GetLocationOnReference();
                        const NiPoint3* v29 = &apActor->GetLocationOnReference();
                        TESObjectCELL* v25 = apActor->GetParentCell();
                        pDataHandler->EnumReferencesCloseToPoint(
                            v25, *v29, 5000.0f, *v33, 5000.0f, &LowProcess::UnLockDoorsatLocation, apActor
                        );
                    }
                }
            }

            if ((GetCurrentPackage()) && (ppack != GetCurrentPackage() || !apActor->GetCurrentTarget())) {
                TESPackage* v16 = GetCurrentPackage();
                if (v16->IsTargetAnActor(apActor)) {
                    TESPackage* v17 = GetCurrentPackage();
                    if (v17->GetPackType()) {
                        TESPackage* v18 = GetCurrentPackage();
                        TESObjectREFR* EscortTargetForPackage = v18->GetEscortTargetForPackage(apActor, false);
                        SetTarget(EscortTargetForPackage);
                    }
                }
            }

            apActor->UnlockLockDoorsProcedure();
            SetLockedLocation(false);
            float z = apActor->GetAngleOnReference().z;
            const NiPoint3* v36 = &apActor->GetLocationOnReference();
            TESObjectCELL* v34 = apActor->GetParentCell();
            TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
            apActor->SetPackageStartLocation(WorldSpace, v34, *v36, z);
            SetContinuingPackage(false);
            SetTarget(nullptr);
            TESPackage* ppack_2 = GetRunOncePackage();
            if ((ppack_2) && (ppack_2->GetPackType() != PACKAGE_TYPE_GET_UP)) {
                ClearRunOncePackage();
            }

            SetCurrentActionComplete(false);
            SetPackageIdleDone(false);
            SetTargetActivated(false);
            while (!ObjectList.IsEmpty()) {
                ObjectstoAcquire* pobj = (&ObjectList)->m_item;
                if (pobj) {
                    pobj->Delete(1u);
                }
                ObjectList.Remove(pobj);
            }
        }
        if (bflag) {
            TESPackage* v21 = GetCurrentPackage();
            StartNewPackage(apActor, v21);
            SetPackageDoneOnce(false);
        }
        return bflag;
    }
    if (ppack->GetPackageSchedule()->PSData.iDuration && ppack->GetMustComplete()) {
        uint32_t Count;
        int32_t v45 = 0;
        bool bcompleteflag = false;
        bool bnonending = false;
        BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
        if (CurrentAIProcess->GetCurrentActionComplete() ||
            (Count = ppack->ePROCEDURE_TYPE, Procedure_Action[Count][GetProcedureIndexRunning()] == PROCEDURE_DONE)) {
            bcompleteflag = true;
        }
        else {
            if (ppack->GetPackType() == PACKAGE_TYPE_WANDER ||
                (ppack->GetPackType() == PACKAGE_TYPE_SLEEP ||
                 (ppack->GetPackType() == PACKAGE_TYPE_EAT || ppack->GetPackType() == PACKAGE_TYPE_SANDBOX))) {
                bcompleteflag = true;
                bnonending = true;
            }
        }
        if (bcompleteflag) {
            int32_t PackageStartTime;
            if (ppack->GetPackageSchedule()->GetStartTime() == -1) {
                PackageStartTime = static_cast<int32_t>(CurrentPackage.GetPackageStartTime());
            }
            else {
                PackageStartTime = ppack->GetPackageSchedule()->GetStartTime();
            }
            int32_t iTimedone = ppack->GetPackageSchedule()->PSData.iDuration + PackageStartTime;
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
                    char Day = appCalendar.GetDay();
                    apActor->SetRunOnceExtraPackage(ppack, Day);
                }
            }
        }
    }
    else {
        if (ppack->GetMustComplete()) {
            uint32_t v9;
            BaseProcess* v38 = apActor->pCurrentProcess;
            if (v38->GetCurrentActionComplete() ||
                (ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE &&
                 (v9 = ppack->ePROCEDURE_TYPE, Procedure_Action[v9][GetProcedureIndexRunning()] == PROCEDURE_DONE))) {
                if (ppack->GetOncePerDay()) {
                    char v10 = appCalendar.GetDay();
                    apActor->SetRunOnceExtraPackage(ppack, v10);
                }
                bflag = true;
            }
            else {
                bflag = false;
                abFlag = false;
            }
        }
        else {
            if (!ppack->GetMustReachLocation() || apActor->GetEVPBuffered()) {
                bflag = true;
            }
            else {
                uint32_t v11;
                if (ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE &&
                    (ppack->IsActorAtLocation(apActor, false, -1.0f, false) ||
                     (v11 = ppack->ePROCEDURE_TYPE,
                      Procedure_Action[v11][GetProcedureIndexRunning()] == PROCEDURE_DONE))) {
                    uint32_t v12 = ppack->ePROCEDURE_TYPE;
                    if (Procedure_Action[v12][GetProcedureIndexRunning()] == PROCEDURE_DONE && ppack->GetOncePerDay()) {
                        char v13 = appCalendar.GetDay();
                        apActor->SetRunOnceExtraPackage(ppack, v13);
                    }
                    bflag = true;
                }
                else {
                    abFlag = false;
                }
            }
        }
    }
    if (!bflag && (!abFlag || ppack->IsScriptPackage())) {
        goto LABEL_79;
    }
    if (ppack->GetIsCreated()) {
        apActor->EndInterruptPackage(false);
        if ((GetCurrentPackage()) && (ppack == GetCurrentPackage())) {
            ClearCurrentDataforProcess(apActor);
            ClearCurrentPackage();
        }

        ppack = nullptr;
    }
    if (!SetCurrentPackageFromSchedule(apActor)) {
        return false;
    }
    TESPackage* pcurrentPackage = GetCurrentPackage();
    if (!ppack || (ppack == pcurrentPackage || (pcurrentPackage && pcurrentPackage->IsInterruptPackage()))) {
        bflag = false;
        goto LABEL_79;
    }
    if (!ppack->GetPCContinue()) {
        goto LABEL_79;
    }
    TESObjectCELL* v14 = pPlayer->GetParentCell();
    if (v14 != apActor->GetParentCell()) {
        goto LABEL_79;
    }
    if (GetProcedureIndexRunning()) {
        SetContinuingPackage(true);
    }
    SetCurrentPackage(ppack, apActor);
    return false;
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
    uint8_t result = 0;
    if (!GetCurrentPackage()) {
        return (bool)result;
    }
    TESPackage* v1 = GetCurrentPackage();
    PACKAGE_PROCEDURE_TYPE parent = v1->TESPackage::GetProcedureType();
    if (Procedure_Action[(uint32_t)parent][GetProcedureIndexRunning()] == 1) {
        return true;
    }
    return (bool)result;
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
    TESPackage* v5 = GetCurrentPackage();
    TESObjectREFR* v2 = GetTargetForPackage(apActor, v5);
    SetTarget(v2);
    return (pTarget != nullptr);
}

TESObjectREFR* LowProcess::SetLocationForPackage(Actor* apActor, bool abSecondLoc) {
    TESPackage* v6 = GetCurrentPackage();
    if (!v6) {
        return nullptr;
    }
    TESObjectREFR* v5 = GetLocationForPackage(apActor, v6, abSecondLoc);
    if (abSecondLoc) {
        pGenericSecondLocation = v5;
    }
    else {
        pGenericLocation = v5;
    }
    return v5;
}

TESObjectREFR* LowProcess::GetCurrentDestinationReference(Actor* apActor, bool abInterrupt) {
    TESPackage* PackageExtra = GetCurrentPackage();
    Actor* pTarget = static_cast<Actor*>(this->pTarget);
    int32_t PackageExtraIndex = GetProcedureIndexRunning();
    if (!abInterrupt && (!PackageExtra || PackageExtra->IsInterruptPackage())) {
        PackageExtra = (&apActor->m_Extra)->GetPackageExtra();
        PackageExtraIndex = (&apActor->m_Extra)->GetPackageExtraIndex();
        pTarget = static_cast<Actor*>((&apActor->m_Extra)->GetPackageExtraTarget());
    }
    if (!PackageExtra) {
        return nullptr;
    }
    if (PackageExtra->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
        return nullptr;
    }
    switch (Procedure_Action[PackageExtra->ePROCEDURE_TYPE][PackageExtraIndex]) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT:
        case PROCEDURE_ESCORT:
        case PROCEDURE_USE_WEAPON: {
            TESObjectREFR* LocReference;
            if (PackageExtra->pPackLoc) {
                LocReference = PackageExtra->pPackLoc->GetLocReference();
            }
            else {
                LocReference = nullptr;
            }
            pTarget = static_cast<Actor*>(LocReference);
            if (!LocReference) {
                TESObjectREFR* v6;
                if (apActor->pCurrentProcess) {
                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                    v6 = CurrentAIProcess->GetGenericLocation();
                }
                else {
                    v6 = nullptr;
                }
                pTarget = static_cast<Actor*>(v6);
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
            if ((pTarget) && (pTarget->IsActor())) {
                TESPackage* CurrentPackage = pTarget->GetCurrentPackage();
                if (CurrentPackage) {
                    pTarget = static_cast<Actor*>(CurrentPackage->GetLocationReference(pTarget));
                }
            }

            break;
        }
        case PROCEDURE_GUARD: {
            if (PackageExtra->SandboxGetAllowConversation() && PackageExtra->pPackLoc) {
                pTarget = static_cast<Actor*>(PackageExtra->pPackLoc->GetLocReference());
            }
            else {
                BaseProcess* v10 = apActor->pCurrentProcess;
                if (v10) {
                    pTarget = static_cast<Actor*>(v10->GetTarget());
                    if (!pTarget) {
                        pTarget = static_cast<Actor*>(v10->GetGenericLocation());
                    }
                    if (!pTarget) {
                        pTarget = apActor;
                    }
                }
            }
            break;
        }
        case PROCEDURE_DONE: {
            if (PackageExtra->pPackLoc) {
                pTarget = static_cast<Actor*>(PackageExtra->pPackLoc->GetLocReference());
            }
            if (!pTarget) {
                TESObjectREFR* v4;
                if (apActor->pCurrentProcess) {
                    BaseProcess* v5 = apActor->pCurrentProcess;
                    v4 = v5->GetGenericLocation();
                }
                else {
                    v4 = nullptr;
                }
                pTarget = static_cast<Actor*>(v4);
            }
            if (!pTarget) {
                pTarget = apActor;
            }
            break;
        }
        default: {
            return pTarget;
        }
    }
    return pTarget;
}

NiPoint3 LowProcess::GetCurrentDestinationCoordinate(Actor* apActor, bool abInterrupt) {
    NiPoint3 result;
    NiPoint3 v12;
    NiPoint3 v14;
    NiPoint3 ppoint = NiPoint3_ZERO;
    TESPackage* pPack = GetCurrentPackage();
    Actor* ptargAct = static_cast<Actor*>(pTarget);
    int32_t iIndex = GetProcedureIndexRunning();
    if ((!abInterrupt) && (pPack) && (pPack->IsInterruptPackage())) {
        pPack = (&apActor->m_Extra)->GetPackageExtra();
        iIndex = (&apActor->m_Extra)->GetPackageExtraIndex();
        ptargAct = static_cast<Actor*>((&apActor->m_Extra)->GetPackageExtraTarget());
    }

    if (pPack) {
        if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
            result = ppoint;
            return result;
        }

        switch (Procedure_Action[pPack->ePROCEDURE_TYPE][iIndex]) {
            case PROCEDURE_TRAVEL:
            case PROCEDURE_SLEEP:
            case PROCEDURE_EAT:
            case PROCEDURE_ESCORT:
            case PROCEDURE_USE_WEAPON: {
                const NiPoint3* LocationCoord;
                if (pPack->pPackLoc) {
                    LocationCoord = &(v14 = pPack->GetLocationCoord(apActor));
                }
                else {
                    LocationCoord = &apActor->GetLocationOnReference();
                }
                ppoint = *LocationCoord;
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
                const NiPoint3* v5;
                if (ptargAct) {
                    v5 = &ptargAct->GetLocationOnReference();
                }
                else {
                    v5 = &apActor->GetLocationOnReference();
                }
                ppoint = *v5;
                break;
            }
            case PROCEDURE_ACCOMPANY: {
                ppoint = apActor->GetLocationOnReference();
                if ((ptargAct) && (ptargAct->IsActor())) {
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
                else {
                    if (pPack->GetPackageTarget() && pPack->GetPackageTarget()->GetTargReference()) {
                        Actor* TargReference = static_cast<Actor*>(pPack->GetPackageTarget()->GetTargReference());
                        ppoint = TargReference->GetLocationOnReference();
                    }
                    else {
                        ppoint = apActor->GetLocationOnReference();
                    }
                }
                break;
            }
            case PROCEDURE_DONE: {
                if (abInterrupt) {
                    const NiPoint3* v9;
                    if (ptargAct) {
                        v9 = &ptargAct->GetLocationOnReference();
                    }
                    else {
                        v9 = &apActor->GetLocationOnReference();
                    }
                    ppoint = *v9;
                }
                else {
                    if (pPack->GetPackType() == PACKAGE_TYPE_TRAVEL) {
                        const NiPoint3* PackageStartLocationCoord;
                        if (pPack->pPackLoc) {
                            PackageStartLocationCoord = &(v12 = pPack->GetLocationCoord(apActor));
                        }
                        else {
                            PackageStartLocationCoord = &apActor->GetPackageStartLocationCoord();
                        }
                        ppoint = *PackageStartLocationCoord;
                    }
                    else {
                        const NiPoint3* v8;
                        if (ptargAct) {
                            v8 = &ptargAct->GetLocationOnReference();
                        }
                        else {
                            v8 = &apActor->GetLocationOnReference();
                        }
                        ppoint = *v8;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
        result = ppoint;
        return result;
    }

    result = ppoint;
    return result;
}

TESObjectCELL* LowProcess::GetCurrentDestinationCell(Actor* apActor) {
    TESObjectCELL* pCell = nullptr;
    if (apActor) {
        pCell = apActor->GetParentCell();
    }
    if (!GetCurrentPackage()) {
        return pCell;
    }
    TESPackage* v3 = GetCurrentPackage();
    uint32_t Count = v3->ePROCEDURE_TYPE;
    switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_USE_WEAPON: {
            if (!GetCurrentPackage()->pPackLoc) {
                goto LABEL_18;
            }
            goto LABEL_24;
        }
        case PROCEDURE_WANDER:
        case PROCEDURE_ACTIVATE:
        case PROCEDURE_ACQUIRE:
        case PROCEDURE_ALARM:
        case PROCEDURE_TRAVEL_TARGET:
        case PROCEDURE_PURSUE: {
            if (!pTarget) {
                goto LABEL_18;
            }
            pCell = pTarget->GetParentCell();
            break;
        }
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT: {
            if (!GetCurrentPackage()->pPackLoc) {
                goto LABEL_18;
            }
            goto LABEL_24;
        }
        case PROCEDURE_FOLLOW:
        case PROCEDURE_WAIT:
        case PROCEDURE_AMBUSH_WAIT: {
            if (pTarget) {
                return pTarget->GetParentCell();
            }

            return apActor->GetParentCell();
        }
        case PROCEDURE_ESCORT: {
            if (GetCurrentPackage()->pPackLoc) {
                goto LABEL_24;
            }
            goto LABEL_30;
        }
        case PROCEDURE_ACCOMPANY:
        case PROCEDURE_DONE: {
        LABEL_30:;
            pCell = apActor->GetParentCell();
            break;
        }
        case PROCEDURE_SANDBOX: {
            SandBoxActorPackageData* InstanceDataThatIsRunning =
                static_cast<SandBoxActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
            if (InstanceDataThatIsRunning) {
                TESObjectREFR* Speaker = InstanceDataThatIsRunning->pCurrentTargetObject;
                if (Speaker) {
                    pCell = Speaker->GetParentCell();
                }
            }
            break;
        }
        case PROCEDURE_GUARD: {
            TESPackage* v5;
            v5 = GetCurrentPackage();
            if (v5->SandboxGetAllowConversation() && GetCurrentPackage()->pPackLoc) {
            LABEL_24:;
                TESPackage* v7 = GetCurrentPackage();
                pCell = v7->GetLocationCell(apActor);
            }
            else {
                if (GetTarget()) {
                    pCell = GetTarget()->GetParentCell();
                }
                else {
                LABEL_18:;
                    pCell = apActor->GetParentCell();
                }
            }
            break;
        }
        default: {
            return pCell;
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
    uint32_t Count = ppack->ePROCEDURE_TYPE;
    switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
        case PROCEDURE_TRAVEL:
        case PROCEDURE_USE_WEAPON: {
            if (!ppack->pPackLoc) {
                goto LABEL_28;
            }
            goto LABEL_22;
        }
        case PROCEDURE_WANDER:
        case PROCEDURE_ACTIVATE:
        case PROCEDURE_ACQUIRE:
        case PROCEDURE_ALARM:
        case PROCEDURE_TRAVEL_TARGET:
        case PROCEDURE_PURSUE: {
            if (pTarget) {
                return pTarget->GetWorldSpace();
            }
            goto LABEL_28;
        }
        case PROCEDURE_SLEEP:
        case PROCEDURE_EAT: {
            if (!ppack->pPackLoc) {
                goto LABEL_28;
            }
            goto LABEL_22;
        }
        case PROCEDURE_FOLLOW:
        case PROCEDURE_WAIT:
        case PROCEDURE_AMBUSH_WAIT: {
            if (!pTarget) {
                goto LABEL_28;
            }
            return pTarget->GetWorldSpace();
        }
        case PROCEDURE_ESCORT: {
            if (ppack->pPackLoc) {
                goto LABEL_22;
            }
            goto LABEL_28;
        }
        case PROCEDURE_ACCOMPANY:
        case PROCEDURE_DONE: {
            goto LABEL_28;
        }
        case PROCEDURE_SANDBOX: {
            SandBoxActorPackageData* pdata =
                static_cast<SandBoxActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
            if (pdata) {
                TESObjectREFR* ptarg = pdata->pCurrentTargetObject;
                if (ptarg) {
                    return ptarg->GetWorldSpace();
                }
            }
            return pWorld;
        }
        case PROCEDURE_GUARD: {
            if (ppack->SandboxGetAllowConversation() && ppack->pPackLoc) {
            LABEL_22:;
                pWorld = ppack->GetLocationWorld(apActor);
            }
            else {
                if (GetTarget()) {
                    TESObjectREFR* v4 = GetTarget();
                    pWorld = v4->GetWorldSpace();
                }
                else {
                LABEL_28:;
                    pWorld = apActor->GetWorldSpace();
                }
            }
            break;
        }
        default: {
            return pWorld;
        }
    }
    return pWorld;
}

float LowProcess::GetCurrentDestinationRadius(Actor* apActor) {
    float fradius = 128.0f;
    if (GetCurrentPackage()) {
        TESPackage* v3 = GetCurrentPackage();
        uint32_t Count = v3->ePROCEDURE_TYPE;
        switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
            case PROCEDURE_TRAVEL:
            case PROCEDURE_USE_WEAPON: {
                if (GetCurrentPackage()->pPackLoc) {
                    TESPackage* v5 = GetCurrentPackage();
                    fradius = v5->GetPackageRadiusActorToLocation(apActor, false);
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
                    TESPackage* v8 = GetCurrentPackage();
                    fradius = v8->GetPackageRadiusActorToRefTarget(apActor, false);
                }
                break;
            }
            case PROCEDURE_FOLLOW:
            case PROCEDURE_ESCORT:
            case PROCEDURE_WAIT:
            case PROCEDURE_AMBUSH_WAIT: {
                if (GetCurrentPackage()->GetPackageTarget()) {
                    TESPackage* v9 = GetCurrentPackage();
                    fradius = v9->GetPackageRadiusActorToRefTarget(apActor, false);
                }
                break;
            }
            case PROCEDURE_GUARD: {
                PackageTarget* ppackTarg = GetCurrentPackage()->GetPackageTarget();
                TESPackage* v6 = GetCurrentPackage();
                if (v6->SandboxGetAllowConversation() && GetCurrentPackage()->pPackLoc) {
                    TESPackage* v7 = GetCurrentPackage();
                    v7->GetLocationRadius(apActor);
                }
                else {
                    if (ppackTarg) {}
                }
                break;
            }
            default: {
                return fradius;
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
        apActor, nullptr, false, false, nullptr, false, false, false, false, false, false, false, apPackage
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
            pCurrentPackage, 0, pTarget, GetCurrentActionComplete(), IsTargetActivated(), IsPackageDoneOnce()
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
    BipedAnim* v5;
    TESBoundObject* objectReference = apActor->TESObjectREFR::GetObjectReference();
    TESNPC* pNPC = dynamic_cast<TESNPC*>(objectReference);
    TESBoundObject* v4 = apActor->TESObjectREFR::GetObjectReference();
    TESCreature* pCreature = dynamic_cast<TESCreature*>(v4);
    TESPackage* pPackage = GetCurrentPackage();
    uint8_t bAwake = 1;
    uint8_t v9 = 1;
    if (pPackage) {
        if (((int32_t)pPackage->GetPackType()) == 4) {
            bAwake = 0;
            v9 = 0;
        }
        else {
            v9 = (uint8_t)(!pPackage->GetForceUnequipWeapon());
        }
    }
    if (pNPC) {
        v5 = apActor->GetBiped();
        pNPC->InitParts(apActor, v5, true);
    }
    else {
        if (pCreature) {
            pCreature->InitDefaultWorn(apActor, bAwake, v9, true);
        }
    }
    if (abInterateIndex) {
        AddToProcedureIndexRunning(apActor, 1u);
    }
}

void LowProcess::CreateFollowforEscort(Actor* apActor, TESPackage* apEscort, bool abCheck) {
    Actor* pTarget = dynamic_cast<Actor*>(this->pTarget);
    if ((pTarget != apActor) && (pTarget) && (pTarget != pPlayer)) {
        if (apActor->GetSitSleepState()) {
            apActor->InitiateGetUpPackage();
        }
        TESPackage* Package = TESPackage::CreatePackage(PACKAGE_TYPE_FOLLOW);
        Package->SetPackType(PACKAGE_TYPE_FOLLOW);
        Package->SetMustReachLocation(true);
        Package->SetMustComplete(true);
        TESPackage* v4 = GetCurrentPackage();
        PackageLocation* Location_ = v4->GetLocation();
        Package->SetPackageLocation(Location_);
        uint32_t EscortFollowDistance = apEscort->GetEscortFollowDistance();
        PackageLocation* v6 = Package->GetLocation();
        v6->SetRadius(EscortFollowDistance);
        TESPackage* v7 = GetCurrentPackage();
        PackageTarget* PackageTarget_ = v7->GetPackageTarget();
        Package->SetPackageTarget(PackageTarget_);
        Package->SetProcedureType(PACKAGE_PROCEDURE_FOLLOW_WITH_ESCORT);
        PackageTarget* v9 = Package->GetPackageTarget();
        v9->SetTargType(PTT_REFERENCE);
        PackageTarget* v10 = Package->GetPackageTarget();
        v10->SetTargReference(apActor);
        PackageTarget* v11 = apEscort->GetPackageTarget();
        uint32_t FormFlags = v11->iValue;
        PackageTarget* v12 = Package->GetPackageTarget();
        v12->iValue = FormFlags;
        Package->CopyAllFlags(apEscort);
        BaseProcess* CurrentAIProcess = pTarget->GetCurrentAIProcess();
        CurrentAIProcess->ComputeLastTimeProcessed();
        CurrentAIProcess->SavePackageToExtraData(pTarget);
        apActor->AddFollower(pTarget);
        pTarget->PutCreatedPackage(Package, false, true);
        CurrentAIProcess->SetTarget(apActor);
    }
}

void LowProcess::ModEssentialDownTimer() {
    fEssentialDownTimer = (fEssentialDownTimer - (&appTimer)->GetSeconds());
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
    TESPackage* v5 = GetCurrentPackage();
    if (!v5) {
        return true;
    }
    PACKAGE_PROCEDURE_TYPE parent = v5->TESPackage::GetProcedureType();
    return (bool)((uint8_t)(Procedure_Action[(uint32_t)parent][GetProcedureIndexRunning()] == 54));
}

void LowProcess::AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb) {
    PACKAGE_PROCEDURE_TYPE parent;
    PACKAGE_PROCEDURE_TYPE v4;
    ExtraDataList* extra;
    int32_t procedureCount;
    TESPackage* a1 = GetCurrentPackage();
    AddToCurrentProcedureIndex(ainumb);
    if ((GetCurrentProcedureIndex() & 0x80000000u) != 0) {
        SetCurrentProcedureIndex((uint32_t)0);
    }
    if (a1) {
        parent = a1->TESPackage::GetProcedureType();
        procedureCount = Procedure::GetProcedureCount(static_cast<PACKAGE_PROCEDURE_TYPE>((int32_t)parent));
        if (GetCurrentProcedureIndex() >= procedureCount) {
            SetCurrentProcedureIndex((uint32_t)(procedureCount - 1));
        }
        v4 = a1->TESPackage::GetProcedureType();
        if ((Procedure_Action[(uint32_t)v4][GetCurrentProcedureIndex()] == 54) && (!IsPackageDoneOnce())) {
            extra = apActor->TESObjectREFR::GetExtra();
            Script::SetActionFlag(a1, extra, SCRIPT_ACTION_ONPACKAGEEND);
            PackageDone(static_cast<Actor*>(apActor), a1);
            SetPackageDoneOnce(true);
        }
    }
}

void LowProcess::ProcessFollow(Actor* apActor, bool abflag, int32_t emovemode, bool abpursuit) {
    TESPackage* v5;
    if (!GetCurrentPackage()) {
        goto LABEL_8;
    }
    v5 = GetCurrentPackage();
    if (!v5->GetWaiting()) {
        goto LABEL_8;
    }
    if (!pPlayer->IsPlayerFollower(apActor)) {
        int32_t NumberOfFollowers;
        NumberOfFollowers = pPlayer->GetNumberOfFollowers();
        if (NumberOfFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
            TESPackage* v7;
            TESPackage* v8;
            TESPackage* v14;
            v7 = GetCurrentPackage();
            v7->SetWaiting(false);
        LABEL_8:;
            if (!pTarget) {
                SetTargetForPackage(apActor);
            }
            if (!pTarget ||
                (GetCurrentPackage()->pPackLoc &&
                 ((v14 = GetCurrentPackage(), v14->IsActorAtLocation(apActor, false, -1.0f, false)) &&
                  (v8 = GetCurrentPackage(), v8->GetPackType() == PACKAGE_TYPE_FOLLOW)))) {
                AddToProcedureIndexRunning(apActor, 1);
                TESPackage* v9 = GetCurrentPackage();
                if (v9->GetIsCreated()) {
                    ClearCurrentDataforProcess(apActor);
                    ClearCurrentPackage();
                    CheckforNewPackage(apActor, false);
                }
            }
            else {
                GetTimeDelta(apActor);
                TESPackage* v10 = GetCurrentPackage();
                float distance = v10->GetPackageRadiusActorToRefTarget(apActor, false);
                TESWorldSpace* pws1 = apActor->GetWorldSpace();
                TESWorldSpace* pws2 = pTarget->GetWorldSpace();
                if ((apActor->IsPathingComplete()) && (pTarget) &&
                    (pws1 != pws2 ||
                     static_cast<double>(distance) < apActor->GetDistanceFromReference(pTarget, false, false))) {
                    TESWorldSpace* WorldSpace = pTarget->GetWorldSpace();
                    TESObjectCELL* ParentCell = pTarget->GetParentCell();
                    const NiPoint3* v11 = &pTarget->GetLocationOnReference();
                    apActor->SetPathfindingGoal(*v11, ParentCell, WorldSpace, distance, nullptr);
                }
            }
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
        if (!(apActor->GetDistanceFromReference(this->pTarget, false, false) > 200.0)) {
            TESPackage* v5;
            if (!apActor->IsActoraGuard() || (v5 = GetCurrentPackage(), v5->GetPackType() == PACKAGE_TYPE_COMBAT_LOW)) {
                AddToProcedureIndexRunning(apActor, 1);
            }
        }
        else {
            ProcessFollow(apActor, false, emovemode, true);
        }
    }
    else {
        AddToProcedureIndexRunning(apActor, 1);
    }
}

void LowProcess::ProcessFlee(Actor* apActor) {
    TESPackage* v2 = GetCurrentPackage();
    if (v2->IsActorAtLocation(apActor, false, (-0x1p+0f), false)) {
        apActor->EndInterruptPackage(false);
    }
    else {
        GetTimeDelta(apActor);
    }
}

// Says aptopic's matching info to the actor's dialogue target (taken and cleared) and runs its begin script; with
// abSayCallBack the actor records the topic and info and the say-to callback runs at once. The current package is read
// and discarded, as the game does.
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
    ExtraFollower* pfollow = (&apActor->m_Extra)->GetFollowerExtra();
    if (pfollow) {
        BSSimpleList<Actor*>* pTempList;
        void* pTempMemory = ourMalloc(sizeof(BSSimpleList<Actor*>));
        if (pTempMemory) {
            pTempList = ::new (pTempMemory) BSSimpleList<Actor*>;
        }
        else {
            pTempList = nullptr;
        }
        BSSimpleList<Actor*>* pHead;
        void* pHeadMemory = ourMalloc(sizeof(BSSimpleList<Actor*>));
        if (pHeadMemory) {
            pHead = ::new (pHeadMemory) BSSimpleList<Actor*>;
        }
        else {
            pHead = nullptr;
        }
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
                TESPackage* pactEdit = (&apActor->m_Extra)->GetPackageExtra();
                if ((!pactpack || pactpack->IsInterruptPackage()) && (pactEdit)) {
                    pactpack = pactEdit;
                }

                if ((pactpack) && (pactpack->GetPackType() == PACKAGE_TYPE_ESCORT)) {
                    bremove = true;
                }

                TESPackage* ptargpack = ptarget->GetPackageSetAsPcurrent();
                TESPackage* pEditorpack = (&ptarget->m_Extra)->GetPackageExtra();
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
            (&apActor->m_Extra)->RemoveFollower(plist->m_item);
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
            (&apActor->m_Extra)->RemoveFollowerExtra();
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
    double v4;
    float afValue_2;
    if ((!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) && (aiWhich == 16)) {
        if (apActor) {
            v4 = (double)Actor::CheckClampDamageModifier(
                static_cast<ActorValueOwner*>(apActor), static_cast<ActorValue::Index>(16), afValue
            );
        }
        else {
            v4 = (double)Actor::CheckClampDamageModifier(nullptr, static_cast<ActorValue::Index>(16), afValue);
        }
        afValue_2 = (float)v4;
        (&pDamageModifiers)->AdjustModifier(16, afValue_2, ModifierList::LE_ZERO);
    }
}

void LowProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) {
    float v4 = static_cast<float>(aiValue);
    DamageModActorValue(apActor, aiWhich, v4);
}

void LowProcess::ClearObjectFromAcquireList(TESObjectREFR* apRef) {
    if ((pAcquireObject) && (pAcquireObject->pRef == apRef)) {
        if (pAcquireObject) {
            pAcquireObject->Delete(1u);
        }
        pAcquireObject = nullptr;
    }

    if ((pSavedAcquireObject) && (pSavedAcquireObject->pRef == apRef)) {
        if (pSavedAcquireObject) {
            pSavedAcquireObject->Delete(1u);
        }
        pSavedAcquireObject = nullptr;
    }

    BSSimpleList<ObjectstoAcquire*>* plist = &ObjectList;
    while (plist && plist->m_item) {
        ObjectstoAcquire* pobj = plist->m_item;
        if (pobj->pRef == apRef) {
            ObjectList.Remove(pobj);
            if (pobj) {
                pobj->Delete(1u);
            }
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
    if ((pAcquireObject) && (pAcquireObject->pRef->iFormID == aID)) {
        if (pAcquireObject) {
            pAcquireObject->Delete(1u);
        }
        pAcquireObject = nullptr;
    }

    if ((pSavedAcquireObject) && (pSavedAcquireObject->pRef->iFormID == aID)) {
        if (pSavedAcquireObject) {
            pSavedAcquireObject->Delete(1u);
        }
        pSavedAcquireObject = nullptr;
    }

    BSSimpleList<ObjectstoAcquire*>* plist = &ObjectList;
    while (plist && plist->m_item) {
        ObjectstoAcquire* pobj = plist->m_item;
        if (pobj->pRef->iFormID == aID) {
            ObjectList.Remove(pobj);
            if (pobj) {
                pobj->Delete(1u);
            }
            plist = &ObjectList;
        }
        else {
            plist = plist->m_pkNext;
        }
    }
    if ((pGenericLocation) && (pGenericLocation->iFormID == aID)) {
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
        pAcquireObject->Delete(1u);
        pAcquireObject = nullptr;
    }
    if (pSavedAcquireObject) {
        ObjectList.Remove(pSavedAcquireObject);
        if (pSavedAcquireObject) {
            pSavedAcquireObject->Delete(1u);
        }
        pSavedAcquireObject = nullptr;
    }
    while (!ObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj = ObjectList.m_item;
        if (pobj) {
            pobj->Delete(1u);
        }
        ObjectList.Remove(pobj);
    }
    GenericLocationsList.RemoveAll();
}

uint16_t LowProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    uint16_t SaveSize = BaseProcess::GetSaveSize(aiFlags, pmob);
    uint16_t v7 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    int16_t v9 = m_uFlags + SaveSize;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 75u) {
        v9 += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 79u) {
        v9 += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 86u) {
        v9 += 4;
    }
    uint16_t isize = v9 + 8;
    if ((aiFlags & CHANGE_ACTOR_DAMAGE_MODIFIERS) != 0) {
        isize += pDamageModifiers.GetSaveSize();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 116u) {
        isize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v7,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 3802,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v7, 3802,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
    }
    return isize;
}

void LowProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESObjectREFR* apForm;
    BGSChangeFlags result;
    TESObjectREFR* v6;
    BSSimpleList<TESObjectREFR*>* ecx0;
    BaseProcess::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&m_uFlags)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fEssentialDownTimer)), 4u, 0);
    apSaveGameBuffer->SaveFormID(pItemBeingUsed, (uint32_t)0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iNumberItemsActivate)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fDeathTime)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fTrackedDamage)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bSkippedTimeStampForPathing)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&sFormListNumb)), 2u, 0);
    (&fCombatDelayTimer)->SaveGame(apSaveGameBuffer);
    if (pAcquireObject) {
        apForm = nullptr;
    }
    else {
        apForm = pTarget;
    }
    apSaveGameBuffer->SaveFormID(apForm, (uint32_t)0);
    apSaveGameBuffer->SaveFormID(pGenericLocation, (uint32_t)0);
    apSaveGameBuffer->SaveFormID(pGenericSecondLocation, (uint32_t)0);
    apSaveGameBuffer->SaveFormID(pTargetFormList, (uint32_t)0);
    apSaveGameBuffer->SaveFormID(pPatrolLocation, (uint32_t)0);
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (ecx0 = &GenericLocationsList; ecx0; ecx0 = ecx0->m_pkNext) {
        v6 = ecx0->m_item;
        if (v6) {
            apSaveGameBuffer->SaveFormID(v6, (uint32_t)0);
            ++aiVariableValue;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
    BGSChangeFlags* v2 = &(result = apSaveGameBuffer->GetChangeFlags());
    if (v2->CheckFlags(0x200000u)) {
        (&pDamageModifiers)->SaveGame(apSaveGameBuffer);
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&m_uFlags, 1u);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 75u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fEssentialDownTimer, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 79u) {
        iitemBeingUsedID = 0;
        if (pItemBeingUsed) {
            iitemBeingUsedID = pItemBeingUsed->iFormID;
        }
        pSaveLoadGameOLD->SaveNumericID(&iitemBeingUsedID, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 86u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&iNumberItemsActivate, 4u);
    }
    itargetID = 0;
    if ((pTarget) && (!pAcquireObject)) {
        itargetID = pTarget->iFormID;
    }

    pSaveLoadGameOLD->SaveNumericID(&itargetID, 4u);
    ilocationID = 0;
    if (pGenericLocation) {
        ilocationID = pGenericLocation->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&ilocationID, 4u);
    if ((aiFlags & 0x200000) != 0) {
        pDamageModifiers.SaveGame_009378D0();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 116u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fDeathTime, 4u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&fTrackedDamage, 4u);
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v3 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                BufferPointer - itempStartPosition, pheader->iFormID, v3, iFlags, 3872,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition, 3872,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3872
            );
        }
        *pblockSize = static_cast<uint16_t>(pbufferEnd - pblockStart);
    }
}

void LowProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pForm;
    uint32_t ptempData_2;
    BaseProcess::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() < 6u) {
        char ptempData[4];
        apLoadGameBuffer->LoadData(ptempData, 4u);
        apLoadGameBuffer->LoadData(&ptempData_2, 4u);
    }
    apLoadGameBuffer->LoadData(&m_uFlags, 1u);
    apLoadGameBuffer->LoadData(&fEssentialDownTimer, 4u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pItemBeingUsed = dynamic_cast<TESBoundObject*>(FormByNumericID);
    apLoadGameBuffer->LoadData(&iNumberItemsActivate, 4u);
    apLoadGameBuffer->LoadData(&fDeathTime, 4u);
    apLoadGameBuffer->LoadData(&fTrackedDamage, 4u);
    apLoadGameBuffer->LoadData(&bSkippedTimeStampForPathing, 1u);
    apLoadGameBuffer->LoadData(&sFormListNumb, 2u);
    fCombatDelayTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pTarget);
    apLoadGameBuffer->LoadFormID(pGenericLocation);
    apLoadGameBuffer->LoadFormID(pGenericSecondLocation);
    uint32_t v4 = apLoadGameBuffer->LoadFormID();
    TESForm* v5 = TESForm::GetFormByNumericID(v4);
    pTargetFormList = dynamic_cast<BGSListForm*>(v5);
    apLoadGameBuffer->LoadFormID(pPatrolLocation);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        pForm = nullptr;
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3885, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3885,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 54u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&ftemp, 4u);
        int32_t dvalue = static_cast<int32_t>(std::fmod(ftemp, 100.0f));
        double dvalue_2 = std::fmod(ftemp, 10000.0f);
        int32_t imonth = static_cast<int32_t>(dvalue_2) / 100;
        int32_t iyear = static_cast<int32_t>(ftemp) / 0x2710;
        iDateLastProcessed = iyear << 13 | (dvalue | imonth << 9);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&m_uFlags, 1u);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 75u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fEssentialDownTimer, 4u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 79u) {
        pSaveLoadGameOLD->LoadNumericID(&iitemBeingUsedID, 4u);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(iitemBeingUsedID);
        pItemBeingUsed = dynamic_cast<TESBoundObject*>(FormByNumericID);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 86u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iNumberItemsActivate, 4u);
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
        pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
        if (icount) {
            pSaveLoadGameOLD->AdvanceBuffer(8 * icount);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 116u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fDeathTime, 4u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&fTrackedDamage, 4u);
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t afYa = pheader_2->iFlags;
                    int32_t afXa = pheader_2->cVersion;
                    const char* v6 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3959, pheader_2->iFormID,
                        v6, afXa, afYa
                    );
                }
            }
            else {
                uint32_t afY = pheader_2->iFlags;
                int32_t afX = pheader_2->cVersion;
                const char* v5 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3959, pheader_2->iFormID, v5,
                    afX, afY
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t afYc = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3959, afYc
                    );
                }
            }
            else {
                int32_t afYb = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\LowProcess.cpp", 3959, afYb
                );
            }
        }
    }
}

void LowProcess::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v8;
    TESObjectREFR* v9;
    TESObjectREFR* v10;
    TESObjectREFR* ptempTarget;
    BaseProcess::InitLoadGame(apLoadGameBuffer);
    if (pTarget) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        ptempTarget = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        ptempTarget = nullptr;
    }
    TESObjectREFR** pmutableTarget = &pTarget;
    pTarget = nullptr;
    SetTarget(ptempTarget);
    if (pGenericLocation) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGenericLocation)));
        v10 = dynamic_cast<TESObjectREFR*>(v3);
    }
    else {
        v10 = nullptr;
    }
    pGenericLocation = v10;
    if (pGenericSecondLocation) {
        TESForm* v4 =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGenericSecondLocation)));
        v9 = dynamic_cast<TESObjectREFR*>(v4);
    }
    else {
        v9 = nullptr;
    }
    pGenericSecondLocation = v9;
    if (pPatrolLocation) {
        TESForm* v5 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPatrolLocation)));
        v8 = dynamic_cast<TESObjectREFR*>(v5);
    }
    else {
        v8 = nullptr;
    }
    pPatrolLocation = v8;
    BSSimpleList<TESObjectREFR*>* plist = &GenericLocationsList;
    BSSimpleList<TESObjectREFR*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        TESObjectREFR* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* v6 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<TESObjectREFR*>(v6);
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
                BSSimpleList<TESObjectREFR*>* v7 = plist;
                plastList->Remove(v7->m_item);
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
        TESForm* pForm =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSavedLocation)));
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
            if (pobj) {
                pobj->Delete(1u);
            }
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
    ExtraDataList* extra;
    ScriptLocals* scriptLocals;
    TESTopic* v6;
    Script* v7;
    if (apPack) {
        v7 = apPack->GetOnChangeScript();
        if ((v7) && (v7->GetCompiledData() || v7->GetText())) {
            extra = apActor->TESObjectREFR::GetExtra();
            scriptLocals = extra->GetScriptLocals();
            v7->Run(apActor, scriptLocals, nullptr, true);
        }

        v6 = apPack->GetOnChangeTopicInfo();
        if (v6) {
            ProcessGreet(apActor, v6, false, false, true, false);
        }
    }
}

void LowProcess::PackageDone(Actor* apActor, TESPackage* apPack) {
    if (apPack) {
        Script* v7 = apPack->GetOnEndScript();
        if (v7) {
            ExtraDataList* Extra = apActor->GetExtra();
            ScriptLocals* ScriptLocals_ = Extra->GetScriptLocals();
            v7->Run(apActor, ScriptLocals_, nullptr, true);
        }
        TESTopic* v6 = apPack->GetOnEndTopic();
        if (v6) {
            ProcessGreet(apActor, v6, false, false, true, false);
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
    apMob->AddChange(0x80000000);
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
        TESObjectREFR* PackageExtraTarget = pextra->GetPackageExtraTarget();
        CurrentPackage.InitFromPackage(pextra->GetPackageExtra(), PackageExtraTarget);
        // The current package is only handed to an empty function on apMob, so its result is unused and only the
        // virtual call stays.
        GetCurrentPackage();
        int32_t iindex = pextra->GetPackageExtraIndex();
        PACKAGE_PROCEDURE_TYPE eprocType = CurrentPackage.pPackage->ePROCEDURE_TYPE;
        if (eprocType != PACKAGE_PROCEDURE_NONE && iindex >= Procedure::GetProcedureCount(eprocType)) {
            iindex = 0;
        }
        SetCurrentProcedureIndex(iindex);
        TESObjectREFR* v3 = pextra->GetPackageExtraTarget();
        SetTarget(v3);
        bool PackageExtraComplete = pextra->GetPackageExtraComplete();
        SetCurrentActionComplete(PackageExtraComplete);
        bool PackageExtraDoneOnce = pextra->GetPackageExtraDoneOnce();
        SetPackageDoneOnce(PackageExtraDoneOnce);
        bool PackageExtraActivate = pextra->GetPackageExtraActivate();
        SetTargetActivated(PackageExtraActivate);
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
    if ((apAggressor) && (apAggressor->IsPc() || apAggressor->Actor::IsPlayerTeammate())) {
        fTrackedDamage = (fTrackedDamage + afAmount);
    }
}

void LowProcess::ProcessEscort(Actor* apActor) {
    uint32_t v2;
    uint32_t v3;
    NiPoint3 v21;
    NiPoint3 destcoord;
    float ftargdistance;
    float fMydistance;
    TESPackage* pPack = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    int32_t ainumb = 0;
    ItemChange* pitem = apActor->GetItemBeingEscorted();
    bool bManyflag = false;
    if (pitem ||
        ((pTarget && (pTarget->IsActor() || (v2 = pTarget->iFormID, apActor->HasObjectId(v2)))) ||
         ((SetTargetForPackage(apActor), pTarget) &&
          (pTarget->IsActor() || (v3 = pTarget->iFormID, apActor->HasObjectId(v3)))))) {
        uint32_t CastingType;
        if ((!pitem) && (pAcquireObject) && (pAcquireObject->pRef == apActor)) {
            pitem = apActor->GetInventoryItem(pAcquireObject->pObject, 0);
            bManyflag = true;
        }

        if (GetCurrentPackage()->GetPackageTarget()) {
            TESPackage* v4 = GetCurrentPackage();
            ainumb = v4->GetInitialTargetCount();
        }
        if ((pTarget && (pTarget->IsActor() || (CastingType = pTarget->iFormID, apActor->HasObjectId(CastingType)))) ||
            CheckIfHasObject(apActor, ainumb)) {
            float ftimedelta = GetTimeDelta(apActor);
            TESPackage* v19 = GetCurrentPackage();
            if (v19->IsActorAtLocation(apActor, false, -1.0f, false)) {
                if ((!pTarget || pTarget->IsActor()) && pTarget) {
                    TESPackage* v18 = GetCurrentPackage();
                    if (v18->IsActorAtLocation(static_cast<MobileObject*>(pTarget), false, -1.0f, false)) {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                }
                else {
                    if (pitem) {
                        ExtraDataList* plist = nullptr;
                        if (pitem->pExtraObjectList) {
                            plist = pitem->pExtraObjectList->m_item;
                        }
                        uint32_t id = 0;
                        if ((plist) && (plist->GetReferencePointer())) {
                            TESObjectREFR* ReferencePointer = plist->GetReferencePointer();
                            id = ReferencePointer->iFormID;
                        }

                        TESObjectREFR* pcont = pPack->pPackLoc->GetLocReference();
                        if (!pcont) {
                            pcont = pGenericLocation;
                        }
                        if (pcont && pcont->HasContainer()) {
                            int32_t InitialTargetCount = pPack->GetInitialTargetCount();
                            apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, InitialTargetCount, id);
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
                                    TESBoundObject* v9;
                                    TESBoundObject* ObjectReference = pref->GetObjectReference();
                                    if (ObjectReference == pXMarker ||
                                        (v9 = pref->GetObjectReference(), v9 == pXMarkerHeading)) {
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
                double DistanceFromReference;
                TESPackage* v10 = GetCurrentPackage();
                int32_t distance = static_cast<int32_t>(v10->GetPackageRadiusActorToRefTarget(apActor, false));
                fMydistance = 0.0f;
                ftargdistance = 0.0f;
                if ((pTarget) && (pTarget->IsActor())) {
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
                     (!pTarget->IsActor() ||
                      ((DistanceFromReference = apActor->GetDistanceFromReference(pTarget, false, false),
                        static_cast<double>(distance) > DistanceFromReference) ||
                       ftargdistance < static_cast<double>(fMydistance)))) &&
                    (apActor->IsPathingComplete())) {
                    TESPackage* pPack_2 = GetCurrentPackage();
                    TESWorldSpace* LocationWorld = pPack_2->GetLocationWorld(apActor);
                    TESPackage* pPack_3 = GetCurrentPackage();
                    TESObjectCELL* LocationCell = pPack_3->GetLocationCell(apActor);
                    TESPackage* pPack_4 = GetCurrentPackage();
                    NiPoint3* LocationCoord = &(v21 = pPack_4->GetLocationCoord(apActor));
                    apActor->SetPathfindingGoal(*LocationCoord, LocationCell, LocationWorld, -1.0f, nullptr);
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
    TESPackage* v4;
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (!pTarget || (v4 = GetCurrentPackage(), v4->IsActorAtRefTarget(apActor, 0))) {
        if (pTarget == pPlayer && (pPlayer->IsSleepingorResting() && !pPlayer->btransporting)) {
            pPlayer->SetSleeping(0, true);
            AddToProcedureIndexRunning(apActor, -1);
        }
        else {
            int32_t v6 = static_cast<int32_t>(fCombatRoundAmount.Float() * 4.0);
            Actor* v8 = nullptr;
            char v7 = 0;
            while (v6) {
                if (apActor->IsDead(false)) {
                    break;
                }
                apActor->PerformOneRoundofCombat();
                if (!apActor->IsInCombat()) {
                    break;
                }
                v8 = dynamic_cast<Actor*>(pTarget);
                if (v8) {
                    v8->SetCurrentTarget(apActor);
                    if (!v8->IsDead(false)) {
                        if (!v8->IsInCombat()) {
                            BaseProcess* CurrentAIProcess = v8->GetCurrentAIProcess();
                            CurrentAIProcess->EnterCombat(
                                v8, apActor, false, false, nullptr, false, false, false, 0, false, false, true, nullptr
                            );
                        }
                        v8->PerformOneRoundofCombat();
                        v7 = 1;
                    }
                }
                --v6;
            }
            if (v7) {
                BaseProcess* v2 = v8->GetCurrentAIProcess();
                v2->ComputeLastTimeProcessed();
            }
        }
    }
    else {
        ProcessFollow(apActor, false, 257, false);
    }
}

void LowProcess::ProcessAlarm(Actor* apActor) {
    TESPackage* v2 = GetCurrentPackage();
    AlarmPackage* ppack = dynamic_cast<AlarmPackage*>(v2);
    Actor* pactor = nullptr;
    if (ppack) {
        TESClass* Class = apActor->GetClass();
        if (Class->IsGuard()) {
            AddToProcedureIndexRunning(apActor, 1);
            ProcessPursue(apActor, false, true, 257);
            if ((apActor->GetDistanceFromReference(pPlayer, false, false) > 3500.0) && (pPlayer->GetWorldSpace())) {
                TESWorldSpace* WorldSpace = pPlayer->GetWorldSpace();
                if (WorldSpace == apActor->GetWorldSpace()) {
                    apActor->EndInterruptPackage(false);
                }
            }
        }
        else {
            BSSimpleList<Crime*>* plist = &ppack->Crimes;
            bool bcombatflag = false;
            while (plist && (plist->m_item && !bcombatflag)) {
                Crime* pcrime = plist->m_item;
                TESObjectREFR* pcrimeRef = nullptr;
                int32_t pcrimeAct = 0;
                Actor* pactor_2 = pcrime->pcriminal;
                pcrimeRef = pcrime->pcrimetarget;
                FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                if (!bcombatflag) {
                    if (pcrime->ecrime <= PackageNS::CRIME_TYPE_TRESPASS) {
                        if (apActor->GetShouldAttackActor(pactor_2, false, ereaction, false)) {
                            bcombatflag = true;
                        }
                    }
                    else {
                        if ((pcrimeAct) && (apActor->GetShouldAttackActor(pactor_2, false, ereaction, false))) {
                            bcombatflag = true;
                        }
                    }
                }
                plist = plist->m_pkNext;
            }
            if (bcombatflag) {
                ProcessFollow(apActor, false, 257, false);
                EnterCombat(
                    apActor, pactor, false, false, nullptr, false, false, false, 0, false, false, true, nullptr
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
    if (pLocRef && pLocRef->IsFurniture() &&
        (pTarget->GetObjectReference() == pXMarkerHeading || pTarget->GetObjectReference() == pXMarker)) {
        return true;
    }
    if (pPackage->GetPackType() == PACKAGE_TYPE_AMBUSH && !pPackage->IsActorAtLocation(apActor, false, 300.0f, false)) {
        AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-2));
    }
    // The delta is stored in a local that is never read, as the game does.
    [[maybe_unused]] float fTimeDelta = GetTimeDelta(apActor);
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
            if (pOld) {
                pOld->Delete(1);
            }
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
                        apActor, static_cast<Actor*>(pTarget), false, false, nullptr, false, true, false, true, false,
                        false, true, nullptr
                    );
                }
                else if (pAcquireObject->bSteal) {
                    pTarget->Activate(apActor, true, pAcquireObject->pObject, iCount);
                    ObjectstoAcquire* pOld = pAcquireObject;
                    if (pOld) {
                        pOld->Delete(1);
                    }
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
    TESPackage* v2;
    TESPackage* v3;
    PackageTarget* pPackTarget = nullptr;
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    if (pTarget) {
        pPackTarget = GetCurrentPackage()->GetPackageTarget();
    }
    if (pTarget &&
        ((v2 = GetCurrentPackage(), v2->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT) ||
         (v3 = GetCurrentPackage(), v3->GetPackType() == PACKAGE_TYPE_USE_WEAPON))) {
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
                goto LABEL_34;
            }
            case LIGH_ID: {
                TESObjectLIGH* plight = dynamic_cast<TESObjectLIGH*>(pTarget->GetObjectReference());
                if ((plight) && (!plight->GetCanCarry())) {
                    AddToProcedureIndexRunning(apActor, 1);
                }

                break;
            }
            default: {
                goto LABEL_13;
            }
        }
    }
    else {
        TESPackage* v4;
    LABEL_13:;
        if ((pPackTarget && pPackTarget->GetTargReference()) || (v4 = GetCurrentPackage(), v4->GetMustComplete())) {
            int32_t ainumb = 1;
            if (GetCurrentPackage()->GetPackageTarget()) {
                TESPackage* v5 = GetCurrentPackage();
                ainumb = v5->GetInitialTargetCount();
            }
            if (CheckIfHasObject(apActor, ainumb)) {
                TESPackage* v6;
                TESPackage* v11 = GetCurrentPackage();
                if (v11->IsActorAtLocation(apActor, false, -1.0f, false) ||
                    (v6 = GetCurrentPackage(), v6->GetPackType() != PACKAGE_TYPE_EAT)) {
                    TESPackage* v8;
                    TESPackage* v7 = GetCurrentPackage();
                    if (v7->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC ||
                        (v8 = GetCurrentPackage(), v8->GetPackageSchedule()->PSData.iDuration)) {
                        AddToProcedureIndexRunning(apActor, 1);
                    }
                    else {
                        AddToProcedureIndexRunning(apActor, 2);
                    }
                }
                else {
                    SetProcedureIndexRunning(0);
                }
                TESPackage* v9 = GetCurrentPackage();
                if (v9->GetPackType() == PACKAGE_TYPE_ESCORT) {
                    ProcessEscort(apActor);
                }
            }
            else {
                if (apActor->GetCurrentProcessType() >= PROCESS_TYPE_MIDDLE_LOW) {
                    ProcessActivate(apActor, true);
                }
                else {
                    ProcessActivateOneHour(apActor, 3600.0f);
                }
            }
        }
        else {
            TESPackage* v10;
            v10 = GetCurrentPackage();
            if (v10->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                AddToProcedureIndexRunning(apActor, 2);
            }
            else {
            LABEL_34:;
                AddToProcedureIndexRunning(apActor, 1);
            }
        }
    }
}

void LowProcess::ProcessAmbushWait(Actor* apActor) {
    TESPackage* v4 = GetPackageThatIsRunning();
    if (!pTarget || pTarget->GetDisabled()) {
        goto LABEL_3;
    }
    if (pTarget->GetDelete()) {
        if (pTarget->GetDelete()) {
            v4->SetNeverRun(pTarget, true);
        }
    LABEL_3:;
        AddToProcedureIndexRunning(apActor, 2);
        return;
    }
    if (!pTarget->IsDead(true) || pAcquireObject) {
        if (v4->IsActorAtLocation(apActor, false, -1.0f, false)) {
            float DistanceFromReference = apActor->GetDistanceFromReference(pTarget, false, false);
            if (DistanceFromReference != 3.4028234663852886e+38 && v4->IsTargetAtLocation(apActor, 0)) {
                AddToProcedureIndexRunning(apActor, 1);
            }
        }
        else {
            AddToProcedureIndexRunning(apActor, -1);
        }
    }
    else {
        v4->SetNeverRun(pTarget, true);
    }
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
    PackageTarget* packageTarget;
    TESPackage* v4 = GetPackageThatIsRunning();
    if (!pTarget) {
        SetTargetForPackage(apActor);
    }
    SetLocationForPackage(apActor, false);
    float formFlags = (float)0.0;
    if (v4->GetPackageTarget()) {
        packageTarget = v4->GetPackageTarget();
        formFlags = (float)(double)packageTarget->iValue;
    }
    if (((!pTarget) || (!(((double)formFlags) < apActor->GetDistanceFromReference(pTarget, false, false)))) &&
        (!pTarget)) {
        v4->IsActorAtLocation(apActor, false, (-0x1p+0f), false);
    }
}

void LowProcess::ProcessAvoidArea(Actor* apActor) {
    AddToProcedureIndexRunning(apActor, (uint32_t)1);
}

void LowProcess::ProcessSandBox(Actor* apActor) {
    SandBoxActorPackageData* pPackData =
        static_cast<SandBoxActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
    if (pPackData) {
        switch (pPackData->eState) {
            case SandBoxActorPackageData::SANDBOX_STATE_INIT: {
                TESPackage* v3 = GetPackageThatIsRunning();
                pPackData->eState = SandBoxActorPackageData::SANDBOX_STATE_MOVING;
                PackageLocation* Location_ = v3->GetLocation();
                pPackData->InitLocation(apActor, Location_);
                break;
            }
            case SandBoxActorPackageData::SANDBOX_STATE_GETTING_UP:
            case SandBoxActorPackageData::SANDBOX_STATE_MOVING: {
                pPackData->eState = SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF;
                goto LABEL_6;
            }
            case SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF: {
            LABEL_6:;
                if (apActor->IsPathingComplete()) {
                    TESObjectREFR* Thread = pPackData->pLocation;
                    if (pPackData->MoveToNextLocation()) {
                        TESObjectREFR* apPathingRequest = pPackData->pLocation;
                        if ((apPathingRequest) && (apPathingRequest != Thread)) {
                            TESObjectCELL* cell = Thread->GetParentCell();
                            TESObjectCELL* apCell = apPathingRequest->GetParentCell();
                            bool v9 = (!cell || !TES::pInstance->IsCellLoaded(cell, false)) &&
                                (!apCell || !TES::pInstance->IsCellLoaded(apCell, false));
                            bool v8 = cell && cell == apCell;
                            if (v9) {
                                const NiPoint3* v5 = &apPathingRequest->GetLocationOnReference();
                                apActor->SetPosition(*v5);
                                if (!v8) {
                                    TESWorldSpace* WorldSpace = apPathingRequest->GetWorldSpace();
                                    TESObjectREFR::MoveRefToNewSpace(apActor, apCell, WorldSpace);
                                }
                            }
                            else {
                                TESPackage* v7 = GetPackageThatIsRunning();
                                float LocationRadius = static_cast<float>(v7->GetLocationRadius(apActor));
                                float v12 = BSMAX(LocationRadius, 10.0f);
                                apActor->SetPathfindingGoal(
                                    apPathingRequest, v12, static_cast<PathingAvoidNodeArray*>(nullptr)
                                );
                            }
                        }
                    }
                }
                break;
            }
            default: {
                return;
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
        TESPackage* v2 = GetRunOncePackage();
        if (v2->GetPackType() == PACKAGE_TYPE_GUARD) {
            pdata = static_cast<GuardActorPackageData*>(GetRunOncePackageInstanceData());
        }
    }
    if (pdata) {
        TESObjectREFR* pguardedRef_3;
        TESObjectREFR* pguardedRef_2;
        TESPackage* ppack = GetPackageThatIsRunning();
        TESObjectREFR* pguardedRef = ppack->ComputeInitialTarget(apActor);
        if (pguardedRef) {
            pguardedRef_2 = pguardedRef;
        }
        else {
            pguardedRef_2 = apActor;
        }
        TESObjectREFR* pholder = (&pguardedRef_2->m_Extra)->GetReferencePointer();
        if (pholder) {
            pguardedRef_3 = pholder;
        }
        else {
            pguardedRef_3 = pguardedRef_2;
        }
        if (GetTarget() != pguardedRef_3) {
            SetTarget(pguardedRef_3);
            (&pguardedRef_3->m_Extra)->AddGuard(apActor);
            pguardedRef_3->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }
        if (!IsRunningRunOnce() &&
            (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
             apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
            pdata->eState = GuardActorPackageData::GUARD_STATE_INIT;
            apActor->InitiateGetUpPackage();
        }
        else {
            if (!apActor->IsImmobile()) {
                bool atLoc;
                if (ppack->pPackLoc) {
                    atLoc = ppack->IsActorAtLocation(apActor, false, -1.0f, false);
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
                    atLoc = static_cast<double>(ftargRadius) >
                        apActor->GetDistanceFromReference(pguardedRef_3, false, false);
                }
                if ((apActor->IsPathingComplete() && !atLoc) || apActor->DidPathingFail()) {
                    ProcessGuardStartTravelToLocation(apActor, pdata);
                }
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
    TESWorldSpace* pworldspace__pcell = nullptr;
    TESPackage* ppack = GetPackageThatIsRunning();
    PackageLocation* ploc = ppack->pPackLoc;
    PackageTarget* ptarg = ppack->GetPackageTarget();
    bool buseTargetAsDefault = true;
    if (ploc) {
        if (ploc->GetLocType() == PLT_EDITORLOC) {
            location = apActor->GetEditorLocationCoord();
            pcell = apActor->GetEditorLocationInteriorCell();
            pworldspace__pcell = apActor->GetEditorLocationWorld();
            buseTargetAsDefault = false;
        }
        else {
            if (ploc->GetLocType() == PLT_CURRLOC) {
                preferenceForLocation = apActor;
            }
            else {
                if (ploc->GetLocType() == PLT_LINKEDREF) {
                    preferenceForLocation = apActor->GetLinkedRef();
                }
                else {
                    preferenceForLocation = ploc->GetLocReference();
                    if (!preferenceForLocation && !GetRunOncePackage()) {
                        preferenceForLocation = pGenericLocation;
                    }
                }
            }
        }
    }
    if (!preferenceForLocation && buseTargetAsDefault) {
        preferenceForLocation = GetTarget();
    }
    if (ploc) {
        fguardRadius = static_cast<float>(ppack->GetLocationRadius(apActor));
    }
    else {
        float fguardRadius_2;
        if (ptarg) {
            fguardRadius_2 = static_cast<float>(ptarg->iValue);
        }
        else {
            fguardRadius_2 = 30.0f;
        }
        fguardRadius = fguardRadius_2;
    }
    float v7 = fguardRadius * 0.5;
    float fguardRadius_3 = BSMAX(v7, 15.0f);
    if (preferenceForLocation) {
        PathingRequest request;
        TESWorldSpace* WorldSpace = preferenceForLocation->GetWorldSpace();
        TESObjectCELL* ParentCell = preferenceForLocation->GetParentCell();
        const NiPoint3* v3 = &preferenceForLocation->GetLocationOnReference();
        apActor->BuildRequest(&request, *v3, ParentCell, WorldSpace, fguardRadius_3, nullptr);
        request.SetSoftGoalRadius(false);
        apActor->SetPathfindingGoal(&request);
        apData->eState = GuardActorPackageData::GUARD_STATE_IDLE;
    }
    else {
        if (pcell || pworldspace__pcell) {
            PathingRequest request_2;
            apActor->BuildRequest(&request_2, location, pcell, pworldspace__pcell, fguardRadius_3, nullptr);
            request_2.SetSoftGoalRadius(false);
            apActor->SetPathfindingGoal(&request_2);
            apData->eState = GuardActorPackageData::GUARD_STATE_IDLE;
        }
    }
}

void LowProcess::ProcessPatrol(Actor* apActor) {
    PatrolActorPackageData* pPackageData =
        static_cast<PatrolActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
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
                    else {
                        if (eState == 2) {
                            fTimeDelta = Patrol_AtRefLow(apActor, pPackageData, fTimeDelta);
                        }
                        else {
                            fTimeDelta = 0.0;
                        }
                    }
                }
                else {
                    if (!Patrol_StartLow(apActor, pPackageData)) {
                        fTimeDelta = 0.0;
                    }
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
    AddToProcedureIndexRunning(apActor, (uint32_t)1);
}

bool LowProcess::CheckIfHasObject(Actor* apActor, int32_t ainumb) {
    PACKAGE_OBJECT_TYPE eUseItem;
    TESPackage* pPack = nullptr;
    pPack = GetCurrentPackage();
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
                uint32_t CastingType = pref->iFormID;
                bdone = apActor->HasObjects(pref->GetObjectReference(), POTYPE_NONE, 1, CastingType, eUseItem);
            }
            else {
                bdone = false;
            }
            break;
        }
        case PTT_OBJECT: {
            TESBoundObject* TargObject = static_cast<TESBoundObject*>(ptarg->GetTargObject());
            bdone = apActor->HasObjects(TargObject, POTYPE_NONE, ainumb, 0, eUseItem);
            break;
        }
        case PTT_TYPE: {
            PACKAGE_OBJECT_TYPE TargObjectType = ptarg->GetTargObjectType();
            bdone = apActor->HasObjects(nullptr, TargObjectType, ainumb, 0, eUseItem);
            break;
        }
        default: {
            return bdone;
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
            else {
                if (apPackage->GetMustComplete()) {
                    if (ObjectList.IsEmpty()) {
                        switch (apPackage->GetPackType()) {
                            case PACKAGE_TYPE_FOLLOW:
                            case PACKAGE_TYPE_ESCORT:
                            case PACKAGE_TYPE_ACCOMPANY: {
                                if (!apPackage->IsTargetAnActor(apActor)) {
                                    goto LABEL_31;
                                }
                                preturntarget = apPackage->GetEscortTargetForPackage(apActor, false);
                                break;
                            }
                            case PACKAGE_TYPE_PATROL: {
                            LABEL_31:;
                                PatrolActorPackageData* v19 = nullptr;
                                if (bpackagesmatch) {
                                    PatrolActorPackageData* pdata_2 = static_cast<PatrolActorPackageData*>(
                                        BaseProcess::GetInstanceDataThatIsRunning()
                                    );
                                    if (pdata_2) {
                                        preturntarget = pdata_2->GetCurrentReference();
                                    }
                                }
                                if (!preturntarget) {
                                    preturntarget =
                                        PatrolActorPackageData::GetInitialReferenceForSetup(apActor, apPackage);
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
                                            for (int32_t inum = 0;
                                                 plist && (!plist->IsEmpty() && sFormListNumb <= inum); ++inum) {
                                                TESForm* pform = plist->m_item;
                                                plist = plist->m_pkNext;
                                            }
                                        }
                                        if (pForm) {
                                            pObjecttoAcquire = static_cast<TESBoundObject*>(pForm);
                                        }
                                        eFormType = POTYPE_NONE;
                                    }
                                    else {
                                        if (pForm->GetFormType() == FACT_ID) {
                                            pFactiontoAquire = static_cast<TESFaction*>(pForm);
                                            eFormType = POTYPE_ACTOR;
                                        }
                                        else {
                                            pObjecttoAcquire = static_cast<TESBoundObject*>(ptarget->GetTargObject());
                                            eFormType = POTYPE_NONE;
                                        }
                                    }
                                }
                                else {
                                    if (etype == PTT_TYPE) {
                                        pObjecttoAcquire = nullptr;
                                        eFormType = ptarget->GetTargObjectType();
                                    }
                                }
                                float facquireDist = 0.0f;
                                if (apPackage->GetPackageSearchLocation()) {
                                    float facquireDist_2;
                                    uint32_t iradius = apPackage->GetSearchLocationRadius(apActor);
                                    pCell = apPackage->GetSearchLocationCell(apActor);
                                    pointB = apPackage->GetSearchLocationCoord(apActor);
                                    if (iradius) {
                                        facquireDist_2 = static_cast<float>(iradius);
                                    }
                                    else {
                                        float facquireDist_3;
                                        if (pCell->GetInterior()) {
                                            facquireDist_3 = 3.4028235e+38f;
                                        }
                                        else {
                                            facquireDist_3 = fAIAcquireObjectDistance.Float();
                                        }
                                        facquireDist_2 = facquireDist_3;
                                    }
                                    facquireDist = facquireDist_2;
                                    pDataHandler->EnumReferencesCloseToPoint(
                                        pCell, pointB, facquireDist_2, pointB, facquireDist_2,
                                        &LowProcess_EvaluateReferenceforAcquire, apActor
                                    );
                                }
                                else {
                                    if (!apPackage->GetAcquireRadius(apActor, facquireDist)) {
                                        facquireDist = fAIAcquireObjectDistance.Float();
                                    }
                                    float afRadius2 = facquireDist;
                                    const NiPoint3* v5 = &apActor->GetLocationOnReference();
                                    pDataHandler->EnumReferencesCloseToPoint(
                                        pCell, pointB, facquireDist, *v5, afRadius2,
                                        &LowProcess_EvaluateReferenceforAcquire, apActor
                                    );
                                }
                                eFormType = POTYPE_NONE;
                                pObjecttoAcquire = nullptr;
                                EvaluateOrderAcquireList(apActor);
                                PackageTarget* ptarg_2 = apPackage->GetPackageTarget();
                                if ((ptarg_2) && (ptarg_2->GetTargType())) {
                                    iNumberItemsActivate = apPackage->GetInitialTargetCount();
                                }

                                break;
                            }
                        }
                    }
                    if (!ObjectList.IsEmpty()) {
                        pAcquireObject = (&ObjectList)->m_item;
                        if (pAcquireObject->eType == ACQUIRE_TYPE_BUY_NPC) {
                            if (pAcquireObject->pRef->IsActor()) {
                                preturntarget = pAcquireObject->pRef;
                            }
                            else {
                                TESForm* Owner = pAcquireObject->pRef->GetOwner();
                                TESNPC* pnpc = dynamic_cast<TESNPC*>(Owner);
                                if (pnpc) {
                                    preturntarget =
                                        (TESObjectREFR*)ReferenceProcessLists.GetActorRefInHigh(pnpc, false);
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
        }
        else {
            if (!ptarg->GetTargReference()) {
                return preturntarget;
            }
            TESObjectREFR* preturntarget_2 = ptarg->GetTargReference();
            if (preturntarget_2->IsDead(true)) {
                TESObjectREFR* preturntarget_3 = ptarg->GetTargReference();
                apPackage->SetNeverRun(preturntarget_3, true);
                return preturntarget;
            }
            switch (apPackage->GetPackType()) {
                case PACKAGE_TYPE_FOLLOW:
                case PACKAGE_TYPE_ESCORT:
                case PACKAGE_TYPE_ACCOMPANY: {
                    if (!apPackage->IsTargetAnActor(apActor)) {
                        goto LABEL_12;
                    }
                    preturntarget = apPackage->GetEscortTargetForPackage(apActor, false);
                    break;
                }
                case PACKAGE_TYPE_PATROL: {
                LABEL_12:;
                    PatrolActorPackageData* pdata =
                        static_cast<PatrolActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
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
            if ((preturntarget) && (preturntarget->GetDelete() && !preturntarget->IsActor())) {
                // Tests this address (&preturntarget->m_Extra), which is never null, as the game does.
                preturntarget = (&preturntarget->m_Extra)->GetReferencePointer();
            }
        }
    }
    if ((preturntarget) && (preturntarget->GetDelete()) && ((&preturntarget->m_Extra)->GetReferencePointer())) {
        return (&preturntarget->m_Extra)->GetReferencePointer();
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
                else {
                    if (etype == PLT_TYPE) {
                        pObjectForLocation = nullptr;
                        eFormType = pLoc->GetLocObjectType();
                    }
                }
                float facquireDist = 0.0f;
                if (!apPackage || !apPackage->GetAcquireRadius(apActor, facquireDist)) {
                    facquireDist = fAIAcquireObjectDistance.Float();
                }
                float afRadius2 = facquireDist;
                const NiPoint3* v4 = &apActor->GetLocationOnReference();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell, pointB, facquireDist, *v4, afRadius2, &EvaluateGenericLocation, apActor
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
    bool ballowKilling;
    bool ballowStealing;
    bool ballowBuying;
    BSSimpleList<ObjectstoAcquire*>* pRefItem = &TempObjectList;
    TESPackage* ppack = GetPackageThatIsRunning();
    if (ppack) {
        ballowBuying = ppack->SearchGetAllowBuying();
    }
    else {
        ballowBuying = false;
    }
    if (ppack) {
        ballowStealing = ppack->SearchGetAllowStealing();
    }
    else {
        ballowStealing = false;
    }
    if (ppack) {
        ballowKilling = ppack->SearchGetAllowKilling();
    }
    else {
        ballowKilling = false;
    }
    while (pRefItem && pRefItem->m_item) {
        TESBoundObject* v3;
        ObjectstoAcquire* pObject = pRefItem->m_item;
        TESObjectREFR* pRef = pObject->pRef;
        TESObjectREFR* pactor = nullptr;
        TESBoundObject* pObj = pObject->pObject;
        Actor* bbuyit = pRef->IsActor() ? static_cast<Actor*>(pRef) : nullptr;
        char v26 = 0;
        bool badd = true;
        if (bbuyit) {
            badd = !bbuyit->IsPlayerTeammate();
        }
        if ((pRef->GetOwner() &&
             (v3 = static_cast<TESBoundObject*>(pRef->GetOwner()), v3 != apActor->GetObjectReference())) ||
            pRef->IsActor()) {
            bool bsteal = false;
            bool bkill = false;
            int32_t ivalue = TESValueForm::GetFormValue(pObj);
            TESNPC* pnpc = nullptr;
            if (!pRef->IsDead(false)) {
                int32_t GoldAmount = apActor->GetGoldAmount();
                if (GoldAmount >= ivalue) {
                    Actor* pbuyactor = nullptr;
                    if (pRef->IsNpc()) {
                        pbuyactor = static_cast<Actor*>(pRef);
                    }
                    else {
                        TESForm* Owner = pRef->GetOwner();
                        pnpc = dynamic_cast<TESNPC*>(Owner);
                        if (pnpc) {
                            pbuyactor = ReferenceProcessLists.GetActorRefInHigh(pnpc, false);
                        }
                    }
                    if ((pbuyactor) && (ballowBuying && pbuyactor->GetActorBase()->SellBuysItem(pObj))) {
                        if (apActor->IsInCombat()) {
                            badd = false;
                        }
                        else {
                            pObject->eType = ACQUIRE_TYPE_BUY_NPC;
                            uint32_t DistanceFromReference =
                                static_cast<int32_t>(apActor->GetDistanceFromReference(pRef, false, false));
                            pObject->iWantValue = DistanceFromReference;
                            v26 = 1;
                        }
                    }
                }
            }
            if (!v26 && (!bbuyit || !bbuyit->IsPlayerTeammate())) {
                if (pRef->IsActor()) {
                    if (pRef->IsDead(false)) {
                        badd = true;
                    }
                    else {
                        if (pRef->IsNpc()) {
                            pactor = pRef;
                            if (ballowStealing) {
                                bsteal = true;
                            }
                            if (ballowKilling) {
                                bkill = true;
                            }
                        }
                    }
                }
                else {
                    if (ballowStealing) {
                        bsteal = true;
                    }
                }
                if ((!bsteal && !bkill) || (bbuyit && bbuyit->IsPlayerTeammate())) {
                    if (apActor->IsCreature()) {
                        badd = false;
                    }
                    else {
                        TESBoundObject* v7;
                        if (!pRef->IsActor() ||
                            (pRef->IsNpc() ||
                             ((v7 = static_cast<TESBoundObject*>(pRef->GetOwner()),
                               v7 == apActor->GetObjectReference()) ||
                              apActor->bPlayerTeammate))) {
                            badd = false;
                        }
                        else {
                            if ((bbuyit) && (!bbuyit->IsPlayerTeammate())) {
                                pObject->eType = ACQUIRE_TYPE_KILL;
                                badd = true;
                            }
                        }
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
                        else {
                            if (bkill) {
                                if (apActor->IsInCombat() || apActor->bPlayerTeammate) {
                                    badd = false;
                                }
                                else {
                                    if (!bbuyit || !bbuyit->IsPlayerTeammate()) {
                                        pObject->eType = ACQUIRE_TYPE_KILL;
                                    }
                                }
                            }
                        }
                    }
                    else {
                        if (pObject->eType == ACQUIRE_TYPE_NOOWNED_CONTAINER) {
                            PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
                            pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
                            if ((!apActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem)) && (pRef->GetLock())) {
                                badd = false;
                            }
                        }
                        else {
                            pObject->eType = ACQUIRE_TYPE_STEALGROUND;
                        }
                    }
                    pObject->bSteal = bsteal;
                    pObject->bKill = bkill;
                }
            }
        }
        else {
            double v8 = apActor->GetDistanceFromReference(pRef, false, false);
            pObject->iWantValue = static_cast<int32_t>(v8);
        }
        if (badd) {
            pObject->pRef->SetTargeted(true);
            ObjectList.AddTail(pObject);
        }
        else {
            if (pObject) {
                pObject->Delete(1u);
            }
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
    apData->fCurrentTimeAtReference = (float)0.0;
    apData->eState = PatrolActorPackageData::PATROL_STATE_AT_REF;
}

bool LowProcess::UpdatePackageTargetForCombat(Actor* apActor, TESPackage* apPackage) {
    ExtraDataList* v4;
    ExtraDataList* v6;
    TESObjectREFR* v7;
    TESObjectREFR* currentReference;
    if (((int32_t)apPackage->GetPackType()) != 13) {
        return false;
    }
    ExtraDataList* extraDataList = apActor->TESObjectREFR::GetExtra();
    PatrolActorPackageData* packageDataData =
        static_cast<PatrolActorPackageData*>(extraDataList->GetActorPackageData());
    if (!packageDataData) {
        return false;
    }
    if (packageDataData->eState) {
        if (packageDataData->SetNextReferenceForPatrol(true)) {
            currentReference = packageDataData->GetCurrentReference();
            v6 = apActor->TESObjectREFR::GetExtra();
            v6->SetPackageExtraTarget(currentReference);
            return true;
        }
    }
    else {
        if (packageDataData->Init(apActor, apPackage)) {
            packageDataData->eState = PatrolActorPackageData::PATROL_STATE_TRAVELING_TO_NEXT_REF;
            v7 = packageDataData->GetCurrentReference();
            v4 = apActor->TESObjectREFR::GetExtra();
            v4->SetPackageExtraTarget(v7);
            return true;
        }
    }
    return false;
}

float LowProcess::GetTimeDelta(Actor* apActor) {
    TESGlobal* formByNumericID = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58));
    float hour = (&appCalendar)->GetHour();
    if (hour < BaseProcess::GetLastTimeProcessed()) {
        hour = (float)(hour + 24.0);
    }
    float v4 = hour - BaseProcess::GetLastTimeProcessed();
    return (float)((3600.0 / formByNumericID->TESGlobal::GetValue()) * v4);
}

void LowProcess::Patrol_StartTravelToRefLow(Actor* a1, PatrolActorPackageData* a2) {
    TESObjectREFR* CurrentReference = a2->GetCurrentReference();
    if (CurrentReference) {
        SetTarget(CurrentReference);
        bool v16 = false;
        if (pPlayer->IsSleepingorResting()) {
            v16 = true;
        }
        else {
            if (!a1->Get3D()) {
                TESObjectCELL* cell = a1->GetParentCell();
                TESObjectCELL* apCell = CurrentReference->GetParentCell();
                bool v8 = (!cell || !TES::pInstance->IsCellLoaded(cell, false)) &&
                    (!apCell || !TES::pInstance->IsCellLoaded(apCell, false));
                v16 = v8;
            }
        }
        if (!v16) {
            float DefaultRadius = PathingRequest::GetDefaultRadius();
            TESPackage* v11 = GetPackageThatIsRunning();
            if (v11) {
                float a2a = static_cast<float>(v11->GetLocationRadius(a1));
                DefaultRadius = BSMAX(DefaultRadius, a2a);
            }
            PathingRequest apRequest;
            TESWorldSpace* WorldSpace = CurrentReference->GetWorldSpace();
            TESObjectCELL* ParentCell = CurrentReference->GetParentCell();
            const NiPoint3* v3 = &CurrentReference->GetLocationOnReference();
            a1->BuildRequest(&apRequest, *v3, ParentCell, WorldSpace, DefaultRadius, nullptr);
            apRequest.SetSoftGoalRadius(false);
            TESBoundObject* ObjectReference = CurrentReference->GetObjectReference();
            if (ObjectReference->GetFormType() == IDLM_ID || ObjectReference == pXMarkerHeading) {
                apRequest.SetGoalAngleToFace((&CurrentReference->GetAngleOnReference())->z);
            }
            a1->SetPathfindingGoal(&apRequest);
        }
        a2->eState = PatrolActorPackageData::PATROL_STATE_TRAVELING_TO_NEXT_REF;
    }
    else {
        SetProcedureIndexRunning(-1);
    }
}

char LowProcess::Patrol_StartLow(Actor* apActor, PatrolActorPackageData* apData) {
    TESPackage* v3 = GetCurrentPackage();
    if (apData->Init(apActor, v3)) {
        Patrol_StartTravelToRefLow(apActor, apData);
        return 1;
    }

    SetProcedureIndexRunning(0xFFFFFFFFu);
    return 0;
}

void LowProcess::UpdateTarget() {
    TESPackage* v1;
    TESPackage* v2;
    PackageTarget* packageTarget;
    TESPackage* v4;
    PackageTarget* v5;
    TESObjectREFR* targReference;
    if (GetCurrentPackage()) {
        v1 = GetCurrentPackage();
        if (v1->GetPackageTarget()) {
            v2 = GetCurrentPackage();
            packageTarget = v2->GetPackageTarget();
            if (packageTarget->GetTargReference()) {
                v4 = GetCurrentPackage();
                v5 = v4->GetPackageTarget();
                targReference = v5->GetTargReference();
                if ((!targReference->GetDelete()) && (targReference != pTarget)) {
                    SetTarget(targReference);
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
        TESPackage* v3;
        if (!GetCurrentPackage()->pPackLoc || (v3 = GetCurrentPackage(), v3->pPackLoc->GetLocType() == PLT_CURRLOC)) {
            float a5 = apActor->GetAngleOnReference().z;
            const NiPoint3* v6 = &apActor->GetLocationOnReference();
            TESObjectCELL* ParentCell = apActor->GetParentCell();
            TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
            apActor->SetPackageStartLocation(WorldSpace, ParentCell, *v6, a5);
        }
    }
    CombatController* v9 = apActor->GetCombatController();
    if (v9) {
        v9->UpdatePackageData(pNewPackage);
    }
    CheckAndStartCombatControllerForPackage(apActor, pNewPackage, false);
    return true;
}

bool LowProcess::ShouldStopCombatControllerToFinishPackage(TESPackage* apPackage, int32_t aiIndex) {
    uint8_t result;
    switch ((int32_t)apPackage->GetPackType()) {
        case 0:
        case 1:
        case 6:
        case 7:
        case 13:
        case 14:
        case 17: {
            result = 0;
            break;
        }
        default: {
            result = (uint8_t)((((NiNode*)apPackage->TESPackage::GetProcedureType()) == ((NiNode*)(-1))) ||
                               (Procedure_Action[(uint32_t)apPackage->TESPackage::GetProcedureType()][aiIndex] != 54));
            break;
        }
    }
    return (bool)result;
}

void LowProcess::ProcessKill(Actor* apActor) {
    if (pTarget) {
        if (pTarget->IsDead(false)) {
            ProcessActivate(apActor, false);
        }
        else {
            EnterCombat(
                apActor, static_cast<Actor*>(pTarget), false, false, nullptr, false, true, true, 0, false, false, true,
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
        if (pAcquireObject) {
            pAcquireObject->Delete(1u);
        }
        pAcquireObject = nullptr;
    }
    if (pSavedAcquireObject) {
        pSavedAcquireObject->Delete(1u);
        pSavedAcquireObject = nullptr;
    }
    while (!ObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj = (&ObjectList)->m_item;
        ObjectList.Remove(pobj);
        if (pobj) {
            pobj->Delete(1u);
        }
    }
    GenericLocationsList.RemoveAll();
    pGenericLocation = nullptr;
    while (!TempObjectList.IsEmpty()) {
        ObjectstoAcquire* pobj_2 = (&TempObjectList)->m_item;
        TempObjectList.Remove(pobj_2);
        if (pobj_2) {
            pobj_2->Delete(1u);
        }
    }
    TempLocList.RemoveAll();
    if (pObjecttoAcquire) {
        delete pObjecttoAcquire;
    }
    pObjecttoAcquire = nullptr;
}

void LowProcess::BuildMulitTargetObjectList(Actor* apActor, TESObjectREFR* pref, PACKAGE_OBJECT_TYPE eType) {
    uint32_t iradius_2;
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
                int32_t icount_3 = pchange->GetCount(true);
                ItemChange* pitem_5 = nullptr;
                for (int32_t i_3 = 0; i_3 < icount_3; ++i_3) {
                    pitem_5 = pchange->GetItembyIndex(i_3);
                    if (pitem_5) {
                        if (dynamic_cast<TESObjectWEAP*>(pitem_5->pContainerObj)) {
                            ObjectstoAcquire* pacquire_5;
                            ObjectstoAcquire* v37 = static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
                            if (v37) {
                                pacquire_5 = ::new (v37) ObjectstoAcquire;
                            }
                            else {
                                pacquire_5 = nullptr;
                            }
                            ObjectstoAcquire* v38 = pacquire_5;
                            ObjectstoAcquire* pacquire_6 = pacquire_5;
                            TESBoundObject* FormFlags = pitem_5->pContainerObj;
                            pacquire_6->pObject = FormFlags;
                            uint32_t FormHealth_0 = pitem_5->iNumber;
                            pacquire_6->iamount = FormHealth_0;
                            pacquire_6->pRef = pref;
                            pacquire_6->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire_6);
                        }
                        ItemChange* v35 = pitem_5;
                        ItemChange* pitem_6 = pitem_5;
                        if (pitem_5) {
                            delete pitem_6;
                        }
                        pitem_5 = nullptr;
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
                        if (parmo || (pweap || pammo)) {
                            ObjectstoAcquire* pacquire;
                            void* apMemory = ourMalloc(sizeof(ObjectstoAcquire));
                            if (apMemory) {
                                pacquire = ::new (apMemory) ObjectstoAcquire;
                            }
                            else {
                                pacquire = nullptr;
                            }
                            ObjectstoAcquire* v46 = pacquire;
                            ObjectstoAcquire* pacquire_2 = pacquire;
                            TESBoundObject* v4 = pitem->pContainerObj;
                            pacquire_2->pObject = v4;
                            uint32_t v5 = pitem->iNumber;
                            pacquire_2->iamount = v5;
                            pacquire_2->pRef = pref;
                            pacquire_2->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire_2);
                        }
                        ItemChange* v43 = pitem;
                        ItemChange* pitem_2 = pitem;
                        if (pitem) {
                            delete pitem_2;
                        }
                        pitem = nullptr;
                    }
                }
                break;
            }
            case POTYPE_WEAR: {
                int32_t icount_2 = pchange->GetCount(true);
                ItemChange* pitem_3 = nullptr;
                for (int32_t i_2 = 0; i_2 < icount_2; ++i_2) {
                    pitem_3 = pchange->GetItembyIndex(i_2);
                    if (pitem_3) {
                        TESObjectARMO* parmo_2 = dynamic_cast<TESObjectARMO*>(pitem_3->pContainerObj);
                        TESObjectWEAP* pweap_2 = dynamic_cast<TESObjectWEAP*>(pitem_3->pContainerObj);
                        TESAmmo* pammo_2 = dynamic_cast<TESAmmo*>(pitem_3->pContainerObj);
                        TESObjectCLOT* pcloth = dynamic_cast<TESObjectCLOT*>(pitem_3->pContainerObj);
                        if (parmo_2 || (pweap_2 || (pammo_2 || pcloth))) {
                            ObjectstoAcquire* pacquire_3;
                            ObjectstoAcquire* v41 = static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
                            if (v41) {
                                pacquire_3 = ::new (v41) ObjectstoAcquire;
                            }
                            else {
                                pacquire_3 = nullptr;
                            }
                            ObjectstoAcquire* v42 = pacquire_3;
                            ObjectstoAcquire* pacquire_4 = pacquire_3;
                            TESBoundObject* v6 = pitem_3->pContainerObj;
                            pacquire_4->pObject = v6;
                            uint32_t v7 = pitem_3->iNumber;
                            pacquire_4->iamount = v7;
                            pacquire_4->pRef = pref;
                            pacquire_4->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire_4);
                        }
                        ItemChange* v39 = pitem_3;
                        ItemChange* pitem_4 = pitem_3;
                        if (pitem_3) {
                            delete pitem_4;
                        }
                        pitem_3 = nullptr;
                    }
                }
                break;
            }
            case POTYPE_WEAP_MELEE: {
                int32_t icount_4 = pchange->GetCount(true);
                ItemChange* pitem_7 = nullptr;
                for (int32_t i_4 = 0; i_4 < icount_4; ++i_4) {
                    pitem_7 = pchange->GetItembyIndex(i_4);
                    if (pitem_7) {
                        TESObjectWEAP* pweap_3 = dynamic_cast<TESObjectWEAP*>(pitem_7->pContainerObj);
                        if ((pweap_3) && (pweap_3->IsMeleeWeapon())) {
                            ObjectstoAcquire* pacquire_7;
                            ObjectstoAcquire* v33 = static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
                            if (v33) {
                                pacquire_7 = ::new (v33) ObjectstoAcquire;
                            }
                            else {
                                pacquire_7 = nullptr;
                            }
                            ObjectstoAcquire* v34 = pacquire_7;
                            ObjectstoAcquire* pacquire_8 = pacquire_7;
                            TESBoundObject* v10 = pitem_7->pContainerObj;
                            pacquire_8->pObject = v10;
                            uint32_t v11 = pitem_7->iNumber;
                            pacquire_8->iamount = v11;
                            pacquire_8->pRef = pref;
                            pacquire_8->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire_8);
                        }

                        ItemChange* v31 = pitem_7;
                        ItemChange* pitem_8 = pitem_7;
                        if (pitem_7) {
                            delete pitem_8;
                        }
                        pitem_7 = nullptr;
                    }
                }
                break;
            }
            case POTYPE_WEAP_RANGED: {
                int32_t icount_5 = pchange->GetCount(true);
                ItemChange* pitem_9 = nullptr;
                for (int32_t i_5 = 0; i_5 < icount_5; ++i_5) {
                    pitem_9 = pchange->GetItembyIndex(i_5);
                    if (pitem_9) {
                        TESObjectWEAP* pweap_4 = dynamic_cast<TESObjectWEAP*>(pitem_9->pContainerObj);
                        if ((pweap_4) && (pweap_4->IsRangedWeapon())) {
                            ObjectstoAcquire* pacquire_9;
                            ObjectstoAcquire* v29 = static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
                            if (v29) {
                                pacquire_9 = ::new (v29) ObjectstoAcquire;
                            }
                            else {
                                pacquire_9 = nullptr;
                            }
                            ObjectstoAcquire* v30 = pacquire_9;
                            ObjectstoAcquire* pacquire_10 = pacquire_9;
                            TESBoundObject* v12 = pitem_9->pContainerObj;
                            pacquire_10->pObject = v12;
                            uint32_t v13 = pitem_9->iNumber;
                            pacquire_10->iamount = v13;
                            pacquire_10->pRef = pref;
                            pacquire_10->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            ObjectList.AddHead(pacquire_10);
                        }

                        ItemChange* v27 = pitem_9;
                        ItemChange* pitem_10 = pitem_9;
                        if (pitem_9) {
                            delete pitem_10;
                        }
                        pitem_9 = nullptr;
                    }
                }
                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        TESPackage* v14;
        TESObjectCELL* pCell = apActor->GetParentCell();
        NiPoint3 pointB = apActor->GetLocationOnReference();
        eFormType = eType;
        float facquireDist = 0.0f;
        TESPackage* pPack = GetPackageThatIsRunning();
        if (pCell && (v14 = GetCurrentPackage(), v14->GetPackageSearchLocation())) {
            uint32_t iradius = pPack->GetSearchLocationRadius(apActor);
            pCell = pPack->GetSearchLocationCell(apActor);
            pointB = pPack->GetSearchLocationCoord(apActor);
            if (pCell) {
                if (iradius) {
                    *reinterpret_cast<float*>(&iradius_2) = static_cast<float>(iradius);
                }
                else {
                    float facquireDist_2;
                    if (pCell->GetInterior()) {
                        facquireDist_2 = 3.4028235e+38f;
                    }
                    else {
                        facquireDist_2 = fAIAcquireObjectDistance.Float();
                    }
                    *reinterpret_cast<float*>(&iradius_2) = facquireDist_2;
                }
                facquireDist = *reinterpret_cast<float*>(&iradius_2);
            }
            else {
                Warning("Package on %s is using an invalid Search Location\n", apActor->GetFullName());
            }
            pDataHandler->EnumReferencesCloseToPoint(
                pCell, pointB, facquireDist, pointB, facquireDist, &LowProcess_EvaluateReferenceforAcquire, apActor
            );
        }
        else {
            TESPackage* v15;
            if (!GetCurrentPackage() || (v15 = GetCurrentPackage(), !v15->GetAcquireRadius(apActor, facquireDist))) {
                facquireDist = fAIAcquireObjectDistance.Float();
            }
            float afRadius2 = facquireDist;
            const NiPoint3* v16 = &apActor->GetLocationOnReference();
            pDataHandler->EnumReferencesCloseToPoint(
                pCell, pointB, facquireDist, *v16, afRadius2, &LowProcess_EvaluateReferenceforAcquire, apActor
            );
        }
        eFormType = POTYPE_NONE;
        pObjecttoAcquire = nullptr;
        pFactiontoAquire = nullptr;
        EvaluateOrderAcquireList(apActor);
    }
}

double LowProcess::Patrol_TravelingToNextRefLow(Actor* apActor, PatrolActorPackageData* apData, float afTimeRemaining) {
    TESObjectREFR* apRef = apData->GetCurrentReference();
    if (apRef) {
        float v25 = afTimeRemaining;
        char v26 = 0;
        TESObjectCELL* cell = apActor->GetParentCell();
        TESObjectCELL* apCell = apRef->GetParentCell();
        bool v9 = cell && cell == apCell;
        if (pPlayer->IsSleepingorResting()) {
            v26 = 1;
        }
        else {
            bool v8 = (!cell || !TES::pInstance->IsCellLoaded(cell, false)) &&
                (!apCell || !TES::pInstance->IsCellLoaded(apCell, false));
            v26 = v8;
        }
        if (v26 || !apActor->DidPathingFail()) {
            int32_t v22 = 257;
            TESPackage* v23 = GetPackageThatIsRunning();
            bool AlwaysRun = v23->GetAlwaysRun();
            bool v24 = false;
            if (AlwaysRun) {
                float DistanceFromReference = apActor->GetDistanceFromReference(apRef, false, false);
                float v18 = 64.0f;
                v24 = 64.0f < static_cast<double>(DistanceFromReference);
                if (64.0f < static_cast<double>(DistanceFromReference)) {
                    float PackageRadiusActorToLocation = v23->GetPackageRadiusActorToLocation(apActor, false);
                    float v7 = PackageRadiusActorToLocation + PackageRadiusActorToLocation;
                    v22 = CalculateMoveMode(
                        apActor, DistanceFromReference, PackageRadiusActorToLocation, v7, false, false
                    );
                }
            }
            SetActorsAnimation(apActor, v22, v24);
            if (v26) {
                float v15;
                float a1 = apActor->GetCurrentSpeed();
                float v16 = BSMAX(a1, static_cast<float>(1.0));
                if (v9) {
                    const NiPoint3* v13 = &apRef->GetLocationOnReference();
                    NiPoint3 v14 = *v13;
                    const NiPoint3* v5 = &apActor->GetLocationOnReference();
                    v14 -= *v5;
                    v15 = v14.Length();
                }
                else {
                    PathingLocation aOrigin(apActor);
                    PathingLocation aDestination(apRef);
                    v15 = Pathing::ComputeTeleportDoorPathLength(
                        aOrigin, aDestination, 4096.0f, nullptr, Pathing::DOOR_LOCK_DISPOSITION_NORMAL
                    );
                }
                v25 = v25 - v15 / v16;
                apActor->MoveActorAndFollowers(apRef, true, false);
                StartAtPatrolRefBehaviorLow(apActor, apData);
            }
            else {
                if (apActor->IsPathingComplete()) {
                    StartAtPatrolRefBehaviorLow(apActor, apData);
                }
                else {
                    return static_cast<float>(0.0);
                }
            }
            return v25;
        }

        Patrol_StartTravelToRefLow(apActor, apData);
        return v25;
    }

    SetProcedureIndexRunning(-1);
    return 0.0;
}

double LowProcess::Patrol_AtRefLow(Actor* apActor, PatrolActorPackageData* apData, float afTimeRemaining) {
    TESObjectREFR* CurrentReference = apData->GetCurrentReference();
    if (!CurrentReference) {
        SetProcedureIndexRunning(-1);
        return 0.0;
    }
    float DecalMinWidth = 0.0;
    ExtraDataList* Extra = CurrentReference->GetExtra();
    if (Extra->GetPatrolRefData()) {
        ExtraDataList* v6 = CurrentReference->GetExtra();
        PatrolRefData* PatrolRefData_ = v6->GetPatrolRefData();
        DecalMinWidth = PatrolRefData_->GetTimeAtRef();
    }
    float v10 = DecalMinWidth - apData->fCurrentTimeAtReference;
    DecalMinWidth = BSMAX(static_cast<float>(0.0), v10);
    float v14 = afTimeRemaining - DecalMinWidth;
    if (!(v14 > 0.0)) {
        apData->fCurrentTimeAtReference = apData->fCurrentTimeAtReference + afTimeRemaining;
    }
    else {
        if (!apData->SetNextReferenceForPatrol(true)) {
            AddToProcedureIndexRunning(apActor, 1);
            return 0.0;
        }
        if (apData->GetCurrentReference() == CurrentReference) {
            v14 = 0.0;
        }
        else {
            TESObjectREFR* v9 = apData->GetCurrentReference();
            NiPoint3 v12 = v9->GetLocationOnReference();
            const NiPoint3* v8 = &CurrentReference->GetLocationOnReference();
            v12 -= *v8;
            if (v12.SqrLength() < 0.0010000000474974513) {
                v14 = 0.0;
            }
        }
        Patrol_StartTravelToRefLow(apActor, apData);
    }
    return v14;
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

void LowProcess_calcByt030(LowProcess* apThis, [[maybe_unused]] void* apEdx, uint8_t a2, char a3, char a4) {
    apThis->m_uFlags = (uint8_t)((a2 << a4) | ((~a3) & apThis->m_uFlags));
}

bool LowProcess_EvaluateReferenceforAcquire(TESObjectREFR* a1, void* a2) {
    // The filter data is the searching actor.
    Actor* pAcquirer = static_cast<Actor*>(a2);
    if (!a1 || (a1 == pAcquirer || (a1->GetDelete() || (a1->GetTemporary() || a1->GetDisabled())))) {
        return false;
    }
    if (!pAcquirer) {
        return true;
    }
    ObjectstoAcquire* a2a = nullptr;
    bool v34 = false;
    HighProcess* pProcess = nullptr;
    bool v36 = a1->IsActor();
    TESForm* apForm = (TESForm*)a1->GetObjectReference();
    int32_t FormType = apForm->GetFormType();
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
    if (pProcess->eFormType && TESPackage::FormMatchesPackageObjectType(apForm, pProcess->eFormType)) {
        if (pProcess->pFactiontoAquire && v36) {
            if (static_cast<Actor*>(a1)->Actor::IsInFaction(pProcess->pFactiontoAquire)) {
                v34 = true;
            }
        }
        else {
            v34 = true;
        }
    }
    else {
        if (pProcess->pObjecttoAcquire) {
            if (pProcess->pObjecttoAcquire == apForm) {
                v34 = true;
            }
            else {
                if (v36 && (&a1->m_Extra)->GetLevCreaOriginalBase() == pProcess->pObjecttoAcquire) {
                    v34 = true;
                }
            }
        }
        else {
            if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjecttoAcquire) {
                v34 = true;
            }
        }
    }
    if (FormType == 25) {
        if (static_cast<TESObjectBOOK*>(apForm)->TESObjectBOOK::GetCantTake()) {
            v34 = false;
        }
    }
    else {
        if (FormType == 30 && !static_cast<TESObjectLIGH*>(apForm)->TESObjectLIGH::GetCanCarry()) {
            v34 = false;
        }
    }
    char v30 = 0;
    if (FormType == 40) {
        // The weapon's TESHealthForm base, null-tested as a base cast.
        TESHealthForm* pHealthForm = static_cast<TESObjectWEAP*>(apForm);
        if (pAcquirer->IsInCombat()) {
            v30 = 1;
        }
        if ((pHealthForm) && (a1->GetHealth() <= 0.0) && (v30)) {
            v34 = false;
        }
    }
    if (v34) {
        ObjectstoAcquire* v14;
        ObjectstoAcquire* apMemory = static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
        if (apMemory) {
            v14 = ::new (apMemory) ObjectstoAcquire;
        }
        else {
            v14 = nullptr;
        }
        a2a = v14;
        v14->pObject = a1->GetObjectReference();
        a2a->pRef = a1;
        a2a->iamount = (&a1->m_Extra)->GetCount();
        a2a->eType = ACQUIRE_TYPE_NOOWNED_GROUND;
        ExtraDataList* Extra = &a1->m_Extra;
        a2a->pExtra = Extra;
        pProcess->TempObjectList.AddTail(a2a);
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
    if ((bValidItem && (v36 && a1 != pPlayer)) || FormType == 27) {
        InventoryChanges* ContainerChanges = (&a1->m_Extra)->GetContainerChanges();
        if (ContainerChanges) {
            BSSimpleList<ContainerObject*>* ecx0 = nullptr;
            if (ContainerChanges->GetOriginalContainer()) {
                ecx0 = &ContainerChanges->GetOriginalContainer()->objectList;
            }
            while (ecx0 && ecx0->m_item) {
                char v26 = 0;
                ContainerObject* m_item = ecx0->m_item;
                if (m_item) {
                    ItemChange* InventoryItem = ContainerChanges->GetInventoryItem(m_item->pObj, 0);
                    if (InventoryItem) {
                        delete InventoryItem;
                    }
                    else {
                        if (pProcess->eFormType &&
                            TESPackage::FormMatchesPackageObjectType(m_item->pObj, pProcess->eFormType)) {
                            v26 = 1;
                        }
                        else {
                            if (pProcess->pObjecttoAcquire && pProcess->pObjecttoAcquire == m_item->pObj) {
                                v26 = 1;
                            }
                            else {
                                if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjecttoAcquire) {
                                    v26 = 1;
                                }
                            }
                        }
                        if (v26) {
                            TESForm* Space = a1->GetSpace();
                            if (Space == pAcquirer->GetSpace()) {
                                ObjectstoAcquire* v13;
                                ObjectstoAcquire* v17 =
                                    static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
                                if (v17) {
                                    v13 = ::new (v17) ObjectstoAcquire;
                                }
                                else {
                                    v13 = nullptr;
                                }
                                a2a = v13;
                                v13->pObject = m_item->pObj;
                                a2a->pRef = a1;
                                a2a->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                                if (a1->HasContainer()) {
                                    InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(a1);
                                    if (InventoryChanges_) {
                                        uint32_t ObjectCount = InventoryChanges_->GetObjectCount(m_item->pObj);
                                        a2a->iamount = ObjectCount;
                                    }
                                }
                                else {
                                    a2a->iamount = 1;
                                }
                                pProcess->TempObjectList.AddTail(a2a);
                            }
                        }
                    }
                }
                ecx0 = ecx0->m_pkNext;
            }
        }
        int32_t InventoryCount = a1->GetInventoryCount(false, true);
        for (int32_t auiIndex = 0; auiIndex < InventoryCount; ++auiIndex) {
            ItemChange* InventoryItem_005754A0 = a1->GetInventoryItem_005754A0(auiIndex, false);
            if (InventoryItem_005754A0) {
                if (!InventoryItem_005754A0->GetWorn(false)) {
                    char v20 = 0;
                    if (pProcess->eFormType &&
                        TESPackage::FormMatchesPackageObjectType(
                            InventoryItem_005754A0->pContainerObj, pProcess->eFormType
                        )) {
                        v20 = 1;
                    }
                    else {
                        TESBoundObject* FormFlags;
                        if (pProcess->pObjecttoAcquire &&
                            (FormFlags = InventoryItem_005754A0->pContainerObj,
                             pProcess->pObjecttoAcquire == FormFlags)) {
                            v20 = 1;
                        }
                        else {
                            if (pProcess->eFormType == POTYPE_NONE && !pProcess->pObjecttoAcquire) {
                                v20 = 1;
                            }
                        }
                    }
                    if (v20) {
                        TESForm* v8 = a1->GetSpace();
                        if (v8 == pAcquirer->GetSpace()) {
                            ObjectstoAcquire* v12;
                            ObjectstoAcquire* v16 = static_cast<ObjectstoAcquire*>(ourMalloc(sizeof(ObjectstoAcquire)));
                            if (v16) {
                                v12 = ::new (v16) ObjectstoAcquire;
                            }
                            else {
                                v12 = nullptr;
                            }
                            a2a = v12;
                            TESBoundObject* v9 = InventoryItem_005754A0->pContainerObj;
                            a2a->pObject = v9;
                            a2a->pRef = a1;
                            a2a->eType = ACQUIRE_TYPE_NOOWNED_CONTAINER;
                            if (a1->HasContainer()) {
                                InventoryChanges* v19 = InventoryChanges::GetInventoryChanges(a1);
                                if (v19) {
                                    uint32_t v10 = v19->GetObjectCount(a2a->pObject);
                                    a2a->iamount = v10;
                                }
                            }
                            else {
                                a2a->iamount = 1;
                            }
                            pProcess->TempObjectList.AddTail(a2a);
                            if (InventoryItem_005754A0->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* v11 = InventoryItem_005754A0->pExtraObjectList;
                                a2a->pExtra = v11->m_item;
                            }
                        }
                    }
                }
                delete InventoryItem_005754A0;
            }
        }
    }
    return false;
}

// aiFilterData is the searching actor.
bool EvaluateGenericLocation(TESObjectREFR* a1, void* aiFilterData) {
    Actor* a2 = static_cast<Actor*>(aiFilterData);
    if (!a1 ||
        (a1->GetDelete() ||
         (a1->GetTemporary() || (a1->GetDisabled() || (a2->IsChild(false) && !a2->CanChildUseObject(a1)))))) {
        return false;
    }
    if (!a2) {
        return true;
    }
    LowProcess* v9 = dynamic_cast<LowProcess*>(a2->pCurrentProcess);
    if (!v9) {
        return true;
    }
    bool v10 = a1->IsActor();
    int32_t FormType = a1->GetObjectReference()->GetFormType();
    if (v9->eFormType && TESPackage::FormMatchesPackageObjectType(a1->GetObjectReference(), v9->eFormType)) {
        if (!a1->IsDead(false)) {
            a1->SetTargeted(true);
            v9->TempLocList.AddTail(a1);
        }
    }
    else {
        if (v9->pObjectForLocation && v9->pObjectForLocation == a1->GetObjectReference()) {
            if (!a1->IsDead(false)) {
                a1->SetTargeted(true);
                v9->TempLocList.AddTail(a1);
            }
        }
        else {
            if (v9->eFormType == POTYPE_NONE && !v9->pObjectForLocation) {
                a1->SetTargeted(true);
                v9->TempLocList.AddTail(a1);
            }
        }
    }
    if ((v10 && a1 != pPlayer) || FormType == 27) {
        int32_t InventoryCount = a1->GetInventoryCount(false, true);
        for (int32_t auiIndex = 0; auiIndex < InventoryCount; ++auiIndex) {
            ItemChange* InventoryItem_005754A0 = a1->GetInventoryItem_005754A0(auiIndex, false);
            if (InventoryItem_005754A0) {
                if (!InventoryItem_005754A0->GetWorn(false)) {
                    char v3 = 0;
                    if (v9->eFormType &&
                        TESPackage::FormMatchesPackageObjectType(
                            InventoryItem_005754A0->pContainerObj, v9->eFormType
                        )) {
                        v3 = 1;
                    }
                    else {
                        if (v9->pObjectForLocation == InventoryItem_005754A0->pContainerObj) {
                            v3 = 1;
                        }
                        else {
                            if (v9->eFormType == POTYPE_NONE && !v9->pObjecttoAcquire && !v9->pObjectForLocation) {
                                v3 = 1;
                            }
                        }
                    }
                    if (v3) {
                        a1->SetTargeted(true);
                        v9->TempLocList.AddTail(a1);
                    }
                }
                delete InventoryItem_005754A0;
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
