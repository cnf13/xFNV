#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NavMeshInfo;
class PathingRequest;
class PathingSolution;

class PathSmoother {
public:
	struct PathEdge;

	// Whether a path edge enters or leaves a water triangle; HandleWaterTriangles stores it.
	enum ENTER_LEAVE_WATER : int32_t {
		ELW_NONE = 0,
		ELW_ENTER = 1,
		ELW_LEAVE = 2,
	};

	// A sphere the smoothed path keeps out of.
	struct AvoidNode {
		// Whether aPoint lies inside the sphere.
		bool IsPointInNode(const NiPoint3& aPoint) const;

		NiPoint3 Center;
		float fRadius;
	};

	// The 0x40-byte vertex allocated by 0x6EDC40; its constructor initializes EdgeList at +0x2C.
	struct UniqueVert {
		UniqueVert(ScrapHeap* apHeap);
		~UniqueVert();

		// The exe's deleting destructor (0x6ED4E0) frees with ourFree.
		static void operator delete(void* apMemory);

		NiPoint3 Location;
		NiPoint3 ShrunkLocation;
		NiPoint3 PushDelta;
		NavMeshTriHandle hNavMeshTri;
		BSScrapArray<PathEdge*> EdgeList;
	};

	// The 0x1C-byte bounds edge allocated by 0x6EDFC0; its constructor initializes hNavMeshEdge at +0x8.
	struct BoundsEdge {
		BoundsEdge();

		UniqueVert* pStartVert;
		UniqueVert* pEndVert;
		NavMeshEdgeHandle hNavMeshEdge;
		bool bOpenEdge;
		uint8_t padding15[3];
		int32_t iEdgeIndex;
	};

	// The 0x58-byte path edge allocated by 0x6EE070; its constructor initializes the four points and hNavMeshEdge
	// at +0x3C.
	struct PathEdge {
		PathEdge();

		UniqueVert* pLeftVert;
		UniqueVert* pRightVert;
		NiPoint3 Center;
		NiPoint3 BestPointOnEdge;
		NiPoint3 LeftShrunkLocation;
		NiPoint3 RightShrunkLocation;
		float fTriangleNormalDelta;
		NavMeshEdgeHandle hNavMeshEdge;
		int32_t iPathEdgeIndex;
		// The bounds edges the path edge was built from (the exe stores BoundsEdge pointers here).
		BoundsEdge* pPrevLeftEdge;
		BoundsEdge* pPrevRightEdge;
		// ELW_ENTER on the edge into a water triangle, ELW_LEAVE on the edge out of one.
		ENTER_LEAVE_WATER eEnterLeaveWater;
	};

	PathSmoother();
	~PathSmoother();

	void AddPathingNode(const PathingLocation& aLocation);

	// The PDB has these private; src needs them public to register them.
	// Fills the vertex, edge and path-edge arrays from Triangles and Edges: a vertex and a bounds edge at
	// StartLocation, one per edge crossed, and a vertex and two bounds edges at GoalLocation.
	void BuildInternalArrays();
	// The vertex at StartLocation, on the first triangle of Triangles.
	void AddStartVert();
	// The vertex and the bounds and path edges between the start vertex's two edge points.
	void AddFirstEdge(const NiPoint3& aLeftLocation, const NiPoint3& aRightLocation);
	// The vertex at aLocation on the left side, from edge aiEdgeIndex of Edges.
	void AddLeftVert(const NiPoint3& aLocation, uint32_t aiEdgeIndex);
	// The vertex at aLocation on the right side, from edge aiEdgeIndex of Edges.
	void AddRightVert(const NiPoint3& aLocation, uint32_t aiEdgeIndex);
	// The vertex at GoalLocation, on the last triangle of Triangles.
	void AddGoalVert();

	// Allocates the 0x40-byte vertex in TheScrapHeap and constructs it.
	UniqueVert* CreateVert();
	// Allocates the 0x1C-byte bounds edge in TheScrapHeap and constructs it.
	BoundsEdge* CreateBoundsEdge();
	// Allocates the 0x58-byte path edge (a PathEdge) in TheScrapHeap and constructs it.
	PathSmoother::PathEdge* CreatePathEdge();
	bool SmoothPath(PathingRequest* apRequest, const BSSimpleArray<NavMeshSearchNode, 1024>& aTriSolution,
		const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks, const NiPoint3& aStartLocation,
		const NiPoint3& aGoalLocation, PathingSolution* apSolution,
		BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray, bool abAddNodesIfFail);
	// Inferred type: aLocation: NiPoint3* from 0x6ed5ce (parameter 3 of PathSmoother::SmoothPath)
	void AddPathingNode_006ED730(const NiPoint3& aLocation, NavMeshInfo* apInfo);
	bool CheckLineOfSight(uint32_t aiFirstEdgeIndex, uint32_t aiLastEdgeIndex, const NiPoint3& aLineStart,
		const NiPoint3& aLineEnd);
	void BuildShrunkEdgeLeftRightArrays();
	// Shrinks vertex aiVertIndex of the left (abLeft) or right bounds by afRadius: splits a sharp corner the
	// vertex's path edge runs between (DuplicateVert), else pushes it out along the corner's bisector. Returns the
	// index of the next vertex to shrink.
	uint32_t ComputeShrunkPoint(uint32_t aiVertIndex, float afRadius, bool abLeft);
	// The shrunk location of aPoint along aPushDelta, into aRetLocation, settled by lines of sight on aTri's navmesh
	// (to aPrevPoint and aNextPoint); 0 when aPoint is kept as it is.
	char PushPoint(NiPoint3* aPoint, NiPoint3* aRetLocation, NavMeshTriHandle aTri, NiPoint3* aPushDelta,
		NiPoint3* aPrevPoint, NiPoint3* aNextPoint);
	bool CheckBoundsEdges();
	// Whether bounds edge apEdge1 crosses apEdge2's end vertex or apEdge2 itself (abLeft: apEdge1 is a left
	// edge); where the vertex can move, splits apEdge1 and moves the new vertex there.
	bool TestBoundsEdgeAgainstOtherBound(BoundsEdge* apEdge1, BoundsEdge* apEdge2, bool abLeft);
	void SetLeftRightShrunkLocations();
	void ComputeCenters();
	void BuildBestPointOnEdge();
	void HandleWaterTriangles();
	void BuildStraightenedPath();
	void HandleAvoidNodes();
	void BuildFinalPath(bool abAddStartNode, bool abAddGoalNode);
	void ComputeTangents();
	bool IsSegmentValid(NiPoint3* a1, NiPoint3* a2);
	char SegmentClearOfEdges(NiPoint3* a1, NiPoint3* a2);
	bool PointInShrunkPath(NiPoint3* aLocation);
	PathSmoother::UniqueVert* DuplicateVert(uint32_t aiVertIndex, bool abLeft, const NiPoint3& aPreviousVertNewLocation,
		const NiPoint3& aNewVertLocation);
	PathSmoother::UniqueVert* SplitBoundsEdge(uint32_t aiEdgeIndex, bool abLeft);
	void InsertPathEdge(uint32_t aiInsertIndex, PathSmoother::PathEdge* apEdge);
	void InsertBoundsEdge(uint32_t aiInsertIndex, PathSmoother::BoundsEdge* apEdge, bool abLeft);

	// The links the smoothing rejected (SmoothPath's apBadNodeArray, which it stores here); null when the caller
	// does not collect them.
	BSSimpleArray<NavMeshSearchLink, 1024>* pBadNodes;
	PathingRequest* pRequest;
	NiPoint3 StartLocation;
	NiPoint3 GoalLocation;
	// The A* search's triangles and the edges crossed between them (SmoothPath's aTriSolution and aLinks; the
	// caller builds both as scrap arrays, which it passes as these base pointers, like BuildNodePath's arrays).
	// Const as in the PDB (types 0x2D061/0x2D062): the bodies index them through the arrays' const operator[]
	// (0x6DAD90, 0x6A7AF0) and const GetFirst/GetLast (0x6B3180, 0x6B31A0, 0x6CED70).
	const BSSimpleArray<NavMeshSearchNode, 1024>* Triangles;
	const BSSimpleArray<NavMeshSearchLink, 1024>* Edges;
	PathingSolution* pSolution;
	float fSmoothingRadius;
	// PC ScrapHeap is 0x10 bytes, not the Xbox PDB's 0x18; following arrays move back eight bytes. The constructor
	// (0x6ED100) builds it with AStarDefaultHeap's (0x6BBC30).
	AStarDefaultHeap TheScrapHeap;
	BSScrapArray<UniqueVert*> LeftVerts;
	BSScrapArray<UniqueVert*> RightVerts;
	BSScrapArray<BoundsEdge*> LeftEdges;
	BSScrapArray<BoundsEdge*> RightEdges;
	BSScrapArray<PathEdge*> PathEdges;
	BSScrapArray<PathingLocation> StraightenedPath;
};
// A link CheckBoundsEdges reports to pBadNodes, with the edge's two vertices (PDB names).
struct NavMeshSearchEdge : NavMeshSearchLink {
	NavMeshSearchEdge();

	NiPoint3 Vertices[2];
};

// The left and right vertex arrays: the exe's out-of-line constructor of the scrap array (defined in
// PathSmoother.cpp) and of the simple array it builds on, plus the empty-array setup they call.
using UniqueVertArray = BSSimpleArray<PathSmoother::UniqueVert*>;
using UniqueVertScrapArray = BSScrapArray<PathSmoother::UniqueVert*>;

using BoundsEdgeArray = BSSimpleArray<PathSmoother::BoundsEdge*>;
using PathEdgeArray = BSSimpleArray<PathSmoother::PathEdge*>;

// The game settings PathSmoother.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDrawPathBounds;
