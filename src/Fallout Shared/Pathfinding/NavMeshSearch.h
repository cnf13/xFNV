#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarNodeHeap.h"
#include "Fallout Shared/Pathfinding/AStarQueue.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NavMesh;
class NavMeshInfo;
class PathingRequest;

// A navmesh triangle and its center: what a NavMeshSearch node stands for (PDB names).
struct NavMeshSearchNode {
	// Builds the center in apCenter and returns it (the PDB returns the NiPoint3 by value: the same hidden result
	// pointer).
	NiPoint3* GetCenter(NiPoint3* apCenter) const;
	// Copies arCenter into CenterX, CenterY and CenterZ.
	void SetCenter(const NiPoint3& arCenter);
	// The PDB's const member (?GetTriangle@NavMeshSearchNode@@QBA...).
	NavMeshTriangle* GetTriangle() const;

	NavMesh* pNavMesh;
	uint16_t sTriangle;
	float CenterX;
	float CenterY;
	float CenterZ;
};

// The edge of a triangle a NavMeshSearch step crossed (PDB names).
struct NavMeshSearchLink {
	NavMesh* pNavMesh;
	uint16_t sTriangle;
	int32_t iEdgeIndex;
};

// Not named in the PDB or IDA; name chosen here (IDA calls it NavMeshSearch::GetOrCreateAStarNode). The navmesh
// search's AStarSearch::GetOrCreateAStarNode (0x6F3E30, which IDA also names so, is the teleport door search's),
// compiled into NavMeshSearch.cpp: finds the node of aiKey in kNodeMap, or allocates it from kNodeHeap; 0 when
// created, 1 when an existing node got the lower afCost (and left kSearchQueue), 2 when it kept its cost (thiscall,
// ret 0xC, 0x6A7E00; called by AddStartNode 0x6A7C90 and the NavMeshSearch GetNodeConnections overrides). A
// per-address function for a duplicated template instance.
int32_t AStarSearch_GetOrCreateAStarNode_6A7E00(AStarSearch<NavMeshSearchNode, NavMeshSearchLink>* apThis,
	uintptr_t aiKey, float afCost, AStarNode<NavMeshSearchNode, NavMeshSearchLink>** apRetNode);
// Runs the (empty) constructor of auiCount search nodes at apNodes;
// BSSimpleArray<NavMeshSearchNode>'s _InitialSetup and SetSize pass it their buffer.
void ConstructNodes(NavMeshSearchNode* apNodes, uint32_t auiCount);

// IDA's name for the node of a NavMeshSearch.
using AStarNode_NavMeshSearch = AStarNode<NavMeshSearchNode, NavMeshSearchLink>;

// The NavMeshSearch node heap (IDA's AStarNodeHeap::AllocateNode), defined in NavMeshSearch.cpp.
using AStarNodeHeap_NavMeshSearch = AStarNodeHeap<NavMeshSearchNode, NavMeshSearchLink, 512>;

using BSSimpleArray_NavMeshSearchNode_1024 = BSSimpleArray<NavMeshSearchNode, 1024>;
using BSSimpleArray_NavMeshSearchNode_1024 = BSSimpleArray<NavMeshSearchNode, 1024>;
using BSSimpleArray_NavMeshSearchLink_1024 = BSSimpleArray<NavMeshSearchLink, 1024>;

// The A* search over the triangles of the navmeshes for a pathing request: the request, the start and goal triangles,
// the navmeshes and links to avoid, the high-level (navmesh) solution to stay in, and the door lock it may cross.
class NavMeshSearch : public AStarSearch<NavMeshSearchNode, NavMeshSearchLink> {
public:
	NavMeshSearch();
	~NavMeshSearch() override;

	bool GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	// Sets pBadNodes and returns this (thiscall, ret 4, 0x6CD880); the exe's `mov eax, this`
	// is why the PDB's void return is not applied.
	NavMeshSearch* SetBadNodeArray(const BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray);
	// Builds the node path from arStart to arGoal.
	bool BuildNodePath(const PathingRequest* apRequest, const NavMeshSearchNode& aStartNode,
		const NavMeshSearchNode& aGoalNode, BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
		BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray,
		const BSSimpleArray<const NavMeshInfo*, 1024>* apHighLevelSolution);
	uint32_t GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const;
	// Not named in the PDB or IDA; name chosen here. Sets apNode's data to triangle aiTriangle of apNavMesh and its
	// heuristic to a hundredth of its center's distance to GoalNode's (thiscall, ret 0xC; called by the
	// GetNodeConnections of NavMeshSearch and NavMeshSearchMaxCost).
	void SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh, uint16_t aiTriangle);

	static void operator delete(void* apMemory);

	const PathingRequest* pCurrentRequest;
	NavMeshSearchNode StartNode;
	NavMeshSearchNode GoalNode;
	const BSSimpleArray<NavMeshSearchLink, 1024>* pBadNodes;
	const BSSimpleArray<NavMeshInfo const*, 1024>* pHighLevelSolution;
	DoorLock TempDoorLock;
};
inline constexpr fnvd::Vtable NavMeshSearch_vftable{};

// The game settings NavMeshSearch.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fPathingLargeActorRadius;
inline float fRadiusErrorCostMult = 1.0f;
