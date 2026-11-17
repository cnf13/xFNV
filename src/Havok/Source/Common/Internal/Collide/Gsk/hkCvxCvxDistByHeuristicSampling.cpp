#include "Havok/Source/Common/Internal/Collide/Gsk/hkCvxCvxDistByHeuristicSampling.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/Collide/Gsk/hkGsk.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"

#include <cmath>
#include <xmmintrin.h>

// Samples the support points of A and B along dir (B's through aTb's rotation transposed), and keeps them in sample
// when their distance along dir is above the sample's: the sign of the sample's distance less this one decides, so a
// NaN difference with its sign set keeps the new sample too. The exe passes shapeA in ecx, aTb in esi and dir in edi
// (cdecl otherwise, ret 0) and leaves the sample's address in eax; its one caller (0xE62740) does not read it.
static void hkCvxCvxDistByHeuristicSamplingPerformSample(const hkGskShape* shapeA, const hkGskShape* shapeB,
	const hkTransform& aTb, const hkVector4& direction, hkCvxCvxDistByHeuristicSamplingSample& sampleOut) {
	hkVector4 negDir;
	negDir.m_quad = _mm_xor_ps(direction.m_quad, hkQuadRealNegateMask);

	// EXE-INLINED none: hkVector4::_setRotatedInverseDir
	const hkRotation& rotation = aTb.m_rotation;
	__m128 c0c1Lo = _mm_unpacklo_ps(rotation.m_col0.m_quad, rotation.m_col1.m_quad);
	__m128 c0c1Hi = _mm_unpackhi_ps(rotation.m_col0.m_quad, rotation.m_col1.m_quad);
	__m128 row0 = _mm_movelh_ps(c0c1Lo, rotation.m_col2.m_quad);
	__m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, c0c1Lo), rotation.m_col2.m_quad, 0xD4);
	__m128 row2 = _mm_shuffle_ps(c0c1Hi, rotation.m_col2.m_quad, 0xE4);
	hkVector4 dirB;
	dirB.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 0x55), row1),
								 _mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 0x00), row0)),
		_mm_mul_ps(_mm_shuffle_ps(direction.m_quad, direction.m_quad, 0xAA), row2));

	hkVector4 vertexAinA;
	shapeA->getSupportingVertex(negDir, &vertexAinA);
	hkVector4 vertexBinB;
	shapeB->getSupportingVertex(dirB, &vertexBinB);

	// EXE-INLINED none: hkVector4::_setTransformedPos
	hkVector4 vertexBinA;
	vertexBinA.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vertexBinB.m_quad, vertexBinB.m_quad, 0x55),
												  aTb.m_rotation.m_col1.m_quad),
									   _mm_mul_ps(_mm_shuffle_ps(vertexBinB.m_quad, vertexBinB.m_quad, 0x00),
										   aTb.m_rotation.m_col0.m_quad)),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vertexBinB.m_quad, vertexBinB.m_quad, 0xAA), aTb.m_rotation.m_col2.m_quad),
			aTb.m_translation.m_quad));

	// The distance: z + (y + x) of (vertexAinA - vertexBinA) * dir.
	__m128 products = _mm_mul_ps(_mm_sub_ps(vertexAinA.m_quad, vertexBinA.m_quad), direction.m_quad);
	__m128 distance = _mm_add_ss(_mm_shuffle_ps(products, products, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(products, products, 0x55), products));
	__m128 bestDistance =
		_mm_shuffle_ps(sampleOut.m_separatingNormalA.m_quad, sampleOut.m_separatingNormalA.m_quad, 0xFF);
	if (_mm_movemask_ps(_mm_sub_ss(bestDistance, distance)) & 1) {
		sampleOut.m_vertexAinA = vertexAinA;
		sampleOut.m_vertexBinB = vertexBinB;
		sampleOut.m_vertexBinA = vertexBinA;
		sampleOut.m_separatingNormalA = direction;
		sampleOut.getDistance() = _mm_cvtss_f32(distance);
	}
}

// KEEP: bestSample's support points and normal are read uninitialised when none of the first six samples beats its
// starting distance (a NaN or -infinity distance from degenerate shapes): the axis loop then reads the normal's x, y
// and z (0xE62848) and the end copies the points out (0xE62A90).
void hkCvxCvxDistByHeuristicSampling(const hkGskShape* shapeA, const hkGskShape* shapeB, const hkTransform& btoa,
	hkVector4* pointAinAOut, hkVector4* pointBinBOut, hkGskOut& output) {
	hkCvxCvxDistByHeuristicSamplingSample bestSample;
	bestSample.getDistance() = -3.40282e+38f;

	// The negated axes, each sampled twice.
	for (int32_t i = 0; i < 3; i++) {
		hkVector4 dir = hkTransformIdentity.m_rotation.getColumn(i);
		dir.m_quad = _mm_xor_ps(hkQuadRealNegateMask, dir.m_quad);
		hkCvxCvxDistByHeuristicSamplingPerformSample(shapeA, shapeB, btoa, dir, bestSample);
		hkCvxCvxDistByHeuristicSamplingPerformSample(shapeA, shapeB, btoa, dir, bestSample);
	}

	const __m128 zero = _mm_setzero_ps();
	const __m128 three = _mm_set_ss(3.0f);
	const __m128 half = _mm_set_ss(0.5f);
	float step = 1.0f;
	do {
		float angle = step * 0.9900000095367432;
		hkVector4 normal = bestSample.m_separatingNormalA;

		// A direction perpendicular to the normal: the two components other than the smallest of x and y, or z
		// when it is smaller still, swapped with one negated.
		float absX = static_cast<float>(std::fabs(normal(0)));
		float absY = static_cast<float>(std::fabs(normal(1)));
		float absZ = static_cast<float>(std::fabs(normal(2)));
		int32_t smallest = 0;
		int32_t i0 = 1;
		int32_t i1 = 2;
		float smallestXY = absX;
		if (absX > absY) {
			smallestXY = absY;
			i0 = 0;
			smallest = 1;
		}
		if (smallestXY > absZ) {
			i1 = smallest;
		}
		hkVector4 dir;
		dir.m_quad = zero;
		dir(i0) = normal(i1);
		dir(i1) = -normal(i0);

		// EXE-INLINED none: hkVector4::normalize3
		__m128 squares = _mm_mul_ps(dir.m_quad, dir.m_quad);
		__m128 length2 = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares));
		__m128 lengthInv = _mm_rsqrt_ss(length2);
		__m128 isZero = _mm_cmpeq_ss(length2, zero);
		__m128 scale = _mm_mul_ss(_mm_mul_ss(half, lengthInv),
			_mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(length2, lengthInv), lengthInv)));
		scale = _mm_andnot_ps(isZero, scale);
		dir.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0x00), dir.m_quad);

		__m128 angles = _mm_set1_ps(angle);
		__m128 tilt = _mm_mul_ps(angles, dir.m_quad);
		hkVector4 orthonormal0;
		orthonormal0.setCross(normal, dir);
		orthonormal0.m_quad = _mm_mul_ps(orthonormal0.m_quad, angles);

		// EXE-INLINED none: hkVector4::normalize3 (the factor of normal + tilt scales all four directions)
		__m128 tilted = _mm_add_ps(normal.m_quad, tilt);
		squares = _mm_mul_ps(tilted, tilted);
		length2 = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares));
		lengthInv = _mm_rsqrt_ss(length2);
		isZero = _mm_cmpeq_ss(length2, zero);
		scale = _mm_mul_ss(_mm_mul_ss(half, lengthInv),
			_mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(length2, lengthInv), lengthInv)));
		scale = _mm_andnot_ps(isZero, scale);
		scale = _mm_shuffle_ps(scale, scale, 0x00);

		hkVector4 dir0p;
		dir0p.m_quad = _mm_mul_ps(scale, tilted);
		hkVector4 dir0n;
		dir0n.m_quad = _mm_mul_ps(_mm_sub_ps(normal.m_quad, tilt), scale);
		hkVector4 dir1p;
		dir1p.m_quad = _mm_mul_ps(_mm_add_ps(orthonormal0.m_quad, normal.m_quad), scale);
		hkVector4 dir1n;
		dir1n.m_quad = _mm_mul_ps(_mm_sub_ps(normal.m_quad, orthonormal0.m_quad), scale);
		hkCvxCvxDistByHeuristicSamplingPerformSample(shapeA, shapeB, btoa, dir0p, bestSample);
		hkCvxCvxDistByHeuristicSamplingPerformSample(shapeA, shapeB, btoa, dir0n, bestSample);
		hkCvxCvxDistByHeuristicSamplingPerformSample(shapeA, shapeB, btoa, dir1p, bestSample);
		hkCvxCvxDistByHeuristicSamplingPerformSample(shapeA, shapeB, btoa, dir1n, bestSample);
		step = angle * 0.5;
	} while (step > 0.001f);

	*pointAinAOut = bestSample.m_vertexAinA;
	*pointBinBOut = bestSample.m_vertexBinB;
	output.m_normalInA = bestSample.m_separatingNormalA;
	output.m_pointAinA = bestSample.m_vertexAinA;
}
