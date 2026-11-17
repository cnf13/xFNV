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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
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

double CalculateCostForDistance(CombatController* a1, BGSWorldLocation* arg4, BGSWorldLocation* a2) {
    float v4 = arg4->GetDistanceSquared(*a2) / 589824.0;
    float v5 = BSMIN(v4, 1e+06f);
    return BSMAX(v5, 0.01f);
}

CombatLocationTarget::CombatLocationTarget()
  : cType(0), bNearTarget(false), pRef(nullptr), fMinDistance(0.0f), fMaxDistance(0.0f) {}

void CombatLocationTarget::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&cType, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bNearTarget, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&fMinDistance, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fMaxDistance, 4u, 0);
    apSaveGameBuffer->SaveFormID(pRef, 0);
    WorldLoc.SaveGame(apSaveGameBuffer);
}

// The load buffer is unused. The field's raw bytes are handed to GetFormByNumericID as they stand, then the resolved
// reference is stored back over them.
void CombatLocationTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v3;
    if (pRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRef));
        v3 = dynamic_cast<TESObjectREFR*>(pForm);
    }
    else {
        v3 = nullptr;
    }
    pRef = v3;
}

void CombatLocationTarget::LoadGame(BGSLoadGameBuffer* a2) {
    a2->LoadData(this, 1u);
    a2->LoadData(&bNearTarget, 1u);
    a2->LoadData(&fMinDistance, 4u);
    a2->LoadData(&fMaxDistance, 4u);
    a2->LoadFormID(pRef);
    WorldLoc.LoadGame(a2);
}

CombatState::TargetedObject::TargetedObject() : pRef(nullptr), pObject(nullptr) {}

CombatState::CombatState(CombatController* apCombatController, float afTimerDelay)
  : bBlocking(false), iLocationUnreachable(0), fThreatRatio(100.0f), pCombatWeapons{}, pCurrentEmbeddedWeapon(nullptr),
    iHasCombatWeapon(0), fCombatWeaponsDPS{}, fMaxCombatWeaponDPS(0.0f), fMaxRangedWeaponDPS(0.0f),
    fMaxModifiedCombatWeaponDPS(0.0f), fDisarmedDPS(0.0f), fBetterWeaponOutOfRangeDistance(0.0f),
    iTargetDetectionLevel(-1), bTargetLineOfSight(false), bTarget360LineOfSight(false), bTargetBlocked(false),
    cTargetFlushed(-1), bTargetSpaceDifferent(false), bHasHandToHandEquip(false), bGrenadeThrowHighArc(false),
    bInventoryDirty(false), pExplosiveProjectileBlocked(nullptr), eTargetSegmentInView(ACTOR_SEGMENT_INVIEW_NONE),
    fThreatScore(0.0f), LastSeenTargetLocation(NiPoint3_ZERO), iCombatPermissions(0),
    fLastTargetDetectedTimeStamp(-3.4028235e+38f), fLastTarget360LineOfSightTimeStamp(-3.4028235e+38f),
    fLastAttackTimeStamp(-3.4028235e+38f), fLastDamagedByAttackerTimeStamp(-3.4028235e+38f),
    fFiringStartedTimeStamp(-3.4028235e+38f), pLocationTarget(nullptr), cStartingConfidence(0), cCurrentConfidence(0),
    fCombatStrength(-1.0f), fDamagePerSecond(-1.0f), cFleeState(0), iLastFleeDoor(0),
    fLastFleeDoorTimeStamp(-3.4028235e+38f), pCoverLocation(nullptr), pSecondaryCoverLocation(nullptr),
    cLastDodgeDirection(-1), pCombatLocation(nullptr), pCombatItems{}, pUsableCombatItems{}, iHasCombatItem(0),
    pLastTarget(nullptr), pCombatController(apCombatController), bInitializing(true), spPathCheckQueue(nullptr),
    iQueuedPathCheck(-1) {
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
    else {
        if (3.4028235e+38f == pplannerData->GetCost(aiLocation)) {
            return false;
        }
        else {
            CombatCoverLocation* pfoundCoverLocation;
            CombatLocation* pfoundCombatLocation;
            bool bfound = false;
            CombatLocation* v16 = static_cast<CombatLocation*>(ourMalloc(sizeof(CombatLocation)));
            if (v16) {
                pfoundCombatLocation = ::new (v16) CombatLocation;
            }
            else {
                pfoundCombatLocation = nullptr;
            }
            CombatCoverLocation* v15 = static_cast<CombatCoverLocation*>(ourMalloc(sizeof(CombatCoverLocation)));
            if (v15) {
                pfoundCoverLocation = ::new (v15) CombatCoverLocation;
            }
            else {
                pfoundCoverLocation = nullptr;
            }
            pfoundCombatLocation->iLocation = aiLocation;
            pfoundCombatLocation->SetRadius(kCombatLocationTypeData[aiLocation].fRadius);
            if (pCombatLocation) {
                if (pCombatLocation->iLocation == aiLocation) {
                    pfoundCombatLocation->Copy(pCombatLocation);
                    if (pCoverLocation) {
                        if (pCoverLocation->iLocation == aiLocation) {
                            pfoundCoverLocation->Copy(pCoverLocation);
                        }
                    }
                    if (!kCombatLocationTypeData[aiLocation].bRecheckLocation) {
                        pfoundCombatLocation->UpdateRefLocation();
                        bfound = true;
                    }
                }
            }
            bool bignoreCombatRadius = false;
            if (aiLocation == HIDE_LOCATION || (aiLocation == FLEE_LOCATION || aiLocation == INVESTIGATE_LOCATION)) {
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
                            pCombatController->GetCurrentWeapon(), &CoverSearchInfo, pfoundCoverLocation,
                            pfoundCombatLocation, aiLocation
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
                                Actor* Target = pCombatController->GetTarget();
                                NiPoint3 advanceLocation = Target->GetLocationOnReference();
                                bfound = FindBestAdvanceLocation(
                                    &CoverSearchInfo, pfoundCoverLocation, pfoundCombatLocation, aiLocation,
                                    advanceLocation, 0.0f, nullptr
                                );
                            }
                        }
                        else {
                            bfound = FindBestAdvanceLocation(
                                &CoverSearchInfo, pfoundCoverLocation, pfoundCombatLocation, aiLocation,
                                pLocationTarget->WorldLoc.Point, 0.0f, nullptr
                            );
                        }
                        break;
                    }
                    case 7:
                    case 8:
                    case 9:
                    case 16: {
                        if (pLocationTarget) {
                            if (aiLocation != 8 || !GetLocationUnreachable(256)) {
                                if (pLocationTarget->pRef) {
                                    pfoundCombatLocation->SetRef(pLocationTarget->pRef);
                                    if (pLocationTarget->pRef->IsActor()) {
                                        Actor* pactor = static_cast<Actor*>(pLocationTarget->pRef);
                                        double Radius = pactor->GetRadius();
                                        float v9 = pattacker->GetRadius() + Radius;
                                        pfoundCombatLocation->SetRadius(v9);
                                    }
                                }
                                else {
                                    pfoundCombatLocation->SetLocation(pLocationTarget->WorldLoc);
                                }
                                pfoundCombatLocation->SetRadius(pLocationTarget->fMinDistance);
                                bfound = true;
                            }
                        }
                        break;
                    }
                    case 10:
                    case 12: {
                        bfound = FindBestFleeLocation(pfoundCoverLocation, pfoundCombatLocation, aiLocation);
                        break;
                    }
                    case 11: {
                        bfound =
                            FindGrenadeThrowLocation(pfoundCombatLocation, bGrenadeThrowHighArc, pCombatWeapons[3]);
                        break;
                    }
                    case 13: {
                        bfound = FindLOSLocation(pfoundCombatLocation, aiLocation);
                        break;
                    }
                    case 14: {
                        pfoundCombatLocation->SetFlag(1u, true);
                        bfound = true;
                        break;
                    }
                    case 15: {
                        CombatSearchLocation* psearchLocation = pCombatController->pCombatGroup->FindSearchLocation(
                            pattacker, 50, &pfoundCombatLocation->WorldLoc
                        );
                        if (psearchLocation) {
                            pfoundCombatLocation->SetLocation(psearchLocation->WorldLoc);
                            bfound = true;
                        }
                        else if (pfoundCombatLocation->GetPoint() != NiPoint3_ZERO) {
                            bfound = true;
                        }
                        if (bfound) {
                            pfoundCombatLocation->SetFlag(2u, true);
                        }
                        break;
                    }
                    case 17:
                    case 18:
                    case 19:
                    case 20:
                    case 21: {
                        CombatState::TargetedObject* pcombatItem =
                            GetTargetedObject(static_cast<uint32_t>(aiLocation - 17));
                        if (pcombatItem) {
                            if (pcombatItem->pRef) {
                                pfoundCombatLocation->SetRef(static_cast<TESObjectREFR*>(pcombatItem->pRef));
                                double ModelBoundSize =
                                    static_cast<TESObjectREFR*>(pcombatItem->pRef)->TESObjectREFR::GetModelBoundSize();
                                double v7 = pattacker->GetForwardLength() + ModelBoundSize;
                                float v6 = fPickupItemDistanceFudge.Float() + v7;
                                pfoundCombatLocation->SetRadius(v6);
                                pfoundCombatLocation->SetFlag(2u, true);
                                bfound = true;
                            }
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
            if (!bfound) {
                goto LABEL_77;
            }
            if (!pfoundCombatLocation->GetSpace()) {
                TESForm* Space = pattacker->GetSpace();
                pfoundCombatLocation->SetSpace(Space);
            }
            worldLoc = pfoundCombatLocation->WorldLoc;
            if ((worldLoc.Point == NiPoint3_ZERO) || CheckMovement(worldLoc, true, bignoreCombatRadius)) {
                float fcost = 0.0f;
                if (kCombatLocationTypeData[aiLocation].bUsesMovementCost) {
                    pattacker->GetWorldLocation(&attackerLoc);
                    fcost = CalculateCostForDistance(pCombatController, &attackerLoc, &worldLoc);
                }
                pplannerData->SetCombatLocation(aiLocation, pfoundCombatLocation, fcost);
                if (pfoundCoverLocation->iCoverLocationKey) {
                    pplannerData->SetCombatCoverLocation(aiLocation, pfoundCoverLocation);
                }
                else {
                    if (pfoundCoverLocation) {
                        delete pfoundCoverLocation;
                    }
                }
                return true;
            }
            else {
            LABEL_77:;
                ourFree(pfoundCombatLocation);
                if (pfoundCoverLocation) {
                    delete pfoundCoverLocation;
                }
                pplannerData->SetCombatLocation(aiLocation, nullptr, 3.4028235e+38f);
                return false;
            }
        }
    }
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
    NiPoint3 kSide = kAway.Cross(NiPoint3_UNIT_Z);
    NiPoint3 akPoints[4];
    rt::eh_vector_constructor_iterator(akPoints, sizeof(NiPoint3), 4, &emptyPointConstructor);
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
        kOffset.~NiPoint3();
        ::new (&kOffset) NiPoint3;
        switch (uiDirection) {
            case 1: kOffset = kSide; break;
            case 2: kOffset = -kSide; break;
            case 4: kOffset = -kAway; break;
            case 8: kOffset = kAway; break;
            default: break;
        }
        NiPoint3 kPoint = kAttackerPos + kOffset * afDodgeDistance;
        NiPoint3 kEdge = kPoint + kOffset * fRadius;
        CombatTimeStamp kSkipOlderThreats;
        kSkipOlderThreats.~CombatTimeStamp();
        ::new (&kSkipOlderThreats) CombatTimeStamp(0.0f);
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
    CombatTimeStamp kNow;
    FNVD_RECONSTRUCT(CombatTimeStamp)(&kNow, CombatTimer::GetTimer());
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
    BGSWorldLocation apLocation;
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
                    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
                    if (CurrentCombatStyle->IsFleeBasedOnPersonalSurvival()) {
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
                float afRadius = fCombatFleeBoostConfidenceTargetRadius.Float();
                BGSWorldLocation* WorldLocation = pattacker->GetWorldLocation(&apLocation);
                if (pgroup->FindTargetNearLocation(*WorldLocation, afRadius, true)) {
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
        UnreachableCoverLocation* unreachableCoverLocation =
            &UnreachableCoverLocations.operator[](static_cast<uint32_t>(i));
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
    if (arCoverLocation.IsStandCover() && !abHideCover && !arCoverLocation.bLeftEdgeOpen &&
        !arCoverLocation.bRightEdgeOpen) {
        areCoverState = COVER_STATE_NO_EDGES_OPEN;
        return false;
    }
    if (aiCoverKey == aiCurrentCoverLocationKey) {
        const NiPoint3* pTargetLocation = &apTarget->GetLocationOnReference();
        NiPoint3 kLocation = arCoverLocation.GetLocation();
        if (!NavMeshMath::Find2DIntersection3(
                kLocation, *pTargetLocation, arCoverLocation.LeftVertex, arCoverLocation.RightVertex
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
                aiCoverKey, &arCoverLocation, pCombatController, fRadius, apReservationInfo, abHideCover, bLeftEdge
            )) {
            eState = COVER_STATE_CANT_RESERVE;
            continue;
        }
        NiPoint3 kCoverPosition;
        NiPoint3 kFirePosition;
        CombatUtilities::CalculateCoverPositions(
            pCombatController, &arCoverLocation, apReservationInfo, &kCoverPosition, &kFirePosition
        );
        if (arCoverLocation.IsLedgeCover() && apTarget &&
            !CheckLedgeCoverAngle(arCoverLocation, kCoverPosition, arCoverSearchTargetLocation, apTarget)) {
            eState = COVER_STATE_LEDGE_ANGLE;
            continue;
        }
        PathingLocation kCoverLocation(kCoverPosition, arCoverLocation);
        PathingLocation* pCoverLocation = &kCoverLocation;
        if (!pCoverLocation->IsValidLocation(pCombatant->GetRadius())) {
            eState = COVER_STATE_COVER_INVALID;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        if (bCheckFirePosition) {
            PathingLocation kFireLocation(kFirePosition, arCoverLocation);
            PathingLocation* pFireLocation = &kFireLocation;
            if (!pFireLocation->IsValidLocation(pCombatant->GetRadius())) {
                eState = COVER_STATE_FIRE_INVALID;
                FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pFireLocation);
                FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
                continue;
            }
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pFireLocation);
        }
        if (!CheckMovement(kCoverPosition, static_cast<TESObjectCELL*>(apAttacker->GetSpace()), true, false)) {
            eState = COVER_STATE_CHECK_MOVEMENT_FAILED;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        arfSquaredDistance = (arCoverSearchTargetLocation - kCoverPosition).SqrLength();
        // An unordered (NaN) distance passes both range checks.
        if (arfSquaredDistance > afMaxRange * afMaxRange) {
            eState = COVER_STATE_TOO_FAR;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        if (arfSquaredDistance < afMinRange * afMinRange) {
            eState = COVER_STATE_TOO_CLOSE;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        BGSWorldLocation kWorldLocation;
        kWorldLocation.~BGSWorldLocation();
        ::new (&kWorldLocation) BGSWorldLocation;
        kWorldLocation.SetLocation(kCoverPosition, static_cast<TESObjectCELL*>(apAttacker->GetSpace()));
        if (pCombatController->GetCombatGroup()->FindTargetNearLocation(kWorldLocation, fTargetSearchRadius, false)) {
            eState = COVER_STATE_HOSTILES_NEAR;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        if (fTravelTime == -1.0) {
            NiPoint3 kToCover = apAttacker->GetLocationOnReference() - kCoverPosition;
            double dDistance = kToCover.Length();
            fTravelTime = static_cast<float>(dDistance / apAttacker->GetCurrentSpeed());
        }
        CombatTimeStamp kSkipOlderThreats;
        kSkipOlderThreats.~CombatTimeStamp();
        ::new (&kSkipOlderThreats) CombatTimeStamp(0.0f);
        if (pCombatManager->GetThreatMap()->ComputeThreatScore(
                kCoverPosition, apAttacker, fTravelTime, 0.0f, kSkipOlderThreats, 0.0f
            ) > 0.0) {
            eState = COVER_STATE_COVER_THREATENED;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        if (bCheckFirePosition) {
            CombatTimeStamp kFireSkipOlderThreats;
            kFireSkipOlderThreats.~CombatTimeStamp();
            ::new (&kFireSkipOlderThreats) CombatTimeStamp(0.0f);
            if (pCombatManager->GetThreatMap()->ComputeThreatScore(
                    kFirePosition, apAttacker, fTravelTime, 0.0f, kFireSkipOlderThreats, 0.0f
                ) > 0.0) {
                eState = COVER_STATE_FIRE_THREATENED;
                FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
                continue;
            }
        }
        bool bSeen = CheckCoverLOS(*pCoverLocation, arCoverSearchTargetLocation, true);
        if (!bSeen) {
            bSeen = CheckTargetsCoverLOS(*pCoverLocation, apTarget);
        }
        if (bSeen) {
            eState = COVER_STATE_COVER_LOS;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
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
                apAttacker, apTarget, kFirePosition, arCoverSearchTargetLocation, true, -1
            )) {
            eState = COVER_STATE_FIRE_LOS;
            FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
            continue;
        }
        apRetLocation->SetPoint(kCoverPosition, nullptr);
        FNVD_DESTRUCT_IN_PLACE(PathingLocation)(pCoverLocation);
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
    double elapsedTime = (double)(&fLastTarget360LineOfSightTimeStamp)->ElapsedTime();
    return (bool)((uint8_t)((&fCombatLOSBufferTime)->Float() > elapsedTime));
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
    NiPoint3* location;
    Actor* v2;
    NiPoint3 result;
    uint8_t v5;
    Actor* a3 = pCombatController->GetTarget();
    if (pCoverLocation) {
        if (a3) {
            v2 = a3;
            location = &(result = (&pCoverLocation->CoverLocation)->GetLocation());
            v5 = (uint8_t)CheckCoverSafe(pCoverLocation->CoverLocation, *location, v2, 0);
            if ((!v5) || (!GetHasTarget360LineOfSight())) {
                pCoverLocation->bDirtyFlag = true;
            }
        }
    }
    if (pCombatLocation) {
        if (((uint32_t)pCombatLocation->iLocation) <= 1u) {
            ClearCombatLocation();
        }
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
    CombatTimeStamp TimeStamp;
    TimeStamp.~CombatTimeStamp();
    ::new (&TimeStamp) CombatTimeStamp(afTime);
    fLastFleeDoorTimeStamp = TimeStamp;
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
    CombatTimer var4;
    if ((&fLastDamagedByAttackerTimeStamp)->ElapsedTime() > 1.0) {
        UpdateCombatArea(true);
    }
    float a2 = CombatTimer::GetTimer();
    ::new (&var4) CombatTimeStamp(a2);
    fLastDamagedByAttackerTimeStamp.fTimeStamp = var4.fStartTime;
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
        MEM_CONTEXT_COMBAT_AI, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatState.cpp", 838
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
                                EffectArchetypes::VALUE_MODIFIER, MagicSystem::SELF, ActorValue::HEALTH
                            );
                            fTotalEffect = static_cast<EffectItemList*>(pAlchItem)->GetEffectScore(
                                               EffectArchetypes::VALUE_AND_PARTS, MagicSystem::SELF, ActorValue::HEALTH
                                           ) +
                                fTotalEffect;
                            break;
                        case 18:
                            if (!pCombatStyle->IsFlagSet(TESCombatStyle::CANNOT_USE_STEALTHBOY)) {
                                fTotalEffect = static_cast<EffectItemList*>(pAlchItem)->GetEffectScore(
                                    EffectArchetypes::CHAMELEON, MagicSystem::SELF, ActorValue::ACTOR_VALUE_COUNT
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
    if (pCurrentWeaponItem && pCurrentWeaponItem->GetExtraDataList() &&
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
            Actor* pHolder =
                (pCurrentWeaponItem && pWeapon == pCurrentWeaponItem->GetItemChangeObject()) ? pActor : nullptr;
            TESAmmo* pAmmo = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(pHolder));
            if (pAmmo && !kAmmoArray.IsInArray(pAmmo)) {
                bool bNeedsAmmo = false;
                TESAmmo* pBestAmmo =
                    static_cast<TESAmmo*>(pInventoryChanges->GetBestAmmoForWeapon(pWeapon, bNeedsAmmo));
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
                        void* pMemory = ourMalloc(sizeof(ItemChange));
                        ItemChange* pNewAmmo = pMemory ? ::new (pMemory) ItemChange(pBestAmmo, iCount) : nullptr;
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
                        bool bOutOfAmmo = pCurrentAmmo && pCurrentAmmo->GetCount() <= 0 && pHeldWeapon &&
                            pHeldWeapon->GetAmmoPerShot() > 0;
                        bool bReloading = pActor->IsReloading() || (pAnimation && pAnimation->ReloadAnimQueued());
                        if (bOutOfAmmo && !bReloading) {
                            pActor->QueueReload();
                        }
                    }
                }
            }
            rWeaponData.fDamagePerSecond = CombatFormulas::CalcCombatWeaponDPS(
                pActor, pWeapon, rWeaponData.fCondition, pCombatStyle, nullptr, nullptr
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
                    pCombatController, pWeapon, fMinRange, fMaxRange, fAbsoluteMaxRange
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
        for (uint32_t j = 0; j < kWeaponDataArray.GetSize(); ++j) {
            WeaponData& rWeaponData = kWeaponDataArray[j];
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
        fCombatWeaponsDPS[COMBAT_WEAPON_TYPE_COUNT] =
            CombatFormulas::CalcCombatWeaponDPS(pActor, nullptr, 1.0f, pCombatStyle, nullptr, nullptr);
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
        BGSExplosion* pExplosion =
            pCombatWeapons[COMBAT_WEAPON_RANGED_EXPLOSIVE]->GetProjectile(pActor)->GetExplosion();
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
    Actor* arg0 = pCombatController->GetAttacker();
    Actor* a3 = pCombatController->GetTarget();
    if (a3) {
        CombatGroup* CombatGroup_ = pCombatController->GetCombatGroup();
        CombatGroup_->GetTargetDetectionLevel(
            arg0, a3, iTargetDetectionLevel, fLastTargetDetectedTimeStamp, bTargetLineOfSight, bTarget360LineOfSight
        );
    }
    if (bInventoryDirty) {
        UpdateInventoryData();
    }
    if (CombatStrengthUpdateTimer.Check()) {
        UpdateCombatStrength();
        float* v2 = &fCombatStrengthUpdateTime.Float();
        CombatStrengthUpdateTimer.Reset(*v2);
    }
    if (ThreatRatioUpdateTimer.Check()) {
        UpdateThreatRatio();
        float* v3 = &fCombatThreatRatioUpdateTime.Float();
        ThreatRatioUpdateTimer.Reset(*v3);
    }
}

void CombatState::UpdatePeriodicData() {
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    int32_t inumEmbeddedWeapons = EmbeddedWeaponsArray.iSize;
    if (inumEmbeddedWeapons > 1) {
        bool bswitchingWeapons = pCombatController->IsSwitchingWeapons();
        if (!EmbeddedWeaponUpdateTimer.IsSet() && !bswitchingWeapons) {
            float* v1 = &fEmbeddedWeaponSwitchTime.Float();
            EmbeddedWeaponUpdateTimer.Reset(*v1);
        }
        if ((EmbeddedWeaponUpdateTimer.IsSet() && EmbeddedWeaponUpdateTimer.Check()) || bInitializing) {
            bool bforceSwitch = EmbeddedWeaponUpdateTimer.IsForced();
            float* v2 = &fEmbeddedWeaponSwitchChance.Float();
            if (RandomBool(*v2) || (bInitializing || bforceSwitch)) {
                TESObjectWEAP* v20 = nullptr;
                if (bforceSwitch && pCurrentEmbeddedWeapon) {
                    int32_t iembeddedWeaponIndex = RandomInt(0, inumEmbeddedWeapons - 1);
                    for (uint32_t apThis_16 = 0; apThis_16 < EmbeddedWeaponsArray.iSize; ++apThis_16) {
                        TESObjectWEAP* apThis_12 = EmbeddedWeaponsArray.operator[](apThis_16);
                        if (apThis_12 != pCurrentEmbeddedWeapon) {
                            if (!iembeddedWeaponIndex) {
                                v20 = apThis_12;
                                break;
                            }
                            --iembeddedWeaponIndex;
                        }
                    }
                }
                else {
                    uint32_t apThis_8 = RandomInt(0, inumEmbeddedWeapons);
                    v20 = EmbeddedWeaponsArray.operator[](apThis_8);
                }
                if (v20 != pCurrentEmbeddedWeapon) {
                    pCurrentEmbeddedWeapon = v20;
                    UpdateInventoryData();
                    pCombatController->ForcePlanUpdate();
                }
            }
            EmbeddedWeaponUpdateTimer.Clear();
        }
    }
    if (pExplosiveProjectileBlocked) {
        if (!ExplosiveProjectileBlockedTimer.IsSet()) {
            float* v3 = &fExplosiveProjectileBlockedResetTime.Float();
            ExplosiveProjectileBlockedTimer.Reset(*v3);
        }
        if (ExplosiveProjectileBlockedTimer.Check()) {
            const NiPoint3* v4;
            if (ptarget &&
                (v4 = &ptarget->GetLocationOnReference(),
                 !pCombatManager->CheckExplosionAttack(pattacker, pExplosiveProjectileBlocked, *v4, 2.0f))) {
                float* v5 = &fExplosiveProjectileBlockedResetTime.Float();
                ExplosiveProjectileBlockedTimer.Reset(*v5);
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
            float* v7 = &fCombatDisarmedFindBetterWeaponTime.Float();
            FindBetterWeaponTimer.Reset(*v7);
        }
        else {
            float* v6 = &fCombatFindBetterWeaponTime.Float();
            FindBetterWeaponTimer.Reset(*v6);
            fDisarmedDPS = 0.0f;
        }
    }
    if (CombatStrengthUpdateTimer.Check()) {
        UpdateCombatStrength();
        float* v8 = &fCombatStrengthUpdateTime.Float();
        CombatStrengthUpdateTimer.Reset(*v8);
    }
    if (ThreatRatioUpdateTimer.Check()) {
        UpdateThreatRatio();
        float* v9 = &fCombatThreatRatioUpdateTime.Float();
        ThreatRatioUpdateTimer.Reset(*v9);
    }
    if (spPathCheckQueue.m_pObject) {
        ActorPathingMessage v15;
        ActorPathingMessageQueue* v11 = spPathCheckQueue.m_pObject;
        if (v11->TryPop(&v15)) {
            if (v15.msgType) {
                if (iQueuedPathCheck == 1 && (GetTargetUnreachable() && ptarget)) {
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
                spPathCheckQueue.operator=(static_cast<ActorPathingMessageQueue*>(nullptr));
            }
        }
    }
    else {
        if (GetTargetUnreachable() && ptarget) {
            if (!TargetUnreachableTimer.IsSet()) {
                TargetUnreachableTimer.Reset(5.0f);
            }
            if (TargetUnreachableTimer.Check()) {
                bool v14 = false;
                if (GetLocationUnreachable(2)) {
                    v14 = QueuePathCheck(1);
                }
                if (!v14) {
                    QueuePathCheck(0);
                }
                TargetUnreachableTimer.Reset(5.0f);
            }
        }
        else {
            TargetUnreachableTimer.Clear();
        }
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
        bool v13 = true;
        if (pCombatController->pCurrentGoal) {
            if (pCombatController->pCurrentGoal->eType == CombatGoal::COMBAT_GOAL_GUARD) {
                v13 = !CheckShouldFlee();
            }
        }
        if (v13) {
            cFleeState = 0;
        }
    }
}

void CombatState::UpdateImmediateData() {
    double DecalMinWidth;
    BGSWorldLocation apLocation;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation targetWorldLoc;
    BGSWorldLocation kAttackerLocation;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    TESForm* pattackerSpace = pattacker->GetSpace();
    if (!ptarget || pattackerSpace == ptarget->GetSpace()) {
        bTargetSpaceDifferent = false;
    }
    else {
        if (!bTargetSpaceDifferent) {
            if (bTargetLineOfSight) {
                pCombatController->ForcePlanUpdate();
            }
            bTargetSpaceDifferent = true;
        }
    }
    if (ptarget) {
        if (pCombatController->GetUpdatedDetection() || pLastTarget != ptarget) {
            int32_t inewDetectionLevel = -100;
            pCombatController->pCombatGroup->GetTargetDetectionLevel(
                pattacker, ptarget, inewDetectionLevel, fLastTargetDetectedTimeStamp, bTargetLineOfSight,
                bTarget360LineOfSight
            );
            iTargetDetectionLevel = inewDetectionLevel;
            pCombatController->SetUpdatedDetection(false);
        }
        if (bTarget360LineOfSight) {
            float za = CombatTimer::GetTimer();
            CombatTimeStamp apMemory(za);
            fLastTarget360LineOfSightTimeStamp = apMemory;
            LastSeenTargetLocation = ptarget->GetLocationOnReference();
            CombatGoal* pCombatGoal = pCombatController->pCurrentGoal;
            if (!pCombatGoal || pCombatGoal->eType != CombatGoal::COMBAT_GOAL_FLUSH_TARGET) {
                cTargetFlushed = -1;
                bGrenadeThrowHighArc = false;
            }
        }
        if (bTarget360LineOfSight && (!bTargetBlocked && pCombatController->CombatPlanContainsAction(24))) {
            pCombatController->ForcePlanUpdate();
        }
        if (!GetHasBuffered360LineOfSight() && cTargetFlushed == -1) {
            cTargetFlushed = 0;
        }
    }
    else {
        iTargetDetectionLevel = -1;
        bTarget360LineOfSight = false;
        CombatTimeStamp detectedTimeStamp(-3.4028235e+38f);
        fLastTargetDetectedTimeStamp = detectedTimeStamp;
        CombatTimeStamp lineOfSightTimeStamp(-3.4028235e+38f);
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
    else {
        if (pCoverLocation) {
            if (pCoverLocation->iCoverLocationLastMovedTimeStamp != -1 &&
                !pCoverLocation->CoverLocation.CheckDynamicCoverValid(
                    pCoverLocation->iCoverLocationLastMovedTimeStamp
                )) {
                DynamicCoverLostTimer.Reset(2.0f);
            }
        }
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
                if (pLocationTarget) {
                    if (pLocationTarget->pRef) {
                        if (pLocationTarget->pRef->IsActor()) {
                            pcurrentPartner = static_cast<Actor*>(pLocationTarget->pRef);
                        }
                    }
                }
                Actor* ppartner =
                    pCombatController->pCombatGroup->FindClusterPartner(pattacker, pcurrentPartner, fgatherDistance);
                if (ppartner) {
                    SetLocationTargetRef(ppartner, LOCATION_TARGET_CLUSTER, fminDistance, fmaxDistance);
                    goto LABEL_56;
                }
                goto LABEL_55;
            }
            CombatGroupCluster* pcluster = pCombatController->pCombatGroup->GetClusterForMember(pattacker);
            if (pcluster) {
                SetLocationTargetLocation(&pcluster->Center, 2, fminDistance, fmaxDistance);
            }
            else {
                ClearLocationTarget();
            }
        }
    }
    else {
        if (pLocationTarget && pLocationTarget->cType == 2) {
        LABEL_55:;
            ClearLocationTarget();
        }
    }
LABEL_56:;
    if (pLocationTarget) {
        if (pLocationTarget->cType == 3) {
            TESForm* Space = pLocationTarget->WorldLoc.pSpace;
            if (Space != pattackerSpace) {
                UpdateCombatArea(true);
            }
        }
        if (pLocationTarget->pRef) {
            pLocationTarget->WorldLoc = *pLocationTarget->pRef->GetWorldLocation(&apLocation);
        }
        NiPoint3 coordMult(1.0f, 1.0f, 0.25f);
        BGSWorldLocation* WorldLocation = pattacker->GetWorldLocation(&kAttackerLocation);
        float fsquaredDistance = pLocationTarget->WorldLoc.GetModifiedDistanceSquared(*WorldLocation, coordMult);
        bool bignoreLocationTarget = false;
        if (!pLocationTarget->cType || pLocationTarget->cType == 1) {
            if (ptarget) {
                if (pCombatController->GetPretendInCombat() && !pCombatController->GetContinueDuringCombat()) {
                    bignoreLocationTarget = true;
                }
            }
        }
        if (bignoreLocationTarget) {
            pLocationTarget->bNearTarget = true;
        }
        else {
            if (pLocationTarget->fMinDistance < static_cast<double>(pLocationTarget->fMaxDistance) &&
                CheckUseLocationTargetMaxDistance()) {
                float fnearTargetDistance = GetLocationTargetNearTargetDistance();
                if (pLocationTarget->bNearTarget) {
                    if (fsquaredDistance > fnearTargetDistance * fnearTargetDistance) {
                        pLocationTarget->bNearTarget = false;
                    }
                }
                else {
                    if (fsquaredDistance < fnearTargetDistance * fnearTargetDistance) {
                        pLocationTarget->bNearTarget = true;
                    }
                }
            }
            else {
                pLocationTarget->bNearTarget = false;
                if (fsquaredDistance < pLocationTarget->fMinDistance * pLocationTarget->fMinDistance) {
                    pLocationTarget->bNearTarget = true;
                }
            }
        }
        char cType = pLocationTarget->cType;
        if (cType) {
            if (cType == 1 && pCombatController->GetAttackerCanSneak()) {
                if (pLocationTarget->pRef) {
                    if (pLocationTarget->pRef->IsActor()) {
                        float* v3 = &fCombatFollowSneakFollowRadius.Float();
                        float* v4 = &fCombatFollowSneakFollowRadius.Float();
                        if (!(fsquaredDistance < *v3 * *v4)) {
                            if (pCombatController->GetForceSneak()) {
                                pCombatController->SetForceSneak(false);
                            }
                        }
                        else {
                            Actor* pguardActor = static_cast<Actor*>(pLocationTarget->pRef);
                            bool bguardSneaking = pguardActor->IsSneaking();
                            int32_t bguardSneaking_2 = bguardSneaking;
                            if (bguardSneaking_2 == pattacker->IsSneaking()) {
                                GuardSneakTimer.Clear();
                            }
                            else {
                                if (GuardSneakTimer.IsSet()) {
                                    if (GuardSneakTimer.Check()) {
                                        pCombatController->SetForceSneak(bguardSneaking);
                                        GuardSneakTimer.Clear();
                                    }
                                }
                                else {
                                    float z;
                                    if (pguardActor == pPlayer) {
                                        z = RandomFloat(0.5f, 1.0f);
                                    }
                                    else {
                                        z = RandomFloat(0.1f, 0.5f);
                                    }
                                    GuardSneakTimer.Reset(z);
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            if (pLocationTarget->bNearTarget && !bignoreLocationTarget) {
                ClearLocationTarget();
                pCombatController->FinishedPackageTask();
            }
        }
    }
    float flastThreatScore = fThreatScore;
    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
    if (!CurrentCombatStyle->GetIgnoreThreats()) {
        if (fCombatThreatSignificantScore.Float() < static_cast<double>(fThreatScore) ||
            (iCombatPermissions & 256) != 0) {
            CombatTimeStamp y;
            y.fTimeStamp = fThreatsConsideredTimeStamp.fTimeStamp;
            NiPoint3 v42 = pattacker->GetLocationOnReference();
            fThreatScore = pCombatManager->pThreatMap->ComputeThreatScore(v42, pattacker, 0.0f, 0.0f, y, 0.0f);
            if (fCombatThreatSignificantScore.Float() <= static_cast<double>(fThreatScore)) {
                float* v7 = &fCombatThreatSignificantScore.Float();
                if (*v7 > static_cast<double>(flastThreatScore)) {
                    pCombatController->ForcePlanUpdate();
                }
            }
        }
        else {
            fThreatScore = 0.0f;
        }
    }
    if (-3.4028235e+38f != fFiringStartedTimeStamp.fTimeStamp && !pattacker->IsAttacking()) {
        CombatTimeStamp firingStartedTimeStamp(-3.4028235e+38f);
        fFiringStartedTimeStamp = firingStartedTimeStamp;
    }
    if (ptarget) {
        DecalMinWidth = fBetterWeaponOutOfRangeDistance;
        if (DecalMinWidth > 0.0) {
            pattacker->GetWorldLocation(&attackerWorldLoc);
            ptarget->GetWorldLocation(&targetWorldLoc);
            DecalMinWidth = attackerWorldLoc.GetDistanceSquared(targetWorldLoc);
            if (fBetterWeaponOutOfRangeDistance * fBetterWeaponOutOfRangeDistance > DecalMinWidth) {
                UpdateInventoryTimer.ForceTrigger();
            }
        }
    }
    if (bInventoryDirty || (UpdateInventoryTimer.IsSet() && UpdateInventoryTimer.Check())) {
        UpdateInventoryData();
    }
    UpdateCombatArea(false);
    if (iLastFleeDoor) {
        DecalMinWidth = fLastFleeDoorTimeStamp.ElapsedTime();
        if (fCombatFleeLastDoorRestrictTime.Float() < DecalMinWidth) {
            iLastFleeDoor = 0;
        }
    }
    pLastTarget = ptarget;
}

void CombatState::SetTargetBlocked(bool abValue) {
    bTargetBlocked = (bool)((uint8_t)abValue);
}

bool CombatState::GetTargetBlocked() {
    return (bool)((uint8_t)bTargetBlocked);
}

bool CombatState::GetTargetSpaceDifferent() {
    return (bool)((uint8_t)bTargetSpaceDifferent);
}

CombatCoverLocation* CombatState::GetCoverLocation() {
    return pCoverLocation;
}

TESBoundObject* CombatState::GetUsableCombatItem(int32_t aiCombatItem) {
    return pUsableCombatItems[aiCombatItem];
}

void CombatState::SetInventoryDirty(bool abValue) {
    bInventoryDirty = (bool)((uint8_t)abValue);
}

bool CombatState::GetHasTarget360LineOfSight() {
    return (bool)((uint8_t)bTarget360LineOfSight);
}

bool CombatState::IsMeleeOnly() {
    return (bool)((uint8_t)(fMaxRangedWeaponDPS == 0.0));
}

void CombatState::Update() {
    Actor* packageOwner = pCombatController->GetAttacker();
    if (packageOwner->MobileObject::GetCurrentProcessType()) {
        UpdateLow();
    }
    else {
        UpdateImmediateData();
        UpdatePeriodicData();
    }
}

void CombatState::ReloadingWeapon(TESObjectWEAP* apWeapon, bool abForced) {
    if (!abForced) {
        (&EmbeddedWeaponUpdateTimer)->ForceTrigger();
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
    cTargetFlushed = (int8_t)((uint8_t)abValue);
}

bool CombatState::GetPermission(uint32_t aiPermission) {
    return ((iCombatPermissions & ((int32_t)(1 << aiPermission))) != 0);
}

void CombatState::RemoveTargetedObject(uint32_t auiIndex) {
    ourFree(pCombatItems[auiIndex]);
    pCombatItems[auiIndex] = nullptr;
}

bool CombatState::ShouldUseMelee() {
    return (bool)((uint8_t)(fMaxRangedWeaponDPS < ((double)fMaxCombatWeaponDPS)));
}

void CombatState::SetFlag(int32_t a2) {
    iLocationUnreachable |= a2;
}

TESObjectWEAP* CombatState::GetWeaponByCombatType(COMBAT_WEAPON_TYPE aeType) {
    return pCombatWeapons[aeType];
}

void CombatState::PerformedMeleeAttack(Actor* apTarget) {
    CombatTimeStamp TimeStamp;
    FNVD_RECONSTRUCT(CombatTimeStamp)(&TimeStamp, CombatTimer::GetTimer());
    fLastAttackTimeStamp = TimeStamp;
}

// Records the projectile's attack time, then starts the firing window if fFiringStartedTimeStamp is still the -FLT_MAX
// sentinel; the window starts only on an exact match with -FLT_MAX.
void CombatState::FiredProjectile(Projectile* apProjectile) {
    CombatTimeStamp TimeStamp;
    FNVD_RECONSTRUCT(CombatTimeStamp)(&TimeStamp, CombatTimer::GetTimer());
    fLastAttackTimeStamp = TimeStamp;
    if (static_cast<float>(fFiringStartedTimeStamp) == -3.4028234663852886e+38f) {
        CombatTimeStamp StartTimeStamp;
        FNVD_RECONSTRUCT(CombatTimeStamp)(&StartTimeStamp, CombatTimer::GetTimer());
        fFiringStartedTimeStamp = StartTimeStamp;
    }
}

bool CombatState::GetHasBufferedDetection() {
    double elapsedTime = (double)(&fLastTargetDetectedTimeStamp)->ElapsedTime();
    return (bool)((uint8_t)((&fCombatDetectionBufferTime)->Float() > elapsedTime));
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
    BGSWorldLocation apLocation;
    if (!pLocationTarget || (pLocationTarget->cType == 3 && abForceUpdate)) {
        TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
        float TargetTimeMult = CurrentCombatStyle->SimpleData.fCombatRadius;
        if (TargetTimeMult > 0.0) {
            Actor* pattacker = pCombatController->pAttacker;
            float v5 = TargetTimeMult * fCombatRadiusMinMult.Float();
            BGSWorldLocation* WorldLocation = pattacker->GetWorldLocation(&apLocation);
            SetLocationTargetLocation(WorldLocation, 3, v5, TargetTimeMult);
        }
    }
}

void CombatState::BoostConfidence(bool abChangeFleeStatus) {
    BGSWorldLocation* worldLocation;
    BGSWorldLocation v4;
    if (((uint32_t)((int32_t)cCurrentConfidence)) < 4u) {
        if (TESAIForm::GetConfidenceLevel(cCurrentConfidence) < 100.0) {
            ++cCurrentConfidence;
        }
        pCombatController->ForcePlanUpdate();
        if (abChangeFleeStatus) {
            if (pLocationTarget) {
                if (pLocationTarget->cType == 1) {
                    worldLocation = pCombatController->pAttacker->TESObjectREFR::GetWorldLocation(&v4);
                    if ((&pLocationTarget->WorldLoc)->GetDistanceSquared(*worldLocation) < 4096.0) {
                        abChangeFleeStatus = false;
                    }
                }
            }
        }
        if (abChangeFleeStatus) {
            cFleeState = (uint8_t)0;
        }
        if (pCoverLocation) {
            if (((uint32_t)pCoverLocation->iLocation) == 12) {
                ClearCoverLocation(false);
            }
        }
    }
}

bool CombatState::CheckMovement(const BGSWorldLocation& apWorldLocation, bool abCheckUnreachable, bool abPassive) {
    if (pLocationTarget) {
        if ((pLocationTarget->cType == LOCATION_TARGET_GUARD && !GetLocationUnreachable(256)) ||
            (pLocationTarget->cType == LOCATION_TARGET_CLUSTER ||
             (pLocationTarget->cType == LOCATION_TARGET_COMBAT_AREA && !abPassive))) {
            float fmaxDistance = GetLocationTargetCheckMovementDistance();
            if (fmaxDistance * fmaxDistance < apWorldLocation.GetDistanceSquared(pLocationTarget->WorldLoc)) {
                return false;
            }
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
    BGSWorldLocation WorldLocation;
    return CheckMovement(
        *FNVD_RECONSTRUCT(BGSWorldLocation)(&WorldLocation, arPoint, static_cast<TESObjectCELL*>(apSpace)),
        abCheckLocationReachable, abIgnoreCombatAreaCheck
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
        PathingLocation kCoverLocation;
        kCoverLocation.~PathingLocation();
        ::new (&kCoverLocation) PathingLocation(arCoverPoint, apTarget);
        bool bBlocked = Pathing::CheckLineOfSight(kCoverLocation, kTargetLocation, nullptr, nullptr);
        if (!bBlocked && arCoverLocation.IsLedgeCover()) {
            bBlocked =
                !CheckLedgeCoverAngle(arCoverLocation, arCoverPoint, apTarget->GetLocationOnReference(), apTarget);
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
            &arCoverLocation.LeftVertex, &arCoverLocation.RightVertex, &kTargetLocation, &arCoverPoint, &kIntersection
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
    TargetedObject* pTargetedObject;
    uint32_t i;
    CombatState::TargetedObject* pObj3 = pCombatItems[3];
    CombatState::TargetedObject* pObj4 = pCombatItems[4];
    for (i = 0; i < 5; ++i) {
        pTargetedObject = GetTargetedObject(i);
        if (pTargetedObject) {
            if (static_cast<const void*>(pTargetedObject->pRef) == static_cast<const void*>(apRef)) {
                RemoveTargetedObject(i);
            }
        }
    }
    if ((!abSelf) &&
        ((static_cast<const void*>(pObj3) != static_cast<const void*>(GetTargetedObject(3))) ||
         (static_cast<const void*>(pObj4) != static_cast<const void*>(GetTargetedObject(4))))) {
        RemoveTargetedObject(3);
        RemoveTargetedObject(4);
    }
    if (pLocationTarget) {
        if (pLocationTarget->pRef == apRef) {
            ClearLocationTarget();
        }
    }
    if (pCombatLocation) {
        if (static_cast<const void*>(pCombatLocation->GetRef()) == static_cast<const void*>(apRef)) {
            ClearCombatLocation();
        }
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
    CombatTimeStamp TimeStamp;
    FNVD_RECONSTRUCT(CombatTimeStamp)(&TimeStamp, CombatTimer::GetTimer());
    UnreachableLocations.GetAt(uiIndex).fTimeStamp = TimeStamp;
}

double CombatState::GetMaxDPSForWeaponType(COMBAT_WEAPON_TYPE aeType) {
    return fCombatWeaponsDPS[(int32_t)aeType];
}

void CombatState::ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel) {
    if (((int32_t)aeOldLevel) == 0) {
        if (aeNewLevel) {
            ClearCoverLocation(false);
            ClearCoverSearchInfo();
            ClearSecondaryCoverLocation(false);
            ClearSecondaryCoverSearchInfo();
            ClearCombatLocation();
        }
    }
}

void CombatState::UpdateCombatStrength() {
    Actor* packageOwner = pCombatController->GetAttacker();
    fDamagePerSecond = packageOwner->GetDamagePerSecond();
    fCombatStrength = packageOwner->CalculateCombatStrength(fDamagePerSecond);
}

bool CombatState::GetTargetUnreachable() {
    return (bool)((uint8_t)GetLocationUnreachable(3));
}

void CombatState::ClearTargetUnreachable() {
    ClearLocationUnreachable(3);
}

void CombatState::CheckReferences() {
    TargetedObject* targetedObject;
    uint32_t a2;
    CombatState::TargetedObject* v5 = pCombatItems[3];
    CombatState::TargetedObject* v4 = pCombatItems[4];
    for (a2 = 0; a2 < 5; ++a2) {
        targetedObject = GetTargetedObject(a2);
        if (targetedObject) {
            if (targetedObject->pRef) {
                if (targetedObject->pRef->GetDisabledOrDeleted()) {
                    RemoveTargetedObject(a2);
                }
            }
        }
    }
    if ((static_cast<const void*>(v5) != static_cast<const void*>(GetTargetedObject(3))) ||
        (static_cast<const void*>(v4) != static_cast<const void*>(GetTargetedObject(4)))) {
        RemoveTargetedObject(3);
        RemoveTargetedObject(4);
    }
}

void CombatState::ClearCovers() {
    if (pCoverLocation) {
        if (!pCoverLocation->bReserved) {
            ClearCoverLocation(false);
        }
    }
    if (pSecondaryCoverLocation) {
        if (!pSecondaryCoverLocation->bReserved) {
            ClearSecondaryCoverLocation(false);
        }
    }
}

bool CombatState::GetLastSeenTargetFiringLocation(NiPoint3& arLastSeenFiringLocation) {
    if (!GetLastSeenTargetLocation(arLastSeenFiringLocation)) {
        return false;
    }
    Actor* apRef = pCombatController->GetTarget();
    (&arLastSeenFiringLocation)->z =
        (CombatUtilities::GetTargetFiringZOffset(apRef, ACTOR_SEGMENT_INVIEW_TOP, true) +
         (&arLastSeenFiringLocation)->z);
    return true;
}

float CombatState::GetLocationTargetNearTargetDistance() {
    CombatLocationTarget* pCombatLocationTarget;
    if (pLocationTarget->bNearTarget) {
        pCombatLocationTarget = pLocationTarget;
        return (pCombatLocationTarget->fMaxDistance + (&fCombatGuardFollowBufferDistance)->Float());
    }
    else {
        return (float)(((pLocationTarget->fMaxDistance - pLocationTarget->fMinDistance) * 0.75) +
                       pLocationTarget->fMinDistance);
    }
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
    CombatController* pCombatController;
    if ((!apCoverLocation) || apCoverLocation->IsReserved()) {
        return true;
    }
    pCombatController = this->pCombatController;
    CombatCoverMap* parent = pCombatManager->CombatManager::GetCoverMap();
    return (bool)((int8_t)parent->TryReserveCoverLocation(pCombatController, apCoverLocation));
}

void CombatState::ReachedDestination(const BGSWorldLocation& arDestination, int32_t aiLocation) {
    CombatGroup* combatGroup;
    if (((uint32_t)aiLocation) == 0xFu) {
        combatGroup = pCombatController->CombatController::GetCombatGroup();
        combatGroup->InvestigatedSearchLocation(arDestination);
    }
    if (pCombatLocation) {
        if (pCombatLocation->GetLocation() == ((uint32_t)aiLocation)) {
            ClearCombatLocation();
        }
    }
}

double CombatState::GetLocationTargetMinDistance() {
    if (pLocationTarget) {
        return (float)(pLocationTarget->fMinDistance * 0.8999999761581421);
    }
    else {
        return (float)0.0;
    }
}

TESObjectWEAP* CombatState::GetBestCombatWeapon() {
    TESObjectWEAP* pWeapon;
    float fMaxDPS;
    int32_t i;
    float fSmallestDPS = (float)0.0;
    TESObjectWEAP* v6 = nullptr;
    for (i = 0; ((uint32_t)i) < 7; ++i) {
        fMaxDPS = (float)GetMaxDPSForWeaponType(static_cast<COMBAT_WEAPON_TYPE>(i));
        if (fSmallestDPS < ((double)fMaxDPS)) {
            fSmallestDPS = fMaxDPS;
            if (i == 6) {
                pWeapon = nullptr;
            }
            else {
                pWeapon = GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(i));
            }
            v6 = pWeapon;
        }
    }
    return v6;
}

bool CombatState::IsDetectionLost() {
    double elapsedTime = (double)(&fLastTargetDetectedTimeStamp)->ElapsedTime();
    return (bool)((uint8_t)((&fCombatDetectionLostTime)->Float() <= elapsedTime));
}

void CombatState::SetLocationTargetLocation(BGSWorldLocation* a1, uint8_t a2, float a3, float a4) {
    CombatLocationTarget* v7;
    ClearLocationTarget();
    CombatLocationTarget* apMemory = static_cast<CombatLocationTarget*>(ourMalloc(sizeof(CombatLocationTarget)));
    if (apMemory) {
        v7 = ::new (apMemory) CombatLocationTarget;
    }
    else {
        v7 = nullptr;
    }
    pLocationTarget = v7;
    pLocationTarget->cType = a2;
    pLocationTarget->pRef = nullptr;
    pLocationTarget->WorldLoc = *a1;
    float* v5 = &fCombatLocationTargetRadiusMin.Float();
    pLocationTarget->fMinDistance = BSMAX(a3, *v5);
    float* v6 = &fCombatLocationTargetRadiusMin.Float();
    pLocationTarget->fMaxDistance = BSMAX(a4, *v6);
}

void CombatState::UpdateCombatPermissions() {
    if (CombatPermissionsTimer5sec.Check()) {
        iCombatPermissions &= 0xFFFF00FFu;
        float* v1 = &fCombatAvoidThreatsChance.Float();
        if (RandomBool(*v1)) {
            iCombatPermissions |= 256;
        }
        CombatPermissionsTimer5sec.Reset(5.0f);
    }
    if (CombatPermissionsTimer10sec.Check()) {
        iCombatPermissions &= 0xFFFFFF00u;
        TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
        float SequenceWeight = CurrentCombatStyle->SimpleData.fCoverChance;
        float v3 = SequenceWeight / 100.0;
        if (RandomBool(v3)) {
            iCombatPermissions |= 1;
        }
        if (SequenceWeight > 0.0) {
            iCombatPermissions |= 2;
        }
        CombatPermissionsTimer10sec.Reset(10.0f);
    }
}

bool CombatState::CheckShouldUpdateDetection(Actor* apAttacker, Actor* apTarget) {
    if (apTarget != pLastTarget) {
        return true;
    }
    bool v5 = true;
    if (iTargetDetectionLevel > 0) {
        CombatProcedureBeInCover* pProcedure =
            static_cast<CombatProcedureBeInCover*>(pCombatController->pMovementProcedure);
        if (pProcedure) {
            if (pProcedure->GetType() == CombatProcedure::BE_IN_COVER) {
                return !pProcedure->GetBehindCover();
            }
        }
    }
    return v5;
}

void CombatState::ActivatedReference(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    int32_t aiCount,
    bool abActivatedScriptOnly
) {
    CombatTimer apMemory;
    if (pCombatController->IsFleeing()) {
        TESBoundObject* ObjectReference = apRef->GetObjectReference();
        if (ObjectReference->GetFormType() == 28) {
            iLastFleeDoor = apRef->GetFormID();
            float a2 = CombatTimer::GetTimer();
            ::new (&apMemory) CombatTimeStamp(a2);
            fLastFleeDoorTimeStamp.fTimeStamp = apMemory.fStartTime;
        }
    }
}

void CombatState::UsedCombatItem(TESBoundObject* apCombatItem) {
    for (uint32_t i = 0; i < 2; ++i) {
        if (pUsableCombatItems[i] == apCombatItem) {
            if (!i) {
                float ftime = fCombatItemRestoreTimer.Float();
            }
            float ftime_2 = fCombatItemBuffTimer.Float();
            UsableCombatItemTimers[i].Reset(ftime_2);
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
        kTargetLocation.z =
            CombatUtilities::GetTargetFiringZOffset(pTarget, ACTOR_SEGMENT_INVIEW_TOP, false) + kTargetLocation.z;
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
                pAttacker, pTarget, pProjectile, kAttackerLocation, kTargetLocation, NiPoint3_ZERO, arbHighArc,
                kVelocity, -1.0f
            )) {
            continue;
        }
        if (pExplosion) {
            float fTime = CombatUtilities::ComputeProjectileTime_(
                pProjectile, kAttackerLocation, kTargetLocation, arbHighArc, 0.0f
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
    double v9;
    NiPoint3 center;
    CombatAI_CoverSearch.Start();
    float fMaxHeightChange = 512.0f;
    float fMaxDist = 2048.0f;
    Actor* pattacker = pCombatController->pAttacker;
    Actor* pTarget = pCombatController->GetTarget();
    if (!pTarget) {
        return false;
    }
    float arfMinRange = 0.0f;
    float fMinDistToShootFrom = 512.0f;
    float arfMaxRange = 2048.0f;
    float arfAbsoluteMaxRange = 2048.0f;
    CombatUtilities::ComputeOptimalWeaponRanges(
        pCombatController, apWeapon, arfMinRange, arfMaxRange, arfAbsoluteMaxRange
    );
    float* v7 = &fCombatCoverMaxRangeMult.Float();
    arfMaxRange = arfMaxRange * *v7;
    arfMaxRange = BSMIN(arfMaxRange, arfAbsoluteMaxRange);
    bool v33 = false;
    bool v40 = false;
    bool v42 = false;
    bool v43 = false;
    switch (aiLocation) {
        case 2: {
            TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
            fMaxDist = CurrentCombatStyle->SimpleData.fCoverSearchRadius;
            break;
        }
        case 4: {
            v43 = true;
            v33 = true;
            break;
        }
        case 5: {
            fMaxDist = 4096.0f;
            arfMaxRange = 10000.0f;
            v43 = true;
            v40 = true;
            v42 = true;
            break;
        }
        case 10: {
            arfMaxRange = 10000.0f;
            v43 = true;
            break;
        }
        case 12: {
            fMinDistToShootFrom = fCombatFleeMinCoverDistance.Float();
            fMaxDist = fCombatFleeMaxCoverDistance.Float();
            arfMaxRange = 10000.0f;
            v40 = true;
            v42 = true;
            v43 = true;
            break;
        }
        default: {
            break;
        }
    }
    COVER_STATE ecurrentCoverState = COVER_STATE_NONE;
    v9 = 0.0;
    float arfSquaredDistance = 0.0f;
    if (apCombatCoverLocation->iCoverLocationKey) {
        apCoverSearchInfo->uiPreviousCoverKey = apCombatCoverLocation->iCoverLocationKey;
        apCoverSearchInfo->uiPreviousState = 0;
        if (aiLocation == apCombatCoverLocation->iLocation) {
            float fmodifiedMinDistToShootFrom = fMinDistToShootFrom / 2.0;
            v9 = fmodifiedMinDistToShootFrom;
            if (CheckCoverLocation(
                    apCombatCoverLocation->CoverLocation, apRetLocation, &apCombatCoverLocation->ReservationInfo,
                    apCombatCoverLocation->CoverSearchTargetLocation, apCombatCoverLocation->iCoverLocationKey,
                    apCombatCoverLocation->iCoverLocationKey, aiLocation, pattacker, pTarget,
                    fmodifiedMinDistToShootFrom, arfMaxRange, ecurrentCoverState, arfSquaredDistance, v43
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
        v9 = GetLocationTargetDistance();
        fMaxDist = v9;
    }
    else {
        center = pattacker->GetLocationOnReference();
    }
    PathingRequestCover v32;
    PathingLocation apMemory(center, pattacker);
    v32.SetOrigin(apMemory);
    if (aiLocation == 5) {
        NiPoint3 targetCenter;
        NiPoint3 memberCenter;
        float fsquaredRadius = pCombatController->pCombatGroup->ComputeTargetBoundingSphere(targetCenter);
        float fsquaredRadius2 = pCombatController->pCombatGroup->ComputeMemberBoundingSphere(memberCenter);
        apCoverSearchInfo->kSearchLocation = targetCenter;
        NiPoint3 distanceProjectionVector = memberCenter - targetCenter;
        v32.SetDistanceProjectionVector(distanceProjectionVector);
    }
    else {
        v9 = fLastTarget360LineOfSightTimeStamp.ElapsedTime();
        apCoverSearchInfo->bSearching = fCoverEvaluationLastSeenExpireTime.Float() > v9;
        if (apCoverSearchInfo->bSearching) {
            apCoverSearchInfo->kSearchLocation = LastSeenTargetLocation;
        }
        else {
            apCoverSearchInfo->kSearchLocation = pTarget->GetLocationOnReference();
        }
    }
    v32.SetThreat(apCoverSearchInfo->kSearchLocation);
    v32.SetMaxHeightChange(fMaxHeightChange);
    v32.SetMaxCoverDistance(fMaxDist);
    v32.SetMaxResults(200u);
    v32.SetFindBehindTarget(v33);
    v32.SetSortFarthestFirst(v40);
    v32.SetSortDistanceFromTarget(v42);
    float a2 = pCombatController->fAttackerHeight;
    v32.SetActorHeight(a2);
    float a2a = pCombatController->fAttackerSneakHeight;
    v32.SetCrouchHeight(a2a);
    v32.bCanCrouch = pCombatController->GetAttackerCanSneak();
    v32.SetHideCover(v43);
    apCombatCoverLocation->iLocation = aiLocation;
    apCombatCoverLocation->CoverSearchTargetLocation = apCoverSearchInfo->kSearchLocation;
    float a2b = CombatTimer::GetTimer();
    CombatTimeStamp v17(a2b);
    apCoverSearchInfo->kLastSearch = v17;
    apCoverSearchInfo->nipoint3C = center;
    apCoverSearchInfo->float18 = fMaxDist;
    apCoverSearchInfo->kCoverLocations.Clear(false);
    if (Pathing::GetCoverPoints(&v32, apCoverSearchInfo->kCoverLocations)) {
        pCombatManager->pCoverMap->LockForWrite();
        for (uint32_t i = 0;; ++i) {
            uint32_t FormFlags = apCoverSearchInfo->kCoverLocations.iSize;
            if (i >= FormFlags) {
                break;
            }
            PathingCoverLocation* coverLoc = &apCoverSearchInfo->kCoverLocations.GetAt(static_cast<int32_t>(i));
            COVER_STATE estate = COVER_STATE_NONE;
            float fsquaredDistToTarget = 0.0f;
            uint32_t icoverKey = coverLoc->GetKey();
            if (icoverKey == apCombatCoverLocation->iCoverLocationKey) {
                coverLoc->cState = ecurrentCoverState;
            }
            else {
                uint32_t Time;
                uint32_t ilastMoveTime = coverLoc->GetDynamicCoverObstacleTimeStamp();
                if (ilastMoveTime &&
                    (Time = appTimer.iLastTime, static_cast<double>(Time - ilastMoveTime) / 1000.0 < 3.0)) {
                    coverLoc->cState = 1;
                }
                else {
                    CoverReservationInfo reservationInfo;
                    if (CheckCoverLocation(
                            *coverLoc, apRetLocation, &reservationInfo,
                            apCombatCoverLocation->CoverSearchTargetLocation, icoverKey, 0, aiLocation, pattacker,
                            pTarget, fMinDistToShootFrom, arfMaxRange, estate, fsquaredDistToTarget, v43
                        )) {
                        coverLoc->cState = 19;
                        apCombatCoverLocation->CoverLocation.operator=(*coverLoc);
                        apCombatCoverLocation->iCoverLocationKey = icoverKey;
                        apCombatCoverLocation->iCoverLocationLastMovedTimeStamp = ilastMoveTime;
                        apCombatCoverLocation->ReservationInfo = reservationInfo;
                        apCombatCoverLocation->cCoverHeight = apCombatCoverLocation->CoverLocation.GetHeight();
                        apCombatCoverLocation->bDirtyFlag = false;
                        DynamicCoverLostTimer.Clear();
                        CombatAI_CoverSearch.Stop();
                        return true;
                    }
                    coverLoc->cState = estate;
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
    float fminAdvanceDistance;
    // These three limits are set outside coveredMoveRequest and never read, as the game does.
    [[maybe_unused]] float fMaxHeightChange = 1024.0f;
    [[maybe_unused]] float fMaxDist = 2048.0f;
    [[maybe_unused]] int32_t iMaxCoverLocations = 200;
    Actor* pattacker = pCombatController->pAttacker;
    TESObjectREFR* ptarget = (TESObjectREFR*)pCombatController->GetTarget();
    if (afMinDistance == 0.0) {
        fminAdvanceDistance = 256.0f;
    }
    else {
        fminAdvanceDistance = afMinDistance;
    }
    float fminAdvanceDistance_2 = fminAdvanceDistance;
    if (aiLocation == 6) {
        float* v8 = &fCoveredAdvanceMinAdvanceDistanceMin.Float();
        TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
        double v15 = 1.0 - CurrentCombatStyle->SimpleData.fCoverChance / 100.0;
        float* v10 = &fCoveredAdvanceMinAdvanceDistanceMax.Float();
        fminAdvanceDistance_2 = (*v10 - fCoveredAdvanceMinAdvanceDistanceMin.Float()) * v15 + *v8;
    }
    apCoverSearchInfo->uiPreviousCoverKey = 0;
    if (apCombatCoverLocation->iCoverLocationKey) {
        if (aiLocation == apCombatCoverLocation->iLocation) {
            bool bLeftEdge = apCombatCoverLocation->ReservationInfo.bLeftEdge;
            float fRadius = pattacker->GetRadius();
            if (pCombatManager->pCoverMap->CanReserveCoverLocation(
                    apCombatCoverLocation->iCoverLocationKey, &apCombatCoverLocation->CoverLocation, pCombatController,
                    fRadius, &apCombatCoverLocation->ReservationInfo, true, bLeftEdge
                )) {
                return true;
            }
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
    bool bresult = CombatState::FindBestAdvanceLocation(&coveredMoveRequest, fminAdvanceDistance_2, false);
    if (bresult) {
        NiPoint3* ScriptList = &coveredMoveRequest.CoverLocation;
        apRetLocation->SetPoint(*ScriptList, nullptr);
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
    if (pCoverLocation) {
        if (pCoverLocation->bReserved) {
            apCoveredMoveRequest->GetUnreachableCoverLocations()->Add(pCoverLocation->iCoverLocationKey);
        }
    }
}

bool CombatState::FindCoveredMoveLocation(
    PathingCoverLocation* apCoverLocation,
    const NiPoint3& arDestination,
    uint32_t aiLocation,
    float afMinDistance,
    PathingAvoidNodeArray* apPathingAvoidNodeArray
) {
    CombatCoverLocation* v7;
    ClearSecondaryCoverLocation(false);
    CombatCoverLocation* apMemory = static_cast<CombatCoverLocation*>(ourMalloc(sizeof(CombatCoverLocation)));
    if (apMemory) {
        v7 = ::new (apMemory) CombatCoverLocation;
    }
    else {
        v7 = nullptr;
    }
    pSecondaryCoverLocation = v7;
    CombatLocation coverLocation;
    if (FindBestAdvanceLocation(
            &SecondaryCoverSearchInfo, pSecondaryCoverLocation, &coverLocation, aiLocation, arDestination,
            afMinDistance, apPathingAvoidNodeArray
        )) {
        pSecondaryCoverLocation->iCoverLocationKey = pSecondaryCoverLocation->CoverLocation.GetKey();
        pSecondaryCoverLocation->iCoverLocationLastMovedTimeStamp =
            pSecondaryCoverLocation->CoverLocation.GetDynamicCoverObstacleTimeStamp();
        apCoverLocation->operator=(pSecondaryCoverLocation->CoverLocation);
        return true;
    }
    else {
        ClearSecondaryCoverLocation(false);
        return false;
    }
}

bool CombatState::FindOptimalLocation(
    float afMinRange,
    float afMaxRange,
    float afAbsoluteMinRange,
    float afAbsoluteMaxRange,
    CombatLocation* apRetLocation,
    bool abCheckTargetLOS
) {
    double v10;
    NiPoint3 a2;
    NiPoint3 result;
    NiPoint3 arLocation;
    // Filled by Pathing::FindClosestReachableLocation; nothing stores into it before.
    FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
    NiPoint3 arTargetPos;
    NiPoint3 ecx0;
    NiPoint3 v30;
    Actor* apActor = pCombatController->GetAttacker();
    TESObjectREFR* apTarget = pCombatController->GetTarget();
    float v28 = (afMinRange + afMaxRange) / 2.0;
    NiPoint3 v23 = apActor->GetLocationOnReference();
    arTargetPos = apTarget->GetLocationOnReference();
    result = arTargetPos - v23;
    float Length = result.UnitizeGetLength();
    arLocation = arTargetPos - result * v28;
    float v13 = v28 - Length;
    float v26 = std::fabs(v13);
    PathingLocation v22(apActor);
    TESObjectCELL* Cell = v22.GetCell();
    TESWorldSpace* ShaderPropertyType = v22.GetWorldspaceSimple();
    if (Pathing::FindClosestPointOnNavmesh(ShaderPropertyType, Cell, arLocation, &ecx0) &&
        (v30 = ecx0 - arLocation, (v10 = v30.SqrLength(), !(v26 * v26 < v10)))) {
        PathingLocation apMemory(ecx0, apActor);
        float v12 = v26 + v26;
        Pathing::FindClosestReachableLocation(&apLocation, v22, apMemory, v12);
        a2 = apLocation.GetLocation();
        result = a2 - arTargetPos;
        float v27 = result.SqrLength();
        if (!(v27 < afAbsoluteMinRange * afAbsoluteMinRange) &&
            (!(v27 > afAbsoluteMaxRange * afAbsoluteMaxRange) &&
             (!abCheckTargetLOS || CombatUtilities::CalculateLOS(apActor, apTarget, a2, arTargetPos, false, -1)))) {
            apRetLocation->SetPoint(a2, nullptr);
            apLocation.~PathingLocation();
            return true;
        }
        else {
            apLocation.~PathingLocation();
            return false;
        }
    }
    else {
        return false;
    }
}

bool CombatState::FindBestFleeLocation(CombatCoverLocation* arg0, CombatLocation* arg4, int32_t arg8) {
    BGSWorldLocation apResult;
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
        float* v5 = &fCombatFleeMaxAllyDistance.Float();
        float fclosestHelperDistanceSquared = *v5 * fCombatFleeMaxAllyDistance.Float();
        LoadedReferenceMap* pactorMap = &pLoadedReferenceCollection->ActorsMap;
        NiTMapItem<uint32_t, TESObjectREFR*>* it = pactorMap->GetFirstPos();
        while (it) {
            uint32_t iformID = 0;
            TESObjectREFR* pref = nullptr;
            pactorMap->GetNext(it, iformID, pref);
            if (!pref->GetDisabledOrDeleted() && pref->IsActor() && !static_cast<Actor*>(pref)->IsDead(false) &&
                pref != pattacker && pref != ptarget) {
                Actor* phelper = static_cast<Actor*>(pref);
                phelper->GetWorldLocation(&helperWorldLoc);
                float fsquaredDistance = attackerWorldLoc.GetDistanceSquared(helperWorldLoc);
                float* v6 = &fCombatFleeMinAllyDistance.Float();
                if (!(fsquaredDistance < *v6 * fCombatFleeMinAllyDistance.Float())) {
                    if (!(fclosestHelperDistanceSquared < static_cast<double>(fsquaredDistance))) {
                        FIGHT_REACTION ereaction;
                        float ftargetHelperDistanceSquared = targetWorldLoc.GetDistanceSquared(helperWorldLoc);
                        float ftargetAttackerDistanceSquared = targetWorldLoc.GetDistanceSquared(attackerWorldLoc);
                        if (!(ftargetHelperDistanceSquared < static_cast<double>(ftargetAttackerDistanceSquared)) &&
                            CheckMovement(helperWorldLoc, true, false) && !phelper->IsFleeing(false) &&
                            (phelper->GetShouldAttackActor(ptarget, false, ereaction, false) ||
                             pCombatManager->GetWanttoHelpGroup(phelper, pattacker)) &&
                            !phelper->GetShouldAttackActor(pattacker, false, ereaction, false)) {
                            pclosestHelper = phelper;
                            fclosestHelperDistanceSquared = fsquaredDistance;
                        }
                    }
                }
            }
        }
        if (pclosestHelper) {
            arg4->SetRef(pclosestHelper);
            double Radius = pclosestHelper->GetRadius();
            double v19 = pattacker->GetRadius() + Radius;
            float v18 = fCombatMoveToActorBufferDistance.Float() + v19;
            arg4->SetRadius(v18);
            return true;
        }
    }
    TESActorBase* pactorBase = pattacker->GetActorBase();
    if (static_cast<uint8_t>(pactorBase->HasLowLevelProcessing())) {
        if ((!pattacker->IsCreature() || !pactorBase->GetCantOpenDoors()) && arg8 != 10) {
            if (!cFleeState) {
                if (RandomBool(fCombatFleeInitialDoorRestrictChance.Float())) {
                    CombatTimeStamp kTimeStamp(CombatTimer::GetTimer());
                    fLastFleeDoorTimeStamp = kTimeStamp;
                }
            }
            // ElapsedTime is called before the setting's Float, and its result is kept as a double.
            double fElapsedTime = fLastFleeDoorTimeStamp.ElapsedTime();
            if (fCombatFleeUseDoorRestrictTime.Float() < fElapsedTime) {
                TESObjectREFR* pclosestDoor = nullptr;
                NiPoint3 apArray_20 = pattacker->GetLocationOnReference();
                float apArray_16 = fCombatFleeMaxDoorDistance.Float();
                float fclosestDoorDistanceSquared = apArray_16 * apArray_16;
                using DoorArray = BSSimpleArray<TESObjectREFR*, 1024>;
                DoorArray v37;
                TESForm* Space = pattacker->GetSpace();
                if (Pathing::FindTeleportDoorsForLocation(Space, apArray_20, apArray_16, &v37)) {
                    for (uint32_t i = 0;; ++i) {
                        uint32_t FormFlags = v37.iSize;
                        if (i >= FormFlags) {
                            break;
                        }
                        TESObjectREFR* pdoor = v37.operator[](i);
                        if (!pdoor->GetDisabledOrDeleted() && pdoor->iFormID != iLastFleeDoor) {
                            DoorTeleportData* pteleportData = pdoor->GetTeleport();
                            TESObjectREFR* plinkedDoor = pteleportData->pLinkedDoor;
                            if (plinkedDoor->iFormID != iLastFleeDoor && !plinkedDoor->GetDisabledOrDeleted()) {
                                BGSEncounterZone* pdoorZone;
                                BGSEncounterZone* pactorZone;
                                if (!bEncounterZoneTargetRestrict.Bool()) {
                                    goto LABEL_44;
                                }
                                pdoorZone = plinkedDoor->GetEncounterZone();
                                pactorZone = pattacker->GetEncounterZone();
                                if (!pdoorZone) {
                                    pdoorZone = BGSEncounterZone::GetNoZone();
                                }
                                if (!pactorZone) {
                                    pactorZone = BGSEncounterZone::GetNoZone();
                                }
                                if (pdoorZone == pactorZone) {
                                LABEL_44:;
                                    pdoor->GetWorldLocation(&doorWorldLocation);
                                    if (CheckMovement(doorWorldLocation, true, false)) {
                                        BGSWorldLocation* TeleportWorldLocation =
                                            pteleportData->GetTeleportWorldLocation(&apResult);
                                        if (CheckMovement(*TeleportWorldLocation, true, true)) {
                                            float* v12 = &fCombatFleeDoorTargetCheckDistance.Float();
                                            if (!pCombatController->pCombatGroup->FindTargetNearLocation(
                                                    doorWorldLocation, *v12, false
                                                )) {
                                                NiPoint3 doorLoc = pdoor->GetLocationOnReference();
                                                NiPoint3 doorVec = doorLoc - apArray_20;
                                                float fdistanceSquared_2 = doorVec.SqrLength();
                                                if (fclosestDoorDistanceSquared >
                                                    static_cast<double>(fdistanceSquared_2)) {
                                                    fclosestDoorDistanceSquared = fdistanceSquared_2;
                                                    pclosestDoor = pdoor;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (pclosestDoor) {
                    DoorTeleportData* pteleportData_2 = pclosestDoor->GetTeleport();
                    TESForm* pspace = (TESForm*)pteleportData_2->GetTeleportWorldSpace();
                    if (!pspace) {
                        pspace = (TESForm*)pteleportData_2->GetTeleportCell();
                    }
                    arg4->SetPoint(*&pteleportData_2->position, pspace);
                    return true;
                }
            }
        }
    }
    if (arg0 && FindBestCoverLocation(nullptr, &CoverSearchInfo, arg0, arg4, arg8)) {
        return true;
    }
    if (pLocationTarget && pLocationTarget->cType == 1) {
        if (pLocationTarget->pRef) {
            arg4->SetRef(pLocationTarget->pRef);
            if (pLocationTarget->pRef->IsActor()) {
                double v17 = static_cast<Actor*>(pLocationTarget->pRef)->GetRadius();
                double v16 = pattacker->GetRadius() + v17;
                float v15 = fCombatMoveToActorBufferDistance.Float() + v16;
                arg4->SetRadius(v15);
            }
        }
        else {
            arg4->SetLocation(pLocationTarget->WorldLoc);
        }
        return true;
    }
    else {
        arg4->SetFlag(1u, true);
        return true;
    }
}

bool CombatState::FindLOSLocation(CombatLocation* apRetLocation, int32_t a3) {
    BGSWorldLocation memberWorldLoc;
    CombatController* pcombatController;
    Actor* pactor;
    BGSWorldLocation attackerWorldLoc;
    CombatGroup* pgroup = pCombatController->pCombatGroup;
    if (!apRetLocation->pRef) {
        goto LABEL_9;
    }
    if (!apRetLocation->pRef->IsActor()) {
        goto LABEL_9;
    }
    pactor = static_cast<Actor*>(apRetLocation->pRef);
    if (!pgroup->IsMember(pactor)) {
        goto LABEL_9;
    }
    pcombatController = pactor->GetCombatController();
    if (pcombatController && pcombatController->pCombatState->GetHasBuffered360LineOfSight()) {
        return true;
    }
    if (pactor->Has360LOS(pCombatController->GetTarget())) {
        return true;
    }
LABEL_9:;
    Actor* pattacker = pCombatController->pAttacker;
    pattacker->GetWorldLocation(&attackerWorldLoc);
    Actor* plosMember = nullptr;
    float fmaxDistanceSquared = 67108864.0f;
    uint32_t imemberCount = pgroup->GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        Actor* pmember = pgroup->GetMember(i);
        if (pmember != pattacker) {
            bool bhasLOS = false;
            CombatController* pcombatController_2 = pmember->GetCombatController();
            if (pcombatController_2 && pcombatController_2->pCombatState->GetHasBuffered360LineOfSight()) {
                bhasLOS = true;
            }
            else {
                if (pmember->Has360LOS(pCombatController->GetTarget())) {
                    bhasLOS = true;
                }
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
    double Radius = plosMember->GetRadius();
    double v5 = pattacker->GetRadius() + Radius;
    float v4 = fCombatMoveToActorBufferDistance.Float() + v5;
    apRetLocation->SetRadius(v4);
    return true;
}

bool CombatState::FindBetterWeapon() {
    float fbaseWeaponDPS;
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
    BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
    ItemChange* pcurrentWeaponItem = CurrentAIProcess->GetCurrentWeapon();
    if (pcurrentWeaponItem) {
        if (pcurrentWeaponItem->pExtraObjectList) {
            if (pcurrentWeaponItem->pExtraObjectList->m_item) {
                ExtraDataList** v2 = &pcurrentWeaponItem->pExtraObjectList->m_item;
                if ((*v2)->GetCanNotWear()) {
                    return false;
                }
            }
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
    if (btargetUnreachable) {
        fbaseWeaponDPS = fMaxRangedWeaponDPS;
    }
    else {
        fbaseWeaponDPS = fMaxCombatWeaponDPS;
    }
    float* v3 = &fPickupWeaponMinDPSImprovementPercent.Float();
    float fweaponDPS = (*v3 + 1.0) * fbaseWeaponDPS;
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
                float* v4 = &fPickupWeaponRangedDistanceMax.Float();
                if (fpickupWeaponDistanceMax < static_cast<double>(*v4)) {
                    float* v5 = &fPickupWeaponRangedDistanceMax.Float();
                    fpickupWeaponDistanceMax =
                        (*v5 - fpickupWeaponDistanceMax) * frangedMeleeDPSRatio + fpickupWeaponDistanceMax;
                }
            }
        }
    }
    fpickupWeaponDistanceMin = fpickupWeaponDistanceMin * fdistanceMult;
    fpickupWeaponDistanceMax = fpickupWeaponDistanceMax * fdistanceMult;
    float ffoundWeaponDPS = 0.0f;
    float ffoundWeaponTotalDistanceSquared = 3.4028235e+38f;
    FindObjectIterator weaponIterator(WEAP_ID, pattacker);
    TESObjectREFR* pweaponRef = nullptr;
    ItemChange* pweaponItemChange = nullptr;
    while (pLoadedReferenceCollection->GetNextObject(weaponIterator, pweaponRef, pweaponItemChange, 3u)) {
        float fhealth;
        if (pweaponItemChange) {
            fhealth = pweaponItemChange->GetItemHealth(false);
        }
        else {
            fhealth = (&pweaponRef->m_Extra)->GetHealth();
        }
        float v33 = fhealth;
        if (!pweaponRef->GetDestroyed() && v33 != 0.0) {
            TESObjectWEAP* FormFlags;
            if (pweaponItemChange) {
                FormFlags = static_cast<TESObjectWEAP*>(pweaponItemChange->pContainerObj);
            }
            else {
                FormFlags = static_cast<TESObjectWEAP*>(pweaponRef->GetObjectReference());
            }
            TESObjectWEAP* pweapon_2 = FormFlags;
            float fcondition = 1.0f;
            if (v33 != -1.0) {
                int64_t fTotalHealth = TESHealthForm::GetFormHealth(pweapon_2);
                fcondition = v33 / static_cast<double>(fTotalHealth);
            }
            TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
            float fnewWeaponDPS = CombatFormulas::CalcCombatWeaponDPS(
                pattacker, pweapon_2, fcondition, CurrentCombatStyle, nullptr, nullptr
            );
            if (pweapon_2->IsMeleeWeapon()) {
                float* v8 = &fPickupWeaponMeleeWeaponDPSMult.Float();
                fnewWeaponDPS = fnewWeaponDPS * *v8;
            }
            if (!(fweaponDPS >= static_cast<double>(fnewWeaponDPS)) &&
                (!btargetUnreachable || pweapon_2->IsRangedWeapon()) && pattacker->CanUseWeapon(pweapon_2, false)) {
                float fsquaredMaxDistance = 0.0;
                if (!(fweaponDPS <= 0.0)) {
                    float fnewWeaponDPSRatio = fnewWeaponDPS / fweaponDPS;
                    float* v9 = &fPickupWeaponDistanceMinMaxDPSMult.Float();
                    fnewWeaponDPSRatio = BSMIN(fnewWeaponDPSRatio, *v9);
                    double v13 = fnewWeaponDPSRatio - 1.0;
                    float v30 = v13 / (fPickupWeaponDistanceMinMaxDPSMult.Float() - 1.0);
                    float v32 = (fpickupWeaponDistanceMax - fpickupWeaponDistanceMin) * v30 + fpickupWeaponDistanceMin;
                    fsquaredMaxDistance = v32 * v32;
                }
                else {
                    fsquaredMaxDistance = fpickupWeaponDistanceMax * fpickupWeaponDistanceMax;
                }
                pweaponRef->GetWorldLocation(&weaponWorldLoc);
                float ftotalDistanceSquared = weaponWorldLoc.GetDistanceSquared(attackerWorldLoc);
                if (!(fsquaredMaxDistance <= static_cast<double>(ftotalDistanceSquared)) &&
                    CheckMovement(weaponWorldLoc, true, false)) {
                    LockData* Lock;
                    TESObjectWEAP* pweapon_4;
                    if (!pweaponRef->GetObjectReference() ||
                        (pweaponRef->GetObjectReference()->GetFormType() != CONT_ID ||
                         ((pweapon_4 = static_cast<TESObjectWEAP*>(pweaponRef->GetObjectReference())) == nullptr ||
                          ((!pweaponRef->GetLock() || (Lock = pweaponRef->GetLock(), !Lock->IsLocked())) &&
                           (!pweaponRef->GetOwner() || pweaponRef->IsAnOwner(pattacker, true)))))) {
                        TESObjectREFR* ptempNewAmmo = nullptr;
                        bool bhasAmmo = true;
                        TESAmmo* pammo = pweapon_2->GetCurrentAmmo(pattacker);
                        if (pammo) {
                            bhasAmmo = false;
                            if (!pweaponItemChange) {
                                bhasAmmo = (&pweaponRef->m_Extra)->GetAmmoExtra() != nullptr;
                            }
                            if (!bhasAmmo) {
                                bhasAmmo = pattacker->GetInventoryObjectCount(pammo) != 0;
                            }
                            if (!bhasAmmo) {
                                if (pweaponItemChange) {
                                    if (pweaponRef->GetInventoryObjectCount(pammo)) {
                                        bhasAmmo = true;
                                        ptempNewAmmo = pweaponRef;
                                    }
                                }
                            }
                            if (!bhasAmmo) {
                                float fsquaredTotalDistance = 3.4028235e+38f;
                                NiPoint3 weaponLoc = pweaponRef->GetLocationOnReference();
                                FindObjectIterator ammoIterator(pammo, pattacker);
                                TESObjectREFR* pammoRef = nullptr;
                                ItemChange* pammoItemChange = nullptr;
                                while (pLoadedReferenceCollection->GetNextObject(
                                    ammoIterator, pammoRef, pammoItemChange, 3u
                                )) {
                                    pammoRef->GetWorldLocation(&ammoWorldLoc);
                                    double fAmmoDistance = ammoWorldLoc.GetDistanceSquared(attackerWorldLoc);
                                    float fnewSquaredTotalDistance =
                                        ammoWorldLoc.GetDistanceSquared(weaponWorldLoc) + fAmmoDistance;
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
                                    buseWeapon =
                                        ffoundWeaponTotalDistanceSquared > static_cast<double>(ftotalDistanceSquared);
                                }
                            }
                            else {
                                buseWeapon = true;
                            }
                            if (buseWeapon) {
                                ffoundWeaponDPS = fnewWeaponDPS * 10000.0 / ftotalDistance;
                                ffoundWeaponTotalDistanceSquared = ftotalDistanceSquared;
                                pnewWeapon = pweaponRef;
                                pnewWeaponObject = pweaponRef != nullptr ? pweapon_2 : nullptr;
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
    else {
        return false;
    }
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
        CombatState::TargetedObject* pItem;
        void* pMemory = ourMalloc(sizeof(CombatState::TargetedObject));
        if (pMemory) {
            pItem = ::new (pMemory) CombatState::TargetedObject;
        }
        else {
            pItem = nullptr;
        }
        pCombatItems[auiIndex] = pItem;
        pCombatItems[auiIndex]->pRef = (TESForm*)apReference;
        pCombatItems[auiIndex]->pObject = apObject;
        apReference->SetTargeted(true);
    }
}

double CombatState::GetLocationTargetDistance() {
    if (CheckUseLocationTargetMaxDistance()) {
        return pLocationTarget->fMaxDistance;
    }
    else {
        return GetLocationTargetMinDistance();
    }
}

float CombatState::GetLocationTargetCheckMovementDistance() {
    CombatGoal* pGoal = pCombatController->pCurrentGoal;
    if (!pGoal) {
        return this->pLocationTarget->fMaxDistance;
    }
    CombatGoal::COMBAT_GOAL egoal = pGoal->eType;
    if (egoal == CombatGoal::COMBAT_GOAL_HIDE_FROM_TARGET ||
        (egoal == CombatGoal::COMBAT_GOAL_GUARD || egoal == CombatGoal::COMBAT_GOAL_STAY_IN_COMBAT_AREA)) {
        return this->pLocationTarget->fMaxDistance;
    }
    else {
        CombatLocationTarget* pLocationTarget = this->pLocationTarget;
        return pLocationTarget->fMaxDistance + fCombatGuardFollowBufferDistance.Float();
    }
}

void CombatState::AddUnreachableCoverLocation(uint32_t aiCoverKey) {
    if (aiCoverKey) {
        uint32_t iindex = UnreachableCoverLocations.AddUninitialized();
        (&UnreachableCoverLocations.GetAt(static_cast<int32_t>(iindex)))->iCoverKey = aiCoverKey;
        float a2 = CombatTimer::GetTimer();
        CombatTimeStamp apMemory(a2);
        UnreachableCoverLocation* v2 = &UnreachableCoverLocations.GetAt(static_cast<int32_t>(iindex));
        v2->fTimeStamp = apMemory;
    }
}

void CombatState::BuildUnreachableCoverLocationsArray(BSSimpleArray<uint32_t, 1024>* apUnreachableCoverLocationsArray) {
    apUnreachableCoverLocationsArray->SetReservedSize(UnreachableCoverLocations.iSize);
    for (int32_t i = 0; i < UnreachableCoverLocations.iSize; ++i) {
        UnreachableCoverLocation* unreachableCoverLocation =
            &UnreachableCoverLocations.operator[](static_cast<uint32_t>(i));
        if (!(unreachableCoverLocation->fTimeStamp.ElapsedTime() > 10.0)) {
            apUnreachableCoverLocationsArray->Add(unreachableCoverLocation->iCoverKey);
        }
        else {
            UnreachableCoverLocations.RemoveFast(i--, 1u);
        }
    }
}

bool CombatState::IsLocationUnreachable(const BGSWorldLocation& arWorldLocation, float afRadius) {
    float fradiusSquared;
    if (afRadius == -1.0) {
        fradiusSquared = 16384.0f;
    }
    else {
        fradiusSquared = afRadius * afRadius;
    }
    for (int32_t i = 0; i < UnreachableLocations.iSize; ++i) {
        UnreachableLocation* unreachableLocation = &UnreachableLocations.operator[](i);
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

// KEEP: src cannot express it exactly. The exe builds BuildPath's two smart pointer arguments (0x6EBF30, 0x633C90)
// before it calls PathManager::QInstance (0x47D0B0) for the object; C++17 evaluates the object expression of a member
// call before its arguments, so clang calls QInstance first.
bool CombatState::QueuePathCheck(int32_t aiLocation) {
    Actor* pattacker = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    if (!ptarget) {
        return false;
    }
    if (!this->spPathCheckQueue.m_pObject) {
        ActorPathingMessageQueue* pQueue;
        void* apMemory = NiMemObject::operator new(sizeof(ActorPathingMessageQueue));
        if (apMemory) {
            pQueue = ::new (apMemory) ActorPathingMessageQueue;
        }
        else {
            pQueue = nullptr;
        }
        this->spPathCheckQueue.operator=(pQueue);
    }
    if (aiLocation) {
        if (aiLocation == 1) {
            const NiPoint3* pTargetLocation = &ptarget->GetLocationOnReference();
            NiPoint3 faceTargetLocation =
                CombatUtilities::ComputeFaceTargetLocation(pattacker, ptarget, *pTargetLocation, -1.0f);
            PathingLocation pathingLocationTarget(faceTargetLocation, pattacker);
            if (!pathingLocationTarget.IsValidLocation(pattacker->GetRadius())) {
                return false;
            }
            PathingRequest* prequest;
            void* pRequestMemory = NiMemObject::operator new(sizeof(PathingRequest));
            if (pRequestMemory) {
                prequest = ::new (pRequestMemory) PathingRequest;
            }
            else {
                prequest = nullptr;
            }
            pattacker->BuildRequest(
                prequest, pathingLocationTarget.GetLocation(), pathingLocationTarget.pCell,
                pathingLocationTarget.pWorldSpace, -1.0f, nullptr
            );
            // BuildPath takes both smart pointers by value: the queue is copied into its argument, then the request's
            // is built from the raw pointer, and the callee releases them.
            PathManager::QInstance()->BuildPath(NiPointer<PathingRequest>(prequest), spPathCheckQueue, true);
        }
    }
    else {
        PathingRequest* prequest;
        void* pRequestMemory = NiMemObject::operator new(sizeof(PathingRequest));
        if (pRequestMemory) {
            prequest = ::new (pRequestMemory) PathingRequest;
        }
        else {
            prequest = nullptr;
        }
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
    uint32_t isaveGameCount_3;
    uint32_t isaveGameCount_2;
    uint32_t isaveGameCount;
    uint8_t ccombatItems;
    uint8_t csavedData;
    apSaveGameBuffer->SaveDataEndian(this, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iLocationUnreachable, 4u, 0);
    for (uint32_t i = 0; i < 6; ++i) {
        apSaveGameBuffer->SaveFormID(pCombatWeapons[i], 0);
    }
    // The address &EmbeddedWeaponsArray is never null, so it is not tested.
    isaveGameCount = EmbeddedWeaponsArray.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t auiIndex = 0; auiIndex < isaveGameCount; ++auiIndex) {
        apSaveGameBuffer->SaveFormID(EmbeddedWeaponsArray.operator[](auiIndex), 0);
    }
    apSaveGameBuffer->SaveFormID(pCurrentEmbeddedWeapon, 0);
    apSaveGameBuffer->SaveDataEndian(&iHasCombatWeapon, 4u, 0);
    for (uint32_t j = 0; j < 7; ++j) {
        apSaveGameBuffer->SaveDataEndian(&fCombatWeaponsDPS[j], 4u, 0);
    }
    apSaveGameBuffer->SaveDataEndian(&fMaxCombatWeaponDPS, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fMaxRangedWeaponDPS, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fDisarmedDPS, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fBetterWeaponOutOfRangeDistance, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iTargetDetectionLevel, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetLineOfSight, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bTarget360LineOfSight, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetBlocked, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cTargetFlushed, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetSpaceDifferent, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bHasHandToHandEquip, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bGrenadeThrowHighArc, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bInventoryDirty, 1u, 0);
    apSaveGameBuffer->SaveFormID(pExplosiveProjectileBlocked, 0);
    apSaveGameBuffer->SaveDataEndian(&eTargetSegmentInView, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fThreatScore, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&LastSeenTargetLocation, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&iCombatPermissions, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&cStartingConfidence, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cCurrentConfidence, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&fConfidenceModifier, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fCombatStrength, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fDamagePerSecond, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fThreatRatio, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&cFleeState, 1u, 0);
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
    csavedData = 0;
    if (pCoverLocation) {
        csavedData |= 1u;
    }
    if (-3.4028235e+38f != CoverSearchInfo.kLastSearch.fTimeStamp) {
        csavedData |= 2u;
    }
    if (pSecondaryCoverLocation) {
        csavedData |= 4u;
    }
    if (-3.4028235e+38f != SecondaryCoverSearchInfo.kLastSearch.fTimeStamp) {
        csavedData |= 8u;
    }
    if (pCombatLocation) {
        csavedData |= 16u;
    }
    if (pLocationTarget) {
        csavedData |= 32u;
    }
    apSaveGameBuffer->SaveDataEndian(&csavedData, 1u, 0);
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
    if (-3.4028235e+38f != CoverSearchInfo.kLastSearch.fTimeStamp) {
        CoverSearchInfo.SaveGame(apSaveGameBuffer);
    }
    if (-3.4028235e+38f != SecondaryCoverSearchInfo.kLastSearch.fTimeStamp) {
        SecondaryCoverSearchInfo.SaveGame(apSaveGameBuffer);
    }
    isaveGameCount_2 = UnreachableCoverLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2);
    for (uint32_t i_2 = 0; i_2 < isaveGameCount_2; ++i_2) {
        UnreachableCoverLocations.operator[](i_2).SaveGame(apSaveGameBuffer);
    }
    // No null test is needed: the address &UnreachableLocations is never null.
    isaveGameCount_3 = UnreachableLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_3);
    for (uint32_t i_3 = 0; i_3 < isaveGameCount_3; ++i_3) {
        (&UnreachableLocations.operator[](i_3))->SaveGame(apSaveGameBuffer);
    }
    fLastDodgeSearch.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&cLastDodgeDirection, 1u, 0);
    ccombatItems = 0;
    for (uint32_t i_4 = 0; i_4 < 5; ++i_4) {
        if (pCombatItems[i_4]) {
            ccombatItems |= 1 << i_4;
        }
    }
    apSaveGameBuffer->SaveDataEndian(&ccombatItems, 1u, 0);
    for (uint32_t i_5 = 0; i_5 < 5; ++i_5) {
        if (pCombatItems[i_5]) {
            apSaveGameBuffer->SaveFormID(pCombatItems[i_5]->pRef, 0);
            apSaveGameBuffer->SaveFormID(pCombatItems[i_5]->pObject, 0);
        }
    }
    for (uint32_t i_6 = 0; i_6 < 2; ++i_6) {
        apSaveGameBuffer->SaveFormID(pUsableCombatItems[i_6], 0);
        UsableCombatItemTimers[i_6].SaveGame(apSaveGameBuffer);
    }
    apSaveGameBuffer->SaveDataEndian(&iHasCombatItem, 4u, 0);
    apSaveGameBuffer->SaveFormID(pLastTarget, 0);
    apSaveGameBuffer->SaveDataEndian(&bInitializing, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iQueuedPathCheck, 4u, 0);
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
    uint8_t ccombatItems;
    uint8_t csavedData;
    apLoadGameBuffer->LoadData(this, 1u);
    apLoadGameBuffer->LoadData(&iLocationUnreachable, 4u);
    if (iLocationUnreachable == -1) {
        iLocationUnreachable = 0;
    }
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
        pCombatWeapons[i] = dynamic_cast<TESObjectWEAP*>(FormByNumericID);
    }
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    EmbeddedWeaponsArray.SetSize(isaveGameCount, true);
    for (uint32_t i_2 = 0; i_2 < isaveGameCount; ++i_2) {
        TESObjectWEAP** pembeddedWeapon = &EmbeddedWeaponsArray.operator[](i_2);
        uint32_t v4 = apLoadGameBuffer->LoadFormID();
        TESForm* v5 = TESForm::GetFormByNumericID(v4);
        *pembeddedWeapon = dynamic_cast<TESObjectWEAP*>(v5);
    }
    uint32_t v6 = apLoadGameBuffer->LoadFormID();
    TESForm* v7 = TESForm::GetFormByNumericID(v6);
    pCurrentEmbeddedWeapon = dynamic_cast<TESObjectWEAP*>(v7);
    apLoadGameBuffer->LoadData(&iHasCombatWeapon, 4u);
    for (uint32_t i_3 = 0; i_3 < 7; ++i_3) {
        apLoadGameBuffer->LoadData(&fCombatWeaponsDPS[i_3], 4u);
    }
    apLoadGameBuffer->LoadData(&fMaxCombatWeaponDPS, 4u);
    apLoadGameBuffer->LoadData(&fMaxRangedWeaponDPS, 4u);
    apLoadGameBuffer->LoadData(&fDisarmedDPS, 4u);
    apLoadGameBuffer->LoadData(&fBetterWeaponOutOfRangeDistance, 4u);
    apLoadGameBuffer->LoadData(&iTargetDetectionLevel, 4u);
    apLoadGameBuffer->LoadData(&bTargetLineOfSight, 1u);
    apLoadGameBuffer->LoadData(&bTarget360LineOfSight, 1u);
    apLoadGameBuffer->LoadData(&bTargetBlocked, 1u);
    apLoadGameBuffer->LoadData(&cTargetFlushed, 1u);
    apLoadGameBuffer->LoadData(&bTargetSpaceDifferent, 1u);
    apLoadGameBuffer->LoadData(&bHasHandToHandEquip, 1u);
    apLoadGameBuffer->LoadData(&bGrenadeThrowHighArc, 1u);
    apLoadGameBuffer->LoadData(&bInventoryDirty, 1u);
    uint32_t v8 = apLoadGameBuffer->LoadFormID();
    TESForm* v9 = TESForm::GetFormByNumericID(v8);
    pExplosiveProjectileBlocked = dynamic_cast<BGSExplosion*>(v9);
    apLoadGameBuffer->LoadData(&eTargetSegmentInView, 4u);
    apLoadGameBuffer->LoadData(&fThreatScore, 4u);
    apLoadGameBuffer->LoadData(&LastSeenTargetLocation, 12u);
    apLoadGameBuffer->LoadData(&iCombatPermissions, 4u);
    apLoadGameBuffer->LoadData(&cStartingConfidence, 1u);
    apLoadGameBuffer->LoadData(&cCurrentConfidence, 1u);
    apLoadGameBuffer->LoadData(&fConfidenceModifier, 4u);
    apLoadGameBuffer->LoadData(&fCombatStrength, 4u);
    apLoadGameBuffer->LoadData(&fDamagePerSecond, 4u);
    apLoadGameBuffer->LoadData(&fThreatRatio, 4u);
    apLoadGameBuffer->LoadData(&cFleeState, 1u);
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
    csavedData = 0;
    apLoadGameBuffer->LoadData(&csavedData, 1u);
    if ((csavedData & 1) != 0) {
        CombatCoverLocation* v18;
        CombatCoverLocation* apMemory = static_cast<CombatCoverLocation*>(ourMalloc(sizeof(CombatCoverLocation)));
        if (apMemory) {
            v18 = ::new (apMemory) CombatCoverLocation;
        }
        else {
            v18 = nullptr;
        }
        pCoverLocation = v18;
        pCoverLocation->LoadGame(apLoadGameBuffer);
        if (pCoverLocation->bReserved) {
            pCoverLocation->SetReserved(false);
            ReserveCoverLocation(pCoverLocation);
        }
    }
    if ((csavedData & 4) != 0) {
        CombatCoverLocation* v17;
        CombatCoverLocation* v23 = static_cast<CombatCoverLocation*>(ourMalloc(sizeof(CombatCoverLocation)));
        if (v23) {
            v17 = ::new (v23) CombatCoverLocation;
        }
        else {
            v17 = nullptr;
        }
        pSecondaryCoverLocation = v17;
        pSecondaryCoverLocation->LoadGame(apLoadGameBuffer);
        if (pSecondaryCoverLocation->bReserved) {
            pSecondaryCoverLocation->SetReserved(false);
            ReserveCoverLocation(pSecondaryCoverLocation);
        }
    }
    if ((csavedData & 16) != 0) {
        CombatLocation* v16;
        CombatLocation* v22 = static_cast<CombatLocation*>(ourMalloc(sizeof(CombatLocation)));
        if (v22) {
            v16 = ::new (v22) CombatLocation;
        }
        else {
            v16 = nullptr;
        }
        pCombatLocation = v16;
        pCombatLocation->LoadGame(apLoadGameBuffer);
    }
    if ((csavedData & 32) != 0) {
        CombatLocationTarget* v15;
        CombatLocationTarget* v21 = static_cast<CombatLocationTarget*>(ourMalloc(sizeof(CombatLocationTarget)));
        if (v21) {
            v15 = ::new (v21) CombatLocationTarget;
        }
        else {
            v15 = nullptr;
        }
        pLocationTarget = v15;
        pLocationTarget->LoadGame(apLoadGameBuffer);
    }
    if ((csavedData & 2) != 0) {
        CoverSearchInfo.LoadGameBGS(apLoadGameBuffer);
    }
    if ((csavedData & 8) != 0) {
        SecondaryCoverSearchInfo.LoadGameBGS(apLoadGameBuffer);
    }
    uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
    UnreachableCoverLocations.SetSize(isaveGameCount_2, true);
    for (uint32_t i_4 = 0; i_4 < isaveGameCount_2; ++i_4) {
        (&UnreachableCoverLocations.operator[](i_4))->LoadGame(apLoadGameBuffer);
    }
    uint32_t isaveGameCount_3 = apLoadGameBuffer->LoadVariableSizedValue();
    UnreachableLocations.SetSize(isaveGameCount_3, true);
    for (uint32_t i_5 = 0; i_5 < isaveGameCount_3; ++i_5) {
        (&UnreachableLocations.operator[](i_5))->LoadGame(apLoadGameBuffer);
    }
    fLastDodgeSearch.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&cLastDodgeDirection, 1u);
    ccombatItems = 0;
    apLoadGameBuffer->LoadData(&ccombatItems, 1u);
    for (uint32_t i_6 = 0; i_6 < 5; ++i_6) {
        if ((1 << i_6 & ccombatItems) != 0) {
            CombatState::TargetedObject* v14;
            void* v20 = ourMalloc(sizeof(CombatState::TargetedObject));
            if (v20) {
                v14 = ::new (v20) CombatState::TargetedObject;
            }
            else {
                v14 = nullptr;
            }
            pCombatItems[i_6] = v14;
            // Loads the raw form ID into the reference field, as untyped storage; InitLoadGame resolves it.
            apLoadGameBuffer->LoadFormID(pCombatItems[i_6]->pRef);
            uint32_t v10 = apLoadGameBuffer->LoadFormID();
            TESForm* v11 = TESForm::GetFormByNumericID(v10);
            pCombatItems[i_6]->pObject = dynamic_cast<TESBoundObject*>(v11);
        }
    }
    for (uint32_t i_7 = 0; i_7 < 2; ++i_7) {
        uint32_t v12 = apLoadGameBuffer->LoadFormID();
        TESForm* v13 = TESForm::GetFormByNumericID(v12);
        pUsableCombatItems[i_7] = dynamic_cast<TESBoundObject*>(v13);
        UsableCombatItemTimers[i_7].LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&iHasCombatItem, 4u);
    apLoadGameBuffer->LoadFormID(pLastTarget);
    apLoadGameBuffer->LoadData(&bInitializing, 1u);
    if (apLoadGameBuffer->GetVersion() >= 15u) {
        apLoadGameBuffer->LoadData(&iQueuedPathCheck, 4u);
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
    Actor* v4;
    if (pLocationTarget) {
        pLocationTarget->InitLoadGame(apLoadGameBuffer);
    }
    for (uint32_t i = 0; i < 5; ++i) {
        if (pCombatItems[i]) {
            TESForm* v5;
            if (pCombatItems[i]->pRef) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCombatItems[i]->pRef))
                );
                v5 = static_cast<TESForm*>(
                    FNVD_DYNAMIC_CAST_VOID(TESObjectREFR)(FormByNumericID, RTTI_TESForm, RTTI_TESObjectREFR)
                );
            }
            else {
                v5 = nullptr;
            }
            pCombatItems[i]->pRef = v5;
            if (pCombatItems[i]->pRef) {
                static_cast<TESObjectREFR*>(pCombatItems[i]->pRef)->TESObjectREFR::SetTargeted(true);
            }
        }
    }
    if (pCombatLocation) {
        pCombatLocation->InitLoadGame(apLoadGameBuffer);
    }
    if (pLastTarget) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLastTarget)));
        v4 = dynamic_cast<Actor*>(v3);
    }
    else {
        v4 = nullptr;
    }
    pLastTarget = v4;
    if (iQueuedPathCheck != -1) {
        QueuePathCheck(iQueuedPathCheck);
    }
    for (uint32_t i_2 = 0; i_2 < 7; ++i_2) {
        fMaxModifiedCombatWeaponDPS = BSMAX(fMaxModifiedCombatWeaponDPS, fCombatWeaponsDPS[i_2]);
    }
}

bool CombatState::TryReserveSecondaryCoverLocation() {
    return TryReserveCoverLocation(pSecondaryCoverLocation);
}

void CombatState::ClearLocationUnreachable(uint32_t aiLocation) {
    iLocationUnreachable = ~aiLocation & iLocationUnreachable;
}

void CombatState::SetLocationTargetRef(TESObjectREFR* apRef, LOCATION_TARGET_TYPE aeType, float afMin, float afMax) {
    BGSWorldLocation apLocation;
    ClearLocationTarget();
    if (apRef) {
        CombatLocationTarget* v7;
        void* apMemory = ourMalloc(sizeof(CombatLocationTarget));
        if (apMemory) {
            v7 = ::new (apMemory) CombatLocationTarget;
        }
        else {
            v7 = nullptr;
        }
        pLocationTarget = v7;
        pLocationTarget->cType = aeType;
        pLocationTarget->pRef = apRef;
        pLocationTarget->WorldLoc = *apRef->GetWorldLocation(&apLocation);
        float* v5 = &fCombatLocationTargetRadiusMin.Float();
        pLocationTarget->fMinDistance = BSMAX(afMin, *v5);
        float* v6 = &fCombatLocationTargetRadiusMin.Float();
        pLocationTarget->fMaxDistance = BSMAX(afMax, *v6);
    }
}

bool CombatState::CheckCoverLOS(
    PathingLocation& arCoverPathingLocation,
    const NiPoint3& arTargetLocation,
    bool abSkipDistanceCheck
) {
    NiPoint3* Location_;
    NiPoint3 result;
    NiPoint3 v6;
    bool v4 = (abSkipDistanceCheck ||
               (Location_ = &(result = arCoverPathingLocation.GetLocation()),
                (v6 = arTargetLocation - *Location_, !(v6.SqrLength() > 4194304.0)))) &&
        Pathing::CheckLineOfSight(
                  arCoverPathingLocation, arTargetLocation, static_cast<PathingLocation*>(nullptr),
                  static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        );
    return v4;
}

bool CombatState::CheckTargetsCoverLOS_0(
    PathingLocation& arCoverPathingLocation,
    BSSimpleArray<NiPoint3, 1024>* apLocationsArray
) {
    NiPoint3 arTargetLocation;
    for (uint32_t auiIndex = 0;; ++auiIndex) {
        uint32_t FormFlags = apLocationsArray->GetSize();
        if (auiIndex >= FormFlags) {
            break;
        }
        arTargetLocation = *static_cast<NiPoint3*>(&apLocationsArray->operator[](auiIndex));
        if (CombatState::CheckCoverLOS(arCoverPathingLocation, arTargetLocation, false)) {
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
        *originPathingLocation, *destinationPathingLocation, pavoidNodeArray, &ffullPathDistance, nullptr
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
    // Scales by the double 0.9f and rounds the factor to a float before the product, as the game does.
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
    pcombatCoverSearchInfo->nipoint3C = center;
    pcombatCoverSearchInfo->float18 = fMaxDist;
    pcombatCoverSearchInfo->kSearchLocation = pcombatCoverLocation->CoverSearchTargetLocation;
    pcombatCoverSearchInfo->kCoverLocations.Clear(false);
    if (Pathing::GetCoverPoints(&coverRequest, pcombatCoverSearchInfo->kCoverLocations)) {
        if (!abBackgroundThread) {
            pCombatManager->pCoverMap->LockForWrite();
        }
        for (uint32_t i = 0; i < pcombatCoverSearchInfo->kCoverLocations.iSize; ++i) {
            PathingCoverLocation* coverLoc = &pcombatCoverSearchInfo->kCoverLocations.GetAt(static_cast<int32_t>(i));
            uint32_t icoverKey = coverLoc->GetKey();
            if (apCoveredMoveRequest->GetUnreachableCoverLocations()->Find(
                    icoverKey, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress
                ) != -1) {
                coverLoc->cState = 3;
                continue;
            }
            CoverReservationInfo reservationInfo;
            if (!pCombatManager->pCoverMap->CanReserveCoverLocation(
                    icoverKey, coverLoc, nullptr, apCoveredMoveRequest->fActorRadius, &reservationInfo, true, false
                )) {
                coverLoc->cState = 9;
                continue;
            }
            NiPoint3 coverLocation = coverLoc->GetLocation();
            CombatUtilities::CalculateCoverPositions_1(
                apCoveredMoveRequest->fActorRadius, coverLoc, &reservationInfo, &coverLocation, nullptr
            );
            PathingLocation coverPathingLocation(coverLocation, *coverLoc);
            if (!coverPathingLocation.IsValidLocation(apCoveredMoveRequest->fAttackerRadius)) {
                coverLoc->cState = 4;
                continue;
            }
            NiPoint3 destinationVector = coverLocation - destinationPathingLocation->GetLocation();
            if (fadvancePathingDistance * fadvancePathingDistance < destinationVector.SqrLength()) {
                coverLoc->cState = 7;
                continue;
            }
            NiPoint3 coverOriginVector = originPathingLocation->GetLocation() - coverLocation;
            if (afMinDistance * afMinDistance > coverOriginVector.SqrLength()) {
                coverLoc->cState = 8;
                continue;
            }
            if (busingTargetLocation) {
                NiPoint3 targetVector = coverLocation - pcombatCoverLocation->CoverSearchTargetLocation;
                if (targetVector.SqrLength() < 16384.0) {
                    coverLoc->cState = 10;
                    continue;
                }
            }
            float ftimeToReachCover = coverOriginVector.Length() / apCoveredMoveRequest->fAttackerSpeed;
            CombatTimeStamp kSkipOlderThreats(0.0f);
            if (pCombatManager->pThreatMap->ComputeThreatScore(
                    coverLocation, nullptr, ftimeToReachCover, 0.0f, kSkipOlderThreats,
                    apCoveredMoveRequest->fAttackerHealth
                ) > 0.0) {
                coverLoc->cState = 11;
                continue;
            }
            bool bhasCoverLOS = false;
            if (apCoveredMoveRequest->GetTargetLocations()->iSize) {
                bhasCoverLOS = CombatState::CheckTargetsCoverLOS_0(
                    coverPathingLocation, apCoveredMoveRequest->GetTargetLocations()
                );
            }
            else {
                bhasCoverLOS =
                    CombatState::CheckCoverLOS(coverPathingLocation, destinationPathingLocation->GetLocation(), false);
            }
            if (bhasCoverLOS) {
                coverLoc->cState = 13;
                continue;
            }
            float fcoverDestinationPathingDistance = 0.0f;
            BSScrapArray<NavMeshSearchNode> coverDestinationNodes;
            bool bresult = Pathing::ComputeApproximatePathingDistance(
                coverPathingLocation, *destinationPathingLocation, pavoidNodeArray, &fcoverDestinationPathingDistance,
                &coverDestinationNodes
            );
            if ((bfullPathingResult && !bresult) || fadvancePathingDistance < fcoverDestinationPathingDistance) {
                coverLoc->cState = 17;
                continue;
            }
            float fattackerCoverPathingDistance = 0.0f;
            bresult = Pathing::ComputeApproximatePathingDistance(
                *originPathingLocation, coverPathingLocation, pavoidNodeArray, &fattackerCoverPathingDistance, nullptr
            );
            if ((bfullPathingResult && !bresult) ||
                fmaxCombinedPathingDistance < fcoverDestinationPathingDistance + fattackerCoverPathingDistance) {
                coverLoc->cState = 18;
                continue;
            }
            coverLoc->cState = 19;
            pcombatCoverLocation->CoverLocation = *coverLoc;
            pcombatCoverLocation->iCoverLocationKey = icoverKey;
            pcombatCoverLocation->iCoverLocationLastMovedTimeStamp =
                pcombatCoverLocation->CoverLocation.GetDynamicCoverObstacleTimeStamp();
            pcombatCoverLocation->ReservationInfo = reservationInfo;
            pcombatCoverLocation->cCoverHeight = pcombatCoverLocation->CoverLocation.GetHeight();
            pcombatCoverLocation->bDirtyFlag = false;
            if (busingTargetLocation) {
                NiPoint3 nodeCenter = targetLocation;
                BSScrapArray<NavMeshSearchNode> coverTargetNodes;
                PathingLocation targetPathingLocation(targetLocation, *originPathingLocation);
                if (Pathing::ComputeApproximatePathingDistance(
                        coverPathingLocation, targetPathingLocation, pavoidNodeArray, nullptr, &coverTargetNodes
                    ) &&
                    coverTargetNodes.iSize > 1) {
                    const BSSimpleArray<NavMeshSearchNode>& kTargetNodes = coverTargetNodes;
                    NiPoint3 kSecondNodeCenter;
                    nodeCenter = *kTargetNodes[1].GetCenter(&kSecondNodeCenter);
                }
                if (pcombatCoverLocation->CoverLocation.IsLeftSideClosest(nodeCenter)) {
                    pcombatCoverLocation->ReservationInfo.bLeftEdge = true;
                }
            }
            else if (coverDestinationNodes.iSize > 1) {
                const BSSimpleArray<NavMeshSearchNode>& kDestinationNodes = coverDestinationNodes;
                NiPoint3 nodeCenter;
                kDestinationNodes[1].GetCenter(&nodeCenter);
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
    bDirtyFlag = (bool)((uint8_t)apCopy->bDirtyFlag);
    bReserved = (bool)((uint8_t)apCopy->bReserved);
    cCoverHeight = apCopy->cCoverHeight;
    iLocation = (int32_t)((uint32_t)apCopy->iLocation);
    CoverSearchTargetLocation = apCopy->CoverSearchTargetLocation;
    &(&CoverLocation)->operator=(apCopy->CoverLocation);
    iCoverLocationKey = apCopy->iCoverLocationKey;
    iCoverLocationLastMovedTimeStamp = apCopy->iCoverLocationLastMovedTimeStamp;
    ReservationInfo = apCopy->ReservationInfo;
}

void CombatCoverLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bReserved)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bDirtyFlag)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cCoverHeight)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iLocation)), 4u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&CoverSearchTargetLocation)), 0xCu);
    (&CoverLocation)->LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iCoverLocationKey)), 4u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iCoverLocationLastMovedTimeStamp)), 4u);
    ReservationInfo.LoadGame(apLoadGameBuffer);
}

void CombatCoverLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&bReserved, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(this, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cCoverHeight, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iLocation, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&CoverSearchTargetLocation, 12u, 0);
    CoverLocation.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&iCoverLocationKey, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iCoverLocationLastMovedTimeStamp, 4u, 0);
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
  : kSearchLocation(NiPoint3_ZERO), nipoint3C(NiPoint3_ZERO), float18(0.0f), kLastSearch(-3.4028235e+38f),
    uiPreviousCoverKey(0), uiPreviousState(0), bSearching(false) {}

void CombatCoverSearchInfo::Clear() {
    float18 = 0.0f;
    CombatTimeStamp kTimeStamp;
    kTimeStamp.~CombatTimeStamp();
    ::new (&kTimeStamp) CombatTimeStamp(-3.40282347e+38f);
    kLastSearch = kTimeStamp;
    kCoverLocations.Clear(true);
}

void CombatCoverSearchInfo::Copy(const CombatCoverSearchInfo* apCopy) {
    if (!apCopy) {
        Clear();
        return;
    }
    kSearchLocation = apCopy->kSearchLocation;
    nipoint3C = apCopy->nipoint3C;
    float18 = apCopy->float18;
    kLastSearch = apCopy->kLastSearch;
    apCopy->kCoverLocations.CopyTo(&kCoverLocations);
    uiPreviousCoverKey = apCopy->uiPreviousCoverKey;
    uiPreviousState = apCopy->uiPreviousState;
    bSearching = apCopy->bSearching;
}

void CombatCoverSearchInfo::LoadGameBGS(BGSLoadGameBuffer* a2) {
    a2->LoadData(this, 12u);
    a2->LoadData(&nipoint3C, 12u);
    a2->LoadData(&float18, 4u);
    a2->LoadData(&bSearching, 1u);
    a2->LoadData(&uiPreviousCoverKey, 4u);
    a2->LoadData(&uiPreviousState, 4u);
    kLastSearch.LoadGame(a2);
    uint32_t aiNewSize = a2->LoadVariableSizedValue();
    kCoverLocations.SetSize(aiNewSize, true);
    for (uint32_t aiIndex = 0; aiIndex < aiNewSize; ++aiIndex) {
        PathingCoverLocation* v3 = &kCoverLocations.operator[](aiIndex);
        v3->LoadGame(a2);
    }
}

void CombatCoverSearchInfo::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount;
    apSaveGameBuffer->SaveDataEndian(this, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&nipoint3C, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&float18, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bSearching, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&uiPreviousCoverKey, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&uiPreviousState, 4u, 0);
    kLastSearch.SaveGame(apSaveGameBuffer);
    // The address &kCoverLocations is never null.
    isaveGameCount = kCoverLocations.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        PathingCoverLocation* coverLocation = &kCoverLocations.operator[](i);
        coverLocation->SaveGame(apSaveGameBuffer);
    }
}

void CombatLocation::SetRadius(float afRadius) {
    fRadius = afRadius;
}

bool CombatLocation::GetFlag(uint32_t aiFlag) {
    return ((aiFlag & iFlags) != 0);
}

TESForm* CombatLocation::GetSpace() {
    return WorldLoc.GetSpace();
}

void CombatLocation::Copy(const CombatLocation* apCopy) {
    *this = *apCopy;
}

void CombatLocation::SetRef(TESObjectREFR* apRef) {
    BGSWorldLocation v2;
    pRef = apRef;
    WorldLoc = *pRef->GetWorldLocation(&v2);
}

void CombatLocation::UpdateRefLocation() {
    BGSWorldLocation v1;
    if (pRef) {
        WorldLoc = *pRef->GetWorldLocation(&v1);
    }
}

void CombatLocation::SetFlag(uint32_t aiFlag, bool abValue) {
    if (abValue) {
        iFlags |= aiFlag;
    }
    else {
        iFlags &= (~aiFlag);
    }
}

void CombatLocation::SetPoint(const NiPoint3& arPoint, TESForm* apSpace) {
    (&WorldLoc)->SetLocation(arPoint, static_cast<TESObjectCELL*>(apSpace));
    pRef = nullptr;
}

void CombatLocation::SetLocation(const BGSWorldLocation& arLocation) {
    WorldLoc = arLocation;
}

void CombatLocation::SetSpace(TESForm* apSpace) {
    WorldLoc.SetSpace(apSpace);
}

void CombatLocation::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm* formByNumericID;
    TESObjectREFR* v3;
    if (pRef) {
        formByNumericID = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRef));
        v3 = dynamic_cast<TESObjectREFR*>(formByNumericID);
    }
    else {
        v3 = nullptr;
    }
    pRef = v3;
}

void CombatLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    WorldLoc.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iLocation), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iFlags), 4);
    apLoadGameBuffer->LoadFormID(pRef);
}

void CombatLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    WorldLoc.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iLocation), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadius), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFlags), 4u, 0);
    apSaveGameBuffer->SaveFormID(pRef, static_cast<uint32_t>(0));
}

bool CombatProcedureBeInCover::GetBehindCover() {
    return eState != 3 && eState != 4;
}

UnreachableCoverLocation::UnreachableCoverLocation() {}

void UnreachableCoverLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iCoverKey, 4u, 0);
    fTimeStamp.SaveGame(apSaveGameBuffer);
}

void UnreachableCoverLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 4u);
    fTimeStamp.LoadGame(apLoadGameBuffer);
}

float NiSqrt(float fValue) {
    return std::sqrt(fValue);
}
