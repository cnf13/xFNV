#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/ObstacleUndoData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

struct NavMeshObstacleEdge;
struct NavMeshObstacleVertex;
struct TaskPortalSwap;

enum EDGE_TYPE : int32_t;

int32_t CompareUInt(const uint32_t* apOne, const uint32_t* apTwo);
// The equality function BSSimpleArray<NavMeshEdgeHandle>'s Find takes: NavMeshEdgeHandle::operator== of the two
// handles.
bool EdgeHandleEqual(const NavMeshEdgeHandle* apOne, const NavMeshEdgeHandle* apTwo);

// A navmesh vertex the obstacle cutter looks at: its index and where it lies against the obstacle and the navmesh.
struct NavMeshObstacleVertex {
    NavMeshObstacleVertex(uint16_t ausVertexIndex, bool abOnObstacle, bool abOnBounds, bool abOnOpenEdge);

    uint16_t usVertexIndex;
    bool bOnObstacle;
    bool bOnBounds;
    bool bOnOpenEdge;
    bool bInOverlappedArea;
    bool bInObstacleArea;
};

// The scrap heap the A* searches and the obstacle cutter allocate from; a ScrapHeap with nothing added (its constructor
// only runs ScrapHeap's).
struct AStarDefaultHeap : public ScrapHeap {
    AStarDefaultHeap();
    ~AStarDefaultHeap();
};

class NavMeshObstacleCutter {
public:
    NavMeshObstacleCutter();
    ~NavMeshObstacleCutter();

    // Cuts apObstacle out of apNavMesh; apOutPoints collects the new points.
    bool
    SplitNavMeshFromObstacle(NavMesh* apNavMesh, const NiPointer<ObstacleData>& aBox, TaskPortalSwap& aPortalSwaps);
    // Whether the edge between two vertices may be added to the running edge list.
    bool CheckPotentialEdgeAgainstRunningEdgeList(NavMeshObstacleVertex* apVert1, NavMeshObstacleVertex* apVert2);
    // Creates the edge between two vertices.
    NavMeshObstacleEdge*
    CreateEdge(NavMeshObstacleVertex* apVert1, NavMeshObstacleVertex* apVert2, EDGE_TYPE aeEdgeType);
    // Whether apPoint lies in the area the obstacle and the navmesh overlap.
    bool IsPointInOverlappedArea(const NiPoint3& aPoint);
    // Whether apVertex lies in the overlapped area.
    bool IsVertInOverlappedArea(NavMeshObstacleVertex* apVert);
    // Adds the triangles the obstacle added to the navmesh's grid.
    void AddNewTrianglesToNavMeshGrid();
    // Removes the triangles the obstacle added from the navmesh's grid.
    void RemoveTrianglesFromNavMeshGrid();
    bool IsPointInObstacleArea(const NiPoint3& aPoint) const;
    bool ResetNavMeshFromObstacleUndoData(NavMesh* apNavMesh, ObstacleUndoData* apUndoData);
    bool CheckTriangleAgainstOverlapped(
        NavMeshObstacleVertex* apVert1,
        NavMeshObstacleVertex* apVert2,
        NavMeshObstacleVertex* apVert3
    );
    // Whether the triangle of the three vertices lies outside the obstacle.
    bool CheckTriangleAgainstObstacle(
        NavMeshObstacleVertex* apVert1,
        NavMeshObstacleVertex* apVert2,
        NavMeshObstacleVertex* apVert3
    );
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
    bool CreateNewTriangles();

public:
    // Undoes apUndoData's cut of apNavMesh and records the portal edges it swaps back in aPortalSwaps.
    bool
    UndoNavMeshFromObstacleUndoData(NavMesh* apNavMesh, ObstacleUndoData* apUndoData, TaskPortalSwap& aPortalSwaps);
    void RemovePOVs();
    uint32_t FindVert(uint16_t ausVertexIndex);
    bool IsEdgeInObstacle(NavMeshObstacleEdge* apEdge);
    NavMeshObstacleVertex* CreateVertex(uint16_t aiIndex, bool abOnObstacle, bool abOnBounds, bool abOnOpenEdge);

    NavMesh* pNavMesh;
    NiPointer<ObstacleData> spObstacle;
    ObstacleUndoData* pUndoData;
    AStarDefaultHeap LocalScrapHeap;
    BSScrapArray<NavMeshObstacleVertex*> Vertices;
    BSScrapArray<NavMeshObstacleEdge*> Edges;
    uint32_t iMaxObstacleCount;
};
