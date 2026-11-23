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

// The positions are deliberately left unset.
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
    BGSWorldLocation kLocation;
    BGSWorldLocation kAttackerWorldLoc;
    BGSWorldLocation kTargetWorldLoc;
    Actor* pAttacker = pCombatController->pAttacker;
    Actor* pTarget = pCombatController->GetTarget();
    CombatState* pCombatState = pCombatController->pCombatState;
    CombatProcedureHideFromTarget::HIDE_STATE eState = this->eState;
    if (eState) {
        if (eState == CombatProcedureHideFromTarget::HIDE_STATE_MOVING_TO_COVER) {
            SetMoveMode(3);
            if (pAttacker->DidPathingFail()) {
                if (pAttacker->IsPathValid()) {
                    SetFailed("Pathing failed while moving to hiding location");
                }
                else {
                    SetFailed("Path Building failed to hiding location");
                }
            }
            else if (pAttacker->IsPathingComplete()) {
                HidingTimer.Reset(fNumSecondsToHide);
                SetLookAtTarget(LookAtPosition, 3);
                SetMoveMode(2);
                this->eState = CombatProcedureHideFromTarget::HIDE_STATE_HIDING;
            }
        }
        else if (eState == CombatProcedureHideFromTarget::HIDE_STATE_HIDING) {
            if (HidingTimer.Check()) {
                if (bClearCoverWhenFinished) {
                    pCombatState->ClearCoverLocation(false);
                }
                SetFinished();
            }
            else {
                float fHidingTime = HidingTimer.fStartTime + 1.0;
                bool bCheckHiding = static_cast<double>(fHidingTime) < CombatTimer::GetTimer();
                if (bCheckHiding) {
                    CombatLocationTarget* pLocationTarget = pCombatState->pLocationTarget;
                    if (pLocationTarget && pLocationTarget->cType == 1) {
                        BGSWorldLocation* pWorldLocation = pAttacker->GetWorldLocation(&kLocation);
                        if (pLocationTarget->WorldLoc.GetDistanceSquared(*pWorldLocation) < 4096.0) {
                            bCheckHiding = false;
                        }
                    }
                }
                if (bCheckHiding) {
                    double dDistanceSquared;
                    if (pCombatState->GetHasTarget360LineOfSight() &&
                        (!bIgnoreLOS ||
                         (pTarget &&
                          (pAttacker->GetWorldLocation(&kAttackerWorldLoc),
                           pTarget->GetWorldLocation(&kTargetWorldLoc),
                           dDistanceSquared = kAttackerWorldLoc.GetDistanceSquared(kTargetWorldLoc),
                           fCombatHideFailedTargetDistance.Float() * fCombatHideFailedTargetDistance.Float() >
                               dDistanceSquared)))) {
                        pCombatController->pCombatState->ClearCoverLocation(true);
                        pCombatState->cFleeState = 0;
                        SetFinished();
                    }
                    else {
                        double dDamagedTimeStamp = pCombatState->GetLastDamagedByAttackerTimeStamp().fTimeStamp;
                        if (fHidingTime < dDamagedTimeStamp) {
                            pCombatController->pCombatState->ClearCoverLocation(true);
                            pCombatState->cFleeState = 0;
                            SetFinished();
                        }
                    }
                }
            }
        }
    }
    else {
        bool bCoverValid = pCombatState->IsCoverLocationValid(pCoverLocation);
        if (bCoverValid) {
            double dFiringZOffset;
            pCoverLocation->ReservationInfo.bHideCover = true;
            CombatUtilities::CalculateCoverPositions(pCombatController, pCoverLocation, &HidePosition, &LookAtPosition);
            if (pTarget) {
                dFiringZOffset = CombatUtilities::GetTargetFiringZOffset(pTarget, ACTOR_SEGMENT_INVIEW_TOP, false);
            }
            else {
                dFiringZOffset = CombatUtilities::GetTargetFiringZOffset(pAttacker, ACTOR_SEGMENT_INVIEW_TOP, false);
            }
            LookAtPosition.z = dFiringZOffset + LookAtPosition.z;
        }
        else {
            HidePosition = pAttacker->GetLocationOnReference();
            if (pTarget) {
                LookAtPosition = CombatUtilities::GetTargetFiringLocation(pTarget, ACTOR_SEGMENT_INVIEW_TOP);
            }
        }
        NiPoint3 kHideVector = pAttacker->GetLocationOnReference() - HidePosition;
        if (bCoverValid && kHideVector.SqrLength() > 625.0) {
            SetMoveMode(3);
            PathingRequest kRequest;
            TESWorldSpace* pWorldSpace = pAttacker->GetWorldSpace();
            TESObjectCELL* pParentCell = pAttacker->GetParentCell();
            pAttacker->BuildRequest(&kRequest, HidePosition, pParentCell, pWorldSpace, 0.0f, nullptr);
            kRequest.SetInitialPathHeading(pAttacker->GetHeading(false));
            if (pAttacker->SetPathfindingGoal(&kRequest)) {
                this->eState = CombatProcedureHideFromTarget::HIDE_STATE_MOVING_TO_COVER;
            }
            else {
                SetFailed("Cannot setup path to hiding location");
            }
        }
        else {
            HidingTimer.Reset(fNumSecondsToHide);
            if (pTarget) {
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
        else if (eState == CombatProcedureHideFromTarget::HIDE_STATE_HIDING) {
            apDebugText->PrintLine(aeLocation, aeJustify, "HIDING");
            double dTargetTime = HidingTimer.fTargetTime;
            double dElapsedTime = HidingTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Hiding Timer: %.02f/%.02f", dElapsedTime, dTargetTime);
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
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(&fNumSecondsToHide, sizeof(fNumSecondsToHide), 0);
    HidingTimer.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&HidePosition, sizeof(HidePosition), 0);
    apSaveGameBuffer->SaveDataEndian(&LookAtPosition, sizeof(LookAtPosition), 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreLOS, sizeof(bIgnoreLOS), 0);
    apSaveGameBuffer->SaveDataEndian(&bClearCoverWhenFinished, sizeof(bClearCoverWhenFinished), 0);
    uint8_t cCoverLocation = 0;
    if (pCoverLocation) {
        cCoverLocation = (pCoverLocation == pCombatController->pCombatState->GetSecondaryCoverLocation()) + 1;
    }
    apSaveGameBuffer->SaveDataEndian(&cCoverLocation, sizeof(cCoverLocation), 0);
}

void CombatProcedureHideFromTarget::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    apLoadGameBuffer->LoadData(&fNumSecondsToHide, sizeof(fNumSecondsToHide));
    HidingTimer.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&HidePosition, sizeof(HidePosition));
    apLoadGameBuffer->LoadData(&LookAtPosition, sizeof(LookAtPosition));
    apLoadGameBuffer->LoadData(&bIgnoreLOS, sizeof(bIgnoreLOS));
    apLoadGameBuffer->LoadData(&bClearCoverWhenFinished, sizeof(bClearCoverWhenFinished));
    uint8_t cCoverLocation = 0;
    apLoadGameBuffer->LoadData(&cCoverLocation, sizeof(cCoverLocation));
    if (cCoverLocation == 2) {
        pCoverLocation = pCombatController->pCombatState->GetSecondaryCoverLocation();
    }
    else if (cCoverLocation == 1) {
        pCoverLocation = pCombatController->pCombatState->GetCoverLocation();
    }
}

void CombatProcedureHideFromTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

CombatTimeStamp CombatState::GetLastDamagedByAttackerTimeStamp() {
    return fLastDamagedByAttackerTimeStamp;
}
