#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

// hkInternal's copy of the polytope the penetration depth solver grows (hkpWingedEdge's twin on plain vectors; the
// release hkinternal.pdb's members). The exe's copy is the one hkConvexPenetrationUtil's calculatePenetrationDepth
// (0xE63770) runs: initializePolytopeExpansion 0xE63BA0, findVisibleTriangles 0xE67660, addTetrahedron 0xE679D0.
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

		// The PDB's calculateTriangleData (thiscall, ret 0, 0xE67570; addTetrahedron 0xE679D0 calls it): sets
		// m_normal to the unit normal of the plane through the three edge vertices and m_distanceToOrigin to the
		// plane's distance; returns the first vertex's position (eax). hkpWingedEdge's copy is 0xE2CFE0, the same
		// code.
		__m128* calculateTriangleData();
		// The PDB's calcStateAndUpdateTolerance; the signature is the registered copy's.
		TriangleState calcStateAndUpdateTolerance(const hkVector4& pointOfView, float* toleranceBorder,
			float* previousMinimumDistance);

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
	// The PDB's findTriangleNearestOrigin (thiscall, ret 4, 0xE67AE0; hkpWingedEdge's copy 0xE2D550 is the same 355
	// bytes; the one caller is calculatePenetrationDepth 0xE63770). Resets the states, then walks from apTriangle to
	// the triangle nearest the origin and returns it.
	Triangle* findTriangleNearestOrigin(Triangle* triangleOnNearestPlane);
	// The PDB's patchInBoundaryToPointOfViewExpansion (thiscall, ret 0xC, 0xE67740; hkpWingedEdge's copy 0xE2D1B0 is
	// the same 643 bytes). Takes aiNumBoundaryEdges triangles off m_freeTriangle, then m_triangles (+0xF70, stride
	// 0x50), and joins each boundary edge from apBoundaryEdge to apNewVertex. Returns 0, or 2 when the boundary walk
	// ends early (no next edge, a degenerate new triangle) or does not come back to apBoundaryEdge.
	int32_t patchInBoundaryToPointOfViewExpansion(int32_t chainLength, Edge* edgeChain, Vertex* pointOfView);
	// The PDB's findVisibleTriangles (protected there; calculatePenetrationDepth 0xE63770 calls it, the exe having
	// inlined findVisibilityBoundaryEdges; thiscall, ret 0x10, 0xE67660; hkpWingedEdge's copy 0xE2D0D0 is the same 214
	// bytes). Frees apTriangle onto m_freeTriangle (+0xDE0, count +0xC) after visiting its neighbours (edge +0x18,
	// stride 0x10): those that see arPoint recursively, the others' shared edges counted in arNumBoundaryEdges and
	// kept in arpBoundaryEdge. Returns 2 when the free stack is full or a neighbour reports 4.
	int32_t findVisibleTriangles(Triangle* initialTriangle, const hkVector4& pointOfView, int32_t& chainLength,
		Edge*& edgeChain);
	// The PDB's initializePolytopeExpansion (protected there; calculatePenetrationDepth 0xE63770 calls it; thiscall,
	// ret 0xC, 0xE63BA0; hkpWingedEdge's copy 0xDFAF80 is the same 184 bytes): sets m_numVertices (+8) to 4, fills
	// the four vertices (+0x20, stride 0x40) from the simplex and the support points with no referee, runs
	// addTetrahedron and clears m_numFreeTriangle (+0xC).
	void initializePolytopeExpansion(const hkVector4* apPointsAinA, const hkVector4* apPointsBinB,
		const hkVector4* apSimplex);
	// The PDB's resetTriangleAndVertexStates (protected there); the signature is the registered copy's.
	int32_t resetTriangleAndVertexStates();
	// The PDB's addTetrahedron (thiscall, ret 0, 0xE679D0; hkpWingedEdge's copy 0xE2D440 is the same 263 bytes).
	// Sets m_numTriangles (+0x10) to 4 and links the first four triangles' edges to the vertices through the table
	// at 0x10ED0F0; the one caller 0xE63BA0 ignores eax.
	void addTetrahedron();

	// The PDB's find2ClosestMultiTriangles (static; the PDB's (hkWingedEdge&, hkWingedEdge&, hkVector4&,
	// Triangle*&, Triangle*&); cdecl, ret 0, 0xE67C50; the exe has no caller). It lies among hkWingedEdge's
	// functions (0xE67570..0xE67FD0).
	static void find2ClosestMultiTriangles(hkWingedEdge& we0, hkWingedEdge& we1, hkVector4& directionOut,
		Triangle*& nearestTriangle0, Triangle*& nearestTriangle1);
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

// The PDB's topologyData (addTetrahedron's local static, const int[12][3], in both hkWingedEdge's and hkpWingedEdge's
// addTetrahedron; the exe keeps one copy, which 0xE679D0 and 0xE2D440 both read). One row per edge of the four start
// triangles, three rows per triangle in order, as byte offsets in the exe's layout: the edge's start vertex from
// m_vertex, then its neighbour edge from m_triangles[0].m_edge as a triangle offset plus an edge offset (the exe holds
// index * 0x40, index * 0x50 and index * 0x10).
using topologyDataArray = int32_t[12][3];
inline topologyDataArray topologyData{{0, 160, 32}, {128, 240}, {64, 80}, {0, 0, 32}, {64, 240, 32}, {192, 160},
	{0, 80, 32}, {192, 240, 16}, {128}, {64, 0, 16}, {128, 160, 16}, {192, 80, 16}};
