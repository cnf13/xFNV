#pragma once

#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Reconstruction.hpp"

// An edge location with its distance to the location being searched for; the elements the closest-edge search sorts.
// The PDB's source module is Pathfind.cpp.
struct FastNavMeshEdgeLocationProxy {
	// Builds Edge unset; the distances stay unset, as in the exe.
	FastNavMeshEdgeLocationProxy() {}

	// The PDB's compiler-generated operator=(__that) (Pathfind.cpp), out of line: Edge through
	// FastNavMeshEdgeLocation::Copy 0x6CF900, then the two floats; returns apThis (thiscall, ret 4, 0x6DBB40).
	// BSSimpleArray<FastNavMeshEdgeLocationProxy, 1024>'s code calls it. Not operator=: a user-declared one would
	// change every copy of the proxy in src.
	// Inferred type: arOther: FastNavMeshEdgeLocationProxy* from 0x6db43b (parameter 0 of
	// BSSimpleArray<FastNavMeshEdgeLocationProxy, 1024>::Add), its members used at 0x6dbb4a
	static FastNavMeshEdgeLocationProxy* Assign(FastNavMeshEdgeLocationProxy* apThis,
		const FastNavMeshEdgeLocationProxy& arOther);

	FastNavMeshEdgeLocation Edge;
	float fSqrDistToLocation;
	float fzDelta;
};
