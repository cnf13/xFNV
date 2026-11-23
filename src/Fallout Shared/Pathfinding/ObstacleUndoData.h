#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <stdint.h>

// A portal edge an obstacle cut replaced: the old and the new edge, and whether the swap came from adding the obstacle.
struct PortalEdgeSwap {
    PortalEdgeSwap();

    // Copies arOther's edges and flag.
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
    uint16_t Triangles[3];
};

// What it takes to undo one obstacle's cut of the navmesh: the vertices and triangles it added, the triangles it
// overlapped and removed, and the edge swaps it made.
class ObstacleUndoData : public NiRefObject {
public:
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
