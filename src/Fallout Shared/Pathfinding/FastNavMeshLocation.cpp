#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"

#include <new>

FastNavMeshEdgeLocation* FastNavMeshEdgeLocation::Copy(const FastNavMeshEdgeLocation& arOther) {
	FastNavMeshEdgeHandle::Copy(arOther);
	Location = arOther.Location;
	return this;
}

NiPoint3* FastNavMeshTriLocation::GetLocation(NiPoint3* apLocation) {
	::new (apLocation) NiPoint3(Location.x, Location.y, Location.z);
	return apLocation;
}
