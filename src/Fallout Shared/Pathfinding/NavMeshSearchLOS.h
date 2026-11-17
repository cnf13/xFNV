#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NavMesh;
class PathingLOSMap;
class PathingRequestLOS;

// The navmesh search for the triangles that see a target: the request, the start and goal nodes, the line-of-sight
// map being filled and the best nodes found so far.
class NavMeshSearchLOS : public AStarSearch<NavMeshSearchNode, NavMeshSearchLink> {
public:
	NavMeshSearchLOS();
	~NavMeshSearchLOS() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	bool BuildNodePath(const PathingRequestLOS* apRequest, const NavMeshSearchNode& aStartNode,
		BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray);
	void SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh, uint16_t aiTriangle);
	uint32_t GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const;
	const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* GetFirstIgnoredGoal();
	const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* GetLastValidNode();
	const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* GetClosestNode();
	PathingLOSMap* GetLOSMap();

	const PathingRequestLOS* pCurrentRequest;
	NavMeshSearchNode StartNode;
	NiPointer<PathingLOSMap> spLOSMap;
	bool bHasGoalNode;
	NavMeshSearchNode GoalNode;
	int8_t cMinLOSScore;
	const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pFirstIgnoredGoal;
	const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pLastValidNode;
	const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pClosestNode;
	float fClosestNodeDistanceSquared;
};
