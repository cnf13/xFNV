#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

hkDisplayGeometry::hkDisplayGeometry(hkDisplayGeometryType type) : m_geometry(nullptr), m_type(type) {
    m_transform.setIdentity();
}

// Frees the geometry with the heap's blockFree, after its destructor.
hkDisplayGeometry::~hkDisplayGeometry() {
    if (m_geometry) {
        m_geometry->~hkGeometry();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_geometry, sizeof(hkGeometry));
        m_geometry = nullptr;
    }
}

// Snaps every vertex to a grid of resolution - 1 steps over the vertices' bounds widened by 0.01. Each grid coordinate
// is truncated to an integer, then turned back into a float from its two 16-bit halves (the high half deliberately
// scaled by 1e16, not 65536) and clamped to [0, resolution - 1].
void quantize(hkGeometry& geomInOut, int32_t resolution) {
    const int32_t numVertices = geomInOut.m_vertices.m_size;
    __m128 vMin = _mm_set1_ps(HK_REAL_MAX);
    __m128 vMax = _mm_set1_ps(-HK_REAL_MAX);
    if (numVertices > 0) {
        const hkVector4* pVertex = geomInOut.m_vertices.m_data;
        for (int32_t i = numVertices; i != 0; --i) {
            const __m128 v = pVertex->m_quad;
            ++pVertex;
            vMin = _mm_min_ps(vMin, v);
            vMax = _mm_max_ps(vMax, v);
        }
    }
    const __m128 margin = _mm_set1_ps(0.01f);
    vMax = _mm_add_ps(vMax, margin);
    const float maxValue = static_cast<float>(static_cast<double>(resolution) - 1.0);
    vMin = _mm_sub_ps(vMin, margin);
    const __m128 extent = _mm_sub_ps(vMax, vMin);
    const __m128 vMaxValue = _mm_set1_ps(maxValue);
    const __m128 toWorld = _mm_div_ps(extent, vMaxValue);
    const __m128 toGrid = _mm_div_ps(vMaxValue, extent);
    if (numVertices > 0) {
        const __m128 highScale = _mm_set1_ps(1e16f);
        int32_t i = 0;
        do {
            hkVector4& vertex = geomInOut.m_vertices.m_data[i];
            alignas(16) float grid[4];
            _mm_store_ps(grid, _mm_mul_ps(_mm_sub_ps(vertex.m_quad, vMin), toGrid));
            const __m128i packed = _mm_setr_epi32(
                static_cast<int32_t>(static_cast<int64_t>(grid[0])),
                static_cast<int32_t>(static_cast<int64_t>(grid[1])),
                static_cast<int32_t>(static_cast<int64_t>(grid[2])),
                static_cast<int32_t>(static_cast<int64_t>(grid[3]))
            );
            const __m128 low = _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(packed, 16), 16));
            const __m128 value = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(packed, 16)), highScale), low);
            const __m128 clamped = _mm_min_ps(vMaxValue, _mm_max_ps(hkVector4Zero.m_quad, value));
            vertex.m_quad = _mm_add_ps(_mm_mul_ps(clamped, toWorld), vMin);
            ++i;
        }
        while (i < geomInOut.m_vertices.m_size);
    }
}
