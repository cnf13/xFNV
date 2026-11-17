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
    bHasMoveRightAnim(false), bHasMoveForwardAnim(false), bHasMoveBackwardAnim(false), Location(NiPoint3_ZERO) {}

bool CombatMovementRestrictions::CanMove(uint16_t asMoveMode) {
    return (((uint16_t)(((int16_t)asMoveMode) & sRestrictedMoves)) == 0);
}

void CombatMovementRestrictions::SetRestrictedMove(int16_t a2, char a3) {
    uint16_t sRestrictedMoves;
    if (a3) {
        this->sRestrictedMoves &= (~a2);
    }
    else {
        this->sRestrictedMoves |= a2;
    }
    sRestrictedMoves = this->sRestrictedMoves;
}

void CombatMovementRestrictions::SetReset(bool abValue) {
    bReset = true;
}

char CombatMovementRestrictions::IsLocationSafe(TESObjectREFR* a1, NiPoint3* arTarget) {
    PathingLocation apLocation(a1);
    char v4 = Pathing::CheckLineOfSight(apLocation, *arTarget, nullptr, nullptr);
    return v4;
}

int32_t CombatMovementRestrictions::CheckMovementRestrictions(Actor* apActor, uint32_t a3) {
    NiMatrix3 ecx0;
    if (!bInitialized) {
        BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
        bHasMoveLeftAnim = CurrentAIProcess->HasMovementAnim(ANIM_GROUP_MOVE_LEFT);
        BaseProcess* v19 = apActor->pCurrentProcess;
        bHasMoveRightAnim = v19->HasMovementAnim(ANIM_GROUP_MOVE_RIGHT);
        BaseProcess* v18 = apActor->pCurrentProcess;
        bHasMoveForwardAnim = v18->HasMovementAnim(ANIM_GROUP_MOVE_FORWARD);
        BaseProcess* v17 = apActor->pCurrentProcess;
        bHasMoveBackwardAnim = v17->HasMovementAnim(ANIM_GROUP_MOVE_BACK);
        bInitialized = true;
    }
    if (!bReset && apActor->IsPathing()) {
        return 4;
    }
    if (!bReset && !a3) {
        const NiPoint3* v4 = &apActor->GetLocationOnReference();
        NiPoint3 result = Location - *v4;
        double v16 = result.Length();
        if (fAIUpdateMovementRestrictionsDistance.Float() > v16) {
            return 4;
        }
    }
    Location = apActor->GetLocationOnReference();
    float a2 = apActor->GetHeading(false);
    ecx0.MakeZRotation(a2);
    float Width = apActor->GetWidth();
    if (Width == 0.0) {
        Width = 32.0f;
    }
    float Length = apActor->GetLength();
    if (Length == 0.0) {
        Length = Width;
    }
    float v37 = Width / Length;
    if (v37 > 0.5 && v37 < 2.0) {
        Width = BSMAX(Width, Length);
        Length = Width;
    }
    Width = BSMIN(Width, 64.0f);
    Length = BSMIN(Length, 64.0f);
    uint32_t MoveMode = apActor->GetMoveMode();
    switch (a3) {
        case 0u: {
            NiPoint3 dst(0.0f, -Length, 0.0f);
            if ((MoveMode & 2) != 0) {
                dst.y = dst.y - Length;
            }
            dst = ecx0 * dst + Location;
            if (bHasMoveBackwardAnim) {
                char v15 = IsLocationSafe(apActor, &dst);
                SetRestrictedMove(2, v15);
            }
            else {
                SetRestrictedMove(2, 0);
            }
            break;
        }
        case 1u: {
            NiPoint3 arTarget(0.0f, Length, 0.0f);
            if ((MoveMode & 1) != 0) {
                arTarget.y = arTarget.y + Length;
            }
            arTarget = ecx0 * arTarget + Location;
            if (bHasMoveForwardAnim) {
                char v14 = IsLocationSafe(apActor, &arTarget);
                SetRestrictedMove(1, v14);
            }
            else {
                SetRestrictedMove(1, 0);
            }
            break;
        }
        case 2u: {
            NiPoint3 pt(-Width, 0.0f, 0.0f);
            if ((MoveMode & 4) != 0) {
                pt.x = pt.x - Width;
            }
            pt = ecx0 * pt + Location;
            if (bHasMoveLeftAnim) {
                char v13 = IsLocationSafe(apActor, &pt);
                SetRestrictedMove(4, v13);
            }
            else {
                SetRestrictedMove(4, 0);
            }
            break;
        }
        case 3u: {
            NiPoint3 v30(Width, 0.0f, 0.0f);
            if ((MoveMode & 8) != 0) {
                v30.x = v30.x + Width;
            }
            v30 = ecx0 * v30 + Location;
            if (bHasMoveRightAnim) {
                char v12 = IsLocationSafe(apActor, &v30);
                SetRestrictedMove(8, v12);
            }
            else {
                SetRestrictedMove(8, 0);
            }
            break;
        }
        default: {
            return 1;
        }
    }
    return 1;
}

void CombatMovementRestrictions::Update(CombatController* apCombatController) {
    if (UpdateTimer.Check() || cCurrentUpdate) {
        cCurrentUpdate +=
            CheckMovementRestrictions(static_cast<PlayerCharacter*>(apCombatController->pAttacker), cCurrentUpdate);
    }
    if (cCurrentUpdate >= 4) {
        bReset = false;
        cCurrentUpdate = 0;
        float* v2 = &fMeleeMovementRestrictionsUpdateTime.Float();
        UpdateTimer.Reset(*v2);
    }
}

CombatProcedureAttackMelee::CombatProcedureAttackMelee()
  : eManeuverState(ENGAGE), eCombatState(DONE), eLastCombatState(DONE),
    eCurrentAttack(static_cast<ANIM_GROUP_ENUM>(255)), bRangedAllyWantsToShoot(false), iAdvanceMoveMode(256),
    fCurrentAttackReach(-1.0f), fFacingTargetDistance(-1.0f) {
    // The attack-pick-failed time stamp is a timer's start time set to -FLT_MAX.
    CombatTimer kTimer;
    kTimer.SetStartTime(-0x1.fffffe0000000p+127f);
    fAttackPickFailedTimestamp.fTimeStamp = kTimer.fStartTime;
}

CombatProcedureAttackMelee::~CombatProcedureAttackMelee() {}

void CombatProcedureAttackMelee::Update() {
    BGSWorldLocation apLocation;
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    fFacingTargetDistance = -1.0f;
    CombatAI_Melee.Start();
    MovementRestrictions.Update(pCombatController);
    AttackDecision();
    if (eManeuverState == 3 || eManeuverState == 4) {
        if (!pattacker->DidPathingFail() || pattacker->IsPathValid()) {
            if (pattacker->IsPathValid()) {
                TargetUnreachableTimer.Clear();
            }
        }
        else {
            if (TargetUnreachableTimer.IsSet()) {
                if (TargetUnreachableTimer.Check()) {
                    pCombatController->pCombatState->SetFlag(2);
                    BGSWorldLocation* WorldLocation = ptarget->GetWorldLocation(&apLocation);
                    pCombatController->pCombatState->AddUnreachableLocation(*WorldLocation);
                    SetFailed("Could not reach target");
                }
            }
            else {
                TargetUnreachableTimer.Reset(2.0f);
            }
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
    bool buseLookAtTarget = false;
    if (eCombatState == 3) {
        if (eManeuverState == 3) {
            buseLookAtTarget = true;
        }
        else {
            float fdistance = GetFacingTargetDistance();
            float freach = GetCurrentAttackReach();
            double v3 = fdistance - freach;
            float* v2 = &fCombatDistance.Float();
            if (pattacker->GetScale() * *v2 * 2.25 > v3) {
                buseLookAtTarget = true;
            }
        }
    }
    else {
        buseLookAtTarget = true;
    }
    if (buseLookAtTarget) {
        NiPoint3 targetLoc = CombatUtilities::GetTargetFiringLocation(ptarget, ACTOR_SEGMENT_INVIEW_TOP);
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
    Actor* attacker = CombatProcedure::GetAttacker();
    CombatProcedure::ClearLookAtTarget();
    attacker->StopMoving();
    attacker->SetBlock(false);
}

// Each timer's target time is read, then its elapsed time; both are passed on as doubles.
void CombatProcedureAttackMelee::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
    char Dst[108];
    Actor* pattacker = GetAttacker();
    int16_t smoveMode = pattacker->GetMoveMode();
    std::memset(Dst, 0, 100);
    CombatProcedureAttackMelee::Maneuver eManeuverState = this->eManeuverState;
    if (eManeuverState == CombatProcedureAttackMelee::IDLE) {
        double dTargetTime = ManeuverTimer.GetTargetTime();
        double dElapsedTime = ManeuverTimer.ElapsedTime();
        BSsprintf(Dst, 100, "%.2f/%.2f", dElapsedTime, dTargetTime);
    }
    else {
        if (eManeuverState == CombatProcedureAttackMelee::DODGE) {
            const char* v26;
            if ((smoveMode & 1) != 0) {
                v26 = " FORWARD ";
            }
            else {
                const char* v25;
                if ((smoveMode & 2) != 0) {
                    v25 = " BACKWARD ";
                }
                else {
                    const char* v24;
                    if ((smoveMode & 4) != 0) {
                        v24 = " LEFT ";
                    }
                    else {
                        const char* v23;
                        if ((smoveMode & 8) != 0) {
                            v23 = " RIGHT ";
                        }
                        else {
                            v23 = "";
                        }
                        v24 = v23;
                    }
                    v25 = v24;
                }
                v26 = v25;
            }
            double dTargetTime = ManeuverTimer.GetTargetTime();
            double dElapsedTime = ManeuverTimer.ElapsedTime();
            BSsprintf(Dst, 100, "%s %.2f/%.2f", v26, dElapsedTime, dTargetTime);
        }
        else {
            Dst[0] = 0;
        }
    }
    apDebugText->PrintLine(aeLocation, aeJustify, "Maneuver: %s %s", pManeuverNames[this->eManeuverState], Dst);
    CombatProcedureAttackMelee::CombatState eCombatState = this->eCombatState;
    if (eCombatState) {
        if (eCombatState == CombatProcedureAttackMelee::HOLD) {
            double dTargetTime = MeleeTimer.GetTargetTime();
            double dElapsedTime = MeleeTimer.ElapsedTime();
            BSsprintf(Dst, 100, "%.2f/%.2f", dElapsedTime, dTargetTime);
        }
        else {
            Dst[0] = 0;
        }
    }
    else {
        if (eCurrentAttack == ANIM_GROUP_ATTACK_LEFT) {
            BSstrcpy(Dst, 100, "LEFT");
        }
        else {
            const char* v21;
            if (eCurrentAttack == ANIM_GROUP_ATTACK_RIGHT) {
                v21 = "RIGHT";
            }
            else {
                const char* v20;
                if (eCurrentAttack == ANIM_GROUP_ATTACK_NORMAL_POWER) {
                    v20 = "NORMAL POWER";
                }
                else {
                    const char* v19;
                    if (eCurrentAttack == ANIM_GROUP_ATTACK_FORWARD_POWER) {
                        v19 = "FORWARD POWER";
                    }
                    else {
                        const char* v18;
                        if (eCurrentAttack == ANIM_GROUP_ATTACK_BACK_POWER) {
                            v18 = "BACK POWER";
                        }
                        else {
                            const char* v17;
                            if (eCurrentAttack == ANIM_GROUP_ATTACK_LEFT_POWER) {
                                v17 = "LEFT POWER";
                            }
                            else {
                                const char* v16;
                                if (eCurrentAttack == ANIM_GROUP_ATTACK_RIGHT_POWER) {
                                    v16 = "RIGHT POWER";
                                }
                                else {
                                    v16 = "";
                                }
                                v17 = v16;
                            }
                            v18 = v17;
                        }
                        v19 = v18;
                    }
                    v20 = v19;
                }
                v21 = v20;
            }
            BSstrcpy(Dst, 100, v21);
        }
    }
    if (TargetUnreachableTimer.IsSet()) {
        double dTargetTime = TargetUnreachableTimer.GetTargetTime();
        double dElapsedTime = TargetUnreachableTimer.ElapsedTime();
        apDebugText->PrintLine(aeLocation, aeJustify, "Target Unreachable Timer: %.2f/%.2f", dElapsedTime, dTargetTime);
    }
    apDebugText->PrintLine(aeLocation, aeJustify, "Melee: %s %s", pCombatStateNames[this->eCombatState], Dst);
    if (MovementRestrictions.sRestrictedMoves) {
        if (!pattacker->IsPathing()) {
            const char* v12;
            const char* v13;
            const char* v14;
            const char* v15;
            if (MovementRestrictions.CanMove(2)) {
                v15 = "Y";
            }
            else {
                v15 = "N";
            }
            if (MovementRestrictions.CanMove(1)) {
                v14 = "Y";
            }
            else {
                v14 = "N";
            }
            if (MovementRestrictions.CanMove(8)) {
                v13 = "Y";
            }
            else {
                v13 = "N";
            }
            if (MovementRestrictions.CanMove(4)) {
                v12 = "Y";
            }
            else {
                v12 = "N";
            }
            apDebugText->PrintLine(
                aeLocation, aeJustify, "Movement Restrictions: L:%s R:%s F:%s B:%s", v12, v13, v14, v15
            );
        }
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
    apSaveGameBuffer->SaveDataEndian(&eManeuverState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&eCombatState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&eLastCombatState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&eCurrentAttack, 4u, 0);
    MeleeTimer.SaveGame(apSaveGameBuffer);
    ManeuverTimer.SaveGame(apSaveGameBuffer);
    DodgeTimer.SaveGame(apSaveGameBuffer);
    AdvanceTimer.SaveGame(apSaveGameBuffer);
    CloseTimer.SaveGame(apSaveGameBuffer);
    TargetUnreachableTimer.SaveGame(apSaveGameBuffer);
    fAttackPickFailedTimestamp.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bRangedAllyWantsToShoot, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iAdvanceMoveMode, 4u, 0);
}

void CombatProcedureAttackMelee::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&eManeuverState, 4u);
    apLoadGameBuffer->LoadData(&eCombatState, 4u);
    apLoadGameBuffer->LoadData(&eLastCombatState, 4u);
    apLoadGameBuffer->LoadData(&eCurrentAttack, 4u);
    MeleeTimer.LoadGame(apLoadGameBuffer);
    ManeuverTimer.LoadGame(apLoadGameBuffer);
    DodgeTimer.LoadGame(apLoadGameBuffer);
    AdvanceTimer.LoadGame(apLoadGameBuffer);
    CloseTimer.LoadGame(apLoadGameBuffer);
    TargetUnreachableTimer.LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        fAttackPickFailedTimestamp.LoadGame(apLoadGameBuffer);
    }
    apLoadGameBuffer->LoadData(&bRangedAllyWantsToShoot, 1u);
    apLoadGameBuffer->LoadData(&iAdvanceMoveMode, 4u);
}

void CombatProcedureAttackMelee::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

void CombatProcedureAttackMelee::CheckUpdateMovementRestrictions() {
    if (!Main::GetSceneIsComplex()) {
        NiPoint3 result;
        Actor* Attacker = GetAttacker();
        const NiPoint3* v3 = &Attacker->GetLocationOnReference();
        NiPoint3* FollowUpInfoList = &MovementRestrictions.Location;
        result = *FollowUpInfoList - *v3;
        double v4 = result.SqrLength();
        float* v2 = &fAIUpdateMovementRestrictionsDistance.Float();
        if (*v2 * fAIUpdateMovementRestrictionsDistance.Float() <= v4) {
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
    if (eManeuverState == aeManeuverState) {
        goto LABEL_9;
    }
    if (aeManeuverState || (eManeuverState != 4 || !IsTargetMovingAway())) {
        if (aeManeuverState == 4 || eManeuverState == 4) {}
        UpdateMovementRestrictions();
    LABEL_9:;
        eManeuverState = static_cast<Maneuver>(aeManeuverState);
    }
}

void CombatProcedureAttackMelee::StartEngage() {
    SetManeuverState(CombatProcedureAttackMelee::ENGAGE);
}

void CombatProcedureAttackMelee::ManeuverDecision() {
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    if (ptarget) {
        bool btargetAttacking = ptarget->GetAnimAction() == ANIM_ACTION_ATTACK;
        bool btargetRanged = Actor::IsUsingNonMeleeWeapon(ptarget);
        float a1a = pattacker->GetFatiguePercentage();
        int32_t WalkSpeed = static_cast<int32_t>(pattacker->GetWalkSpeed());
        TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
        float fdodgeScore = CombatFormulas::ChooseDodge(CurrentCombatStyle, WalkSpeed, a1a, btargetAttacking);
        if (fdodgeScore < 0.0) {
            fdodgeScore = 0.0f;
        }
        if (this->bRangedAllyWantsToShoot) {
            fdodgeScore = fdodgeScore + 25.0;
        }
        if (eCombatState == CombatProcedureAttackMelee::HOLD) {
            fdodgeScore = fdodgeScore + 10.0;
        }
        if (ptarget != pPlayer && !ptarget->IsInCombat()) {
            fdodgeScore = fdodgeScore - 100.0;
        }
        if (pattacker->IsSwimming()) {
            fdodgeScore = 0.0f;
        }
        float fdieRoll = static_cast<float>(Random() % 100);
        if (!(fdodgeScore >= static_cast<double>(fdieRoll))) {
            if (eManeuverState != CombatProcedureAttackMelee::IDLE) {
                TESCombatStyle* v13 = pCombatController->GetCurrentCombatStyle();
                float a2d = v13->Data.fIdleMaxTime;
                TESCombatStyle* v14 = pCombatController->GetCurrentCombatStyle();
                float a1d = v14->Data.fIdleMinTime;
                float fmaneuverTime_3 = CombatFormulas::GetRandomBetween(a1d, a2d);
                ManeuverTimer.Reset(fmaneuverTime_3);
                StopMoving();
                SetManeuverState(CombatProcedureAttackMelee::IDLE);
            }
        }
        else {
            bool bdodgerAttacking = pattacker->GetAnimAction() == ANIM_ACTION_ATTACK;
            float fdistance = GetFacingTargetDistance();
            float freach = GetCurrentAttackReach();
            uint16_t a2a = MovementRestrictions.sRestrictedMoves;
            bool bRangedAllyWantsToShoot = this->bRangedAllyWantsToShoot;
            TESCombatStyle* v2 = pCombatController->GetCurrentCombatStyle();
            uint16_t smoveMode = CombatFormulas::SelectDodgeDirection(
                v2, bdodgerAttacking, btargetAttacking, fdistance, freach, bRangedAllyWantsToShoot, btargetRanged, a2a
            );
            if (smoveMode) {
                Actor* pattacker_2 = GetAttacker();
                if (pattacker_2->GetAnimation()) {
                    float fmaneuverTime;
                    ANIM_GROUP_ENUM emove = ANIM_GROUP_MOVE_FORWARD;
                    int32_t v30 = smoveMode & 15;
                    if (v30 == 2) {
                        emove = ANIM_GROUP_MOVE_BACK;
                        TESCombatStyle* v7 = pCombatController->GetCurrentCombatStyle();
                        float a2c = v7->Data.fDodgeBackwardMaxTime;
                        TESCombatStyle* v8 = pCombatController->GetCurrentCombatStyle();
                        float a1c = v8->Data.fDodgeBackwardMinTime;
                        fmaneuverTime = CombatFormulas::GetRandomBetween(a1c, a2c);
                    }
                    else {
                        if (v30 == 4) {
                            emove = ANIM_GROUP_MOVE_LEFT;
                            GetMovementRestrictionFlag(4);
                            TESCombatStyle* v3 = pCombatController->GetCurrentCombatStyle();
                            float a2b = v3->Data.fDodgeLeftRightMaxTime;
                            TESCombatStyle* v4 = pCombatController->GetCurrentCombatStyle();
                            float a1b = v4->Data.fDodgeLeftRightMinTime;
                            fmaneuverTime = CombatFormulas::GetRandomBetween(a1b, a2b);
                        }
                        else {
                            float a1;
                            float a2;
                            if (v30 == 8) {
                                emove = ANIM_GROUP_MOVE_RIGHT;
                                GetMovementRestrictionFlag(8);
                                TESCombatStyle* v5 = pCombatController->GetCurrentCombatStyle();
                                a2 = v5->Data.fDodgeLeftRightMaxTime;
                                TESCombatStyle* v6 = pCombatController->GetCurrentCombatStyle();
                                a1 = v6->Data.fDodgeLeftRightMinTime;
                            }
                            else {
                                TESCombatStyle* v9 = pCombatController->GetCurrentCombatStyle();
                                a2 = v9->Data.fDodgeForwardMaxTime;
                                TESCombatStyle* v10 = pCombatController->GetCurrentCombatStyle();
                                a1 = v10->Data.fDodgeForwardMinTime;
                            }
                            fmaneuverTime = CombatFormulas::GetRandomBetween(a1, a2);
                        }
                    }
                    Actor* pattacker_3 = GetAttacker();
                    uint16_t AnimGroup = pattacker_3->GetAnimGroup(emove, nullptr, false, nullptr);
                    if (TESAnimGroup::GetType(AnimGroup) == emove) {
                        float fmoveCycleDuration = pattacker->GetAnimActionTime(emove);
                        float fmaneuverTime_2 = BSMAX(fmoveCycleDuration, fmaneuverTime);
                        if (btargetAttacking &&
                            fAIDodgeWalkChance.Float() < static_cast<double>(static_cast<float>(0.0))) {
                            pattacker->ForceMoveMode(smoveMode | 512);
                        }
                        else {
                            pattacker->ForceMoveMode(smoveMode | 256);
                        }
                        ManeuverTimer.Reset(fmaneuverTime_2);
                        DodgeTimer.Reset(fmoveCycleDuration);
                        SetManeuverState(CombatProcedureAttackMelee::DODGE);
                    }
                    else {
                        if (CombatController::GetShowVerboseMessages()) {
                            const char* v29;
                            if ((smoveMode & 4) != 0) {
                                v29 = "LEFT";
                            }
                            else {
                                const char* v28;
                                if ((smoveMode & 8) != 0) {
                                    v28 = "RIGHT";
                                }
                                else {
                                    const char* v27;
                                    if ((smoveMode & 1) != 0) {
                                        v27 = "FOREWARD";
                                    }
                                    else {
                                        v27 = "BACK";
                                    }
                                    v28 = v27;
                                }
                                v29 = v28;
                            }
                            Interface::PrintLine(
                                "%.20s chose to dodge %s but has no corresponding animation", pattacker->GetFullName(),
                                v29
                            );
                        }
                    }
                }
            }
        }
    }
}

void CombatProcedureAttackMelee::AttackDecision() {
    Actor* Target = GetTarget();
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
            int32_t v1;
            if (Target && Target->pCurrentProcess) {
                BaseProcess* CurrentAIProcess = Target->pCurrentProcess;
                v1 = CurrentAIProcess->GetFadeState();
            }
            else {
                v1 = 0;
            }
            if (!static_cast<uint8_t>(pCombatController->IsNotDetectingTarget()) && !v1 &&
                (!Target || !Target->IsDead(false))) {
                MeleeDecision();
            }
            break;
        }
        default: {
            return;
        }
    }
}

void CombatProcedureAttackMelee::HandleClose() {
    NiPoint3 currentGoal;
    if (eManeuverState == CombatProcedureAttackMelee::CLOSE) {
        Actor* pattacker = GetAttacker();
        Actor* ptarget = GetTarget();
        float fdistance = GetFacingTargetDistance();
        float freach = GetCurrentAttackReach();
        uint16_t smoveMode = pattacker->GetMoveMode();
        if (pattacker->IsPathing()) {
            float afAttackerReach = GetCurrentAttackReach();
            const NiPoint3* v1 = &ptarget->GetLocationOnReference();
            NiPoint3 faceTargetLoc =
                CombatUtilities::ComputeFaceTargetLocation(pattacker, ptarget, *v1, afAttackerReach);
            float* v2 = &fCombatMinEngageDistance.Float();
            if (!(fdistance > *v2 + *v2)) {
                if (WithinAttackRange(fdistance, freach, nullptr) && pCombatController->HasLineOfSight()) {
                    bool bgoalWithinRange = false;
                    if (pattacker->IsWaitingOnPath()) {
                        bgoalWithinRange = true;
                    }
                    else {
                        if (pattacker->GetCurrentPathfindingGoal_008B3840(&currentGoal)) {
                            float factorExtentSum = CombatUtilities::GetAttackerTargetExtentSum(pattacker, ptarget);
                            const NiPoint3* v4 = &ptarget->GetLocationOnReference();
                            float ffacedistance = (currentGoal - *v4).XYLength() - factorExtentSum;
                            bgoalWithinRange = CombatProcedure::WithinAttackRange(ffacedistance, freach, 0);
                        }
                    }
                    if (!bgoalWithinRange) {
                        MoveToFaceTargetLocation(faceTargetLoc);
                    }
                }
                else {
                    if (CombatProcedure::WithinAttackRange(fdistance, freach, 0)) {
                        float* v5;
                        if ((smoveMode & 2) != 0 &&
                            (v5 = &fCombatMinEngageDistance.Float(), *v5 > static_cast<double>(fdistance))) {
                            AttackDecision();
                        }
                        else {
                            StartEngage();
                        }
                    }
                }
            }
            else {
                if (!eCombatState) {
                    StartAdvance();
                }
            }
        }
        else {
            StartEngage();
        }
    }
}

void CombatProcedureAttackMelee::MoveToFaceTargetLocation(NiPoint3& arFaceTargetLocation) {
    const NiPoint3* v2;
    PathingAvoidNode v19;
    FNVD_INLINED_LOCAL(NiPointer<PathingAvoidNodeArray>, spavoidNodeArray, std::nullopt, std::nullopt);
    Actor* apMemory_40 = GetAttacker();
    Actor* ptarget = GetTarget();
    PathingRequest pathingRequest;
    PathingLocation v26(arFaceTargetLocation, apMemory_40);
    float WeaponReach = apMemory_40->GetRadius();
    if (v26.IsValidLocation(WeaponReach) &&
        (v2 = &ptarget->GetLocationOnReference(),
         CombatUtilities::CalculateLOS(apMemory_40, ptarget, arFaceTargetLocation, *v2, false, 30))) {
        PathingAvoidNodeArray* v16;
        PathingAvoidNodeArray* v20 =
            static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
        if (v20) {
            v16 = ::new (v20) PathingAvoidNodeArray;
        }
        else {
            v16 = nullptr;
        }
        NiPointer_PathingAvoidNodeArray_Construct(&spavoidNodeArray, v16);
        float WeaponReacha = fMovementNearTargetAvoidCost.Float();
        float afRadius = fMovementNearTargetAvoidRadius.Float();
        const NiPoint3* v3 = &ptarget->GetLocationOnReference();
        PathingAvoidNode* v4 = FNVD_RECONSTRUCT(PathingAvoidNode)(&v19, *v3, afRadius, WeaponReacha);
        spavoidNodeArray.m_pObject->Add(*v4);
        PathingAvoidNodeArray* WeaponReachb = spavoidNodeArray.m_pObject;
        TESWorldSpace* ListenerIdle = v26.pWorldSpace;
        TESObjectCELL* Cell = v26.GetCell();
        apMemory_40->BuildRequest(&pathingRequest, v26.GetLocation(), Cell, ListenerIdle, 5.0f, WeaponReachb);
        spavoidNodeArray.~NiPointer<PathingAvoidNodeArray>();
    }
    else {
        float fradius = 10.0f;
        if (pCombatController->pCombatState->bTargetLineOfSight) {
            float WeaponReachc = GetCurrentAttackReach();
            fradius = CombatUtilities::ComputeFaceTargetDistance(apMemory_40, ptarget, WeaponReachc);
        }
        TESWorldSpace* WorldSpace = ptarget->GetWorldSpace();
        TESObjectCELL* ParentCell = ptarget->GetParentCell();
        const NiPoint3* v6 = &ptarget->GetLocationOnReference();
        apMemory_40->BuildRequest(&pathingRequest, *v6, ParentCell, WorldSpace, fradius, nullptr);
    }
    apMemory_40->SetPathfindingGoal(&pathingRequest);
}

void CombatProcedureAttackMelee::HandleAdvance() {
    NiPoint3 currentGoal;
    if (eManeuverState == CombatProcedureAttackMelee::ADVANCE) {
        Actor* pattacker = GetAttacker();
        Actor* ptarget = GetTarget();
        float fdistance = GetFacingTargetDistance();
        float freach = GetCurrentAttackReach();
        bool btargetMovingAway = IsTargetMovingAway();
        BaseProcess* pprocess = pattacker->pCurrentProcess;
        if ((WithinAttackRange(fdistance, freach, nullptr) && pCombatController->HasLineOfSight()) ||
            !pCombatController->HasLineOfSight()) {
            float afAttackerReach = GetCurrentAttackReach();
            const NiPoint3* v1 = &ptarget->GetLocationOnReference();
            NiPoint3 targetLoc = CombatUtilities::ComputeFaceTargetLocation(pattacker, ptarget, *v1, afAttackerReach);
            bool bgoalWithinRange = false;
            if (pattacker->IsWaitingOnPath()) {
                bgoalWithinRange = true;
            }
            else {
                if (!pattacker->IsStoppingMovement() && pattacker->GetCurrentPathfindingGoal_008B3840(&currentGoal)) {
                    float factorExtentSum = CombatUtilities::GetAttackerTargetExtentSum(pattacker, ptarget);
                    const NiPoint3* v3 = &ptarget->GetLocationOnReference();
                    float ffacedistance = (currentGoal - *v3).XYLength() - factorExtentSum;
                    bgoalWithinRange = CombatProcedure::WithinAttackRange(ffacedistance, freach, 0);
                }
            }
            if (!bgoalWithinRange) {
                MoveToFaceTargetLocation(targetLoc);
            }
        }
        if (pattacker->IsPathingComplete()) {
            if (!btargetMovingAway) {
                StartEngage();
            }
        }
        else {
            if (pattacker->GetBlocked()) {
                pattacker->SetBlock(false);
            }
            else {
                if (CombatProcedure::WithinAttackRange(fdistance, freach, 0) && pCombatController->HasLineOfSight()) {
                    float* v4;
                    if (btargetMovingAway && (v4 = &fCombatMinEngageDistance.Float(), fdistance < *v4 + *v4)) {
                        StartClose();
                    }
                    else {
                        StartEngage();
                    }
                }
                else {
                    float freach_2;
                    if (eCombatState &&
                        ((freach_2 = freach + freach, CombatProcedure::WithinAttackRange(fdistance, freach_2, 0)) &&
                         (pCombatController->HasLineOfSight() &&
                          (pattacker->IsNpc() && (pprocess && pprocess->CanAttack()))))) {
                        if (CombatUtilities::IsTargetWithinStrikeAngle(pattacker, ptarget, nullptr)) {
                            TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
                            uint32_t v5 = Random() % 100;
                            if (v5 < static_cast<int32_t>(fCombatAdvanceNormalAttackChance.Float() * 100.0)) {
                                ANIM_GROUP_ENUM eattack = ANIM_GROUP_NONE;
                                const char* afReach = NiControllerSequence::pMorphTag;
                                Animation* v6 = pattacker->GetAnimation();
                                if (v6->FindNextNoteKey(AGS_MOVEMENT, afReach, nullptr) == 108) {
                                    eattack = ANIM_GROUP_ATTACK_LEFT;
                                }
                                else {
                                    eattack = ANIM_GROUP_ATTACK_RIGHT;
                                }
                                StartAttack(eattack, false);
                            }
                        }
                    }
                    else {
                        if (eCombatState) {
                            CheckForwardPowerAttack(fdistance, freach);
                        }
                    }
                }
            }
        }
    }
}

void CombatProcedureAttackMelee::MeleeDecision() {
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    if (eCombatState == CombatProcedureAttackMelee::DONE) {
        if (ptarget) {
            if (!pCombatController->IsSwitchingWeapons()) {
                eCurrentAttack = ANIM_GROUP_NONE;
                float fdistance = GetFacingTargetDistance();
                float freach = GetCurrentAttackReach();
                Animation* panim = pattacker->GetAnimation();
                ANIMATION_ACTION eanimAction = pattacker->GetAnimAction();
                ANIMATION_ACTION etargetAnimAction = ptarget->GetAnimAction();
                uint16_t Group = panim->GetGroup(AGS_WEAPON);
                bool bmidAttack = TESAnimGroup::IsAttackAction(Group);
                bool bfollowThrough = panim->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP;
                bool bpostAttack = eanimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
                bool v22 = eanimAction == ANIM_ACTION_RECOIL || eanimAction == ANIM_ACTION_STAGGER;
                bool btargetAttacking = ptarget->IsAttacking();
                bool btargetPostAttack = etargetAnimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
                bool btargetRecoiling =
                    etargetAnimAction == ANIM_ACTION_RECOIL || etargetAnimAction == ANIM_ACTION_STAGGER;
                bool btargetInCombat = ptarget->IsInCombat();
                bool btargetBlocking = ptarget->GetBlocked();
                if (CombatProcedure::WithinAttackRange(fdistance, freach, 0)) {
                    uint32_t v16;
                    bool bcanAttack;
                    float abDoPowerAttack_1 = 0.0f;
                    bool a1 = ptarget->IsKnockedOut();
                    bool v10 = pCombatController->GetCurrentWeapon() == nullptr;
                    int32_t afReach = pattacker->GetClampedActorValue(ActorValue::LUCK);
                    int32_t afDistance = pCombatController->GetAttackSkill();
                    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
                    float fattackScore = CombatFormulas::ChooseAttack(
                        CurrentCombatStyle, afDistance, afReach, btargetAttacking, v10, btargetRecoiling, a1,
                        btargetBlocking
                    );
                    // The elapsed time is compared with 5.0 (a double) without being stored first.
                    if (pCombatController->pCombatState->GetLastAttackTimeStamp().ElapsedTime() > 5.0 &&
                        fattackScore != 0.0) {
                        fattackScore = fattackScore + 100.0;
                    }
                    if (!btargetInCombat && ptarget != pPlayer) {
                        fattackScore = fattackScore + 100.0;
                    }
                    TESCombatStyle* v4 = pCombatController->GetCurrentCombatStyle();
                    float fblockScore = CombatFormulas::ChooseBlock(v4, 50, 50, false, btargetAttacking);
                    bool v20 = ptarget->IsParalyzed() ||
                        (ptarget->IsKnockedOut() ||
                         (ptarget->IsUnConscious() || (ptarget->IsSleeping() || ptarget->IsRestrained())));
                    TESObjectWEAP* ptargetWeap = ptarget->GetCurrentWeapon();
                    if (v20 || (!ptarget->IsWeaponDrawn() || (ptargetWeap && ptargetWeap->IsRangedWeapon()))) {
                        fblockScore = 0.0f;
                    }
                    if (bmidAttack && fattackScore > 0.0) {
                        bcanAttack = true;
                    }
                    else {
                        BaseProcess* CurrentAIProcess = GetAttacker()->pCurrentProcess;
                        bcanAttack = CurrentAIProcess->CanAttack();
                    }
                    if (!bcanAttack || !pCombatController->HasLineOfSight()) {
                        fattackScore = 0.0f;
                        if (!pCombatController->HasLineOfSight()) {
                            fblockScore = 0.0f;
                        }
                    }
                    if ((bmidAttack && !bfollowThrough) ||
                        (eManeuverState == CombatProcedureAttackMelee::ADVANCE || v22)) {
                        fblockScore = 0.0f;
                    }
                    if (!panim->GroupLoaded(170)) {
                        fblockScore = 0.0f;
                    }
                    float* v5 = &fCombatMaxHoldScore.Float();
                    float v17 = 100.0 - (fattackScore + fblockScore);
                    float fholdScore = BSMAX(v17, *v5);
                    if (bmidAttack && !bfollowThrough) {
                        fholdScore = 0.0f;
                    }
                    if (eLastCombatState == CombatProcedureAttackMelee::HOLD) {
                        fholdScore = 0.0f;
                    }
                    int32_t isum = static_cast<int32_t>(fattackScore + fblockScore + fholdScore);
                    if (isum <= 0) {
                        v16 = 100;
                    }
                    else {
                        v16 = isum;
                    }
                    float fdieRoll_2 = static_cast<float>(Random() % v16);
                    if (!(fattackScore > static_cast<double>(fdieRoll_2))) {
                        if (!(fblockScore > 0.0) || !(fdieRoll_2 < fattackScore + fblockScore)) {
                            if (eLastCombatState != CombatProcedureAttackMelee::HOLD) {
                                SetCombatState(CombatProcedureAttackMelee::HOLD);
                                TESCombatStyle* v6 = pCombatController->GetCurrentCombatStyle();
                                float a2a = v6->Data.fHoldMaxTime;
                                TESCombatStyle* v7 = pCombatController->GetCurrentCombatStyle();
                                float a1a = v7->Data.fHoldMinTime;
                                float fmeleeTargetTime = CombatFormulas::GetRandomBetween(a1a, a2a);
                                MeleeTimer.Reset(fmeleeTargetTime);
                            }
                        }
                        else {
                            pattacker->SetBlock(true);
                            SetCombatState(CombatProcedureAttackMelee::BLOCK);
                        }
                    }
                    else {
                        if (pCombatController->IsBlocking()) {
                            pattacker->SetBlock(false);
                            if (eCombatState == CombatProcedureAttackMelee::BLOCK) {
                                SetCombatState(CombatProcedureAttackMelee::DONE);
                            }
                        }
                        bool bdoPowerAttack = false;
                        bool bswitchAttack = false;
                        ANIM_GROUP_ENUM eattack = SelectAttack(
                            bmidAttack, fdistance, freach, btargetAttacking, btargetPostAttack, btargetRecoiling,
                            bdoPowerAttack, bswitchAttack
                        );
                        if (eattack != ANIM_GROUP_NONE) {
                            Actor* afTarget = GetTarget();
                            if (afTarget->Get3D()) {
                                if (bdoPowerAttack) {
                                    pCombatDialogueManager->StartDialogue(
                                        pCombatController, DT_COMBAT, DC_POWER_ATTACK, false
                                    );
                                }
                                else {
                                    pCombatDialogueManager->StartDialogue(
                                        pCombatController, DT_COMBAT, DC_ATTACK, false
                                    );
                                }
                                StartAttack(eattack, bswitchAttack);
                            }
                        }
                    }
                }
            }
        }
    }
}

void CombatProcedureAttackMelee::HandleAttack() {
    if (!eCombatState) {
        Actor* pattacker = GetAttacker();
        GetTarget();
        Animation* v1 = pattacker->GetAnimation();
        uint16_t Group = v1->GetGroup(AGS_WEAPON);
        if (TESAnimGroup::IsAttackAction(Group)) {
            Animation* v3 = pattacker->GetAnimation();
            if (v3->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP) {
                SetCombatState(CombatProcedureAttackMelee::DONE);
                MeleeDecision();
            }
            TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
            if (pweapon) {
                uint16_t AttackAnim = pweapon->GetAttackAnim();
                if (TESAnimGroup::IsAttackSpinAction(AttackAnim)) {
                    ANIM_GROUP_ENUM v5 = pweapon->GetAttackAnim();
                    pattacker->QueueAttack(v5);
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
        Actor* Attacker = GetAttacker();
        Actor* Target = GetTarget();
        if (!Attacker->GetBlocked()) {
            SetCombatState(DONE);
        }
        BaseProcess* CurrentAIProcess = Attacker->pCurrentProcess;
        ItemChange* v14 = CurrentAIProcess->GetCurrentWeapon();
        if (v14) {
            bool v10 = false;
            Actor* v7 = GetAttacker();
            if (v7->GetAnimation()) {
                Actor* v6 = GetAttacker();
                Animation* v1 = v6->GetAnimation();
                if (v1->GetCurrentSequence(AGS_LEFT_ARM)) {
                    Actor* v5 = GetAttacker();
                    Animation* v2 = v5->GetAnimation();
                    BSAnimGroupSequence* CurrentSequence = v2->GetCurrentSequence(AGS_LEFT_ARM);
                    v10 = CurrentSequence->GetState() != NiControllerSequence::kAnimating;
                }
            }
            float ItemHealth = v14->GetItemHealth(false);
            if (!(ItemHealth <= 0.0) || v10) {
                bool v4;
                if (Target) {
                    v4 = Target->GetAnimAction() == ANIM_ACTION_RECOIL;
                }
                else {
                    v4 = false;
                }
                if (!v4 || v10) {
                    if (!v10) {
                        CheckStopBlocking();
                    }
                }
                else {
                    Attacker->SetBlock(false);
                    SetCombatState(DONE);
                    AttackDecision();
                }
            }
            else {
                Attacker->SetBlock(false);
                SetCombatState(DONE);
            }
        }
    }
}

void CombatProcedureAttackMelee::UpdateMovementRestrictions() {
    Actor* Attacker = GetAttacker();
    if (!Attacker->IsPathing()) {
        MovementRestrictions.SetReset(true);
    }
}

bool CombatProcedureAttackMelee::IsTargetMovingAway() {
    NiPoint3 result;
    NiPoint3 v12;
    NiPoint3 pt = NiPoint3_ZERO;
    if (!GetTarget()) {
        return false;
    }
    Actor* Target = GetTarget();
    if (!Target->IsMoving()) {
        return false;
    }
    Actor* v7 = GetTarget();
    v7->GetLinearVelocity(pt);
    pt.z = 0.0;
    Actor* v6 = GetTarget();
    const NiPoint3* v3 = &v6->GetLocationOnReference();
    result = *v3 + pt;
    float FacingTargetDistance = GetFacingTargetDistance();
    Actor* Attacker = GetAttacker();
    const NiPoint3* v4 = &Attacker->GetLocationOnReference();
    v12 = *v4 - result;
    float v13 = v12.SqrLength();
    return v13 >= FacingTargetDistance * FacingTargetDistance;
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
    TESCombatStyle* currentCombatStyle = pCombatController->GetCurrentCombatStyle();
    currentCombatStyle->QPowerAttackChance();
    return (bool)((uint8_t)(afDistance >= (afReach + ((float)0.0))));
}

float CombatProcedureAttackMelee::GetCurrentAttackReach() {
    Actor* packageOwner = CombatProcedure::GetAttacker();
    if (!(fCurrentAttackReach < 0.0)) {
        return fCurrentAttackReach;
    }
    TESObjectWEAP* consoleSelection = pCombatController->GetCurrentWeapon();
    fCurrentAttackReach = CombatUtilities::GetWeaponReach(packageOwner, consoleSelection);
    return fCurrentAttackReach;
}

void CombatProcedureAttackMelee::StartAdvance() {
    SetManeuverState(CombatProcedureAttackMelee::ADVANCE);
    CombatProcedure::SetMoveMode(3);
    (&ManeuverTimer)->Reset(3.0f);
    HandleAdvance();
}

CombatProcedureAttackMelee::CombatState CombatProcedureAttackMelee::GetCombatState() {
    return eCombatState;
}

void CombatProcedureAttackMelee::StartAttack(ANIM_GROUP_ENUM aeAttack, bool abSwitchAttack) {
    Actor* th_s;
    if (((int32_t)aeAttack) != 255) {
        if (GetCombatState()) {
            th_s = CombatProcedure::GetAttacker();
            if (th_s->QueueAttack(aeAttack)) {
                SetCombatState(CombatProcedureAttackMelee::ATTACK);
                eCurrentAttack = aeAttack;
            }
        }
    }
}

void CombatProcedureAttackMelee::HandleHold() {
    if ((CombatProcedureAttackMelee::GetCombatState() == 2) && (&MeleeTimer)->Check()) {
        SetCombatState(CombatProcedureAttackMelee::DONE);
    }
}

float CombatProcedureAttackMelee::GetFacingTargetDistance() {
    if (!(fFacingTargetDistance < 0.0)) {
        return fFacingTargetDistance;
    }
    Actor* target = CombatProcedure::GetTarget();
    Actor* packageOwner = CombatProcedure::GetAttacker();
    fFacingTargetDistance = CombatUtilities::GetDistanceBetweenFacingActors(packageOwner, target, false);
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
    Actor* pattacker = GetAttacker();
    Actor* ptarget = GetTarget();
    abSwitchAttack = false;
    ANIM_GROUP_ENUM eattack_3 = ANIM_GROUP_NONE;
    TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
    if (!ptarget) {
        return ANIM_GROUP_NONE;
    }
    if (pattacker->IsNpc() && pattacker->IsSwimming()) {
        abDoPowerAttack = false;
    }
    else {
        bool v15 = ptarget->IsKnockedOut();
        float a3 = pattacker->GetFatiguePercentage();
        TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
        abDoPowerAttack = CombatFormulas::ChoosePowerAttack(CurrentCombatStyle, a3, abTargetRecoiling, v15, false);
    }
    if (eManeuverState == CombatProcedureAttackMelee::ADVANCE) {
        afReach = afReach + fCombatStepAdvanceDistance.Float();
        if (Random() % 100 < 2) {
            afReach = afReach + 100.0;
        }
    }
    bool bwithinNormalReach = CombatProcedure::WithinAttackRange(afDistance, afReach, 0);
    if (abMidAttack) {
        Animation* v12 = pattacker->GetAnimation();
        if (v12->GetGroupAction(AGS_WEAPON) == AGA_SPECIAL_IDLE_STOP) {
            if (abDoPowerAttack) {
                eattack_3 = static_cast<ANIM_GROUP_ENUM>(
                    SelectPowerAttack(afDistance, afReach, abTargetAttacking, abTargetPostAttack, false)
                );
            }
            if (eattack_3 == ANIM_GROUP_NONE && bwithinNormalReach) {
                Actor* Attacker = GetAttacker();
                Animation* v13 = Attacker->GetAnimation();
                switch (v13->FindSwitchKey(nullptr)) {
                    case 51: {
                        eattack_3 = ANIM_GROUP_ATTACK_3;
                        break;
                    }
                    case 52: {
                        eattack_3 = ANIM_GROUP_ATTACK_4;
                        break;
                    }
                    case 53: {
                        eattack_3 = ANIM_GROUP_ATTACK_5;
                        break;
                    }
                    case 54: {
                        eattack_3 = ANIM_GROUP_ATTACK_6;
                        break;
                    }
                    case 55: {
                        eattack_3 = ANIM_GROUP_ATTACK_7;
                        break;
                    }
                    case 56: {
                        eattack_3 = ANIM_GROUP_ATTACK_8;
                        break;
                    }
                    case 108: {
                        eattack_3 = ANIM_GROUP_ATTACK_LEFT;
                        break;
                    }
                    default: {
                        if (!pweapon || pweapon->GetAttackAnim() == ANIM_GROUP_NONE) {
                            eattack_3 = ANIM_GROUP_ATTACK_RIGHT;
                        }
                        else {
                            eattack_3 = pweapon->GetAttackAnim();
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
            eattack_3 = static_cast<ANIM_GROUP_ENUM>(
                SelectPowerAttack(afDistance, afReach, abTargetAttacking, abTargetPostAttack, false)
            );
        }
        if (eattack_3 == ANIM_GROUP_NONE && bwithinNormalReach) {
            ANIM_GROUP_ENUM eattack;
            if (pweapon) {
                eattack = pweapon->GetAttackAnim();
            }
            else {
                eattack = ANIM_GROUP_NONE;
            }
            eattack_3 = eattack;
            if (eattack == ANIM_GROUP_NONE) {
                const char* v16 = NiControllerSequence::pMorphTag;
                Animation* v11 = pattacker->GetAnimation();
                if (v11->FindNextNoteKey(AGS_MOVEMENT, v16, nullptr) == 108) {
                    return ANIM_GROUP_ATTACK_LEFT;
                }
                else {
                    return ANIM_GROUP_ATTACK_RIGHT;
                }
            }
        }
    }
    return eattack_3;
}

void CombatProcedureAttackMelee::CheckStopBlocking() {
    Animation* v1;
    Animation* v2;
    BSAnimGroupSequence* CurrentSequence;
    Actor* v6;
    Actor* v7;
    Actor* Attacker = GetAttacker();
    if (!Attacker->GetAnimation() ||
        ((v7 = GetAttacker(), (v1 = v7->GetAnimation(), !v1->GetCurrentSequence(AGS_LEFT_ARM))) ||
         (v6 = GetAttacker(),
          (v2 = v6->GetAnimation(),
           (CurrentSequence = v2->GetCurrentSequence(AGS_LEFT_ARM),
            CurrentSequence->GetState() == NiControllerSequence::kAnimating))))) {
        Actor* v16 = GetAttacker();
        Actor* Target = GetTarget();
        if (pCombatController->IsBlocking() || eCombatState == 1) {
            if (Target) {
                bool IsAttacking = Target->IsAttacking();
                bool v12 = Target->GetAnimAction() == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
                char IsBlocking = 0;
                if (Target->GetCombatController()) {
                    CombatController* v4 = Target->GetCombatController();
                    IsBlocking = v4->IsBlocking();
                }
                float FacingTargetDistance = GetFacingTargetDistance();
                float CurrentAttackReach = GetCurrentAttackReach();
                bool v5 = Target->IsParalyzed() ||
                    (Target->IsKnockedOut() ||
                     (Target->IsUnConscious() || (Target->IsSleeping() || Target->IsRestrained())));
                if (CurrentAttackReach < static_cast<double>(FacingTargetDistance) ||
                    (v5 || (v12 || (IsBlocking || ((Random() % 100 < 5 && !IsAttacking) || !v16->GetBlocked()))))) {
                    if (v16->GetBlocked()) {
                        v16->SetBlock(false);
                    }
                    pCombatController->SetIsBlocking(0);
                    if (eCombatState == 1) {
                        SetCombatState(DONE);
                    }
                }
            }
            else {
                if (v16->GetBlocked()) {
                    v16->SetBlock(false);
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
        PlayerCharacter* apAttacker = static_cast<PlayerCharacter*>(GetAttacker());
        Actor* apTarget = GetTarget();
        float fFacingTargetDistance = GetFacingTargetDistance();
        float fCurrentAttackReach = GetCurrentAttackReach();
        if (CombatProcedure::WithinAttackRange(fFacingTargetDistance, fCurrentAttackReach, 0) &&
            (!pCombatController->HasLineOfSight() &&
             CombatUtilities::IsTargetWithinStrikeAngle(apAttacker, apTarget, nullptr))) {
            goto LABEL_5;
        }
        if (pCombatController->IsBlocking() || eCombatState == 1) {
            CheckStopBlocking();
        }
        else {
            if (WithinAttackRange(fFacingTargetDistance, fCurrentAttackReach, nullptr)) {
                if (fCombatMinEngageDistance.Float() < fFacingTargetDistance - fCurrentAttackReach ||
                    IsTargetMovingAway()) {
                LABEL_5:;
                    StartAdvance();
                    return;
                }
                StartClose();
            }
            else {
                ManeuverDecision();
            }
        }
    }
}

void CombatProcedureAttackMelee::HandleDodge() {
    if (eManeuverState == CombatProcedureAttackMelee::DODGE) {
        ANIMATION_ACTION etargetAnimAction;
        if (ManeuverTimer.Check()) {
            StartEngage();
            UpdateMovementRestrictions();
            return;
        }
        Actor* pattacker = GetAttacker();
        Actor* ptarget = GetTarget();
        uint32_t imoveMode = pattacker->GetMoveMode();
        if (!pattacker->IsMoving()) {
            goto LABEL_5;
        }
        CheckUpdateMovementRestrictions();
        if (ptarget) {
            etargetAnimAction = ptarget->GetAnimAction();
        }
        else {
            etargetAnimAction = ANIM_ACTION_NONE;
        }
        if (etargetAnimAction == ANIM_ACTION_ATTACK && eCombatState == CombatProcedureAttackMelee::HOLD) {
            SetCombatState(CombatProcedureAttackMelee::DONE);
            AttackDecision();
        }
        if (!MovementRestrictions.CanMove(imoveMode)) {
            StopMoving();
            if (CombatController::GetShowVerboseMessages()) {
                Interface::PrintLine("%.20s has run out of room to dodge.", pattacker->GetFullName());
            }
        LABEL_5:;
            StartEngage();
            return;
        }
        float fdistance = GetFacingTargetDistance();
        float freach = GetCurrentAttackReach();
        if ((imoveMode & 4) != 0 || (imoveMode & 8) != 0) {
            if (!(freach < static_cast<double>(fdistance))) {
                if (fCombatMinEngageDistance.Float() > static_cast<double>(fdistance) && (imoveMode & 1) != 0) {
                    pattacker->ForceMoveMode(imoveMode & 0xFFFFFFFEu);
                }
            }
            else {
                if (WithinAttackRange(fdistance, freach, nullptr)) {
                    if (DodgeTimer.Check()) {
                        StopMoving();
                        StartAdvance();
                        return;
                    }
                }
                else {
                    pattacker->ForceMoveMode(imoveMode | 1);
                }
            }
        }
        else {
            if ((imoveMode & 1) != 0 &&
                (fCombatMinEngageDistance.Float() >= static_cast<double>(fdistance) && DodgeTimer.Check())) {
                StopMoving();
                StartEngage();
                return;
            }
        }
        if (etargetAnimAction == ANIM_ACTION_ATTACK) {
            float fdieRoll = static_cast<float>(Random() % 100);
            if ((imoveMode & 256) != 0 && !pCombatController->IsBlocking() &&
                fAIDodgeWalkChance.Float() < static_cast<double>(fdieRoll)) {
                pattacker->ForceMoveMode((imoveMode & 0xFFFFFCFFu) | 512);
            }
        }
        else {
            if ((imoveMode & 512) != 0) {
                pattacker->ForceMoveMode((imoveMode & 0xFFFFFCFFu) | 256);
            }
        }
        pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_ATTACK, false);
    }
}

void CombatProcedureAttackMelee::HandleIdle() {
    if (eManeuverState == CombatProcedureAttackMelee::IDLE) {
        float freach;
        float fdistance;
        Actor* ptarget = GetTarget();
        // The elapsed time is compared with 0.5 as a double without being stored.
        if (ManeuverTimer.Check() ||
            (ManeuverTimer.ElapsedTime() > 0.5 &&
             (fdistance = GetFacingTargetDistance(),
              (freach = GetCurrentAttackReach(), WithinAttackRange(fdistance, freach, nullptr))))) {
            StartEngage();
        }
        else {
            if ((ptarget && ptarget->IsAttacking()) || bRangedAllyWantsToShoot) {
                if (eCombatState == CombatProcedureAttackMelee::HOLD) {
                    SetCombatState(CombatProcedureAttackMelee::DONE);
                    AttackDecision();
                }
                ManeuverDecision();
            }
        }
    }
}

void CombatProcedureAttackMelee::StartClose() {
    NiPoint3 arFaceTargetLocation;
    SetManeuverState(CLOSE);
    Actor* apAttacker = GetAttacker();
    Actor* apTarget = GetTarget();
    SetMoveMode(1);
    float afAttackerReach = GetCurrentAttackReach();
    const NiPoint3* v1 = &apTarget->GetLocationOnReference();
    arFaceTargetLocation = CombatUtilities::ComputeFaceTargetLocation(apAttacker, apTarget, *v1, afAttackerReach);
    MoveToFaceTargetLocation(arFaceTargetLocation);
    HandleClose();
}

void CombatProcedureAttackMelee::CheckForwardPowerAttack(float afDistance, float afReach) {
    if (eCombatState == CombatProcedureAttackMelee::DONE) {
        Actor* pattacker = GetAttacker();
        Actor* ptarget = GetTarget();
        if (ptarget) {
            if (!(fAttackPickFailedTimestamp.ElapsedTime() < 1.0) && pCombatController->bHasForwardAttacks) {
                BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
                if (CurrentAIProcess->CanAttack()) {
                    bool bmidAttack;
                    if (pattacker->GetAnimation()) {
                        Animation* v3 = pattacker->GetAnimation();
                        uint16_t Group = v3->GetGroup(AGS_WEAPON);
                        bmidAttack = TESAnimGroup::IsPowerAttackAction(Group);
                    }
                    else {
                        bmidAttack = false;
                    }
                    if (!bmidAttack && (!pattacker->IsNpc() || !pattacker->IsSwimming())) {
                        if (pCombatController->HasLineOfSight()) {
                            if (CombatUtilities::IsTargetWithinStrikeAngle(pattacker, ptarget, nullptr)) {
                                ANIMATION_ACTION etargetAnimAction = ptarget->GetAnimAction();
                                bool btargetAttacking = ptarget->IsAttacking();
                                bool btargetPostAttack = etargetAnimAction == ANIM_ACTION_ATTACK_FOLLOW_THROUGH;
                                bool btargetRecoiling =
                                    etargetAnimAction == ANIM_ACTION_RECOIL || etargetAnimAction == ANIM_ACTION_STAGGER;
                                bool btargetBlocking = ptarget->GetBlocked();
                                ANIM_GROUP_ENUM eattack = static_cast<ANIM_GROUP_ENUM>(
                                    SelectPowerAttack(afDistance, afReach, btargetAttacking, btargetPostAttack, true)
                                );
                                if (eattack != ANIM_GROUP_NONE && CanDoPowerAttack(eattack)) {
                                    int32_t idieRoll = Random() % 100;
                                    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
                                    if (idieRoll < CurrentCombatStyle->GetRushingAttackPercentChance()) {
                                        CombatTimer fattackScore;
                                        bool a1 = ptarget->IsKnockedOut();
                                        bool a3 = pCombatController->GetCurrentWeapon() == nullptr;
                                        int32_t ClampedActorValue = pattacker->GetClampedActorValue(ActorValue::LUCK);
                                        int32_t AttackSkill = pCombatController->GetAttackSkill();
                                        TESCombatStyle* v6 = pCombatController->GetCurrentCombatStyle();
                                        fattackScore.fTargetTime = CombatFormulas::ChooseAttack(
                                            v6, AttackSkill, ClampedActorValue, btargetAttacking, a3, btargetRecoiling,
                                            a1, btargetBlocking
                                        );
                                        TESCombatStyle* v7 = pCombatController->GetCurrentCombatStyle();
                                        float fblockScore =
                                            CombatFormulas::ChooseBlock(v7, 50, 50, false, btargetAttacking);
                                        float* v8 = &fCombatMaxHoldScore.Float();
                                        float v16 = 100.0 - (fattackScore.fTargetTime + fblockScore);
                                        float fholdScore = BSMAX(v16, *v8);
                                        if (fblockScore < static_cast<double>(fattackScore.fTargetTime) &&
                                            fholdScore < static_cast<double>(fattackScore.fTargetTime)) {
                                            const NiPoint3* v13 = &ptarget->GetLocationOnReference();
                                            const NiPoint3* v9 = &pattacker->GetLocationOnReference();
                                            if (CombatUtilities::CalculateLOS(
                                                    pattacker, ptarget, *v9, *v13, false, 30
                                                )) {
                                                StartAttack(eattack, false);
                                                SetManeuverState(CombatProcedureAttackMelee::ENGAGE);
                                            }
                                            else {
                                                float a2 = CombatTimer::GetTimer();
                                                fattackScore.SetStartTime(a2);
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
        }
    }
}

bool CombatProcedureAttackMelee::CanDoPowerAttack(ANIM_GROUP_ENUM aePowerAttack) {
    GetAttacker();
    bool v11 = false;
    TESCombatStyle* CurrentCombatStyle = pCombatController->GetCurrentCombatStyle();
    if (static_cast<double>(CurrentCombatStyle->QPowerAttackChance()) > 0.0) {
        v11 = true;
        TESCombatStyle* v3 = pCombatController->GetCurrentCombatStyle();
        if (v3->IsFlagSet(TESCombatStyle::USE_PA_PERCENT_CHANCE)) {
            switch (aePowerAttack) {
                case ANIM_GROUP_ATTACK_NORMAL_POWER: {
                    TESCombatStyle* v4 = pCombatController->GetCurrentCombatStyle();
                    v11 = v4->GetPowerAttackN() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_FORWARD_POWER: {
                    TESCombatStyle* v5 = pCombatController->GetCurrentCombatStyle();
                    v11 = v5->GetPowerAttackF() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_BACK_POWER: {
                    TESCombatStyle* v6 = pCombatController->GetCurrentCombatStyle();
                    v11 = v6->GetPowerAttackB() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_LEFT_POWER: {
                    TESCombatStyle* v7 = pCombatController->GetCurrentCombatStyle();
                    v11 = v7->GetPowerAttackL() > 0;
                    break;
                }
                case ANIM_GROUP_ATTACK_RIGHT_POWER: {
                    TESCombatStyle* v8 = pCombatController->GetCurrentCombatStyle();
                    // The byte is read as signed.
                    v11 = static_cast<int8_t>(v8->Data.cPowerAttackRightChance) > 0;
                    break;
                }
                default: {
                    return v11;
                }
            }
        }
    }
    return v11;
}

bool CombatProcedureAttackMelee::GetMovementRestrictionFlag(uint16_t a1) {
    return MovementRestrictions.CanMove(a1);
}

float CombatProcedureAttackMelee::GetMovingTargetVelocity() {
    Actor* v1;
    if (GetTarget() && (v1 = GetTarget(), v1->IsMoving())) {
        NiPoint3 v6 = NiPoint3_ZERO;
        Actor* Target = GetTarget();
        Target->GetLinearVelocity(v6);
        return v6.XYLength();
    }
    else {
        return 0.0;
    }
}
