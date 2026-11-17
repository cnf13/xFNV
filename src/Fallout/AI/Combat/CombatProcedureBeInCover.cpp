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

// The positions are deliberately left unset, as the game does.
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
                    bool bcoverSafe =
                        pcombatState->CheckCoverSafe(pCoverLocation->CoverLocation, CurrentCoverPosition, ptarget, 1);
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
                    if (targetVec.SqrLength() > 589824.0) {
                        if (pcombatGroup->GetTargetMemberLOSCount(ptarget)) {
                            pCoverLocation->bDirtyFlag = true;
                        }
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
                pCombatController, pCoverLocation, &CurrentCoverPosition, &CurrentFirePosition
            );
            LookAtPosition.x = CurrentFirePosition.x;
            LookAtPosition.y = CurrentFirePosition.y;
            LookAtPosition.z = CurrentFirePosition.z;
            if (pCombatController->GetAttackerCanSneak()) {
                fheight = pattacker->GetSneakHeight(false);
            }
            else {
                fheight = pattacker->GetHeight();
            }
            LookAtPosition.z = fheight * 0.75 + LookAtPosition.z;
            PathingRequest request;
            TESWorldSpace* WorldSpace = pattacker->GetWorldSpace();
            TESObjectCELL* ParentCell = pattacker->GetParentCell();
            pattacker->BuildRequest(&request, CurrentCoverPosition, ParentCell, WorldSpace, 0.0f, nullptr);
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
                double CurrentAmmoClipPercent;
                if (!(pcombatState->GetLastAttackTimeStamp().ElapsedTime() > 0.5) ||
                    (CurrentAmmoClipPercent = pattacker->GetCurrentAmmoClipPercent(),
                     !(fCoverWaitReloadClipPercent.Float() > CurrentAmmoClipPercent))) {
                    if (WaitBehindCoverTimer.Check()) {
                        PathingRequest request_2;
                        TESWorldSpace* v16 = pattacker->GetWorldSpace();
                        TESObjectCELL* v12 = pattacker->GetParentCell();
                        pattacker->BuildRequest(&request_2, CurrentFirePosition, v12, v16, 0.0f, nullptr);
                        if (pattacker->SetPathfindingGoal(&request_2)) {
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
            else {
                if (pattacker->IsPathingComplete()) {
                    eState = CombatProcedureBeInCover::COVER_STATE_WAITING_OUT_OF_COVER;
                    ClearLookAtTarget();
                    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
                    float a2 = CurrentCombatStyle->GetRandomCoverWaitTimer();
                    WaitingOutOfCoverTimer.Reset(a2);
                }
            }
            break;
        }
        case CombatProcedureBeInCover::COVER_STATE_WAITING_OUT_OF_COVER: {
            double flastAttack = static_cast<float>(pcombatState->GetLastAttackTimeStamp());
            if (!(WaitBehindCoverTimer.GetStartTime() < flastAttack)) {
                if (WaitingOutOfCoverTimer.Check()) {
                    pcombatState->ClearCoverLocation(false);
                    SetFailed("Waited out of cover without firing");
                    return;
                }
                if (WaitingOutOfCoverTimer.ElapsedTime() > 2.0 &&
                    (!pcombatState->GetHasTarget360LineOfSight() && pcombatGroup->GetTargetMemberLOSCount(ptarget))) {
                    pcombatState->ClearCoverLocation(false);
                    SetFailed("Waited out of cover and group member has LOS on target");
                    return;
                }
            }
            else {
                eState = CombatProcedureBeInCover::COVER_STATE_FIRING_OUT_OF_COVER;
                TESCombatStyle* v6 = pCombatController->GetCurrentCombatStyle();
                float a2a = v6->GetRandomFireTimer();
                FiringOutOfCoverTimer.Reset(a2a);
                TargetBlockedTimer.Clear();
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
                (pattacker->IsReloading() || FiringOutOfCoverTimer.Check())) {
                PathingRequest request_2;
                TESWorldSpace* v17 = pattacker->GetWorldSpace();
                TESObjectCELL* v13 = pattacker->GetParentCell();
                pattacker->BuildRequest(&request_2, CurrentCoverPosition, v13, v17, 0.0f, nullptr);
                if (pattacker->SetPathfindingGoal(&request_2)) {
                    int32_t v24;
                    if (bledgeCover && bcanSneak) {
                        v24 = 2;
                    }
                    else {
                        v24 = 1;
                    }
                    SetMoveMode(v24);
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
                const char* v23;
                if (pattacker->IsPathValid()) {
                    v23 = "Pathing failed while moving in";
                }
                else {
                    v23 = "Path Building failed to move in";
                }
                SetFailed(v23);
            }
            else {
                if (pattacker->IsPathingComplete()) {
                    eState = CombatProcedureBeInCover::COVER_STATE_WAITING_BEHIND_COVER;
                    ClearLookAtTarget();
                    SetMoveMode(2);
                    TESCombatStyle* v7 = pCombatController->GetCurrentCombatStyle();
                    float a2b = v7->GetRandomWaitTime();
                    WaitBehindCoverTimer.Reset(a2b);
                }
                else {
                    const NiPoint3* v8 = &pattacker->GetLocationOnReference();
                    float fdistToCoverSquared = (*v8 - CurrentCoverPosition).SqrLength();
                    if (fdistToCoverSquared < 1024.0) {
                        eState = CombatProcedureBeInCover::COVER_STATE_MOVING_IN_AND_ROTATE;
                        if (pCoverLocation->HasFireLocation()) {
                            SetLookAtTarget(LookAtPosition, 3);
                        }
                        SetMoveMode(1);
                    }
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
            else {
                if (pattacker->IsPathingComplete()) {
                    eState = CombatProcedureBeInCover::COVER_STATE_WAITING_BEHIND_COVER;
                    SetMoveMode(2);
                    TESCombatStyle* v10 = pCombatController->GetCurrentCombatStyle();
                    float a2c = v10->GetRandomWaitTime();
                    WaitBehindCoverTimer.Reset(a2c);
                }
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
                        NiPoint3 targetLoc_2 = ptarget->GetLocationOnReference();
                        NiPoint3 attackerLoc = pattacker->GetLocationOnReference();
                        NiPoint3 targetVec_2 = attackerLoc - targetLoc_2;
                        if (targetVec_2.SqrLength() < 262144.0) {
                            pcombatState->ClearCoverLocation(false);
                            SetFailed("Target has LOS on Cover position");
                        }
                    }
                }
            }
            else {
                PathingRequest request_3;
                TESWorldSpace* v18 = pattacker->GetWorldSpace();
                TESObjectCELL* v14 = pattacker->GetParentCell();
                pattacker->BuildRequest(&request_3, CurrentCoverPosition, v14, v18, 0.0f, nullptr);
                if (pattacker->SetPathfindingGoal(&request_3)) {
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
            return;
        }
    }
}

void CombatProcedureBeInCover::Shutdown() {
    Actor* attacker = pCombatController->GetAttacker();
    attacker->StopMoving();
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
                    aeLocation, aeJustify, "Target Blocked Timer: %.02f/%.02f", fblockedElapsed, fblockedTarget
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
        aeLocation, aeJustify, "Cover Location: %.02f, %.02f, %.02f", CurrentCoverPosition.x, CurrentCoverPosition.y,
        CurrentCoverPosition.z
    );
    apDebugText->PrintLine(
        aeLocation, aeJustify, "Fire Location: %.02f, %.02f, %.02f", CurrentFirePosition.x, CurrentFirePosition.y,
        CurrentFirePosition.z
    );
}

void CombatProcedureBeInCover::FiredProjectile(Projectile* apProjectile) {
    TESCombatStyle* currentCombatStyle;
    Actor* attacker;
    TESCombatStyle* v4;
    float a2;
    float a2a;
    COVER_STATE eState;
    eState = this->eState;
    if (eState == 1) {
        currentCombatStyle = pCombatController->GetCurrentCombatStyle();
        a2 = (float)currentCombatStyle->GetRandomWaitTime();
        (&WaitBehindCoverTimer)->Reset(a2);
    }
    else {
        if (eState == 2) {
            attacker = CombatProcedure::GetAttacker();
            attacker->StopMoving();
            CombatProcedure::ClearLookAtTarget();
            this->eState = COVER_STATE_FIRING_OUT_OF_COVER;
            v4 = pCombatController->GetCurrentCombatStyle();
            a2a = (float)v4->GetRandomFireTimer();
            (&FiringOutOfCoverTimer)->Reset(a2a);
        }
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureBeInCover::GetType() {
    return BE_IN_COVER;
}

void CombatProcedureBeInCover::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    bool bsecondaryCoverLocation;
    CombatProcedure::SaveGame(apSaveGameBuffer);
    bsecondaryCoverLocation = pCombatController->pCombatState->GetSecondaryCoverLocation() == pCoverLocation;
    apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bsecondaryCoverLocation, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&CurrentCoverPosition, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&CurrentFirePosition, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&LookAtPosition, 12u, 0);
    WaitBehindCoverTimer.SaveGame(apSaveGameBuffer);
    WaitingOutOfCoverTimer.SaveGame(apSaveGameBuffer);
    FiringOutOfCoverTimer.SaveGame(apSaveGameBuffer);
    ReevaluateCoverTimer.SaveGame(apSaveGameBuffer);
    HoldingGroundTimer.SaveGame(apSaveGameBuffer);
    TargetBlockedTimer.SaveGame(apSaveGameBuffer);
}

void CombatProcedureBeInCover::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatCoverLocation* SecondaryCoverLocation;
    bool bsecondaryCoverLocation;
    CombatProcedure::LoadGame(apLoadGameBuffer);
    bsecondaryCoverLocation = false;
    apLoadGameBuffer->LoadData(&eState, 4u);
    apLoadGameBuffer->LoadData(&bsecondaryCoverLocation, 1u);
    if (bsecondaryCoverLocation) {
        SecondaryCoverLocation = pCombatController->pCombatState->GetSecondaryCoverLocation();
    }
    else {
        SecondaryCoverLocation = pCombatController->pCombatState->GetCoverLocation();
    }
    pCoverLocation = SecondaryCoverLocation;
    apLoadGameBuffer->LoadData(&CurrentCoverPosition, 12u);
    apLoadGameBuffer->LoadData(&CurrentFirePosition, 12u);
    apLoadGameBuffer->LoadData(&LookAtPosition, 12u);
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
