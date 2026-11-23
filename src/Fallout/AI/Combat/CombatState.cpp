#include "Fallout/AI/Combat/CombatState.h"

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TimeGlobal.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FindObjectIterator.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/InventoryIterator.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestCover.h"
#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatCoverSearch.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatGroupStrategy.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatProcedureBeInCover.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/Combat/UnreachableCoverLocation.h"
#include "Fallout/AI/Combat/WeaponData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <cmath>
#include <limits>
#include <new>

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> iMaxHiPerfPCTargetCount("iMaxHiPerfPCTargetCount:Combat", 4);
SettingT<INISettingCollection> iMaxHiPerfNPCTargetCount("iMaxHiPerfNPCTargetCount:Combat", 4);
SettingT<INISettingCollection> fHiPerfPCTargetLOSTimer("fHiPerfPCTargetLOSTimer:Combat", 0.25f);
SettingT<INISettingCollection> fLowPerfPCTargetLOSTimer("fLowPerfPCTargetLOSTimer:Combat", 0.5f);
SettingT<INISettingCollection> fHiPerfNPCTargetLOSTimer("fHiPerfNPCTargetLOSTimer:Combat", 0.5f);
SettingT<INISettingCollection> fLowPerfNPCTargetLOSTimer("fLowPerfNPCTargetLOSTimer:Combat", 1.0f);
BSPerformanceTimer CombatAI_CoverSearch("Combat AI", "Cover Search", true, true);
BSPerformanceTimer CombatAIAdvanceSearch("Combat AI", "Advance Search", true, true);
BSPerformanceTimer CombatAI_CombatItemSearch("Combat AI", "Combat Item Search", true, true);

double
CalculateCostForDistance(CombatController* apCombatController, BGSWorldLocation* apFrom, BGSWorldLocation* apTo) {
    float fCost = apFrom->GetDistanceSquared(*apTo) / 589824.0;
    float fClampedCost = BSMIN(fCost, 1e+06f);
    return BSMAX(fClampedCost, 0.01f);
}

CombatLocationTarget::CombatLocationTarget()
  : cType(0), bNearTarget(false), pRef(nullptr), fMinDistance(0.0f), fMaxDistance(0.0f) {}

void CombatLocationTarget::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&cType, sizeof(cType), 0);
    apSaveGameBuffer->SaveDataEndian(&bNearTarget, sizeof(bNearTarget), 0);
    apSaveGameBuffer->SaveDataEndian(&fMinDistance, sizeof(fMinDistance), 0);
    apSaveGameBuffer->SaveDataEndian(&fMaxDistance, sizeof(fMaxDistance), 0);
    apSaveGameBuffer->SaveFormID(pRef, 0);
    WorldLoc.SaveGame(apSaveGameBuffer);
}

// The load buffer is unused. The field's raw bytes are handed to GetFormByNumericID as they stand, then the resolved
// reference is stored back over them.
void CombatLocationTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pLoadedRef = nullptr;
    if (pRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRef));
        pLoadedRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pRef = pLoadedRef;
}

void CombatLocationTarget::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&cType, sizeof(cType));
    apLoadGameBuffer->LoadData(&bNearTarget, sizeof(bNearTarget));
    apLoadGameBuffer->LoadData(&fMinDistance, sizeof(fMinDistance));
    apLoadGameBuffer->LoadData(&fMaxDistance, sizeof(fMaxDistance));
    apLoadGameBuffer->LoadFormID(pRef);
    WorldLoc.LoadGame(apLoadGameBuffer);
}

CombatState::TargetedObject::TargetedObject() : pRef(nullptr), pObject(nullptr) {}

CombatState::CombatState(CombatController* apCombatController, float afTimerDelay)
  : bBlocking(false), iLocationUnreachable(0), fThreatRatio(100.0f), pCombatWeapons{}, pCurrentEmbeddedWeapon(nullptr),
    iHasCombatWeapon(0), fCombatWeaponsDPS{}, fMaxCombatWeaponDPS(0.0f), fMaxRangedWeaponDPS(0.0f),
    fMaxModifiedCombatWeaponDPS(0.0f), fDisarmedDPS(0.0f), fBetterWeaponOutOfRangeDistance(0.0f),
    iTargetDetectionLevel(-1), bTargetLineOfSight(false), bTarget360LineOfSight(false), bTargetBlocked(false),
    cTargetFlushed(-1), bTargetSpaceDifferent(false), bHasHandToHandEquip(false), bGrenadeThrowHighArc(false),
    bInventoryDirty(false), pExplosiveProjectileBlocked(nullptr), eTargetSegmentInView(ACTOR_SEGMENT_INVIEW_NONE),
    fThreatScore(0.0f), LastSeenTargetLocation(NiPoint3::ZERO), iCombatPermissions(0),
    fLastTargetDetectedTimeStamp(-std::numeric_limits<float>::max()),
    fLastTarget360LineOfSightTimeStamp(-std::numeric_limits<float>::max()),
    fLastAttackTimeStamp(-std::numeric_limits<float>::max()),
    fLastDamagedByAttackerTimeStamp(-std::numeric_limits<float>::max()),
    fFiringStartedTimeStamp(-std::numeric_limits<float>::max()), pLocationTarget(nullptr), cStartingConfidence(0),
    cCurrentConfidence(0), fCombatStrength(-1.0f), fDamagePerSecond(-1.0f), cFleeState(0), iLastFleeDoor(0),
    fLastFleeDoorTimeStamp(-std::numeric_limits<float>::max()), pCoverLocation(nullptr),
    pSecondaryCoverLocation(nullptr), cLastDodgeDirection(-1), pCombatLocation(nullptr), pCombatItems{},
    pUsableCombatItems{}, iHasCombatItem(0), pLastTarget(nullptr), pCombatController(apCombatController),
    bInitializing(true), spPathCheckQueue(nullptr), iQueuedPathCheck(-1) {
    ThreatRatioUpdateTimer.Reset(RandomFloat(0.0f, fCombatThreatRatioUpdateTime.Float()));
    fConfidenceModifier = RandomFloat(fCombatConfidenceModifierMin.Float(), fCombatConfidenceModifierMax.Float());
}

CombatState::~CombatState() {
    DeleteInventoryData();
    ClearCoverLocation(false);
    ClearSecondaryCoverLocation(false);
    ClearCombatLocation();
    ClearLocationTarget();
    for (uint32_t i = 0; i < 5; ++i) {
        RemoveTargetedObject(i);
    }
}

bool CombatState::FindCombatLocation(int32_t aiLocation) {
    BGSWorldLocation attackerLoc;
    BGSWorldLocation worldLoc;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    CombatGroup* pgroup = pCombatController->pCombatGroup;
    CombatPlannerData* pplannerData = pCombatController->pCombatPlannerData;
    if (pplannerData->GetLocation(aiLocation)) {
        pplannerData->SetUnk4Ptr(aiLocation);
        return true;
    }

    if (std::numeric_limits<float>::max() == pplannerData->GetCost(aiLocation)) {
        return false;
    }

    bool bfound = false;
    CombatLocation* pfoundCombatLocation = new CombatLocation;
    CombatCoverLocation* pfoundCoverLocation = new CombatCoverLocation;
    pfoundCombatLocation->iLocation = aiLocation;
    pfoundCombatLocation->SetRadius(kCombatLocationTypeData[aiLocation].fRadius);
    if (pCombatLocation && pCombatLocation->iLocation == aiLocation) {
        pfoundCombatLocation->Copy(pCombatLocation);
        if (pCoverLocation && pCoverLocation->iLocation == aiLocation) {
            pfoundCoverLocation->Copy(pCoverLocation);
        }

        if (!kCombatLocationTypeData[aiLocation].bRecheckLocation) {
            pfoundCombatLocation->UpdateRefLocation();
            bfound = true;
        }
    }

    bool bignoreCombatRadius = false;
    if (aiLocation == HIDE_LOCATION || aiLocation == FLEE_LOCATION || aiLocation == INVESTIGATE_LOCATION) {
        bignoreCombatRadius = true;
    }
    if (!bfound) {
        switch (aiLocation) {
            case 0:
            case 1: {
                if (ptarget) {
                    if (GetHasBufferedDetection()) {
                        if (!GetTargetUnreachable()) {
                            pfoundCombatLocation->SetRef(ptarget);
                            bfound = true;
                        }
                    }
                    else {
                        BGSWorldLocation lastDetectedLocation;
                        if (pgroup->GetTargetLastDetectedLocation(ptarget, lastDetectedLocation)) {
                            pfoundCombatLocation->SetLocation(lastDetectedLocation);
                            bfound = true;
                        }
                    }
                }
                break;
            }
            case 2:
            case 4:
            case 5: {
                bfound = FindBestCoverLocation(
                    pCombatController->GetCurrentWeapon(),
                    &CoverSearchInfo,
                    pfoundCoverLocation,
                    pfoundCombatLocation,
                    aiLocation
                );
                break;
            }
            case 3: {
                bfound = FindBestDodgeLocation(pfoundCombatLocation, 3u, 15u, 128.0f, 128.0f);
                break;
            }
            case 6: {
                if (!pLocationTarget || pLocationTarget->cType) {
                    if (pCombatController->GetTarget()) {
                        Actor* pcurrentTarget = pCombatController->GetTarget();
                        NiPoint3 advanceLocation = pcurrentTarget->GetLocationOnReference();
                        bfound = FindBestAdvanceLocation(
                            &CoverSearchInfo,
                            pfoundCoverLocation,
                            pfoundCombatLocation,
                            aiLocation,
                            advanceLocation,
                            0.0f,
                            nullptr
                        );
                    }
                }
                else {
                    bfound = FindBestAdvanceLocation(
                        &CoverSearchInfo,
                        pfoundCoverLocation,
                        pfoundCombatLocation,
                        aiLocation,
                        pLocationTarget->WorldLoc.Point,
                        0.0f,
                        nullptr
                    );
                }
                break;
            }
            case 7:
            case 8:
            case 9:
            case 16: {
                if (pLocationTarget && (aiLocation != 8 || !GetLocationUnreachable(256))) {
                    if (pLocationTarget->pRef) {
                        pfoundCombatLocation->SetRef(pLocationTarget->pRef);
                        if (pLocationTarget->pRef->IsActor()) {
                            Actor* pactor = static_cast<Actor*>(pLocationTarget->pRef);
                            double dtargetRadius = pactor->GetRadius();
                            float fradius = pattacker->GetRadius() + dtargetRadius;
                            pfoundCombatLocation->SetRadius(fradius);
                        }
                    }
                    else {
                        pfoundCombatLocation->SetLocation(pLocationTarget->WorldLoc);
                    }
                    pfoundCombatLocation->SetRadius(pLocationTarget->fMinDistance);
                    bfound = true;
                }

                break;
            }
            case 10:
            case 12: {
                bfound = FindBestFleeLocation(pfoundCoverLocation, pfoundCombatLocation, aiLocation);
                break;
            }
            case 11: {
                bfound = FindGrenadeThrowLocation(pfoundCombatLocation, bGrenadeThrowHighArc, pCombatWeapons[3]);
                break;
            }
            case 13: {
                bfound = FindLOSLocation(pfoundCombatLocation, aiLocation);
                break;
            }
            case 14: {
                pfoundCombatLocation->SetFlag(CombatLocation::FLAG_EVADE, true);
                bfound = true;
                break;
            }
            case 15: {
                CombatSearchLocation* psearchLocation = pCombatController->pCombatGroup->FindSearchLocation(
                    pattacker,
                    50,
                    &pfoundCombatLocation->WorldLoc
                );
                if (psearchLocation) {
                    pfoundCombatLocation->SetLocation(psearchLocation->WorldLoc);
                    bfound = true;
                }
                else if (pfoundCombatLocation->GetPoint() != NiPoint3::ZERO) {
                    bfound = true;
                }
                if (bfound) {
                    pfoundCombatLocation->SetFlag(CombatLocation::FLAG_NEGATIVE_RADIUS, true);
                }
                break;
            }
            case 17:
            case 18:
            case 19:
            case 20:
            case 21: {
                CombatState::TargetedObject* pcombatItem = GetTargetedObject(static_cast<uint32_t>(aiLocation - 17));
                if (pcombatItem && pcombatItem->pRef) {
                    pfoundCombatLocation->SetRef(static_cast<TESObjectREFR*>(pcombatItem->pRef));
                    double dmodelBoundSize = static_cast<TESObjectREFR*>(pcombatItem->pRef)
                                                 ->TESObjectREFR::GetModelBoundSize();
                    double dreach = pattacker->GetForwardLength() + dmodelBoundSize;
                    float fpickupRadius = fPickupItemDistanceFudge.Float() + dreach;
                    pfoundCombatLocation->SetRadius(fpickupRadius);
                    pfoundCombatLocation->SetFlag(CombatLocation::FLAG_NEGATIVE_RADIUS, true);
                    bfound = true;
                }

                break;
            }
            default: {
                Warning(
                    "COMBAT: Invalid combat location type %i passed to CombatState::FindCombatLocation()",
                    aiLocation
                );
                break;
            }
        }
    }
    if (bfound) {
        if (!pfoundCombatLocation->GetSpace()) {
            TESForm* pspace = pattacker->GetSpace();
            pfoundCombatLocation->SetSpace(pspace);
        }
        worldLoc = pfoundCombatLocation->WorldLoc;
        if (worldLoc.Point == NiPoint3::ZERO || CheckMovement(worldLoc, true, bignoreCombatRadius)) {
            float fcost = 0.0f;
            if (kCombatLocationTypeData[aiLocation].bUsesMovementCost) {
                pattacker->GetWorldLocation(&attackerLoc);
                fcost = CalculateCostForDistance(pCombatController, &attackerLoc, &worldLoc);
            }
            pplannerData->SetCombatLocation(aiLocation, pfoundCombatLocation, fcost);
            if (pfoundCoverLocation->iCoverLocationKey) {
                pplannerData->SetCombatCoverLocation(aiLocation, pfoundCoverLocation);
            }
            else if (pfoundCoverLocation) {
                delete pfoundCoverLocation;
            }
            return true;
        }
    }

    ourFree(pfoundCombatLocation);
    if (pfoundCoverLocation) {
        delete pfoundCoverLocation;
    }
    pplannerData->SetCombatLocation(aiLocation, nullptr, std::numeric_limits<float>::max());
    return false;
}

// Tries the requested directions in order (bit 1: the side the cross product gives, 2: the other side, 4: towards the
// target, 8: away from it); a point counts when it has no threat, the attacker's navmesh line reaches it (or, when cut,
// the cut point is far enough to shorten it), the line of sight to the target holds where asked, and the attacker can
// move there. The last direction is kept for 10 seconds while it is still a candidate, otherwise a random candidate is
// taken.
bool CombatState::FindBestDodgeLocation(
    CombatLocation* apRetLocation,
    uint32_t aiDodgeDirections,
    uint32_t aiDodgeDirectionLOSCheck,
    float afDodgeDistance,
    float afMinDodgeDistance
) {
    Actor* pAttacker = pCombatController->GetAttacker();
    Actor* pTarget = pCombatController->GetTarget();
    float fRadius = pAttacker->GetRadius() + 8.0;
    BGSWorldLocation kWorldLocation;
    pAttacker->GetWorldLocation(&kWorldLocation);
    NiPoint3 kAttackerPos = *kWorldLocation.GetPoint();
    NiPoint3 kTargetPos = pTarget->GetLocationOnReference();
    PathingLocation kAttackerLocation(pAttacker);
    NiPoint3 kAway = kAttackerPos - kTargetPos;
    kAway.Unitize();
    NiPoint3 kSide = kAway.Cross(NiPoint3::UNIT_Z);
    NiPoint3 akPoints[4];
    uint32_t uiCount = 0;
    // Per direction the index of its point, -1 for none; per point its direction.
    int8_t acPointIndex[4];
    int8_t acDirection[4];
    for (uint32_t i = 0; i < 4; ++i) {
        acPointIndex[i] = -1;
        uint32_t uiDirection = 1 << i;
        if (!(aiDodgeDirections & uiDirection)) {
            continue;
        }
        NiPoint3 kOffset;
        switch (uiDirection) {
            case 1: kOffset = kSide; break;
            case 2: kOffset = -kSide; break;
            case 4: kOffset = -kAway; break;
            case 8: kOffset = kAway; break;
            default: break;
        }
        NiPoint3 kPoint = kAttackerPos + kOffset * afDodgeDistance;
        NiPoint3 kEdge = kPoint + kOffset * fRadius;
        CombatTimeStamp kSkipOlderThreats(0.0f);
        // Only a positive score skips the direction; a score that is less, equal or unordered (NaN) goes on.
        if (pCombatManager->GetThreatMap()->ComputeThreatScore(kPoint, pAttacker, 0.0f, 0.0f, kSkipOlderThreats, 0.0f) >
            0.0) {
            continue;
        }
        PathingLocation kCutLocation;
        if (!Pathing::CheckLineOfSight(kAttackerLocation, kEdge, &kCutLocation, nullptr)) {
            if (!(afDodgeDistance > afMinDodgeDistance)) {
                continue;
            }
            // Measures the horizontal distance of the NiPoint3 difference, and compares the minimum with the unrounded
            // difference.
            float fCutDistance = (kAttackerPos - kCutLocation.GetLocation()).XYLength();
            if (afMinDodgeDistance > static_cast<double>(fCutDistance) - fRadius) {
                continue;
            }
            float fDistance = fCutDistance - fRadius;
            kPoint = kAttackerPos + kOffset * fDistance;
        }
        if ((aiDodgeDirectionLOSCheck & uiDirection) &&
            !CombatUtilities::CalculateLOS(pAttacker, pTarget, kPoint, kTargetPos, false, -1)) {
            continue;
        }
        if (!CheckMovement(kPoint, kWorldLocation.GetSpace(), true, false)) {
            continue;
        }
        akPoints[uiCount] = kPoint;
        acPointIndex[i] = static_cast<int8_t>(uiCount);
        acDirection[uiCount] = static_cast<int8_t>(i);
        ++uiCount;
    }
    if (!uiCount) {
        return false;
    }
    // A new direction is picked when the elapsed time is >= 10 seconds or unordered: the last direction stays only
    // under 10 seconds.
    if (cLastDodgeDirection != -1 && fLastDodgeSearch.ElapsedTime() < 10.0 && acPointIndex[cLastDodgeDirection] != -1) {
        apRetLocation->SetPoint(akPoints[acPointIndex[cLastDodgeDirection]], nullptr);
        return true;
    }
    uint32_t uiPick = RandomUnsignedInt(uiCount);
    cLastDodgeDirection = acDirection[uiPick];
    CombatTimeStamp kNow(CombatTimer::GetTimer());
    fLastDodgeSearch = kNow;
    apRetLocation->SetPoint(akPoints[acPointIndex[cLastDodgeDirection]], nullptr);
    return true;
}

void CombatState::ReserveCoverLocation(CombatCoverLocation* apCoverLocation) {
    if (apCoverLocation && !apCoverLocation->IsReserved()) {
        pCombatManager->GetCoverMap()->ReserveCoverLocation(pCombatController, apCoverLocation);
    }
}

void CombatState::UnreserveSecondaryCoverLocation() {
    UnreserveCoverLocation(pSecondaryCoverLocation);
}

void CombatState::UnreserveCoverLocation(CombatCoverLocation* apCoverLocation) {
    if (apCoverLocation && apCoverLocation->IsReserved()) {
        pCombatManager->GetCoverMap()->UnreserveCoverLocation(pCombatController, apCoverLocation);
    }
}

void CombatState::UpdateThreatRatio() {
    BGSWorldLocation attackerLoc;
    Actor* pattacker = pCombatController->pAttacker;
    if (cFleeState != 1) {
        fThreatRatio = 100.0f;
        if (fLastDamagedByAttackerTimeStamp.ElapsedTime() < 120.0 &&
            pattacker->GetActorFloatValue(ActorValue::AGGRESSION) != 3.0) {
            CombatGroup* pgroup = pCombatController->pCombatGroup;
            float fmemberCombatStrength = pgroup->GetMemberCombatStrength(pattacker);
            float ftargetCombatStrength = pgroup->fTargetCombatStrength;
            if (fmemberCombatStrength >= 0.0 && ftargetCombatStrength > 0.0) {
                fThreatRatio = fmemberCombatStrength / ftargetCombatStrength;
                if (fThreatRatio > 0.0) {
                    TESCombatStyle* pcombatStyle = pCombatController->GetCurrentCombatStyle();
                    if (pcombatStyle->IsFleeBasedOnPersonalSurvival()) {
                        float faverageMemberDPS = pgroup->GetAverageMemberDamagePerSecond(pattacker);
                        float faverageTargetCombatStrength = pgroup->fAverageTargetCombatStrength;
                        if (faverageMemberDPS >= 0.0 && faverageTargetCombatStrength > 0.0) {
                            float fpersonalCombatStrength = pattacker->CalculateCombatStrength(faverageMemberDPS);
                            float ftempThreatRatio = fpersonalCombatStrength / faverageTargetCombatStrength;
                            if (fThreatRatio > static_cast<double>(ftempThreatRatio)) {
                                fThreatRatio = ftempThreatRatio;
                            }
                        }
                    }
                }
            }
            if (cFleeState == 2 && cCurrentConfidence < 4u) {
                float fradius = fCombatFleeBoostConfidenceTargetRadius.Float();
                BGSWorldLocation* pworldLocation = pattacker->GetWorldLocation(&attackerLoc);
                if (pgroup->FindTargetNearLocation(*pworldLocation, fradius, true)) {
                    BoostConfidence(true);
                }
            }
        }
    }
}

bool CombatState::CheckTargetsCoverLOS(PathingLocation& arCoverPathingLocation, Actor* apIgnoreTarget) {
    CombatGroup* pgroup = pCombatController->pCombatGroup;
    uint32_t itargetCount = pgroup->GetTargetCount();
    for (uint32_t i = 0; i < itargetCount; ++i) {
        Actor* ptarget = pgroup->GetTarget(i);
        if (ptarget == apIgnoreTarget) {
            continue;
        }
        BGSWorldLocation targetWorldLoc;
        NiPoint3 targetLocation;
        if (pgroup->GetTargetLastSeenLocation(i, targetWorldLoc, 3.0f)) {
            targetLocation = *targetWorldLoc.GetPoint();
        }
        else {
            targetLocation = ptarget->GetLocationOnReference();
        }
        if (CheckCoverLOS(arCoverPathingLocation, targetLocation, false)) {
            return true;
        }
    }
    return false;
}

bool CombatState::IsCoverLocationUnreachable(uint32_t aiCoverKey) {
    for (int32_t i = 0; i < UnreachableCoverLocations.iSize; ++i) {
        UnreachableCoverLocation* unreachableCoverLocation = &UnreachableCoverLocations[static_cast<uint32_t>(i)];
        if (!(unreachableCoverLocation->fTimeStamp.ElapsedTime() > 10.0)) {
            if (unreachableCoverLocation->iCoverKey == aiCoverKey) {
                return true;
            }
        }
        else {
            UnreachableCoverLocations.RemoveFast(i--, 1u);
        }
    }
    return false;
}

// areCoverState gets COVER_STATE_NO_EDGES_OPEN (closed stand cover), COVER_STATE_COVER_LOS (not between the target and
// the cover, or no cover from it), COVER_STATE_UNREACHABLE or, after the side loop, the larger of the last two reasons
// a side failed. The PathingLocation locals are destroyed by explicit Destruct calls.
bool CombatState::CheckCoverLocation(
    PathingCoverLocation& arCoverLocation,
    CombatLocation* apRetLocation,
    CoverReservationInfo* apReservationInfo,
    NiPoint3& arCoverSearchTargetLocation,
    uint32_t aiCoverKey,
    uint32_t aiCurrentCoverLocationKey,
    int32_t aiLocation,
    Actor* apAttacker,
    Actor* apTarget,
    float afMinRange,
    float afMaxRange,
    COVER_STATE& areCoverState,
    float& arfSquaredDistance,
    bool abHideCover
) {
    Actor* pCombatant = pCombatController->GetAttacker();
    float fTargetSearchRadius = 256.0f;
    if (arCoverLocation.IsStandCover() &&
        !abHideCover &&
        !arCoverLocation.bLeftEdgeOpen &&
        !arCoverLocation.bRightEdgeOpen) {
        areCoverState = COVER_STATE_NO_EDGES_OPEN;
        return false;
    }
    if (aiCoverKey == aiCurrentCoverLocationKey) {
        const NiPoint3* pTargetLocation = &apTarget->GetLocationOnReference();
        NiPoint3 kLocation = arCoverLocation.GetLocation();
        if (!NavMeshMath::Find2DIntersection3(
                kLocation,
                *pTargetLocation,
                arCoverLocation.LeftVertex,
                arCoverLocation.RightVertex
            )) {
            areCoverState = COVER_STATE_COVER_LOS;
            return false;
        }
    }
    if (IsCoverLocationUnreachable(aiCoverKey)) {
        areCoverState = COVER_STATE_UNREACHABLE;
        return false;
    }
    COVER_STATE eState = COVER_STATE_NONE;
    COVER_STATE ePreviousState = COVER_STATE_NONE;
    arfSquaredDistance = 0.0f;
    float fTravelTime = -1.0f;
    float fRadius = pCombatant->GetRadius();
    uint32_t uiClosestSide = 1;
    if (apReservationInfo->cSegmentCount) {
        if (apReservationInfo->bLeftEdge) {
            uiClosestSide = 0;
        }
    }
    else if (arCoverLocation.IsLeftSideClosest(apAttacker->GetLocationOnReference())) {
        uiClosestSide = 0;
    }
    bool bBothSides = arCoverLocation.IsStandCover() && !abHideCover;
    bool bCheckFirePosition = (arCoverLocation.IsStandCover() || arCoverLocation.IsLedgeCover()) && !abHideCover;
    uint32_t uiSides = (bBothSides ? 1u : 0u) + 1;
    for (uint32_t i = 0; i < uiSides; ++i) {
        if (eState) {
            ePreviousState = eState;
        }
        bool bLeftEdge = false;
        if (bBothSides) {
            if (i == uiClosestSide && arCoverLocation.bLeftEdgeOpen) {
                bLeftEdge = true;
            }
            else if (i != uiClosestSide && arCoverLocation.bRightEdgeOpen) {
                bLeftEdge = false;
            }
            else {
                continue;
            }
        }
        if (!pCombatManager->GetCoverMap()->CanReserveCoverLocation(
                aiCoverKey,
                &arCoverLocation,
                pCombatController,
                fRadius,
                apReservationInfo,
                abHideCover,
                bLeftEdge
            )) {
            eState = COVER_STATE_CANT_RESERVE;
            continue;
        }
        NiPoint3 kCoverPosition;
        NiPoint3 kFirePosition;
        CombatUtilities::CalculateCoverPositions(
            pCombatController,
            &arCoverLocation,
            apReservationInfo,
            &kCoverPosition,
            &kFirePosition
        );
        if (arCoverLocation.IsLedgeCover() &&
            apTarget &&
            !CheckLedgeCoverAngle(arCoverLocation, kCoverPosition, arCoverSearchTargetLocation, apTarget)) {
            eState = COVER_STATE_LEDGE_ANGLE;
            continue;
        }
        PathingLocation kCoverLocation(kCoverPosition, arCoverLocation);
        PathingLocation* pCoverLocation = &kCoverLocation;
        if (!pCoverLocation->IsValidLocation(pCombatant->GetRadius())) {
            eState = COVER_STATE_COVER_INVALID;
            continue;
        }
        if (bCheckFirePosition) {
            PathingLocation kFireLocation(kFirePosition, arCoverLocation);
            PathingLocation* pFireLocation = &kFireLocation;
            if (!pFireLocation->IsValidLocation(pCombatant->GetRadius())) {
                eState = COVER_STATE_FIRE_INVALID;
                continue;
            }
        }
        if (!CheckMovement(kCoverPosition, static_cast<TESObjectCELL*>(apAttacker->GetSpace()), true, false)) {
            eState = COVER_STATE_CHECK_MOVEMENT_FAILED;
            continue;
        }
        arfSquaredDistance = (arCoverSearchTargetLocation - kCoverPosition).SqrLength();
        // An unordered (NaN) distance passes both range checks.
        if (arfSquaredDistance > afMaxRange * afMaxRange) {
            eState = COVER_STATE_TOO_FAR;
            continue;
        }
        if (arfSquaredDistance < afMinRange * afMinRange) {
            eState = COVER_STATE_TOO_CLOSE;
            continue;
        }
        BGSWorldLocation kWorldLocation;
        kWorldLocation.SetLocation(kCoverPosition, static_cast<TESObjectCELL*>(apAttacker->GetSpace()));
        if (pCombatController->GetCombatGroup()->FindTargetNearLocation(kWorldLocation, fTargetSearchRadius, false)) {
            eState = COVER_STATE_HOSTILES_NEAR;
            continue;
        }
        if (fTravelTime == -1.0) {
            NiPoint3 kToCover = apAttacker->GetLocationOnReference() - kCoverPosition;
            double dDistance = kToCover.Length();
            fTravelTime = static_cast<float>(dDistance / apAttacker->GetCurrentSpeed());
        }
        CombatTimeStamp kSkipOlderThreats(0.0f);
        if (pCombatManager->GetThreatMap()
                ->ComputeThreatScore(kCoverPosition, apAttacker, fTravelTime, 0.0f, kSkipOlderThreats, 0.0f) > 0.0) {
            eState = COVER_STATE_COVER_THREATENED;
            continue;
        }
        if (bCheckFirePosition) {
            CombatTimeStamp kFireSkipOlderThreats(0.0f);
            if (pCombatManager->GetThreatMap()
                    ->ComputeThreatScore(kFirePosition, apAttacker, fTravelTime, 0.0f, kFireSkipOlderThreats, 0.0f) >
                0.0) {
                eState = COVER_STATE_FIRE_THREATENED;
                continue;
            }
        }
        bool bSeen = CheckCoverLOS(*pCoverLocation, arCoverSearchTargetLocation, true);
        if (!bSeen) {
            bSeen = CheckTargetsCoverLOS(*pCoverLocation, apTarget);
        }
        if (bSeen) {
            eState = COVER_STATE_COVER_LOS;
            continue;
        }
        bool bCheckFireLOS = true;
        // 5: the cover search's location type that needs no line of sight.
        if (aiLocation == 5) {
            bCheckFireLOS = false;
        }
        else if (abHideCover) {
            bCheckFireLOS = false;
        }
        else if (aiCoverKey == aiCurrentCoverLocationKey) {
            bCheckFireLOS = false;
        }
        if (bCheckFireLOS &&
            !CombatUtilities::CalculateLOS(
                apAttacker,
                apTarget,
                kFirePosition,
                arCoverSearchTargetLocation,
                true,
                -1
            )) {
            eState = COVER_STATE_FIRE_LOS;
            continue;
        }
        apRetLocation->SetPoint(kCoverPosition, nullptr);
        return true;
    }
    if (ePreviousState > eState) {
        areCoverState = ePreviousState;
    }
    else {
        areCoverState = eState;
    }
    return false;
}

bool CombatState::GetHasBuffered360LineOfSight() {
    double elapsedTime = fLastTarget360LineOfSightTimeStamp.ElapsedTime();
    return fCombatLOSBufferTime.Float() > elapsedTime;
}

CombatCoverLocation* CombatState::GetSecondaryCoverLocation() {
    return pSecondaryCoverLocation;
}

void CombatState::DoInitialUpdate() {
    Actor* pattacker = pCombatController->pAttacker;
    bHasHandToHandEquip = CombatController::CheckHasEquip(pattacker, nullptr);
    UpdateInventoryData();
    Update();
    cStartingConfidence = static_cast<uint8_t>(pattacker->GetActorValue(ActorValue::CONFIDENCE));
    cCurrentConfidence = cStartingConfidence;
    bInitializing = false;
}

void CombatState::TargetChanged() {
    Actor* ptarget = pCombatController->GetTarget();
    if (pCoverLocation && ptarget) {
        NiPoint3 coverLoc = pCoverLocation->CoverLocation.GetLocation();
        uint8_t bcoverSafe = CheckCoverSafe(pCoverLocation->CoverLocation, coverLoc, ptarget, 0);
        if (!bcoverSafe || !GetHasTarget360LineOfSight()) {
            pCoverLocation->bDirtyFlag = true;
        }
    }

    if (pCombatLocation && static_cast<uint32_t>(pCombatLocation->iLocation) <= 1u) {
        ClearCombatLocation();
    }

    ClearTargetUnreachable();
    UpdateInventoryData();
}

// The float elapsed time is compared with the double 120.0; an unordered compare keeps the location.
bool CombatState::GetLastSeenTargetLocation(NiPoint3& arLastSeenLocation) {
    if (!pCombatController->GetTarget() || fLastTarget360LineOfSightTimeStamp.ElapsedTime() > 120.0) {
        return false;
    }
    arLastSeenLocation = LastSeenTargetLocation;
    return true;
}

void CombatState::SetLastFleeDoorTimeStamp(float afTime) {
    CombatTimeStamp kTimeStamp(afTime);
    fLastFleeDoorTimeStamp = kTimeStamp;
}

void CombatState::ClearCoverLocation(bool abInvalidateCover) {
    if (pCoverLocation) {
        if (abInvalidateCover) {
            AddUnreachableCoverLocation(pCoverLocation->iCoverLocationKey);
        }
        if (pCoverLocation->bReserved) {
            UnreserveCoverLocation(pCoverLocation);
        }
        if (pCoverLocation) {
            delete pCoverLocation;
        }
        pCoverLocation = nullptr;
    }
}

void CombatState::DamagedByAttacker(Actor* apAttacker, float afDamage) {
    CombatTimer kTimeStamp;
    if (fLastDamagedByAttackerTimeStamp.ElapsedTime() > 1.0) {
        UpdateCombatArea(true);
    }
    ::new (&kTimeStamp) CombatTimeStamp(CombatTimer::GetTimer());
    fLastDamagedByAttackerTimeStamp.fTimeStamp = kTimeStamp.fStartTime;
    if (cFleeState == 2) {
        BoostConfidence(true);
    }
}

// Raises the disarmed DPS to the best combat weapon's and restarts the better-weapon search timer, then refreshes
// the inventory data.
void CombatState::Disarmed() {
    if (fMaxCombatWeaponDPS > fDisarmedDPS) {
        fDisarmedDPS = fMaxCombatWeaponDPS;
        FindBetterWeaponTimer.Reset(fCombatDisarmedFindBetterWeaponInitialTime.Float());
    }
    UpdateInventoryData();
}

// Rebuilds the combat weapon and item data from the attacker's inventory: the best healing and stealth items, each
// usable weapon's DPS and range against the target (weapons without usable ammo dropped, the held weapon reloaded when
// its ammo ran out), the best weapon per combat type, the embedded weapons in range, and whether the best explosive
// would hit the attacker; then restarts the inventory update timer. A held weapon that cannot be unequipped is the only
// weapon considered.
void CombatState::UpdateInventoryData() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_COMBAT_AI,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatState.cpp",
        838
    );
    Actor* pActor = pCombatController->GetAttacker();
    Actor* pTarget = pCombatController->GetTarget();
    TESCombatStyle* pCombatStyle = pCombatController->GetCurrentCombatStyle();
    float fTargetDistanceSquared = 0.0f;
    BGSWorldLocation kActorLocation;
    BGSWorldLocation kTargetLocation;
    if (pTarget) {
        pActor->GetWorldLocation(&kActorLocation);
        pTarget->GetWorldLocation(&kTargetLocation);
        fTargetDistanceSquared = kActorLocation.GetDistanceSquared(kTargetLocation);
    }
    DeleteInventoryData();
    bInventoryDirty = false;
    fMaxCombatWeaponDPS = 0.0f;
    fMaxRangedWeaponDPS = 0.0f;
    fMaxModifiedCombatWeaponDPS = 0.0f;
    fBetterWeaponOutOfRangeDistance = 0.0f;
    iHasCombatWeapon = 0;
    iHasCombatItem = 0;
    ourMemset(fCombatWeaponsDPS, 0, sizeof(fCombatWeaponsDPS));
    ourMemset(pUsableCombatItems, 0, sizeof(pUsableCombatItems));
    float fTotalEffects[2];
    ourMemset(fTotalEffects, 0, sizeof(fTotalEffects));
    alignas(BSSimpleArray<WeaponData>) uint8_t aucWeaponData[sizeof(BSSimpleArray<WeaponData>)];
    BSSimpleArray<WeaponData>& kWeaponDataArray = *::new (aucWeaponData) BSSimpleArray<WeaponData>(32, 0);
    alignas(BSSimpleArray<TESAmmo*>) uint8_t aucAmmo[sizeof(BSSimpleArray<TESAmmo*>)];
    BSSimpleArray<TESAmmo*>& kAmmoArray = *::new (aucAmmo) BSSimpleArray<TESAmmo*>(32, 0);
    ItemChange* pItem = nullptr;
    alignas(InventoryIterator) uint8_t aucIter[sizeof(InventoryIterator)];
    InventoryIterator* pIter = ::new (aucIter) InventoryIterator;
    InventoryChanges* pInventoryChanges = InventoryChanges::GetInventoryChanges(pActor);
    while (pInventoryChanges->GetNextItem(*pIter, pItem, 0)) {
        TESBoundObject* pItemObject = pItem->GetItemChangeObject();
        switch (pItemObject->GetFormType()) {
            case ALCH_ID: {
                AlchemyItem* pAlchItem = static_cast<AlchemyItem*>(pItemObject);
                // The best healing item (bit 17) and, unless the style forbids it, the best stealth item (bit 18); an
                // item only fills the first slot it beats.
                for (uint32_t i = 0; i < 2; ++i) {
                    float fTotalEffect = 0.0f;
                    uint32_t uiItemBit = i + 17;
                    switch (uiItemBit) {
                        case 17:
                            fTotalEffect = static_cast<EffectItemList*>(pAlchItem)->GetEffectScore(
                                EffectArchetypes::VALUE_MODIFIER,
                                MagicSystem::SELF,
                                ActorValue::HEALTH
                            );
                            fTotalEffect = static_cast<EffectItemList*>(pAlchItem)->GetEffectScore(
                                               EffectArchetypes::VALUE_AND_PARTS,
                                               MagicSystem::SELF,
                                               ActorValue::HEALTH
                                           ) +
                                fTotalEffect;
                            break;
                        case 18:
                            if (!pCombatStyle->IsFlagSet(TESCombatStyle::CANNOT_USE_STEALTHBOY)) {
                                fTotalEffect = static_cast<EffectItemList*>(pAlchItem)->GetEffectScore(
                                    EffectArchetypes::CHAMELEON,
                                    MagicSystem::SELF,
                                    ActorValue::ACTOR_VALUE_COUNT
                                );
                            }
                            break;
                    }
                    if (fTotalEffects[i] < fTotalEffect) {
                        pUsableCombatItems[i] = pItemObject;
                        fTotalEffects[i] = fTotalEffect;
                        iHasCombatItem |= 1 << uiItemBit;
                        break;
                    }
                }
                break;
            }
            case WEAP_ID: {
                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pItemObject);
                if (pActor->CanUseWeapon(pWeapon, true)) {
                    uint32_t uiIndex = kWeaponDataArray.AddUninitialized();
                    ourMemset(&kWeaponDataArray.GetAt(uiIndex), 0, sizeof(WeaponData));
                    kWeaponDataArray.GetAt(uiIndex).pWeapon = pWeapon;
                    // The quotient is kept as a double until the store.
                    double dCondition = pItem->GetItemHealth(true) / 100.0;
                    kWeaponDataArray.GetAt(uiIndex).fCondition = static_cast<float>(dCondition);
                }
                break;
            }
            case AMMO_ID: {
                TESAmmo* pAmmo = static_cast<TESAmmo*>(pItemObject);
                kAmmoArray.Add(pAmmo);
                break;
            }
        }
    }
    TESBoundObject* pCurrentWeapon = nullptr;
    ItemChange* pCurrentWeaponItem = pActor->GetCurrentAIProcess()->GetCurrentWeapon();
    if (pCurrentWeaponItem &&
        pCurrentWeaponItem->GetExtraDataList() &&
        pCurrentWeaponItem->GetExtraDataList()->GetItem() &&
        pCurrentWeaponItem->GetExtraDataList()->GetItem()->GetCanNotWear()) {
        pCurrentWeapon = pCurrentWeaponItem->GetItemChangeObject();
    }
    if (!pCurrentWeapon) {
        float fLargestDPS = 0.0f;
        TESObjectWEAP* pLargestDPSWeapon = nullptr;
        float fLargestDistanceDPS = 0.0f;
        for (uint32_t k = 0; k < kWeaponDataArray.GetSize(); ++k) {
            WeaponData& rWeaponData = kWeaponDataArray[k];
            TESObjectWEAP* pWeapon = rWeaponData.pWeapon;
            Actor* pHolder = (pCurrentWeaponItem && pWeapon == pCurrentWeaponItem->GetItemChangeObject()) ? pActor
                                                                                                          : nullptr;
            TESAmmo* pAmmo = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(pHolder));
            if (pAmmo && !kAmmoArray.IsInArray(pAmmo)) {
                bool bNeedsAmmo = false;
                TESAmmo* pBestAmmo = static_cast<TESAmmo*>(
                    pInventoryChanges->GetBestAmmoForWeapon(pWeapon, bNeedsAmmo)
                );
                if (!pBestAmmo || !kAmmoArray.IsInArray(pBestAmmo)) {
                    kWeaponDataArray.RemoveFast(k, 1);
                    --k;
                    continue;
                }
                // The held weapon's ammo ran out but other ammo is in the inventory: switch to it and reload.
                if (pCurrentWeaponItem && pWeapon == pCurrentWeaponItem->GetItemChangeObject()) {
                    Animation* pAnimation = pActor->GetAnimation();
                    bool bReloading = pActor->IsReloading() || (pAnimation && pAnimation->ReloadAnimQueued());
                    if (!bReloading) {
                        BaseProcess* pProcess = pActor->GetCurrentAIProcess();
                        int32_t iCount = pInventoryChanges->GetObjectCount(pBestAmmo);
                        ItemChange* pNewAmmo = new ItemChange(pBestAmmo, iCount);
                        pProcess->SetCurrentAmmo(pNewAmmo);
                        pProcess->GetCurrentAmmo()->SetCount(0);
                        pActor->QueueReload();
                    }
                }
            }
            else if (pCurrentWeaponItem && pWeapon == pCurrentWeaponItem->GetItemChangeObject()) {
                // The held weapon's loaded ammo is used up: reload unless a reload is under way.
                ItemChange* pCurrentAmmo = pActor->GetCurrentAIProcess()->GetCurrentAmmo();
                if (pCurrentAmmo) {
                    Animation* pAnimation = pActor->GetAnimation();
                    TESObjectWEAP* pHeldWeapon = static_cast<TESObjectWEAP*>(pCurrentWeaponItem->GetItemChangeObject());
                    if (pAnimation && pHeldWeapon) {
                        bool bOutOfAmmo = pCurrentAmmo &&
                            pCurrentAmmo->GetCount() <= 0 &&
                            pHeldWeapon &&
                            pHeldWeapon->GetAmmoPerShot() > 0;
                        bool bReloading = pActor->IsReloading() || (pAnimation && pAnimation->ReloadAnimQueued());
                        if (bOutOfAmmo && !bReloading) {
                            pActor->QueueReload();
                        }
                    }
                }
            }
            rWeaponData.fDamagePerSecond = CombatFormulas::CalcCombatWeaponDPS(
                pActor,
                pWeapon,
                rWeaponData.fCondition,
                pCombatStyle,
                nullptr,
                nullptr
            );
            if (rWeaponData.fDamagePerSecond <= 0.0) {
                kWeaponDataArray.RemoveFast(k, 1);
                --k;
                continue;
            }
            rWeaponData.bInRange = true;
            rWeaponData.fAbsoluteMaxRange = 0.0f;
            COMBAT_WEAPON_TYPE eType = rWeaponData.pWeapon->GetCombatWeaponType();
            if (pTarget && (eType == COMBAT_WEAPON_RANGED || eType == COMBAT_WEAPON_RANGED_EXPLOSIVE)) {
                float fMinRange = 0.0f;
                float fMaxRange = 0.0f;
                float fAbsoluteMaxRange = 0.0f;
                CombatUtilities::ComputeOptimalWeaponRanges(
                    pCombatController,
                    pWeapon,
                    fMinRange,
                    fMaxRange,
                    fAbsoluteMaxRange
                );
                rWeaponData.fAbsoluteMaxRange = fAbsoluteMaxRange;
                if (pCombatController->GetCurrentWeapon() == pWeapon) {
                    fAbsoluteMaxRange = fAbsoluteMaxRange * fCombatCurrentWeaponAbsoluteMaxRangeMult.Float();
                }
                // Compares against the unrounded square; a NaN counts as in range.
                if (fTargetDistanceSquared > static_cast<double>(fAbsoluteMaxRange) * fAbsoluteMaxRange) {
                    rWeaponData.bInRange = false;
                    if (fLargestDPS < rWeaponData.fDamagePerSecond) {
                        fLargestDPS = rWeaponData.fDamagePerSecond;
                        pLargestDPSWeapon = pWeapon;
                    }
                }
                else if (fLargestDistanceDPS < rWeaponData.fDamagePerSecond) {
                    fLargestDistanceDPS = rWeaponData.fDamagePerSecond;
                }
            }
        }
        // With a weapon in range, the best out-of-range one is not preferred.
        if (fLargestDistanceDPS > 0.0) {
            pLargestDPSWeapon = nullptr;
        }
        for (WeaponData& rWeaponData : kWeaponDataArray) {
            TESObjectWEAP* pWeapon = rWeaponData.pWeapon;
            COMBAT_WEAPON_TYPE eType = rWeaponData.pWeapon->GetCombatWeaponType();
            float fDamagePerSecond = rWeaponData.fDamagePerSecond;
            if (pWeapon == pCurrentEmbeddedWeapon) {
                fDamagePerSecond = static_cast<float>(fDamagePerSecond + 1000000.0);
            }
            if (pWeapon == pLargestDPSWeapon) {
                rWeaponData.bInRange = true;
            }
            if (!rWeaponData.bInRange) {
                fDamagePerSecond = static_cast<float>(fDamagePerSecond / 10000.0);
                if (fBetterWeaponOutOfRangeDistance < rWeaponData.fAbsoluteMaxRange &&
                    fLargestDistanceDPS < rWeaponData.fDamagePerSecond) {
                    fBetterWeaponOutOfRangeDistance = rWeaponData.fAbsoluteMaxRange;
                }
            }
            if (pWeapon->IsEmbeddedWeapon() && rWeaponData.bInRange) {
                EmbeddedWeaponsArray.Add(pWeapon);
            }
            fMaxCombatWeaponDPS = BSMAX(fMaxCombatWeaponDPS, rWeaponData.fDamagePerSecond);
            if (eType == COMBAT_WEAPON_RANGED || eType == COMBAT_WEAPON_RANGED_EXPLOSIVE) {
                fMaxRangedWeaponDPS = BSMAX(fMaxRangedWeaponDPS, rWeaponData.fDamagePerSecond);
            }
            fMaxModifiedCombatWeaponDPS = BSMAX(fMaxModifiedCombatWeaponDPS, fDamagePerSecond);
            iHasCombatWeapon |= 1 << eType;
            if (fCombatWeaponsDPS[eType] < fDamagePerSecond) {
                pCombatWeapons[eType] = pWeapon;
                fCombatWeaponsDPS[eType] = fDamagePerSecond;
            }
        }
        // The DPS without a weapon, in the entry after the weapon types.
        fCombatWeaponsDPS[COMBAT_WEAPON_TYPE_COUNT] = CombatFormulas::CalcCombatWeaponDPS(
            pActor,
            nullptr,
            1.0f,
            pCombatStyle,
            nullptr,
            nullptr
        );
        fMaxCombatWeaponDPS = BSMAX(fMaxCombatWeaponDPS, fCombatWeaponsDPS[COMBAT_WEAPON_TYPE_COUNT]);
        fMaxModifiedCombatWeaponDPS = BSMAX(fMaxModifiedCombatWeaponDPS, fCombatWeaponsDPS[COMBAT_WEAPON_TYPE_COUNT]);
        iHasCombatWeapon |= 1 << COMBAT_WEAPON_TYPE_COUNT;
    }
    else {
        // A held weapon that cannot be unequipped is the only one.
        COMBAT_WEAPON_TYPE eType = static_cast<TESObjectWEAP*>(pCurrentWeapon)->GetCombatWeaponType();
        pCombatWeapons[eType] = static_cast<TESObjectWEAP*>(pCurrentWeapon);
        fCombatWeaponsDPS[eType] = 9999999.0f;
        iHasCombatWeapon |= 1 << eType;
        fMaxCombatWeaponDPS = fCombatWeaponsDPS[eType];
        fMaxModifiedCombatWeaponDPS = fCombatWeaponsDPS[eType];
        if (eType == COMBAT_WEAPON_RANGED || eType == COMBAT_WEAPON_RANGED_EXPLOSIVE) {
            fMaxRangedWeaponDPS = fCombatWeaponsDPS[eType];
        }
    }
    if (pTarget && pCombatWeapons[COMBAT_WEAPON_RANGED_EXPLOSIVE]) {
        BGSExplosion*
            pExplosion = pCombatWeapons[COMBAT_WEAPON_RANGED_EXPLOSIVE]->GetProjectile(pActor)->GetExplosion();
        if (!pCombatManager->CheckExplosionAttack(pActor, pExplosion, pTarget->GetLocationOnReference(), 2.0f)) {
            SetExplosiveProjectileBlocked(pExplosion);
        }
    }
    UpdateInventoryTimer.Reset(fCombatInventoryUpdateTimer.Float());
    pIter->~InventoryIterator();
    kAmmoArray.~BSSimpleArray<TESAmmo*>();
    kWeaponDataArray.~BSSimpleArray<WeaponData>();
    pContext->~AutoMemContext();
}

void CombatState::UpdateLow() {
    Actor* pattacker = pCombatController->GetAttacker();
    Actor* ptarget = pCombatController->GetTarget();
    if (ptarget) {
        CombatGroup* pcombatGroup = pCombatController->GetCombatGroup();
        pcombatGroup->GetTargetDetectionLevel(
            pattacker,
            ptarget,
            iTargetDetectionLevel,
            fLastTargetDetectedTimeStamp,
            bTargetLineOfSight,
            bTarget360LineOfSight
        );
    }
    if (bInventoryDirty) {
        UpdateInventoryData();
    }
    if (CombatStrengthUpdateTimer.Check()) {
        UpdateCombatStrength();
        CombatStrengthUpdateTimer.Reset(fCombatStrengthUpdateTime.Float());
    }
    if (ThreatRatioUpdateTimer.Check()) {
        UpdateThreatRatio();
        ThreatRatioUpdateTimer.Reset(fCombatThreatRatioUpdateTime.Float());
    }
}

void CombatState::UpdatePeriodicData() {
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    int32_t inumEmbeddedWeapons = EmbeddedWeaponsArray.iSize;
    if (inumEmbeddedWeapons > 1) {
        bool bswitchingWeapons = pCombatController->IsSwitchingWeapons();
        if (!EmbeddedWeaponUpdateTimer.IsSet() && !bswitchingWeapons) {
            EmbeddedWeaponUpdateTimer.Reset(fEmbeddedWeaponSwitchTime.Float());
        }
        if ((EmbeddedWeaponUpdateTimer.IsSet() && EmbeddedWeaponUpdateTimer.Check()) || bInitializing) {
            bool bforceSwitch = EmbeddedWeaponUpdateTimer.IsForced();
            if (RandomBool(fEmbeddedWeaponSwitchChance.Float()) || bInitializing || bforceSwitch) {
                TESObjectWEAP* pnewEmbeddedWeapon = nullptr;
                if (bforceSwitch && pCurrentEmbeddedWeapon) {
                    int32_t iembeddedWeaponIndex = RandomInt(0, inumEmbeddedWeapons - 1);
                    for (TESObjectWEAP* pEmbeddedWeapon : EmbeddedWeaponsArray) {
                        if (pEmbeddedWeapon != pCurrentEmbeddedWeapon) {
                            if (!iembeddedWeaponIndex) {
                                pnewEmbeddedWeapon = pEmbeddedWeapon;
                                break;
                            }
                            --iembeddedWeaponIndex;
                        }
                    }
                }
                else {
                    uint32_t uirandomIndex = RandomInt(0, inumEmbeddedWeapons);
                    pnewEmbeddedWeapon = EmbeddedWeaponsArray[uirandomIndex];
                }
                if (pnewEmbeddedWeapon != pCurrentEmbeddedWeapon) {
                    pCurrentEmbeddedWeapon = pnewEmbeddedWeapon;
                    UpdateInventoryData();
                    pCombatController->ForcePlanUpdate();
                }
            }
            EmbeddedWeaponUpdateTimer.Clear();
        }
    }
    if (pExplosiveProjectileBlocked) {
        if (!ExplosiveProjectileBlockedTimer.IsSet()) {
            ExplosiveProjectileBlockedTimer.Reset(fExplosiveProjectileBlockedResetTime.Float());
        }
        if (ExplosiveProjectileBlockedTimer.Check()) {
            const NiPoint3* ptargetLocation;
            if (ptarget &&
                (ptargetLocation = &ptarget->GetLocationOnReference(),
                 !pCombatManager
                      ->CheckExplosionAttack(pattacker, pExplosiveProjectileBlocked, *ptargetLocation, 2.0f))) {
                ExplosiveProjectileBlockedTimer.Reset(fExplosiveProjectileBlockedResetTime.Float());
            }
            else {
                pExplosiveProjectileBlocked = nullptr;
            }
        }
    }
    else {
        ExplosiveProjectileBlockedTimer.Clear();
    }
    if (FindBetterWeaponTimer.Check()) {
        FindBetterWeapon();
        if (!(fMaxCombatWeaponDPS > static_cast<double>(fDisarmedDPS))) {
            FindBetterWeaponTimer.Reset(fCombatDisarmedFindBetterWeaponTime.Float());
        }
        else {
            FindBetterWeaponTimer.Reset(fCombatFindBetterWeaponTime.Float());
            fDisarmedDPS = 0.0f;
        }
    }
    if (CombatStrengthUpdateTimer.Check()) {
        UpdateCombatStrength();
        CombatStrengthUpdateTimer.Reset(fCombatStrengthUpdateTime.Float());
    }
    if (ThreatRatioUpdateTimer.Check()) {
        UpdateThreatRatio();
        ThreatRatioUpdateTimer.Reset(fCombatThreatRatioUpdateTime.Float());
    }
    if (spPathCheckQueue.m_pObject) {
        ActorPathingMessage kmessage;
        if (spPathCheckQueue->TryPop(&kmessage)) {
            if (kmessage.msgType) {
                if (iQueuedPathCheck == 1 && GetTargetUnreachable() && ptarget) {
                    if (!QueuePathCheck(0)) {
                        iQueuedPathCheck = -1;
                    }
                }
                else {
                    iQueuedPathCheck = -1;
                }
            }
            else {
                ClearTargetUnreachable();
                iQueuedPathCheck = -1;
            }
            if (iQueuedPathCheck == -1) {
                spPathCheckQueue = static_cast<ActorPathingMessageQueue*>(nullptr);
            }
        }
    }
    else if (GetTargetUnreachable() && ptarget) {
        if (!TargetUnreachableTimer.IsSet()) {
            TargetUnreachableTimer.Reset(5.0f);
        }
        if (TargetUnreachableTimer.Check()) {
            bool bqueued = false;
            if (GetLocationUnreachable(2)) {
                bqueued = QueuePathCheck(1);
            }
            if (!bqueued) {
                QueuePathCheck(0);
            }
            TargetUnreachableTimer.Reset(5.0f);
        }
    }
    else {
        TargetUnreachableTimer.Clear();
    }
    UpdateCombatPermissions();
    if (pCombatController->IsFleeing()) {
        if (!pCombatController->IsPlanUpdateForced() && !cFleeState) {
            cFleeState = 1;
            pattacker->SetWantWeaponDrawn(false);
            pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, 3, true);
        }
    }
    else {
        bool bresetFleeState = true;
        if (pCombatController->pCurrentGoal &&
            pCombatController->pCurrentGoal->eType == CombatGoal::COMBAT_GOAL_GUARD) {
            bresetFleeState = !CheckShouldFlee();
        }

        if (bresetFleeState) {
            cFleeState = 0;
        }
    }
}

void CombatState::UpdateImmediateData() {
    BGSWorldLocation refWorldLoc;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation targetWorldLoc;
    BGSWorldLocation kAttackerLocation;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    TESForm* pattackerSpace = pattacker->GetSpace();
    if (!ptarget || pattackerSpace == ptarget->GetSpace()) {
        bTargetSpaceDifferent = false;
    }
    else if (!bTargetSpaceDifferent) {
        if (bTargetLineOfSight) {
            pCombatController->ForcePlanUpdate();
        }
        bTargetSpaceDifferent = true;
    }
    if (ptarget) {
        if (pCombatController->GetUpdatedDetection() || pLastTarget != ptarget) {
            int32_t inewDetectionLevel = -100;
            pCombatController->pCombatGroup->GetTargetDetectionLevel(
                pattacker,
                ptarget,
                inewDetectionLevel,
                fLastTargetDetectedTimeStamp,
                bTargetLineOfSight,
                bTarget360LineOfSight
            );
            iTargetDetectionLevel = inewDetectionLevel;
            pCombatController->SetUpdatedDetection(false);
        }
        if (bTarget360LineOfSight) {
            CombatTimeStamp kNow(CombatTimer::GetTimer());
            fLastTarget360LineOfSightTimeStamp = kNow;
            LastSeenTargetLocation = ptarget->GetLocationOnReference();
            CombatGoal* pCombatGoal = pCombatController->pCurrentGoal;
            if (!pCombatGoal || pCombatGoal->eType != CombatGoal::COMBAT_GOAL_FLUSH_TARGET) {
                cTargetFlushed = -1;
                bGrenadeThrowHighArc = false;
            }
        }
        if (bTarget360LineOfSight && !bTargetBlocked && pCombatController->CombatPlanContainsAction(24)) {
            pCombatController->ForcePlanUpdate();
        }
        if (!GetHasBuffered360LineOfSight() && cTargetFlushed == -1) {
            cTargetFlushed = 0;
        }
    }
    else {
        iTargetDetectionLevel = -1;
        bTarget360LineOfSight = false;
        CombatTimeStamp detectedTimeStamp(-std::numeric_limits<float>::max());
        fLastTargetDetectedTimeStamp = detectedTimeStamp;
        CombatTimeStamp lineOfSightTimeStamp(-std::numeric_limits<float>::max());
        fLastTarget360LineOfSightTimeStamp = lineOfSightTimeStamp;
        cTargetFlushed = -1;
    }
    CheckReferences();
    if (DynamicCoverLostTimer.IsSet()) {
        if (DynamicCoverLostTimer.Check()) {
            ClearCoverLocation(false);
            DynamicCoverLostTimer.Clear();
        }
    }
    else if (
        pCoverLocation &&
        pCoverLocation->iCoverLocationLastMovedTimeStamp != -1 &&
        !pCoverLocation->CoverLocation.CheckDynamicCoverValid(pCoverLocation->iCoverLocationLastMovedTimeStamp)
    ) {
        DynamicCoverLostTimer.Reset(2.0f);
    }
    if (CombatGroup::bUseClustering) {
        if (!pLocationTarget || pLocationTarget->cType == 2) {
            float fminDistance = CombatGroup::fStaticMinDistance;
            float fmaxDistance = CombatGroup::fstaticMaxDistance;
            float fgatherDistance = CombatGroup::fstaticGatherDistance;
            if (CombatUtilities::IsActorUsingMelee(pattacker)) {
                fminDistance = fminDistance + fminDistance;
                fmaxDistance = fmaxDistance + fmaxDistance;
            }
            if (fgatherDistance < fmaxDistance + 256.0) {
                fgatherDistance = fmaxDistance + 256.0;
            }
            if (CombatGroup::bUseSimpleCombatGroupClustering) {
                Actor* pcurrentPartner = nullptr;
                if (pLocationTarget && pLocationTarget->pRef && pLocationTarget->pRef->IsActor()) {
                    pcurrentPartner = static_cast<Actor*>(pLocationTarget->pRef);
                }

                Actor* ppartner = pCombatController->pCombatGroup
                                      ->FindClusterPartner(pattacker, pcurrentPartner, fgatherDistance);
                if (ppartner) {
                    SetLocationTargetRef(ppartner, LOCATION_TARGET_CLUSTER, fminDistance, fmaxDistance);
                }
                else {
                    ClearLocationTarget();
                }
            }
            else {
                CombatGroupCluster* pcluster = pCombatController->pCombatGroup->GetClusterForMember(pattacker);
                if (pcluster) {
                    SetLocationTargetLocation(&pcluster->Center, 2, fminDistance, fmaxDistance);
                }
                else {
                    ClearLocationTarget();
                }
            }
        }
    }
    else if (pLocationTarget && pLocationTarget->cType == 2) {
        ClearLocationTarget();
    }
    if (pLocationTarget) {
        if (pLocationTarget->cType == 3) {
            if (pLocationTarget->WorldLoc.pSpace != pattackerSpace) {
                UpdateCombatArea(true);
            }
        }
        if (pLocationTarget->pRef) {
            pLocationTarget->WorldLoc = *pLocationTarget->pRef->GetWorldLocation(&refWorldLoc);
        }
        NiPoint3 coordMult(1.0f, 1.0f, 0.25f);
        BGSWorldLocation* pattackerLocation = pattacker->GetWorldLocation(&kAttackerLocation);
        float fsquaredDistance = pLocationTarget->WorldLoc.GetModifiedDistanceSquared(*pattackerLocation, coordMult);
        bool bignoreLocationTarget = false;
        if ((!pLocationTarget->cType || pLocationTarget->cType == 1) &&
            ptarget &&
            pCombatController->GetPretendInCombat() &&
            !pCombatController->GetContinueDuringCombat()) {
            bignoreLocationTarget = true;
        }

        if (bignoreLocationTarget) {
            pLocationTarget->bNearTarget = true;
        }
        else if (
            pLocationTarget->fMinDistance < static_cast<double>(pLocationTarget->fMaxDistance) &&
            CheckUseLocationTargetMaxDistance()
        ) {
            float fnearTargetDistance = GetLocationTargetNearTargetDistance();
            if (pLocationTarget->bNearTarget) {
                if (fsquaredDistance > fnearTargetDistance * fnearTargetDistance) {
                    pLocationTarget->bNearTarget = false;
                }
            }
            else if (fsquaredDistance < fnearTargetDistance * fnearTargetDistance) {
                pLocationTarget->bNearTarget = true;
            }
        }
        else {
            pLocationTarget->bNearTarget = false;
            if (fsquaredDistance < pLocationTarget->fMinDistance * pLocationTarget->fMinDistance) {
                pLocationTarget->bNearTarget = true;
            }
        }
        char cType = pLocationTarget->cType;
        if (cType) {
            if (cType == 1 &&
                pCombatController->GetAttackerCanSneak() &&
                pLocationTarget->pRef &&
                pLocationTarget->pRef->IsActor()) {
                if (!(fsquaredDistance <
                      fCombatFollowSneakFollowRadius.Float() * fCombatFollowSneakFollowRadius.Float())) {
                    if (pCombatController->GetForceSneak()) {
                        pCombatController->SetForceSneak(false);
                    }
                }
                else {
                    Actor* pguardActor = static_cast<Actor*>(pLocationTarget->pRef);
                    bool bguardSneaking = pguardActor->IsSneaking();
                    if (bguardSneaking == pattacker->IsSneaking()) {
                        GuardSneakTimer.Clear();
                    }
                    else if (GuardSneakTimer.IsSet()) {
                        if (GuardSneakTimer.Check()) {
                            pCombatController->SetForceSneak(bguardSneaking);
                            GuardSneakTimer.Clear();
                        }
                    }
                    else {
                        float fsneakDelay = pguardActor == pPlayer ? RandomFloat(0.5f, 1.0f) : RandomFloat(0.1f, 0.5f);
                        GuardSneakTimer.Reset(fsneakDelay);
                    }
                }
            }
        }
        else if (pLocationTarget->bNearTarget && !bignoreLocationTarget) {
            ClearLocationTarget();
            pCombatController->FinishedPackageTask();
        }
    }
    float flastThreatScore = fThreatScore;
    TESCombatStyle* pcombatStyle = pCombatController->GetCurrentCombatStyle();
    if (!pcombatStyle->GetIgnoreThreats()) {
        if (fCombatThreatSignificantScore.Float() < static_cast<double>(fThreatScore) ||
            (iCombatPermissions & 256) != 0) {
            CombatTimeStamp threatsConsideredTimeStamp;
            threatsConsideredTimeStamp.fTimeStamp = fThreatsConsideredTimeStamp.fTimeStamp;
            NiPoint3 attackerLocation = pattacker->GetLocationOnReference();
            fThreatScore = pCombatManager->pThreatMap->ComputeThreatScore(
                attackerLocation,
                pattacker,
                0.0f,
                0.0f,
                threatsConsideredTimeStamp,
                0.0f
            );
            if (fCombatThreatSignificantScore.Float() <= static_cast<double>(fThreatScore)) {
                if (fCombatThreatSignificantScore.Float() > static_cast<double>(flastThreatScore)) {
                    pCombatController->ForcePlanUpdate();
                }
            }
        }
        else {
            fThreatScore = 0.0f;
        }
    }
    if (-std::numeric_limits<float>::max() != fFiringStartedTimeStamp.fTimeStamp && !pattacker->IsAttacking()) {
        CombatTimeStamp firingStartedTimeStamp(-std::numeric_limits<float>::max());
        fFiringStartedTimeStamp = firingStartedTimeStamp;
    }
    if (ptarget) {
        if (fBetterWeaponOutOfRangeDistance > 0.0) {
            pattacker->GetWorldLocation(&attackerWorldLoc);
            ptarget->GetWorldLocation(&targetWorldLoc);
            double dtargetDistanceSquared = attackerWorldLoc.GetDistanceSquared(targetWorldLoc);
            if (fBetterWeaponOutOfRangeDistance * fBetterWeaponOutOfRangeDistance > dtargetDistanceSquared) {
                UpdateInventoryTimer.ForceTrigger();
            }
        }
    }
    if (bInventoryDirty || (UpdateInventoryTimer.IsSet() && UpdateInventoryTimer.Check())) {
        UpdateInventoryData();
    }
    UpdateCombatArea(false);
    if (iLastFleeDoor) {
        double delapsedTime = fLastFleeDoorTimeStamp.ElapsedTime();
        if (fCombatFleeLastDoorRestrictTime.Float() < delapsedTime) {
            iLastFleeDoor = 0;
        }
    }
    pLastTarget = ptarget;
}

void CombatState::SetTargetBlocked(bool abValue) {
    bTargetBlocked = abValue;
}

bool CombatState::GetTargetBlocked() {
    return bTargetBlocked;
}

bool CombatState::GetTargetSpaceDifferent() {
    return bTargetSpaceDifferent;
}

CombatCoverLocation* CombatState::GetCoverLocation() {
    return pCoverLocation;
}

TESBoundObject* CombatState::GetUsableCombatItem(int32_t aiCombatItem) {
    return pUsableCombatItems[aiCombatItem];
}

void CombatState::SetInventoryDirty(bool abValue) {
    bInventoryDirty = abValue;
}

bool CombatState::GetHasTarget360LineOfSight() {
    return bTarget360LineOfSight;
}

bool CombatState::IsMeleeOnly() {
    return fMaxRangedWeaponDPS == 0.0;
}

void CombatState::Update() {
    Actor* pattacker = pCombatController->GetAttacker();
    if (pattacker->MobileObject::GetCurrentProcessType()) {
        UpdateLow();
    }
    else {
        UpdateImmediateData();
        UpdatePeriodicData();
    }
}

void CombatState::ReloadingWeapon(TESObjectWEAP* apWeapon, bool abForced) {
    if (!abForced) {
        EmbeddedWeaponUpdateTimer.ForceTrigger();
    }
}

void CombatState::SetCoverLocation(CombatCoverLocation* apCombatCoverLocation) {
    ClearCoverLocation(false);
    pCoverLocation = apCombatCoverLocation;
}

void CombatState::ClearCombatLocation() {
    ourFree(pCombatLocation);
    pCombatLocation = nullptr;
}

void CombatState::ClearLocationTarget() {
    ourFree(pLocationTarget);
    pLocationTarget = nullptr;
}

CombatTimer* CombatState::GetUsableCombatItemTimer(int32_t aiCombatItem) {
    return &UsableCombatItemTimers[aiCombatItem];
}

void CombatState::SetTargetFlushed(bool abValue) {
    cTargetFlushed = abValue;
}

bool CombatState::GetPermission(uint32_t aiPermission) {
    return (iCombatPermissions & (1 << aiPermission)) != 0;
}

void CombatState::RemoveTargetedObject(uint32_t auiIndex) {
    ourFree(pCombatItems[auiIndex]);
    pCombatItems[auiIndex] = nullptr;
}

bool CombatState::ShouldUseMelee() {
    return fMaxRangedWeaponDPS < fMaxCombatWeaponDPS;
}

void CombatState::SetFlag(int32_t aiFlag) {
    iLocationUnreachable |= aiFlag;
}

TESObjectWEAP* CombatState::GetWeaponByCombatType(COMBAT_WEAPON_TYPE aeType) {
    return pCombatWeapons[aeType];
}

void CombatState::PerformedMeleeAttack(Actor* apTarget) {
    CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
    fLastAttackTimeStamp = kTimeStamp;
}

// Records the projectile's attack time, then starts the firing window if fFiringStartedTimeStamp is still the -FLT_MAX
// sentinel; the window starts only on an exact match with -FLT_MAX.
void CombatState::FiredProjectile(Projectile* apProjectile) {
    CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
    fLastAttackTimeStamp = kTimeStamp;
    if (static_cast<float>(fFiringStartedTimeStamp) == -std::numeric_limits<float>::max()) {
        CombatTimeStamp kStartTimeStamp(CombatTimer::GetTimer());
        fFiringStartedTimeStamp = kStartTimeStamp;
    }
}

bool CombatState::GetHasBufferedDetection() {
    double elapsedTime = fLastTargetDetectedTimeStamp.ElapsedTime();
    return fCombatDetectionBufferTime.Float() > elapsedTime;
}

void CombatState::ClearSecondaryCoverSearchInfo() {
    SecondaryCoverSearchInfo.Clear();
}

void CombatState::ClearCoverSearchInfo() {
    CoverSearchInfo.Clear();
}

void CombatState::ReserveCoverLocation() {
    ReserveCoverLocation(pCoverLocation);
}

void CombatState::SetCombatLocation(CombatLocation* apCombatLocation) {
    ClearCombatLocation();
    pCombatLocation = apCombatLocation;
}

void CombatState::ClearSecondaryCoverLocation(bool abClearLocation) {
    if (pSecondaryCoverLocation) {
        if (abClearLocation) {
            AddUnreachableCoverLocation(pSecondaryCoverLocation->iCoverLocationKey);
        }
        if (pSecondaryCoverLocation->bReserved) {
            UnreserveCoverLocation(pSecondaryCoverLocation);
        }
        if (pSecondaryCoverLocation) {
            delete pSecondaryCoverLocation;
        }
        pSecondaryCoverLocation = nullptr;
    }
}

CombatState::TargetedObject* CombatState::GetTargetedObject(uint32_t auiIndex) {
    return pCombatItems[auiIndex];
}

void CombatState::UpdateCombatArea(bool abForceUpdate) {
    BGSWorldLocation attackerLoc;
    if (!pLocationTarget || (pLocationTarget->cType == 3 && abForceUpdate)) {
        TESCombatStyle* pcombatStyle = pCombatController->GetCurrentCombatStyle();
        float fcombatRadius = pcombatStyle->SimpleData.fCombatRadius;
        if (fcombatRadius > 0.0) {
            Actor* pattacker = pCombatController->pAttacker;
            float fminRadius = fcombatRadius * fCombatRadiusMinMult.Float();
            BGSWorldLocation* pattackerLocation = pattacker->GetWorldLocation(&attackerLoc);
            SetLocationTargetLocation(pattackerLocation, 3, fminRadius, fcombatRadius);
        }
    }
}

void CombatState::BoostConfidence(bool abChangeFleeStatus) {
    BGSWorldLocation attackerLoc;
    if (cCurrentConfidence < 4u) {
        if (TESAIForm::GetConfidenceLevel(cCurrentConfidence) < 100.0) {
            ++cCurrentConfidence;
        }
        pCombatController->ForcePlanUpdate();
        if (abChangeFleeStatus && pLocationTarget && pLocationTarget->cType == 1) {
            BGSWorldLocation* pattackerLocation = pCombatController->pAttacker->TESObjectREFR::GetWorldLocation(
                &attackerLoc
            );
            if (pLocationTarget->WorldLoc.GetDistanceSquared(*pattackerLocation) < 4096.0) {
                abChangeFleeStatus = false;
            }
        }

        if (abChangeFleeStatus) {
            cFleeState = 0;
        }
        if (pCoverLocation && pCoverLocation->iLocation == 12) {
            ClearCoverLocation(false);
        }
    }
}

bool CombatState::CheckMovement(const BGSWorldLocation& apWorldLocation, bool abCheckUnreachable, bool abPassive) {
    if (pLocationTarget &&
        ((pLocationTarget->cType == LOCATION_TARGET_GUARD && !GetLocationUnreachable(256)) ||
         pLocationTarget->cType == LOCATION_TARGET_CLUSTER ||
         (pLocationTarget->cType == LOCATION_TARGET_COMBAT_AREA && !abPassive))) {
        float fmaxDistance = GetLocationTargetCheckMovementDistance();
        if (fmaxDistance * fmaxDistance < apWorldLocation.GetDistanceSquared(pLocationTarget->WorldLoc)) {
            return false;
        }
    }

    return !abCheckUnreachable || !IsLocationUnreachable(apWorldLocation, -1.0f);
}

bool CombatState::CheckMovement(
    const NiPoint3& arPoint,
    TESForm* apSpace,
    bool abCheckLocationReachable,
    bool abIgnoreCombatAreaCheck
) {
    return CheckMovement(
        BGSWorldLocation(arPoint, static_cast<TESObjectCELL*>(apSpace)),
        abCheckLocationReachable,
        abIgnoreCombatAreaCheck
    );
}

// The squared distance is compared with 16384.0 (128 squared) and the angle with pi/4.
bool CombatState::CheckCoverSafe(
    const PathingCoverLocation& arCoverLocation,
    const NiPoint3& arCoverPoint,
    Actor* apTarget,
    uint8_t abUseViewCone
) {
    NiPoint3 kTargetLocation = apTarget->GetLocationOnReference();
    NiPoint3 kCoverDirection = arCoverPoint - kTargetLocation;
    float fSqrDistance = kCoverDirection.SqrLength();
    // An unordered compare (NaN) goes on.
    if (fSqrDistance < 16384.0) {
        return false;
    }
    if (abUseViewCone) {
        float fCoverAngle = GetZAngleFromVector(kCoverDirection);
        float fHeading = apTarget->GetHeading(false);
        // GetAngleDelta stores 0 here before anything else.
        float fOutDirection;
        float fAngleDelta = GetAngleDelta(fHeading, fCoverAngle, fOutDirection);
        // Only a delta above pi/4 is safe; an unordered (NaN) one is not.
        if (std::fabs(fAngleDelta) > 0.7853981852531433) {
            return true;
        }
    }
    if (CombatUtilities::IsActorUsingMelee(apTarget)) {
        TESObjectWEAP* pWeapon = apTarget->GetCurrentWeapon();
        float fReach = CombatUtilities::GetWeaponReach(apTarget, pWeapon) * 1.5;
        if (fSqrDistance < fReach * fReach) {
            return false;
        }
    }
    else {
        PathingLocation kCoverLocation(arCoverPoint, apTarget);
        bool bBlocked = Pathing::CheckLineOfSight(kCoverLocation, kTargetLocation, nullptr, nullptr);
        if (!bBlocked && arCoverLocation.IsLedgeCover()) {
            bBlocked = !CheckLedgeCoverAngle(
                arCoverLocation,
                arCoverPoint,
                apTarget->GetLocationOnReference(),
                apTarget
            );
        }
        if (bBlocked) {
            return false;
        }
    }
    return true;
}

// The cover's lower edge must stand above the target's firing point, the line from it to the cover point must cross the
// edge, and the edge's height over that line, as a ratio of the two horizontal lengths, must reach the attacker's own
// height (its sneaking one while it sneaks). For some comparisons an unordered compare goes on, and for others an
// unordered compare returns true.
bool CombatState::CheckLedgeCoverAngle(
    const PathingCoverLocation& arCoverLocation,
    const NiPoint3& arCoverPoint,
    const NiPoint3& arTargetLocation,
    Actor* apTarget
) {
    float fCoverHeight = BSMIN(arCoverLocation.LeftVertex.z, arCoverLocation.RightVertex.z);
    if (fCoverHeight <= arTargetLocation.z) {
        return false;
    }
    NiPoint3 kTargetLocation = arTargetLocation;
    kTargetLocation.z = CombatUtilities::GetAttackerFiringZOffset(apTarget, true) + kTargetLocation.z;
    if (fCoverHeight <= kTargetLocation.z) {
        return false;
    }
    NiPoint3 kIntersection;
    if (!NavMeshMath::LineToLineSegment2DIntersection(
            &arCoverLocation.LeftVertex,
            &arCoverLocation.RightVertex,
            &kTargetLocation,
            &arCoverPoint,
            &kIntersection
        )) {
        return false;
    }
    float fHeightDelta = kIntersection.z - kTargetLocation.z;
    NiPoint3 kTargetDelta = kIntersection - kTargetLocation;
    float fTargetDistance = kTargetDelta.XYLength();
    NiPoint3 kCoverDelta = kIntersection - arCoverPoint;
    float fCoverDistance = kCoverDelta.XYLength();
    float fHeightRatio = fHeightDelta * fCoverDistance / fTargetDistance;
    float fAttackerHeight;
    if (pCombatController->GetAttackerCanSneak()) {
        fAttackerHeight = pCombatController->GetAttackerSneakHeight();
    }
    else {
        fAttackerHeight = pCombatController->GetAttackerHeight();
    }
    return !(fHeightRatio < fAttackerHeight);
}

void CombatState::DeleteInventoryData() {
    ourMemset(pCombatWeapons, 0, sizeof(pCombatWeapons));
    EmbeddedWeaponsArray.Clear(false);
}

void CombatState::PreActivateReference(TESObjectREFR* apRef) {
    TargetedObject* pTargetedObject = GetTargetedObject(2);
    if (pTargetedObject && pTargetedObject->pRef == apRef) {
        pCombatController->FinishedPackageTask();
    }
    ClearReferencePointer(apRef, true);
}

void CombatState::ClearReferencePointer(TESObjectREFR* apRef, bool abSelf) {
    CombatState::TargetedObject* pObj3 = pCombatItems[3];
    CombatState::TargetedObject* pObj4 = pCombatItems[4];
    for (uint32_t i = 0; i < 5; ++i) {
        TargetedObject* pTargetedObject = GetTargetedObject(i);
        if (pTargetedObject && static_cast<const void*>(pTargetedObject->pRef) == static_cast<const void*>(apRef)) {
            RemoveTargetedObject(i);
        }
    }
    if (!abSelf &&
        (static_cast<const void*>(pObj3) != static_cast<const void*>(GetTargetedObject(3)) ||
         static_cast<const void*>(pObj4) != static_cast<const void*>(GetTargetedObject(4)))) {
        RemoveTargetedObject(3);
        RemoveTargetedObject(4);
    }
    if (pLocationTarget && pLocationTarget->pRef == apRef) {
        ClearLocationTarget();
    }

    if (pCombatLocation && static_cast<const void*>(pCombatLocation->GetRef()) == static_cast<const void*>(apRef)) {
        ClearCombatLocation();
    }

    if (static_cast<const void*>(pLastTarget) == static_cast<const void*>(apRef)) {
        pLastTarget = nullptr;
    }
}

void CombatState::SetSecondaryCoverLocation(CombatCoverLocation* apCombatCoverLocation) {
    ClearSecondaryCoverLocation(false);
    pSecondaryCoverLocation = apCombatCoverLocation;
}

void CombatState::AddUnreachableLocation(const BGSWorldLocation& arWorldLocation) {
    uint32_t uiIndex = UnreachableLocations.AddUninitialized();
    UnreachableLocations.GetAt(uiIndex).Location = arWorldLocation;
    CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
    UnreachableLocations.GetAt(uiIndex).fTimeStamp = kTimeStamp;
}

double CombatState::GetMaxDPSForWeaponType(COMBAT_WEAPON_TYPE aeType) {
    return fCombatWeaponsDPS[aeType];
}

void CombatState::ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel) {
    if (aeOldLevel == PROCESS_TYPE_HIGH && aeNewLevel) {
        ClearCoverLocation(false);
        ClearCoverSearchInfo();
        ClearSecondaryCoverLocation(false);
        ClearSecondaryCoverSearchInfo();
        ClearCombatLocation();
    }
}

void CombatState::UpdateCombatStrength() {
    Actor* pattacker = pCombatController->GetAttacker();
    fDamagePerSecond = pattacker->GetDamagePerSecond();
    fCombatStrength = pattacker->CalculateCombatStrength(fDamagePerSecond);
}

bool CombatState::GetTargetUnreachable() {
    return GetLocationUnreachable(3);
}

void CombatState::ClearTargetUnreachable() {
    ClearLocationUnreachable(3);
}

void CombatState::CheckReferences() {
    CombatState::TargetedObject* pObj3 = pCombatItems[3];
    CombatState::TargetedObject* pObj4 = pCombatItems[4];
    for (uint32_t i = 0; i < 5; ++i) {
        TargetedObject* pTargetedObject = GetTargetedObject(i);
        if (pTargetedObject && pTargetedObject->pRef && pTargetedObject->pRef->GetDisabledOrDeleted()) {
            RemoveTargetedObject(i);
        }
    }
    if (static_cast<const void*>(pObj3) != static_cast<const void*>(GetTargetedObject(3)) ||
        static_cast<const void*>(pObj4) != static_cast<const void*>(GetTargetedObject(4))) {
        RemoveTargetedObject(3);
        RemoveTargetedObject(4);
    }
}

void CombatState::ClearCovers() {
    if (pCoverLocation && !pCoverLocation->bReserved) {
        ClearCoverLocation(false);
    }

    if (pSecondaryCoverLocation && !pSecondaryCoverLocation->bReserved) {
        ClearSecondaryCoverLocation(false);
    }
}

bool CombatState::GetLastSeenTargetFiringLocation(NiPoint3& arLastSeenFiringLocation) {
    if (!GetLastSeenTargetLocation(arLastSeenFiringLocation)) {
        return false;
    }
    Actor* ptarget = pCombatController->GetTarget();
    arLastSeenFiringLocation.z = CombatUtilities::GetTargetFiringZOffset(ptarget, ACTOR_SEGMENT_INVIEW_TOP, true) +
        arLastSeenFiringLocation.z;
    return true;
}

float CombatState::GetLocationTargetNearTargetDistance() {
    if (pLocationTarget->bNearTarget) {
        return pLocationTarget->fMaxDistance + fCombatGuardFollowBufferDistance.Float();
    }

    return static_cast<float>(
        (pLocationTarget->fMaxDistance - pLocationTarget->fMinDistance) * 0.75 + pLocationTarget->fMinDistance
    );
}

bool CombatState::CheckUseLocationTargetMaxDistance() {
    Actor* pTarget = pCombatController->GetTarget();
    if (pTarget && (GetHasBufferedDetection() || pCombatController->IsSearching())) {
        return true;
    }
    return !pTarget && pLocationTarget && pLocationTarget->cType == 2;
}

bool CombatState::CheckShouldFlee() {
    if (!pCombatController->GetTarget()) {
        return false;
    }
    float fConfidence = TESAIForm::GetConfidenceLevel(cCurrentConfidence);
    if (fConfidence > 0.0) {
        fConfidence = fConfidence - fConfidenceModifier;
    }
    return fConfidence > fThreatRatio;
}

bool CombatState::TryReserveCoverLocation(CombatCoverLocation* apCoverLocation) {
    if (!apCoverLocation || apCoverLocation->IsReserved()) {
        return true;
    }
    CombatController* pcombatController = pCombatController;
    CombatCoverMap* pcoverMap = pCombatManager->CombatManager::GetCoverMap();
    return pcoverMap->TryReserveCoverLocation(pcombatController, apCoverLocation);
}

void CombatState::ReachedDestination(const BGSWorldLocation& arDestination, int32_t aiLocation) {
    if (static_cast<uint32_t>(aiLocation) == 0xFu) {
        CombatGroup* pcombatGroup = pCombatController->CombatController::GetCombatGroup();
        pcombatGroup->InvestigatedSearchLocation(arDestination);
    }
    if (pCombatLocation && pCombatLocation->GetLocation() == aiLocation) {
        ClearCombatLocation();
    }
}

double CombatState::GetLocationTargetMinDistance() {
    if (pLocationTarget) {
        return static_cast<float>(pLocationTarget->fMinDistance * 0.8999999761581421);
    }

    return 0.0f;
}

TESObjectWEAP* CombatState::GetBestCombatWeapon() {
    float fSmallestDPS = 0.0f;
    TESObjectWEAP* pBestWeapon = nullptr;
    for (int32_t i = 0; static_cast<uint32_t>(i) < 7; ++i) {
        float fMaxDPS = static_cast<float>(GetMaxDPSForWeaponType(static_cast<COMBAT_WEAPON_TYPE>(i)));
        if (fSmallestDPS < static_cast<double>(fMaxDPS)) {
            fSmallestDPS = fMaxDPS;
            if (i == 6) {
                pBestWeapon = nullptr;
            }
            else {
                pBestWeapon = GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(i));
            }
        }
    }
    return pBestWeapon;
}

bool CombatState::IsDetectionLost() {
    double elapsedTime = fLastTargetDetectedTimeStamp.ElapsedTime();
    return fCombatDetectionLostTime.Float() <= elapsedTime;
}

void CombatState::SetLocationTargetLocation(
    BGSWorldLocation* apWorldLocation,
    uint8_t acType,
    float afMinDistance,
    float afMaxDistance
) {
    ClearLocationTarget();
    pLocationTarget = new CombatLocationTarget;
    pLocationTarget->cType = acType;
    pLocationTarget->pRef = nullptr;
    pLocationTarget->WorldLoc = *apWorldLocation;
    pLocationTarget->fMinDistance = BSMAX(afMinDistance, fCombatLocationTargetRadiusMin.Float());
    pLocationTarget->fMaxDistance = BSMAX(afMaxDistance, fCombatLocationTargetRadiusMin.Float());
}

void CombatState::UpdateCombatPermissions() {
    if (CombatPermissionsTimer5sec.Check()) {
        iCombatPermissions &= 0xFFFF00FFu;
        if (RandomBool(fCombatAvoidThreatsChance.Float())) {
            iCombatPermissions |= 256;
        }
        CombatPermissionsTimer5sec.Reset(5.0f);
    }
    if (CombatPermissionsTimer10sec.Check()) {
        iCombatPermissions &= 0xFFFFFF00u;
        TESCombatStyle* pcombatStyle = pCombatController->GetCurrentCombatStyle();
        float fcoverChance = pcombatStyle->SimpleData.fCoverChance;
        float fcoverProbability = fcoverChance / 100.0;
        if (RandomBool(fcoverProbability)) {
            iCombatPermissions |= 1;
        }
        if (fcoverChance > 0.0) {
            iCombatPermissions |= 2;
        }
        CombatPermissionsTimer10sec.Reset(10.0f);
    }
}

bool CombatState::CheckShouldUpdateDetection(Actor* apAttacker, Actor* apTarget) {
    if (apTarget != pLastTarget) {
        return true;
    }
    if (iTargetDetectionLevel > 0) {
        CombatProcedureBeInCover* pProcedure = static_cast<CombatProcedureBeInCover*>(
            pCombatController->pMovementProcedure
        );
        if (pProcedure && pProcedure->GetType() == CombatProcedure::BE_IN_COVER) {
            return !pProcedure->GetBehindCover();
        }
    }
    return true;
}

void CombatState::ActivatedReference(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    int32_t aiCount,
    bool abActivatedScriptOnly
) {
    CombatTimer kTimeStamp;
    if (pCombatController->IsFleeing()) {
        TESBoundObject* pObject = apRef->GetObjectReference();
        if (pObject->GetFormType() == DOOR_ID) {
            iLastFleeDoor = apRef->GetFormID();
            ::new (&kTimeStamp) CombatTimeStamp(CombatTimer::GetTimer());
            fLastFleeDoorTimeStamp.fTimeStamp = kTimeStamp.fStartTime;
        }
    }
}

void CombatState::UsedCombatItem(TESBoundObject* apCombatItem) {
    for (uint32_t i = 0; i < 2; ++i) {
        if (pUsableCombatItems[i] == apCombatItem) {
            float ftime;
            if (!i) {
                ftime = fCombatItemRestoreTimer.Float();
            }
            ftime = fCombatItemBuffTimer.Float();
            UsableCombatItemTimers[i].Reset(ftime);
            return;
        }
    }
}

// Looks for a point to throw apGrenade at: the target's last detected location in the attacker's space, else its
// last seen location while that is at most 10 seconds old, raised to the target's top firing height. The point must be
// within the projectile's range and at least 256 units away, out of the blast's reach of the attacker, and reachable
// by an arc (arbHighArc receives which).
bool CombatState::FindGrenadeThrowLocation(CombatLocation* apRetLocation, bool& arbHighArc, TESObjectWEAP* apGrenade) {
    if (!apGrenade || !apGrenade->GetProjectile(pCombatController->pAttacker)) {
        return false;
    }
    Actor* pAttacker = pCombatController->pAttacker;
    Actor* pTarget = pCombatController->GetTarget();
    if (!pTarget) {
        return false;
    }
    BGSProjectile* pProjectile = apGrenade->GetProjectile(pAttacker);
    float fRange = CombatUtilities::GetProjectileMaximumRange(pProjectile, 0.0f);
    NiPoint3 kAttackerLocation = CombatUtilities::GetAttackerFiringLocation(pAttacker);
    for (uint32_t i = 0; i < 2; ++i) {
        NiPoint3 kTargetLocation;
        if (i) {
            // An unordered (NaN) elapsed time goes on rather than skipping.
            if (fLastTarget360LineOfSightTimeStamp.ElapsedTime() > 10.0) {
                continue;
            }
            kTargetLocation = LastSeenTargetLocation;
        }
        else {
            BGSWorldLocation kTargetWorldLocation;
            if (!pCombatController->pCombatGroup->GetTargetLastDetectedLocation(pTarget, kTargetWorldLocation)) {
                continue;
            }
            if (kTargetWorldLocation.pSpace != pAttacker->GetSpace()) {
                continue;
            }
            kTargetLocation = kTargetWorldLocation.Point;
        }
        kTargetLocation.z = CombatUtilities::GetTargetFiringZOffset(pTarget, ACTOR_SEGMENT_INVIEW_TOP, false) +
            kTargetLocation.z;
        NiPoint3 kTargetVector = kTargetLocation - kAttackerLocation;
        float fDistanceSquared = kTargetVector.SqrXYLength();
        // The compare is against the unrounded square, and an unordered (NaN) distance is not skipped and goes on.
        if (fDistanceSquared > fRange * fRange) {
            continue;
        }
        // An unordered distance goes on.
        if (fDistanceSquared < 65536.0) {
            continue;
        }
        BGSExplosion* pExplosion = pProjectile->GetExplosion();
        if (pExplosion && !pCombatManager->CheckExplosionAttack(pAttacker, pExplosion, kTargetLocation, 0.0f)) {
            continue;
        }
        NiPoint3 kVelocity;
        if (!CombatUtilities::CheckProjectileArc(
                pAttacker,
                pTarget,
                pProjectile,
                kAttackerLocation,
                kTargetLocation,
                NiPoint3::ZERO,
                arbHighArc,
                kVelocity,
                -1.0f
            )) {
            continue;
        }
        if (pExplosion) {
            float fTime = CombatUtilities::ComputeProjectileTime_(
                pProjectile,
                kAttackerLocation,
                kTargetLocation,
                arbHighArc,
                0.0f
            );
            if (!pCombatManager->CheckExplosionAttack(pAttacker, pExplosion, kTargetLocation, fTime)) {
                continue;
            }
        }
        apRetLocation->SetPoint(kTargetLocation, nullptr);
        return true;
    }
    return false;
}

bool CombatState::FindBestCoverLocation(
    TESObjectWEAP* apWeapon,
    CombatCoverSearchInfo* apCoverSearchInfo,
    CombatCoverLocation* apCombatCoverLocation,
    CombatLocation* apRetLocation,
    int32_t aiLocation
) {
    NiPoint3 center;
    CombatAI_CoverSearch.Start();
    float fMaxHeightChange = 512.0f;
    float fMaxDist = 2048.0f;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* pTarget = pCombatController->GetTarget();
    if (!pTarget) {
        return false;
    }
    float fMinRange = 0.0f;
    float fMinDistToShootFrom = 512.0f;
    float fMaxRange = 2048.0f;
    float fAbsoluteMaxRange = 2048.0f;
    CombatUtilities::ComputeOptimalWeaponRanges(pCombatController, apWeapon, fMinRange, fMaxRange, fAbsoluteMaxRange);
    fMaxRange = fMaxRange * fCombatCoverMaxRangeMult.Float();
    fMaxRange = BSMIN(fMaxRange, fAbsoluteMaxRange);
    bool bFindBehindTarget = false;
    bool bSortFarthestFirst = false;
    bool bSortDistanceFromTarget = false;
    bool bHideCover = false;
    switch (aiLocation) {
        case 2: {
            TESCombatStyle* pCombatStyle = pCombatController->GetCurrentCombatStyle();
            fMaxDist = pCombatStyle->SimpleData.fCoverSearchRadius;
            break;
        }
        case 4: {
            bHideCover = true;
            bFindBehindTarget = true;
            break;
        }
        case 5: {
            fMaxDist = 4096.0f;
            fMaxRange = 10000.0f;
            bHideCover = true;
            bSortFarthestFirst = true;
            bSortDistanceFromTarget = true;
            break;
        }
        case 10: {
            fMaxRange = 10000.0f;
            bHideCover = true;
            break;
        }
        case 12: {
            fMinDistToShootFrom = fCombatFleeMinCoverDistance.Float();
            fMaxDist = fCombatFleeMaxCoverDistance.Float();
            fMaxRange = 10000.0f;
            bSortFarthestFirst = true;
            bSortDistanceFromTarget = true;
            bHideCover = true;
            break;
        }
        default: {
            break;
        }
    }
    COVER_STATE ecurrentCoverState = COVER_STATE_NONE;
    float fSquaredDistance = 0.0f;
    if (apCombatCoverLocation->iCoverLocationKey) {
        apCoverSearchInfo->uiPreviousCoverKey = apCombatCoverLocation->iCoverLocationKey;
        apCoverSearchInfo->uiPreviousState = 0;
        if (aiLocation == apCombatCoverLocation->iLocation) {
            float fmodifiedMinDistToShootFrom = fMinDistToShootFrom / 2.0;
            if (CheckCoverLocation(
                    apCombatCoverLocation->CoverLocation,
                    apRetLocation,
                    &apCombatCoverLocation->ReservationInfo,
                    apCombatCoverLocation->CoverSearchTargetLocation,
                    apCombatCoverLocation->iCoverLocationKey,
                    apCombatCoverLocation->iCoverLocationKey,
                    aiLocation,
                    pattacker,
                    pTarget,
                    fmodifiedMinDistToShootFrom,
                    fMaxRange,
                    ecurrentCoverState,
                    fSquaredDistance,
                    bHideCover
                )) {
                apCombatCoverLocation->bDirtyFlag = false;
                CombatAI_CoverSearch.Stop();
                return true;
            }
            apCoverSearchInfo->uiPreviousState = ecurrentCoverState;
        }
        else {
            apCoverSearchInfo->uiPreviousState = 15;
        }
    }
    else {
        apCoverSearchInfo->uiPreviousCoverKey = 0;
    }
    if (pLocationTarget && (pLocationTarget->cType == 1 || pLocationTarget->cType == 2)) {
        center = pLocationTarget->WorldLoc.Point;
        fMaxDist = GetLocationTargetDistance();
    }
    else {
        center = pattacker->GetLocationOnReference();
    }
    PathingRequestCover coverRequest;
    PathingLocation originLocation(center, pattacker);
    coverRequest.SetOrigin(originLocation);
    if (aiLocation == 5) {
        NiPoint3 targetCenter;
        NiPoint3 memberCenter;
        pCombatController->pCombatGroup->ComputeTargetBoundingSphere(targetCenter);
        pCombatController->pCombatGroup->ComputeMemberBoundingSphere(memberCenter);
        apCoverSearchInfo->kSearchLocation = targetCenter;
        NiPoint3 distanceProjectionVector = memberCenter - targetCenter;
        coverRequest.SetDistanceProjectionVector(distanceProjectionVector);
    }
    else {
        double delapsedTime = fLastTarget360LineOfSightTimeStamp.ElapsedTime();
        apCoverSearchInfo->bSearching = fCoverEvaluationLastSeenExpireTime.Float() > delapsedTime;
        if (apCoverSearchInfo->bSearching) {
            apCoverSearchInfo->kSearchLocation = LastSeenTargetLocation;
        }
        else {
            apCoverSearchInfo->kSearchLocation = pTarget->GetLocationOnReference();
        }
    }
    coverRequest.SetThreat(apCoverSearchInfo->kSearchLocation);
    coverRequest.SetMaxHeightChange(fMaxHeightChange);
    coverRequest.SetMaxCoverDistance(fMaxDist);
    coverRequest.SetMaxResults(200u);
    coverRequest.SetFindBehindTarget(bFindBehindTarget);
    coverRequest.SetSortFarthestFirst(bSortFarthestFirst);
    coverRequest.SetSortDistanceFromTarget(bSortDistanceFromTarget);
    coverRequest.SetActorHeight(pCombatController->fAttackerHeight);
    coverRequest.SetCrouchHeight(pCombatController->fAttackerSneakHeight);
    coverRequest.bCanCrouch = pCombatController->GetAttackerCanSneak();
    coverRequest.SetHideCover(bHideCover);
    apCombatCoverLocation->iLocation = aiLocation;
    apCombatCoverLocation->CoverSearchTargetLocation = apCoverSearchInfo->kSearchLocation;
    CombatTimeStamp kNow(CombatTimer::GetTimer());
    apCoverSearchInfo->kLastSearch = kNow;
    apCoverSearchInfo->kCenter = center;
    apCoverSearchInfo->fRadius = fMaxDist;
    apCoverSearchInfo->kCoverLocations.Clear(false);
    if (Pathing::GetCoverPoints(&coverRequest, apCoverSearchInfo->kCoverLocations)) {
        pCombatManager->pCoverMap->LockForWrite();
        for (uint32_t i = 0; i < apCoverSearchInfo->kCoverLocations.iSize; ++i) {
            PathingCoverLocation* pcoverLoc = &apCoverSearchInfo->kCoverLocations.GetAt(static_cast<int32_t>(i));
            COVER_STATE estate = COVER_STATE_NONE;
            float fsquaredDistToTarget = 0.0f;
            uint32_t icoverKey = pcoverLoc->GetKey();
            if (icoverKey == apCombatCoverLocation->iCoverLocationKey) {
                pcoverLoc->cState = ecurrentCoverState;
            }
            else {
                uint32_t ilastMoveTime = pcoverLoc->GetDynamicCoverObstacleTimeStamp();
                if (ilastMoveTime && static_cast<double>(appTimer.iLastTime - ilastMoveTime) / 1000.0 < 3.0) {
                    pcoverLoc->cState = 1;
                }
                else {
                    CoverReservationInfo reservationInfo;
                    if (CheckCoverLocation(
                            *pcoverLoc,
                            apRetLocation,
                            &reservationInfo,
                            apCombatCoverLocation->CoverSearchTargetLocation,
                            icoverKey,
                            0,
                            aiLocation,
                            pattacker,
                            pTarget,
                            fMinDistToShootFrom,
                            fMaxRange,
                            estate,
                            fsquaredDistToTarget,
                            bHideCover
                        )) {
                        pcoverLoc->cState = 19;
                        apCombatCoverLocation->CoverLocation = *pcoverLoc;
                        apCombatCoverLocation->iCoverLocationKey = icoverKey;
                        apCombatCoverLocation->iCoverLocationLastMovedTimeStamp = ilastMoveTime;
                        apCombatCoverLocation->ReservationInfo = reservationInfo;
                        apCombatCoverLocation->cCoverHeight = apCombatCoverLocation->CoverLocation.GetHeight();
                        apCombatCoverLocation->bDirtyFlag = false;
                        DynamicCoverLostTimer.Clear();
                        CombatAI_CoverSearch.Stop();
                        return true;
                    }
                    pcoverLoc->cState = estate;
                }
            }
        }
        pCombatManager->pCoverMap->UnlockForWrite();
    }
    CombatAI_CoverSearch.Stop();
    return false;
}

// Fills apLocationsArray with each target's last seen location (seen within 3 seconds), else its current location.
void CombatState::BuildTargetLOSLocationsArray(BSSimpleArray<NiPoint3, 1024>* apLocationsArray) {
    CombatGroup* pGroup = pCombatController->pCombatGroup;
    uint32_t uiTargetCount = pGroup->GetTargetCount();
    apLocationsArray->SetReservedSize(uiTargetCount);
    for (uint32_t i = 0; i < uiTargetCount; ++i) {
        const NiPoint3* pPoint;
        Actor* pTarget = pGroup->GetTarget(i);
        BGSWorldLocation kTargetWorldLocation;
        if (pGroup->GetTargetLastSeenLocation(i, kTargetWorldLocation, 3.0f)) {
            pPoint = &kTargetWorldLocation.Point;
        }
        else {
            pPoint = &pTarget->GetLocationOnReference();
        }
        NiPoint3 kTargetLocation = *pPoint;
        apLocationsArray->Add(kTargetLocation);
    }
}

bool CombatState::FindBestAdvanceLocation(
    CombatCoverSearchInfo* apCoverSearchInfo,
    CombatCoverLocation* apCombatCoverLocation,
    CombatLocation* apRetLocation,
    int32_t aiLocation,
    const NiPoint3& arDestination,
    float afMinDistance,
    PathingAvoidNodeArray* apPathingAvoidNodeArray
) {
    Actor* pattacker = pCombatController->pAttacker;
    float fminAdvanceDistance = afMinDistance == 0.0 ? 256.0f : afMinDistance;
    if (aiLocation == 6) {
        TESCombatStyle* pcombatStyle = pCombatController->GetCurrentCombatStyle();
        double dnoCoverChance = 1.0 - pcombatStyle->SimpleData.fCoverChance / 100.0;
        fminAdvanceDistance = (fCoveredAdvanceMinAdvanceDistanceMax.Float() -
                               fCoveredAdvanceMinAdvanceDistanceMin.Float()) *
                dnoCoverChance +
            fCoveredAdvanceMinAdvanceDistanceMin.Float();
    }
    apCoverSearchInfo->uiPreviousCoverKey = 0;
    if (apCombatCoverLocation->iCoverLocationKey && aiLocation == apCombatCoverLocation->iLocation) {
        bool bLeftEdge = apCombatCoverLocation->ReservationInfo.bLeftEdge;
        float fRadius = pattacker->GetRadius();
        if (pCombatManager->pCoverMap->CanReserveCoverLocation(
                apCombatCoverLocation->iCoverLocationKey,
                &apCombatCoverLocation->CoverLocation,
                pCombatController,
                fRadius,
                &apCombatCoverLocation->ReservationInfo,
                true,
                bLeftEdge
            )) {
            return true;
        }
    }

    PathingRequestCoveredMove coveredMoveRequest;
    BuildCoveredMoveRequest(&coveredMoveRequest);
    PathingLocation attackerPathingLocation(pattacker);
    coveredMoveRequest.SetOrigin(attackerPathingLocation);
    PathingLocation destinationPathingLocation(arDestination, pattacker);
    coveredMoveRequest.SetDestination(destinationPathingLocation);
    coveredMoveRequest.SetCombatCoverSearchInfo(apCoverSearchInfo);
    coveredMoveRequest.pCombatCoverLocation = apCombatCoverLocation;
    coveredMoveRequest.SetAvoidNodeArray(apPathingAvoidNodeArray);
    coveredMoveRequest.iLocationIndex = aiLocation;
    bool bresult = CombatState::FindBestAdvanceLocation(&coveredMoveRequest, fminAdvanceDistance, false);
    if (bresult) {
        apRetLocation->SetPoint(coveredMoveRequest.CoverLocation, nullptr);
    }
    coveredMoveRequest.ClearCombatCoverData();
    return bresult;
}

// Fills apCoveredMoveRequest with the attacker's radius, speed, heights and health, the target's location when it
// is detected in the attacker's space, every target's line-of-sight location, and the unreachable cover locations
// (the reserved current cover among them).
void CombatState::BuildCoveredMoveRequest(PathingRequestCoveredMove* apCoveredMoveRequest) {
    Actor* pAttacker = pCombatController->pAttacker;
    Actor* pTarget = pCombatController->GetTarget();
    apCoveredMoveRequest->SetAttackerRadius(pAttacker->GetRadius());
    apCoveredMoveRequest->SetAttackerSpeed(pAttacker->GetCurrentSpeed());
    apCoveredMoveRequest->SetAttackerHeight(pCombatController->fAttackerHeight);
    if (pCombatController->GetAttackerCanSneak()) {
        apCoveredMoveRequest->SetActorSneakHeight(pCombatController->fAttackerSneakHeight);
    }
    apCoveredMoveRequest->SetAttackerHealth(static_cast<float>(pAttacker->GetActorValue(ActorValue::HEALTH)));
    if (pTarget) {
        if (GetHasBufferedDetection() && !bTargetSpaceDifferent) {
            apCoveredMoveRequest->SetTargetLocation(pTarget->GetLocationOnReference());
        }
        BuildTargetLOSLocationsArray(apCoveredMoveRequest->GetTargetLocations());
    }
    BuildUnreachableCoverLocationsArray(apCoveredMoveRequest->GetUnreachableCoverLocations());
    if (pCoverLocation && pCoverLocation->bReserved) {
        apCoveredMoveRequest->GetUnreachableCoverLocations()->Add(pCoverLocation->iCoverLocationKey);
    }
}

bool CombatState::FindCoveredMoveLocation(
    PathingCoverLocation* apCoverLocation,
    const NiPoint3& arDestination,
    uint32_t aiLocation,
    float afMinDistance,
    PathingAvoidNodeArray* apPathingAvoidNodeArray
) {
    ClearSecondaryCoverLocation(false);
    pSecondaryCoverLocation = new CombatCoverLocation;
    CombatLocation coverLocation;
    if (FindBestAdvanceLocation(
            &SecondaryCoverSearchInfo,
            pSecondaryCoverLocation,
            &coverLocation,
            aiLocation,
            arDestination,
            afMinDistance,
            apPathingAvoidNodeArray
        )) {
        pSecondaryCoverLocation->iCoverLocationKey = pSecondaryCoverLocation->CoverLocation.GetKey();
        pSecondaryCoverLocation->iCoverLocationLastMovedTimeStamp = pSecondaryCoverLocation->CoverLocation
                                                                        .GetDynamicCoverObstacleTimeStamp();
        *apCoverLocation = pSecondaryCoverLocation->CoverLocation;
        return true;
    }

    ClearSecondaryCoverLocation(false);
    return false;
}

bool CombatState::FindOptimalLocation(
    float afMinRange,
    float afMaxRange,
    float afAbsoluteMinRange,
    float afAbsoluteMaxRange,
    CombatLocation* apRetLocation,
    bool abCheckTargetLOS
) {
    NiPoint3 kReachablePoint;
    NiPoint3 kDirection;
    NiPoint3 kLocation;
    NiPoint3 kTargetPos;
    NiPoint3 kNavmeshPoint;
    Actor* pAttacker = pCombatController->GetAttacker();
    TESObjectREFR* pTarget = pCombatController->GetTarget();
    float fOptimalRange = (afMinRange + afMaxRange) / 2.0;
    NiPoint3 kAttackerPos = pAttacker->GetLocationOnReference();
    kTargetPos = pTarget->GetLocationOnReference();
    kDirection = kTargetPos - kAttackerPos;
    float fTargetDistance = kDirection.UnitizeGetLength();
    kLocation = kTargetPos - kDirection * fOptimalRange;
    float fRangeDelta = std::fabs(fOptimalRange - fTargetDistance);
    PathingLocation kAttackerLocation(pAttacker);
    TESObjectCELL* pCell = kAttackerLocation.GetCell();
    TESWorldSpace* pWorldSpace = kAttackerLocation.GetWorldspaceSimple();
    if (Pathing::FindClosestPointOnNavmesh(pWorldSpace, pCell, kLocation, &kNavmeshPoint)) {
        NiPoint3 kNavmeshOffset = kNavmeshPoint - kLocation;
        double dNavmeshSqrDistance = kNavmeshOffset.SqrLength();
        if (!(fRangeDelta * fRangeDelta < dNavmeshSqrDistance)) {
            PathingLocation kNavmeshLocation(kNavmeshPoint, pAttacker);
            float fSearchRadius = fRangeDelta + fRangeDelta;
            PathingLocation kReachableLocation = Pathing::FindClosestReachableLocation(
                kAttackerLocation,
                kNavmeshLocation,
                fSearchRadius
            );
            kReachablePoint = kReachableLocation.GetLocation();
            NiPoint3 kTargetOffset = kReachablePoint - kTargetPos;
            float fTargetSqrDistance = kTargetOffset.SqrLength();
            if (!(fTargetSqrDistance < afAbsoluteMinRange * afAbsoluteMinRange) &&
                !(fTargetSqrDistance > afAbsoluteMaxRange * afAbsoluteMaxRange) &&
                (!abCheckTargetLOS ||
                 CombatUtilities::CalculateLOS(pAttacker, pTarget, kReachablePoint, kTargetPos, false, -1))) {
                apRetLocation->SetPoint(kReachablePoint, nullptr);
                return true;
            }

            return false;
        }
    }

    return false;
}

bool CombatState::FindBestFleeLocation(
    CombatCoverLocation* apCombatCoverLocation,
    CombatLocation* apRetLocation,
    int32_t aiLocation
) {
    BGSWorldLocation teleportWorldLoc;
    BGSWorldLocation doorWorldLocation;
    BGSWorldLocation helperWorldLoc;
    BGSWorldLocation targetWorldLoc;
    BGSWorldLocation attackerWorldLoc;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    pattacker->GetWorldLocation(&attackerWorldLoc);
    if (ptarget) {
        ptarget->GetWorldLocation(&targetWorldLoc);
        Actor* pclosestHelper = nullptr;
        float fclosestHelperDistanceSquared = fCombatFleeMaxAllyDistance.Float() * fCombatFleeMaxAllyDistance.Float();
        LoadedReferenceMap* pactorMap = &pLoadedReferenceCollection->ActorsMap;
        NiTMapItem<uint32_t, TESObjectREFR*>* it = pactorMap->GetFirstPos();
        while (it) {
            uint32_t iformID = 0;
            TESObjectREFR* pref = nullptr;
            pactorMap->GetNext(it, iformID, pref);
            if (!pref->GetDisabledOrDeleted() &&
                pref->IsActor() &&
                !static_cast<Actor*>(pref)->IsDead(false) &&
                pref != pattacker &&
                pref != ptarget) {
                Actor* phelper = static_cast<Actor*>(pref);
                phelper->GetWorldLocation(&helperWorldLoc);
                float fsquaredDistance = attackerWorldLoc.GetDistanceSquared(helperWorldLoc);
                if (!(fsquaredDistance < fCombatFleeMinAllyDistance.Float() * fCombatFleeMinAllyDistance.Float()) &&
                    !(fclosestHelperDistanceSquared < static_cast<double>(fsquaredDistance))) {
                    FIGHT_REACTION ereaction;
                    float ftargetHelperDistanceSquared = targetWorldLoc.GetDistanceSquared(helperWorldLoc);
                    float ftargetAttackerDistanceSquared = targetWorldLoc.GetDistanceSquared(attackerWorldLoc);
                    if (!(ftargetHelperDistanceSquared < static_cast<double>(ftargetAttackerDistanceSquared)) &&
                        CheckMovement(helperWorldLoc, true, false) &&
                        !phelper->IsFleeing(false) &&
                        (phelper->GetShouldAttackActor(ptarget, false, ereaction, false) ||
                         pCombatManager->GetWanttoHelpGroup(phelper, pattacker)) &&
                        !phelper->GetShouldAttackActor(pattacker, false, ereaction, false)) {
                        pclosestHelper = phelper;
                        fclosestHelperDistanceSquared = fsquaredDistance;
                    }
                }
            }
        }
        if (pclosestHelper) {
            apRetLocation->SetRef(pclosestHelper);
            double dhelperRadius = pclosestHelper->GetRadius();
            double dradii = pattacker->GetRadius() + dhelperRadius;
            float fradius = fCombatMoveToActorBufferDistance.Float() + dradii;
            apRetLocation->SetRadius(fradius);
            return true;
        }
    }
    TESActorBase* pactorBase = pattacker->GetActorBase();
    if (static_cast<uint8_t>(pactorBase->HasLowLevelProcessing()) &&
        (!pattacker->IsCreature() || !pactorBase->GetCantOpenDoors()) &&
        aiLocation != 10) {
        if (!cFleeState && RandomBool(fCombatFleeInitialDoorRestrictChance.Float())) {
            CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
            fLastFleeDoorTimeStamp = kTimeStamp;
        }

        // ElapsedTime is called before the setting's Float, and its result is kept as a double.
        double dElapsedTime = fLastFleeDoorTimeStamp.ElapsedTime();
        if (fCombatFleeUseDoorRestrictTime.Float() < dElapsedTime) {
            TESObjectREFR* pclosestDoor = nullptr;
            NiPoint3 attackerLoc = pattacker->GetLocationOnReference();
            float fmaxDoorDistance = fCombatFleeMaxDoorDistance.Float();
            float fclosestDoorDistanceSquared = fmaxDoorDistance * fmaxDoorDistance;
            using DoorArray = BSSimpleArray<TESObjectREFR*, 1024>;
            DoorArray doors;
            TESForm* pattackerSpace = pattacker->GetSpace();
            if (Pathing::FindTeleportDoorsForLocation(pattackerSpace, attackerLoc, fmaxDoorDistance, &doors)) {
                for (uint32_t i = 0; i < doors.iSize; ++i) {
                    TESObjectREFR* pdoor = doors[i];
                    if (!pdoor->GetDisabledOrDeleted() && pdoor->iFormID != iLastFleeDoor) {
                        DoorTeleportData* pteleportData = pdoor->GetTeleport();
                        TESObjectREFR* plinkedDoor = pteleportData->pLinkedDoor;
                        if (plinkedDoor->iFormID != iLastFleeDoor && !plinkedDoor->GetDisabledOrDeleted()) {
                            if (bEncounterZoneTargetRestrict.Bool()) {
                                BGSEncounterZone* pdoorZone = plinkedDoor->GetEncounterZone();
                                BGSEncounterZone* pactorZone = pattacker->GetEncounterZone();
                                if (!pdoorZone) {
                                    pdoorZone = BGSEncounterZone::GetNoZone();
                                }
                                if (!pactorZone) {
                                    pactorZone = BGSEncounterZone::GetNoZone();
                                }
                                if (pdoorZone != pactorZone) {
                                    continue;
                                }
                            }
                            pdoor->GetWorldLocation(&doorWorldLocation);
                            if (CheckMovement(doorWorldLocation, true, false)) {
                                BGSWorldLocation* pteleportWorldLoc = pteleportData->GetTeleportWorldLocation(
                                    &teleportWorldLoc
                                );
                                if (CheckMovement(*pteleportWorldLoc, true, true)) {
                                    if (!pCombatController->pCombatGroup->FindTargetNearLocation(
                                            doorWorldLocation,
                                            fCombatFleeDoorTargetCheckDistance.Float(),
                                            false
                                        )) {
                                        NiPoint3 doorLoc = pdoor->GetLocationOnReference();
                                        NiPoint3 doorVec = doorLoc - attackerLoc;
                                        float fdoorDistanceSquared = doorVec.SqrLength();
                                        if (fclosestDoorDistanceSquared > static_cast<double>(fdoorDistanceSquared)) {
                                            fclosestDoorDistanceSquared = fdoorDistanceSquared;
                                            pclosestDoor = pdoor;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (pclosestDoor) {
                DoorTeleportData* pteleportData = pclosestDoor->GetTeleport();
                TESForm* pspace = pteleportData->GetTeleportWorldSpace();
                if (!pspace) {
                    pspace = pteleportData->GetTeleportCell();
                }
                apRetLocation->SetPoint(pteleportData->position, pspace);
                return true;
            }
        }
    }

    if (apCombatCoverLocation &&
        FindBestCoverLocation(nullptr, &CoverSearchInfo, apCombatCoverLocation, apRetLocation, aiLocation)) {
        return true;
    }
    if (pLocationTarget && pLocationTarget->cType == 1) {
        if (pLocationTarget->pRef) {
            apRetLocation->SetRef(pLocationTarget->pRef);
            if (pLocationTarget->pRef->IsActor()) {
                double dtargetRadius = static_cast<Actor*>(pLocationTarget->pRef)->GetRadius();
                double dradii = pattacker->GetRadius() + dtargetRadius;
                float fradius = fCombatMoveToActorBufferDistance.Float() + dradii;
                apRetLocation->SetRadius(fradius);
            }
        }
        else {
            apRetLocation->SetLocation(pLocationTarget->WorldLoc);
        }
        return true;
    }

    apRetLocation->SetFlag(CombatLocation::FLAG_EVADE, true);
    return true;
}

bool CombatState::FindLOSLocation(CombatLocation* apRetLocation, int32_t aiLocation) {
    BGSWorldLocation memberWorldLoc;
    CombatController* pcombatController;
    Actor* pactor;
    BGSWorldLocation attackerWorldLoc;
    CombatGroup* pgroup = pCombatController->pCombatGroup;
    if (apRetLocation->pRef && apRetLocation->pRef->IsActor()) {
        pactor = static_cast<Actor*>(apRetLocation->pRef);
        if (pgroup->IsMember(pactor)) {
            pcombatController = pactor->GetCombatController();
            if (pcombatController && pcombatController->pCombatState->GetHasBuffered360LineOfSight()) {
                return true;
            }
            if (pactor->Has360LOS(pCombatController->GetTarget())) {
                return true;
            }
        }
    }
    Actor* pattacker = pCombatController->pAttacker;
    pattacker->GetWorldLocation(&attackerWorldLoc);
    Actor* plosMember = nullptr;
    float fmaxDistanceSquared = 67108864.0f;
    uint32_t imemberCount = pgroup->GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        Actor* pmember = pgroup->GetMember(i);
        if (pmember != pattacker) {
            bool bhasLOS = false;
            pcombatController = pmember->GetCombatController();
            if (pcombatController && pcombatController->pCombatState->GetHasBuffered360LineOfSight()) {
                bhasLOS = true;
            }
            else if (pmember->Has360LOS(pCombatController->GetTarget())) {
                bhasLOS = true;
            }
            if (bhasLOS) {
                pmember->GetWorldLocation(&memberWorldLoc);
                float fsqrLength = memberWorldLoc.GetDistanceSquared(attackerWorldLoc);
                if (!(fmaxDistanceSquared <= static_cast<double>(fsqrLength)) &&
                    CheckMovement(memberWorldLoc, true, false)) {
                    plosMember = pmember;
                    fmaxDistanceSquared = fsqrLength;
                }
            }
        }
    }
    if (!plosMember) {
        return false;
    }
    apRetLocation->SetRef(plosMember);
    double dmemberRadius = plosMember->GetRadius();
    double dradii = pattacker->GetRadius() + dmemberRadius;
    float fradius = fCombatMoveToActorBufferDistance.Float() + dradii;
    apRetLocation->SetRadius(fradius);
    return true;
}

bool CombatState::FindBetterWeapon() {
    BGSWorldLocation ammoWorldLoc;
    BGSWorldLocation weaponWorldLoc;
    BGSWorldLocation targetWorldLoc;
    BGSWorldLocation attackerWorldLoc;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    if (!pCombatController->bAttackerCanMove) {
        return false;
    }
    if (pCurrentEmbeddedWeapon || EmbeddedWeaponsArray.iSize) {
        return false;
    }
    if (pattacker->IsCreature()) {
        TESCreature* pcreature = static_cast<TESCreature*>(pattacker->GetActorBase());
        if (!pcreature->GetWeaponList()) {
            return false;
        }
    }
    BaseProcess* pprocess = pattacker->pCurrentProcess;
    ItemChange* pcurrentWeaponItem = pprocess->GetCurrentWeapon();
    if (pcurrentWeaponItem && pcurrentWeaponItem->pExtraObjectList && pcurrentWeaponItem->pExtraObjectList->m_item) {
        if (pcurrentWeaponItem->pExtraObjectList->m_item->GetCanNotWear()) {
            return false;
        }
    }

    CombatAI_CombatItemSearch.Start();
    pattacker->GetWorldLocation(&attackerWorldLoc);
    TESObjectREFR* pnewWeapon = nullptr;
    TESBoundObject* pnewWeaponObject = nullptr;
    TESObjectREFR* pnewAmmo = nullptr;
    TESBoundObject* pnewAmmoObject = nullptr;
    TESObjectWEAP* pcurrentWeapon = pCombatController->GetCurrentWeapon();
    bool btargetUnreachable = GetTargetUnreachable();
    float fbaseWeaponDPS = btargetUnreachable ? fMaxRangedWeaponDPS : fMaxCombatWeaponDPS;
    float fweaponDPS = (fPickupWeaponMinDPSImprovementPercent.Float() + 1.0) * fbaseWeaponDPS;
    float fdistanceMult = 1.0f;
    if (fMaxCombatWeaponDPS < static_cast<double>(fDisarmedDPS)) {
        fdistanceMult = gs_fDisarmedPickupWeaponDistanceMult.Float();
        fweaponDPS = BSMIN(fweaponDPS, fDisarmedDPS);
    }
    if (btargetUnreachable) {
        fdistanceMult = fPickupWeaponTargetUnreachableDistanceMult.Float();
    }
    float fpickupWeaponDistanceMin = 0.0f;
    float fpickupWeaponDistanceMax = 0.0f;
    if (btargetUnreachable || (pcurrentWeapon && pcurrentWeapon->IsRangedWeapon())) {
        fpickupWeaponDistanceMin = fPickupWeaponRangedDistanceMin.Float();
        fpickupWeaponDistanceMax = fPickupWeaponRangedDistanceMax.Float();
    }
    else {
        if (pcurrentWeapon) {
            fpickupWeaponDistanceMin = fPickupWeaponMeleeDistanceMin.Float();
            fpickupWeaponDistanceMax = fPickupWeaponMeleeDistanceMax.Float();
        }
        else {
            fpickupWeaponDistanceMin = fPickupWeaponUnarmedDistanceMin.Float();
            fpickupWeaponDistanceMax = fPickupWeaponUnarmedDistanceMax.Float();
        }
        bool bcloseToTarget = false;
        if (ptarget) {
            ptarget->GetWorldLocation(&targetWorldLoc);
            if (attackerWorldLoc.GetDistanceSquared(targetWorldLoc) < 589824.0) {
                bcloseToTarget = true;
            }
        }
        if (!bcloseToTarget && fMaxRangedWeaponDPS > 0.0 && fMaxCombatWeaponDPS > 0.0) {
            float frangedMeleeDPSRatio = fMaxRangedWeaponDPS / fMaxCombatWeaponDPS;
            if (fPickupWeaponRangedMeleeDPSRatioThreshold.Float() < static_cast<double>(frangedMeleeDPSRatio)) {
                if (fpickupWeaponDistanceMax < static_cast<double>(fPickupWeaponRangedDistanceMax.Float())) {
                    fpickupWeaponDistanceMax = (fPickupWeaponRangedDistanceMax.Float() - fpickupWeaponDistanceMax) *
                            frangedMeleeDPSRatio +
                        fpickupWeaponDistanceMax;
                }
            }
        }
    }
    fpickupWeaponDistanceMin = fpickupWeaponDistanceMin * fdistanceMult;
    fpickupWeaponDistanceMax = fpickupWeaponDistanceMax * fdistanceMult;
    float ffoundWeaponDPS = 0.0f;
    float ffoundWeaponTotalDistanceSquared = std::numeric_limits<float>::max();
    FindObjectIterator weaponIterator(WEAP_ID, pattacker);
    TESObjectREFR* pweaponRef = nullptr;
    ItemChange* pweaponItemChange = nullptr;
    while (pLoadedReferenceCollection->GetNextObject(weaponIterator, pweaponRef, pweaponItemChange, 3u)) {
        float fhealth;
        if (pweaponItemChange) {
            fhealth = pweaponItemChange->GetItemHealth(false);
        }
        else {
            fhealth = pweaponRef->m_Extra.GetHealth();
        }
        if (!pweaponRef->GetDestroyed() && fhealth != 0.0) {
            TESObjectWEAP* pweapon;
            if (pweaponItemChange) {
                pweapon = static_cast<TESObjectWEAP*>(pweaponItemChange->pContainerObj);
            }
            else {
                pweapon = static_cast<TESObjectWEAP*>(pweaponRef->GetObjectReference());
            }
            float fcondition = 1.0f;
            if (fhealth != -1.0) {
                int64_t itotalHealth = TESHealthForm::GetFormHealth(pweapon);
                fcondition = fhealth / static_cast<double>(itotalHealth);
            }
            TESCombatStyle* pcombatStyle = pCombatController->GetCurrentCombatStyle();
            float fnewWeaponDPS = CombatFormulas::CalcCombatWeaponDPS(
                pattacker,
                pweapon,
                fcondition,
                pcombatStyle,
                nullptr,
                nullptr
            );
            if (pweapon->IsMeleeWeapon()) {
                fnewWeaponDPS = fnewWeaponDPS * fPickupWeaponMeleeWeaponDPSMult.Float();
            }
            if (!(fweaponDPS >= static_cast<double>(fnewWeaponDPS)) &&
                (!btargetUnreachable || pweapon->IsRangedWeapon()) &&
                pattacker->CanUseWeapon(pweapon, false)) {
                float fsquaredMaxDistance = 0.0;
                if (!(fweaponDPS <= 0.0)) {
                    float fnewWeaponDPSRatio = fnewWeaponDPS / fweaponDPS;
                    fnewWeaponDPSRatio = BSMIN(fnewWeaponDPSRatio, fPickupWeaponDistanceMinMaxDPSMult.Float());
                    double dratioAboveBase = fnewWeaponDPSRatio - 1.0;
                    float fdistanceRatio = dratioAboveBase / (fPickupWeaponDistanceMinMaxDPSMult.Float() - 1.0);
                    float fmaxDistance = (fpickupWeaponDistanceMax - fpickupWeaponDistanceMin) * fdistanceRatio +
                        fpickupWeaponDistanceMin;
                    fsquaredMaxDistance = fmaxDistance * fmaxDistance;
                }
                else {
                    fsquaredMaxDistance = fpickupWeaponDistanceMax * fpickupWeaponDistanceMax;
                }
                pweaponRef->GetWorldLocation(&weaponWorldLoc);
                float ftotalDistanceSquared = weaponWorldLoc.GetDistanceSquared(attackerWorldLoc);
                if (!(fsquaredMaxDistance <= static_cast<double>(ftotalDistanceSquared)) &&
                    CheckMovement(weaponWorldLoc, true, false)) {
                    if (!pweaponRef->GetObjectReference() ||
                        pweaponRef->GetObjectReference()->GetFormType() != CONT_ID ||
                        static_cast<TESObjectWEAP*>(pweaponRef->GetObjectReference()) == nullptr ||
                        ((!pweaponRef->GetLock() || !pweaponRef->GetLock()->IsLocked()) &&
                         (!pweaponRef->GetOwner() || pweaponRef->IsAnOwner(pattacker, true)))) {
                        TESObjectREFR* ptempNewAmmo = nullptr;
                        bool bhasAmmo = true;
                        TESAmmo* pammo = pweapon->GetCurrentAmmo(pattacker);
                        if (pammo) {
                            bhasAmmo = false;
                            if (!pweaponItemChange) {
                                bhasAmmo = pweaponRef->m_Extra.GetAmmoExtra() != nullptr;
                            }
                            if (!bhasAmmo) {
                                bhasAmmo = pattacker->GetInventoryObjectCount(pammo) != 0;
                            }
                            if (!bhasAmmo && pweaponItemChange && pweaponRef->GetInventoryObjectCount(pammo)) {
                                bhasAmmo = true;
                                ptempNewAmmo = pweaponRef;
                            }

                            if (!bhasAmmo) {
                                float fsquaredTotalDistance = std::numeric_limits<float>::max();
                                pweaponRef->GetLocationOnReference();
                                FindObjectIterator ammoIterator(pammo, pattacker);
                                TESObjectREFR* pammoRef = nullptr;
                                ItemChange* pammoItemChange = nullptr;
                                while (pLoadedReferenceCollection
                                           ->GetNextObject(ammoIterator, pammoRef, pammoItemChange, 3u)) {
                                    pammoRef->GetWorldLocation(&ammoWorldLoc);
                                    double dAmmoDistance = ammoWorldLoc.GetDistanceSquared(attackerWorldLoc);
                                    float fnewSquaredTotalDistance = ammoWorldLoc.GetDistanceSquared(weaponWorldLoc) +
                                        dAmmoDistance;
                                    if (!(fsquaredMaxDistance < static_cast<double>(fnewSquaredTotalDistance)) &&
                                        !(fsquaredTotalDistance < static_cast<double>(fnewSquaredTotalDistance)) &&
                                        CheckMovement(ammoWorldLoc, true, false)) {
                                        ptempNewAmmo = pammoRef;
                                        fsquaredTotalDistance = fnewSquaredTotalDistance;
                                        ftotalDistanceSquared = fnewSquaredTotalDistance;
                                        bhasAmmo = true;
                                    }
                                }
                            }
                        }
                        if (bhasAmmo) {
                            float fItemDistance = NiSqrt(ftotalDistanceSquared);
                            float ftotalDistance = BSMAX(fItemDistance, 64.0f);
                            float fmodifiedNewWeaponDPS = fnewWeaponDPS * 10000.0 / ftotalDistance;
                            bool buseWeapon = false;
                            if (!(ffoundWeaponDPS < static_cast<double>(fmodifiedNewWeaponDPS))) {
                                if (ffoundWeaponDPS == fmodifiedNewWeaponDPS) {
                                    buseWeapon = ffoundWeaponTotalDistanceSquared >
                                        static_cast<double>(ftotalDistanceSquared);
                                }
                            }
                            else {
                                buseWeapon = true;
                            }
                            if (buseWeapon) {
                                ffoundWeaponDPS = fnewWeaponDPS * 10000.0 / ftotalDistance;
                                ffoundWeaponTotalDistanceSquared = ftotalDistanceSquared;
                                pnewWeapon = pweaponRef;
                                pnewWeaponObject = pweaponRef != nullptr ? pweapon : nullptr;
                                pnewAmmo = ptempNewAmmo;
                                pnewAmmoObject = ptempNewAmmo != nullptr ? pammo : nullptr;
                            }
                        }
                    }
                }
            }
        }
    }
    CombatAI_CombatItemSearch.Stop();
    if (pnewWeapon) {
        AddObjectToTarget(3u, pnewAmmo, pnewAmmoObject);
        AddObjectToTarget(4u, pnewWeapon, pnewWeaponObject);
        return true;
    }

    return false;
}

bool CombatState::IsCoverLocationValid(CombatCoverLocation* apCoverLocation) {
    if (!apCoverLocation) {
        return false;
    }
    return apCoverLocation == pCoverLocation || apCoverLocation == pSecondaryCoverLocation;
}

void CombatState::AddObjectToTarget(uint32_t auiIndex, TESObjectREFR* apReference, TESBoundObject* apObject) {
    RemoveTargetedObject(auiIndex);
    if (apReference) {
        pCombatItems[auiIndex] = new CombatState::TargetedObject;
        pCombatItems[auiIndex]->pRef = apReference;
        pCombatItems[auiIndex]->pObject = apObject;
        apReference->SetTargeted(true);
    }
}

double CombatState::GetLocationTargetDistance() {
    if (CheckUseLocationTargetMaxDistance()) {
        return pLocationTarget->fMaxDistance;
    }

    return GetLocationTargetMinDistance();
}

float CombatState::GetLocationTargetCheckMovementDistance() {
    CombatGoal* pGoal = pCombatController->pCurrentGoal;
    if (!pGoal) {
        return this->pLocationTarget->fMaxDistance;
    }
    CombatGoal::COMBAT_GOAL egoal = pGoal->eType;
    if (egoal == CombatGoal::COMBAT_GOAL_HIDE_FROM_TARGET ||
        egoal == CombatGoal::COMBAT_GOAL_GUARD ||
        egoal == CombatGoal::COMBAT_GOAL_STAY_IN_COMBAT_AREA) {
        return this->pLocationTarget->fMaxDistance;
    }

    return pLocationTarget->fMaxDistance + fCombatGuardFollowBufferDistance.Float();
}

void CombatState::AddUnreachableCoverLocation(uint32_t aiCoverKey) {
    if (aiCoverKey) {
        uint32_t iindex = UnreachableCoverLocations.AddUninitialized();
        UnreachableCoverLocations.GetAt(static_cast<int32_t>(iindex)).iCoverKey = aiCoverKey;
        CombatTimeStamp kNow(CombatTimer::GetTimer());
        UnreachableCoverLocations.GetAt(static_cast<int32_t>(iindex)).fTimeStamp = kNow;
    }
}

void CombatState::BuildUnreachableCoverLocationsArray(BSSimpleArray<uint32_t, 1024>* apUnreachableCoverLocationsArray) {
    apUnreachableCoverLocationsArray->SetReservedSize(UnreachableCoverLocations.iSize);
    for (int32_t i = 0; i < UnreachableCoverLocations.iSize; ++i) {
        UnreachableCoverLocation* unreachableCoverLocation = &UnreachableCoverLocations[static_cast<uint32_t>(i)];
        if (!(unreachableCoverLocation->fTimeStamp.ElapsedTime() > 10.0)) {
            apUnreachableCoverLocationsArray->Add(unreachableCoverLocation->iCoverKey);
        }
        else {
            UnreachableCoverLocations.RemoveFast(i--, 1u);
        }
    }
}

bool CombatState::IsLocationUnreachable(const BGSWorldLocation& arWorldLocation, float afRadius) {
    float fradiusSquared = afRadius == -1.0 ? 16384.0f : afRadius * afRadius;
    for (int32_t i = 0; i < UnreachableLocations.iSize; ++i) {
        UnreachableLocation* unreachableLocation = &UnreachableLocations[i];
        if (!(unreachableLocation->fTimeStamp.ElapsedTime() > 10.0)) {
            if (fradiusSquared > unreachableLocation->Location.GetDistanceSquared(arWorldLocation)) {
                return true;
            }
        }
        else {
            UnreachableLocations.RemoveFast(i--, 1u);
        }
    }
    return false;
}

// KEEP: src cannot express it exactly. BuildPath's two smart pointer arguments are meant to be built before
// PathManager::QInstance is called for the object; C++17 evaluates the object expression of a member call before its
// arguments, so clang calls QInstance first.
bool CombatState::QueuePathCheck(int32_t aiLocation) {
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    if (!ptarget) {
        return false;
    }
    if (!this->spPathCheckQueue.m_pObject) {
        ActorPathingMessageQueue* pQueue = new ActorPathingMessageQueue;
        this->spPathCheckQueue = pQueue;
    }
    if (aiLocation) {
        if (aiLocation == 1) {
            const NiPoint3* pTargetLocation = &ptarget->GetLocationOnReference();
            NiPoint3 faceTargetLocation = CombatUtilities::ComputeFaceTargetLocation(
                pattacker,
                ptarget,
                *pTargetLocation,
                -1.0f
            );
            PathingLocation pathingLocationTarget(faceTargetLocation, pattacker);
            if (!pathingLocationTarget.IsValidLocation(pattacker->GetRadius())) {
                return false;
            }
            PathingRequest* prequest = new PathingRequest;
            pattacker->BuildRequest(
                prequest,
                pathingLocationTarget.GetLocation(),
                pathingLocationTarget.pCell,
                pathingLocationTarget.pWorldSpace,
                -1.0f,
                nullptr
            );
            // BuildPath takes both smart pointers by value: the queue is copied into its argument, then the request's
            // is built from the raw pointer, and the callee releases them.
            PathManager::QInstance()->BuildPath(NiPointer<PathingRequest>(prequest), spPathCheckQueue, true);
        }
    }
    else {
        PathingRequest* prequest = new PathingRequest;
        BGSWorldLocation targetWorldLoc;
        ptarget->GetWorldLocation(&targetWorldLoc);
        pattacker->BuildRequest_0(prequest, targetWorldLoc.Point, targetWorldLoc.pSpace, -1.0f, nullptr);
        // As above: the queue's slot first, then the request's; the callee releases both.
        PathManager::QInstance()->BuildPath(NiPointer<PathingRequest>(prequest), spPathCheckQueue, true);
    }
    this->iQueuedPathCheck = aiLocation;
    return true;
}

void CombatState::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&bBlocking, sizeof(bBlocking), 0);
    apSaveGameBuffer->SaveDataEndian(&iLocationUnreachable, sizeof(iLocationUnreachable), 0);
    for (TESObjectWEAP* pWeapon : pCombatWeapons) {
        apSaveGameBuffer->SaveFormID(pWeapon, 0);
    }
    // The address &EmbeddedWeaponsArray is never null, so it is not tested.
    uint32_t isaveGameCount = EmbeddedWeaponsArray.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        apSaveGameBuffer->SaveFormID(EmbeddedWeaponsArray[i], 0);
    }
    apSaveGameBuffer->SaveFormID(pCurrentEmbeddedWeapon, 0);
    apSaveGameBuffer->SaveDataEndian(&iHasCombatWeapon, sizeof(iHasCombatWeapon), 0);
    for (float& fWeaponDPS : fCombatWeaponsDPS) {
        apSaveGameBuffer->SaveDataEndian(&fWeaponDPS, sizeof(fWeaponDPS), 0);
    }
    apSaveGameBuffer->SaveDataEndian(&fMaxCombatWeaponDPS, sizeof(fMaxCombatWeaponDPS), 0);
    apSaveGameBuffer->SaveDataEndian(&fMaxRangedWeaponDPS, sizeof(fMaxRangedWeaponDPS), 0);
    apSaveGameBuffer->SaveDataEndian(&fDisarmedDPS, sizeof(fDisarmedDPS), 0);
    apSaveGameBuffer->SaveDataEndian(&fBetterWeaponOutOfRangeDistance, sizeof(fBetterWeaponOutOfRangeDistance), 0);
    apSaveGameBuffer->SaveDataEndian(&iTargetDetectionLevel, sizeof(iTargetDetectionLevel), 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetLineOfSight, sizeof(bTargetLineOfSight), 0);
    apSaveGameBuffer->SaveDataEndian(&bTarget360LineOfSight, sizeof(bTarget360LineOfSight), 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetBlocked, sizeof(bTargetBlocked), 0);
    apSaveGameBuffer->SaveDataEndian(&cTargetFlushed, sizeof(cTargetFlushed), 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetSpaceDifferent, sizeof(bTargetSpaceDifferent), 0);
    apSaveGameBuffer->SaveDataEndian(&bHasHandToHandEquip, sizeof(bHasHandToHandEquip), 0);
    apSaveGameBuffer->SaveDataEndian(&bGrenadeThrowHighArc, sizeof(bGrenadeThrowHighArc), 0);
    apSaveGameBuffer->SaveDataEndian(&bInventoryDirty, sizeof(bInventoryDirty), 0);
    apSaveGameBuffer->SaveFormID(pExplosiveProjectileBlocked, 0);
    apSaveGameBuffer->SaveDataEndian(&eTargetSegmentInView, sizeof(eTargetSegmentInView), 0);
    apSaveGameBuffer->SaveDataEndian(&fThreatScore, sizeof(fThreatScore), 0);
    apSaveGameBuffer->SaveDataEndian(&LastSeenTargetLocation, sizeof(LastSeenTargetLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&iCombatPermissions, sizeof(iCombatPermissions), 0);
    apSaveGameBuffer->SaveDataEndian(&cStartingConfidence, sizeof(cStartingConfidence), 0);
    apSaveGameBuffer->SaveDataEndian(&cCurrentConfidence, sizeof(cCurrentConfidence), 0);
    apSaveGameBuffer->SaveDataEndian(&fConfidenceModifier, sizeof(fConfidenceModifier), 0);
    apSaveGameBuffer->SaveDataEndian(&fCombatStrength, sizeof(fCombatStrength), 0);
    apSaveGameBuffer->SaveDataEndian(&fDamagePerSecond, sizeof(fDamagePerSecond), 0);
    apSaveGameBuffer->SaveDataEndian(&fThreatRatio, sizeof(fThreatRatio), 0);
    apSaveGameBuffer->SaveDataEndian(&cFleeState, sizeof(cFleeState), 0);
    apSaveGameBuffer->SaveFormID(iLastFleeDoor, 0);
    fLastTargetDetectedTimeStamp.SaveGame(apSaveGameBuffer);
    fLastTarget360LineOfSightTimeStamp.SaveGame(apSaveGameBuffer);
    fLastAttackTimeStamp.SaveGame(apSaveGameBuffer);
    fLastDamagedByAttackerTimeStamp.SaveGame(apSaveGameBuffer);
    fFiringStartedTimeStamp.SaveGame(apSaveGameBuffer);
    AttackDialogueTimer.SaveGame(apSaveGameBuffer);
    FireExplosiveDialogueTimer.SaveGame(apSaveGameBuffer);
    fThreatsConsideredTimeStamp.SaveGame(apSaveGameBuffer);
    FleeDialogueTimer.SaveGame(apSaveGameBuffer);
    fLastFleeDoorTimeStamp.SaveGame(apSaveGameBuffer);
    uint8_t csavedData = 0;
    if (pCoverLocation) {
        csavedData |= 1u;
    }
    if (-std::numeric_limits<float>::max() != CoverSearchInfo.kLastSearch.fTimeStamp) {
        csavedData |= 2u;
    }
    if (pSecondaryCoverLocation) {
        csavedData |= 4u;
    }
    if (-std::numeric_limits<float>::max() != SecondaryCoverSearchInfo.kLastSearch.fTimeStamp) {
        csavedData |= 8u;
    }
    if (pCombatLocation) {
        csavedData |= 16u;
    }
    if (pLocationTarget) {
        csavedData |= 32u;
    }
    apSaveGameBuffer->SaveDataEndian(&csavedData, sizeof(csavedData), 0);
    if (pCoverLocation) {
        pCoverLocation->SaveGame(apSaveGameBuffer);
    }
    if (pSecondaryCoverLocation) {
        pSecondaryCoverLocation->SaveGame(apSaveGameBuffer);
    }
    if (pCombatLocation) {
        pCombatLocation->SaveGame(apSaveGameBuffer);
    }
    if (pLocationTarget) {
        pLocationTarget->SaveGame(apSaveGameBuffer);
    }
    if (-std::numeric_limits<float>::max() != CoverSearchInfo.kLastSearch.fTimeStamp) {
        CoverSearchInfo.SaveGame(apSaveGameBuffer);
    }
    if (-std::numeric_limits<float>::max() != SecondaryCoverSearchInfo.kLastSearch.fTimeStamp) {
        SecondaryCoverSearchInfo.SaveGame(apSaveGameBuffer);
    }
    isaveGameCount = UnreachableCoverLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        UnreachableCoverLocations[i].SaveGame(apSaveGameBuffer);
    }
    // No null test is needed: the address &UnreachableLocations is never null.
    isaveGameCount = UnreachableLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        UnreachableLocations[i].SaveGame(apSaveGameBuffer);
    }
    fLastDodgeSearch.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&cLastDodgeDirection, sizeof(cLastDodgeDirection), 0);
    uint8_t ccombatItems = 0;
    for (uint32_t i = 0; i < 5; ++i) {
        if (pCombatItems[i]) {
            ccombatItems |= 1 << i;
        }
    }
    apSaveGameBuffer->SaveDataEndian(&ccombatItems, sizeof(ccombatItems), 0);
    for (CombatState::TargetedObject*& pItem : pCombatItems) {
        if (pItem) {
            apSaveGameBuffer->SaveFormID(pItem->pRef, 0);
            apSaveGameBuffer->SaveFormID(pItem->pObject, 0);
        }
    }
    for (uint32_t i = 0; i < 2; ++i) {
        apSaveGameBuffer->SaveFormID(pUsableCombatItems[i], 0);
        UsableCombatItemTimers[i].SaveGame(apSaveGameBuffer);
    }
    apSaveGameBuffer->SaveDataEndian(&iHasCombatItem, sizeof(iHasCombatItem), 0);
    apSaveGameBuffer->SaveFormID(pLastTarget, 0);
    apSaveGameBuffer->SaveDataEndian(&bInitializing, sizeof(bInitializing), 0);
    apSaveGameBuffer->SaveDataEndian(&iQueuedPathCheck, sizeof(iQueuedPathCheck), 0);
    TargetUnreachableTimer.SaveGame(apSaveGameBuffer);
    FindBetterWeaponTimer.SaveGame(apSaveGameBuffer);
    ExplosiveProjectileBlockedTimer.SaveGame(apSaveGameBuffer);
    CombatPermissionsTimer5sec.SaveGame(apSaveGameBuffer);
    CombatPermissionsTimer10sec.SaveGame(apSaveGameBuffer);
    GuardSneakTimer.SaveGame(apSaveGameBuffer);
    DynamicCoverLostTimer.SaveGame(apSaveGameBuffer);
    ThreatRatioUpdateTimer.SaveGame(apSaveGameBuffer);
    CombatStrengthUpdateTimer.SaveGame(apSaveGameBuffer);
    EmbeddedWeaponUpdateTimer.SaveGame(apSaveGameBuffer);
    UpdateInventoryTimer.SaveGame(apSaveGameBuffer);
}

void CombatState::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&bBlocking, sizeof(bBlocking));
    apLoadGameBuffer->LoadData(&iLocationUnreachable, sizeof(iLocationUnreachable));
    if (iLocationUnreachable == -1) {
        iLocationUnreachable = 0;
    }
    for (TESObjectWEAP*& pWeapon : pCombatWeapons) {
        uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        pWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
    }
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    EmbeddedWeaponsArray.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        TESObjectWEAP** pembeddedWeapon = &EmbeddedWeaponsArray[i];
        uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        *pembeddedWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
    }
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pCurrentEmbeddedWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
    apLoadGameBuffer->LoadData(&iHasCombatWeapon, sizeof(iHasCombatWeapon));
    for (float& fWeaponDPS : fCombatWeaponsDPS) {
        apLoadGameBuffer->LoadData(&fWeaponDPS, sizeof(fWeaponDPS));
    }
    apLoadGameBuffer->LoadData(&fMaxCombatWeaponDPS, sizeof(fMaxCombatWeaponDPS));
    apLoadGameBuffer->LoadData(&fMaxRangedWeaponDPS, sizeof(fMaxRangedWeaponDPS));
    apLoadGameBuffer->LoadData(&fDisarmedDPS, sizeof(fDisarmedDPS));
    apLoadGameBuffer->LoadData(&fBetterWeaponOutOfRangeDistance, sizeof(fBetterWeaponOutOfRangeDistance));
    apLoadGameBuffer->LoadData(&iTargetDetectionLevel, sizeof(iTargetDetectionLevel));
    apLoadGameBuffer->LoadData(&bTargetLineOfSight, sizeof(bTargetLineOfSight));
    apLoadGameBuffer->LoadData(&bTarget360LineOfSight, sizeof(bTarget360LineOfSight));
    apLoadGameBuffer->LoadData(&bTargetBlocked, sizeof(bTargetBlocked));
    apLoadGameBuffer->LoadData(&cTargetFlushed, sizeof(cTargetFlushed));
    apLoadGameBuffer->LoadData(&bTargetSpaceDifferent, sizeof(bTargetSpaceDifferent));
    apLoadGameBuffer->LoadData(&bHasHandToHandEquip, sizeof(bHasHandToHandEquip));
    apLoadGameBuffer->LoadData(&bGrenadeThrowHighArc, sizeof(bGrenadeThrowHighArc));
    apLoadGameBuffer->LoadData(&bInventoryDirty, sizeof(bInventoryDirty));
    uiFormID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiFormID);
    pExplosiveProjectileBlocked = dynamic_cast<BGSExplosion*>(pForm);
    apLoadGameBuffer->LoadData(&eTargetSegmentInView, sizeof(eTargetSegmentInView));
    apLoadGameBuffer->LoadData(&fThreatScore, sizeof(fThreatScore));
    apLoadGameBuffer->LoadData(&LastSeenTargetLocation, sizeof(LastSeenTargetLocation));
    apLoadGameBuffer->LoadData(&iCombatPermissions, sizeof(iCombatPermissions));
    apLoadGameBuffer->LoadData(&cStartingConfidence, sizeof(cStartingConfidence));
    apLoadGameBuffer->LoadData(&cCurrentConfidence, sizeof(cCurrentConfidence));
    apLoadGameBuffer->LoadData(&fConfidenceModifier, sizeof(fConfidenceModifier));
    apLoadGameBuffer->LoadData(&fCombatStrength, sizeof(fCombatStrength));
    apLoadGameBuffer->LoadData(&fDamagePerSecond, sizeof(fDamagePerSecond));
    apLoadGameBuffer->LoadData(&fThreatRatio, sizeof(fThreatRatio));
    apLoadGameBuffer->LoadData(&cFleeState, sizeof(cFleeState));
    apLoadGameBuffer->LoadFormID(iLastFleeDoor);
    fLastTargetDetectedTimeStamp.LoadGame(apLoadGameBuffer);
    fLastTarget360LineOfSightTimeStamp.LoadGame(apLoadGameBuffer);
    fLastAttackTimeStamp.LoadGame(apLoadGameBuffer);
    fLastDamagedByAttackerTimeStamp.LoadGame(apLoadGameBuffer);
    fFiringStartedTimeStamp.LoadGame(apLoadGameBuffer);
    AttackDialogueTimer.LoadGame(apLoadGameBuffer);
    FireExplosiveDialogueTimer.LoadGame(apLoadGameBuffer);
    fThreatsConsideredTimeStamp.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 20u) {
        FleeDialogueTimer.LoadGame(apLoadGameBuffer);
        fLastFleeDoorTimeStamp.LoadGame(apLoadGameBuffer);
    }
    uint8_t csavedData = 0;
    apLoadGameBuffer->LoadData(&csavedData, sizeof(csavedData));
    if ((csavedData & 1) != 0) {
        pCoverLocation = new CombatCoverLocation;
        pCoverLocation->LoadGame(apLoadGameBuffer);
        if (pCoverLocation->bReserved) {
            pCoverLocation->SetReserved(false);
            ReserveCoverLocation(pCoverLocation);
        }
    }
    if ((csavedData & 4) != 0) {
        pSecondaryCoverLocation = new CombatCoverLocation;
        pSecondaryCoverLocation->LoadGame(apLoadGameBuffer);
        if (pSecondaryCoverLocation->bReserved) {
            pSecondaryCoverLocation->SetReserved(false);
            ReserveCoverLocation(pSecondaryCoverLocation);
        }
    }
    if ((csavedData & 16) != 0) {
        pCombatLocation = new CombatLocation;
        pCombatLocation->LoadGame(apLoadGameBuffer);
    }
    if ((csavedData & 32) != 0) {
        pLocationTarget = new CombatLocationTarget;
        pLocationTarget->LoadGame(apLoadGameBuffer);
    }
    if ((csavedData & 2) != 0) {
        CoverSearchInfo.LoadGameBGS(apLoadGameBuffer);
    }
    if ((csavedData & 8) != 0) {
        SecondaryCoverSearchInfo.LoadGameBGS(apLoadGameBuffer);
    }
    isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    UnreachableCoverLocations.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        UnreachableCoverLocations[i].LoadGame(apLoadGameBuffer);
    }
    isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    UnreachableLocations.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        UnreachableLocations[i].LoadGame(apLoadGameBuffer);
    }
    fLastDodgeSearch.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&cLastDodgeDirection, sizeof(cLastDodgeDirection));
    uint8_t ccombatItems = 0;
    apLoadGameBuffer->LoadData(&ccombatItems, sizeof(ccombatItems));
    for (uint32_t i = 0; i < 5; ++i) {
        if ((1 << i & ccombatItems) != 0) {
            pCombatItems[i] = new CombatState::TargetedObject;
            // Loads the raw form ID into the reference field, as untyped storage; InitLoadGame resolves it.
            apLoadGameBuffer->LoadFormID(pCombatItems[i]->pRef);
            uiFormID = apLoadGameBuffer->LoadFormID();
            pForm = TESForm::GetFormByNumericID(uiFormID);
            pCombatItems[i]->pObject = dynamic_cast<TESBoundObject*>(pForm);
        }
    }
    for (uint32_t i = 0; i < 2; ++i) {
        uiFormID = apLoadGameBuffer->LoadFormID();
        pForm = TESForm::GetFormByNumericID(uiFormID);
        pUsableCombatItems[i] = dynamic_cast<TESBoundObject*>(pForm);
        UsableCombatItemTimers[i].LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&iHasCombatItem, sizeof(iHasCombatItem));
    apLoadGameBuffer->LoadFormID(pLastTarget);
    apLoadGameBuffer->LoadData(&bInitializing, sizeof(bInitializing));
    if (apLoadGameBuffer->GetVersion() >= 15u) {
        apLoadGameBuffer->LoadData(&iQueuedPathCheck, sizeof(iQueuedPathCheck));
    }
    TargetUnreachableTimer.LoadGame(apLoadGameBuffer);
    FindBetterWeaponTimer.LoadGame(apLoadGameBuffer);
    ExplosiveProjectileBlockedTimer.LoadGame(apLoadGameBuffer);
    CombatPermissionsTimer5sec.LoadGame(apLoadGameBuffer);
    CombatPermissionsTimer10sec.LoadGame(apLoadGameBuffer);
    GuardSneakTimer.LoadGame(apLoadGameBuffer);
    DynamicCoverLostTimer.LoadGame(apLoadGameBuffer);
    ThreatRatioUpdateTimer.LoadGame(apLoadGameBuffer);
    CombatStrengthUpdateTimer.LoadGame(apLoadGameBuffer);
    EmbeddedWeaponUpdateTimer.LoadGame(apLoadGameBuffer);
    UpdateInventoryTimer.LoadGame(apLoadGameBuffer);
}

void CombatState::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pLocationTarget) {
        pLocationTarget->InitLoadGame(apLoadGameBuffer);
    }
    for (CombatState::TargetedObject*& pItem : pCombatItems) {
        if (pItem) {
            TESForm* pLoadedRef = nullptr;
            if (pItem->pRef) {
                TESForm* pForm = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pItem->pRef))
                );
                pLoadedRef = dynamic_cast<TESObjectREFR*>(pForm);
            }
            pItem->pRef = pLoadedRef;
            if (pItem->pRef) {
                static_cast<TESObjectREFR*>(pItem->pRef)->TESObjectREFR::SetTargeted(true);
            }
        }
    }
    if (pCombatLocation) {
        pCombatLocation->InitLoadGame(apLoadGameBuffer);
    }
    Actor* pLoadedTarget = nullptr;
    if (pLastTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLastTarget)));
        pLoadedTarget = dynamic_cast<Actor*>(pForm);
    }
    pLastTarget = pLoadedTarget;
    if (iQueuedPathCheck != -1) {
        QueuePathCheck(iQueuedPathCheck);
    }
    for (float fWeaponDPS : fCombatWeaponsDPS) {
        fMaxModifiedCombatWeaponDPS = BSMAX(fMaxModifiedCombatWeaponDPS, fWeaponDPS);
    }
}

bool CombatState::TryReserveSecondaryCoverLocation() {
    return TryReserveCoverLocation(pSecondaryCoverLocation);
}

void CombatState::ClearLocationUnreachable(uint32_t aiLocation) {
    iLocationUnreachable = ~aiLocation & iLocationUnreachable;
}

void CombatState::SetLocationTargetRef(TESObjectREFR* apRef, LOCATION_TARGET_TYPE aeType, float afMin, float afMax) {
    BGSWorldLocation refWorldLoc;
    ClearLocationTarget();
    if (apRef) {
        pLocationTarget = new CombatLocationTarget;
        pLocationTarget->cType = aeType;
        pLocationTarget->pRef = apRef;
        pLocationTarget->WorldLoc = *apRef->GetWorldLocation(&refWorldLoc);
        pLocationTarget->fMinDistance = BSMAX(afMin, fCombatLocationTargetRadiusMin.Float());
        pLocationTarget->fMaxDistance = BSMAX(afMax, fCombatLocationTargetRadiusMin.Float());
    }
}

bool CombatState::CheckCoverLOS(
    PathingLocation& arCoverPathingLocation,
    const NiPoint3& arTargetLocation,
    bool abSkipDistanceCheck
) {
    if (!abSkipDistanceCheck) {
        NiPoint3 kCoverLocation = arCoverPathingLocation.GetLocation();
        NiPoint3 kTargetVector = arTargetLocation - kCoverLocation;
        if (kTargetVector.SqrLength() > 4194304.0) {
            return false;
        }
    }
    return Pathing::CheckLineOfSight(
        arCoverPathingLocation,
        arTargetLocation,
        static_cast<PathingLocation*>(nullptr),
        static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
    );
}

bool CombatState::CheckTargetsCoverLOS_0(
    PathingLocation& arCoverPathingLocation,
    BSSimpleArray<NiPoint3, 1024>* apLocationsArray
) {
    NiPoint3 kTargetLocation;
    for (uint32_t i = 0; i < apLocationsArray->GetSize(); ++i) {
        kTargetLocation = *static_cast<NiPoint3*>(&(*apLocationsArray)[i]);
        if (CombatState::CheckCoverLOS(arCoverPathingLocation, kTargetLocation, false)) {
            return true;
        }
    }
    return false;
}

// Searches cover points around a spot part of the way from the origin to the destination and takes the first one that
// is reservable, valid, closer to the destination and not threatened, out of sight of the targets and reachable within
// the combined pathing distance; each rejected point's cState says why.
bool CombatState::FindBestAdvanceLocation(
    PathingRequestCoveredMove* apCoveredMoveRequest,
    float afMinDistance,
    bool abBackgroundThread
) {
    float fMaxHeightChange = 1024.0f;
    float fMaxDist = 2048.0f;
    const int32_t iMaxCoverLocations = 200;
    PathingLocation* originPathingLocation = &apCoveredMoveRequest->Origin;
    PathingLocation* destinationPathingLocation = &apCoveredMoveRequest->Destination;
    PathingAvoidNodeArray* pavoidNodeArray = apCoveredMoveRequest->GetAvoidNodeArray();
    CombatCoverLocation* pcombatCoverLocation = apCoveredMoveRequest->pCombatCoverLocation;
    CombatCoverSearchInfo* pcombatCoverSearchInfo = apCoveredMoveRequest->pCombatCoverSearchInfo;
    // Takes the origin's location before the destination's.
    NiPoint3 originLocation = originPathingLocation->GetLocation();
    NiPoint3 originDestinationVector = destinationPathingLocation->GetLocation() - originLocation;
    float foriginDestinationLengthSquared = originDestinationVector.SqrLength();
    if (foriginDestinationLengthSquared < afMinDistance * afMinDistance) {
        return false;
    }
    float ffullPathDistance = 0.0f;
    bool bfullPathingResult = Pathing::ComputeApproximatePathingDistance(
        *originPathingLocation,
        *destinationPathingLocation,
        pavoidNodeArray,
        &ffullPathDistance,
        nullptr
    );
    float fcombinedMultiplier = 1.0f;
    if (apCoveredMoveRequest->iLocationIndex == 6) {
        fcombinedMultiplier = 3.0f;
    }
    float fadvancePathingDistance = ffullPathDistance - afMinDistance / 2.0;
    float fmaxCombinedPathingDistance = afMinDistance * fcombinedMultiplier + ffullPathDistance;
    if (fMaxDist > ffullPathDistance) {
        fMaxDist = ffullPathDistance;
    }
    float foriginDestinationLength = originDestinationVector.UnitizeGetLength();
    float fradius = foriginDestinationLength / 2.0;
    if (fMaxDist > fradius) {
        fMaxDist = fradius;
    }
    // Scales by the double 0.9f and rounds the factor to a float before the product.
    float fcenterScale = fMaxDist * 0.8999999761581421;
    NiPoint3 centerOffset = originDestinationVector * fcenterScale;
    NiPoint3 center = originPathingLocation->GetLocation() + centerOffset;
    PathingLocation centerPathingLocation(center, *originPathingLocation);
    NiPoint3 targetLocation = destinationPathingLocation->GetLocation();
    bool busingTargetLocation = false;
    if (apCoveredMoveRequest->bHasTargetLocation) {
        targetLocation = apCoveredMoveRequest->TargetLocation;
        busingTargetLocation = true;
    }
    PathingRequestCover coverRequest;
    coverRequest.SetOrigin(centerPathingLocation);
    coverRequest.SetActorPos(originPathingLocation->GetLocation());
    coverRequest.SetThreat(targetLocation);
    coverRequest.SetMaxHeightChange(fMaxHeightChange);
    coverRequest.SetMaxCoverDistance(fMaxDist);
    coverRequest.SetMaxResults(iMaxCoverLocations);
    coverRequest.SetActorHeight(apCoveredMoveRequest->fAttackerHeight);
    coverRequest.SetCrouchHeight(apCoveredMoveRequest->fAttackerSneakHeight);
    coverRequest.bCanCrouch = apCoveredMoveRequest->fAttackerSneakHeight != 0.0;
    coverRequest.SetHideCover(true);
    if (busingTargetLocation) {
        coverRequest.SetFindBothSides(true);
    }
    pcombatCoverLocation->iLocation = apCoveredMoveRequest->iLocationIndex;
    pcombatCoverLocation->CoverSearchTargetLocation = targetLocation;
    CombatTimeStamp kSearchTimeStamp(CombatTimer::GetTimer());
    pcombatCoverSearchInfo->kLastSearch = kSearchTimeStamp;
    pcombatCoverSearchInfo->kCenter = center;
    pcombatCoverSearchInfo->fRadius = fMaxDist;
    pcombatCoverSearchInfo->kSearchLocation = pcombatCoverLocation->CoverSearchTargetLocation;
    pcombatCoverSearchInfo->kCoverLocations.Clear(false);
    if (Pathing::GetCoverPoints(&coverRequest, pcombatCoverSearchInfo->kCoverLocations)) {
        if (!abBackgroundThread) {
            pCombatManager->pCoverMap->LockForWrite();
        }
        for (PathingCoverLocation& rCoverLocation : pcombatCoverSearchInfo->kCoverLocations) {
            PathingCoverLocation* pcoverLoc = &rCoverLocation;
            uint32_t icoverKey = pcoverLoc->GetKey();
            if (apCoveredMoveRequest->GetUnreachableCoverLocations()
                    ->Find(icoverKey, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) != -1) {
                pcoverLoc->cState = 3;
                continue;
            }
            CoverReservationInfo reservationInfo;
            if (!pCombatManager->pCoverMap->CanReserveCoverLocation(
                    icoverKey,
                    pcoverLoc,
                    nullptr,
                    apCoveredMoveRequest->fActorRadius,
                    &reservationInfo,
                    true,
                    false
                )) {
                pcoverLoc->cState = 9;
                continue;
            }
            NiPoint3 coverLocation = pcoverLoc->GetLocation();
            CombatUtilities::CalculateCoverPositions_1(
                apCoveredMoveRequest->fActorRadius,
                pcoverLoc,
                &reservationInfo,
                &coverLocation,
                nullptr
            );
            PathingLocation coverPathingLocation(coverLocation, *pcoverLoc);
            if (!coverPathingLocation.IsValidLocation(apCoveredMoveRequest->fAttackerRadius)) {
                pcoverLoc->cState = 4;
                continue;
            }
            NiPoint3 destinationVector = coverLocation - destinationPathingLocation->GetLocation();
            if (fadvancePathingDistance * fadvancePathingDistance < destinationVector.SqrLength()) {
                pcoverLoc->cState = 7;
                continue;
            }
            NiPoint3 coverOriginVector = originPathingLocation->GetLocation() - coverLocation;
            if (afMinDistance * afMinDistance > coverOriginVector.SqrLength()) {
                pcoverLoc->cState = 8;
                continue;
            }
            if (busingTargetLocation) {
                NiPoint3 targetVector = coverLocation - pcombatCoverLocation->CoverSearchTargetLocation;
                if (targetVector.SqrLength() < 16384.0) {
                    pcoverLoc->cState = 10;
                    continue;
                }
            }
            float ftimeToReachCover = coverOriginVector.Length() / apCoveredMoveRequest->fAttackerSpeed;
            CombatTimeStamp kSkipOlderThreats(0.0f);
            if (pCombatManager->pThreatMap->ComputeThreatScore(
                    coverLocation,
                    nullptr,
                    ftimeToReachCover,
                    0.0f,
                    kSkipOlderThreats,
                    apCoveredMoveRequest->fAttackerHealth
                ) > 0.0) {
                pcoverLoc->cState = 11;
                continue;
            }
            bool bhasCoverLOS = false;
            if (apCoveredMoveRequest->GetTargetLocations()->iSize) {
                bhasCoverLOS = CombatState::CheckTargetsCoverLOS_0(
                    coverPathingLocation,
                    apCoveredMoveRequest->GetTargetLocations()
                );
            }
            else {
                bhasCoverLOS = CombatState::CheckCoverLOS(
                    coverPathingLocation,
                    destinationPathingLocation->GetLocation(),
                    false
                );
            }
            if (bhasCoverLOS) {
                pcoverLoc->cState = 13;
                continue;
            }
            float fcoverDestinationPathingDistance = 0.0f;
            BSScrapArray<NavMeshSearchNode> coverDestinationNodes;
            bool bresult = Pathing::ComputeApproximatePathingDistance(
                coverPathingLocation,
                *destinationPathingLocation,
                pavoidNodeArray,
                &fcoverDestinationPathingDistance,
                &coverDestinationNodes
            );
            if ((bfullPathingResult && !bresult) || fadvancePathingDistance < fcoverDestinationPathingDistance) {
                pcoverLoc->cState = 17;
                continue;
            }
            float fattackerCoverPathingDistance = 0.0f;
            bresult = Pathing::ComputeApproximatePathingDistance(
                *originPathingLocation,
                coverPathingLocation,
                pavoidNodeArray,
                &fattackerCoverPathingDistance,
                nullptr
            );
            if ((bfullPathingResult && !bresult) ||
                fmaxCombinedPathingDistance < fcoverDestinationPathingDistance + fattackerCoverPathingDistance) {
                pcoverLoc->cState = 18;
                continue;
            }
            pcoverLoc->cState = 19;
            pcombatCoverLocation->CoverLocation = *pcoverLoc;
            pcombatCoverLocation->iCoverLocationKey = icoverKey;
            pcombatCoverLocation->iCoverLocationLastMovedTimeStamp = pcombatCoverLocation->CoverLocation
                                                                         .GetDynamicCoverObstacleTimeStamp();
            pcombatCoverLocation->ReservationInfo = reservationInfo;
            pcombatCoverLocation->cCoverHeight = pcombatCoverLocation->CoverLocation.GetHeight();
            pcombatCoverLocation->bDirtyFlag = false;
            if (busingTargetLocation) {
                NiPoint3 nodeCenter = targetLocation;
                BSScrapArray<NavMeshSearchNode> coverTargetNodes;
                PathingLocation targetPathingLocation(targetLocation, *originPathingLocation);
                if (Pathing::ComputeApproximatePathingDistance(
                        coverPathingLocation,
                        targetPathingLocation,
                        pavoidNodeArray,
                        nullptr,
                        &coverTargetNodes
                    ) &&
                    coverTargetNodes.iSize > 1) {
                    const BSSimpleArray<NavMeshSearchNode>& kTargetNodes = coverTargetNodes;
                    nodeCenter = kTargetNodes[1].GetCenter();
                }
                if (pcombatCoverLocation->CoverLocation.IsLeftSideClosest(nodeCenter)) {
                    pcombatCoverLocation->ReservationInfo.bLeftEdge = true;
                }
            }
            else if (coverDestinationNodes.iSize > 1) {
                const BSSimpleArray<NavMeshSearchNode>& kDestinationNodes = coverDestinationNodes;
                NiPoint3 nodeCenter = kDestinationNodes[1].GetCenter();
                if (pcombatCoverLocation->CoverLocation.IsLeftSideClosest(nodeCenter)) {
                    pcombatCoverLocation->ReservationInfo.bLeftEdge = true;
                }
            }
            apCoveredMoveRequest->SetCoverLocation(coverLocation);
            return true;
        }
        if (!abBackgroundThread) {
            pCombatManager->pCoverMap->UnlockForWrite();
        }
    }
    return false;
}

void CombatCoverLocation::Copy(const CombatCoverLocation* apCopy) {
    bDirtyFlag = apCopy->bDirtyFlag;
    bReserved = apCopy->bReserved;
    cCoverHeight = apCopy->cCoverHeight;
    iLocation = apCopy->iLocation;
    CoverSearchTargetLocation = apCopy->CoverSearchTargetLocation;
    CoverLocation = apCopy->CoverLocation;
    iCoverLocationKey = apCopy->iCoverLocationKey;
    iCoverLocationLastMovedTimeStamp = apCopy->iCoverLocationLastMovedTimeStamp;
    ReservationInfo = apCopy->ReservationInfo;
}

void CombatCoverLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bReserved)), sizeof(bReserved));
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bDirtyFlag)), sizeof(bDirtyFlag));
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cCoverHeight)), sizeof(cCoverHeight));
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iLocation)), sizeof(iLocation));
    apLoadGameBuffer->LoadData(
        static_cast<char*>(static_cast<void*>(&CoverSearchTargetLocation)),
        sizeof(CoverSearchTargetLocation)
    );
    CoverLocation.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iCoverLocationKey)), sizeof(iCoverLocationKey));
    apLoadGameBuffer->LoadData(
        static_cast<char*>(static_cast<void*>(&iCoverLocationLastMovedTimeStamp)),
        sizeof(iCoverLocationLastMovedTimeStamp)
    );
    ReservationInfo.LoadGame(apLoadGameBuffer);
}

void CombatCoverLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&bReserved, sizeof(bReserved), 0);
    apSaveGameBuffer->SaveDataEndian(&bDirtyFlag, sizeof(bDirtyFlag), 0);
    apSaveGameBuffer->SaveDataEndian(&cCoverHeight, sizeof(cCoverHeight), 0);
    apSaveGameBuffer->SaveDataEndian(&iLocation, sizeof(iLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&CoverSearchTargetLocation, sizeof(CoverSearchTargetLocation), 0);
    CoverLocation.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&iCoverLocationKey, sizeof(iCoverLocationKey), 0);
    apSaveGameBuffer->SaveDataEndian(&iCoverLocationLastMovedTimeStamp, sizeof(iCoverLocationLastMovedTimeStamp), 0);
    ReservationInfo.SaveGame(apSaveGameBuffer);
}

UnreachableLocation::UnreachableLocation() {}

void UnreachableLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Location.SaveGame(apSaveGameBuffer);
    fTimeStamp.SaveGame(apSaveGameBuffer);
}

void UnreachableLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Location.LoadGame(apLoadGameBuffer);
    fTimeStamp.LoadGame(apLoadGameBuffer);
}

PathingCoverLocation::PathingCoverLocation(const PathingCoverLocation& aLoc)
  : PathingLocation(aLoc), LeftVertex(aLoc.LeftVertex), RightVertex(aLoc.RightVertex),
    EdgeDirection(aLoc.EdgeDirection), cCoverHeight(aLoc.cCoverHeight), bStandCover(aLoc.bStandCover),
    bHigherThanCrouch(aLoc.bHigherThanCrouch), bLeftEdgeOpen(aLoc.bLeftEdgeOpen), bRightEdgeOpen(aLoc.bRightEdgeOpen),
    cEdge(aLoc.cEdge), cState(aLoc.cState) {}

CombatCoverSearchInfo::CombatCoverSearchInfo()
  : kSearchLocation(NiPoint3::ZERO), kCenter(NiPoint3::ZERO), fRadius(0.0f),
    kLastSearch(-std::numeric_limits<float>::max()), uiPreviousCoverKey(0), uiPreviousState(0), bSearching(false) {}

void CombatCoverSearchInfo::Clear() {
    fRadius = 0.0f;
    CombatTimeStamp kTimeStamp(-std::numeric_limits<float>::max());
    kLastSearch = kTimeStamp;
    kCoverLocations.Clear(true);
}

void CombatCoverSearchInfo::Copy(const CombatCoverSearchInfo* apCopy) {
    if (!apCopy) {
        Clear();
        return;
    }
    kSearchLocation = apCopy->kSearchLocation;
    kCenter = apCopy->kCenter;
    fRadius = apCopy->fRadius;
    kLastSearch = apCopy->kLastSearch;
    apCopy->kCoverLocations.CopyTo(&kCoverLocations);
    uiPreviousCoverKey = apCopy->uiPreviousCoverKey;
    uiPreviousState = apCopy->uiPreviousState;
    bSearching = apCopy->bSearching;
}

void CombatCoverSearchInfo::LoadGameBGS(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&kSearchLocation, sizeof(kSearchLocation));
    apLoadGameBuffer->LoadData(&kCenter, sizeof(kCenter));
    apLoadGameBuffer->LoadData(&fRadius, sizeof(fRadius));
    apLoadGameBuffer->LoadData(&bSearching, sizeof(bSearching));
    apLoadGameBuffer->LoadData(&uiPreviousCoverKey, sizeof(uiPreviousCoverKey));
    apLoadGameBuffer->LoadData(&uiPreviousState, sizeof(uiPreviousState));
    kLastSearch.LoadGame(apLoadGameBuffer);
    uint32_t uiNewSize = apLoadGameBuffer->LoadVariableSizedValue();
    kCoverLocations.SetSize(uiNewSize, true);
    for (uint32_t i = 0; i < uiNewSize; ++i) {
        kCoverLocations[i].LoadGame(apLoadGameBuffer);
    }
}

void CombatCoverSearchInfo::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&kSearchLocation, sizeof(kSearchLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&kCenter, sizeof(kCenter), 0);
    apSaveGameBuffer->SaveDataEndian(&fRadius, sizeof(fRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&bSearching, sizeof(bSearching), 0);
    apSaveGameBuffer->SaveDataEndian(&uiPreviousCoverKey, sizeof(uiPreviousCoverKey), 0);
    apSaveGameBuffer->SaveDataEndian(&uiPreviousState, sizeof(uiPreviousState), 0);
    kLastSearch.SaveGame(apSaveGameBuffer);
    // The address &kCoverLocations is never null.
    uint32_t isaveGameCount = kCoverLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        kCoverLocations[i].SaveGame(apSaveGameBuffer);
    }
}

void CombatLocation::SetRadius(float afRadius) {
    fRadius = afRadius;
}

bool CombatLocation::GetFlag(uint32_t aiFlag) {
    return (aiFlag & iFlags) != 0;
}

TESForm* CombatLocation::GetSpace() {
    return WorldLoc.GetSpace();
}

void CombatLocation::Copy(const CombatLocation* apCopy) {
    *this = *apCopy;
}

void CombatLocation::SetRef(TESObjectREFR* apRef) {
    BGSWorldLocation refWorldLoc;
    pRef = apRef;
    WorldLoc = *pRef->GetWorldLocation(&refWorldLoc);
}

void CombatLocation::UpdateRefLocation() {
    BGSWorldLocation refWorldLoc;
    if (pRef) {
        WorldLoc = *pRef->GetWorldLocation(&refWorldLoc);
    }
}

void CombatLocation::SetFlag(uint32_t aiFlag, bool abValue) {
    if (abValue) {
        iFlags |= aiFlag;
    }
    else {
        iFlags &= ~aiFlag;
    }
}

void CombatLocation::SetPoint(const NiPoint3& arPoint, TESForm* apSpace) {
    WorldLoc.SetLocation(arPoint, static_cast<TESObjectCELL*>(apSpace));
    pRef = nullptr;
}

void CombatLocation::SetLocation(const BGSWorldLocation& arLocation) {
    WorldLoc = arLocation;
}

void CombatLocation::SetSpace(TESForm* apSpace) {
    WorldLoc.SetSpace(apSpace);
}

void CombatLocation::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pLoadedRef = nullptr;
    if (pRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRef));
        pLoadedRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pRef = pLoadedRef;
}

void CombatLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    WorldLoc.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iLocation), sizeof(iLocation));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), sizeof(fRadius));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iFlags), sizeof(iFlags));
    apLoadGameBuffer->LoadFormID(pRef);
}

void CombatLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    WorldLoc.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iLocation), sizeof(iLocation), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadius), sizeof(fRadius), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFlags), sizeof(iFlags), 0);
    apSaveGameBuffer->SaveFormID(pRef, static_cast<uint32_t>(0));
}

bool CombatProcedureBeInCover::GetBehindCover() {
    return eState != 3 && eState != 4;
}

UnreachableCoverLocation::UnreachableCoverLocation() {}

void UnreachableCoverLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iCoverKey, sizeof(iCoverKey), 0);
    fTimeStamp.SaveGame(apSaveGameBuffer);
}

void UnreachableCoverLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&iCoverKey, sizeof(iCoverKey));
    fTimeStamp.LoadGame(apLoadGameBuffer);
}

float NiSqrt(float fValue) {
    return std::sqrt(fValue);
}
