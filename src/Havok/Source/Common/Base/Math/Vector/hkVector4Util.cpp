#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"

#include "Havok/Source/Animation/Animation/Ik/FootPlacement/hkaFootPlacementIkSolver.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"

#include <bit>
#include <xmmintrin.h>

// Only lane 0 of the convertToUint16 result (offset hkVector4Zero, scale one) is used, so only lane 0 is computed, with
// each step rounded to float. The midpoint and the result are sums of floats times the double 0.5, stored as floats.
float hkVector4Util::getFloatToInt16FloorCorrection() {
	float l = 10.0f;
	float h = 11.0f;
	for (int32_t i = 23; i != 0; --i) {
		float m = static_cast<float>((static_cast<double>(h) + l) * 0.5);
		// EXE-INLINED none: hkVector4Util::convertToUint16
		float fConverted = (m + hkVector4Zero.m_quad[0]) * 1.0f + hkVector4Util_floatToUint16Magic[0];
		if (static_cast<uint16_t>(std::bit_cast<uint32_t>(fConverted) >> 6) < 11) {
			l = m;
		} else {
			h = m;
		}
	}
	return static_cast<float>((static_cast<double>(h) + l) * 0.5 - 11.0);
}

// Only lane 0 of the convertToUint32 result (offset hkVector4Zero, scale one) is used, so only lane 0 is computed,
// rounded in the FPU's current rounding mode. The midpoint and the result are sums of floats times the double 0.5,
// stored as floats.
float hkVector4Util::getFloatToInt32FloorCorrection() {
	float l = 10.0f;
	float h = 11.0f;
	for (int32_t i = 23; i != 0; --i) {
		float m = static_cast<float>((static_cast<double>(h) + l) * 0.5);
		// EXE-INLINED none: hkVector4Util::convertToUint32
		float fConverted = (m + hkVector4Zero.m_quad[0]) * 1.0f;
		uint32_t uiOut = static_cast<uint32_t>(hkMath::hkToIntFast(fConverted));
		if (uiOut < 11) {
			l = m;
		} else {
			h = m;
		}
	}
	return static_cast<float>((static_cast<double>(h) + l) * 0.5 - 11.0);
}

// KEEP: the exe body writes no general register, and its callers keep ecx and eax live across the call, which clang
// does not guarantee: mapPoseLocalSpace_0 pushes ecx (0xCE350C) and moves eax (0xCE350D) after the call at 0xCE3507;
// mapPose moves eax into ecx (0xCE3F67) after the call at 0xCE3F62.
void hkVector4Util::normalizeVectors2(hkVector4& arV0, hkVector4& arV1) {
	__m128 v2 = _mm_mul_ps(arV0.m_quad, arV0.m_quad);
	float v3 = _mm_shuffle_ps(v2, v2, 85)[0] + v2[0];
	__m128 v4 = _mm_shuffle_ps(v2, v2, 170);
	v4[0] = v4[0] + v3;
	__m128 v5 = _mm_mul_ps(arV1.m_quad, arV1.m_quad);
	float v6 = _mm_shuffle_ps(v5, v5, 85)[0] + v5[0];
	__m128 v7 = _mm_shuffle_ps(v5, v5, 170);
	v7[0] = v7[0] + v6;
	__m128 v8 = _mm_shuffle_ps(v4, v4, 0);
	__m128 v9 = _mm_shuffle_ps(v8, _mm_unpackhi_ps(v8, _mm_shuffle_ps(v7, v7, 0)), 196);
	__m128 v10 = _mm_rsqrt_ps(v9);
	__m128 v11 =
		_mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v10, v9), v10)), _mm_mul_ps(v10, hkQuadRealHalf));
	arV0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), arV0.m_quad);
	__m128 v12 = _mm_shuffle_ps(v11, v11, 255);
	arV1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), arV1.m_quad);
}

// Takes each lane's bits 6 to 13 with masks and shifts and merges them into one 32-bit value; w's bits above 13 shift
// out of it.
uint32_t hkVector4Util::packQuaternionIntoInt32(const hkVector4& qin) {
	hkQuadReal u =
		_mm_add_ps(_mm_mul_ps(qin.m_quad, hkVector4Util_packQuaternionScale), hkVector4Util_packQuaternionOffset);
	uint32_t x = std::bit_cast<uint32_t>(u[0]);
	uint32_t y = std::bit_cast<uint32_t>(u[1]);
	uint32_t z = std::bit_cast<uint32_t>(u[2]);
	uint32_t w = std::bit_cast<uint32_t>(u[3]);
	return ((x >> 6) & 0xFF) | (((y >> 6) & 0xFF) << 8) | (((z >> 6) & 0xFF) << 16) | ((w >> 6) << 24);
}

// The components are read as floats and passed promoted to double.
const char* hkVector4Util::toString3(const hkVector4& x, hkStringBuf& s, char separator) {
	s.printf("%f%c%f%c%f", x.m_quad[0], separator, x.m_quad[1], separator, x.m_quad[2]);
	return s.m_string.begin();
}

const char* hkVector4Util::toString4(const hkVector4& x, hkStringBuf& s, char separator) {
	s.printf("%f%c%f%c%f%c%f", x.m_quad[0], separator, x.m_quad[1], separator, x.m_quad[2], separator, x.m_quad[3]);
	return s.m_string.begin();
}
