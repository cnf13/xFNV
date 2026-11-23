#include "Fallout/AI/Combat/CombatProcedureAttackMelee.h"

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/string.h"

BSPerformanceTimer CombatAI_Melee("Combat AI", "Melee", true, true);

CombatMovementRestrictions::CombatMovementRestrictions()
  : bReset(false), bInitialized(false), cCurrentUpdate(0), sRestrictedMoves(0), bHasMoveLeftAnim(false),
    bHasMoveRightAnim(false), bHasMoveForwardAnim(false), bHasMoveBackwardAnim(false), Location(NiPoint3::ZERO) {}

bool CombatMovementRestrictions::CanMove(uint16_t asMoveMode) {
    return (asMoveMode & sRestrictedMoves) == 0;
}

void CombatMovementRestrictions::SetRestrictedMove(int16_t asMoveMode, char acAllow) {
    if (acAllow) {
        sRestrictedMoves &= ~asMoveMode;
    }
    else {
        sRestrictedMoves |= asMoveMode;
    }
}

void CombatMovementRestrictions::SetReset(bool abValue) {
    bReset = true;
}

char CombatMovementRestrictions::IsLocationSafe(TESObjectREFR* apRef, NiPoint3* arTarget) {
    PathingLocation kLocation(apRef);
    return Pathing::CheckLineOfSight(kLocation, *arTarget, nullptr, nullptr);
}

int32_t CombatMovementRestrictions::CheckMovementRestrictions(Actor* apActor, uint32_t auiDirection) {
    NiMatrix3 kRotation;
    if (!bInitialized) {
        bHasMoveLeftAnim = apActor->pCurrentProcess->HasMovementAnim(ANIM_GROUP_MOVE_LEFT);
        bHasMoveRightAnim = apActor->pCurrentProcess->HasMovementAnim(ANIM_GROUP_MOVE_RIGHT);
        bHasMoveForwardAnim = apActor->pCurrentProcess->HasMovementAnim(ANIM_GROUP_MOVE_FORWARD);
        bHasMoveBackwardAnim = apActor->pCurrentProcess->HasMovementAnim(ANIM_GROUP_MOVE_BACK);
        bInitialized = true;
    }
    if (!bReset && apActor->IsPathing()) {
        return 4;
    }
    if (!bReset && !auiDirection) {
        double dDistance = (Location - apActor->GetLocationOnReference()).Length();
        if (fAIUpdateMovementRestrictionsDistance.Float() > dDistance) {
            return 4;
        }
    }
    Location = apActor->GetLocationOnReference();
    kRotation.MakeZRotation(apActor->GetHeading(false));
    float fWidth = apActor->GetWidth();
    if (fWidth == 0.0) {
        fWidth = 32.0f;
    }
    float fLength = apActor->GetLength();
    if (fLength == 0.0) {
        fLength = fWidth;
    }
    float fRatio = fWidth / fLength;
    if (fRatio > 0.5 && fRatio < 2.0) {
        fWidth = BSMAX(fWidth, fLength);
        fLength = fWidth;
    }
    fWidth = BSMIN(fWidth, 64.0f);
    fLength = BSMIN(fLength, 64.0f);
    uint32_t uiMoveMode = apActor->GetMoveMode();
    switch (auiDirection) {
        case 0u: {
            NiPoint3 kPoint(0.0f, -fLength, 0.0f);
            if ((uiMoveMode & 2) != 0) {
                kPoint.y = kPoint.y - fLength;
            }
            kPoint = kRotation * kPoint + Location;
            if (bHasMoveBackwardAnim) {
                SetRestrictedMove(2, IsLocationSafe(apActor, &kPoint));
            }
            else {
                SetRestrictedMove(2, 0);
            }
            break;
        }
        case 1u: {
            NiPoint3 kPoint(0.0f, fLength, 0.0f);
            if ((uiMoveMode & 1) != 0) {
                kPoint.y = kPoint.y + fLength;
            }
            kPoint = kRotation * kPoint + Location;
            if (bHasMoveForwardAnim) {
                SetRestrictedMove(1, IsLocationSafe(apActor, &kPoint));
            }
            else {
                SetRestrictedMove(1, 0);
            }
            break;
        }
        case 2u: {
            NiPoint3 kPoint(-fWidth, 0.0f, 0.0f);
            if ((uiMoveMode & 4) != 0) {
                kPoint.x = kPoint.x - fWidth;
            }
            kPoint = kRotation * kPoint + Location;
            if (bHasMoveLeftAnim) {
                SetRestrictedMove(4, IsLocationSafe(apActor, &kPoint));
            }
            else {
                SetRestrictedMove(4, 0);
            }
            break;
        }
        case 3u: {
            NiPoint3 kPoint(fWidth, 0.0f, 0.0f);
            if ((uiMoveMode & 8) != 0) {
                kPoint.x = kPoint.x + fWidth;
            }
            kPoint = kRotation * kPoint + Location;
            if (bHasMoveRightAnim) {
                SetRestrictedMove(8, IsLocationSafe(apActor, &kPoint));
            }
            else {
                SetRestrictedMove(8, 0);
            }
            break;
        }
        default: {
            break;
        }
    }
    return 1;
}

void CombatMovementRestrictions::Update(CombatController* apCombatController) {
    if (UpdateTimer.Check() || cCurrentUpdate) {
        cCurrentUpdate += CheckMovementRestrictions(
            static_cast<PlayerCharacter*>(apCombatController->pAttacker),
            cCurrentUpdate
        );
    }
    if (cCurrentUpdate >= 4) {
        bReset = false;
        cCurrentUpdate = 0;
        UpdateTimer.Reset(fMeleeMovementRestrictionsUpdateTime.Float());
    }
}

CombatProcedureAttackMelee::CombatProcedureAttackMelee()
  : eManeuverState(ENGAGE), eCombatState(DONE), eLastCombatState(DONE), eCurrentAttack(ANIM_GROUP_NONE),
    bRangedAllyWantsToShoot(false), iAdvanceMoveMode(256), fCurrentAttackReach(-1.0f), fFacingTargetDistance(-1.0f) {
    // The attack-pick-failed time stamp is a timer's start time set to -FLT_MAX.
    CombatTimer kTimer;
    kTimer.SetStartTime(-0x1.fffffe0000000p+127f);
    fAttackPickFailedTimestamp.fTimeStamp = kTimer.fStartTime;
}

CombatProcedureAttackMelee::~CombatProcedureAttackMelee() {}

void CombatProcedureAttackMelee::Update() {
    BGSWorldLocation kLocation;
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    fFacingTargetDistance = -1.0f;
    CombatAI_Melee.Start();
    MovementRestrictions.Update(pCombatController);
    AttackDecision();
    if (eManeuverState == 3 || eManeuverState == 4) {
        if (!pAttacker->DidPathingFail() || pAttacker->IsPathValid()) {
            if (pAttacker->IsPathValid()) {
                TargetUnreachableTimer.Clear();
            }
        }
        else if (TargetUnreachableTimer.IsSet()) {
            if (TargetUnreachableTimer.Check()) {
                pCombatController->pCombatState->SetFlag(UNREACHABLE_NEAR_TARGET_LOCATION);
                BGSWorldLocation* pWorldLocation = pTarget->GetWorldLocation(&kLocation);
                pCombatController->pCombatState->AddUnreachableLocation(*pWorldLocation);
                SetFailed("Could not reach target");
            }
        }
        else {
            TargetUnreachableTimer.Reset(2.0f);
        }
    }
    else {
        TargetUnreachableTimer.Clear();
    }
    switch (eManeuverState) {
        case 0u: {
            HandleEngage();
            break;
        }
        case 1u: {
            HandleIdle();
            break;
        }
        case 2u: {
            HandleDodge();
            break;
        }
        case 3u: {
            HandleClose();
            break;
        }
        case 4u: {
            HandleAdvance();
            break;
        }
        default: {
            break;
        }
    }
    bool bUseLookAtTarget = false;
    if (eCombatState == 3) {
        if (eManeuverState == 3) {
            bUseLookAtTarget = true;
        }
        else {
            float fDistance = GetFacingTargetDistance();
            float fReach = GetCurrentAttackReach();
            double dDistanceOutOfReach = fDistance - fReach;
            if (pAttacker->GetScale() * fCombatDistance.Float() * 2.25 > dDistanceOutOfReach) {
                bUseLookAtTarget = true;
            }
        }
    }
    else {
        bUseLookAtTarget = true;
    }
    if (bUseLookAtTarget) {
        NiPoint3 targetLoc = CombatUtilities::GetTargetFiringLocation(pTarget, ACTOR_SEGMENT_INVIEW_TOP);
        SetLookAtTarget(targetLoc, 4);
    }
    else {
        ClearLookAtTarget();
    }
    CombatAI_Melee.Stop();
}

void CombatProcedureAttackMelee::Initialize(CombatController* apCombatController) {
    CombatProcedure::Initialize(apCombatController);
}

void CombatProcedureAttackMelee::Shutdown() {
    Actor* pAttacker = CombatProcedure::GetAttacker();
    CombatProcedure::ClearLookAtTarget();
    pAttacker->StopMoving();
    pAttacker->SetBlock(false);
}

// Each timer's target time is read, then its elapsed time; both are passed on as doubles.
void CombatProcedureAttackMelee::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    char cText[108];
    Actor* pAttacker = GetAttacker();
    int16_t sMoveMode = pAttacker->GetMoveMode();
    std::memset(cText, 0, 100);
    CombatProcedureAttackMelee::Maneuver eManeuverState = this->eManeuverState;
    if (eManeuverState == CombatProcedureAttackMelee::IDLE) {
        double dTargetTime = ManeuverTimer.GetTargetTime();
        double dElapsedTime = ManeuverTimer.ElapsedTime();
        BSsprintf(cText, 100, "%.2f/%.2f", dElapsedTime, dTargetTime);
    }
    else if (eManeuverState == CombatProcedureAttackMelee::DODGE) {
        const char* pDirection;
        if ((sMoveMode & 1) != 0) {
            pDirection = " FORWARD ";
        }
        else if ((sMoveMode & 2) != 0) {
            pDirection = " BACKWARD ";
        }
        else if ((sMoveMode & 4) != 0) {
            pDirection = " LEFT ";
        }
        else if ((sMoveMode & 8) != 0) {
            pDirection = " RIGHT ";
        }
        else {
            pDirection = "";
        }
        double dTargetTime = ManeuverTimer.GetTargetTime();
        double dElapsedTime = ManeuverTimer.ElapsedTime();
        BSsprintf(cText, 100, "%s %.2f/%.2f", pDirection, dElapsedTime, dTargetTime);
    }
    else {
        cText[0] = 0;
    }
    apDebugText->PrintLine(aeLocation, aeJustify, "Maneuver: %s %s", pManeuverNames[this->eManeuverState], cText);
    CombatProcedureAttackMelee::CombatState eCombatState = this->eCombatState;
    if (eCombatState) {
        if (eCombatState == CombatProcedureAttackMelee::HOLD) {
            double dTargetTime = MeleeTimer.GetTargetTime();
            double dElapsedTime = MeleeTimer.ElapsedTime();
            BSsprintf(cText, 100, "%.2f/%.2f", dElapsedTime, dTargetTime);
        }
        else {
            cText[0] = 0;
        }
    }
    else if (eCurrentAttack == ANIM_GROUP_ATTACK_LEFT) {
        BSstrcpy(cText, 100, "LEFT");
    }
    else {
        const char* pAttackName;
        if (eCurrentAttack == ANIM_GROUP_ATTACK_RIGHT) {
            pAttackName = "RIGHT";
        }
        else if (eCurrentAttack == ANIM_GROUP_ATTACK_NORMAL_POWER) {
            pAttackName = "NORMAL POWER";
        }
        else if (eCurrentAttack == ANIM_GROUP_ATTACK_FORWARD_POWER) {
            pAttackName = "FORWARD POWER";
        }
        else if (eCurrentAttack == ANIM_GROUP_ATTACK_BACK_POWER) {
            pAttackName = "BACK POWER";
        }
        else if (eCurrentAttack == ANIM_GROUP_ATTACK_LEFT_POWER) {
            pAttackName = "LEFT POWER";
        }
        else if (eCurrentAttack == ANIM_GROUP_ATTACK_RIGHT_POWER) {
            pAttackName = "RIGHT POWER";
        }
        else {
            pAttackName = "";
        }
        BSstrcpy(cText, 100, pAttackName);
    }
    if (TargetUnreachableTimer.IsSet()) {
        double dTargetTime = TargetUnreachableTimer.GetTargetTime();
        double dElapsedTime = TargetUnreachableTimer.ElapsedTime();
        apDebugText->PrintLine(aeLocation, aeJustify, "Target Unreachable Timer: %.2f/%.2f", dElapsedTime, dTargetTime);
    }
    apDebugText->PrintLine(aeLocation, aeJustify, "Melee: %s %s", pCombatStateNames[this->eCombatState], cText);
    if (MovementRestrictions.sRestrictedMoves && !pAttacker->IsPathing()) {
        const char* pBackward = MovementRestrictions.CanMove(2) ? "Y" : "N";
        const char* pForward = MovementRestrictions.CanMove(1) ? "Y" : "N";
        const char* pRight = MovementRestrictions.CanMove(8) ? "Y" : "N";
        const char* pLeft = MovementRestrictions.CanMove(4) ? "Y" : "N";
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Movement Restrictions: L:%s R:%s F:%s B:%s",
            pLeft,
            pRight,
            pForward,
            pBackward
        );
    }

    if (MovementRestrictions.bReset) {
        apDebugText->PrintLine(aeLocation, aeJustify, "Reset Movement restrictions");
    }
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureAttackMelee::GetType() {
    return ATTACK_MELEE;
}

void CombatProcedureAttackMelee::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&eManeuverState, sizeof(eManeuverState), 0);
    apSaveGameBuffer->SaveDataEndian(&eCombatState, sizeof(eCombatState), 0);
    apSaveGameBuffer->SaveDataEndian(&eLastCombatState, sizeof(eLastCombatState), 0);
    apSaveGameBuffer->SaveDataEndian(&eCurrentAttack, sizeof(eCurrentAttack), 0);
    MeleeTimer.SaveGame(apSaveGameBuffer);
    ManeuverTimer.SaveGame(apSaveGameBuffer);
    DodgeTimer.SaveGame(apSaveGameBuffer);
    AdvanceTimer.SaveGame(apSaveGameBuffer);
    CloseTimer.SaveGame(apSaveGameBuffer);
    TargetUnreachableTimer.SaveGame(apSaveGameBuffer);
    fAttackPickFailedTimestamp.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bRangedAllyWantsToShoot, sizeof(bRangedAllyWantsToShoot), 0);
    apSaveGameBuffer->SaveDataEndian(&iAdvanceMoveMode, sizeof(iAdvanceMoveMode), 0);
}

void CombatProcedureAttackMelee::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eManeuverState, sizeof(eManeuverState));
    apLoadGameBuffer->LoadData(&eCombatState, sizeof(eCombatState));
    apLoadGameBuffer->LoadData(&eLastCombatState, sizeof(eLastCombatState));
    apLoadGameBuffer->LoadData(&eCurrentAttack, sizeof(eCurrentAttack));
    MeleeTimer.LoadGame(apLoadGameBuffer);
    ManeuverTimer.LoadGame(apLoadGameBuffer);
    DodgeTimer.LoadGame(apLoadGameBuffer);
    AdvanceTimer.LoadGame(apLoadGameBuffer);
    CloseTimer.LoadGame(apLoadGameBuffer);
    TargetUnreachableTimer.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        fAttackPickFailedTimestamp.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&bRangedAllyWantsToShoot, sizeof(bRangedAllyWantsToShoot));
    apLoadGameBuffer->LoadData(&iAdvanceMoveMode, sizeof(iAdvanceMoveMode));
}

void CombatProcedureAttackMelee::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

void CombatProcedureAttackMelee::CheckUpdateMovementRestrictions() {
    if (!Main::GetSceneIsComplex()) {
        NiPoint3 kDelta;
        Actor* pAttacker = GetAttacker();
        kDelta = MovementRestrictions.Location - pAttacker->GetLocationOnReference();
        double dDistanceSqr = kDelta.SqrLength();
        if (fAIUpdateMovementRestrictionsDistance.Float() * fAIUpdateMovementRestrictionsDistance.Float() <=
            dDistanceSqr) {
            UpdateMovementRestrictions();
        }
    }
}

void CombatProcedureAttackMelee::StopMoving() {
    Actor* pAttacker = GetAttacker();
    pAttacker->ClearForcedMoveMode();
    pAttacker->StopMoving();
    CheckUpdateMovementRestrictions();
}

void CombatProcedureAttackMelee::SetManeuverState(CombatProcedureAttackMelee::Maneuver aeManeuverState) {
    if (eManeuverState != aeManeuverState) {
        if (!aeManeuverState && eManeuverState == 4 && IsTargetMovingAway()) {
            return;
        }
        if (aeManeuverState == 4 || eManeuverState == 4) {}
        UpdateMovementRestrictions();
    }
    eManeuverState = static_cast<Maneuver>(aeManeuverState);
}

void CombatProcedureAttackMelee::StartEngage() {
    SetManeuverState(CombatProcedureAttackMelee::ENGAGE);
}

void CombatProcedureAttackMelee::ManeuverDecision() {
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    if (pTarget) {
        bool bTargetAttacking = pTarget->GetAnimAction() == ANIM_ACTION_ATTACK;
        bool bTargetRanged = Actor::IsUsingNonMeleeWeapon(pTarget);
        float fFatiguePercent = pAttacker->GetFatiguePercentage();
        int32_t iWalkSpeed = static_cast<int32_t>(pAttacker->GetWalkSpeed());
        float fDodgeScore = CombatFormulas::ChooseDodge(
            pCombatController->GetCurrentCombatStyle(),
            iWalkSpeed,
            fFatiguePercent,
            bTargetAttacking
        );
        if (fDodgeScore < 0.0) {
            fDodgeScore = 0.0f;
        }
        if (this->bRangedAllyWantsToShoot) {
            fDodgeScore = fDodgeScore + 25.0;
        }
        if (eCombatState == CombatProcedureAttackMelee::HOLD) {
            fDodgeScore = fDodgeScore + 10.0;
        }
        if (pTarget != pPlayer && !pTarget->IsInCombat()) {
            fDodgeScore = fDodgeScore - 100.0;
        }
        if (pAttacker->IsSwimming()) {
            fDodgeScore = 0.0f;
        }
        float fDieRoll = static_cast<float>(Random() % 100);
        if (!(fDodgeScore >= static_cast<double>(fDieRoll))) {
            if (eManeuverState != CombatProcedureAttackMelee::IDLE) {
                float fMaxTime = pCombatController->GetCurrentCombatStyle()->Data.fIdleMaxTime;
                float fMinTime = pCombatController->GetCurrentCombatStyle()->Data.fIdleMinTime;
                float fManeuverTime = CombatFormulas::GetRandomBetween(fMinTime, fMaxTime);
                ManeuverTimer.Reset(fManeuverTime);
                StopMoving();
                SetManeuverState(CombatProcedureAttackMelee::IDLE);
            }
        }
        else {
            bool bDodgerAttacking = pAttacker->GetAnimAction() == ANIM_ACTION_ATTACK;
            float fDistance = GetFacingTargetDistance();
            float fReach = GetCurrentAttackReach();
            uint16_t sRestrictedMoves = MovementRestrictions.sRestrictedMoves;
            bool bRangedAllyWantsToShoot = this->bRangedAllyWantsToShoot;
            uint16_t sMoveMode = CombatFormulas::SelectDodgeDirection(
                pCombatController->GetCurrentCombatStyle(),
                bDodgerAttacking,
                bTargetAttacking,
                fDistance,
                fReach,
                bRangedAllyWantsToShoot,
                bTargetRanged,
                sRestrictedMoves
            );
            if (sMoveMode && GetAttacker()->GetAnimation()) {
                float fManeuverTime;
                float fMinTime;
                float fMaxTime;
                ANIM_GROUP_ENUM eMove = ANIM_GROUP_MOVE_FORWARD;
                int32_t iDirection = sMoveMode & 15;
                if (iDirection == 2) {
                    eMove = ANIM_GROUP_MOVE_BACK;
                    fMaxTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeBackwardMaxTime;
                    fMinTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeBackwardMinTime;
                    fManeuverTime = CombatFormulas::GetRandomBetween(fMinTime, fMaxTime);
                }
                else if (iDirection == 4) {
                    eMove = ANIM_GROUP_MOVE_LEFT;
                    GetMovementRestrictionFlag(ActorMover::MF_LEFT);
                    fMaxTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeLeftRightMaxTime;
                    fMinTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeLeftRightMinTime;
                    fManeuverTime = CombatFormulas::GetRandomBetween(fMinTime, fMaxTime);
                }
                else {
                    if (iDirection == 8) {
                        eMove = ANIM_GROUP_MOVE_RIGHT;
                        GetMovementRestrictionFlag(ActorMover::MF_RIGHT);
                        fMaxTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeLeftRightMaxTime;
                        fMinTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeLeftRightMinTime;
                    }
                    else {
                        fMaxTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeForwardMaxTime;
                        fMinTime = pCombatController->GetCurrentCombatStyle()->Data.fDodgeForwardMinTime;
                    }
                    fManeuverTime = CombatFormulas::GetRandomBetween(fMinTime, fMaxTime);
                }
                uint16_t sAnimGroup = GetAttacker()->GetAnimGroup(eMove, nullptr, false, nullptr);
                if (TESAnimGroup::GetType(sAnimGroup) == eMove) {
                    float fMoveCycleDuration = pAttacker->GetAnimActionTime(eMove);
                    fManeuverTime = BSMAX(fMoveCycleDuration, fManeuverTime);
                    if (bTargetAttacking && fAIDodgeWalkChance.Float() < static_cast<double>(static_cast<float>(0.0))) {
                        pAttacker->ForceMoveMode(sMoveMode | 512);
                    }
                    else {
                        pAttacker->ForceMoveMode(sMoveMode | 256);
                    }
                    ManeuverTimer.Reset(fManeuverTime);
                    DodgeTimer.Reset(fMoveCycleDuration);
                    SetManeuverState(CombatProcedureAttackMelee::DODGE);
                }
                else if (CombatController::GetShowVerboseMessages()) {
                    const char* pDirection;
                    if ((sMoveMode & 4) != 0) {
                        pDirection = "LEFT";
                    }
                    else if ((sMoveMode & 8) != 0) {
                        pDirection = "RIGHT";
                    }
                    else if ((sMoveMode & 1) != 0) {
                        pDirection = "FOREWARD";
                    }
                    else {
                        pDirection = "BACK";
                    }
                    Interface::PrintLine(
                        "%.20s chose to dodge %s but has no corresponding animation",
                        pAttacker->GetFullName(),
                        pDirection
                    );
                }
            }
        }
    }
}

void CombatProcedureAttackMelee::AttackDecision() {
    Actor* pTarget = GetTarget();
    switch (eCombatState) {
        case 0u: {
            HandleAttack();
            break;
        }
        case 1u: {
            HandleBlock();
            break;
        }
        case 2u: {
            HandleHold();
            break;
        }
        case 3u: {
            int32_t iFadeState;
            if (pTarget && pTarget->pCurrentProcess) {
                iFadeState = pTarget->pCurrentProcess->GetFadeState();
            }
            else {
                iFadeState = 0;
            }
            if (!static_cast<uint8_t>(pCombatController->IsNotDetectingTarget()) &&
                !iFadeState &&
                (!pTarget || !pTarget->IsDead(false))) {
                MeleeDecision();
            }
            break;
        }
        default: {
            break;
        }
    }
}

void CombatProcedureAttackMelee::HandleClose() {
    NiPoint3 currentGoal;
    if (eManeuverState == CombatProcedureAttackMelee::CLOSE) {
        Actor* pAttacker = GetAttacker();
        Actor* pTarget = GetTarget();
        float fDistance = GetFacingTargetDistance();
        float fReach = GetCurrentAttackReach();
        uint16_t sMoveMode = pAttacker->GetMoveMode();
        if (pAttacker->IsPathing()) {
            float fAttackerReach = GetCurrentAttackReach();
            NiPoint3 faceTargetLoc = CombatUtilities::ComputeFaceTargetLocation(
                pAttacker,
                pTarget,
                pTarget->GetLocationOnReference(),
                fAttackerReach
            );
            if (!(fDistance > fCombatMinEngageDistance.Float() + fCombatMinEngageDistance.Float())) {
                if (WithinAttackRange(fDistance, fReach, nullptr) && pCombatController->HasLineOfSight()) {
                    bool bGoalWithinRange = false;
                    if (pAttacker->IsWaitingOnPath()) {
                        bGoalWithinRange = true;
                    }
                    else if (pAttacker->GetCurrentPathfindingGoal(&currentGoal)) {
                        float fActorExtentSum = CombatUtilities::GetAttackerTargetExtentSum(pAttacker, pTarget);
                        float fFaceDistance = (currentGoal - pTarget->GetLocationOnReference()).XYLength() -
                            fActorExtentSum;
                        bGoalWithinRange = CombatProcedure::WithinAttackRange(fFaceDistance, fReach, 0);
                    }
                    if (!bGoalWithinRange) {
                        MoveToFaceTargetLocation(faceTargetLoc);
                    }
                }
                else if (CombatProcedure::WithinAttackRange(fDistance, fReach, 0)) {
                    if ((sMoveMode & 2) != 0 && fCombatMinEngageDistance.Float() > static_cast<double>(fDistance)) {
                        AttackDecision();
                    }
                    else {
                        StartEngage();
                    }
                }
            }
            else if (!eCombatState) {
                StartAdvance();
            }
        }
        else {
            StartEngage();
        }
    }
}

void CombatProcedureAttackMelee::MoveToFaceTargetLocation(NiPoint3& arFaceTargetLocation) {
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    PathingRequest pathingRequest;
    PathingLocation kLocation(arFaceTargetLocation, pAttacker);
    if (kLocation.IsValidLocation(pAttacker->GetRadius()) &&
        CombatUtilities::CalculateLOS(
            pAttacker,
            pTarget,
            arFaceTargetLocation,
            pTarget->GetLocationOnReference(),
            false,
            30
        )) {
        NiPointer<PathingAvoidNodeArray> spAvoidNodeArray(new PathingAvoidNodeArray);
        float fAvoidCost = fMovementNearTargetAvoidCost.Float();
        float fAvoidRadius = fMovementNearTargetAvoidRadius.Float();
        const NiPoint3& kTargetPos = pTarget->GetLocationOnReference();
        spAvoidNodeArray->Add(PathingAvoidNode(kTargetPos, fAvoidRadius, fAvoidCost));
        PathingAvoidNodeArray* pAvoidNodeArray = spAvoidNodeArray.m_pObject;
        TESWorldSpace* pWorldSpace = kLocation.pWorldSpace;
        TESObjectCELL* pCell = kLocation.GetCell();
        pAttacker->BuildRequest(&pathingRequest, kLocation.GetLocation(), pCell, pWorldSpace, 5.0f, pAvoidNodeArray);
    }
    else {
        float fRadius = 10.0f;
        if (pCombatController->pCombatState->bTargetLineOfSight) {
            fRadius = CombatUtilities::ComputeFaceTargetDistance(pAttacker, pTarget, GetCurrentAttackReach());
        }
        TESWorldSpace* pWorldSpace = pTarget->GetWorldSpace();
        TESObjectCELL* pParentCell = pTarget->GetParentCell();
        pAttacker->BuildRequest(
            &pathingRequest,
            pTarget->GetLocationOnReference(),
            pParentCell,
            pWorldSpace,
            fRadius,
            nullptr
        );
    }
    pAttacker->SetPathfindingGoal(&pathingRequest);
}

void CombatProcedureAttackMelee::HandleAdvance() {
    NiPoint3 currentGoal;
    if (eManeuverState == CombatProcedureAttackMelee::ADVANCE) {
        Actor* pAttacker = GetAttacker();
        Actor* pTarget = GetTarget();
        float fDistance = GetFacingTargetDistance();
        float fReach = GetCurrentAttackReach();
        bool bTargetMovingAway = IsTargetMovingAway();
        BaseProcess* pProcess = pAttacker->pCurrentProcess;
        if ((WithinAttackRange(fDistance, fReach, nullptr) && pCombatController->HasLineOfSight()) ||
            !pCombatController->HasLineOfSight()) {
            float fAttackerReach = GetCurrentAttackReach();
            NiPoint3 targetLoc = CombatUtilities::ComputeFaceTargetLocation(
                pAttacker,
                pTarget,
                pTarget->GetLocationOnReference(),
                fAttackerReach
            );
            bool bGoalWithinRange = false;
            if (pAttacker->IsWaitingOnPath()) {
                bGoalWithinRange = true;
            }
            else if (!pAttacker->IsStoppingMovement() && pAttacker->GetCurrentPathfindingGoal(&currentGoal)) {
                float fActorExtentSum = CombatUtilities::GetAttackerTargetExtentSum(pAttacker, pTarget);
                float fFaceDistance = (currentGoal - pTarget->GetLocationOnReference()).XYLength() - fActorExtentSum;
                bGoalWithinRange = CombatProcedure::WithinAttackRange(fFaceDistance, fReach, 0);
            }
            if (!bGoalWithinRange) {
                MoveToFaceTargetLocation(targetLoc);
            }
        }
        if (pAttacker->IsPathingComplete()) {
            if (!bTargetMovingAway) {
                StartEngage();
            }
        }
        else if (pAttacker->GetBlocked()) {
            pAttacker->SetBlock(false);
        }
        else if (CombatProcedure::WithinAttackRange(fDistance, fReach, 0) && pCombatController->HasLineOfSight()) {
            bool bClose = false;
            if (bTargetMovingAway) {
                float fMinEngageDistance = fCombatMinEngageDistance.Float();
                bClose = fDistance < fMinEngageDistance + fMinEngageDistance;
            }
            if (bClose) {
                StartClose();
            }
            else {
                StartEngage();
            }
        }
        else if (
            eCombatState &&
            CombatProcedure::WithinAttackRange(fDistance, fReach + fReach, 0) &&
            pCombatController->HasLineOfSight() &&
            pAttacker->IsNpc() &&
            pProcess &&
            pProcess->CanAttack()
        ) {
            if (CombatUtilities::IsTargetWithinStrikeAngle(pAttacker, pTarget, nullptr)) {
                uint32_t uiDieRoll = Random() % 100;
                if (uiDieRoll < static_cast<int32_t>(fCombatAdvanceNormalAttackChance.Float() * 100.0)) {
                    ANIM_GROUP_ENUM eAttack = ANIM_GROUP_NONE;
                    const char* pMorphTag = NiControllerSequence::pMorphTag;
                    if (pAttacker->GetAnimation()->FindNextNoteKey(AGS_MOVEMENT, pMorphTag, nullptr) == 108) {
                        eAttack = ANIM_GROUP_ATTACK_LEFT;
                    }
                    else {
                        eAttack = ANIM_GROUP_ATTACK_RIGHT;
                    }
                    StartAttack(eAttack, false);
                }
            }
        }
        else if (eCombatState) {
            CheckForwardPowerAttack(fDistance, fReach);
        }
    }
}

void CombatProcedureAttackMelee::MeleeDecision() {
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    if (eCombatState == CombatProcedureAttackMelee::DONE && pTarget && !pCombatController->IsSwitchingWeapons()) {
        eCurrentAttack = ANIM_GROUP_NONE;
        float fDistance = GetFacingTargetDistance();
        float fReach = GetCurrentAttackReach();
        Animation* pAnim = pAttacker->GetAnimation();
        ANIMATION_ACTION eAnimAction = pAttacker->GetAnimAction();
        ANIMATION_ACTION eTargetAnimAction = pTarget->GetAnimAction();
        bool bMidAttack = TESAnimGroup::IsAttackAction(pAnim->GetGroup(AGS_WEAPON));
        bool bFollowThrough = pAnim->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP;
        bool bRecoiling = eAnimAction == ANIM_ACTION_RECOIL || eAnimAction == ANIM_ACTION_STAGGER;
        bool bTargetAttacking = pTarget->IsAttacking();
        bool bTargetPostAttack = eTargetAnimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
        bool bTargetRecoiling = eTargetAnimAction == ANIM_ACTION_RECOIL || eTargetAnimAction == ANIM_ACTION_STAGGER;
        bool bTargetInCombat = pTarget->IsInCombat();
        bool bTargetBlocking = pTarget->GetBlocked();
        if (CombatProcedure::WithinAttackRange(fDistance, fReach, 0)) {
            bool bCanAttack;
            bool bTargetKnockedOut = pTarget->IsKnockedOut();
            bool bNoWeapon = pCombatController->GetCurrentWeapon() == nullptr;
            int32_t iLuck = pAttacker->GetClampedActorValue(ActorValue::LUCK);
            int32_t iAttackSkill = pCombatController->GetAttackSkill();
            float fAttackScore = CombatFormulas::ChooseAttack(
                pCombatController->GetCurrentCombatStyle(),
                iAttackSkill,
                iLuck,
                bTargetAttacking,
                bNoWeapon,
                bTargetRecoiling,
                bTargetKnockedOut,
                bTargetBlocking
            );
            // The elapsed time is compared with 5.0 (a double) without being stored first.
            if (pCombatController->pCombatState->GetLastAttackTimeStamp().ElapsedTime() > 5.0 && fAttackScore != 0.0) {
                fAttackScore = fAttackScore + 100.0;
            }
            if (!bTargetInCombat && pTarget != pPlayer) {
                fAttackScore = fAttackScore + 100.0;
            }
            float fBlockScore = CombatFormulas::ChooseBlock(
                pCombatController->GetCurrentCombatStyle(),
                50,
                50,
                false,
                bTargetAttacking
            );
            bool bTargetDisabled = pTarget->IsParalyzed() ||
                pTarget->IsKnockedOut() ||
                pTarget->IsUnConscious() ||
                pTarget->IsSleeping() ||
                pTarget->IsRestrained();
            TESObjectWEAP* pTargetWeap = pTarget->GetCurrentWeapon();
            if (bTargetDisabled || !pTarget->IsWeaponDrawn() || (pTargetWeap && pTargetWeap->IsRangedWeapon())) {
                fBlockScore = 0.0f;
            }
            if (bMidAttack && fAttackScore > 0.0) {
                bCanAttack = true;
            }
            else {
                bCanAttack = GetAttacker()->pCurrentProcess->CanAttack();
            }
            if (!bCanAttack || !pCombatController->HasLineOfSight()) {
                fAttackScore = 0.0f;
                if (!pCombatController->HasLineOfSight()) {
                    fBlockScore = 0.0f;
                }
            }
            if ((bMidAttack && !bFollowThrough) ||
                eManeuverState == CombatProcedureAttackMelee::ADVANCE ||
                bRecoiling) {
                fBlockScore = 0.0f;
            }
            if (!pAnim->GroupLoaded(170)) {
                fBlockScore = 0.0f;
            }
            float fRemainingScore = 100.0 - (fAttackScore + fBlockScore);
            float fHoldScore = BSMAX(fRemainingScore, fCombatMaxHoldScore.Float());
            if (bMidAttack && !bFollowThrough) {
                fHoldScore = 0.0f;
            }
            if (eLastCombatState == CombatProcedureAttackMelee::HOLD) {
                fHoldScore = 0.0f;
            }
            int32_t iSum = static_cast<int32_t>(fAttackScore + fBlockScore + fHoldScore);
            uint32_t uiDieRange = iSum <= 0 ? 100 : iSum;
            float fDieRoll = static_cast<float>(Random() % uiDieRange);
            if (!(fAttackScore > static_cast<double>(fDieRoll))) {
                if (!(fBlockScore > 0.0) || !(fDieRoll < fAttackScore + fBlockScore)) {
                    if (eLastCombatState != CombatProcedureAttackMelee::HOLD) {
                        SetCombatState(CombatProcedureAttackMelee::HOLD);
                        float fMaxTime = pCombatController->GetCurrentCombatStyle()->Data.fHoldMaxTime;
                        float fMinTime = pCombatController->GetCurrentCombatStyle()->Data.fHoldMinTime;
                        float fMeleeTargetTime = CombatFormulas::GetRandomBetween(fMinTime, fMaxTime);
                        MeleeTimer.Reset(fMeleeTargetTime);
                    }
                }
                else {
                    pAttacker->SetBlock(true);
                    SetCombatState(CombatProcedureAttackMelee::BLOCK);
                }
            }
            else {
                if (pCombatController->IsBlocking()) {
                    pAttacker->SetBlock(false);
                    if (eCombatState == CombatProcedureAttackMelee::BLOCK) {
                        SetCombatState(CombatProcedureAttackMelee::DONE);
                    }
                }
                bool bDoPowerAttack = false;
                bool bSwitchAttack = false;
                ANIM_GROUP_ENUM eAttack = SelectAttack(
                    bMidAttack,
                    fDistance,
                    fReach,
                    bTargetAttacking,
                    bTargetPostAttack,
                    bTargetRecoiling,
                    bDoPowerAttack,
                    bSwitchAttack
                );
                if (eAttack != ANIM_GROUP_NONE) {
                    if (GetTarget()->Get3D()) {
                        if (bDoPowerAttack) {
                            pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_POWER_ATTACK, false);
                        }
                        else {
                            pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_ATTACK, false);
                        }
                        StartAttack(eAttack, bSwitchAttack);
                    }
                }
            }
        }
    }
}

void CombatProcedureAttackMelee::HandleAttack() {
    if (!eCombatState) {
        Actor* pAttacker = GetAttacker();
        GetTarget();
        if (TESAnimGroup::IsAttackAction(pAttacker->GetAnimation()->GetGroup(AGS_WEAPON))) {
            if (pAttacker->GetAnimation()->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP) {
                SetCombatState(CombatProcedureAttackMelee::DONE);
                MeleeDecision();
            }
            TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
            if (pWeapon) {
                uint16_t sAttackAnim = pWeapon->GetAttackAnim();
                if (TESAnimGroup::IsAttackSpinAction(sAttackAnim)) {
                    pAttacker->QueueAttack(pWeapon->GetAttackAnim());
                }
            }
        }
        else {
            SetCombatState(CombatProcedureAttackMelee::DONE);
        }
    }
}

void CombatProcedureAttackMelee::HandleBlock() {
    if (eCombatState == 1) {
        Actor* pAttacker = GetAttacker();
        Actor* pTarget = GetTarget();
        if (!pAttacker->GetBlocked()) {
            SetCombatState(DONE);
        }
        ItemChange* pWeaponItem = pAttacker->pCurrentProcess->GetCurrentWeapon();
        if (pWeaponItem) {
            bool bLeftArmStopped = false;
            if (GetAttacker()->GetAnimation() && GetAttacker()->GetAnimation()->GetCurrentSequence(AGS_LEFT_ARM)) {
                BSAnimGroupSequence* pSequence = GetAttacker()->GetAnimation()->GetCurrentSequence(AGS_LEFT_ARM);
                bLeftArmStopped = pSequence->GetState() != NiControllerSequence::kAnimating;
            }
            float fItemHealth = pWeaponItem->GetItemHealth(false);
            if (!(fItemHealth <= 0.0) || bLeftArmStopped) {
                bool bTargetRecoiling = pTarget && pTarget->GetAnimAction() == ANIM_ACTION_RECOIL;
                if (!bTargetRecoiling || bLeftArmStopped) {
                    if (!bLeftArmStopped) {
                        CheckStopBlocking();
                    }
                }
                else {
                    pAttacker->SetBlock(false);
                    SetCombatState(DONE);
                    AttackDecision();
                }
            }
            else {
                pAttacker->SetBlock(false);
                SetCombatState(DONE);
            }
        }
    }
}

void CombatProcedureAttackMelee::UpdateMovementRestrictions() {
    Actor* pAttacker = GetAttacker();
    if (!pAttacker->IsPathing()) {
        MovementRestrictions.SetReset(true);
    }
}

bool CombatProcedureAttackMelee::IsTargetMovingAway() {
    NiPoint3 kPredictedPos;
    NiPoint3 kDelta;
    NiPoint3 kVelocity = NiPoint3::ZERO;
    if (!GetTarget()) {
        return false;
    }
    if (!GetTarget()->IsMoving()) {
        return false;
    }
    GetTarget()->GetLinearVelocity(kVelocity);
    kVelocity.z = 0.0;
    kPredictedPos = GetTarget()->GetLocationOnReference() + kVelocity;
    float fFacingDistance = GetFacingTargetDistance();
    kDelta = GetAttacker()->GetLocationOnReference() - kPredictedPos;
    float fDistanceSqr = kDelta.SqrLength();
    return fDistanceSqr >= fFacingDistance * fFacingDistance;
}

void CombatProcedureAttackMelee::SetCombatState(CombatProcedureAttackMelee::CombatState aeCombatState) {
    eLastCombatState = eCombatState;
    eCombatState = static_cast<CombatState>(aeCombatState);
}

bool CombatProcedureAttackMelee::WithinAttackRange(
    float afDistance,
    float afReach,
    ANIM_GROUP_ENUM* aeAttackWithinReach
) {
    CombatProcedure::GetAttacker();
    pCombatController->GetCurrentCombatStyle()->QPowerAttackChance();
    return afDistance >= afReach + 0.0f;
}

float CombatProcedureAttackMelee::GetCurrentAttackReach() {
    Actor* pAttacker = CombatProcedure::GetAttacker();
    if (!(fCurrentAttackReach < 0.0)) {
        return fCurrentAttackReach;
    }
    fCurrentAttackReach = CombatUtilities::GetWeaponReach(pAttacker, pCombatController->GetCurrentWeapon());
    return fCurrentAttackReach;
}

void CombatProcedureAttackMelee::StartAdvance() {
    SetManeuverState(CombatProcedureAttackMelee::ADVANCE);
    CombatProcedure::SetMoveMode(3);
    ManeuverTimer.Reset(3.0f);
    HandleAdvance();
}

CombatProcedureAttackMelee::CombatState CombatProcedureAttackMelee::GetCombatState() {
    return eCombatState;
}

void CombatProcedureAttackMelee::StartAttack(ANIM_GROUP_ENUM aeAttack, bool abSwitchAttack) {
    if (aeAttack != ANIM_GROUP_NONE && GetCombatState()) {
        if (CombatProcedure::GetAttacker()->QueueAttack(aeAttack)) {
            SetCombatState(CombatProcedureAttackMelee::ATTACK);
            eCurrentAttack = aeAttack;
        }
    }
}

void CombatProcedureAttackMelee::HandleHold() {
    if (CombatProcedureAttackMelee::GetCombatState() == 2 && MeleeTimer.Check()) {
        SetCombatState(CombatProcedureAttackMelee::DONE);
    }
}

float CombatProcedureAttackMelee::GetFacingTargetDistance() {
    if (!(fFacingTargetDistance < 0.0)) {
        return fFacingTargetDistance;
    }
    Actor* pTarget = CombatProcedure::GetTarget();
    Actor* pAttacker = CombatProcedure::GetAttacker();
    fFacingTargetDistance = CombatUtilities::GetDistanceBetweenFacingActors(pAttacker, pTarget, false);
    return fFacingTargetDistance;
}

ANIM_GROUP_ENUM CombatProcedureAttackMelee::SelectAttack(
    bool abMidAttack,
    float afDistance,
    float afReach,
    bool abTargetAttacking,
    bool abTargetPostAttack,
    bool abTargetRecoiling,
    bool& abDoPowerAttack,
    bool& abSwitchAttack
) {
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    abSwitchAttack = false;
    ANIM_GROUP_ENUM eAttack = ANIM_GROUP_NONE;
    TESObjectWEAP* pWeapon = pCombatController->GetCurrentWeapon();
    if (!pTarget) {
        return ANIM_GROUP_NONE;
    }
    if (pAttacker->IsNpc() && pAttacker->IsSwimming()) {
        abDoPowerAttack = false;
    }
    else {
        bool bTargetKnockedOut = pTarget->IsKnockedOut();
        float fFatiguePercent = pAttacker->GetFatiguePercentage();
        abDoPowerAttack = CombatFormulas::ChoosePowerAttack(
            pCombatController->GetCurrentCombatStyle(),
            fFatiguePercent,
            abTargetRecoiling,
            bTargetKnockedOut,
            false
        );
    }
    if (eManeuverState == CombatProcedureAttackMelee::ADVANCE) {
        afReach = afReach + fCombatStepAdvanceDistance.Float();
        if (Random() % 100 < 2) {
            afReach = afReach + 100.0;
        }
    }
    bool bWithinNormalReach = CombatProcedure::WithinAttackRange(afDistance, afReach, 0);
    if (abMidAttack) {
        if (pAttacker->GetAnimation()->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP) {
            if (abDoPowerAttack) {
                eAttack = static_cast<ANIM_GROUP_ENUM>(
                    SelectPowerAttack(afDistance, afReach, abTargetAttacking, abTargetPostAttack, false)
                );
            }
            if (eAttack == ANIM_GROUP_NONE && bWithinNormalReach) {
                switch (GetAttacker()->GetAnimation()->FindSwitchKey(nullptr)) {
                    case 51: {
                        eAttack = ANIM_GROUP_ATTACK_3;
                        break;
                    }
                    case 52: {
                        eAttack = ANIM_GROUP_ATTACK_4;
                        break;
                    }
                    case 53: {
                        eAttack = ANIM_GROUP_ATTACK_5;
                        break;
                    }
                    case 54: {
                        eAttack = ANIM_GROUP_ATTACK_6;
                        break;
                    }
                    case 55: {
                        eAttack = ANIM_GROUP_ATTACK_7;
                        break;
                    }
                    case 56: {
                        eAttack = ANIM_GROUP_ATTACK_8;
                        break;
                    }
                    case 108: {
                        eAttack = ANIM_GROUP_ATTACK_LEFT;
                        break;
                    }
                    default: {
                        if (!pWeapon || pWeapon->GetAttackAnim() == ANIM_GROUP_NONE) {
                            eAttack = ANIM_GROUP_ATTACK_RIGHT;
                        }
                        else {
                            eAttack = pWeapon->GetAttackAnim();
                        }
                        break;
                    }
                }
                abSwitchAttack = true;
            }
        }
    }
    else {
        if (abDoPowerAttack) {
            eAttack = static_cast<ANIM_GROUP_ENUM>(
                SelectPowerAttack(afDistance, afReach, abTargetAttacking, abTargetPostAttack, false)
            );
        }
        if (eAttack == ANIM_GROUP_NONE && bWithinNormalReach) {
            eAttack = pWeapon ? pWeapon->GetAttackAnim() : ANIM_GROUP_NONE;
            if (eAttack == ANIM_GROUP_NONE) {
                const char* pMorphTag = NiControllerSequence::pMorphTag;
                if (pAttacker->GetAnimation()->FindNextNoteKey(AGS_MOVEMENT, pMorphTag, nullptr) == 108) {
                    return ANIM_GROUP_ATTACK_LEFT;
                }

                return ANIM_GROUP_ATTACK_RIGHT;
            }
        }
    }
    return eAttack;
}

void CombatProcedureAttackMelee::CheckStopBlocking() {
    if (!GetAttacker()->GetAnimation() ||
        !GetAttacker()->GetAnimation()->GetCurrentSequence(AGS_LEFT_ARM) ||
        GetAttacker()->GetAnimation()->GetCurrentSequence(AGS_LEFT_ARM)->GetState() ==
            NiControllerSequence::kAnimating) {
        Actor* pAttacker = GetAttacker();
        Actor* pTarget = GetTarget();
        if (pCombatController->IsBlocking() || eCombatState == 1) {
            if (pTarget) {
                bool bTargetAttacking = pTarget->IsAttacking();
                bool bTargetPostAttack = pTarget->GetAnimAction() == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
                char cTargetBlocking = 0;
                if (pTarget->GetCombatController()) {
                    cTargetBlocking = pTarget->GetCombatController()->IsBlocking();
                }
                float fDistance = GetFacingTargetDistance();
                float fReach = GetCurrentAttackReach();
                bool bTargetDisabled = pTarget->IsParalyzed() ||
                    pTarget->IsKnockedOut() ||
                    pTarget->IsUnConscious() ||
                    pTarget->IsSleeping() ||
                    pTarget->IsRestrained();
                if (fReach < static_cast<double>(fDistance) ||
                    bTargetDisabled ||
                    bTargetPostAttack ||
                    cTargetBlocking ||
                    (Random() % 100 < 5 && !bTargetAttacking) ||
                    !pAttacker->GetBlocked()) {
                    if (pAttacker->GetBlocked()) {
                        pAttacker->SetBlock(false);
                    }
                    pCombatController->SetIsBlocking(0);
                    if (eCombatState == 1) {
                        SetCombatState(DONE);
                    }
                }
            }
            else {
                if (pAttacker->GetBlocked()) {
                    pAttacker->SetBlock(false);
                }
                pCombatController->SetIsBlocking(0);
                if (eCombatState == 1) {
                    SetCombatState(DONE);
                }
            }
        }
    }
}

void CombatProcedureAttackMelee::HandleEngage() {
    if (!eManeuverState) {
        PlayerCharacter* pAttacker = static_cast<PlayerCharacter*>(GetAttacker());
        Actor* pTarget = GetTarget();
        float fDistance = GetFacingTargetDistance();
        float fReach = GetCurrentAttackReach();
        if (!CombatProcedure::WithinAttackRange(fDistance, fReach, 0) ||
            pCombatController->HasLineOfSight() ||
            !CombatUtilities::IsTargetWithinStrikeAngle(pAttacker, pTarget, nullptr)) {
            if (pCombatController->IsBlocking() || eCombatState == 1) {
                CheckStopBlocking();
                return;
            }
            if (!WithinAttackRange(fDistance, fReach, nullptr)) {
                ManeuverDecision();
                return;
            }
            if (!(fCombatMinEngageDistance.Float() < fDistance - fReach) && !IsTargetMovingAway()) {
                StartClose();
                return;
            }
        }
        StartAdvance();
    }
}

void CombatProcedureAttackMelee::HandleDodge() {
    if (eManeuverState == CombatProcedureAttackMelee::DODGE) {
        ANIMATION_ACTION eTargetAnimAction;
        if (ManeuverTimer.Check()) {
            StartEngage();
            UpdateMovementRestrictions();
            return;
        }
        Actor* pAttacker = GetAttacker();
        Actor* pTarget = GetTarget();
        uint32_t uiMoveMode = pAttacker->GetMoveMode();
        if (pAttacker->IsMoving()) {
            CheckUpdateMovementRestrictions();
            eTargetAnimAction = pTarget ? pTarget->GetAnimAction() : ANIM_ACTION_NONE;
            if (eTargetAnimAction == ANIM_ACTION_ATTACK && eCombatState == CombatProcedureAttackMelee::HOLD) {
                SetCombatState(CombatProcedureAttackMelee::DONE);
                AttackDecision();
            }
            if (MovementRestrictions.CanMove(uiMoveMode)) {
                float fDistance = GetFacingTargetDistance();
                float fReach = GetCurrentAttackReach();
                if ((uiMoveMode & ActorMover::MF_LEFT) != 0 || (uiMoveMode & ActorMover::MF_RIGHT) != 0) {
                    if (!(fReach < static_cast<double>(fDistance))) {
                        if (fCombatMinEngageDistance.Float() > static_cast<double>(fDistance) &&
                            (uiMoveMode & ActorMover::MF_FORWARD) != 0) {
                            pAttacker->ForceMoveMode(uiMoveMode & ~ActorMover::MF_FORWARD);
                        }
                    }
                    else if (WithinAttackRange(fDistance, fReach, nullptr)) {
                        if (DodgeTimer.Check()) {
                            StopMoving();
                            StartAdvance();
                            return;
                        }
                    }
                    else {
                        pAttacker->ForceMoveMode(uiMoveMode | ActorMover::MF_FORWARD);
                    }
                }
                else if (
                    (uiMoveMode & ActorMover::MF_FORWARD) != 0 &&
                    fCombatMinEngageDistance.Float() >= static_cast<double>(fDistance) &&
                    DodgeTimer.Check()
                ) {
                    StopMoving();
                    StartEngage();
                    return;
                }
                if (eTargetAnimAction == ANIM_ACTION_ATTACK) {
                    float fDieRoll = static_cast<float>(Random() % 100);
                    if ((uiMoveMode & ActorMover::MF_WALKING) != 0 &&
                        !pCombatController->IsBlocking() &&
                        fAIDodgeWalkChance.Float() < static_cast<double>(fDieRoll)) {
                        pAttacker->ForceMoveMode(
                            (uiMoveMode & ~(ActorMover::MF_WALKING | ActorMover::MF_RUNNING)) | ActorMover::MF_RUNNING
                        );
                    }
                }
                else if ((uiMoveMode & ActorMover::MF_RUNNING) != 0) {
                    pAttacker->ForceMoveMode(
                        (uiMoveMode & ~(ActorMover::MF_WALKING | ActorMover::MF_RUNNING)) | ActorMover::MF_WALKING
                    );
                }
                pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_ATTACK, false);
                return;
            }
            StopMoving();
            if (CombatController::GetShowVerboseMessages()) {
                Interface::PrintLine("%.20s has run out of room to dodge.", pAttacker->GetFullName());
            }
        }
        StartEngage();
    }
}

void CombatProcedureAttackMelee::HandleIdle() {
    if (eManeuverState == CombatProcedureAttackMelee::IDLE) {
        Actor* pTarget = GetTarget();
        bool bEngage = ManeuverTimer.Check();
        // The elapsed time is compared with 0.5 as a double without being stored.
        if (!bEngage && ManeuverTimer.ElapsedTime() > 0.5) {
            float fDistance = GetFacingTargetDistance();
            float fReach = GetCurrentAttackReach();
            bEngage = WithinAttackRange(fDistance, fReach, nullptr);
        }
        if (bEngage) {
            StartEngage();
        }
        else if ((pTarget && pTarget->IsAttacking()) || bRangedAllyWantsToShoot) {
            if (eCombatState == CombatProcedureAttackMelee::HOLD) {
                SetCombatState(CombatProcedureAttackMelee::DONE);
                AttackDecision();
            }
            ManeuverDecision();
        }
    }
}

void CombatProcedureAttackMelee::StartClose() {
    NiPoint3 faceTargetLoc;
    SetManeuverState(CLOSE);
    Actor* pAttacker = GetAttacker();
    Actor* pTarget = GetTarget();
    SetMoveMode(1);
    float fAttackerReach = GetCurrentAttackReach();
    faceTargetLoc = CombatUtilities::ComputeFaceTargetLocation(
        pAttacker,
        pTarget,
        pTarget->GetLocationOnReference(),
        fAttackerReach
    );
    MoveToFaceTargetLocation(faceTargetLoc);
    HandleClose();
}

void CombatProcedureAttackMelee::CheckForwardPowerAttack(float afDistance, float afReach) {
    if (eCombatState == CombatProcedureAttackMelee::DONE) {
        Actor* pAttacker = GetAttacker();
        Actor* pTarget = GetTarget();
        if (pTarget && !(fAttackPickFailedTimestamp.ElapsedTime() < 1.0) && pCombatController->bHasForwardAttacks) {
            if (pAttacker->pCurrentProcess->CanAttack()) {
                bool bMidAttack;
                if (pAttacker->GetAnimation()) {
                    bMidAttack = TESAnimGroup::IsPowerAttackAction(pAttacker->GetAnimation()->GetGroup(AGS_WEAPON));
                }
                else {
                    bMidAttack = false;
                }
                if (!bMidAttack &&
                    (!pAttacker->IsNpc() || !pAttacker->IsSwimming()) &&
                    pCombatController->HasLineOfSight() &&
                    CombatUtilities::IsTargetWithinStrikeAngle(pAttacker, pTarget, nullptr)) {
                    ANIMATION_ACTION eTargetAnimAction = pTarget->GetAnimAction();
                    bool bTargetAttacking = pTarget->IsAttacking();
                    bool bTargetPostAttack = eTargetAnimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
                    bool bTargetRecoiling = eTargetAnimAction == ANIM_ACTION_RECOIL ||
                        eTargetAnimAction == ANIM_ACTION_STAGGER;
                    bool bTargetBlocking = pTarget->GetBlocked();
                    ANIM_GROUP_ENUM eAttack = static_cast<ANIM_GROUP_ENUM>(
                        SelectPowerAttack(afDistance, afReach, bTargetAttacking, bTargetPostAttack, true)
                    );
                    if (eAttack != ANIM_GROUP_NONE && CanDoPowerAttack(eAttack)) {
                        int32_t iDieRoll = Random() % 100;
                        if (iDieRoll < pCombatController->GetCurrentCombatStyle()->GetRushingAttackPercentChance()) {
                            CombatTimer fattackScore;
                            bool bTargetKnockedOut = pTarget->IsKnockedOut();
                            bool bNoWeapon = pCombatController->GetCurrentWeapon() == nullptr;
                            int32_t iLuck = pAttacker->GetClampedActorValue(ActorValue::LUCK);
                            int32_t iAttackSkill = pCombatController->GetAttackSkill();
                            fattackScore.fTargetTime = CombatFormulas::ChooseAttack(
                                pCombatController->GetCurrentCombatStyle(),
                                iAttackSkill,
                                iLuck,
                                bTargetAttacking,
                                bNoWeapon,
                                bTargetRecoiling,
                                bTargetKnockedOut,
                                bTargetBlocking
                            );
                            float fBlockScore = CombatFormulas::ChooseBlock(
                                pCombatController->GetCurrentCombatStyle(),
                                50,
                                50,
                                false,
                                bTargetAttacking
                            );
                            float fRemainingScore = 100.0 - (fattackScore.fTargetTime + fBlockScore);
                            float fHoldScore = BSMAX(fRemainingScore, fCombatMaxHoldScore.Float());
                            if (fBlockScore < static_cast<double>(fattackScore.fTargetTime) &&
                                fHoldScore < static_cast<double>(fattackScore.fTargetTime)) {
                                const NiPoint3& kTargetPos = pTarget->GetLocationOnReference();
                                const NiPoint3& kAttackerPos = pAttacker->GetLocationOnReference();
                                if (CombatUtilities::CalculateLOS(
                                        pAttacker,
                                        pTarget,
                                        kAttackerPos,
                                        kTargetPos,
                                        false,
                                        30
                                    )) {
                                    StartAttack(eAttack, false);
                                    SetManeuverState(CombatProcedureAttackMelee::ENGAGE);
                                }
                                else {
                                    fattackScore.SetStartTime(CombatTimer::GetTimer());
                                    fAttackPickFailedTimestamp.fTimeStamp = fattackScore.fStartTime;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

bool CombatProcedureAttackMelee::CanDoPowerAttack(ANIM_GROUP_ENUM aePowerAttack) {
    GetAttacker();
    bool bCanDo = false;
    if (static_cast<double>(pCombatController->GetCurrentCombatStyle()->QPowerAttackChance()) > 0.0) {
        bCanDo = true;
        if (pCombatController->GetCurrentCombatStyle()->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE)) {
            switch (aePowerAttack) {
                case ANIM_GROUP_ATTACK_NORMAL_POWER: {
                    bCanDo = pCombatController->GetCurrentCombatStyle()->GetPowerAttackN() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_FORWARD_POWER: {
                    bCanDo = pCombatController->GetCurrentCombatStyle()->GetPowerAttackF() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_BACK_POWER: {
                    bCanDo = pCombatController->GetCurrentCombatStyle()->GetPowerAttackB() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_LEFT_POWER: {
                    bCanDo = pCombatController->GetCurrentCombatStyle()->GetPowerAttackL() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_RIGHT_POWER: {
                    // The byte is read as signed.
                    bCanDo = static_cast<int8_t>(
                                 pCombatController->GetCurrentCombatStyle()->Data.cPowerAttackRightChance
                             ) > 0;
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    return bCanDo;
}

bool CombatProcedureAttackMelee::GetMovementRestrictionFlag(uint16_t asMoveMode) {
    return MovementRestrictions.CanMove(asMoveMode);
}

float CombatProcedureAttackMelee::GetMovingTargetVelocity() {
    if (GetTarget() && GetTarget()->IsMoving()) {
        NiPoint3 kVelocity = NiPoint3::ZERO;
        GetTarget()->GetLinearVelocity(kVelocity);
        return kVelocity.XYLength();
    }

    return 0.0;
}
