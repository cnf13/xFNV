#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>

class hkRotation;

// A rotation as a four component vector: x, y, z axis scaled by half the angle, w the cosine.
class hkQuaternion {
public:
    // Stores nothing.
    hkQuaternion() {}
    hkQuaternion(const hkRotation& r);
    // The quaternion whose x, y, z and w are v's.
    explicit hkQuaternion(const hkVector4& v) : m_vec(v) {}

    // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
    HK_DECLARE_POD_TYPE();
    // Sets the identity rotation (0, 0, 0, 1).
    void setIdentity();
    // Scales the vector to unit length.
    void normalize();

    // Sets this to the quaternion of a rotation matrix.
    void set(const hkRotation& r);
    // The normalized quaternion of a rotation matrix: calls quaternionFromRotatation and then normalize.
    void setAndNormalize(const hkRotation& r);

    // this = q0 * q1.
    void setMul(const hkQuaternion& q0, const hkQuaternion& q1);
    // this = this * q.
    void mul(const hkQuaternion& q);
    // Sets this to q0 * the inverse of q1.
    void setMulInverse(const hkQuaternion& q0, const hkQuaternion& q1);
    // The unit rotation axis.
    void getAxis(hkVector4& axisOut) const;
    // The shortest rotation that takes the unit vector `from` to the unit vector `to`: the identity when they are
    // nearly parallel, setFlippedRotation(from) when they nearly oppose.
    void setShortestRotation(const hkVector4& from, const hkVector4& to);
    // The shortest rotation from the unit vector `from` to the unit vector `to`, scaled down by gain.
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

    const float& operator()(int32_t i) const {
        return m_vec(i);
    }
    // this = the inverse of q0 * q1.
    void setInverseMul(const hkQuaternion& q0, const hkQuaternion& q1);
    // The SDK's inline: the conjugate of q (its imaginary part negated), defined below.
    void setInverse(const hkQuaternion& q);
    // Whether the quaternion is valid: its components are valid and its length is 1 within 0.001. The hkBool is
    // returned through apResult. Const, as callers use it on a const pose's rotation.
    hkBool isOk() const;
    // An estimate of the rotation from this to `to` as an angle vector, 2 * (imag x to.imag - to.w * imag + w *
    // to.imag), negated when the 4-lane dot product is negative.
    void estimateAngleTo(const hkQuaternion& to, hkVector4& angleOut) const;
    void setFlippedRotation(const hkVector4& from);

    static const hkQuaternion& getIdentity();

    hkVector4 m_vec;
};

// The rotation is const: it is only read, and hkQuaternion::set and setAndNormalize pass their const hkRotation&.
void quaternionFromRotatation(const hkRotation* rotation, hkQuaternion* quatOut);

using hkQuaternionf = hkQuaternion;

extern hkQuaternion hkQuaternionIdentity;

// The four -0.5 lanes the quaternion rotation adds to w * w.
inline constexpr hkQuadReal hkQuadRealMinusHalf{-0.5f, -0.5f, -0.5f, -0.5f};

inline constexpr hkQuadReal hkQuadRealMax{HK_REAL_MAX, HK_REAL_MAX, HK_REAL_MAX, HK_REAL_MAX};

// hkQuadRealEps: HK_REAL_EPSILON in four lanes. hkpBoxMotion::getInertiaWorld and
// hkpSimpleConstraintInfo::BodyInfo::getEnergy clamp inverse masses up to it.
inline constexpr hkQuadReal hkQuadRealEps{HK_REAL_EPSILON, HK_REAL_EPSILON, HK_REAL_EPSILON, HK_REAL_EPSILON};

// SSE lane masks: hkQuadRealNegateXYZMask holds the sign bits xor'ed to conjugate a quaternion (setInverse),
// hkQuadRealXYZMask the xyz mask hkVector4::setXYZ0 and's in (setInverse's scale, after setReciprocal3). The mask that
// negates all four lanes (setNeg4) is hkQuadRealNegateMask (hkVector4.h).
inline hkQuadReal hkQuadRealNegateXYZMask{-0.0f, -0.0f, -0.0f};
inline hkQuadReal hkQuadRealXYZMask{
    std::bit_cast<float>(0xffffffffu),
    std::bit_cast<float>(0xffffffffu),
    std::bit_cast<float>(0xffffffffu)
};

// The SDK's nearlyOneQuad (1 - 1e-5 in four lanes) and somewhatNearlyNegOneQuad (-(1 - 1e-3) in four lanes), the
// constants setShortestRotation compares against.
inline hkQuadReal nearlyOneQuad{0.99999f, 0.99999f, 0.99999f, 0.99999f};
inline hkQuadReal somewhatNearlyNegOneQuad{-0.999f, -0.999f, -0.999f, -0.999f};

// Packed over all four lanes, w included. The inverse rotation crosses dir with imag instead of imag with dir.
inline void hkVector4::setRotatedDir(const hkQuaternion& quat, const hkVector4& direction) {
    hkVector4 minusHalf;
    minusHalf.m_quad = hkQuadRealMinusHalf;
    hkVector4 qreal;
    qreal.setBroadcast(quat.m_vec, 3);
    hkVector4 q2minus1;
    q2minus1.setAddMul4(minusHalf, qreal, qreal);
    hkVector4 ret;
    ret.setMul4(direction, q2minus1);
    ret.addMul4(quat.m_vec.dot3(direction), quat.m_vec);
    hkVector4 imagCrossDir;
    imagCrossDir.setCross(quat.m_vec, direction);
    ret.addMul4(qreal, imagCrossDir);
    setAdd4(ret, ret);
}

inline void hkVector4::setRotatedInverseDir(const hkQuaternion& quat, const hkVector4& direction) {
    hkVector4 minusHalf;
    minusHalf.m_quad = hkQuadRealMinusHalf;
    hkVector4 qreal;
    qreal.setBroadcast(quat.m_vec, 3);
    hkVector4 q2minus1;
    q2minus1.setAddMul4(minusHalf, qreal, qreal);
    hkVector4 ret;
    ret.setMul4(direction, q2minus1);
    ret.addMul4(quat.m_vec.dot3(direction), quat.m_vec);
    hkVector4 dirCrossImag;
    dirCrossImag.setCross(direction, quat.m_vec);
    ret.addMul4(qreal, dirCrossImag);
    setAdd4(ret, ret);
}

// The SDK's SSE form of hkQuaternion::setInverse: the sign bits of x, y and z flipped with the mask above.
inline void hkQuaternion::setInverse(const hkQuaternion& q) {
    m_vec.m_quad = _mm_xor_ps(q.m_vec.m_quad, hkQuadRealNegateXYZMask);
}

inline void hkQuaternion::setIdentity() {
    m_vec.m_quad = hkQuadReal0001;
}

inline void hkQuaternion::normalize() {
    m_vec.normalize4();
}

inline const hkQuaternion& hkQuaternion::getIdentity() {
    return hkQuaternionIdentity;
}

// The imaginary part is the cross product of the imaginary parts plus each one scaled by the other's w; w is q0.w *
// q1.w less the 3-lane dot product (summed z + (y + x)).
inline void hkQuaternion::setMul(const hkQuaternion& q0, const hkQuaternion& q1) {
    __m128 v3 = _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 255);
    __m128 v4 = _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 255);
    __m128 v5 = _mm_mul_ps(q1.m_vec.m_quad, q0.m_vec.m_quad);
    __m128 v6 = _mm_shuffle_ps(v5, v5, 170);
    XFNV_M128_F32(v6, 0) = XFNV_M128_F32(v6, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    __m128 v7 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0.m_vec.m_quad),
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), q1.m_vec.m_quad),
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 210),
                    _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 201),
                    _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 210)
                )
            )
        )
    );
    m_vec.m_quad = _mm_shuffle_ps(v7, _mm_unpackhi_ps(v7, _mm_sub_ps(v5, _mm_shuffle_ps(v6, v6, 0))), 196);
}

// SSE form of setMul(*this, q).
inline void hkQuaternion::mul(const hkQuaternion& q) {
    __m128 v2 = _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 255);
    __m128 v3 = _mm_shuffle_ps(q.m_vec.m_quad, q.m_vec.m_quad, 255);
    __m128 v4 = _mm_mul_ps(m_vec.m_quad, q.m_vec.m_quad);
    __m128 v5 = _mm_shuffle_ps(v4, v4, 170);
    XFNV_M128_F32(v5, 0) = XFNV_M128_F32(v5, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v4, v4, 85), 0) + XFNV_M128_F32(v4, 0));
    __m128 v6 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), m_vec.m_quad),
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v2, v2, 0), q.m_vec.m_quad),
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 201),
                    _mm_shuffle_ps(q.m_vec.m_quad, q.m_vec.m_quad, 210)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 210),
                    _mm_shuffle_ps(q.m_vec.m_quad, q.m_vec.m_quad, 201)
                )
            )
        )
    );
    m_vec.m_quad = _mm_shuffle_ps(v6, _mm_unpackhi_ps(v6, _mm_sub_ps(v4, _mm_shuffle_ps(v5, v5, 0))), 196);
}

// The SDK's SSE form: xyz = q1.imag x q0.imag - q0.w * q1.imag + q1.w * q0.imag, w the 4-lane dot product.
inline void hkQuaternion::setMulInverse(const hkQuaternion& q0, const hkQuaternion& q1) {
    __m128 v3 = _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 255);
    __m128 v4 = _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 255);
    __m128 v5 = _mm_mul_ps(q1.m_vec.m_quad, q0.m_vec.m_quad);
    __m128 v6 = _mm_add_ps(_mm_shuffle_ps(v5, v5, 78), v5);
    __m128 v7 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 177), v6);
    __m128 v8 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0.m_vec.m_quad),
        _mm_sub_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 201),
                    _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 210)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 210),
                    _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 201)
                )
            ),
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), q1.m_vec.m_quad)
        )
    );
    m_vec.m_quad = _mm_shuffle_ps(v8, _mm_unpackhi_ps(v8, _mm_shuffle_ps(v7, v7, 0)), 196);
}

// The SDK's SSE form: xyz = q1.imag x q0.imag + q0.w * q1.imag - q1.w * q0.imag, w the 4-lane dot product.
inline void hkQuaternion::setInverseMul(const hkQuaternion& q0, const hkQuaternion& q1) {
    __m128 v3 = _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 255);
    __m128 v4 = _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 255);
    __m128 v5 = _mm_mul_ps(q1.m_vec.m_quad, q0.m_vec.m_quad);
    __m128 v6 = _mm_add_ps(_mm_shuffle_ps(v5, v5, 78), v5);
    __m128 v7 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 177), v6);
    __m128 v8 = _mm_sub_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), q1.m_vec.m_quad),
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 201),
                    _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 210)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(q1.m_vec.m_quad, q1.m_vec.m_quad, 210),
                    _mm_shuffle_ps(q0.m_vec.m_quad, q0.m_vec.m_quad, 201)
                )
            )
        ),
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), q0.m_vec.m_quad)
    );
    m_vec.m_quad = _mm_shuffle_ps(v8, _mm_unpackhi_ps(v8, _mm_shuffle_ps(v7, v7, 0)), 196);
}

// The vector normalized over xyz (reciprocal square root with one Newton step 0.5 * r * (3 - x * r * r), zero for a
// zero length), negated when w is negative.
inline void hkQuaternion::getAxis(hkVector4& axisOut) const {
    __m128 v2 = _mm_mul_ps(m_vec.m_quad, m_vec.m_quad);
    float v3 = XFNV_M128_F32(_mm_shuffle_ps(v2, v2, 85), 0) + XFNV_M128_F32(v2, 0);
    __m128 v4 = _mm_shuffle_ps(v2, v2, 170);
    XFNV_M128_F32(v4, 0) = XFNV_M128_F32(v4, 0) + v3;
    float v5 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v4, 0))));
    __m128 v7 = _mm_cmpeq_ss(v4, _mm_setzero_ps());
    float fNewton = 3.0f - ((XFNV_M128_F32(v4, 0) * v5) * v5);
    __m128 v8 = _mm_setzero_ps();
    XFNV_M128_F32(v8, 0) = (0.5f * v5) * fNewton;
    __m128 v9 = _mm_andnot_ps(v7, v8);
    __m128 v10 = _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), m_vec.m_quad);
    if ((_mm_movemask_ps(_mm_cmplt_ps(m_vec.m_quad, _mm_setzero_ps())) & 8) != 0) {
        axisOut.m_quad = _mm_xor_ps(hkQuadRealNegateMask, v10);
    }
    else {
        axisOut.m_quad = v10;
    }
}

inline void hkQuaternion::estimateAngleTo(const hkQuaternion& to, hkVector4& angleOut) const {
    __m128 v3 = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(to.m_vec.m_quad, to.m_vec.m_quad, 210),
            _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 201)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(to.m_vec.m_quad, to.m_vec.m_quad, 201),
            _mm_shuffle_ps(m_vec.m_quad, m_vec.m_quad, 210)
        )
    );
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

inline void hkQuaternion::setShortestRotation(const hkVector4& from, const hkVector4& to) {
    __m128 v3 = _mm_mul_ps(to.m_quad, from.m_quad);
    float v4 = XFNV_M128_F32(_mm_shuffle_ps(v3, v3, 85), 0) + XFNV_M128_F32(v3, 0);
    __m128 v5 = _mm_shuffle_ps(v3, v3, 170);
    XFNV_M128_F32(v5, 0) = XFNV_M128_F32(v5, 0) + v4;
    __m128 v6 = _mm_shuffle_ps(v5, v5, 0);
    __m128 v7 = _mm_cmplt_ps(v6, _mm_xor_ps(hkQuadRealNegateMask, nearlyOneQuad));
    if (_mm_movemask_ps(_mm_cmplt_ps(nearlyOneQuad, v6))) {
        m_vec.m_quad = hkQuadReal0001;
    }
    else if (_mm_movemask_ps(v7)) {
        setFlippedRotation(from);
    }
    else {
        __m128 v8 = _mm_add_ps(_mm_mul_ps(hkQuadRealHalf, v6), hkQuadRealHalf);
        __m128 v9 = _mm_rsqrt_ps(v8);
        __m128 v10 = _mm_mul_ps(
            _mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v9, v8), v9)),
            _mm_mul_ps(v9, hkQuadRealHalf)
        );
        __m128 v11 = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 210), _mm_shuffle_ps(from.m_quad, from.m_quad, 201)),
            _mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 201), _mm_shuffle_ps(from.m_quad, from.m_quad, 210))
        );
        __m128 v12 = _mm_mul_ps(v10, hkQuadRealHalf);
        if (_mm_movemask_ps(_mm_cmplt_ps(v6, somewhatNearlyNegOneQuad))) {
            __m128 v13 = _mm_mul_ps(v11, v11);
            float v14 = XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0);
            __m128 v15 = _mm_shuffle_ps(v13, v13, 170);
            XFNV_M128_F32(v15, 0) = XFNV_M128_F32(v15, 0) + v14;
            __m128 v16 = _mm_sub_ps(v8, v6);
            float v25 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v15, 0))));
            __m128 v18 = _mm_cmpeq_ss(v15, _mm_setzero_ps());
            float v19 = 3.0f - ((XFNV_M128_F32(v15, 0) * v25) * v25);
            __m128 v20 = _mm_setzero_ps();
            XFNV_M128_F32(v20, 0) = (0.5f * v25) * v19;
            __m128 v21 = _mm_andnot_ps(v18, v20);
            __m128 v22 = _mm_rsqrt_ps(v16);
            __m128 v23 = _mm_mul_ps(
                _mm_mul_ps(
                    _mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(v22, v16), v22)),
                    _mm_mul_ps(v22, hkQuadRealHalf)
                ),
                v16
            );
            v12 = _mm_mul_ps(v23, _mm_shuffle_ps(v21, v21, 0));
        }
        __m128 v24 = _mm_mul_ps(v12, v11);
        m_vec.m_quad = _mm_shuffle_ps(v24, _mm_unpackhi_ps(v24, _mm_mul_ps(v8, v10)), 196);
    }
}

inline void hkQuaternion::setShortestRotationDamped(float gain, const hkVector4& from, const hkVector4& to) {
    __m128 v4 = _mm_mul_ps(to.m_quad, from.m_quad);
    float v5 = XFNV_M128_F32(_mm_shuffle_ps(v4, v4, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v4, v4, 85), 0) + XFNV_M128_F32(v4, 0));
    float v15 = 1.0 - gain + v5 * gain;
    float v16 = (v15 + 1.0) * 0.5;
    if (v16 <= 0.0f || v5 < -0.9999989867210388) {
        setFlippedRotation(from);
    }
    else if (!(v5 > 0.9999989867210388)) {
        float v17 = std::sqrt(static_cast<double>(v16));
        double v6 = v17;
        float fRescaleSin = gain * 0.5 / v17;
        __m128 v19 = _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 210), _mm_shuffle_ps(from.m_quad, from.m_quad, 201)),
                _mm_mul_ps(_mm_shuffle_ps(to.m_quad, to.m_quad, 201), _mm_shuffle_ps(from.m_quad, from.m_quad, 210))
            ),
            _mm_shuffle_ps(_mm_set_ss(fRescaleSin), _mm_set_ss(fRescaleSin), 0)
        );
        XFNV_M128_F32(v19, 3) = v6;
        __m128 v7 = _mm_mul_ps(v19, v19);
        __m128 v8 = _mm_add_ps(_mm_shuffle_ps(v7, v7, 78), v7);
        __m128 v9 = _mm_add_ps(_mm_shuffle_ps(v8, v8, 177), v8);
        float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v9, 0))));
        __m128 v12 = _mm_cmpeq_ss(v9, _mm_setzero_ps());
        float fNewton = 3.0f - ((XFNV_M128_F32(v9, 0) * v10) * v10);
        __m128 v13 = _mm_setzero_ps();
        XFNV_M128_F32(v13, 0) = (0.5f * v10) * fNewton;
        __m128 v14 = _mm_andnot_ps(v12, v13);
        m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), v19);
    }
    else {
        m_vec.m_quad = hkQuadReal0001;
    }
}
