#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class NavMeshInfo;
class PathingLOSMap;
class PathingLocation;

// One cell of a navmesh's line-of-sight grid: which of the cell's triangles were seen. The PDB's source module is
// PathingLOSGridMap.cpp.
class PathingLOSGridCell {
public:
	PathingLOSGridCell();
	PathingLOSGridCell(NavMeshInfo* apNavMeshInfo, uint32_t aiGridIndex);
	~PathingLOSGridCell();

	// Fills aspLOSMap's triangle scores from the seen triangles.
	bool SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap);
	// Fills the seen triangles from the triangle scores of aspLOSMap's map.
	bool SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap);
	void Setup(NavMeshInfo* apNavMeshInfo, uint32_t aiGridIndex);
	bool SetLocationSeen(const PathingLocation& arLocation, bool abSeen);
	// Inferred type: a1: BGSLoadGameBuffer* from 0x6e0155 (parameter 0 of PathingLOSGrid::LoadGame)
	uint32_t Load(BGSLoadGameBuffer* a1);
	bool IsLocationSeen(const PathingLocation& arLocation);
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);

	// The exe frees with ourFree.
	static void operator delete(void* apMemory);

	bool bAllSeen;
	// One byte per triangle (IDA: kSeenTriangles).
	BSSimpleArray<char, 1024> SeenTriangleList;
	NavMeshInfo* pNavMeshInfo;
	uint32_t iNavMeshGridCellIndex;
};
