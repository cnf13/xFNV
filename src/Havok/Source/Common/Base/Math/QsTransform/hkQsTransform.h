#pragma once

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <xmmintrin.h>

class hkTransform;

// A transform kept as translation, rotation quaternion and scale.
class hkQsTransform {
public:
	// Tags for the constructors that set the identity or zero transform.
	enum IdentityInitializer : int32_t {
		IDENTITY = 0,
	};

	enum ZeroInitializer : int32_t {
		ZERO = 0,
	};

	// The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
	HK_DECLARE_POD_TYPE();
	// Copies the translation, rotation and scale.
	hkQsTransform& operator=(const hkQsTransform& other);

	void setScale(const hkVector4& s) { m_scale = s; }

	// Sets the translation and rotation from the transform; the scale becomes one.
	void setFromTransformNoScale(const hkTransform& transform);
	// Sets the translation, rotation and scale from the transform.
	void setFromTransform(const hkTransform& transform);
	// Sets the translation, rotation and scale from the transform's decomposition; false when the transform has skew.
	// setFromTransform calls it.
	hkBool setFromTransformChecked(const hkTransform& arTransform);
	// The transform of this translation, rotation and scale.
	void copyToTransform(hkTransform& transformOut) const;
	// The transform of this translation and rotation, the scale left out.
	void copyToTransformNoScale(hkTransform& transformOut) const;
	// Writes the transform of this translation, rotation and scale (the rotation matrix times the scale matrix) as 16
	// column-major floats to p. copyToTransform jumps to it: an hkTransform is those 16
	// floats.
	void get4x4ColumnMajor(float* d) const;
	// Inlines defined below: the inverse of t; aTb times bTc; the inverse of a times b; a to b at t.
	void setInverse(const hkQsTransform& t);
	void setMul(const hkQsTransform& aTb, const hkQsTransform& bTc);
	void setMulInverseMul(const hkQsTransform& a, const hkQsTransform& b);
	void setInterpolate4(const hkQsTransform& a, const hkQsTransform& b, const hkSimdReal& t);
	hkBool* isOk(hkBool* apResult) const;
	bool* isApproximatelyEqual(bool* a1, __m128* a2, uint32_t a3);

	hkVector4 m_translation;
	hkQuaternion m_rotation;
	hkVector4 m_scale;
};

// Copies the scale of each of the aiCount transforms at apSrc into the transforms at apDst.
void hkQsTransformCopyScales(hkQsTransform* apDst, const hkQsTransform* apSrc, int32_t aiCount);
// Normalizes the rotations of the aiCount transforms at apTransforms.
void normalizeRotations(hkQsTransform* apTransforms, int32_t aiCount);
// Divides the translation and scale of each of the auiCount
// transforms at apTransforms by its weight in apWeights, then normalizes their rotations.
void blendNormalizeBatch(hkQsTransform* apTransforms, const float* apWeights, uint32_t auiCount);

// The SDK's inline: the translation rotated back by the rotation and negated, the conjugate rotation, and the
// reciprocal of the scale's x, y and z with w cleared.
inline void hkQsTransform::setInverse(const hkQsTransform& t) {
	m_translation.setRotatedInverseDir(t.m_rotation, t.m_translation);
	m_translation.setNeg4(m_translation);
	m_rotation.setInverse(t.m_rotation);
	hkVector4 invScale;
	invScale.setReciprocal3(t.m_scale);
	m_scale.setXYZ0(invScale);
}

// The SDK's inline: the translation of bTc rotated into aTb's frame plus aTb's, the rotations multiplied
// (hkQuaternion::setMul) and the scales multiplied.
inline void hkQsTransform::setMul(const hkQsTransform& aTb, const hkQsTransform& bTc) {
	hkVector4 extraTrans;
	extraTrans.setRotatedDir(aTb.m_rotation, bTc.m_translation);
	m_translation.setAdd4(aTb.m_translation, extraTrans);
	m_rotation.setMul(aTb.m_rotation, bTc.m_rotation);
	m_scale.setMul4(aTb.m_scale, bTc.m_scale);
}

// The SDK's inline: setMul of the inverse of t1 and t2.
inline void hkQsTransform::setMulInverseMul(const hkQsTransform& a, const hkQsTransform& b) {
	hkQsTransform h;
	h.setInverse(a);
	setMul(h, b);
}

// The scale and translation are interpolated linearly from a's (t = 0) to b's (t = 1); the rotation linearly from a's
// towards b's, or towards its negation when their dot product is negative (the nearer of the two), then normalized.
inline void hkQsTransform::setInterpolate4(const hkQsTransform& a, const hkQsTransform& b, const hkSimdReal& t) {
	m_scale.setInterpolate4(a.m_scale, b.m_scale, t);
	m_translation.setInterpolate4(a.m_translation, b.m_translation, t);
	const float oneMinusT = 1.0f - static_cast<float>(t);
	const hkQuadReal x2 = _mm_mul_ps(b.m_rotation.m_vec.m_quad, a.m_rotation.m_vec.m_quad);
	const hkQuadReal sum0 = _mm_add_ps(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 0, 3, 2)), x2);
	const float dot = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum0, sum0, _MM_SHUFFLE(2, 3, 0, 1)), sum0));
	const float signedT = (dot < 0.0f) ? _mm_cvtss_f32(_mm_sub_ss(hkQuadReal0000, t.m_real)) : static_cast<float>(t);
	hkQuaternion out;
	out.m_vec.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(signedT), b.m_rotation.m_vec.m_quad),
		_mm_mul_ps(_mm_set1_ps(oneMinusT), a.m_rotation.m_vec.m_quad));
	out.normalize();
	m_rotation = out;
}

using hkQsTransformf = hkQsTransform;

extern hkQsTransform hkQsTransformIdentity;
// The identity transform's twelve floats, which hkQsTransformIdentity's initialiser copies.
using hkQsTransformIdentity_storage_t = float[12];
inline hkQsTransformIdentity_storage_t hkQsTransformIdentity_storage{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 0.0f};
