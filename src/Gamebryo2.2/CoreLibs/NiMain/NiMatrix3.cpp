#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

NiMatrix3::NiMatrix3(const NiPoint3& col0, const NiPoint3& col1, const NiPoint3& col2) {
    SetCol(0, col0);
    SetCol(1, col1);
    SetCol(2, col2);
}

void NiMatrix3::FromEulerAnglesZXY(float fZAngle, float fXAngle, float fYAngle) {
    NiMatrix3 kXRot;
    NiMatrix3 kYRot;
    NiMatrix3 kZRot;
    kXRot.MakeXRotation(fXAngle);
    kYRot.MakeYRotation(fYAngle);
    kZRot.MakeZRotation(fZAngle);
    *this = kZRot * (kXRot * kYRot);
}

void NiMatrix3::FromEulerAnglesZYX(float fZAngle, float fYAngle, float fXAngle) {
    NiMatrix3 kXRot;
    NiMatrix3 kYRot;
    NiMatrix3 kZRot;
    kXRot.MakeXRotation(fXAngle);
    kYRot.MakeYRotation(fYAngle);
    kZRot.MakeZRotation(fZAngle);
    *this = kZRot * (kYRot * kXRot);
}

void NiMatrix3::GetRow(uint32_t uiRow, NiPoint3& row) const {
    row.x = m_pEntry[uiRow][0];
    row.y = m_pEntry[uiRow][1];
    row.z = m_pEntry[uiRow][2];
}

void NiMatrix3::GetRow(uint32_t uiRow, float& f0, float& f1, float& f2) const {
    f0 = m_pEntry[uiRow][0];
    f1 = m_pEntry[uiRow][1];
    f2 = m_pEntry[uiRow][2];
}

void NiMatrix3::SetRow(uint32_t uiRow, const NiPoint3& row) {
    m_pEntry[uiRow][0] = row.x;
    m_pEntry[uiRow][1] = row.y;
    m_pEntry[uiRow][2] = row.z;
}

void NiMatrix3::SetRow(uint32_t uiRow, float f0, float f1, float f2) {
    m_pEntry[uiRow][0] = f0;
    m_pEntry[uiRow][1] = f1;
    m_pEntry[uiRow][2] = f2;
}

void NiMatrix3::SetRow(uint32_t uiRow, const float* row) {
    m_pEntry[uiRow][0] = row[0];
    m_pEntry[uiRow][1] = row[1];
    m_pEntry[uiRow][2] = row[2];
}

void NiMatrix3::GetCol(uint32_t uiCol, NiPoint3& col) const {
    col.x = m_pEntry[0][uiCol];
    col.y = m_pEntry[1][uiCol];
    col.z = m_pEntry[2][uiCol];
}

void NiMatrix3::SetCol(uint32_t uiCol, const NiPoint3& col) {
    m_pEntry[0][uiCol] = col.x;
    m_pEntry[1][uiCol] = col.y;
    m_pEntry[2][uiCol] = col.z;
}

void NiMatrix3::SetCol(uint32_t uiCol, float f0, float f1, float f2) {
    m_pEntry[0][uiCol] = f0;
    m_pEntry[1][uiCol] = f1;
    m_pEntry[2][uiCol] = f2;
}

NiMatrix3 NiMatrix3::Transpose() const {
    NiPoint3 row[3];
    GetRow(0, row[0]);
    GetRow(1, row[1]);
    GetRow(2, row[2]);
    return NiMatrix3(row[0], row[1], row[2]);
}

NiMatrix3 NiMatrix3::operator*(const NiMatrix3& mat) const {
    NiMatrix3 result;
    NiMatrix3 prd;
    prd.m_pEntry[0][0] = m_pEntry[0][0] * mat.m_pEntry[0][0] + m_pEntry[0][1] * mat.m_pEntry[1][0] +
        m_pEntry[0][2] * mat.m_pEntry[2][0];
    prd.m_pEntry[1][0] = m_pEntry[1][0] * mat.m_pEntry[0][0] + m_pEntry[1][1] * mat.m_pEntry[1][0] +
        m_pEntry[1][2] * mat.m_pEntry[2][0];
    prd.m_pEntry[2][0] = m_pEntry[2][0] * mat.m_pEntry[0][0] + m_pEntry[2][1] * mat.m_pEntry[1][0] +
        m_pEntry[2][2] * mat.m_pEntry[2][0];
    prd.m_pEntry[0][1] = m_pEntry[0][0] * mat.m_pEntry[0][1] + m_pEntry[0][1] * mat.m_pEntry[1][1] +
        m_pEntry[0][2] * mat.m_pEntry[2][1];
    prd.m_pEntry[1][1] = m_pEntry[1][0] * mat.m_pEntry[0][1] + m_pEntry[1][1] * mat.m_pEntry[1][1] +
        m_pEntry[1][2] * mat.m_pEntry[2][1];
    prd.m_pEntry[2][1] = m_pEntry[2][0] * mat.m_pEntry[0][1] + m_pEntry[2][1] * mat.m_pEntry[1][1] +
        m_pEntry[2][2] * mat.m_pEntry[2][1];
    prd.m_pEntry[0][2] = m_pEntry[0][0] * mat.m_pEntry[0][2] + m_pEntry[0][1] * mat.m_pEntry[1][2] +
        m_pEntry[0][2] * mat.m_pEntry[2][2];
    prd.m_pEntry[1][2] = m_pEntry[1][0] * mat.m_pEntry[0][2] + m_pEntry[1][1] * mat.m_pEntry[1][2] +
        m_pEntry[1][2] * mat.m_pEntry[2][2];
    prd.m_pEntry[2][2] = m_pEntry[2][0] * mat.m_pEntry[0][2] + m_pEntry[2][1] * mat.m_pEntry[1][2] +
        m_pEntry[2][2] * mat.m_pEntry[2][2];
    result = prd;
    return result;
}

// Each row's sum of products is computed in double and rounded to float once (x87).
NiPoint3 NiMatrix3::operator*(const NiPoint3& pt) const {
    NiPoint3 result;
    float z = static_cast<float>(
        static_cast<double>(m_pEntry[2][0]) * pt.x + static_cast<double>(m_pEntry[2][1]) * pt.y +
        static_cast<double>(m_pEntry[2][2]) * pt.z
    );
    float y = static_cast<float>(
        static_cast<double>(m_pEntry[1][0]) * pt.x + static_cast<double>(m_pEntry[1][1]) * pt.y +
        static_cast<double>(m_pEntry[1][2]) * pt.z
    );
    float x = static_cast<float>(
        static_cast<double>(m_pEntry[0][0]) * pt.x + static_cast<double>(m_pEntry[0][1]) * pt.y +
        static_cast<double>(m_pEntry[0][2]) * pt.z
    );
    ::new (&result) NiPoint3(x, y, z);
    return result;
}

NiMatrix3 NiMatrix3::operator*(float fScalar) const {
    NiMatrix3 result;
    result.m_pEntry[0][0] = m_pEntry[0][0] * fScalar;
    result.m_pEntry[0][1] = m_pEntry[0][1] * fScalar;
    result.m_pEntry[0][2] = m_pEntry[0][2] * fScalar;
    result.m_pEntry[1][0] = m_pEntry[1][0] * fScalar;
    result.m_pEntry[1][1] = m_pEntry[1][1] * fScalar;
    result.m_pEntry[1][2] = m_pEntry[1][2] * fScalar;
    result.m_pEntry[2][0] = m_pEntry[2][0] * fScalar;
    result.m_pEntry[2][1] = m_pEntry[2][1] * fScalar;
    result.m_pEntry[2][2] = fScalar * m_pEntry[2][2];
    return result;
}

NiMatrix3 NiMatrix3::operator+(const NiMatrix3& mat) const {
    NiMatrix3 result;
    result = *this;
    result.m_pEntry[0][0] = result.m_pEntry[0][0] + mat.m_pEntry[0][0];
    result.m_pEntry[0][1] = mat.m_pEntry[0][1] + result.m_pEntry[0][1];
    result.m_pEntry[0][2] = mat.m_pEntry[0][2] + result.m_pEntry[0][2];
    result.m_pEntry[1][0] = mat.m_pEntry[1][0] + result.m_pEntry[1][0];
    result.m_pEntry[1][1] = mat.m_pEntry[1][1] + result.m_pEntry[1][1];
    result.m_pEntry[1][2] = mat.m_pEntry[1][2] + result.m_pEntry[1][2];
    result.m_pEntry[2][0] = mat.m_pEntry[2][0] + result.m_pEntry[2][0];
    result.m_pEntry[2][1] = mat.m_pEntry[2][1] + result.m_pEntry[2][1];
    result.m_pEntry[2][2] = mat.m_pEntry[2][2] + result.m_pEntry[2][2];
    return result;
}

NiMatrix3 NiMatrix3::operator-(const NiMatrix3& mat) const {
    NiMatrix3 result;
    result = *this;
    result.m_pEntry[0][0] = result.m_pEntry[0][0] - mat.m_pEntry[0][0];
    result.m_pEntry[0][1] = result.m_pEntry[0][1] - mat.m_pEntry[0][1];
    result.m_pEntry[0][2] = result.m_pEntry[0][2] - mat.m_pEntry[0][2];
    result.m_pEntry[1][0] = result.m_pEntry[1][0] - mat.m_pEntry[1][0];
    result.m_pEntry[1][1] = result.m_pEntry[1][1] - mat.m_pEntry[1][1];
    result.m_pEntry[1][2] = result.m_pEntry[1][2] - mat.m_pEntry[1][2];
    result.m_pEntry[2][0] = result.m_pEntry[2][0] - mat.m_pEntry[2][0];
    result.m_pEntry[2][1] = result.m_pEntry[2][1] - mat.m_pEntry[2][1];
    result.m_pEntry[2][2] = result.m_pEntry[2][2] - mat.m_pEntry[2][2];
    return result;
}

NiMatrix3 NiMatrix3::TransposeTimes(const NiMatrix3& mat) const {
    NiMatrix3 result;
    result.m_pEntry[0][0] = mat.m_pEntry[0][0] * m_pEntry[0][0] + mat.m_pEntry[1][0] * m_pEntry[1][0] +
        mat.m_pEntry[2][0] * m_pEntry[2][0];
    result.m_pEntry[1][0] = m_pEntry[1][1] * mat.m_pEntry[1][0] + m_pEntry[0][1] * mat.m_pEntry[0][0] +
        mat.m_pEntry[2][0] * m_pEntry[2][1];
    result.m_pEntry[2][0] = m_pEntry[1][2] * mat.m_pEntry[1][0] + m_pEntry[0][2] * mat.m_pEntry[0][0] +
        mat.m_pEntry[2][0] * m_pEntry[2][2];
    result.m_pEntry[0][1] = mat.m_pEntry[0][1] * m_pEntry[0][0] + mat.m_pEntry[1][1] * m_pEntry[1][0] +
        m_pEntry[2][0] * mat.m_pEntry[2][1];
    result.m_pEntry[1][1] = mat.m_pEntry[1][1] * m_pEntry[1][1] + mat.m_pEntry[0][1] * m_pEntry[0][1] +
        mat.m_pEntry[2][1] * m_pEntry[2][1];
    result.m_pEntry[2][1] = mat.m_pEntry[1][1] * m_pEntry[1][2] + mat.m_pEntry[0][1] * m_pEntry[0][2] +
        mat.m_pEntry[2][1] * m_pEntry[2][2];
    result.m_pEntry[0][2] = mat.m_pEntry[0][2] * m_pEntry[0][0] + mat.m_pEntry[1][2] * m_pEntry[1][0] +
        m_pEntry[2][0] * mat.m_pEntry[2][2];
    result.m_pEntry[1][2] = mat.m_pEntry[1][2] * m_pEntry[1][1] + mat.m_pEntry[0][2] * m_pEntry[0][1] +
        mat.m_pEntry[2][2] * m_pEntry[2][1];
    result.m_pEntry[2][2] = mat.m_pEntry[1][2] * m_pEntry[1][2] + mat.m_pEntry[0][2] * m_pEntry[0][2] +
        mat.m_pEntry[2][2] * m_pEntry[2][2];
    return result;
}

bool NiMatrix3::Inverse(NiMatrix3& inv) const {
    int32_t col;
    int32_t row;
    float fInvdet;
    inv.m_pEntry[0][0] = ((m_pEntry[1][1] * m_pEntry[2][2]) - (m_pEntry[1][2] * m_pEntry[2][1]));
    inv.m_pEntry[0][1] = ((m_pEntry[0][2] * m_pEntry[2][1]) - (m_pEntry[0][1] * m_pEntry[2][2]));
    inv.m_pEntry[0][2] = ((m_pEntry[0][1] * m_pEntry[1][2]) - (m_pEntry[0][2] * m_pEntry[1][1]));
    inv.m_pEntry[1][0] = ((m_pEntry[1][2] * m_pEntry[2][0]) - (m_pEntry[1][0] * m_pEntry[2][2]));
    inv.m_pEntry[1][1] = ((m_pEntry[0][0] * m_pEntry[2][2]) - (m_pEntry[0][2] * m_pEntry[2][0]));
    inv.m_pEntry[1][2] = ((m_pEntry[0][2] * m_pEntry[1][0]) - (m_pEntry[0][0] * m_pEntry[1][2]));
    inv.m_pEntry[2][0] = ((m_pEntry[1][0] * m_pEntry[2][1]) - (m_pEntry[1][1] * m_pEntry[2][0]));
    inv.m_pEntry[2][1] = ((m_pEntry[0][1] * m_pEntry[2][0]) - (m_pEntry[0][0] * m_pEntry[2][1]));
    inv.m_pEntry[2][2] = ((m_pEntry[0][0] * m_pEntry[1][1]) - (m_pEntry[0][1] * m_pEntry[1][0]));
    float fDet = ((m_pEntry[0][0] * inv.m_pEntry[0][0]) + (m_pEntry[0][1] * inv.m_pEntry[1][0])) +
        (m_pEntry[0][2] * inv.m_pEntry[2][0]);
    if (NiAbs(fDet) <= 0x1.0c6f7a0000000p-20) {
        return false;
    }
    for (row = 0; row < 3; ++row) {
        for (col = 0; col < 3; ++col) {
            fInvdet = static_cast<float>(1.0 / fDet);
            inv.m_pEntry[row][col] = (inv.m_pEntry[row][col] * fInvdet);
        }
    }
    return true;
}

bool NiMatrix3::ToEulerAnglesXYZ(float& rfXAngle, float& rfYAngle, float& rfZAngle) const {
    double v6;
    float v10;
    double v5 = m_pEntry[0][2];
    if (!(v5 > -1.0)) {
        v6 = NI_HALF_PI;
        v10 = -NI_HALF_PI;
    }
    else {
        if (!(v5 < 1.0)) {
            v6 = NI_HALF_PI;
            v10 = NI_HALF_PI;
        }
        else {
            v10 = std::asin(v5);
            v6 = NI_HALF_PI;
        }
    }
    float v11 = -v10;
    rfYAngle = v11;
    if (!(v11 < v6)) {
        float rfYAngleb = NiFastATan2(m_pEntry[1][0], m_pEntry[1][1]);
        rfZAngle = 0.0;
        rfXAngle = 0.0 - rfYAngleb;
        return false;
    }

    if (!(-v6 < v11)) {
        float rfYAnglea = NiFastATan2(m_pEntry[1][0], m_pEntry[1][1]);
        rfZAngle = 0.0;
        rfXAngle = rfYAnglea - 0.0;
        return false;
    }

    float fY = -m_pEntry[1][2];
    rfXAngle = -NiFastATan2(fY, m_pEntry[2][2]);
    float fYa = -m_pEntry[0][1];
    rfZAngle = -NiFastATan2(fYa, m_pEntry[0][0]);
    return true;
}

bool NiMatrix3::ToEulerAnglesZXY(float& rfZAngle, float& rfXAngle, float& rfYAngle) const {
    double v6;
    float v10;
    double v5 = m_pEntry[2][1];
    if (!(v5 > -1.0)) {
        v6 = NI_HALF_PI;
        v10 = -NI_HALF_PI;
    }
    else {
        if (!(v5 < 1.0)) {
            v6 = NI_HALF_PI;
            v10 = NI_HALF_PI;
        }
        else {
            v10 = std::asin(v5);
            v6 = NI_HALF_PI;
        }
    }
    float v11 = -v10;
    rfXAngle = v11;
    if (!(v11 < v6)) {
        float xa = NiFastATan2(m_pEntry[0][2], m_pEntry[0][0]);
        rfYAngle = 0.0;
        rfZAngle = 0.0 - xa;
        return false;
    }

    if (!(-v6 < v11)) {
        float x = NiFastATan2(m_pEntry[0][2], m_pEntry[0][0]);
        rfYAngle = 0.0;
        rfZAngle = x - 0.0;
        return false;
    }

    float fY = -m_pEntry[0][1];
    rfZAngle = -NiFastATan2(fY, m_pEntry[1][1]);
    float fYa = -m_pEntry[2][0];
    rfYAngle = -NiFastATan2(fYa, m_pEntry[2][2]);
    return true;
}

void NiMatrix3::FromEulerAnglesXYZ(float fXAngle, float fYAngle, float fZAngle) {
    NiMatrix3 kXRot;
    NiMatrix3 kYRot;
    NiMatrix3 kZRot;
    kXRot.MakeXRotation(fXAngle);
    kYRot.MakeYRotation(fYAngle);
    kZRot.MakeZRotation(fZAngle);
    *this = kXRot * (kYRot * kZRot);
}

void NiMatrix3::MakeXRotation(float fAngle) {
    float sine;
    float cosine;
    NiSinCos(fAngle, sine, cosine);

    m_pEntry[0][0] = 1.0f;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = cosine;
    m_pEntry[1][2] = sine;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = -sine;
    m_pEntry[2][2] = cosine;
}

void NiMatrix3::MakeYRotation(float fAngle) {
    float sine;
    float cosine;
    NiSinCos(fAngle, sine, cosine);

    m_pEntry[0][0] = cosine;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = -sine;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = 1.0f;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = sine;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = cosine;
}

void NiMatrix3::MakeZRotation(float fAngle) {
    float sine;
    float cosine;
    NiSinCos(fAngle, sine, cosine);

    m_pEntry[0][0] = cosine;
    m_pEntry[0][1] = sine;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = -sine;
    m_pEntry[1][1] = cosine;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = 1.0f;
}

void NiMatrix3::MakeRotation(float fAngle, float x, float y, float z) {
    float cosOut;
    float sinOut;
    NiSinCos(fAngle, sinOut, cosOut);
    float omcs = static_cast<float>(1.0 - cosOut);
    float x2 = x * x;
    float y2 = y * y;
    float z2 = z * z;
    float xym = (x * y) * omcs;
    float xzm = (x * z) * omcs;
    float yzm = (y * z) * omcs;
    float xsin = x * sinOut;
    float ysin = y * sinOut;
    float zsin = z * sinOut;
    m_pEntry[0][0] = ((x2 * omcs) + cosOut);
    m_pEntry[0][1] = (xym + zsin);
    m_pEntry[0][2] = (xzm - ysin);
    m_pEntry[1][0] = (xym - zsin);
    m_pEntry[1][1] = ((y2 * omcs) + cosOut);
    m_pEntry[1][2] = (yzm + xsin);
    m_pEntry[2][0] = (xzm + ysin);
    m_pEntry[2][1] = (yzm - xsin);
    m_pEntry[2][2] = ((z2 * omcs) + cosOut);
}

// Each sum of products is computed in double and rounded to float once. The reference source's last
// length test is left out because its result is unused.
void NiMatrix3::ExtractAngleAndAxis(float& fAngle, float& fX, float& fY, float& fZ) const {
    float fTrace = static_cast<float>(static_cast<double>(m_pEntry[0][0]) + m_pEntry[1][1] + m_pEntry[2][2]);
    float fCos = static_cast<float>((static_cast<double>(fTrace) - 1.0) * 0.5);
    fAngle = NiACos(fCos);
    fX = m_pEntry[1][2] - m_pEntry[2][1];
    fY = m_pEntry[2][0] - m_pEntry[0][2];
    float fZDiff = m_pEntry[0][1] - m_pEntry[1][0];
    fZ = fZDiff;
    float fSqrLength = static_cast<float>(
        static_cast<double>(fY) * fY + static_cast<double>(fX) * fX + static_cast<double>(fZDiff) * fZDiff
    );
    float fLength = NiSqrt(fSqrLength);
    if (fLength > 1e-06f) {
        float fInvLength = static_cast<float>(1.0 / static_cast<double>(fLength));
        fX = fX * fInvLength;
        fY = fY * fInvLength;
        fZ = fInvLength * fZ;
        return;
    }
    if (!(fAngle > 1.0f)) {
        fX = 1.0f;
        fY = 0.0f;
        fZ = 0.0f;
        return;
    }
    fX = NiSqrt(static_cast<float>((static_cast<double>(m_pEntry[0][0]) + 1.0) * 0.5));
    fY = NiSqrt(static_cast<float>((static_cast<double>(m_pEntry[1][1]) + 1.0) * 0.5));
    float fZRoot = NiSqrt(static_cast<float>((static_cast<double>(m_pEntry[2][2]) + 1.0) * 0.5));
    fZ = fZRoot;
    float tx = static_cast<float>(
        static_cast<double>(fX) * m_pEntry[0][0] + static_cast<double>(m_pEntry[0][1]) * fY +
        static_cast<double>(m_pEntry[0][2]) * fZRoot - fX
    );
    float ty = static_cast<float>(
        static_cast<double>(m_pEntry[1][0]) * fX + static_cast<double>(fY) * m_pEntry[1][1] +
        static_cast<double>(fZRoot) * m_pEntry[1][2] - fY
    );
    float tz = static_cast<float>(
        static_cast<double>(m_pEntry[2][0]) * fX + static_cast<double>(fY) * m_pEntry[2][1] +
        static_cast<double>(fZRoot) * m_pEntry[2][2] - fZRoot
    );
    float fSqrDiff = static_cast<float>(
        static_cast<double>(ty) * ty + static_cast<double>(tx) * tx + static_cast<double>(tz) * tz
    );
    if (1e-06f > fSqrDiff) {
        return;
    }
    float fNegZ = -fZRoot;
    fZ = fNegZ;
    tx = static_cast<float>(
        static_cast<double>(fX) * m_pEntry[0][0] + static_cast<double>(m_pEntry[0][1]) * fY +
        static_cast<double>(fNegZ) * m_pEntry[0][2] - fX
    );
    ty = static_cast<float>(
        static_cast<double>(m_pEntry[1][0]) * fX + static_cast<double>(fY) * m_pEntry[1][1] +
        static_cast<double>(fNegZ) * m_pEntry[1][2] - fY
    );
    tz = static_cast<float>(
        static_cast<double>(m_pEntry[2][0]) * fX + static_cast<double>(fY) * m_pEntry[2][1] +
        static_cast<double>(fNegZ) * m_pEntry[2][2] - fNegZ
    );
    fSqrDiff = static_cast<float>(
        static_cast<double>(ty) * ty + static_cast<double>(tx) * tx + static_cast<double>(tz) * tz
    );
    if (1e-06f > fSqrDiff) {
        return;
    }
    // The Gamebryo reference source tests the residual once more; this code deliberately skips that test.
    fY = -fY;
}

bool NiMatrix3::operator==(const NiMatrix3& mat) const {
    for (int32_t iRow = 0; iRow < 3; iRow++) {
        for (int32_t iCol = 0; iCol < 3; iCol++) {
            if (m_pEntry[iRow][iCol] != mat.m_pEntry[iRow][iCol]) {
                return false;
            }
        }
    }
    return true;
}

void NiMatrix3::LoadBinary(NiStream& stream) {
    for (auto& afRow : m_pEntry) {
        for (float& fEntry : afRow) {
            NiStreamLoadBinary(stream, fEntry);
        }
    }
}

void NiMatrix3::SaveBinary(NiStream& stream) {
    Snap();
    for (auto& afRow : m_pEntry) {
        for (float& fEntry : afRow) {
            NiStreamSaveBinary(stream, fEntry);
        }
    }
}

char* NiMatrix3::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 128;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString,
        stLength,
        "%s = ((%g,%g,%g),(%g,%g,%g),(%g,%g,%g))",
        pPrefix,
        static_cast<double>(m_pEntry[0][0]),
        static_cast<double>(m_pEntry[0][1]),
        static_cast<double>(m_pEntry[0][2]),
        static_cast<double>(m_pEntry[1][0]),
        static_cast<double>(m_pEntry[1][1]),
        static_cast<double>(m_pEntry[1][2]),
        static_cast<double>(m_pEntry[2][0]),
        static_cast<double>(m_pEntry[2][1]),
        static_cast<double>(m_pEntry[2][2])
    );
    return pcString;
}

void NiMatrix3::MakeZero() {
    SetCol(0, 0.0f, 0.0f, 0.0f);
    SetCol(1, 0.0f, 0.0f, 0.0f);
    SetCol(2, 0.0f, 0.0f, 0.0f);
}

bool NiMatrix3::operator!=(const NiMatrix3& mat) const {
    return !(*this == mat);
}

float NiMatrix3::GetEntry(uint32_t uiRow, uint32_t uiCol) const {
    return m_pEntry[uiRow][uiCol];
}

void NiMatrix3::MakeIdentity() {
    m_pEntry[0][0] = 1.0f;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = 1.0f;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = 1.0f;
}

void NiMatrix3::SetEntry(uint32_t uiRow, uint32_t uiCol, float fEntry) {
    m_pEntry[uiRow][uiCol] = fEntry;
}

// Each entry whose magnitude is at most 1e-8 (NaN excluded) and which is not already zero becomes zero.
void NiMatrix3::Snap() {
    for (auto& afRow : m_pEntry) {
        for (float& fEntry : afRow) {
            float fAbs = fabsf(fEntry);
            if (fAbs <= 1e-08f && fEntry != 0.0f) {
                fEntry = 0.0f;
            }
        }
    }
}

char NiMatrix3::Tridiagonal(float* afDiag, float* afSubd) {
    float v6 = m_pEntry[0][1];
    float v8 = m_pEntry[0][2];
    float v10 = m_pEntry[1][1];
    float v12 = m_pEntry[1][2];
    float v11 = m_pEntry[2][2];
    float fAbsEntry02 = std::fabs(static_cast<double>(v8));
    afDiag[0] = m_pEntry[0][0];
    if (!(fAbsEntry02 >= 1e-08f)) {
        afDiag[1] = v10;
        afDiag[2] = v11;
        afSubd[0] = v6;
        afSubd[1] = v12;
        m_pEntry[0][0] = 1.0;
        m_pEntry[0][1] = 0.0;
        m_pEntry[0][2] = 0.0;
        m_pEntry[1][0] = 0.0;
        m_pEntry[1][2] = 0.0;
        m_pEntry[2][0] = 0.0;
        m_pEntry[2][1] = 0.0;
        m_pEntry[1][1] = 1.0;
        m_pEntry[2][2] = 1.0;
        return 0;
    }

    float fSqrLength = v8 * v8 + v6 * v6;
    float fLength = std::sqrt(static_cast<double>(fSqrLength));
    float fInvLength = 1.0 / fLength;
    float v7 = fInvLength * v6;
    float v9 = fInvLength * v8;
    float fQ = (v7 + v7) * v12 + (v11 - v10) * v9;
    afSubd[0] = fLength;
    afDiag[1] = v10 + fQ * v9;
    afDiag[2] = v11 - fQ * v9;
    afSubd[1] = v12 - fQ * v7;
    m_pEntry[0][0] = 1.0;
    m_pEntry[0][1] = 0.0;
    m_pEntry[0][2] = 0.0;
    m_pEntry[1][0] = 0.0;
    m_pEntry[2][0] = 0.0;
    m_pEntry[1][1] = v7;
    m_pEntry[1][2] = v9;
    m_pEntry[2][1] = v9;
    m_pEntry[2][2] = -v7;
    return 1;
}

char NiMatrix3::QLAlgorithm(float* afDiag, float* afSubd) {
    double v3 = 1.0;
    int32_t v6 = 0;
    while (true) {
        double v10;
        double v16;
        double v17;
        double v18;
        double v27;
        double v28;
        float fAbsDiag1 = std::fabs(static_cast<double>(afDiag[1]));
        float fAbsDiag0 = std::fabs(static_cast<double>(afDiag[0]));
        double v7 = fAbsDiag1;
        float fSum01 = fAbsDiag0 + fAbsDiag1;
        float fAbsSubd0 = std::fabs(static_cast<double>(afSubd[0]));
        if (fSum01 == fAbsSubd0 + fSum01) {
            break;
        }
        float fAbsDiag2 = std::fabs(static_cast<double>(afDiag[2]));
        float fSum12 = v7 + fAbsDiag2;
        float fAbsSubd1 = std::fabs(static_cast<double>(afSubd[1]));
        if (fSum12 == fAbsSubd1 + fSum12) {
            double v39 = 0.0;
            afSubd[1] = 0.0;
            float fAbs0 = std::fabs(static_cast<double>(afDiag[0]));
            double v40 = fAbs0;
            float fAbs1 = std::fabs(static_cast<double>(afDiag[1]));
            float fSum = v40 + fAbs1;
            float fAbsSub = std::fabs(static_cast<double>(afSubd[0]));
            if (fSum != fAbsSub + fSum) {
                double v41;
                float v50;
                float fTrace = afDiag[0] + afDiag[1];
                float fDiff = afDiag[0] - afDiag[1];
                float v67 = fDiff * fDiff + afSubd[0] * 4.0 * afSubd[0];
                float v68 = std::sqrt(static_cast<double>(v67));
                float v57 = (fTrace - v68) * 0.5;
                float v63 = (fTrace + v68) * 0.5;
                if (!(fDiff >= 0.0)) {
                    v50 = afDiag[1] - v57;
                    v41 = afSubd[0];
                }
                else {
                    v50 = afSubd[0];
                    v41 = afDiag[0] - v57;
                }
                float fVecX = v41;
                float fSqrLength = fVecX * fVecX + v50 * v50;
                float fLength = std::sqrt(static_cast<double>(fSqrLength));
                float fInvLength = 1.0 / fLength;
                float v51 = fInvLength * v50;
                float fSin = fInvLength * fVecX;
                double v42 = m_pEntry[0][1];
                m_pEntry[0][1] = v42 * v51 + m_pEntry[0][0] * fSin;
                m_pEntry[0][0] = m_pEntry[0][0] * v51 - v42 * fSin;
                double v43 = m_pEntry[1][1];
                m_pEntry[1][1] = v43 * v51 + fSin * m_pEntry[1][0];
                m_pEntry[1][0] = v51 * m_pEntry[1][0] - v43 * fSin;
                float fEntry21 = m_pEntry[2][1];
                m_pEntry[2][1] = m_pEntry[2][0] * fSin + fEntry21 * v51;
                m_pEntry[2][0] = v51 * m_pEntry[2][0] - fSin * fEntry21;
                afDiag[0] = v57;
                afDiag[1] = v63;
                v39 = 0.0;
            }
            afSubd[0] = v39;
            return 1;
        }
        float fTheta = (afDiag[1] - afDiag[0]) / (afSubd[0] + afSubd[0]);
        float fThetaRadicand = v3 + fTheta * fTheta;
        float fThetaRoot = std::sqrt(static_cast<double>(fThetaRadicand));
        float v58 = afSubd[1];
        double v8 = fTheta;
        double v9 = fThetaRoot;
        if (!(fTheta >= 0.0)) {
            v10 = v8 - v9;
        }
        else {
            v10 = v8 + v9;
        }
        float fDiagDiff = afDiag[2] - afDiag[0];
        float fG = afSubd[0] / v10 + fDiagDiff;
        double v11 = v58;
        float fAbsF = std::fabs(static_cast<double>(v58));
        double v12 = fAbsF;
        float fAbsG = std::fabs(static_cast<double>(fG));
        bool v13 = !(fAbsG >= v12);
        bool v14 = fAbsG == v12;
        double v15 = fG;
        if (v13 || v14) {
            float fRatio = v15 / v11;
            float fRadicand = fRatio * fRatio + 1.0;
            float fRoot = std::sqrt(static_cast<double>(fRadicand));
            v16 = 1.0;
            float fInvRoot = 1.0 / fRoot;
            v17 = fInvRoot;
            float v44 = fRatio * fInvRoot;
            v18 = v44;
        }
        else {
            float fRatio = v11 / v15;
            float fRadicand = fRatio * fRatio + 1.0;
            float fRoot = std::sqrt(static_cast<double>(fRadicand));
            v16 = 1.0;
            float v45 = 1.0 / fRoot;
            float fScaledRatio = fRatio * v45;
            v18 = v45;
            v17 = fScaledRatio;
        }
        double v19 = m_pEntry[0][2];
        m_pEntry[0][2] = m_pEntry[0][1] * v17 + v19 * v18;
        m_pEntry[0][1] = m_pEntry[0][1] * v18 - v19 * v17;
        double v20 = m_pEntry[1][2];
        m_pEntry[1][2] = m_pEntry[1][1] * v17 + v20 * v18;
        m_pEntry[1][1] = m_pEntry[1][1] * v18 - v20 * v17;
        double v21 = m_pEntry[2][2];
        m_pEntry[2][2] = v21 * v18 + v17 * m_pEntry[2][1];
        m_pEntry[2][1] = v18 * m_pEntry[2][1] - v21 * v17;
        float v52 = (afDiag[1] - afDiag[2]) * v17 + (afSubd[1] + afSubd[1]) * v18;
        float v64 = afSubd[0] * v18;
        float v59 = afSubd[0] * v17;
        float fQ = v18 * v52 - afSubd[1];
        float v53 = v17 * v52;
        double v22 = v59;
        float fAbsP = std::fabs(static_cast<double>(v59));
        double v23 = fAbsP;
        float fAbsQ = std::fabs(static_cast<double>(fQ));
        bool v24 = !(fAbsQ >= v23);
        bool v25 = fAbsQ == v23;
        double v26 = fQ;
        if (v24 || v25) {
            float fRatio = v26 / v22;
            float fRadicand = v16 + fRatio * fRatio;
            float fRoot = std::sqrt(static_cast<double>(fRadicand));
            v3 = 1.0;
            float fInvRoot = 1.0 / fRoot;
            v27 = fInvRoot;
            float v46 = fRatio * fInvRoot;
            v28 = v46;
            afSubd[1] = v59 * fRoot;
        }
        else {
            float fRatio = v22 / v26;
            float v60 = v16 + fRatio * fRatio;
            float v61 = std::sqrt(static_cast<double>(v60));
            v3 = 1.0;
            float v47 = 1.0 / v61;
            float fScaledRatio = fRatio * v47;
            v28 = v47;
            v27 = fScaledRatio;
            afSubd[1] = fQ * v61;
        }
        double v29 = m_pEntry[0][1];
        m_pEntry[0][1] = m_pEntry[0][0] * v27 + v29 * v28;
        m_pEntry[0][0] = m_pEntry[0][0] * v28 - v29 * v27;
        double v30 = m_pEntry[1][1];
        m_pEntry[1][1] = v30 * v28 + v27 * m_pEntry[1][0];
        m_pEntry[1][0] = v28 * m_pEntry[1][0] - v30 * v27;
        double v31 = m_pEntry[2][1];
        m_pEntry[2][1] = m_pEntry[2][0] * v27 + v31 * v28;
        m_pEntry[2][0] = m_pEntry[2][0] * v28 - v31 * v27;
        float fDiag1 = afDiag[1] - v53;
        afDiag[2] = v53 + afDiag[2];
        float v54 = (afDiag[0] - fDiag1) * v27 + (v64 + v64) * v28;
        afSubd[0] = v28 * v54 - v64;
        float v55 = v27 * v54;
        ++v6;
        afDiag[1] = fDiag1 + v55;
        afDiag[0] = afDiag[0] - v55;
        if (v6 >= 32) {
            return 0;
        }
    }
    double v33 = 0.0;
    afSubd[0] = 0.0;
    float fAbsDiag1 = std::fabs(static_cast<double>(afDiag[1]));
    double v34 = fAbsDiag1;
    float fAbsDiag2 = std::fabs(static_cast<double>(afDiag[2]));
    float fSum12 = v34 + fAbsDiag2;
    float fAbsSubd1 = std::fabs(static_cast<double>(afSubd[1]));
    if (fSum12 != fAbsSubd1 + fSum12) {
        double v35;
        float v48;
        float fTrace = afDiag[2] + afDiag[1];
        float fDiff = afDiag[1] - afDiag[2];
        float v65 = fDiff * fDiff + afSubd[1] * 4.0 * afSubd[1];
        float v66 = std::sqrt(static_cast<double>(v65));
        float v56 = (fTrace - v66) * 0.5;
        float v62 = (fTrace + v66) * 0.5;
        if (!(fDiff >= 0.0)) {
            v48 = afDiag[2] - v56;
            v35 = afSubd[1];
        }
        else {
            v48 = afSubd[1];
            v35 = afDiag[1] - v56;
        }
        float fVecX = v35;
        float fSqrLength = fVecX * fVecX + v48 * v48;
        float fLength = std::sqrt(static_cast<double>(fSqrLength));
        float fInvLength = 1.0 / fLength;
        float v49 = fInvLength * v48;
        float fSin = fInvLength * fVecX;
        double v36 = m_pEntry[0][2];
        m_pEntry[0][2] = v36 * v49 + m_pEntry[0][1] * fSin;
        m_pEntry[0][1] = m_pEntry[0][1] * v49 - v36 * fSin;
        double v37 = m_pEntry[1][2];
        m_pEntry[1][2] = m_pEntry[1][1] * fSin + v37 * v49;
        m_pEntry[1][1] = m_pEntry[1][1] * v49 - v37 * fSin;
        double v38 = m_pEntry[2][2];
        m_pEntry[2][2] = v38 * v49 + fSin * m_pEntry[2][1];
        m_pEntry[2][1] = v49 * m_pEntry[2][1] - fSin * v38;
        afDiag[1] = v56;
        afDiag[2] = v62;
        v33 = 0.0;
    }
    afSubd[1] = v33;
    return 1;
}

// Each output point is rot * point + trn, each coordinate summed in the game's order and its input read again.
void NiMatrix3::TransformVertices(
    const NiMatrix3& rot,
    const NiPoint3& trn,
    uint32_t uiVertexCount,
    const NiPoint3* pInVertex,
    NiPoint3* pOutVertex
) {
    for (uint32_t i = 0; i < uiVertexCount; i++) {
        pOutVertex[i].x = rot.m_pEntry[0][0] * pInVertex[i].x + trn.x + rot.m_pEntry[0][1] * pInVertex[i].y +
            pInVertex[i].z * rot.m_pEntry[0][2];
        pOutVertex[i].y = rot.m_pEntry[1][0] * pInVertex[i].x + trn.y + rot.m_pEntry[1][1] * pInVertex[i].y +
            rot.m_pEntry[1][2] * pInVertex[i].z;
        pOutVertex[i].z = rot.m_pEntry[2][0] * pInVertex[i].x + trn.z + pInVertex[i].y * rot.m_pEntry[2][1] +
            rot.m_pEntry[2][2] * pInVertex[i].z;
    }
}

float*
NiMatrix3::EigenSolveSymmetric(void* apThis, [[maybe_unused]] void* apEdx, float* afEigenvalue, float* akEigenvector) {
    float v5[2];
    NiMatrix3 v6;
    std::memcpy(&v6, apThis, sizeof(v6));
    int8_t v3 = v6.Tridiagonal(afEigenvalue, v5);
    v6.QLAlgorithm(afEigenvalue, v5);
    float* result = akEigenvector;
    akEigenvector[0] = v6.m_pEntry[0][0];
    akEigenvector[1] = v6.m_pEntry[1][0];
    akEigenvector[2] = v6.m_pEntry[2][0];
    akEigenvector[3] = v6.m_pEntry[0][1];
    akEigenvector[4] = v6.m_pEntry[1][1];
    akEigenvector[5] = v6.m_pEntry[2][1];
    if (v3) {
        akEigenvector[6] = (-v6.m_pEntry[0][2]);
        akEigenvector[7] = (-v6.m_pEntry[1][2]);
        akEigenvector[8] = (-v6.m_pEntry[2][2]);
    }
    else {
        akEigenvector[6] = v6.m_pEntry[0][2];
        akEigenvector[7] = v6.m_pEntry[1][2];
        akEigenvector[8] = v6.m_pEntry[2][2];
    }
    return result;
}

// Clamped arc cosine: pi at or below -1 (and for NaN), 0 at or above 1, else acos rounded to float.
float NiACos(float fValue) {
    if (!(fValue > -1.0)) {
        return 3.1415927f;
    }
    if (!(1.0 > fValue)) {
        return 0.0f;
    }
    return std::acos(fValue);
}
