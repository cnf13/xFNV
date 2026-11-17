#include "Havok/Source/Common/Visualize/Shape/hkDisplayPlane.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <xmmintrin.h>

hkDisplayPlane::hkDisplayPlane() : hkDisplayGeometry(HK_DISPLAY_PLANE) {
	m_normal.setZero4();
	m_center.setZero4();
	m_perpToNormal.setZero4();
	m_extent.setZero4();
}

hkDisplayPlane::hkDisplayPlane(const hkVector4& normal, const hkVector4& perpToNormal, const hkVector4& center,
	const hkVector4& extent)
	: hkDisplayGeometry(HK_DISPLAY_PLANE), m_normal(normal), m_center(center), m_perpToNormal(perpToNormal),
	  m_extent(extent) {}

void hkDisplayPlane::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
	// The plane's four corners, which createPlanePoints fills.
	hkVector4 points[4];
	if ((lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < 12) {
		int32_t n12 = 2 * (lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
		if (n12 <= 12) {
			n12 = 12;
		}
		hkArrayUtil::_reserve(a, &lines, n12, sizeof(hkVector4));
	}
	lines.m_size = 12;
	createPlanePoints(points);
	lines.m_data[0] = points[0];
	lines.m_data[1] = points[1];
	lines.m_data[2] = points[1];
	lines.m_data[3] = points[2];
	lines.m_data[4] = points[2];
	lines.m_data[5] = points[3];
	lines.m_data[6] = points[3];
	lines.m_data[7] = points[0];
	lines.m_data[8] = points[0];
	lines.m_data[9] = points[2];
	lines.m_data[10] = points[1];
	lines.m_data[11] = points[3];
}

void hkDisplayPlane::buildGeometry() {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkGeometry* v3 = static_cast<hkGeometry*>(Instance->m_heap->blockAlloc(sizeof(hkGeometry)));
	hkGeometry* v4 = 0;
	if (v3) {
		v3->m_vertices.m_data = 0;
		v3->m_vertices.m_size = 0;
		v3->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		v3->m_triangles.m_data = 0;
		v3->m_triangles.m_size = 0;
		v3->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
		v4 = v3;
	}
	m_geometry = v4;
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	if ((v4->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < 5) {
		int32_t v6 = 2 * (v4->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
		if (v6 <= 5) {
			v6 = 5;
		}
		hkArrayUtil::_reserve(*m_heap, v4, v6, sizeof(hkVector4));
	}
	v4->m_vertices.m_size = 5;
	m_geometry->m_vertices.m_data[4] = m_center;
	createPlanePoints(m_geometry->m_vertices.begin());
	hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* p_m_triangles = &m_geometry->m_triangles;
	hkMemoryAllocator* v8 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_size = p_m_triangles->m_size;
	int32_t v10 = m_size + 1;
	int32_t v11 = p_m_triangles->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
	if (v11 < m_size + 1) {
		int32_t v12 = 2 * v11;
		if (v10 < v12) {
			v10 = v12;
		}
		hkArrayUtil::_reserve(*v8, p_m_triangles, v10, sizeof(hkGeometry::Triangle));
	}
	++p_m_triangles->m_size;
	hkGeometry::Triangle* v13 = &p_m_triangles->m_data[m_size];
	v13->m_a = 2;
	v13->m_b = 4;
	v13->m_c = 3;
	v13->m_material = -1;
	hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v14 = &m_geometry->m_triangles;
	hkMemoryAllocator* v15 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v16 = v14->m_size;
	int32_t v17 = v16 + 1;
	int32_t v18 = v14->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
	if (v18 < v16 + 1) {
		int32_t v19 = 2 * v18;
		if (v17 < v19) {
			v17 = v19;
		}
		hkArrayUtil::_reserve(*v15, v14, v17, sizeof(hkGeometry::Triangle));
	}
	++v14->m_size;
	hkGeometry::Triangle* v20 = &v14->m_data[v16];
	v20->m_a = 0;
	v20->m_b = 4;
	v20->m_c = 2;
	v20->m_material = -1;
	hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v21 = &m_geometry->m_triangles;
	hkMemoryAllocator* v22 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v23 = v21->m_size;
	int32_t v24 = v23 + 1;
	int32_t v25 = v21->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
	if (v25 < v23 + 1) {
		int32_t v26 = 2 * v25;
		if (v24 < v26) {
			v24 = v26;
		}
		hkArrayUtil::_reserve(*v22, v21, v24, sizeof(hkGeometry::Triangle));
	}
	++v21->m_size;
	hkGeometry::Triangle* v27 = &v21->m_data[v23];
	v27->m_a = 1;
	v27->m_b = 4;
	v27->m_c = 0;
	v27->m_material = -1;
	hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v28 = &m_geometry->m_triangles;
	hkMemoryAllocator* v29 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v30 = v28->m_size;
	int32_t v31 = v30 + 1;
	int32_t v32 = v28->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
	if (v32 < v30 + 1) {
		int32_t v33 = 2 * v32;
		if (v31 < v33) {
			v31 = v33;
		}
		hkArrayUtil::_reserve(*v29, v28, v31, sizeof(hkGeometry::Triangle));
	}
	hkGeometry::Triangle* v34 = &v28->m_data[v30];
	++v28->m_size;
	v34->m_b = 4;
	v34->m_a = 3;
	v34->m_c = 1;
	v34->m_material = -1;
}

void hkDisplayPlane::setParameters(const hkVector4& normal, const hkVector4& perpToNormal, const hkVector4& center,
	const hkVector4& extent) {
	m_normal = normal;
	m_center = center;
	m_perpToNormal = perpToNormal;
	m_extent = extent;
}

hkVector4& hkDisplayPlane::getNormal() {
	return m_normal;
}

hkVector4& hkDisplayPlane::getCenter() {
	return m_center;
}

hkVector4& hkDisplayPlane::getPerpToNormal() {
	return m_perpToNormal;
}

hkVector4& hkDisplayPlane::getExtents() {
	return m_extent;
}

void hkDisplayPlane::createPlanePoints(hkVector4* points) {
	hkVector4 v2;
	hkVector4 v3;
	v2.m_quad = m_normal.m_quad;
	v3.m_quad = m_perpToNormal.m_quad;
	*points = m_center;
	points[1] = m_center;
	points[2] = m_center;
	points[3] = m_center;
	__m128 v4 = _mm_mul_ps(m_extent.m_quad,
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 210), _mm_shuffle_ps(v2.m_quad, v2.m_quad, 201)),
			_mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 201), _mm_shuffle_ps(v2.m_quad, v2.m_quad, 210))));
	__m128 v5 = _mm_mul_ps(m_extent.m_quad, m_perpToNormal.m_quad);
	points->m_quad = _mm_add_ps(_mm_add_ps(points->m_quad, v4), v5);
	points[1].m_quad = _mm_add_ps(points[1].m_quad, v4);
	points[1].m_quad = _mm_sub_ps(points[1].m_quad, v5);
	points[2].m_quad = _mm_sub_ps(points[2].m_quad, v4);
	points[2].m_quad = _mm_add_ps(points[2].m_quad, v5);
	points[3].m_quad = _mm_sub_ps(points[3].m_quad, v4);
	points[3].m_quad = _mm_sub_ps(points[3].m_quad, v5);
}
