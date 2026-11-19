#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

// Returns the quaternion, because NiRotKey::Interpolate reads the return value.
NiQuaternion& NiQuaternion::SetValues(float w, float x, float y, float z) {
    m_fW = w;
    m_fX = x;
    m_fY = y;
    m_fZ = z;
    return *this;
}

void NiQuaternion::SetY(float y) {
    m_fY = y;
}

void NiQuaternion::SetZ(float z) {
    m_fZ = z;
}

void NiQuaternion::SetW(float w) {
    m_fW = w;
}

NiQuaternion NiQuaternion::operator+(const NiQuaternion& q) const {
    NiQuaternion kSum;
    kSum.m_fW = q.m_fW + m_fW;
    kSum.m_fX = q.m_fX + m_fX;
    kSum.m_fY = q.m_fY + m_fY;
    kSum.m_fZ = q.m_fZ + m_fZ;
    return kSum;
}

NiQuaternion NiQuaternion::operator-(const NiQuaternion& kQ) const {
    NiQuaternion kDiff;
    kDiff.m_fW = m_fW - kQ.m_fW;
    kDiff.m_fX = m_fX - kQ.m_fX;
    kDiff.m_fY = m_fY - kQ.m_fY;
    kDiff.m_fZ = m_fZ - kQ.m_fZ;
    return kDiff;
}

// The Hamilton product: rotating by the result rotates by kQ first, then by this. Computes z, y, x, then w, each
// rounded to float, with the operand order below.
NiQuaternion NiQuaternion::operator*(const NiQuaternion& kQ) const {
    float fZ = kQ.m_fW * m_fZ + m_fW * kQ.m_fZ + kQ.m_fY * m_fX - kQ.m_fX * m_fY;
    float fY = kQ.m_fY * m_fW + m_fY * kQ.m_fW + kQ.m_fX * m_fZ - m_fX * kQ.m_fZ;
    float fX = m_fX * kQ.m_fW + kQ.m_fX * m_fW + m_fY * kQ.m_fZ - kQ.m_fY * m_fZ;
    float fW = kQ.m_fW * m_fW - kQ.m_fX * m_fX - m_fY * kQ.m_fY - kQ.m_fZ * m_fZ;
    NiQuaternion kProduct;
    kProduct.SetValues(fW, fX, fY, fZ);
    return kProduct;
}

void NiQuaternion::Normalize() {
    // The squared length and the root are each rounded to float before use.
    float fSqrLength = m_fW * m_fW + m_fX * m_fX + m_fY * m_fY + m_fZ * m_fZ;
    float fLength = std::sqrt(fSqrLength);
    float fInvLength = 1.0f / fLength;
    m_fW *= fInvLength;
    m_fX *= fInvLength;
    m_fY *= fInvLength;
    m_fZ *= fInvLength;
}

NiQuaternion NiQuaternion::operator-() const {
    NiQuaternion a2;
    a2.m_fW = -m_fW;
    a2.m_fX = -m_fX;
    a2.m_fY = -m_fY;
    a2.m_fZ = -m_fZ;
    return a2;
}

void NiQuaternion::FastNormalize() {
    double v5;
    double v6;
    double v7;
    float v10;
    float v11;
    float v12;
    float v13;
    float v8 = (((m_fY * m_fY) + (m_fX * m_fX)) + (m_fZ * m_fZ)) + (m_fW * m_fW);
    double v1 = (double)v8;
    double v2 = (double)ISQRT_ADDITIVE_CONSTANT;
    float v9 = (float)(((v8 - 0.9590659737586975) * ISQRT_FACTOR) + ISQRT_ADDITIVE_CONSTANT);
    double v3 = v1;
    double v4 = (double)ISQRT_FACTOR;
    if (v3 <= 0.9152119755744934) {
        v5 = (double)v9;
        v10 = (float)((v9 * v9) * v3);
        v11 = (float)(((v10 - 0.9590659737586975) * v4) + v2);
        v9 = (float)(v5 * v11);
        if (v3 <= 0.6521196961402893f) {
            v6 = v3 * (v9 * v9);
            v7 = (double)v9;
            v12 = (float)v6;
            v13 = (float)(v2 + (v4 * (v12 - 0.9590659737586975)));
            v9 = (float)(v7 * v13);
        }
    }
    m_fX = (m_fX * v9);
    m_fY = (v9 * m_fY);
    m_fZ = (m_fZ * v9);
    m_fW = (v9 * m_fW);
}

bool NiQuaternion::operator==(const NiQuaternion& q) const {
    return m_fW == q.m_fW && m_fX == q.m_fX && m_fY == q.m_fY && m_fZ == q.m_fZ;
}

// A component already zero (either sign) is left untouched.
void NiQuaternion::Snap() {
    const float fEpsilon = 1e-08f;
    if (fabsf(m_fX) <= fEpsilon && m_fX != 0.0f) {
        m_fX = 0.0f;
    }
    if (fabsf(m_fY) <= fEpsilon && m_fY != 0.0f) {
        m_fY = 0.0f;
    }
    if (fabsf(m_fZ) <= fEpsilon && m_fZ != 0.0f) {
        m_fZ = 0.0f;
    }
    if (fabsf(m_fW) <= fEpsilon && m_fW != 0.0f) {
        m_fW = 0.0f;
    }
}

void NiQuaternion::LoadBinary(NiStream& stream) {
    NiStreamLoadBinary(stream, m_fW);
    NiStreamLoadBinary(stream, m_fX);
    NiStreamLoadBinary(stream, m_fY);
    NiStreamLoadBinary(stream, m_fZ);
}

void NiQuaternion::SaveBinary(NiStream& stream) {
    Snap();
    NiStreamSaveBinary(stream, m_fW);
    NiStreamSaveBinary(stream, m_fX);
    NiStreamSaveBinary(stream, m_fY);
    NiStreamSaveBinary(stream, m_fZ);
}

char* NiQuaternion::GetViewerString(const char* pcPrefix) const {
    size_t stLength = strlen(pcPrefix) + 128;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString, stLength, "%s: (w=%g,x=%g,y=%g,z=%g)", pcPrefix, static_cast<double>(m_fW), static_cast<double>(m_fX),
        static_cast<double>(m_fY), static_cast<double>(m_fZ)
    );
    return pcString;
}

void NiQuaternion::FromRotation(const NiMatrix3& rot) {
    static FromRotation_next_t next{1, 2};
    float rota = rot.m_pEntry[0][0] + rot.m_pEntry[1][1] + rot.m_pEntry[2][2];
    if (!(rota > 0.0f)) {
        float* quat[3];
        int32_t v3 = rot.m_pEntry[0][0] < static_cast<double>(rot.m_pEntry[1][1]);
        // The diagonal entry [i][i]; indexing row 0 past its end would be undefined.
        if (rot.m_pEntry[v3][v3] < static_cast<double>(rot.m_pEntry[2][2])) {
            v3 = 2;
        }
        int32_t v4 = next[v3];
        int32_t v5 = next[v4];
        float rotd = rot.m_pEntry[v3][v3] - rot.m_pEntry[v4][v4] - rot.m_pEntry[v5][v5] + 1.0;
        float rote = std::sqrt((double)rotd);
        double v6 = rote;
        quat[0] = &m_fX;
        quat[1] = &m_fY;
        quat[2] = &m_fZ;
        float rotf = rote * 0.5;
        *quat[v3] = rotf;
        float rotg = 0.5 / v6;
        double v7 = rotg;
        m_fW = (rot.m_pEntry[v5][v4] - rot.m_pEntry[v4][v5]) * rotg;
        float roth = (rot.m_pEntry[v3][v4] + rot.m_pEntry[v4][v3]) * rotg;
        *quat[v4] = roth;
        float roti = v7 * (rot.m_pEntry[v3][v5] + rot.m_pEntry[v5][v3]);
        *quat[v5] = roti;
    }
    else {
        float rotb = rota + 1.0;
        float fRoot = std::sqrt((double)rotb);
        m_fW = fRoot * 0.5;
        float rotc = 0.5 / fRoot;
        m_fX = (rot.m_pEntry[2][1] - rot.m_pEntry[1][2]) * rotc;
        m_fY = (rot.m_pEntry[0][2] - rot.m_pEntry[2][0]) * rotc;
        m_fZ = rotc * (rot.m_pEntry[1][0] - rot.m_pEntry[0][1]);
    }
}

void NiQuaternion::ToRotation(NiMatrix3& rot) const {
    float v11 = (float)(m_fX * 2.0);
    float v20 = (float)(m_fY * 2.0);
    float v15 = (float)(m_fZ * 2.0);
    float v21 = v11 * m_fW;
    float v12 = v20 * m_fW;
    float v14 = v15 * m_fW;
    float v16 = v11 * m_fX;
    float v17 = v20 * m_fX;
    float v19 = v15 * m_fX;
    float v18 = v20 * m_fY;
    float v13 = v15 * m_fY;
    float v22 = v15 * m_fZ;
    float v10 = v19 - v12;
    float v9 = v17 + v14;
    float v8 = (float)(1.0 - (v18 + v22));
    (&rot)->SetCol((uint32_t)0, v8, v9, v10);
    float v7 = v13 + v21;
    float v6 = (float)(1.0 - (v16 + v22));
    float v5 = v17 - v14;
    (&rot)->SetCol(1u, v5, v6, v7);
    float v4 = (float)(1.0 - (v16 + v18));
    float v3 = v13 - v21;
    float v2 = v19 + v12;
    (&rot)->SetCol(2u, v2, v3, v4);
}

// Takes the square root of the float squared length. A NaN length or w takes the acos branch's NaN path, as the game
// does (w <= -1 and NaN give pi).
void NiQuaternion::ToAngleAxis(float& angle, NiPoint3& axis) const {
    float fSqrLength = m_fY * m_fY + m_fX * m_fX + m_fZ * m_fZ;
    float fLength = std::sqrt(fSqrLength);
    if (fLength < 0.001f) {
        angle = 0.0f;
        axis.x = 0.0f;
        axis.y = 0.0f;
        axis.z = 0.0f;
        return;
    }
    float fHalfAngle = NiACos(m_fW);
    angle = fHalfAngle + fHalfAngle;
    float fInvLength = 1.0f / fLength;
    axis.x = m_fX * fInvLength;
    axis.y = fInvLength * m_fY;
    axis.z = fInvLength * m_fZ;
}

// Negates the X and Z half angles, and rounds each half angle, cosine, sine and the two products cZ*cY and sZ*sY to
// float.
void NiQuaternion::FromEulerAnglesXYZ(float afXAngle, float afYAngle, float afZAngle) {
    float fHalfX = -afXAngle * 0.5;
    float fCosX = std::cos(fHalfX);
    float fHalfY = afYAngle * 0.5;
    float fCosY = std::cos(fHalfY);
    float fHalfZ = -afZAngle * 0.5;
    float fCosZ = std::cos(fHalfZ);
    float fSinX = std::sin(fHalfX);
    float fSinY = std::sin(fHalfY);
    float fSinZ = std::sin(fHalfZ);
    float fCosZCosY = fCosZ * fCosY;
    float fSinZSinY = fSinZ * fSinY;
    m_fW = fCosZCosY * fCosX + fSinZSinY * fSinX;
    m_fX = fCosZCosY * fSinX - fSinZSinY * fCosX;
    m_fY = fSinX * fCosY * fSinZ + fSinY * fCosX * fCosZ;
    m_fZ = fCosY * fCosX * fSinZ - fSinY * fSinX * fCosZ;
}

void NiQuaternion::Slerp(float t, const NiQuaternion& p, const NiQuaternion& q) {
    double v7;
    float a3b;
    float a3a = ((((&p)->m_fW * (&q)->m_fW) + ((&p)->m_fX * (&q)->m_fX)) + ((&p)->m_fY * (&q)->m_fY)) +
        ((&p)->m_fZ * (&q)->m_fZ);
    float v8 = a3a;
    if (t > 0x1p-1f) {
        a3b = (float)(1.0 - t);
        v7 = 1.0 - NiQuaternion::CounterWarp(a3b, v8);
    }
    else {
        v7 = (double)NiQuaternion::CounterWarp(t, a3a);
    }
    float a4a = (float)v7;
    m_fW = ((&p)->m_fW + (((&q)->m_fW - (&p)->m_fW) * a4a));
    m_fX = ((((&q)->m_fX - (&p)->m_fX) * a4a) + (&p)->m_fX);
    m_fY = ((((&q)->m_fY - (&p)->m_fY) * a4a) + (&p)->m_fY);
    m_fZ = ((a4a * ((&q)->m_fZ - (&p)->m_fZ)) + (&p)->m_fZ);
    FastNormalize();
}

void NiQuaternion::Squad(
    float t,
    const NiQuaternion& p,
    const NiQuaternion& a,
    const NiQuaternion& b,
    const NiQuaternion& q
) {
    NiQuaternion v7;
    NiQuaternion v8;
    (&v8)->Slerp(t, p, q);
    (&v7)->Slerp(t, a, b);
    float v9 = (float)((1.0 - t) * (t + t));
    Slerp(v9, v8, v7);
}

NiQuaternion NiQuaternion::UnitInverse(const NiQuaternion& p) {
    NiQuaternion kInverse;
    kInverse.m_fW = p.m_fW;
    kInverse.m_fX = -p.m_fX;
    kInverse.m_fY = -p.m_fY;
    kInverse.m_fZ = -p.m_fZ;
    return kInverse;
}

// w <= -1 (or NaN) gives pi, w >= 1 gives 0. A NaN |sin| still divides, as the game does.
NiQuaternion NiQuaternion::Log(const NiQuaternion& q) {
    float fAngle = NiACos(q.m_fW);
    float fSin = std::sin(fAngle);
    float fAbsSin = std::fabsf(fSin);
    float fCoeff = fAbsSin < 0.001f ? 1.0f : fAngle / fSin;
    NiQuaternion kLog;
    kLog.m_fW = 0.0f;
    kLog.m_fX = q.m_fX * fCoeff;
    kLog.m_fY = q.m_fY * fCoeff;
    kLog.m_fZ = fCoeff * q.m_fZ;
    return kLog;
}

float NiQuaternion::CounterWarp(float t, float fCos) {
    float fCosa = (float)(1.0 - (fCos * 0.8227968811988831));
    float fCosb = fCosa * fCosa;
    float fCosc = (float)(fCosb * 0.5854921936988831);
    return (float)(t * (fCosc + (((fCosc * t) * ((t + t) - 3.0)) + 1.0)));
}

NiQuaternion* NiQuaternion::Intermediate(
    NiQuaternion* apResult,
    const NiQuaternion& q0,
    const NiQuaternion& q1,
    const NiQuaternion& q2
) {
    NiQuaternion inv;
    NiQuaternion q;
    NiQuaternion v13;
    NiQuaternion v15;
    inv.m_fW = q1.m_fW;
    inv.m_fX = -q1.m_fX;
    inv.m_fY = -q1.m_fY;
    inv.m_fZ = -q1.m_fZ;
    NiQuaternion* v5 = ::new (&v13) NiQuaternion(NiQuaternion::Log(inv * q2));
    NiQuaternion* v7 = ::new (&v15) NiQuaternion(NiQuaternion::Log(inv * q0));
    inv.m_fW = v5->m_fW + v7->m_fW;
    inv.m_fX = v5->m_fX + v7->m_fX;
    inv.m_fY = v5->m_fY + v7->m_fY;
    inv.m_fZ = v5->m_fZ + v7->m_fZ;
    q.m_fW = inv.m_fW * -0.25;
    q.m_fX = inv.m_fX * -0.25;
    q.m_fY = inv.m_fY * -0.25;
    q.m_fZ = -0.25 * inv.m_fZ;
    NiQuaternion exp = NiQuaternion::Exp(q);
    ::new (apResult) NiQuaternion(q1 * exp);
    return apResult;
}
