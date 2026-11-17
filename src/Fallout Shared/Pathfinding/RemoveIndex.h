#pragma once

#include "Reconstruction.hpp"

// A navmesh point of view the POV search removes: its master and minor indices. The PDB's source module is
// NavMeshPOVSearch.cpp; 8 bytes (the array's 8-byte stride in _ConstructItems 0x6A23A0).
struct RemoveIndex {
	RemoveIndex();
	RemoveIndex(int32_t aiMasterIndex, int32_t aiMinorIndex);

	// Both indices equal aIndex's (thiscall, ret 4, 0x6A1B20). The array's default equality
	// function 0x6A1B00 (BSSimpleArray<RemoveIndex,1024>::DefaultEqualFunction<RemoveIndex>, the Find callback of
	// NavMeshPOVSearch::BuildPOVListForTriangles) calls it.
	bool operator==(const RemoveIndex& aIndex) const;

	int32_t iMasterIndex;
	int32_t iMinorIndex;
};
