#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Reconstruction.hpp"

// A link between two points of view of the path smoother: the angles the path leaves and enters at, and the edge
// locations they belong to. The PDB's source module is PathSmootherPOV.cpp.
class PathSmootherPOVLink {
public:
	PathSmootherPOVLink();

	// Copies the angles, then the edge locations; returns this.
	PathSmootherPOVLink& operator=(const PathSmootherPOVLink& __that);

	// IDA types the first as a NiPoint2 (both are 8 bytes with empty constructors); the PDB's names say angles.
	AngleSinCos ExitAngle;
	AngleSinCos EntryAngle;
	FastNavMeshEdgeLocation ExitLocation;
	FastNavMeshEdgeLocation EntryLocation;
};
