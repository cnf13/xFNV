#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <xmmintrin.h>

class hkQsTransform;

// A rotation and the translation it places the rotated object at.
class hkTransform {
public:
    hkTransform();
    hkTransform(const hkTransform& arTransform);
    hkTransform(const hkQuaternion& arRotation, const hkVector4& arTranslation);

    // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
    HK_DECLARE_POD_TYPE();

    void setTranslation(const hkVector4& t) {
        m_translation = t;
    }

    // Whether the translation and the rotation are valid.
    hkBool isOk() const;
    // Writes the transform that first applies arB, then arA on top of it.
    void setMul(const hkTransform& aTb, const hkTransform& bTc);
    // The SDK's inline form of setMul: bTc's four columns turned by aTb's rotation (_setMul3), from the translation
    // down to column 0, then aTb's translation added. This must be neither aTb nor bTc. Defined below.
    void _setMul(const hkTransform& aTb, const hkTransform& bTc);
    // Writes the transform that first applies the inverse of arA's rotation and translation,
    // then arB.
    void setMulInverseMul(const hkTransform& bTa, const hkTransform& bTc);
    // Copy assignment: copies the four columns and returns this. Not declared as operator= because a user-declared copy
    // assignment would change every copy of an hkTransform.
    hkTransform& Assign(const hkTransform& arTransform);

    // The rest of the SDK's hkTransform members.
    // The translation and the rotation.
    hkVector4& getTranslation() {
        return m_translation;
    }
    const hkVector4& getTranslation() const {
        return m_translation;
    }
    hkRotation& getRotation() {
        return m_rotation;
    }
    const hkRotation& getRotation() const {
        return m_rotation;
    }
    // The column aiIndex of the rotation; column 3 is the translation.
    hkVector4& getColumn(int32_t aiIndex) {
        return (&m_rotation.m_col0)[aiIndex];
    }
    const hkVector4& getColumn(int32_t aiIndex) const {
        return m_rotation.getColumn(aiIndex);
    }
    void setRotation(const hkRotation& arRotation) {
        m_rotation = arRotation;
    }
    void setRotation(const hkQuaternion& arRotation) {
        m_rotation.set(arRotation);
    }

    void set(const hkQuaternion& q, const hkVector4& t) {
        m_rotation.set(q);
        m_translation = t;
    }

    void setIdentity() {
        m_rotation.setIdentity();
        m_translation.setZero4();
    }

    // The SDK's inline: hkTransformIdentity. Defined below.
    static const hkTransform& getIdentity();

    void setInverse(const hkTransform& t);
    void setMulEq(const hkTransform& b);
    void setMulMulInverse(const hkTransform& wTa, const hkTransform& wTb);

    void setRows4(const hkVector4& r0, const hkVector4& r1, const hkVector4& r2, const hkVector4& r3) {
        getColumn(0).set(r0(0), r1(0), r2(0), r3(0));
        getColumn(1).set(r0(1), r1(1), r2(1), r3(1));
        getColumn(2).set(r0(2), r1(2), r2(2), r3(2));
        getColumn(3).set(r0(3), r1(3), r2(3), r3(3));
    }

    // Declared only; these have no definition.
    void get4x4ColumnMajor(float* d) const;
    void set4x4ColumnMajor(const float* p);
    hkBool isApproximatelyEqual(const hkTransform& t, float zero = 1e-3f) const;
    __m128* get4x4ColumnMajorQuads(__m128* p);
    // Sets this from the 4x4 matrix at p, which it only reads, and
    // returns p.
    const __m128* set4x4ColumnMajorQuads(const __m128* p);
    void setMul_0(const hkQsTransform& aTb, const hkTransform& bTc);

    hkRotation m_rotation;
    hkVector4 m_translation;
};

// The identity transform hkMatrix3::getIdentity returns: rotation rows of one, zero translation.
extern hkTransform hkTransformIdentity;
// The identity transform's sixteen floats, which hkTransformIdentity's initialiser copies.
using hkTransformIdentity_storage_t = float[16];
inline hkTransformIdentity_storage_t hkTransformIdentity_storage{
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f
};

inline const hkTransform& hkTransform::getIdentity() {
    return hkTransformIdentity;
}

// b's x, y and z, each broadcast, scale t's columns: (y * col1 + x * col0) + (z * col2 + translation).
inline void hkVector4::_setTransformedPos(const hkTransform& t, const hkVector4& b) {
    const hkRotation& r = t.getRotation();
    const hkQuadReal b1 = _mm_shuffle_ps(b.m_quad, b.m_quad, _MM_SHUFFLE(1, 1, 1, 1));
    const hkQuadReal r1 = _mm_mul_ps(b1, r.getColumn(1).m_quad);
    const hkQuadReal b0 = _mm_shuffle_ps(b.m_quad, b.m_quad, _MM_SHUFFLE(0, 0, 0, 0));
    const hkQuadReal r0 = _mm_mul_ps(b0, r.getColumn(0).m_quad);
    const hkQuadReal b2 = _mm_shuffle_ps(b.m_quad, b.m_quad, _MM_SHUFFLE(2, 2, 2, 2));
    const hkQuadReal r2 = _mm_mul_ps(b2, r.getColumn(2).m_quad);
    m_quad = _mm_add_ps(_mm_add_ps(r1, r0), _mm_add_ps(r2, t.getTranslation().m_quad));
}

// The SDK's hkVector4Util::rotatePoints of bTc's four columns: aTb's rotation is copied first, then each column from
// the translation down is _setMul3 of it.
inline void hkTransform::_setMul(const hkTransform& aTb, const hkTransform& bTc) {
    hkMatrix3 unaliased;
    unaliased = aTb.getRotation();
    int32_t i = 3;
    do {
        getColumn(i)._setMul3(unaliased, bTc.getColumn(i));
    }
    while (--i >= 0);
    m_translation.add4(aTb.m_translation);
}
