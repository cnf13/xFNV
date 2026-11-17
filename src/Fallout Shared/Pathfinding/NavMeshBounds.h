#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A navmesh's bounds: the bounding box, and the bounds' points and the triangles over them.
class NavMeshBounds {
public:
	// A triangle of the bounds as three indices into Vertices.
	struct NavMeshBoundsTriangle {
		uint16_t Vertices[3];
	};

	// An inside-out box (min at -FLT_MAX, max at FLT_MAX) and empty arrays.
	NavMeshBounds();

	// Whether aPoint lies inside the bounds.
	bool IsPointInBounds(const NiPoint3& aPoint);
	// The offset from aPoint to the bounds (thiscall, ret 8: the hidden result and aPoint,
	// 0x69C070).
	NiPoint3 DistanceFromPointToBounds(const NiPoint3& aPoint);
	// Writes the box, the point and triangle counts, then the points and triangles to apChunkBuffer, byte swapped
	// when the save swaps.
	void FillBoundsChunk(char* apChunkBuffer);
	// Copies the box, the points and the triangles into apDest.
	void CopyTo(NavMeshBounds* apDestBounds);
	// The bytes FillBoundsChunk writes.
	int32_t GetBoundsChunkSize();
	// Reads the bounds from apChunkBuffer, byte swapped when apFile swaps.
	void SetFromBoundsChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber);
	void Free();
	// Resets the box and empties both arrays (thiscall, ret 0, 0x69BE00): the PDB's Clear,
	// which SetFromBoundsChunk runs on its object first; it returns no value (eax is its last callee's).
	void Clear();

	// The exe's deleting destructor frees the bounds with ourFree.
	static void operator delete(void* apMemory);

	NiPoint3 BoundingBoxMin;
	NiPoint3 BoundingBoxMax;
	BSSimpleArray<NavMeshBoundsTriangle> Triangles;
	BSSimpleArray<NiPoint3> Vertices;
};

using BSSimpleArray_NavMeshBounds_NavMeshBoundsTriangle_1024 =
	BSSimpleArray<NavMeshBounds::NavMeshBoundsTriangle, 1024>;
