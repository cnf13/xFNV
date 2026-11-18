#pragma once

#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A point on an edge of a navmesh triangle.
struct FastNavMeshEdgeLocation : public FastNavMeshEdgeHandle {
    // Leaves the handle and the point unset, as the game's constructor does.
    FastNavMeshEdgeLocation() {}

    FastNavMeshEdgeLocation(const FastNavMeshEdgeHandle& aHandle, const NiPoint3& aLocation)
      : FastNavMeshEdgeHandle(aHandle), Location(aLocation) {}

    // Copies the handle, then the point; returns this.
    FastNavMeshEdgeLocation* Copy(const FastNavMeshEdgeLocation& arOther);

    NiPoint3 Location;
};

// A point on a navmesh triangle.
struct FastNavMeshTriLocation : public FastNavMeshTriHandle {
    // Constructs the point in apLocation and returns it.
    NiPoint3 GetLocation();

    NiPoint3 Location;
};
