#include "BSHavok/bhkGeometryUtil.h"

#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"

// Builds the convex hull of apVerts: its plane equations go to arPlaneEquations and its vertices are swapped into
// arOutVerts (arOutVerts' old buffer leaves with the hull geometry). The exe's cleanup-only C++ EH frame is left out.
void bhkGeometryUtil::ComputePlanesFromVertices(const hkArray<hkVector4, hkContainerHeapAllocator>& arVerts,
	hkArray<hkVector4, hkContainerHeapAllocator>& arPlaneEquations,
	hkArray<hkVector4, hkContainerHeapAllocator>& arOutVerts) {
	// EXE-INLINED none: hkStridedVertices::set
	hkStridedVertices vertData;
	vertData.m_vertices = &arVerts.m_data[0](0);
	vertData.m_numVertices = arVerts.m_size;
	vertData.m_striding = sizeof(hkVector4);
	// EXE-INLINED 0xCCE5E0: hkGeometry::hkGeometry
	FNVD_INLINED_LOCAL(hkGeometry, outGeom, std::nullopt, std::nullopt);
	outGeom.m_vertices.m_data = nullptr;
	outGeom.m_vertices.m_size = 0;
	outGeom.m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	outGeom.m_triangles.m_data = nullptr;
	outGeom.m_triangles.m_size = 0;
	outGeom.m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
	hkGeometryUtility::createConvexGeometry(vertData, outGeom, arPlaneEquations);
	outGeom.m_vertices.swap(arOutVerts);
	outGeom.~hkGeometry();
}
