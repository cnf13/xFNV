#pragma once

#include "Reconstruction.hpp"

// One track's quantization: the bit width of its quantized samples, whether the first sample is kept as it is, and
// the offset and scale that map the samples to their quantized form. The PDB's members in its order (the PDB does
// not name the header they are declared in); the sizes and offsets are the exe's.
struct hkaQuantizeDesc {
	uint8_t m_bitWidth;
	uint8_t m_preserved;
	float m_scale;
	float m_offset;
};

// Fills desc from data's values after the first preserve of them: the offset and scale of
// their range, the bit width qBits (8 when qBits is outside 1..16) and preserve itself. data holds nValues floats.
void hkQuantizeCalcDesc(const float* data, uint32_t nValues, hkaQuantizeDesc& desc, uint32_t qBits, uint32_t preserve);
// Writes data's nValues floats to quantized: the first desc.m_preserved values as their raw 4
// bytes, the rest packed as desc.m_bitWidth-bit samples of (value - m_offset) / m_scale.
void hkQuantizeReal(const float* data, uint32_t nValues, const hkaQuantizeDesc& desc, uint8_t* quantized);
int32_t hkCalcQuantizedSize(int32_t nValues, const hkaQuantizeDesc& desc);
// Expands nValues quantized samples into data, as desc says (the PDB's form; cdecl, ret 0,
// 0xE51660).
void hkExpandReal(const uint8_t* quantized, const hkaQuantizeDesc& desc, float* data, uint32_t nValues);
