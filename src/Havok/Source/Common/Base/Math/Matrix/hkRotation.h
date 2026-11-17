#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Reconstruction.hpp"

// A three by three orthonormal rotation.
class hkRotation : public hkMatrix3 {
public:
	// Both exe copies (0x56DF00, 0x621990) store nothing.
	hkRotation();

	// The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
	HK_DECLARE_POD_TYPE();
	// Re-orthonormalizes the matrix through a quaternion.
	void renormalize();

	// Writes the rotation the quaternion describes.
	void set(const hkQuaternion& qi);

	// Sets this to the rotation of afAngle radians about the unit axis q.
	void setAxisAngle(const hkVector4& axis, float angle);

	// Whether the matrix is valid, as hkMatrix3::isOk judges it.
	hkBool isOk() const;
	// Whether each column has unit length and the third is the cross product of the first two, within epsilon
	// (thiscall, ret 8, 0xCB2E70). PDB: returns hkBool; the exe returns it through the hidden result pointer apResult.
	// Havok SDK signature (hkBase.lib, tied by reloc).
	// Havok SDK: result through the hidden result pointer apResult.
	hkBool* isOrthonormal(hkBool* apResult, float epsilon) const;
};

inline hkQuadReal rdata_10C8770{-0.0f, 0.0f, -0.0f, -0.0f};
inline hkQuadReal rdata_10C8780{0.0f, -0.0f, -0.0f, -0.0f};

// The SDK's inline (hkSseVector4.inl): b rotated by r's transpose. The columns are transposed (HK_TRANSPOSE3,
// hkSseMathTypes.inl), then x, y and z of b, each broadcast, scale the rows and are summed. The exe's out-of-line copy
// is 0x4B4AD0 (thiscall, ret 8); it reads the columns through 0x4B4CF0, its out-of-line getColumn.
inline void hkVector4::_setRotatedInverseDir(const hkRotation& r, const hkVector4& b) {
	hkQuadReal c0 = r.getColumn(0).m_quad;
	hkQuadReal c1 = r.getColumn(1).m_quad;
	hkQuadReal c2 = r.getColumn(2).m_quad;
	hkQuadReal tmp = _mm_unpacklo_ps(c0, c1);
	hkQuadReal ctmp = _mm_shuffle_ps(_mm_unpackhi_ps(c0, c1), c2, _MM_SHUFFLE(3, 2, 1, 0));
	c0 = _mm_movelh_ps(tmp, c2);
	c1 = _mm_shuffle_ps(_mm_movehl_ps(c0, tmp), c2, _MM_SHUFFLE(3, 1, 1, 0));
	c2 = ctmp;

	const hkQuadReal b0 = _mm_shuffle_ps(b.m_quad, b.m_quad, _MM_SHUFFLE(0, 0, 0, 0));
	const hkQuadReal r0 = _mm_mul_ps(c0, b0);
	const hkQuadReal b1 = _mm_shuffle_ps(b.m_quad, b.m_quad, _MM_SHUFFLE(1, 1, 1, 1));
	const hkQuadReal r1 = _mm_mul_ps(c1, b1);
	const hkQuadReal b2 = _mm_shuffle_ps(b.m_quad, b.m_quad, _MM_SHUFFLE(2, 2, 2, 2));
	const hkQuadReal r2 = _mm_mul_ps(c2, b2);
	m_quad = _mm_add_ps(_mm_add_ps(r0, r1), r2);
}

// The SDK's inline (hkVector4.inl). The exe's out-of-line copy is 0x4B4AB0 (thiscall, ret 8), a call of 0x4B4AD0.
inline void hkVector4::setRotatedInverseDir(const hkRotation& t, const hkVector4& v) {
	_setRotatedInverseDir(t, v);
}
