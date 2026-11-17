#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkWingedEdge.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"

#include <stdint.h>
#include <xmmintrin.h>

// The polytope the penetration depth solver grows around the origin of the Minkowski difference: vertices, triangles
// and their winged edges, in fixed pools.
class hkpWingedEdge {
public:
    struct Triangle;
    struct Vertex;

    // The pools' sizes (m_freeTriangle and m_triangles hold 100, m_vertex 55).
    enum : int32_t {
        HK_WINGED_EDGE_TRIANGLE_LIMIT = 100,
        HK_WINGED_EDGE_VERTICIES_LIMIT = 55,
    };

    // How a polytope operation went.
    enum WingedEdgeStatus : int32_t {
        HK_WINGED_EDGE_OK = 0,
        HK_WINGED_EDGE_OUT_OF_MEMORY = 1,
        HK_WINGED_EDGE_FAIL = 2,
    };

    // One side of a triangle: its start vertex, the next edge of the triangle, the edge of the neighbouring
    // triangle that shares it, and the triangle.
    struct Edge {
        Vertex* m_vertex;
        Edge* m_next;
        Edge* m_neighbour;
        Triangle* m_triangle;
    };

    // A face of the polytope: its plane and its distance from the origin.
    struct Triangle {
        enum TriangleState : int32_t {
            HK_NOT_VISITED = 0,
            HK_VISIBLE = 1,
            HK_INVISIBLE = 2,
            HK_FIND_NEAREST_TRIANGLE_VISITED = 3,
            HK_UNKNOWN = 4,
        };

        // Sets m_normal to the unit normal of the plane through the three edge vertices and m_distanceToOrigin to the
        // plane's distance; returns the first vertex's position.
        __m128* calculateTriangleData();
        // Classifies the triangle against arPoint by its plane's distance (from m_normal and the vertex of its first
        // edge): 2 (visible, raising *apToleranceBorder to just past the distance) when *apToleranceBorder >= it, else
        // 1 (lowering *apPreviousMinimumDistance to it); 4 once *apPreviousMinimumDistance <= *apToleranceBorder. The
        // callers store the result in m_state. hkWingedEdge::Triangle::calcStateAndUpdateTolerance is the same
        // function.
        hkpWingedEdge::Triangle::TriangleState calcStateAndUpdateTolerance(
            const hkVector4& arPoint,
            float* apToleranceBorder,
            float* apPreviousMinimumDistance
        );

        hkVector4 m_normal;
        Edge m_edge[3];
        float m_distanceToOrigin;
        TriangleState m_state;
    };

    // A point of the Minkowski difference (A - B in A's space) with the support points of A and B it came from.
    struct Vertex {
        hkVector4 m_position;
        hkpCdVertex m_pointAinA;
        hkpCdVertex m_pointBinB;
        hkPadSpu<Edge*> m_referee;
    };

    // Walks from triangleOnNearestPlane to the triangle nearest the origin and returns it; resets the triangle states
    // first.
    Triangle* findTriangleNearestOrigin(Triangle* triangleOnNearestPlane);
    int32_t resetTriangleAndVertexStates();
    WingedEdgeStatus findVisibleTriangles(
        Triangle* initialTriangle,
        const hkVector4& pointOfView,
        hkPadSpu<int32_t>& chainLength,
        hkPadSpu<Edge*>& edgeChain
    );
    int32_t patchInBoundaryToPointOfViewExpansion(int32_t chainLength, Edge* edgeChain, Vertex* pointOfView);
    void addTetrahedron();
    // Clears the two outputs, sets m_toleranceBorder to 1e-7 and m_previousMinimumDistance to FLT_MAX, classifies
    // initialTriangle with calcStateAndUpdateTolerance and, when it sees pointOfView, collects the boundary through
    // findVisibleTriangles; else returns 2.
    WingedEdgeStatus findVisibilityBoundaryEdges(
        Triangle* initialTriangle,
        const hkVector4& pointOfView,
        hkPadSpu<int32_t>& chainLength,
        hkPadSpu<Edge*>& edgeChain
    );
    // Returns the triangle of m_triangles with the least m_distanceToOrigin.
    Triangle* findTriangleOnNearestPlane();
    // Called by calculatePenetrationDepth. Same as hkWingedEdge::initializePolytopeExpansion, with addTetrahedron.
    void initializePolytopeExpansion(
        const hkpCdVertex* apPointsAinA,
        const hkpCdVertex* apPointsBinB,
        const hkVector4* apSimplex
    );

    // The bounds of hkpWingedEdge.cpp's code (empty markers).
    static void codeBegin();
    static void codeEnd();

    // Set to 1e-7 and FLT_MAX by findVisibilityBoundaryEdges before it classifies a triangle.
    hkPadSpu<float> m_toleranceBorder;
    hkPadSpu<float> m_previousMinimumDistance;
    // initializePolytopeExpansion sets it to 4.
    hkPadSpu<int32_t> m_numVertices;
    hkPadSpu<int32_t> m_numFreeTriangle;
    // addTetrahedron sets it to 4.
    hkPadSpu<int32_t> m_numTriangles;
    // The vertices from 0x20 (16-byte aligned), 0x40 bytes apart.
    Vertex m_vertex[55];
    // The freed triangles patchInBoundaryToPointOfViewExpansion takes first.
    Triangle* m_freeTriangle[100];
    // The triangles from 0xF70, 0x50 bytes apart.
    Triangle m_triangles[100];
};
