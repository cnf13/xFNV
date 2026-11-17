#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkWingedEdge.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

// The polytope the penetration depth solver grows around the origin of the Minkowski difference: vertices, triangles
// and their winged edges, in fixed pools (hkpinternal.pdb, sizeof 0x2EB0; the exe's accesses below name +0 .. +0xF70).
class hkpWingedEdge {
public:
	struct Triangle;
	struct Vertex;

	// The pools' sizes (the PDB's unnamed enum; m_freeTriangle and m_triangles hold 100, m_vertex 55).
	enum : int32_t {
		HK_WINGED_EDGE_TRIANGLE_LIMIT = 100,
		HK_WINGED_EDGE_VERTICIES_LIMIT = 55,
	};

	// How a polytope operation went (the PDB's values; the exe's results are 0 and 2).
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

		// The PDB's calculateTriangleData (thiscall, ret 0, 0xE2CFE0; addTetrahedron 0xE2D440 calls it): sets
		// m_normal to the unit normal of the plane through the three edge vertices and m_distanceToOrigin to the
		// plane's distance; returns the first vertex's position (eax). hkWingedEdge's copy is 0xE67570, the same code.
		__m128* calculateTriangleData();
		// Not named in the PDB or IDA; name chosen here. Classifies the triangle against arPoint
		// by its plane's distance (normal +0, the vertex of its first edge +0x10): 2 (visible, raising
		// *apToleranceBorder to just past the distance) when *apToleranceBorder >= it, else 1 (lowering
		// *apPreviousMinimumDistance to it); 4 once *apPreviousMinimumDistance <= *apToleranceBorder (thiscall,
		// ret 0xC, 0xE63AF0). The callers store the result in m_state. hkWingedEdge's copy is the same function
		// (hkWingedEdge::Triangle::calcStateAndUpdateTolerance, called by 0xE63770 and 0xE67660).
		// Inferred type: return: hkpWingedEdge::Triangle::TriangleState: stored into hkpWingedEdge::Triangle::m_state
		// (parameter 0 of hkpWingedEdge::findVisibilityBoundaryEdges) at 0xdfb080; arPoint: hkVector4* from 0xdfb07b
		// (parameter 1 of hkpWingedEdge::findVisibilityBoundaryEdges), its members used at 0xe63b02
		hkpWingedEdge::Triangle::TriangleState calcStateAndUpdateTolerance(const hkVector4& arPoint,
			float* apToleranceBorder, float* apPreviousMinimumDistance);

		hkVector4 m_normal;
		Edge m_edge[3];
		float m_distanceToOrigin;
		TriangleState m_state;
	};

	// A point of the Minkowski difference (A - B in A's space) with the support points of A and B it came from.
	// The PDB places it in .\Collide\Gjk\Penetration\hkpConvexPenetrationUtil.cpp.
	struct Vertex {
		hkVector4 m_position;
		hkpCdVertex m_pointAinA;
		hkpCdVertex m_pointBinB;
		hkPadSpu<Edge*> m_referee;
	};

	// The PDB's findTriangleNearestOrigin (thiscall, ret 4, 0xE2D550): walks from apTriangle to the triangle nearest
	// the origin and returns it; resets the triangle states first. hkWingedEdge's copy is 0xE67AE0, the same code.
	Triangle* findTriangleNearestOrigin(Triangle* triangleOnNearestPlane);
	// Not named in the PDB or IDA; name chosen here. hkWingedEdge's copy is the same function
	// (hkWingedEdge::resetTriangleAndVertexStates, called by 0xE67AE0).
	int32_t resetTriangleAndVertexStates();
	// The PDB's findVisibleTriangles (thiscall, ret 0x10, 0xE2D0D0); hkWingedEdge's copy is 0xE67660.
	WingedEdgeStatus findVisibleTriangles(Triangle* initialTriangle, const hkVector4& pointOfView,
		hkPadSpu<int32_t>& chainLength, hkPadSpu<Edge*>& edgeChain);
	// The PDB's patchInBoundaryToPointOfViewExpansion (thiscall, ret 0xC, 0xE2D1B0;
	// tryToExpandPolytope 0xDF9C90 calls it); hkWingedEdge's copy is 0xE67740.
	int32_t patchInBoundaryToPointOfViewExpansion(int32_t chainLength, Edge* edgeChain, Vertex* pointOfView);
	// The PDB's addTetrahedron (thiscall, ret 0, 0xE2D440; the one caller 0xDFAF80 ignores
	// eax); hkWingedEdge's copy is 0xE679D0.
	void addTetrahedron();
	// The PDB's findVisibilityBoundaryEdges (thiscall, ret 0x10, 0xDFB040): clears the two
	// outputs, sets m_toleranceBorder (+0) to 1e-7 and m_previousMinimumDistance (+4) to FLT_MAX, classifies
	// apTriangle with calcStateAndUpdateTolerance and, when it sees arPoint, collects the boundary through findVisibleTriangles;
	// else returns 2.
	WingedEdgeStatus findVisibilityBoundaryEdges(Triangle* initialTriangle, const hkVector4& pointOfView,
		hkPadSpu<int32_t>& chainLength, hkPadSpu<Edge*>& edgeChain);
	// The triangle of m_triangles (+0xF70, stride 0x50, m_numTriangles at +0x10) with the
	// least m_distanceToOrigin (thiscall, ret 0, 0xDFB0B0; Havok SDK symbol, hkCollide.lib, tied by bytes; the
	// SDK's findTriangleNearestOrigin takes a triangle and pops 4 bytes). hkWingedEdge's copy is the same function
	// (hkWingedEdge::findTriangleOnNearestPlane, called by 0xE63770).
	Triangle* findTriangleOnNearestPlane();
	// The PDB's initializePolytopeExpansion (thiscall, ret 0xC, 0xDFAF80; calculatePenetrationDepth 0xDFA940 and
	// 0xDFABA0 call it): as hkWingedEdge::initializePolytopeExpansion (0xE63BA0, the same 184 bytes), with
	// addTetrahedron (0xE2D440).
	void initializePolytopeExpansion(const hkpCdVertex* apPointsAinA, const hkpCdVertex* apPointsBinB,
		const hkVector4* apSimplex);

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
