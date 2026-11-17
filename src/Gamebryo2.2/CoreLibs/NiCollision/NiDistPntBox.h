#pragma once

#include "Reconstruction.hpp"

class NiBox;

// Not named in the PDB or IDA; name chosen here. The box clamp of NiSqrDistance::Compute_5 (0xEB50F0): the point
// in ecx, the box in edx (the body reads its m_afExtent, +0x30), the squared distance in esi.
void Case000(float* a1, const NiBox* apBox, float* apfSqrDistance);
// a1 is the box: NiSqrDistance::Compute_5
// passes its const NiBox's address, and the body reads its m_afExtent (+0x30) and hands it on to
// Face.
float* CaseNoZeros(const NiBox* a1, float* a2, float* a3, float* a4, float* r_eax);
// Not named in the PDB or IDA; name chosen here. The line-box distance case where the line direction is zero
// along one axis (ai2) only: ai0 in edx, ai1 in ecx, the point (apPnt) in edi, the direction (apDir) in esi.
void Case0(int32_t ai1, int32_t ai0, const NiBox* apBox, int32_t ai2, float* apfLParam, float* apfSqrDistance,
	float* apPnt, float* apDir);
// a2 is the box (CaseNoZeros's a1): the body
// reads its m_afExtent[i] (+0x30 + 4 * i).
float* Face(int32_t a1, const NiBox* a2, int32_t a3, float* a4, float* a5, float* a6, float* r_ebx, float* r_edi,
	int32_t r_esi);
void CaseTwoZeros(float* afPnt, const NiBox* pkBox, int32_t i0, const float* afDir, float* pfLParam,
	float* pfSqrDistance, int32_t i2, int32_t i1);
