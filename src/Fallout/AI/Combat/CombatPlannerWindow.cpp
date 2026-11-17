#include "Fallout/AI/Combat/CombatPlannerWindow.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// A tree window under the main window, at the default position (CW_USEDEFAULT), 800 by 600, filled at once.
CombatPlannerWindow::CombatPlannerWindow(CombatPlanner* apPlanner, CombatPlan* apPlan,
	BSScrapArray<CombatPlanNode*, 1024>* arCombatNodes)
	: NiTreeCtrl(appMain->hInstance, appMain->hWnd, nullptr, "Combat Plan", static_cast<int32_t>(0x80000000u),
		  static_cast<int32_t>(0x80000000u), 800, 600),
	  pPlanner(apPlanner), pCombatPlan(apPlan), pNodeArray(arCombatNodes) {
	Display();
}

// The game fills the tree here with the planner's world states, the plan's actions, then the planner's nodes. There is
// no tree window (NiTreeCtrl::CreateWindows logs that), so there is nothing to fill.
void CombatPlannerWindow::Display() {}
