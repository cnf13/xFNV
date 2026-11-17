#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"

int32_t CompareUShort(const uint16_t* a1, const uint16_t* a2);

// A navmesh vertex of a point-of-view search: the vertex and the edges and portal edges that meet at it.
struct POVVertexData {
	POVVertexData();
	~POVVertexData();

	// The deleting destructor frees with ourFree.
	static void operator delete(void* apMemory);

	FastNavMeshVertHandle hVertex;
	BSSimpleArray<FastNavMeshEdgeHandle, 1024> Edges;
	BSSimpleArray<FastNavMeshEdgeHandle, 1024> PortalEdges;
};
