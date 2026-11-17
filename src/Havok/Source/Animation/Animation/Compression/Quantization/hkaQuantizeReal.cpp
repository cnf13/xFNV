#include "Havok/Source/Animation/Animation/Compression/Quantization/hkaQuantizeReal.h"

#include <bit>

// Not named in the PDB or IDA; name chosen here. The minimum (into pMin, esi) and the maximum (into pMax, edx) of
// the n floats of data, both zero when n is not positive (usercall).
static void calcMinMax(float* pMax, const float* data, int32_t n, float* pMin);

static void calcMinMax(float* pMax, const float* data, int32_t n, float* pMin) {
	if (n <= 0) {
		*pMax = 0.0f;
		*pMin = 0.0f;
		return;
	}
	const float* pData = data;
	*pMin = pData[0];
	*pMax = pData[0];
	for (int32_t i = 0; i < n; i++) {
		if (*pMin > pData[i]) {
			*pMin = pData[i];
		} else if (*pMax < pData[i]) {
			*pMax = pData[i];
		}
	}
}

// Fills desc from the range of data's values after the first preserve of them, their bit width and preserve. The
// exe has calcMinMax write the maximum and the minimum into its own desc and preserve argument slots.
void hkQuantizeCalcDesc(const float* data, uint32_t nValues, hkaQuantizeDesc& desc, uint32_t qBits, uint32_t preserve) {
	desc.m_preserved = static_cast<uint8_t>(preserve);
	float fMax;
	float fMin;
	calcMinMax(&fMax, &data[preserve], static_cast<int32_t>(nValues - preserve), &fMin);
	float fOffset = fMin;
	desc.m_offset = fOffset;
	desc.m_scale = fMax - fOffset;
	if (qBits - 1 > 15) {
		desc.m_bitWidth = 8;
	} else {
		desc.m_bitWidth = static_cast<uint8_t>(qBits);
	}
}

// Writes the first m_preserved values raw (four bytes each, little-endian), then each other value quantized to
// m_bitWidth bits and packed into a bit stream flushed 16 bits at a time.
void hkQuantizeReal(const float* data, uint32_t nValues, const hkaQuantizeDesc& desc, uint8_t* quantized) {
	for (int32_t i = 0; i < desc.m_preserved; ++i) {
		// The exe copies the value through the x87 stack (fld/fstp) before storing its bytes.
		float fValue = data[i];
		const uint32_t uiBits = std::bit_cast<uint32_t>(static_cast<float>(fValue));
		quantized[0] = static_cast<uint8_t>(uiBits);
		quantized[1] = static_cast<uint8_t>(uiBits >> 8);
		quantized[2] = static_cast<uint8_t>(uiBits >> 16);
		quantized[3] = static_cast<uint8_t>(uiBits >> 24);
		quantized += 4;
	}

	// The exe's shl masks the count to five bits.
	const uint32_t uiMaxVal = 1u << (desc.m_bitWidth & 31);
	int32_t iBitsUsed = 0;
	uint32_t uiBuffer = 0;
	const uint32_t uiPreserved = desc.m_preserved;
	if (uiPreserved < nValues) {
		float fMaxVal = static_cast<float>(uiMaxVal);
		const uint32_t uiMaxQ = uiMaxVal - 1;
		for (uint32_t i = uiPreserved; i < nValues; ++i) {
			float fR;
			// A NaN scale takes the division (the exe's test ah,0x41 after comparing the epsilon with it).
			if (!(1.1920929e-07f > desc.m_scale)) {
				fR = (data[i] - desc.m_offset) / desc.m_scale;
			} else {
				fR = 0.0f;
			}
			// The exe truncates through a 64-bit fistp and keeps the low dword.
			uint32_t uiQ =
				static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(fMaxVal) * static_cast<double>(fR)));
			if (uiQ > uiMaxQ) {
				uiQ = uiMaxQ;
			}
			uiBuffer |= uiQ << (iBitsUsed & 31);
			iBitsUsed += desc.m_bitWidth;
			if (iBitsUsed > 15) {
				quantized[0] = static_cast<uint8_t>(uiBuffer);
				quantized[1] = static_cast<uint8_t>(uiBuffer >> 8);
				quantized += 2;
				uiBuffer >>= 16;
				iBitsUsed -= 16;
			}
		}
	}
	if (iBitsUsed > 0) {
		quantized[0] = static_cast<uint8_t>(uiBuffer);
		if (iBitsUsed > 8) {
			quantized[1] = static_cast<uint8_t>(uiBuffer >> 8);
		}
	}
}

int32_t hkCalcQuantizedSize(int32_t nValues, const hkaQuantizeDesc& desc) {
	return ((desc.m_bitWidth * (nValues - desc.m_preserved)) + 7) / 8 + 4 * desc.m_preserved;
}
