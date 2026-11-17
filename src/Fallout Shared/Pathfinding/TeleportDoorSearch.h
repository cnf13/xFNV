#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Reconstruction.hpp"

class ActorLockData;

// The A* search over spaces and the doors between them (PDB names). The object is built and destroyed only through
// the exe's constructor and destructor.
class TeleportDoorSearch : public AStarSearch<ParentSpaceNode, TeleportLink> {
public:
	TeleportDoorSearch();
	~TeleportDoorSearch() override;

	bool GetNodeConnections(const AStarNode<ParentSpaceNode, TeleportLink>* apNode,
		BSSimpleArray<AStarNode<ParentSpaceNode, TeleportLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<ParentSpaceNode, TeleportLink>* apNode) override;

	// Searches for the door path from aStartLocation to aGoalLocation into apRetPath.
	bool BuildNodePath(const PathingLocation& aStartLocation, const PathingLocation& aGoalLocation,
		TeleportPath* apRetPath, Pathing::TELEPORT_DOOR_PATH_RESTRICTION aRestrictions, float afMaxDist,
		ActorLockData* apLockData, Pathing::DOOR_LOCK_DISPOSITION aDisposition);

	ParentSpaceNode StartNode;
	PathingLocation StartLocation;
	ParentSpaceNode GoalNode;
	Pathing::TELEPORT_DOOR_PATH_RESTRICTION Restrictions;
	float fMaxDist;
	ActorLockData* pLockData;
	// The constructor builds it as a BSSimpleArray<TESObjectREFR*, 1024>.
	BSSimpleArray<TESObjectREFR*> DoorList;
	Pathing::DOOR_LOCK_DISPOSITION DoorLockDisposition;
};
inline constexpr fnvd::Vtable TeleportDoorSearch_vftable{};
