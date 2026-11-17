#include "Fallout/AI/Combat/CombatActions.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatActionAcquireLineOfSight.h"
#include "Fallout/AI/Combat/CombatActionActivateCombatItem.h"
#include "Fallout/AI/Combat/CombatActionApproachTarget.h"
#include "Fallout/AI/Combat/CombatActionAttackGrenade.h"
#include "Fallout/AI/Combat/CombatActionAttackGrenadeFlushTarget.h"
#include "Fallout/AI/Combat/CombatActionAttackGrenadeFromCover.h"
#include "Fallout/AI/Combat/CombatActionAttackHandToHand.h"
#include "Fallout/AI/Combat/CombatActionAttackMelee.h"
#include "Fallout/AI/Combat/CombatActionAttackRanged.h"
#include "Fallout/AI/Combat/CombatActionAttackRangedExplosive.h"
#include "Fallout/AI/Combat/CombatActionAttackRangedExplosiveFromCover.h"
#include "Fallout/AI/Combat/CombatActionAttackRangedFromCover.h"
#include "Fallout/AI/Combat/CombatActionAvoidThreat.h"
#include "Fallout/AI/Combat/CombatActionDodge.h"
#include "Fallout/AI/Combat/CombatActionDrawWeapon.h"
#include "Fallout/AI/Combat/CombatActionFlee.h"
#include "Fallout/AI/Combat/CombatActionHide.h"
#include "Fallout/AI/Combat/CombatActionIgnoreBlockedTarget.h"
#include "Fallout/AI/Combat/CombatActionInvestigate.h"
#include "Fallout/AI/Combat/CombatActionMove.h"
#include "Fallout/AI/Combat/CombatActionMoveAndAttackGrenade.h"
#include "Fallout/AI/Combat/CombatActionMoveAndAttackRanged.h"
#include "Fallout/AI/Combat/CombatActionMoveAndAttackRangedExplosive.h"
#include "Fallout/AI/Combat/CombatActionMoveAndSwitchToMelee.h"
#include "Fallout/AI/Combat/CombatActionSearch.h"
#include "Fallout/AI/Combat/CombatActionSwitchWeapon.h"
#include "Fallout/AI/Combat/CombatActionUseCombatItem.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatProcedureActivateObject.h"
#include "Fallout/AI/Combat/CombatProcedureAttackGrenade.h"
#include "Fallout/AI/Combat/CombatProcedureAttackMelee.h"
#include "Fallout/AI/Combat/CombatProcedureAttackRanged.h"
#include "Fallout/AI/Combat/CombatProcedureBeInCover.h"
#include "Fallout/AI/Combat/CombatProcedureEngageTarget.h"
#include "Fallout/AI/Combat/CombatProcedureEvade.h"
#include "Fallout/AI/Combat/CombatProcedureHideFromTarget.h"
#include "Fallout/AI/Combat/CombatProcedureMove.h"
#include "Fallout/AI/Combat/CombatProcedureSearch.h"
#include "Fallout/AI/Combat/CombatProcedureSwitchWeapon.h"
#include "Fallout/AI/Combat/CombatProcedureUseCombatItem.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/CombatController.h"

bool GetGrenadeThrowHighArc(CombatState* apThis, [[maybe_unused]] void* apEdx) {
    return (bool)((uint8_t)apThis->bGrenadeThrowHighArc);
}

CombatAction::CombatAction(COMBAT_ACTION aeType)
  : eType(aeType), iFlags(CombatAction::kDescriptors[aeType].uiDefaultFlags) {}

// Frees the world states the four lists own; the lists' own destructors run after.
CombatAction::~CombatAction() {
    for (CombatWorldStateList* plist = &Effects; plist->m_item; plist->RemoveHead()) {
        ourFree(plist->m_item);
    }
    for (CombatWorldStateList* plist = &ExecutionConditions; plist->m_item; plist->RemoveHead()) {
        ourFree(plist->m_item);
    }
    for (CombatWorldStateList* plist = &Preconditions; plist->m_item; plist->RemoveHead()) {
        ourFree(plist->m_item);
    }
    for (CombatWorldStateList* plist = &PossibleEffects; plist->m_item; plist->RemoveHead()) {
        ourFree(plist->m_item);
    }
}

const char* CombatAction::GetCombatActionInfoName() {
    return CombatAction::kDescriptors[eType].pName;
}

void CombatAction::AddPrecondition(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    Preconditions.AddWorldState(aeState, aiValue);
}

void CombatAction::RemovePrecondition(COMBAT_WORLD_STATE aeState) {
    Preconditions.RemoveWorldState(aeState);
}

void CombatAction::AddExecutionCondition(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    ExecutionConditions.AddWorldState(aeState, aiValue);
}

void CombatAction::RemoveExecutionCondition(COMBAT_WORLD_STATE aeState) {
    ExecutionConditions.RemoveWorldState(aeState);
}

void CombatAction::AddEffect(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    Effects.AddWorldState(aeState, aiValue);
}

void CombatAction::AddPossibleEffect(COMBAT_WORLD_STATE a1, int32_t a2) {
    PossibleEffects.AddWorldState(a1, a2);
}

void CombatAction::RemoveResult(COMBAT_WORLD_STATE a1) {
    Effects.RemoveWorldState(a1);
}

bool CombatAction::HasTargetState(CombatWorldState* apEntry) {
    uint32_t FormHealth_0 = apEntry->iValue;
    return Effects.HasMatchingWorldState(apEntry->eState, FormHealth_0);
}

BSSimpleList<CombatAction*>* CombatAction::GetActionsForTargetState(COMBAT_WORLD_STATE aeState) {
    return CombatAction::pPerStateLists[aeState];
}

CombatActionAcquireLineOfSight::CombatActionAcquireLineOfSight(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 13);
    AddEffect(COMBAT_WORLD_STATE_TARGET_360_LOS, 1);
}

CombatActionActivateCombatItem::CombatActionActivateCombatItem(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 0xFFFFFFFFu);
    AddEffect(COMBAT_WORLD_STATE_HAS_COMBAT_ITEM, (-1));
}

void CombatActionActivateCombatItem::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatState* MusicType = apCombatController->pCombatState;
    CombatState::TargetedObject* TargetedObject = MusicType->GetTargetedObject(aiActionValue - 17);
    if (TargetedObject) {
        CombatProcedureActivateObject* v4;
        CombatProcedureActivateObject* apMemory =
            static_cast<CombatProcedureActivateObject*>(ourMalloc(sizeof(CombatProcedureActivateObject)));
        if (apMemory) {
            v4 = ::new (apMemory) CombatProcedureActivateObject(
                static_cast<TESObjectREFR*>(TargetedObject->pRef), TargetedObject->pObject
            );
        }
        else {
            v4 = nullptr;
        }
        apCombatController->SetActionProcedure(v4);
    }
}

void CombatActionActivateCombatItem::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

CombatActionApproachTarget::CombatActionApproachTarget(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 0);
    AddEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
    AddEffect(COMBAT_WORLD_STATE_TARGET_360_LOS, 1);
}

CombatActionAttackGrenade::CombatActionAttackGrenade(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED, 0);
    AddPrecondition(COMBAT_WORLD_STATE_TARGET_360_LOS, 1u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 3u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 3u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackGrenade::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureEngageTarget* v3;
    CombatProcedureAttackGrenade* v4;
    CombatProcedureAttackGrenade* v6 =
        static_cast<CombatProcedureAttackGrenade*>(ourMalloc(sizeof(CombatProcedureAttackGrenade)));
    if (v6) {
        v4 = ::new (v6) CombatProcedureAttackGrenade;
    }
    else {
        v4 = nullptr;
    }
    apCombatController->SetActionProcedure(v4);
    CombatProcedureEngageTarget* apMemory =
        static_cast<CombatProcedureEngageTarget*>(ourMalloc(sizeof(CombatProcedureEngageTarget)));
    if (apMemory) {
        v3 = ::new (apMemory) CombatProcedureEngageTarget;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetMovementProcedure(v3);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackGrenade::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost =
        static_cast<float>(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE) + dCost);
    return fCost;
}

CombatActionAttackGrenadeFlushTarget::CombatActionAttackGrenadeFlushTarget(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 3u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 3u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_FLUSHED, 1);
}

bool CombatActionAttackGrenadeFlushTarget::CanExecute(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiActionValue
) {
    if (!CombatAction::CanExecute(apCombatController, apWorldStateArray, aiActionValue)) {
        return false;
    }
    CombatState* MusicType = apCombatController->pCombatState;
    return MusicType->FindCombatLocation(11);
}

void CombatActionAttackGrenadeFlushTarget::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackGrenade* v6;
    CombatState* apThis = apCombatController->pCombatState;
    CombatState* MusicType = apCombatController->pCombatState;
    CombatLocation* HealthRestoration = MusicType->pCombatLocation;
    CombatProcedureAttackGrenade* apMemory =
        static_cast<CombatProcedureAttackGrenade*>(ourMalloc(sizeof(CombatProcedureAttackGrenade)));
    if (apMemory) {
        bool v5 = GetGrenadeThrowHighArc(apThis, nullptr);
        const NiPoint3* Pos = &HealthRestoration->GetPoint();
        v6 = ::new (apMemory) CombatProcedureAttackGrenade(*Pos, v5);
    }
    else {
        v6 = nullptr;
    }
    apCombatController->SetActionProcedure(v6);
}

// A weapon cost of FLT_MAX means no grenade: the action costs FLT_MAX. Otherwise the float weapon cost is added to the
// float CombatAction::GetCost returns and the sum is rounded to float.
float CombatActionAttackGrenadeFlushTarget::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    float fWeaponCost = CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE);
    if (fWeaponCost >= 3.40282347e+38f) {
        return 3.40282347e+38f;
    }
    float fCost = CombatAction::GetCost(apCombatController, aiActionValue) + fWeaponCost;
    return fCost;
}

CombatActionAttackGrenadeFromCover::CombatActionAttackGrenadeFromCover(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_DETECTED, 1);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 3);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 3);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 2);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackGrenadeFromCover::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureBeInCover* v5;
    CombatProcedureAttackGrenade* v6;
    CombatProcedureAttackGrenade* apMemory =
        static_cast<CombatProcedureAttackGrenade*>(ourMalloc(sizeof(CombatProcedureAttackGrenade)));
    if (apMemory) {
        v6 = ::new (apMemory) CombatProcedureAttackGrenade;
    }
    else {
        v6 = nullptr;
    }
    apCombatController->SetActionProcedure(v6);
    CombatProcedureBeInCover* v7 = static_cast<CombatProcedureBeInCover*>(ourMalloc(sizeof(CombatProcedureBeInCover)));
    if (v7) {
        CombatState* MusicType = apCombatController->pCombatState;
        CombatCoverLocation* CoverLocation = MusicType->GetCoverLocation();
        v5 = ::new (v7) CombatProcedureBeInCover(CoverLocation);
    }
    else {
        v5 = nullptr;
    }
    apCombatController->SetMovementProcedure(v5);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackGrenadeFromCover::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost =
        static_cast<float>(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE) + dCost);
    return fCost;
}

CombatActionAttackHandToHand::CombatActionAttackHandToHand(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0x11u);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0x12u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 6u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 6u);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 1u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackHandToHand::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackMelee* v3;
    void* v4 = ourMalloc(sizeof(CombatProcedureAttackMelee));
    if (v4) {
        v3 = ::new (v4) CombatProcedureAttackMelee;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetActionProcedure(v3);
}

float CombatActionAttackHandToHand::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (apCombatController->GetAttacker() && apCombatController->GetAttacker()->IsPlayerTeammate() &&
        apCombatController->GetAttacker()->GetCurrentWeapon()) {
        return 3.40282347e+38f;
    }
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost =
        static_cast<float>(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_TYPE_COUNT) + dCost);
    return fCost;
}

CombatActionAttackMelee::CombatActionAttackMelee(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0x11u);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0x12u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 2u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 2u);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 1u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackMelee::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackMelee* v3;
    void* v4 = ourMalloc(sizeof(CombatProcedureAttackMelee));
    if (v4) {
        v3 = ::new (v4) CombatProcedureAttackMelee;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetActionProcedure(v3);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackMelee::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost =
        static_cast<float>(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_MELEE) + dCost);
    return fCost;
}

CombatActionAttackRanged::CombatActionAttackRanged(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddPrecondition(COMBAT_WORLD_STATE_TARGET_360_LOS, 1u);
    AddPrecondition(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 1u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 1u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackRanged::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureEngageTarget* v3;
    CombatProcedureAttackRanged* v4;
    void* v6 = ourMalloc(sizeof(CombatProcedureAttackRanged));
    if (v6) {
        v4 = ::new (v6) CombatProcedureAttackRanged;
    }
    else {
        v4 = nullptr;
    }
    apCombatController->SetActionProcedure(v4);
    void* v5 = ourMalloc(sizeof(CombatProcedureEngageTarget));
    if (v5) {
        v3 = ::new (v5) CombatProcedureEngageTarget;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetMovementProcedure(v3);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackRanged::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (apCombatController->GetAttacker() && apCombatController->GetAttacker()->IsPlayerTeammate()) {
        if (apCombatController->GetAttacker()->GetLocalVariableOnActor("CombatStyleMelee") == 1.0) {
            return 3.40282347e+38f;
        }
    }
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost =
        static_cast<float>(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED) + dCost);
    return fCost;
}

CombatActionAttackRangedExplosive::CombatActionAttackRangedExplosive(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED, 0);
    AddPrecondition(COMBAT_WORLD_STATE_TARGET_360_LOS, 1u);
    AddPrecondition(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 0);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackRangedExplosive::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureEngageTarget* v3;
    CombatProcedureAttackRanged* v4;
    void* v6 = ourMalloc(sizeof(CombatProcedureAttackRanged));
    if (v6) {
        v4 = ::new (v6) CombatProcedureAttackRanged;
    }
    else {
        v4 = nullptr;
    }
    apCombatController->SetActionProcedure(v4);
    void* v5 = ourMalloc(sizeof(CombatProcedureEngageTarget));
    if (v5) {
        v3 = ::new (v5) CombatProcedureEngageTarget;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetMovementProcedure(v3);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackRangedExplosive::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED_EXPLOSIVE) + dCost
    );
    return fCost;
}

CombatActionAttackRangedExplosiveFromCover::CombatActionAttackRangedExplosiveFromCover(
    CombatAction::COMBAT_ACTION aeType
)
  : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_DETECTED, 1);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 0);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 2);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackRangedExplosiveFromCover::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackRanged* pAttack;
    CombatProcedureAttackRanged* pAttackMemory =
        static_cast<CombatProcedureAttackRanged*>(ourMalloc(sizeof(CombatProcedureAttackRanged)));
    if (pAttackMemory) {
        pAttack = ::new (pAttackMemory) CombatProcedureAttackRanged;
    }
    else {
        pAttack = nullptr;
    }
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureBeInCover* pCover;
    CombatProcedureBeInCover* pCoverMemory =
        static_cast<CombatProcedureBeInCover*>(ourMalloc(sizeof(CombatProcedureBeInCover)));
    if (pCoverMemory) {
        CombatState* pCombatState = apCombatController->pCombatState;
        CombatCoverLocation* pCoverLocation = pCombatState->GetCoverLocation();
        pCover = ::new (pCoverMemory) CombatProcedureBeInCover(pCoverLocation);
    }
    else {
        pCover = nullptr;
    }
    apCombatController->SetMovementProcedure(pCover);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackRangedExplosiveFromCover::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED_EXPLOSIVE) + dCost
    );
    return fCost;
}

CombatActionAttackRangedFromCover::CombatActionAttackRangedFromCover(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0x11u);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_DETECTED, 1u);
    AddExecutionCondition(COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT, 0);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 1u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 1u);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 2u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

void CombatActionAttackRangedFromCover::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureBeInCover* v5;
    CombatProcedureAttackRanged* v6;
    CombatProcedureAttackRanged* apMemory =
        static_cast<CombatProcedureAttackRanged*>(ourMalloc(sizeof(CombatProcedureAttackRanged)));
    if (apMemory) {
        v6 = ::new (apMemory) CombatProcedureAttackRanged;
    }
    else {
        v6 = nullptr;
    }
    apCombatController->SetActionProcedure(v6);
    CombatProcedureBeInCover* v7 = static_cast<CombatProcedureBeInCover*>(ourMalloc(sizeof(CombatProcedureBeInCover)));
    if (v7) {
        CombatState* MusicType = apCombatController->pCombatState;
        CombatCoverLocation* CoverLocation = MusicType->GetCoverLocation();
        v5 = ::new (v7) CombatProcedureBeInCover(CoverLocation);
    }
    else {
        v5 = nullptr;
    }
    apCombatController->SetMovementProcedure(v5);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackRangedFromCover::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost =
        static_cast<float>(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED) + dCost);
    return fCost;
}

CombatActionAvoidThreat::CombatActionAvoidThreat(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 14);
    AddEffect(COMBAT_WORLD_STATE_THREATENED, 0);
}

CombatActionDodge::CombatActionDodge(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 3);
    AddEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

CombatActionDrawWeapon::CombatActionDrawWeapon(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_HAS_WEAPON_TYPE, 0xFFFFFFFFu);
    AddExecutionCondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 7u);
    AddEffect(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, (-1));
    AddEffect(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, (-1));
}

void CombatActionDrawWeapon::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureSwitchWeapon* v3;
    TESObjectWEAP* a2 = nullptr;
    // The action value is the weapon type, 6 for none.
    if (aiActionValue != 6) {
        a2 = apCombatController->pCombatState->GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(aiActionValue));
    }
    CombatProcedureSwitchWeapon* apMemory =
        static_cast<CombatProcedureSwitchWeapon*>(ourMalloc(sizeof(CombatProcedureSwitchWeapon)));
    if (apMemory) {
        v3 = ::new (apMemory) CombatProcedureSwitchWeapon(a2);
    }
    else {
        v3 = nullptr;
    }
    apCombatController->AddBackgroundProcedure(v3);
    apCombatController->pCurrentWeapon = a2;
}

void CombatActionDrawWeapon::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(), s_combatActionDrawWeaponTypes[aiActionValue]
    );
}

CombatActionFlee::CombatActionFlee(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 12);
    AddEffect(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
}

void CombatActionFlee::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureHideFromTarget* v5;
    apCombatController->pCombatState->cFleeState = 2;
    CombatProcedureHideFromTarget* apMemory =
        static_cast<CombatProcedureHideFromTarget*>(ourMalloc(sizeof(CombatProcedureHideFromTarget)));
    if (apMemory) {
        float afNumSeconds = fCombatFleeWaitTime.Float();
        CombatCoverLocation* CoverLocation = apCombatController->pCombatState->GetCoverLocation();
        v5 = ::new (apMemory) CombatProcedureHideFromTarget(CoverLocation, afNumSeconds, true, true);
    }
    else {
        v5 = nullptr;
    }
    apCombatController->SetMovementProcedure(v5);
}

CombatActionHide::CombatActionHide(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 10);
    AddEffect(COMBAT_WORLD_STATE_HIDING, (-1));
}

void CombatActionHide::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureHideFromTarget* v6;
    CombatProcedureHideFromTarget* apMemory =
        static_cast<CombatProcedureHideFromTarget*>(ourMalloc(sizeof(CombatProcedureHideFromTarget)));
    if (apMemory) {
        float afNumSeconds = static_cast<float>(aiActionValue);
        CombatState* MusicType = apCombatController->pCombatState;
        CombatCoverLocation* CoverLocation = MusicType->GetCoverLocation();
        v6 = ::new (apMemory) CombatProcedureHideFromTarget(CoverLocation, afNumSeconds, false, false);
    }
    else {
        v6 = nullptr;
    }
    apCombatController->SetMovementProcedure(v6);
}

void CombatActionHide::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    const char* combatActionInfoName = CombatAction::GetCombatActionInfoName();
    BSsprintf(apDebugText, aiMaxLength, "%s: %i", combatActionInfoName, aiActionValue);
}

CombatActionIgnoreBlockedTarget::CombatActionIgnoreBlockedTarget(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

void CombatActionIgnoreBlockedTarget::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatState* MusicType = apCombatController->pCombatState;
    MusicType->SetTargetBlocked(false);
}

CombatActionInvestigate::CombatActionInvestigate(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, (-65512));
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 15);
    AddEffect(COMBAT_WORLD_STATE_TARGET_LOST, 0);
}

CombatActionMove::CombatActionMove(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 0x11u);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 0xFFFF0018u);
    AddEffect(COMBAT_WORLD_STATE_AT_LOCATION, 0xFFFFFFFFu);
    AddPossibleEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

bool CombatActionMove::CanExecute(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiActionValue
) {
    return CombatAction::CanExecute(apCombatController, apWorldStateArray, aiActionValue) &&
        ChooseLocation(apCombatController, apWorldStateArray, aiActionValue);
}

void CombatActionMove::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    StartMovementProcedure(apCombatController, aiActionValue);
}

bool CombatActionMove::CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) {
    return CheckLocationIdentical(apCombatController, aiActionValue);
}

float CombatActionMove::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    float Cost = CombatAction::GetCost(apCombatController, aiActionValue);
    CombatPlannerData* DebugTextRoot = apCombatController->pCombatPlannerData;
    return DebugTextRoot->GetCost(aiActionValue) + Cost;
}

void CombatActionMove::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

void CombatActionMove::StartMovementProcedure(CombatController* apCombatController, int32_t aiLocation) {
    CombatProcedure* pmovementProcedure_2;
    CombatLocation* pcombatLocation = apCombatController->pCombatState->pCombatLocation;
    if (pcombatLocation->GetFlag(1u)) {
        CombatProcedure* pmovementProcedure;
        CombatProcedureEvade* apMemory = static_cast<CombatProcedureEvade*>(ourMalloc(sizeof(CombatProcedureEvade)));
        if (apMemory) {
            pmovementProcedure = ::new (apMemory) CombatProcedureEvade(aiLocation);
        }
        else {
            pmovementProcedure = nullptr;
        }
        pmovementProcedure_2 = pmovementProcedure;
    }
    else {
        CombatProcedure* pmovementProcedure_3;
        CombatProcedureMove* v5 = static_cast<CombatProcedureMove*>(ourMalloc(sizeof(CombatProcedureMove)));
        if (v5) {
            float afTargetRadius = pcombatLocation->fRadius;
            pmovementProcedure_3 = ::new (v5) CombatProcedureMove(pcombatLocation, afTargetRadius, aiLocation);
        }
        else {
            pmovementProcedure_3 = nullptr;
        }
        pmovementProcedure_2 = pmovementProcedure_3;
    }
    apCombatController->SetMovementProcedure(pmovementProcedure_2);
}

bool CombatActionMove::CheckLocationIdentical(CombatController* apCombatController, int32_t aiLocation) {
    NiPoint3 a1;
    CombatProcedure* pmovementProcedure = apCombatController->pMovementProcedure;
    if (!pmovementProcedure) {
        return false;
    }
    CombatLocation* pcombatLocation = apCombatController->pCombatState->pCombatLocation;
    if (!pcombatLocation || pcombatLocation->iLocation != aiLocation) {
        return false;
    }
    CombatProcedure::COMBAT_PROCEDURES etype = pmovementProcedure->GetType();
    if (etype == CombatProcedure::MOVE_TO_LOCATION) {
        CombatProcedureMove* pmoveProcedure = static_cast<CombatProcedureMove*>(pmovementProcedure);
        if (static_cast<uint32_t>(pcombatLocation->iLocation) != pmoveProcedure->iLocation) {
            return false;
        }
        if (pcombatLocation->pRef) {
            TESObjectREFR* Ref = pcombatLocation->pRef;
            if (Ref != pmoveProcedure->pTargetRef) {
                return false;
            }
        }
        else {
            NiPoint3* DestinationPos = pmoveProcedure->GetDestinationPos(&a1);
            if (pcombatLocation->GetPoint() != *DestinationPos) {
                return false;
            }
        }
        if (pcombatLocation->GetFlag(1u)) {
            return false;
        }
    }
    else {
        if (etype != CombatProcedure::EVADE) {
            return false;
        }
        CombatProcedureEvade* pevadeProcedure = static_cast<CombatProcedureEvade*>(pmovementProcedure);
        if (!pcombatLocation->GetFlag(1u)) {
            return false;
        }
        if (pcombatLocation->iLocation != pevadeProcedure->iLocation) {
            return false;
        }
    }
    return true;
}

// apWorldStateArray is unused.
bool CombatActionMove::ChooseLocation(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiLocation
) {
    return apCombatController->pCombatState->FindCombatLocation(aiLocation);
}

CombatActionMoveAndAttackGrenade::CombatActionMoveAndAttackGrenade(CombatAction::COMBAT_ACTION aeType)
  : CombatActionAttackGrenade(aeType) {
    RemoveExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST);
    RemovePrecondition(COMBAT_WORLD_STATE_AT_LOCATION);
    RemovePrecondition(COMBAT_WORLD_STATE_TARGET_BLOCKED);
    RemovePrecondition(COMBAT_WORLD_STATE_TARGET_360_LOS);
    RemoveResult(COMBAT_WORLD_STATE_TARGET_DEAD);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddExecutionCondition(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
    AddEffect(COMBAT_WORLD_STATE_AT_LOCATION, (-1));
    AddPossibleEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

bool CombatActionMoveAndAttackGrenade::CanExecute(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiActionValue
) {
    return CombatAction::CanExecute(apCombatController, apWorldStateArray, aiActionValue) &&
        CombatActionMove::ChooseLocation(apCombatController, apWorldStateArray, aiActionValue);
}

void CombatActionMoveAndAttackGrenade::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackGrenade* v3;
    CombatProcedureAttackGrenade* apMemory =
        static_cast<CombatProcedureAttackGrenade*>(ourMalloc(sizeof(CombatProcedureAttackGrenade)));
    if (apMemory) {
        v3 = ::new (apMemory) CombatProcedureAttackGrenade;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetActionProcedure(v3);
    CombatActionMove::StartMovementProcedure(apCombatController, aiActionValue);
}

bool CombatActionMoveAndAttackGrenade::CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) {
    return CombatActionMove::CheckLocationIdentical(apCombatController, aiActionValue);
}

float CombatActionMoveAndAttackGrenade::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE) >= 3.4028235e+38f)) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    else {
        return 3.4028235e+38f;
    }
}

void CombatActionMoveAndAttackGrenade::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

CombatActionMoveAndAttackRanged::CombatActionMoveAndAttackRanged(CombatAction::COMBAT_ACTION aeType)
  : CombatActionAttackRanged(aeType) {
    RemoveExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST);
    RemoveExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS);
    RemovePrecondition(COMBAT_WORLD_STATE_AT_LOCATION);
    RemovePrecondition(COMBAT_WORLD_STATE_TARGET_BLOCKED);
    RemovePrecondition(COMBAT_WORLD_STATE_TARGET_360_LOS);
    RemoveResult(COMBAT_WORLD_STATE_TARGET_DEAD);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddExecutionCondition(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
    AddEffect(COMBAT_WORLD_STATE_AT_LOCATION, (-1));
    AddPossibleEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

bool CombatActionMoveAndAttackRanged::CanExecute(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiActionValue
) {
    return CombatAction::CanExecute(apCombatController, apWorldStateArray, aiActionValue) &&
        CombatActionMove::ChooseLocation(apCombatController, apWorldStateArray, aiActionValue);
}

void CombatActionMoveAndAttackRanged::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackRanged* v3;
    CombatProcedureAttackRanged* apMemory =
        static_cast<CombatProcedureAttackRanged*>(ourMalloc(sizeof(CombatProcedureAttackRanged)));
    if (apMemory) {
        v3 = ::new (apMemory) CombatProcedureAttackRanged;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetActionProcedure(v3);
    CombatActionMove::StartMovementProcedure(apCombatController, aiActionValue);
}

bool CombatActionMoveAndAttackRanged::CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) {
    return CombatActionMove::CheckLocationIdentical(apCombatController, aiActionValue);
}

float CombatActionMoveAndAttackRanged::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED) >= 3.4028235e+38f)) {
        Actor* Attacker;
        Actor* v5;
        if (apCombatController->GetAttacker() &&
            ((Attacker = apCombatController->GetAttacker(), Attacker->IsPlayerTeammate()) &&
             (v5 = apCombatController->GetAttacker(), v5->GetLocalVariableOnActor("CombatStyleMelee") == 1.0))) {
            return 3.4028235e+38f;
        }
        else {
            return CombatAction::GetCost(apCombatController, aiActionValue);
        }
    }
    else {
        return 3.4028235e+38f;
    }
}

void CombatActionMoveAndAttackRanged::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

CombatActionMoveAndAttackRangedExplosive::CombatActionMoveAndAttackRangedExplosive(CombatAction::COMBAT_ACTION aeType)
  : CombatActionAttackRangedExplosive(aeType) {
    RemoveExecutionCondition(COMBAT_WORLD_STATE_TARGET_LOST);
    RemoveExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS);
    RemovePrecondition(COMBAT_WORLD_STATE_AT_LOCATION);
    RemovePrecondition(COMBAT_WORLD_STATE_TARGET_BLOCKED);
    RemovePrecondition(COMBAT_WORLD_STATE_TARGET_360_LOS);
    RemoveResult(COMBAT_WORLD_STATE_TARGET_DEAD);
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddExecutionCondition(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
    AddEffect(COMBAT_WORLD_STATE_AT_LOCATION, -1);
    AddPossibleEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

bool CombatActionMoveAndAttackRangedExplosive::CanExecute(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiActionValue
) {
    return CombatAction::CanExecute(apCombatController, apWorldStateArray, aiActionValue) &&
        CombatActionMove::ChooseLocation(apCombatController, apWorldStateArray, aiActionValue);
}

void CombatActionMoveAndAttackRangedExplosive::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureAttackRanged* pProcedure;
    void* pMemory = ourMalloc(sizeof(CombatProcedureAttackRanged));
    if (pMemory) {
        pProcedure = ::new (pMemory) CombatProcedureAttackRanged;
    }
    else {
        pProcedure = nullptr;
    }
    apCombatController->SetActionProcedure(pProcedure);
    CombatActionMove::StartMovementProcedure(apCombatController, aiActionValue);
}

bool CombatActionMoveAndAttackRangedExplosive::CheckIdentical(
    CombatController* apCombatController,
    int32_t aiActionValue
) {
    return CombatActionMove::CheckLocationIdentical(apCombatController, aiActionValue);
}

float CombatActionMoveAndAttackRangedExplosive::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED_EXPLOSIVE) >=
          0x1.fffffe0000000p+127f)) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    else {
        return 0x1.fffffe0000000p+127f;
    }
}

void CombatActionMoveAndAttackRangedExplosive::GetDebugText(
    char* apDebugText,
    uint32_t aiMaxLength,
    int32_t aiActionValue
) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

CombatActionMoveAndSwitchToMelee::CombatActionMoveAndSwitchToMelee(CombatAction::COMBAT_ACTION aeType)
  : CombatActionMove(aeType) {
    RemovePrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN);
    AddExecutionCondition(COMBAT_WORLD_STATE_HAS_WEAPON_TYPE, 2);
    AddPossibleEffect(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, 2);
    AddPossibleEffect(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 2);
}

void CombatActionMoveAndSwitchToMelee::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureSwitchWeapon* v3;
    CombatActionMove::Execute(apCombatController, aiActionValue);
    TESObjectWEAP* pweapon = apCombatController->pCombatState->GetWeaponByCombatType(COMBAT_WEAPON_MELEE);
    CombatProcedureSwitchWeapon* apMemory =
        static_cast<CombatProcedureSwitchWeapon*>(ourMalloc(sizeof(CombatProcedureSwitchWeapon)));
    if (apMemory) {
        v3 = ::new (apMemory) CombatProcedureSwitchWeapon(pweapon);
    }
    else {
        v3 = nullptr;
    }
    apCombatController->AddBackgroundProcedure(v3);
    apCombatController->pCurrentWeapon = pweapon;
}

CombatActionSearch::CombatActionSearch(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 0xFFFF0018u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_LOST, 0);
}

void CombatActionSearch::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureSearch* v3;
    CombatProcedureSearch* apMemory = static_cast<CombatProcedureSearch*>(ourMalloc(sizeof(CombatProcedureSearch)));
    if (apMemory) {
        v3 = ::new (apMemory) CombatProcedureSearch;
    }
    else {
        v3 = nullptr;
    }
    apCombatController->SetMovementProcedure(v3);
}

CombatActionSwitchWeapon::CombatActionSwitchWeapon(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_HAS_WEAPON_TYPE, 0xFFFFFFFFu);
    AddEffect(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, (-1));
    AddEffect(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, (-1));
}

void CombatActionSwitchWeapon::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureSwitchWeapon* pProcedure;
    TESObjectWEAP* pWeapon = nullptr;
    // The action value is the weapon type.
    if (aiActionValue != COMBAT_WEAPON_TYPE_NONE) {
        pWeapon =
            apCombatController->pCombatState->GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(aiActionValue));
    }
    void* pMemory = ourMalloc(sizeof(CombatProcedureSwitchWeapon));
    if (pMemory) {
        pProcedure = ::new (pMemory) CombatProcedureSwitchWeapon(pWeapon);
    }
    else {
        pProcedure = nullptr;
    }
    apCombatController->AddBackgroundProcedure(pProcedure);
    apCombatController->pCurrentWeapon = pWeapon;
}

float CombatActionSwitchWeapon::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!apCombatController->GetAttacker()) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    Actor* Attacker = apCombatController->GetAttacker();
    if (!Attacker->IsPlayerTeammate()) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    Actor* v4 = apCombatController->GetAttacker();
    if (v4->GetLocalVariableOnActor("CombatStyleMelee") != 1.0) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    TESObjectWEAP* WeaponByCombatType = nullptr;
    if (aiActionValue != 6) {
        CombatState* MusicType = apCombatController->pCombatState;
        WeaponByCombatType = MusicType->GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(aiActionValue));
    }
    if (WeaponByCombatType && !WeaponByCombatType->IsMeleeWeapon()) {
        return 3.4028235e+38f;
    }
    else {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
}

void CombatActionSwitchWeapon::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(), s_combatActionDrawWeaponTypes[aiActionValue]
    );
}

CombatActionUseCombatItem::CombatActionUseCombatItem(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_HAS_COMBAT_ITEM, (-1));
    AddEffect(COMBAT_WORLD_STATE_USED_COMBAT_ITEM, (-1));
}

void CombatActionUseCombatItem::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatState* MusicType = apCombatController->pCombatState;
    TESBoundObject* apItem = MusicType->GetUsableCombatItem(aiActionValue - 17);
    if (apItem) {
        CombatProcedureUseCombatItem* v4;
        CombatProcedureUseCombatItem* apMemory =
            static_cast<CombatProcedureUseCombatItem*>(ourMalloc(sizeof(CombatProcedureUseCombatItem)));
        if (apMemory) {
            v4 = ::new (apMemory) CombatProcedureUseCombatItem(apItem);
        }
        else {
            v4 = nullptr;
        }
        apCombatController->SetActionProcedure(v4);
    }
}

void CombatActionUseCombatItem::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText, aiMaxLength, "%s: %s", GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

int32_t CombatLocation::GetLocation() {
    return iLocation;
}

TESObjectREFR* CombatLocation::GetRef() {
    return pRef;
}

const NiPoint3& CombatLocation::GetPoint() {
    return WorldLoc.Point;
}
