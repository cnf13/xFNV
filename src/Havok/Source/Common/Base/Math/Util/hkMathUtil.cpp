#include "Havok/Source/Common/Base/Math/Util/hkMathUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <xmmintrin.h>

// The basis is the rotation part's polar decomposition: replaced by the mean of itself and its inverse transpose until
// it stops changing (at most 30 steps). Its normalised columns, the first negated when they form a left-handed frame,
// give the rotation; what the inverse of that rotation and translation leaves of the matrix is the scale (its diagonal)
// and skew.
void hkMathUtil::decompose4x4ColTransform(const float* matrixIn, Decomposition& decompOut) {
	hkTransform transform;
	transform.set4x4ColumnMajorQuads(reinterpret_cast<const __m128*>(matrixIn));
	hkMatrix4 matrix4;
	matrix4.set4x4ColumnMajor(matrixIn);
	matrix4.resetFourthRow();
	decompOut.m_translation.m_quad = transform.m_translation.m_quad;

	hkMatrix3 qCur;
	qCur.m_col0.m_quad = transform.m_rotation.m_col0.m_quad;
	qCur.m_col1.m_quad = transform.m_rotation.m_col1.m_quad;
	qCur.m_col2.m_quad = transform.m_rotation.m_col2.m_quad;
	int32_t i = 0;
	while (true) {
		hkMatrix3 qPrev;
		qPrev.m_col0.m_quad = qCur.m_col0.m_quad;
		qPrev.m_col1.m_quad = qCur.m_col1.m_quad;
		qPrev.m_col2.m_quad = qCur.m_col2.m_quad;
		hkMatrix3 qInvT;
		qInvT.m_col0.m_quad = qCur.m_col0.m_quad;
		qInvT.m_col1.m_quad = qCur.m_col1.m_quad;
		qInvT.m_col2.m_quad = qCur.m_col2.m_quad;
		qInvT.invert(1.1920929e-07f);
		qInvT.transpose();
		hkMatrix3 qNext;
		qNext.m_col0.m_quad = qPrev.m_col0.m_quad;
		qNext.m_col1.m_quad = qPrev.m_col1.m_quad;
		qNext.m_col2.m_quad = qPrev.m_col2.m_quad;
		qNext.add(qInvT);
		// The hkSimdReal 0.5, with the other lanes 0.
		__m128 half = _mm_set_ss(0.5f);
		qCur.setMulScalar(&half, &qNext.m_col0.m_quad);
		if (++i >= 30) {
			break;
		}
		if (qCur.isApproximatelyEqual(qPrev, 1.1920929e-07f).m_bool) {
			break;
		}
	}
	decompOut.m_basis.m_col0.m_quad = qCur.m_col0.m_quad;
	decompOut.m_basis.m_col1.m_quad = qCur.m_col1.m_quad;
	decompOut.m_basis.m_col2.m_quad = qCur.m_col2.m_quad;

	// EXE-INLINED 0x62AEA0 (hkVector4::normalize3, three times): one Newton step on rsqrtss of the squared length
	// (y + x, then z + that), a zero length giving a zero column.
	const __m128 zero = _mm_set_ss(0.0f);
	__m128 v = decompOut.m_basis.m_col0.m_quad;
	__m128 sq = _mm_mul_ps(v, v);
	__m128 len2 = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
	len2 = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), len2);
	__m128 r = _mm_rsqrt_ss(len2);
	__m128 inv =
		_mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), r), _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, r), r)));
	inv = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), inv);
	__m128 c0 = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), v);

	v = decompOut.m_basis.m_col1.m_quad;
	sq = _mm_mul_ps(v, v);
	len2 = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
	len2 = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), len2);
	r = _mm_rsqrt_ss(len2);
	inv = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), r), _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, r), r)));
	inv = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), inv);
	const __m128 c1 = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), v);

	v = decompOut.m_basis.m_col2.m_quad;
	sq = _mm_mul_ps(v, v);
	len2 = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
	len2 = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), len2);
	r = _mm_rsqrt_ss(len2);
	inv = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), r), _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, r), r)));
	inv = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), inv);
	const __m128 c2 = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), v);

	// c0 . (c1 x c2), summed y + x, then z + that.
	__m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(c2, c2, 0xD2), _mm_shuffle_ps(c1, c1, 0xC9)),
		_mm_mul_ps(_mm_shuffle_ps(c2, c2, 0xC9), _mm_shuffle_ps(c1, c1, 0xD2)));
	__m128 dots = _mm_mul_ps(cross, c0);
	__m128 det = _mm_add_ss(_mm_shuffle_ps(dots, dots, 0x55), dots);
	det = _mm_add_ss(_mm_shuffle_ps(dots, dots, 0xAA), det);
	const float fDet = _mm_cvtss_f32(det);
	if (fDet < 0.0f) {
		c0 = _mm_mul_ps(_mm_set1_ps(-1.0f), c0);
	}
	decompOut.m_flips.m_bool = fDet < 0.0f;

	hkTransform t;
	t.m_rotation.m_col0.m_quad = c0;
	t.m_rotation.m_col1.m_quad = c1;
	t.m_rotation.m_col2.m_quad = c2;
	hkQuaternion q;
	q.set(t.m_rotation);
	decompOut.m_rotation.m_vec.m_quad = q.m_vec.m_quad;
	decompOut.m_rotation.normalize();
	t.m_rotation.set(decompOut.m_rotation);
	t.m_translation.m_quad = decompOut.m_translation.m_quad;
	hkTransform invTransform;
	invTransform.setInverse(t);
	hkMatrix4 invTransform4;
	invTransform4.set4x4ColumnMajor(reinterpret_cast<const float*>(&invTransform));
	invTransform4.resetFourthRow();
	decompOut.m_scaleAndSkew.setMul(invTransform4, matrix4);

	const hkMatrix4& scaleAndSkew = decompOut.m_scaleAndSkew;
	const __m128 scale = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(scaleAndSkew.m_col0.m_quad[0]),
											 _mm_set_ss(scaleAndSkew.m_col2.m_quad[2])),
		_mm_unpacklo_ps(_mm_set_ss(scaleAndSkew.m_col1.m_quad[1]), _mm_set_ss(0.0f)));
	decompOut.m_scale.m_quad = scale;
	const __m128 eps = _mm_set1_ps(0.001f);
	decompOut.m_hasScale.m_bool =
		(_mm_movemask_ps(_mm_cmple_ps(_mm_and_ps(_mm_sub_ps(scale, _mm_set1_ps(1.0f)), hkVector4_setAbs4_absMask),
			 eps)) &
			7) != 7;

	// The inverse scale: 1 over each scale component (x87 divisions) on the diagonal, 1 in the corner.
	hkMatrix4 scaleInv;
	scaleInv.m_col0.m_quad = _mm_setzero_ps();
	scaleInv.m_col1.m_quad = _mm_setzero_ps();
	scaleInv.m_col2.m_quad = _mm_setzero_ps();
	scaleInv.m_col3.m_quad = _mm_setzero_ps();
	scaleInv.m_col0.m_quad[0] = 1.0f / decompOut.m_scale.m_quad[0];
	scaleInv.m_col1.m_quad[1] = 1.0f / decompOut.m_scale.m_quad[1];
	scaleInv.m_col2.m_quad[2] = 1.0f / decompOut.m_scale.m_quad[2];
	scaleInv.m_col3.m_quad[3] = 1.0f;
	decompOut.m_skew.setMul(scaleInv, decompOut.m_scaleAndSkew);

	const hkMatrix4& skew = decompOut.m_skew;
	const __m128 d0 =
		_mm_and_ps(_mm_sub_ps(skew.m_col0.m_quad, hkMatrix4Identity.m_col0.m_quad), hkVector4_setAbs4_absMask);
	const __m128 d1 =
		_mm_and_ps(_mm_sub_ps(skew.m_col1.m_quad, hkMatrix4Identity.m_col1.m_quad), hkVector4_setAbs4_absMask);
	const __m128 d2 =
		_mm_and_ps(_mm_sub_ps(skew.m_col2.m_quad, hkMatrix4Identity.m_col2.m_quad), hkVector4_setAbs4_absMask);
	const __m128 d3 =
		_mm_and_ps(_mm_sub_ps(skew.m_col3.m_quad, hkMatrix4Identity.m_col3.m_quad), hkVector4_setAbs4_absMask);
	const __m128 dMax = _mm_max_ps(_mm_max_ps(d0, d1), _mm_max_ps(d2, d3));
	decompOut.m_hasSkew.m_bool = (_mm_movemask_ps(_mm_cmple_ps(dMax, eps)) & 15) != 15;
}

void hkMathUtil::decomposeMatrix(const hkMatrix4& matrixIn, Decomposition& decomposition) {
	// Passes the matrix as its 16 column-major floats.
	decompose4x4ColTransform(reinterpret_cast<const float*>(&matrixIn), decomposition);
}

// The SDK's: the three columns and a zero fourth one, the fourth row reset to 0, 0, 0, 1.
void hkMathUtil::decomposeMatrix(const hkMatrix3& matrixIn, Decomposition& decomposition) {
	hkMatrix4 m;
	m.m_col0.m_quad = matrixIn.m_col0.m_quad;
	m.m_col1.m_quad = matrixIn.m_col1.m_quad;
	m.m_col2.m_quad = matrixIn.m_col2.m_quad;
	m.m_col3.m_quad = hkVector4Zero.m_quad;
	m.resetFourthRow();
	decomposeMatrix(m, decomposition);
}
