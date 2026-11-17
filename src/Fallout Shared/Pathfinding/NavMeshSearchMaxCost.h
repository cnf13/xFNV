#pragma once

#include "BSCore/BSScrapArray.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"

class PathingRequestOptimalLocation;

// A NavMeshSearch that gives up on nodes farther than fMaxDist and remembers whether it has seen a goal node.
class NavMeshSearchMaxCost : public NavMeshSearch {
public:
	NavMeshSearchMaxCost();
	~NavMeshSearchMaxCost() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	// The point closest to aGoalLocation the search reaches from aStartLocation within afMaxDist.
	NiPoint3 FindClosestPoint(const NiPoint3& aStartLocation, const FastNavMeshTriHandle& aStartNode,
		const NiPoint3& aGoalLocation, const FastNavMeshTriHandle& aGoalNode, float afMaxDist);
	void SetMaxDistance(float afMaxDist);
	// Sets up the search from aStartNode for apRequest, with aOptimalLocation's triangle as the goal node when it
	// resolves to a loaded navmesh, and runs it; true when apReturnNodeArray got a path.
	// Pathing::BuildOptimalLocationPath calls it. Each search class has its own overload, as NavMeshSearchHide does: it
	// hides NavMeshSearch::BuildNodePath.
	bool BuildNodePath(const PathingRequestOptimalLocation* apRequest, const NavMeshSearchNode& aStartNode,
		PathingLocation& aOptimalLocation, BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray);

	float fMaxDist;
	bool bHasGoalNode;
};
