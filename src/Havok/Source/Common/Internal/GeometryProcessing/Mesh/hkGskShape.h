#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// The PDB has no game source path; placed beside the geometry-processing mesh adapter that uses it.
// The Havok PC PDB gives a four-byte abstract interface and this three-slot order.
struct hkGskShape {
	virtual ~hkGskShape();

	virtual void getSupportingVertex(const hkVector4& direction, hkVector4* vertexOut) const = 0;
	virtual void convertVertexIdsToVertices(uint8_t* ids, int32_t numVerts, hkVector4* verticesOut) const;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkGskShape_vftable{};
