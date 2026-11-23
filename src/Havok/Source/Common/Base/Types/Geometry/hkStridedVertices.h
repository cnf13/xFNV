#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <cstdint>

// A view of vertex data: the first float of the first vertex, the vertex count and the stride between vertices.
class hkStridedVertices {
public:
    void set(const hkArrayBase<hkVector4>& vertices) {
        set(vertices.begin(), vertices.getSize());
    }

    template <typename T>
    void set(const T* vertices, int32_t numVertices) {
        m_vertices = reinterpret_cast<const float*>(vertices);
        m_numVertices = numVertices;
        m_striding = sizeof(T);
    }

    const float* m_vertices;
    int32_t m_numVertices;
    int32_t m_striding;
};
