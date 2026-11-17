#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NavMeshEdgeLocation : public NavMeshEdgeHandle {
public:
	// Leaves the handle and the point unset, as the exe's constructor does.
	NavMeshEdgeLocation() {}

	NavMeshEdgeLocation(const NavMeshEdgeLocation& aLocation)
		: NavMeshEdgeHandle(aLocation), Location(aLocation.Location) {}

	// The location of aFastEdgeLocation: its edge's handle by info record, then the point.
	NavMeshEdgeLocation(const FastNavMeshEdgeLocation& aFastEdgeLocation);

	// The PDB's assignment; the exe folds it into FastNavMeshEdgeLocation::Copy (0x6CF900).
	NavMeshEdgeLocation& operator=(const NavMeshEdgeLocation& aTriHandle);

	NiPoint3 Location;
};
