#include "Fallout Shared/Pathfinding/AStarSearch.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarNodeHeap.h"
#include "Fallout Shared/Pathfinding/AStarNodeMap.h"
#include "Fallout Shared/Pathfinding/AStarQueue.h"
#include "Fallout Shared/Pathfinding/AStarSearch.inl"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"
#include "Fallout Shared/Pathfinding/TeleportDoorSearch.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Combat/CombatPlanner.h"

// The searches of the standalone build: their members are the generic bodies of AStarSearch.inl.
template class AStarSearch<NavMeshSearchNode, NavMeshSearchLink>;
template class AStarSearch<NavMeshPOVSearchNode, NavMeshPOVSearchLink>;
template class AStarSearch<PathSmootherPOVNode, PathSmootherPOVLink>;
template class AStarSearch<CombatPlanNode*, CombatPlanLink*>;
template class AStarSearch<const NavMeshInfo*, TESObjectREFR*>;
template class AStarSearch<ParentSpaceNode, TeleportLink>;
