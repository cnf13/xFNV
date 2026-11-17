#pragma once

#include <emmintrin.h>
#include <stdint.h>

// The four 32-bit integers an integer vector is stored in.
using hkQuadUint = __m128i;

// An integer vector's lanes as four unsigned integers.
union hkQuadUintUnion {
    uint32_t u[4];
    hkQuadUint q;
};

// An integer vector's lanes as four signed integers.
union hkQuadIntUnion {
    int32_t u[4];
    hkQuadUint q;
};

// An integer vector's lanes as eight 16-bit integers.
union hkQuadShortUnion {
    uint16_t u[8];
    hkQuadUint q;
};

// An integer vector's lanes as sixteen bytes.
union hkQuadCharUnion {
    uint8_t u[16];
    hkQuadUint q;
};

// A vector of four 32-bit integers (or eight 16-bit, sixteen 8-bit ones).
class hkIntVector {
public:
    hkQuadUint m_quad;
};
