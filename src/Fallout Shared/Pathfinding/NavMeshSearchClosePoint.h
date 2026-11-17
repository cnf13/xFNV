#pragma once

#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NavMesh;
class PathingRequestClosePoint;

// The navmesh search for the closest point to an origin that is not within reach of an avoided node center: the
// request, the closest distance found so far, the origin, the best point and the avoid center.
class NavMeshSearchClosePoint : public AStarSearch<NavMeshSearchNode, NavMeshSearchLink> {
public:
	NavMeshSearchClosePoint();
	~NavMeshSearchClosePoint() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	// Sets up the search from aStartNode for apRequest and runs it; true when apReturnNodeArray got a path.
	bool BuildNodePath(const PathingRequestClosePoint* apRequest, const NavMeshSearchNode& aStartNode,
		BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray);
	uint32_t GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const;
	// Fills the node's navmesh, triangle and center and sets its heuristic to the upper bound less the center's
	// distance from AvoidNodeCenter, clamped.
	void SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh, uint16_t aiTriangle);

	static void operator delete(void* apMemory);

	const PathingRequestClosePoint* pCurrentRequest;
	NavMeshSearchNode StartNode;
	float fMinDistance;
	NiPoint3 Origin;
	NiPoint3 Result;
	NiPoint3 AvoidNodeCenter;
	float fDistanceUpperBound;
};
