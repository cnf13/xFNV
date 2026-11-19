#include "Havok/Source/Common/Visualize/Shape/hkDisplayCone.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

hkDisplayCone::hkDisplayCone()
  : hkDisplayGeometry(HK_DISPLAY_CONE), m_numSegments(0), m_coneAngle(0.0f), m_coneHeight(0.0f) {
    m_startPos.setZero4();
    m_coneAxis.setZero4();
}

hkDisplayCone::hkDisplayCone(
    float coneAngle,
    float coneHeight,
    int32_t numSegments,
    const hkVector4& coneAxis,
    const hkVector4& startPos
)
  : hkDisplayGeometry(HK_DISPLAY_CONE), m_numSegments(numSegments), m_coneAngle(coneAngle), m_coneHeight(coneHeight),
    m_startPos(startPos), m_coneAxis(coneAxis) {}

void hkDisplayCone::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
    hkInplaceArray<hkVector4, 256, hkContainerHeapAllocator> vertices;
    generateConeVertices(vertices);
    int32_t m_size = 4 * this->m_numSegments;
    int32_t m_size_1 = lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (m_size_1 < m_size) {
        int32_t aiReqElem = 2 * m_size_1;
        if (m_size >= aiReqElem) {
            aiReqElem = 4 * this->m_numSegments;
        }
        hkArrayUtil::_reserve(a, &lines, aiReqElem, sizeof(hkVector4));
    }
    lines.m_size = m_size;
    // Each segment: the apex to the rim vertex, and the rim vertex to the next one.
    for (int32_t i = 0; i < this->m_numSegments; ++i) {
        lines.m_data[4 * i] = m_startPos;
        lines.m_data[4 * i + 1] = vertices.m_data[i];
        lines.m_data[4 * i + 2] = vertices.m_data[i];
        lines.m_data[4 * i + 3] = vertices.m_data[(i + 1) % this->m_numSegments];
    }
}

void hkDisplayCone::buildGeometry() {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkGeometry* m_geometry_1 = static_cast<hkGeometry*>(Instance->m_heap->blockAlloc(sizeof(hkGeometry)));
    if (m_geometry_1) {
        m_geometry_1->m_vertices.m_data = nullptr;
        m_geometry_1->m_vertices.m_size = 0;
        m_geometry_1->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        m_geometry_1->m_triangles.m_data = nullptr;
        m_geometry_1->m_triangles.m_size = 0;
        m_geometry_1->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
    }
    else {
        m_geometry_1 = nullptr;
    }
    this->m_geometry = m_geometry_1;
    generateConeVertices(m_geometry_1->m_vertices);
    // The apex.
    this->m_geometry->m_vertices.pushBack(m_startPos);
    int32_t m_numSegments = this->m_numSegments;
    hkArray<hkGeometry::Triangle>* p_m_triangles = &this->m_geometry->m_triangles;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_numSegments_2 = p_m_triangles->m_capacityAndFlags & hkArrayBase<hkGeometry::Triangle>::CAPACITY_MASK;
    if (m_numSegments_2 < m_numSegments) {
        int32_t m_numSegments_1 = 2 * m_numSegments_2;
        if (m_numSegments >= m_numSegments_1) {
            m_numSegments_1 = m_numSegments;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_triangles, m_numSegments_1, sizeof(hkGeometry::Triangle));
    }
    p_m_triangles->m_size = m_numSegments;
    int32_t m_c = 0;
    if (this->m_numSegments - 1 > 0) {
        int32_t v12 = 0;
        do {
            hkGeometry::Triangle* v13 = &this->m_geometry->m_triangles.m_data[v12];
            v13->m_a = this->m_numSegments;
            v13->m_c = m_c;
            v13->m_b = m_c + 1;
            v13->m_material = -1;
            ++m_c;
            ++v12;
        }
        while (m_c < this->m_numSegments - 1);
    }
    hkGeometry::Triangle* v14 = &this->m_geometry->m_triangles.m_data[m_c];
    int32_t m_numSegments_3 = this->m_numSegments;
    v14->m_material = -1;
    v14->m_a = m_numSegments_3;
    v14->m_b = 0;
    v14->m_c = m_c;
}

void hkDisplayCone::setParameters(
    float coneAngle,
    float coneHeight,
    int32_t numSegments,
    const hkVector4& coneAxis,
    const hkVector4& startPos
) {
    m_coneAngle = coneAngle;
    m_startPos = startPos;
    hkVector4 v6 = coneAxis;
    m_coneHeight = coneHeight;
    m_coneAxis = v6;
    m_numSegments = numSegments;
}

hkVector4 hkDisplayCone::getPosition() {
    return m_startPos;
}

hkVector4 hkDisplayCone::getAxis() {
    return m_coneAxis;
}

float hkDisplayCone::getHeight() {
    return m_coneHeight;
}

float hkDisplayCone::getAngle() {
    return m_coneAngle;
}

int32_t hkDisplayCone::getNumSegments() {
    return m_numSegments;
}

void hkDisplayCone::generateConeVertices(hkArrayBase<hkVector4>& vertices) {
    __m128 v15 = _mm_undefined_ps();
    hkVector4 arAxis;
    hkQuaternion v38;
    hkQuaternion v39;
    int32_t m_numSegments = this->m_numSegments;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_numSegments_2 = vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (m_numSegments_2 < m_numSegments) {
        int32_t m_numSegments_1 = 2 * m_numSegments_2;
        if (m_numSegments >= m_numSegments_1) {
            m_numSegments_1 = m_numSegments;
        }
        hkArrayUtil::_reserve(*m_heap, &vertices, m_numSegments_1, sizeof(hkVector4));
    }
    vertices.m_size = m_numSegments;
    hkVector4* p_m_coneAxis = &m_coneAxis;
    float v31 = std::fabs((double)XFNV_M128_F32(m_coneAxis.m_quad, 0));
    int32_t v8 = 1;
    int32_t n2_1 = 0;
    int32_t n2 = 2;
    float v33 = std::fabs((double)XFNV_M128_F32(m_coneAxis.m_quad, 1));
    float v34 = std::fabs((double)XFNV_M128_F32(m_coneAxis.m_quad, 2));
    if (v31 > static_cast<double>(v33)) {
        v31 = v33;
        v8 = 0;
        n2_1 = 1;
    }
    if (v31 > static_cast<double>(v34)) {
        n2 = n2_1;
    }
    double v10 = XFNV_M128_F32(p_m_coneAxis->m_quad, n2);
    arAxis.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(arAxis.m_quad, v8) = v10;
    XFNV_M128_F32(arAxis.m_quad, n2) = -XFNV_M128_F32(p_m_coneAxis->m_quad, v8);
    __m128 v11 = _mm_mul_ps(arAxis.m_quad, arAxis.m_quad);
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0);
    __m128 v13 = _mm_shuffle_ps(v11, v11, 170);
    XFNV_M128_F32(v13, 0) = XFNV_M128_F32(v13, 0) + v12;
    float v14 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v13, 0))));
    XFNV_M128_F32(v15, 0) = std::bit_cast<float>(static_cast<int32_t>(0.0f));
    __m128 v16 = _mm_cmpeq_ss(v13, v15);
    XFNV_M128_F32(v15, 0) = 3.0f - ((XFNV_M128_F32(v13, 0) * v14) * v14);
    __m128 v17 = _mm_setzero_ps();
    XFNV_M128_F32(v17, 0) = (0.5f * v14) * XFNV_M128_F32(v15, 0);
    __m128 v18 = _mm_andnot_ps(v16, v17);
    float afAngle = m_coneAngle;
    arAxis.m_quad = _mm_mul_ps(_mm_shuffle_ps(v18, v18, 0), arAxis.m_quad);
    v38.setAxisAngle(arAxis, afAngle);
    float afAngle_1 = 6.2831854820251465 / static_cast<double>(this->m_numSegments);
    v39.setAxisAngle(m_coneAxis, afAngle_1);
    __m128 v19 = _mm_mul_ps(p_m_coneAxis->m_quad, v38.m_vec.m_quad);
    __m128 v20 = _mm_shuffle_ps(v19, v19, 170);
    XFNV_M128_F32(v20, 0) =
        XFNV_M128_F32(v20, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v19, v19, 85), 0) + XFNV_M128_F32(v19, 0));
    __m128 v21 = _mm_shuffle_ps(v38.m_vec.m_quad, v38.m_vec.m_quad, 255);
    int32_t m_numSegments_3 = 0;
    __m128 v23 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(v38.m_vec.m_quad, v38.m_vec.m_quad, 201),
                    _mm_shuffle_ps(p_m_coneAxis->m_quad, p_m_coneAxis->m_quad, 210)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(v38.m_vec.m_quad, v38.m_vec.m_quad, 210),
                    _mm_shuffle_ps(p_m_coneAxis->m_quad, p_m_coneAxis->m_quad, 201)
                )
            ),
            v21
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v21, v21), hkQuadRealMinusHalf), p_m_coneAxis->m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v20, v20, 0), v38.m_vec.m_quad)
        )
    );
    __m128 v24 = _mm_add_ps(v23, v23);
    if (this->m_numSegments > 0) {
        int32_t v25 = 0;
        do {
            vertices.m_data[v25] = m_startPos;
            vertices.m_data[v25].m_quad = _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(m_coneHeight), _mm_set_ss(m_coneHeight), 0), v24),
                vertices.m_data[v25].m_quad
            );
            __m128 v26 = _mm_mul_ps(v39.m_vec.m_quad, v24);
            __m128 v27 = _mm_shuffle_ps(v26, v26, 170);
            XFNV_M128_F32(v27, 0) =
                XFNV_M128_F32(v27, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v26, v26, 85), 0) + XFNV_M128_F32(v26, 0));
            __m128 v28 = _mm_shuffle_ps(v39.m_vec.m_quad, v39.m_vec.m_quad, 255);
            ++m_numSegments_3;
            __m128 v29 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_shuffle_ps(v39.m_vec.m_quad, v39.m_vec.m_quad, 201), _mm_shuffle_ps(v24, v24, 210)
                        ),
                        _mm_mul_ps(
                            _mm_shuffle_ps(v39.m_vec.m_quad, v39.m_vec.m_quad, 210), _mm_shuffle_ps(v24, v24, 201)
                        )
                    ),
                    v28
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v28, v28), hkQuadRealMinusHalf), v24),
                    _mm_mul_ps(_mm_shuffle_ps(v27, v27, 0), v39.m_vec.m_quad)
                )
            );
            ++v25;
            v24 = _mm_add_ps(v29, v29);
        }
        while (m_numSegments_3 < this->m_numSegments);
    }
}
