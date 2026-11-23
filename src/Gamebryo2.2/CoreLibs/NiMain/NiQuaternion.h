#pragma once

#include <cmath>

class NiMatrix3;
class NiPoint3;
class NiStream;

// Rotation quaternion, stored w first.
class NiQuaternion {
public:
    // Returns this.
    NiQuaternion& SetValues(float w, float x, float y, float z);
    void SetX(float x);
    // m_fY = fY; HK2NI_QUATERNION calls it.
    void SetY(float y);
    // m_fZ = fZ; HK2NI_QUATERNION calls it.
    void SetZ(float z);
    // m_fW = fW; HK2NI_QUATERNION calls it.
    void SetW(float w);

    NiQuaternion operator+(const NiQuaternion& q) const;
    NiQuaternion operator-(const NiQuaternion& kQ) const;
    NiQuaternion operator*(const NiQuaternion& kQ) const;
    // Scales to unit length; a zero quaternion becomes NaN.
    void Normalize();
    // Componentwise negation.
    NiQuaternion operator-() const;
    // Every component times fScalar.
    NiQuaternion operator*(float fScalar) const;
    // Normalize with an approximate inverse square root.
    void FastNormalize();
    // Componentwise; a NaN component never compares equal.
    bool operator==(const NiQuaternion& q) const;
    // True when any component differs (a NaN component always differs).
    bool operator!=(const NiQuaternion& q) const;

    // Zeroes every component within 1e-8 of zero.
    void Snap();

    void LoadBinary(NiStream& stream);
    // Snaps this first, so saving can change the quaternion.
    void SaveBinary(NiStream& stream);
    // "<prefix>: (w=..,x=..,y=..,z=..)", a fresh _NiMalloc string.
    char* GetViewerString(const char* pcPrefix) const;

    void FromRotation(const NiMatrix3& rot);
    void ToRotation(NiMatrix3& rot) const;
    // The rotation angle (radians) and unit axis.
    void ToAngleAxis(float& angle, NiPoint3& axis) const;
    // Sets this from half-angle sines and cosines of the three angles; the X and Z angles are negated.
    void FromEulerAnglesXYZ(float afXAngle, float afYAngle, float afZAngle);
    // Sets this to the spherical interpolation from p (t = 0) to q (t = 1). A member writing this, not Gamebryo's
    // static by-value form.
    void Slerp(float t, const NiQuaternion& p, const NiQuaternion& q);
    void Squad(float t, const NiQuaternion& p, const NiQuaternion& a, const NiQuaternion& b, const NiQuaternion& q);
    void FromAngleAxisX(float fAngle);

    // Conjugate of a unit quaternion.
    static NiQuaternion UnitInverse(const NiQuaternion& p);
    // The logarithm of a unit quaternion: w = 0, the axis scaled by the angle.
    static NiQuaternion Log(const NiQuaternion& q);
    // The exponential of a pure quaternion: w = cos |v|, v scaled by sin |v| / |v|.
    static NiQuaternion Exp(const NiQuaternion& q);
    static float CounterWarp(float t, float fCos);
    static NiQuaternion*
    Intermediate(NiQuaternion* apResult, const NiQuaternion& q0, const NiQuaternion& q1, const NiQuaternion& q2);

    float m_fW;
    float m_fX;
    float m_fY;
    float m_fZ;

    static NiQuaternion IDENTITY;
};

// Every component of kQ times fScalar.
NiQuaternion operator*(float fScalar, const NiQuaternion& kQ);

// Guessed bound: the database gives none; the next data item starts 0xc bytes on.
using FromRotation_next_t = int32_t[3];

// The identity quaternion (w = 1).
inline NiQuaternion NiQuaternion::IDENTITY{1.0f};

// Constants for ISqrt_approx_in_neighborhood: the approximation of 1/sqrt(x) near ISQRT_NEIGHBORHOOD.
// ISQRT_ADDITIVE_CONSTANT and ISQRT_FACTOR are statics computed at startup in every file that includes the header: one
// copy per file.
const float ISQRT_NEIGHBORHOOD = 0.959066f;
const float ISQRT_SCALE = 1.000311f;
static const float ISQRT_ADDITIVE_CONSTANT = ISQRT_SCALE / std::sqrt(ISQRT_NEIGHBORHOOD);
static const float ISQRT_FACTOR = static_cast<float>(
    ISQRT_SCALE * (-0.5 / (static_cast<double>(std::sqrt(ISQRT_NEIGHBORHOOD)) * ISQRT_NEIGHBORHOOD))
);
