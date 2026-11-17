#pragma once

#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

struct NavMeshObstacleVertex;
class ObstacleData;

// The kind of a cut navmesh edge.
enum EDGE_TYPE : int32_t {
	EDGE_TYPE_TWOSIDED = 0,
	EDGE_TYPE_OBSTACLE = 1,
	EDGE_TYPE_BOUNDS = 2,
	EDGE_TYPE_OPEN = 3,
	EDGE_TYPE_PORTAL = 4,
};

// An edge of the navmesh being cut by an obstacle: its two vertices, the (up to two) triangles on its sides, and the
// edge it was split from.
struct NavMeshObstacleEdge {
	NavMeshObstacleEdge(NavMeshObstacleVertex* apVert0, NavMeshObstacleVertex* apVert1, uint32_t aeEdgeType);

	bool CanBeUsed() const;
	// Records triangle ausTri on the edge's first free side, as its edge ausEdge.
	void AddTriangle(uint16_t ausTri, uint16_t ausEdge);

	NavMeshObstacleVertex* pVerts[2];
	uint16_t Triangles[2];
	uint16_t TriangleEdges[2];
	// The edge's kind (the constructor's third argument).
	EDGE_TYPE EdgeType;
	uint16_t EdgeExtraInfoIndex;
	uint16_t coverData;
	bool bLeftSide;
	bool bRightSide;
	NiPointer<ObstacleData> spObstacle;
	NavMeshEdgeHandle hOriginalEdge;
	uint16_t hOriginalEdgeExtraInfo;
	bool bFromSplit;
};
