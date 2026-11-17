#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <bit>
#include <cmath>
#include <new>
#include <xmmintrin.h>

hkQuaternion hkQuaternionIdentity(hkVector4(0.0f, 0.0f, 0.0f, 1.0f));

hkQuaternion::hkQuaternion(const hkRotation& r) {
	set(r);
}

void hkQuaternion::set(const hkRotation& r) {
	quaternionFromRotatation(&r, this);
}

void hkQuaternion::setAndNormalize(const hkRotation& r) {
	quaternionFromRotatation(&r, this);
	normalize();
}

// The exe forms the half angle in x87 double precision (fmul qword 0.5 at 0x1011588) and narrows it to float, then
// loads that float into ST(0) for the CRT's intrinsic entry points __CIsin 0xECA0A0 and _CIcos 0xEC9F70 (std::sin
// and std::cos). Each result is narrowed to float. The scaled axis goes out as one 16-byte store (mulps of the
// sine broadcast over the axis) and w is then overwritten by the cosine. The exe's frame has no /GS cookie.
FNVD_NO_STACK_COOKIE void hkQuaternion::setAxisAngle(const hkVector4& axis, float angle) {
	const float fHalfAngle = angle * 0.5;
	const float fSin = static_cast<float>(std::sin(fHalfAngle));
	_mm_store_ps(&reinterpret_cast<float*>(&m_vec.m_quad)[0],
		_mm_mul_ps(_mm_set1_ps(fSin), _mm_load_ps(&reinterpret_cast<const float*>(&axis.m_quad)[0])));
	m_vec.m_quad[3] = static_cast<float>(std::cos(fHalfAngle));
}

// The cosine is the four-lane dot product (two shufps/addps rounds), taken positive with the sign kept for q1's
// weight. At or above 0.999 (a qword constant; a NaN goes this way too) the weights are 1 - t and t; otherwise the
// angle comes from hkMath::acos and the weights from __CIsin over sin(theta) through __CIsqrt, each intermediate
// stored as a float as the exe does. 1 - c * c is one x87 expression in the exe (the product is exact), written
// here in double. The result is normalized inline with the SDK's sqrtInverse: rsqrtss, one Newton step
// 0.5 * r * (3 - x * r * r) and the zero mask from cmpeqss. The exe's scalar steps up to the weights are x87; the
// dot products, the weighting and the normalization are SSE1. The exe's frame has no /GS cookie.
FNVD_NO_STACK_COOKIE void hkQuaternion::setSlerp(const hkQuaternion& q0, const hkQuaternion& q1, float t) {
	const __m128 vQ0 = _mm_load_ps(&reinterpret_cast<const float*>(&q0.m_vec.m_quad)[0]);
	const __m128 vQ1 = _mm_load_ps(&reinterpret_cast<const float*>(&q1.m_vec.m_quad)[0]);
	__m128 vDot = _mm_mul_ps(vQ1, vQ0);
	vDot = _mm_add_ps(_mm_shuffle_ps(vDot, vDot, 0x4E), vDot);
	vDot = _mm_add_ps(_mm_shuffle_ps(vDot, vDot, 0xB1), vDot);
	float fCosTheta = _mm_cvtss_f32(vDot);
	float fSign = 1.0f;
	if (0.0f > fCosTheta) {
		fCosTheta = -fCosTheta;
		fSign = -1.0f;
	}
	__m128 vResult;
	if (fCosTheta < 0.9989999999525025) {
		const float fTheta = hkMath::acos(fCosTheta);
		const float fSinSq = static_cast<float>(1.0 - static_cast<double>(fCosTheta) * fCosTheta);
		const float fSinTheta = std::sqrt(fSinSq);
		const float fInvSin = 1.0f / fSinTheta;
		const float fTTheta = fTheta * t;
		const float fRest = fTheta - fTTheta;
		float fW0 = static_cast<float>(std::sin(fRest));
		fW0 = fW0 * fInvSin;
		float fW1 = static_cast<float>(std::sin(fTTheta));
		fW1 = fW1 * fInvSin;
		fW1 = fW1 * fSign;
		vResult = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fW1), vQ1), _mm_mul_ps(_mm_set1_ps(fW0), vQ0));
	} else {
		const float fW0 = 1.0f - t;
		const float fW1 = t * fSign;
		vResult = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fW1), vQ1), _mm_mul_ps(_mm_set1_ps(fW0), vQ0));
	}
	__m128 vLenSq = _mm_mul_ps(vResult, vResult);
	vLenSq = _mm_add_ps(_mm_shuffle_ps(vLenSq, vLenSq, 0x4E), vLenSq);
	vLenSq = _mm_add_ps(_mm_shuffle_ps(vLenSq, vLenSq, 0xB1), vLenSq);
	const __m128 vEstimate = _mm_rsqrt_ss(vLenSq);
	const __m128 vZeroMask = _mm_cmpeq_ss(vLenSq, _mm_set_ss(0.0f));
	// The exe's mulss/subss steps are written packed: only lane 0 reaches the broadcast below.
	const __m128 vStep = _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(vLenSq, vEstimate), vEstimate));
	__m128 vInvLen = _mm_mul_ps(_mm_mul_ps(_mm_set_ss(0.5f), vEstimate), vStep);
	vInvLen = _mm_andnot_ps(vZeroMask, vInvLen);
	_mm_store_ps(&reinterpret_cast<float*>(&m_vec.m_quad)[0], _mm_mul_ps(_mm_shuffle_ps(vInvLen, vInvLen, 0), vResult));
}

void hkQuaternion::removeAxisComponent(const hkVector4& axis) {
	__m128 m_quad = m_vec.m_quad;
	__m128 v4 = _mm_mul_ps(axis.m_quad, m_vec.m_quad);
	__m128 v5 = _mm_shuffle_ps(v4, v4, 170);
	v5[0] = v5[0] + static_cast<float>(_mm_shuffle_ps(v4, v4, 85)[0] + v4[0]);
	__m128 v6 = _mm_shuffle_ps(m_quad, m_quad, 255);
	__m128 v7 = _mm_shuffle_ps(axis.m_quad, axis.m_quad, 201);
	__m128 v8 = _mm_shuffle_ps(axis.m_quad, axis.m_quad, 210);
	__m128 v9 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 201), v8),
										  _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 210), v7)),
							   v6),
		_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(v6, v6), hkQuadRealMinusHalf), axis.m_quad),
			_mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), m_quad)));
	__m128 v10 = _mm_add_ps(v9, v9);
	__m128 v11 = _mm_mul_ps(axis.m_quad, v10);
	double v12 = static_cast<float>(_mm_shuffle_ps(v11, v11, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]));
	if (!(v12 - 1.0 > -0.0010000000474974513)) {
		hkVector4 arAxis;
		if (!(v12 + 1.0 < 0.0010000000474974513)) {
			__m128 v22;
			__m128 v17 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v10, v10, 210), v7),
				_mm_mul_ps(_mm_shuffle_ps(v10, v10, 201), v8));
			__m128 v18 = _mm_mul_ps(v17, v17);
			float v19 = _mm_shuffle_ps(v18, v18, 85)[0] + v18[0];
			__m128 v20 = _mm_shuffle_ps(v18, v18, 170);
			v20[0] = v20[0] + v19;
			float v21 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v20[0])));
			v22[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
			__m128 v23 = _mm_cmpeq_ss(v20, v22);
			v22[0] = 3.0f - static_cast<float>(static_cast<float>(v20[0] * v21) * v21);
			__m128 v24 = _mm_setzero_ps();
			v24[0] = static_cast<float>(0.5f * v21) * v22[0];
			__m128 v25 = _mm_andnot_ps(v23, v24);
			float r = v12;
			arAxis.m_quad = _mm_mul_ps(_mm_shuffle_ps(v25, v25, 0), v17);
			float ra = hkMath::acos(r);
			setAxisAngle(arAxis, ra);
		} else {
			int32_t v13 = 0;
			int32_t v14 = 1;
			int32_t v15 = 2;
			float v28 = std::fabs((double)axis.m_quad[0]);
			float v29 = std::fabs((double)axis.m_quad[1]);
			float v30 = std::fabs((double)axis.m_quad[2]);
			if (v28 > static_cast<double>(v29)) {
				v28 = v29;
				v14 = 0;
				v13 = 1;
			}
			if (v28 > static_cast<double>(v30)) {
				v15 = v13;
			}
			double v16 = axis.m_quad[v15];
			arAxis.m_quad = _mm_setzero_ps();
			arAxis.m_quad[v14] = v16;
			arAxis.m_quad[v15] = -axis.m_quad[v14];
			setAxisAngle(arAxis, 3.1415927f);
		}
	} else {
		(*this).m_vec.m_quad = hkQuadReal0001;
	}
}

double hkQuaternion::getAngle() const {
	float v5 = std::fabs((double)m_vec.m_quad[3]);
	double v1 = v5;
	float v6 = std::fabs((double)v5);
	if (!(v6 >= 1.0f)) {
		float ad = std::acos(v1);
		return static_cast<float>(ad + ad);
	} else {
		bool v2 = v1 > 0.0;
		double v3 = 0.0;
		if (!v2) {
			v3 = 3.1415927f;
		}
		float ab = v3;
		return static_cast<float>(ab + ab);
	}
}

// The SDK's: this copied into rest with one 16-byte store, axisRot's component taken out of rest, the conjugate of
// rest (x, y and z negated with the mask at 0x10C4C00, w kept) multiplied by this (hkQuaternion::setMul) and the
// angle of the product read. The angle is written before the check that negates it: the exe multiplies axisRot by
// the product (mulps), sums z + (y + x) with addss and scales that by the product's w (mulss), negating the angle only
// when the value is below zero (x87 fcompp against 0.0, so a NaN keeps the angle).
void hkQuaternion::decomposeRestAxis(const hkVector4& axis, hkQuaternion& restOut, float& angleOut) const {
	restOut.m_vec.m_quad = m_vec.m_quad;
	restOut.removeAxisComponent(axis);
	hkQuaternion negated;
	negated.setInverse(restOut);
	hkQuaternion restRotation;
	restRotation.setMul(negated, *this);
	const float fAngle = static_cast<float>(restRotation.getAngle());
	angleOut = fAngle;
	const __m128 vProduct = _mm_mul_ps(axis.m_quad, restRotation.m_vec.m_quad);
	const __m128 vSumXY = _mm_add_ss(_mm_shuffle_ps(vProduct, vProduct, 0x55), vProduct);
	const __m128 vSum = _mm_add_ss(_mm_shuffle_ps(vProduct, vProduct, 0xAA), vSumXY);
	const __m128 vValue = _mm_mul_ss(_mm_set_ss(restRotation.m_vec(3)), vSum);
	if (_mm_cvtss_f32(vValue) < 0.0f) {
		angleOut = -fAngle;
	}
}

hkBool* hkQuaternion::isOk(hkBool* a1) const {
	::new (a1) hkBool(m_vec.isOk4());
	__m128 v3 = _mm_mul_ps(m_vec.m_quad, m_vec.m_quad);
	__m128 v4 = _mm_add_ps(_mm_shuffle_ps(v3, v3, 78), v3);
	float v6 = static_cast<float>(_mm_shuffle_ps(v4, v4, 177)[0] + v4[0]) - 1.0;
	float v7 = std::fabs((double)v6);
	if (a1->m_bool && v7 < 0.001f) {
		a1->m_bool = 1;
		return a1;
	} else {
		a1->m_bool = 0;
		return a1;
	}
}

void hkQuaternion::setFlippedRotation(const hkVector4& from) {
	__m128 v10 = _mm_undefined_ps();
	float v14 = std::fabs((double)from.m_quad[0]);
	int32_t v2 = 0;
	int32_t v3 = 1;
	int32_t v4 = 2;
	float v15 = std::fabs((double)from.m_quad[1]);
	float v16 = std::fabs((double)from.m_quad[2]);
	if (v14 > static_cast<double>(v15)) {
		v14 = v15;
		v3 = 0;
		v2 = 1;
	}
	if (v14 > static_cast<double>(v16)) {
		v4 = v2;
	}
	double v5 = from.m_quad[v4];
	__m128 v18 = _mm_setzero_ps();
	v18[v3] = v5;
	v18[v4] = -from.m_quad[v3];
	__m128 v6 = _mm_mul_ps(v18, v18);
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
	v18 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v18);
	v18[3] = 0.0f;
	m_vec.m_quad = v18;
}

// The SDK's rotation-to-quaternion conversion. With a positive trace the exe builds the quaternion from the trace;
// otherwise from the largest diagonal element i, with j and k the next indices (the exe's table {1, 2, 0}) and the
// matrix read as twelve floats (column c, row r at r + 4c). The trace plus one, its root, and 0.5 over the root are
// rounded to float (fstp dword); the differences and sums stay in ST0 until the product with that factor is stored.
void quaternionFromRotatation(const hkRotation* a1, hkQuaternion* a2) {
	hkQuaternion v17;
	float v9 = a1->m_col0(0) + a1->m_col1(1) + a1->m_col2(2);
	if (!(v9 > 0.0f)) {
		const int32_t aiNext[3] = {1, 2, 0};
		int32_t i = 0;
		if (a1->m_col0(0) < a1->m_col1(1)) {
			i = 1;
		}
		if (a1->getColumn(i)(i) < a1->m_col2(2)) {
			i = 2;
		}
		const int32_t j = aiNext[i];
		const int32_t k = aiNext[j];
		float v13 = a1->getColumn(i)(i) - (a1->getColumn(k)(k) + a1->getColumn(j)(j)) + 1.0;
		float v14 = std::sqrt((double)v13);
		double v8 = v14;
		float v15 = 0.5 / v14;
		v17.m_vec(i) = v8 * 0.5;
		v17.m_vec(3) = (a1->getColumn(j)(k) - a1->getColumn(k)(j)) * v15;
		v17.m_vec(j) = (a1->getColumn(j)(i) + a1->getColumn(i)(j)) * v15;
		v17.m_vec(k) = v15 * (a1->getColumn(k)(i) + a1->getColumn(i)(k));
		(*a2).m_vec.m_quad = v17.m_vec.m_quad;
	} else {
		float v10 = v9 + 1.0;
		float v11 = std::sqrt((double)v10);
		double v2 = v11;
		float v12 = 0.5 / v11;
		v17.m_vec(0) = (a1->m_col1(2) - a1->m_col2(1)) * v12;
		v17.m_vec(1) = (a1->m_col2(0) - a1->m_col0(2)) * v12;
		v17.m_vec(2) = v12 * (a1->m_col0(1) - a1->m_col1(0));
		v17.m_vec(3) = v2 * 0.5;
		(*a2).m_vec.m_quad = v17.m_vec.m_quad;
	}
}
