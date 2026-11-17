#pragma once

#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"

class CombatPlanLink;
class CombatPlanNode;
class NavMeshInfo;
class TESObjectREFR;

// A node of an A* search over nodes of type NodeData joined by links of type Link. The parent, previous and next nodes
// are the search's open-list links; pNextNodeInBucket chains the node in AStarNodeMap's hash bucket.
template <class NodeData, class Link> class AStarNode {
public:
	// Zero costs, null node links and key; the node data and a pointer link stay unset, a class link is
	// default-constructed.
	AStarNode()
		: fCost(0.0f), fHeuristic(0.0f), pParentAStarNode(nullptr), ppPrevNode(nullptr), pNextNode(nullptr), iKey(0),
		  pNextNodeInBucket(nullptr) {}

	float GetFitness() { return fCost + fHeuristic; }
	float GetCost() const { return fCost; }
	void SetCost(float afCost) { fCost = afCost; }
	void SetHeuristic(float afHeuristic) { fHeuristic = afHeuristic; }
	float GetHeuristic() { return fHeuristic; }
	const NodeData& GetNodeData() const { return tNodeData; }
	// The link taken from the parent node.
	const Link& GetParentLink() const { return ParentLink; }

	float fCost;
	float fHeuristic;
	NodeData tNodeData;
	// The link taken from the parent node to this one.
	Link ParentLink;
	AStarNode* pParentAStarNode;
	AStarNode** ppPrevNode;
	AStarNode* pNextNode;
	// The node's key in AStarMap. The POV, navmesh-info and teleport searches key nodes by an address
	// (PathSmootherPOVSearch::GetKeyForNode: the POV's address plus the direction), so the key is pointer-sized.
	uintptr_t iKey;
	AStarNode* pNextNodeInBucket;
};

using AStarNode_NavMeshInfo_TESObjectREFR = AStarNode<const NavMeshInfo*, TESObjectREFR*>;
