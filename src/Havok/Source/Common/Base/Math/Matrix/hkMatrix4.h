#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkTransform;

class hkMatrix4 {
public:
    // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
    HK_DECLARE_POD_TYPE();
    // Inverts the matrix in place; HK_FAILURE when the determinant is within epsilon of zero.
    hkResult invert(float epsilon);
    // Sets this to the inverse of m; HK_FAILURE when the determinant is within epsilon of zero.
    hkResult setInverse(const hkMatrix4& m, float epsilon);
    // Transposes the matrix in place.
    void transpose();
    // Sets this to a * b.
    void setMul(const hkMatrix4& a, const hkMatrix4& b);
    // Sets this to a * inverse(b).
    void setMulInverse(const hkMatrix4& a, const hkMatrix4& b);
    // Sets each column of this to the matching column of a times scale.
    void setMul(const hkSimdReal& scale, const hkMatrix4& a);
    // Multiplies each column by scale.
    void mul(const hkSimdReal& scale);

    void setZero() {
        m_col0.setZero4();
        m_col1.setZero4();
        m_col2.setZero4();
        m_col3.setZero4();
    }

    void setIdentity() {
        setZero();
        float one = 1.0f;
        m_col0(0) = one;
        m_col1(1) = one;
        m_col2(2) = one;
        m_col3(3) = one;
    }

    static const hkMatrix4& getIdentity();

    // positionIn's x, y and z through the columns, plus the fourth column.
    void transformPosition(const hkVector4& positionIn, hkVector4& positionOut) const {
        hkVector4 xb;
        xb.setBroadcast(positionIn, 0);
        hkVector4 yb;
        yb.setBroadcast(positionIn, 1);
        hkVector4 zb;
        zb.setBroadcast(positionIn, 2);
        hkVector4 t0;
        t0.setMul4(xb, m_col0);
        t0.addMul4(yb, m_col1);
        t0.addMul4(zb, m_col2);
        t0.add4(m_col3);
        positionOut = t0;
    }

    void operator=(const hkMatrix4& a) {
        m_col0 = a.m_col0;
        m_col1 = a.m_col1;
        m_col2 = a.m_col2;
        m_col3 = a.m_col3;
    }

    hkBool isOk();
    void setTranspose(const hkMatrix4& s);
    void add(const hkMatrix4& a);
    void sub(const hkMatrix4& a);
    void get4x4RowMajor(float* d);
    void set4x4RowMajor(const float* p);
    // Copies the 16 floats as they are.
    void get4x4ColumnMajor(float* d) const;
    // Copies the 16 column-major floats at p into the columns.
    void set4x4ColumnMajor(const float* p);
    // Sets the fourth row to 0, 0, 0, 1.
    void resetFourthRow();
    void set(const hkTransform& t);
    void get(hkTransform& t);
    void setMulInverseAffine(const hkMatrix4& a, const hkMatrix4& b);
    void mul(hkMatrix4* b);

    static int32_t isAffineTransformation(uint32_t* matrix);

    hkVector4 m_col0;
    hkVector4 m_col1;
    hkVector4 m_col2;
    hkVector4 m_col3;
};

// The identity matrix hkMatrix4::getIdentity returns.
extern hkMatrix4 hkMatrix4Identity;
inline const hkMatrix4& hkMatrix4::getIdentity() {
    return hkMatrix4Identity;
}

// The identity matrix's sixteen floats, which hkMatrix4Identity's initialiser copies.
using hkMatrix4Identity_storage_t = float[16];
inline hkMatrix4Identity_storage_t hkMatrix4Identity_storage{
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
