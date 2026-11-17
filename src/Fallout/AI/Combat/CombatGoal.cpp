#include "Fallout/AI/Combat/CombatGoal.h"

#include "BSCore/MemoryManager.h"
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
                return 0;
            }
        }
        else {
            if (estate == SPECIFIED_VARIABLE_NON_ZERO) {
                if (!apWorldStateArray->GetSpecifiedVariable(ivalue)) {
                    return 0;
                }
            }
            else {
                if (!apWorldStateArray->IsTrue(estate, ivalue)) {
                    return 0;
                }
            }
        }
    }
    return 1;
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
    for (uint32_t aiGoal = 0; aiGoal < 16; ++aiGoal) {
        CombatPlan::pGoals[aiGoal] = InitCombatGoal(static_cast<CombatGoal::COMBAT_GOAL>(aiGoal));
    }
}

void CombatGoal::DeleteGoals() {
    for (uint32_t i = 0; i < 16; ++i) {
        if (CombatPlan::pGoals[i]) {
            delete CombatPlan::pGoals[i];
        }
    }
}

CombatGoal* InitCombatGoal(CombatGoal::COMBAT_GOAL aeGoal) {
    CombatGoal* result;
    switch (aeGoal) {
        case CombatGoal::COMBAT_GOAL_STAY_IN_COMBAT_AREA: {
            CombatGoalStayInCombatArea* v2;
            CombatGoalStayInCombatArea* v18 =
                static_cast<CombatGoalStayInCombatArea*>(ourMalloc(sizeof(CombatGoalStayInCombatArea)));
            if (v18) {
                v2 = ::new (v18) CombatGoalStayInCombatArea(aeGoal);
            }
            else {
                v2 = 0;
            }
            result = v2;
            break;
        }
        case CombatGoal::COMBAT_GOAL_USE_COMBAT_ITEM: {
            CombatGoalUseCombatItem* v12;
            CombatGoalUseCombatItem* v28 =
                static_cast<CombatGoalUseCombatItem*>(ourMalloc(sizeof(CombatGoalUseCombatItem)));
            if (v28) {
                v12 = ::new (v28) CombatGoalUseCombatItem(aeGoal);
            }
            else {
                v12 = 0;
            }
            result = v12;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FLEE: {
            CombatGoalFlee* v14;
            CombatGoalFlee* v30 = static_cast<CombatGoalFlee*>(ourMalloc(sizeof(CombatGoalFlee)));
            if (v30) {
                v14 = ::new (v30) CombatGoalFlee(aeGoal);
            }
            else {
                v14 = 0;
            }
            result = v14;
            break;
        }
        case CombatGoal::COMBAT_GOAL_AVOID_THREATS: {
            CombatGoalAvoidThreats* v15;
            CombatGoalAvoidThreats* v31 =
                static_cast<CombatGoalAvoidThreats*>(ourMalloc(sizeof(CombatGoalAvoidThreats)));
            if (v31) {
                v15 = ::new (v31) CombatGoalAvoidThreats(aeGoal);
            }
            else {
                v15 = 0;
            }
            result = v15;
            break;
        }
        case CombatGoal::COMBAT_GOAL_ACTIVATE_COMBAT_ITEM: {
            CombatGoalActivateCombatItem* v13;
            CombatGoalActivateCombatItem* v29 =
                static_cast<CombatGoalActivateCombatItem*>(ourMalloc(sizeof(CombatGoalActivateCombatItem)));
            if (v29) {
                v13 = ::new (v29) CombatGoalActivateCombatItem(aeGoal);
            }
            else {
                v13 = 0;
            }
            result = v13;
            break;
        }
        case CombatGoal::COMBAT_GOAL_REACH_DESTINATION: {
            CombatGoalReachDestination* v8;
            CombatGoalReachDestination* v24 =
                static_cast<CombatGoalReachDestination*>(ourMalloc(sizeof(CombatGoalReachDestination)));
            if (v24) {
                v8 = ::new (v24) CombatGoalReachDestination(aeGoal);
            }
            else {
                v8 = 0;
            }
            result = v8;
            break;
        }
        case CombatGoal::COMBAT_GOAL_GUARD: {
            CombatGoalGuard* v6;
            CombatGoalGuard* v22 = static_cast<CombatGoalGuard*>(ourMalloc(sizeof(CombatGoalGuard)));
            if (v22) {
                v6 = ::new (v22) CombatGoalGuard(aeGoal);
            }
            else {
                v6 = 0;
            }
            result = v6;
            break;
        }
        case CombatGoal::COMBAT_GOAL_CLUSTER: {
            CombatGoalCluster* v5;
            CombatGoalCluster* v21 = static_cast<CombatGoalCluster*>(ourMalloc(sizeof(CombatGoalCluster)));
            if (v21) {
                v5 = ::new (v21) CombatGoalCluster(aeGoal);
            }
            else {
                v5 = 0;
            }
            result = v5;
            break;
        }
        case CombatGoal::COMBAT_GOAL_ADVANCE: {
            CombatGoalAdvance* v7;
            CombatGoalAdvance* v23 = static_cast<CombatGoalAdvance*>(ourMalloc(sizeof(CombatGoalAdvance)));
            if (v23) {
                v7 = ::new (v23) CombatGoalAdvance(aeGoal);
            }
            else {
                v7 = 0;
            }
            result = v7;
            break;
        }
        case CombatGoal::COMBAT_GOAL_AMBUSH: {
            CombatGoalAmbush* v9;
            CombatGoalAmbush* v25 = static_cast<CombatGoalAmbush*>(ourMalloc(sizeof(CombatGoalAmbush)));
            if (v25) {
                v9 = ::new (v25) CombatGoalAmbush(aeGoal);
            }
            else {
                v9 = 0;
            }
            result = v9;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FALLBACK: {
            CombatGoalFallBack* v10;
            CombatGoalFallBack* v26 = static_cast<CombatGoalFallBack*>(ourMalloc(sizeof(CombatGoalFallBack)));
            if (v26) {
                v10 = ::new (v26) CombatGoalFallBack(aeGoal);
            }
            else {
                v10 = 0;
            }
            result = v10;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FLANK: {
            CombatGoalFlank* v11;
            CombatGoalFlank* v27 = static_cast<CombatGoalFlank*>(ourMalloc(sizeof(CombatGoalFlank)));
            if (v27) {
                v11 = ::new (v27) CombatGoalFlank(aeGoal);
            }
            else {
                v11 = 0;
            }
            result = v11;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FIND_TARGET: {
            CombatGoalFindTarget* Target;
            CombatGoalFindTarget* v32 = static_cast<CombatGoalFindTarget*>(ourMalloc(sizeof(CombatGoalFindTarget)));
            if (v32) {
                Target = ::new (v32) CombatGoalFindTarget(aeGoal);
            }
            else {
                Target = 0;
            }
            result = Target;
            break;
        }
        case CombatGoal::COMBAT_GOAL_FLUSH_TARGET: {
            CombatGoalFlushTarget* v3;
            CombatGoalFlushTarget* v19 = static_cast<CombatGoalFlushTarget*>(ourMalloc(sizeof(CombatGoalFlushTarget)));
            if (v19) {
                v3 = ::new (v19) CombatGoalFlushTarget(aeGoal);
            }
            else {
                v3 = 0;
            }
            result = v3;
            break;
        }
        case CombatGoal::COMBAT_GOAL_KILL_TARGET: {
            CombatGoalKillTarget* v17;
            CombatGoalKillTarget* apMemory =
                static_cast<CombatGoalKillTarget*>(ourMalloc(sizeof(CombatGoalKillTarget)));
            if (apMemory) {
                v17 = ::new (apMemory) CombatGoalKillTarget(aeGoal);
            }
            else {
                v17 = 0;
            }
            result = v17;
            break;
        }
        case CombatGoal::COMBAT_GOAL_HIDE_FROM_TARGET: {
            CombatGoalHideFromTarget* v4;
            CombatGoalHideFromTarget* v20 =
                static_cast<CombatGoalHideFromTarget*>(ourMalloc(sizeof(CombatGoalHideFromTarget)));
            if (v20) {
                v4 = ::new (v20) CombatGoalHideFromTarget(aeGoal);
            }
            else {
                v4 = 0;
            }
            result = v4;
            break;
        }
        default: {
            Warning("COMBAT: Could not create combat goal of type %i", aeGoal);
            result = 0;
            break;
        }
    }
    return result;
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
