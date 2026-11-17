#pragma once

#include "Reconstruction.hpp"

#include <emmintrin.h>
#include <stdint.h>

// The four 32-bit integers an integer vector is stored in. The exe was built from the SDK's SSE form
// (hkIntVector.h's HK_COMPILER_HAS_INTRINSICS_IA32 arm): the kd-tree code works on it in xmm registers (pxor, movdqa
// in 0xD99570). The SDK's FPU form is a 16-aligned struct of four hkUint32.
typedef __m128i hkQuadUint;

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

// A vector of four 32-bit integers (or eight 16-bit, sixteen 8-bit ones), worked on with the SSE2 integer
// instructions; its members are SDK inlines (hkSseIntVector.inl) the exe inlines.
class hkIntVector {
public:
	hkQuadUint m_quad;
};
