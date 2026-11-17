#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Havok's helpers for the primitives of a mesh section (index counts, triangle indices): static members only. The
// exe's calculateNumIndices is calculateNumIndices (hkMemoryMeshShape::lockSection, 0xE4A040, calls it with the
// section's
// primitive type and count).
class hkMeshPrimitiveUtil {
public:
	// The kind of primitive a hkMeshSection::PrimitiveType draws.
	enum PrimitiveStyle : int32_t {
		PRIMITIVE_STYLE_UNKNOWN = 0,
		PRIMITIVE_STYLE_POINT = 1,
		PRIMITIVE_STYLE_LINE = 2,
		PRIMITIVE_STYLE_TRIANGLE = 3,
	};

	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
	static void appendTriangleIndices(hkMeshSection::PrimitiveType primType, int32_t numVertices, int32_t indexBase,
		hkArrayBase<uint16_t>& indicesOut, hkMemoryAllocator& a);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
	static void appendTriangleIndices(hkMeshSection::PrimitiveType primType, int32_t numVertices, int32_t indexBase,
		hkArray<uint32_t, hkContainerHeapAllocator>& indicesOut);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
	static void appendTriangleIndices32(hkMeshSection::PrimitiveType primType, const uint32_t* srcIndices,
		int32_t numIndices, int32_t indexBase, hkArray<uint32_t, hkContainerHeapAllocator>& indicesOut);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
	static void appendTriangleIndices16(hkMeshSection::PrimitiveType primType, const uint16_t* srcIndices,
		int32_t numIndices, int32_t indexBase, hkArray<uint16_t, hkContainerHeapAllocator>& indicesOut);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
	static void appendTriangleIndices(const hkMeshSection& section,
		hkArray<uint16_t, hkContainerHeapAllocator>& indicesOut);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes).
	static void appendTriangleIndices(const hkMeshShape* shape, int32_t sectionIndex,
		hkArray<uint16_t, hkContainerHeapAllocator>& indicesOut);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes).
	static void appendTriangleIndices(const hkMeshSection& section,
		hkArray<uint32_t, hkContainerHeapAllocator>& indicesOut);
};
