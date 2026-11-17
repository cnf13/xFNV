#include "Fallout Shared/Pathfinding/AStarNodeMap.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"

// The exe's one copy of each AStarMap member, shared by every instantiation; registered on the CombatPlan one.
using AStarMap_CombatPlan = AStarMap<AStarNode<CombatPlanNode*, CombatPlanLink*>>;
