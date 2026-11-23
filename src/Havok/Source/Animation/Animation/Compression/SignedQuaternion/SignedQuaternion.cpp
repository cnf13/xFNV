#include "Havok/Source/Animation/Animation/Compression/Common/hkaProcessFlyingColors.h"
#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

void packSignedQuaternion16(const hkQuaternion* in, uint8_t* out) {
    const float* pIn = reinterpret_cast<const float*>(in);
    char v4 = 7 - static_cast<int32_t>(pIn[2] * -7.0);
    double v2 = -7.0 * pIn[3];
    char result = v4 | 16 * (7 - static_cast<int32_t>(v2));
    out[0] = 7 - static_cast<int32_t>(pIn[0] * -7.0) | 16 * (7 - static_cast<int32_t>(pIn[1] * -7.0));
    out[1] = result;
}

// The SDK's form (the THREECOMP24 rotation writer): the index of the largest component of |in| and its sign, and the
// other three components, each scaled into seven bits, packed into three bytes at out.
void packSignedQuaternion24(const hkQuaternion* in, uint8_t* out) {
    // Function-local scale of packSignedQuaternion24: sqrt(2) * 63.
    static const float scale = static_cast<float>(std::sqrt(2.0)) * 63.0;
    const __m128 v11 = _mm_and_ps(in->m_vec.m_quad, hkVector4::absMask);
    int32_t v2 = XFNV_M128_F32(v11, 0) < static_cast<double>(XFNV_M128_F32(v11, 1));
    if (XFNV_M128_F32(v11, v2) < static_cast<double>(XFNV_M128_F32(v11, 2))) {
        v2 = 2;
    }
    if (XFNV_M128_F32(v11, v2) < static_cast<double>(XFNV_M128_F32(v11, 3))) {
        v2 = 3;
    }
    int32_t v3 = v2 == 0;
    char v4 = static_cast<int32_t>(XFNV_M128_F32(in->m_vec.m_quad, v3) * scale) + 63;
    int32_t v5 = (v3 + 1 == v2) + (v3 + 1);
    char v10 = static_cast<int32_t>(XFNV_M128_F32(in->m_vec.m_quad, v5) * scale) + 63;
    double v6 = scale * XFNV_M128_F32(in->m_vec.m_quad, v5 + 1 + (v5 + 1 == v2));
    uint8_t v7 = static_cast<int32_t>(v6) + 63;
    out[0] = v4 | static_cast<uint8_t>(v2) << 7;
    out[1] = v10 | (v2 & 254) << 6;
    out[2] = v7;
    if (XFNV_M128_F32(in->m_vec.m_quad, v2) < 0.0f) {
        out[2] = v7 | 128;
    }
}

// Three sign bits and the fourth's, an angle pair of the normalised x, y, z and the magnitude of w, packed into one
// 32-bit word at out8.
void packSignedQuaternion32(const hkQuaternion* in, uint8_t* out8) {
    // out8 is untyped bytes written as one 32-bit value.
    uint32_t* pOut = reinterpret_cast<uint32_t*>(out8);
    __m128 m_quad = in->m_vec.m_quad;
    *pOut = 0;
    // Each component is tested against 0: the bit is set only for "below 0", never for an unordered NaN.
    *pOut = ((XFNV_M128_F32(m_quad, 0) < 0.0f) |
             2 *
                 ((XFNV_M128_F32(m_quad, 1) < 0.0f) |
                  2 * ((XFNV_M128_F32(m_quad, 2) < 0.0f) | 2 * (XFNV_M128_F32(m_quad, 3) < 0.0f))))
        << 28;
    __m128 v5 = _mm_and_ps(hkVector4::absMask, m_quad);
    if (0.0f != XFNV_M128_F32(v5, 0) || (0.0f != XFNV_M128_F32(v5, 1) || 0.0f != XFNV_M128_F32(v5, 2))) {
        __m128 v6 = _mm_mul_ps(v5, v5);
        float v7 = XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0);
        __m128 v8 = _mm_shuffle_ps(v6, v6, 170);
        XFNV_M128_F32(v8, 0) = XFNV_M128_F32(v8, 0) + v7;
        float v9 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v8, 0))));
        __m128 v10 = _mm_setzero_ps();
        __m128 v11 = _mm_cmpeq_ss(v8, v10);
        XFNV_M128_F32(v10, 0) = 3.0f - ((XFNV_M128_F32(v8, 0) * v9) * v9);
        __m128 v12 = _mm_setzero_ps();
        XFNV_M128_F32(v12, 0) = (0.5f * v9) * XFNV_M128_F32(v10, 0);
        __m128 v13 = _mm_andnot_ps(v11, v12);
        __m128 afY = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v5);
        double X = XFNV_M128_F32(afY, 2);
        float v18 = std::fabs(static_cast<double>(XFNV_M128_F32(afY, 2)));
        float v21;
        if (!(v18 >= 1.0f)) {
            v21 = std::acos(X);
        }
        else if (!(X > 0.0f)) {
            v21 = 3.1415927f;
        }
        else {
            v21 = 0.0f;
        }
        int32_t v16 = static_cast<int32_t>((v21 + v21) / 3.1415927410125732 * 511.0 + 0.5);
        double v17 = hkMath_atan2fApproximation(XFNV_M128_F32(afY, 1), XFNV_M128_F32(afY, 0));
        float v22 = 1.0 - XFNV_M128_F32(v5, 3);
        float v23 = std::sqrt(static_cast<double>(v22));
        *pOut |= v16 * v16 +
                static_cast<int32_t>((v17 + v17) / 3.1415927410125732 * static_cast<double>(2 * v16) + 0.5) |
            static_cast<uint32_t>(static_cast<int64_t>(v23 * 1023.0 + 0.5)) << 18;
    }
}

void packSignedQuaternion40(const hkQuaternion* in, uint8_t* out) {
    static const float scale = static_cast<float>(std::sqrt(2.0)) * 2047.0;
    const __m128 absIn = _mm_and_ps(in->m_vec.m_quad, hkVector4::absMask);
    int32_t n2 = XFNV_M128_F32(absIn, 0) < static_cast<double>(XFNV_M128_F32(absIn, 1));
    if (XFNV_M128_F32(absIn, n2) < static_cast<double>(XFNV_M128_F32(absIn, 2))) {
        n2 = 2;
    }
    if (XFNV_M128_F32(absIn, n2) < static_cast<double>(XFNV_M128_F32(absIn, 3))) {
        n2 = 3;
    }
    int32_t v3 = n2 == 0;
    uint16_t usA = static_cast<int32_t>(XFNV_M128_F32(in->m_vec.m_quad, v3) * scale) + 2047;
    int32_t v4 = (v3 + 1 == n2) + (v3 + 1);
    uint16_t usB = static_cast<int32_t>(XFNV_M128_F32(in->m_vec.m_quad, v4) * scale) + 2047;
    uint16_t usC = static_cast<int32_t>(scale * XFNV_M128_F32(in->m_vec.m_quad, v4 + 1 + (v4 + 1 == n2))) + 2047;
    out[0] = static_cast<uint8_t>(usA);
    out[1] = static_cast<uint8_t>(usA >> 8) | static_cast<uint8_t>(usB << 4);
    out[3] = static_cast<uint8_t>(usC);
    out[2] = static_cast<uint8_t>(usB >> 4);
    out[4] = static_cast<uint8_t>((n2 | 4 * (XFNV_M128_F32(in->m_vec.m_quad, n2) < 0.0f)) << 4) |
        static_cast<uint8_t>(usC >> 8);
}

void packSignedQuaternion48(const hkQuaternion* in, uint8_t* out8) {
    // Guarded function-local scale: sqrt(2) * 16383.
    static const float scale = static_cast<float>(std::sqrt(2.0)) * 16383.0;
    // out8 is untyped bytes, written as three 16-bit words.
    uint16_t* out = reinterpret_cast<uint16_t*>(out8);
    const __m128 absIn = _mm_and_ps(in->m_vec.m_quad, hkVector4::absMask);
    int16_t n2 = XFNV_M128_F32(absIn, 0) < static_cast<double>(XFNV_M128_F32(absIn, 1));
    if (XFNV_M128_F32(absIn, n2) < static_cast<double>(XFNV_M128_F32(absIn, 2))) {
        n2 = 2;
    }
    if (XFNV_M128_F32(absIn, n2) < static_cast<double>(XFNV_M128_F32(absIn, 3))) {
        n2 = 3;
    }
    int32_t n2_1 = n2;
    int32_t v4 = n2 == 0;
    uint16_t usA = static_cast<int32_t>(XFNV_M128_F32(in->m_vec.m_quad, v4) * scale) + 16383;
    int32_t v5 = (v4 + 1 == n2) + (v4 + 1);
    uint16_t usB = static_cast<int32_t>(XFNV_M128_F32(in->m_vec.m_quad, v5) * scale) + 16383;
    uint16_t usC = static_cast<int32_t>(scale * XFNV_M128_F32(in->m_vec.m_quad, v5 + 1 + (v5 + 1 == n2))) + 16383;
    out[0] = usA | n2 << 15;
    out[1] = usB | (n2 & 0xFFFE) << 14;
    out[2] = usC | static_cast<uint16_t>(XFNV_M128_F32(in->m_vec.m_quad, n2_1) < 0.0f) << 15;
}

// The SDK's form (the UNCOMPRESSED rotation writer): in's four components copied to out8.
void packSignedQuaternion128(const hkQuaternion* in, uint8_t* out8) {
    // out8 is untyped bytes written as four floats.
    float* pOut = reinterpret_cast<float*>(out8);
    pOut[0] = XFNV_M128_F32(in->m_vec.m_quad, 0);
    pOut[1] = XFNV_M128_F32(in->m_vec.m_quad, 1);
    pOut[2] = XFNV_M128_F32(in->m_vec.m_quad, 2);
    pOut[3] = XFNV_M128_F32(in->m_vec.m_quad, 3);
}

void unpackSignedQuaternion16(const uint8_t* in, hkQuaternion* out) {
    // The float nearest 1/7, widened to double.
    const double kScale = 0.1428571492433548;
    uint8_t v2 = in[1];
    double v3 = static_cast<double>((in[0] & 15) - 7) * kScale;
    int32_t v4 = (in[0] >> 4) - 7;
    out->m_vec.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(out->m_vec.m_quad, 0) = v3;
    XFNV_M128_F32(out->m_vec.m_quad, 1) = static_cast<double>(v4) * kScale;
    XFNV_M128_F32(out->m_vec.m_quad, 2) = static_cast<double>((v2 & 15) - 7) * kScale;
    XFNV_M128_F32(out->m_vec.m_quad, 3) = kScale * static_cast<double>((v2 >> 4) - 7);
    out->normalize();
}

void unpackSignedQuaternion24(const uint8_t* in, hkQuaternion* out) {
    static const float scale = 1.0 / (static_cast<float>(std::sqrt(2.0)) * 63.0);
    uint8_t v2 = in[1];
    uint8_t v3 = in[2] & 127;
    uint32_t v4 = (in[0] >> 1 | (v2 & 128u)) >> 6;
    double v5 = static_cast<double>((in[0] & 127) - 63);
    int32_t v6 = (v4 == 0) + 1;
    double dScale = scale;
    int32_t v9 = (v6 == v4) + v6 + 1;
    float v11 = dScale * static_cast<double>(v3 - 63);
    out->m_vec.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(out->m_vec.m_quad, v6 - 1) = v5 * dScale;
    XFNV_M128_F32(out->m_vec.m_quad, v9 - 1) = static_cast<double>((v2 & 127) - 63) * dScale;
    XFNV_M128_F32(out->m_vec.m_quad, v9 + (v9 == v4)) = v11;
    float v12 = 1.0 - XFNV_M128_F32(out->m_vec.m_quad, 0) * XFNV_M128_F32(out->m_vec.m_quad, 0) -
        XFNV_M128_F32(out->m_vec.m_quad, 1) * XFNV_M128_F32(out->m_vec.m_quad, 1) -
        XFNV_M128_F32(out->m_vec.m_quad, 2) * XFNV_M128_F32(out->m_vec.m_quad, 2) -
        XFNV_M128_F32(out->m_vec.m_quad, 3) * XFNV_M128_F32(out->m_vec.m_quad, 3);
    float v13 = std::sqrt(static_cast<double>(v12));
    XFNV_M128_F32(out->m_vec.m_quad, v4) = v13;
    if ((in[2] & 128) != 0) {
        XFNV_M128_F32(out->m_vec.m_quad, v4) = -v13;
    }
}

void unpackSignedQuaternion32(const uint8_t* in8, hkQuaternion* out) {
    const uint32_t* pIn = reinterpret_cast<const uint32_t*>(in8);
    float* pOut = reinterpret_cast<float*>(out);
    double v5;
    int32_t v3 = pIn[0] & 0x3FFFF;
    float v11 = 1.0 -
        static_cast<double>(pIn[0] >> 18 & 1023) / 1023.0 * (static_cast<double>(pIn[0] >> 18 & 1023) / 1023.0);
    float rootF = std::sqrt(static_cast<double>(v3));
    int32_t v4 = static_cast<int32_t>(rootF);
    float rowF = static_cast<float>(v4);
    float v9 = rowF / 511.0 * 0.5 * 3.1415927410125732;
    float rowTwice = rowF + rowF;
    if (rowTwice == 0.0f) {
        v5 = 0.0;
    }
    else {
        v5 = 0.5 * (static_cast<double>(v3 - v4 * v4) / rowTwice) * 3.1415927410125732;
    }
    float v7 = v5;
    float xyzLengthSq = 1.0 - v11 * v11;
    float xyzLength = std::sqrt(static_cast<double>(xyzLengthSq));
    float v8 = std::sin(static_cast<double>(v9));
    float v12 = std::cos(static_cast<double>(v7));
    float v13 = v12 * v8;
    float v14 = v13 * xyzLength;
    pOut[0] = v14;
    float sinPhi = std::sin(static_cast<double>(v7));
    float sinPhiSinTheta = sinPhi * v8;
    float outY = sinPhiSinTheta * xyzLength;
    pOut[1] = outY;
    float v10 = std::cos(static_cast<double>(v9));
    float outZ = v10 * xyzLength;
    pOut[2] = outZ;
    pOut[3] = v11;
    if ((pIn[0] & 0x10000000) != 0) {
        pOut[0] = -v14;
    }
    if ((pIn[0] & 0x20000000) != 0) {
        pOut[1] = -outY;
    }
    if ((pIn[0] & 0x40000000) != 0) {
        pOut[2] = -outZ;
    }
    if ((pIn[0] & 0x80000000u) != 0) {
        pOut[3] = -v11;
    }
}

void unpackSignedQuaternion40(const uint8_t* in, hkQuaternion* out) {
    // Guarded function-local scale: 1 / (sqrt(2) * 2047).
    static const float scale = 1.0 / (static_cast<float>(std::sqrt(2.0)) * 2047.0);
    int16_t v2 = in[1] >> 4 | 16 * in[2];
    uint8_t v9 = in[4];
    int16_t v14 = in[3] | (v9 & 15) << 8;
    int32_t v3 = v9 >> 4 & 3;
    double v4 = static_cast<double>((in[0] | static_cast<uint16_t>((in[1] & 15) << 8)) - 2047);
    int32_t v5 = (v3 == 0) + 1;
    double dScale = scale;
    int32_t v8 = (v5 == v3) + v5;
    float v11 = dScale * static_cast<double>(v14 - 2047);
    out->m_vec.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(out->m_vec.m_quad, v5 - 1) = v4 * dScale;
    XFNV_M128_F32(out->m_vec.m_quad, v8) = static_cast<double>(v2 - 2047) * dScale;
    XFNV_M128_F32(out->m_vec.m_quad, v8 + 1 + (v8 + 1 == v3)) = v11;
    float v12 = 1.0 - XFNV_M128_F32(out->m_vec.m_quad, 0) * XFNV_M128_F32(out->m_vec.m_quad, 0) -
        XFNV_M128_F32(out->m_vec.m_quad, 1) * XFNV_M128_F32(out->m_vec.m_quad, 1) -
        XFNV_M128_F32(out->m_vec.m_quad, 2) * XFNV_M128_F32(out->m_vec.m_quad, 2) -
        XFNV_M128_F32(out->m_vec.m_quad, 3) * XFNV_M128_F32(out->m_vec.m_quad, 3);
    float v13 = std::sqrt(static_cast<double>(v12));
    XFNV_M128_F32(out->m_vec.m_quad, v3) = v13;
    if ((in[4] & 64) != 0) {
        XFNV_M128_F32(out->m_vec.m_quad, v3) = -v13;
    }
}

void unpackSignedQuaternion48(const uint8_t* in8, hkQuaternion* out) {
    const uint16_t* in16 = reinterpret_cast<const uint16_t*>(in8);
    static const float scale = 1.0 / (static_cast<float>(std::sqrt(2.0)) * 16383.0);
    uint16_t v2 = in16[1];
    int16_t v16 = in16[2] & 0x7FFF;
    uint32_t v3 = (in16[0] >> 1 | (v2 & 0x8000u)) >> 14;
    double v4 = static_cast<double>((in16[0] & 0x7FFF) - 0x3FFF);
    int32_t v5 = (v3 == 0) + 1;
    double v6 = v4 * scale;
    double v7 = scale;
    int32_t v8 = (v5 == v3) + v5;
    float v13 = v7 * static_cast<double>(v16 - 0x3FFF);
    out->m_vec.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(out->m_vec.m_quad, v5 - 1) = v6;
    XFNV_M128_F32(out->m_vec.m_quad, v8) = static_cast<double>((v2 & 0x7FFF) - 0x3FFF) * v7;
    XFNV_M128_F32(out->m_vec.m_quad, v8 + 1 + (v8 + 1 == v3)) = v13;
    __m128 v9 = _mm_mul_ps(out->m_vec.m_quad, out->m_vec.m_quad);
    __m128 v10 = _mm_add_ps(_mm_shuffle_ps(v9, v9, 78), v9);
    float v14 = 1.0 - (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 177), 0) + XFNV_M128_F32(v10, 0));
    float v15 = std::sqrt(static_cast<double>(v14));
    XFNV_M128_F32(out->m_vec.m_quad, v3) = v15;
    if ((in16[2] & 0x8000) != 0) {
        XFNV_M128_F32(out->m_vec.m_quad, v3) = -v15;
    }
}

void unpackSignedQuaternion128(const uint8_t* in8, hkQuaternion* out) {
    // Reads the packed track bytes at in as four floats (untyped data).
    const float* in32 = reinterpret_cast<const float*>(in8);
    XFNV_M128_F32(out->m_vec.m_quad, 0) = in32[0];
    XFNV_M128_F32(out->m_vec.m_quad, 1) = in32[1];
    XFNV_M128_F32(out->m_vec.m_quad, 2) = in32[2];
    XFNV_M128_F32(out->m_vec.m_quad, 3) = in32[3];
}

void handleEndianSignedQuaternion24(uint8_t* dataInOut) {
    ;
}

void handleEndianSignedQuaternion16(uint8_t* dataInOut) {
    ;
}

void handleEndianSignedQuaternion40(uint8_t* dataInOut) {
    ;
}

const uint8_t* computePackedNurbsOffsets(
    const uint8_t* base,
    const uint32_t* m_blockOffsets,
    uint32_t block,
    uint32_t maskAndQuantizationSize
) {
    if (HK_flyingcolors_animation) {
        return &base[(maskAndQuantizationSize & 0x7FFFFFFF) + m_blockOffsets[block]];
    }
    hkaProcessFlyingColors(&HK_flyingcolors_animation);
    if (HK_flyingcolors_animation) {
        return &base[(maskAndQuantizationSize & 0x7FFFFFFF) + m_blockOffsets[block]];
    }

    return nullptr;
}

// The SDK's form: the four bytes of a POLAR32 quaternion reversed in place.
void handleEndianSignedQuaternion32(uint8_t* dataInOut) {
    uint8_t b0 = dataInOut[0];
    dataInOut[0] = dataInOut[3];
    dataInOut[3] = b0;
    uint8_t b1 = dataInOut[1];
    dataInOut[1] = dataInOut[2];
    dataInOut[2] = b1;
}

void handleEndianSignedQuaternion48(uint8_t* dataInOut) {
    char v2 = dataInOut[0];
    dataInOut[0] = dataInOut[1];
    dataInOut[1] = v2;
    char v3 = dataInOut[2];
    dataInOut[2] = dataInOut[3];
    dataInOut[3] = v3;
    char v4 = dataInOut[4];
    dataInOut[4] = dataInOut[5];
    dataInOut[5] = v4;
}

void handleEndianSignedQuaternion128(uint8_t* dataInOut) {
    int8_t v2 = dataInOut[0];
    dataInOut[0] = dataInOut[3];
    dataInOut[3] = v2;
    int8_t v3 = dataInOut[1];
    dataInOut[1] = dataInOut[2];
    dataInOut[2] = v3;
    int8_t v4 = dataInOut[4];
    dataInOut[4] = dataInOut[7];
    dataInOut[7] = v4;
    int8_t v5 = dataInOut[5];
    dataInOut[5] = dataInOut[6];
    dataInOut[6] = v5;
    int8_t v6 = dataInOut[8];
    dataInOut[8] = dataInOut[11];
    dataInOut[11] = v6;
    int8_t v7 = dataInOut[9];
    dataInOut[9] = dataInOut[10];
    dataInOut[10] = v7;
}
