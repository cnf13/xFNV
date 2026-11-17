#pragma once

#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class PathingRequestHide;

// The navmesh search for a place to hide from a threat: the hide request, the start node, the center of the area to
// avoid, how far the search may go from it and where the threat is.
class NavMeshSearchHide : public NavMeshSearch {
public:
	NavMeshSearchHide();
	~NavMeshSearchHide() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	// Sets up the search from aStartNode for apRequest and runs it; true when apReturnNodeArray got a path.
	bool BuildNodePath(const PathingRequestHide* apRequest, const NavMeshSearchNode& aStartNode,
		BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray);
	// Sets apNode's data to triangle aiTriangle of apNavMesh and its heuristic to how far inside the avoided area its
	// center lies.
	void SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh, uint16_t aiTriangle);

	const PathingRequestHide* pCurrentHideRequest;
	NavMeshSearchNode StartNode;
	NiPoint3 AvoidNodeCenter;
	float fDistanceUpperBound;
	NiPoint3 ThreatLocation;
};
