#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <xmmintrin.h>

// hkInternal's copy of the polytope the penetration depth solver grows (hkpWingedEdge's twin on plain vectors). It is
// the one hkConvexPenetrationUtil's calculatePenetrationDepth runs through initializePolytopeExpansion,
// findVisibleTriangles and addTetrahedron.
class hkWingedEdge {
public:
    struct Triangle;
    struct Vertex;

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

    // One side of a triangle: its start vertex, the next edge of the triangle, the edge of the neighbouring triangle
    // that shares it, and the triangle.
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
        TriangleState calcStateAndUpdateTolerance(
            const hkVector4& pointOfView,
            float* toleranceBorder,
            float* previousMinimumDistance
        );

        hkVector4 m_normal;
        Edge m_edge[3];
        float m_distanceToOrigin;
        TriangleState m_state;
    };

    // A point of the Minkowski difference with the support points of A and B it came from.
    struct Vertex {
        hkVector4 m_position;
        hkVector4 m_pointAinA;
        hkVector4 m_pointBinB;
        hkPadSpu<Edge*> m_referee;
    };

    Triangle* findTriangleOnNearestPlane();
    // Resets the states, then walks from triangleOnNearestPlane to the triangle nearest the origin and returns it.
    Triangle* findTriangleNearestOrigin(Triangle* triangleOnNearestPlane);
    // Takes chainLength triangles off m_freeTriangle, then m_triangles, and joins each boundary edge from edgeChain to
    // pointOfView. Returns 0, or 2 when the boundary walk ends early (no next edge, a degenerate new triangle) or does
    // not come back to edgeChain.
    int32_t patchInBoundaryToPointOfViewExpansion(int32_t chainLength, Edge* edgeChain, Vertex* pointOfView);
    // Frees initialTriangle onto m_freeTriangle after visiting its neighbours: those that see pointOfView recursively,
    // the others' shared edges counted in chainLength and kept in edgeChain. Returns 2 when the free stack is full or a
    // neighbour reports 4.
    int32_t findVisibleTriangles(
        Triangle* initialTriangle,
        const hkVector4& pointOfView,
        int32_t& chainLength,
        Edge*& edgeChain
    );
    // Called by calculatePenetrationDepth. Sets m_numVertices to 4, fills the four vertices from the simplex and the
    // support points with no referee, runs addTetrahedron and clears m_numFreeTriangle.
    void initializePolytopeExpansion(
        const hkVector4* apPointsAinA,
        const hkVector4* apPointsBinB,
        const hkVector4* apSimplex
    );
    int32_t resetTriangleAndVertexStates();
    // Sets m_numTriangles to 4 and links the first four triangles' edges to the vertices through a fixed table.
    void addTetrahedron();

    static void find2ClosestMultiTriangles(
        hkWingedEdge& we0,
        hkWingedEdge& we1,
        hkVector4& directionOut,
        Triangle*& nearestTriangle0,
        Triangle*& nearestTriangle1
    );
    // The bounds of hkWingedEdge.cpp's code (empty markers).
    static void codeBegin();
    static void codeEnd();

    hkPadSpu<float> m_toleranceBorder;
    hkPadSpu<float> m_previousMinimumDistance;
    hkPadSpu<int32_t> m_numVertices;
    hkPadSpu<int32_t> m_numFreeTriangle;
    hkPadSpu<int32_t> m_numTriangles;
    Vertex m_vertex[HK_WINGED_EDGE_VERTICIES_LIMIT];
    Triangle* m_freeTriangle[HK_WINGED_EDGE_TRIANGLE_LIMIT];
    Triangle m_triangles[HK_WINGED_EDGE_TRIANGLE_LIMIT];
};

// topologyData is addTetrahedron's table, shared by hkWingedEdge's and hkpWingedEdge's addTetrahedron. One row per edge
// of the four start triangles, three rows per triangle in order, as byte offsets: the edge's start vertex from
// m_vertex, then its neighbour edge from m_triangles[0].m_edge as a triangle offset plus an edge offset (the values are
// index * 0x40, index * 0x50 and index * 0x10).
using topologyDataArray = int32_t[12][3];
inline topologyDataArray topologyData{{0, 160, 32},  {128, 240},  {64, 80},       {0, 0, 32},
                                      {64, 240, 32}, {192, 160},  {0, 80, 32},    {192, 240, 16},
                                      {128},         {64, 0, 16}, {128, 160, 16}, {192, 80, 16}};
