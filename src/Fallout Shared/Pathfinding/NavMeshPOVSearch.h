#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

struct NavMeshPOV;
class PathingRequest;
class PathingSolution;
struct Sphere;

// A node of the navmesh POV search: a point of view, the direction the path goes around it and its index in the
// search's POV list.
class NavMeshPOVSearchNode {
public:
	NavMeshPOV* pPOV;
	ANGLE_DIRECTION Direction;
	uint32_t iMasterIndex;
};

// A link of the search graph: the angle at which the path leaves the parent node's POV and the angle at which it enters
// the next one, each with the edge location it belongs to. Its constructors and its copy assignment are
// compiler-generated, so it is an aggregate here.
class NavMeshPOVSearchLink {
public:
	AngleSinCos ExitAngle;
	AngleSinCos EntryAngle;
	FastNavMeshEdgeLocation ExitLocation;
	FastNavMeshEdgeLocation EntryLocation;
};

// The A* search over a navmesh's points of view (POVs): it finds the chain of POVs between a start and a goal
// location.
class NavMeshPOVSearch : public AStarSearch<NavMeshPOVSearchNode, NavMeshPOVSearchLink> {
public:
	NavMeshPOVSearch();
	~NavMeshPOVSearch() override;

	// The nodes reachable from apNode: for each POV of the search's arrays from apNode's master index on, each
	// tangent from apNode's POV circle to it that the navmesh and the other POVs allow, added to apRetNodes.
	bool GetNodeConnections(const AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* apNode,
		BSSimpleArray<AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* apNode) override;

	// Follows the segment from aStartLocation to aEndLocation across the navmesh triangles and their portals; true
	// with apReturnLocation at the end point when it is reached, else false with the edge where the walk stopped.
	bool CheckSegmentAgainstNavMesh(const FastNavMeshEdgeLocation& aStartLocation, const NiPoint3& aEndLocation,
		FastNavMeshEdgeLocation* apReturnLocation);
	// Checks aArc around apThisPOV against the search's other POVs, skipping apIgnorePOV. The returned values include
	// the constants 1.0, 2.0 and 100.0.
	float CheckArcAgainstPOVs(NavMeshPOV* apThisPOV, AngleSinCosArc aArc, NavMeshPOV* apIgnorePOV);
	// Walks aArc around aSphere from aEntryLocation in up to 24 steps of 15 degrees
	// (0.2617994), each through CheckSegmentAgainstNavMesh; true with apReturnLocation at the arc's end when it
	// stays on the navmesh.
	bool CheckArcAgainstNavMesh2(const FastNavMeshEdgeLocation& aEntryLocation, Sphere& aSphere, AngleSinCosArc aArc,
		FastNavMeshEdgeLocation* apReturnLocation);
	// Sets apNode's POV, direction and master index, and its heuristic to 0.75 of the POV's distance to the goal POV.
	void SetNodeData(AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* apNode, NavMeshPOV* apPOV,
		ANGLE_DIRECTION aDirection, int32_t aiMasterIndex);
	// Checks the segment from aStart to aGoal against the search's other POVs, skipping apThisPOV, apIgnorePOV and
	// apThisPOV's duplicates. Returns 1.0, 2.0 or 100.0.
	float CheckSegmentAgainstPOVs(NavMeshPOV* apThisPOV, const NiPoint3& aStart, const NiPoint3& aGoal,
		NavMeshPOV* apIgnorePOV);
	// The key of apPOV's node in aDirection. The key is the POV's address plus the direction, so it is pointer-sized.
	uintptr_t GetKeyForNode(NavMeshPOV* apPOV, ANGLE_DIRECTION aDirection);
	bool BuildPOVListForTriangles(const BSSimpleArray<FastNavMeshTriHandle, 1024>& apList,
		BSSimpleArray<BSSimpleArray<NavMeshPOV*, 1024>, 1024>& aRetPOVs);
	bool BuildPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
		const BSSimpleArray<FastNavMeshTriHandle, 1024>* apList, PathingRequest* apRequest,
		PathingSolution* apSolution);

	static void operator delete(void* apMemory);
	static void DrawLine(const NiPoint3& aStart, const NiPoint3& aGoal, const NiColorA& aColor);
	static void DrawArc(const NiPoint3& aCenter, float afRadius, const AngleSinCos& aStartAngle,
		const AngleSinCos& aGoalAngle, ANGLE_DIRECTION aDirection, const NiColorA& aColor);

	BSSimpleArray<NavMeshPOV*, 1024> allPOVs;
	BSSimpleArray<BSSimpleArray<NavMeshPOV*, 1024>, 1024> POVs;
	NavMeshPOV* pStartPov;
	NavMeshPOV* pGoalPov;
	PathingRequest* pRequest;
	PathingSolution* pSolution;
	PathingLocation StartLocation;
	PathingLocation GoalLocation;
	BSSimpleArray<NavMeshPOVSearchNode, 1024> Nodes;
	BSSimpleArray<NavMeshPOVSearchLink, 1024> Links;

	static inline bool bDrawDebugData;
	static NiPointer<NiNode> spRootDrawNode;
};

// The node arrays' destructor, constructor and initial setup are not declared.
