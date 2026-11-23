#include "Fallout Shared/Pathfinding/AStarNodeMap.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"

using AStarMap_CombatPlan = AStarMap<AStarNode<CombatPlanNode*, CombatPlanLink*>>;
