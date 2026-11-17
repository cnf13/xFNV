#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"

#include "Fallout Shared/Magic/EffectSetting.h"
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

// The three rotations are built inline, then multiplied together.
void NiMatrix3::FromEulerAnglesZXY(float fZAngle, float fXAngle, float fYAngle) {
    NiMatrix3 kXRot, kYRot, kZRot;
    float fSin;
    float fCos;
    // EXE-INLINED 0x524AC0
    fSin = std::sin(fXAngle);
    fCos = std::cos(fXAngle);
    kXRot.m_pEntry[0][0] = 1.0f;
    kXRot.m_pEntry[0][1] = 0.0f;
    kXRot.m_pEntry[0][2] = 0.0f;
    kXRot.m_pEntry[1][0] = 0.0f;
    kXRot.m_pEntry[1][1] = fCos;
    kXRot.m_pEntry[1][2] = fSin;
    kXRot.m_pEntry[2][0] = 0.0f;
    kXRot.m_pEntry[2][1] = -fSin;
    kXRot.m_pEntry[2][2] = fCos;
    // EXE-INLINED 0x43F850
    fSin = std::sin(fYAngle);
    fCos = std::cos(fYAngle);
    kYRot.m_pEntry[0][0] = fCos;
    kYRot.m_pEntry[0][1] = 0.0f;
    kYRot.m_pEntry[0][2] = -fSin;
    kYRot.m_pEntry[1][0] = 0.0f;
    kYRot.m_pEntry[1][1] = 1.0f;
    kYRot.m_pEntry[1][2] = 0.0f;
    kYRot.m_pEntry[2][0] = fSin;
    kYRot.m_pEntry[2][1] = 0.0f;
    kYRot.m_pEntry[2][2] = fCos;
    // EXE-INLINED 0x4A0C90
    fSin = std::sin(fZAngle);
    fCos = std::cos(fZAngle);
    kZRot.m_pEntry[0][0] = fCos;
    kZRot.m_pEntry[0][1] = fSin;
    kZRot.m_pEntry[0][2] = 0.0f;
    kZRot.m_pEntry[1][0] = -fSin;
    kZRot.m_pEntry[1][1] = fCos;
    kZRot.m_pEntry[1][2] = 0.0f;
    kZRot.m_pEntry[2][0] = 0.0f;
    kZRot.m_pEntry[2][1] = 0.0f;
    kZRot.m_pEntry[2][2] = 1.0f;
    *this = kZRot * (kXRot * kYRot);
}

// The three rotations are built inline, then multiplied together.
void NiMatrix3::FromEulerAnglesZYX(float fZAngle, float fYAngle, float fXAngle) {
    NiMatrix3 kXRot, kYRot, kZRot;
    float fSin;
    float fCos;
    // EXE-INLINED 0x524AC0
    fSin = std::sin(fXAngle);
    fCos = std::cos(fXAngle);
    kXRot.m_pEntry[0][0] = 1.0f;
    kXRot.m_pEntry[0][1] = 0.0f;
    kXRot.m_pEntry[0][2] = 0.0f;
    kXRot.m_pEntry[1][0] = 0.0f;
    kXRot.m_pEntry[1][1] = fCos;
    kXRot.m_pEntry[1][2] = fSin;
    kXRot.m_pEntry[2][0] = 0.0f;
    kXRot.m_pEntry[2][1] = -fSin;
    kXRot.m_pEntry[2][2] = fCos;
    // EXE-INLINED 0x43F850
    fSin = std::sin(fYAngle);
    fCos = std::cos(fYAngle);
    kYRot.m_pEntry[0][0] = fCos;
    kYRot.m_pEntry[0][1] = 0.0f;
    kYRot.m_pEntry[0][2] = -fSin;
    kYRot.m_pEntry[1][0] = 0.0f;
    kYRot.m_pEntry[1][1] = 1.0f;
    kYRot.m_pEntry[1][2] = 0.0f;
    kYRot.m_pEntry[2][0] = fSin;
    kYRot.m_pEntry[2][1] = 0.0f;
    kYRot.m_pEntry[2][2] = fCos;
    // EXE-INLINED 0x4A0C90
    fSin = std::sin(fZAngle);
    fCos = std::cos(fZAngle);
    kZRot.m_pEntry[0][0] = fCos;
    kZRot.m_pEntry[0][1] = fSin;
    kZRot.m_pEntry[0][2] = 0.0f;
    kZRot.m_pEntry[1][0] = -fSin;
    kZRot.m_pEntry[1][1] = fCos;
    kZRot.m_pEntry[1][2] = 0.0f;
    kZRot.m_pEntry[2][0] = 0.0f;
    kZRot.m_pEntry[2][1] = 0.0f;
    kZRot.m_pEntry[2][2] = 1.0f;
    *this = kZRot * (kYRot * kXRot);
}

void NiMatrix3::GetRow(uint32_t uiRow, NiPoint3& row) const {
    row.x = m_pEntry[uiRow][0];
    row.y = m_pEntry[uiRow][1];
    row.z = m_pEntry[uiRow][2];
}

void NiMatrix3::SetRow(uint32_t uiRow, float f0, float f1, float f2) {
    m_pEntry[uiRow][0] = f0;
    m_pEntry[uiRow][1] = f1;
    m_pEntry[uiRow][2] = f2;
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
    rt::eh_vector_constructor_iterator(row, sizeof(NiPoint3), 3, &emptyPointConstructor);
    GetRow(0, row[0]);
    GetRow(1, row[1]);
    GetRow(2, row[2]);
    return NiMatrix3(row[0], row[1], row[2]);
}

NiMatrix3 NiMatrix3::operator*(const NiMatrix3& mat) const {
    NiMatrix3 result;
    NiMatrix3 prd;
    prd.m_pEntry[0][0] =
        m_pEntry[0][0] * mat.m_pEntry[0][0] + m_pEntry[0][1] * mat.m_pEntry[1][0] + m_pEntry[0][2] * mat.m_pEntry[2][0];
    prd.m_pEntry[1][0] =
        m_pEntry[1][0] * mat.m_pEntry[0][0] + m_pEntry[1][1] * mat.m_pEntry[1][0] + m_pEntry[1][2] * mat.m_pEntry[2][0];
    prd.m_pEntry[2][0] =
        m_pEntry[2][0] * mat.m_pEntry[0][0] + m_pEntry[2][1] * mat.m_pEntry[1][0] + m_pEntry[2][2] * mat.m_pEntry[2][0];
    prd.m_pEntry[0][1] =
        m_pEntry[0][0] * mat.m_pEntry[0][1] + m_pEntry[0][1] * mat.m_pEntry[1][1] + m_pEntry[0][2] * mat.m_pEntry[2][1];
    prd.m_pEntry[1][1] =
        m_pEntry[1][0] * mat.m_pEntry[0][1] + m_pEntry[1][1] * mat.m_pEntry[1][1] + m_pEntry[1][2] * mat.m_pEntry[2][1];
    prd.m_pEntry[2][1] =
        m_pEntry[2][0] * mat.m_pEntry[0][1] + m_pEntry[2][1] * mat.m_pEntry[1][1] + m_pEntry[2][2] * mat.m_pEntry[2][1];
    prd.m_pEntry[0][2] =
        m_pEntry[0][0] * mat.m_pEntry[0][2] + m_pEntry[0][1] * mat.m_pEntry[1][2] + m_pEntry[0][2] * mat.m_pEntry[2][2];
    prd.m_pEntry[1][2] =
        m_pEntry[1][0] * mat.m_pEntry[0][2] + m_pEntry[1][1] * mat.m_pEntry[1][2] + m_pEntry[1][2] * mat.m_pEntry[2][2];
    prd.m_pEntry[2][2] =
        m_pEntry[2][0] * mat.m_pEntry[0][2] + m_pEntry[2][1] * mat.m_pEntry[1][2] + m_pEntry[2][2] * mat.m_pEntry[2][2];
    result = prd;
    return result;
}

NiPoint3 NiMatrix3::operator*(const NiPoint3& pt) const {
    NiPoint3 result;
    float z = m_pEntry[2][0] * pt.x + m_pEntry[2][1] * pt.y + m_pEntry[2][2] * pt.z;
    float y = m_pEntry[1][0] * pt.x + m_pEntry[1][1] * pt.y + m_pEntry[1][2] * pt.z;
    float x = m_pEntry[0][0] * pt.x + m_pEntry[0][1] * pt.y + m_pEntry[0][2] * pt.z;
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
    result.m_pEntry[0][0] =
        mat.m_pEntry[0][0] * m_pEntry[0][0] + mat.m_pEntry[1][0] * m_pEntry[1][0] + mat.m_pEntry[2][0] * m_pEntry[2][0];
    result.m_pEntry[1][0] =
        m_pEntry[1][1] * mat.m_pEntry[1][0] + m_pEntry[0][1] * mat.m_pEntry[0][0] + mat.m_pEntry[2][0] * m_pEntry[2][1];
    result.m_pEntry[2][0] =
        m_pEntry[1][2] * mat.m_pEntry[1][0] + m_pEntry[0][2] * mat.m_pEntry[0][0] + mat.m_pEntry[2][0] * m_pEntry[2][2];
    result.m_pEntry[0][1] =
        mat.m_pEntry[0][1] * m_pEntry[0][0] + mat.m_pEntry[1][1] * m_pEntry[1][0] + m_pEntry[2][0] * mat.m_pEntry[2][1];
    result.m_pEntry[1][1] =
        mat.m_pEntry[1][1] * m_pEntry[1][1] + mat.m_pEntry[0][1] * m_pEntry[0][1] + mat.m_pEntry[2][1] * m_pEntry[2][1];
    result.m_pEntry[2][1] =
        mat.m_pEntry[1][1] * m_pEntry[1][2] + mat.m_pEntry[0][1] * m_pEntry[0][2] + mat.m_pEntry[2][1] * m_pEntry[2][2];
    result.m_pEntry[0][2] =
        mat.m_pEntry[0][2] * m_pEntry[0][0] + mat.m_pEntry[1][2] * m_pEntry[1][0] + m_pEntry[2][0] * mat.m_pEntry[2][2];
    result.m_pEntry[1][2] =
        mat.m_pEntry[1][2] * m_pEntry[1][1] + mat.m_pEntry[0][2] * m_pEntry[0][1] + mat.m_pEntry[2][2] * m_pEntry[2][1];
    result.m_pEntry[2][2] =
        mat.m_pEntry[1][2] * m_pEntry[1][2] + mat.m_pEntry[0][2] * m_pEntry[0][2] + mat.m_pEntry[2][2] * m_pEntry[2][2];
    return result;
}

bool NiMatrix3::Inverse(NiMatrix3& inv) const {
    int32_t col;
    int32_t row;
    float fInvdet;
    (&inv)->m_pEntry[0][0] = ((m_pEntry[1][1] * m_pEntry[2][2]) - (m_pEntry[1][2] * m_pEntry[2][1]));
    (&inv)->m_pEntry[0][1] = ((m_pEntry[0][2] * m_pEntry[2][1]) - (m_pEntry[0][1] * m_pEntry[2][2]));
    (&inv)->m_pEntry[0][2] = ((m_pEntry[0][1] * m_pEntry[1][2]) - (m_pEntry[0][2] * m_pEntry[1][1]));
    (&inv)->m_pEntry[1][0] = ((m_pEntry[1][2] * m_pEntry[2][0]) - (m_pEntry[1][0] * m_pEntry[2][2]));
    (&inv)->m_pEntry[1][1] = ((m_pEntry[0][0] * m_pEntry[2][2]) - (m_pEntry[0][2] * m_pEntry[2][0]));
    (&inv)->m_pEntry[1][2] = ((m_pEntry[0][2] * m_pEntry[1][0]) - (m_pEntry[0][0] * m_pEntry[1][2]));
    (&inv)->m_pEntry[2][0] = ((m_pEntry[1][0] * m_pEntry[2][1]) - (m_pEntry[1][1] * m_pEntry[2][0]));
    (&inv)->m_pEntry[2][1] = ((m_pEntry[0][1] * m_pEntry[2][0]) - (m_pEntry[0][0] * m_pEntry[2][1]));
    (&inv)->m_pEntry[2][2] = ((m_pEntry[0][0] * m_pEntry[1][1]) - (m_pEntry[0][1] * m_pEntry[1][0]));
    float fDet = ((m_pEntry[0][0] * (&inv)->m_pEntry[0][0]) + (m_pEntry[0][1] * (&inv)->m_pEntry[1][0])) +
        (m_pEntry[0][2] * (&inv)->m_pEntry[2][0]);
    if (NiAbs(fDet) <= 0x1.0c6f7a0000000p-20) {
        return false;
    }
    for (row = 0; row < 3; ++row) {
        for (col = 0; col < 3; ++col) {
            fInvdet = (float)(1.0 / fDet);
            (&inv)->m_pEntry[row][col] = ((&inv)->m_pEntry[row][col] * fInvdet);
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
    else {
        if (!(-v6 < v11)) {
            float rfYAnglea = NiFastATan2(m_pEntry[1][0], m_pEntry[1][1]);
            rfZAngle = 0.0;
            rfXAngle = rfYAnglea - 0.0;
            return false;
        }
        else {
            float fY = -m_pEntry[1][2];
            rfXAngle = -NiFastATan2(fY, m_pEntry[2][2]);
            float fYa = -m_pEntry[0][1];
            rfZAngle = -NiFastATan2(fYa, m_pEntry[0][0]);
            return true;
        }
    }
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
    else {
        if (!(-v6 < v11)) {
            float x = NiFastATan2(m_pEntry[0][2], m_pEntry[0][0]);
            rfYAngle = 0.0;
            rfZAngle = x - 0.0;
            return false;
        }
        else {
            float fY = -m_pEntry[0][1];
            rfZAngle = -NiFastATan2(fY, m_pEntry[1][1]);
            float fYa = -m_pEntry[2][0];
            rfYAngle = -NiFastATan2(fYa, m_pEntry[2][2]);
            return true;
        }
    }
}

// The three rotations are built inline, then multiplied together.
void NiMatrix3::FromEulerAnglesXYZ(float fXAngle, float fYAngle, float fZAngle) {
    NiMatrix3 kXRot, kYRot, kZRot;
    float fSin;
    float fCos;
    // EXE-INLINED 0x524AC0
    fSin = std::sin(fXAngle);
    fCos = std::cos(fXAngle);
    kXRot.m_pEntry[0][0] = 1.0f;
    kXRot.m_pEntry[0][1] = 0.0f;
    kXRot.m_pEntry[0][2] = 0.0f;
    kXRot.m_pEntry[1][0] = 0.0f;
    kXRot.m_pEntry[1][1] = fCos;
    kXRot.m_pEntry[1][2] = fSin;
    kXRot.m_pEntry[2][0] = 0.0f;
    kXRot.m_pEntry[2][1] = -fSin;
    kXRot.m_pEntry[2][2] = fCos;
    // EXE-INLINED 0x43F850
    fSin = std::sin(fYAngle);
    fCos = std::cos(fYAngle);
    kYRot.m_pEntry[0][0] = fCos;
    kYRot.m_pEntry[0][1] = 0.0f;
    kYRot.m_pEntry[0][2] = -fSin;
    kYRot.m_pEntry[1][0] = 0.0f;
    kYRot.m_pEntry[1][1] = 1.0f;
    kYRot.m_pEntry[1][2] = 0.0f;
    kYRot.m_pEntry[2][0] = fSin;
    kYRot.m_pEntry[2][1] = 0.0f;
    kYRot.m_pEntry[2][2] = fCos;
    // EXE-INLINED 0x4A0C90
    fSin = std::sin(fZAngle);
    fCos = std::cos(fZAngle);
    kZRot.m_pEntry[0][0] = fCos;
    kZRot.m_pEntry[0][1] = fSin;
    kZRot.m_pEntry[0][2] = 0.0f;
    kZRot.m_pEntry[1][0] = -fSin;
    kZRot.m_pEntry[1][1] = fCos;
    kZRot.m_pEntry[1][2] = 0.0f;
    kZRot.m_pEntry[2][0] = 0.0f;
    kZRot.m_pEntry[2][1] = 0.0f;
    kZRot.m_pEntry[2][2] = 1.0f;
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
    float omcs = (float)(1.0 - cosOut);
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

// Each sum of products is computed in double and rounded to float once, as the game does. The reference source's last
// length test is left out because its result is unused.
void NiMatrix3::ExtractAngleAndAxis(float& fAngle, float& fX, float& fY, float& fZ) const {
    float fTrace = static_cast<float>(static_cast<double>(m_pEntry[0][0]) + m_pEntry[1][1] + m_pEntry[2][2]);
    float fCos = static_cast<float>((static_cast<double>(fTrace) - 1.0) * 0.5);
    float fACos;
    // EXE-INLINED 0xA58510
    if (!(fCos > -1.0)) {
        fACos = 3.1415927f;
    }
    else if (!(1.0 > fCos)) {
        fACos = 0.0f;
    }
    else {
        fACos = std::acos(fCos);
    }
    fAngle = fACos;
    fX = m_pEntry[1][2] - m_pEntry[2][1];
    fY = m_pEntry[2][0] - m_pEntry[0][2];
    float fZDiff = m_pEntry[0][1] - m_pEntry[1][0];
    fZ = fZDiff;
    float fSqrLength = static_cast<float>(
        static_cast<double>(fY) * fY + static_cast<double>(fX) * fX + static_cast<double>(fZDiff) * fZDiff
    );
    // EXE-INLINED none: NiSqrt (the exe's __CIsqrt)
    float fLength = std::sqrt(fSqrLength);
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
    // EXE-INLINED none: NiSqrt (the exe's __CIsqrt)
    fX = std::sqrt(static_cast<float>((static_cast<double>(m_pEntry[0][0]) + 1.0) * 0.5));
    // EXE-INLINED none: NiSqrt (the exe's __CIsqrt)
    fY = std::sqrt(static_cast<float>((static_cast<double>(m_pEntry[1][1]) + 1.0) * 0.5));
    // EXE-INLINED none: NiSqrt (the exe's __CIsqrt)
    float fZRoot = std::sqrt(static_cast<float>((static_cast<double>(m_pEntry[2][2]) + 1.0) * 0.5));
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
    float fSqrDiff =
        static_cast<float>(static_cast<double>(ty) * ty + static_cast<double>(tx) * tx + static_cast<double>(tz) * tz);
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
    fSqrDiff =
        static_cast<float>(static_cast<double>(ty) * ty + static_cast<double>(tx) * tx + static_cast<double>(tz) * tz);
    if (1e-06f > fSqrDiff) {
        return;
    }
    // The Gamebryo reference source tests the residual once more; this code deliberately skips that test, as the game
    // does.
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
    for (int32_t iRow = 0; iRow < 3; iRow++) {
        for (int32_t iCol = 0; iCol < 3; iCol++) {
            NiStreamLoadBinary(stream, m_pEntry[iRow][iCol]);
        }
    }
}

void NiMatrix3::SaveBinary(NiStream& stream) {
    Snap();
    for (int32_t iRow = 0; iRow < 3; iRow++) {
        for (int32_t iCol = 0; iCol < 3; iCol++) {
            NiStreamSaveBinary(stream, m_pEntry[iRow][iCol]);
        }
    }
}

char* NiMatrix3::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 128;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString, stLength, "%s = ((%g,%g,%g),(%g,%g,%g),(%g,%g,%g))", pPrefix, static_cast<double>(m_pEntry[0][0]),
        static_cast<double>(m_pEntry[0][1]), static_cast<double>(m_pEntry[0][2]), static_cast<double>(m_pEntry[1][0]),
        static_cast<double>(m_pEntry[1][1]), static_cast<double>(m_pEntry[1][2]), static_cast<double>(m_pEntry[2][0]),
        static_cast<double>(m_pEntry[2][1]), static_cast<double>(m_pEntry[2][2])
    );
    return pcString;
}

void NiMatrix3::MakeZero() {
    SetCol((uint32_t)0, (float)0.0, (float)0.0, (float)0.0);
    SetCol((uint32_t)1, (float)0.0, (float)0.0, (float)0.0);
    SetCol((uint32_t)2, (float)0.0, (float)0.0, (float)0.0);
}

bool NiMatrix3::operator!=(const NiMatrix3& mat) const {
    return (bool)((uint8_t)(!operator==(mat)));
}

float NiMatrix3::GetEntry(uint32_t uiRow, uint32_t uiCol) const {
    return m_pEntry[uiRow][uiCol];
}

void NiMatrix3::MakeIdentity() {
    m_pEntry[0][0] = (float)1.0;
    m_pEntry[0][1] = (float)0.0;
    m_pEntry[0][2] = (float)0.0;
    m_pEntry[1][0] = (float)0.0;
    m_pEntry[1][1] = (float)1.0;
    m_pEntry[1][2] = (float)0.0;
    m_pEntry[2][0] = (float)0.0;
    m_pEntry[2][1] = (float)0.0;
    m_pEntry[2][2] = (float)1.0;
}

void NiMatrix3::SetEntry(uint32_t uiRow, uint32_t uiCol, float fEntry) {
    m_pEntry[uiRow][uiCol] = fEntry;
}

void NiMatrix3::SetRow_005DF510(int32_t a2, void* a3) {
    m_pEntry[a2][0] = *static_cast<float*>(a3);
    m_pEntry[a2][1] = *(static_cast<float*>(a3) + 1);
    m_pEntry[a2][2] = *(static_cast<float*>(a3) + 2);
}

void NiMatrix3::GetRow_005DF560(int32_t a2, float* a3, float* a4, float* a5) {
    *a3 = m_pEntry[a2][0];
    *a4 = m_pEntry[a2][1];
    *a5 = m_pEntry[a2][2];
}

// Each entry whose magnitude is at most 1e-8 (NaN excluded) and which is not already zero becomes zero.
void NiMatrix3::Snap() {
    for (int32_t iRow = 0; iRow < 3; iRow++) {
        for (int32_t iCol = 0; iCol < 3; iCol++) {
            float fAbs = fabsf(m_pEntry[iRow][iCol]);
            if (fAbs <= 1e-08f && m_pEntry[iRow][iCol] != 0.0f) {
                m_pEntry[iRow][iCol] = 0.0f;
            }
        }
    }
}

char NiMatrix3::Tridiagonal(float* a1, float* a2) {
    float v6 = m_pEntry[0][1];
    float v8 = m_pEntry[0][2];
    float v10 = m_pEntry[1][1];
    float v12 = m_pEntry[1][2];
    float v11 = m_pEntry[2][2];
    *a1 = m_pEntry[0][0];
    float a1a = std::fabs((double)v8);
    if (!(a1a >= 1e-08f)) {
        a1[1] = v10;
        a1[2] = v11;
        *a2 = v6;
        a2[1] = v12;
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
    else {
        float a1b = v8 * v8 + v6 * v6;
        float a1c = std::sqrt((double)a1b);
        *a2 = a1c;
        float a2a = 1.0 / a1c;
        float v7 = a2a * v6;
        float v9 = a2a * v8;
        float a2b = (v7 + v7) * v12 + (v11 - v10) * v9;
        a1[1] = v10 + a2b * v9;
        a1[2] = v11 - a2b * v9;
        a2[1] = v12 - a2b * v7;
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
}

char NiMatrix3::QLAlgorithm(float* a1, float* a2) {
    double v3 = 1.0;
    int32_t v6 = 0;
    while (true) {
        double v10;
        double v16;
        double v17;
        double v18;
        double v27;
        double v28;
        float a2b = std::fabs((double)a1[1]);
        float a1a = std::fabs((double)*a1);
        double v7 = a2b;
        float a1b = a1a + a2b;
        float a2c = std::fabs((double)*a2);
        if (a1b == a2c + a1b) {
            break;
        }
        float a1c = std::fabs((double)a1[2]);
        float a1d = v7 + a1c;
        float a2d = std::fabs((double)a2[1]);
        if (a1d == a2d + a1d) {
            double v39 = 0.0;
            a2[1] = 0.0;
            float a1v = std::fabs((double)*a1);
            double v40 = a1v;
            float a1w = std::fabs((double)a1[1]);
            float a1x = v40 + a1w;
            float a2w = std::fabs((double)*a2);
            if (a1x != a2w + a1x) {
                double v41;
                float v50;
                float a1y = *a1 + a1[1];
                float a2x = *a1 - a1[1];
                float v67 = a2x * a2x + *a2 * 4.0 * *a2;
                float v68 = std::sqrt((double)v67);
                float v57 = (a1y - v68) * 0.5;
                float v63 = (a1y + v68) * 0.5;
                if (!(a2x >= 0.0)) {
                    v50 = a1[1] - v57;
                    v41 = *a2;
                }
                else {
                    v50 = *a2;
                    v41 = *a1 - v57;
                }
                float a2y = v41;
                float a1z = a2y * a2y + v50 * v50;
                float a1ba = std::sqrt((double)a1z);
                float a1bb = 1.0 / a1ba;
                float v51 = a1bb * v50;
                float a2z = a1bb * a2y;
                double v42 = m_pEntry[0][1];
                m_pEntry[0][1] = v42 * v51 + m_pEntry[0][0] * a2z;
                m_pEntry[0][0] = m_pEntry[0][0] * v51 - v42 * a2z;
                double v43 = m_pEntry[1][1];
                m_pEntry[1][1] = v43 * v51 + a2z * m_pEntry[1][0];
                m_pEntry[1][0] = v51 * m_pEntry[1][0] - v43 * a2z;
                float a1bc = m_pEntry[2][1];
                m_pEntry[2][1] = m_pEntry[2][0] * a2z + a1bc * v51;
                m_pEntry[2][0] = v51 * m_pEntry[2][0] - a2z * a1bc;
                *a1 = v57;
                a1[1] = v63;
                v39 = 0.0;
            }
            *a2 = v39;
            return 1;
        }
        float a1e = (a1[1] - *a1) / (*a2 + *a2);
        float a2e = v3 + a1e * a1e;
        float a2f = std::sqrt((double)a2e);
        float v58 = a2[1];
        double v8 = a1e;
        double v9 = a2f;
        if (!(a1e >= 0.0)) {
            v10 = v8 - v9;
        }
        else {
            v10 = v8 + v9;
        }
        float a2g = a1[2] - *a1;
        float a2h = *a2 / v10 + a2g;
        double v11 = v58;
        float a1f = std::fabs((double)v58);
        double v12 = a1f;
        float a1g = std::fabs((double)a2h);
        bool v13 = !(a1g >= v12);
        bool v14 = a1g == v12;
        double v15 = a2h;
        if (v13 || v14) {
            float a1h = v15 / v11;
            float a2i = a1h * a1h + 1.0;
            float a2j = std::sqrt((double)a2i);
            v16 = 1.0;
            float a2k = 1.0 / a2j;
            v17 = a2k;
            float v44 = a1h * a2k;
            v18 = v44;
        }
        else {
            float a1i = v11 / v15;
            float a2l = a1i * a1i + 1.0;
            float a2m = std::sqrt((double)a2l);
            v16 = 1.0;
            float v45 = 1.0 / a2m;
            float a2n = a1i * v45;
            v18 = v45;
            v17 = a2n;
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
        float v52 = (a1[1] - a1[2]) * v17 + (a2[1] + a2[1]) * v18;
        float v64 = *a2 * v18;
        float v59 = *a2 * v17;
        float a2a = v18 * v52 - a2[1];
        float v53 = v17 * v52;
        double v22 = v59;
        float a1j = std::fabs((double)v59);
        double v23 = a1j;
        float a1k = std::fabs((double)a2a);
        bool v24 = !(a1k >= v23);
        bool v25 = a1k == v23;
        double v26 = a2a;
        if (v24 || v25) {
            float a1l = v26 / v22;
            float a2o = v16 + a1l * a1l;
            float a2p = std::sqrt((double)a2o);
            a2[1] = v59 * a2p;
            v3 = 1.0;
            float a2q = 1.0 / a2p;
            v27 = a2q;
            float v46 = a1l * a2q;
            v28 = v46;
        }
        else {
            float a1m = v22 / v26;
            float v60 = v16 + a1m * a1m;
            float v61 = std::sqrt((double)v60);
            a2[1] = a2a * v61;
            v3 = 1.0;
            float v47 = 1.0 / v61;
            float a2r = a1m * v47;
            v28 = v47;
            v27 = a2r;
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
        float a1n = a1[1] - v53;
        a1[2] = v53 + a1[2];
        float v54 = (*a1 - a1n) * v27 + (v64 + v64) * v28;
        *a2 = v28 * v54 - v64;
        float v55 = v27 * v54;
        ++v6;
        a1[1] = a1n + v55;
        *a1 = *a1 - v55;
        if (v6 >= 32) {
            return 0;
        }
    }
    double v33 = 0.0;
    *a2 = 0.0;
    float a1o = std::fabs((double)a1[1]);
    double v34 = a1o;
    float a1p = std::fabs((double)a1[2]);
    float a1q = v34 + a1p;
    float a2s = std::fabs((double)a2[1]);
    if (a1q != a2s + a1q) {
        double v35;
        float v48;
        float a1r = a1[2] + a1[1];
        float a2t = a1[1] - a1[2];
        float v65 = a2t * a2t + a2[1] * 4.0 * a2[1];
        float v66 = std::sqrt((double)v65);
        float v56 = (a1r - v66) * 0.5;
        float v62 = (a1r + v66) * 0.5;
        if (!(a2t >= 0.0)) {
            v48 = a1[2] - v56;
            v35 = a2[1];
        }
        else {
            v48 = a2[1];
            v35 = a1[1] - v56;
        }
        float a2u = v35;
        float a1s = a2u * a2u + v48 * v48;
        float a1t = std::sqrt((double)a1s);
        float a1u = 1.0 / a1t;
        float v49 = a1u * v48;
        float a2v = a1u * a2u;
        double v36 = m_pEntry[0][2];
        m_pEntry[0][2] = v36 * v49 + m_pEntry[0][1] * a2v;
        m_pEntry[0][1] = m_pEntry[0][1] * v49 - v36 * a2v;
        double v37 = m_pEntry[1][2];
        m_pEntry[1][2] = m_pEntry[1][1] * a2v + v37 * v49;
        m_pEntry[1][1] = m_pEntry[1][1] * v49 - v37 * a2v;
        double v38 = m_pEntry[2][2];
        m_pEntry[2][2] = v38 * v49 + a2v * m_pEntry[2][1];
        m_pEntry[2][1] = v49 * m_pEntry[2][1] - a2v * v38;
        a1[1] = v56;
        a1[2] = v62;
        v33 = 0.0;
    }
    a2[1] = v33;
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

float* NiMatrix3::EigenSolveSymmetric(void* apThis, [[maybe_unused]] void* apEdx, float* a2, float* a3) {
    float v5[2];
    NiMatrix3 v6;
    __builtin_memcpy(&v6, apThis, sizeof(v6));
    int8_t v3 = v6.Tridiagonal(a2, v5);
    v6.QLAlgorithm(a2, v5);
    float* result = a3;
    *a3 = v6.m_pEntry[0][0];
    a3[1] = v6.m_pEntry[1][0];
    a3[2] = v6.m_pEntry[2][0];
    a3[3] = v6.m_pEntry[0][1];
    a3[4] = v6.m_pEntry[1][1];
    a3[5] = v6.m_pEntry[2][1];
    if (v3) {
        a3[6] = (-v6.m_pEntry[0][2]);
        a3[7] = (-v6.m_pEntry[1][2]);
        a3[8] = (-v6.m_pEntry[2][2]);
    }
    else {
        a3[6] = v6.m_pEntry[0][2];
        a3[7] = v6.m_pEntry[1][2];
        a3[8] = v6.m_pEntry[2][2];
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
