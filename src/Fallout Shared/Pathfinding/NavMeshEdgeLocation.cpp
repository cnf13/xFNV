#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"

// The PDB's NavMeshEdgeLocation::operator=: the exe folds it into
// FastNavMeshEdgeLocation::Copy (the edge handle, then the point; thiscall, ret 4, returns this).
NavMeshEdgeLocation& NavMeshEdgeLocation::operator=(const NavMeshEdgeLocation& aTriHandle) {
	Copy(aTriHandle);
	Location = aTriHandle.Location;
	return *this;
}
