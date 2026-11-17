#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class NavMeshInfo;
class NavMeshPtr;
class PathingLOSGridCell;
class PathingLOSMap;
class PathingLocation;

// The line-of-sight grid of one navmesh: a cell per navmesh grid cell, each remembering which triangles were seen. The
// PDB's source module is PathingLOSGridMap.cpp.
class PathingLOSGrid {
public:
	PathingLOSGrid();
	PathingLOSGrid(NavMeshInfo* apNavMeshInfo);
	PathingLOSGrid(const NavMeshPtr& aspNavMesh);
	~PathingLOSGrid();

	// Marks arLocation as seen or not seen.
	bool SetLocationSeen(const PathingLocation& arLocation, bool abSeen);
	bool GetNavMesh(NavMeshPtr& spRetNavMesh) const;
	// Reads the grid back from a save game.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	void Setup(NavMeshInfo* a1);
	// The PDB's Setup(aspNavMesh): rebuilds the cell array when the navmesh's grid cell
	// count changed (a PathingLOSGridCell per cell), then keeps the navmesh's info (thiscall, ret 4, 0x6DFBA0; the
	// NavMeshInfo overload 0x6DFB00 calls it with the info's navmesh).
	void Setup(const NavMeshPtr& aspNavMesh);
	bool SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap);
	bool SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap);
	bool IsLocationSeen(const PathingLocation& arLocation);

	// The exe frees with ourFree.
	static void operator delete(void* apMemory);

	// The exe's array holds pointers to the cells (its destructor deletes each one).
	BSSimpleArray<PathingLOSGridCell*, 1024> LOSGridCells;
	NavMeshInfo* pNavMeshInfo;
};
