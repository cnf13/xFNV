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
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <limits>
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
    // The save holds the exe's 16-byte centre: the point, then the space pointer's 32 bits. A pointer means nothing
    // in another process and a 64-bit one does not fit, so 0 is written there (LoadGame never reads it back).
    struct {
        NiPoint3 Point;
        uint32_t uiSpace;
    } kCenter = {Center.Point, 0};
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&kCenter), sizeof(kCenter), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iMemberCount), sizeof(iMemberCount), 0);
}

using BSSimpleArray_CombatMember_1024 = BSSimpleArray<CombatMember, 1024>;

using BSSimpleArray_CombatSearchLocation_1024 = BSSimpleArray<CombatSearchLocation, 1024>;

using BSSimpleArray_CombatSearchLocation_1024 = BSSimpleArray<CombatSearchLocation, 1024>;

using BSSimpleArray_CombatTarget_1024 = BSSimpleArray<CombatTarget, 1024>;

using BSSimpleArray_CombatMember_1024 = BSSimpleArray<CombatMember, 1024>;

void CombatGroupCluster::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    // The exe's 16-byte centre: the point and a saved pointer's 32 bits, stale in this process; the space stays null
    // (its users test it).
    uint32_t uiSpace;
    apLoadGameBuffer->LoadData(&Center.Point, sizeof(Center.Point));
    apLoadGameBuffer->LoadData(&uiSpace, sizeof(uiSpace));
    Center.pSpace = nullptr;
    apLoadGameBuffer->LoadData(&iMemberCount, sizeof(iMemberCount));
}

CombatSearchLocation::CombatSearchLocation() {}

float CombatSearchLocation::GetScore() {
    return fScore - fTimeStamp.ElapsedTime();
}

// The score loses the squared distance to arMemberWorldLoc (capped at 4096 squared) over 360000; a location with a
// target gains 10.
float CombatSearchLocation::GetModifiedScore(const BGSWorldLocation& arMemberWorldLoc) {
    float fScore = GetScore();
    float fDistanceSquared = arMemberWorldLoc.GetDistanceSquared(WorldLoc);
    fDistanceSquared = BSMIN(fDistanceSquared, 16777216.0f);
    fScore = static_cast<float>(fScore - fDistanceSquared / 360000.0);
    if (iTargetID) {
        fScore = static_cast<float>(fScore + 10.0);
    }
    return fScore;
}

NiColorA* CombatSearchLocation::GetColor(NiColorA* apResult) {
    NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
    float fScore = GetScore();
    if (fScore > 50.0) {
        NiColorA kHigh(1.0f, 0.0f, 1.0f, 0.5f);
        kColor = kHigh;
    }
    else if (fScore > 30.0) {
        NiColorA kMiddle(0.0f, 0.0f, 1.0f, 0.5f);
        kColor = kMiddle;
    }
    else {
        NiColorA kLow(1.0f, 1.0f, 0.0f, 0.5f);
        kColor = kLow;
    }
    *apResult = kColor;
    return apResult;
}

CombatTarget::CombatTarget() {}

CombatTarget::CombatTarget(Actor* apTarget, uint32_t aiSearchCount)
  : pActor(apTarget), iDetectionLevel(0), sLastSearchNoticed(static_cast<uint16_t>(aiSearchCount)), sAttackerCount(0),
    fLastSeenTimeStamp(-std::numeric_limits<float>::max()), fLastDetectedTimeStamp(-std::numeric_limits<float>::max()),
    fLastNoticedTimeStamp(CombatTimer::GetTimer()), fLastAttackedMemberTimeStamp(-std::numeric_limits<float>::max()),
    cMemberLOSCount(0), cMember360LOSCount(0) {
    BGSWorldLocation kLocation;
    LastNoticedLocation = *apTarget->GetWorldLocation(&kLocation);
}

bool CombatTarget::GetHasBufferedDetection() {
    double dElapsedTime = fLastDetectedTimeStamp.ElapsedTime();
    return fCombatDetectionBufferTime.Float() > dElapsedTime;
}

void CombatTarget::CopyData(CombatTarget* apCombatTarget) {
    if (iDetectionLevel < apCombatTarget->iDetectionLevel) {
        iDetectionLevel = apCombatTarget->iDetectionLevel;
    }
    double dDetectedTime = fLastDetectedTimeStamp;
    if (apCombatTarget->fLastDetectedTimeStamp > dDetectedTime) {
        fLastDetectedTimeStamp.fTimeStamp = apCombatTarget->fLastDetectedTimeStamp.fTimeStamp;
        LastDetectedLocation = apCombatTarget->LastDetectedLocation;
    }
    double dSeenTime = fLastSeenTimeStamp;
    if (apCombatTarget->fLastSeenTimeStamp > dSeenTime) {
        fLastSeenTimeStamp.fTimeStamp = apCombatTarget->fLastSeenTimeStamp.fTimeStamp;
        LastSeenLocation = apCombatTarget->LastSeenLocation;
    }
    double dUpdateTime = fDetectionLevelUpdateTimeStamp;
    if (apCombatTarget->fDetectionLevelUpdateTimeStamp > dUpdateTime) {
        fDetectionLevelUpdateTimeStamp.fTimeStamp = apCombatTarget->fDetectionLevelUpdateTimeStamp.fTimeStamp;
    }
    double dNoticedTime = fLastNoticedTimeStamp;
    if (apCombatTarget->fLastNoticedTimeStamp > dNoticedTime) {
        fLastNoticedTimeStamp.fTimeStamp = apCombatTarget->fLastNoticedTimeStamp.fTimeStamp;
    }
    double dAttackedTime = fLastAttackedMemberTimeStamp;
    if (apCombatTarget->fLastAttackedMemberTimeStamp > dAttackedTime) {
        fLastAttackedMemberTimeStamp.fTimeStamp = apCombatTarget->fLastAttackedMemberTimeStamp.fTimeStamp;
        LastAttackedMemberLocation = apCombatTarget->LastAttackedMemberLocation;
    }
    sAttackerCount += apCombatTarget->sAttackerCount;
    cMemberLOSCount += apCombatTarget->cMemberLOSCount;
    cMember360LOSCount += apCombatTarget->cMember360LOSCount;
}

void CombatTarget::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup) {
    apSaveGameBuffer->SaveFormID(pActor, static_cast<uint32_t>(0));
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iDetectionLevel), sizeof(iDetectionLevel), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sLastSearchNoticed), sizeof(sLastSearchNoticed), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sAttackerCount), sizeof(sAttackerCount), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cMemberLOSCount), sizeof(cMemberLOSCount), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cMember360LOSCount), sizeof(cMember360LOSCount), 0);
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
    apLoadGameBuffer->LoadData(&iDetectionLevel, sizeof(iDetectionLevel));
    apLoadGameBuffer->LoadData(&sLastSearchNoticed, sizeof(sLastSearchNoticed));
    apLoadGameBuffer->LoadData(&sAttackerCount, sizeof(sAttackerCount));
    apLoadGameBuffer->LoadData(&cMemberLOSCount, sizeof(cMemberLOSCount));
    apLoadGameBuffer->LoadData(&cMember360LOSCount, sizeof(cMember360LOSCount));
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
    Actor* pTargetActor = nullptr;
    if (pActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        pTargetActor = dynamic_cast<Actor*>(pForm);
    }
    pActor = pTargetActor;
}

CombatGroupCluster::CombatGroupCluster() : iMemberCount(0) {}

ClusterData::ClusterData() {}

CombatMember::CombatMember(Actor* apActor)
  : pActor(apActor), iGroupStrategyAssignment(0), fDamagePerSecond(-1.0f), fCombatStrength(-1.0f), pCluster(nullptr) {}

void CombatMember::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer, CombatGroup* apGroup) {
    apSaveGameBuffer->SaveFormID(pActor, 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&iGroupStrategyAssignment), sizeof(iGroupStrategyAssignment), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDamagePerSecond), sizeof(fDamagePerSecond), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCombatStrength), sizeof(fCombatStrength), 0);
    int8_t cClusterIndex = -1;
    if (pCluster) {
        cClusterIndex = apGroup->GetClusterIndex(pCluster);
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cClusterIndex), sizeof(cClusterIndex), 0);
}

// Reads what SaveGame writes: the actor's form ID, the strategy assignment, the damage per second, the combat strength,
// then the cluster index; -1 clears the cluster and any other index resolves through apGroup.
void CombatMember::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    apLoadGameBuffer->LoadFormID(pActor);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iGroupStrategyAssignment), sizeof(iGroupStrategyAssignment));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDamagePerSecond), sizeof(fDamagePerSecond));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCombatStrength), sizeof(fCombatStrength));
    int8_t cClusterIndex = -1;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cClusterIndex), sizeof(cClusterIndex));
    if (cClusterIndex == -1) {
        pCluster = nullptr;
    }
    else {
        pCluster = apGroup->GetCluster(cClusterIndex);
    }
}

void CombatMember::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer, CombatGroup* apGroup) {
    Actor* pMemberActor = nullptr;
    if (pActor) {
        // Until this resolves it, the Actor* field holds the saved form ID.
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        pMemberActor = dynamic_cast<Actor*>(pForm);
    }
    pActor = pMemberActor;
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
    fSearchStartedTimeStamp(-std::numeric_limits<float>::max()), pSearchingMember(nullptr),
    SearchFocalPoint(NiPoint3::ZERO), fSearchRadius(0.0f), spSearchDebugGeometry(nullptr),
    bUpdateSearchDebugGeometry(false), iInitializedMemberCount(0), iFleeingMemberCount(0), iNonFleeingMemberCount(0),
    cCombatMusicState(0xFF), spDebugGeometry(nullptr) {}

// Tells the combat manager the targets and members are gone, frees the clusters and removes the debug geometry; the
// members' destructors run after.
CombatGroup::~CombatGroup() {
    uint32_t uiTargetCount = GetTargetCount();
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        pCombatManager->NotifyCombatGroupRemovedTarget(this, GetTarget(i));
    }
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        pCombatManager->NotifyCombatGroupRemovedMember(this, GetMember(i));
    }
    for (CombatGroupCluster* pCluster : ClusterArray) {
        ourFree(pCluster);
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
    BGSWorldLocation memberLoc;
    BGSWorldLocation targetLoc;
    BGSWorldLocation currentTargetLoc;
    arbTargetLineOfSight = false;
    arbTarget360LineOfSight = false;
    ariDetectionLevel = -100;
    CombatTimeStamp kNeverDetected(-std::numeric_limits<float>::max());
    arfLastTargetDetectedTimeStamp.fTimeStamp = kNeverDetected.fTimeStamp;
    CombatTarget* pCombatTarget = GetCombatTargetForActor(apTarget);
    if (pCombatTarget) {
        int32_t iCurrentDetectionLevel = pCombatTarget->iDetectionLevel;
        double dUpdateTime = pCombatTarget->fDetectionLevelUpdateTimeStamp.fTimeStamp;
        if (CombatTimer::GetTimer() > dUpdateTime) {
            int32_t iNewDetectionLevel = -100;
            int32_t iDetectionEventLevel = -100;
            bool bFindLastDetectedLocation = pCombatTarget->LastDetectedLocation.pSpace == nullptr;
            Actor* pBestDetectingMember = nullptr;
            pCombatTarget->cMember360LOSCount = 0;
            pCombatTarget->cMemberLOSCount = 0;
            BGSWorldLocation lastDetectedLocation;
            CombatTimeStamp fLastDetectedTime(-std::numeric_limits<float>::max());
            uint32_t uiMemberCount = GetMemberCount();
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                Actor* pMember = GetMember(i);
                DetectionState* pDetectionState = pMember->pCurrentProcess->GetDetectionState(apTarget, 0);
                DetectionState detectionState;
                if (pMember->GetCurrentProcessType() && apTarget->GetCurrentProcessType()) {
                    pMember->GetWorldLocation(&memberLoc);
                    apTarget->GetWorldLocation(&targetLoc);
                    if (memberLoc.GetDistanceSquared(targetLoc) < 67108864.0) {
                        detectionState.iLevel = 1;
                        pDetectionState = &detectionState;
                    }
                }

                if (pDetectionState) {
                    if (pDetectionState->bLineofSight) {
                        ++pCombatTarget->cMemberLOSCount;
                    }
                    if (pDetectionState->b360view) {
                        ++pCombatTarget->cMember360LOSCount;
                    }
                    if (pMember == apAttacker) {
                        arbTargetLineOfSight = pDetectionState->bLineofSight;
                        arbTarget360LineOfSight = pDetectionState->b360view;
                    }
                    int32_t iDetectionLevel = pDetectionState->iLevel;
                    if (pDetectionState->iDetectedEventLevel != 0x7FFFFFFF) {
                        if (iDetectionEventLevel < pDetectionState->iDetectedEventLevel) {
                            iDetectionEventLevel = pDetectionState->iDetectedEventLevel;
                        }
                        double dNoticedMin = fSneakNoticedMin.Float();
                        if (iDetectionLevel == static_cast<int32_t>(dNoticedMin) + 1) {
                            double dDetectedEventLevel = static_cast<double>(pDetectionState->iDetectedEventLevel);
                            if (fSneakNoticedMin.Float() < dDetectedEventLevel) {
                                iDetectionLevel = static_cast<int32_t>(fSneakNoticedMin.Float());
                            }
                        }
                    }
                    CombatController* pCombatController = pMember->GetCombatController();
                    if (pCombatController &&
                        pCombatController->pTarget == apTarget &&
                        !pCombatController->pCombatState->CheckShouldUpdateDetection(pMember, apTarget)) {
                        if (iCurrentDetectionLevel > iNewDetectionLevel) {
                            iNewDetectionLevel = iCurrentDetectionLevel;
                            pBestDetectingMember = pMember;
                        }
                    }
                    else if (pDetectionState && iDetectionLevel > iNewDetectionLevel) {
                        iNewDetectionLevel = iDetectionLevel;
                        pBestDetectingMember = pMember;
                    }
                    if (bFindLastDetectedLocation) {
                        double dLastTimeDetected = pDetectionState->fLastTimeDetected.fTimeStamp;
                        if (fLastDetectedTime.fTimeStamp < dLastTimeDetected) {
                            lastDetectedLocation.SetLocation(
                                pDetectionState->LastPositionDetected,
                                pMember->GetSpace()
                            );
                            fLastDetectedTime.fTimeStamp = pDetectionState->fLastTimeDetected.fTimeStamp;
                        }
                    }
                }
            }
            if (iNewDetectionLevel <= static_cast<int32_t>(fSneakNoticedMin.Float()) &&
                pCombatTarget->fLastAttackedMemberTimeStamp.ElapsedTime() < 1.0) {
                iNewDetectionLevel = static_cast<int32_t>(fSneakNoticedMin.Float() + 1.0);
            }

            if (IsSearching() && iCurrentDetectionLevel <= 0 && iNewDetectionLevel > 0) {
                if (-std::numeric_limits<float>::max() == pCombatTarget->fLastDetectedTimeStamp.fTimeStamp) {
                    for (uint32_t i = 0; i < uiMemberCount; ++i) {
                        Actor* pMember = GetMember(i);
                        if (pMember != pPlayer) {
                            Script::SetActionFlag(
                                pMember->GetCombatTarget(),
                                &pMember->m_Extra,
                                SCRIPT_ACTION_ONSTARTCOMBAT
                            );
                        }
                    }
                    pCombatDialogueManager
                        ->StartDialogue(pBestDetectingMember, apTarget, DT_DETECTION, 3, true, nullptr);
                }
                else {
                    pCombatDialogueManager
                        ->StartDialogue(pBestDetectingMember, apTarget, DT_DETECTION, 9, true, nullptr);
                }
            }
            pCombatTarget->iDetectionLevel = iNewDetectionLevel;
            CombatTimeStamp kUpdateTime(CombatTimer::GetTimer());
            pCombatTarget->fDetectionLevelUpdateTimeStamp = kUpdateTime;
            apTarget->GetWorldLocation(&currentTargetLoc);
            if (iNewDetectionLevel > 0) {
                CombatTimeStamp kDetectedTime(CombatTimer::GetTimer());
                pCombatTarget->fLastDetectedTimeStamp = kDetectedTime;
            }
            // The elapsed time is stored as a double before the compare.
            double dElapsed = pCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
            if (fCombatDetectionBufferTime.Float() > dElapsed) {
                pCombatTarget->LastDetectedLocation = currentTargetLoc;
            }
            else if (bFindLastDetectedLocation) {
                pCombatTarget->LastDetectedLocation = lastDetectedLocation;
            }
            if (iNewDetectionLevel > static_cast<int32_t>(fSneakNoticedMin.Float())) {
                CombatTimeStamp kNoticedTime(CombatTimer::GetTimer());
                pCombatTarget->fLastNoticedTimeStamp.fTimeStamp = kNoticedTime.fTimeStamp;
                pCombatTarget->LastNoticedLocation = currentTargetLoc;
                pCombatTarget->sLastSearchNoticed = this->GetSearchCount();
            }
            if (pCombatTarget->cMember360LOSCount) {
                pCombatTarget->LastSeenLocation = currentTargetLoc;
                CombatTimeStamp kSeenTime(CombatTimer::GetTimer());
                pCombatTarget->fLastSeenTimeStamp.fTimeStamp = kSeenTime.fTimeStamp;
            }
            if (fSneakNoticedMin.Float() < static_cast<double>(iDetectionEventLevel) && IsSearching()) {
                DetectionEvent* pDetectionEvent = apTarget->pCurrentProcess->GetActorsDetectionEvent(apTarget);
                if (pDetectionEvent && pDetectionEvent->fTimeStamp.ElapsedTime() < 5.0) {
                    double dEventTime = pDetectionEvent->fTimeStamp.fTimeStamp;
                    if (pCombatTarget->fDetectionEventTimeStamp.fTimeStamp < dEventTime) {
                        float fScore = 0.0f;
                        if (iDetectionEventLevel <= 0) {
                            fScore = static_cast<double>(
                                         iDetectionEventLevel - static_cast<int32_t>(fSneakNoticedMin.Float())
                                     ) /
                                    2.0 +
                                40.0;
                        }
                        else {
                            float fLevelScore = static_cast<double>(iDetectionEventLevel) / 20.0;
                            fScore = BSMIN(fLevelScore, 5.0f) + 55.0;
                        }
                        BGSWorldLocation worldLoc;
                        worldLoc.SetLocation(pDetectionEvent->Location, apTarget->GetSpace());
                        AddSearchLocation(worldLoc, fScore, 0);
                        pCombatTarget->fDetectionEventTimeStamp.fTimeStamp = pDetectionEvent->fTimeStamp.fTimeStamp;
                    }
                }
            }
        }
        else {
            DetectionState* pDetectionState = nullptr;
            if (apAttacker) {
                pDetectionState = apAttacker->pCurrentProcess->GetDetectionState(apTarget, 0);
            }
            if (pDetectionState) {
                arbTargetLineOfSight = pDetectionState->bLineofSight;
                arbTarget360LineOfSight = pDetectionState->b360view;
            }
        }
        ariDetectionLevel = pCombatTarget->iDetectionLevel;
        arfLastTargetDetectedTimeStamp = pCombatTarget->fLastDetectedTimeStamp;
    }
}

void CombatGroup::RemoveTarget(Actor* apActor) {
    for (uint32_t i = 0; i < TargetArray.BSSimpleArray<CombatTarget>::GetSize(); ++i) {
        if (TargetArray[i].pActor == apActor) {
            RemoveTarget(i);
            return;
        }
    }
}

void CombatGroup::RemoveTarget(uint32_t aiIndex) {
    Actor* pActor = TargetArray[aiIndex].pActor;
    pCombatManager->NotifyCombatGroupRemovedTarget(this, pActor);
    TargetArray.RemoveFast(aiIndex, 1u);
}

uint32_t CombatGroup::GetMemberGroupStrategyAssignment(Actor* apMember) {
    CombatMember* pCombatMember = GetCombatMemberForActor(apMember);
    if (pCombatMember) {
        return pCombatMember->iGroupStrategyAssignment;
    }

    return 0;
}

uint32_t CombatGroup::GetMemberGroupStrategyAssignment(uint32_t aiIndex) {
    return MemberArray[aiIndex].iGroupStrategyAssignment;
}

CombatMember* CombatGroup::GetCombatMember(uint32_t aiIndex) {
    return &MemberArray[aiIndex];
}

void CombatGroup::UpdateAttackerCounts() {
    uint32_t uiNumTargets = GetTargetCount();
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumTargets; ++i) {
        CombatTarget* pCombatTarget = GetCombatTarget(i);
        pCombatTarget->sAttackerCount = 0;
        for (uint32_t j = 0; j < uiNumMembers; ++j) {
            PlayerCharacter* pMember = static_cast<PlayerCharacter*>(GetMember(j));
            if (pMember != pPlayer && pMember->GetCombatTarget() == pCombatTarget->pActor) {
                ++pCombatTarget->sAttackerCount;
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
        CombatStrengthTimer.Reset(fCombatGroupCombatStrengthUpdateTime.Float());
    }
    if (GetMemberCount() > 1) {
        UpdateClusters();
    }
    if (static_cast<char>(cCombatMusicState) == -1) {
        if (CombatMusicUpdateTimer.IsSet()) {
            if (CombatMusicUpdateTimer.Check()) {
                cCombatMusicState = 0;
                if (fTargetCombatStrength > 0.0) {
                    float fThreatRatio = fMemberCombatStrength / fTargetCombatStrength;
                    if (fCombatMusicGroupThreatRatioMin.Float() < static_cast<double>(fThreatRatio) &&
                        fCombatMusicGroupThreatRatioMax.Float() > static_cast<double>(fThreatRatio)) {
                        cCombatMusicState = 1;
                    }
                }
            }
        }
        else if (!IsSearching()) {
            CombatMusicUpdateTimer.Reset(fCombatMusicGroupThreatRatioTimer.Float());
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
    float fRemoveDistanceSquared = fCombatSearchLocationRemoveDistance.Float() *
        fCombatSearchLocationRemoveDistance.Float();
    NiPoint3 coordMult(1.0f, 1.0f, 0.25f);
    uint32_t uiSearchLocationCount = GetNumSearchLocations();
    for (uint32_t i = 0; i < uiSearchLocationCount; ++i) {
        CombatSearchLocation* pSearchLoc = &GetSearchLocation(i);
        double dDistanceSquared = pSearchLoc->WorldLoc.GetModifiedDistanceSquared(arLocation, coordMult);
        if (fRemoveDistanceSquared > dDistanceSquared) {
            float fScore = pSearchLoc->GetScore();
            if (fScore > 30.0) {
                pSearchLoc->fScore = pSearchLoc->fScore - (fScore - 30.0);
            }
            if (pSearchLoc->iTargetID) {
                CombatTarget* pCombatTarget = GetCombatTargetForNumericID(pSearchLoc->iTargetID);
                if (pCombatTarget) {
                    CombatTimer kTimer;
                    float fDetectedScore;
                    float fAttackedScore;
                    float fNoticedScore;
                    CalculateTargetSearchLocationScores(pCombatTarget, fDetectedScore, fNoticedScore, fAttackedScore);
                    if (fDetectedScore > 30.0) {
                        double dTargetDistanceSquared = pCombatTarget->LastDetectedLocation.GetModifiedDistanceSquared(
                            arLocation,
                            coordMult
                        );
                        if (fRemoveDistanceSquared > dTargetDistanceSquared) {
                            double dTimeStamp = pCombatTarget->fLastDetectedTimeStamp.fTimeStamp;
                            float fNewTimeStamp = dTimeStamp - (fDetectedScore - 30.0);
                            CombatTimeStamp kTimeStamp(fNewTimeStamp);
                            pCombatTarget->fLastDetectedTimeStamp = kTimeStamp;
                        }
                    }
                    if (fNoticedScore > 30.0) {
                        double dTargetDistanceSquared = pCombatTarget->LastNoticedLocation.GetModifiedDistanceSquared(
                            arLocation,
                            coordMult
                        );
                        if (fRemoveDistanceSquared > dTargetDistanceSquared) {
                            double dTimeStamp = pCombatTarget->fLastNoticedTimeStamp.fTimeStamp;
                            float fNewTimeStamp = dTimeStamp - (fNoticedScore - 30.0);
                            ::new (&kTimer.fTargetTime) CombatTimeStamp(fNewTimeStamp);
                            pCombatTarget->fLastNoticedTimeStamp.fTimeStamp = kTimer.fTargetTime;
                        }
                    }
                    if (fAttackedScore > 30.0) {
                        double dTargetDistanceSquared = pCombatTarget->LastAttackedMemberLocation
                                                            .GetModifiedDistanceSquared(arLocation, coordMult);
                        if (fRemoveDistanceSquared > dTargetDistanceSquared) {
                            double dTimeStamp = pCombatTarget->fLastAttackedMemberTimeStamp.fTimeStamp;
                            float fNewTimeStamp = dTimeStamp - (fAttackedScore - 30.0);
                            kTimer.SetStartTime(fNewTimeStamp);
                            pCombatTarget->fLastAttackedMemberTimeStamp.fTimeStamp = kTimer.fStartTime;
                        }
                    }
                }
            }
        }
    }
}

Actor* CombatGroup::FindTargetNearLocation(const BGSWorldLocation& arWorldLoc, float afRadius, bool abDetectedOnly) {
    BGSWorldLocation kLocation;
    float fMaxDist = afRadius * afRadius;
    uint32_t uiNumTargets = GetTargetCount();
    for (int32_t i = 0; static_cast<uint32_t>(i) < uiNumTargets; ++i) {
        CombatTarget* pTarget = GetCombatTarget(i);
        if (!abDetectedOnly || pTarget->GetHasBufferedDetection()) {
            pTarget->pActor->TESObjectREFR::GetWorldLocation(&kLocation);
            double dDistanceSquared = arWorldLoc.GetDistanceSquared(kLocation);
            if (fMaxDist > dDistanceSquared) {
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
                float fInit = BoundsCornerIsMax[i][j] ? -std::numeric_limits<float>::max()
                                                      : std::numeric_limits<float>::max();
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
                if (BoundsCornerIsMax[i][j]) {
                    if (apCorners[i][j] < fValue) {
                        apCorners[i][j] = fValue;
                    }
                }
                else if (apCorners[i][j] > fValue) {
                    apCorners[i][j] = fValue;
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
    for (uint32_t i = 0; i < SearchTeleportDoors.iSize; ++i) {
        CombatSearchDoor* pSearchDoor = &SearchTeleportDoors[i];
        if (pSearchDoor->pDoor == apDoor || pSearchDoor->pLinkedDoor == apDoor) {
            return pSearchDoor;
        }
    }
    return nullptr;
}

CombatTarget* CombatGroup::GetCombatTargetForActor(Actor* apActor) {
    return GetCombatTargetForNumericID(apActor->TESForm::GetFormID());
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
        const char* pName = apActor->GetFormEditorIDLength() ? apActor->GetFormEditorID() : apActor->GetFullName();
        Warning(
            "COMBAT: Trying to add actor %08X %s to Combat Group member list when they are a target.",
            apActor->iFormID,
            pName
        );
        return false;
    }

    MemberArray.Add(CombatMember(apActor));
    pCombatManager->NotifyCombatGroupAddedMember(this, apActor);
    if (bDebugCombatGroups.Bool() && apActor->Get3D()) {
        NiColorA kGroupColor = BSTestObjects::GetColor(iGroupNum);
        NiColor color(kGroupColor.r, kGroupColor.g, kGroupColor.b);
        TintScenegraph(apActor->Get3D(), color, nullptr);
    }

    ClusterUpdateTimer.ForceTrigger();
    return true;
}

CombatMember* CombatGroup::GetCombatMemberForActor(Actor* apActor) {
    return GetCombatMemberForNumericID(apActor->TESForm::GetFormID());
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
        const char* pName = apActor->GetFormEditorIDLength() ? apActor->GetFormEditorID() : apActor->GetFullName();
        Warning(
            "COMBAT: Trying to add actor %08X %s to Combat Group target list when they are a member.",
            apActor->GetFormID(),
            pName
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
    return iSearchCount != 0;
}

CombatTimer* CombatGroup::GetDetectionDialogueTimer(uint32_t aiIndex) {
    return &DetectionDialogueTimers[aiIndex];
}

bool CombatGroup::IsTarget(Actor* apActor) {
    return GetCombatTargetForActor(apActor) != nullptr;
}

bool CombatGroup::IsMember(Actor* apActor) {
    return GetCombatMemberForActor(apActor) != nullptr;
}

bool CombatGroup::CanAddMember(Actor* apActor) {
    uint32_t uiNumTargets = GetTargetCount();
    for (uint32_t i = 0; i < uiNumTargets; ++i) {
        Actor* pTarget = GetTarget(i);
        if (!apActor->CanAttackActor(pTarget)) {
            return false;
        }
    }
    return true;
}

bool CombatGroup::IsEmpty() {
    return MemberArray.IsEmpty();
}

void CombatGroup::IncrementAttackerCount(Actor* apTarget) {
    CombatTarget* pCombatTarget = GetCombatTargetForActor(apTarget);
    if (pCombatTarget) {
        ++pCombatTarget->sAttackerCount;
    }
}

void CombatGroup::DecrementAttackerCount(Actor* apTarget) {
    CombatTarget* pCombatTarget = GetCombatTargetForActor(apTarget);
    if (pCombatTarget) {
        --pCombatTarget->sAttackerCount;
    }
}

char CombatGroup::GetCombatMusicState() {
    return cCombatMusicState;
}

void CombatGroup::InvestigatedSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* pSearchDoor = GetCombatSearchDoor(apDoor);
    if (pSearchDoor) {
        pSearchDoor->bInvestigated = true;
    }
}

void CombatGroup::ReserveSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* pSearchDoor = GetCombatSearchDoor(apDoor);
    if (pSearchDoor) {
        pSearchDoor->bReserved = true;
    }
}

void CombatGroup::UnreserveSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* pSearchDoor = GetCombatSearchDoor(apDoor);
    if (pSearchDoor) {
        pSearchDoor->bReserved = false;
    }
}

CombatGroupCluster* CombatGroup::GetClusterForMember(Actor* apMember) {
    CombatMember* pCombatMember = GetCombatMemberForActor(apMember);
    if (pCombatMember) {
        return pCombatMember->pCluster;
    }

    return nullptr;
}

char CombatGroup::GetClusterIndex(CombatGroupCluster* apCluster) {
    return ClusterArray.Find(apCluster, 0, &BSSimpleArray<CombatGroupCluster*>::DefaultEqualFunctionByAddress);
}

CombatGroupCluster* CombatGroup::GetCluster(char acIndex) {
    if (acIndex >= 0) {
        return *static_cast<CombatGroupCluster**>(&ClusterArray[acIndex]);
    }

    return nullptr;
}

void CombatGroup::SetSearchingMember(Actor* apMember) {
    pSearchingMember = apMember;
}

Actor* CombatGroup::GetSearchingMember() {
    return pSearchingMember;
}

void CombatGroup::SetMemberGroupStrategyAssignment(Actor* apMember, uint32_t auiAssignment) {
    CombatMember* pCombatMember = GetCombatMemberForActor(apMember);
    if (pCombatMember) {
        pCombatMember->iGroupStrategyAssignment = auiAssignment;
    }
}

void* CombatGroup::SetGroupStrategyAssignment(int32_t aiIndex, uint32_t auiAssignment) {
    CombatMember* pCombatMember = &MemberArray[aiIndex];
    pCombatMember->iGroupStrategyAssignment = auiAssignment;
    return pCombatMember;
}

void CombatGroup::RemoveMember(Actor* apActor) {
    for (uint32_t i = 0; i < MemberArray.iSize; ++i) {
        if (MemberArray[i].pActor == apActor) {
            pCombatManager->CombatManager::NotifyCombatGroupRemovedMember(this, apActor);
            MemberArray.RemoveFast(i, 1u);
            if (pSearchingMember == apActor) {
                pSearchingMember = nullptr;
            }
            if (bDebugCombatGroups.Bool() && apActor->Get3D()) {
                UnTintScenegraph(apActor->Get3D());
            }

            return;
        }
    }
}

CombatTarget* CombatGroup::GetCombatTargetForNumericID(uint32_t aiTargetID) {
    for (CombatTarget& rTarget : TargetArray) {
        if (rTarget.pActor->GetFormID() == aiTargetID) {
            return &rTarget;
        }
    }
    return nullptr;
}

CombatMember* CombatGroup::GetCombatMemberForNumericID(uint32_t auiRefID) {
    for (uint32_t i = 0; i < MemberArray.iSize; ++i) {
        CombatMember* pCombatMember = &MemberArray[i];
        if (pCombatMember->pActor->TESForm::GetFormID() == auiRefID) {
            return pCombatMember;
        }
    }
    return nullptr;
}

Actor* CombatGroup::GetMember(uint32_t aiIndex) {
    return MemberArray[aiIndex].pActor;
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
    NiPoint3* pMemberPositions = static_cast<NiPoint3*>(XFNV_ALLOCA16(12 * uiMemberCount));
    NiPoint3* pTargetPositions = static_cast<NiPoint3*>(XFNV_ALLOCA16(12 * uiTargetCount));
    NiPoint3 kMemberCenter = NiPoint3::ZERO;
    NiPoint3 kTargetCenter = NiPoint3::ZERO;
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
    kDirection.UnitizeGetLength();
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
    return &TargetArray[aiIndex];
}

Actor* CombatGroup::FindMemberNearLocation(const BGSWorldLocation& arWorldLoc, float afRadius, Actor* apIgnoreMember) {
    BGSWorldLocation kLocation;
    float fMaxDist = afRadius * afRadius;
    uint32_t uiNumMembers = GetMemberCount();
    for (int32_t i = 0; static_cast<uint32_t>(i) < uiNumMembers; ++i) {
        Actor* pMember = GetMember(i);
        if (pMember != apIgnoreMember) {
            pMember->TESObjectREFR::GetWorldLocation(&kLocation);
            double dDistanceSquared = arWorldLoc.GetDistanceSquared(kLocation);
            if (fMaxDist > dDistanceSquared) {
                return pMember;
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
    BGSWorldLocation kLocation;
    float fClosestDistSqr = std::numeric_limits<float>::max() == afMaxDistance ? std::numeric_limits<float>::max()
                                                                               : afMaxDistance * afMaxDistance;
    Actor* pClosestMember = nullptr;
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        if (pMember != apIgnoreMember) {
            pMember->GetWorldLocation(&kLocation);
            float fDistanceSquared = arWorldLoc.GetDistanceSquared(kLocation);
            if (fClosestDistSqr > static_cast<double>(fDistanceSquared)) {
                fClosestDistSqr = fDistanceSquared;
                pClosestMember = pMember;
            }
        }
    }
    return pClosestMember;
}

bool CombatGroup::CanAddTarget(Actor* apActor) {
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        Actor* pMember = GetMember(i);
        if (pMember && !pMember->CanAttackActor(apActor)) {
            return false;
        }
    }
    return true;
}

Actor* CombatGroup::FindMemberNearActor(Actor* apActor, float afRadius) {
    BGSWorldLocation kLocation;
    return FindMemberNearLocation(*apActor->TESObjectREFR::GetWorldLocation(&kLocation), afRadius, apActor);
}

Actor* CombatGroup::FindClosestMemberToActor(Actor* apActor, float afMaxDistance) {
    BGSWorldLocation kLocation;
    return FindClosestMemberToLocation(*apActor->TESObjectREFR::GetWorldLocation(&kLocation), apActor, afMaxDistance);
}

void CombatGroup::FailedToInvestigateSearchDoor(TESObjectREFR* apDoor) {
    CombatSearchDoor* pSearchDoor = GetCombatSearchDoor(apDoor);
    if (pSearchDoor) {
        pSearchDoor->bReserved = false;
        ++pSearchDoor->cFailedAttempts;
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
    iInitializedMemberCount = 0;
    iFleeingMemberCount = 0;
    iNonFleeingMemberCount = 0;
    uint32_t uiNumMembers = GetMemberCount();
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        Actor* pMember = GetMember(i);
        CombatController* pController = pMember->GetCombatController();
        if (pController) {
            if (pController->TESForm::GetInitialized()) {
                ++iInitializedMemberCount;
            }
            if (pController->IsFleeing()) {
                ++iFleeingMemberCount;
            }
            else {
                ++iNonFleeingMemberCount;
            }
        }
    }
}

bool CombatGroup::ShouldPlayCombatMusic() {
    return cCombatMusicState == 1;
}

char CombatGroup::GetClusterCount() {
    return static_cast<int8_t>(ClusterArray.iSize);
}

uint32_t CombatGroup::GetNumSearchLocations() {
    return SearchLocations.iSize;
}

void CombatGroup::FillTargetArray(BSSimpleArray<Actor*, 1024>* apActorArray) {
    apActorArray->SetSize(TargetArray.BSSimpleArray<CombatTarget>::GetSize(), true);
    for (uint32_t i = 0; i < TargetArray.BSSimpleArray<CombatTarget>::GetSize(); ++i) {
        apActorArray->SetAt(i, TargetArray[i].pActor);
    }
}

Actor* CombatGroup::GetBestTarget(Actor* apMember) {
    BGSWorldLocation memberWorldLoc;
    BGSWorldLocation targetWorldLoc;
    if (TargetArray.IsEmpty()) {
        return nullptr;
    }
    float fCurrentTargetScore = -std::numeric_limits<float>::max();
    CombatTimeStamp fCurrentLastDetectedTimeStamp(-std::numeric_limits<float>::max());
    Actor* pBestTarget = nullptr;
    Actor* pLastDetectedTarget = nullptr;
    Actor* pLastTargetChecked = nullptr;
    apMember->GetWorldLocation(&memberWorldLoc);
    CombatController* pCombatController = apMember->GetCombatController();
    if (!pCombatController) {
        RemoveMember(apMember);
        return nullptr;
    }
    Actor* pCurrentTarget = pCombatController->GetTarget();
    CombatState* pCombatState = pCombatController->pCombatState;
    float fTargetingFOV = pCombatController->GetCurrentCombatStyle()->SimpleData.fTargetingFOV;
    BGSEncounterZone* pMemberEncounterZone = apMember->GetLocationEncounterZone();
    bool bIsMelee = CombatUtilities::IsActorUsingMelee(apMember);
    uint32_t uiTargetCount = GetTargetCount();
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        CombatTarget* pCombatTarget = GetCombatTarget(i);
        Actor* pTarget = pCombatTarget->pActor;
        if (fTargetingFOV > 0.0) {
            float fTargetingArc = fTargetingFOV * 0.01745329238474369;
            if (!CombatUtilities::IsTargetWithinFiringArc(
                    apMember,
                    pTarget->GetLocationOnReference(),
                    fTargetingArc,
                    6.2831855f
                )) {
                continue;
            }
        }
        if (bEncounterZoneTargetRestrict.Bool()) {
            BGSEncounterZone* pTargetEncounterZone = pTarget->GetLocationEncounterZone();
            if (pMemberEncounterZone != pTargetEncounterZone) {
                continue;
            }
        }
        if (apMember->bPlayerTeammate) {
            FIGHT_REACTION eFightReaction = FIGHT_REACTION_NEUTRAL;
            if (!apMember->GetShouldAttackActor(pTarget, false, eFightReaction, false)) {
                continue;
            }
        }
        int32_t iDetectionLevel = 0;
        bool bLineOfSight = false;
        bool b360LineOfSight = false;
        CombatTimeStamp fLastDetectedTimeStamp;
        GetTargetDetectionLevel(
            apMember,
            pTarget,
            iDetectionLevel,
            fLastDetectedTimeStamp,
            bLineOfSight,
            b360LineOfSight
        );
        if (iDetectionLevel < 0 && pTarget == pCurrentTarget && pCombatState->GetHasBufferedDetection()) {
            iDetectionLevel = 1;
        }
        if (iDetectionLevel <= 0) {
            float fModifiedLastNoticedTimeStamp = pCombatTarget->fLastNoticedTimeStamp.fTimeStamp - 30.0;
            float fLatestTimeStamp = BSMAX(fModifiedLastNoticedTimeStamp, fLastDetectedTimeStamp.fTimeStamp);
            CombatTimeStamp kLatestTimeStamp(fLatestTimeStamp);
            fLastDetectedTimeStamp.fTimeStamp = kLatestTimeStamp.fTimeStamp;
            double dLastDetectedTimeStamp = fLastDetectedTimeStamp.fTimeStamp;
            if (fCurrentLastDetectedTimeStamp.fTimeStamp < dLastDetectedTimeStamp) {
                pLastDetectedTarget = pTarget;
                fCurrentLastDetectedTimeStamp.fTimeStamp = fLastDetectedTimeStamp.fTimeStamp;
            }
        }
        else {
            float fTargetScore = 0.0f;
            if (uiTargetCount > 1) {
                if (b360LineOfSight) {
                    fTargetScore = fTargetScore + f360LOSBonus;
                }
                if (bLineOfSight) {
                    fTargetScore = fTargetScore + fLOSBonus;
                }
                if (pCurrentTarget == pTarget) {
                    fTargetScore = fTargetScore + fCurrentTargetBonus;
                    // Reads the combat state through the controller again here.
                    if (!b360LineOfSight &&
                        fCurrentTargetLOSFalloffTime >
                            pCombatController->pCombatState->GetLastTarget360LineOfSightTimeStamp().ElapsedTime()) {
                        fTargetScore = fTargetScore + f360LOSBonus;
                    }
                }
                bool bIsTargetMelee = CombatUtilities::IsActorUsingMelee(pTarget);
                if (bIsMelee == bIsTargetMelee) {
                    fTargetScore = fTargetScore + fweaponTypeBonus;
                }
                float fSqrDist = 0.0f;
                pTarget->GetWorldLocation(&targetWorldLoc);
                bool bSpaceDifferent = targetWorldLoc.pSpace != memberWorldLoc.pSpace;
                if (bSpaceDifferent) {
                    if (pTarget == pCurrentTarget) {
                        fSqrDist = 0.0f;
                    }
                    else {
                        fSqrDist = fMaxDistance * fMaxDistance;
                    }
                }
                else {
                    fSqrDist = memberWorldLoc.GetDistanceSquared(targetWorldLoc);
                }
                fSqrDist = BSMIN(4194304.0f, fSqrDist);
                fTargetScore = (1.0 - fSqrDist / 4194304.0) * fDistanceBonus + fTargetScore;
                uint32_t uiShareCount = pCurrentTarget == pTarget;
                if (pCombatTarget->sAttackerCount > uiShareCount) {
                    fTargetScore = fTargetScore + fShareTargetBonus;
                }
                if ((pCombatState->IsMeleeOnly() || bSpaceDifferent) &&
                    pCombatState->IsLocationUnreachable(targetWorldLoc, 256.0f)) {
                    fTargetScore = fTargetScore - fUnreachablePenalty;
                }
                if (bSpaceDifferent && !CombatUtilities::CanActorExistInSpace(apMember, targetWorldLoc)) {
                    fTargetScore = fTargetScore - fCantExistInTargetSpacePenalty;
                }
                if (pTarget->IsDead(false) || pTarget->IsKnockedOut()) {
                    fTargetScore = fTargetScore - fDyingPenalty;
                }
                if (pTarget->IsFleeing(false)) {
                    fTargetScore = fTargetScore - fFleeingPenalty;
                }
            }
            if (fCurrentTargetScore < static_cast<double>(fTargetScore)) {
                fCurrentTargetScore = fTargetScore;
                pBestTarget = pTarget;
            }
        }
        pLastTargetChecked = pTarget;
    }
    if (!pBestTarget) {
        pBestTarget = pLastDetectedTarget;
    }
    if (!pBestTarget) {
        pBestTarget = pLastTargetChecked;
    }
    return pBestTarget;
}

void CombatGroup::FillMemberArray(BSSimpleArray<Actor*, 1024>* apActorArray) {
    apActorArray->SetSize(MemberArray.iSize, true);
    for (uint32_t i = 0; i < MemberArray.iSize; ++i) {
        apActorArray->SetAt(i, MemberArray[i].pActor);
    }
}

void CombatGroup::RemoveFromMembers() {
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        pMember->SetCombatGroup(nullptr);
    }
}

uint8_t CombatGroup::GetTargetMemberLOSCount(Actor* apTarget) {
    CombatTarget* pCombatTarget = GetCombatTargetForActor(apTarget);
    if (pCombatTarget) {
        return pCombatTarget->cMemberLOSCount;
    }

    return 0;
}

// Centres the search area on the search locations, weighted by score, or on the members when there are none (the
// highest-process member's space); the radius covers them plus fCombatSearchBufferDistance, clamped to the
// min/max search radius of the location.
void CombatGroup::ComputeSearchArea() {
    float fSearchMinRadius = 0.0f;
    float fSearchMaxRadius = 0.0f;
    uint32_t uiSearchLocationCount = GetNumSearchLocations();
    if (uiSearchLocationCount) {
        float fBestScore = -std::numeric_limits<float>::max();
        if (uiSearchLocationCount <= 1) {
            CombatSearchLocation* pSearchLoc = &GetSearchLocation(0);
            fBestScore = pSearchLoc->GetScore();
            SearchCenter = pSearchLoc->WorldLoc;
            SearchFocalPoint = pSearchLoc->WorldLoc.Point;
            GetMinMaxSearchRadius(pSearchLoc->WorldLoc, fSearchMinRadius, fSearchMaxRadius);
            fSearchRadius = fSearchMinRadius;
        }
        else {
            uint32_t uiBestIndex = 0;
            for (uint32_t i = 0; i < uiSearchLocationCount; ++i) {
                CombatSearchLocation* pSearchLoc = &GetSearchLocation(i);
                float fScore = pSearchLoc->GetScore();
                if (fBestScore < static_cast<double>(fScore)) {
                    fBestScore = fScore;
                    uiBestIndex = i;
                }
            }
            CombatSearchLocation* pBestSearchLoc = &GetSearchLocation(uiBestIndex);
            NiPoint3 center = NiPoint3::ZERO;
            float fTotalScore = 0.0f;
            GetMinMaxSearchRadius(pBestSearchLoc->WorldLoc, fSearchMinRadius, fSearchMaxRadius);
            for (uint32_t i = 0; i < uiSearchLocationCount; ++i) {
                CombatSearchLocation* pSearchLoc = &GetSearchLocation(i);
                double dDistanceSquared = pBestSearchLoc->WorldLoc.GetDistanceSquared(pSearchLoc->WorldLoc);
                if (!(fSearchMaxRadius * fSearchMaxRadius < dDistanceSquared)) {
                    float fScore = pSearchLoc->GetScore();
                    center += pSearchLoc->WorldLoc.Point * fScore;
                    fTotalScore = fTotalScore + fScore;
                }
            }
            center /= fTotalScore;
            SearchCenter.SetLocation(center, pBestSearchLoc->WorldLoc.pSpace);
            SearchFocalPoint = pBestSearchLoc->WorldLoc.Point;
            float fMaxDistanceFromCenterSquared = 0.0f;
            for (uint32_t i = 0; i < uiSearchLocationCount; ++i) {
                CombatSearchLocation* pSearchLoc = &GetSearchLocation(i);
                if (pSearchLoc->WorldLoc.pSpace == SearchCenter.pSpace) {
                    float fDistanceFromCenterSquared = SearchCenter.GetDistanceSquared(pSearchLoc->WorldLoc);
                    if (fMaxDistanceFromCenterSquared < static_cast<double>(fDistanceFromCenterSquared)) {
                        fMaxDistanceFromCenterSquared = fDistanceFromCenterSquared;
                        if (fMaxDistanceFromCenterSquared > fSearchMaxRadius * fSearchMaxRadius) {
                            break;
                        }
                    }
                }
            }
            if (fMaxDistanceFromCenterSquared > fSearchMaxRadius * fSearchMaxRadius) {
                fSearchRadius = fSearchMaxRadius;
            }
            else {
                double dDistance = NiSqrt(fMaxDistanceFromCenterSquared);
                fSearchRadius = fCombatSearchBufferDistance.Float() + dDistance;
                fSearchRadius = BSMIN(fSearchRadius, fSearchMaxRadius);
                fSearchRadius = BSMAX(fSearchRadius, fSearchMinRadius);
            }
        }
    }
    else if (!SearchCenter.pSpace) {
        uint32_t uiMemberCount = GetMemberCount();
        if (uiMemberCount <= 1) {
            BGSWorldLocation memberWorldLoc;
            Actor* pMember = GetMember(0);
            SearchCenter = *pMember->GetWorldLocation(&memberWorldLoc);
            SearchFocalPoint = SearchCenter.Point;
            GetMinMaxSearchRadius(SearchCenter, fSearchMinRadius, fSearchMaxRadius);
            fSearchRadius = fSearchMinRadius;
        }
        else {
            // The member in the highest process level (the lowest PROCESS_TYPE) gives the space and the radius.
            PROCESS_TYPE eHighestProcessLevel = PROCESS_TYPE_COUNT;
            Actor* pHighestMember = nullptr;
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                Actor* pMember = GetMember(i);
                PROCESS_TYPE eLevel = pMember->GetCurrentProcessType();
                if (eLevel < eHighestProcessLevel) {
                    eHighestProcessLevel = eLevel;
                    pHighestMember = pMember;
                }
            }
            BGSWorldLocation highestMemberWorldLoc;
            pHighestMember->GetWorldLocation(&highestMemberWorldLoc);
            NiPoint3 center = NiPoint3::ZERO;
            uint32_t uiTotalMembers = 0;
            GetMinMaxSearchRadius(highestMemberWorldLoc, fSearchMinRadius, fSearchMaxRadius);
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                Actor* pMember = GetMember(i);
                BGSWorldLocation memberWorldLoc;
                pMember->GetWorldLocation(&memberWorldLoc);
                double dDistanceSquared = highestMemberWorldLoc.GetDistanceSquared(memberWorldLoc);
                if (!(fSearchMaxRadius * fSearchMaxRadius < dDistanceSquared)) {
                    center += memberWorldLoc.Point;
                    ++uiTotalMembers;
                }
            }
            center /= static_cast<float>(uiTotalMembers);
            SearchCenter.SetLocation(center, highestMemberWorldLoc.pSpace);
            SearchFocalPoint = highestMemberWorldLoc.Point;
            float fMaxDistanceFromCenterSquared = 0.0f;
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                Actor* pMember = GetMember(i);
                BGSWorldLocation memberWorldLoc;
                pMember->GetWorldLocation(&memberWorldLoc);
                if (memberWorldLoc.pSpace == SearchCenter.pSpace) {
                    float fDistanceFromCenterSquared = SearchCenter.GetDistanceSquared(memberWorldLoc);
                    if (fMaxDistanceFromCenterSquared < static_cast<double>(fDistanceFromCenterSquared)) {
                        fMaxDistanceFromCenterSquared = fDistanceFromCenterSquared;
                        if (fMaxDistanceFromCenterSquared > fSearchMaxRadius * fSearchMaxRadius) {
                            break;
                        }
                    }
                }
            }
            if (fMaxDistanceFromCenterSquared > fSearchMaxRadius * fSearchMaxRadius) {
                fSearchRadius = fSearchMaxRadius;
            }
            else {
                double dDistance = NiSqrt(fMaxDistanceFromCenterSquared);
                fSearchRadius = fCombatSearchBufferDistance.Float() + dDistance;
                fSearchRadius = BSMIN(fSearchRadius, fSearchMaxRadius);
                fSearchRadius = BSMAX(fSearchRadius, fSearchMinRadius);
            }
        }
    }
}

void CombatGroup::BuildSearchTeleportDoorArray() {
    BGSWorldLocation memberWorldLoc;
    using doorArray_t = BSSimpleArray<TESObjectREFR*, 1024>;
    doorArray_t doorArray;
    float fSearchMinRadius = 0.0f;
    float fSearchMaxRadius = 0.0f;
    GetMinMaxSearchRadius(SearchCenter, fSearchMinRadius, fSearchMaxRadius);
    Pathing::FindTeleportDoorsForLocation(SearchCenter.pSpace, SearchCenter.Point, fSearchRadius, &doorArray);
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        pMember->GetWorldLocation(&memberWorldLoc);
        double dDistance = SearchCenter.GetDistance(memberWorldLoc);
        if (!(fSearchMaxRadius * fSearchMaxRadius > dDistance)) {
            Pathing::FindTeleportDoorsForLocation(
                memberWorldLoc.pSpace,
                memberWorldLoc.Point,
                fSearchMinRadius,
                &doorArray
            );
        }
    }
    doorArray.Sort([](const void* apOne, const void* apTwo) {
        return CompareUInt(static_cast<const uint32_t*>(apOne), static_cast<const uint32_t*>(apTwo));
    });
    for (uint32_t i = 0; i < doorArray.iSize; ++i) {
        TESObjectREFR* pDoor = doorArray[i];
        uint32_t uiNumDuplicates = 0;
        for (uint32_t j = i + 1; j < doorArray.iSize && pDoor == doorArray[j]; ++j) {
            ++uiNumDuplicates;
        }
        if (uiNumDuplicates) {
            doorArray.RemoveRange(i + 1, uiNumDuplicates, false);
        }
    }
    for (TESObjectREFR* pDoor : doorArray) {
        AddSearchDoor(pDoor);
    }
}

void CombatGroup::UpdateSearchLocations() {
    for (uint32_t i = 0; i < SearchLocations.iSize; ++i) {
        CombatSearchLocation* pSearchLocation = &SearchLocations[i];
        float fScore = pSearchLocation->GetScore();
        if (fScore < 0.0 || pSearchLocation->iTargetID) {
            SearchLocations.RemoveFast(i--, 1u);
        }
    }
    uint32_t uiTargetCount = GetTargetCount();
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        float fDetectedScore;
        float fAttackedScore;
        float fNoticedScore;
        CombatTarget* pCombatTarget = GetCombatTarget(i);
        CalculateTargetSearchLocationScores(pCombatTarget, fDetectedScore, fNoticedScore, fAttackedScore);
        if (fDetectedScore > 0.0 || fNoticedScore > 0.0 || fAttackedScore > 0.0) {
            if (fNoticedScore < static_cast<double>(fDetectedScore) ||
                fNoticedScore < static_cast<double>(fAttackedScore)) {
                if (fAttackedScore <= static_cast<double>(fDetectedScore)) {
                    AddSearchLocation(
                        pCombatTarget->LastDetectedLocation,
                        fDetectedScore,
                        pCombatTarget->pActor->iFormID
                    );
                }
                else {
                    AddSearchLocation(
                        pCombatTarget->LastAttackedMemberLocation,
                        fAttackedScore,
                        pCombatTarget->pActor->iFormID
                    );
                }
            }
            else if (fNoticedScore > 0.0) {
                BGSWorldLocation otherLoc;
                float fOtherScore = BSMAX(fDetectedScore, fAttackedScore);
                if (fOtherScore > 0.0) {
                    if (fAttackedScore < static_cast<double>(fDetectedScore)) {
                        otherLoc = pCombatTarget->LastDetectedLocation;
                    }
                    else {
                        otherLoc = pCombatTarget->LastAttackedMemberLocation;
                    }
                }
                if (fOtherScore > 0.0 && otherLoc.pSpace == pCombatTarget->LastNoticedLocation.pSpace) {
                    NiPoint3 interpolatedPoint = otherLoc.Point * fOtherScore;
                    interpolatedPoint += pCombatTarget->LastNoticedLocation.Point * fNoticedScore;
                    interpolatedPoint /= fOtherScore + fNoticedScore;
                    BGSWorldLocation interpolatedLocation(interpolatedPoint, pCombatTarget->LastNoticedLocation.pSpace);
                    AddSearchLocation(interpolatedLocation, fNoticedScore, pCombatTarget->pActor->iFormID);
                }
                else {
                    AddSearchLocation(
                        pCombatTarget->LastNoticedLocation,
                        fNoticedScore,
                        pCombatTarget->pActor->iFormID
                    );
                }
            }
        }
    }
}

bool CombatGroup::UpdateSearchArea() {
    UpdateSearchLocations();
    BGSWorldLocation searchCenter = SearchCenter;
    float fOldSearchRadius = fSearchRadius;
    ComputeSearchArea();
    uint8_t bAreaChanged = fOldSearchRadius > fSearchRadius + 128.0 ||
        fOldSearchRadius < fSearchRadius - 128.0 ||
        searchCenter.GetDistanceSquared(SearchCenter) > 4096.0;
    if (bAreaChanged) {
        BuildSearchTeleportDoorArray();
    }
    return bAreaChanged;
}

void CombatGroup::UpdateGroupStrategy() {
    CombatTimer kTimeStamp;
    if (GetMemberCount() <= 1) {
        pGroupStrategy = nullptr;
        bStrategyForced = false;
        ChooseStrategyTimer.Reset(0.0);
    }
    else {
        if (!bStrategyForced && ChooseStrategyTimer.Check()) {
            pGroupStrategy = CombatGroup::SelectGroupStrategy(this);
            ChooseStrategyTimer.Reset(5.0f);
            if (pGroupStrategy) {
                iLastGroupStrategyChosenIndex = pGroupStrategy->GetType();
                kTimeStamp.SetStartTime(CombatTimer::GetTimer());
                fLastGroupStrategyChosenTimeStamp.fTimeStamp = kTimeStamp.fStartTime;
            }
        }
        if (pGroupStrategy && UpdateStrategyTimer.Check()) {
            if (!pGroupStrategy->Update(this)) {
                pGroupStrategy = nullptr;
                bStrategyForced = false;
            }
            UpdateStrategyTimer.Reset(1.0);
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
    NiPoint3 kTargetPos;
    BSSimpleArray<PathingAvoidNode, 1024>* pAvoidNodes = apExistingAvoidNodeArray;
    NiPoint3 kMidPoint = (apActor->GetLocationOnReference() + arDestination) / 2.0f;
    float fAvoidRadiusSqr = (arDestination - kMidPoint).SqrLength() + afExtraRadius * afExtraRadius;
    fAvoidRadiusSqr = fMovementTargetAvoidRadiusMult.Float() * fMovementTargetAvoidRadiusMult.Float() * fAvoidRadiusSqr;
    uint32_t uiTargetCount = GetTargetCount();
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        Actor* pTarget = GetTarget(i);
        if (pTarget != apIgnoreActor) {
            kTargetPos = pTarget->GetLocationOnReference();
            double dDistanceSquared = (kMidPoint - kTargetPos).SqrLength();
            if (fAvoidRadiusSqr > dDistanceSquared) {
                if (!pAvoidNodes) {
                    pAvoidNodes = new PathingAvoidNodeArray;
                }
                float fCost = fMovementTargetAvoidCost.Float();
                pAvoidNodes->Add(PathingAvoidNode(kTargetPos, fMovementTargetAvoidRadius.Float(), fCost));
            }
        }
    }
    return static_cast<PathingAvoidNodeArray*>(pAvoidNodes);
}

PathingAvoidNodeArray*
CombatGroup::CreateFleeAvoidNodeArray(Actor* apActor, PathingAvoidNodeArray* apExistingAvoidNodeArray) {
    BGSWorldLocation kTargetLoc;
    BGSWorldLocation kActorLoc;
    BSSimpleArray<PathingAvoidNode, 1024>* pAvoidNodes = apExistingAvoidNodeArray;
    uint32_t uiTargetCount = GetTargetCount();
    float fGatherRadiusSqr = fCombatFleeTargetGatherRadius.Float() * fCombatFleeTargetGatherRadius.Float();
    apActor->GetWorldLocation(&kActorLoc);
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        TESObjectREFR* pTarget = GetTarget(i);
        pTarget->GetWorldLocation(&kTargetLoc);
        double dDistanceSquared = kActorLoc.GetDistanceSquared(kTargetLoc);
        if (fGatherRadiusSqr > dDistanceSquared) {
            if (!pAvoidNodes) {
                pAvoidNodes = new PathingAvoidNodeArray;
            }
            float fCost = fCombatFleeTargetAvoidCost.Float();
            float fRadius = fCombatFleeTargetAvoidRadius.Float();
            pAvoidNodes->Add(PathingAvoidNode(*kTargetLoc.GetPoint(), fRadius, fCost));
        }
    }
    return static_cast<PathingAvoidNodeArray*>(pAvoidNodes);
}

double CombatGroup::GetMemberCombatStrength(Actor* apMember) {
    if (!apMember) {
        return fMemberCombatStrength;
    }
    float fCombatStrength = 0.0f;
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        CombatMember* pCombatMember = GetCombatMember(i);
        CombatController* pCombatController = pCombatMember->pActor->GetCombatController();
        if (apMember == pCombatMember->pActor && pCombatController) {
            float fMemberStrength = pCombatController->pCombatState->fCombatStrength;
            if (fMemberStrength > 0.0) {
                fCombatStrength = fCombatStrength + fMemberStrength;
            }
        }
        else if ((!pCombatController || !pCombatController->IsFleeing()) && pCombatMember->fCombatStrength > 0.0) {
            fCombatStrength = fCombatStrength + pCombatMember->fCombatStrength;
        }
    }
    return fCombatStrength;
}

void CombatGroup::UpdateClusters() {
    BGSWorldLocation kLocation;
    if (!CombatGroup::bUseSimpleCombatGroupClustering) {
        UpdateClusterCenters();
        if (ClusterUpdateTimer.Check()) {
            ClusterData clusterData[64];
            float fDistanceSqrArray[64][64];
            ClusterUpdateTimer.Reset(CombatGroup::fCombatClusterUpdateTime);
            // BSMIN is the int instantiation here, so the compare is signed.
            uint32_t uiMemberCount = BSMIN(static_cast<int32_t>(GetMemberCount()), 63);
            ourMemset(clusterData, 0, sizeof(clusterData));
            ourMemset(fDistanceSqrArray, 0, sizeof(fDistanceSqrArray));
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                clusterData[i].pCombatMember = GetCombatMember(i);
                clusterData[i].WorldLoc = *clusterData[i].pCombatMember->pActor->GetWorldLocation(&kLocation);
                clusterData[i].cClosestMember = -1;
                if (clusterData[i].pCombatMember->pCluster &&
                    fClusterDistance * fClusterDistance * 4.0 <
                        clusterData[i].pCombatMember->pCluster->Center.GetDistanceSquared(clusterData[i].WorldLoc)) {
                    --clusterData[i].pCombatMember->pCluster->iMemberCount;
                    clusterData[i].pCombatMember->pCluster = nullptr;
                }
            }
            for (uint32_t i = 0; i < ClusterArray.iSize; ++i) {
                CombatGroupCluster* pCluster = ClusterArray[i];
                if (!pCluster->iMemberCount) {
                    ClusterArray.RemoveFast(i, 1u);
                    ourFree(pCluster);
                    --i;
                }
            }
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                if (!clusterData[i].pCombatMember->pCluster) {
                    float fClosestDistSqr = std::numeric_limits<float>::max();
                    int8_t cClosestMember = 0;
                    for (uint32_t j = 0; j < uiMemberCount; ++j) {
                        if (i != j) {
                            if (fDistanceSqrArray[i][j] == 0.0) {
                                float fDistSqr = clusterData[i].WorldLoc.GetDistanceSquared(clusterData[j].WorldLoc);
                                fDistanceSqrArray[i][j] = fDistSqr;
                                fDistanceSqrArray[j][i] = fDistSqr;
                            }
                            if (fClosestDistSqr > static_cast<double>(fDistanceSqrArray[i][j])) {
                                cClosestMember = j;
                                fClosestDistSqr = fDistanceSqrArray[i][j];
                            }
                        }
                    }
                    clusterData[i].cClosestMember = cClosestMember;
                }
            }
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                if (!clusterData[i].pCombatMember->pCluster &&
                    clusterData[clusterData[i].cClosestMember].cClosestMember == static_cast<int8_t>(i)) {
                    CombatGroupCluster* pCluster = new CombatGroupCluster;
                    ClusterArray.Add(pCluster);
                    clusterData[i].pCombatMember->pCluster = pCluster;
                    clusterData[clusterData[i].cClosestMember].pCombatMember->pCluster = pCluster;
                    NiPoint3 center = clusterData[i].WorldLoc.Point +
                        clusterData[clusterData[i].cClosestMember].WorldLoc.Point;
                    center /= 2.0f;
                    pCluster->Center.SetLocation(center, clusterData[i].WorldLoc.pSpace);
                }
            }
            for (uint32_t i = 0; i < ClusterArray.iSize; ++i) {
                CombatGroupCluster* pCluster1 = ClusterArray[i];
                for (uint32_t j = i + 1; j < ClusterArray.iSize; ++j) {
                    CombatGroupCluster* pCluster2 = ClusterArray[j];
                    if (fClusterDistance * fClusterDistance > pCluster1->Center.GetDistanceSquared(pCluster2->Center)) {
                        for (uint32_t k = 0; k < uiMemberCount; ++k) {
                            if (clusterData[k].pCombatMember->pCluster == pCluster2) {
                                clusterData[k].pCombatMember->pCluster = pCluster1;
                            }
                        }
                        ourFree(pCluster2);
                        ClusterArray.RemoveFast(j--, 1u);
                    }
                }
            }
            for (uint32_t i = 0; i < uiMemberCount; ++i) {
                if (!clusterData[i].pCombatMember->pCluster) {
                    CombatGroupCluster* pCluster = nullptr;
                    uint32_t uiClosestMember = clusterData[i].cClosestMember;
                    while (!pCluster) {
                        pCluster = clusterData[uiClosestMember].pCombatMember->pCluster;
                        uiClosestMember = clusterData[uiClosestMember].cClosestMember;
                    }
                    if (fClusterDistance * fClusterDistance >
                        clusterData[i].WorldLoc.GetDistanceSquared(pCluster->Center)) {
                        clusterData[i].pCombatMember->pCluster = pCluster;
                        for (uiClosestMember = clusterData[i].cClosestMember;
                             !clusterData[uiClosestMember].pCombatMember->pCluster;
                             uiClosestMember = clusterData[uiClosestMember].cClosestMember) {
                            clusterData[uiClosestMember].pCombatMember->pCluster = pCluster;
                        }
                    }
                }
            }
            UpdateClusterCenters();
        }
    }
}

void CombatGroup::UpdateClusterCenters() {
    NiPoint3 averagePoint;
    NiPoint3 centerPoint;
    for (CombatGroupCluster* pCluster : ClusterArray) {
        pCluster->Center.SetPoint(NiPoint3::ZERO);
        pCluster->iMemberCount = 0;
    }
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        CombatMember* pCombatMember = GetCombatMember(i);
        if (pCombatMember->pCluster) {
            centerPoint = pCombatMember->pCluster->Center.Point;
            centerPoint += pCombatMember->pActor->GetLocationOnReference();
            pCombatMember->pCluster->Center.SetPoint(centerPoint);
            ++pCombatMember->pCluster->iMemberCount;
        }
    }
    for (uint32_t i = 0; i < ClusterArray.iSize; ++i) {
        CombatGroupCluster* pCluster = ClusterArray[i];
        averagePoint = pCluster->Center.Point;
        averagePoint /= static_cast<float>(pCluster->iMemberCount);
        pCluster->Center.SetPoint(averagePoint);
    }
}

void CombatGroup::ForceGroupStrategy(CombatGroupStrategy* apStrategy) {
    pGroupStrategy = apStrategy;
    if (pGroupStrategy) {
        CombatGroupData kGroupData(this);
        ClearGroupStrategyAssignments();
        pGroupStrategy->Execute(this, &kGroupData);
        iLastGroupStrategyChosenIndex = pGroupStrategy->GetType();
        CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
        fLastGroupStrategyChosenTimeStamp = kTimeStamp;
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
    if (IsFleeing()) {
        double dDetectedElapsed = apCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
        if (fCombatGroupFleeingTargetLostRemoveTime.Float() < dDetectedElapsed) {
            return true;
        }
        dDetectedElapsed = apCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
        if (fCombatGroupFleeingTargetLostRemoveDistanceTime.Float() < dDetectedElapsed) {
            if (!FindMemberNearActor(apCombatTarget->pActor, fCombatGroupFleeingTargetLostRemoveDistance.Float())) {
                return true;
            }
        }
    }
    double dNoticedElapsed = apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
    if (fCombatTargetLostRemoveTime.Float() < dNoticedElapsed) {
        if (!apCombatTarget->sAttackerCount) {
            return true;
        }
        dNoticedElapsed = apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
        if (fCombatTargetLostRemoveDistanceTime.Float() < dNoticedElapsed) {
            bool bNothingSeen = false;
            if (iSearchCount == 1 && spPathingLOSGridMap.m_pObject) {
                bNothingSeen = spPathingLOSGridMap->fPercentSeen == 0.0;
            }

            if (bNothingSeen) {
                return true;
            }
            if (!FindMemberNearActor(apCombatTarget->pActor, fCombatTargetLostRemoveDistance.Float())) {
                return true;
            }
        }
    }
    if (!iSearchCount) {
        return apCombatTarget->pActor->IsUnConscious() ||
            (apCombatTarget->pActor->IsDead(false) && !apCombatTarget->pActor->IsDying());
    }
    int32_t iLastSearchNoticed = apCombatTarget->sLastSearchNoticed;
    if (iSearchCount > iCombatTargetLostRemoveSearchCount.Int() + iLastSearchNoticed) {
        return true;
    }
    double dSearchElapsed = fSearchStartedTimeStamp.ElapsedTime();
    if (fCombatSearchTargetHiddenMaxTime.Float() > dSearchElapsed) {
        double dAttackedElapsed = apCombatTarget->fLastAttackedMemberTimeStamp.ElapsedTime();
        if (fCombatSearchTargetHiddenMaxTime.Float() <= dAttackedElapsed &&
            -std::numeric_limits<float>::max() == apCombatTarget->fLastDetectedTimeStamp.fTimeStamp) {
            dNoticedElapsed = apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
            if (fCombatSearchTargetHiddenRemoveTime.Float() < dNoticedElapsed) {
                return true;
            }
        }
    }

    return apCombatTarget->pActor->IsUnConscious() ||
        (apCombatTarget->pActor->IsDead(false) && !apCombatTarget->pActor->IsDying());
}

bool CombatGroup::MergeGroup(CombatGroup* apCombatGroup) {
    BSSimpleArray<CombatTarget, 1024>* pNewTargetArray = &apCombatGroup->TargetArray;
    for (uint32_t i = 0; i < pNewTargetArray->iSize; ++i) {
        CombatTarget* pNewTarget = &(*pNewTargetArray)[i];
        if (!CanAddTarget(pNewTarget->pActor)) {
            return false;
        }
    }
    for (uint32_t i = 0; i < TargetArray.iSize; ++i) {
        CombatTarget* pTarget = &TargetArray[i];
        if (!apCombatGroup->CanAddTarget(pTarget->pActor)) {
            return false;
        }
    }
    for (uint32_t i = 0; i < apCombatGroup->GetClusterCount(); ++i) {
        CombatGroupCluster* pCluster = apCombatGroup->GetCluster(i);
        ClusterArray.Add(pCluster);
    }
    apCombatGroup->ClearClusterArray();
    BSSimpleArray<CombatMember, 1024>* pNewMemberArray = &apCombatGroup->MemberArray;
    for (uint32_t i = 0; i < pNewMemberArray->iSize; ++i) {
        CombatMember* pNewMember = &(*pNewMemberArray)[i];
        if (AddMember(pNewMember->pActor)) {
            pNewMember->pActor->SetCombatGroup(this);
            CombatMember* pCombatMember = GetCombatMemberForActor(pNewMember->pActor);
            if (pCombatMember) {
                pCombatMember->CopyData(pNewMember);
            }
        }
        else {
            const char* pEditorID = pNewMember->pActor->GetFormEditorID();
            Warning(
                "COMBAT: Error merging groups.  Could not add member %08X %s",
                pNewMember->pActor->iFormID,
                pEditorID
            );
        }
    }
    for (uint32_t i = 0; i < pNewTargetArray->iSize; ++i) {
        CombatTarget* pNewTarget = &(*pNewTargetArray)[i];
        if (AddTarget(pNewTarget->pActor)) {
            CombatTarget* pCombatTarget = GetCombatTargetForActor(pNewTarget->pActor);
            if (pCombatTarget) {
                pCombatTarget->CopyData(pNewTarget);
            }
        }
        else {
            const char* pEditorID = pNewTarget->pActor->GetFormEditorID();
            Warning(
                "COMBAT: Error merging groups.  Could not add target %08X %s",
                pNewTarget->pActor->iFormID,
                pEditorID
            );
        }
    }
    char cGroupCombatMusicState = apCombatGroup->GetCombatMusicState();
    if (cGroupCombatMusicState != -1) {
        cCombatMusicState = cGroupCombatMusicState;
    }
    return true;
}

void CombatGroup::ClearClusterArray() {
    ClusterArray.Clear(true);
}

int32_t CombatGroup::GetDetectionValue(Actor* apTarget) {
    int32_t iDetectionLevel = -100;
    CombatTimeStamp fLastDetectedTimeStamp;
    bool bLineOfSight = false;
    bool b360LineOfSight = false;
    GetTargetDetectionLevel(nullptr, apTarget, iDetectionLevel, fLastDetectedTimeStamp, bLineOfSight, b360LineOfSight);
    return iDetectionLevel;
}

CombatTimeStamp* CombatGroup::GetTargetLastDetectedTimeStamp(CombatTimeStamp* apResult, Actor* apTarget) {
    CombatTarget* pCombatTarget = GetCombatTargetForActor(apTarget);
    if (pCombatTarget) {
        *apResult = pCombatTarget->fLastDetectedTimeStamp;
    }
    else {
        ::new (apResult) CombatTimeStamp(-std::numeric_limits<float>::max());
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
    float fRadius = 1024.0f;
    float fBoundingSphereRadiusSquared = ComputeTargetBoundingSphere(targetCenter);
    if (fBoundingSphereRadiusSquared > fRadius * fRadius) {
        fRadius = NiSqrt(fBoundingSphereRadiusSquared);
    }
    PathingAvoidNodeArray* pAvoidNodeArray = apExistingAvoidNodeArray;
    if (!apExistingAvoidNodeArray) {
        pAvoidNodeArray = new PathingAvoidNodeArray;
    }
    float fCost = fMovementTargetAvoidCost.Float() * 4.0;
    pAvoidNodeArray->Add(PathingAvoidNode(targetCenter, fRadius, fCost));
    return pAvoidNodeArray;
}

float CombatGroup::ComputeMemberBoundingSphere(NiPoint3& arCenter) {
    NiPoint3 corners[8];
    ComputeMemberBoundingBox(corners, false);
    arCenter = (*corners + corners[7]) / 2.0f;
    NiPoint3 vec = *corners - corners[7];
    return vec.SqrLength();
}

float CombatGroup::ComputeTargetBoundingSphere(NiPoint3& arCenter) {
    // Eight corners, as ComputeMemberBoundingSphere has them: the box's opposite corners are the first and the last.
    NiPoint3 corners[8];
    ComputeTargetBoundingBox(corners, false);
    arCenter = (*corners + corners[7]) / 2.0f;
    NiPoint3 vec = *corners - corners[7];
    return vec.SqrLength();
}

void CombatGroup::ComputeMemberBoundingBox(NiPoint3* arPoints, bool abAlreadyInitialized) {
    if (!abAlreadyInitialized) {
        for (uint32_t i = 0; i < 8; ++i) {
            for (uint32_t j = 0; j < 3; ++j) {
                float fInit = BoundsCornerIsMax[i][j] ? -std::numeric_limits<float>::max()
                                                      : std::numeric_limits<float>::max();
                arPoints[i][j] = fInit;
            }
        }
    }
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t m = 0; m < uiMemberCount; ++m) {
        Actor* pMember = GetMember(m);
        NiPoint3 kLocation = pMember->GetLocationOnReference();
        for (uint32_t i = 0; i < 8; ++i) {
            for (uint32_t j = 0; j < 3; ++j) {
                float fValue = kLocation[j];
                if (BoundsCornerIsMax[i][j]) {
                    if (arPoints[i][j] < static_cast<double>(fValue)) {
                        arPoints[i][j] = fValue;
                    }
                }
                else if (arPoints[i][j] > static_cast<double>(fValue)) {
                    arPoints[i][j] = fValue;
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
    float fHealthPercentage = 0.0f;
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        double dHealth = pMember->GetActorFloatValue(ActorValue::HEALTH);
        fHealthPercentage = dHealth / static_cast<double>(pMember->GetBaseActorValue(ActorValue::HEALTH)) +
            fHealthPercentage;
    }
    return fHealthPercentage / static_cast<double>(uiMemberCount);
}

void CombatGroup::UpdateSearch() {
    NiPoint3 closestPoint;
    if (iInitializedMemberCount) {
        uint32_t uiTargetCount = GetTargetCount();
        bool bDontSearch = false;
        if (!uiTargetCount || iFleeingMemberCount >= iInitializedMemberCount) {
            bDontSearch = true;
        }
        if (IsSearching()) {
            if (bDontSearch) {
                ClearSearch();
            }
            else {
                for (uint32_t i = 0; i < uiTargetCount; ++i) {
                    CombatTarget* pCombatTarget = GetCombatTarget(i);
                    // The elapsed time is stored as a double before the compare.
                    double dElapsed = pCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
                    if (fCombatDetectionBufferTime.Float() > dElapsed) {
                        ClearSearch();
                        break;
                    }
                }
            }
        }
        else if (!bDontSearch) {
            uint32_t uiLostTargetCount = 0;
            uint32_t uiTargetedTargets = 0;
            for (uint32_t i = 0; i < uiTargetCount; ++i) {
                CombatTarget* pCombatTarget = GetCombatTarget(i);
                // The elapsed time is stored as a double before the compare.
                double dElapsed = pCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
                if (fCombatDetectionLostTime.Float() <= dElapsed) {
                    ++uiLostTargetCount;
                }
                if (pCombatTarget->sAttackerCount) {
                    ++uiTargetedTargets;
                }
            }
            if (uiTargetedTargets && uiLostTargetCount == uiTargetCount) {
                StartSearch();
            }
        }
    }
    if (IsSearching()) {
        if (pSearchingMember && (pSearchingMember->DidPathingFail() || !pSearchingMember->IsWaitingOnPath())) {
            pSearchingMember = nullptr;
        }

        if (SearchUpdateTimer.Check()) {
            if (spPathingLOSGridMap.m_pObject && spPathingLOSGridMap->fPercentSeen > 0.9900000095367432) {
                spPathingLOSGridMap->ClearSeen();
                ++iSearchCount;
                SearchAreaUpdateTimer.ForceTrigger();
            }

            bool bSearchAreaChanged = false;
            if (SearchAreaUpdateTimer.Check()) {
                bSearchAreaChanged = UpdateSearchArea();
                SearchAreaUpdateTimer.Reset(fCombatSearchAreaUpdateTime.Float());
            }
            if (SearchCenter.IsLocationLoaded()) {
                if (!spPathingLOSGridMap.m_pObject || bSearchAreaChanged) {
                    TESWorldSpace* pWorld = nullptr;
                    TESObjectCELL* pCell = nullptr;
                    if (SearchCenter.GetCellOrWorld(pCell, pWorld) &&
                        Pathing::FindClosestPointOnNavmesh(pWorld, pCell, SearchFocalPoint, &closestPoint)) {
                        PathingLocation searchPathingCenter(&closestPoint, pCell, pWorld);
                        if (searchPathingCenter.ResolveNavMeshInfo(nullptr)) {
                            searchPathingCenter.SetLocationSimple(SearchCenter.Point);
                            if (!spPathingLOSGridMap.m_pObject) {
                                spPathingLOSGridMap = new PathingLOSGridMap;
                            }
                            spPathingLOSGridMap->Initialize(searchPathingCenter, fSearchRadius);
                            spPathingLOSGridMap->SetSightRadius(fCombatSearchSightRadius.Float());
                            bUpdateSearchDebugGeometry = true;
                        }
                    }
                }
            }
            else if (spPathingLOSGridMap.m_pObject) {
                spPathingLOSGridMap = static_cast<PathingLOSGridMap*>(nullptr);
            }
            if (spPathingLOSGridMap.m_pObject) {
                uint32_t uiMemberCount = GetMemberCount();
                BSScrapArray<PathingLocation> Locations(uiMemberCount, 0u, nullptr);
                BSScrapArray<float> Headings(uiMemberCount, 0u, nullptr);
                for (uint32_t i = 0; i < uiMemberCount; ++i) {
                    Actor* pMember = GetMember(i);
                    TESForm* pSpace = pMember->GetSpace();
                    if (pSpace == SearchCenter.pSpace) {
                        PathingLocation memberPathingLocation(pMember);
                        if (memberPathingLocation.HasNavMesh()) {
                            Locations.Add(memberPathingLocation);
                            float fHeading = ClampAngle(pMember->GetHeading(false));
                            Headings.Add(fHeading);
                        }
                    }
                }
                if (Locations.iSize) {
                    float fSeenPercent = spPathingLOSGridMap->fPercentSeen;
                    Pathing::ComputeLineOfSightMap(spPathingLOSGridMap, Locations, &Headings);
                    double dNewSeenPercent = spPathingLOSGridMap->fPercentSeen;
                    if (fSeenPercent != dNewSeenPercent) {
                        bUpdateSearchDebugGeometry = true;
                    }
                }
            }
            SearchUpdateTimer.Reset(fCombatSearchUpdateTime.Float());
            uint32_t uiMemberCount = GetMemberCount();
            uint32_t uiDialogueMember = RandomInt(0, static_cast<int32_t>(uiMemberCount));
            Actor* pMember = GetMember(uiDialogueMember);
            Actor* pTarget = pMember->GetCombatTarget();
            if (pMember != pPlayer && pTarget) {
                CombatTarget* pCombatTarget = GetCombatTargetForActor(pTarget);
                if (!pCombatTarget ||
                    -std::numeric_limits<float>::max() == pCombatTarget->fLastDetectedTimeStamp.fTimeStamp) {
                    pCombatDialogueManager->StartDialogue(pMember, pTarget, DT_DETECTION, 0, false, nullptr);
                }
                else {
                    pCombatDialogueManager->StartDialogue(pMember, pTarget, DT_DETECTION, 1, false, nullptr);
                }
            }
        }
    }
}

// The last detected and last noticed targets are found and stored but never read.
void CombatGroup::StartSearch() {
    uint32_t uiTargetCount = GetTargetCount();
    UpdateSearchLocations();
    ComputeSearchArea();
    BuildSearchTeleportDoorArray();
    iSearchCount = 1;
    SearchUpdateTimer.Reset(0.5f);
    SearchAreaUpdateTimer.Reset(fCombatSearchAreaUpdateTime.Float());
    CombatTimeStamp kStarted(CombatTimer::GetTimer());
    fSearchStartedTimeStamp = kStarted;
    CombatTimeStamp fLastDetectedTargetTimeStamp(-std::numeric_limits<float>::max());
    CombatTimeStamp fLastNoticedTargetTimeStamp(-std::numeric_limits<float>::max());
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        CombatTarget* pCombatTarget = GetCombatTarget(i);
        pCombatTarget->sLastSearchNoticed = 0;
        if (fLastDetectedTargetTimeStamp.fTimeStamp < pCombatTarget->fLastDetectedTimeStamp.fTimeStamp) {
            fLastDetectedTargetTimeStamp.fTimeStamp = pCombatTarget->fLastDetectedTimeStamp.fTimeStamp;
        }
        else if (fLastNoticedTargetTimeStamp.fTimeStamp < pCombatTarget->fLastNoticedTimeStamp.fTimeStamp) {
            fLastNoticedTargetTimeStamp.fTimeStamp = pCombatTarget->fLastNoticedTimeStamp.fTimeStamp;
        }
    }
    float fMax = fCombatDetectionDialogueIdleMaxElapsedTime.Float();
    DetectionDialogueTimers[0].Reset(RandomFloat(fCombatDetectionDialogueIdleMinElapsedTime.Float(), fMax));
    float fMax2 = fCombatDetectionDialogueIdleMaxElapsedTime.Float();
    DetectionDialogueTimers[1].Reset(RandomFloat(fCombatDetectionDialogueIdleMinElapsedTime.Float(), fMax2));
}

void CombatGroup::GetMinMaxSearchRadius(BGSWorldLocation& arSearchLocation, float& arfMinRadius, float& arfMaxRadius) {
    int32_t iExterior = !arSearchLocation.pSpace || arSearchLocation.pSpace->GetFormType() != CELL_ID;
    int32_t iRadiusIndex = iSearchCount >= 2;
    arfMinRadius = pSearchRadius[iExterior][iRadiusIndex]->Float();
    arfMaxRadius = pSearchRadius[iExterior][iRadiusIndex + 1]->Float();
}

void CombatGroup::ClearSearch() {
    CombatTimer kTimer;
    spPathingLOSGridMap = static_cast<PathingLOSGridMap*>(nullptr);
    spSearchDebugGeometry = static_cast<NiRefObject*>(nullptr);
    bUpdateSearchDebugGeometry = false;
    SearchUpdateTimer.Clear();
    SearchAreaUpdateTimer.Clear();
    kTimer.SetStartTime(-std::numeric_limits<float>::max());
    fSearchStartedTimeStamp.fTimeStamp = kTimer.fStartTime;
    iSearchCount = 0;
    fSearchRadius = 0.0f;
    SearchCenter.Clear();
    SearchFocalPoint = NiPoint3::ZERO;
    SearchLocations.Clear(true);
    SearchTeleportDoors.Clear(true);
}

CombatSearchLocation*
CombatGroup::FindSearchLocation(Actor* apMember, uint8_t acMinScore, BGSWorldLocation* apCurrentSearchLocation) {
    BGSWorldLocation memberWorldLoc;
    CombatController* pCombatController = apMember->GetCombatController();
    apMember->GetWorldLocation(&memberWorldLoc);
    int32_t iIndex = -1;
    float fCurrentScore = 0.0f;
    uint32_t uiSearchLocationCount = GetNumSearchLocations();
    for (uint32_t i = 0; i < uiSearchLocationCount; ++i) {
        CombatSearchLocation* pSearchLoc = &GetSearchLocation(i);
        float fScore = pSearchLoc->GetScore();
        float fScoreBonus = 0.0f;
        if (apCurrentSearchLocation) {
            if (pSearchLoc->WorldLoc.pSpace == apCurrentSearchLocation->pSpace &&
                pSearchLoc->WorldLoc.Point == apCurrentSearchLocation->Point) {
                fScoreBonus = fScoreBonus + 5.0;
            }
        }
        if (!(static_cast<double>(acMinScore) > fScore + fScoreBonus)) {
            pSearchLoc->GetModifiedScore(memberWorldLoc);
            if (!(fCurrentScore > static_cast<double>(fScore)) &&
                (!pCombatController ||
                 pCombatController->pCombatState->CheckMovement(pSearchLoc->WorldLoc, true, false))) {
                iIndex = i;
                fCurrentScore = fScore;
            }
        }
    }
    if (iIndex == -1) {
        return nullptr;
    }

    return &GetSearchLocation(static_cast<uint32_t>(iIndex));
}

void CombatGroup::CalculateTargetSearchLocationScores(
    CombatTarget* apCombatTarget,
    float& arfDetectedScore,
    float& arfNoticedScore,
    float& arfAttackedScore
) {
    double dDetectedScore = 70.0 - apCombatTarget->fLastDetectedTimeStamp.ElapsedTime();
    arfDetectedScore = fCombatDetectionLostTime.Float() + dDetectedScore;
    arfNoticedScore = 30.0 - apCombatTarget->fLastNoticedTimeStamp.ElapsedTime();
    arfAttackedScore = 70.0 - apCombatTarget->fLastAttackedMemberTimeStamp.ElapsedTime();
}

TESObjectREFR* CombatGroup::FindSearchDoor(Actor* apMember, float afMaxDistance, bool abCheckExist) {
    BGSWorldLocation teleportWorldLoc;
    BGSWorldLocation doorWorldLoc;
    BGSWorldLocation memberWorldLoc;
    if (apMember->IsCreature()) {
        TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(apMember->GetActorBase());
        if (pBaseData->GetCantOpenDoors()) {
            return nullptr;
        }
    }
    CombatController* pCombatController = apMember->GetCombatController();
    apMember->GetWorldLocation(&memberWorldLoc);
    TESObjectREFR* pClosestDoor = nullptr;
    float fClosestDoorDistanceSquared = afMaxDistance * afMaxDistance;
    for (uint32_t i = 0; i < SearchTeleportDoors.iSize; ++i) {
        CombatSearchDoor* pSearchDoor = &SearchTeleportDoors[i];
        if (pSearchDoor->pDoor && pSearchDoor->pLinkedDoor) {
            uint32_t uiFailedAttempts = pSearchDoor->cFailedAttempts;
            if (uiFailedAttempts <= iCombatSearchMaxSearchDoorFailure.Int() &&
                !pSearchDoor->bReserved &&
                !pSearchDoor->bInvestigated) {
                DoorTeleportData* pTeleportData = pSearchDoor->pDoor->GetTeleport();
                if (pTeleportData &&
                    (!abCheckExist ||
                     (pTeleportData->GetTeleportWorldLocation(&teleportWorldLoc),
                      CombatUtilities::CanActorExistInSpace(apMember, teleportWorldLoc)))) {
                    pSearchDoor->pDoor->GetWorldLocation(&doorWorldLoc);
                    if (!pCombatController ||
                        pCombatController->pCombatState->CheckMovement(doorWorldLoc, true, false)) {
                        float fDistanceSquared = memberWorldLoc.GetDistanceSquared(doorWorldLoc);
                        if (fClosestDoorDistanceSquared > static_cast<double>(fDistanceSquared)) {
                            pClosestDoor = pSearchDoor->pDoor;
                            fClosestDoorDistanceSquared = fDistanceSquared;
                        }
                    }
                }
            }
        }
    }
    return pClosestDoor;
}

void CombatGroup::AddSearchDoor(TESObjectREFR* apDoor) {
    if (!GetCombatSearchDoor(apDoor)) {
        DoorTeleportData* pTeleportData = apDoor->GetTeleport();
        if (pTeleportData && pTeleportData->pLinkedDoor) {
            uint32_t uiIndex = SearchTeleportDoors.AddUninitialized();
            SearchTeleportDoors.GetAt(static_cast<int32_t>(uiIndex)).pDoor = apDoor;
            SearchTeleportDoors.GetAt(static_cast<int32_t>(uiIndex)).pLinkedDoor = pTeleportData->pLinkedDoor;
            SearchTeleportDoors.GetAt(static_cast<int32_t>(uiIndex)).cFailedAttempts = 0;
            SearchTeleportDoors.GetAt(static_cast<int32_t>(uiIndex)).bReserved = false;
            SearchTeleportDoors.GetAt(static_cast<int32_t>(uiIndex)).bInvestigated = false;
        }
    }
}

bool CombatGroup::CalculateSearchIgnoreLocations(Actor* apSearcher, BSSimpleArray<NiPoint3, 1024>* apIgnoreLocations) {
    NiPoint3 kPathGoal;
    uint32_t uiMemberCount = GetMemberCount();
    if (uiMemberCount <= 1) {
        return false;
    }
    bool bFound = false;
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        if (pMember != apSearcher) {
            CombatController* pController = pMember->GetCombatController();
            if (pController && pController->IsSearching() && pMember->GetCurrentPathfindingGoal(&kPathGoal)) {
                apIgnoreLocations->Add(kPathGoal);
                bFound = true;
            }
        }
    }
    return bFound;
}

void CombatGroup::UpdateCombatStrength() {
    fMemberCombatStrength = 0.0f;
    fAverageMemberDamagePerSecond = 0.0f;
    uint32_t uiMemberCount = GetMemberCount();
    uint32_t uiNonFleeingMemberCount = 0;
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        bool bFleeing = false;
        CombatMember* pCombatMember = GetCombatMember(i);
        if (pCombatMember->pActor == pPlayer) {
            pCombatMember->fDamagePerSecond = pPlayer->GetDamagePerSecond();
            pCombatMember->fCombatStrength = pPlayer->CalculateCombatStrength(pCombatMember->fDamagePerSecond);
        }
        else {
            CombatController* pCombatController = pCombatMember->pActor->GetCombatController();
            if (pCombatController) {
                bFleeing = pCombatController->IsFleeing();
                pCombatMember->fDamagePerSecond = pCombatController->pCombatState->fDamagePerSecond;
                pCombatMember->fCombatStrength = pCombatController->pCombatState->fCombatStrength;
            }
        }
        if (!bFleeing && pCombatMember->fCombatStrength >= 0.0 && pCombatMember->fDamagePerSecond >= 0.0) {
            fMemberCombatStrength = fMemberCombatStrength + pCombatMember->fCombatStrength;
            fAverageMemberDamagePerSecond = fAverageMemberDamagePerSecond + pCombatMember->fDamagePerSecond;
            ++uiNonFleeingMemberCount;
        }
    }
    if (uiNonFleeingMemberCount) {
        fAverageMemberDamagePerSecond = fAverageMemberDamagePerSecond / static_cast<double>(uiNonFleeingMemberCount);
    }
    fTargetCombatStrength = 0.0f;
    fAverageTargetCombatStrength = 0.0f;
    uint32_t uiTargetCount = GetTargetCount();
    uint32_t uiNonFleeingTargetCount = 0;
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        float fCombatStrength = -1.0f;
        Actor* pTarget = GetTarget(i);
        if (pTarget == pPlayer) {
            fCombatStrength = pPlayer->CalculateCombatStrength(-1.0f);
        }
        else if (!pTarget->IsFleeing(false)) {
            CombatController* pCombatController = pTarget->GetCombatController();
            if (pCombatController) {
                fCombatStrength = pCombatController->pCombatState->fCombatStrength;
            }
            else {
                fCombatStrength = pTarget->CalculateCombatStrength(-1.0f);
            }
        }
        if (fCombatStrength >= 0.0) {
            fTargetCombatStrength = fTargetCombatStrength + fCombatStrength;
            ++uiNonFleeingTargetCount;
        }
    }
    if (uiNonFleeingTargetCount) {
        fAverageTargetCombatStrength = fTargetCombatStrength / static_cast<double>(uiNonFleeingTargetCount);
    }
}

double CombatGroup::GetAverageMemberDamagePerSecond(Actor* apMember) {
    if (!apMember) {
        return fAverageMemberDamagePerSecond;
    }
    float fAverageDPS = 0.0f;
    uint32_t uiMemberCount = GetMemberCount();
    uint32_t uiNonFleeingMemberCount = 0;
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        CombatMember* pCombatMember = GetCombatMember(i);
        CombatController* pCombatController = pCombatMember->pActor->GetCombatController();
        if (apMember == pCombatMember->pActor && pCombatController) {
            float fMemberDPS = pCombatController->pCombatState->fDamagePerSecond;
            if (fMemberDPS > 0.0) {
                fAverageDPS = fAverageDPS + fMemberDPS;
                ++uiNonFleeingMemberCount;
            }
        }
        else if ((!pCombatController || !pCombatController->IsFleeing()) && pCombatMember->fCombatStrength >= 0.0) {
            fAverageDPS = fAverageDPS + pCombatMember->fDamagePerSecond;
            ++uiNonFleeingMemberCount;
        }
    }
    if (uiNonFleeingMemberCount) {
        return static_cast<float>(fAverageDPS / static_cast<double>(uiNonFleeingMemberCount));
    }
    return fAverageDPS;
}

Actor* CombatGroup::FindClusterPartner(Actor* apMember, Actor* apCurrentPartner, float afMaxDistance) {
    float fPartnerMemberRadiusSquared = afMaxDistance * afMaxDistance;
    Actor* pPartner = nullptr;
    NiPoint3 memberLoc = apMember->GetLocationOnReference();
    uint32_t uiMemberCount = GetMemberCount();
    for (uint32_t i = 0; i < uiMemberCount; ++i) {
        Actor* pMember = GetMember(i);
        if (pMember != apMember) {
            NiPoint3 vec = pMember->GetLocationOnReference() - memberLoc;
            float fSquaredLength = vec.SqrLength();
            if (pMember == apCurrentPartner) {
                fSquaredLength = fSquaredLength / 2.25;
            }
            if (fPartnerMemberRadiusSquared > static_cast<double>(fSquaredLength)) {
                fPartnerMemberRadiusSquared = fSquaredLength;
                pPartner = pMember;
            }
        }
    }
    if (pPartner) {
        CombatController* pPartnerCombatController = pPartner->GetCombatController();
        if (pPartnerCombatController) {
            CombatLocationTarget* pLocationTarget = pPartnerCombatController->pCombatState->pLocationTarget;
            if (pLocationTarget && pLocationTarget->pRef == apMember) {
                return nullptr;
            }
        }
    }
    return pPartner;
}

void CombatGroup::UpdateTargets() {
    uint32_t uiTargetCount = GetTargetCount();
    if (uiTargetCount) {
        for (uint32_t i = 0; i < uiTargetCount; ++i) {
            CombatTarget* pCombatTarget = GetCombatTarget(i);
            if (CheckRemoveCombatTarget(pCombatTarget)) {
                if (uiTargetCount == 1) {
                    Actor* pMember = FindClosestMemberToActor(pCombatTarget->pActor, 1e+06f);
                    if (!pMember) {
                        pMember = FindClosestMemberToActor(pPlayer, 1e+06f);
                    }
                    if (pMember && !pCombatTarget->pActor->IsDead(false)) {
                        if (-std::numeric_limits<float>::max() == pCombatTarget->fLastDetectedTimeStamp) {
                            pCombatDialogueManager->StartDialogue(
                                pMember,
                                pCombatTarget->pActor,
                                DT_DETECTION,
                                5,
                                true,
                                static_cast<CombatController*>(nullptr)
                            );
                        }
                        else {
                            pCombatDialogueManager->StartDialogue(
                                pMember,
                                pCombatTarget->pActor,
                                DT_DETECTION,
                                8,
                                true,
                                static_cast<CombatController*>(nullptr)
                            );
                        }
                    }
                }
                RemoveTarget(i--);
                --uiTargetCount;
            }
        }
    }
}

void CombatGroup::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    char cGroupStrategyType;
    uint32_t uiSaveGameCount;
    bool bHasPathingLOSGridMap;
    uiSaveGameCount = ClusterArray.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        ClusterArray[i]->SaveGame(apSaveGameBuffer, this);
    }
    // The address &TargetArray is never null, so it is not tested.
    uiSaveGameCount = TargetArray.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (int32_t i = 0; i < uiSaveGameCount; ++i) {
        TargetArray[i].SaveGame(apSaveGameBuffer, this);
    }
    // The address &MemberArray is never null, so it is not tested.
    uiSaveGameCount = MemberArray.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        MemberArray[i].SaveGame(apSaveGameBuffer, this);
    }
    if (pGroupStrategy) {
        cGroupStrategyType = pGroupStrategy->eType;
    }
    else {
        cGroupStrategyType = -1;
    }
    apSaveGameBuffer->SaveDataEndian(&cGroupStrategyType, sizeof(cGroupStrategyType), 0);
    apSaveGameBuffer->SaveDataEndian(&bStrategyForced, sizeof(bStrategyForced), 0);
    ChooseStrategyTimer.SaveGame(apSaveGameBuffer);
    UpdateStrategyTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&iLastGroupStrategyChosenIndex, sizeof(iLastGroupStrategyChosenIndex), 0);
    fLastGroupStrategyChosenTimeStamp.SaveGame(apSaveGameBuffer);
    AvoidThreatDialogueTimer.SaveGame(apSaveGameBuffer);
    ClusterUpdateTimer.SaveGame(apSaveGameBuffer);
    TargetUpdateTimer.SaveGame(apSaveGameBuffer);
    for (CombatTimer& rTimer : DetectionDialogueTimers) {
        rTimer.SaveGame(apSaveGameBuffer);
    }
    apSaveGameBuffer->SaveDataEndian(&iSearchCount, sizeof(iSearchCount), 0);
    if (this->GetSearchCount()) {
        apSaveGameBuffer->SaveFormID(pSearchingMember, 0);
        apSaveGameBuffer->SaveDataEndian(&SearchFocalPoint, sizeof(SearchFocalPoint), 0);
        SearchUpdateTimer.SaveGame(apSaveGameBuffer);
        SearchAreaUpdateTimer.SaveGame(apSaveGameBuffer);
        fSearchStartedTimeStamp.SaveGame(apSaveGameBuffer);
        SearchCenter.SaveGame(apSaveGameBuffer);
        apSaveGameBuffer->SaveDataEndian(&fSearchRadius, sizeof(fSearchRadius), 0);
        uiSaveGameCount = SearchLocations.iSize;
        apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            CombatSearchLocation* pSearchLoc = &SearchLocations[i];
            pSearchLoc->WorldLoc.SaveGame(apSaveGameBuffer);
            apSaveGameBuffer->SaveDataEndian(&pSearchLoc->fScore, sizeof(pSearchLoc->fScore), 0);
            apSaveGameBuffer->SaveFormID(pSearchLoc->iTargetID, 0);
            pSearchLoc->fTimeStamp.SaveGame(apSaveGameBuffer);
        }
        uiSaveGameCount = SearchTeleportDoors.iSize;
        apSaveGameBuffer->SaveVariableSizedValue(uiSaveGameCount);
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            CombatSearchDoor* pSearchDoor = &SearchTeleportDoors[i];
            apSaveGameBuffer->SaveFormID(pSearchDoor->pDoor, 0);
            apSaveGameBuffer->SaveFormID(pSearchDoor->pLinkedDoor, 0);
            apSaveGameBuffer->SaveDataEndian(&pSearchDoor->cFailedAttempts, sizeof(pSearchDoor->cFailedAttempts), 0);
            apSaveGameBuffer->SaveDataEndian(&pSearchDoor->bInvestigated, sizeof(pSearchDoor->bInvestigated), 0);
            apSaveGameBuffer->SaveDataEndian(&pSearchDoor->bReserved, sizeof(pSearchDoor->bReserved), 0);
        }
        bHasPathingLOSGridMap = spPathingLOSGridMap.m_pObject != nullptr;
        apSaveGameBuffer->SaveDataEndian(&bHasPathingLOSGridMap, sizeof(bHasPathingLOSGridMap), 0);
        if (bHasPathingLOSGridMap) {
            spPathingLOSGridMap->SaveGame(apSaveGameBuffer);
        }
    }
    CombatStrengthTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&fMemberCombatStrength, sizeof(fMemberCombatStrength), 0);
    apSaveGameBuffer->SaveDataEndian(&fAverageMemberDamagePerSecond, sizeof(fAverageMemberDamagePerSecond), 0);
    apSaveGameBuffer->SaveDataEndian(&fTargetCombatStrength, sizeof(fTargetCombatStrength), 0);
    apSaveGameBuffer->SaveDataEndian(&fAverageTargetCombatStrength, sizeof(fAverageTargetCombatStrength), 0);
    CombatMusicUpdateTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&cCombatMusicState, sizeof(cCombatMusicState), 0);
}

void CombatGroup::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    bool bHasPathingLOSGridMap;
    char cGroupStrategyType;
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    ClusterArray.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        CombatGroupCluster** ppCluster = &ClusterArray[i];
        CombatGroupCluster* pCluster = new CombatGroupCluster;
        *ppCluster = pCluster;
        (*ppCluster)->LoadGame(apLoadGameBuffer, this);
    }
    uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    TargetArray.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        TargetArray[i].LoadGame(apLoadGameBuffer, this);
    }
    uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    MemberArray.SetSize(uiSaveGameCount, true);
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        MemberArray[i].LoadGame(apLoadGameBuffer, this);
    }
    cGroupStrategyType = -1;
    apLoadGameBuffer->LoadData(&cGroupStrategyType, sizeof(cGroupStrategyType));
    if (cGroupStrategyType != -1) {
        pGroupStrategy = CombatGroupStrategy::GetCombatGroupStrategy(cGroupStrategyType);
    }
    apLoadGameBuffer->LoadData(&bStrategyForced, sizeof(bStrategyForced));
    ChooseStrategyTimer.LoadGame(apLoadGameBuffer);
    UpdateStrategyTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&iLastGroupStrategyChosenIndex, sizeof(iLastGroupStrategyChosenIndex));
    fLastGroupStrategyChosenTimeStamp.LoadGame(apLoadGameBuffer);
    AvoidThreatDialogueTimer.LoadGame(apLoadGameBuffer);
    ClusterUpdateTimer.LoadGame(apLoadGameBuffer);
    TargetUpdateTimer.LoadGame(apLoadGameBuffer);
    for (CombatTimer& rTimer : DetectionDialogueTimers) {
        rTimer.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&iSearchCount, sizeof(iSearchCount));
    if (this->GetSearchCount()) {
        apLoadGameBuffer->LoadFormID(pSearchingMember);
        apLoadGameBuffer->LoadData(&SearchFocalPoint, sizeof(SearchFocalPoint));
        SearchUpdateTimer.LoadGame(apLoadGameBuffer);
        SearchAreaUpdateTimer.LoadGame(apLoadGameBuffer);
        if (apLoadGameBuffer->GetVersion() >= 5u) {
            fSearchStartedTimeStamp.LoadGame(apLoadGameBuffer);
        }
        SearchCenter.LoadGame(apLoadGameBuffer);
        apLoadGameBuffer->LoadData(&fSearchRadius, sizeof(fSearchRadius));
        uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        SearchLocations.SetSize(uiSaveGameCount, true);
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            CombatSearchLocation* pSearchLoc = &SearchLocations[i];
            pSearchLoc->WorldLoc.LoadGame(apLoadGameBuffer);
            apLoadGameBuffer->LoadData(&pSearchLoc->fScore, sizeof(pSearchLoc->fScore));
            apLoadGameBuffer->LoadFormID(pSearchLoc->iTargetID);
            pSearchLoc->fTimeStamp.LoadGame(apLoadGameBuffer);
        }
        uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        SearchTeleportDoors.SetSize(uiSaveGameCount, true);
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            CombatSearchDoor* pSearchDoor = &SearchTeleportDoors[i];
            uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            pSearchDoor->pDoor = dynamic_cast<TESObjectREFR*>(pForm);
            uiFormID = apLoadGameBuffer->LoadFormID();
            pForm = TESForm::GetFormByNumericID(uiFormID);
            pSearchDoor->pLinkedDoor = dynamic_cast<TESObjectREFR*>(pForm);
            apLoadGameBuffer->LoadData(&pSearchDoor->cFailedAttempts, sizeof(pSearchDoor->cFailedAttempts));
            apLoadGameBuffer->LoadData(&pSearchDoor->bInvestigated, sizeof(pSearchDoor->bInvestigated));
            apLoadGameBuffer->LoadData(&pSearchDoor->bReserved, sizeof(pSearchDoor->bReserved));
        }
        bHasPathingLOSGridMap = false;
        apLoadGameBuffer->LoadData(&bHasPathingLOSGridMap, sizeof(bHasPathingLOSGridMap));
        if (bHasPathingLOSGridMap) {
            spPathingLOSGridMap = new PathingLOSGridMap;
            spPathingLOSGridMap->LoadGame(apLoadGameBuffer);
        }
        bUpdateSearchDebugGeometry = true;
    }
    CombatStrengthTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fMemberCombatStrength, sizeof(fMemberCombatStrength));
    apLoadGameBuffer->LoadData(&fAverageMemberDamagePerSecond, sizeof(fAverageMemberDamagePerSecond));
    apLoadGameBuffer->LoadData(&fTargetCombatStrength, sizeof(fTargetCombatStrength));
    apLoadGameBuffer->LoadData(&fAverageTargetCombatStrength, sizeof(fAverageTargetCombatStrength));
    CombatMusicUpdateTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&cCombatMusicState, sizeof(cCombatMusicState));
}

void CombatGroup::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t uiSaveGameCount = TargetArray.iSize;
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        bool bRemove = false;
        CombatTarget* pCombatTarget = &TargetArray[i];
        pCombatTarget->InitLoadGame(apLoadGameBuffer, this);
        if (pCombatTarget->pActor) {
            pCombatManager->TargetLocalActor(pCombatTarget->pActor, this);
        }
        else {
            bRemove = true;
        }
        if (bRemove) {
            TargetArray.RemoveFast(i--, 1u);
            --uiSaveGameCount;
        }
    }
    uiSaveGameCount = MemberArray.iSize;
    for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
        CombatMember* pCombatMember = &MemberArray[i];
        pCombatMember->InitLoadGame(apLoadGameBuffer, this);
        if (!pCombatMember->pActor) {
            MemberArray.RemoveFast(i--, 1u);
            --uiSaveGameCount;
        }
    }
    if (IsSearching()) {
        Actor* pMember = nullptr;
        if (pSearchingMember) {
            TESForm* pForm = TESForm::GetFormByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSearchingMember))
            );
            pMember = dynamic_cast<Actor*>(pForm);
        }
        pSearchingMember = pMember;
    }
    UpdateCounts();
}

bool CombatGroup::GetTargetLastDetectedLocation(Actor* apTarget, BGSWorldLocation& arLastDetectedLocation) {
    CombatTarget* pCombatTarget = GetCombatTargetForActor(apTarget);
    if (!pCombatTarget || !pCombatTarget->LastDetectedLocation.pSpace) {
        return false;
    }
    arLastDetectedLocation = pCombatTarget->LastDetectedLocation;
    return true;
}

void CombatGroup::CreateDebugGeometry(eBool abClear) {
    if (spDebugGeometry.m_pObject && spDebugGeometry->m_pkParent) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiNode* pNode = spDebugGeometry.m_pObject;
            TaskQueueInterface::GetInstance()->QueueNodeDetach(pNode);
        }
        else {
            NiNode* pParent = spDebugGeometry->m_pkParent;
            pParent->DetachChild(spDebugGeometry.m_pObject);
        }
    }

    spDebugGeometry = static_cast<NiNode*>(nullptr);
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
        for (CombatGroupCluster* pCluster : ClusterArray) {
            if (!spDebugGeometry.m_pObject) {
                spDebugGeometry = new NiNode(0);
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
                    NiAVObject*
                        pArrow = BSTestObjects::Create3DArrow(kStart, kCenter, 4.0f, 4.0f, &kStartColor, &kEndColor);
                    spDebugGeometry->AttachChild(pArrow, 1);
                    bHasMembers = true;
                }
            }
            if (bHasMembers) {
                for (uint32_t k = 0; k < 5; ++k) {
                    NiAVObject* pCircle = BSTestObjects::CreateCircle(fClusterDistance, 36, &kClusterColor);
                    float fZ = static_cast<double>(k) * 25.0 + kCenter.z;
                    pCircle->SetTranslate(kCenter.x, kCenter.y, fZ);
                    spDebugGeometry->AttachChild(pCircle, 1);
                }
                NiAVObject*
                    pCylinder = BSTestObjects::CreateCappedCylinder(16.0f, 16.0f, 400.0f, 10, 10, &kClusterColor);
                pCylinder->SetTranslate(kCenter);
                spDebugGeometry->AttachChild(pCylinder, 1);
            }
        }
    }
    if (bDebugCombatGroups2.Bool()) {
        if (!spDebugGeometry.m_pObject) {
            spDebugGeometry = new NiNode(0);
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
                spDebugGeometry->AttachChild(pCylinder, 1);
            }
        }
    }
    NiAVObject* pSearchGeometry = nullptr;
    if (bDebugCombatSearch.Bool() && pDebugActor) {
        if (!spDebugGeometry.m_pObject) {
            spDebugGeometry = new NiNode(0);
        }
        uint32_t uiNumSearchLocations = GetNumSearchLocations();
        for (uint32_t i = 0; i < uiNumSearchLocations; ++i) {
            CombatSearchLocation* pSearchLocation = &GetSearchLocation(i);
            if (pSearchLocation->WorldLoc.pSpace == pSpace) {
                NiColorA kColor;
                pSearchLocation->GetColor(&kColor);
                NiAVObject* pSphere = BSTestObjects::CreateSphere(32.0f, 10, 10, &kColor);
                pSphere->SetTranslate(pSearchLocation->WorldLoc.Point);
                spDebugGeometry->AttachChild(pSphere, 1);
            }
        }
        for (const CombatSearchDoor& rDoor : SearchTeleportDoors) {
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
            spDebugGeometry->AttachChild(pCylinder, 1);
        }
        if (spPathingLOSGridMap.m_pObject && bUpdateSearchDebugGeometry && pDebugActor->GetSpace() == pSpace) {
            NiPointer<PathingLOSMap> spLOSMap = new PathingLOSMap(
                spPathingLOSGridMap->GetLocation(),
                spPathingLOSGridMap->fRadius
            );
            spPathingLOSGridMap->SetLOSMapFromSeenTriangles(spLOSMap);
            spSearchDebugGeometry = static_cast<NiRefObject*>(spLOSMap->CreateDebugGeometry());
            bUpdateSearchDebugGeometry = false;
        }
        pSearchGeometry = static_cast<NiAVObject*>(spSearchDebugGeometry.m_pObject);
    }
    if (spDebugGeometry.m_pObject) {
        NiAlphaProperty* pAlpha = new NiAlphaProperty;
        pAlpha->SetAlphaBlending(true);
        spDebugGeometry->AttachProperty(pAlpha);
        AddNoLightingPropertyRecurse(spDebugGeometry.m_pObject);
        BSShaderManager::PrepareObject(spDebugGeometry.m_pObject, 0, 0);
        if (pSearchGeometry) {
            spDebugGeometry->AttachChild(pSearchGeometry, 1);
        }
        NiNode* pMarkerNode = pPlayer->GetParentCell()->GetMarkerNode();
        pMarkerNode->AttachChild(spDebugGeometry.m_pObject, 1);
        spDebugGeometry->UpdateProperties();
        NiUpdateData kUpdateData(0.0f, false, false);
        spDebugGeometry->Update(kUpdateData);
    }
}

CombatGroupStrategy* CombatGroup::SelectGroupStrategy(CombatGroup* apGroup) {
    alignas(CombatGroupData) uint8_t kData[sizeof(CombatGroupData)];
    if (bDisableCombatGroupStrategies.Bool()) {
        return nullptr;
    }
    ::new (&reinterpret_cast<CombatGroupData&>(kData)) CombatGroupData(apGroup);
    CombatGroupStrategy* pCurrStrategy = apGroup->pGroupStrategy;
    if (static_cast<int32_t>(reinterpret_cast<CombatGroupData&>(kData).iMemberCount) >= 2) {
        for (uint32_t i = 0; i < 4; ++i) {
            CombatGroupStrategy* pGroupStrategy = CombatGroupStrategy::GetCombatGroupStrategy(i);
            if (pGroupStrategy) {
                if (pGroupStrategy == pCurrStrategy) {
                    reinterpret_cast<CombatGroupData&>(kData).~CombatGroupData();
                    return pCurrStrategy;
                }
                if (pGroupStrategy->CanExecute(apGroup, &reinterpret_cast<CombatGroupData&>(kData))) {
                    apGroup->ClearGroupStrategyAssignments();
                    pGroupStrategy->Execute(apGroup, &reinterpret_cast<CombatGroupData&>(kData));
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
