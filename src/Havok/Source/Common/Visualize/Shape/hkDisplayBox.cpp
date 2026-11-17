#include "Havok/Source/Common/Visualize/Shape/hkDisplayBox.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <bit>
#include <xmmintrin.h>

hkDisplayBox::hkDisplayBox() : hkDisplayGeometry(HK_DISPLAY_BOX) {
    m_halfExtents.setZero4();
}

hkDisplayBox::hkDisplayBox(const hkVector4& halfExtents)
  : hkDisplayGeometry(HK_DISPLAY_BOX), m_halfExtents(halfExtents) {}

hkDisplayBox::~hkDisplayBox() {}

// Fills lines with the box's 12 edges, two end points each: the 8 corners (half extents with the signs of the
// corner index's bits, transformed), then each corner joined to every corner differing from it in one bit.
void hkDisplayBox::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
    // EXE-INLINED none: hkArrayBase<hkVector4>::_setSize
    int32_t capacity = static_cast<int32_t>(lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
    if (capacity < 24) {
        int32_t newCapacity = capacity + capacity;
        if (newCapacity <= 24) {
            newCapacity = 24;
        }
        hkArrayUtil::_reserve(&a, &lines, newCapacity, sizeof(hkVector4));
    }
    lines.m_size = 24;

    alignas(16) hkVector4 vertices[8];
    for (int32_t i = 0; i < 8; i++) {
        alignas(16) hkVector4 v;
        v.m_quad = m_halfExtents.m_quad;
        // Scales by a double -1.0 and stores the float back.
        if (i & 1) {
            v.m_quad[0] = static_cast<float>(v.m_quad[0] * -1.0);
        }
        if (i & 2) {
            v.m_quad[1] = static_cast<float>(v.m_quad[1] * -1.0);
        }
        if (i & 4) {
            v.m_quad[2] = static_cast<float>(-1.0 * v.m_quad[2]);
        }
        vertices[i].setTransformedPos(m_transform, v);
    }

    int32_t index = 0;
    for (int32_t i = 0; i < 8; i++) {
        for (int32_t j = 1; j < 8; j <<= 1) {
            int32_t k = i ^ j;
            if (i < k) {
                lines.m_data[index++].m_quad = vertices[i].m_quad;
                lines.m_data[index++].m_quad = vertices[k].m_quad;
            }
        }
    }
}

void hkDisplayBox::buildGeometry() {
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
    this->m_geometry = v4;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_size = v4->m_vertices.m_size;
    int32_t v7 = m_size + 1;
    int32_t v8 = v4->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v8 < m_size + 1) {
        int32_t v9 = 2 * v8;
        if (v7 < v9) {
            v7 = v9;
        }
        hkArrayUtil::_reserve(*m_heap, v4, v7, sizeof(hkVector4));
    }
    ++v4->m_vertices.m_size;
    __m128 v10 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v10[0] = -0.0f - m_halfExtents.m_quad[0];
    v4->m_vertices.m_data[m_size].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v10, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[2])))),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* m_geometry = this->m_geometry;
    hkMemoryAllocator* v12 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v13 = m_geometry->m_vertices.m_size;
    int32_t v14 = v13 + 1;
    int32_t v15 = m_geometry->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v15 < v13 + 1) {
        int32_t v16 = 2 * v15;
        if (v14 < v16) {
            v14 = v16;
        }
        hkArrayUtil::_reserve(*v12, m_geometry, v14, sizeof(hkVector4));
    }
    ++m_geometry->m_vertices.m_size;
    m_geometry->m_vertices.m_data[v13].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v17 = this->m_geometry;
    hkMemoryAllocator* v18 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v19 = v17->m_vertices.m_size;
    int32_t v20 = v19 + 1;
    int32_t v21 = v17->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v21 < v19 + 1) {
        int32_t v22 = 2 * v21;
        if (v20 < v22) {
            v20 = v22;
        }
        hkArrayUtil::_reserve(*v18, v17, v20, sizeof(hkVector4));
    }
    ++v17->m_vertices.m_size;
    __m128 v23 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v23[0] = -0.0f - m_halfExtents.m_quad[1];
    __m128 v24 = _mm_unpacklo_ps(v23, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))));
    v17->m_vertices.m_data[v19].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[2])))
        ),
        v24
    );
    hkGeometry* v25 = this->m_geometry;
    hkMemoryAllocator* v26 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v27 = v25->m_vertices.m_size;
    int32_t v28 = v27 + 1;
    int32_t v29 = v25->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v29 < v27 + 1) {
        int32_t v30 = 2 * v29;
        if (v28 < v30) {
            v28 = v30;
        }
        hkArrayUtil::_reserve(*v26, v25, v28, sizeof(hkVector4));
    }
    ++v25->m_vertices.m_size;
    __m128 v31 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v24[0] = -0.0f - m_halfExtents.m_quad[0];
    v31[0] = -0.0f - m_halfExtents.m_quad[1];
    __m128 v32 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v24, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[2])))),
        _mm_unpacklo_ps(v31, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    v25->m_vertices.m_data[v27].m_quad = v32;
    hkGeometry* v33 = this->m_geometry;
    hkMemoryAllocator* v34 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v35 = v33->m_vertices.m_size;
    int32_t v36 = v35 + 1;
    int32_t v37 = v33->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v37 < v35 + 1) {
        int32_t v38 = 2 * v37;
        if (v36 < v38) {
            v36 = v38;
        }
        hkArrayUtil::_reserve(*v34, v33, v36, sizeof(hkVector4));
    }
    ++v33->m_vertices.m_size;
    __m128 v39 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v32[0] = -0.0f - m_halfExtents.m_quad[0];
    v39[0] = -0.0f - m_halfExtents.m_quad[2];
    v33->m_vertices.m_data[v35].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v32, v39),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v40 = this->m_geometry;
    hkMemoryAllocator* v41 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v42 = v40->m_vertices.m_size;
    int32_t v43 = v42 + 1;
    int32_t v44 = v40->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v44 < v42 + 1) {
        int32_t v45 = 2 * v44;
        if (v43 < v45) {
            v43 = v45;
        }
        hkArrayUtil::_reserve(*v41, v40, v43, sizeof(hkVector4));
    }
    ++v40->m_vertices.m_size;
    __m128 v46 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v46[0] = -0.0f - m_halfExtents.m_quad[2];
    v40->m_vertices.m_data[v42].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[0]))), v46),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v47 = this->m_geometry;
    hkMemoryAllocator* v48 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v49 = v47->m_vertices.m_size;
    int32_t v50 = v49 + 1;
    int32_t v51 = v47->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v51 < v49 + 1) {
        int32_t v52 = 2 * v51;
        if (v50 < v52) {
            v50 = v52;
        }
        hkArrayUtil::_reserve(*v48, v47, v50, sizeof(hkVector4));
    }
    ++v47->m_vertices.m_size;
    __m128 v53 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v46[0] = -0.0f - m_halfExtents.m_quad[1];
    v53[0] = -0.0f - m_halfExtents.m_quad[2];
    __m128 v54 = _mm_unpacklo_ps(v46, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))));
    __m128 v55 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_halfExtents.m_quad[0]))), v53), v54
    );
    v47->m_vertices.m_data[v49].m_quad = v55;
    hkGeometry* v56 = this->m_geometry;
    hkMemoryAllocator* v57 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v58 = v56->m_vertices.m_size;
    int32_t v59 = v58 + 1;
    int32_t v60 = v56->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v60 < v58 + 1) {
        int32_t v61 = 2 * v60;
        if (v59 < v61) {
            v59 = v61;
        }
        hkArrayUtil::_reserve(*v57, v56, v59, sizeof(hkVector4));
    }
    ++v56->m_vertices.m_size;
    __m128 v62 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    v55[0] = -0.0f - m_halfExtents.m_quad[0];
    v54[0] = -0.0f - m_halfExtents.m_quad[1];
    v62[0] = -0.0f - m_halfExtents.m_quad[2];
    __m128* p_m_quad = &v56->m_vertices.m_data[v58].m_quad;
    int32_t v64 = 0;
    *p_m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v55, v62), _mm_unpacklo_ps(v54, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    if (this->m_geometry->m_vertices.m_size > 0) {
        int32_t v65 = 0;
        do {
            this->m_geometry->m_vertices.m_data[v65].setTransformedPos(
                m_transform, this->m_geometry->m_vertices.m_data[v65]
            );
            ++v64;
            ++v65;
        }
        while (v64 < this->m_geometry->m_vertices.m_size);
    }
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* p_m_triangles = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v67 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v68 = p_m_triangles->m_size;
    int32_t v69 = v68 + 1;
    int32_t v70 = p_m_triangles->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v70 < v68 + 1) {
        int32_t v71 = 2 * v70;
        if (v69 < v71) {
            v69 = v71;
        }
        hkArrayUtil::_reserve(*v67, p_m_triangles, v69, sizeof(hkGeometry::Triangle));
    }
    ++p_m_triangles->m_size;
    hkGeometry::Triangle* v72 = &p_m_triangles->m_data[v68];
    v72->m_a = 3;
    v72->m_b = 2;
    v72->m_c = 1;
    v72->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v73 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v74 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v75 = v73->m_size;
    int32_t v76 = v75 + 1;
    int32_t v77 = v73->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v77 < v75 + 1) {
        int32_t v78 = 2 * v77;
        if (v76 < v78) {
            v76 = v78;
        }
        hkArrayUtil::_reserve(*v74, v73, v76, sizeof(hkGeometry::Triangle));
    }
    ++v73->m_size;
    hkGeometry::Triangle* v79 = &v73->m_data[v75];
    v79->m_a = 3;
    v79->m_b = 1;
    v79->m_c = 0;
    v79->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v80 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v81 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v82 = v80->m_size;
    int32_t v83 = v82 + 1;
    int32_t v84 = v80->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v84 < v82 + 1) {
        int32_t v85 = 2 * v84;
        if (v83 < v85) {
            v83 = v85;
        }
        hkArrayUtil::_reserve(*v81, v80, v83, sizeof(hkGeometry::Triangle));
    }
    ++v80->m_size;
    hkGeometry::Triangle* v86 = &v80->m_data[v82];
    v86->m_a = 6;
    v86->m_b = 7;
    v86->m_c = 4;
    v86->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v87 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v88 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v89 = v87->m_size;
    int32_t v90 = v89 + 1;
    int32_t v91 = v87->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v91 < v89 + 1) {
        int32_t v92 = 2 * v91;
        if (v90 < v92) {
            v90 = v92;
        }
        hkArrayUtil::_reserve(*v88, v87, v90, sizeof(hkGeometry::Triangle));
    }
    ++v87->m_size;
    hkGeometry::Triangle* v93 = &v87->m_data[v89];
    v93->m_a = 6;
    v93->m_b = 4;
    v93->m_c = 5;
    v93->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v94 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v95 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v96 = v94->m_size;
    int32_t v97 = v96 + 1;
    int32_t v98 = v94->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v98 < v96 + 1) {
        int32_t v99 = 2 * v98;
        if (v97 < v99) {
            v97 = v99;
        }
        hkArrayUtil::_reserve(*v95, v94, v97, sizeof(hkGeometry::Triangle));
    }
    ++v94->m_size;
    hkGeometry::Triangle* v100 = &v94->m_data[v96];
    v100->m_a = 4;
    v100->m_b = 7;
    v100->m_c = 3;
    v100->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v101 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v102 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v103 = v101->m_size;
    int32_t v104 = v103 + 1;
    int32_t v105 = v101->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v105 < v103 + 1) {
        int32_t v106 = 2 * v105;
        if (v104 < v106) {
            v104 = v106;
        }
        hkArrayUtil::_reserve(*v102, v101, v104, sizeof(hkGeometry::Triangle));
    }
    ++v101->m_size;
    hkGeometry::Triangle* v107 = &v101->m_data[v103];
    v107->m_a = 4;
    v107->m_b = 3;
    v107->m_c = 0;
    v107->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v108 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v109 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v110 = v108->m_size;
    int32_t v111 = v110 + 1;
    int32_t v112 = v108->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v112 < v110 + 1) {
        int32_t v113 = 2 * v112;
        if (v111 < v113) {
            v111 = v113;
        }
        hkArrayUtil::_reserve(*v109, v108, v111, sizeof(hkGeometry::Triangle));
    }
    ++v108->m_size;
    hkGeometry::Triangle* v114 = &v108->m_data[v110];
    v114->m_a = 2;
    v114->m_b = 6;
    v114->m_c = 5;
    v114->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v115 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v116 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v117 = v115->m_size;
    int32_t v118 = v117 + 1;
    int32_t v119 = v115->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v119 < v117 + 1) {
        int32_t v120 = 2 * v119;
        if (v118 < v120) {
            v118 = v120;
        }
        hkArrayUtil::_reserve(*v116, v115, v118, sizeof(hkGeometry::Triangle));
    }
    ++v115->m_size;
    hkGeometry::Triangle* v121 = &v115->m_data[v117];
    v121->m_a = 2;
    v121->m_b = 5;
    v121->m_c = 1;
    v121->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v122 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v123 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v124 = v122->m_size;
    int32_t v125 = v124 + 1;
    int32_t v126 = v122->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v126 < v124 + 1) {
        int32_t v127 = 2 * v126;
        if (v125 < v127) {
            v125 = v127;
        }
        hkArrayUtil::_reserve(*v123, v122, v125, sizeof(hkGeometry::Triangle));
    }
    ++v122->m_size;
    hkGeometry::Triangle* v128 = &v122->m_data[v124];
    v128->m_a = 7;
    v128->m_b = 6;
    v128->m_c = 2;
    v128->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v129 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v130 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v131 = v129->m_size;
    int32_t v132 = v131 + 1;
    int32_t v133 = v129->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v133 < v131 + 1) {
        int32_t v134 = 2 * v133;
        if (v132 < v134) {
            v132 = v134;
        }
        hkArrayUtil::_reserve(*v130, v129, v132, sizeof(hkGeometry::Triangle));
    }
    ++v129->m_size;
    hkGeometry::Triangle* v135 = &v129->m_data[v131];
    v135->m_a = 7;
    v135->m_b = 2;
    v135->m_c = 3;
    v135->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v136 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v137 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v138 = v136->m_size;
    int32_t v139 = v138 + 1;
    int32_t v140 = v136->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v140 < v138 + 1) {
        int32_t v141 = 2 * v140;
        if (v139 < v141) {
            v139 = v141;
        }
        hkArrayUtil::_reserve(*v137, v136, v139, sizeof(hkGeometry::Triangle));
    }
    ++v136->m_size;
    hkGeometry::Triangle* v142 = &v136->m_data[v138];
    v142->m_a = 1;
    v142->m_b = 5;
    v142->m_c = 4;
    v142->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v143 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v144 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v145 = v143->m_size;
    int32_t v146 = v145 + 1;
    int32_t v147 = v143->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v147 < v145 + 1) {
        int32_t v148 = 2 * v147;
        if (v146 < v148) {
            v146 = v148;
        }
        hkArrayUtil::_reserve(*v144, v143, v146, sizeof(hkGeometry::Triangle));
    }
    ++v143->m_size;
    hkGeometry::Triangle* v149 = &v143->m_data[v145];
    v149->m_material = -1;
    v149->m_a = 1;
    v149->m_b = 4;
    v149->m_c = 0;
}

void hkDisplayBox::setParameters(const hkVector4& halfExtents, const hkTransform& t) {
    m_halfExtents = halfExtents;
    m_transform = t;
}
