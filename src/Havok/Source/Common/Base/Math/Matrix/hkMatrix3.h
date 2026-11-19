#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <xmmintrin.h>

class hkRotation;

// The Jacobi rotation diagonalizeSymmetric applies: its cosine and sine and the element (p, q) it zeroes.
struct SchurMatrix {
    float c;
    float s;
    int32_t p;
    int32_t q;
};

// A three by three matrix stored as three sixteen byte columns.
class hkMatrix3 {
public:
    // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
    HK_DECLARE_POD_TYPE();

    void operator=(const hkMatrix3& a) {
        m_col0 = a.getColumn(0);
        m_col1 = a.getColumn(1);
        m_col2 = a.getColumn(2);
    }

    // Sets this to the product of arA and arB.
    void setMul(const hkMatrix3& arA, const hkMatrix3& arB);
    // this = this * temp, through setMul into a local.
    void mul(const hkMatrix3& temp);
    // Scales every element by s's first lane. The SDK's name.
    void mul(const hkSimdReal& s);
    // Called by changeBasis. Writes this from aTb's columns and the transpose of cTb; reads aTb and cTb only.
    // this = a times the inverse (transpose) of the rotation b.
    void setMulInverse(const hkMatrix3& aTb, const hkMatrix3& cTb);
    // Validity check of the matrix.
    hkBool isOk() const;
    // Whether every element is within epsilon of m's.
    hkBool isApproximatelyEqual(const hkMatrix3& m, float zero) const;
    // Whether the matrix equals its transpose within epsilon.
    bool isSymmetric(float epsilon) const;
    // Computes col0 . (col1 x col2), in lane 0 of the returned hkSimdReal.
    hkSimdReal getDeterminant() const;
    // this += arA, column by column.
    void add(const hkMatrix3& a);
    // this = r * this * r^T.
    void changeBasis(const hkRotation& r);
    // Transposes the matrix in place, swapping the three pairs of off-diagonal elements.
    void transpose();
    // this = the transpose of s.
    void setTranspose(const hkMatrix3& s);
    // The matrix of the cross product with r: this * v == r x v.
    void setCrossSkewSymmetric(const hkVector4& r);
    // this -= arA, column by column.
    void sub(const hkMatrix3& a);
    // Inverts the matrix in place; hkResult 0, or 1 when the determinant is within epsilon
    // of zero.
    hkResult invert(float epsilon);
    // Havok SDK: the eigenvectors (columns of eigenVec) and eigenvalues (x, y, z of eigenVal, w zero) of the
    // symmetric matrix by Jacobi rotations, at most maxIter of them; hkResult 0, or 1 when the matrix is not
    // symmetric within epsilon or the off-diagonal part did not fall below epsilon times the matrix's norm.
    hkResult diagonalizeSymmetric(hkMatrix3& eigenVec, hkVector4& eigenVal, int32_t maxIter, float epsilon) const;
    // The norm of the off-diagonal part.
    double _offDiagNorm() const;
    // The SDK's inline column accessor: the three columns are contiguous (pinned at 0x0, 0x10 and 0x20).
    const hkVector4& getColumn(int32_t i) const {
        return (&m_col0)[i];
    }
    hkVector4& getColumn(int32_t i) {
        return (&m_col0)[i];
    }
    // Element access: component r of column c (getColumn(c)(r)).
    float& operator()(int32_t r, int32_t c) {
        return (&m_col0)[c](r);
    }
    // Havok SDK signature (hkBase.lib, tied by reloc). this = the transpose of a times b.
    void setMulInverseMul(const hkRotation& bTa, const hkMatrix3& bTc);
    __m128* setMulScalar(__m128* a1, __m128* a2);
    __m128* addMul(__m128* a1, __m128* a2);
    void invertSymmetric();

    // Sets every column to zero.
    void setZero() {
        hkVector4 zero;
        zero.setZero4();
        (&m_col0)[0] = zero;
        (&m_col0)[1] = zero;
        (&m_col0)[2] = zero;
    }

    // The SDK's inline body: zero, then ones on the diagonal.
    void setIdentity() {
        setZero();
        float one = 1.0f;
        (*this)(0, 0) = one;
        (*this)(1, 1) = one;
        (*this)(2, 2) = one;
    }

    // The SDK's inline body: zero, then m00, m11 and m22 on the diagonal.
    void setDiagonal(float m00, float m11, float m22) {
        setZero();
        (*this)(0, 0) = m00;
        (*this)(1, 1) = m11;
        (*this)(2, 2) = m22;
    }

    // The SDK's inline body: the matrix whose rows are the x, y and z of r0, r1 and r2.
    void setRows(const hkVector4& r0, const hkVector4& r1, const hkVector4& r2) {
        hkVector4 c0;
        c0.set(r0(0), r1(0), r2(0), 0.0f);
        hkVector4 c1;
        c1.set(r0(1), r1(1), r2(1), 0.0f);
        hkVector4 c2;
        c2.set(r0(2), r1(2), r2(2), 0.0f);
        m_col0 = c0;
        m_col1 = c1;
        m_col2 = c2;
    }

    // The transform of an unrotated, unmoved object.
    static const hkMatrix3& getIdentity();
    // The Jacobi helpers that diagonalizeSymmetric calls. _frobeniusNorm returns the norm of the whole matrix.
    static double _frobeniusNorm(const hkMatrix3* apM);
    // Writes the row and column of the largest off-diagonal element of apM into *apP and *apQ.
    static void _findLargestOffDiagEntry(const hkMatrix3* apM, int32_t* apP, int32_t* apQ);
    // The Jacobi rotation zeroing element (aiP, aiQ) of apM, into apRotOut.
    static void _constructSchurMatrix(const hkMatrix3* apM, int32_t aiP, int32_t aiQ, SchurMatrix* apRotOut);
    // Builds the rotation matrix apOut of the Jacobi rotation at apRot.
    static void setJacobiRotation(const SchurMatrix* apRot, hkMatrix3* apOut);

    hkVector4 m_col0;
    hkVector4 m_col1;
    hkVector4 m_col2;
};

// v's x, y and z, each broadcast, scale the columns: (y * col1 + x * col0) + z * col2.
inline void hkVector4::_setMul3(const hkMatrix3& r, const hkVector4& v) {
    const hkQuadReal b1 = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(1, 1, 1, 1));
    const hkQuadReal r1 = _mm_mul_ps(b1, r.getColumn(1).m_quad);
    const hkQuadReal b0 = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(0, 0, 0, 0));
    const hkQuadReal r0 = _mm_mul_ps(b0, r.getColumn(0).m_quad);
    const hkQuadReal b2 = _mm_shuffle_ps(v.m_quad, v.m_quad, _MM_SHUFFLE(2, 2, 2, 2));
    const hkQuadReal r2 = _mm_mul_ps(b2, r.getColumn(2).m_quad);
    m_quad = _mm_add_ps(_mm_add_ps(r1, r0), r2);
}

// Writes the product apA * apB into apResult.
void hkMatrix3_setMulMat3Mat3(hkMatrix3* apResult, const hkMatrix3* apA, const hkMatrix3* apB);
// A wrapper that calls hkMatrix3::invertSymmetric on apM.
void hkMatrix3_invertSymmetric(hkMatrix3* apM);
