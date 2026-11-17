#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"

#include <xmmintrin.h>

// The exe inlines hkVector4::setTransformedPos (0xC7F640), the transposed rotation and the negation: they are
// written in the exe's SSE order, w lanes included, since the vectors go to the shapes' virtuals.
void hkpGskCache::init(const hkpConvexShape* shapeA, const hkpConvexShape* shapeB, const hkTransform& aTb) {
	uint16_t ausVertexIds[2] = {0, 0};
	hkpCdVertex kVertexA;
	hkpCdVertex kVertexB;
	shapeA->convertVertexIdsToVerticesImpl(ausVertexIds, 1, &kVertexA);
	shapeB->convertVertexIdsToVerticesImpl(ausVertexIds, 1, &kVertexB);

	const hkQuadReal& rB = kVertexB.m_quad;
	const hkQuadReal& rC0 = aTb.m_rotation.m_col0.m_quad;
	const hkQuadReal& rC1 = aTb.m_rotation.m_col1.m_quad;
	const hkQuadReal& rC2 = aTb.m_rotation.m_col2.m_quad;
	const hkQuadReal& rT = aTb.m_translation.m_quad;
	// EXE-INLINED 0xC7F640
	hkVector4 kVertexBInA;
	kVertexBInA.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(rB, rB, 0x55), rC1),
										_mm_mul_ps(_mm_shuffle_ps(rB, rB, 0x00), rC0)),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(rB, rB, 0xAA), rC2), rT));

	hkVector4 kDirection;
	kDirection.m_quad = _mm_sub_ps(kVertexBInA.m_quad, kVertexA.m_quad);
	hkpCdVertex kSupportA;
	shapeA->getSupportingVertexImpl(kDirection, kSupportA);

	const hkQuadReal kDiff = _mm_sub_ps(kVertexBInA.m_quad, kSupportA.m_quad);
	// The transposed rows the exe's unpck/movlh/movhl/shufps build: row0 = (c0x, c1x, c2x, c2y),
	// row1 = (c0y, c1y, c2y, c2w), row2 = (c0z, c1z, c2z, c2w).
	const hkQuadReal kLow = _mm_unpacklo_ps(rC0, rC1);
	const hkQuadReal kHigh = _mm_unpackhi_ps(rC0, rC1);
	const hkQuadReal kRow0 = _mm_movelh_ps(kLow, rC2);
	const hkQuadReal kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLow), rC2, 0xD4);
	const hkQuadReal kRow2 = _mm_shuffle_ps(kHigh, rC2, 0xE4);
	const hkQuadReal kRotated = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kDiff, kDiff, 0x55), kRow1),
											   _mm_mul_ps(_mm_shuffle_ps(kDiff, kDiff, 0x00), kRow0)),
		_mm_mul_ps(kRow2, _mm_shuffle_ps(kDiff, kDiff, 0xAA)));
	hkVector4 kDirectionInB;
	kDirectionInB.m_quad = _mm_xor_ps(kRotated, hkQuadRealNegateMask);
	hkpCdVertex kSupportB;
	shapeB->getSupportingVertexImpl(kDirectionInB, kSupportB);

	m_vertices[1] = kSupportB.getId();
	m_vertices[0] = kSupportA.getId();
	m_vertices[3] = 0;
	m_vertices[2] = 0;
	m_dimA = 1;
	m_dimB = 1;
	int32_t iSpheresA = shapeA->getNumCollisionSpheresImpl();
	int32_t iSpheresB = shapeB->getNumCollisionSpheresImpl();
	if (iSpheresA > 3) {
		iSpheresA = 0xF;
	}
	m_maxDimA = static_cast<uint8_t>(iSpheresA);
	if (iSpheresB > 3) {
		iSpheresB = 0xF;
	}
	m_maxDimB = static_cast<uint8_t>(iSpheresB);
	m_gskFlags = 0;
}

// An extruded triangle takes the general init. Otherwise B's three vertices start the cache, and A's vertex is its
// support along the triangle's normal turned towards A's first vertex. The exe inlines the transform of the vertices
// into A's space (last vertex first), the edges' cross product and the dot product.
void hkpGskCache::initTriangle(const hkpConvexShape* shapeA, const hkpTriangleShape* shapeB, const hkTransform& btoa) {
	if (shapeB->m_isExtruded) {
		init(shapeA, shapeB, btoa);
		return;
	}
	m_vertices[1] = 0;
	m_vertices[2] = 1;
	m_vertices[3] = 2;

	const hkQuadReal& rC0 = btoa.m_rotation.m_col0.m_quad;
	const hkQuadReal& rC1 = btoa.m_rotation.m_col1.m_quad;
	const hkQuadReal& rC2 = btoa.m_rotation.m_col2.m_quad;
	const hkQuadReal& rT = btoa.m_translation.m_quad;
	// EXE-INLINED none: hkVector4Util::transformPoints
	hkVector4 vertsInA[3];
	for (int32_t i = 2; i >= 0; --i) {
		const hkQuadReal& v = shapeB->getVertex(i).m_quad;
		vertsInA[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), rC1),
											_mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), rC0)),
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), rC2), rT));
	}
	hkVector4 edge0;
	edge0.setSub4(vertsInA[1], vertsInA[0]);
	hkVector4 edge1;
	edge1.setSub4(vertsInA[2], vertsInA[1]);
	hkVector4 normal;
	normal.setCross(edge0, edge1);

	hkpCdVertex vertexA;
	shapeA->convertVertexIdsToVerticesImpl(&m_vertices[1], 1, &vertexA);
	// EXE-INLINED 0x62B0A0
	hkQuadReal products = _mm_mul_ps(_mm_sub_ps(vertexA.m_quad, vertsInA[0].m_quad), normal.m_quad);
	hkQuadReal distance = _mm_add_ss(_mm_shuffle_ps(products, products, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(products, products, 0x55), products));
	hkVector4 direction;
	direction.m_quad =
		_mm_xor_ps(_mm_and_ps(_mm_shuffle_ps(distance, distance, 0x00), hkQuadRealNegateMask), normal.m_quad);
	hkpCdVertex support;
	shapeA->getSupportingVertexImpl(direction, support);
	m_vertices[0] = static_cast<uint16_t>(support.getId());
	m_dimA = 1;
	m_dimB = 3;
	int32_t numSpheresA = shapeA->getNumCollisionSpheresImpl();
	if (numSpheresA > 3) {
		numSpheresA = 0xF;
	}
	m_maxDimA = static_cast<uint8_t>(numSpheresA);
	m_maxDimB = 3;
	m_gskFlags = 0;
}
