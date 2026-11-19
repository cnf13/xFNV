#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"

class hkStringBuf;

// Eight bytes seen as integers of every width, over a whole vector: convertToUint16's output, whose low eight bytes
// hold the four 16-bit results (hkpSampledHeightFieldShape's collideSpheres reads u16[0] and u16[2]). The hkQuadReal
// member makes it 16 bytes, 16-aligned.
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
    // The correction convertToUint16 needs to floor rather than round: a binary search for the value just below 11 that
    // still converts to 10, minus 11.
    static float getFloatToInt16FloorCorrection();
    // The same correction for convertToUint32: the value just below 11 that still converts to 10, minus 11.
    static float getFloatToInt32FloorCorrection();
    // Scales arV0 and arV1 to unit length with one reciprocal square root of both squared lengths (one Newton step,
    // with the constants 3.0 and 0.5).
    static void normalizeVectors2(hkVector4& arV0, hkVector4& arV1);
    // The quaternion's four components, in [-1.1, 1.1], as one byte each, x in the low byte.
    static uint32_t packQuaternionIntoInt32(const hkVector4& qin);
    // Formats x's first three components into s as "%f", separated by separator; returns s's characters.
    static const char* toString3(const hkVector4& x, hkStringBuf& s, char separator);
    // The same for all four components.
    static const char* toString4(const hkVector4& x, hkStringBuf& s, char separator);

    // The box is moved by offsetLow and offsetHigh, scaled, clamped to [0, hkAabbUint32MaxVal] and rounded in the FPU's
    // rounding mode. The fourth lane is written through out[3], into the word after m_min and after m_max: the
    // expansion bytes and m_shapeKeyByte.
    static void convertAabbToUint32(
        const hkAabb& aabb,
        const hkVector4& offsetLow,
        const hkVector4& offsetHigh,
        const hkVector4& scale,
        hkAabbUint32& aabbOut
    ) {
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

    // Stores how far the expanded box reaches past the unexpanded one, per axis, as bytes scaled down by a shared shift
    // (by 4 while the widest reach is 0x800 or more, then by 1 while it is 0x100 or more).
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

    // The SDK's inlines below, defined after the class.
    // Each of the numVectors points vectorsIn transformed by t into vectorsOut, (y * col1 + x * col0) + (z * col2 +
    // translation) as hkVector4::_setTransformedPos, from the last down; the loop runs at least once. t's columns are
    // read first, so vectorsOut may overlap t.
    static void
    transformPoints(const hkTransform& t, const hkVector4* vectorsIn, int32_t numVectors, hkVector4* vectorsOut);
    // As transformPoints, keeping each sphere's radius in w.
    static void
    transformSpheres(const hkTransform& t, const hkVector4* vectorsIn, int32_t numVectors, hkVector4* vectorsOut);
    // Each of the numVectors vectors rotated by the transpose of t, first to last: the columns are transposed once
    // (HK_TRANSPOSE3), then each sum is (y * row1 + x * row0) + z * row2.
    static void
    rotateInversePoints(const hkRotation& t, const hkVector4* vectorsIn, int32_t numVectors, hkVector4* vectorsOut);
    // A vector perpendicular to vectorIn, not normalized: the lane of vectorIn's smallest magnitude is zero, the
    // other two are vectorIn's swapped, one negated.
    static void calculatePerpendicularVector(const hkVector4& vectorIn, hkVector4& biVectorOut);
    // (in + offset) * scale as 16-bit integers, floored by the float magic hkVector4Util_floatToUint16Magic.
    static void
    convertToUint16(const hkVector4& in, const hkVector4& offset, const hkVector4& scale, hkIntUnion64& out);
    // The inverse of packQuaternionIntoInt32: each byte less 128, times 1.1 / 128.
    static void unpackInt32IntoQuaternion(uint32_t ivalue, hkVector4& qout);
    // The three dot products a0 . b0, a1 . b1 and a2 . b2 in dotsOut's x, y and z: the products transposed
    // (HK_TRANSPOSE3) and summed.
    static void dot3_3vs3(
        const hkVector4& a0,
        const hkVector4& b0,
        const hkVector4& a1,
        const hkVector4& b1,
        const hkVector4& a2,
        const hkVector4& b2,
        hkVector4& dotsOut
    );
    // The inverse of convertAabbToUint32: the integers as floats, divided by scale, less offsetLow.
    static void convertAabbFromUint32(
        const hkAabbUint32& aabbIn,
        const hkVector4& offsetLow,
        const hkVector4& scale,
        hkAabb& aabbOut
    );
    // The inverse of compressExpandedAabbUint32: the box grown by its expansion bytes shifted up; the expansion bytes
    // of expandedAabbOut are not written.
    static void uncompressExpandedAabbUint32(const hkAabbUint32& unexpandedAabbInt, hkAabbUint32& expandedAabbOut);

    // The largest value a quantised AABB coordinate takes, which the packed boxes are clamped against.
    static inline hkQuadReal hkAabbUint32MaxVal{2147352576.0f, 2147352576.0f, 2147352576.0f, 2147352576.0f};
};

// The bias for convertToUint16WithClip is a non-const file static, so every file including the header has its own copy.
static hkQuadReal convertToUint16WithClipMagicQuad{hkQuadReal{65536.0f, 65536.0f, 65536.0f, 65536.0f}};

// The vector convertToUint16 adds after scaling, so that the low bits
// of each float's mantissa hold the 16-bit integer, shifted left by 6.
inline hkQuadReal hkVector4Util_floatToUint16Magic{196608.0f, 196608.0f, 196608.0f, 196608.0f};

// packQuaternionIntoInt32's scale (128 / 1.1 in each lane) and the
// offset it adds after (196736.5), so that bits 6 to 13 of each float hold the component's byte.
inline hkQuadReal hkVector4Util_packQuaternionScale{116.36363f, 116.36363f, 116.36363f, 116.36363f};
inline hkQuadReal hkVector4Util_packQuaternionOffset{196736.5f, 196736.5f, 196736.5f, 196736.5f};

inline void hkVector4Util::transformPoints(
    const hkTransform& t,
    const hkVector4* vectorsIn,
    int32_t numVectors,
    hkVector4* vectorsOut
) {
    const hkQuadReal c0 = t.getColumn(0).m_quad;
    const hkQuadReal c1 = t.getColumn(1).m_quad;
    const hkQuadReal c2 = t.getColumn(2).m_quad;
    const hkQuadReal c3 = t.getColumn(3).m_quad;
    int32_t i = numVectors - 1;
    do {
        const hkQuadReal b = vectorsIn[i].m_quad;
        const hkQuadReal r1 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 1, 1, 1)), c1);
        const hkQuadReal r0 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 0, 0)), c0);
        const hkQuadReal r2 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 2, 2, 2)), c2);
        vectorsOut[i].m_quad = _mm_add_ps(_mm_add_ps(r1, r0), _mm_add_ps(r2, c3));
    }
    while (--i >= 0);
}

inline void hkVector4Util::transformSpheres(
    const hkTransform& t,
    const hkVector4* vectorsIn,
    int32_t numVectors,
    hkVector4* vectorsOut
) {
    const hkQuadReal c0 = t.getColumn(0).m_quad;
    const hkQuadReal c1 = t.getColumn(1).m_quad;
    const hkQuadReal c2 = t.getColumn(2).m_quad;
    const hkQuadReal c3 = t.getColumn(3).m_quad;
    int32_t i = numVectors - 1;
    do {
        const hkQuadReal b = vectorsIn[i].m_quad;
        const hkQuadReal r1 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 1, 1, 1)), c1);
        const hkQuadReal r0 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 0, 0)), c0);
        const hkQuadReal r2 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 2, 2, 2)), c2);
        hkVector4 v;
        v.m_quad = _mm_add_ps(_mm_add_ps(r1, r0), _mm_add_ps(r2, c3));
        v.setW(vectorsIn[i]);
        vectorsOut[i] = v;
    }
    while (--i >= 0);
}

inline void hkVector4Util::rotateInversePoints(
    const hkRotation& t,
    const hkVector4* vectorsIn,
    int32_t numVectors,
    hkVector4* vectorsOut
) {
    hkQuadReal c0 = t.getColumn(0).m_quad;
    hkQuadReal c1 = t.getColumn(1).m_quad;
    hkQuadReal c2 = t.getColumn(2).m_quad;
    const hkQuadReal tmp = _mm_unpacklo_ps(c0, c1);
    const hkQuadReal ctmp = _mm_shuffle_ps(_mm_unpackhi_ps(c0, c1), c2, _MM_SHUFFLE(3, 2, 1, 0));
    c0 = _mm_movelh_ps(tmp, c2);
    c1 = _mm_shuffle_ps(_mm_movehl_ps(c0, tmp), c2, _MM_SHUFFLE(3, 1, 1, 0));
    c2 = ctmp;
    for (int32_t i = 0; i < numVectors; ++i) {
        const hkQuadReal b = vectorsIn[i].m_quad;
        const hkQuadReal r1 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 1, 1, 1)), c1);
        const hkQuadReal r0 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 0, 0)), c0);
        const hkQuadReal r2 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 2, 2, 2)), c2);
        vectorsOut[i].m_quad = _mm_add_ps(_mm_add_ps(r1, r0), r2);
    }
}

inline void hkVector4Util::calculatePerpendicularVector(const hkVector4& vectorIn, hkVector4& biVectorOut) {
    // The index of (one of) the smallest components and of the other two.
    int32_t min = 0;
    int32_t ok1 = 1;
    int32_t ok2 = 2;
    float a0 = hkMath::fabs(vectorIn(0));
    const float a1 = hkMath::fabs(vectorIn(1));
    const float a2 = hkMath::fabs(vectorIn(2));
    if (a1 < a0) {
        ok1 = 0;
        min = 1;
        a0 = a1;
    }
    if (a2 < a0) {
        ok2 = min;
    }
    biVectorOut.setZero4();
    biVectorOut(ok1) = vectorIn(ok2);
    biVectorOut(ok2) = -vectorIn(ok1);
}

inline void hkVector4Util::convertToUint16(
    const hkVector4& in,
    const hkVector4& offset,
    const hkVector4& scale,
    hkIntUnion64& out
) {
    hkVector4 x;
    x.setAdd4(in, offset);
    x.mul4(scale);
    x.m_quad = _mm_add_ps(x.m_quad, hkVector4Util_floatToUint16Magic);
    out.u16[0] = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, x(0)) >> 6);
    out.u16[1] = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, x(1)) >> 6);
    out.u16[2] = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, x(2)) >> 6);
    out.u16[3] = static_cast<uint16_t>(__builtin_bit_cast(uint32_t, x(3)) >> 6);
}

// Each byte's product is taken with the double constant (float)(1.1 / 128).
inline void hkVector4Util::unpackInt32IntoQuaternion(uint32_t ivalue, hkVector4& qout) {
    qout(0) = (static_cast<int32_t>(ivalue & 0xFF) - 0x80) * 0.008593750186264515;
    qout(1) = (static_cast<int32_t>((ivalue >> 8) & 0xFF) - 0x80) * 0.008593750186264515;
    qout(2) = (static_cast<int32_t>((ivalue >> 16) & 0xFF) - 0x80) * 0.008593750186264515;
    qout(3) = (static_cast<int32_t>(ivalue >> 24) - 0x80) * 0.008593750186264515;
}

inline void hkVector4Util::dot3_3vs3(
    const hkVector4& a0,
    const hkVector4& b0,
    const hkVector4& a1,
    const hkVector4& b1,
    const hkVector4& a2,
    const hkVector4& b2,
    hkVector4& dotsOut
) {
    hkQuadReal m0 = _mm_mul_ps(b0.m_quad, a0.m_quad);
    hkQuadReal m1 = _mm_mul_ps(b1.m_quad, a1.m_quad);
    hkQuadReal m2 = _mm_mul_ps(b2.m_quad, a2.m_quad);
    const hkQuadReal tmp = _mm_unpacklo_ps(m0, m1);
    const hkQuadReal mtmp = _mm_shuffle_ps(_mm_unpackhi_ps(m0, m1), m2, _MM_SHUFFLE(3, 2, 1, 0));
    m0 = _mm_movelh_ps(tmp, m2);
    m1 = _mm_shuffle_ps(_mm_movehl_ps(m0, tmp), m2, _MM_SHUFFLE(3, 1, 1, 0));
    m2 = mtmp;
    dotsOut.m_quad = _mm_add_ps(_mm_add_ps(m0, m1), m2);
}

inline void hkVector4Util::convertAabbFromUint32(
    const hkAabbUint32& aabbIn,
    const hkVector4& offsetLow,
    const hkVector4& scale,
    hkAabb& aabbOut
) {
    aabbOut.m_min.set(
        static_cast<float>(aabbIn.m_min[0]), static_cast<float>(aabbIn.m_min[1]), static_cast<float>(aabbIn.m_min[2]),
        0.0f
    );
    aabbOut.m_max.set(
        static_cast<float>(aabbIn.m_max[0]), static_cast<float>(aabbIn.m_max[1]), static_cast<float>(aabbIn.m_max[2]),
        0.0f
    );
    hkVector4 invScale;
    invScale.setReciprocal3(scale);
    aabbOut.m_min.mul4(invScale);
    aabbOut.m_max.mul4(invScale);
    aabbOut.m_min.sub4(offsetLow);
    aabbOut.m_max.sub4(offsetLow);
}

inline void
hkVector4Util::uncompressExpandedAabbUint32(const hkAabbUint32& unexpandedAabbInt, hkAabbUint32& expandedAabbOut) {
    const int32_t shift = unexpandedAabbInt.m_expansionShift;
    expandedAabbOut.m_min[0] = unexpandedAabbInt.m_min[0] - (unexpandedAabbInt.m_expansionMin[0] << shift);
    expandedAabbOut.m_min[1] = unexpandedAabbInt.m_min[1] - (unexpandedAabbInt.m_expansionMin[1] << shift);
    expandedAabbOut.m_min[2] = unexpandedAabbInt.m_min[2] - (unexpandedAabbInt.m_expansionMin[2] << shift);
    expandedAabbOut.m_max[0] = unexpandedAabbInt.m_max[0] + (unexpandedAabbInt.m_expansionMax[0] << shift);
    expandedAabbOut.m_max[1] = unexpandedAabbInt.m_max[1] + (unexpandedAabbInt.m_expansionMax[1] << shift);
    expandedAabbOut.m_max[2] = unexpandedAabbInt.m_max[2] + (unexpandedAabbInt.m_expansionMax[2] << shift);
}
