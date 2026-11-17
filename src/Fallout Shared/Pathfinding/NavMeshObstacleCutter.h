#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/ScrapHeap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Reconstruction.hpp"

#include <stdint.h>

enum EDGE_TYPE : int32_t;
struct NavMeshObstacleEdge;
struct NavMeshObstacleVertex;
class ObstacleUndoData;
struct TaskPortalSwap;

int32_t CompareUInt(const uint32_t* a1, const uint32_t* a2);
// Not named in the PDB or IDA; name chosen here. The equality BSSimpleArray<NavMeshEdgeHandle>'s
// Find takes (cdecl): NavMeshEdgeHandle::operator== (0x6BFB90) of the two handles; the PDB has
// BSSimpleArray<NavMeshEdgeHandle,1024>::DefaultEqualFunction<NavMeshEdgeHandle> of this form.
bool EdgeHandleEqual(const NavMeshEdgeHandle* a1, const NavMeshEdgeHandle* a2);

// A navmesh vertex the obstacle cutter looks at: its index and where it lies against the obstacle and the navmesh.
struct NavMeshObstacleVertex {
	NavMeshObstacleVertex(uint16_t ausVertexIndex, bool abOnObstacle, bool abOnBounds, bool abOnOpenEdge);

	uint16_t usVertexIndex;
	bool bOnObstacle;
	bool bOnBounds;
	bool bOnOpenEdge;
	bool bInOverlappedArea;
	bool bInObstacleArea;
	uint8_t padding07;
};

// A portal edge an obstacle cut replaced (constructor 0x6BC8B0): the old and the new edge, and whether the swap came
// from adding the obstacle.
struct PortalEdgeSwap {
	PortalEdgeSwap();

	// Copies arOther's edges and flag (thiscall, ret 4, 0x6BFC40; BSSimpleArray<PortalEdgeSwap, 1024>::Add's only
	// callee on the new item).
	PortalEdgeSwap& operator=(const PortalEdgeSwap& arOther);

	NavMeshEdgeHandle hOldEdge;
	NavMeshEdgeHandle hNewEdge;
	bool bFromAddition;
};

// A triangle at the bounds of an obstacle's cut and its three edges before the cut: 12 bytes (the array's GetAt
// stride). UndoNavMeshFromObstacleUndoData restores each edge from Triangles[i], as the edge's extra info (portal)
// when bPortals[i] is set, else as its neighbour triangle.
struct BoundsTriangleInfo {
	uint16_t usTriangleIndex;
	bool bPortals[3];
	uint8_t padding05;
	uint16_t Triangles[3];
};

// What it takes to undo one obstacle's cut of the navmesh: the vertices and triangles it added, the triangles it
// overlapped and removed, and the edge swaps it made.
class ObstacleUndoData : public NiRefObject {
public:
	// Values from the Xbox PDB.
	enum OBSTACLE_UNDO_STATE : int32_t {
		OBSTACLE_UNDO_STATE_INVALID = 0x0,
		OBSTACLE_UNDO_STATE_ACTIVE = 0x1,
		OBSTACLE_UNDO_STATE_WANT_TO_UNDO = 0x2,
		OBSTACLE_UNDO_STATE_UNDONE = 0x3,
		OBSTACLE_UNDO_STATE_DELETED = 0x4,
	};

	ObstacleUndoData();
	~ObstacleUndoData() override;

	OBSTACLE_UNDO_STATE eState;
	NiPointer<ObstacleData> spObstacle;
	uint32_t iFirstAddedVertexIndex;
	uint32_t iAddedVertexCount;
	uint32_t iFirstAddedTriangleIndex;
	uint32_t iAddedTriangleCount;
	BSSimpleArray<uint16_t, 1024> OverlappedTriangles;
	BSSimpleArray<BoundsTriangleInfo, 1024> BoundsTriangles;
	NiTMap<uint16_t, NiPointer<ObstacleData>> RemovedTriangleToObstacleMap;
	BSSimpleArray<PortalEdgeSwap, 1024> EdgeSwap;
	BSSimpleArray<uint16_t, 1024> AddedPOVs;
};

// The scrap heap the A* searches and the obstacle cutter allocate from; a ScrapHeap with nothing added (its
// constructor 0x6BBC30 only runs ScrapHeap's).
struct AStarDefaultHeap : public ScrapHeap {
	AStarDefaultHeap();
	~AStarDefaultHeap();
};

class NavMeshObstacleCutter {
public:
	NavMeshObstacleCutter();
	~NavMeshObstacleCutter();

	// Cuts apObstacle out of apNavMesh; apOutPoints collects the new points.
	bool SplitNavMeshFromObstacle(NavMesh* apNavMesh, const NiPointer<ObstacleData>& aBox,
		TaskPortalSwap& aPortalSwaps);
	// Whether the edge between two vertices may be added to the running edge list.
	bool CheckPotentialEdgeAgainstRunningEdgeList(NavMeshObstacleVertex* apVert1, NavMeshObstacleVertex* apVert2);
	// Creates the edge between two vertices.
	NavMeshObstacleEdge* CreateEdge(NavMeshObstacleVertex* apVert1, NavMeshObstacleVertex* apVert2,
		EDGE_TYPE aeEdgeType);
	// Whether apPoint lies in the area the obstacle and the navmesh overlap.
	bool IsPointInOverlappedArea(const NiPoint3& aPoint);
	// Whether apVertex lies in the overlapped area.
	bool IsVertInOverlappedArea(NavMeshObstacleVertex* apVert);
	// Adds the triangles the obstacle added to the navmesh's grid.
	void AddNewTrianglesToNavMeshGrid();
	// Removes the triangles the obstacle added from the navmesh's grid.
	// ResetNavMeshFromObstacleUndoData calls it out of line.
	void RemoveTrianglesFromNavMeshGrid();
	// Whether apVertex lies inside the obstacle's area (thiscall, ret 4, 0x6BF2D0). Const as in the PDB
	// (?IsPointInObstacleArea@NavMeshObstacleCutter@@ABA_NABVNiPoint3@@@Z).
	bool IsPointInObstacleArea(const NiPoint3& aPoint) const;
	bool ResetNavMeshFromObstacleUndoData(NavMesh* apNavMesh, ObstacleUndoData* apUndoData);
	bool CheckTriangleAgainstOverlapped(NavMeshObstacleVertex* apVert1, NavMeshObstacleVertex* apVert2,
		NavMeshObstacleVertex* apVert3);
	// Whether the triangle of the three vertices lies outside the obstacle; the PDB's
	// CheckTriangleAgainstObstacle, matched by its three vertex parameters and its one caller (thiscall, ret 0xC,
	// 0x6BEC40).
	bool CheckTriangleAgainstObstacle(NavMeshObstacleVertex* apVert1, NavMeshObstacleVertex* apVert2,
		NavMeshObstacleVertex* apVert3);
	uint32_t GetTrianglesMaxObstacleCount();
	void GetModifiedTriangleList(BSSimpleArray<BoundsTriangleInfo, 1024>& aBoundsTriangle);
	void GenerateOutsideVertsAndEdges();
	void GenerateBoxVertsAndEdges();
	void SplitEdges();
	void RemoveInvalidEdges();
	void CreateNewEdges();
	void CreateRemainingEdges();
	void AddNewPOVs();
	void SetObstacleCount();
	void ConnectTriangles();
	void CreateSwappedEdgeList(TaskPortalSwap& aPortalSwaps);
	bool IsVertInObstacleArea(NavMeshObstacleVertex* apVert) const;

protected:
	void UpdateCoverArray();
	// Private in the PDB.
	bool CreateNewTriangles();

public:
	// Undoes apUndoData's cut of apNavMesh and records the portal edges it swaps back in aPortalSwaps.
	bool UndoNavMeshFromObstacleUndoData(NavMesh* apNavMesh, ObstacleUndoData* apUndoData,
		TaskPortalSwap& aPortalSwaps);
	void RemovePOVs();
	uint32_t FindVert(uint16_t a2);
	bool IsEdgeInObstacle(NavMeshObstacleEdge* apEdge);
	NavMeshObstacleVertex* CreateVertex(uint16_t aiIndex, bool abOnObstacle, bool abOnBounds, bool abOnOpenEdge);

	NavMesh* pNavMesh;
	NiPointer<ObstacleData> spObstacle;
	ObstacleUndoData* pUndoData;
	// PC ScrapHeap is 0x10 bytes, not the Xbox PDB's 0x18; following arrays move back eight bytes.
	AStarDefaultHeap LocalScrapHeap;
	BSScrapArray<NavMeshObstacleVertex*> Vertices;
	BSScrapArray<NavMeshObstacleEdge*> Edges;
	uint32_t iMaxObstacleCount;
};

inline constexpr fnvd::Vtable ObstacleUndoData_vftable{};
