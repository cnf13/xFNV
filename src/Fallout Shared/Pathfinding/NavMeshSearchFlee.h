#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NavMesh;
class PathingRequestFlee;

// The navmesh search for a place to flee to: the request, the start node, the center of the area to get away from
// and how far to go from it.
class NavMeshSearchFlee : public AStarSearch<NavMeshSearchNode, NavMeshSearchLink> {
public:
	NavMeshSearchFlee();
	~NavMeshSearchFlee() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	bool BuildNodePath(const PathingRequestFlee* apRequest, const NavMeshSearchNode& aStartNode,
		BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray);
	void SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh, uint16_t aiTriangle);
	uint32_t GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri);

	static void operator delete(void* apMemory);

	const PathingRequestFlee* pCurrentRequest;
	NavMeshSearchNode StartNode;
	NiPoint3 AvoidNodeCenter;
	float fDistanceUpperBound;
	bool bFavorGoingTowardsGoal;
	NiPoint3 GoalLocation;
};
