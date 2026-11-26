#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A 3x3 rotation (or general linear) matrix, row-major: m_pEntry[row][column].
class NiMatrix3 {
public:
    // Leaves the entries undefined, like Gamebryo's empty `NiMatrix3() {}`. Defaulted so that a zero global (`NiMatrix3
    // x{}`) is constant data.
    NiMatrix3() = default;
    // from three columns
    NiMatrix3(const NiPoint3& col0, const NiPoint3& col1, const NiPoint3& col2);

    // From the nine entries, row by row. Not Gamebryo's: constexpr so that a global built from constants (IDENTITY) is
    // constant data.
    constexpr NiMatrix3(
        float m00,
        float m01,
        float m02,
        float m10,
        float m11,
        float m12,
        float m20,
        float m21,
        float m22
    )
      : m_pEntry{{m00, m01, m02}, {m10, m11, m12}, {m20, m21, m22}} {}

    // Sets the matrix to the rotation of the Euler angles fZAngle, fXAngle, fYAngle applied in Z, X, Y order.
    void FromEulerAnglesZXY(float fZAngle, float fXAngle, float fYAngle);
    // Sets the matrix to the rotation of the Euler angles fZAngle, fYAngle, fXAngle applied in Z, Y, X order.
    void FromEulerAnglesZYX(float fZAngle, float fYAngle, float fXAngle);

    void GetRow(uint32_t uiRow, NiPoint3& row) const;
    void GetRow(uint32_t uiRow, float& f0, float& f1, float& f2) const;
    void SetRow(uint32_t uiRow, const NiPoint3& row);
    void SetRow(uint32_t uiRow, float f0, float f1, float f2);
    void SetRow(uint32_t uiRow, const float* row);
    void GetCol(uint32_t uiCol, NiPoint3& col) const;
    void SetCol(uint32_t uiCol, const NiPoint3& col);
    void SetCol(uint32_t uiCol, float f0, float f1, float f2);

    NiMatrix3 Transpose() const;
    NiMatrix3 operator*(const NiMatrix3& mat) const;
    NiPoint3 operator*(const NiPoint3& pt) const;
    // Every entry times fScalar.
    NiMatrix3 operator*(float fScalar) const;
    NiMatrix3 operator+(const NiMatrix3& mat) const;
    NiMatrix3 operator-(const NiMatrix3& mat) const;
    // Transpose() * mat.
    NiMatrix3 TransposeTimes(const NiMatrix3& mat) const;
    // Writes the inverse to inv and returns whether it exists.
    bool Inverse(NiMatrix3& inv) const;
    // Writes the inverse, or the zero matrix when there is none, to apResult and returns apResult.
    NiMatrix3* Inverse(NiMatrix3* apResult) const;
    // The X, Y and Z Euler angles of the rotation; false when the decomposition is not unique.
    bool ToEulerAnglesXYZ(float& rfXAngle, float& rfYAngle, float& rfZAngle) const;
    // The Z, X and Y Euler angles of the rotation; false when the decomposition is not unique.
    bool ToEulerAnglesZXY(float& rfZAngle, float& rfXAngle, float& rfYAngle) const;
    // Makes the rotation of the X, Y and Z Euler angles.
    void FromEulerAnglesXYZ(float fXAngle, float fYAngle, float fZAngle);
    void MakeXRotation(float fAngle);
    void MakeYRotation(float fAngle);
    void MakeZRotation(float fAngle);
    // The rotation by fAngle about Z.
    void MakeZRotationQ(float fAngle);
    // Rotation by fAngle about the axis (x, y, z).
    void MakeRotation(float fAngle, float x, float y, float z);
    // The rotation's angle (NiACos of the trace) and unit axis; near-zero and half-turn angles handled apart.
    void ExtractAngleAndAxis(float& fAngle, float& fX, float& fY, float& fZ) const;

    // Exact entry compare (unordered, i.e. NaN, is unequal).
    bool operator==(const NiMatrix3& mat) const;

    // Reads the nine entries row by row, one 4-byte transfer each.
    void LoadBinary(NiStream& stream);
    // Snap() to zero the near-zero entries, then writes them like LoadBinary reads them.
    void SaveBinary(NiStream& stream);

    // "<prefix> = ((..),(..),(..))" row by row with "%g", in a fresh _NiMalloc block the caller owns.
    char* GetViewerString(const char* pPrefix) const;
    void MakeZero();
    bool operator!=(const NiMatrix3& mat) const;
    float GetEntry(uint32_t uiRow, uint32_t uiCol) const;
    void MakeIdentity();
    void SetEntry(uint32_t uiRow, uint32_t uiCol, float fEntry);
    void Snap();
    char Tridiagonal(float* afDiag, float* afSubd);
    char QLAlgorithm(float* afDiag, float* afSubd);

    // pOutVertex[i] = rot * pInVertex[i] + trn for uiVertexCount vertices.
    static void TransformVertices(
        const NiMatrix3& rot,
        const NiPoint3& trn,
        uint32_t uiVertexCount,
        const NiPoint3* pInVertex,
        NiPoint3* pOutVertex
    );
    static float*
    EigenSolveSymmetric(void* apThis, [[maybe_unused]] void* apEdx, float* afEigenvalue, float* akEigenvector);

    float m_pEntry[3][3];

    static NiMatrix3 ZERO;

    static NiMatrix3 IDENTITY;
};
inline NiMatrix3 NiMatrix3::ZERO{};

inline NiMatrix3 NiMatrix3::IDENTITY{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

// Row vector times matrix: the point's dot product with each column, z computed first.
inline NiPoint3 operator*(const NiPoint3& pt, const NiMatrix3& mat) {
    const float fZ = pt.x * mat.m_pEntry[0][2] + pt.y * mat.m_pEntry[1][2] + pt.z * mat.m_pEntry[2][2];
    const float fY = pt.x * mat.m_pEntry[0][1] + pt.y * mat.m_pEntry[1][1] + pt.z * mat.m_pEntry[2][1];
    const float fX = pt.x * mat.m_pEntry[0][0] + pt.y * mat.m_pEntry[1][0] + pt.z * mat.m_pEntry[2][0];
    return NiPoint3(fX, fY, fZ);
}

inline NiMatrix3* NiMatrix3::Inverse(NiMatrix3* apResult) const {
    NiMatrix3 kInverse;
    if (!Inverse(kInverse)) {
        kInverse.MakeZero();
    }
    *apResult = kInverse;
    return apResult;
}

inline void NiMatrix3::MakeZRotationQ(float fAngle) {
    float cs;
    float sn;
    NiSinCosQ(fAngle, &sn, &cs);
    m_pEntry[0][0] = cs;
    m_pEntry[0][1] = sn;
    m_pEntry[0][2] = 0.0;
    m_pEntry[1][0] = -sn;
    m_pEntry[1][1] = cs;
    m_pEntry[1][2] = 0.0;
    m_pEntry[2][0] = 0.0;
    m_pEntry[2][1] = 0.0;
    m_pEntry[2][2] = 1.0;
}
