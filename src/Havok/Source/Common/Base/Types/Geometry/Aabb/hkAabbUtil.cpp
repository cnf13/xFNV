#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"

#include <xmmintrin.h>

// Each corner takes the point's component when it is the smaller or larger, else the box's, then both w components are
// cleared. The empty box's corners are HK_REAL_MAX splatted. Each point is read through the array again after the store
// before it (a box over its own points sees the store).
hkAabb* hkAabbUtil::calcAabb(const hkVector4* const* vectorArray, int32_t numVertices, hkAabb& aabbOut) {
	if (numVertices == 0) {
		aabbOut.m_min.m_quad[0] = 3.40282e38f;
		aabbOut.m_min.m_quad[1] = 3.40282e38f;
		aabbOut.m_min.m_quad[2] = 3.40282e38f;
		aabbOut.m_min.m_quad[3] = 3.40282e38f;
		aabbOut.m_max.m_quad[0] = -3.40282e38f;
		aabbOut.m_max.m_quad[1] = -3.40282e38f;
		aabbOut.m_max.m_quad[2] = -3.40282e38f;
		aabbOut.m_max.m_quad[3] = -3.40282e38f;
		return &aabbOut;
	}
	aabbOut.m_min.m_quad = vectorArray[0]->m_quad;
	aabbOut.m_max.m_quad = vectorArray[0]->m_quad;
	for (int32_t i = 1; i < numVertices; ++i) {
		const hkQuadReal kPoint = vectorArray[i]->m_quad;
		const hkQuadReal kMin = aabbOut.m_min.m_quad;
		hkQuadReal kNewMin;
		kNewMin[0] = kPoint[0] < kMin[0] ? kPoint[0] : kMin[0];
		kNewMin[1] = kPoint[1] < kMin[1] ? kPoint[1] : kMin[1];
		kNewMin[2] = kPoint[2] < kMin[2] ? kPoint[2] : kMin[2];
		kNewMin[3] = kPoint[3] < kMin[3] ? kPoint[3] : kMin[3];
		const hkQuadReal kMax = aabbOut.m_max.m_quad;
		aabbOut.m_min.m_quad = kNewMin;
		const hkQuadReal kPointAgain = vectorArray[i]->m_quad;
		hkQuadReal kNewMax;
		kNewMax[0] = kPointAgain[0] > kMax[0] ? kPointAgain[0] : kMax[0];
		kNewMax[1] = kPointAgain[1] > kMax[1] ? kPointAgain[1] : kMax[1];
		kNewMax[2] = kPointAgain[2] > kMax[2] ? kPointAgain[2] : kMax[2];
		kNewMax[3] = kPointAgain[3] > kMax[3] ? kPointAgain[3] : kMax[3];
		aabbOut.m_max.m_quad = kNewMax;
	}
	aabbOut.m_min.m_quad[3] = 0.0f;
	aabbOut.m_max.m_quad[3] = 0.0f;
	return &aabbOut;
}

hkAabb* hkAabbUtil::calcAabb(const hkVector4* vertices, int32_t numVertices, hkAabb& aabbOut) {
	if (numVertices) {
		__m128 m_quad = vertices->m_quad;
		__m128 v5 = vertices->m_quad;
		if (numVertices > 1) {
			const hkVector4* v6 = vertices + 1;
			int32_t v7 = numVertices - 1;
			do {
				__m128 v8 = v6->m_quad;
				++v6;
				--v7;
				m_quad = _mm_min_ps(m_quad, v8);
				v5 = _mm_max_ps(v5, v8);
			} while (v7);
		}
		aabbOut.m_min.m_quad = _mm_and_ps(rdata_10C4C10, m_quad);
		aabbOut.m_max.m_quad = _mm_and_ps(rdata_10C4C10, v5);
		return &aabbOut;
	} else {
		aabbOut.m_min.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
		aabbOut.m_max.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
		return &aabbOut;
	}
}

// calcAabb over strided float points: min/max over each point's x, y and z, then the xyz mask (setXYZ0). The point's w
// is masked out of both results, so it is written as 0.
hkAabb* hkAabbUtil::calcAabb(const float* vertexArray, int32_t numVertices, int32_t striding, hkAabb& aabbOut) {
	hkVector4 point;
	point.set(vertexArray[0], vertexArray[1], vertexArray[2], 0.0f);
	hkQuadReal vMin = point.m_quad;
	hkQuadReal vMax = point.m_quad;
	const float* pVertex = vertexArray;
	for (int32_t i = 1; i < numVertices; i++) {
		// EXE-INLINED none: hkAddByteOffsetConst (the points are raw bytes striding apart, as the exe steps them)
		pVertex = reinterpret_cast<const float*>(reinterpret_cast<const char*>(pVertex) + striding);
		point.set(pVertex[0], pVertex[1], pVertex[2], 0.0f);
		vMin = _mm_min_ps(vMin, point.m_quad);
		vMax = _mm_max_ps(vMax, point.m_quad);
	}
	aabbOut.m_min.m_quad = _mm_and_ps(rdata_10C4C10, vMin);
	aabbOut.m_max.m_quad = _mm_and_ps(rdata_10C4C10, vMax);
	return &aabbOut;
}

void hkAabbUtil::calcAabb(const hkTransform& localToWorld, const hkVector4& halfExtents, const hkVector4& center,
	float extraRadius, hkAabb& aabbOut) {
	hkVector4 v5;
	hkVector4 v6;
	v5.m_quad = localToWorld.m_rotation.m_col1.m_quad;
	v6.m_quad = localToWorld.m_rotation.m_col2.m_quad;
	__m128 v7 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(center.m_quad, center.m_quad, 0),
										  localToWorld.m_rotation.m_col0.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(center.m_quad, center.m_quad, 85), v5.m_quad)),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(center.m_quad, center.m_quad, 170), v6.m_quad),
			localToWorld.m_translation.m_quad));
	__m128 v8 =
		_mm_add_ps(_mm_add_ps(_mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 0),
											 localToWorld.m_rotation.m_col0.m_quad),
								  hkVector4_setAbs4_absMask),
					   _mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 85), v5.m_quad),
						   hkVector4_setAbs4_absMask)),
			_mm_add_ps(_mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 170), v6.m_quad),
						   hkVector4_setAbs4_absMask),
				_mm_shuffle_ps(_mm_set_ss(extraRadius), _mm_set_ss(extraRadius), 0)));
	aabbOut.m_max.m_quad = _mm_add_ps(v7, v8);
	aabbOut.m_min.m_quad = _mm_add_ps(_mm_xor_ps(v8, hkQuadRealNegateMask), v7);
}

void hkAabbUtil::calcAabb(const hkTransform& BvToWorld, const hkVector4& halfExtents, float extraRadius,
	hkAabb& aabbOut) {
	hkVector4 v4;
	v4.m_quad = BvToWorld.m_translation.m_quad;
	__m128 v5 = _mm_add_ps(_mm_add_ps(_mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 85),
													 BvToWorld.m_rotation.m_col1.m_quad),
										  hkVector4_setAbs4_absMask),
							   _mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 0),
											  BvToWorld.m_rotation.m_col0.m_quad),
								   hkVector4_setAbs4_absMask)),
		_mm_add_ps(_mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(halfExtents.m_quad, halfExtents.m_quad, 170),
								  BvToWorld.m_rotation.m_col2.m_quad),
					   hkVector4_setAbs4_absMask),
			_mm_shuffle_ps(_mm_set_ss(extraRadius), _mm_set_ss(extraRadius), 0)));
	aabbOut.m_max.m_quad = _mm_add_ps(v4.m_quad, v5);
	aabbOut.m_min.m_quad = _mm_add_ps(_mm_xor_ps(v5, hkQuadRealNegateMask), v4.m_quad);
}
