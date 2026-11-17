#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"

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

hkDisplayAABB::hkDisplayAABB() : hkDisplayGeometry(HK_DISPLAY_AABB) {
    m_minExtent.setZero4();
    m_maxExtent.setZero4();
}

hkDisplayAABB::hkDisplayAABB(const hkVector4& min, const hkVector4& max)
  : hkDisplayGeometry(HK_DISPLAY_AABB), m_minExtent(min), m_maxExtent(max) {}

hkDisplayAABB::~hkDisplayAABB() {}

void hkDisplayAABB::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
    __m128 v3 = _mm_undefined_ps();
    if ((lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < 24) {
        int32_t v5 = 2 * (lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
        if (v5 <= 24) {
            v5 = 24;
        }
        hkArrayUtil::_reserve(a, &lines, v5, sizeof(hkVector4));
    }
    lines.m_size = 24;
    v3[0] = 0.0f;
    (&lines)->begin()->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[1].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[2].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[3].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[4].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[5].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[6].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[7].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[8].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[9].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[10].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[11].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[12].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[13].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[14].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[15].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[16].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[17].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[18].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))), v3)
    );
    lines.m_data[19].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[20].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[21].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[22].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))), v3)
    );
    lines.m_data[23].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
}

void hkDisplayAABB::buildGeometry() {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkGeometry* v3 = static_cast<hkGeometry*>(Instance->m_heap->blockAlloc(sizeof(hkGeometry)));
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
    v4->m_vertices.m_data[m_size].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* m_geometry = this->m_geometry;
    hkMemoryAllocator* v11 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v12 = m_geometry->m_vertices.m_size;
    int32_t v13 = v12 + 1;
    int32_t v14 = m_geometry->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v14 < v12 + 1) {
        int32_t v15 = 2 * v14;
        if (v13 < v15) {
            v13 = v15;
        }
        hkArrayUtil::_reserve(*v11, m_geometry, v13, sizeof(hkVector4));
    }
    ++m_geometry->m_vertices.m_size;
    m_geometry->m_vertices.m_data[v12].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v16 = this->m_geometry;
    hkMemoryAllocator* v17 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v18 = v16->m_vertices.m_size;
    int32_t v19 = v18 + 1;
    int32_t v20 = v16->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v20 < v18 + 1) {
        int32_t v21 = 2 * v20;
        if (v19 < v21) {
            v19 = v21;
        }
        hkArrayUtil::_reserve(*v17, v16, v19, sizeof(hkVector4));
    }
    ++v16->m_vertices.m_size;
    v16->m_vertices.m_data[v18].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v22 = this->m_geometry;
    hkMemoryAllocator* v23 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v24 = v22->m_vertices.m_size;
    int32_t v25 = v24 + 1;
    int32_t v26 = v22->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v26 < v24 + 1) {
        int32_t v27 = 2 * v26;
        if (v25 < v27) {
            v25 = v27;
        }
        hkArrayUtil::_reserve(*v23, v22, v25, sizeof(hkVector4));
    }
    ++v22->m_vertices.m_size;
    v22->m_vertices.m_data[v24].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v28 = this->m_geometry;
    hkMemoryAllocator* v29 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v30 = v28->m_vertices.m_size;
    int32_t v31 = v30 + 1;
    int32_t v32 = v28->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v32 < v30 + 1) {
        int32_t v33 = 2 * v32;
        if (v31 < v33) {
            v31 = v33;
        }
        hkArrayUtil::_reserve(*v29, v28, v31, sizeof(hkVector4));
    }
    ++v28->m_vertices.m_size;
    v28->m_vertices.m_data[v30].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v34 = this->m_geometry;
    hkMemoryAllocator* v35 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v36 = v34->m_vertices.m_size;
    int32_t v37 = v36 + 1;
    int32_t v38 = v34->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v38 < v36 + 1) {
        int32_t v39 = 2 * v38;
        if (v37 < v39) {
            v37 = v39;
        }
        hkArrayUtil::_reserve(*v35, v34, v37, sizeof(hkVector4));
    }
    ++v34->m_vertices.m_size;
    v34->m_vertices.m_data[v36].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))),
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
    v40->m_vertices.m_data[v42].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v46 = this->m_geometry;
    hkMemoryAllocator* v47 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v48 = v46->m_vertices.m_size;
    int32_t v49 = v48 + 1;
    int32_t v50 = v46->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v50 < v48 + 1) {
        int32_t v51 = 2 * v50;
        if (v49 < v51) {
            v49 = v51;
        }
        hkArrayUtil::_reserve(*v47, v46, v49, sizeof(hkVector4));
    }
    ++v46->m_vertices.m_size;
    v46->m_vertices.m_data[v48].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[0]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_minExtent.m_quad[2])))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_maxExtent.m_quad[1]))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* p_m_triangles = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v53 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v54 = p_m_triangles->m_size;
    int32_t v55 = v54 + 1;
    int32_t v56 = p_m_triangles->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v56 < v54 + 1) {
        int32_t v57 = 2 * v56;
        if (v55 < v57) {
            v55 = v57;
        }
        hkArrayUtil::_reserve(*v53, p_m_triangles, v55, sizeof(hkGeometry::Triangle));
    }
    ++p_m_triangles->m_size;
    hkGeometry::Triangle* v58 = &p_m_triangles->m_data[v54];
    v58->m_a = 0;
    v58->m_b = 3;
    v58->m_c = 1;
    v58->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v59 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v60 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v61 = v59->m_size;
    int32_t v62 = v61 + 1;
    int32_t v63 = v59->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v63 < v61 + 1) {
        int32_t v64 = 2 * v63;
        if (v62 < v64) {
            v62 = v64;
        }
        hkArrayUtil::_reserve(*v60, v59, v62, sizeof(hkGeometry::Triangle));
    }
    ++v59->m_size;
    hkGeometry::Triangle* v65 = &v59->m_data[v61];
    v65->m_a = 1;
    v65->m_b = 3;
    v65->m_c = 2;
    v65->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v66 = &this->m_geometry->m_triangles;
    hkMemoryAllocator* v67 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v68 = v66->m_size;
    int32_t v69 = v68 + 1;
    int32_t v70 = v66->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (v70 < v68 + 1) {
        int32_t v71 = 2 * v70;
        if (v69 < v71) {
            v69 = v71;
        }
        hkArrayUtil::_reserve(*v67, v66, v69, sizeof(hkGeometry::Triangle));
    }
    ++v66->m_size;
    hkGeometry::Triangle* v72 = &v66->m_data[v68];
    v72->m_a = 2;
    v72->m_b = 6;
    v72->m_c = 5;
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
    v79->m_a = 5;
    v79->m_b = 1;
    v79->m_c = 2;
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
    v86->m_a = 5;
    v86->m_b = 6;
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
    v93->m_a = 4;
    v93->m_b = 6;
    v93->m_c = 7;
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
    v100->m_a = 7;
    v100->m_b = 3;
    v100->m_c = 0;
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
    v107->m_a = 0;
    v107->m_b = 4;
    v107->m_c = 7;
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
    v114->m_a = 0;
    v114->m_b = 1;
    v114->m_c = 4;
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
    v121->m_a = 4;
    v121->m_b = 1;
    v121->m_c = 5;
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
    v128->m_a = 2;
    v128->m_b = 3;
    v128->m_c = 6;
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
    v135->m_b = 3;
    v135->m_a = 6;
    v135->m_c = 7;
    v135->m_material = -1;
}

const hkVector4& hkDisplayAABB::getMinExtent() {
    return m_minExtent;
}

const hkVector4& hkDisplayAABB::getMaxExtent() {
    return m_maxExtent;
}

void hkDisplayAABB::setExtents(const hkVector4& min, const hkVector4& max) {
    m_minExtent = min;
    m_maxExtent = max;
}
