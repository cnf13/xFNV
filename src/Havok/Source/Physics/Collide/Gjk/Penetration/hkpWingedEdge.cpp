#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpWingedEdge.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

__m128* hkpWingedEdge::Triangle::calculateTriangleData() {
	hkVector4 v1;
	hkVector4 v5;
	__m128 v10 = _mm_undefined_ps();
	hkVector4 v14;
	v1.m_quad = m_edge[1].m_vertex->m_position.m_quad;
	__m128* result = &m_edge[0u].m_vertex->m_position.m_quad;
	__m128 v3 = _mm_sub_ps(*result, v1.m_quad);
	__m128 v4 = _mm_sub_ps(v1.m_quad, m_edge[2].m_vertex->m_position.m_quad);
	v5.m_quad = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v4, v4, 210), _mm_shuffle_ps(v3, v3, 201)),
		_mm_mul_ps(_mm_shuffle_ps(v4, v4, 201), _mm_shuffle_ps(v3, v3, 210)));
	__m128 v6 = _mm_mul_ps(v5.m_quad, v5.m_quad);
	float v7 = _mm_shuffle_ps(v6, v6, 85)[0] + v6[0];
	__m128 v8 = _mm_shuffle_ps(v6, v6, 170);
	v8[0] = v8[0] + v7;
	float v9 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v8[0])));
	v10[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
	__m128 v11 = _mm_cmpeq_ss(v8, v10);
	v10[0] = 3.0f - static_cast<float>(static_cast<float>(v8[0] * v9) * v9);
	__m128 v12 = _mm_setzero_ps();
	v12[0] = static_cast<float>(0.5f * v9) * v10[0];
	__m128 v13 = _mm_andnot_ps(v11, v12);
	v14.m_quad = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v5.m_quad);
	m_normal.m_quad = v5.m_quad;
	m_normal.m_quad = v14.m_quad;
	__m128 v15 = _mm_mul_ps(v14.m_quad, *result);
	m_distanceToOrigin =
		_mm_shuffle_ps(v15, v15, 170)[0] + static_cast<float>(_mm_shuffle_ps(v15, v15, 85)[0] + v15[0]);
	return result;
}

// From apTriangle, steps across each edge but the one it came over whose plane through the edge and the triangle's
// normal has the edge's start on the inside side by more than a tolerance, unless apTriangle lies beyond the origin and
// the neighbour is not nearer than 0.9 of apTriangle's distance; stops at a triangle with no such edge, or at one it
// visited already, and returns it.
hkpWingedEdge::Triangle* hkpWingedEdge::findTriangleNearestOrigin(Triangle* triangleOnNearestPlane) {
	resetTriangleAndVertexStates();
	Triangle* pTriangle = triangleOnNearestPlane;
	Edge* pLastEdge = nullptr;
	bool bFound = false;
	while (!bFound) {
		pTriangle->m_state = Triangle::HK_FIND_NEAREST_TRIANGLE_VISITED;
		int32_t i = 0;
		for (; i < 3; ++i) {
			Edge* pEdge = &pTriangle->m_edge[i];
			if (pEdge == pLastEdge) {
				continue;
			}
			const __m128 kStart = pEdge->m_vertex->m_position.m_quad;
			const __m128 kEdge = _mm_sub_ps(pEdge->m_next->m_vertex->m_position.m_quad, kStart);
			const __m128 kLen2 = _mm_mul_ps(kEdge, kEdge);
			const float fLength2 = _mm_shuffle_ps(kLen2, kLen2, 170)[0] +
				static_cast<float>(_mm_shuffle_ps(kLen2, kLen2, 85)[0] + kLen2[0]);
			const __m128 kNormal = pTriangle->m_normal.m_quad;
			const __m128 kDot =
				_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kEdge, kEdge, 201),
										  _mm_shuffle_ps(kNormal, kNormal, 210)),
							   _mm_mul_ps(_mm_shuffle_ps(kEdge, kEdge, 210), _mm_shuffle_ps(kNormal, kNormal, 201))),
					kStart);
			const float fDist =
				_mm_shuffle_ps(kDot, kDot, 170)[0] + static_cast<float>(_mm_shuffle_ps(kDot, kDot, 85)[0] + kDot[0]);
			const float fAbsDist = std::fabs(static_cast<double>(fDist));
			const float fTolerance = fLength2 * -9.99999949475751e-11;
			if (!(fTolerance > fDist * fAbsDist)) {
				continue;
			}
			Triangle* pNeighbour = pEdge->m_neighbour->m_triangle;
			if (0.0 < triangleOnNearestPlane->m_distanceToOrigin &&
				triangleOnNearestPlane->m_distanceToOrigin * 0.8999999761581421 > pNeighbour->m_distanceToOrigin) {
				continue;
			}
			pLastEdge = pEdge->m_neighbour;
			pTriangle = pNeighbour;
			bFound = pNeighbour->m_state == Triangle::HK_FIND_NEAREST_TRIANGLE_VISITED;
			break;
		}
		if (i == 3) {
			return pTriangle;
		}
	}
	return pTriangle;
}

// Marks every triangle unvisited and clears every vertex's referee, and returns the vertex count reached. Indexed by
// element, because the element strides differ between 32-bit and 64-bit builds.
int32_t hkpWingedEdge::resetTriangleAndVertexStates() {
	for (int32_t iTriangle = 0; iTriangle < m_numTriangles.m_storage; ++iTriangle) {
		m_triangles[iTriangle].m_state = Triangle::HK_NOT_VISITED;
	}
	int32_t iVertex = 0;
	for (; iVertex < m_numVertices.m_storage; ++iVertex) {
		m_vertex[iVertex].m_referee.m_storage = nullptr;
	}
	return iVertex;
}

hkpWingedEdge::WingedEdgeStatus hkpWingedEdge::findVisibleTriangles(hkpWingedEdge::Triangle* initialTriangle,
	const hkVector4& pointOfView, hkPadSpu<int32_t>& chainLength, hkPadSpu<hkpWingedEdge::Edge*>& edgeChain) {
	hkpWingedEdge::WingedEdgeStatus VisibleTriangles = hkpWingedEdge::HK_WINGED_EDGE_OK;
	int32_t v13 = 0;
	hkpWingedEdge::Edge** p_m_neighbour = &initialTriangle->m_edge[0].m_neighbour;
	while (1) {
		hkpWingedEdge::Edge* v7 = *p_m_neighbour;
		hkpWingedEdge::Triangle* m_triangle = (*p_m_neighbour)->m_triangle;
		hkpWingedEdge::Triangle::TriangleState m_state = m_triangle->m_state;
		if (m_state == hkpWingedEdge::Triangle::HK_NOT_VISITED) {
			m_state = m_triangle->calcStateAndUpdateTolerance(pointOfView, &m_toleranceBorder.m_storage,
				&m_previousMinimumDistance.m_storage);
			m_triangle->m_state = m_state;
			if (m_state == hkpWingedEdge::Triangle::HK_VISIBLE) {
				VisibleTriangles = findVisibleTriangles(m_triangle, pointOfView, chainLength, edgeChain);
				if (VisibleTriangles == hkpWingedEdge::HK_WINGED_EDGE_FAIL) {
					goto LABEL_12;
				}
				goto LABEL_9;
			}
			if (m_state == hkpWingedEdge::Triangle::HK_UNKNOWN) {
				break;
			}
		}
		if (m_state == hkpWingedEdge::Triangle::HK_INVISIBLE) {
			v7->m_vertex->m_referee.m_storage = v7;
			++chainLength.m_storage;
			edgeChain.m_storage = v7;
		}
	LABEL_9:;
		p_m_neighbour += 4;
		if (++v13 >= 3) {
			goto LABEL_12;
		}
	}
	VisibleTriangles = hkpWingedEdge::HK_WINGED_EDGE_FAIL;
LABEL_12:;
	initialTriangle->m_distanceToOrigin = 3.40282e+38f;
	int32_t m_storage = m_numFreeTriangle.m_storage;
	if (m_storage >= 100) {
		return hkpWingedEdge::HK_WINGED_EDGE_FAIL;
	}
	m_freeTriangle[m_storage] = initialTriangle;
	++m_numFreeTriangle.m_storage;
	return VisibleTriangles;
}

// Takes the new triangles off the free stack first, then from the end of m_triangles, and fans them from apNewVertex
// over the boundary edges, following each edge's end vertex's m_referee to the next boundary edge.
int32_t hkpWingedEdge::patchInBoundaryToPointOfViewExpansion(int32_t chainLength, Edge* edgeChain,
	Vertex* pointOfView) {
	Triangle* apNewTriangles[HK_WINGED_EDGE_TRIANGLE_LIMIT];
	int32_t iNumNew = 0;
	int32_t iFree = m_numFreeTriangle.m_storage - 1;
	int32_t iRemaining = chainLength;
	while (iRemaining != 0 && iFree >= 0) {
		Triangle* pFree = m_freeTriangle[iFree];
		--m_numFreeTriangle.m_storage;
		apNewTriangles[iNumNew] = pFree;
		--iRemaining;
		++iNumNew;
		--iFree;
	}
	if (iRemaining > 0) {
		int32_t iTriangle = m_numTriangles.m_storage;
		const int32_t iNumTriangles = iTriangle + iRemaining;
		Triangle** ppNew = &apNewTriangles[iNumNew];
		do {
			*ppNew++ = &m_triangles[iTriangle++];
		} while (--iRemaining != 0);
		m_numTriangles.m_storage = iNumTriangles;
	}
	Edge* pEdge = edgeChain;
	for (int32_t i = 0; i < chainLength;) {
		if (pEdge == nullptr) {
			return HK_WINGED_EDGE_FAIL;
		}
		Triangle* pTriangle = apNewTriangles[i];
		Triangle* pNext = i + 1 < chainLength ? apNewTriangles[i + 1] : apNewTriangles[0];
		++i;
		pTriangle->m_edge[0].m_triangle = pTriangle;
		pTriangle->m_edge[1].m_triangle = pTriangle;
		pTriangle->m_edge[2].m_triangle = pTriangle;
		pTriangle->m_edge[0].m_next = &pTriangle->m_edge[1];
		pTriangle->m_edge[1].m_next = &pTriangle->m_edge[2];
		pTriangle->m_edge[2].m_next = &pTriangle->m_edge[0];
		Vertex* pEnd = pEdge->m_next->m_vertex;
		Vertex* pStart = pEdge->m_vertex;
		pTriangle->m_edge[0].m_vertex = pEnd;
		pTriangle->m_edge[2].m_vertex = pointOfView;
		pTriangle->m_edge[1].m_vertex = pStart;
		pEdge->m_neighbour = &pTriangle->m_edge[0];
		pTriangle->m_edge[0].m_neighbour = pEdge;
		pNext->m_edge[1].m_neighbour = &pTriangle->m_edge[2];
		pTriangle->m_edge[2].m_neighbour = &pNext->m_edge[1];
		// The plane through the end, the start and the new vertex: its normal's squared length is summed as z + (y +
		// x); a zero length fails, else one Newton step of the reciprocal square root estimate gives the unit normal.
		__m128 vEnd = pEnd->m_position.m_quad;
		__m128 vStart = pStart->m_position.m_quad;
		__m128 vStartEnd = _mm_sub_ps(vEnd, vStart);
		__m128 vNewStart = _mm_sub_ps(vStart, pointOfView->m_position.m_quad);
		__m128 vNormal = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(vNewStart, vNewStart, 0xD2),
										_mm_shuffle_ps(vStartEnd, vStartEnd, 0xC9)),
			_mm_mul_ps(_mm_shuffle_ps(vNewStart, vNewStart, 0xC9), _mm_shuffle_ps(vStartEnd, vStartEnd, 0xD2)));
		__m128 vSquares = _mm_mul_ps(vNormal, vNormal);
		float fXY = _mm_shuffle_ps(vSquares, vSquares, 0x55)[0] + vSquares[0];
		__m128 vLengthSq = _mm_shuffle_ps(vSquares, vSquares, 0xAA);
		vLengthSq[0] = vLengthSq[0] + fXY;
		if (0.0f == vLengthSq[0]) {
			return HK_WINGED_EDGE_FAIL;
		}
		float fRsqrt = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(vLengthSq[0])));
		__m128 vIsZero = _mm_cmpeq_ss(vLengthSq, _mm_setzero_ps());
		float fThree = 3.0f - static_cast<float>(static_cast<float>(vLengthSq[0] * fRsqrt) * fRsqrt);
		__m128 vScale = _mm_setzero_ps();
		vScale[0] = static_cast<float>(0.5f * fRsqrt) * fThree;
		vScale = _mm_andnot_ps(vIsZero, vScale);
		__m128 vUnit = _mm_mul_ps(_mm_shuffle_ps(vScale, vScale, 0), vNormal);
		__m128 vProducts = _mm_mul_ps(vEnd, vUnit);
		pTriangle->m_distanceToOrigin = _mm_shuffle_ps(vProducts, vProducts, 0xAA)[0] +
			static_cast<float>(_mm_shuffle_ps(vProducts, vProducts, 0x55)[0] + vProducts[0]);
		pTriangle->m_normal.m_quad = vUnit;
		Vertex* pEndVertex = pEdge->m_next->m_vertex;
		pEdge = pEndVertex->m_referee.m_storage;
		pEndVertex->m_referee.m_storage = nullptr;
	}
	// EXE-INLINED 0xE67FD0
	for (int32_t i = 0; i < m_numTriangles.m_storage; ++i) {
		m_triangles[i].m_state = Triangle::HK_NOT_VISITED;
	}
	for (int32_t i = 0; i < m_numVertices.m_storage; ++i) {
		m_vertex[i].m_referee.m_storage = nullptr;
	}
	return pEdge != edgeChain ? HK_WINGED_EDGE_FAIL : HK_WINGED_EDGE_OK;
}

// Links the four start triangles through topologyData, then orients the tetrahedron: when the first vertex lies on
// the positive side of the fourth triangle's plane, the first two vertices swap places in the triangles that use them.
void hkpWingedEdge::addTetrahedron() {
	m_numTriangles.m_storage = 4;
	const int32_t* pTopology = topologyData[0];
	for (Triangle* pTriangle = m_triangles; pTriangle != &m_triangles[4]; ++pTriangle) {
		pTriangle->m_edge[0].m_next = &pTriangle->m_edge[1];
		pTriangle->m_edge[1].m_next = &pTriangle->m_edge[2];
		pTriangle->m_state = Triangle::HK_NOT_VISITED;
		pTriangle->m_edge[2].m_next = &pTriangle->m_edge[0];
		for (Edge* pEdge = pTriangle->m_edge; pEdge < &pTriangle->m_edge[3]; ++pEdge, pTopology += 3) {
			pEdge->m_triangle = pTriangle;
			// topologyData holds 32-bit byte offsets (Vertex 0x40, Triangle 0x50 and Edge 0x10 apart), which are
			// decoded into indices because Triangle and Edge are larger on x64.
			pEdge->m_vertex = &m_vertex[pTopology[0] / 0x40];
			pEdge->m_neighbour = &m_triangles[pTopology[1] / 0x50].m_edge[pTopology[2] / 0x10];
		}
	}
	m_triangles[3].calculateTriangleData();
	__m128 vProducts = _mm_mul_ps(_mm_sub_ps(m_vertex[0].m_position.m_quad, m_vertex[1].m_position.m_quad),
		m_triangles[3].m_normal.m_quad);
	float fDistance = _mm_shuffle_ps(vProducts, vProducts, 0xAA)[0] +
		static_cast<float>(_mm_shuffle_ps(vProducts, vProducts, 0x55)[0] + vProducts[0]);
	if (fDistance > 0.0f) {
		m_triangles[2].m_edge[0].m_vertex = &m_vertex[1];
		m_triangles[1].m_edge[0].m_vertex = &m_vertex[1];
		m_triangles[0].m_edge[0].m_vertex = &m_vertex[1];
		m_triangles[3].m_edge[0].m_vertex = &m_vertex[0];
		m_triangles[1].m_edge[1].m_vertex = &m_vertex[0];
		m_triangles[0].m_edge[2].m_vertex = &m_vertex[0];
	}
	for (int32_t i = 0; i < 4; ++i) {
		m_triangles[i].calculateTriangleData();
	}
}

hkpWingedEdge::Triangle* hkpWingedEdge::findTriangleOnNearestPlane() {
	hkpWingedEdge::Triangle* m_triangles = this->m_triangles;
	float m_distanceToOrigin = this->m_triangles[0].m_distanceToOrigin;
	int32_t v2 = m_numTriangles.m_storage - 2;
	uint32_t v3 = v2 + 1;
	hkpWingedEdge::Triangle* v4 = m_triangles;
	if (v2 + 1 >= 4) {
		uint32_t v5 = v3 >> 2;
		v2 -= 4 * (v3 >> 2);
		do {
			float v9 = m_triangles[1].m_distanceToOrigin;
			if (m_distanceToOrigin > static_cast<double>(v9)) {
				m_distanceToOrigin = v9;
				v4 = m_triangles + 1;
			}
			float v10 = m_triangles[2].m_distanceToOrigin;
			if (m_distanceToOrigin > static_cast<double>(v10)) {
				m_distanceToOrigin = v10;
				v4 = m_triangles + 2;
			}
			float v11 = m_triangles[3].m_distanceToOrigin;
			if (m_distanceToOrigin > static_cast<double>(v11)) {
				m_distanceToOrigin = v11;
				v4 = m_triangles + 3;
			}
			float v12 = m_triangles[4].m_distanceToOrigin;
			if (m_distanceToOrigin > static_cast<double>(v12)) {
				m_distanceToOrigin = v12;
				v4 = m_triangles + 4;
			}
			m_triangles += 4;
			--v5;
		} while (v5);
	}
	for (; v2 >= 0; --v2) {
		double v6 = m_triangles[1].m_distanceToOrigin;
		++m_triangles;
		float v13 = v6;
		if (m_distanceToOrigin > static_cast<double>(v13)) {
			m_distanceToOrigin = v6;
			v4 = m_triangles;
		}
	}
	return v4;
}

void hkpWingedEdge::initializePolytopeExpansion(const hkpCdVertex* apPointsAinA, const hkpCdVertex* apPointsBinB,
	const hkVector4* apSimplex) {
	m_numVertices.m_storage = 4;
	m_vertex[0].m_pointAinA.m_quad = apPointsAinA->m_quad;
	m_vertex[0].m_pointBinB.m_quad = apPointsBinB->m_quad;
	m_vertex[0].m_position.m_quad = apSimplex->m_quad;
	m_vertex[0].m_referee.m_storage = 0;
	m_vertex[1].m_pointAinA.m_quad = apPointsAinA[1].m_quad;
	m_vertex[1].m_pointBinB.m_quad = apPointsBinB[1].m_quad;
	m_vertex[1].m_position.m_quad = apSimplex[1].m_quad;
	m_vertex[1].m_referee.m_storage = 0;
	m_vertex[2].m_pointAinA.m_quad = apPointsAinA[2].m_quad;
	m_vertex[2].m_pointBinB.m_quad = apPointsBinB[2].m_quad;
	m_vertex[2].m_position.m_quad = apSimplex[2].m_quad;
	m_vertex[2].m_referee.m_storage = 0;
	m_vertex[3].m_pointAinA.m_quad = apPointsAinA[3].m_quad;
	m_vertex[3].m_pointBinB.m_quad = apPointsBinB[3].m_quad;
	m_vertex[3].m_position.m_quad = apSimplex[3].m_quad;
	m_vertex[3].m_referee.m_storage = 0;
	addTetrahedron();
	m_numFreeTriangle.m_storage = 0;
}
