#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"

// A link between two points of view of the path smoother: the angles the path leaves and enters at, and the edge
// locations they belong to.
class PathSmootherPOVLink {
public:
    PathSmootherPOVLink();

    // Copies the angles, then the edge locations; returns this.
    PathSmootherPOVLink& operator=(const PathSmootherPOVLink& __that);

    AngleSinCos ExitAngle;
    AngleSinCos EntryAngle;
    FastNavMeshEdgeLocation ExitLocation;
    FastNavMeshEdgeLocation EntryLocation;
};
