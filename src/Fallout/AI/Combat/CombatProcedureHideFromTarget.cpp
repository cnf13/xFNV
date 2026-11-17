#include "Fallout/AI/Combat/CombatProcedureHideFromTarget.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The positions are deliberately left unset, as the game does.
CombatProcedureHideFromTarget::CombatProcedureHideFromTarget(
    CombatCoverLocation* apCombatCoverLocation,
    float afNumSeconds,
    bool abIgnoreLOS,
    bool abClearCoverWhenFinished
)
  : eState(CombatProcedureHideFromTarget::HIDE_STATE_INITIALIZING), fNumSecondsToHide(afNumSeconds),
    pCoverLocation(apCombatCoverLocation), bIgnoreLOS(abIgnoreLOS), bClearCoverWhenFinished(abClearCoverWhenFinished) {}

CombatProcedureHideFromTarget::~CombatProcedureHideFromTarget() {}

void CombatProcedureHideFromTarget::Update() {
    BGSWorldLocation apLocation;
    BGSWorldLocation attackerWorldLoc;
    BGSWorldLocation targetWorldLoc;
    Actor* pattacker__ptarget = pCombatController->pAttacker;
    Actor* ptarget = pCombatController->GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    CombatProcedureHideFromTarget::HIDE_STATE eState = this->eState;
    if (eState) {
        if (eState == CombatProcedureHideFromTarget::HIDE_STATE_MOVING_TO_COVER) {
            SetMoveMode(3);
            if (pattacker__ptarget->DidPathingFail()) {
                if (pattacker__ptarget->IsPathValid()) {
                    SetFailed("Pathing failed while moving to hiding location");
                }
                else {
                    SetFailed("Path Building failed to hiding location");
                }
            }
            else {
                if (pattacker__ptarget->IsPathingComplete()) {
                    HidingTimer.Reset(fNumSecondsToHide);
                    SetLookAtTarget(LookAtPosition, 3);
                    SetMoveMode(2);
                    this->eState = CombatProcedureHideFromTarget::HIDE_STATE_HIDING;
                }
            }
        }
        else {
            if (eState == CombatProcedureHideFromTarget::HIDE_STATE_HIDING) {
                if (HidingTimer.Check()) {
                    if (bClearCoverWhenFinished) {
                        pcombatState->ClearCoverLocation(false);
                    }
                    SetFinished();
                }
                else {
                    float fhidingTime = HidingTimer.fStartTime + 1.0;
                    bool bcheckHiding = static_cast<double>(fhidingTime) < CombatTimer::GetTimer();
                    if (bcheckHiding) {
                        CombatLocationTarget* plocationTarget = pcombatState->pLocationTarget;
                        if ((plocationTarget) && (plocationTarget->cType == 1)) {
                            BGSWorldLocation* WorldLocation = pattacker__ptarget->GetWorldLocation(&apLocation);
                            if (plocationTarget->WorldLoc.GetDistanceSquared(*WorldLocation) < 4096.0) {
                                bcheckHiding = false;
                            }
                        }
                    }
                    if (bcheckHiding) {
                        float* v4;
                        double DistanceSquared;
                        if (pcombatState->GetHasTarget360LineOfSight() &&
                            (!bIgnoreLOS ||
                             (ptarget &&
                              (pattacker__ptarget->GetWorldLocation(&attackerWorldLoc),
                               (ptarget->GetWorldLocation(&targetWorldLoc),
                                (DistanceSquared = attackerWorldLoc.GetDistanceSquared(targetWorldLoc),
                                 (v4 = &fCombatHideFailedTargetDistance.Float(),
                                  *v4 * fCombatHideFailedTargetDistance.Float() > DistanceSquared))))))) {
                            pCombatController->pCombatState->ClearCoverLocation(true);
                            pcombatState->cFleeState = 0;
                            SetFinished();
                        }
                        else {
                            double DecalMinWidth = pcombatState->GetLastDamagedByAttackerTimeStamp().fTimeStamp;
                            if (fhidingTime < DecalMinWidth) {
                                pCombatController->pCombatState->ClearCoverLocation(true);
                                pcombatState->cFleeState = 0;
                                SetFinished();
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        NiPoint3 hideVector;
        bool bcoverValid = pcombatState->IsCoverLocationValid(pCoverLocation);
        if (bcoverValid) {
            double TargetFiringZOffset;
            pCoverLocation->ReservationInfo.bHideCover = true;
            CombatUtilities::CalculateCoverPositions(pCombatController, pCoverLocation, &HidePosition, &LookAtPosition);
            if (ptarget) {
                TargetFiringZOffset = CombatUtilities::GetTargetFiringZOffset(ptarget, ACTOR_SEGMENT_INVIEW_TOP, false);
            }
            else {
                TargetFiringZOffset =
                    CombatUtilities::GetTargetFiringZOffset(pattacker__ptarget, ACTOR_SEGMENT_INVIEW_TOP, false);
            }
            LookAtPosition.z = TargetFiringZOffset + LookAtPosition.z;
        }
        else {
            HidePosition = pattacker__ptarget->GetLocationOnReference();
            if (ptarget) {
                LookAtPosition = CombatUtilities::GetTargetFiringLocation(ptarget, ACTOR_SEGMENT_INVIEW_TOP);
            }
        }
        const NiPoint3* v2 = &pattacker__ptarget->GetLocationOnReference();
        hideVector = *v2 - HidePosition;
        if (bcoverValid && hideVector.SqrLength() > 625.0) {
            SetMoveMode(3);
            PathingRequest request;
            TESWorldSpace* WorldSpace = pattacker__ptarget->GetWorldSpace();
            TESObjectCELL* ParentCell = pattacker__ptarget->GetParentCell();
            pattacker__ptarget->BuildRequest(&request, HidePosition, ParentCell, WorldSpace, 0.0f, nullptr);
            float a2 = pattacker__ptarget->GetHeading(false);
            request.SetInitialPathHeading(a2);
            if (pattacker__ptarget->SetPathfindingGoal(&request)) {
                this->eState = CombatProcedureHideFromTarget::HIDE_STATE_MOVING_TO_COVER;
            }
            else {
                SetFailed("Cannot setup path to hiding location");
            }
        }
        else {
            HidingTimer.Reset(fNumSecondsToHide);
            if (ptarget) {
                SetLookAtTarget(LookAtPosition, 3);
            }
            SetMoveMode(2);
            this->eState = CombatProcedureHideFromTarget::HIDE_STATE_HIDING;
        }
    }
}

void CombatProcedureHideFromTarget::Shutdown() {
    Actor* pAttacker = pCombatController->GetAttacker();
    pAttacker->StopMoving();
    CombatProcedure::ClearMoveMode();
    CombatProcedure::ClearLookAtTarget();
}

// Both timer values are passed as doubles: fTargetTime widened from its float, and the ElapsedTime result.
void CombatProcedureHideFromTarget::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    CombatProcedureHideFromTarget::HIDE_STATE eState = this->eState;
    if (eState) {
        if (eState == CombatProcedureHideFromTarget::HIDE_STATE_MOVING_TO_COVER) {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_TO_COVER");
        }
        else {
            if (eState == CombatProcedureHideFromTarget::HIDE_STATE_HIDING) {
                apDebugText->PrintLine(aeLocation, aeJustify, "HIDING");
                double dTargetTime = HidingTimer.fTargetTime;
                double dElapsedTime = HidingTimer.ElapsedTime();
                apDebugText->PrintLine(aeLocation, aeJustify, "Hiding Timer: %.02f/%.02f", dElapsedTime, dTargetTime);
            }
        }
    }
    else {
        apDebugText->PrintLine(aeLocation, aeJustify, "INITIALIZING");
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureHideFromTarget::GetType() {
    return CombatProcedure::HIDE_FROM_TARGET;
}

void CombatProcedureHideFromTarget::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t ccoverLocation;
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fNumSecondsToHide, 4u, 0);
    HidingTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&HidePosition, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&LookAtPosition, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreLOS, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bClearCoverWhenFinished, 1u, 0);
    ccoverLocation = 0;
    if (pCoverLocation) {
        ccoverLocation = (pCoverLocation == pCombatController->pCombatState->GetSecondaryCoverLocation()) + 1;
    }
    apSaveGameBuffer->SaveDataEndian(&ccoverLocation, 1u, 0);
}

void CombatProcedureHideFromTarget::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t ccoverLocation;
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eState, 4u);
    apLoadGameBuffer->LoadData(&fNumSecondsToHide, 4u);
    HidingTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&HidePosition, 12u);
    apLoadGameBuffer->LoadData(&LookAtPosition, 12u);
    apLoadGameBuffer->LoadData(&bIgnoreLOS, 1u);
    apLoadGameBuffer->LoadData(&bClearCoverWhenFinished, 1u);
    ccoverLocation = 0;
    apLoadGameBuffer->LoadData(&ccoverLocation, 1u);
    if (ccoverLocation == 2) {
        pCoverLocation = pCombatController->pCombatState->GetSecondaryCoverLocation();
    }
    else {
        if (ccoverLocation == 1) {
            pCoverLocation = pCombatController->pCombatState->GetCoverLocation();
        }
    }
}

void CombatProcedureHideFromTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

CombatTimeStamp CombatState::GetLastDamagedByAttackerTimeStamp() {
    return fLastDamagedByAttackerTimeStamp;
}
