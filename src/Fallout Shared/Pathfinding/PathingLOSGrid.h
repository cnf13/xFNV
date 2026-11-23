#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class NavMeshInfo;
class NavMeshPtr;
class PathingLocation;
class PathingLOSGridCell;
class PathingLOSMap;

// The line-of-sight grid of one navmesh: a cell per navmesh grid cell, each remembering which triangles were seen.
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
    void Setup(NavMeshInfo* apNavMeshInfo);
    // Rebuilds the cell array when the navmesh's grid cell count changed (a PathingLOSGridCell per cell), then keeps
    // the navmesh's info. The NavMeshInfo overload calls it with the info's navmesh.
    void Setup(const NavMeshPtr& aspNavMesh);
    bool SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap);
    bool SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap);
    bool IsLocationSeen(const PathingLocation& arLocation);

    // Frees with ourFree.
    static void operator delete(void* apMemory);

    // The array holds pointers to the cells, and the destructor deletes each one.
    BSSimpleArray<PathingLOSGridCell*, 1024> LOSGridCells;
    NavMeshInfo* pNavMeshInfo;
};
