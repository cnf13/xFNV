#pragma once

#include <cstdint>

// A view of vertex data: the first float of the first vertex, the vertex count and the stride between vertices.
class hkStridedVertices {
public:
    const float* m_vertices;
    int32_t m_numVertices;
    int32_t m_striding;
};
