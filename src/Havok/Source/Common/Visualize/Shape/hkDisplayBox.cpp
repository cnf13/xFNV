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
#include "xfnv/Compiler.h"

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
    lines._setSize(a, 24);

    alignas(16) hkVector4 vertices[8];
    for (int32_t i = 0; i < 8; i++) {
        alignas(16) hkVector4 v;
        v.m_quad = m_halfExtents.m_quad;
        // Scales by a double -1.0 and stores the float back.
        if (i & 1) {
            XFNV_M128_F32(v.m_quad, 0) = static_cast<float>(XFNV_M128_F32(v.m_quad, 0) * -1.0);
        }
        if (i & 2) {
            XFNV_M128_F32(v.m_quad, 1) = static_cast<float>(XFNV_M128_F32(v.m_quad, 1) * -1.0);
        }
        if (i & 4) {
            XFNV_M128_F32(v.m_quad, 2) = static_cast<float>(-1.0 * XFNV_M128_F32(v.m_quad, 2));
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
    hkGeometry* v3 = static_cast<hkGeometry*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkGeometry)));
    hkGeometry* v4 = nullptr;
    if (v3) {
        v3->m_vertices.m_data = nullptr;
        v3->m_vertices.m_size = 0;
        v3->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        v3->m_triangles.m_data = nullptr;
        v3->m_triangles.m_size = 0;
        v3->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
        v4 = v3;
    }
    this->m_geometry = v4;
    hkVector4* pVertex = v4->m_vertices.expandBy(1);
    __m128 v10 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v10, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 0);
    pVertex->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            v10,
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* m_geometry = this->m_geometry;
    pVertex = m_geometry->m_vertices.expandBy(1);
    pVertex->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v17 = this->m_geometry;
    pVertex = v17->m_vertices.expandBy(1);
    __m128 v23 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v23, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 1);
    __m128 v24 = _mm_unpacklo_ps(v23, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))));
    pVertex->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 2))))
        ),
        v24
    );
    hkGeometry* v25 = this->m_geometry;
    pVertex = v25->m_vertices.expandBy(1);
    __m128 v31 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v24, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 0);
    XFNV_M128_F32(v31, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 1);
    __m128 v32 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            v24,
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 2))))
        ),
        _mm_unpacklo_ps(v31, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    pVertex->m_quad = v32;
    hkGeometry* v33 = this->m_geometry;
    pVertex = v33->m_vertices.expandBy(1);
    __m128 v39 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v32, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 0);
    XFNV_M128_F32(v39, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 2);
    pVertex->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v32, v39),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v40 = this->m_geometry;
    pVertex = v40->m_vertices.expandBy(1);
    __m128 v46 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v46, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 2);
    pVertex->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 0)))),
            v46
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v47 = this->m_geometry;
    pVertex = v47->m_vertices.expandBy(1);
    __m128 v53 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v46, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 1);
    XFNV_M128_F32(v53, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 2);
    __m128 v54 = _mm_unpacklo_ps(v46, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))));
    __m128 v55 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_halfExtents.m_quad, 0)))),
            v53
        ),
        v54
    );
    pVertex->m_quad = v55;
    hkGeometry* v56 = this->m_geometry;
    pVertex = v56->m_vertices.expandBy(1);
    __m128 v62 = _mm_set_ss(std::bit_cast<float>(0x80000000u));
    XFNV_M128_F32(v55, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 0);
    XFNV_M128_F32(v54, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 1);
    XFNV_M128_F32(v62, 0) = -0.0f - XFNV_M128_F32(m_halfExtents.m_quad, 2);
    __m128* p_m_quad = &pVertex->m_quad;
    int32_t v64 = 0;
    *p_m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v55, v62),
        _mm_unpacklo_ps(v54, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    if (this->m_geometry->m_vertices.m_size > 0) {
        int32_t v65 = 0;
        do {
            this->m_geometry->m_vertices.m_data[v65].setTransformedPos(
                m_transform,
                this->m_geometry->m_vertices.m_data[v65]
            );
            ++v64;
            ++v65;
        }
        while (v64 < this->m_geometry->m_vertices.m_size);
    }
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* p_m_triangles = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v72 = p_m_triangles->expandBy(1);
    v72->m_a = 3;
    v72->m_b = 2;
    v72->m_c = 1;
    v72->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v73 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v79 = v73->expandBy(1);
    v79->m_a = 3;
    v79->m_b = 1;
    v79->m_c = 0;
    v79->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v80 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v86 = v80->expandBy(1);
    v86->m_a = 6;
    v86->m_b = 7;
    v86->m_c = 4;
    v86->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v87 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v93 = v87->expandBy(1);
    v93->m_a = 6;
    v93->m_b = 4;
    v93->m_c = 5;
    v93->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v94 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v100 = v94->expandBy(1);
    v100->m_a = 4;
    v100->m_b = 7;
    v100->m_c = 3;
    v100->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v101 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v107 = v101->expandBy(1);
    v107->m_a = 4;
    v107->m_b = 3;
    v107->m_c = 0;
    v107->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v108 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v114 = v108->expandBy(1);
    v114->m_a = 2;
    v114->m_b = 6;
    v114->m_c = 5;
    v114->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v115 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v121 = v115->expandBy(1);
    v121->m_a = 2;
    v121->m_b = 5;
    v121->m_c = 1;
    v121->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v122 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v128 = v122->expandBy(1);
    v128->m_a = 7;
    v128->m_b = 6;
    v128->m_c = 2;
    v128->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v129 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v135 = v129->expandBy(1);
    v135->m_a = 7;
    v135->m_b = 2;
    v135->m_c = 3;
    v135->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v136 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v142 = v136->expandBy(1);
    v142->m_a = 1;
    v142->m_b = 5;
    v142->m_c = 4;
    v142->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v143 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v149 = v143->expandBy(1);
    v149->m_material = -1;
    v149->m_a = 1;
    v149->m_b = 4;
    v149->m_c = 0;
}

void hkDisplayBox::setParameters(const hkVector4& halfExtents, const hkTransform& t) {
    m_halfExtents = halfExtents;
    m_transform = t;
}
