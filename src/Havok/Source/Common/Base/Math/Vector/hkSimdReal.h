#pragma once

#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkVector4;

// hkVector4.h's hkQuadReal (the SDK's SSE form); hkVector4.h includes this header.
typedef __m128 hkQuadReal;

// 16-aligned like the PDB's __vector4 m_real: the exe's callees store a result into it with movaps, and callers such
// as HK2NI (0x458620) realign their frame so the hidden result buffers they pass are 16-aligned.
class alignas(16) hkSimdReal {
public:
	// The two constructors are the SDK's inlines (hkSseMathTypes.inl) and are defined here, so that the exe's
	// out-of-line copies are transparent functions: the exe inlines them in most places and calls a copy in others.
	//
	// The constructor from a float: x in the first lane, the other three cleared (thiscall, ret 4, 0x5DBF20,
	// registered in TES.cpp). The exe's copy has a stack-realigned frame with a security cookie, left out here: it
	// only holds the 16-byte copy of the movss result, which then goes to the object whole (x, 0, 0, 0) with one
	// movaps. The four lanes are written one by one, through the quad's float lanes, so that the body needs no SSE.
	// Stores nothing, as the implicit constructor the class had without the two below.
	hkSimdReal() = default;
	// x in the first lane, the other three cleared.
	hkSimdReal(float x) : m_real(_mm_set_ss(x)) {}
	hkSimdReal(const hkQuadReal& x) : m_real(x) {}

	// IDA name; not in the PDB. The real in all four lanes of a vector (the exe's out-of-line copy: thiscall, ret 4,
	// the result pointer first, 0x561240). PDB: hkQuadReal broadcast() const; the exe returns a 16-byte class through
	// a hidden pointer, so the result is an hkVector4. Defined in hkVector4.h, where hkVector4 is complete.
	hkVector4 broadcast() const;
	// The four lanes. The exe's out-of-line copy is its shared one-field getter (thiscall, ret 0, 0x6815C0, which
	// returns this). PDB: const __vector4& getQuad() const, beside a non-const overload; __vector4 is the Xbox 360
	// vector, the PC exe's hkQuadReal (__m128).
	const hkQuadReal& getQuad() const { return m_real; }

	// The real as a float, returned in ST0 (the exe's out-of-line copy: thiscall, ret 0, 0x4586D0).
	operator float() const {
		float s;
		_mm_store_ss(&s, m_real);
		return s;
	}

	// The real, in the quad's first lane (Construct, 0x5DBF20, clears the other three). The PDB's __vector4 m_real
	// is the SSE vector hkQuadReal; operator float (0x4586D0) reads its first lane.
	hkQuadReal m_real;
};

// The SDK's inline operator* of two reals (hkSseMathTypes.inl), out of line in the exe (IDA's _sub_891060; cdecl, the
// result through a hidden pointer): r with its first lane multiplied by s's (_mm_mul_ss).
hkSimdReal operator*(const hkSimdReal& r, const hkSimdReal& s);
// The SDK's inline operator- of two reals (hkSseMathTypes.inl), out of line in the exe (IDA's _sub_C6CCC0; cdecl, the
// result through a hidden pointer): r with its first lane minus s's (_mm_sub_ss).
// Inferred type: hkSseMathTypes.inl:71/94 types both operands hkSimdRealParameter = const hkSimdReal&, and the body
// (0xC6CCC0) reads each as a 16-byte quad; the result is the hidden first pointer (the caller hkSolveConstraints
// 0xD8F3E6-0xD8F419 pushes it first).
hkSimdReal operator-(const hkSimdReal& r, const hkSimdReal& s);
// The SDK's inline operator+ of two reals (hkSseMathTypes.inl), out of line in the exe (IDA's _sub_C6CCE0; cdecl, the
// result through a hidden pointer): r with its first lane plus s's (_mm_add_ss).
// Inferred type: hkSseMathTypes.inl:71/99 types both operands hkSimdRealParameter = const hkSimdReal&, and the body
// (0xC6CCE0) reads each as a 16-byte quad; the result is the hidden first pointer (the caller hkSolveConstraints
// 0xD8F4D4-0xD8F4E5 pushes it first).
hkSimdReal operator+(const hkSimdReal& r, const hkSimdReal& s);
