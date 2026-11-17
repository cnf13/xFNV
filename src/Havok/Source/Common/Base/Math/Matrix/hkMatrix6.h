#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

// A six-dimensional vector as two halves, the linear and the angular one (w unused in each): the powered chain
// solver's velocities and temporaries (hkpPoweredChainSolverUtil; the game PDB's hkPadSpu<hkVector8*>).
class hkVector8 {
public:
	hkVector4 m_lin;
	hkVector4 m_ang;
};

// A six by six matrix stored as four three by three blocks, m_m[rowBlock][columnBlock]. The exe's hkMatrix6::setInvert
// (0xE29620) zeroes the twelve columns (0xC0 bytes) and writes 1.0 to 0x00, 0x14 and 0x28 (m_m[0][0]'s diagonal) and
// to 0x90, 0xA4 and 0xB8 (m_m[1][1]'s diagonal), so m_m[1][1] is the block at 0x90.
class hkMatrix6 {
public:
	// Sets this to the inverse of in, block by block.
	void setInvert(const hkMatrix6& tmpIn);

	hkMatrix3 m_m[2][2];
};

// The SDK's free function: out = in's inverse, which the exe compiles as the wrapper around
// hkMatrix6::setInvert (cdecl, ret 0, 0xE298F0).
void hkMatrix6SetInvert(hkMatrix6& out, const hkMatrix6& in);
// The SDK's hkMatrix6SetMulV(hkVector8& out, const hkMatrix6& a, const hkVector8& b) (the PDB's hkMatrix6.cpp):
// out = m * v, block row by block row (cdecl, 0xE29510). Its callers, the powered chain solver's 0xDF11A0, 0xDF1260
// and 0xDF1430, pass two hkVector8 and a hkMatrix6 of a hkpConstraintChainMatrix6Triple.
const hkMatrix6* hkMatrix6SetMulV(hkVector8& out, const hkMatrix6& a, const hkVector8& b);
// The SDK's hkMatrix6SetTranspose(hkMatrix6& out, const hkMatrix6& in) (hkMatrix6.h): each
// of out's four blocks is the transpose of in's mirrored block (the exe swaps blocks 0x30 and 0x60).
void hkMatrix6SetTranspose(hkMatrix6& out, const hkMatrix6& in);
__m128* addMatrix6(__m128* a1, __m128* a2);
