#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

#include <cmath>

class hkRotation;

// A rotation as a four component vector: x, y, z axis scaled by half the angle, w the cosine.
class hkQuaternion {
public:
	// Stores nothing.
	hkQuaternion() {}
	hkQuaternion(const hkRotation& r);
	// The quaternion whose x, y, z and w are v's (the C++20 aggregate initialisation the hybrid side's
	// `static_cast<hkQuaternion>(v)` is: m_vec copied from v).
	explicit hkQuaternion(const hkVector4& v) : m_vec(v) {}

	// The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
	HK_DECLARE_POD_TYPE();
	// The SDK's inline: the identity rotation (0, 0, 0, 1) (the exe's copy is 0x56D250).
	void setIdentity();
	// The SDK's inline: scales the vector to unit length (the exe's copy is 0x5611C0).
	void normalize();

	// Havok SDK: the quaternion of a rotation matrix (thiscall, ret 4, 0xCB26E0). The body
	// only calls quaternionFromRotatation (0xCB2550), a __usercall that takes the rotation in esi and the result
	// pointer as its one stack argument; fnvd::original has no form for an esi argument, so it stays a forwarder.
	void set(const hkRotation& r);
	// Havok SDK: the normalized quaternion of a rotation matrix.
	// It calls quaternionFromRotatation the same way and then normalize, so it stays a forwarder too.
	void setAndNormalize(const hkRotation& r);

	// The SDK's inline: this = q0 * q1 (the exe's copy is 0xC66320, compiled out of line into bhkWorld.cpp).
	void setMul(const hkQuaternion& q0, const hkQuaternion& q1);
	// The SDK's inline: this = this * q (the exe's copy is 0xCE0CA0, compiled out of line into
	// hkaLookAtIkSolver.cpp; matched to the PDB's hkQuaternion::mul by its shape and call sites).
	void mul(const hkQuaternion& q);
	// The SDK's inline: this = q0 * the inverse of q1 (the exe's copy is 0xC74AC0).
	void setMulInverse(const hkQuaternion& q0, const hkQuaternion& q1);
	// The SDK's inline: the unit rotation axis (the exe's copy is 0xC74C90).
	void getAxis(hkVector4& axisOut) const;
	// The SDK's inline: the shortest rotation that takes the unit vector `from` to the unit vector `to`: the
	// identity when they are nearly parallel, setFlippedRotation(from) when they nearly oppose (the exe's copy is
	// 0xCD9D10).
	void setShortestRotation(const hkVector4& from, const hkVector4& to);
	// The SDK's inline: the shortest rotation from the unit vector `from` to the unit vector `to`, scaled down by
	// gain (the exe's copy is 0xCDB910, compiled into hkaTwoJointsIkSolver.cpp).
	void setShortestRotationDamped(float gain, const hkVector4& from, const hkVector4& to);

	// The rotation of afAngle radians about the unit axis arAxis: the axis scaled by sin(afAngle / 2) on all four
	// lanes, with w then replaced by cos(afAngle / 2).
	void setAxisAngle(const hkVector4& axis, float angle);

	// The spherical interpolation from q0 (t = 0) to q1 (t = 1) along the shorter arc, normalized; linear when the
	// two are nearly parallel.
	void setSlerp(const hkQuaternion& q0, const hkQuaternion& q1, float t);

	// The quaternion with the component about perpVector taken out.
	void removeAxisComponent(const hkVector4& axis);

	// The rotation angle in radians.
	double getAngle() const;

	// Splits this rotation into rest, the rotation about axisRot, and angle, the rotation left about axisRot.
	void decomposeRestAxis(const hkVector4& axis, hkQuaternion& restOut, float& angleOut) const;

	// The SDK's inline: component aiIndex of the quaternion (the exe's copy is 0x560D10; it calls
	// hkVector4::operator() 0x560D30).
	const float& operator()(int32_t i) const { return m_vec(i); }
	// The SDK's inline: this = the inverse of q0 * q1 (the exe's copy is 0xC74B70).
	void setInverseMul(const hkQuaternion& q0, const hkQuaternion& q1);
	// The SDK's inline: the conjugate of q (its imaginary part negated), defined below.
	void setInverse(const hkQuaternion& q);
	// Havok SDK: whether the quaternion is valid: its components are valid and its length is 1 within 0.001; the
	// exe returns the hkBool through apResult (thiscall, ret 4, 0xCB24C0). Const in the SDK, as callers use it on a
	// const pose's rotation.
	hkBool* isOk(hkBool* apResult) const;
	// The SDK's inline: an estimate of the rotation from this to `to` as an angle vector, 2 * (imag x to.imag -
	// to.w * imag + w * to.imag), negated when the 4-lane dot product is negative (the exe's copy is 0xD6BEC0,
	// compiled out of line into hkaKeyFrameHierarchyUtility.cpp).
	void estimateAngleTo(const hkQuaternion& to, hkVector4& angleOut) const;
	void setFlippedRotation(const hkVector4& from);

	// The SDK's inline: the identity quaternion, hkQuaternionIdentity (the exe's copy is 0x62AAD0, compiled out of
	// line into TESArrowTargetListener.cpp).
	static const hkQuaternion& getIdentity();

	hkVector4 m_vec;
};

// The rotation is const: the exe (0xCB2550) only reads it (esi), and hkQuaternion::set and setAndNormalize pass
// their const hkRotation&.
// Inferred type: a1: hkRotation* from 0xcb26e6 (parameter 0 of hkQuaternion::set), its members used at 0xcb2550; a2:
// hkQuaternion* from 0xcb26e6 (this of hkQuaternion::set), its members used at 0xcb2550
void quaternionFromRotatation(const hkRotation* a1, hkQuaternion* a2);

// IDA's name for the same class (its float-precision spelling); the PDB has hkQuaternion only.
using hkQuaternionf = hkQuaternion;

extern hkQuaternion hkQuaternionIdentity;

// The SDK's hkQuadRealMinusHalf (hkMath.h; defined in hkMath.cpp): the four -0.5 lanes the SDK's inlined quaternion
// rotation adds to w * w.
inline constexpr hkQuadReal hkQuadRealMinusHalf{-0.5f, -0.5f, -0.5f, -0.5f};

// The SDK's hkQuadRealMax (hkMath.h; defined in hkMath.cpp, PDB module hkMath.obj): HK_REAL_MAX in four lanes. No code
// in the exe reads it; declared so that hkMath.cpp's data block covers the exe's bytes.
inline constexpr hkQuadReal hkQuadRealMax{3.40282e+38f, 3.40282e+38f, 3.40282e+38f, 3.40282e+38f};

// The SDK's hkQuadRealEps (hkMath.h; defined in hkMath.cpp): HK_REAL_EPSILON in four lanes, the entry after
// hkQuadRealMax in the exe's block. hkpBoxMotion::getInertiaWorld (0xD1E8C0) and
// hkpSimpleConstraintInfo::BodyInfo::getEnergy (0xDF6380) clamp inverse masses up to it.
inline constexpr hkQuadReal hkQuadRealEps{1.1920929e-07f, 1.1920929e-07f, 1.1920929e-07f, 1.1920929e-07f};

// IDA names; not in the PDB. The SSE lane masks the SDK's inlines apply: 10C4C00 the sign bits xor'ed to conjugate a
// quaternion (setInverse), 10C4C10 the xyz mask hkVector4::setXYZ0 and's in (setInverse's scale, after the inlined
// setReciprocal3). The mask that negates all four lanes (setNeg4) is hkQuadRealNegateMask (hkVector4.h).
inline hkQuadReal rdata_10C4C00{-0.0f, -0.0f, -0.0f};
inline hkQuadReal rdata_10C4C10{__builtin_bit_cast(float, 0xffffffffu), __builtin_bit_cast(float, 0xffffffffu),
	__builtin_bit_cast(float, 0xffffffffu)};

// The SDK's nearlyOneQuad (1 - 1e-5 in four lanes) of
// setShortestRotation.
// This file's copies of hkQuaternion.inl's header statics nearlyOneQuad and somewhatNearlyNegOneQuad: the pair right
// below this file's vtable (hkaFootPlacementIkSolver's, locator 0x10CB640), and the only pair the exe reads, from its
// out-of-line hkQuaternion::setShortestRotation (0xCD9D10, registered here). src's inline reads them by these names.
inline hkQuadReal rdata_10CB620{0.99999f, 0.99999f, 0.99999f, 0.99999f};

// The SDK's somewhatNearlyNegOneQuad (-(1 - 1e-3) in four lanes) of
// setShortestRotation.
inline hkQuadReal rdata_10CB630{-0.999f, -0.999f, -0.999f, -0.999f};

// The SDK's header statics of hkQuaternion.inl (lines 205-206, HK_CONFIG_SIMD branch): every file including the
// header has its own pair of copies in the exe's .rdata (about 700 pairs); only the copies a defined global ends at get
// a FNVD_TU_COPY line. setShortestRotation above still reads the pair 0xCD9D10 reads, as rdata_10CB620/630.
static const hkQuadReal nearlyOneQuad{hkQuadReal{0.99999f, 0.99999f, 0.99999f, 0.99999f}};
static const hkQuadReal somewhatNearlyNegOneQuad{hkQuadReal{-0.999f, -0.999f, -0.999f, -0.999f}};

// The SDK's inline hkVector4::setRotatedDir(quat, dir): 2 * ((imag x dir) * w + (w * w - 0.5) * dir + (imag . dir) *
// quat) on all four lanes; setRotatedInverseDir negates the cross product. The exe computes them with SSE; src rounds
// each lane operation to float in the exe's order (the dot product sums z + (y + x)), which gives the same bits
// (NaN payloads aside).
inline void hkVector4::setRotatedDir(const hkQuaternion& quat, const hkVector4& direction) {
	const float* q = &reinterpret_cast<const float*>(&quat.m_vec.m_quad)[0];
	const float* v = &reinterpret_cast<const float*>(&direction.m_quad)[0];
	const float* c = &reinterpret_cast<const float*>(&hkQuadRealMinusHalf)[0];
	constexpr int32_t kYZX[4] = {1, 2, 0, 3};
	constexpr int32_t kZXY[4] = {2, 0, 1, 3};
	const float px = v[0] * q[0];
	const float py = v[1] * q[1];
	const float pz = v[2] * q[2];
	const float fSumXY = py + px;
	const float fDot = pz + fSumXY;
	const float w = q[3];
	float r[4];
	for (int32_t i = 0; i < 4; ++i) {
		const float a = v[kYZX[i]] * q[kZXY[i]];
		const float b = v[kZXY[i]] * q[kYZX[i]];
		const float fCross = b - a;
		const float fCrossW = fCross * w;
		const float fWW = w * w;
		const float fWWC = fWW + c[i];
		const float fScaledDir = fWWC * v[i];
		const float fDotQ = fDot * q[i];
		const float fSum = fScaledDir + fDotQ;
		const float fTotal = fCrossW + fSum;
		r[i] = fTotal + fTotal;
	}
	m_quad[0] = r[0];
	m_quad[1] = r[1];
	m_quad[2] = r[2];
	m_quad[3] = r[3];
}

inline void hkVector4::setRotatedInverseDir(const hkQuaternion& quat, const hkVector4& direction) {
	const float* q = &reinterpret_cast<const float*>(&quat.m_vec.m_quad)[0];
	const float* v = &reinterpret_cast<const float*>(&direction.m_quad)[0];
	const float* c = &reinterpret_cast<const float*>(&hkQuadRealMinusHalf)[0];
	constexpr int32_t kYZX[4] = {1, 2, 0, 3};
	constexpr int32_t kZXY[4] = {2, 0, 1, 3};
	const float px = q[0] * v[0];
	const float py = q[1] * v[1];
	const float pz = q[2] * v[2];
	const float fSumXY = py + px;
	const float fDot = pz + fSumXY;
	const float w = q[3];
	float r[4];
	for (int32_t i = 0; i < 4; ++i) {
		const float a = v[kYZX[i]] * q[kZXY[i]];
		const float b = v[kZXY[i]] * q[kYZX[i]];
		const float fCross = a - b;
		const float fCrossW = fCross * w;
		const float fWW = w * w;
		const float fWWC = fWW + c[i];
		const float fScaledDir = fWWC * v[i];
		const float fDotQ = fDot * q[i];
		const float fSum = fScaledDir + fDotQ;
		const float fTotal = fCrossW + fSum;
		r[i] = fTotal + fTotal;
	}
	m_quad[0] = r[0];
	m_quad[1] = r[1];
	m_quad[2] = r[2];
	m_quad[3] = r[3];
}

// The SDK's SSE form of hkQuaternion::setInverse: the sign bits of x, y and z flipped with the mask above.
inline void hkQuaternion::setInverse(const hkQuaternion& q) {
	m_vec.m_quad = _mm_xor_ps(q.m_vec.m_quad, rdata_10C4C00);
}

// The SDK's SSE form: the quad of hkQuadReal0001. The exe's copy takes the quad through the shared one-field getter
// 0x6815C0.
inline void hkQuaternion::setIdentity() {
	m_vec.m_quad = hkQuadReal0001;
}

// The exe's copy calls hkVector4::normalize4 (0x5611E0) out of line.
inline void hkQuaternion::normalize() {
	m_vec.normalize4();
}

inline const hkQuaternion& hkQuaternion::getIdentity() {
	return hkQuaternionIdentity;
}

// The SDK's SSE form, inlined as the exe has it: the cross product of the imaginary parts plus each one scaled by the
// other's w; w is q0.w * q1.w less the 3-lane dot product (summed z + (y + x)).
inline void hkQuaternion::setMul(const hkQuaternion& q0, const hkQuaternion& q1) {
	__m128 v3 = _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 255);
	__m128 v4 = _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 255);
	__m128 v5 = _mm_mul_ps(q1.m_vec.m_quad, q0.m_vec.m_quad);
	__m128 v6 = _mm_shuffle_ps(v5, v5, 170);
	v6[0] = v6[0] + static_cast<float>(_mm_shuffle_ps(v5, v5, 85)[0] + v5[0]);
	__m128 v7 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0.m_vec.m_quad),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), q1.m_vec.m_quad),
			_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 210),
						   _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 201)),
				_mm_mul_ps(_mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 201),
					_mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 210)))));
	m_vec.m_quad = _mm_shuffle_ps(v7, _mm_unpackhi_ps(v7, _mm_sub_ps(v5, _mm_shuffle_ps(v6, v6, 0))), 196);
}

// The SDK's SSE form of setMul(*this, q), inlined as the exe has it.
inline void hkQuaternion::mul(const hkQuaternion& q) {
	__m128 v2 = _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 255);
	__m128 v3 = _mm_shuffle_ps(q.m_vec.m_quad, q.m_vec.m_quad, 255);
	__m128 v4 = _mm_mul_ps(m_vec.m_quad, q.m_vec.m_quad);
	__m128 v5 = _mm_shuffle_ps(v4, v4, 170);
	v5[0] = v5[0] + static_cast<float>(_mm_shuffle_ps(v4, v4, 85)[0] + v4[0]);
	__m128 v6 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), m_vec.m_quad),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v2, v2, 0), q.m_vec.m_quad),
			_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 201),
						   _mm_shuffle_ps(q.m_vec.m_quad, q.m_vec.m_quad, 210)),
				_mm_mul_ps(_mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 210),
					_mm_shuffle_ps(q.m_vec.m_quad, q.m_vec.m_quad, 201)))));
	m_vec.m_quad = _mm_shuffle_ps(v6, _mm_unpackhi_ps(v6, _mm_sub_ps(v4, _mm_shuffle_ps(v5, v5, 0))), 196);
}

// The SDK's SSE form: xyz = q1.imag x q0.imag - q0.w * q1.imag + q1.w * q0.imag, w the 4-lane dot product.
inline void hkQuaternion::setMulInverse(const hkQuaternion& q0, const hkQuaternion& q1) {
	__m128 v3 = _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 255);
	__m128 v4 = _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 255);
	__m128 v5 = _mm_mul_ps(q1.m_vec.m_quad, q0.m_vec.m_quad);
	__m128 v6 = _mm_add_ps(_mm_shuffle_ps(v5, v5, 78), v5);
	__m128 v7 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 177), v6);
	__m128 v8 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0.m_vec.m_quad),
		_mm_sub_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 201),
								  _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 210)),
					   _mm_mul_ps(_mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 210),
						   _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 201))),
			_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), q1.m_vec.m_quad)));
	m_vec.m_quad = _mm_shuffle_ps(v8, _mm_unpackhi_ps(v8, _mm_shuffle_ps(v7, v7, 0)), 196);
}

// The SDK's SSE form: xyz = q1.imag x q0.imag + q0.w * q1.imag - q1.w * q0.imag, w the 4-lane dot product.
inline void hkQuaternion::setInverseMul(const hkQuaternion& q0, const hkQuaternion& q1) {
	__m128 v3 = _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 255);
	__m128 v4 = _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 255);
	__m128 v5 = _mm_mul_ps(q1.m_vec.m_quad, q0.m_vec.m_quad);
	__m128 v6 = _mm_add_ps(_mm_shuffle_ps(v5, v5, 78), v5);
	__m128 v7 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 177), v6);
	__m128 v8 = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), q1.m_vec.m_quad),
							   _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 201),
											  _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 210)),
								   _mm_mul_ps(_mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 210),
									   _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 201)))),
		_mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0.m_vec.m_quad));
	m_vec.m_quad = _mm_shuffle_ps(v8, _mm_unpackhi_ps(v8, _mm_shuffle_ps(v7, v7, 0)), 196);
}

// The SDK's SSE form: the vector normalized over xyz (the SDK's inline normalize3: rsqrtss, one Newton step
// 0.5 * r * (3 - x * r * r), zero for a zero length), negated when w is negative.
inline void hkQuaternion::getAxis(hkVector4& axisOut) const {
	__m128 v2 = _mm_mul_ps(m_vec.m_quad, m_vec.m_quad);
	float v3 = _mm_shuffle_ps(v2, v2, 85)[0] + v2[0];
	__m128 v4 = _mm_shuffle_ps(v2, v2, 170);
	v4[0] = v4[0] + v3;
	float v5 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v4[0])));
	__m128 v7 = _mm_cmpeq_ss(v4, _mm_setzero_ps());
	float fNewton = 3.0f - static_cast<float>(static_cast<float>(v4[0] * v5) * v5);
	__m128 v8 = _mm_setzero_ps();
	v8[0] = static_cast<float>(0.5f * v5) * fNewton;
	__m128 v9 = _mm_andnot_ps(v7, v8);
	__m128 v10 = _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), m_vec.m_quad);
	if ((_mm_movemask_ps(_mm_cmplt_ps(m_vec.m_quad, _mm_setzero_ps())) & 8) != 0) {
		axisOut.m_quad = _mm_xor_ps(hkQuadRealNegateMask, v10);
	} else {
		axisOut.m_quad = v10;
	}
}

// The SDK's SSE form, inlined as the exe has it.
inline void hkQuaternion::estimateAngleTo(const hkQuaternion& to, hkVector4& angleOut) const {
	__m128 v3 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(to.m_vec.m_quad, to.m_vec.m_quad, 210),
							   _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 201)),
		_mm_mul_ps(_mm_shuffle_ps(to.m_vec.m_quad, to.m_vec.m_quad, 201),
			_mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 210)));
	angleOut.m_quad = v3;
	__m128 v4 = _mm_shuffle_ps(to.m_vec.m_quad, to.m_vec.m_quad, 255);
	__m128 v5 = _mm_sub_ps(v3, _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), m_vec.m_quad));
	angleOut.m_quad = v5;
	__m128 v6 = _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 255);
	__m128 v7 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), to.m_vec.m_quad), v5);
	__m128 v8 = _mm_add_ps(v7, v7);
	angleOut.m_quad = v8;
	__m128 v9 = _mm_mul_ps(to.m_vec.m_quad, m_vec.m_quad);
	__m128 v10 = _mm_add_ps(_mm_shuffle_ps(v9, v9, 78), v9);
	if ((_mm_movemask_ps(_mm_add_ps(_mm_shuffle_ps(v10, v10, 177), v10)) & 1) != 0) {
		angleOut.m_quad = _mm_xor_ps(v8, hkQuadRealNegateMask);
	}
}

// The SDK's SSE form (hkQuaternion.inl's HK_CONFIG_SIMD branch), inlined as the exe has it.
inline void hkQuaternion::setShortestRotation(const hkVector4& from, const hkVector4& to) {
	__m128 v3 = _mm_mul_ps(to.m_quad, from.m_quad);
	float v4 = _mm_shuffle_ps(v3, v3, 85)[0] + v3[0];
	__m128 v5 = _mm_shuffle_ps(v3, v3, 170);
	v5[0] = v5[0] + v4;
	__m128 v6 = _mm_shuffle_ps(v5, v5, 0);
	__m128 v7 = _mm_cmplt_ps(v6, _mm_xor_ps(hkQuadRealNegateMask, rdata_10CB620));
	if (_mm_movemask_ps(_mm_cmplt_ps(rdata_10CB620, v6))) {
		m_vec.m_quad = hkQuadReal0001;
	} else if (_mm_movemask_ps(v7)) {
		setFlippedRotation(from);
	} else {
		__m128 v8 = _mm_add_ps(_mm_mul_ps(hkQuadRealHalf, v6), hkQuadRealHalf);
		__m128 v9 = _mm_rsqrt_ps(v8);
		__m128 v10 =
			_mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v9, v8), v9)), _mm_mul_ps(v9, hkQuadRealHalf));
		__m128 v11 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 210),
									_mm_shuffle_ps(from.m_quad, from.m_quad, 201)),
			_mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 201), _mm_shuffle_ps(from.m_quad, from.m_quad, 210)));
		__m128 v12 = _mm_mul_ps(v10, hkQuadRealHalf);
		if (_mm_movemask_ps(_mm_cmplt_ps(v6, rdata_10CB630))) {
			__m128 v13 = _mm_mul_ps(v11, v11);
			float v14 = _mm_shuffle_ps(v13, v13, 85)[0] + v13[0];
			__m128 v15 = _mm_shuffle_ps(v13, v13, 170);
			v15[0] = v15[0] + v14;
			__m128 v16 = _mm_sub_ps(v8, v6);
			float v25 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v15[0])));
			__m128 v18 = _mm_cmpeq_ss(v15, _mm_setzero_ps());
			float v19 = 3.0f - static_cast<float>(static_cast<float>(v15[0] * v25) * v25);
			__m128 v20 = _mm_setzero_ps();
			v20[0] = static_cast<float>(0.5f * v25) * v19;
			__m128 v21 = _mm_andnot_ps(v18, v20);
			__m128 v22 = _mm_rsqrt_ps(v16);
			__m128 v23 = _mm_mul_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v22, v16), v22)),
										_mm_mul_ps(v22, hkQuadRealHalf)),
				v16);
			v12 = _mm_mul_ps(v23, _mm_shuffle_ps(v21, v21, 0));
		}
		__m128 v24 = _mm_mul_ps(v12, v11);
		m_vec.m_quad = _mm_shuffle_ps(v24, _mm_unpackhi_ps(v24, _mm_mul_ps(v8, v10)), 196);
	}
}

// The SDK's body: its scalar steps on x87 (the exe calls __CIsqrt 0xEC6040 out of line), the cross product, the
// scale and the final normalize4 in SSE.
inline void hkQuaternion::setShortestRotationDamped(float gain, const hkVector4& from, const hkVector4& to) {
	__m128 v4 = _mm_mul_ps(to.m_quad, from.m_quad);
	float v5 = _mm_shuffle_ps(v4, v4, 170)[0] + static_cast<float>(_mm_shuffle_ps(v4, v4, 85)[0] + v4[0]);
	float v15 = 1.0 - gain + v5 * gain;
	float v16 = (v15 + 1.0) * 0.5;
	if (v16 <= 0.0f || v5 < -0.9999989867210388) {
		setFlippedRotation(from);
	} else if (!(v5 > 0.9999989867210388)) {
		float v17 = std::sqrt((double)v16);
		double v6 = v17;
		float fRescaleSin = gain * 0.5 / v17;
		__m128 v19 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 210),
											   _mm_shuffle_ps(from.m_quad, from.m_quad, 201)),
									_mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 201),
										_mm_shuffle_ps(from.m_quad, from.m_quad, 210))),
			_mm_shuffle_ps(_mm_set_ss(fRescaleSin), _mm_set_ss(fRescaleSin), 0));
		v19[3] = v6;
		__m128 v7 = _mm_mul_ps(v19, v19);
		__m128 v8 = _mm_add_ps(_mm_shuffle_ps(v7, v7, 78), v7);
		__m128 v9 = _mm_add_ps(_mm_shuffle_ps(v8, v8, 177), v8);
		float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v9[0])));
		__m128 v12 = _mm_cmpeq_ss(v9, _mm_setzero_ps());
		float fNewton = 3.0f - static_cast<float>(static_cast<float>(v9[0] * v10) * v10);
		__m128 v13 = _mm_setzero_ps();
		v13[0] = static_cast<float>(0.5f * v10) * fNewton;
		__m128 v14 = _mm_andnot_ps(v12, v13);
		m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), v19);
	} else {
		m_vec.m_quad = hkQuadReal0001;
	}
}
