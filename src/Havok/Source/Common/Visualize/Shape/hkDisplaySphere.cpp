#include "Havok/Source/Common/Visualize/Shape/hkDisplaySphere.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <bit>
#include <xmmintrin.h>

// Not named in the PDB or IDA; name chosen here. a1 is the geometry whose triangles it
// subdivides: it reads its vertex count (+0x4) and triangle count (+0x10); hkDisplaySphere::buildGeometry (0xE0B360,
// its only caller) passes its m_geometry.
// Inferred type: a1: hkGeometry* from 0xe0b832 (hkDisplayGeometry::m_geometry (this of
// hkDisplaySphere::buildGeometry)), its members used at 0xe0b054
static void _subdivideRecursive(hkGeometry* a1, int32_t arg4, float a3, float a4);

static void _subdivideRecursive(hkGeometry* a1, int32_t arg4, float a3, float a4) {
	if (0.0f != a4) {
		__m128 v58 = _mm_undefined_ps();
		v58[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
		int32_t v4 = arg4;
		__m128 v5 = _mm_setzero_ps();
		v5[0] = 0.5f;
		while (1) {
			__m128 v15 = _mm_undefined_ps();
			int32_t m_size = a1->m_vertices.m_size;
			int32_t v56 = a1->m_triangles.m_size;
			hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
			int32_t v7 = a1->m_vertices.m_size + 3;
			int32_t v8 = a1->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
			if (v8 < v7) {
				int32_t v9 = 2 * v8;
				if (v7 < v9) {
					v7 = v9;
				}
				hkArrayUtil::_reserve(*m_heap, a1, v7, sizeof(hkVector4));
			}
			a1->m_vertices.m_size += 3;
			hkMemoryAllocator* v10 = (&hkMemoryRouter::getInstance())->m_heap;
			int32_t v11 = a1->m_triangles.m_size + 3;
			int32_t v12 = a1->m_triangles.m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
			if (v12 < v11) {
				int32_t v13 = 2 * v12;
				if (v11 < v13) {
					v11 = v13;
				}
				hkArrayUtil::_reserve(*v10, &a1->m_triangles, v11, sizeof(hkGeometry::Triangle));
			}
			a1->m_triangles.m_size += 3;
			hkGeometry::Triangle* v14 = &a1->m_triangles.m_data[v4];
			hkVector4* v53 = &a1->m_vertices.m_data[v14->m_b];
			v15[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
			int32_t m_a = v14->m_a;
			__m128 m_quad = a1->m_vertices.m_data[m_a].m_quad;
			hkVector4* v18 = &a1->m_vertices.m_data[m_a];
			hkVector4* v19 = &a1->m_vertices.m_data[m_size];
			hkVector4* v20 = &a1->m_vertices.m_data[m_size + 2];
			__m128 v21 = _mm_add_ps(v53->m_quad, m_quad);
			hkVector4* v22 = &a1->m_vertices.m_data[v14->m_c];
			__m128 v23 = _mm_mul_ps(v21, v21);
			float v24 = _mm_shuffle_ps(v23, v23, 85)[0] + v23[0];
			__m128 v25 = _mm_shuffle_ps(v23, v23, 170);
			v25[0] = v25[0] + v24;
			float v26 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v25[0])));
			__m128 v27 = _mm_cmpeq_ss(v25, v58);
			float v28 = 3.0f - static_cast<float>(static_cast<float>(v25[0] * v26) * v26);
			v5[0] = static_cast<float>(v5[0] * v26) * v28;
			__m128 v30 = _mm_andnot_ps(v27, v5);
			__m128 v31 = _mm_mul_ps(_mm_shuffle_ps(v30, v30, 0), v21);
			(*v19).m_quad = v21;
			(*v19).m_quad = v31;
			v21[0] = a3;
			v19->m_quad = _mm_mul_ps(_mm_shuffle_ps(v21, v21, 0), v31);
			__m128 v32 = _mm_add_ps(v22->m_quad, v53->m_quad);
			__m128 v33 = _mm_mul_ps(v32, v32);
			v30[0] = _mm_shuffle_ps(v33, v33, 85)[0] + v33[0];
			__m128 v34 = _mm_shuffle_ps(v33, v33, 170);
			v34[0] = v34[0] + v30[0];
			float v35 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v34[0])));
			v19[1].m_quad = v32;
			__m128 v36 = _mm_cmpeq_ss(v34, v15);
			float v37 = 3.0f - static_cast<float>(static_cast<float>(v34[0] * v35) * v35);
			v5[0] = static_cast<float>(v5[0] * v35) * v37;
			__m128 v39 = _mm_andnot_ps(v36, v5);
			float v54 = a4 - 1.0;
			__m128 v40 = _mm_mul_ps(_mm_shuffle_ps(v39, v39, 0), v32);
			v19[1].m_quad = v40;
			v32[0] = a3;
			v19[1].m_quad = _mm_mul_ps(_mm_shuffle_ps(v32, v32, 0), v40);
			__m128 v41 = _mm_add_ps(v22->m_quad, v18->m_quad);
			(*v20).m_quad = v41;
			__m128 v42 = _mm_mul_ps(v41, v41);
			v39[0] = _mm_shuffle_ps(v42, v42, 85)[0] + v42[0];
			__m128 v43 = _mm_shuffle_ps(v42, v42, 170);
			v43[0] = v43[0] + v39[0];
			float v44 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v43[0])));
			__m128 v45 = v43;
			float v46 = 3.0f - static_cast<float>(static_cast<float>(v43[0] * v44) * v44);
			v5[0] = static_cast<float>(v5[0] * v44) * v46;
			__m128 v48 = _mm_andnot_ps(_mm_cmpeq_ss(v45, v15), v5);
			__m128 v49 = _mm_mul_ps(_mm_shuffle_ps(v48, v48, 0), v41);
			(*v20).m_quad = v49;
			v41[0] = a3;
			v20->m_quad = _mm_mul_ps(_mm_shuffle_ps(v41, v41, 0), v49);
			hkGeometry::Triangle* m_data = a1->m_triangles.begin();
			hkGeometry::Triangle* v51 = &m_data[v56];
			hkGeometry::Triangle* v52 = &m_data[v56 + 2];
			v51->m_a = v14->m_a;
			v51->m_b = m_size;
			v51->m_c = m_size + 2;
			v51[1].m_a = m_size;
			v51[1].m_b = v14->m_b;
			v51[1].m_c = m_size + 1;
			v52->m_a = m_size + 2;
			v52->m_b = m_size + 1;
			v52->m_c = v14->m_c;
			v14->m_a = m_size;
			v14->m_b = m_size + 1;
			v14->m_c = m_size + 2;
			_subdivideRecursive(a1, v56, a3, v54);
			_subdivideRecursive(a1, v56 + 1, a3, v54);
			_subdivideRecursive(a1, v56 + 2, a3, v54);
			a4 = v54;
			if (0.0f == v54) {
				break;
			}
			v4 = arg4;
		}
	}
}

hkDisplaySphere::hkDisplaySphere(const hkSphere& sphere, int32_t xRes, int32_t yRes)
	: hkDisplayGeometry(HK_DISPLAY_SPHERE), m_sphere(sphere), m_xRes(xRes), m_yRes(yRes) {
	m_transform.m_rotation.set(hkQuaternionIdentity);
	m_transform.m_translation = sphere.m_pos;
}

void hkDisplaySphere::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
	;
}

void hkDisplaySphere::buildGeometry() {
	__m128 v1 = _mm_undefined_ps();
	__m128 v2 = _mm_undefined_ps();
	__m128 v3 = _mm_undefined_ps();
	__m128 v4 = _mm_undefined_ps();
	hkGeometry* v8;
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkGeometry* v7 = static_cast<hkGeometry*>(Instance->m_heap->blockAlloc(sizeof(hkGeometry)));
	if (v7) {
		v7->m_vertices.m_data = 0;
		v7->m_vertices.m_size = 0;
		v7->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		v7->m_triangles.m_data = 0;
		v7->m_triangles.m_size = 0;
		v7->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
		v8 = v7;
	} else {
		v8 = 0;
	}
	m_geometry = v8;
	float a3 = m_sphere.m_pos.m_quad[3];
	float v57 = 0.525731086730957 * a3;
	float v55 = a3 * 0.8506507873535156;
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	if ((v8->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < 12) {
		int32_t v10 = 2 * (v8->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
		if (v10 <= 12) {
			v10 = 12;
		}
		hkArrayUtil::_reserve(*m_heap, v8, v10, sizeof(hkVector4));
	}
	__m128 v11 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v55)));
	__m128 v12 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
	__m128* p_m_quad = &v8->m_vertices.begin()->m_quad;
	v3[0] = a3 * 0.8506507873535156;
	v4[0] = 0.0f;
	v12[0] = -0.0f - v57;
	v1[0] = -0.0f - v57;
	v2[0] = 0.0f;
	v8->m_vertices.m_size = 12;
	__m128 v14 = _mm_unpacklo_ps(v2, v4);
	__m128 v15 = _mm_unpacklo_ps(_mm_unpacklo_ps(v1, v3), v14);
	*p_m_quad = v15;
	v15[0] = 0.525731086730957 * a3;
	v14[0] = 0.0f;
	__m128 v16 = _mm_unpacklo_ps(v14, v4);
	v8->m_vertices.m_data[1].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(v15, v3), v16);
	__m128 v17 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
	v17[0] = -0.0f - v55;
	v3[0] = -0.0f - v55;
	v11[0] = -0.0f - v57;
	v16[0] = 0.0f;
	__m128 v18 = _mm_unpacklo_ps(v16, v4);
	__m128 v19 = _mm_unpacklo_ps(_mm_unpacklo_ps(v11, v3), v18);
	v8->m_vertices.m_data[2].m_quad = v19;
	v19[0] = 0.525731086730957 * a3;
	v18[0] = 0.0f;
	__m128 v20 = _mm_unpacklo_ps(_mm_unpacklo_ps(v19, v3), _mm_unpacklo_ps(v18, v4));
	v8->m_vertices.m_data[3].m_quad = v20;
	v3[0] = 0.525731086730957 * a3;
	v20[0] = 0.0f;
	__m128 v21 = _mm_unpacklo_ps(_mm_unpacklo_ps(v20, v3),
		_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v55))), v4));
	v8->m_vertices.m_data[4].m_quad = v21;
	v3[0] = -0.0f - v57;
	v21[0] = 0.0f;
	__m128 v22 = _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v55))), v4);
	__m128 v23 = _mm_unpacklo_ps(_mm_unpacklo_ps(v21, v3), v22);
	v8->m_vertices.m_data[5].m_quad = v23;
	v3[0] = 0.525731086730957 * a3;
	v23[0] = 0.0f;
	v22[0] = -0.0f - v55;
	__m128 v24 = _mm_unpacklo_ps(v22, v4);
	__m128 v25 = _mm_unpacklo_ps(_mm_unpacklo_ps(v23, v3), v24);
	v8->m_vertices.m_data[6].m_quad = v25;
	v25[0] = 0.0f;
	v24[0] = -0.0f - v55;
	v3[0] = -0.0f - v57;
	__m128 v26 = _mm_unpacklo_ps(v24, v4);
	v8->m_vertices.m_data[7].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(v25, v3), v26);
	v26[0] = 0.525731086730957 * a3;
	v3[0] = 0.0f;
	__m128 v27 = _mm_unpacklo_ps(v26, v4);
	__m128 v28 =
		_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v55))), v3), v27);
	v8->m_vertices.m_data[8].m_quad = v28;
	v28[0] = -0.0f - v55;
	v27[0] = 0.0f;
	__m128 v29 = _mm_unpacklo_ps(_mm_unpacklo_ps(v28, v27),
		_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v57))), v3));
	v8->m_vertices.m_data[9].m_quad = v29;
	v29[0] = -0.0f - v57;
	__m128 v30 = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v55))), v27),
		_mm_unpacklo_ps(v29, v3));
	v8->m_vertices.m_data[10].m_quad = v30;
	v30[0] = 0.0f;
	v8->m_vertices.m_data[11].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(v17, v30),
		_mm_unpacklo_ps(v12, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))));
	hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* p_m_triangles = &m_geometry->m_triangles;
	hkMemoryAllocator* v32 = (&hkMemoryRouter::getInstance())->m_heap;
	if ((p_m_triangles->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK) < 20) {
		int32_t v33 = 2 * (p_m_triangles->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK);
		if (v33 <= 20) {
			v33 = 20;
		}
		hkArrayUtil::_reserve(*v32, p_m_triangles, v33, sizeof(hkGeometry::Triangle));
	}
	hkGeometry::Triangle* m_data = p_m_triangles->begin();
	p_m_triangles->m_size = 20;
	m_data->m_c = 0;
	m_data->m_a = 1;
	m_data->m_b = 4;
	m_data->m_material = -1;
	hkGeometry::Triangle* v35 = p_m_triangles->begin();
	v35[1].m_c = 0;
	v35[1].m_a = 4;
	v35[1].m_material = -1;
	v35[1].m_b = 9;
	hkGeometry::Triangle* v36 = p_m_triangles->begin();
	v36[2].m_a = 4;
	v36[2].m_c = 9;
	v36[2].m_material = -1;
	v36[2].m_b = 5;
	hkGeometry::Triangle* v37 = p_m_triangles->begin();
	v37[3].m_b = 5;
	v37[3].m_c = 4;
	v37[3].m_material = -1;
	v37[3].m_a = 8;
	hkGeometry::Triangle* v38 = p_m_triangles->begin();
	v38[4].m_b = 8;
	v38[4].m_a = 1;
	v38[4].m_c = 4;
	v38[4].m_material = -1;
	hkGeometry::Triangle* v39 = p_m_triangles->begin();
	v39[5].m_c = 8;
	v39[5].m_a = 1;
	v39[5].m_b = 10;
	v39[5].m_material = -1;
	hkGeometry::Triangle* v40 = p_m_triangles->begin();
	v40[6].m_c = 8;
	v40[6].m_a = 10;
	v40[6].m_b = 3;
	v40[6].m_material = -1;
	hkGeometry::Triangle* v41 = p_m_triangles->begin();
	v41[7].m_a = 8;
	v41[7].m_c = 5;
	v41[7].m_b = 3;
	v41[7].m_material = -1;
	hkGeometry::Triangle* v42 = p_m_triangles->begin();
	v42[8].m_c = 5;
	v42[8].m_a = 3;
	v42[8].m_material = -1;
	v42[8u].m_b = 2;
	hkGeometry::Triangle* v43 = p_m_triangles->begin();
	v43[9].m_a = 3;
	v43[9].m_c = 2;
	v43[9].m_material = -1;
	v43[9].m_b = 7;
	hkGeometry::Triangle* v44 = p_m_triangles->begin() + 10;
	v44->m_a = 3;
	v44->m_b = 10;
	v44->m_c = 7;
	v44->m_material = -1;
	hkGeometry::Triangle* v45 = p_m_triangles->begin() + 11;
	v45->m_a = 10;
	v45->m_b = 6;
	v45->m_c = 7;
	v45->m_material = -1;
	hkGeometry::Triangle* v46 = p_m_triangles->begin() + 12;
	v46->m_a = 6;
	v46->m_b = 11;
	v46->m_c = 7;
	v46->m_material = -1;
	hkGeometry::Triangle* v47 = p_m_triangles->begin();
	v47[13].m_a = 6;
	v47[13].m_b = 0;
	v47[13].m_c = 11;
	v47[13].m_material = -1;
	hkGeometry::Triangle* v48 = p_m_triangles->begin();
	v48[14].m_a = 6;
	v48[14].m_b = 1;
	v48[14].m_c = 0;
	v48[14].m_material = -1;
	hkGeometry::Triangle* v49 = p_m_triangles->begin();
	v49[15].m_a = 10;
	v49[15].m_b = 1;
	v49[15].m_c = 6;
	v49[15].m_material = -1;
	hkGeometry::Triangle* v50 = p_m_triangles->begin();
	v50[16].m_a = 11;
	v50[16].m_b = 0;
	v50[16].m_c = 9;
	v50[16].m_material = -1;
	hkGeometry::Triangle* v51 = p_m_triangles->begin();
	v51[17].m_a = 2;
	v51[17].m_b = 11;
	v51[17].m_c = 9;
	v51[17].m_material = -1;
	hkGeometry::Triangle* v52 = p_m_triangles->begin() + 18;
	v52->m_a = 5;
	v52->m_b = 2;
	v52->m_c = 9;
	v52->m_material = -1;
	hkGeometry::Triangle* v53 = p_m_triangles->begin() + 19;
	v53->m_a = 11;
	v53->m_b = 2;
	v53->m_c = 7;
	v53->m_material = -1;
	for (int32_t i = 0; i < 20; ++i) {
		_subdivideRecursive(m_geometry, i, a3, 2.0f);
	}
}
