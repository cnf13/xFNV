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
#include "xfnv/Compiler.h"

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
    lines._setSize(a, 24);
    XFNV_M128_F32(v3, 0) = 0.0f;
    lines.begin()->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[1].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[2].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[3].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[4].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[5].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[6].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[7].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[8].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[9].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[10].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[11].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[12].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[13].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[14].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[15].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[16].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[17].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[18].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[19].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[20].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[21].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[22].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            v3
        )
    );
    lines.m_data[23].m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
}

void hkDisplayAABB::buildGeometry() {
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
    hkVector4* v10 = v4->m_vertices.expandBy(1);
    v10->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* m_geometry = this->m_geometry;
    hkVector4* v12 = m_geometry->m_vertices.expandBy(1);
    v12->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v16 = this->m_geometry;
    hkVector4* v18 = v16->m_vertices.expandBy(1);
    v18->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v22 = this->m_geometry;
    hkVector4* v24 = v22->m_vertices.expandBy(1);
    v24->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v28 = this->m_geometry;
    hkVector4* v30 = v28->m_vertices.expandBy(1);
    v30->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v34 = this->m_geometry;
    hkVector4* v36 = v34->m_vertices.expandBy(1);
    v36->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v40 = this->m_geometry;
    hkVector4* v42 = v40->m_vertices.expandBy(1);
    v42->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkGeometry* v46 = this->m_geometry;
    hkVector4* v48 = v46->m_vertices.expandBy(1);
    v48->m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 0)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_minExtent.m_quad, 2))))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(XFNV_M128_F32(m_maxExtent.m_quad, 1)))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* p_m_triangles = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v58 = p_m_triangles->expandBy(1);
    v58->m_a = 0;
    v58->m_b = 3;
    v58->m_c = 1;
    v58->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v59 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v65 = v59->expandBy(1);
    v65->m_a = 1;
    v65->m_b = 3;
    v65->m_c = 2;
    v65->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v66 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v72 = v66->expandBy(1);
    v72->m_a = 2;
    v72->m_b = 6;
    v72->m_c = 5;
    v72->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v73 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v79 = v73->expandBy(1);
    v79->m_a = 5;
    v79->m_b = 1;
    v79->m_c = 2;
    v79->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v80 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v86 = v80->expandBy(1);
    v86->m_a = 5;
    v86->m_b = 6;
    v86->m_c = 4;
    v86->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v87 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v93 = v87->expandBy(1);
    v93->m_a = 4;
    v93->m_b = 6;
    v93->m_c = 7;
    v93->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v94 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v100 = v94->expandBy(1);
    v100->m_a = 7;
    v100->m_b = 3;
    v100->m_c = 0;
    v100->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v101 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v107 = v101->expandBy(1);
    v107->m_a = 0;
    v107->m_b = 4;
    v107->m_c = 7;
    v107->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v108 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v114 = v108->expandBy(1);
    v114->m_a = 0;
    v114->m_b = 1;
    v114->m_c = 4;
    v114->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v115 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v121 = v115->expandBy(1);
    v121->m_a = 4;
    v121->m_b = 1;
    v121->m_c = 5;
    v121->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v122 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v128 = v122->expandBy(1);
    v128->m_a = 2;
    v128->m_b = 3;
    v128->m_c = 6;
    v128->m_material = -1;
    hkArray<hkGeometry::Triangle, hkContainerHeapAllocator>* v129 = &this->m_geometry->m_triangles;
    hkGeometry::Triangle* v135 = v129->expandBy(1);
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
