#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

#include <cstdint>
#include <cstring>

// Uniform clamped B-spline basis of degree DEGREE over m_iQuantity control points, evaluated at a time in [0, 1].
// The values of the last evaluated time are cached.
template <class REAL, int32_t DEGREE>
class NiBSplineBasis : public NiMemObject {
public:
    // No control points yet. The values stay unset: no valid time equals -1, so the first Compute fills them.
    NiBSplineBasis() : m_iQuantity(0), m_fLastTime(-1.0f), m_iMin(0), m_iMax(DEGREE) {}

    int32_t GetQuantity() const {
        return m_iQuantity;
    }
    REAL GetValue(uint32_t i) const {
        return m_afValue[i];
    }
    bool IsEqual(const NiBSplineBasis* pkOther) const {
        return m_iQuantity == pkOther->m_iQuantity;
    }

    // Fills m_afValue with the basis functions non-zero at fTime, for control points iMin..iMax.
    void Compute(REAL fTime, int32_t& iMin, int32_t& iMax) const;

    int32_t m_iQuantity;
    mutable REAL m_afValue[DEGREE + 1];
    mutable REAL m_fLastTime;
    mutable int32_t m_iMin;
    mutable int32_t m_iMax;
};

using NiBSplineBasisFloat3 = NiBSplineBasis<float, 3>;

// General evaluation (Cox-de Boor on the clamped uniform knots). A time whose bits equal the last one's reuses the
// cached values; the test compares bits, not floating-point values.
template <class REAL, int32_t DEGREE>
void NiBSplineBasis<REAL, DEGREE>::Compute(REAL fTime, int32_t& iMin, int32_t& iMax) const {
    if (std::memcmp(&fTime, &m_fLastTime, sizeof(REAL)) == 0) {
        iMin = m_iMin;
        iMax = m_iMax;
        return;
    }
    std::memcpy(&m_fLastTime, &fTime, sizeof(REAL));

    REAL fQmD = static_cast<REAL>(m_iQuantity - DEGREE);
    REAL fT;
    if (fTime <= static_cast<REAL>(0.0)) {
        fT = static_cast<REAL>(0.0);
        iMin = 0;
        iMax = DEGREE;
    }
    else if (fTime >= static_cast<REAL>(1.0)) {
        fT = fQmD;
        iMax = m_iQuantity - 1;
        iMin = iMax - DEGREE;
    }
    else {
        fT = fQmD * fTime;
        iMin = static_cast<int32_t>(fT);
        iMax = iMin + DEGREE;
    }
    m_iMin = iMin;
    m_iMax = iMax;

    // The knots of the span, clamped to [0, m_iQuantity - DEGREE].
    REAL afKnot[2 * DEGREE];
    for (int32_t i0 = 0, i1 = iMax + 1 - DEGREE; i0 < 2 * DEGREE; i0++, i1++) {
        if (i1 <= DEGREE) {
            afKnot[i0] = static_cast<REAL>(0.0);
        }
        else if (i1 >= m_iQuantity) {
            afKnot[i0] = fQmD;
        }
        else {
            afKnot[i0] = static_cast<REAL>(i1 - DEGREE);
        }
    }

    // The triangular recursion, from degree 0 up.
    m_afValue[DEGREE] = static_cast<REAL>(1.0);
    for (int32_t iRow = DEGREE - 1; iRow >= 0; iRow--) {
        int32_t iK0 = DEGREE;
        int32_t iK1 = iRow;
        REAL fKnot0 = afKnot[iK0];
        REAL fKnot1 = afKnot[iK1];
        REAL fInvDenom = static_cast<REAL>(1.0) / (fKnot0 - fKnot1);
        REAL fC1 = (fKnot0 - fT) * fInvDenom;
        REAL fC0;
        m_afValue[iRow] = fC1 * m_afValue[iRow + 1];
        for (int32_t iCol = iRow + 1; iCol < DEGREE; iCol++) {
            fC0 = (fT - fKnot1) * fInvDenom;
            m_afValue[iCol] *= fC0;
            fKnot0 = afKnot[++iK0];
            fKnot1 = afKnot[++iK1];
            fInvDenom = static_cast<REAL>(1.0) / (fKnot0 - fKnot1);
            fC1 = (fKnot0 - fT) * fInvDenom;
            m_afValue[iCol] += fC1 * m_afValue[iCol + 1];
        }
        fC0 = (fT - fKnot1) * fInvDenom;
        m_afValue[DEGREE] *= fC0;
    }
}

// Specialized as Gamebryo's unrolled degree-3 form, whose float operations differ in order and rounding from the
// general recursion (defined in NiBSplineData.cpp).
template <>
void NiBSplineBasis<float, 3>::Compute(float fTime, int32_t& riMin, int32_t& riMax) const;
