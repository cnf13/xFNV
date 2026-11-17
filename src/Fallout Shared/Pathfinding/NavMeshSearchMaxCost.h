#pragma once

#include "BSCore/BSScrapArray.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Reconstruction.hpp"

class PathingRequestOptimalLocation;

// A NavMeshSearch that gives up on nodes farther than fMaxDist and remembers whether it has seen a goal node.
class NavMeshSearchMaxCost : public NavMeshSearch {
public:
	NavMeshSearchMaxCost();
	~NavMeshSearchMaxCost() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	// The point closest to aGoalLocation the search reaches from aStartLocation within
	// afMaxDist (thiscall, ret 0x18, 0x6AB670; the result through a hidden pointer).
	NiPoint3 FindClosestPoint(const NiPoint3& aStartLocation, const FastNavMeshTriHandle& aStartNode,
		const NiPoint3& aGoalLocation, const FastNavMeshTriHandle& aGoalNode, float afMaxDist);
	void SetMaxDistance(float afMaxDist);
	// Sets up the search from aStartNode for apRequest, with aOptimalLocation's triangle as
	// the goal node when it resolves to a loaded navmesh, and runs it; true when apReturnNodeArray got a path
	// (thiscall, ret 0x14, 0x6AB4A0; Pathing::BuildOptimalLocationPath calls it). The PDB's own overload per search
	// class, as NavMeshSearchHide's: it hides NavMeshSearch::BuildNodePath, which no src caller reaches through this
	// class.
	bool BuildNodePath(const PathingRequestOptimalLocation* apRequest, const NavMeshSearchNode& aStartNode,
		PathingLocation& aOptimalLocation, BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray);

	float fMaxDist;
	bool bHasGoalNode;
};
inline constexpr fnvd::Vtable NavMeshSearchMaxCost_vftable{};
