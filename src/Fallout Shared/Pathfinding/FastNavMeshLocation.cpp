#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"

FastNavMeshEdgeLocation* FastNavMeshEdgeLocation::Copy(const FastNavMeshEdgeLocation& arOther) {
	FastNavMeshEdgeHandle::Copy(arOther);
	Location = arOther.Location;
	return this;
}

// The exe constructs the point through the out-of-line three-float constructor (0x416870).
NiPoint3* FastNavMeshTriLocation::GetLocation(NiPoint3* apLocation) {
	::new (apLocation) NiPoint3(Location.x, Location.y, Location.z);
	return apLocation;
}
