#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// The game's geometry helpers: they turn vertex and triangle lists into the shapes and creation infos Havok takes. Only
// some of the entries are declared.
class bhkGeometryUtil {
public:
	// Writes the plane equations of the hull of apVerts to arPlaneEquations and the hull's
	// rounded vertices to arOutVerts.
	static void ComputePlanesFromVertices(const hkArray<hkVector4, hkContainerHeapAllocator>& arVerts,
		hkArray<hkVector4, hkContainerHeapAllocator>& arPlaneEquations,
		hkArray<hkVector4, hkContainerHeapAllocator>& arOutVerts);
};
