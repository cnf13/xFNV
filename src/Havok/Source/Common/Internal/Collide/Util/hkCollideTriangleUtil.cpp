#include "Havok/Source/Common/Internal/Collide/Util/hkCollideTriangleUtil.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

float* calcTriangleBarycentricParams(__m128* a1, float* a2) {
	__m128 v2 = a1[1];
	__m128 v3 = _mm_sub_ps(*a1, v2);
	__m128 v4 = _mm_sub_ps(a1[2], v2);
	__m128 v5 = _mm_mul_ps(v3, v3);
	float v14 = _mm_shuffle_ps(v5, v5, 170)[0] + static_cast<float>(_mm_shuffle_ps(v5, v5, 85)[0] + v5[0]);
	__m128 v6 = _mm_mul_ps(v4, v4);
	float v7 = _mm_shuffle_ps(v6, v6, 170)[0] + static_cast<float>(_mm_shuffle_ps(v6, v6, 85)[0] + v6[0]);
	__m128 v8 = _mm_mul_ps(v4, v3);
	float v13 = _mm_shuffle_ps(v8, v8, 170)[0] + static_cast<float>(_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]);
	float v15 = v14 * v7;
	__m128 v10 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v4, v4, 210), _mm_shuffle_ps(v3, v3, 201)),
		_mm_mul_ps(_mm_shuffle_ps(v4, v4, 201), _mm_shuffle_ps(v3, v3, 210)));
	__m128 v11 = _mm_mul_ps(v10, v10);
	double v12 = v15;
	float v16 = v13 * v13;
	float v17 = v12 - v16;
	float v18 = 1.0 / v17;
	*a2 = v14 * v18;
	a2[1] = v7 * v18;
	a2[2] = v13 * v18;
	a2[3] = 1.0 /
		_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v11, v11, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]))))));
	return a2;
}

__m128* calcTriangleInvEdgeLengths(__m128* a1, __m128* a2) {
	__m128 v17;
	__m128 v2 = a1[1];
	__m128 v3 = _mm_sub_ps(*a1, a1[2]);
	__m128 v4 = _mm_sub_ps(a1[2], v2);
	__m128 v5 = _mm_sub_ps(v2, *a1);
	__m128 v6 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, 201), _mm_shuffle_ps(v4, v4, 210));
	__m128 v7 = _mm_shuffle_ps(v3, v3, 210);
	__m128 v8 = _mm_shuffle_ps(v4, v4, 201);
	__m128 v9 = _mm_mul_ps(v4, v4);
	__m128 v10 = _mm_mul_ps(v3, v3);
	__m128 v11 = _mm_sub_ps(_mm_mul_ps(v7, v8), v6);
	__m128 v13 = _mm_mul_ps(v11, v11);
	__m128 v14 = _mm_mul_ps(v5, v5);
	v17[0] = _mm_shuffle_ps(v9, v9, 170)[0] + static_cast<float>(_mm_shuffle_ps(v9, v9, 85)[0] + v9[0]);
	v17[1] = _mm_shuffle_ps(v10, v10, 170)[0] + static_cast<float>(_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]);
	v17[2] = _mm_shuffle_ps(v14, v14, 170)[0] + static_cast<float>(_mm_shuffle_ps(v14, v14, 85)[0] + v14[0]);
	float v16 = _mm_shuffle_ps(v13, v13, 170)[0] + static_cast<float>(_mm_shuffle_ps(v13, v13, 85)[0] + v13[0]);
	v17[3] = v16;
	__m128 v15 = _mm_rsqrt_ps(v17);
	__m128 v18 =
		_mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v15, v17), v15)), _mm_mul_ps(hkQuadRealHalf, v15));
	*a2 = v18;
	a2[1][0] = v16 * v18[3];
	return a2;
}

hkCollideTriangleUtil::ClosestPointTriangleStatus hkCollideTriangleUtil::closestPointTriangle(const hkVector4& position,
	const hkVector4* tri, const hkCollideTriangleUtil::ClosestPointTriangleCache& cache,
	hkCollideTriangleUtil::ClosestPointTriangleResult& result, hkFeatureOutput* featureOutput) {
	hkLineSegmentUtil::ClosestPointLineSegResult a4;
	if (featureOutput) {
		featureOutput->numFeatures = 0;
	}
	__m128 m_quad = tri[1].m_quad;
	__m128 v6 = _mm_sub_ps(m_quad, position.m_quad);
	__m128 v7 = _mm_sub_ps(tri->m_quad, m_quad);
	__m128 v8 = _mm_mul_ps(v7, v6);
	float v9 = _mm_shuffle_ps(v8, v8, 85)[0] + v8[0];
	float v10 = _mm_shuffle_ps(v8, v8, 170)[0];
	__m128 v11 = _mm_sub_ps(tri[2].m_quad, m_quad);
	float v40 = v10 + v9;
	__m128 v12 = _mm_mul_ps(v11, v6);
	float v13 = _mm_shuffle_ps(v12, v12, 170)[0] + static_cast<float>(_mm_shuffle_ps(v12, v12, 85)[0] + v12[0]);
	float a3b = cache.m_QR * v13 - cache.m_RR * v40;
	float v41 = v40 * cache.m_QR - v13 * cache.m_QQ;
	a4.m_pointOnEdge.m_quad[0] = a3b + 0.0010000000474974513;
	a4.m_pointOnEdge.m_quad[1] = 1.0010000000474975 - a3b - v41;
	a4.m_pointOnEdge.m_quad[2] = v41 + 0.0010000000474974513;
	a4.m_pointOnEdge.m_quad[3] = 0.001f;
	int32_t n7 = _mm_movemask_ps(_mm_cmplt_ps(hkVector4Zero.m_quad, a4.m_pointOnEdge.m_quad)) & 7;
	if (n7 == 7) {
		hkVector4 v15;
		v15.m_quad = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v11, v11, 210), _mm_shuffle_ps(v7, v7, 201)),
			_mm_mul_ps(_mm_shuffle_ps(v11, v11, 201), _mm_shuffle_ps(v7, v7, 210)));
		result.hitDirection.m_quad = v15.m_quad;
		__m128 m_quad_1 =
			_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(cache.m_invTriNormal), _mm_set_ss(cache.m_invTriNormal), 0),
				v15.m_quad);
		__m128 v17 = _mm_mul_ps(v6, m_quad_1);
		float distance =
			_mm_shuffle_ps(v17, v17, 170)[0] + static_cast<float>(_mm_shuffle_ps(v17, v17, 85)[0] + v17[0]);
		result.distance.m_storage = distance;
		result.hitDirection.m_quad = m_quad_1;
		if (!(distance > 0.0f)) {
			result.distance.m_storage = -distance;
		} else {
			result.hitDirection.m_quad = _mm_xor_ps(m_quad_1, hkQuadRealNegateMask);
		}
		if (featureOutput) {
			featureOutput->numFeatures = 3;
			featureOutput->featureIds[0u] = 0;
			featureOutput->featureIds[1] = 1;
			featureOutput->featureIds[2] = 2;
		}
		return hkCollideTriangleUtil::HIT_TRIANGLE_FACE;
	} else {
		int32_t v20 = hkCollideTriangleUtil::maskToIndex[n7];
		if (v20 >= 0) {
			hkLineSegmentUtil::ClosestPointLineSegResult v50;
			int32_t n8_1 = hkLineSegmentUtil::closestPointLineSeg(position, tri[v20],
				tri[hkCollideTriangleUtil::vertexToEdgeLut[v20 + 2]], a4);
			int32_t n8 = hkLineSegmentUtil::closestPointLineSeg(position,
				tri[hkCollideTriangleUtil::vertexToEdgeLut[v20]], tri[v20], v50);
			__m128 v34 = _mm_sub_ps(position.m_quad, v50.m_pointOnEdge.m_quad);
			__m128 v51 = _mm_sub_ps(position.m_quad, a4.m_pointOnEdge.m_quad);
			__m128 v35 = _mm_mul_ps(v51, v51);
			float v36 = _mm_shuffle_ps(v35, v35, 85)[0] + v35[0];
			float v37 = _mm_shuffle_ps(v35, v35, 170)[0];
			__m128 v38 = _mm_mul_ps(v34, v34);
			float v39 = v37 + v36;
			float a3 = _mm_shuffle_ps(v38, v38, 170)[0] + static_cast<float>(_mm_shuffle_ps(v38, v38, 85)[0] + v38[0]);
			if (!(a3 > static_cast<double>(v39))) {
				float v48 = std::sqrt((double)a3);
				float v42 = 1.0 / v48;
				if (featureOutput) {
					if (n8 == 4) {
						featureOutput->numFeatures = 1;
						featureOutput->featureIds[0u] = v20;
					} else {
						if (n8 == 8) {
							featureOutput->numFeatures = 1;
							featureOutput->featureIds[0u] = hkCollideTriangleUtil::vertexToEdgeLut[v20];
						} else {
							featureOutput->numFeatures = 2;
							featureOutput->featureIds[0u] = v20;
							featureOutput->featureIds[1] = hkCollideTriangleUtil::vertexToEdgeLut[v20];
						}
					}
				}
				result.distance.m_storage = a3 * v42;
				result.hitDirection.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v42), _mm_set_ss(v42), 0), v34);
				return hkCollideTriangleUtil::HIT_TRIANGLE_EDGE;
			} else {
				float a3c = std::sqrt((double)v39);
				float a3a = 1.0 / a3c;
				if (featureOutput) {
					if (n8_1 == 4) {
						featureOutput->numFeatures = 1;
						featureOutput->featureIds[0u] = v20;
					} else {
						if (n8_1 == 8) {
							featureOutput->numFeatures = 1;
							featureOutput->featureIds[0u] = hkCollideTriangleUtil::vertexToEdgeLut[v20 + 2];
						} else {
							featureOutput->numFeatures = 2;
							featureOutput->featureIds[0u] = v20;
							featureOutput->featureIds[1] = hkCollideTriangleUtil::vertexToEdgeLut[v20 + 2];
						}
					}
				}
				result.distance.m_storage = v39 * a3a;
				result.hitDirection.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(a3a), _mm_set_ss(a3a), 0), v51);
				return hkCollideTriangleUtil::HIT_TRIANGLE_EDGE;
			}
		} else {
			__m128 v28 = _mm_undefined_ps();
			int32_t v21 = hkCollideTriangleUtil::vertexToEdgeLut[v20 + 8];
			int32_t v22 = v20 + 8;
			int32_t n8_2 = hkLineSegmentUtil::closestPointLineSeg(position,
				tri[hkCollideTriangleUtil::vertexToEdgeLut[v22 + 2]], tri[v21], a4);
			if (featureOutput) {
				if (n8_2 == 4) {
					featureOutput->featureIds[0u] = hkCollideTriangleUtil::vertexToEdgeLut[v22];
					featureOutput->numFeatures = 1;
				} else {
					if (n8_2 == 8) {
						featureOutput->featureIds[0u] = hkCollideTriangleUtil::vertexToEdgeLut[v22 + 2];
						featureOutput->numFeatures = 1;
					} else {
						featureOutput->featureIds[0u] = hkCollideTriangleUtil::vertexToEdgeLut[v22];
						featureOutput->featureIds[1] = hkCollideTriangleUtil::vertexToEdgeLut[v22 + 2];
						featureOutput->numFeatures = 2;
					}
				}
			}
			__m128 v24 = _mm_sub_ps(position.m_quad, a4.m_pointOnEdge.m_quad);
			__m128 v25 = _mm_mul_ps(v24, v24);
			__m128 v26 = _mm_shuffle_ps(v25, v25, 170);
			v26[0] = v26[0] + static_cast<float>(_mm_shuffle_ps(v25, v25, 85)[0] + v25[0]);
			float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v26[0])));
			v28[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
			__m128 v29 = _mm_cmpeq_ss(v26, v28);
			v28[0] = 3.0f - static_cast<float>(static_cast<float>(v26[0] * v27) * v27);
			__m128 v30 = _mm_setzero_ps();
			v30[0] = static_cast<float>(0.5f * v27) * v28[0];
			__m128 v31 = _mm_andnot_ps(v29, v30);
			__m128 v32 = _mm_shuffle_ps(v31, v31, 0);
			result.hitDirection.m_quad = _mm_mul_ps(v24, v32);
			result.distance.m_storage = static_cast<float>(_mm_shuffle_ps(v25, v25, 170)[0] +
											static_cast<float>(_mm_shuffle_ps(v25, v25, 85)[0] + v25[0])) *
				v32[0];
			return hkCollideTriangleUtil::HIT_TRIANGLE_EDGE;
		}
	}
}
