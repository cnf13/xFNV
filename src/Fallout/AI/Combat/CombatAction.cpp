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

static uint32_t data_11A427C;

void FreeCombatActions() {
    for (uint32_t i = 0; i < 27; ++i) {
        if (CombatAction::pCombatActions[i]) {
            delete CombatAction::pCombatActions[i];
        }
    }
    for (uint32_t j = 0; j < 20; ++j) {
        if (CombatAction::pPerStateLists[j]) {
            CombatAction::pPerStateLists[j]->RemoveAll();
            if (CombatAction::pPerStateLists[j]) {
                delete CombatAction::pPerStateLists[j];
            }
        }
    }
}

char SetCombatActionCost(char* apName, float afCost) {
    for (uint32_t i = 0; i < 27; ++i) {
        if (!BSstricmp(CombatAction::kDescriptors[i].pName, apName)) {
            CombatAction::kDescriptors[i].fCost = afCost;
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
    const char* combatActionInfoName = GetCombatActionInfoName();
    BSstrcpy(apDebugText, aiMaxLength, combatActionInfoName);
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
        BSSimpleList<CombatAction*>* v2;
        BSSimpleList<CombatAction*>* v3 =
            static_cast<BSSimpleList<CombatAction*>*>(ourMalloc(sizeof(BSSimpleList<CombatAction*>)));
        if (v3) {
            v2 = ::new (v3) BSSimpleList<CombatAction*>;
        }
        else {
            v2 = nullptr;
        }
        CombatAction::pPerStateLists[aeCombatState] = v2;
    }
    CombatAction::pPerStateLists[aeCombatState]->AddTail(apAction);
}

CombatAction* CombatAction::CreateCombatAction(CombatAction::COMBAT_ACTION aeCombatAction) {
    CombatAction* result;
    switch (aeCombatAction) {
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE: {
            CombatActionAttackRangedExplosive* v26;
            CombatActionAttackRangedExplosive* v53 =
                static_cast<CombatActionAttackRangedExplosive*>(ourMalloc(sizeof(CombatActionAttackRangedExplosive)));
            if (v53) {
                v26 = ::new (v53) CombatActionAttackRangedExplosive(aeCombatAction);
            }
            else {
                v26 = nullptr;
            }
            result = v26;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE_FROM_COVER: {
            CombatActionAttackRangedExplosiveFromCover* v25;
            CombatActionAttackRangedExplosiveFromCover* v52 = static_cast<CombatActionAttackRangedExplosiveFromCover*>(
                ourMalloc(sizeof(CombatActionAttackRangedExplosiveFromCover))
            );
            if (v52) {
                v25 = ::new (v52) CombatActionAttackRangedExplosiveFromCover(aeCombatAction);
            }
            else {
                v25 = nullptr;
            }
            result = v25;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED: {
            CombatActionAttackRanged* v28;
            CombatActionAttackRanged* apMemory =
                static_cast<CombatActionAttackRanged*>(ourMalloc(sizeof(CombatActionAttackRanged)));
            if (apMemory) {
                v28 = ::new (apMemory) CombatActionAttackRanged(aeCombatAction);
            }
            else {
                v28 = nullptr;
            }
            result = v28;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_RANGED_FROM_COVER: {
            CombatActionAttackRangedFromCover* v27;
            CombatActionAttackRangedFromCover* v54 =
                static_cast<CombatActionAttackRangedFromCover*>(ourMalloc(sizeof(CombatActionAttackRangedFromCover)));
            if (v54) {
                v27 = ::new (v54) CombatActionAttackRangedFromCover(aeCombatAction);
            }
            else {
                v27 = nullptr;
            }
            result = v27;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_GRENADE: {
            CombatActionAttackGrenade* v22;
            CombatActionAttackGrenade* v49 =
                static_cast<CombatActionAttackGrenade*>(ourMalloc(sizeof(CombatActionAttackGrenade)));
            if (v49) {
                v22 = ::new (v49) CombatActionAttackGrenade(aeCombatAction);
            }
            else {
                v22 = nullptr;
            }
            result = v22;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_GRENADE_FLUSH_TARGET: {
            CombatActionAttackGrenadeFlushTarget* v21;
            CombatActionAttackGrenadeFlushTarget* v48 = static_cast<CombatActionAttackGrenadeFlushTarget*>(
                ourMalloc(sizeof(CombatActionAttackGrenadeFlushTarget))
            );
            if (v48) {
                v21 = ::new (v48) CombatActionAttackGrenadeFlushTarget(aeCombatAction);
            }
            else {
                v21 = nullptr;
            }
            result = v21;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_GRENADE_FROM_COVER: {
            CombatActionAttackGrenadeFromCover* v20;
            CombatActionAttackGrenadeFromCover* v47 =
                static_cast<CombatActionAttackGrenadeFromCover*>(ourMalloc(sizeof(CombatActionAttackGrenadeFromCover)));
            if (v47) {
                v20 = ::new (v47) CombatActionAttackGrenadeFromCover(aeCombatAction);
            }
            else {
                v20 = nullptr;
            }
            result = v20;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_MELEE: {
            CombatActionAttackMelee* v24;
            CombatActionAttackMelee* v51 =
                static_cast<CombatActionAttackMelee*>(ourMalloc(sizeof(CombatActionAttackMelee)));
            if (v51) {
                v24 = ::new (v51) CombatActionAttackMelee(aeCombatAction);
            }
            else {
                v24 = nullptr;
            }
            result = v24;
            break;
        }
        case CombatAction::COMBAT_ACTION_ATTACK_HAND_TO_HAND: {
            CombatActionAttackHandToHand* v23;
            CombatActionAttackHandToHand* v50 =
                static_cast<CombatActionAttackHandToHand*>(ourMalloc(sizeof(CombatActionAttackHandToHand)));
            if (v50) {
                v23 = ::new (v50) CombatActionAttackHandToHand(aeCombatAction);
            }
            else {
                v23 = nullptr;
            }
            result = v23;
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE: {
            CombatActionMove* v19;
            CombatActionMove* v46 = static_cast<CombatActionMove*>(ourMalloc(sizeof(CombatActionMove)));
            if (v46) {
                v19 = ::new (v46) CombatActionMove(aeCombatAction);
            }
            else {
                v19 = nullptr;
            }
            result = v19;
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_SWITCH_TO_MELEE: {
            CombatActionMoveAndSwitchToMelee* v18;
            CombatActionMoveAndSwitchToMelee* v45 =
                static_cast<CombatActionMoveAndSwitchToMelee*>(ourMalloc(sizeof(CombatActionMoveAndSwitchToMelee)));
            if (v45) {
                v18 = ::new (v45) CombatActionMoveAndSwitchToMelee(aeCombatAction);
            }
            else {
                v18 = nullptr;
            }
            result = v18;
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_ATTACK_RANGED: {
            CombatActionMoveAndAttackRanged* v17;
            CombatActionMoveAndAttackRanged* v44 =
                static_cast<CombatActionMoveAndAttackRanged*>(ourMalloc(sizeof(CombatActionMoveAndAttackRanged)));
            if (v44) {
                v17 = ::new (v44) CombatActionMoveAndAttackRanged(aeCombatAction);
            }
            else {
                v17 = nullptr;
            }
            result = v17;
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_ATTACK_RANGED_EXPLOSIVE: {
            CombatActionMoveAndAttackRangedExplosive* v16;
            CombatActionMoveAndAttackRangedExplosive* v43 = static_cast<CombatActionMoveAndAttackRangedExplosive*>(
                ourMalloc(sizeof(CombatActionMoveAndAttackRangedExplosive))
            );
            if (v43) {
                v16 = ::new (v43) CombatActionMoveAndAttackRangedExplosive(aeCombatAction);
            }
            else {
                v16 = nullptr;
            }
            result = v16;
            break;
        }
        case CombatAction::COMBAT_ACTION_MOVE_AND_ATTACK_GRENADE: {
            CombatActionMoveAndAttackGrenade* v15;
            CombatActionMoveAndAttackGrenade* v42 =
                static_cast<CombatActionMoveAndAttackGrenade*>(ourMalloc(sizeof(CombatActionMoveAndAttackGrenade)));
            if (v42) {
                v15 = ::new (v42) CombatActionMoveAndAttackGrenade(aeCombatAction);
            }
            else {
                v15 = nullptr;
            }
            result = v15;
            break;
        }
        case CombatAction::COMBAT_ACTION_DRAW_WEAPON: {
            CombatActionDrawWeapon* v14;
            CombatActionDrawWeapon* v41 =
                static_cast<CombatActionDrawWeapon*>(ourMalloc(sizeof(CombatActionDrawWeapon)));
            if (v41) {
                v14 = ::new (v41) CombatActionDrawWeapon(aeCombatAction);
            }
            else {
                v14 = nullptr;
            }
            result = v14;
            break;
        }
        case CombatAction::COMBAT_ACTION_SWITCH_WEAPON: {
            CombatActionSwitchWeapon* v13;
            CombatActionSwitchWeapon* v40 =
                static_cast<CombatActionSwitchWeapon*>(ourMalloc(sizeof(CombatActionSwitchWeapon)));
            if (v40) {
                v13 = ::new (v40) CombatActionSwitchWeapon(aeCombatAction);
            }
            else {
                v13 = nullptr;
            }
            result = v13;
            break;
        }
        case CombatAction::COMBAT_ACTION_AVOID_THREAT: {
            CombatActionAvoidThreat* v12;
            CombatActionAvoidThreat* v39 =
                static_cast<CombatActionAvoidThreat*>(ourMalloc(sizeof(CombatActionAvoidThreat)));
            if (v39) {
                v12 = ::new (v39) CombatActionAvoidThreat(aeCombatAction);
            }
            else {
                v12 = nullptr;
            }
            result = v12;
            break;
        }
        case CombatAction::COMBAT_ACTION_SEARCH: {
            CombatActionSearch* v11;
            CombatActionSearch* v38 = static_cast<CombatActionSearch*>(ourMalloc(sizeof(CombatActionSearch)));
            if (v38) {
                v11 = ::new (v38) CombatActionSearch(aeCombatAction);
            }
            else {
                v11 = nullptr;
            }
            result = v11;
            break;
        }
        case CombatAction::COMBAT_ACTION_INVESTIGATE: {
            CombatActionInvestigate* v10;
            CombatActionInvestigate* v37 =
                static_cast<CombatActionInvestigate*>(ourMalloc(sizeof(CombatActionInvestigate)));
            if (v37) {
                v10 = ::new (v37) CombatActionInvestigate(aeCombatAction);
            }
            else {
                v10 = nullptr;
            }
            result = v10;
            break;
        }
        case CombatAction::COMBAT_ACTION_DODGE: {
            CombatActionDodge* v9;
            CombatActionDodge* v36 = static_cast<CombatActionDodge*>(ourMalloc(sizeof(CombatActionDodge)));
            if (v36) {
                v9 = ::new (v36) CombatActionDodge(aeCombatAction);
            }
            else {
                v9 = nullptr;
            }
            result = v9;
            break;
        }
        case CombatAction::COMBAT_ACTION_IGNORE_BLOCKED_TARGET: {
            CombatActionIgnoreBlockedTarget* v8;
            CombatActionIgnoreBlockedTarget* v35 =
                static_cast<CombatActionIgnoreBlockedTarget*>(ourMalloc(sizeof(CombatActionIgnoreBlockedTarget)));
            if (v35) {
                v8 = ::new (v35) CombatActionIgnoreBlockedTarget(aeCombatAction);
            }
            else {
                v8 = nullptr;
            }
            result = v8;
            break;
        }
        case CombatAction::COMBAT_ACTION_FLEE: {
            CombatActionFlee* v7;
            CombatActionFlee* v34 = static_cast<CombatActionFlee*>(ourMalloc(sizeof(CombatActionFlee)));
            if (v34) {
                v7 = ::new (v34) CombatActionFlee(aeCombatAction);
            }
            else {
                v7 = nullptr;
            }
            result = v7;
            break;
        }
        case CombatAction::COMBAT_ACTION_ACTIVATE_COMBAT_ITEM: {
            CombatActionActivateCombatItem* v6;
            CombatActionActivateCombatItem* v33 =
                static_cast<CombatActionActivateCombatItem*>(ourMalloc(sizeof(CombatActionActivateCombatItem)));
            if (v33) {
                v6 = ::new (v33) CombatActionActivateCombatItem(aeCombatAction);
            }
            else {
                v6 = nullptr;
            }
            result = v6;
            break;
        }
        case CombatAction::COMBAT_ACTION_USE_COMBAT_ITEM: {
            CombatActionUseCombatItem* v5;
            CombatActionUseCombatItem* v32 =
                static_cast<CombatActionUseCombatItem*>(ourMalloc(sizeof(CombatActionUseCombatItem)));
            if (v32) {
                v5 = ::new (v32) CombatActionUseCombatItem(aeCombatAction);
            }
            else {
                v5 = nullptr;
            }
            result = v5;
            break;
        }
        case CombatAction::COMBAT_ACTION_ACQUIRE_LINE_OF_SIGHT: {
            CombatActionAcquireLineOfSight* v4;
            CombatActionAcquireLineOfSight* v31 =
                static_cast<CombatActionAcquireLineOfSight*>(ourMalloc(sizeof(CombatActionAcquireLineOfSight)));
            if (v31) {
                v4 = ::new (v31) CombatActionAcquireLineOfSight(aeCombatAction);
            }
            else {
                v4 = nullptr;
            }
            result = v4;
            break;
        }
        case CombatAction::COMBAT_ACTION_HIDE: {
            CombatActionHide* v3;
            CombatActionHide* v30 = static_cast<CombatActionHide*>(ourMalloc(sizeof(CombatActionHide)));
            if (v30) {
                v3 = ::new (v30) CombatActionHide(aeCombatAction);
            }
            else {
                v3 = nullptr;
            }
            result = v3;
            break;
        }
        case CombatAction::COMBAT_ACTION_APPROACH_TARGET: {
            CombatActionApproachTarget* v2;
            CombatActionApproachTarget* v29 =
                static_cast<CombatActionApproachTarget*>(ourMalloc(sizeof(CombatActionApproachTarget)));
            if (v29) {
                v2 = ::new (v29) CombatActionApproachTarget(aeCombatAction);
            }
            else {
                v2 = nullptr;
            }
            result = v2;
            break;
        }
        default: {
            Warning("COMBAT: Could not create combat action of type %i", aeCombatAction);
            result = nullptr;
            break;
        }
    }
    return result;
}
