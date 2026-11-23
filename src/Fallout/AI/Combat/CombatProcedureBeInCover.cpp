#include "Fallout/AI/Combat/CombatProcedureBeInCover.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The positions are deliberately left unset.
CombatProcedureBeInCover::CombatProcedureBeInCover(CombatCoverLocation* apCombatCoverLocation)
  : eState(COVER_STATE_INITIALIZING), pCoverLocation(apCombatCoverLocation) {}

CombatProcedureBeInCover::~CombatProcedureBeInCover() {}

void CombatProcedureBeInCover::Update() {
    if (!pCombatController->pCombatState->IsCoverLocationValid(pCoverLocation)) {
        SetFailed("Cover Location no longer exists");
        return;
    }
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    CombatState* pcombatState = pCombatController->pCombatState;
    CombatGroup* pcombatGroup = pCombatController->pCombatGroup;
    bool breevaluatedCover = false;
    bool bcoverInvalid = false;
    bool bledgeCover = pCoverLocation->IsLedgeCover();
    bool bcanSneak = pCombatController->GetAttackerCanSneak();
    if (ReevaluateCoverTimer.IsSet()) {
        if (ReevaluateCoverTimer.Check()) {
            ReevaluateCoverTimer.Reset(1.0f);
            breevaluatedCover = true;
            if (eState) {
                if (pcombatState->GetHasTarget360LineOfSight()) {
                    bool bcoverSafe = pcombatState->CheckCoverSafe(
                        pCoverLocation->CoverLocation,
                        CurrentCoverPosition,
                        ptarget,
                        1
                    );
                    if (!bcoverSafe && eState != CombatProcedureBeInCover::COVER_STATE_HOLDING_GROUND) {
                        pattacker->StopMoving();
                        ClearLookAtTarget();
                        eState = CombatProcedureBeInCover::COVER_STATE_HOLDING_GROUND;
                        HoldingGroundTimer.Reset(2.0f);
                        bcoverInvalid = true;
                    }
                }
                else {
                    NiPoint3 targetLoc = ptarget->GetLocationOnReference();
                    NiPoint3 originalTargetLoc = pCoverLocation->CoverSearchTargetLocation;
                    NiPoint3 targetVec = targetLoc - originalTargetLoc;
                    if (targetVec.SqrLength() > 589824.0 && pcombatGroup->GetTargetMemberLOSCount(ptarget)) {
                        pCoverLocation->bDirtyFlag = true;
                    }
                }
            }
        }
    }
    else {
        ReevaluateCoverTimer.Reset(1.0f);
    }
    switch (eState) {
        case CombatProcedureBeInCover::COVER_STATE_INITIALIZING: {
            float fheight;
            pCoverLocation->ReservationInfo.bHideCover = false;
            CombatUtilities::CalculateCoverPositions(
                pCombatController,
                pCoverLocation,
                &CurrentCoverPosition,
                &CurrentFirePosition
            );
            LookAtPosition = CurrentFirePosition;
            if (pCombatController->GetAttackerCanSneak()) {
                fheight = pattacker->GetSneakHeight(false);
            }
            else {
                fheight = pattacker->GetHeight();
            }
            LookAtPosition.z = fheight * 0.75 + LookAtPosition.z;
            PathingRequest request;
            TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
            TESObjectCELL* pParentCell = pattacker->GetParentCell();
            pattacker->BuildRequest(&request, CurrentCoverPosition, pParentCell, pWorldSpace, 0.0f, nullptr);
            if (pattacker->SetPathfindingGoal(&request)) {
                eState = CombatProcedureBeInCover::COVER_STATE_MOVING_IN_AND_ROTATE;
                if (pCoverLocation->HasFireLocation()) {
                    SetLookAtTarget(LookAtPosition, 3);
                }
                SetMoveMode(3);
            }
            else {
                SetFailed("Cannot setup path to Cover position");
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_WAITING_BEHIND_COVER: {
            if (!pattacker->IsReloading()) {
                double dCurrentAmmoClipPercent;
                if (!(pcombatState->GetLastAttackTimeStamp().ElapsedTime() > 0.5) ||
                    (dCurrentAmmoClipPercent = pattacker->GetCurrentAmmoClipPercent(),
                     !(fCoverWaitReloadClipPercent.Float() > dCurrentAmmoClipPercent))) {
                    if (WaitBehindCoverTimer.Check()) {
                        PathingRequest request;
                        TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
                        TESObjectCELL* pParentCell = pattacker->GetParentCell();
                        pattacker->BuildRequest(&request, CurrentFirePosition, pParentCell, pWorldSpace, 0.0f, nullptr);
                        if (pattacker->SetPathfindingGoal(&request)) {
                            if (pCoverLocation->HasFireLocation()) {
                                ClearLookAtTarget();
                            }
                            SetLookAtTarget(ptarget->GetLookingAtLocation(), 1);
                            eState = CombatProcedureBeInCover::COVER_STATE_MOVING_OUT;
                            if (bledgeCover && bcanSneak) {
                                SetMoveMode(2);
                            }
                            else {
                                SetMoveMode(1);
                            }
                        }
                        else {
                            SetFailed("Cannot setup path to Fire position");
                        }
                    }
                }
                else {
                    pattacker->QueueReload();
                }
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_MOVING_OUT: {
            if (pattacker->DidPathingFail()) {
                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while moving out");
                }
                else {
                    SetFailed("Path Building failed to move out");
                }
            }
            else if (pattacker->IsPathingComplete()) {
                eState = CombatProcedureBeInCover::COVER_STATE_WAITING_OUT_OF_COVER;
                ClearLookAtTarget();
                float fwaitTime = pCombatController->GetCurrentCombatStyle()->GetRandomCoverWaitTimer();
                WaitingOutOfCoverTimer.Reset(fwaitTime);
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_WAITING_OUT_OF_COVER: {
            double flastAttack = static_cast<float>(pcombatState->GetLastAttackTimeStamp());
            if (WaitBehindCoverTimer.GetStartTime() < flastAttack) {
                eState = CombatProcedureBeInCover::COVER_STATE_FIRING_OUT_OF_COVER;
                float ffireTime = pCombatController->GetCurrentCombatStyle()->GetRandomFireTimer();
                FiringOutOfCoverTimer.Reset(ffireTime);
                TargetBlockedTimer.Clear();
            }
            else {
                if (WaitingOutOfCoverTimer.Check()) {
                    pcombatState->ClearCoverLocation(false);
                    SetFailed("Waited out of cover without firing");
                    return;
                }
                if (WaitingOutOfCoverTimer.ElapsedTime() > 2.0 &&
                    !pcombatState->GetHasTarget360LineOfSight() &&
                    pcombatGroup->GetTargetMemberLOSCount(ptarget)) {
                    pcombatState->ClearCoverLocation(false);
                    SetFailed("Waited out of cover and group member has LOS on target");
                    return;
                }
            }
            if (pcombatState->bTargetBlocked) {
                if (TargetBlockedTimer.IsSet()) {
                    if (TargetBlockedTimer.Check()) {
                        pcombatState->ClearCoverLocation(true);
                        SetFailed("Waited out of cover but was blocked");
                    }
                }
                else {
                    TargetBlockedTimer.Reset(2.0f);
                }
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_FIRING_OUT_OF_COVER: {
            float fcurrentAmmoClipPercent = pattacker->GetCurrentAmmoClipPercent();
            if (fCoverFiringReloadClipPercent.Float() > static_cast<double>(fcurrentAmmoClipPercent) ||
                pattacker->IsReloading() ||
                FiringOutOfCoverTimer.Check()) {
                PathingRequest request;
                TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
                TESObjectCELL* pParentCell = pattacker->GetParentCell();
                pattacker->BuildRequest(&request, CurrentCoverPosition, pParentCell, pWorldSpace, 0.0f, nullptr);
                if (pattacker->SetPathfindingGoal(&request)) {
                    int32_t iMoveMode = bledgeCover && bcanSneak ? 2 : 1;
                    SetMoveMode(iMoveMode);
                    eState = CombatProcedureBeInCover::COVER_STATE_MOVING_IN;
                }
                else {
                    SetFailed("Cannot setup path to move back to Cover position");
                }
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_MOVING_IN: {
            if (pattacker->DidPathingFail()) {
                const char* pReason = pattacker->IsPathValid() ? "Pathing failed while moving in"
                                                               : "Path Building failed to move in";
                SetFailed(pReason);
            }
            else if (pattacker->IsPathingComplete()) {
                eState = CombatProcedureBeInCover::COVER_STATE_WAITING_BEHIND_COVER;
                ClearLookAtTarget();
                SetMoveMode(2);
                float fwaitTime = pCombatController->GetCurrentCombatStyle()->GetRandomWaitTime();
                WaitBehindCoverTimer.Reset(fwaitTime);
            }
            else {
                float fdistToCoverSquared = (pattacker->GetLocationOnReference() - CurrentCoverPosition).SqrLength();
                if (fdistToCoverSquared < 1024.0) {
                    eState = CombatProcedureBeInCover::COVER_STATE_MOVING_IN_AND_ROTATE;
                    if (pCoverLocation->HasFireLocation()) {
                        SetLookAtTarget(LookAtPosition, 3);
                    }
                    SetMoveMode(1);
                }
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_MOVING_IN_AND_ROTATE: {
            if (pattacker->DidPathingFail()) {
                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while moving in and rotating");
                }
                else {
                    SetFailed("Path Building failed to move in and rotate");
                }
            }
            else if (pattacker->IsPathingComplete()) {
                eState = CombatProcedureBeInCover::COVER_STATE_WAITING_BEHIND_COVER;
                SetMoveMode(2);
                float fwaitTime = pCombatController->GetCurrentCombatStyle()->GetRandomWaitTime();
                WaitBehindCoverTimer.Reset(fwaitTime);
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_HOLDING_GROUND: {
            if (!breevaluatedCover || bcoverInvalid) {
                if (HoldingGroundTimer.Check()) {
                    pcombatState->ClearCoverLocation(false);
                    SetFailed("Target has LOS on Cover position");
                }
                else {
                    double felapsed = HoldingGroundTimer.ElapsedTime();
                    if (HoldingGroundTimer.GetTargetTime() / 4.0 < felapsed) {
                        NiPoint3 targetLoc = ptarget->GetLocationOnReference();
                        NiPoint3 attackerLoc = pattacker->GetLocationOnReference();
                        NiPoint3 targetVec = attackerLoc - targetLoc;
                        if (targetVec.SqrLength() < 262144.0) {
                            pcombatState->ClearCoverLocation(false);
                            SetFailed("Target has LOS on Cover position");
                        }
                    }
                }
            }
            else {
                PathingRequest request;
                TESWorldSpace* pWorldSpace = pattacker->GetWorldSpace();
                TESObjectCELL* pParentCell = pattacker->GetParentCell();
                pattacker->BuildRequest(&request, CurrentCoverPosition, pParentCell, pWorldSpace, 0.0f, nullptr);
                if (pattacker->SetPathfindingGoal(&request)) {
                    if (bledgeCover && bcanSneak) {
                        SetMoveMode(2);
                    }
                    else {
                        SetMoveMode(1);
                    }
                    eState = CombatProcedureBeInCover::COVER_STATE_MOVING_IN;
                }
                else {
                    SetFailed("Cannot setup path to move back to Cover position");
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void CombatProcedureBeInCover::Shutdown() {
    pCombatController->GetAttacker()->StopMoving();
    CombatProcedure::ClearMoveMode();
    CombatProcedure::ClearLookAtTarget();
}

// Each timer line passes the target time (read first) and the elapsed time as doubles.
void CombatProcedureBeInCover::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    switch (eState) {
        case CombatProcedureBeInCover::COVER_STATE_INITIALIZING: {
            apDebugText->PrintLine(aeLocation, aeJustify, "INITIALIZING");
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_WAITING_BEHIND_COVER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "WAITING_BEHIND_COVER");
            double ftarget = WaitBehindCoverTimer.GetTargetTime();
            double felapsed = WaitBehindCoverTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Wait Timer: %.02f/%.02f", felapsed, ftarget);
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_MOVING_OUT: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_OUT");
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_WAITING_OUT_OF_COVER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "WAITING_OUT_OF_COVER");
            double ftarget = WaitingOutOfCoverTimer.GetTargetTime();
            double felapsed = WaitingOutOfCoverTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Waiting Timer: %.02f/%.02f", felapsed, ftarget);
            if (TargetBlockedTimer.IsSet()) {
                double fblockedTarget = TargetBlockedTimer.GetTargetTime();
                double fblockedElapsed = TargetBlockedTimer.ElapsedTime();
                apDebugText->PrintLine(
                    aeLocation,
                    aeJustify,
                    "Target Blocked Timer: %.02f/%.02f",
                    fblockedElapsed,
                    fblockedTarget
                );
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_FIRING_OUT_OF_COVER: {
            apDebugText->PrintLine(aeLocation, aeJustify, "FIRING_OUT_OF_COVER");
            double ftarget = FiringOutOfCoverTimer.GetTargetTime();
            double felapsed = FiringOutOfCoverTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Firing Timer: %.02f/%.02f", felapsed, ftarget);
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_MOVING_IN: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_IN");
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_MOVING_IN_AND_ROTATE: {
            apDebugText->PrintLine(aeLocation, aeJustify, "MOVING_IN_AND_ROTATE");
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_HOLDING_GROUND: {
            apDebugText->PrintLine(aeLocation, aeJustify, "HOLDING_GROUND");
            double ftarget = HoldingGroundTimer.GetTargetTime();
            double felapsed = HoldingGroundTimer.ElapsedTime();
            apDebugText->PrintLine(aeLocation, aeJustify, "Holding Ground Timer: %.02f/%.02f", felapsed, ftarget);
            break;
        }
        default: {
            break;
        }
    }
    if (ReevaluateCoverTimer.IsSet()) {
        double ftarget = ReevaluateCoverTimer.GetTargetTime();
        double felapsed = ReevaluateCoverTimer.ElapsedTime();
        apDebugText->PrintLine(aeLocation, aeJustify, "Reevaluate Cover Timer: %.02f/%.02f", felapsed, ftarget);
    }
    apDebugText->PrintLine(
        aeLocation,
        aeJustify,
        "Cover Location: %.02f, %.02f, %.02f",
        CurrentCoverPosition.x,
        CurrentCoverPosition.y,
        CurrentCoverPosition.z
    );
    apDebugText->PrintLine(
        aeLocation,
        aeJustify,
        "Fire Location: %.02f, %.02f, %.02f",
        CurrentFirePosition.x,
        CurrentFirePosition.y,
        CurrentFirePosition.z
    );
}

void CombatProcedureBeInCover::FiredProjectile(Projectile* apProjectile) {
    COVER_STATE eState = this->eState;
    if (eState == 1) {
        float fwaitTime = static_cast<float>(pCombatController->GetCurrentCombatStyle()->GetRandomWaitTime());
        WaitBehindCoverTimer.Reset(fwaitTime);
    }
    else if (eState == 2) {
        CombatProcedure::GetAttacker()->StopMoving();
        CombatProcedure::ClearLookAtTarget();
        this->eState = COVER_STATE_FIRING_OUT_OF_COVER;
        float ffireTime = static_cast<float>(pCombatController->GetCurrentCombatStyle()->GetRandomFireTimer());
        FiringOutOfCoverTimer.Reset(ffireTime);
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureBeInCover::GetType() {
    return BE_IN_COVER;
}

void CombatProcedureBeInCover::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    bool bsecondaryCoverLocation;
    CombatProcedure::SaveGame(apSaveGameBuffer);
    bsecondaryCoverLocation = pCombatController->pCombatState->GetSecondaryCoverLocation() == pCoverLocation;
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(&bsecondaryCoverLocation, sizeof(bsecondaryCoverLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&CurrentCoverPosition, sizeof(CurrentCoverPosition), 0);
    apSaveGameBuffer->SaveDataEndian(&CurrentFirePosition, sizeof(CurrentFirePosition), 0);
    apSaveGameBuffer->SaveDataEndian(&LookAtPosition, sizeof(LookAtPosition), 0);
    WaitBehindCoverTimer.SaveGame(apSaveGameBuffer);
    WaitingOutOfCoverTimer.SaveGame(apSaveGameBuffer);
    FiringOutOfCoverTimer.SaveGame(apSaveGameBuffer);
    ReevaluateCoverTimer.SaveGame(apSaveGameBuffer);
    HoldingGroundTimer.SaveGame(apSaveGameBuffer);
    TargetBlockedTimer.SaveGame(apSaveGameBuffer);
}

void CombatProcedureBeInCover::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool bsecondaryCoverLocation;
    CombatProcedure::LoadGame(apLoadGameBuffer);
    bsecondaryCoverLocation = false;
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    apLoadGameBuffer->LoadData(&bsecondaryCoverLocation, sizeof(bsecondaryCoverLocation));
    CombatCoverLocation* pLoadedCoverLocation;
    if (bsecondaryCoverLocation) {
        pLoadedCoverLocation = pCombatController->pCombatState->GetSecondaryCoverLocation();
    }
    else {
        pLoadedCoverLocation = pCombatController->pCombatState->GetCoverLocation();
    }
    pCoverLocation = pLoadedCoverLocation;
    apLoadGameBuffer->LoadData(&CurrentCoverPosition, sizeof(CurrentCoverPosition));
    apLoadGameBuffer->LoadData(&CurrentFirePosition, sizeof(CurrentFirePosition));
    apLoadGameBuffer->LoadData(&LookAtPosition, sizeof(LookAtPosition));
    WaitBehindCoverTimer.LoadGame(apLoadGameBuffer);
    WaitingOutOfCoverTimer.LoadGame(apLoadGameBuffer);
    FiringOutOfCoverTimer.LoadGame(apLoadGameBuffer);
    ReevaluateCoverTimer.LoadGame(apLoadGameBuffer);
    HoldingGroundTimer.LoadGame(apLoadGameBuffer);
    TargetBlockedTimer.LoadGame(apLoadGameBuffer);
}

void CombatProcedureBeInCover::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

double TESCombatStyle::GetRandomCoverWaitTimer() {
    return RandomFloat(SimpleData.fCoverWaitToFireTimerMin, SimpleData.fCoverWaitToFireTimerMax);
}
