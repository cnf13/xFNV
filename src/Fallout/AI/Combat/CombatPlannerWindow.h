#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"

class CombatPlan;
class CombatPlanner;
class CombatPlanNode;

// Displays a combat plan and its nodes in the planner's tree control. CombatPlanner supplies the displayed plan.
class CombatPlannerWindow : public NiTreeCtrl {
public:
    CombatPlannerWindow(
        CombatPlanner* apPlanner,
        CombatPlan* apPlan,
        BSScrapArray<CombatPlanNode*, 1024>* arCombatNodes
    );

    void Display();

    CombatPlanner* pPlanner;
    CombatPlan* pCombatPlan;
    // CombatPlanner::CreatePlan passes its node array, built by
    // BSScrapArray<CombatPlanNode *, 1024>'s constructor.
    BSScrapArray<CombatPlanNode*, 1024>* pNodeArray;
};
