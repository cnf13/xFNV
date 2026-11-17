#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"

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
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <bit>
#include <xmmintrin.h>

hkDisplaySemiCircle::hkDisplaySemiCircle()
  : hkDisplayGeometry(HK_DISPLAY_SEMICIRCLE), m_thetaMin(0.0f), m_thetaMax(0.0f), m_radius(0.0f), m_numSegments(0) {
    m_normal.setZero4();
    m_perp.setZero4();
    m_center.setZero4();
}

// Natively: hkDisplaySemiCircle(center, normal, startPerp, thetaMin, thetaMax, radius, numSegments) below.
hkDisplaySemiCircle::hkDisplaySemiCircle(
    const hkVector4& center,
    const hkVector4& normal,
    const hkVector4& startPerp,
    const float thetaMin,
    const float thetaMax,
    const float radius,
    const int32_t numSegments
)
  : hkDisplayGeometry(HK_DISPLAY_SEMICIRCLE), m_normal(normal), m_perp(startPerp), m_center(center),
    m_thetaMin(thetaMin), m_thetaMax(thetaMax), m_radius(radius), m_numSegments(numSegments) {}

// Two line points per segment, from the semicircle's points: segment i runs from point i to point i + 1.
void hkDisplaySemiCircle::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
    using points_t = hkArray<hkVector4, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(points_t, points, std::nullopt, std::nullopt);
    points.m_data = 0;
    points.m_size = 0;
    points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
    generatePoints(points);
    int32_t numLinePoints = 2 * m_numSegments;
    int32_t capacity = lines.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (capacity < numLinePoints) {
        int32_t newCapacity = 2 * capacity;
        if (numLinePoints >= newCapacity) {
            newCapacity = 2 * m_numSegments;
        }
        hkArrayUtil::_reserve(a, &lines, newCapacity, sizeof(hkVector4));
    }
    lines.m_size = numLinePoints;
    for (int32_t i = 0; i < m_numSegments; ++i) {
        lines.m_data[2 * i] = points.m_data[i];
        lines.m_data[2 * i + 1] = points.m_data[i + 1];
    }
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    points.m_size = 0;
    if (points.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            points.m_data, sizeof(hkVector4) * (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)
        );
    }
}

void hkDisplaySemiCircle::buildGeometry() {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkGeometry* v3 = static_cast<hkGeometry*>(Instance->m_heap->blockAlloc(sizeof(hkGeometry)));
    int32_t v4 = 0;
    if (v3) {
        v3->m_vertices.m_data = 0;
        v3->m_vertices.m_size = 0;
        v3->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        v3->m_triangles.m_data = 0;
        v3->m_triangles.m_size = 0;
        v3->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
    }
    else {
        v3 = 0;
    }
    this->m_geometry = v3;
    generatePoints(v3->m_vertices);
    hkGeometry* m_geometry = this->m_geometry;
    hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
    if (m_geometry->m_vertices.m_size ==
        (m_geometry->m_vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v6->m_heap, m_geometry, sizeof(hkVector4));
    }
    int32_t v7 = 0;
    m_geometry->m_vertices.m_data[m_geometry->m_vertices.m_size++] = m_center;
    for (int32_t i = this->m_geometry->m_vertices.m_size - 1; v7 < m_numSegments; ++v4) {
        hkGeometry::Triangle* v9 = &this->m_geometry->m_triangles.m_data[v4];
        v9->m_c = v7++;
        v9->m_a = i;
        v9->m_b = v7;
        v9->m_material = -1;
    }
}

void hkDisplaySemiCircle::generatePoints(hkArray<hkVector4, hkContainerHeapAllocator>& points) {
    hkVector4 v9;
    hkVector4 v16;
    hkRotation a;
    hkVector4* p_m_normal = &m_normal;
    float afAngle = (m_thetaMax - m_thetaMin) / static_cast<double>(m_numSegments);
    a.setAxisAngle(m_normal, m_thetaMin);
    v16.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(m_radius), _mm_set_ss(m_radius), 0), m_perp.m_quad);
    v16.setRotatedDir(a, v16);
    v16.m_quad = _mm_add_ps(m_center.m_quad, v16.m_quad);
    a.setAxisAngle(*p_m_normal, afAngle);
    int32_t v4 = m_numSegments + 2;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v6 = points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v6 < v4) {
        int32_t v7 = 2 * v6;
        if (v4 >= v7) {
            v7 = v4;
        }
        hkArrayUtil::_reserve(*m_heap, &points, v7, sizeof(hkVector4));
    }
    hkVector4* m_data = (&points)->begin();
    v9.m_quad = v16.m_quad;
    points.m_size = v4;
    int32_t v10 = 0;
    (*m_data).m_quad = v9.m_quad;
    if (m_numSegments >= 0) {
        bool v13;
        __m128 m_quad = v16.m_quad;
        int32_t v15 = 0;
        do {
            hkVector4 v17;
            v17.m_quad = _mm_sub_ps(m_quad, m_center.m_quad);
            v17.setRotatedDir(a, v17);
            hkVector4* v12 = (&points)->begin();
            v17.m_quad = _mm_add_ps(m_center.m_quad, v17.m_quad);
            v12[v15 + 1].m_quad = v17.m_quad;
            m_quad = v17.m_quad;
            v13 = ++v10 <= m_numSegments;
            v16.m_quad = v17.m_quad;
            ++v15;
        }
        while (v13);
    }
}

hkVector4* hkDisplaySemiCircle::getNormal(hkVector4* apResult) {
    apResult->m_quad[0] = m_normal.m_quad[0];
    apResult->m_quad[1] = m_normal.m_quad[1];
    float z = m_normal.m_quad[2];
    float w = m_normal.m_quad[3];
    apResult->m_quad[2] = z;
    apResult->m_quad[3] = w;
    return apResult;
}

hkVector4* hkDisplaySemiCircle::getPerp(hkVector4* apResult) {
    apResult->m_quad[0] = m_perp.m_quad[0];
    apResult->m_quad[1] = m_perp.m_quad[1];
    float z = m_perp.m_quad[2];
    float w = m_perp.m_quad[3];
    apResult->m_quad[2] = z;
    apResult->m_quad[3] = w;
    return apResult;
}

float hkDisplaySemiCircle::getThetaMin() {
    return m_thetaMin;
}

float hkDisplaySemiCircle::getThetaMax() {
    return m_thetaMax;
}

float hkDisplaySemiCircle::getRadius() {
    return m_radius;
}

int32_t hkDisplaySemiCircle::getNumSegments() {
    return m_numSegments;
}

hkVector4* hkDisplaySemiCircle::getCenter(hkVector4* apResult) {
    apResult->m_quad[0] = m_center.m_quad[0];
    apResult->m_quad[1] = m_center.m_quad[1];
    float z = m_center.m_quad[2];
    float w = m_center.m_quad[3];
    apResult->m_quad[2] = z;
    apResult->m_quad[3] = w;
    return apResult;
}

void hkDisplaySemiCircle::setParameters(
    const float radius,
    const float thetaMin,
    const float thetaMax,
    const int32_t numSegments,
    const hkVector4& center,
    const hkVector4& normal,
    const hkVector4& startPerp
) {
    hkVector4 v8;
    m_center.m_quad = center.m_quad;
    m_normal.m_quad = normal.m_quad;
    v8.m_quad = startPerp.m_quad;
    m_thetaMin = thetaMin;
    m_perp.m_quad = v8.m_quad;
    m_thetaMax = thetaMax;
    m_numSegments = numSegments;
    m_radius = radius;
}
