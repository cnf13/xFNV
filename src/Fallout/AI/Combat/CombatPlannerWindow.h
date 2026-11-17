#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSScrapArray.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Reconstruction.hpp"

class CombatPlan;
class CombatPlanNode;
class CombatPlanner;

// Displays a combat plan and its nodes in the planner's tree control.
// The PDB has no source path; placed with CombatPlanner, which supplies the displayed plan.
class CombatPlannerWindow : public NiTreeCtrl {
public:
	CombatPlannerWindow(CombatPlanner* apPlanner, CombatPlan* apPlan,
		BSScrapArray<CombatPlanNode*, 1024>* arCombatNodes);

	void Display();

	CombatPlanner* pPlanner;
	CombatPlan* pCombatPlan;
	// CombatPlanner::CreatePlan passes its node array, built by
	// BSScrapArray<CombatPlanNode *, 1024>'s constructor.
	BSScrapArray<CombatPlanNode*, 1024>* pNodeArray;
};
