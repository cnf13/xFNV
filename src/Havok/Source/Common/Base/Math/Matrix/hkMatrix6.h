#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// A six-dimensional vector as two halves, the linear and the angular one (w unused in each): the powered chain solver's
// velocities and temporaries (hkpPoweredChainSolverUtil).
class hkVector8 {
public:
    hkVector4 m_lin;
    hkVector4 m_ang;
};

// A six by six matrix stored as four three by three blocks, m_m[rowBlock][columnBlock].
class hkMatrix6 {
public:
    void setZero() {
        m_m[0][0].setZero();
        m_m[0][1].setZero();
        m_m[1][0].setZero();
        m_m[1][1].setZero();
    }

    void setIdentity() {
        m_m[0][0].setIdentity();
        m_m[0][1].setZero();
        m_m[1][0].setZero();
        m_m[1][1].setIdentity();
    }

    // Sets this to the inverse of in, block by block.
    void setInvert(const hkMatrix6& tmpIn);

    hkMatrix3 m_m[2][2];
};

// Sets out to in's inverse; a wrapper around hkMatrix6::setInvert.
void hkMatrix6SetInvert(hkMatrix6& out, const hkMatrix6& in);
// out = m * v, block row by block row. Its callers in the powered chain solver pass two hkVector8 and a hkMatrix6 of a
// hkpConstraintChainMatrix6Triple.
const hkMatrix6* hkMatrix6SetMulV(hkVector8& out, const hkMatrix6& a, const hkVector8& b);
// Each of out's four blocks is the transpose of in's mirrored block.
void hkMatrix6SetTranspose(hkMatrix6& out, const hkMatrix6& in);
__m128* addMatrix6(__m128* out, __m128* a);
