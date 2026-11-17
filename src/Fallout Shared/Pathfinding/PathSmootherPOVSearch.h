#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NavMeshInfo;
struct NavMeshSearchNode;
class PathSmootherPOV;
class PathingRequest;
class PathingSolution;
struct Sphere;

// The A* search that smooths a navmesh path: it takes points of view (POVs) on the path's vertices and avoid nodes
// and searches for the shortest line of sight chain between them. The PDB's PathSmootherPOV.cpp. The arrays other
// than the edge vertex ones are padding until they are reconstructed.
class PathSmootherPOVSearch : public AStarSearch<PathSmootherPOVNode, PathSmootherPOVLink> {
public:
	// A path edge's vertex (constructor 0x6AEB70): the vertex, the edge, the vertex indices on the edge's two sides
	// and the POV built on it.
	struct PathEdgeVert {
		PathEdgeVert();

		// The vertex's implicit copy assignment, out of line (the PDB's operator=(__that)):
		// Vert through the handle's copy constructor 0x69A690, Edge through FastNavMeshEdgeHandle::Copy, then the
		// three indices; returns apThis (thiscall, ret 4, 0x6B39A0). BSSimpleArray<PathEdgeVert>::Add (0x6B3730)
		// calls it for the new item. Not operator=: the vertex stays trivially copyable.
		// Inferred type: arOther: PathSmootherPOVSearch::PathEdgeVert* from 0x6b376b (parameter 0 of
		// BSSimpleArray<PathSmootherPOVSearch::PathEdgeVert, 1024>::Add), its members used at 0x6b39aa
		static PathEdgeVert* Assign(PathEdgeVert* apThis, const PathEdgeVert& arOther);

		// The exe reads Vert's first dword as a NavMesh* (NavMesh::GetVertex) and copies Edge through
		// FastNavMeshEdgeHandle::Copy: the fast (NavMesh*) handles.
		FastNavMeshVertHandle Vert;
		FastNavMeshEdgeHandle Edge;
		int32_t iOtherSideIndex;
		int32_t iThisSideIndex;
		int32_t iPOVIndex;
	};

	PathSmootherPOVSearch();
	~PathSmootherPOVSearch() override;

	bool GetNodeConnections(const AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode,
		BSSimpleArray<AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>*, 1024>* apRetNodes) override;
	bool IsGoalNode(const AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode) override;

	// Adds a left (abLeft) or right point of view on vertex aiIndex of aVerts when the path turns around it there.
	void CheckAddPOVForVert(BSSimpleArray<PathSmootherPOVSearch::PathEdgeVert, 1024>& aVerts, uint32_t aiIndex,
		bool abLeft);
	// Adds a point of view for each of the request's avoid nodes, and a second, no-cross one for actor spheres.
	void AddPOVsFromAvoidNodes();
	// Adds a pathing node where the segment from aStartLoc to aGoalLoc crosses an edge between triangles whose
	// normals differ sharply.
	void AddNodeForNormalChanges(NavMesh* apStartMesh, uint16_t ausStartTri, const NiPoint3& aStartLoc,
		const NiPoint3& aGoalLoc);
	// Ends the node chain at the goal: the tangent from the last node's circle to the goal's, checked against the
	// navmesh and the avoid nodes, becomes the last link and the goal the last node. False when it cannot.
	bool CheckForLastNode();
	// Whether the arc aArc around aSphere stays on the navmesh from aEntryLocation; the edge
	// location it leaves at goes to apReturnLocation (thiscall, ret 0x20, the arc by value, 0x6B1E50).
	bool CheckArcAgainstNavMesh(const FastNavMeshEdgeLocation& aEntryLocation, Sphere& aSphere, AngleSinCosArc aArc,
		FastNavMeshEdgeLocation* apReturnLocation);
	// Whether the segment from aStartLocation to aEndLocation stays on the navmesh; the edge
	// location it ends at goes to apReturnLocation.
	bool CheckSegmentAgainstNavMesh(const FastNavMeshEdgeLocation& aStartLocation, const NiPoint3& aEndLocation,
		FastNavMeshEdgeLocation* apReturnLocation, float& afRetPercentPreferred);
	// The second check of the arc aArc around aSphere from aEntryLocation (thiscall, ret 0x1C,
	// the arc by value, 0x6B23D0).
	bool DoubleCheckArcAgainstNavMesh(const FastNavMeshEdgeLocation& aEntryLocation, Sphere& aSphere,
		AngleSinCosArc aArc);
	// CheckSegmentAgainstNavMesh for large actors (thiscall, ret 0x18, the two flags read as
	// bytes, 0x6B25F0).
	bool TripleCheckSegmentAgainstNavMesh(const FastNavMeshEdgeLocation& aStartLocation, const NiPoint3& aEndLocation,
		FastNavMeshEdgeLocation* apReturnLocation, bool abStartNode, bool abGoalNode, float& afRetPercentPreferred);
	// Adds the cost of passing the avoid nodes the segment from aStart to aGoal crosses to its length in afRetCost;
	// false when it crosses a no-cross avoid node the request does not allow.
	bool CheckSegmentAgainstAvoidNodes(PathSmootherPOV* apPrevPOV, PathSmootherPOV* apNextPOV, const NiPoint3& aStart,
		const NiPoint3& aGoal, float* afRetCost);
	// The key of apPOV's node in aDirection: the POV's address plus the direction (thiscall, ret 8, 0x69F900; PDB
	// parameters). PDB: returns unsigned int; the exe's key is a 32-bit address, so pointer-sized here.
	uintptr_t GetKeyForNode(PathSmootherPOV* apPOV, ANGLE_DIRECTION aDirection);
	bool SmoothPath(PathingRequest* apRequest, const BSSimpleArray<NavMeshSearchNode, 1024>& aTriSolution,
		const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks, const NiPoint3& aStartLocation,
		const NiPoint3& aGoalLocation, PathingSolution* apSolution,
		BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray,
		BSSimpleArray<NavMeshInfo const*, 1024>* apDetailedPathInfos);
	void BuildPOVList();
	void AddPOVsFromStaticAvoidNodes();
	void CreatePathingNodes(bool abFailed);
	bool AddIntersectingPOVsAsBadNodes(BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray);
	void AddStartPOV();
	void AddGoalPOV();
	void AddPOVsFromTrianglePath();
	void AddLeftVert(const FastNavMeshVertHandle& aVert, const FastNavMeshEdgeHandle& aEdge);
	void AddRightVert(const FastNavMeshVertHandle& aVert, const FastNavMeshEdgeHandle& aEdge);
	void AddGoalPOVsForGoalsOnNavMeshEdge();
	// Adds a node for each side of the tangents from apNode's POV to apPOV that stays on the navmesh into apRetNodes;
	// false when every side tested failed.
	bool CheckAndAddNodeConnection(const AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode,
		PathSmootherPOV* apPOV, BSSimpleArray<AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>*, 1024>* apRetNodes);
	void SetNodeData(AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode, PathSmootherPOV* apPOV,
		ANGLE_DIRECTION aDirection);

	PathingRequest* pRequest;
	PathingSolution* pSolution;
	NiPoint3 StartLocation;
	NiPoint3 GoalLocation;
	bool bUnderWaterPath;
	uint8_t padding2079[3];
	const BSSimpleArray<NavMeshSearchNode, 1024>* Triangles;
	const BSSimpleArray<NavMeshSearchLink, 1024>* Edges;
	BSSimpleArray<PathSmootherPOV*, 1024> LeftPOVs;
	BSSimpleArray<PathSmootherPOV*, 1024> RightPOVs;
	BSSimpleArray<PathSmootherPOV*, 1024> AvoidNodePOVs;
	BSSimpleArray<PathSmootherPOV*, 1024> allPOVs;
	PathSmootherPOV* pStartPov;
	PathSmootherPOV* pGoalPov;
	BSSimpleArray<PathSmootherPOVNode, 1024> Nodes;
	BSSimpleArray<PathSmootherPOVLink, 1024> Links;
	// The edge vertices on the path's left and right sides (the destructor 0x6AD690 frees both as
	// BSSimpleArray<PathSmootherPOVSearch::PathEdgeVert,1024>).
	BSSimpleArray<PathEdgeVert> LeftVerts;
	BSSimpleArray<PathEdgeVert> RightVerts;
	BSSimpleArray<const NavMeshInfo*, 1024>* pDetailedPathInfos;

	// The node the debug draw of the smoothed paths hangs under.
	static NiPointer<NiNode> spRootDrawNode;
};

using BSSimpleArray_PathSmootherPOVNode_1024 = BSSimpleArray<PathSmootherPOVNode, 1024>;
using BSSimpleArray_PathSmootherPOVSearch_PathEdgeVert_1024 = BSSimpleArray<PathSmootherPOVSearch::PathEdgeVert, 1024>;

// The PDB's AStarSearch<PathSmootherPOVNode,PathSmootherPOVLink>::AStarSearch (0x6B31C0); defined in
// PathSmootherPOV.cpp.
using AStarSearch_PathSmootherPOV = AStarSearch<PathSmootherPOVNode, PathSmootherPOVLink>;

inline constexpr fnvd::Vtable PathSmootherPOVSearch_vftable{};
