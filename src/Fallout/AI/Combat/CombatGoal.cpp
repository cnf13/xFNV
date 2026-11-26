#include "Fallout/AI/Combat/CombatGoal.h"

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/AI/Combat/CombatGoalActivateCombatItem.h"
#include "Fallout/AI/Combat/CombatGoalAdvance.h"
#include "Fallout/AI/Combat/CombatGoalAmbush.h"
#include "Fallout/AI/Combat/CombatGoalAvoidThreats.h"
#include "Fallout/AI/Combat/CombatGoalCluster.h"
#include "Fallout/AI/Combat/CombatGoalFallBack.h"
#include "Fallout/AI/Combat/CombatGoalFindTarget.h"
#include "Fallout/AI/Combat/CombatGoalFlank.h"
#include "Fallout/AI/Combat/CombatGoalFlee.h"
#include "Fallout/AI/Combat/CombatGoalFlushTarget.h"
#include "Fallout/AI/Combat/CombatGoalGuard.h"
#include "Fallout/AI/Combat/CombatGoalHideFromTarget.h"
#include "Fallout/AI/Combat/CombatGoalKillTarget.h"
#include "Fallout/AI/Combat/CombatGoalReachDestination.h"
#include "Fallout/AI/Combat/CombatGoalStayInCombatArea.h"
#include "Fallout/AI/Combat/CombatGoalUseCombatItem.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/CombatController.h"

CombatGoal::CombatGoal(CombatGoal::COMBAT_GOAL aeType) : eType(aeType), iGroupStrategyAssignment(0) {}

// Frees the world states the two lists own; the lists' own destructors run after.
CombatGoal::~CombatGoal() {
    for (CombatWorldStateList* plist = &ActivationConditions; plist->m_item; plist->RemoveHead()) {
        ourFree(plist->m_item);
    }
    for (CombatWorldStateList* plist = &GoalStates; plist->m_item; plist->RemoveHead()) {
        ourFree(plist->m_item);
    }
}

bool CombatGoal::IsActive(CombatWorldStateArray* apWorldStateArray) {
    for (CombatWorldStateList* plist = &ActivationConditions; plist && plist->m_item; plist = plist->GetNext()) {
        CombatWorldState* pstate = plist->m_item;
        COMBAT_WORLD_STATE estate = pstate->eState;
        int32_t ivalue = pstate->iValue;
        if (estate == SPECIFIED_VARIABLE_ZERO) {
            if (apWorldStateArray->GetSpecifiedVariable(ivalue)) {
                return false;
            }
        }
        else if (estate == SPECIFIED_VARIABLE_NON_ZERO) {
            if (!apWorldStateArray->GetSpecifiedVariable(ivalue)) {
                return false;
            }
        }
        else if (!apWorldStateArray->IsTrue(estate, ivalue)) {
            return false;
        }
    }
    return true;
}

void CombatGoal::AddActivationCondition(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    ActivationConditions.AddWorldState(aeState, aiValue);
}

void CombatGoal::AddGoalState(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    GoalStates.AddWorldState(aeState, aiValue);
}

CombatGoal* CombatGoal::GetGoal(uint32_t aiGoal) {
    return CombatPlan::pGoals[aiGoal];
}

void CombatGoal::InitGoals() {
    for (uint32_t uiGoal = 0; uiGoal < 16; ++uiGoal) {
        CombatPlan::pGoals[uiGoal] = InitCombatGoal(static_cast<CombatGoal::COMBAT_GOAL>(uiGoal));
    }
}

void CombatGoal::DeleteGoals() {
    for (CombatGoal*& pGoal : CombatPlan::pGoals) {
        if (pGoal) {
            delete pGoal;
        }
    }
}

CombatGoal* InitCombatGoal(CombatGoal::COMBAT_GOAL aeGoal) {
    CombatGoal* pGoal;
    switch (aeGoal) {
        case CombatGoal::COMBAT_GOAL_STAY_IN_COMBAT_AREA: {
            CombatGoalStayInCombatArea* pNewGoal = new CombatGoalStayInCombatArea(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_USE_COMBAT_ITEM: {
            CombatGoalUseCombatItem* pNewGoal = new CombatGoalUseCombatItem(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FLEE: {
            CombatGoalFlee* pNewGoal = new CombatGoalFlee(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_AVOID_THREATS: {
            CombatGoalAvoidThreats* pNewGoal = new CombatGoalAvoidThreats(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_ACTIVATE_COMBAT_ITEM: {
            CombatGoalActivateCombatItem* pNewGoal = new CombatGoalActivateCombatItem(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_REACH_DESTINATION: {
            CombatGoalReachDestination* pNewGoal = new CombatGoalReachDestination(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_GUARD: {
            CombatGoalGuard* pNewGoal = new CombatGoalGuard(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_CLUSTER: {
            CombatGoalCluster* pNewGoal = new CombatGoalCluster(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_ADVANCE: {
            CombatGoalAdvance* pNewGoal = new CombatGoalAdvance(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_AMBUSH: {
            CombatGoalAmbush* pNewGoal = new CombatGoalAmbush(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FALLBACK: {
            CombatGoalFallBack* pNewGoal = new CombatGoalFallBack(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FLANK: {
            CombatGoalFlank* pNewGoal = new CombatGoalFlank(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FIND_TARGET: {
            CombatGoalFindTarget* pNewGoal = new CombatGoalFindTarget(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FLUSH_TARGET: {
            CombatGoalFlushTarget* pNewGoal = new CombatGoalFlushTarget(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_KILL_TARGET: {
            CombatGoalKillTarget* pNewGoal = new CombatGoalKillTarget(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        case CombatGoal::COMBAT_GOAL_HIDE_FROM_TARGET: {
            CombatGoalHideFromTarget* pNewGoal = new CombatGoalHideFromTarget(aeGoal);
            pGoal = pNewGoal;
            break;
        }
        default: {
            Warning("COMBAT: Could not create combat goal of type %i", aeGoal);
            pGoal = nullptr;
            break;
        }
    }
    return pGoal;
}

CombatGoalActivateCombatItem::CombatGoalActivateCombatItem(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(SPECIFIED_VARIABLE_NON_ZERO, static_cast<int32_t>(0xFFFF0016u));
    AddActivationCondition(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
    AddGoalState(COMBAT_WORLD_STATE_HAS_COMBAT_ITEM, static_cast<int32_t>(0xFFFF0016u));
}

CombatGoalAdvance::CombatGoalAdvance(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT, 4);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 6);
    iGroupStrategyAssignment = 4;
}

CombatGoalAmbush::CombatGoalAmbush(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT, 3);
    AddGoalState(COMBAT_WORLD_STATE_HIDING, 15);
    iGroupStrategyAssignment = 3;
}

CombatGoalAvoidThreats::CombatGoalAvoidThreats(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_THREATENED, 1);
    AddActivationCondition(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
    AddGoalState(COMBAT_WORLD_STATE_THREATENED, 0);
}

CombatGoalCluster::CombatGoalCluster(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_NOT_AT_LOCATION, 9);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 9);
}

CombatGoalFallBack::CombatGoalFallBack(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT, 2);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 5);
    iGroupStrategyAssignment = 2;
}

CombatGoalFindTarget::CombatGoalFindTarget(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_EXECUTION_FLAGS, 16);
    AddActivationCondition(COMBAT_WORLD_STATE_TARGET_LOST, 1);
    AddGoalState(COMBAT_WORLD_STATE_TARGET_LOST, 0);
}

CombatGoalFlank::CombatGoalFlank(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT, 1);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 4);
    iGroupStrategyAssignment = 1;
}

CombatGoalFlee::CombatGoalFlee(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 1);
    AddActivationCondition(COMBAT_WORLD_STATE_AT_LOCATION, 8);
    AddActivationCondition(COMBAT_WORLD_STATE_AT_LOCATION, 7);
    AddGoalState(COMBAT_WORLD_STATE_WANTS_TO_FLEE, 0);
}

CombatGoalFlushTarget::CombatGoalFlushTarget(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddActivationCondition(COMBAT_WORLD_STATE_TARGET_FLUSHED, 0);
    AddGoalState(COMBAT_WORLD_STATE_TARGET_FLUSHED, 1);
}

CombatGoalGuard::CombatGoalGuard(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_NOT_AT_LOCATION, 8);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 8);
}

CombatGoalHideFromTarget::CombatGoalHideFromTarget(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddGoalState(COMBAT_WORLD_STATE_HIDING, 60);
}

CombatGoalKillTarget::CombatGoalKillTarget(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_TARGET_LOST, 0);
    AddGoalState(COMBAT_WORLD_STATE_TARGET_DEAD, 1);
}

CombatGoalReachDestination::CombatGoalReachDestination(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_NOT_AT_LOCATION, 7);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 7);
}

CombatGoalStayInCombatArea::CombatGoalStayInCombatArea(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(COMBAT_WORLD_STATE_NOT_AT_LOCATION, 16);
    AddGoalState(COMBAT_WORLD_STATE_AT_LOCATION, 16);
}

CombatGoalUseCombatItem::CombatGoalUseCombatItem(CombatGoal::COMBAT_GOAL aeType) : CombatGoal(aeType) {
    AddActivationCondition(SPECIFIED_VARIABLE_NON_ZERO, -0xFFE9);
    AddGoalState(COMBAT_WORLD_STATE_USED_COMBAT_ITEM, -0xFFE9);
}
