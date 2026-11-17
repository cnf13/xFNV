#pragma once

#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"
#include "Reconstruction.hpp"

class CombatPlanLink;
class CombatPlanNode;
class NavMeshInfo;
class TESObjectREFR;

// A node of an A* search over nodes of type NodeData joined by links of type Link (PDB: astarnode.h; IDA calls the
// NavMeshSearch instantiation AStarNode_NavMeshSearch). The parent, previous and next nodes are the search's
// open-list links; pNextNodeInBucket chains the node in AStarNodeMap's hash bucket.
template <class NodeData, class Link> class AStarNode {
public:
	// Zero costs, null node links and key; the node data and a pointer link stay unset, a class link is
	// default-constructed.
	AStarNode()
		: fCost(0.0f), fHeuristic(0.0f), pParentAStarNode(nullptr), ppPrevNode(nullptr), pNextNode(nullptr), iKey(0),
		  pNextNodeInBucket(nullptr) {}

	// The sum of the cost and the heuristic, returned in ST0 (thiscall, ret 0). The hybrid forwards to the exe's
	// folded copy, which the CombatPlan instance registers (0x6B9710).
	float GetFitness() { return fCost + fHeuristic; }
	// fCost, returned in ST0 (thiscall, ret 0); a trivial accessor, so no forwarder.
	float GetCost() const { return fCost; }
	// Sets fCost (thiscall, ret 4). The exe calls it out of line: an instantiation src calls gets an explicit
	// specialization forwarding to the exe's (folded) copy, declared beside its search.
	void SetCost(float afCost) { fCost = afCost; }
	// Sets fHeuristic (PDB: SetHeuristic; thiscall, ret 4). The exe calls it out of line: an instantiation src calls
	// gets an explicit specialization forwarding to the exe's (folded) copy, declared beside its search.
	void SetHeuristic(float afHeuristic) { fHeuristic = afHeuristic; }
	// fHeuristic (PDB: GetHeuristic; thiscall, ST0). The exe calls it out of line: an instantiation src calls gets an
	// explicit specialization forwarding to the exe's (folded) copy, declared beside its node data.
	float GetHeuristic() { return fHeuristic; }
	// The node's data. The exe calls it out of line: an instantiation src calls gets an explicit specialization
	// forwarding to the exe's (folded) copy, declared beside its search.
	const NodeData& GetNodeData() const { return tNodeData; }
	// The link taken from the parent node (PDB: GetParentLink). The exe calls it out of line: an instantiation src
	// calls gets an explicit specialization forwarding to the exe's (folded) copy, declared beside its search.
	const Link& GetParentLink() const { return ParentLink; }

	float fCost;
	float fHeuristic;
	NodeData tNodeData;
	// The link taken from the parent node to this one.
	Link ParentLink;
	AStarNode* pParentAStarNode;
	AStarNode** ppPrevNode;
	AStarNode* pNextNode;
	// The node's key in AStarMap. PDB: unsigned int; the POV, navmesh-info and teleport searches key nodes by an
	// address (PathSmootherPOVSearch::GetKeyForNode 0x69F900: the POV's address plus the direction), so the key is
	// pointer-sized.
	uintptr_t iKey;
	AStarNode* pNextNodeInBucket;
};

// IDA's AStarNode_NavMeshInfo_TESObjectREFR is the instantiation over NavMeshInfo const* and TESObjectREFR*.
using AStarNode_NavMeshInfo_TESObjectREFR = AStarNode<const NavMeshInfo*, TESObjectREFR*>;
