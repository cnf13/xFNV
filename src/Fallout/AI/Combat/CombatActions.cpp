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

#include <limits>

bool GetGrenadeThrowHighArc(CombatState* apThis, [[maybe_unused]] void* apEdx) {
    return apThis->bGrenadeThrowHighArc;
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

void CombatAction::AddPossibleEffect(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    PossibleEffects.AddWorldState(aeState, aiValue);
}

void CombatAction::RemoveResult(COMBAT_WORLD_STATE aeState) {
    Effects.RemoveWorldState(aeState);
}

bool CombatAction::HasTargetState(CombatWorldState* apEntry) {
    uint32_t uiValue = apEntry->iValue;
    return Effects.HasMatchingWorldState(apEntry->eState, uiValue);
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
    CombatState* pCombatState = apCombatController->pCombatState;
    CombatState::TargetedObject* pTargetedObject = pCombatState->GetTargetedObject(aiActionValue - 17);
    if (pTargetedObject) {
        CombatProcedureActivateObject* pProcedure = new CombatProcedureActivateObject(
            static_cast<TESObjectREFR*>(pTargetedObject->pRef),
            pTargetedObject->pObject
        );
        apCombatController->SetActionProcedure(pProcedure);
    }
}

void CombatActionActivateCombatItem::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
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
    CombatProcedureAttackGrenade* pAttack = new CombatProcedureAttackGrenade;
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureEngageTarget* pEngage = new CombatProcedureEngageTarget;
    apCombatController->SetMovementProcedure(pEngage);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackGrenade::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE) + dCost
    );
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
    CombatState* pCombatState = apCombatController->pCombatState;
    return pCombatState->FindCombatLocation(11);
}

void CombatActionAttackGrenadeFlushTarget::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatState* pCombatState = apCombatController->pCombatState;
    CombatLocation* pCombatLocation = pCombatState->pCombatLocation;
    CombatProcedureAttackGrenade* pAttack = new CombatProcedureAttackGrenade(
        pCombatLocation->GetPoint(),
        GetGrenadeThrowHighArc(pCombatState, nullptr)
    );
    apCombatController->SetActionProcedure(pAttack);
}

// A weapon cost of FLT_MAX means no grenade: the action costs FLT_MAX. Otherwise the float weapon cost is added to the
// float CombatAction::GetCost returns and the sum is rounded to float.
float CombatActionAttackGrenadeFlushTarget::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    float fWeaponCost = CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE);
    if (fWeaponCost >= std::numeric_limits<float>::max()) {
        return std::numeric_limits<float>::max();
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
    CombatProcedureAttackGrenade* pAttack = new CombatProcedureAttackGrenade;
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureBeInCover* pCover = new CombatProcedureBeInCover(
        apCombatController->pCombatState->GetCoverLocation()
    );
    apCombatController->SetMovementProcedure(pCover);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackGrenadeFromCover::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE) + dCost
    );
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
    CombatProcedureAttackMelee* pProcedure;
    pProcedure = new CombatProcedureAttackMelee;
    apCombatController->SetActionProcedure(pProcedure);
}

float CombatActionAttackHandToHand::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (apCombatController->GetAttacker() &&
        apCombatController->GetAttacker()->IsPlayerTeammate() &&
        apCombatController->GetAttacker()->GetCurrentWeapon()) {
        return std::numeric_limits<float>::max();
    }
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_TYPE_COUNT) + dCost
    );
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
    CombatProcedureAttackMelee* pProcedure;
    pProcedure = new CombatProcedureAttackMelee;
    apCombatController->SetActionProcedure(pProcedure);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackMelee::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_MELEE) + dCost
    );
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
    CombatProcedureAttackRanged* pAttack;
    pAttack = new CombatProcedureAttackRanged;
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureEngageTarget* pEngage;
    pEngage = new CombatProcedureEngageTarget;
    apCombatController->SetMovementProcedure(pEngage);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackRanged::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (apCombatController->GetAttacker() &&
        apCombatController->GetAttacker()->IsPlayerTeammate() &&
        apCombatController->GetAttacker()->GetLocalVariableOnActor("CombatStyleMelee") == 1.0) {
        return std::numeric_limits<float>::max();
    }

    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED) + dCost
    );
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
    CombatProcedureAttackRanged* pAttack;
    pAttack = new CombatProcedureAttackRanged;
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureEngageTarget* pEngage;
    pEngage = new CombatProcedureEngageTarget;
    apCombatController->SetMovementProcedure(pEngage);
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
    CombatProcedureAttackRanged* pAttack = new CombatProcedureAttackRanged;
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureBeInCover* pCover = new CombatProcedureBeInCover(
        apCombatController->pCombatState->GetCoverLocation()
    );
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
    CombatProcedureAttackRanged* pAttack = new CombatProcedureAttackRanged;
    apCombatController->SetActionProcedure(pAttack);
    CombatProcedureBeInCover* pCover = new CombatProcedureBeInCover(
        apCombatController->pCombatState->GetCoverLocation()
    );
    apCombatController->SetMovementProcedure(pCover);
}

// Keeps the base cost as a double and rounds the sum to float.
float CombatActionAttackRangedFromCover::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    double dCost = CombatAction::GetCost(apCombatController, aiActionValue);
    float fCost = static_cast<float>(
        CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED) + dCost
    );
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
    CombatProcedureSwitchWeapon* pProcedure;
    TESObjectWEAP* pWeapon = nullptr;
    // The action value is the weapon type, 6 for none.
    if (aiActionValue != 6) {
        pWeapon = apCombatController->pCombatState->GetWeaponByCombatType(
            static_cast<COMBAT_WEAPON_TYPE>(aiActionValue)
        );
    }
    pProcedure = new CombatProcedureSwitchWeapon(pWeapon);
    apCombatController->AddBackgroundProcedure(pProcedure);
    apCombatController->pCurrentWeapon = pWeapon;
}

void CombatActionDrawWeapon::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
        s_combatActionDrawWeaponTypes[aiActionValue]
    );
}

CombatActionFlee::CombatActionFlee(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 12);
    AddEffect(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
}

void CombatActionFlee::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    apCombatController->pCombatState->cFleeState = 2;
    CombatProcedureHideFromTarget* pProcedure = new CombatProcedureHideFromTarget(
        apCombatController->pCombatState->GetCoverLocation(),
        fCombatFleeWaitTime.Float(),
        true,
        true
    );
    apCombatController->SetMovementProcedure(pProcedure);
}

CombatActionHide::CombatActionHide(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_AT_LOCATION, 10);
    AddEffect(COMBAT_WORLD_STATE_HIDING, (-1));
}

void CombatActionHide::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureHideFromTarget* pProcedure = new CombatProcedureHideFromTarget(
        apCombatController->pCombatState->GetCoverLocation(),
        static_cast<float>(aiActionValue),
        false,
        false
    );
    apCombatController->SetMovementProcedure(pProcedure);
}

void CombatActionHide::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    const char* pcInfoName = CombatAction::GetCombatActionInfoName();
    BSsprintf(apDebugText, aiMaxLength, "%s: %i", pcInfoName, aiActionValue);
}

CombatActionIgnoreBlockedTarget::CombatActionIgnoreBlockedTarget(CombatAction::COMBAT_ACTION aeType)
  : CombatAction(aeType) {
    AddEffect(COMBAT_WORLD_STATE_TARGET_BLOCKED, 0);
}

void CombatActionIgnoreBlockedTarget::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatState* pCombatState = apCombatController->pCombatState;
    pCombatState->SetTargetBlocked(false);
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
    float fCost = CombatAction::GetCost(apCombatController, aiActionValue);
    CombatPlannerData* pPlannerData = apCombatController->pCombatPlannerData;
    return pPlannerData->GetCost(aiActionValue) + fCost;
}

void CombatActionMove::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
        s_combatActionMoveTypes[static_cast<uint16_t>(aiActionValue)]
    );
}

void CombatActionMove::StartMovementProcedure(CombatController* apCombatController, int32_t aiLocation) {
    CombatProcedure* pmovementProcedure;
    CombatLocation* pcombatLocation = apCombatController->pCombatState->pCombatLocation;
    if (pcombatLocation->GetFlag(CombatLocation::FLAG_EVADE)) {
        pmovementProcedure = new CombatProcedureEvade(aiLocation);
    }
    else {
        pmovementProcedure = new CombatProcedureMove(pcombatLocation, pcombatLocation->fRadius, aiLocation);
    }
    apCombatController->SetMovementProcedure(pmovementProcedure);
}

bool CombatActionMove::CheckLocationIdentical(CombatController* apCombatController, int32_t aiLocation) {
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
            if (pcombatLocation->pRef != pmoveProcedure->pTargetRef) {
                return false;
            }
        }
        else if (pcombatLocation->GetPoint() != pmoveProcedure->GetDestinationPos()) {
            return false;
        }
        if (pcombatLocation->GetFlag(CombatLocation::FLAG_EVADE)) {
            return false;
        }
    }
    else {
        if (etype != CombatProcedure::EVADE) {
            return false;
        }
        CombatProcedureEvade* pevadeProcedure = static_cast<CombatProcedureEvade*>(pmovementProcedure);
        if (!pcombatLocation->GetFlag(CombatLocation::FLAG_EVADE)) {
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
    CombatProcedureAttackGrenade* pProcedure = new CombatProcedureAttackGrenade;
    apCombatController->SetActionProcedure(pProcedure);
    CombatActionMove::StartMovementProcedure(apCombatController, aiActionValue);
}

bool CombatActionMoveAndAttackGrenade::CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) {
    return CombatActionMove::CheckLocationIdentical(apCombatController, aiActionValue);
}

float CombatActionMoveAndAttackGrenade::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_GRENADE) >=
          std::numeric_limits<float>::max())) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }

    return std::numeric_limits<float>::max();
}

void CombatActionMoveAndAttackGrenade::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
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
    CombatProcedureAttackRanged* pProcedure = new CombatProcedureAttackRanged;
    apCombatController->SetActionProcedure(pProcedure);
    CombatActionMove::StartMovementProcedure(apCombatController, aiActionValue);
}

bool CombatActionMoveAndAttackRanged::CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) {
    return CombatActionMove::CheckLocationIdentical(apCombatController, aiActionValue);
}

float CombatActionMoveAndAttackRanged::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!(CombatUtilities::GetCostForWeaponType(apCombatController, COMBAT_WEAPON_RANGED) >=
          std::numeric_limits<float>::max())) {
        if (apCombatController->GetAttacker() &&
            apCombatController->GetAttacker()->IsPlayerTeammate() &&
            apCombatController->GetAttacker()->GetLocalVariableOnActor("CombatStyleMelee") == 1.0) {
            return std::numeric_limits<float>::max();
        }

        return CombatAction::GetCost(apCombatController, aiActionValue);
    }

    return std::numeric_limits<float>::max();
}

void CombatActionMoveAndAttackRanged::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
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
    pProcedure = new CombatProcedureAttackRanged;
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

    return 0x1.fffffe0000000p+127f;
}

void CombatActionMoveAndAttackRangedExplosive::GetDebugText(
    char* apDebugText,
    uint32_t aiMaxLength,
    int32_t aiActionValue
) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
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
    CombatProcedureSwitchWeapon* pProcedure;
    CombatActionMove::Execute(apCombatController, aiActionValue);
    TESObjectWEAP* pweapon = apCombatController->pCombatState->GetWeaponByCombatType(COMBAT_WEAPON_MELEE);
    pProcedure = new CombatProcedureSwitchWeapon(pweapon);
    apCombatController->AddBackgroundProcedure(pProcedure);
    apCombatController->pCurrentWeapon = pweapon;
}

CombatActionSearch::CombatActionSearch(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddExecutionCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 17);
    AddPrecondition(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 0xFFFF0018u);
    AddEffect(COMBAT_WORLD_STATE_TARGET_LOST, 0);
}

void CombatActionSearch::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatProcedureSearch* pProcedure;
    pProcedure = new CombatProcedureSearch;
    apCombatController->SetMovementProcedure(pProcedure);
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
        pWeapon = apCombatController->pCombatState->GetWeaponByCombatType(
            static_cast<COMBAT_WEAPON_TYPE>(aiActionValue)
        );
    }
    pProcedure = new CombatProcedureSwitchWeapon(pWeapon);
    apCombatController->AddBackgroundProcedure(pProcedure);
    apCombatController->pCurrentWeapon = pWeapon;
}

float CombatActionSwitchWeapon::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    if (!apCombatController->GetAttacker()) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    Actor* pAttacker = apCombatController->GetAttacker();
    if (!pAttacker->IsPlayerTeammate()) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    pAttacker = apCombatController->GetAttacker();
    if (pAttacker->GetLocalVariableOnActor("CombatStyleMelee") != 1.0) {
        return CombatAction::GetCost(apCombatController, aiActionValue);
    }
    TESObjectWEAP* pWeapon = nullptr;
    if (aiActionValue != 6) {
        CombatState* pCombatState = apCombatController->pCombatState;
        pWeapon = pCombatState->GetWeaponByCombatType(static_cast<COMBAT_WEAPON_TYPE>(aiActionValue));
    }
    if (pWeapon && !pWeapon->IsMeleeWeapon()) {
        return std::numeric_limits<float>::max();
    }

    return CombatAction::GetCost(apCombatController, aiActionValue);
}

void CombatActionSwitchWeapon::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
        s_combatActionDrawWeaponTypes[aiActionValue]
    );
}

CombatActionUseCombatItem::CombatActionUseCombatItem(CombatAction::COMBAT_ACTION aeType) : CombatAction(aeType) {
    AddPrecondition(COMBAT_WORLD_STATE_HAS_COMBAT_ITEM, (-1));
    AddEffect(COMBAT_WORLD_STATE_USED_COMBAT_ITEM, (-1));
}

void CombatActionUseCombatItem::Execute(CombatController* apCombatController, int32_t aiActionValue) {
    CombatState* pCombatState = apCombatController->pCombatState;
    TESBoundObject* pItem = pCombatState->GetUsableCombatItem(aiActionValue - 17);
    if (pItem) {
        CombatProcedureUseCombatItem* pProcedure = new CombatProcedureUseCombatItem(pItem);
        apCombatController->SetActionProcedure(pProcedure);
    }
}

void CombatActionUseCombatItem::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSsprintf(
        apDebugText,
        aiMaxLength,
        "%s: %s",
        GetCombatActionInfoName(),
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
