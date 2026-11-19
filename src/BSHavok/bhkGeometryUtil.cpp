#include "BSHavok/bhkGeometryUtil.h"

#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"

// Builds the convex hull of apVerts: its plane equations go to arPlaneEquations and its vertices are swapped into
// arOutVerts (arOutVerts' old buffer leaves with the hull geometry).
void bhkGeometryUtil::ComputePlanesFromVertices(
    const hkArray<hkVector4, hkContainerHeapAllocator>& arVerts,
    hkArray<hkVector4, hkContainerHeapAllocator>& arPlaneEquations,
    hkArray<hkVector4, hkContainerHeapAllocator>& arOutVerts
) {
    // EXE-INLINED none: hkStridedVertices::set
    hkStridedVertices vertData;
    vertData.m_vertices = &arVerts.m_data[0](0);
    vertData.m_numVertices = arVerts.m_size;
    vertData.m_striding = sizeof(hkVector4);
    hkGeometry outGeom;
    hkGeometryUtility::createConvexGeometry(vertData, outGeom, arPlaneEquations);
    outGeom.m_vertices.swap(arOutVerts);
}
