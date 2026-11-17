#pragma once

#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"

// An edge location with its distance to the location being searched for; the elements the closest-edge search sorts.
struct FastNavMeshEdgeLocationProxy {
	// Builds Edge unset; the distances deliberately stay unset, as the game does.
	FastNavMeshEdgeLocationProxy() {}

	// Copy assignment: copies Edge through FastNavMeshEdgeLocation::Copy, then the two floats; returns apThis.
	// BSSimpleArray<FastNavMeshEdgeLocationProxy, 1024> calls it. Not operator=: a user-declared one would change every
	// copy of the proxy.
	static FastNavMeshEdgeLocationProxy* Assign(FastNavMeshEdgeLocationProxy* apThis,
		const FastNavMeshEdgeLocationProxy& arOther);

	FastNavMeshEdgeLocation Edge;
	float fSqrDistToLocation;
	float fzDelta;
};
