#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Reconstruction.hpp"

class hkStringBuf;

// Eight bytes seen as integers of every width, over a whole vector: convertToUint16's output, whose low eight bytes
// hold the four 16-bit results (hkpSampledHeightFieldShape's collideSpheres reads u16[0] and u16[2]). The SDK's
// hkQuadReal member makes it 16 bytes, 16-aligned, in the exe's SSE build.
union hkIntUnion64 {
	hkQuadReal quad;
	int64_t i64;
	uint64_t u64;
	int32_t i32[2];
	uint32_t u32[2];
	int16_t i16[4];
	uint16_t u16[4];
	int8_t i8[8];
	uint8_t u8[8];
};

// Havok's vector helpers: static functions only.
class hkVector4Util {
public:
	// The correction convertToUint16 needs to floor rather than round: a binary search for the value just below 11
	// that still converts to 10, minus 11 (cdecl, ST0, 0xD97440).
	static float getFloatToInt16FloorCorrection();
	// The same correction for convertToUint32: the value just below 11 that still converts to 10, minus 11 (cdecl,
	// ST0, 0xD974F0).
	static float getFloatToInt32FloorCorrection();
	// Not named in the PDB or IDA; name chosen here. Scales arV0 and arV1 to unit length with one reciprocal square
	// root of both squared lengths (one Newton step, the exe's 3.0 and 0.5 constants). Its arguments come in ecx
	// (arV0) and eax (arV1), which it leaves as they were (ret 0, 0xCE2A50; a KEEP: its callers read both after it).
	static void normalizeVectors2(hkVector4& arV0, hkVector4& arV1);
	// The quaternion's four components, in [-1.1, 1.1], as one byte each, x in the low byte.
	static uint32_t packQuaternionIntoInt32(const hkVector4& qin);
	// Formats x's first three components into s as "%f", separated by separator; returns s's characters.
	static const char* toString3(const hkVector4& x, hkStringBuf& s, char separator);
	// The same for all four components.
	static const char* toString4(const hkVector4& x, hkStringBuf& s, char separator);

	// The SDK's inline (hkVector4Util.inl, convertToUint32WithClip on both corners): the box moved by offsetLow and
	// offsetHigh, scaled, clamped to [0, hkAabbUint32MaxVal] and rounded with fld/fistp in the FPU's mode. The SDK
	// writes the fourth lane through out[3], into the dword after m_min and after m_max: the expansion bytes and
	// m_shapeKeyByte here. The exe has no out-of-line copy.
	static void convertAabbToUint32(const hkAabb& aabb, const hkVector4& offsetLow, const hkVector4& offsetHigh,
		const hkVector4& scale, hkAabbUint32& aabbOut) {
		hkVector4 lo;
		lo.setAdd4(aabb.m_min, offsetLow);
		lo.mul4(scale);
		hkVector4 maxVal;
		maxVal.m_quad = hkAabbUint32MaxVal;
		lo.setMin4(lo, maxVal);
		lo.setMax4(lo, hkVector4Zero);
		aabbOut.m_min[0] = static_cast<uint32_t>(hkMath::hkToIntFast(lo(0)));
		aabbOut.m_min[1] = static_cast<uint32_t>(hkMath::hkToIntFast(lo(1)));
		aabbOut.m_min[2] = static_cast<uint32_t>(hkMath::hkToIntFast(lo(2)));
		const int32_t iMinW = hkMath::hkToIntFast(lo(3));
		aabbOut.m_expansionMin[0] = static_cast<uint8_t>(iMinW);
		aabbOut.m_expansionMin[1] = static_cast<uint8_t>(iMinW >> 8);
		aabbOut.m_expansionMin[2] = static_cast<uint8_t>(iMinW >> 16);
		aabbOut.m_expansionShift = static_cast<uint8_t>(iMinW >> 24);
		hkVector4 hi;
		hi.setAdd4(aabb.m_max, offsetHigh);
		hi.mul4(scale);
		hi.setMin4(hi, maxVal);
		hi.setMax4(hi, hkVector4Zero);
		aabbOut.m_max[0] = static_cast<uint32_t>(hkMath::hkToIntFast(hi(0)));
		aabbOut.m_max[1] = static_cast<uint32_t>(hkMath::hkToIntFast(hi(1)));
		aabbOut.m_max[2] = static_cast<uint32_t>(hkMath::hkToIntFast(hi(2)));
		const int32_t iMaxW = hkMath::hkToIntFast(hi(3));
		aabbOut.m_expansionMax[0] = static_cast<uint8_t>(iMaxW);
		aabbOut.m_expansionMax[1] = static_cast<uint8_t>(iMaxW >> 8);
		aabbOut.m_expansionMax[2] = static_cast<uint8_t>(iMaxW >> 16);
		aabbOut.m_shapeKeyByte = static_cast<uint8_t>(iMaxW >> 24);
	}

	// The SDK's inline (hkVector4Util.inl): how far the expanded box reaches past the unexpanded one, per axis, as
	// bytes scaled down by a shared shift (by 4 while the widest reach is 0x800 or more, then by 1 while it is 0x100
	// or more). The exe has no out-of-line copy.
	static void compressExpandedAabbUint32(const hkAabbUint32& expandedAabbInt, hkAabbUint32& unexpandedAabbInt_InOut) {
		const int32_t mix = unexpandedAabbInt_InOut.m_min[0] - expandedAabbInt.m_min[0];
		const int32_t miy = unexpandedAabbInt_InOut.m_min[1] - expandedAabbInt.m_min[1];
		const int32_t miz = unexpandedAabbInt_InOut.m_min[2] - expandedAabbInt.m_min[2];
		const int32_t maxx = expandedAabbInt.m_max[0] - unexpandedAabbInt_InOut.m_max[0];
		const int32_t may = expandedAabbInt.m_max[1] - unexpandedAabbInt_InOut.m_max[1];
		const int32_t maz = expandedAabbInt.m_max[2] - unexpandedAabbInt_InOut.m_max[2];
		int32_t m = (mix | miy) | (miz | maxx) | (may | maz);
		int32_t shift = 0;
		while (m >= 0x800) {
			shift += 4;
			m = static_cast<int32_t>(static_cast<uint32_t>(m) >> 4);
		}
		while (m >= 0x100) {
			shift += 1;
			m = static_cast<int32_t>(static_cast<uint32_t>(m) >> 1);
		}
		unexpandedAabbInt_InOut.m_expansionShift = static_cast<uint8_t>(shift);
		unexpandedAabbInt_InOut.m_expansionMin[0] = static_cast<uint8_t>(mix >> shift);
		unexpandedAabbInt_InOut.m_expansionMin[1] = static_cast<uint8_t>(miy >> shift);
		unexpandedAabbInt_InOut.m_expansionMin[2] = static_cast<uint8_t>(miz >> shift);
		unexpandedAabbInt_InOut.m_expansionMax[0] = static_cast<uint8_t>(maxx >> shift);
		unexpandedAabbInt_InOut.m_expansionMax[1] = static_cast<uint8_t>(may >> shift);
		unexpandedAabbInt_InOut.m_expansionMax[2] = static_cast<uint8_t>(maz >> shift);
	}

	// The SDK's and the PDB's hkVector4Util::hkAabbUint32MaxVal (an extern hkQuadReal in the SDK's namespace): the
	// largest value a quantised AABB coordinate takes, which the packed boxes are clamped against.
	static inline hkQuadReal hkAabbUint32MaxVal{2147352576.0f, 2147352576.0f, 2147352576.0f, 2147352576.0f};
};

// The SDK's file static of hkVector4Util.inl (convertToUint16WithClip's bias, not const: .data): every file
// including the header has its own copy (160 in the exe's .data).
static hkQuadReal convertToUint16WithClipMagicQuad{hkQuadReal{65536.0f, 65536.0f, 65536.0f, 65536.0f}};

// The vector convertToUint16 adds after scaling, so that the low bits
// of each float's mantissa hold the 16-bit integer, shifted left by 6.
inline hkQuadReal hkVector4Util_floatToUint16Magic{196608.0f, 196608.0f, 196608.0f, 196608.0f};

// packQuaternionIntoInt32's scale (128 / 1.1 in each lane) and the
// offset it adds after (196736.5), so that bits 6 to 13 of each float hold the component's byte.
inline hkQuadReal hkVector4Util_packQuaternionScale{116.36363f, 116.36363f, 116.36363f, 116.36363f};
inline hkQuadReal hkVector4Util_packQuaternionOffset{196736.5f, 196736.5f, 196736.5f, 196736.5f};
