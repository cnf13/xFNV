#include "Havok/Source/Common/Visualize/Shape/hkDisplayConvex.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <xmmintrin.h>

hkDisplayConvex::hkDisplayConvex(hkGeometry* geom) : hkDisplayGeometry(HK_DISPLAY_CONVEX) {
	m_geometry = geom;
}

void hkDisplayConvex::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
	hkGeometry* m_geometry = this->m_geometry;
	if (m_geometry) {
		// Each triangle's three edges, from the last triangle down: a-b, a-c, b-c.
		for (int32_t i = m_geometry->m_triangles.m_size - 1; i >= 0; --i) {
			const hkGeometry::Triangle& kTriangle = this->m_geometry->m_triangles.m_data[i];
			int32_t iA = kTriangle.m_a;
			int32_t iB = kTriangle.m_b;
			int32_t iC = kTriangle.m_c;
			const int32_t aiEdgeEnds[6] = {iA, iB, iA, iC, iB, iC};
			for (int32_t j = 0; j < 6; ++j) {
				const hkVector4* pVertex = &this->m_geometry->m_vertices.m_data[aiEdgeEnds[j]];
				// EXE-INLINED 0x66D9E0
				if (lines.m_size == (lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(a, &lines, sizeof(hkVector4));
				}
				lines.m_data[lines.m_size++] = *pVertex;
			}
		}
	}
}

void hkDisplayConvex::buildGeometry() {
	;
}
