#pragma once

#include <xmmintrin.h>

class hkVector4;

// hkVector4.h's hkQuadReal (the SDK's SSE form); hkVector4.h includes this header.
typedef __m128 hkQuadReal;

class alignas(16) hkSimdReal {
public:
    // The default constructor stores nothing. The constructor from a float puts x in the first lane and clears the
    // other three.
    hkSimdReal() = default;
    // x in the first lane, the other three cleared.
    hkSimdReal(float x) : m_real(_mm_set_ss(x)) {}
    hkSimdReal(const hkQuadReal& x) : m_real(x) {}

    // The real in all four lanes of a vector. Defined in hkVector4.h, where hkVector4 is complete.
    hkVector4 broadcast() const;
    // The four lanes.
    const hkQuadReal& getQuad() const {
        return m_real;
    }

    operator float() const {
        float s;
        _mm_store_ss(&s, m_real);
        return s;
    }

    // The real, in the quad's first lane (Construct clears the other three); operator float reads its first lane.
    hkQuadReal m_real;
};

// Returns r with its first lane multiplied by s's.
hkSimdReal operator*(const hkSimdReal& r, const hkSimdReal& s);
// Returns r with its first lane minus s's (_mm_sub_ss); both operands are read as 16-byte quads.
hkSimdReal operator-(const hkSimdReal& r, const hkSimdReal& s);
// Returns r with its first lane plus s's first lane.
hkSimdReal operator+(const hkSimdReal& r, const hkSimdReal& s);
