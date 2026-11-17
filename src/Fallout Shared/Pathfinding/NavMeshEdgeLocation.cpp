#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"

NavMeshEdgeLocation& NavMeshEdgeLocation::operator=(const NavMeshEdgeLocation& aTriHandle) {
	Copy(aTriHandle);
	Location = aTriHandle.Location;
	return *this;
}
