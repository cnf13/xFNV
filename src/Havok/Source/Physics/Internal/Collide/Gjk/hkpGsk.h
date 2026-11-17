#pragma once

#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkVector4;

// Not named in the PDB or IDA; name chosen here (no symbol in hkpInternal.lib: a file-static helper). The separating
// normal and closest point on A of the simplex of dimA and dimB vertices (cdecl; callers hk4dGskCollideCalcToi
// 0xDB60C0 and 0xDB54B0 pass the hkpGsk's m_verticesA and m_verticesBinA).
void hkGskRecalcContactInternal(const hkVector4* verticesA, const hkVector4* verticesB, int32_t dimA, int32_t dimB,
	const hkVector4* hint, hkVector4* pointOut, hkVector4* normalOut);
// Not named in the PDB or IDA; name chosen here. hkGskRecalcContactInternal's point-triangle normal: triangle a1, normal out a2,
// distance out a3 (its x), point a4, hint r_esi; returns a3 (usercall: ecx, edx, two stack arguments, esi, ret 0).
// No exe callers.
hkVector4* calcTriangleNormalAndDistance(const hkVector4* a1, hkVector4* a2, hkVector4* a3, const hkVector4* a4,
	const hkVector4* r_esi);
// Not named in the PDB or IDA; name chosen here. hkGskRecalcContactInternal's edge-edge normal: verticesA a1, hint a2, pointOut a3,
// verticesB r_edi, normalOut r_esi (usercall: three stack arguments, edi, esi, ret 0). No exe callers.
void calcEdgeEdgeNormalAndDistance(const hkVector4* a1, const hkVector4* a2, hkVector4* a3, const hkVector4* r_edi,
	hkVector4* r_esi);
// A file-static helper of hkpGsk.cpp (no symbol in hkpInternal.lib):
// the tetrahedron of the four vertices at r_eax against the point r_esi, the side sign fSign on the stack.
int32_t findClosestTriangle(float fSign, hkVector4* r_eax, hkVector4* r_esi);
// Not named in the PDB or IDA; name chosen here. The segment simplex a1 against the point a3: the separating
// direction to r_edi, *a2 decremented when the simplex drops to a vertex; r_esi == 1 flips the sign (usercall: ecx,
// edx, stack, edi, esi, ret 0). No exe callers.
int32_t calcEdgeSearchDirection(hkVector4* a1, int32_t* a2, const hkVector4* a3, hkVector4* r_edi, int32_t r_esi);

// Guessed bound: the database gives none; the next data item starts 0x8 bytes on.
using rdata_10CB438_t = char[8];
inline rdata_10CB438_t rdata_10CB438{-1, 0, 1, -6, 2, -7, -8, -1};
