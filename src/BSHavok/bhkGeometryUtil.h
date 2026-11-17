#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

// The game's geometry helpers: they turn vertex and triangle lists into the shapes and creation infos Havok takes.
// Only the entries the reconstructed code calls are declared. The PDB puts them in BSHavok\bhkGeometryUtil.cpp.
class bhkGeometryUtil {
public:
	// Writes the plane equations of the hull of apVerts to arPlaneEquations and the hull's
	// rounded vertices to arOutVerts.
	static void ComputePlanesFromVertices(const hkArray<hkVector4, hkContainerHeapAllocator>& arVerts,
		hkArray<hkVector4, hkContainerHeapAllocator>& arPlaneEquations,
		hkArray<hkVector4, hkContainerHeapAllocator>& arOutVerts);
};
