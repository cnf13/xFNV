#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class NavMeshEdgeLocation : public NavMeshEdgeHandle {
public:
	// Leaves the handle and the point unset, as the game does.
	NavMeshEdgeLocation() {}

	NavMeshEdgeLocation(const NavMeshEdgeLocation& aLocation)
		: NavMeshEdgeHandle(aLocation), Location(aLocation.Location) {}

	// The location of aFastEdgeLocation: its edge's handle by info record, then the point.
	NavMeshEdgeLocation(const FastNavMeshEdgeLocation& aFastEdgeLocation);

	NavMeshEdgeLocation& operator=(const NavMeshEdgeLocation& aTriHandle);

	NiPoint3 Location;
};
