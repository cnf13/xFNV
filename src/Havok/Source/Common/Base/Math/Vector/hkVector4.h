#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <stdint.h>
#include <xmmintrin.h>

class hkMatrix3;
class hkMatrix4;
class hkMotionState;
class hkQsTransform;
class hkQuaternion;
class hkRotation;
class hkTransform;

// The four floats a Havok vector or simd real is stored in.
using hkQuadReal = __m128;

// The four floats of an hkQuadReal as an array (hkSseMathTypes.inl:25).
union hkQuadRealUnion {
    float r[4];
    hkQuadReal q;
};

// The absMask used by hkVector4::setAbs4 (0x7FFFFFFF in each lane); declared as hkQuadReal because every reader loads
// it as a quad.
inline constexpr hkQuadReal hkVector4_setAbs4_absMask{
    __builtin_bit_cast(float, 0x7fffffffu), __builtin_bit_cast(float, 0x7fffffffu),
    __builtin_bit_cast(float, 0x7fffffffu), __builtin_bit_cast(float, 0x7fffffffu)
};

// The mask table for hkVector4::zeroElement: row i has lane i clear and the others all ones. It is declared as
// hkQuadReal rather than hkUint32[4][4] because every reader loads it as a quad.
using hkVector4_zeroElement_cxTable = hkQuadReal[4];
inline constexpr hkVector4_zeroElement_cxTable hkVector4_zeroElement_cx{
    {0.0f, __builtin_bit_cast(float, 0xffffffffu), __builtin_bit_cast(float, 0xffffffffu),
     __builtin_bit_cast(float, 0xffffffffu)},
    {__builtin_bit_cast(float, 0xffffffffu), 0.0f, __builtin_bit_cast(float, 0xffffffffu),
     __builtin_bit_cast(float, 0xffffffffu)},
    {__builtin_bit_cast(float, 0xffffffffu), __builtin_bit_cast(float, 0xffffffffu), 0.0f,
     __builtin_bit_cast(float, 0xffffffffu)},
    {__builtin_bit_cast(float, 0xffffffffu), __builtin_bit_cast(float, 0xffffffffu),
     __builtin_bit_cast(float, 0xffffffffu), 0.0f}
};

// The result of a four lane vector comparison: a lane mask.
class alignas(16) hkVector4Comparison {
public:
    // The lanes a mask selects.
    enum Mask : int32_t {
        MASK_NONE = 0x0,
        MASK_X = 0x1,
        MASK_Y = 0x2,
        MASK_XY = 0x3,
        MASK_Z = 0x4,
        MASK_XZ = 0x5,
        MASK_YZ = 0x6,
        MASK_XYZ = 0x7,
        MASK_W = 0x8,
        MASK_XW = 0x9,
        MASK_YW = 0xA,
        MASK_XYW = 0xB,
        MASK_ZW = 0xC,
        MASK_XZW = 0xD,
        MASK_YZW = 0xE,
        MASK_XYZW = 0xF,
    };

    // Whether every lane of m is set: returns 0 or 1.
    int32_t allAreSet(Mask m) const {
        return (_mm_movemask_ps(m_mask) & m) == m;
    }

    // The lanes set in both a and b.
    void setAnd(const hkVector4Comparison& a, const hkVector4Comparison& b) {
        m_mask = _mm_and_ps(a.m_mask, b.m_mask);
    }

    hkQuadReal m_mask;
};

// A four component vector; x, y, z are the vector, w carries a simd real's lanes.
class alignas(16) hkVector4 {
public:
    // Stores nothing: a default constructed vector keeps whatever its memory held, as in the engine. Defaulted rather
    // than an empty body so that objects with hkVector4 members can be constant-initialised.
    hkVector4() = default;
    hkVector4(float afX, float afY, float afZ, float afW);
    hkVector4(const hkVector4&) = default;

    // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
    HK_DECLARE_POD_TYPE();
    void operator=(const hkVector4& v) {
        m_quad = v.m_quad;
    }
    // Defined in hkRotation.h.
    void setRotatedInverseDir(const hkRotation& arRotation, const hkVector4& arVector);

    // The direction rotated by the quaternion or by its inverse. Defined in hkQuaternion.h.
    void setRotatedDir(const hkQuaternion& quat, const hkVector4& direction);
    void setRotatedInverseDir(const hkQuaternion& quat, const hkVector4& direction);

    // The point v transformed by t: the rotated vector plus the translation.
    void setTransformedPos(const hkTransform& t, const hkVector4& v);
    // The point v transformed by the quaternion-scale transform t: v scaled, rotated, then translated.
    void setTransformedPos(const hkQsTransform& t, const hkVector4& v);

    // Sets x, y and z from xyz, keeping w.
    void setXYZ(const hkVector4& xyz) {
        m_quad = _mm_shuffle_ps(xyz.m_quad, _mm_unpackhi_ps(xyz.m_quad, m_quad), _MM_SHUFFLE(3, 0, 1, 0));
    }

    // Whether x, y and z are valid floats.
    hkBool isOk3() const;

    // Nonzero when x, y and z are each within epsilon of v's.
    int32_t equals3(const hkVector4& v, float epsilon) const {
        hkVector4 t;
        t.setSub4(*this, v);
        t.setAbs4(t);
        hkVector4 epsilon_v;
        epsilon_v.setAll(epsilon);
        return t.compareLessThanEqual4(epsilon_v).allAreSet(hkVector4Comparison::MASK_XYZ);
    }

    void setMin4(const hkVector4& a, const hkVector4& b) {
        m_quad = _mm_min_ps(a.getQuad(), b.getQuad());
    }
    void setMax4(const hkVector4& a, const hkVector4& b) {
        m_quad = _mm_max_ps(a.getQuad(), b.getQuad());
    }
    // Sets x, y and z to the value and keeps w, as the game does, unlike the Havok SDK version, which sets all four
    // lanes.
    void setAll3(float a);
    // The address of component aiIndex: the four floats of m_quad read as an array.
    float& operator()(int32_t aiIndex) {
        return reinterpret_cast<float*>(&m_quad)[aiIndex];
    }
    const float& operator()(int32_t aiIndex) const {
        return reinterpret_cast<const float*>(&m_quad)[aiIndex];
    }
    void set(float x, float y, float z, float w) {
        m_quad = _mm_setr_ps(x, y, z, w);
    }
    void setAll(float a) {
        m_quad = _mm_set_ps1(a);
    }
    void setAdd4(const hkVector4& v0, const hkVector4& v1) {
        m_quad = _mm_add_ps(v0.m_quad, v1.m_quad);
    }
    void setSub4(const hkVector4& v0, const hkVector4& v1) {
        m_quad = _mm_sub_ps(v0.m_quad, v1.m_quad);
    }
    void add4(const hkVector4& a) {
        m_quad = _mm_add_ps(m_quad, a.m_quad);
    }
    void sub4(const hkVector4& a) {
        m_quad = _mm_sub_ps(m_quad, a.m_quad);
    }
    void setZero4() {
        m_quad = _mm_setzero_ps();
    }
    // The componentwise absolute value of x, with the sign bits cleared.
    void setAbs4(const hkVector4& x) {
        m_quad = _mm_and_ps(x.m_quad, hkVector4_setAbs4_absMask);
    }
    // The vector a multiplied by the matrix r.
    void setMul3(const hkMatrix3& r, const hkVector4& b);
    const hkQuadReal& getQuad() const {
        return m_quad;
    }

    // Returns the component i in all four lanes, or as is for 0. Any index past 2 reads w.
    hkSimdReal getSimdAt(int32_t i) const {
        hkSimdReal kResult;
        switch (i) {
            case 0: XFNV_RECONSTRUCT(hkSimdReal)(&kResult, m_quad); break;
            case 1: {
                hkQuadReal kLane = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(1, 1, 1, 1));
                kResult.~hkSimdReal();
                ::new (&kResult) hkSimdReal(kLane);
                break;
            }
            case 2: {
                hkQuadReal kLane = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(2, 2, 2, 2));
                kResult.~hkSimdReal();
                ::new (&kResult) hkSimdReal(kLane);
                break;
            }
            default: {
                hkQuadReal kLane = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(3, 3, 3, 3));
                kResult.~hkSimdReal();
                ::new (&kResult) hkSimdReal(kLane);
                break;
            }
        }
        return kResult;
    }

    // 1 / |xyzw|: rsqrtss refined by one Newton step, 0.5 * e * (3 - len2 * e * e), and 0 where len2 is 0.
    hkSimdReal lengthInverse4() const {
        const hkQuadReal half = _mm_set_ss(0.5f);
        const hkQuadReal three = _mm_set_ss(3.0f);
        const hkQuadReal zero = _mm_set_ss(0.0f);
        const hkQuadReal len2 = dot4(*this).getQuad();
        const hkQuadReal len2EqualsZero = _mm_cmpeq_ss(len2, zero);
        const hkQuadReal approx = _mm_rsqrt_ss(len2);
        const hkQuadReal refined =
            _mm_mul_ss(_mm_mul_ss(half, approx), _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(len2, approx), approx)));
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_andnot_ps(len2EqualsZero, refined));
        return kResult;
    }

    // As lengthInverse4, on dot3.
    hkSimdReal lengthInverse3() const {
        const hkQuadReal half = _mm_set_ss(0.5f);
        const hkQuadReal three = _mm_set_ss(3.0f);
        const hkQuadReal zero = _mm_set_ss(0.0f);
        const hkQuadReal len2 = dot3(*this).getQuad();
        const hkQuadReal len2EqualsZero = _mm_cmpeq_ss(len2, zero);
        const hkQuadReal approx = _mm_rsqrt_ss(len2);
        const hkQuadReal refined =
            _mm_mul_ss(_mm_mul_ss(half, approx), _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(len2, approx), approx)));
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_andnot_ps(len2EqualsZero, refined));
        return kResult;
    }

    // The products' lanes are summed y + x, then z + that; the upper lanes are the shuffles'.
    hkSimdReal dot3(const hkVector4& x) const {
        hkQuadReal x2 = _mm_mul_ps(m_quad, x.m_quad);
        hkQuadReal xySum = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)), x2);
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)), xySum));
        return kResult;
    }

    hkSimdReal length3() const {
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_sqrt_ss(dot3(*this).getQuad()));
        return kResult;
    }

    // The vector scaled by the broadcast inverse length over xyz or xyzw.
    void normalize3() {
        hkQuadReal lenInv = lengthInverse3().broadcast().m_quad;
        m_quad = _mm_mul_ps(lenInv, m_quad);
    }

    void normalize4() {
        hkQuadReal lenInv = lengthInverse4().broadcast().m_quad;
        m_quad = _mm_mul_ps(lenInv, m_quad);
    }

    // The four components scaled by s.
    void mul4(const hkSimdReal& s) {
        m_quad = _mm_mul_ps(s.broadcast().m_quad, m_quad);
    }

    // v1 scaled by r.
    void setMul4(const hkSimdReal& r, const hkVector4& v1) {
        m_quad = _mm_mul_ps(r.broadcast().m_quad, v1.m_quad);
    }

    // hkVector4Parameter is a const reference here and hkBool32 a plain int. An int of up to 24 bits kept in w, under
    // the bits 0x3f000000.
    void setInt24W(int32_t value) {
        reinterpret_cast<uint32_t*>(&m_quad)[3] = static_cast<uint32_t>(value | 0x3f000000);
    }

    int32_t getInt24W() const {
        return static_cast<int32_t>(reinterpret_cast<const uint32_t*>(&m_quad)[3] & ~0x3f000000u);
    }

    void add3clobberW(const hkVector4& a) {
        m_quad = _mm_add_ps(m_quad, a.m_quad);
    }
    void sub3clobberW(const hkVector4& a) {
        m_quad = _mm_sub_ps(m_quad, a.m_quad);
    }
    // Componentwise product and quotient.
    void mul4(const hkVector4& v) {
        m_quad = _mm_mul_ps(m_quad, v.m_quad);
    }
    void setMul4(const hkVector4& a, const hkVector4& b) {
        m_quad = _mm_mul_ps(a.m_quad, b.m_quad);
    }
    void setDiv4(const hkVector4& a, const hkVector4& b) {
        m_quad = _mm_div_ps(a.m_quad, b.m_quad);
    }
    void div4(const hkVector4& a) {
        m_quad = _mm_div_ps(m_quad, a.m_quad);
    }

    // The fast forms multiply by rcpps's estimate of the reciprocal.
    void setDiv4fast(const hkVector4& a, const hkVector4& b) {
        const hkQuadReal t = _mm_rcp_ps(b.m_quad);
        m_quad = _mm_mul_ps(a.m_quad, t);
    }

    void div4fast(const hkVector4& a) {
        const hkQuadReal t = _mm_rcp_ps(a.m_quad);
        m_quad = _mm_mul_ps(m_quad, t);
    }

    // this += r * v1, this += x * y (componentwise), and the subtracting forms.
    void addMul4(const hkSimdReal& r, const hkVector4& v1) {
        m_quad = _mm_add_ps(m_quad, _mm_mul_ps(r.broadcast().m_quad, v1.m_quad));
    }

    void addMul4(const hkVector4& x, const hkVector4& y) {
        m_quad = _mm_add_ps(m_quad, _mm_mul_ps(x.m_quad, y.m_quad));
    }

    void subMul4(const hkSimdReal& r, const hkVector4& v1) {
        m_quad = _mm_sub_ps(m_quad, _mm_mul_ps(r.broadcast().m_quad, v1.m_quad));
    }

    void subMul4(const hkVector4& x, const hkVector4& y) {
        m_quad = _mm_sub_ps(m_quad, _mm_mul_ps(x.m_quad, y.m_quad));
    }

    // this = a + b * r, this = a + x * y (componentwise).
    void setAddMul4(const hkVector4& a, const hkVector4& b, const hkSimdReal& r) {
        m_quad = _mm_add_ps(a.m_quad, _mm_mul_ps(r.broadcast().m_quad, b.m_quad));
    }

    void setAddMul4(const hkVector4& a, const hkVector4& x, const hkVector4& y) {
        m_quad = _mm_add_ps(a.m_quad, _mm_mul_ps(x.m_quad, y.m_quad));
    }

    void setCross(const hkVector4& v0, const hkVector4& v1) {
        hkQuadReal cross0 = _mm_mul_ps(
            _mm_shuffle_ps(v0.m_quad, v0.m_quad, _MM_SHUFFLE(3, 0, 2, 1)),
            _mm_shuffle_ps(v1.m_quad, v1.m_quad, _MM_SHUFFLE(3, 1, 0, 2))
        );
        hkQuadReal cross1 = _mm_mul_ps(
            _mm_shuffle_ps(v0.m_quad, v0.m_quad, _MM_SHUFFLE(3, 1, 0, 2)),
            _mm_shuffle_ps(v1.m_quad, v1.m_quad, _MM_SHUFFLE(3, 0, 2, 1))
        );
        m_quad = _mm_sub_ps(cross0, cross1);
    }

    // Lane i of this (of v) in all four lanes; an i outside 0..3 leaves this unchanged.
    void broadcast(int32_t i) {
        if (i == 0) {
            m_quad = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(0, 0, 0, 0));
        }
        else if (i == 1) {
            m_quad = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(1, 1, 1, 1));
        }
        else if (i == 2) {
            m_quad = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(2, 2, 2, 2));
        }
        else if (i == 3) {
            m_quad = _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(3, 3, 3, 3));
        }
    }

    void setBroadcast(const hkVector4& v, int32_t i) {
        if (i == 0) {
            m_quad = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(0, 0, 0, 0));
        }
        else if (i == 1) {
            m_quad = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(1, 1, 1, 1));
        }
        else if (i == 2) {
            m_quad = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(2, 2, 2, 2));
        }
        else if (i == 3) {
            m_quad = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(3, 3, 3, 3));
        }
    }

    // this = (1 - t) * v0 + t * v1 over all four lanes.
    void setInterpolate4(const hkVector4& v0, const hkVector4& v1, const hkSimdReal& _t) {
        alignas(16) static const float cw[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        hkQuadReal t = _t.broadcast().m_quad;
        hkQuadReal s = _mm_sub_ps(*reinterpret_cast<const hkQuadReal*>(&cw), t);
        m_quad = _mm_add_ps(_mm_mul_ps(s, v0.m_quad), _mm_mul_ps(t, v1.m_quad));
    }

    // As equals3 over the four lanes.
    int32_t equals4(const hkVector4& v, float epsilon = 1e-3f) const {
        hkVector4 t;
        t.setSub4(*this, v);
        t.setAbs4(t);
        hkVector4 epsilon_v;
        epsilon_v.setAll(epsilon);
        return t.compareLessThanEqual4(epsilon_v).allAreSet(hkVector4Comparison::MASK_XYZW);
    }

    // The lanes where this is <= a.
    hkVector4Comparison compareLessThanEqual4(const hkVector4& a) const {
        hkVector4Comparison m;
        m.m_mask = _mm_cmple_ps(m_quad, a.m_quad);
        return m;
    }

    int32_t allLessThan3(const hkVector4& a) const {
        return (_mm_movemask_ps(_mm_cmplt_ps(m_quad, a.m_quad)) & hkVector4Comparison::MASK_XYZ) ==
            hkVector4Comparison::MASK_XYZ;
    }

    int32_t allLessThan4(const hkVector4& a) const {
        return _mm_movemask_ps(_mm_cmplt_ps(m_quad, a.m_quad)) == hkVector4Comparison::MASK_XYZW;
    }

    // v with the signs of x, y and z, or of all four lanes, flipped (xorps).
    void setNeg3(const hkVector4& v) {
        alignas(16) static const uint32_t negateMask[4] = {0x80000000, 0x80000000, 0x80000000, 0x00000000};
        m_quad = _mm_xor_ps(v.m_quad, *reinterpret_cast<const hkQuadReal*>(&negateMask));
    }

    void setNeg4(const hkVector4& x) {
        alignas(16) static const uint32_t negateMask[4] = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
        m_quad = _mm_xor_ps(x.m_quad, *reinterpret_cast<const hkQuadReal*>(&negateMask));
    }

    void setNeg4If(int32_t aiCondition);
    void setRotatedDir(const hkRotation& a, const hkVector4& b);
    void setTransformedInversePos(const hkTransform& a, const hkVector4& b);
    // The point b taken into the space of the quaternion-scale transform a.
    void setTransformedInversePos(const hkQsTransform& a, const hkVector4& b);

    // Horizontal add of the products: the products shuffled with _MM_SHUFFLE(1, 0, 3, 2) added to them, then that sum
    // plus its _MM_SHUFFLE(2, 3, 0, 1) shuffle.
    hkSimdReal dot4(const hkVector4& x) const {
        hkQuadReal x2 = _mm_mul_ps(m_quad, x.m_quad);
        hkQuadReal sum0 = _mm_add_ps(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 0, 3, 2)), x2);
        hkQuadReal sum1 = _mm_shuffle_ps(sum0, sum0, _MM_SHUFFLE(2, 3, 0, 1));
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_add_ps(sum0, sum1));
        return kResult;
    }

    // The dot product of xyz with a's plus this vector's w.
    hkSimdReal dot4xyz1(const hkVector4& x) const {
        hkQuadReal x2 = _mm_mul_ps(m_quad, x.m_quad);
        hkQuadReal xySum = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)), x2);
        hkQuadReal zwSum = _mm_add_ss(
            _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(3, 3, 3, 3))
        );
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_add_ps(xySum, zwSum));
        return kResult;
    }

    hkSimdReal horizontalAdd3() const {
        hkQuadReal xySum = _mm_add_ss(_mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(1, 1, 1, 1)), m_quad);
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(
            &kResult, _mm_add_ss(_mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(2, 2, 2, 2)), xySum)
        );
        return kResult;
    }

    hkSimdReal horizontalAdd4() const {
        hkQuadReal sum0 = _mm_add_ps(_mm_shuffle_ps(m_quad, m_quad, _MM_SHUFFLE(1, 0, 3, 2)), m_quad);
        hkQuadReal sum1 = _mm_shuffle_ps(sum0, sum0, _MM_SHUFFLE(2, 3, 0, 1));
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_add_ps(sum0, sum1));
        return kResult;
    }

    hkSimdReal lengthSquared3() const {
        return dot3(*this);
    }
    hkSimdReal lengthSquared4() const {
        return dot4(*this);
    }

    hkSimdReal length4() const {
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_sqrt_ss(dot4(*this).getQuad()));
        return kResult;
    }

    hkSimdReal distanceTo3(const hkVector4& p) const {
        hkVector4 d;
        d.setSub4(*this, p);
        return d.length3();
    }

    hkSimdReal distanceToSquared3(const hkVector4& p) const {
        hkVector4 d;
        d.setSub4(*this, p);
        return d.lengthSquared3();
    }

    // normalize3 and normalize4 that also return the length: len2 * (1 / length).
    hkSimdReal normalizeWithLength3() {
        hkQuadReal len2 = dot3(*this).getQuad();
        hkQuadReal lenInv = lengthInverse3().broadcast().m_quad;
        m_quad = _mm_mul_ps(lenInv, m_quad);
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_mul_ss(len2, lenInv));
        return kResult;
    }

    hkSimdReal normalizeWithLength4() {
        hkQuadReal len2 = dot4(*this).getQuad();
        hkQuadReal lenInv = lengthInverse4().broadcast().m_quad;
        m_quad = _mm_mul_ps(lenInv, m_quad);
        hkSimdReal kResult;
        XFNV_RECONSTRUCT(hkSimdReal)(&kResult, _mm_mul_ss(len2, lenInv));
        return kResult;
    }

    // Scales the vector by the rsqrtss estimate of 1 / |xyz|, unrefined.
    void fastNormalize3() {
        hkQuadReal len2 = dot3(*this).getQuad();
        hkSimdReal approx;
        XFNV_RECONSTRUCT(hkSimdReal)(&approx, _mm_rsqrt_ss(len2));
        m_quad = _mm_mul_ps(m_quad, approx.broadcast().m_quad);
    }

    // Scales the vector by hkMath::sqrtInverse of its squared length, 1.0f / sqrtf, unless that is zero. The real
    // converts to a simd real through its float constructor.
    hkResult normalize3IfNotZero() {
        const float lengthSqrd3 = lengthSquared3();
        if (lengthSqrd3 != 0.0f) {
            hkSimdReal kInverse;
            XFNV_RECONSTRUCT(hkSimdReal)(&kInverse, 1.0f / std::sqrtf(lengthSqrd3));
            mul4(kInverse);
            return HK_SUCCESS;
        }
        return HK_FAILURE;
    }

    // xyz's x, y, z with w's w; this vector's x, y, z with v's w; xyz's x, y, z with w cleared.
    void setXYZW(const hkVector4& xyz, const hkVector4& w) {
        m_quad = _mm_shuffle_ps(xyz.m_quad, _mm_unpackhi_ps(xyz.m_quad, w.m_quad), _MM_SHUFFLE(3, 0, 1, 0));
    }

    void setW(const hkVector4& w) {
        m_quad = _mm_shuffle_ps(m_quad, _mm_unpackhi_ps(m_quad, w.m_quad), _MM_SHUFFLE(3, 0, 1, 0));
    }

    void setXYZ0(const hkVector4& v) {
        alignas(16) static const uint32_t cw[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0x00000000};
        m_quad = _mm_and_ps(v.m_quad, *reinterpret_cast<const hkQuadReal*>(&cw));
    }

    // The SDK's inline (hkSseVector4.inl, hkMath::quadReciprocal): rcpps refined by one Newton step,
    // 2e - e * v * e.
    void setReciprocal4(const hkVector4& vec) {
        hkQuadReal e = _mm_rcp_ps(vec.m_quad);
        m_quad = _mm_sub_ps(_mm_add_ps(e, e), _mm_mul_ps(_mm_mul_ps(e, vec.m_quad), e));
    }

    // The SDK's inline (hkVector4.inl; the SSE build takes the generic one): the reciprocal of x, y, z, with w
    // taken as 1. Defined below, after hkQuadReal1111.
    void setReciprocal3(const hkVector4& v);

    // The index of the component of xyz with the largest magnitude.
    int32_t getMajorAxis() const {
        hkVector4 tmp;
        tmp.setAbs4(*this);
        return tmp(0) < tmp(1) ? (tmp(1) < tmp(2) ? 2 : 1) : (tmp(0) < tmp(2) ? 2 : 0);
    }

    // Clears component i (andps with a row of the mask table).
    void zeroElement(int32_t i) {
        m_quad = _mm_and_ps(m_quad, hkVector4_zeroElement_cx[i]);
    }
    hkBool isOk4() const;
    hkBool isNormalized3(float eps = 1e-4f) const;
    hkBool isNormalized4(float eps = 1e-4f) const;

    // Loads and stores of three or four floats; the a forms need p 16-byte aligned.
    void load3(const float* p) {
        hkVector4& v = *this;
        v(0) = p[0];
        v(1) = p[1];
        v(2) = p[2];
    }

    void load4(const float* p) {
        m_quad = _mm_loadu_ps(p);
    }
    void load4a(const float* p) {
        m_quad = _mm_load_ps(p);
    }

    void store3(float* p) const {
        const hkVector4& v = *this;
        p[0] = v(0);
        p[1] = v(1);
        p[2] = v(2);
    }

    void store4(float* p) const {
        const hkVector4& v = *this;
        p[0] = v(0);
        p[1] = v(1);
        p[2] = v(2);
        p[3] = v(3);
    }

    void store4a(float* p) const {
        _mm_store_ps(p, m_quad);
    }
    // The SDK's inline: a non-temporal store of the four lanes to 16-byte aligned memory.
    void storeUncached(void* dest) const {
        _mm_stream_ps(static_cast<float*>(dest), m_quad);
    }
    double getNextW() const;
    void sub_D53060(hkRotation* a1, hkRotation* arg4, hkVector4* a3);
    // Applies r's transpose to v. Defined in hkRotation.h.
    void _setRotatedInverseDir(const hkRotation& r, const hkVector4& v);
    bool* hasValidAxis(bool* a1);

    // Returns hkVector4Zero. Defined below, after hkVector4Zero.
    static const hkVector4& getZero();

    hkQuadReal m_quad;
};

// The lane test that isOk3 and isOk4 share, taking the lane count, the result and the vector. Returns the result
// pointer.
hkBool* isOkN(int32_t aiCount, hkBool* apResult, const hkVector4* apVector);
// The triple product of the three vectors.
float calcTripleProduct(const hkVector4* a, const hkVector4* c, const hkVector4* b);
// The point against the triangle's edges: clears weights, returns the
// first vertex with a positive weight or -1.
int32_t sub_E4B570(const hkVector4* apPoint, const hkVector4* apVertices, float* apWeights);
void calcSegmentSearchDirection(hkVector4* apSegment, const hkVector4* apPoint, int32_t aiSide, hkVector4* apOut);
// a1 is three points, a2 five floats, a3 the matrix it builds and returns, a4 the normal it writes.
hkMatrix4* calcTriangleBarycentricMatrix(const hkVector4* a1, const float* a2, hkMatrix4* a3, hkVector4* a4);
// A GSK simplex helper.
int32_t calcTriangleFaceNormalAndDistance(const hkVector4* vertices, const uint8_t* simplex, hkVector4* out);
// As calcTriangleFaceNormalAndDistance.
int32_t calcTriangleSimplexNormal(const hkVector4* vertices, hkVector4* out, const uint8_t* simplex);
hkMatrix4*
calcTriangleBarycentricPlanes(const hkVector4* vertices, const float* params, hkMatrix4* out, hkVector4* normalOut);
// Returns d * |d|, where d is the dot product of (*r_eax - *a1) and *a2.
float calcSignedSquaredDistance(const hkVector4* a1, const hkVector4* a2, const hkVector4* r_eax);
// Copies *a3 to *a4 and writes the dot product of *a3 and (*a1 - *a2) to r_eax's x (its other lanes a3 times the
// difference's z); returns r_eax.
hkVector4*
calcPlaneDistance(const hkVector4* a1, const hkVector4* a2, const hkVector4* a3, hkVector4* a4, hkVector4* r_eax);
// Sums into a6 the angular extents of the two motion states along normal a1 (each object radius times the length of
// r_eax's angular vector crossed with a1), and stores a1's dot product with a4 into a5; returns a6.
float* calcAngularExtents(
    const hkVector4* a1,
    const hkMotionState* a2,
    const hkMotionState* a3,
    const hkVector4* a4,
    float* a5,
    float* a6,
    const hkVector4* r_eax
);
// The separating velocity along a3 at time a5 of the point r_esi: the two angular vectors at a1 crossed with the
// point's offsets from the interpolated mass centres of motion states a2 and r_eax, less *a4.
float calcPointRelativeVelocity(
    const hkVector4* a1,
    const hkMotionState* a2,
    const hkVector4* a3,
    const hkVector4* a4,
    float a5,
    const hkMotionState* r_eax,
    const hkVector4* r_esi
);

// The real in all four lanes. Defined here, where hkVector4 is complete.
inline hkVector4 hkSimdReal::broadcast() const {
    hkVector4 kResult;
    kResult.m_quad = _mm_shuffle_ps(m_real, m_real, 0x00);
    return kResult;
}

using hkVector4f = hkVector4;

// hkVector4Comparison::s_maskFromBits: per Mask value, every bit of a lane set where the mask selects it. Its
// dynamic initializer fills it at startup. 16 entries, one per Mask value (inferred; s_invMaskFromBits
// follows it).
using hkVector4ComparisonMaskTable = hkQuadReal[16];
inline hkVector4ComparisonMaskTable hkVector4Comparison_s_maskFromBits = {};
extern hkVector4 hkVector4Zero;

// The lane mask a vector is xored with to negate it.
inline hkQuadReal hkQuadRealNegateMask{-0.0f, -0.0f, -0.0f, -0.0f};

// The SDK's four zero lanes (hkMath.h): its hkSimdReal operator- subtracts its operand from them
// (hkSseMathTypes.inl).
inline constexpr hkQuadReal hkQuadReal0000{0.0f, 0.0f, 0.0f, 0.0f};

// The SDK's four 1.0 lanes (hkMath.h).
inline hkQuadReal hkQuadReal1111{1.0f, 1.0f, 1.0f, 1.0f};

// The SDK's (1, 0, 0, 0) (hkMath.h).
inline constexpr hkQuadReal hkQuadReal1000{1.0f, 0.0f, 0.0f, 0.0f};

// The SDK's (0, 1, 0, 0) (hkMath.h).
inline constexpr hkQuadReal hkQuadReal0100{0.0f, 1.0f, 0.0f, 0.0f};

// The SDK's (0, 0, 1, 0) (hkMath.h).
inline constexpr hkQuadReal hkQuadReal0010{0.0f, 0.0f, 1.0f, 0.0f};

// The SDK's (0, 0, 0, 1) (hkMath.h), the identity quaternion's quad.
inline constexpr hkQuadReal hkQuadReal0001{0.0f, 0.0f, 0.0f, 1.0f};

// The SDK's four 3.0 lanes and four 0.5 lanes (hkMath.h), the SSE reciprocal square root's Newton step.
inline constexpr hkQuadReal hkQuadReal3333{3.0f, 3.0f, 3.0f, 3.0f};
inline constexpr hkQuadReal hkQuadRealHalf{0.5f, 0.5f, 0.5f, 0.5f};

// The SDK's generic inline (hkVector4.inl): x, y, z with w set to 1, then setReciprocal4.
inline void hkVector4::setReciprocal3(const hkVector4& v) {
    hkVector4 v1111;
    v1111.m_quad = hkQuadReal1111;
    hkVector4 tmp;
    tmp.setXYZW(v, v1111);
    setReciprocal4(tmp);
}

inline const hkVector4& hkVector4::getZero() {
    return hkVector4Zero;
}
