#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// Fetches s's quad (and copies it) before r's, multiplies the first lanes into a copy of r's quad, then builds the
// result from that quad.
hkSimdReal operator*(const hkSimdReal& r, const hkSimdReal& s) {
    const hkQuadReal& kS = s.getQuad();
    hkQuadReal kQuadS = kS;
    const hkQuadReal& kR = r.getQuad();
    hkQuadReal kProduct = kR;
    kProduct[0] = kR[0] * kQuadS[0];
    hkSimdReal kResult;
    kResult.~hkSimdReal();
    ::new (&kResult) hkSimdReal(kProduct);
    return kResult;
}

hkSimdReal operator-(const hkSimdReal& r, const hkSimdReal& s) {
    hkSimdReal result;
    __m128 m_real = r.m_real;
    __m128 v4 = s.m_real;
    m_real[0] = r.m_real[0] - s.m_real[0];
    result.m_real = m_real;
    return result;
}

hkSimdReal operator+(const hkSimdReal& r, const hkSimdReal& s) {
    hkSimdReal result;
    __m128 m_real = r.m_real;
    __m128 v4 = s.m_real;
    m_real[0] = r.m_real[0] + s.m_real[0];
    result.m_real = m_real;
    return result;
}
