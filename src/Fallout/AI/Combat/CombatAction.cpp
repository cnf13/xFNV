#include "Fallout/AI/Combat/CombatAction.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
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
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/CombatController.h"

void FreeCombatActions() {
    for (CombatAction*& pAction : CombatAction::pCombatActions) {
        if (pAction) {
            delete pAction;
        }
    }
    for (BSSimpleList<CombatAction*>*& pList : CombatAction::pPerStateLists) {
        if (pList) {
            pList->RemoveAll();
            if (pList) {
                delete pList;
            }
        }
    }
}

char SetCombatActionCost(char* apName, float afCost) {
    for (CombatAction::Descriptor& rDescriptor : CombatAction::kDescriptors) {
        if (!BSstricmp(rDescriptor.pName, apName)) {
            rDescriptor.fCost = afCost;
            return 1;
        }
    }
    return 0;
}

bool CombatAction::CanExecute(
    CombatController* apCombatController,
    CombatWorldStateArray* apWorldStateArray,
    int32_t aiActionValue
) {
    for (CombatWorldStateList* plist = &ExecutionConditions; plist; plist = plist->GetNext()) {
        CombatWorldState* pworldState = plist->m_item;
        if (pworldState) {
            int32_t ivalue = pworldState->iValue == -1 ? aiActionValue : pworldState->iValue;
            if (!apWorldStateArray->IsTrue(pworldState->eState, ivalue)) {
                return false;
            }
        }
    }
    int32_t icombatLocation = -1;
    if (GetPreconditionWorldStateActionValue(COMBAT_WORLD_STATE_AT_LOCATION, icombatLocation)) {
        apCombatController->pCombatPlannerData->SetUnk4Ptr(icombatLocation);
    }
    return true;
}

bool CombatAction::CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) {
    return true;
}

float CombatAction::GetCost(CombatController* apCombatController, int32_t aiActionValue) {
    return CombatAction::kDescriptors[eType].fCost;
}

void CombatAction::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) {
    BSstrcpy(apDebugText, aiMaxLength, GetCombatActionInfoName());
}

bool CombatAction::GetPreconditionWorldStateActionValue(COMBAT_WORLD_STATE aeState, int32_t& ariActionValue) {
    return Preconditions.GetWorldStateActionValue(aeState, ariActionValue);
}

void CombatAction::InitActions() {
    ourMemset(CombatAction::pPerStateLists, 0, sizeof(CombatAction::pPerStateLists));
    for (uint32_t i = 0; i < 27; ++i) {
        CombatAction* paction = CombatAction::CreateCombatAction(static_cast<CombatAction::COMBAT_ACTION>(i));
        pCombatActions[i] = paction;
        if (paction) {
            for (CombatWorldStateList* plist = &paction->Effects; plist && plist->m_item; plist = plist->GetNext()) {
                CombatAction::AddWorldStateAction(plist->m_item->eState, paction);
            }
        }
    }
}

void CombatAction::AddWorldStateAction(COMBAT_WORLD_STATE aeCombatState, CombatAction* apAction) {
    if (!CombatAction::pPerStateLists[aeCombatState]) {
        CombatAction::pPerStateLists[aeCombatState] = new BSSimpleList<CombatAction*>;
    }
    CombatAction::pPerStateLists[aeCombatState]->AddTail(apAction);
}

CombatAction* CombatAction::CreateCombatAction(CombatAction::COMBAT_ACTION aeCombatAction) {
    CombatAction* pAction;
    switch (aeCombatAction) {
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE: {
            pAction = new CombatActionAttackRangedExplosive(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE_FROM_COVER: {
            pAction = new CombatActionAttackRangedExplosiveFromCover(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED: {
            pAction = new CombatActionAttackRanged(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED_FROM_COVER: {
            pAction = new CombatActionAttackRangedFromCover(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_GRENADE: {
            pAction = new CombatActionAttackGrenade(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_GRENADE_FLUSH_TARGET: {
            pAction = new CombatActionAttackGrenadeFlushTarget(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_GRENADE_FROM_COVER: {
            pAction = new CombatActionAttackGrenadeFromCover(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_MELEE: {
            pAction = new CombatActionAttackMelee(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_HAND_TO_HAND: {
            pAction = new CombatActionAttackHandToHand(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE: {
            pAction = new CombatActionMove(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_SWITCH_TO_MELEE: {
            pAction = new CombatActionMoveAndSwitchToMelee(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_ATTACK_RANGED: {
            pAction = new CombatActionMoveAndAttackRanged(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_ATTACK_RANGED_EXPLOSIVE: {
            pAction = new CombatActionMoveAndAttackRangedExplosive(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_ATTACK_GRENADE: {
            pAction = new CombatActionMoveAndAttackGrenade(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_DRAW_WEAPON: {
            pAction = new CombatActionDrawWeapon(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_SWITCH_WEAPON: {
            pAction = new CombatActionSwitchWeapon(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_AVOID_THREAT: {
            pAction = new CombatActionAvoidThreat(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_SEARCH: {
            pAction = new CombatActionSearch(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_INVESTIGATE: {
            pAction = new CombatActionInvestigate(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_DODGE: {
            pAction = new CombatActionDodge(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_IGNORE_BLOCKED_TARGET: {
            pAction = new CombatActionIgnoreBlockedTarget(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_FLEE: {
            pAction = new CombatActionFlee(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ACTIVATE_COMBAT_ITEM: {
            pAction = new CombatActionActivateCombatItem(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_USE_COMBAT_ITEM: {
            pAction = new CombatActionUseCombatItem(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_ACQUIRE_LINE_OF_SIGHT: {
            pAction = new CombatActionAcquireLineOfSight(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_HIDE: {
            pAction = new CombatActionHide(aeCombatAction);
            break;
        }
        case CombatAction::COMBAT_ACTION_APPROACH_TARGET: {
            pAction = new CombatActionApproachTarget(aeCombatAction);
            break;
        }
        default: {
            Warning("COMBAT: Could not create combat action of type %i", aeCombatAction);
            pAction = nullptr;
            break;
        }
    }
    return pAction;
}
