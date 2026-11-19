#include "Fallout/AI/Combat/CombatGroup.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BoltShader.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingLOSMap.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout Shared/eBool.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroupStrategy.h"
#include "Fallout/AI/Combat/CombatGroupStrategyData.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatSearchDoor.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DetectionEvent.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <new>

pSearchRadius_t pSearchRadius{
    {&fCombatSearchInteriorMinRadius, &fCombatSearchInteriorMediumRadius, &fCombatSearchInteriorMaxRadius},
    {&fCombatSearchExteriorMinRadius, &fCombatSearchExteriorMediumRadius, &fCombatSearchExteriorMaxRadius}
};

// The game settings this file defines, with their IDs and values, in the game's initialization order.
SettingT<INISettingCollection> bDebugCombatGroups("bDebugCombatGroups:Combat", false);
SettingT<INISettingCollection> bDebugCombatGroups2("bDebugCombatGroups2:Combat", false);
SettingT<INISettingCollection> bDebugCombatGroupClusters("bDebugCombatGroupClusters:Combat", false);
SettingT<INISettingCollection> bDebugCombatSearch("bDebugCombatSearch:Combat", false);
SettingT<INISettingCollection> bEncounterZoneTargetRestrict("bEncounterZoneTargetRestrict:Combat", true);

void CombatGroupCluster::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup) {
#if XFNV_64BIT
    // The save holds the exe's 16-byte centre: the point, then the space pointer's 32 bits. A pointer means nothing
    // in another process, so the 64-bit program writes 0 there (LoadGame never reads it back), and saves of both
    // widths keep the same bytes.
    struct {
        NiPoint3 Point;
        uint32_t uiSpace;
    } kCenter = {Center.Point, 0};
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&kCenter), 0x10, 0);
#else
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Center), 0x10, 0);
#endif
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iMemberCount), 4, 0);
}

using BSSimpleArray_CombatMember_1024 = BSSimpleArray<CombatMember, 1024>;

using BSSimpleArray_CombatSearchLocation_1024 = BSSimpleArray<CombatSearchLocation, 1024>;

using BSSimpleArray_CombatSearchLocation_1024 = BSSimpleArray<CombatSearchLocation, 1024>;

using BSSimpleArray_CombatTarget_1024 = BSSimpleArray<CombatTarget, 1024>;

using BSSimpleArray_CombatMember_1024 = BSSimpleArray<CombatMember, 1024>;

void CombatGroupCluster::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
#if XFNV_64BIT
    // The exe's 16-byte centre: the point and a saved pointer's 32 bits, stale in this process; the space stays null
    // (its users test it).
    uint32_t uiSpace;
    apLoadGameBuffer->LoadData(&Center.Point, 12u);
    apLoadGameBuffer->LoadData(&uiSpace, 4u);
    Center.pSpace = nullptr;
#else
    apLoadGameBuffer->LoadData(this, 16u);
#endif
    apLoadGameBuffer->LoadData(&iMemberCount, 4u);
}

CombatSearchLocation::CombatSearchLocation() {}

float CombatSearchLocation::GetScore() {
    return (fScore - (&fTimeStamp)->ElapsedTime());
}

// The score loses the squared distance to arMemberWorldLoc (capped at 4096 squared) over 360000; a location with a
// target gains 10.
float CombatSearchLocation::GetModifiedScore(const BGSWorldLocation& arMemberWorldLoc) {
    float fScore = GetScore();
    float fDistanceSquared = arMemberWorldLoc.GetDistanceSquared(WorldLoc);
    fDistanceSquared = BSMIN(fDistanceSquared, 16777216.0f);
    fScore = float(fScore - fDistanceSquared / 360000.0);
    if (iTargetID) {
        fScore = float(fScore + 10.0);
    }
    return fScore;
}

NiColorA* CombatSearchLocation::GetColor(NiColorA* apResult) {
    NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
    float Score = GetScore();
    if (!(Score > 50.0)) {
        if (!(Score > 30.0)) {
            NiColorA kLow(1.0f, 1.0f, 0.0f, 0.5f);
            kColor = kLow;
        }
        else {
            NiColorA kMiddle(0.0f, 0.0f, 1.0f, 0.5f);
            kColor = kMiddle;
        }
    }
    else {
        NiColorA kHigh(1.0f, 0.0f, 1.0f, 0.5f);
        kColor = kHigh;
    }
    *apResult = kColor;
    return apResult;
}

CombatTarget::CombatTarget() {}

CombatTarget::CombatTarget(Actor* apTarget, uint32_t aiSearchCount)
  : pActor(apTarget), iDetectionLevel(0), sLastSearchNoticed(static_cast<uint16_t>(aiSearchCount)), sAttackerCount(0),
    fLastSeenTimeStamp(-3.4028235e+38f), fLastDetectedTimeStamp(-3.4028235e+38f),
    fLastNoticedTimeStamp(CombatTimer::GetTimer()), fLastAttackedMemberTimeStamp(-3.4028235e+38f), cMemberLOSCount(0),
    cMember360LOSCount(0) {
    BGSWorldLocation kLocation;
    LastNoticedLocation = *apTarget->GetWorldLocation(&kLocation);
}

bool CombatTarget::GetHasBufferedDetection() {
    double elapsedTime = (double)(&fLastDetectedTimeStamp)->ElapsedTime();
    return (bool)((uint8_t)((&fCombatDetectionBufferTime)->Float() > elapsedTime));
}

void CombatTarget::CopyData(CombatTarget* apCombatTarget) {
    if (iDetectionLevel < apCombatTarget->iDetectionLevel) {
        iDetectionLevel = (int32_t)((uint32_t)apCombatTarget->iDetectionLevel);
    }
    double v6 = (double)(&fLastDetectedTimeStamp)->operator float();
    if ((&apCombatTarget->fLastDetectedTimeStamp)->operator float() > v6) {
        fLastDetectedTimeStamp.fTimeStamp = apCombatTarget->fLastDetectedTimeStamp.fTimeStamp;
        LastDetectedLocation = apCombatTarget->LastDetectedLocation;
    }
    double v5 = (double)(&fLastSeenTimeStamp)->operator float();
    if ((&apCombatTarget->fLastSeenTimeStamp)->operator float() > v5) {
        fLastSeenTimeStamp.fTimeStamp = apCombatTarget->fLastSeenTimeStamp.fTimeStamp;
        LastSeenLocation = apCombatTarget->LastSeenLocation;
    }
    double v4 = (double)(&fDetectionLevelUpdateTimeStamp)->operator float();
    if ((&apCombatTarget->fDetectionLevelUpdateTimeStamp)->operator float() > v4) {
        fDetectionLevelUpdateTimeStamp.fTimeStamp = apCombatTarget->fDetectionLevelUpdateTimeStamp.fTimeStamp;
    }
    double v3 = (double)(&fLastNoticedTimeStamp)->operator float();
    if ((&apCombatTarget->fLastNoticedTimeStamp)->operator float() > v3) {
        fLastNoticedTimeStamp.fTimeStamp = apCombatTarget->fLastNoticedTimeStamp.fTimeStamp;
    }
    double v2 = (double)(&fLastAttackedMemberTimeStamp)->operator float();
    if ((&apCombatTarget->fLastAttackedMemberTimeStamp)->operator float() > v2) {
        fLastAttackedMemberTimeStamp.fTimeStamp = apCombatTarget->fLastAttackedMemberTimeStamp.fTimeStamp;
        LastAttackedMemberLocation = apCombatTarget->LastAttackedMemberLocation;
    }
    sAttackerCount += apCombatTarget->sAttackerCount;
    cMemberLOSCount += apCombatTarget->cMemberLOSCount;
    cMember360LOSCount += apCombatTarget->cMember360LOSCount;
}

void CombatTarget::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup) {
    apSaveGameBuffer->SaveFormID(pActor, static_cast<uint32_t>(0));
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iDetectionLevel), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sLastSearchNoticed), 2u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sAttackerCount), 2u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cMemberLOSCount), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cMember360LOSCount), 1u, 0);
    LastNoticedLocation.SaveGame(apSaveGameBuffer);
    LastDetectedLocation.SaveGame(apSaveGameBuffer);
    LastSeenLocation.SaveGame(apSaveGameBuffer);
    LastAttackedMemberLocation.SaveGame(apSaveGameBuffer);
    fLastDetectedTimeStamp.SaveGame(apSaveGameBuffer);
    fDetectionLevelUpdateTimeStamp.SaveGame(apSaveGameBuffer);
    fLastNoticedTimeStamp.SaveGame(apSaveGameBuffer);
    fLastSeenTimeStamp.SaveGame(apSaveGameBuffer);
    fDetectionEventTimeStamp.SaveGame(apSaveGameBuffer);
    fLastAttackedMemberTimeStamp.SaveGame(apSaveGameBuffer);
}

void CombatTarget::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    // The save holds pActor's form ID; InitLoadGame resolves it.
    apLoadGameBuffer->LoadFormID(pActor);
    apLoadGameBuffer->LoadData(&iDetectionLevel, 4u);
    apLoadGameBuffer->LoadData(&sLastSearchNoticed, 2u);
    apLoadGameBuffer->LoadData(&sAttackerCount, 2u);
    apLoadGameBuffer->LoadData(&cMemberLOSCount, 1u);
    apLoadGameBuffer->LoadData(&cMember360LOSCount, 1u);
    LastNoticedLocation.LoadGame(apLoadGameBuffer);
    LastDetectedLocation.LoadGame(apLoadGameBuffer);
    LastSeenLocation.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 14u) {
        LastAttackedMemberLocation.LoadGame(apLoadGameBuffer);
    }
    fLastDetectedTimeStamp.LoadGame(apLoadGameBuffer);
    fDetectionLevelUpdateTimeStamp.LoadGame(apLoadGameBuffer);
    fLastNoticedTimeStamp.LoadGame(apLoadGameBuffer);
    fLastSeenTimeStamp.LoadGame(apLoadGameBuffer);
    fDetectionEventTimeStamp.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 13u) {
        fLastAttackedMemberTimeStamp.LoadGame(apLoadGameBuffer);
    }
}

void CombatTarget::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    Actor* v4;
    if (pActor) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        v4 = dynamic_cast<Actor*>(FormByNumericID);
    }
    else {
        v4 = nullptr;
    }
    pActor = v4;
}

CombatGroupCluster::CombatGroupCluster() : iMemberCount(0) {}

ClusterData::ClusterData() {}

CombatMember::CombatMember(Actor* apActor)
  : pActor(apActor), iGroupStrategyAssignment(0), fDamagePerSecond(-1.0f), fCombatStrength(-1.0f), pCluster(nullptr) {}

void CombatMember::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup) {
    apSaveGameBuffer->SaveFormID(pActor, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iGroupStrategyAssignment), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDamagePerSecond), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCombatStrength), 4, 0);
    int8_t cClusterIndex = -1;
    if (pCluster) {
        cClusterIndex = apGroup->GetClusterIndex(pCluster);
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cClusterIndex), 1, 0);
}

// Reads what SaveGame writes: the actor's form ID, the strategy assignment, the damage per second, the combat strength,
// then the cluster index; -1 clears the cluster and any other index resolves through apGroup.
void CombatMember::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    apLoadGameBuffer->LoadFormID(pActor);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iGroupStrategyAssignment), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDamagePerSecond), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCombatStrength), 4);
    int8_t cClusterIndex = -1;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cClusterIndex), 1);
    if (cClusterIndex == -1) {
        pCluster = nullptr;
    }
    else {
        pCluster = apGroup->GetCluster(cClusterIndex);
    }
}

void CombatMember::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    Actor* v4;
    if (pActor) {
        // Until this resolves it, the Actor* field holds the saved form ID.
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        v4 = dynamic_cast<Actor*>(FormByNumericID);
    }
    else {
        v4 = nullptr;
    }
    pActor = v4;
}

void CombatMember::CopyData(CombatMember* apCombatMember) {
    fDamagePerSecond = apCombatMember->fDamagePerSecond;
    fCombatStrength = apCombatMember->fCombatStrength;
    pCluster = apCombatMember->pCluster;
}

CombatGroup::CombatGroup(uint32_t aiGroupID)
  : iGroupNum(0), iGroupID(aiGroupID), pGroupStrategy(nullptr), bStrategyForced(false),
    iLastGroupStrategyChosenIndex(0), fMemberCombatStrength(-1.0f), fAverageMemberDamagePerSecond(-1.0f),
    fTargetCombatStrength(-1.0f), fAverageTargetCombatStrength(-1.0f), iSearchCount(0), spPathingLOSGridMap(nullptr),
    fSearchStartedTimeStamp(-3.4028235e+38f), pSearchingMember(nullptr), SearchFocalPoint(NiPoint3_ZERO),
    fSearchRadius(0.0f), spSearchDebugGeometry(nullptr), bUpdateSearchDebugGeometry(false), iInitializedMemberCount(0),
    iFleeingMemberCount(0), iNonFleeingMemberCount(0), cCombatMusicState(0xFF), spDebugGeometry(nullptr) {}

// Tells the combat manager the targets and members are gone, frees the clusters and removes the debug geometry; the
// members' destructors run after.
CombatGroup::~CombatGroup() {
    uint32_t itargetCount = GetTargetCount();
    for (uint32_t i = 0; i < itargetCount; ++i) {
        pCombatManager->NotifyCombatGroupRemovedTarget(this, GetTarget(i));
    }
    uint32_t imemberCount = GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        pCombatManager->NotifyCombatGroupRemovedMember(this, GetMember(i));
    }
    for (uint32_t i = 0; i < ClusterArray.iSize; ++i) {
        ourFree(ClusterArray[i]);
    }
    CreateDebugGeometry(eTrue);
}

void CombatGroup::GetTargetDetectionLevel(
    Actor* apAttacker,
    Actor* apTarget,
    int32_t& ariDetectionLevel,
    CombatTimeStamp& arfLastTargetDetectedTimeStamp,
    bool& arbTargetLineOfSight,
    bool& arbTarget360LineOfSight
) {
    double Timer;
    BGSWorldLocation memberLoc;
    BGSWorldLocation targetLoc;
    BGSWorldLocation targetLoc_2;
    arbTargetLineOfSight = false;
    arbTarget360LineOfSight = false;
    ariDetectionLevel = -100;
    CombatTimeStamp v37(-3.4028235e+38f);
    arfLastTargetDetectedTimeStamp.fTimeStamp = v37.fTimeStamp;
    CombatTarget* pcombatTarget = GetCombatTargetForActor(apTarget);
    if (pcombatTarget) {
        int32_t icurrentDetectionLevel = pcombatTarget->iDetectionLevel;
        double DecalMinWidth = pcombatTarget->fDetectionLevelUpdateTimeStamp.fTimeStamp;
        if (!(CombatTimer::GetTimer() > DecalMinWidth)) {
            DetectionState* a3;
            if (apAttacker) {
                BaseProcess* CurrentAIProcess = apAttacker->pCurrentProcess;
                a3 = CurrentAIProcess->GetDetectionState(apTarget, 0);
            }
            else {
                a3 = nullptr;
            }
            if (a3) {
                arbTargetLineOfSight = a3->bLineofSight;
                arbTarget360LineOfSight = a3->b360view;
            }
        }
        else {
            int32_t inewDetectionLevel = -100;
            int32_t idetectionEventLevel = -100;
            bool bfindLastDetectedLocation = pcombatTarget->LastDetectedLocation.pSpace == nullptr;
            Actor* pbestDetectingMember = nullptr;
            pcombatTarget->cMember360LOSCount = 0;
            pcombatTarget->cMemberLOSCount = 0;
            BGSWorldLocation lastDetectedLocation;
            CombatTimeStamp flastDetectedTime(-3.4028235e+38f);
            uint32_t imemberCount = GetMemberCount();
            for (uint32_t i = 0; i < imemberCount; ++i) {
                Actor* pmember = GetMember(i);
                BaseProcess* v31 = pmember->pCurrentProcess;
                DetectionState* p_detectionState = v31->GetDetectionState(apTarget, 0);
                DetectionState detectionState;
                if ((pmember->GetCurrentProcessType()) && (apTarget->GetCurrentProcessType())) {
                    pmember->GetWorldLocation(&memberLoc);
                    apTarget->GetWorldLocation(&targetLoc);
                    if (memberLoc.GetDistanceSquared(targetLoc) < 67108864.0) {
                        detectionState.iLevel = 1;
                        p_detectionState = &detectionState;
                    }
                }

                if (p_detectionState) {
                    if (p_detectionState->bLineofSight) {
                        ++pcombatTarget->cMemberLOSCount;
                    }
                    if (p_detectionState->b360view) {
                        ++pcombatTarget->cMember360LOSCount;
                    }
                    if (pmember == apAttacker) {
                        arbTargetLineOfSight = p_detectionState->bLineofSight;
                        arbTarget360LineOfSight = p_detectionState->b360view;
                    }
                    int32_t idetectionLevel = p_detectionState->iLevel;
                    if (p_detectionState->iDetectedEventLevel != 0x7FFFFFFF) {
                        if (idetectionEventLevel < p_detectionState->iDetectedEventLevel) {
                            idetectionEventLevel = p_detectionState->iDetectedEventLevel;
                        }
                        double v7 = fSneakNoticedMin.Float();
                        if (idetectionLevel == static_cast<int32_t>(v7) + 1) {
                            double iDetectedEventLevel = static_cast<double>(p_detectionState->iDetectedEventLevel);
                            if (fSneakNoticedMin.Float() < iDetectedEventLevel) {
                                idetectionLevel = static_cast<int32_t>(fSneakNoticedMin.Float());
                            }
                        }
                    }
                    CombatController* pcombatController = pmember->GetCombatController();
                    if (pcombatController &&
                        (pcombatController->pTarget == apTarget &&
                         !pcombatController->pCombatState->CheckShouldUpdateDetection(pmember, apTarget))) {
                        if (icurrentDetectionLevel > inewDetectionLevel) {
                            inewDetectionLevel = icurrentDetectionLevel;
                            pbestDetectingMember = pmember;
                        }
                    }
                    else {
                        if ((p_detectionState) && (idetectionLevel > inewDetectionLevel)) {
                            inewDetectionLevel = idetectionLevel;
                            pbestDetectingMember = pmember;
                        }
                    }
                    if (bfindLastDetectedLocation) {
                        double v29 = p_detectionState->fLastTimeDetected.fTimeStamp;
                        if (flastDetectedTime.fTimeStamp < v29) {
                            TESForm* Space = pmember->GetSpace();
                            lastDetectedLocation.SetLocation(p_detectionState->LastPositionDetected, Space);
                            flastDetectedTime.fTimeStamp = p_detectionState->fLastTimeDetected.fTimeStamp;
                        }
                    }
                }
            }
            if ((inewDetectionLevel <= static_cast<int32_t>(fSneakNoticedMin.Float())) &&
                (pcombatTarget->fLastAttackedMemberTimeStamp.ElapsedTime() < 1.0)) {
                inewDetectionLevel = static_cast<int32_t>(fSneakNoticedMin.Float() + 1.0);
            }

            if (IsSearching() && icurrentDetectionLevel <= 0 && inewDetectionLevel > 0) {
                if (-3.4028235e+38f == pcombatTarget->fLastDetectedTimeStamp.fTimeStamp) {
                    for (uint32_t i_2 = 0; i_2 < imemberCount; ++i_2) {
                        Actor* pmember_2 = GetMember(i_2);
                        if (pmember_2 != pPlayer) {
                            ExtraDataList* afScore = &pmember_2->m_Extra;
                            Actor* v10 = pmember_2->GetCombatTarget();
                            Script::SetActionFlag(v10, afScore, SCRIPT_ACTION_ONSTARTCOMBAT);
                        }
                    }
                    pCombatDialogueManager->StartDialogue(
                        pbestDetectingMember, apTarget, DT_DETECTION, 3, true, nullptr
                    );
                }
                else {
                    pCombatDialogueManager->StartDialogue(
                        pbestDetectingMember, apTarget, DT_DETECTION, 9, true, nullptr
                    );
                }
            }
            pcombatTarget->iDetectionLevel = inewDetectionLevel;
            Timer = CombatTimer::GetTimer();
            CombatTimeStamp v36(Timer);
            pcombatTarget->fDetectionLevelUpdateTimeStamp = v36;
            apTarget->GetWorldLocation(&targetLoc_2);
            if (inewDetectionLevel > 0) {
                Timer = CombatTimer::GetTimer();
                CombatTimeStamp v35(Timer);
                pcombatTarget->fLastDetectedTimeStamp = v35;
            }
            // The elapsed time is stored as a double before the compare.
            double fElapsed = pcombatTarget->fLastDetectedTimeStamp.ElapsedTime();
            if (!(fCombatDetectionBufferTime.Float() > fElapsed)) {
                if (bfindLastDetectedLocation) {
                    pcombatTarget->LastDetectedLocation = lastDetectedLocation;
                }
            }
            else {
                pcombatTarget->LastDetectedLocation = targetLoc_2;
            }
            int32_t v12 = static_cast<int32_t>(fSneakNoticedMin.Float());
            if (inewDetectionLevel > v12) {
                CombatTimeStamp kNoticedTime(CombatTimer::GetTimer());
                pcombatTarget->fLastNoticedTimeStamp.fTimeStamp = kNoticedTime.fTimeStamp;
                pcombatTarget->LastNoticedLocation = targetLoc_2;
                pcombatTarget->sLastSearchNoticed = this->GetSearchCount();
            }
            if (pcombatTarget->cMember360LOSCount) {
                pcombatTarget->LastSeenLocation = targetLoc_2;
                CombatTimeStamp kSeenTime(CombatTimer::GetTimer());
                pcombatTarget->fLastSeenTimeStamp.fTimeStamp = kSeenTime.fTimeStamp;
            }
            double v28 = static_cast<double>(idetectionEventLevel);
            if (fSneakNoticedMin.Float() < v28 && IsSearching()) {
                BaseProcess* v27 = apTarget->pCurrentProcess;
                DetectionEvent* pdetectionEvent = v27->GetActorsDetectionEvent(apTarget);
                if ((pdetectionEvent) && (pdetectionEvent->fTimeStamp.ElapsedTime() < 5.0)) {
                    double v26 = pdetectionEvent->fTimeStamp.fTimeStamp;
                    if (pcombatTarget->fDetectionEventTimeStamp.fTimeStamp < v26) {
                        float fscore = 0.0f;
                        if (idetectionEventLevel <= 0) {
                            int32_t v14 = static_cast<int32_t>(fSneakNoticedMin.Float());
                            fscore = static_cast<double>(idetectionEventLevel - v14) / 2.0 + 40.0;
                        }
                        else {
                            float v25 = static_cast<double>(idetectionEventLevel) / 20.0;
                            fscore = BSMIN(v25, 5.0f) + 55.0;
                        }
                        BGSWorldLocation worldLoc;
                        TESForm* v15 = apTarget->GetSpace();
                        worldLoc.SetLocation(pdetectionEvent->Location, v15);
                        AddSearchLocation(worldLoc, fscore, 0);
                        pcombatTarget->fDetectionEventTimeStamp.fTimeStamp = pdetectionEvent->fTimeStamp.fTimeStamp;
                    }
                }
            }
        }
        ariDetectionLevel = pcombatTarget->iDetectionLevel;
        arfLastTargetDetectedTimeStamp = pcombatTarget->fLastDetectedTimeStamp;
    }
}

void CombatGroup::RemoveTarget(Actor* apActor) {
    uint32_t a2;
    for (a2 = 0; a2 < (&TargetArray)->BSSimpleArray<CombatTarget>::GetSize(); ++a2) {
        if ((&(&TargetArray)->operator[](a2))->pActor == apActor) {
            RemoveTarget(a2);
            return;
        }
    }
}

void CombatGroup::RemoveTarget(uint32_t aiIndex) {
    Actor* pActor = (&TargetArray.operator[](aiIndex))->pActor;
    pCombatManager->NotifyCombatGroupRemovedTarget(this, pActor);
    TargetArray.RemoveFast(aiIndex, 1u);
}

uint32_t CombatGroup::GetMemberGroupStrategyAssignment(Actor* apMember) {
    CombatMember* CombatMemberForActor = GetCombatMemberForActor(apMember);
    if (CombatMemberForActor) {
        return CombatMemberForActor->iGroupStrategyAssignment;
    }

    return 0;
}

uint32_t CombatGroup::GetMemberGroupStrategyAssignment(uint32_t aiIndex) {
    return MemberArray[aiIndex].iGroupStrategyAssignment;
}

CombatMember* CombatGroup::GetCombatMember(uint32_t aiIndex) {
    return &(&MemberArray)->operator[](aiIndex);
}

void CombatGroup::UpdateAttackerCounts() {
    PlayerCharacter* member;
    uint32_t i;
    CombatTarget* combatTarget;
    uint32_t a2;
    uint32_t numTargets = GetTargetCount();
    uint32_t numMembers = GetMemberCount();
    for (a2 = 0; a2 < numTargets; ++a2) {
        combatTarget = GetCombatTarget(a2);
        combatTarget->sAttackerCount = (uint16_t)0;
        for (i = 0; i < numMembers; ++i) {
            member = static_cast<PlayerCharacter*>(GetMember(i));
            if ((member != pPlayer) && (member->GetCombatTarget() == combatTarget->pActor)) {
                ++combatTarget->sAttackerCount;
            }
        }
    }
}

void CombatGroup::Update() {
    UpdateCounts();
    if (TargetUpdateTimer.Check()) {
        UpdateAttackerCounts();
        UpdateTargets();
        TargetUpdateTimer.Reset(2.0f);
    }
    UpdateGroupStrategy();
    UpdateSearch();
    if (CombatStrengthTimer.Check()) {
        UpdateCombatStrength();
        float* v1 = &fCombatGroupCombatStrengthUpdateTime.Float();
        CombatStrengthTimer.Reset(*v1);
    }
    if (GetMemberCount() > 1) {
        UpdateClusters();
    }
    if (static_cast<char>(cCombatMusicState) == -1) {
        if (CombatMusicUpdateTimer.IsSet()) {
            if (CombatMusicUpdateTimer.Check()) {
                cCombatMusicState = 0;
                if (fTargetCombatStrength > 0.0) {
                    float v4 = fMemberCombatStrength / fTargetCombatStrength;
                    if (fCombatMusicGroupThreatRatioMin.Float() < static_cast<double>(v4) &&
                        fCombatMusicGroupThreatRatioMax.Float() > static_cast<double>(v4)) {
                        cCombatMusicState = 1;
                    }
                }
            }
        }
        else {
            if (!IsSearching()) {
                float* v2 = &fCombatMusicGroupThreatRatioTimer.Float();
                CombatMusicUpdateTimer.Reset(*v2);
            }
        }
    }
}

void CombatGroup::ClearReferencePointers(TESObjectREFR* apRef) {
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        Actor* pMember = GetMember(i);
        CombatController* pController = pMember->GetCombatController();
        if (pController) {
            pController->ClearReferencePointers(apRef);
        }
    }
    if (apRef->IsActor()) {
        RemoveTarget(static_cast<Actor*>(apRef));
    }
}

void CombatGroup::InvestigatedSearchLocation(const BGSWorldLocation& arLocation) {
    float* v2 = &fCombatSearchLocationRemoveDistance.Float();
    float fremoveDistanceSquared = *v2 * fCombatSearchLocationRemoveDistance.Float();
    NiPoint3 coordMult(1.0f, 1.0f, 0.25f);
    uint32_t isearchLocationCount = GetNumSearchLocations();
    for (uint32_t i = 0; i < isearchLocationCount; ++i) {
        CombatSearchLocation* searchLoc = &GetSearchLocation(i);
        double ModifiedDistanceSquared = searchLoc->WorldLoc.GetModifiedDistanceSquared(arLocation, coordMult);
        if (fremoveDistanceSquared > ModifiedDistanceSquared) {
            float fscore = searchLoc->GetScore();
            if (fscore > 30.0) {
                searchLoc->fScore = searchLoc->fScore - (fscore - 30.0);
            }
            if (searchLoc->iTargetID) {
                CombatTarget* pcombatTarget = GetCombatTargetForNumericID(searchLoc->iTargetID);
                if (pcombatTarget) {
                    CombatTimer v14;
                    float fdetectedScore;
                    float fattackedScore;
                    float fnoticedScore;
                    CalculateTargetSearchLocationScores(pcombatTarget, fdetectedScore, fnoticedScore, fattackedScore);
                    if (fdetectedScore > 30.0) {
                        double v4 =
                            pcombatTarget->LastDetectedLocation.GetModifiedDistanceSquared(arLocation, coordMult);
                        if (fremoveDistanceSquared > v4) {
                            double DecalMinWidth = pcombatTarget->fLastDetectedTimeStamp.fTimeStamp;
                            float v12 = DecalMinWidth - (fdetectedScore - 30.0);
                            CombatTimeStamp apMemory(v12);
                            pcombatTarget->fLastDetectedTimeStamp = apMemory;
                        }
                    }
                    if (fnoticedScore > 30.0) {
                        double v6 =
                            pcombatTarget->LastNoticedLocation.GetModifiedDistanceSquared(arLocation, coordMult);
                        if (fremoveDistanceSquared > v6) {
                            double v7 = pcombatTarget->fLastNoticedTimeStamp.fTimeStamp;
                            float v11 = v7 - (fnoticedScore - 30.0);
                            ::new (&v14.fTargetTime) CombatTimeStamp(v11);
                            pcombatTarget->fLastNoticedTimeStamp.fTimeStamp = v14.fTargetTime;
                        }
                    }
                    if (fattackedScore > 30.0) {
                        double v8 =
                            pcombatTarget->LastAttackedMemberLocation.GetModifiedDistanceSquared(arLocation, coordMult);
                        if (fremoveDistanceSquared > v8) {
                            double v9 = pcombatTarget->fLastAttackedMemberTimeStamp.fTimeStamp;
                            float v10 = v9 - (fattackedScore - 30.0);
                            v14.SetStartTime(v10);
                            pcombatTarget->fLastAttackedMemberTimeStamp.fTimeStamp = v14.fStartTime;
                        }
                    }
                }
            }
        }
    }
}

Actor* CombatGroup::FindTargetNearLocation(const BGSWorldLocation& arWorldLoc, float afRadius, bool abDetectedOnly) {
    double distanceSquared;
    BGSWorldLocation kLocation;
    CombatTarget* pTarget;
    int32_t i;
    float fMaxDist = afRadius * afRadius;
    uint32_t uiNumTargets = GetTargetCount();
    for (i = 0; ((uint32_t)i) < uiNumTargets; ++i) {
        pTarget = GetCombatTarget(i);
        if ((((uint8_t)abDetectedOnly) == 0) || pTarget->GetHasBufferedDetection()) {
            pTarget->pActor->TESObjectREFR::GetWorldLocation(&kLocation);
            distanceSquared = (double)(&arWorldLoc)->GetDistanceSquared(kLocation);
            if (fMaxDist > distanceSquared) {
                return pTarget->pActor;
            }
        }
    }
    return nullptr;
}

// A maximum corner grows when it is below the target's coordinate, a minimum one when it is above (NaN never stores).
void CombatGroup::ComputeTargetBoundingBox(NiPoint3* apCorners, bool abAlreadyInitialized) {
    if (!abAlreadyInitialized) {
        for (uint32_t i = 0; i < 8; ++i) {
            for (uint32_t j = 0; j < 3; ++j) {
                float fInit;
                if (CombatGroup_BoundsCornerIsMax[i][j]) {
                    fInit = -3.4028235e+38f;
                }
                else {
                    fInit = 3.4028235e+38f;
                }
                apCorners[i][j] = fInit;
            }
        }
    }
    uint32_t uiTargetCount = GetTargetCount();
    for (uint32_t t = 0; t < uiTargetCount; ++t) {
        Actor* pTarget = GetTarget(t);
        NiPoint3 kLocation = pTarget->GetLocationOnReference();
        for (uint32_t i = 0; i < 8; ++i) {
            for (uint32_t j = 0; j < 3; ++j) {
                float fValue = kLocation[j];
                if (CombatGroup_BoundsCornerIsMax[i][j]) {
                    if (apCorners[i][j] < fValue) {
                        apCorners[i][j] = fValue;
                    }
                }
                else {
                    if (apCorners[i][j] > fValue) {
                        apCorners[i][j] = fValue;
                    }
                }
            }
        }
    }
}

uint32_t CombatGroup::GetTargetCount() {
    return TargetArray.GetSize();
}

bool CombatGroup::IsFleeing() {
    return !iNonFleeingMemberCount && iFleeingMemberCount;
}

CombatSearchDoor* CombatGroup::GetCombatSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* v4;
    uint32_t i;
    for (i = 0; i < (&SearchTeleportDoors)->iSize; ++i) {
        v4 = &(&SearchTeleportDoors)->operator[](i);
        if ((v4->pDoor == apDoor) || (v4->pLinkedDoor == apDoor)) {
            return v4;
        }
    }
    return nullptr;
}

CombatTarget* CombatGroup::GetCombatTargetForActor(Actor* apActor) {
    uint32_t v2 = apActor->TESForm::GetFormID();
    return GetCombatTargetForNumericID(v2);
}

bool CombatGroup::AddMember(Actor* apActor) {
    if (!apActor) {
        return false;
    }
    if (IsMember(apActor)) {
        return true;
    }
    if (!CanAddMember(apActor)) {
        return false;
    }
    if (IsTarget(apActor)) {
        const char* FullName;
        if (apActor->GetFormEditorIDLength()) {
            FullName = apActor->GetFormEditorID();
        }
        else {
            FullName = apActor->GetFullName();
        }
        Warning(
            "COMBAT: Trying to add actor %08X %s to Combat Group member list when they are a target.", apActor->iFormID,
            FullName
        );
        return false;
    }

    MemberArray.Add(CombatMember(apActor));
    pCombatManager->NotifyCombatGroupAddedMember(this, apActor);
    if ((bDebugCombatGroups.Bool()) && (apActor->Get3D())) {
        NiColorA v8 = BSTestObjects::GetColor(iGroupNum);
        NiColor color(v8.r, v8.g, v8.b);
        NiNode* v4 = apActor->Get3D();
        TintScenegraph(v4, color, nullptr);
    }

    ClusterUpdateTimer.ForceTrigger();
    return true;
}

CombatMember* CombatGroup::GetCombatMemberForActor(Actor* apActor) {
    uint32_t v2 = apActor->TESForm::GetFormID();
    return GetCombatMemberForNumericID(v2);
}

bool CombatGroup::AddTarget(Actor* apActor) {
    if (!apActor) {
        return false;
    }
    if (IsTarget(apActor)) {
        return true;
    }
    if (!CanAddTarget(apActor)) {
        return false;
    }
    if (IsMember(apActor)) {
        const char* FullName;
        if (apActor->GetFormEditorIDLength()) {
            FullName = apActor->GetFormEditorID();
        }
        else {
            FullName = apActor->GetFullName();
        }
        int32_t CastingType = apActor->GetFormID();
        Warning(
            "COMBAT: Trying to add actor %08X %s to Combat Group target list when they are a member.", CastingType,
            FullName
        );
        return false;
    }

    TargetArray.Add(CombatTarget(apActor, iSearchCount));
    pCombatManager->NotifyCombatGroupAddedTarget(this, apActor);
    return true;
}

void CombatGroup::RemoveAllMembers() {
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        Actor* pMember = GetMember(i);
        pCombatManager->NotifyCombatGroupRemovedMember(this, pMember);
    }
    MemberArray.Clear(false);
}

CombatSearchLocation& CombatGroup::GetSearchLocation(uint32_t aiIndex) {
    return SearchLocations[aiIndex];
}

void CombatGroup::NotifyMemberPlanFinished(CombatController* apCombatController, CombatGoal* apGoal) {
    if (pGroupStrategy) {
        Actor* pAttacker = apCombatController->GetAttacker();
        uint32_t uiAssignment = apGoal->GetGroupStrategyAssignment();
        if (uiAssignment && GetMemberGroupStrategyAssignment(pAttacker) == uiAssignment) {
            SetMemberGroupStrategyAssignment(pAttacker, 0);
        }
    }
}

void CombatGroup::NotifyMemberHit(Actor* apMember, Actor* apAttacker) {
    if (apAttacker) {
        CombatTarget* pTarget = GetCombatTargetForActor(apAttacker);
        if (pTarget) {
            CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
            pTarget->fLastAttackedMemberTimeStamp = kTimeStamp;
            BGSWorldLocation kLocation;
            pTarget->LastAttackedMemberLocation = *apAttacker->TESObjectREFR::GetWorldLocation(&kLocation);
        }
    }
}

Actor* CombatGroup::FindMemberWithStrategyAssignment(uint32_t aiAssigment) {
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        if (GetMemberGroupStrategyAssignment(static_cast<int32_t>(i)) == aiAssigment) {
            return GetMember(i);
        }
    }
    return nullptr;
}

void CombatGroup::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    UpdateAttackerCounts();
    for (uint32_t i = 0; i < GetMemberCount(); ++i) {
        Actor* pMember = GetMember(i);
        if (pMember == pPlayer) {
            continue;
        }
        CombatController* pController = pMember->GetCombatController();
        if (!pController || pController->GetCombatGroup() != this) {
            RemoveMember(pMember);
            --i;
        }
    }
}

PathingLOSGridMap* CombatGroup::GetSearchLOSGridMap() {
    return spPathingLOSGridMap.m_pObject;
}

bool CombatGroup::IsSearching() {
    return (((int32_t)iSearchCount) != 0);
}

CombatTimer* CombatGroup::GetDetectionDialogueTimer(uint32_t aiIndex) {
    return &DetectionDialogueTimers[aiIndex];
}

bool CombatGroup::IsTarget(Actor* apActor) {
    return (GetCombatTargetForActor(apActor) != nullptr);
}

bool CombatGroup::IsMember(Actor* apActor) {
    return (GetCombatMemberForActor(apActor) != nullptr);
}

bool CombatGroup::CanAddMember(Actor* apActor) {
    Actor* target;
    uint32_t i;
    uint32_t numTargets = GetTargetCount();
    for (i = 0; i < numTargets; ++i) {
        target = GetTarget(i);
        if (!apActor->CanAttackActor(target)) {
            return false;
        }
    }
    return true;
}

bool CombatGroup::IsEmpty() {
    return (bool)((uint8_t)(&MemberArray)->IsEmpty());
}

void CombatGroup::IncrementAttackerCount(Actor* apTarget) {
    CombatTarget* combatTargetForActor = GetCombatTargetForActor(apTarget);
    if (combatTargetForActor) {
        ++combatTargetForActor->sAttackerCount;
    }
}

void CombatGroup::DecrementAttackerCount(Actor* apTarget) {
    CombatTarget* combatTargetForActor = GetCombatTargetForActor(apTarget);
    if (combatTargetForActor) {
        --combatTargetForActor->sAttackerCount;
    }
}

char CombatGroup::GetCombatMusicState() {
    return (int8_t)cCombatMusicState;
}

void CombatGroup::InvestigatedSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* combatSearchDoor = GetCombatSearchDoor(apDoor);
    if (combatSearchDoor) {
        combatSearchDoor->bInvestigated = true;
    }
}

void CombatGroup::ReserveSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* combatSearchDoor = GetCombatSearchDoor(apDoor);
    if (combatSearchDoor) {
        combatSearchDoor->bReserved = true;
    }
}

void CombatGroup::UnreserveSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* combatSearchDoor = GetCombatSearchDoor(apDoor);
    if (combatSearchDoor) {
        combatSearchDoor->bReserved = false;
    }
}

CombatGroupCluster* CombatGroup::GetClusterForMember(Actor* apMember) {
    CombatMember* combatMemberForActor = GetCombatMemberForActor(apMember);
    if (combatMemberForActor) {
        return combatMemberForActor->pCluster;
    }

    return nullptr;
}

char CombatGroup::GetClusterIndex(CombatGroupCluster* apCluster) {
    return ClusterArray.Find(apCluster, 0, &BSSimpleArray<CombatGroupCluster*>::DefaultEqualFunctionByAddress);
}

CombatGroupCluster* CombatGroup::GetCluster(char acIndex) {
    if (acIndex >= 0) {
        return *static_cast<CombatGroupCluster**>(&ClusterArray.operator[](acIndex));
    }

    return nullptr;
}

void CombatGroup::SetSearchingMember(Actor* apMember) {
    pSearchingMember = apMember;
}

Actor* CombatGroup::GetSearchingMember() {
    return pSearchingMember;
}

void CombatGroup::SetMemberGroupStrategyAssignment(Actor* a2, uint32_t a3) {
    CombatMember* combatMemberForActor = GetCombatMemberForActor(a2);
    if (combatMemberForActor) {
        combatMemberForActor->iGroupStrategyAssignment = (uint32_t)((int32_t)a3);
    }
}

void* CombatGroup::SetGroupStrategyAssignment(int32_t aiIndex, uint32_t auiAssignment) {
    CombatMember* result = &(&MemberArray)->operator[](aiIndex);
    result->iGroupStrategyAssignment = (uint32_t)((int32_t)auiAssignment);
    return result;
}

void CombatGroup::RemoveMember(Actor* apActor) {
    NiNode* v2;
    uint32_t arg0;
    for (arg0 = 0; arg0 < (&MemberArray)->iSize; ++arg0) {
        if ((&(&MemberArray)->operator[](arg0))->pActor == apActor) {
            pCombatManager->CombatManager::NotifyCombatGroupRemovedMember(this, apActor);
            MemberArray.RemoveFast(arg0, 1u);
            if (pSearchingMember == apActor) {
                pSearchingMember = nullptr;
            }
            if (((&::bDebugCombatGroups)->Bool()) && (apActor->Get3D())) {
                v2 = apActor->Get3D();
                UnTintScenegraph(v2);
            }

            return;
        }
    }
}

CombatTarget* CombatGroup::GetCombatTargetForNumericID(uint32_t aiTargetID) {
    for (uint32_t auiIndex = 0; auiIndex < TargetArray.GetSize(); ++auiIndex) {
        CombatTarget* v4 = &TargetArray.operator[](auiIndex);
        if (v4->pActor->GetFormID() == aiTargetID) {
            return v4;
        }
    }
    return nullptr;
}

CombatMember* CombatGroup::GetCombatMemberForNumericID(uint32_t auiRefID) {
    CombatMember* v4;
    uint32_t i;
    for (i = 0; i < (&MemberArray)->iSize; ++i) {
        v4 = &(&MemberArray)->operator[](i);
        if (v4->pActor->TESForm::GetFormID() == auiRefID) {
            return v4;
        }
    }
    return nullptr;
}

Actor* CombatGroup::GetMember(uint32_t aiIndex) {
    return (&(&MemberArray)->operator[](aiIndex))->pActor;
}

// Whether the members and the targets are mixed: with both groups' positions measured along the line from the members'
// centre towards the targets' centre, from a split point, more than a third of each lies behind it, or less than two
// thirds of each does. Fills CombatGroupData::iMembersTargetsIntersecting. The float ratios are compared as doubles
// with the float constants 0.33f and 0.67f widened.
bool CombatGroup::CheckMemberTargetIntersection() {
    uint32_t uiMemberCount = GetMemberCount();
    uint32_t uiTargetCount = GetTargetCount();
    if (!uiMemberCount || !uiTargetCount || (uiMemberCount == 1 && uiTargetCount == 1)) {
        return false;
    }
    NiPoint3* pMemberPositions = static_cast<NiPoint3*>(__builtin_alloca_with_align(12 * uiMemberCount, 128));
    NiPoint3* pTargetPositions = static_cast<NiPoint3*>(__builtin_alloca_with_align(12 * uiTargetCount, 128));
    NiPoint3 kMemberCenter = NiPoint3_ZERO;
    NiPoint3 kTargetCenter = NiPoint3_ZERO;
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        pMemberPositions[i] = pMember->GetLocationOnReference();
        kMemberCenter += pMemberPositions[i];
    }
    for (uint32_t j = 0; j < uiTargetCount; ++j) {
        Actor* pTarget = GetTarget(j);
        pTargetPositions[j] = pTarget->GetLocationOnReference();
        kTargetCenter += pTargetPositions[j];
    }
    NiPoint3 kTotalCenter = kMemberCenter + kTargetCenter;
    kMemberCenter /= static_cast<float>(uiMemberCount);
    kTargetCenter /= static_cast<float>(uiTargetCount);
    kTotalCenter /= static_cast<float>(uiMemberCount + uiTargetCount);
    NiPoint3 kDirection = kTargetCenter - kMemberCenter;
    NiPoint3 kToTotal = kTotalCenter - kMemberCenter;
    float fDot = kToTotal.Dot(kDirection);
    float fDistance = NiSqrt(fDot);
    float fLength = kDirection.UnitizeGetLength();
    (void)fLength;
    NiPoint3 kSplitPoint = kMemberCenter + kDirection * fDistance;
    NiPoint3 kBackward = kMemberCenter - kSplitPoint;
    uint32_t uiMembersBehind = 0;
    uint32_t uiTargetsBehind = 0;
    for (uint32_t k = 0; k < uiMemberCount; ++k) {
        if ((pMemberPositions[k] - kSplitPoint).Dot(kBackward) < 0.0) {
            ++uiMembersBehind;
        }
    }
    for (uint32_t m = 0; m < uiTargetCount; ++m) {
        if ((pTargetPositions[m] - kSplitPoint).Dot(kBackward) < 0.0) {
            ++uiTargetsBehind;
        }
    }
    float fMemberRatio = static_cast<float>(static_cast<double>(uiMembersBehind) / static_cast<double>(uiMemberCount));
    float fTargetRatio = static_cast<float>(static_cast<double>(uiTargetsBehind) / static_cast<double>(uiTargetCount));
    return (fMemberRatio > 0.33000001311302185 && fTargetRatio > 0.33000001311302185) ||
        (fMemberRatio < 0.6700000166893005 && fTargetRatio < 0.6700000166893005);
}

CombatTarget* CombatGroup::GetCombatTarget(uint32_t aiIndex) {
    return &(&TargetArray)->operator[](aiIndex);
}

Actor* CombatGroup::FindMemberNearLocation(const BGSWorldLocation& arWorldLoc, float afRadius, Actor* apIgnoreMember) {
    double distanceSquared;
    BGSWorldLocation v7;
    Actor* memberActor;
    int32_t i;
    float v11 = afRadius * afRadius;
    uint32_t numMembers = GetMemberCount();
    for (i = 0; ((uint32_t)i) < numMembers; ++i) {
        memberActor = GetMember(i);
        if (memberActor != apIgnoreMember) {
            memberActor->TESObjectREFR::GetWorldLocation(&v7);
            distanceSquared = (double)arWorldLoc.GetDistanceSquared(v7);
            if (v11 > distanceSquared) {
                return memberActor;
            }
        }
    }
    return nullptr;
}

Actor* CombatGroup::FindClosestMemberToLocation(
    const BGSWorldLocation& arWorldLoc,
    Actor* apIgnoreMember,
    float afMaxDistance
) {
    float v5;
    Actor* member;
    BGSWorldLocation arWorldLocation;
    float distanceSquared;
    uint32_t aiIndex;
    if (0x1.fffffe0000000p+127f == afMaxDistance) {
        v5 = 0x1.fffffe0000000p+127f;
    }
    else {
        v5 = afMaxDistance * afMaxDistance;
    }
    float v11 = v5;
    Actor* v12 = nullptr;
    uint32_t memberCount = GetMemberCount();
    for (aiIndex = 0; aiIndex < memberCount; ++aiIndex) {
        member = GetMember(aiIndex);
        if (member != apIgnoreMember) {
            member->GetWorldLocation(&arWorldLocation);
            distanceSquared = arWorldLoc.GetDistanceSquared(arWorldLocation);
            if (v11 > ((double)distanceSquared)) {
                v11 = distanceSquared;
                v12 = member;
            }
        }
    }
    return v12;
}

bool CombatGroup::CanAddTarget(Actor* apActor) {
    Actor* memberActor;
    uint32_t i;
    uint32_t numMembers = GetMemberCount();
    for (i = 0; i < numMembers; ++i) {
        memberActor = GetMember(i);
        if (memberActor && (!memberActor->CanAttackActor(apActor))) {
            return false;
        }
    }
    return true;
}

Actor* CombatGroup::FindMemberNearActor(Actor* apActor, float afRadius) {
    BGSWorldLocation v6;
    BGSWorldLocation* worldLocation = apActor->TESObjectREFR::GetWorldLocation(&v6);
    return FindMemberNearLocation(*worldLocation, afRadius, apActor);
}

Actor* CombatGroup::FindClosestMemberToActor(Actor* apActor, float afMaxDistance) {
    BGSWorldLocation v6;
    BGSWorldLocation* worldLocation = apActor->TESObjectREFR::GetWorldLocation(&v6);
    return FindClosestMemberToLocation(*worldLocation, apActor, afMaxDistance);
}

void CombatGroup::FailedToInvestigateSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* combatSearchDoor = GetCombatSearchDoor(apDoor);
    if (combatSearchDoor) {
        combatSearchDoor->bReserved = false;
        ++combatSearchDoor->cFailedAttempts;
    }
}

void CombatGroup::ClearGroupStrategyAssignments() {
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        MemberArray[i].iGroupStrategyAssignment = 0;
    }
}

void CombatGroup::AddSearchLocation(const BGSWorldLocation& arLocation, float afScore, uint32_t aiTargetID) {
    uint32_t uiIndex = SearchLocations.AddUninitialized();
    SearchLocations.GetAt(uiIndex).WorldLoc = arLocation;
    CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
    SearchLocations.GetAt(uiIndex).fTimeStamp = kTimeStamp;
    SearchLocations.GetAt(uiIndex).iTargetID = aiTargetID;
    SearchLocations.GetAt(uiIndex).fScore = afScore;
}

void CombatGroup::UpdateCounts() {
    Actor* pMember;
    CombatController* v3;
    uint32_t i;
    iInitializedMemberCount = (uint32_t)0;
    iFleeingMemberCount = (uint32_t)0;
    iNonFleeingMemberCount = (uint32_t)0;
    uint32_t numMembers = GetMemberCount();
    for (i = 0; i < numMembers; ++i) {
        pMember = GetMember(i);
        v3 = pMember->GetCombatController();
        if (v3) {
            if (v3->TESForm::GetInitialized()) {
                ++iInitializedMemberCount;
            }
            if (v3->IsFleeing()) {
                ++iFleeingMemberCount;
            }
            else {
                ++iNonFleeingMemberCount;
            }
        }
    }
}

bool CombatGroup::ShouldPlayCombatMusic() {
    return (bool)((uint8_t)(((int8_t)cCombatMusicState) == 1));
}

char CombatGroup::GetClusterCount() {
    return (int8_t)(&ClusterArray)->iSize;
}

uint32_t CombatGroup::GetNumSearchLocations() {
    return (&SearchLocations)->iSize;
}

void CombatGroup::FillTargetArray(BSSimpleArray<Actor*, 1024>* apActorArray) {
    CombatTarget* combatTarget;
    uint32_t a2;
    uint32_t formFlags = (&TargetArray)->BSSimpleArray<CombatTarget>::GetSize();
    apActorArray->SetSize(formFlags, true);
    for (a2 = 0; a2 < (&TargetArray)->BSSimpleArray<CombatTarget>::GetSize(); ++a2) {
        combatTarget = &(&TargetArray)->operator[](a2);
        apActorArray->SetAt(a2, combatTarget->pActor);
    }
}

Actor* CombatGroup::GetBestTarget(Actor* apMember) {
    BGSWorldLocation memberWorldLoc;
    BGSWorldLocation targetWorldLoc;
    if (TargetArray.IsEmpty()) {
        return nullptr;
    }
    float fcurrentTargetScore = -3.4028235e+38f;
    CombatTimeStamp fcurrentLastDetectedTimeStamp(-3.4028235e+38f);
    Actor* pbestTarget = nullptr;
    Actor* plastDetectedTarget = nullptr;
    Actor* plastTargetChecked = nullptr;
    apMember->GetWorldLocation(&memberWorldLoc);
    CombatController* pcombatController = apMember->GetCombatController();
    if (!pcombatController) {
        RemoveMember(apMember);
        return nullptr;
    }
    Actor* pcurrentTarget = pcombatController->GetTarget();
    CombatState* pcombatState = pcombatController->pCombatState;
    TESCombatStyle* CurrentCombatStyle = pcombatController->GetCurrentCombatStyle();
    float ftargetingFOV = CurrentCombatStyle->SimpleData.fTargetingFOV;
    BGSEncounterZone* pmemberEncounterZone = apMember->GetLocationEncounterZone();
    bool bisMelee = CombatUtilities::IsActorUsingMelee(apMember);
    uint32_t itargetCount = GetTargetCount();
    for (uint32_t i = 0; i < itargetCount; ++i) {
        CombatTarget* pcombatTarget = GetCombatTarget(i);
        Actor* ptarget = pcombatTarget->pActor;
        if (ftargetingFOV > 0.0) {
            float v11 = ftargetingFOV * 0.01745329238474369;
            const NiPoint3* v4 = &ptarget->GetLocationOnReference();
            if (!CombatUtilities::IsTargetWithinFiringArc(apMember, *v4, v11, 6.2831855f)) {
                continue;
            }
        }
        if (bEncounterZoneTargetRestrict.Bool()) {
            BGSEncounterZone* ptargetEncounterZone = ptarget->GetLocationEncounterZone();
            if (pmemberEncounterZone != ptargetEncounterZone) {
                continue;
            }
        }
        if (apMember->bPlayerTeammate) {
            FIGHT_REACTION efightReaction = FIGHT_REACTION_NEUTRAL;
            if (!apMember->GetShouldAttackActor(ptarget, false, efightReaction, false)) {
                continue;
            }
        }
        // btargetRemoved is cleared and never read, as the game does.
        bool btargetRemoved = false;
        (void)btargetRemoved;
        int32_t idetectionLevel = 0;
        bool blineOfSight = false;
        bool b360LineOfSight = false;
        CombatTimeStamp flastDetectedTimeStamp_2;
        GetTargetDetectionLevel(
            apMember, ptarget, idetectionLevel, flastDetectedTimeStamp_2, blineOfSight, b360LineOfSight
        );
        if (idetectionLevel < 0 && ptarget == pcurrentTarget && pcombatState->GetHasBufferedDetection()) {
            idetectionLevel = 1;
        }
        if (idetectionLevel <= 0) {
            float fmodifiedLastNoticedTimeStamp = pcombatTarget->fLastNoticedTimeStamp.fTimeStamp - 30.0;
            float a2 = flastDetectedTimeStamp_2.fTimeStamp;
            float a2a = BSMAX(fmodifiedLastNoticedTimeStamp, a2);
            CombatTimeStamp flastDetectedTimeStamp(a2a);
            flastDetectedTimeStamp_2.fTimeStamp = flastDetectedTimeStamp.fTimeStamp;
            double DecalMinWidth = flastDetectedTimeStamp_2.fTimeStamp;
            if (fcurrentLastDetectedTimeStamp.fTimeStamp < DecalMinWidth) {
                plastDetectedTarget = ptarget;
                fcurrentLastDetectedTimeStamp.fTimeStamp = flastDetectedTimeStamp_2.fTimeStamp;
            }
        }
        else {
            float ftargetScore = 0.0f;
            if (itargetCount > 1) {
                if (b360LineOfSight) {
                    ftargetScore = ftargetScore + f360LOSBonus;
                }
                if (blineOfSight) {
                    ftargetScore = ftargetScore + fLOSBonus;
                }
                if (pcurrentTarget == ptarget) {
                    ftargetScore = ftargetScore + fCurrentTargetBonus;
                    // Reads the combat state through the controller again here.
                    if (!b360LineOfSight &&
                        fCurrentTargetLOSFalloffTime >
                            pcombatController->pCombatState->GetLastTarget360LineOfSightTimeStamp().ElapsedTime()) {
                        ftargetScore = ftargetScore + f360LOSBonus;
                    }
                }
                bool bisTargetMelee = CombatUtilities::IsActorUsingMelee(ptarget);
                if (bisMelee == bisTargetMelee) {
                    ftargetScore = ftargetScore + fweaponTypeBonus;
                }
                float fsqrDist = 0.0f;
                ptarget->GetWorldLocation(&targetWorldLoc);
                bool bspaceDifferent = targetWorldLoc.pSpace != memberWorldLoc.pSpace;
                if (bspaceDifferent) {
                    if (ptarget == pcurrentTarget) {
                        fsqrDist = 0.0f;
                    }
                    else {
                        fsqrDist = fMaxDistance * fMaxDistance;
                    }
                }
                else {
                    fsqrDist = memberWorldLoc.GetDistanceSquared(targetWorldLoc);
                }
                fsqrDist = BSMIN(4194304.0f, fsqrDist);
                ftargetScore = (1.0 - fsqrDist / 4194304.0) * fDistanceBonus + ftargetScore;
                uint32_t ishareCount = pcurrentTarget == ptarget;
                if (pcombatTarget->sAttackerCount > ishareCount) {
                    ftargetScore = ftargetScore + fShareTargetBonus;
                }
                if ((pcombatState->IsMeleeOnly() || bspaceDifferent) &&
                    pcombatState->IsLocationUnreachable(targetWorldLoc, 256.0f)) {
                    ftargetScore = ftargetScore - fUnreachablePenalty;
                }
                if (bspaceDifferent && !CombatUtilities::CanActorExistInSpace(apMember, targetWorldLoc)) {
                    ftargetScore = ftargetScore - fCantExistInTargetSpacePenalty;
                }
                if (ptarget->IsDead(false) || ptarget->IsKnockedOut()) {
                    ftargetScore = ftargetScore - fDyingPenalty;
                }
                if (ptarget->IsFleeing(false)) {
                    ftargetScore = ftargetScore - fFleeingPenalty;
                }
            }
            if (fcurrentTargetScore < static_cast<double>(ftargetScore)) {
                fcurrentTargetScore = ftargetScore;
                pbestTarget = ptarget;
            }
        }
        plastTargetChecked = ptarget;
    }
    if (!pbestTarget) {
        pbestTarget = plastDetectedTarget;
    }
    if (!pbestTarget) {
        pbestTarget = plastTargetChecked;
    }
    return pbestTarget;
}

void CombatGroup::FillMemberArray(BSSimpleArray<Actor*, 1024>* apActorArray) {
    CombatMember* v3;
    uint32_t a2;
    uint32_t formFlags = (&MemberArray)->iSize;
    apActorArray->SetSize(formFlags, true);
    for (a2 = 0; a2 < (&MemberArray)->iSize; ++a2) {
        v3 = &(&MemberArray)->operator[](a2);
        apActorArray->SetAt(a2, v3->pActor);
    }
}

void CombatGroup::RemoveFromMembers() {
    Actor* member;
    uint32_t aiIndex;
    uint32_t memberCount = GetMemberCount();
    for (aiIndex = 0; aiIndex < memberCount; ++aiIndex) {
        member = GetMember(aiIndex);
        member->SetCombatGroup(nullptr);
    }
}

uint8_t CombatGroup::GetTargetMemberLOSCount(Actor* a2) {
    CombatTarget* combatTargetForActor = GetCombatTargetForActor(a2);
    if (combatTargetForActor) {
        return combatTargetForActor->cMemberLOSCount;
    }

    return (uint8_t)0;
}

// Centres the search area on the search locations, weighted by score, or on the members when there are none (the
// highest-process member's space); the radius covers them plus fCombatSearchBufferDistance, clamped to the
// min/max search radius of the location.
void CombatGroup::ComputeSearchArea() {
    float fsearchMinRadius = 0.0f;
    float fsearchMaxRadius = 0.0f;
    uint32_t isearchLocationCount = GetNumSearchLocations();
    if (isearchLocationCount) {
        float fbestScore = -3.4028235e+38f;
        if (isearchLocationCount <= 1) {
            CombatSearchLocation* searchLoc = &GetSearchLocation(0);
            fbestScore = searchLoc->GetScore();
            SearchCenter = searchLoc->WorldLoc;
            SearchFocalPoint = searchLoc->WorldLoc.Point;
            GetMinMaxSearchRadius(searchLoc->WorldLoc, fsearchMinRadius, fsearchMaxRadius);
            fSearchRadius = fsearchMinRadius;
        }
        else {
            uint32_t ibestIndex = 0;
            for (uint32_t i = 0; i < isearchLocationCount; ++i) {
                CombatSearchLocation* searchLoc = &GetSearchLocation(i);
                float fscore = searchLoc->GetScore();
                if (fbestScore < static_cast<double>(fscore)) {
                    fbestScore = fscore;
                    ibestIndex = i;
                }
            }
            CombatSearchLocation* bestSearchLoc = &GetSearchLocation(ibestIndex);
            NiPoint3 center = NiPoint3_ZERO;
            float ftotalScore = 0.0f;
            GetMinMaxSearchRadius(bestSearchLoc->WorldLoc, fsearchMinRadius, fsearchMaxRadius);
            for (uint32_t i = 0; i < isearchLocationCount; ++i) {
                CombatSearchLocation* searchLoc = &GetSearchLocation(i);
                double fdistanceSquared = bestSearchLoc->WorldLoc.GetDistanceSquared(searchLoc->WorldLoc);
                if (!(fsearchMaxRadius * fsearchMaxRadius < fdistanceSquared)) {
                    float fscore = searchLoc->GetScore();
                    center += searchLoc->WorldLoc.Point * fscore;
                    ftotalScore = ftotalScore + fscore;
                }
            }
            center /= ftotalScore;
            SearchCenter.SetLocation(center, bestSearchLoc->WorldLoc.pSpace);
            SearchFocalPoint = bestSearchLoc->WorldLoc.Point;
            float fmaxDistanceFromCenterSquared = 0.0f;
            for (uint32_t i = 0; i < isearchLocationCount; ++i) {
                CombatSearchLocation* searchLoc = &GetSearchLocation(i);
                if (searchLoc->WorldLoc.pSpace == SearchCenter.pSpace) {
                    float fdistanceSquared = SearchCenter.GetDistanceSquared(searchLoc->WorldLoc);
                    if (fmaxDistanceFromCenterSquared < static_cast<double>(fdistanceSquared)) {
                        fmaxDistanceFromCenterSquared = fdistanceSquared;
                        if (fmaxDistanceFromCenterSquared > fsearchMaxRadius * fsearchMaxRadius) {
                            break;
                        }
                    }
                }
            }
            if (!(fmaxDistanceFromCenterSquared > fsearchMaxRadius * fsearchMaxRadius)) {
                double fdistance = NiSqrt(fmaxDistanceFromCenterSquared);
                fSearchRadius = fCombatSearchBufferDistance.Float() + fdistance;
                fSearchRadius = BSMIN(fSearchRadius, fsearchMaxRadius);
                fSearchRadius = BSMAX(fSearchRadius, fsearchMinRadius);
            }
            else {
                fSearchRadius = fsearchMaxRadius;
            }
        }
    }
    else if (!SearchCenter.pSpace) {
        uint32_t imemberCount = GetMemberCount();
        if (imemberCount <= 1) {
            BGSWorldLocation memberWorldLoc;
            Actor* pmember = GetMember(0);
            SearchCenter = *pmember->GetWorldLocation(&memberWorldLoc);
            SearchFocalPoint = SearchCenter.Point;
            GetMinMaxSearchRadius(SearchCenter, fsearchMinRadius, fsearchMaxRadius);
            fSearchRadius = fsearchMinRadius;
        }
        else {
            // The member in the highest process level (the lowest PROCESS_TYPE) gives the space and the radius.
            PROCESS_TYPE ehighestProcessLevel = PROCESS_TYPE_COUNT;
            Actor* phighestMember = nullptr;
            for (uint32_t i = 0; i < imemberCount; ++i) {
                Actor* pmember = GetMember(i);
                PROCESS_TYPE elevel = pmember->GetCurrentProcessType();
                if (elevel < ehighestProcessLevel) {
                    ehighestProcessLevel = elevel;
                    phighestMember = pmember;
                }
            }
            BGSWorldLocation highestMemberWorldLoc;
            phighestMember->GetWorldLocation(&highestMemberWorldLoc);
            NiPoint3 center = NiPoint3_ZERO;
            uint32_t itotalMembers = 0;
            GetMinMaxSearchRadius(highestMemberWorldLoc, fsearchMinRadius, fsearchMaxRadius);
            for (uint32_t i = 0; i < imemberCount; ++i) {
                Actor* pmember = GetMember(i);
                BGSWorldLocation memberWorldLoc;
                pmember->GetWorldLocation(&memberWorldLoc);
                double fdistanceSquared = highestMemberWorldLoc.GetDistanceSquared(memberWorldLoc);
                if (!(fsearchMaxRadius * fsearchMaxRadius < fdistanceSquared)) {
                    center += memberWorldLoc.Point;
                    ++itotalMembers;
                }
            }
            center /= static_cast<float>(itotalMembers);
            SearchCenter.SetLocation(center, highestMemberWorldLoc.pSpace);
            SearchFocalPoint = highestMemberWorldLoc.Point;
            float fmaxDistanceFromCenterSquared = 0.0f;
            for (uint32_t i = 0; i < imemberCount; ++i) {
                Actor* pmember = GetMember(i);
                BGSWorldLocation memberWorldLoc;
                pmember->GetWorldLocation(&memberWorldLoc);
                if (memberWorldLoc.pSpace == SearchCenter.pSpace) {
                    float fdistanceSquared = SearchCenter.GetDistanceSquared(memberWorldLoc);
                    if (fmaxDistanceFromCenterSquared < static_cast<double>(fdistanceSquared)) {
                        fmaxDistanceFromCenterSquared = fdistanceSquared;
                        if (fmaxDistanceFromCenterSquared > fsearchMaxRadius * fsearchMaxRadius) {
                            break;
                        }
                    }
                }
            }
            if (!(fmaxDistanceFromCenterSquared > fsearchMaxRadius * fsearchMaxRadius)) {
                double fdistance = NiSqrt(fmaxDistanceFromCenterSquared);
                fSearchRadius = fCombatSearchBufferDistance.Float() + fdistance;
                fSearchRadius = BSMIN(fSearchRadius, fsearchMaxRadius);
                fSearchRadius = BSMAX(fSearchRadius, fsearchMinRadius);
            }
            else {
                fSearchRadius = fsearchMaxRadius;
            }
        }
    }
}

void CombatGroup::BuildSearchTeleportDoorArray() {
    BGSWorldLocation memberWorldLoc;
    using doorArray_t = BSSimpleArray<TESObjectREFR*, 1024>;
    doorArray_t doorArray;
    float fsearchMinRadius = 0.0f;
    float fsearchMaxRadius = 0.0f;
    GetMinMaxSearchRadius(SearchCenter, fsearchMinRadius, fsearchMaxRadius);
    Pathing::FindTeleportDoorsForLocation(SearchCenter.pSpace, SearchCenter.Point, fSearchRadius, &doorArray);
    uint32_t imemberCount = GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        Actor* pmember = GetMember(i);
        pmember->GetWorldLocation(&memberWorldLoc);
        double Distance = SearchCenter.GetDistance(memberWorldLoc);
        if (!(fsearchMaxRadius * fsearchMaxRadius > Distance)) {
            Pathing::FindTeleportDoorsForLocation(
                memberWorldLoc.pSpace, memberWorldLoc.Point, fsearchMinRadius, &doorArray
            );
        }
    }
    doorArray.Sort([](const void* apOne, const void* apTwo) {
        return CompareUInt(static_cast<const uint32_t*>(apOne), static_cast<const uint32_t*>(apTwo));
    });
    for (uint32_t i_2 = 0; i_2 < doorArray.iSize; ++i_2) {
        TESObjectREFR* pdoor = doorArray.operator[](i_2);
        uint32_t inumDuplicates = 0;
        for (uint32_t j_2 = i_2 + 1; j_2 < doorArray.iSize && pdoor == doorArray.operator[](j_2); ++j_2) {
            ++inumDuplicates;
        }
        if (inumDuplicates) {
            doorArray.RemoveRange(i_2 + 1, inumDuplicates, false);
        }
    }
    for (uint32_t j = 0; j < doorArray.iSize; ++j) {
        AddSearchDoor(doorArray.operator[](j));
    }
}

void CombatGroup::UpdateSearchLocations() {
    for (uint32_t i = 0; i < SearchLocations.iSize; ++i) {
        CombatSearchLocation* searchLocation = &SearchLocations.operator[](i);
        float fscore = searchLocation->GetScore();
        if (fscore < 0.0 || searchLocation->iTargetID) {
            SearchLocations.RemoveFast(i--, 1u);
        }
    }
    uint32_t itargetCount = GetTargetCount();
    for (uint32_t i_2 = 0; i_2 < itargetCount; ++i_2) {
        float fdetectedScore;
        float fattackedScore;
        float fnoticedScore;
        CombatTarget* pcombatTarget = GetCombatTarget(i_2);
        CalculateTargetSearchLocationScores(pcombatTarget, fdetectedScore, fnoticedScore, fattackedScore);
        if (fdetectedScore > 0.0 || (fnoticedScore > 0.0 || fattackedScore > 0.0)) {
            if (!(fnoticedScore < static_cast<double>(fdetectedScore)) &&
                !(fnoticedScore < static_cast<double>(fattackedScore))) {
                if (fnoticedScore > 0.0) {
                    TESForm* Space;
                    BGSWorldLocation otherLoc;
                    float fotherScore = BSMAX(fdetectedScore, fattackedScore);
                    if (fotherScore > 0.0) {
                        if (!(fattackedScore < static_cast<double>(fdetectedScore))) {
                            otherLoc = pcombatTarget->LastAttackedMemberLocation;
                        }
                        else {
                            otherLoc = pcombatTarget->LastDetectedLocation;
                        }
                    }
                    if (fotherScore > 0.0 &&
                        (Space = otherLoc.pSpace, Space == pcombatTarget->LastNoticedLocation.pSpace)) {
                        NiPoint3 v8;
                        NiPoint3 interpolatedPoint = otherLoc.Point * fotherScore;
                        interpolatedPoint.operator+=(
                            *::new (&v8) NiPoint3(pcombatTarget->LastNoticedLocation.Point * fnoticedScore)
                        );
                        float v6 = fotherScore + fnoticedScore;
                        interpolatedPoint.operator/=(v6);
                        BGSWorldLocation interpolatedLocation(
                            interpolatedPoint, pcombatTarget->LastNoticedLocation.pSpace
                        );
                        uint32_t CastingType = pcombatTarget->pActor->iFormID;
                        AddSearchLocation(interpolatedLocation, fnoticedScore, CastingType);
                    }
                    else {
                        uint32_t v5 = pcombatTarget->pActor->iFormID;
                        AddSearchLocation(pcombatTarget->LastNoticedLocation, fnoticedScore, v5);
                    }
                }
            }
            else {
                if (!(fattackedScore <= static_cast<double>(fdetectedScore))) {
                    uint32_t v2 = pcombatTarget->pActor->iFormID;
                    AddSearchLocation(pcombatTarget->LastAttackedMemberLocation, fattackedScore, v2);
                }
                else {
                    uint32_t v1 = pcombatTarget->pActor->iFormID;
                    AddSearchLocation(pcombatTarget->LastDetectedLocation, fdetectedScore, v1);
                }
            }
        }
    }
}

bool CombatGroup::UpdateSearchArea() {
    float fSearchRadius;
    UpdateSearchLocations();
    BGSWorldLocation searchCenter = SearchCenter;
    fSearchRadius = this->fSearchRadius;
    ComputeSearchArea();
    uint8_t v4 = 0;
    v4 = (uint8_t)(((fSearchRadius > (this->fSearchRadius + 128.0)) ||
                    (fSearchRadius < (this->fSearchRadius - 128.0))) ||
                   ((&searchCenter)->GetDistanceSquared(SearchCenter) > 4096.0));
    if (v4) {
        BuildSearchTeleportDoorArray();
    }
    return (bool)v4;
}

void CombatGroup::UpdateGroupStrategy() {
    float a2;
    CombatTimer kTimeStamp;
    if (GetMemberCount() <= 1) {
        pGroupStrategy = nullptr;
        bStrategyForced = false;
        (&ChooseStrategyTimer)->Reset(0.0);
    }
    else {
        if ((!bStrategyForced) && (&ChooseStrategyTimer)->Check()) {
            pGroupStrategy = CombatGroup::SelectGroupStrategy(this);
            (&ChooseStrategyTimer)->Reset(5.0f);
            if (pGroupStrategy) {
                iLastGroupStrategyChosenIndex = pGroupStrategy->GetType();
                a2 = CombatTimer::GetTimer();
                (&kTimeStamp)->SetStartTime(a2);
                fLastGroupStrategyChosenTimeStamp.fTimeStamp = kTimeStamp.fStartTime;
            }
        }
        if ((pGroupStrategy) && ((&UpdateStrategyTimer)->Check())) {
            if (!pGroupStrategy->Update(this)) {
                pGroupStrategy = nullptr;
                bStrategyForced = false;
            }
            (&UpdateStrategyTimer)->Reset(1.0);
        }
    }
}

PathingAvoidNodeArray* CombatGroup::CreateMovementAvoidNodeArray(
    Actor* apActor,
    const NiPoint3& arDestination,
    float afExtraRadius,
    PathingAvoidNodeArray* apExistingAvoidNodeArray,
    Actor* apIgnoreActor
) {
    NiPoint3 result;
    NiPoint3 arPoint;
    NiPoint3 v25;
    NiPoint3 v28;
    BSSimpleArray<PathingAvoidNode, 1024>* v26 = apExistingAvoidNodeArray;
    const NiPoint3* v6 = &apActor->GetLocationOnReference();
    NiPoint3* v7 = &(result = *v6 + arDestination);
    v28 = *v7 / 2.0f;
    v25 = arDestination - v28;
    float v29 = v25.SqrLength() + afExtraRadius * afExtraRadius;
    float* v8 = &fMovementTargetAvoidRadiusMult.Float();
    float* v9 = &fMovementTargetAvoidRadiusMult.Float();
    v29 = *v8 * *v9 * v29;
    uint32_t TargetCount = GetTargetCount();
    for (uint32_t aiIndex = 0; aiIndex < TargetCount; ++aiIndex) {
        Actor* Target = GetTarget(aiIndex);
        if (Target != apIgnoreActor) {
            NiPoint3 v22;
            arPoint = Target->GetLocationOnReference();
            v22 = v28 - arPoint;
            double v10 = v22.SqrLength();
            if (v29 > v10) {
                if (!v26) {
                    PathingAvoidNodeArray* v15;
                    void* apMemory = NiMemObject::operator new(sizeof(PathingAvoidNodeArray));
                    if (apMemory) {
                        v15 = ::new (apMemory) PathingAvoidNodeArray;
                    }
                    else {
                        v15 = nullptr;
                    }
                    PathingAvoidNodeArray* v19 = v15;
                    v26 = v15;
                }
                float fScalar = fMovementTargetAvoidCost.Float();
                float* v11 = &fMovementTargetAvoidRadius.Float();
                v26->Add(PathingAvoidNode(arPoint, *v11, fScalar));
            }
        }
    }
    return static_cast<PathingAvoidNodeArray*>(v26);
}

PathingAvoidNodeArray*
CombatGroup::CreateFleeAvoidNodeArray(Actor* apActor, PathingAvoidNodeArray* apExistingAvoidNodeArray) {
    BGSWorldLocation ecx0;
    BGSWorldLocation apLocation;
    BSSimpleArray<PathingAvoidNode, 1024>* v20 = apExistingAvoidNodeArray;
    uint32_t TargetCount = GetTargetCount();
    float* v3 = &fCombatFleeTargetGatherRadius.Float();
    float v19 = *v3 * fCombatFleeTargetGatherRadius.Float();
    apActor->GetWorldLocation(&apLocation);
    for (uint32_t aiIndex = 0; aiIndex < TargetCount; ++aiIndex) {
        TESObjectREFR* Target = GetTarget(aiIndex);
        Target->GetWorldLocation(&ecx0);
        double DistanceSquared = apLocation.GetDistanceSquared(ecx0);
        if (v19 > DistanceSquared) {
            if (!v20) {
                PathingAvoidNodeArray* v10;
                void* apMemory = NiMemObject::operator new(sizeof(PathingAvoidNodeArray));
                if (apMemory) {
                    v10 = ::new (apMemory) PathingAvoidNodeArray;
                }
                else {
                    v10 = nullptr;
                }
                PathingAvoidNodeArray* v14 = v10;
                v20 = v10;
            }
            float afCost = fCombatFleeTargetAvoidCost.Float();
            float afRadius = fCombatFleeTargetAvoidRadius.Float();
            NiPoint3* v5 = ecx0.GetPoint();
            v20->Add(PathingAvoidNode(*v5, afRadius, afCost));
        }
    }
    return static_cast<PathingAvoidNodeArray*>(v20);
}

double CombatGroup::GetMemberCombatStrength(Actor* a1) {
    if (!a1) {
        return fMemberCombatStrength;
    }
    float fcombatStrength = 0.0f;
    uint32_t imemberCount = GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        CombatMember* pcombatMember = GetCombatMember(i);
        CombatController* pcombatController = pcombatMember->pActor->GetCombatController();
        if (a1 == pcombatMember->pActor && pcombatController) {
            float fmemberStrength = pcombatController->pCombatState->fCombatStrength;
            if (fmemberStrength > 0.0) {
                fcombatStrength = fcombatStrength + fmemberStrength;
            }
        }
        else {
            if ((!pcombatController || !pcombatController->IsFleeing()) && pcombatMember->fCombatStrength > 0.0) {
                fcombatStrength = fcombatStrength + pcombatMember->fCombatStrength;
            }
        }
    }
    return fcombatStrength;
}

void CombatGroup::UpdateClusters() {
    BGSWorldLocation apLocation;
    if (!CombatGroup::bUseSimpleCombatGroupClustering) {
        UpdateClusterCenters();
        if (ClusterUpdateTimer.Check()) {
            ClusterData clusterData[64];
            float fdistanceSqrArray[64][64];
            ClusterUpdateTimer.Reset(CombatGroup::fCombatClusterUpdateTime);
            float fclusterDistanceSquared = fClusterDistance * fClusterDistance * 4.0;
            // BSMIN is the int instantiation here, so the compare is signed.
            uint32_t imemberCount_2 = BSMIN(static_cast<int32_t>(GetMemberCount()), 63);
            ourMemset(clusterData, 0, sizeof(clusterData));
            ourMemset(fdistanceSqrArray, 0, sizeof(fdistanceSqrArray));
            for (uint32_t i = 0; i < imemberCount_2; ++i) {
                CombatMember* CombatMember_ = GetCombatMember(i);
                clusterData[i].pCombatMember = CombatMember_;
                BGSWorldLocation* WorldLocation = clusterData[i].pCombatMember->pActor->GetWorldLocation(&apLocation);
                clusterData[i].WorldLoc = *WorldLocation;
                clusterData[i].cClosestMember = -1;
                if ((clusterData[i].pCombatMember->pCluster) &&
                    (fClusterDistance * fClusterDistance * 4.0 <
                     clusterData[i].pCombatMember->pCluster->Center.GetDistanceSquared(clusterData[i].WorldLoc))) {
                    --clusterData[i].pCombatMember->pCluster->iMemberCount;
                    clusterData[i].pCombatMember->pCluster = nullptr;
                }
            }
            for (uint32_t i_2 = 0;; ++i_2) {
                uint32_t FormFlags = ClusterArray.iSize;
                if (i_2 >= FormFlags) {
                    break;
                }
                CombatGroupCluster* pcluster = ClusterArray.operator[](i_2);
                if (!pcluster->iMemberCount) {
                    ClusterArray.RemoveFast(i_2, 1u);
                    ourFree(pcluster);
                    --i_2;
                }
            }
            for (uint32_t i_3 = 0; i_3 < imemberCount_2; ++i_3) {
                if (!clusterData[i_3].pCombatMember->pCluster) {
                    float fclosestDistSqr = 3.4028235e+38f;
                    int8_t cclosestMember = 0;
                    for (uint32_t j = 0; j < imemberCount_2; ++j) {
                        if (i_3 != j) {
                            if (fdistanceSqrArray[i_3][j] == 0.0) {
                                float fdistSqr = clusterData[i_3].WorldLoc.GetDistanceSquared(clusterData[j].WorldLoc);
                                fdistanceSqrArray[i_3][j] = fdistSqr;
                                fdistanceSqrArray[j][i_3] = fdistSqr;
                            }
                            if (fclosestDistSqr > static_cast<double>(fdistanceSqrArray[i_3][j])) {
                                cclosestMember = j;
                                fclosestDistSqr = fdistanceSqrArray[i_3][j];
                            }
                        }
                    }
                    clusterData[i_3].cClosestMember = cclosestMember;
                }
            }
            for (uint32_t i_4 = 0; i_4 < imemberCount_2; ++i_4) {
                if (!clusterData[i_4].pCombatMember->pCluster &&
                    clusterData[clusterData[i_4].cClosestMember].cClosestMember == static_cast<int8_t>(i_4)) {
                    CombatGroupCluster* pcluster_2;
                    CombatGroupCluster* v10 = static_cast<CombatGroupCluster*>(ourMalloc(sizeof(CombatGroupCluster)));
                    if (v10) {
                        pcluster_2 = ::new (v10) CombatGroupCluster;
                    }
                    else {
                        pcluster_2 = nullptr;
                    }
                    CombatGroupCluster* pcluster_3 = pcluster_2;
                    ClusterArray.Add(pcluster_3);
                    clusterData[i_4].pCombatMember->pCluster = pcluster_3;
                    clusterData[clusterData[i_4].cClosestMember].pCombatMember->pCluster = pcluster_3;
                    NiPoint3* a2 = &(&clusterData[clusterData[i_4].cClosestMember].WorldLoc)->Point;
                    NiPoint3 center = clusterData[i_4].WorldLoc.Point + *a2;
                    center.operator/=(2.0f);
                    TESForm* a2a = clusterData[i_4].WorldLoc.pSpace;
                    pcluster_3->Center.SetLocation(center, a2a);
                }
            }
            for (uint32_t i_5 = 0;; ++i_5) {
                uint32_t v4 = ClusterArray.iSize;
                if (i_5 >= v4) {
                    break;
                }
                CombatGroupCluster* pcluster1 = ClusterArray.operator[](i_5);
                for (uint32_t j_2 = i_5 + 1;; ++j_2) {
                    uint32_t v5 = ClusterArray.iSize;
                    if (j_2 >= v5) {
                        break;
                    }
                    CombatGroupCluster* pcluster2 = ClusterArray.operator[](j_2);
                    if (fClusterDistance * fClusterDistance > pcluster1->Center.GetDistanceSquared(pcluster2->Center)) {
                        for (uint32_t k = 0; k < imemberCount_2; ++k) {
                            if (clusterData[k].pCombatMember->pCluster == pcluster2) {
                                clusterData[k].pCombatMember->pCluster = pcluster1;
                            }
                        }
                        ourFree(pcluster2);
                        ClusterArray.RemoveFast(j_2--, 1u);
                    }
                }
            }
            for (uint32_t i_6 = 0; i_6 < imemberCount_2; ++i_6) {
                if (!clusterData[i_6].pCombatMember->pCluster) {
                    CombatGroupCluster* pcluster_4 = nullptr;
                    uint32_t iclosestMember = clusterData[i_6].cClosestMember;
                    while (!pcluster_4) {
                        pcluster_4 = clusterData[iclosestMember].pCombatMember->pCluster;
                        iclosestMember = clusterData[iclosestMember].cClosestMember;
                    }
                    if (fClusterDistance * fClusterDistance >
                        clusterData[i_6].WorldLoc.GetDistanceSquared(pcluster_4->Center)) {
                        clusterData[i_6].pCombatMember->pCluster = pcluster_4;
                        for (iclosestMember = clusterData[i_6].cClosestMember;
                             !clusterData[iclosestMember].pCombatMember->pCluster;
                             iclosestMember = clusterData[iclosestMember].cClosestMember) {
                            clusterData[iclosestMember].pCombatMember->pCluster = pcluster_4;
                        }
                    }
                }
            }
            UpdateClusterCenters();
        }
    }
}

void CombatGroup::UpdateClusterCenters() {
    NiPoint3 centerPoint_2;
    NiPoint3 centerPoint;
    for (uint32_t i = 0; i < ClusterArray.iSize; ++i) {
        CombatGroupCluster* pcluster = ClusterArray.operator[](i);
        pcluster->Center.SetPoint(NiPoint3_ZERO);
        pcluster->iMemberCount = 0;
    }
    uint32_t imemberCount = GetMemberCount();
    for (uint32_t i_2 = 0; i_2 < imemberCount; ++i_2) {
        CombatMember* pcombatMember = GetCombatMember(i_2);
        if (pcombatMember->pCluster) {
            centerPoint = pcombatMember->pCluster->Center.Point;
            centerPoint.operator+=(pcombatMember->pActor->GetLocationOnReference());
            pcombatMember->pCluster->Center.SetPoint(centerPoint);
            ++pcombatMember->pCluster->iMemberCount;
        }
    }
    for (uint32_t i_3 = 0;; ++i_3) {
        uint32_t FormFlags = ClusterArray.iSize;
        if (i_3 >= FormFlags) {
            break;
        }
        CombatGroupCluster* pcluster_2 = ClusterArray.operator[](i_3);
        centerPoint_2 = pcluster_2->Center.Point;
        centerPoint_2.operator/=(static_cast<float>(pcluster_2->iMemberCount));
        pcluster_2->Center.SetPoint(centerPoint_2);
    }
}

void CombatGroup::ForceGroupStrategy(CombatGroupStrategy* apStrategy) {
    pGroupStrategy = apStrategy;
    if (pGroupStrategy) {
        CombatGroupData v5(this);
        ClearGroupStrategyAssignments();
        pGroupStrategy->Execute(this, &v5);
        iLastGroupStrategyChosenIndex = pGroupStrategy->GetType();
        float a2 = CombatTimer::GetTimer();
        CombatTimeStamp apMemory(a2);
        fLastGroupStrategyChosenTimeStamp = apMemory;
        bStrategyForced = true;
    }
    else {
        bStrategyForced = false;
    }
}

void CombatGroup::CopyTargets(CombatGroup* apGroup) {
    apGroup->TargetArray.CopyTo(&TargetArray);
}

bool CombatGroup::CheckRemoveCombatTarget(CombatTarget* apCombatTarget) {
    double v6;
    double v7;
    if (IsFleeing()) {
        double v12 = apCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
        if (fCombatGroupFleeingTargetLostRemoveTime.Float() < v12) {
            return true;
        }
        double v11 = apCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
        if (fCombatGroupFleeingTargetLostRemoveDistanceTime.Float() < v11) {
            float* v3 = &fCombatGroupFleeingTargetLostRemoveDistance.Float();
            if (!FindMemberNearActor(apCombatTarget->pActor, *v3)) {
                return true;
            }
        }
    }
    double v10 = apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
    if (fCombatTargetLostRemoveTime.Float() < v10) {
        if (!apCombatTarget->sAttackerCount) {
            return true;
        }
        double v9 = apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
        if (fCombatTargetLostRemoveDistanceTime.Float() < v9) {
            bool v14 = false;
            if ((iSearchCount == 1) && (spPathingLOSGridMap.m_pObject)) {
                v14 = spPathingLOSGridMap.m_pObject->fPercentSeen == 0.0;
            }

            if (v14) {
                return true;
            }
            float* v4 = &fCombatTargetLostRemoveDistance.Float();
            if (!FindMemberNearActor(apCombatTarget->pActor, *v4)) {
                return true;
            }
        }
    }
    if (!iSearchCount) {
        return apCombatTarget->pActor->IsUnConscious() ||
            (apCombatTarget->pActor->IsDead(false) && !apCombatTarget->pActor->IsDying());
    }
    int32_t sLastSearchNoticed = apCombatTarget->sLastSearchNoticed;
    if (iSearchCount > iCombatTargetLostRemoveSearchCount.Int() + sLastSearchNoticed) {
        return true;
    }
    double v8 = fSearchStartedTimeStamp.ElapsedTime();
    if (fCombatSearchTargetHiddenMaxTime.Float() > v8 &&
        ((v7 = apCombatTarget->fLastAttackedMemberTimeStamp.ElapsedTime(),
          fCombatSearchTargetHiddenMaxTime.Float() <= v7) &&
         (-3.4028235e+38f == apCombatTarget->fLastDetectedTimeStamp.fTimeStamp &&
          (v6 = apCombatTarget->fLastNoticedTimeStamp.ElapsedTime(),
           fCombatSearchTargetHiddenRemoveTime.Float() < v6)))) {
        return true;
    }

    return apCombatTarget->pActor->IsUnConscious() ||
        (apCombatTarget->pActor->IsDead(false) && !apCombatTarget->pActor->IsDying());
}

bool CombatGroup::MergeGroup(CombatGroup* apCombatGroup) {
    BSSimpleArray<CombatTarget, 1024>* pnewTargetArray = &apCombatGroup->TargetArray;
    for (uint32_t i = 0; i < pnewTargetArray->iSize; ++i) {
        CombatTarget* combatTarget = &pnewTargetArray->operator[](i);
        if (!CanAddTarget(combatTarget->pActor)) {
            return false;
        }
    }
    for (uint32_t i_2 = 0; i_2 < TargetArray.iSize; ++i_2) {
        CombatTarget* combatTarget_2 = &TargetArray.operator[](i_2);
        if (!apCombatGroup->CanAddTarget(combatTarget_2->pActor)) {
            return false;
        }
    }
    for (uint32_t i_3 = 0;; ++i_3) {
        char ClusterCount = apCombatGroup->GetClusterCount();
        if (i_3 >= ClusterCount) {
            break;
        }
        CombatGroupCluster* pcluster = apCombatGroup->GetCluster(i_3);
        ClusterArray.Add(pcluster);
    }
    apCombatGroup->ClearClusterArray();
    BSSimpleArray<CombatMember, 1024>* pnewMemberArray = &apCombatGroup->MemberArray;
    for (uint32_t i_4 = 0; i_4 < pnewMemberArray->iSize; ++i_4) {
        CombatMember* combatMember = &pnewMemberArray->operator[](i_4);
        if (AddMember(combatMember->pActor)) {
            combatMember->pActor->SetCombatGroup(this);
            CombatMember* pcombatMember = GetCombatMemberForActor(combatMember->pActor);
            if (pcombatMember) {
                pcombatMember->CopyData(combatMember);
            }
        }
        else {
            const char* v4 = combatMember->pActor->GetFormEditorID();
            Warning("COMBAT: Error merging groups.  Could not add member %08X %s", combatMember->pActor->iFormID, v4);
        }
    }
    for (uint32_t i_5 = 0; i_5 < pnewTargetArray->iSize; ++i_5) {
        CombatTarget* combatTarget_3 = &pnewTargetArray->operator[](i_5);
        if (AddTarget(combatTarget_3->pActor)) {
            CombatTarget* pcombatTarget = GetCombatTargetForActor(combatTarget_3->pActor);
            if (pcombatTarget) {
                pcombatTarget->CopyData(combatTarget_3);
            }
        }
        else {
            const char* v5 = combatTarget_3->pActor->GetFormEditorID();
            Warning("COMBAT: Error merging groups.  Could not add target %08X %s", combatTarget_3->pActor->iFormID, v5);
        }
    }
    char ccombatMusicState = apCombatGroup->GetCombatMusicState();
    if (ccombatMusicState != -1) {
        cCombatMusicState = ccombatMusicState;
    }
    return true;
}

void CombatGroup::ClearClusterArray() {
    ClusterArray.Clear(true);
}

int32_t CombatGroup::GetDetectionValue(Actor* a1) {
    int32_t a4 = -100;
    CombatTimeStamp a5;
    bool a6 = false;
    bool a7 = false;
    GetTargetDetectionLevel(nullptr, a1, a4, a5, a6, a7);
    return a4;
}

CombatTimeStamp* CombatGroup::GetTargetLastDetectedTimeStamp(CombatTimeStamp* apResult, Actor* apTarget) {
    CombatTarget* pcombatTarget = GetCombatTargetForActor(apTarget);
    if (pcombatTarget) {
        *apResult = pcombatTarget->fLastDetectedTimeStamp;
    }
    else {
        ::new (apResult) CombatTimeStamp(-3.4028235e+38f);
    }
    return apResult;
}

bool CombatGroup::GetTargetLastSeenLocation(
    uint32_t aiIndex,
    BGSWorldLocation& arRetLastSeenLocation,
    float afMaxTime
) {
    CombatTarget* pCombatTarget = GetCombatTarget(aiIndex);
    if (!pCombatTarget || !(static_cast<double>(afMaxTime) > pCombatTarget->fLastSeenTimeStamp.ElapsedTime())) {
        return false;
    }
    arRetLastSeenLocation = pCombatTarget->LastSeenLocation;
    return true;
}

PathingAvoidNodeArray* CombatGroup::CreateFlankingAvoidNodeArray(
    Actor* apActor,
    const NiPoint3& arDestination,
    float afExtraRadius,
    PathingAvoidNodeArray* apExistingAvoidNodeArray
) {
    NiPoint3 targetCenter;
    float fradius = 1024.0f;
    float fboundingSphereRadiusSquared = ComputeTargetBoundingSphere(targetCenter);
    if (fboundingSphereRadiusSquared > fradius * fradius) {
        fradius = NiSqrt(fboundingSphereRadiusSquared);
    }
    PathingAvoidNodeArray* pavoidNodeArray = apExistingAvoidNodeArray;
    if (!apExistingAvoidNodeArray) {
        PathingAvoidNodeArray* pavoidNodeArray_2;
        void* apMemory = NiMemObject::operator new(sizeof(PathingAvoidNodeArray));
        if (apMemory) {
            pavoidNodeArray_2 = ::new (apMemory) PathingAvoidNodeArray;
        }
        else {
            pavoidNodeArray_2 = nullptr;
        }
        pavoidNodeArray = pavoidNodeArray_2;
    }
    float v7 = fMovementTargetAvoidCost.Float() * 4.0;
    pavoidNodeArray->Add(PathingAvoidNode(targetCenter, fradius, v7));
    return pavoidNodeArray;
}

float CombatGroup::ComputeMemberBoundingSphere(NiPoint3& arCenter) {
    NiPoint3 corners[8];
    rt::eh_vector_constructor_iterator(corners, sizeof(NiPoint3), 8, &emptyPointConstructor);
    ComputeMemberBoundingBox(corners, false);
    arCenter = (*corners + corners[7]) / 2.0f;
    NiPoint3 vec = *corners - corners[7];
    return vec.SqrLength();
}

float CombatGroup::ComputeTargetBoundingSphere(NiPoint3& arCenter) {
    // Eight corners, as ComputeMemberBoundingSphere has them: the box's opposite corners are the first and the last.
    NiPoint3 corners[8];
    rt::eh_vector_constructor_iterator(corners, sizeof(NiPoint3), 8, &emptyPointConstructor);
    ComputeTargetBoundingBox(corners, false);
    arCenter = (*corners + corners[7]) / 2.0f;
    NiPoint3 vec = *corners - corners[7];
    return vec.SqrLength();
}

void CombatGroup::ComputeMemberBoundingBox(NiPoint3* arPoints, bool a3) {
    if (!a3) {
        for (uint32_t j = 0; j < 8; ++j) {
            for (uint32_t a2 = 0; a2 < 3; ++a2) {
                float v3;
                if (CombatGroup_BoundsCornerIsMax[j][a2]) {
                    v3 = -3.4028235e+38f;
                }
                else {
                    v3 = 3.4028235e+38f;
                }
                arPoints[j].operator[](a2) = v3;
            }
        }
    }
    uint32_t MemberCount = GetMemberCount();
    for (uint32_t aiIndex = 0; aiIndex < MemberCount; ++aiIndex) {
        Actor* Member = GetMember(aiIndex);
        NiPoint3 v9 = Member->GetLocationOnReference();
        for (uint32_t k = 0; k < 8; ++k) {
            for (uint32_t i = 0; i < 3; ++i) {
                float v5 = v9.operator[](i);
                if (CombatGroup_BoundsCornerIsMax[k][i]) {
                    if (arPoints[k].operator[](i) < static_cast<double>(v5)) {
                        arPoints[k].operator[](i) = v5;
                    }
                }
                else {
                    if (arPoints[k].operator[](i) > static_cast<double>(v5)) {
                        arPoints[k].operator[](i) = v5;
                    }
                }
            }
        }
    }
}

bool CombatGroup::CheckGroupStrategyChosenTimeStamp(uint32_t aiIndex, float afMinimumTimePassed) {
    if (fLastGroupStrategyChosenTimeStamp.fTimeStamp == 0.0) {
        return true;
    }
    if (iLastGroupStrategyChosenIndex == aiIndex) {
        return static_cast<double>(afMinimumTimePassed) < fLastGroupStrategyChosenTimeStamp.ElapsedTime();
    }
    return true;
}

float CombatGroup::ComputeMemberHealthPercentage() {
    float fhealthPercentage = 0.0f;
    uint32_t imemberCount = GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        Actor* pmember = GetMember(i);
        double fHealth = pmember->GetActorFloatValue(ActorValue::HEALTH);
        fhealthPercentage =
            fHealth / static_cast<double>(pmember->GetBaseActorValue(ActorValue::HEALTH)) + fhealthPercentage;
    }
    return fhealthPercentage / static_cast<double>(imemberCount);
}

void CombatGroup::UpdateSearch() {
    NiPoint3 closestPoint;
    if (iInitializedMemberCount) {
        uint32_t itargetCount = GetTargetCount();
        bool bdontSearch = false;
        if (!itargetCount || iFleeingMemberCount >= iInitializedMemberCount) {
            bdontSearch = true;
        }
        if (IsSearching()) {
            if (bdontSearch) {
                ClearSearch();
            }
            else {
                for (uint32_t i = 0; i < itargetCount; ++i) {
                    CombatTarget* pcombatTarget = GetCombatTarget(i);
                    // The elapsed time is stored as a double before the compare.
                    double fElapsed = pcombatTarget->fLastDetectedTimeStamp.ElapsedTime();
                    if (fCombatDetectionBufferTime.Float() > fElapsed) {
                        ClearSearch();
                        break;
                    }
                }
            }
        }
        else {
            if (!bdontSearch) {
                uint32_t ilostTargetCount = 0;
                uint32_t itargetedTargets = 0;
                for (uint32_t i_2 = 0; i_2 < itargetCount; ++i_2) {
                    CombatTarget* pcombatTarget_2 = GetCombatTarget(i_2);
                    // The elapsed time is stored as a double before the compare.
                    double fElapsed = pcombatTarget_2->fLastDetectedTimeStamp.ElapsedTime();
                    if (fCombatDetectionLostTime.Float() <= fElapsed) {
                        ++ilostTargetCount;
                    }
                    if (pcombatTarget_2->sAttackerCount) {
                        ++itargetedTargets;
                    }
                }
                if ((itargetedTargets) && (ilostTargetCount == itargetCount)) {
                    StartSearch();
                }
            }
        }
    }
    if (IsSearching()) {
        if ((pSearchingMember) && (pSearchingMember->DidPathingFail() || !pSearchingMember->IsWaitingOnPath())) {
            pSearchingMember = nullptr;
        }

        if (SearchUpdateTimer.Check()) {
            if ((spPathingLOSGridMap.m_pObject) && (spPathingLOSGridMap.m_pObject->fPercentSeen > 0.9900000095367432)) {
                spPathingLOSGridMap.m_pObject->ClearSeen();
                ++iSearchCount;
                SearchAreaUpdateTimer.ForceTrigger();
            }

            bool bsearchAreaChanged = false;
            if (SearchAreaUpdateTimer.Check()) {
                bsearchAreaChanged = UpdateSearchArea();
                float* v2 = &fCombatSearchAreaUpdateTime.Float();
                SearchAreaUpdateTimer.Reset(*v2);
            }
            if (SearchCenter.IsLocationLoaded()) {
                if (!spPathingLOSGridMap.m_pObject || bsearchAreaChanged) {
                    TESWorldSpace* pworld = nullptr;
                    TESObjectCELL* pcell = nullptr;
                    if ((SearchCenter.GetCellOrWorld(pcell, pworld)) &&
                        (Pathing::FindClosestPointOnNavmesh(pworld, pcell, SearchFocalPoint, &closestPoint))) {
                        PathingLocation searchPathingCenter(&closestPoint, pcell, pworld);
                        if (searchPathingCenter.ResolveNavMeshInfo(nullptr)) {
                            searchPathingCenter.SetLocationSimple(SearchCenter.Point);
                            if (!spPathingLOSGridMap.m_pObject) {
                                PathingLOSGridMap* v8;
                                PathingLOSGridMap* v13 = static_cast<PathingLOSGridMap*>(
                                    NiMemObject::operator new(sizeof(PathingLOSGridMap))
                                );
                                if (v13) {
                                    v8 = ::new (v13) PathingLOSGridMap;
                                }
                                else {
                                    v8 = nullptr;
                                }
                                spPathingLOSGridMap.operator=(v8);
                            }
                            spPathingLOSGridMap.m_pObject->Initialize(searchPathingCenter, fSearchRadius);
                            float* v3 = &fCombatSearchSightRadius.Float();
                            spPathingLOSGridMap.m_pObject->SetSightRadius(*v3);
                            bUpdateSearchDebugGeometry = true;
                        }
                    }
                }
            }
            else {
                if (spPathingLOSGridMap.m_pObject) {
                    spPathingLOSGridMap.operator=(static_cast<PathingLOSGridMap*>(nullptr));
                }
            }
            if (spPathingLOSGridMap.m_pObject) {
                uint32_t imemberCount = GetMemberCount();
                BSScrapArray<PathingLocation> Locations(imemberCount, 0u, nullptr);
                BSScrapArray<float> Headings(imemberCount, 0u, nullptr);
                for (uint32_t i_3 = 0; i_3 < imemberCount; ++i_3) {
                    Actor* pmember = GetMember(i_3);
                    TESForm* Space = pmember->GetSpace();
                    if (Space == SearchCenter.pSpace) {
                        PathingLocation memberPathingLocation(pmember);
                        if (memberPathingLocation.HasNavMesh()) {
                            Locations.Add(memberPathingLocation);
                            float a2 = pmember->GetHeading(false);
                            float aItem = ClampAngle(a2);
                            Headings.Add(aItem);
                        }
                    }
                }
                if (Locations.iSize) {
                    float fseenPercent = spPathingLOSGridMap.m_pObject->fPercentSeen;
                    Pathing::ComputeLineOfSightMap(spPathingLOSGridMap, Locations, &Headings);
                    double v5 = spPathingLOSGridMap.m_pObject->fPercentSeen;
                    if (fseenPercent != v5) {
                        bUpdateSearchDebugGeometry = true;
                    }
                }
            }
            float* v6 = &fCombatSearchUpdateTime.Float();
            SearchUpdateTimer.Reset(*v6);
            uint32_t imemberCount_2 = GetMemberCount();
            uint32_t idialogueMember = RandomInt(0, static_cast<int32_t>(imemberCount_2));
            Actor* pmember_2 = GetMember(idialogueMember);
            Actor* ptarget = pmember_2->GetCombatTarget();
            if ((pmember_2 != pPlayer) && (ptarget)) {
                CombatTarget* pcombatTarget_3 = GetCombatTargetForActor(ptarget);
                if (!pcombatTarget_3 || -3.4028235e+38f == pcombatTarget_3->fLastDetectedTimeStamp.fTimeStamp) {
                    pCombatDialogueManager->StartDialogue(pmember_2, ptarget, DT_DETECTION, 0, false, nullptr);
                }
                else {
                    pCombatDialogueManager->StartDialogue(pmember_2, ptarget, DT_DETECTION, 1, false, nullptr);
                }
            }
        }
    }
}

// The last detected and last noticed targets are found and stored but never read, as the game does.
void CombatGroup::StartSearch() {
    uint32_t itargetCount = GetTargetCount();
    UpdateSearchLocations();
    ComputeSearchArea();
    BuildSearchTeleportDoorArray();
    iSearchCount = 1;
    SearchUpdateTimer.Reset(0.5f);
    SearchAreaUpdateTimer.Reset(fCombatSearchAreaUpdateTime.Float());
    CombatTimeStamp kStarted(CombatTimer::GetTimer());
    fSearchStartedTimeStamp = kStarted;
    CombatTimeStamp flastDetectedTargetTimeStamp;
    flastDetectedTargetTimeStamp.~CombatTimeStamp();
    ::new (&flastDetectedTargetTimeStamp) CombatTimeStamp(-3.4028235e+38f);
    CombatTimeStamp flastNoticedTargetTimeStamp;
    flastNoticedTargetTimeStamp.~CombatTimeStamp();
    ::new (&flastNoticedTargetTimeStamp) CombatTimeStamp(-3.4028235e+38f);
    [[maybe_unused]] Actor* plastDetectedTarget = nullptr;
    [[maybe_unused]] Actor* plastNoticedTarget = nullptr;
    for (uint32_t i = 0; i < itargetCount; ++i) {
        CombatTarget* pcombatTarget = GetCombatTarget(i);
        pcombatTarget->sLastSearchNoticed = 0;
        if (flastDetectedTargetTimeStamp.fTimeStamp < pcombatTarget->fLastDetectedTimeStamp.fTimeStamp) {
            flastDetectedTargetTimeStamp.fTimeStamp = pcombatTarget->fLastDetectedTimeStamp.fTimeStamp;
            plastDetectedTarget = pcombatTarget->pActor;
        }
        else if (flastNoticedTargetTimeStamp.fTimeStamp < pcombatTarget->fLastNoticedTimeStamp.fTimeStamp) {
            flastNoticedTargetTimeStamp.fTimeStamp = pcombatTarget->fLastNoticedTimeStamp.fTimeStamp;
            plastNoticedTarget = pcombatTarget->pActor;
        }
    }
    float fMax = fCombatDetectionDialogueIdleMaxElapsedTime.Float();
    DetectionDialogueTimers[0].Reset(RandomFloat(fCombatDetectionDialogueIdleMinElapsedTime.Float(), fMax));
    float fMax2 = fCombatDetectionDialogueIdleMaxElapsedTime.Float();
    DetectionDialogueTimers[1].Reset(RandomFloat(fCombatDetectionDialogueIdleMinElapsedTime.Float(), fMax2));
}

void CombatGroup::GetMinMaxSearchRadius(BGSWorldLocation& arSearchLocation, float& arfMinRadius, float& arfMaxRadius) {
    int32_t v4 = !arSearchLocation.pSpace || arSearchLocation.pSpace->GetFormType() != CELL_ID;
    int32_t v6 = iSearchCount >= 2;
    arfMinRadius = pSearchRadius[v4][v6]->Float();
    arfMaxRadius = pSearchRadius[v4][v6 + 1]->Float();
}

void CombatGroup::ClearSearch() {
    CombatTimer apMemory;
    spPathingLOSGridMap.operator=(static_cast<PathingLOSGridMap*>(nullptr));
    spSearchDebugGeometry.operator=(static_cast<NiRefObject*>(nullptr));
    bUpdateSearchDebugGeometry = false;
    SearchUpdateTimer.Clear();
    SearchAreaUpdateTimer.Clear();
    apMemory.SetStartTime(-3.4028235e+38f);
    fSearchStartedTimeStamp.fTimeStamp = apMemory.fStartTime;
    iSearchCount = 0;
    fSearchRadius = 0.0f;
    SearchCenter.Clear();
    SearchFocalPoint = NiPoint3_ZERO;
    SearchLocations.Clear(true);
    SearchTeleportDoors.Clear(true);
}

CombatSearchLocation*
CombatGroup::FindSearchLocation(Actor* apMember, uint8_t acMinScore, BGSWorldLocation* apCurrentSearchLocation) {
    BGSWorldLocation memberWorldLoc;
    CombatController* pcombatController = apMember->GetCombatController();
    apMember->GetWorldLocation(&memberWorldLoc);
    int32_t iindex = -1;
    float fcurrentScore = 0.0f;
    bool bcurrentScoreTarget = false;
    uint32_t isearchLocationCount = GetNumSearchLocations();
    for (uint32_t i = 0; i < isearchLocationCount; ++i) {
        CombatSearchLocation* searchLoc = &GetSearchLocation(i);
        float fscore = searchLoc->GetScore();
        float fscoreBonus = 0.0f;
        if (apCurrentSearchLocation) {
            TESForm* Space = searchLoc->WorldLoc.pSpace;
            if (Space == apCurrentSearchLocation->pSpace &&
                searchLoc->WorldLoc.Point == apCurrentSearchLocation->Point) {
                fscoreBonus = fscoreBonus + 5.0;
            }
        }
        if (!(static_cast<double>(acMinScore) > fscore + fscoreBonus)) {
            searchLoc->GetModifiedScore(memberWorldLoc);
            if (!(fcurrentScore > static_cast<double>(fscore)) &&
                (!pcombatController ||
                 pcombatController->pCombatState->CheckMovement(searchLoc->WorldLoc, true, false))) {
                iindex = i;
                fcurrentScore = fscore;
            }
        }
    }
    if (iindex == -1) {
        return nullptr;
    }

    return &GetSearchLocation(static_cast<uint32_t>(iindex));
}

void CombatGroup::CalculateTargetSearchLocationScores(
    CombatTarget* apCombatTarget,
    float& arfDetectedScore,
    float& arfNoticedScore,
    float& arfAttackedScore
) {
    double v5 = 70.0 - apCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
    arfDetectedScore = fCombatDetectionLostTime.Float() + v5;
    arfNoticedScore = 30.0 - apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
    arfAttackedScore = 70.0 - apCombatTarget->fLastAttackedMemberTimeStamp.ElapsedTime();
}

TESObjectREFR* CombatGroup::FindSearchDoor(Actor* apMember, float afMaxDistance, bool abCheckExist) {
    BGSWorldLocation teleportWorldLoc;
    BGSWorldLocation doorWorldLoc;
    BGSWorldLocation memberWorldLoc;
    if (apMember->IsCreature()) {
        TESActorBaseData* v7 = static_cast<TESActorBaseData*>(apMember->GetActorBase());
        if (v7->GetCantOpenDoors()) {
            return nullptr;
        }
    }
    CombatController* pcombatController = apMember->GetCombatController();
    apMember->GetWorldLocation(&memberWorldLoc);
    TESObjectREFR* pclosestDoor = nullptr;
    float fclosestDoorDistanceSquared = afMaxDistance * afMaxDistance;
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = SearchTeleportDoors.iSize;
        if (i >= FormFlags) {
            break;
        }
        CombatSearchDoor* searchDoor = &SearchTeleportDoors.operator[](i);
        if ((searchDoor->pDoor) && (searchDoor->pLinkedDoor)) {
            uint32_t cFailedAttempts = searchDoor->cFailedAttempts;
            if (cFailedAttempts <= iCombatSearchMaxSearchDoorFailure.Int() && !searchDoor->bReserved &&
                !searchDoor->bInvestigated) {
                DoorTeleportData* pteleportData = searchDoor->pDoor->GetTeleport();
                if ((pteleportData) &&
                    (!abCheckExist ||
                     (pteleportData->GetTeleportWorldLocation(&teleportWorldLoc),
                      CombatUtilities::CanActorExistInSpace(apMember, teleportWorldLoc)))) {
                    searchDoor->pDoor->GetWorldLocation(&doorWorldLoc);
                    if (!pcombatController ||
                        pcombatController->pCombatState->CheckMovement(doorWorldLoc, true, false)) {
                        float fdistanceSquared = memberWorldLoc.GetDistanceSquared(doorWorldLoc);
                        if (fclosestDoorDistanceSquared > static_cast<double>(fdistanceSquared)) {
                            pclosestDoor = searchDoor->pDoor;
                            fclosestDoorDistanceSquared = fdistanceSquared;
                        }
                    }
                }
            }
        }
    }
    return pclosestDoor;
}

void CombatGroup::AddSearchDoor(TESObjectREFR* apDoor) {
    if (!GetCombatSearchDoor(apDoor)) {
        DoorTeleportData* pteleportData = apDoor->GetTeleport();
        if ((pteleportData) && (pteleportData->pLinkedDoor)) {
            uint32_t iindex = SearchTeleportDoors.AddUninitialized();
            (&SearchTeleportDoors.GetAt(static_cast<int32_t>(iindex)))->pDoor = apDoor;
            TESObjectREFR* LinkedDoor = pteleportData->pLinkedDoor;
            (&SearchTeleportDoors.GetAt(static_cast<int32_t>(iindex)))->pLinkedDoor = LinkedDoor;
            (&SearchTeleportDoors.GetAt(static_cast<int32_t>(iindex)))->cFailedAttempts = 0;
            (&SearchTeleportDoors.GetAt(static_cast<int32_t>(iindex)))->bReserved = false;
            (&SearchTeleportDoors.GetAt(static_cast<int32_t>(iindex)))->bInvestigated = false;
        }
    }
}

bool CombatGroup::CalculateSearchIgnoreLocations(Actor* apSearcher, BSSimpleArray<NiPoint3, 1024>* apIgnoreLocations) {
    NiPoint3 ecx0;
    uint32_t MemberCount = GetMemberCount();
    if (MemberCount <= 1) {
        return false;
    }
    bool v9 = false;
    for (uint32_t aiIndex = 0; aiIndex < MemberCount; ++aiIndex) {
        Actor* Member = GetMember(aiIndex);
        if (Member != apSearcher) {
            CombatController* v6 = Member->GetCombatController();
            if ((v6) && (v6->IsSearching()) && (Member->GetCurrentPathfindingGoal_008B3840(&ecx0))) {
                apIgnoreLocations->Add(ecx0);
                v9 = true;
            }
        }
    }
    return v9;
}

void CombatGroup::UpdateCombatStrength() {
    fMemberCombatStrength = 0.0f;
    fAverageMemberDamagePerSecond = 0.0f;
    uint32_t imemberCount = GetMemberCount();
    uint32_t inonFleeingMemberCount = 0;
    for (uint32_t i = 0; i < imemberCount; ++i) {
        bool bfleeing = false;
        CombatMember* pcombatMember = GetCombatMember(i);
        if (pcombatMember->pActor == pPlayer) {
            pcombatMember->fDamagePerSecond = pPlayer->GetDamagePerSecond();
            pcombatMember->fCombatStrength = pPlayer->CalculateCombatStrength(pcombatMember->fDamagePerSecond);
        }
        else {
            CombatController* pcombatController = pcombatMember->pActor->GetCombatController();
            if (pcombatController) {
                bfleeing = pcombatController->IsFleeing();
                pcombatMember->fDamagePerSecond = pcombatController->pCombatState->fDamagePerSecond;
                pcombatMember->fCombatStrength = pcombatController->pCombatState->fCombatStrength;
            }
        }
        if (!bfleeing && pcombatMember->fCombatStrength >= 0.0 && pcombatMember->fDamagePerSecond >= 0.0) {
            fMemberCombatStrength = fMemberCombatStrength + pcombatMember->fCombatStrength;
            fAverageMemberDamagePerSecond = fAverageMemberDamagePerSecond + pcombatMember->fDamagePerSecond;
            ++inonFleeingMemberCount;
        }
    }
    if (inonFleeingMemberCount) {
        fAverageMemberDamagePerSecond = fAverageMemberDamagePerSecond / static_cast<double>(inonFleeingMemberCount);
    }
    fTargetCombatStrength = 0.0f;
    fAverageTargetCombatStrength = 0.0f;
    uint32_t itargetCount = GetTargetCount();
    uint32_t inonFleeingTargetCount = 0;
    for (uint32_t i_2 = 0; i_2 < itargetCount; ++i_2) {
        float fcombatStrength = -1.0f;
        Actor* ptarget = GetTarget(i_2);
        if (ptarget == pPlayer) {
            fcombatStrength = pPlayer->CalculateCombatStrength(-1.0f);
        }
        else {
            if (!ptarget->IsFleeing(false)) {
                CombatController* pcombatController_2 = ptarget->GetCombatController();
                if (pcombatController_2) {
                    fcombatStrength = pcombatController_2->pCombatState->fCombatStrength;
                }
                else {
                    fcombatStrength = ptarget->CalculateCombatStrength(-1.0f);
                }
            }
        }
        if (fcombatStrength >= 0.0) {
            fTargetCombatStrength = fTargetCombatStrength + fcombatStrength;
            ++inonFleeingTargetCount;
        }
    }
    if (inonFleeingTargetCount) {
        fAverageTargetCombatStrength = fTargetCombatStrength / static_cast<double>(inonFleeingTargetCount);
    }
}

double CombatGroup::GetAverageMemberDamagePerSecond(Actor* a1) {
    if (!a1) {
        return fAverageMemberDamagePerSecond;
    }
    float faverageDPS = 0.0f;
    uint32_t imemberCount = GetMemberCount();
    uint32_t inonFleeingMemberCount = 0;
    for (uint32_t i = 0; i < imemberCount; ++i) {
        CombatMember* pcombatMember = GetCombatMember(i);
        CombatController* pcombatController = pcombatMember->pActor->GetCombatController();
        if (a1 == pcombatMember->pActor && pcombatController) {
            float fmemberDPS = pcombatController->pCombatState->fDamagePerSecond;
            if (fmemberDPS > 0.0) {
                faverageDPS = faverageDPS + fmemberDPS;
                ++inonFleeingMemberCount;
            }
        }
        else {
            if ((!pcombatController || !pcombatController->IsFleeing()) && pcombatMember->fCombatStrength >= 0.0) {
                faverageDPS = faverageDPS + pcombatMember->fDamagePerSecond;
                ++inonFleeingMemberCount;
            }
        }
    }
    if (inonFleeingMemberCount) {
        return static_cast<float>(faverageDPS / static_cast<double>(inonFleeingMemberCount));
    }
    return faverageDPS;
}

Actor* CombatGroup::FindClusterPartner(Actor* apMember, Actor* apCurrentPartner, float afMaxDistance) {
    float fpartnerMemberRadiusSquared_2 = afMaxDistance * afMaxDistance;
    Actor* ppartner = nullptr;
    NiPoint3 memberLoc = apMember->GetLocationOnReference();
    uint32_t imemberCount = GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        Actor* pmember = GetMember(i);
        if (pmember != apMember) {
            const NiPoint3* v4 = &pmember->GetLocationOnReference();
            NiPoint3 vec = *v4 - memberLoc;
            float fsquaredLength = vec.SqrLength();
            if (pmember == apCurrentPartner) {
                fsquaredLength = fsquaredLength / 2.25;
            }
            if (fpartnerMemberRadiusSquared_2 > static_cast<double>(fsquaredLength)) {
                fpartnerMemberRadiusSquared_2 = fsquaredLength;
                ppartner = pmember;
            }
        }
    }
    if (ppartner) {
        CombatController* ppartnerCombatController = ppartner->GetCombatController();
        if (ppartnerCombatController) {
            CombatLocationTarget* plocationTarget = ppartnerCombatController->pCombatState->pLocationTarget;
            if ((plocationTarget) && (plocationTarget->pRef == apMember)) {
                return nullptr;
            }
        }
    }
    return ppartner;
}

void CombatGroup::UpdateTargets() {
    uint32_t TargetCount = GetTargetCount();
    if (TargetCount) {
        float a2 = -3.4028235e+38f;
        CombatTimeStamp apMemory(a2);
        for (uint32_t auiIndex = 0; auiIndex < TargetCount; ++auiIndex) {
            CombatTarget* apCombatTarget = GetCombatTarget(auiIndex);
            if (CheckRemoveCombatTarget(apCombatTarget)) {
                if (TargetCount == 1) {
                    Actor* a1 = FindClosestMemberToActor(apCombatTarget->pActor, 1e+06f);
                    if (!a1) {
                        a1 = FindClosestMemberToActor(pPlayer, 1e+06f);
                    }
                    if ((a1) && (!apCombatTarget->pActor->IsDead(false))) {
                        if (-3.4028235e+38f == apCombatTarget->fLastDetectedTimeStamp.operator float()) {
                            pCombatDialogueManager->StartDialogue(
                                a1, apCombatTarget->pActor, DT_DETECTION, 5, true,
                                static_cast<CombatController*>(nullptr)
                            );
                        }
                        else {
                            pCombatDialogueManager->StartDialogue(
                                a1, apCombatTarget->pActor, DT_DETECTION, 8, true,
                                static_cast<CombatController*>(nullptr)
                            );
                        }
                    }
                }
                RemoveTarget(auiIndex--);
                --TargetCount;
            }
        }
    }
}

void CombatGroup::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    char cgroupStrategyType;
    uint32_t isaveGameCount_3;
    uint32_t isaveGameCount_2;
    uint32_t isaveGameCount;
    bool bhasPathingLOSGridMap;
    char apData;
    isaveGameCount = ClusterArray.iSize;
    uint32_t aiVariableValue = isaveGameCount;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t auiIndex = 0; auiIndex < aiVariableValue; ++auiIndex) {
        CombatGroupCluster** pcluster = &ClusterArray.operator[](auiIndex);
        (*pcluster)->SaveGame(apSaveGameBuffer, this);
    }
    // The address &TargetArray is never null, so it is not tested.
    isaveGameCount_2 = TargetArray.iSize;
    uint32_t auiSize = isaveGameCount_2;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2);
    for (int32_t a2 = 0; a2 < auiSize; ++a2) {
        CombatTarget* combatTarget = &TargetArray.operator[](a2);
        combatTarget->SaveGame(apSaveGameBuffer, this);
    }
    // The address &MemberArray is never null, so it is not tested.
    isaveGameCount_3 = MemberArray.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_3);
    for (uint32_t i = 0; i < isaveGameCount_3; ++i) {
        CombatMember* combatMember = &MemberArray.operator[](i);
        combatMember->SaveGame(apSaveGameBuffer, this);
    }
    if (pGroupStrategy) {
        cgroupStrategyType = pGroupStrategy->eType;
    }
    else {
        cgroupStrategyType = -1;
    }
    apData = cgroupStrategyType;
    apSaveGameBuffer->SaveDataEndian(&apData, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bStrategyForced, 1u, 0);
    ChooseStrategyTimer.SaveGame(apSaveGameBuffer);
    UpdateStrategyTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&iLastGroupStrategyChosenIndex, 4u, 0);
    fLastGroupStrategyChosenTimeStamp.SaveGame(apSaveGameBuffer);
    AvoidThreatDialogueTimer.SaveGame(apSaveGameBuffer);
    ClusterUpdateTimer.SaveGame(apSaveGameBuffer);
    TargetUpdateTimer.SaveGame(apSaveGameBuffer);
    for (uint32_t i_2 = 0; i_2 < 10; ++i_2) {
        DetectionDialogueTimers[i_2].SaveGame(apSaveGameBuffer);
    }
    apSaveGameBuffer->SaveDataEndian(&iSearchCount, 4u, 0);
    if (this->GetSearchCount()) {
        uint32_t isaveGameCount_5;
        uint32_t isaveGameCount_4;
        apSaveGameBuffer->SaveFormID(pSearchingMember, 0);
        apSaveGameBuffer->SaveDataEndian(&SearchFocalPoint, 12u, 0);
        SearchUpdateTimer.SaveGame(apSaveGameBuffer);
        SearchAreaUpdateTimer.SaveGame(apSaveGameBuffer);
        fSearchStartedTimeStamp.SaveGame(apSaveGameBuffer);
        SearchCenter.SaveGame(apSaveGameBuffer);
        apSaveGameBuffer->SaveDataEndian(&fSearchRadius, 4u, 0);
        isaveGameCount_4 = SearchLocations.iSize;
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_4);
        for (uint32_t j = 0; j < isaveGameCount_4; ++j) {
            CombatSearchLocation* searchLoc = &SearchLocations.operator[](j);
            searchLoc->WorldLoc.SaveGame(apSaveGameBuffer);
            apSaveGameBuffer->SaveDataEndian(&searchLoc->fScore, 4u, 0);
            apSaveGameBuffer->SaveFormID(searchLoc->iTargetID, 0);
            searchLoc->fTimeStamp.SaveGame(apSaveGameBuffer);
        }
        isaveGameCount_5 = SearchTeleportDoors.iSize;
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_5);
        for (uint32_t k = 0; k < isaveGameCount_5; ++k) {
            CombatSearchDoor* searchDoor = &SearchTeleportDoors.operator[](k);
            apSaveGameBuffer->SaveFormID(searchDoor->pDoor, 0);
            apSaveGameBuffer->SaveFormID(searchDoor->pLinkedDoor, 0);
            apSaveGameBuffer->SaveDataEndian(&searchDoor->cFailedAttempts, 1u, 0);
            apSaveGameBuffer->SaveDataEndian(&searchDoor->bInvestigated, 1u, 0);
            apSaveGameBuffer->SaveDataEndian(&searchDoor->bReserved, 1u, 0);
        }
        bhasPathingLOSGridMap = spPathingLOSGridMap.m_pObject != nullptr;
        apSaveGameBuffer->SaveDataEndian(&bhasPathingLOSGridMap, 1u, 0);
        if (bhasPathingLOSGridMap) {
            spPathingLOSGridMap.m_pObject->SaveGame(apSaveGameBuffer);
        }
    }
    CombatStrengthTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&fMemberCombatStrength, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAverageMemberDamagePerSecond, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fTargetCombatStrength, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fAverageTargetCombatStrength, 4u, 0);
    CombatMusicUpdateTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&cCombatMusicState, 1u, 0);
}

void CombatGroup::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    bool bhasPathingLOSGridMap;
    char cgroupStrategyType;
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    ClusterArray.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        CombatGroupCluster* v7;
        CombatGroupCluster** pcluster = &ClusterArray.operator[](i);
        CombatGroupCluster* apMemory = static_cast<CombatGroupCluster*>(ourMalloc(sizeof(CombatGroupCluster)));
        if (apMemory) {
            v7 = ::new (apMemory) CombatGroupCluster;
        }
        else {
            v7 = nullptr;
        }
        *pcluster = v7;
        (*pcluster)->LoadGame(apLoadGameBuffer, this);
    }
    uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
    TargetArray.SetSize(isaveGameCount_2, true);
    for (uint32_t i_2 = 0; i_2 < isaveGameCount_2; ++i_2) {
        CombatTarget* combatTarget = &TargetArray.operator[](i_2);
        combatTarget->LoadGame(apLoadGameBuffer, this);
    }
    uint32_t isaveGameCount_3 = apLoadGameBuffer->LoadVariableSizedValue();
    MemberArray.SetSize(isaveGameCount_3, true);
    for (uint32_t i_3 = 0; i_3 < isaveGameCount_3; ++i_3) {
        CombatMember* combatMember = &MemberArray.operator[](i_3);
        combatMember->LoadGame(apLoadGameBuffer, this);
    }
    cgroupStrategyType = -1;
    apLoadGameBuffer->LoadData(&cgroupStrategyType, 1u);
    if (cgroupStrategyType != -1) {
        pGroupStrategy = CombatGroupStrategy::GetCombatGroupStrategy(cgroupStrategyType);
    }
    apLoadGameBuffer->LoadData(&bStrategyForced, 1u);
    ChooseStrategyTimer.LoadGame(apLoadGameBuffer);
    UpdateStrategyTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&iLastGroupStrategyChosenIndex, 4u);
    fLastGroupStrategyChosenTimeStamp.LoadGame(apLoadGameBuffer);
    AvoidThreatDialogueTimer.LoadGame(apLoadGameBuffer);
    ClusterUpdateTimer.LoadGame(apLoadGameBuffer);
    TargetUpdateTimer.LoadGame(apLoadGameBuffer);
    for (uint32_t i_4 = 0; i_4 < 10; ++i_4) {
        DetectionDialogueTimers[i_4].LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&iSearchCount, 4u);
    if (this->GetSearchCount()) {
        apLoadGameBuffer->LoadFormID(pSearchingMember);
        apLoadGameBuffer->LoadData(&SearchFocalPoint, 12u);
        SearchUpdateTimer.LoadGame(apLoadGameBuffer);
        SearchAreaUpdateTimer.LoadGame(apLoadGameBuffer);
        if (apLoadGameBuffer->GetVersion() >= 5u) {
            fSearchStartedTimeStamp.LoadGame(apLoadGameBuffer);
        }
        SearchCenter.LoadGame(apLoadGameBuffer);
        apLoadGameBuffer->LoadData(&fSearchRadius, 4u);
        uint32_t isaveGameCount_4 = apLoadGameBuffer->LoadVariableSizedValue();
        SearchLocations.SetSize(isaveGameCount_4, true);
        for (uint32_t i_5 = 0; i_5 < isaveGameCount_4; ++i_5) {
            CombatSearchLocation* searchLoc = &SearchLocations.operator[](i_5);
            searchLoc->WorldLoc.LoadGame(apLoadGameBuffer);
            apLoadGameBuffer->LoadData(&searchLoc->fScore, 4u);
            apLoadGameBuffer->LoadFormID(searchLoc->iTargetID);
            searchLoc->fTimeStamp.LoadGame(apLoadGameBuffer);
        }
        uint32_t isaveGameCount_5 = apLoadGameBuffer->LoadVariableSizedValue();
        SearchTeleportDoors.SetSize(isaveGameCount_5, true);
        for (uint32_t i_6 = 0; i_6 < isaveGameCount_5; ++i_6) {
            CombatSearchDoor* searchDoor = &SearchTeleportDoors.operator[](i_6);
            uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
            searchDoor->pDoor = dynamic_cast<TESObjectREFR*>(FormByNumericID);
            uint32_t v4 = apLoadGameBuffer->LoadFormID();
            TESForm* v5 = TESForm::GetFormByNumericID(v4);
            searchDoor->pLinkedDoor = dynamic_cast<TESObjectREFR*>(v5);
            apLoadGameBuffer->LoadData(&searchDoor->cFailedAttempts, 1u);
            apLoadGameBuffer->LoadData(&searchDoor->bInvestigated, 1u);
            apLoadGameBuffer->LoadData(&searchDoor->bReserved, 1u);
        }
        bhasPathingLOSGridMap = false;
        apLoadGameBuffer->LoadData(&bhasPathingLOSGridMap, 1u);
        if (bhasPathingLOSGridMap) {
            PathingLOSGridMap* v6;
            PathingLOSGridMap* v9 =
                static_cast<PathingLOSGridMap*>(NiMemObject::operator new(sizeof(PathingLOSGridMap)));
            if (v9) {
                v6 = ::new (v9) PathingLOSGridMap;
            }
            else {
                v6 = nullptr;
            }
            spPathingLOSGridMap.operator=(v6);
            spPathingLOSGridMap.m_pObject->LoadGame(apLoadGameBuffer);
        }
        bUpdateSearchDebugGeometry = true;
    }
    CombatStrengthTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fMemberCombatStrength, 4u);
    apLoadGameBuffer->LoadData(&fAverageMemberDamagePerSecond, 4u);
    apLoadGameBuffer->LoadData(&fTargetCombatStrength, 4u);
    apLoadGameBuffer->LoadData(&fAverageTargetCombatStrength, 4u);
    CombatMusicUpdateTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&cCombatMusicState, 1u);
}

void CombatGroup::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t isaveGameCount = TargetArray.iSize;
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        bool bremove = false;
        CombatTarget* combatTarget = &TargetArray.operator[](i);
        combatTarget->InitLoadGame(apLoadGameBuffer, this);
        if (combatTarget->pActor) {
            pCombatManager->TargetLocalActor(combatTarget->pActor, this);
        }
        else {
            bremove = true;
        }
        if (bremove) {
            TargetArray.RemoveFast(i--, 1u);
            --isaveGameCount;
        }
    }
    uint32_t isaveGameCount_2 = MemberArray.iSize;
    for (uint32_t i_2 = 0; i_2 < isaveGameCount_2; ++i_2) {
        CombatMember* combatMember = &MemberArray.operator[](i_2);
        combatMember->InitLoadGame(apLoadGameBuffer, this);
        if (!combatMember->pActor) {
            MemberArray.RemoveFast(i_2--, 1u);
            --isaveGameCount_2;
        }
    }
    if (IsSearching()) {
        Actor* v3;
        if (pSearchingMember) {
            TESForm* FormByNumericID =
                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSearchingMember)));
            v3 = dynamic_cast<Actor*>(FormByNumericID);
        }
        else {
            v3 = nullptr;
        }
        pSearchingMember = v3;
    }
    UpdateCounts();
}

bool CombatGroup::GetTargetLastDetectedLocation(Actor* apTarget, BGSWorldLocation& arLastDetectedLocation) {
    CombatTarget* pcombatTarget = GetCombatTargetForActor(apTarget);
    if (!pcombatTarget || !pcombatTarget->LastDetectedLocation.pSpace) {
        return false;
    }
    arLastDetectedLocation = pcombatTarget->LastDetectedLocation;
    return true;
}

void CombatGroup::CreateDebugGeometry(eBool abClear) {
    if ((spDebugGeometry.m_pObject) && (spDebugGeometry.m_pObject->m_pkParent)) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiNode* pNode = spDebugGeometry.m_pObject;
            TaskQueueInterface::GetInstance()->QueueNodeDetach(pNode);
        }
        else {
            NiNode* pParent = spDebugGeometry.m_pObject->m_pkParent;
            pParent->DetachChild(spDebugGeometry.m_pObject);
        }
    }

    spDebugGeometry.operator=(static_cast<NiNode*>(nullptr));
    if (abClear != eFalse) {
        return;
    }
    if (!pPlayer->GetParentCell() || !pPlayer->GetParentCell()->GetMarkerNode()) {
        return;
    }
    TESForm* pSpace = pPlayer->GetSpace();
    // The debug text reference, when it is an actor of this group.
    Actor* pDebugActor = nullptr;
    if (Main::pDebugTextReference && Main::pDebugTextReference->IsActor()) {
        Actor* pActor = static_cast<Actor*>(Main::pDebugTextReference);
        if (pActor->GetCombatGroup() == this) {
            pDebugActor = pActor;
        }
    }
    if (bDebugCombatGroupClusters.Bool() && !CombatGroup::bUseSimpleCombatGroupClustering && pDebugActor) {
        for (uint32_t i = 0; i < ClusterArray.iSize; ++i) {
            CombatGroupCluster* pCluster = ClusterArray.operator[](i);
            if (!spDebugGeometry.m_pObject) {
                NiNode* pNode;
                void* pMemory = NiMemObject::operator new(sizeof(NiNode));
                if (pMemory) {
                    pNode = ::new (pMemory) NiNode(0);
                }
                else {
                    pNode = nullptr;
                }
                spDebugGeometry.operator=(pNode);
            }
            NiColorA kClusterColor(0.5f, 0.0f, 0.5f, 0.5f);
            bool bHasMembers = false;
            NiPoint3 kCenter = pCluster->Center.Point;
            uint32_t uiMemberCount = GetMemberCount();
            for (uint32_t j = 0; j < uiMemberCount; ++j) {
                CombatMember* pCombatMember = GetCombatMember(j);
                if (pCombatMember->pCluster == pCluster && pCombatMember->pActor->GetSpace() == pSpace) {
                    NiPoint3 kStart = pCombatMember->pActor->GetLocationOnReference();
                    NiColorA kStartColor(0.5f, 0.0f, 0.5f, 0.5f);
                    NiColorA kEndColor(1.0f, 1.0f, 1.0f, 0.5f);
                    NiAVObject* pArrow =
                        BSTestObjects::Create3DArrow(kStart, kCenter, 4.0f, 4.0f, &kStartColor, &kEndColor);
                    spDebugGeometry.m_pObject->AttachChild(pArrow, 1);
                    bHasMembers = true;
                }
            }
            if (bHasMembers) {
                for (uint32_t k = 0; k < 5; ++k) {
                    NiAVObject* pCircle = BSTestObjects::CreateCircle(fClusterDistance, 36, &kClusterColor);
                    float fZ = static_cast<double>(k) * 25.0 + kCenter.z;
                    pCircle->SetTranslate(kCenter.x, kCenter.y, fZ);
                    spDebugGeometry.m_pObject->AttachChild(pCircle, 1);
                }
                NiAVObject* pCylinder =
                    BSTestObjects::CreateCappedCylinder(16.0f, 16.0f, 400.0f, 10, 10, &kClusterColor);
                pCylinder->SetTranslate(kCenter);
                spDebugGeometry.m_pObject->AttachChild(pCylinder, 1);
            }
        }
    }
    if (bDebugCombatGroups2.Bool()) {
        if (!spDebugGeometry.m_pObject) {
            NiNode* pNode;
            NiNode* pMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
            if (pMemory) {
                pNode = ::new (pMemory) NiNode(0);
            }
            else {
                pNode = nullptr;
            }
            spDebugGeometry.operator=(pNode);
        }
        uint32_t uiMemberCount = GetMemberCount();
        for (uint32_t i = 0; i < uiMemberCount; ++i) {
            Actor* pMember = GetMember(i);
            if (pMember->GetSpace() == pSpace) {
                float fRadius = pMember->GetRadius() * 1.5;
                // Copies the whole palette colour, then stores the alpha.
                NiColorA kColor = BSTestObjects::GetColor(iGroupNum);
                kColor.a = 0.5f;
                NiAVObject* pCylinder = BSTestObjects::CreateCappedCylinder(fRadius, fRadius, 16.0f, 10, 10, &kColor);
                pCylinder->SetTranslate(pMember->GetLocationOnReference());
                spDebugGeometry.m_pObject->AttachChild(pCylinder, 1);
            }
        }
    }
    NiAVObject* pSearchGeometry = nullptr;
    if (bDebugCombatSearch.Bool() && pDebugActor) {
        if (!spDebugGeometry.m_pObject) {
            NiNode* pNode;
            NiNode* pMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
            if (pMemory) {
                pNode = ::new (pMemory) NiNode(0);
            }
            else {
                pNode = nullptr;
            }
            spDebugGeometry.operator=(pNode);
        }
        uint32_t uiNumSearchLocations = GetNumSearchLocations();
        for (uint32_t i = 0; i < uiNumSearchLocations; ++i) {
            CombatSearchLocation* pSearchLocation = &GetSearchLocation(i);
            if (pSearchLocation->WorldLoc.pSpace == pSpace) {
                NiColorA kColor;
                pSearchLocation->GetColor(&kColor);
                NiAVObject* pSphere = BSTestObjects::CreateSphere(32.0f, 10, 10, &kColor);
                pSphere->SetTranslate(pSearchLocation->WorldLoc.Point);
                spDebugGeometry.m_pObject->AttachChild(pSphere, 1);
            }
        }
        for (uint32_t i = 0; i < SearchTeleportDoors.iSize; ++i) {
            CombatSearchDoor& rDoor = SearchTeleportDoors.operator[](i);
            TESObjectREFR* pDoor = nullptr;
            if (rDoor.pDoor && rDoor.pDoor->GetSpace() == pSpace) {
                pDoor = rDoor.pDoor;
            }
            else if (rDoor.pLinkedDoor && rDoor.pLinkedDoor->GetSpace() == pSpace) {
                pDoor = rDoor.pLinkedDoor;
            }
            if (!pDoor) {
                continue;
            }
            NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
            if (rDoor.bInvestigated) {
                NiColorA kInvestigatedColor(0.0f, 0.0f, 1.0f, 0.5f);
                kColor = kInvestigatedColor;
            }
            else if (rDoor.bReserved) {
                NiColorA kReservedColor(0.0f, 1.0f, 0.0f, 0.5f);
                kColor = kReservedColor;
            }
            else {
                NiColorA kOpenColor(1.0f, 1.0f, 0.0f, 0.5f);
                kColor = kOpenColor;
            }
            NiPoint3 kPosition = pDoor->GetLocationOnReference();
            NiAVObject* pCylinder = BSTestObjects::CreateCappedCylinder(32.0f, 32.0f, 256.0f, 10, 10, &kColor);
            pCylinder->SetTranslate(kPosition);
            spDebugGeometry.m_pObject->AttachChild(pCylinder, 1);
        }
        if (spPathingLOSGridMap.m_pObject && bUpdateSearchDebugGeometry && pDebugActor->GetSpace() == pSpace) {
            PathingLOSMap* pLOSMap;
            PathingLOSMap* pMemory = static_cast<PathingLOSMap*>(NiMemObject::operator new(sizeof(PathingLOSMap)));
            if (pMemory) {
                float fRadius = spPathingLOSGridMap.m_pObject->fRadius;
                NiPoint3 kCenter = spPathingLOSGridMap.m_pObject->GetLocation();
                pLOSMap = ::new (pMemory) PathingLOSMap(kCenter, fRadius);
            }
            else {
                pLOSMap = nullptr;
            }
            NiPointer<PathingLOSMap> spLOSMap = pLOSMap;
            spPathingLOSGridMap.m_pObject->SetLOSMapFromSeenTriangles(spLOSMap);
            spSearchDebugGeometry.operator=(static_cast<NiRefObject*>(spLOSMap.m_pObject->CreateDebugGeometry()));
            bUpdateSearchDebugGeometry = false;
        }
        pSearchGeometry = static_cast<NiAVObject*>(spSearchDebugGeometry.m_pObject);
    }
    if (spDebugGeometry.m_pObject) {
        NiAlphaProperty* pAlpha;
        NiAlphaProperty* pMemory = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
        if (pMemory) {
            pAlpha = ::new (pMemory) NiAlphaProperty;
        }
        else {
            pAlpha = nullptr;
        }
        pAlpha->SetAlphaBlending(true);
        spDebugGeometry.m_pObject->AttachProperty(pAlpha);
        AddNoLightingPropertyRecurse(spDebugGeometry.m_pObject);
        BSShaderManager::PrepareObject(spDebugGeometry.m_pObject, 0, 0);
        if (pSearchGeometry) {
            spDebugGeometry.m_pObject->AttachChild(pSearchGeometry, 1);
        }
        NiNode* pMarkerNode = pPlayer->GetParentCell()->GetMarkerNode();
        pMarkerNode->AttachChild(spDebugGeometry.m_pObject, 1);
        spDebugGeometry.m_pObject->UpdateProperties();
        NiUpdateData kUpdateData(0.0f, false, false);
        spDebugGeometry.m_pObject->Update(kUpdateData);
    }
}

CombatGroupStrategy* CombatGroup::SelectGroupStrategy(CombatGroup* a1) {
    alignas(CombatGroupData) uint8_t kData[sizeof(CombatGroupData)];
    if (bDisableCombatGroupStrategies.Bool()) {
        return nullptr;
    }
    ::new (&reinterpret_cast<CombatGroupData&>(kData)) CombatGroupData(a1);
    CombatGroupStrategy* pCurrStrategy = a1->pGroupStrategy;
    if (static_cast<int32_t>(reinterpret_cast<CombatGroupData&>(kData).iMemberCount) >= 2) {
        for (uint32_t aiIndex = 0; aiIndex < 4; ++aiIndex) {
            CombatGroupStrategy* pGroupStrategy = CombatGroupStrategy::GetCombatGroupStrategy(aiIndex);
            if (pGroupStrategy) {
                if (pGroupStrategy == pCurrStrategy) {
                    reinterpret_cast<CombatGroupData&>(kData).~CombatGroupData();
                    return pCurrStrategy;
                }
                if (pGroupStrategy->CanExecute(a1, &reinterpret_cast<CombatGroupData&>(kData))) {
                    a1->ClearGroupStrategyAssignments();
                    pGroupStrategy->Execute(a1, &reinterpret_cast<CombatGroupData&>(kData));
                    reinterpret_cast<CombatGroupData&>(kData).~CombatGroupData();
                    return pGroupStrategy;
                }
            }
        }
    }
    reinterpret_cast<CombatGroupData&>(kData).~CombatGroupData();
    return nullptr;
}

uint32_t CombatGoal::GetGroupStrategyAssignment() {
    return iGroupStrategyAssignment;
}

CombatGroupStrategy::COMBAT_GROUP_STRATEGY CombatGroupStrategy::GetType() {
    return eType;
}
