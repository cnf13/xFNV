#include "Havok/Source/Animation/Internal/Compression/hkaCompression.h"

// Copies the m_preserved raw floats, then decodes the rest: a bit per value at the start of the stream says whether
// it is m_runSymbol, else the next m_bitWidth-bit code from the bit stream after the flags. Returns the bytes read.
uint32_t hkBlockDecode(const uint8_t* encodedData, hkaBlockDesc& desc, uint8_t* rawData, uint32_t rawDataSizeBytes) {
	uint32_t m_preserved = desc.m_preserved;
	uint32_t m_preserved_1 = 0;
	uint32_t v33 = 8 * (rawDataSizeBytes - 4 * m_preserved) / desc.m_bitWidth;
	// Both buffers are untyped bytes whose first m_preserved dwords the exe copies as floats (fld/fstp).
	float* pRawReals = reinterpret_cast<float*>(rawData);
	const float* pEncodedReals = reinterpret_cast<const float*>(encodedData);
	if (m_preserved) {
		do {
			pRawReals[m_preserved_1] = pEncodedReals[m_preserved_1];
			++m_preserved_1;
		} while (m_preserved_1 < desc.m_preserved);
	}
	uint32_t m_bitWidth = desc.m_bitWidth;
	uint32_t v9 = 4 * desc.m_preserved;
	int16_t v36 = (1 << desc.m_bitWidth) - 1;
	const uint8_t* v31 = &encodedData[v9];
	const uint8_t* v10 = &encodedData[v9 + ((v33 + 7) >> 3)];
	uint8_t* v11 = &rawData[v9];
	uint32_t v12 = static_cast<uint16_t>(*v10 | v10[1] << 8);
	uint8_t n0xF = 0;
	const uint8_t* v14 = v10 + 2;
	uint8_t n16 = 16;
	uint32_t a3 = 0;
	uint8_t a2 = 1;
	uint32_t a1 = v12;
	char n16_1 = 16;
	int32_t v32 = 0;
	if (desc.m_bitWidth == 8) {
		if (v33) {
			uint32_t v28 = v33;
			do {
				if ((a2 & *v31) != 0) {
					*v11++ = desc.m_runSymbol;
				} else {
					if (n16 < 8u) {
						int32_t v29 = static_cast<uint16_t>(v14[1] << 8 | *v14) << n16;
						v14 += 2;
						a1 |= v29;
						n16 += 16;
					}
					*v11++ = a1 & v36;
					n16 -= 8;
					++v32;
					a1 >>= 8;
				}
				a2 *= 2;
				if (!a2) {
					++v31;
					a2 = 1;
				}
				--v28;
			} while (v28);
		}
	} else {
		if (m_bitWidth == 16) {
			uint8_t m_runSymbol = desc.m_runSymbol;
			char a3a = static_cast<uint8_t>(desc.m_runSymbol >> 8);
			if (v33) {
				uint32_t v35 = v33;
				do {
					if ((a2 & *v31) != 0) {
						*v11 = m_runSymbol;
						uint8_t* v24 = v11 + 1;
						*v24 = a3a;
						v11 = v24 + 1;
					} else {
						if (n16 < 16u) {
							int32_t v25 = static_cast<uint16_t>(v14[1] << 8 | *v14) << n16;
							v14 += 2;
							v12 = v25 | a1;
							n16 += 16;
						}
						*v11 = v12;
						uint8_t* v26 = v11 + 1;
						uint32_t v27 = v12 >> 8;
						v12 >>= 16;
						*v26 = v27;
						v11 = v26 + 1;
						n16 -= 16;
						++v32;
						a1 = v12;
					}
					a2 *= 2;
					if (!a2) {
						++v31;
						a2 = 1;
					}
					--v35;
				} while (v35);
			}
		} else {
			if (v33) {
				uint32_t v34 = v33;
				do {
					if ((a2 & *v31) != 0) {
						uint8_t n0xF_2 = n0xF;
						n0xF += static_cast<uint8_t>(desc.m_bitWidth);
						uint32_t v17 = desc.m_runSymbol << n0xF_2;
						int16_t v18 = v17 | a3;
						a3 |= v17;
						if (n0xF > 15u) {
							a3 >>= 16;
							*v11 = v18;
							uint8_t* v19 = v11 + 1;
							*v19 = static_cast<uint8_t>(v18 >> 8);
							v11 = v19 + 1;
							n0xF -= 16;
						}
					} else {
						if (n16 < desc.m_bitWidth) {
							int32_t v20 = static_cast<uint16_t>(v14[1] << 8 | *v14) << n16;
							v14 += 2;
							a1 |= v20;
							n16_1 = n16 + 16;
						}
						uint8_t n0xF_1 = n0xF;
						n0xF += static_cast<uint8_t>(desc.m_bitWidth);
						uint32_t v22 = static_cast<uint16_t>(v36 & a1) << n0xF_1 | a3;
						a3 = v22;
						if (n0xF > 15u) {
							*v11 = v22;
							uint8_t* v23 = v11 + 1;
							*v23 = static_cast<uint8_t>(v22 >> 8);
							v11 = v23 + 1;
							a3 = static_cast<uint16_t>(v22 >> 16);
							n0xF -= 16;
						}
						a1 >>= desc.m_bitWidth;
						n16 = n16_1 - desc.m_bitWidth;
						++v32;
						n16_1 = n16;
					}
					a2 *= 2;
					if (!a2) {
						++v31;
						a2 = 1;
					}
					--v34;
				} while (v34);
				if (n0xF) {
					*v11 = a3;
					if (n0xF > 7u) {
						v11[1] = static_cast<uint8_t>(a3 >> 8);
					}
				}
			}
		}
	}
	return (32 * desc.m_preserved + (v33 + v32 * desc.m_bitWidth) + 7) >> 3;
}
