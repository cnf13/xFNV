#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendIndices.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// NiColorA's Get differs from the generic one: a compressed stream packs the colour in one dword (B in the
// low byte, then G, R, A), each channel scaled by 1/255.
template <> void NiTConvertingStrideIterator<NiColorA>::Get(int32_t aiIndex, NiColorA& arData) {
	// The stream is raw vertex memory the exe itself walks as untyped bytes.
	uint8_t* pElement = static_cast<uint8_t*>(m_pvCurrentPosition) + m_iStride * static_cast<uint32_t>(aiIndex);
	if (m_bCompressed) {
		uint32_t uiStoredVal = *reinterpret_cast<uint32_t*>(pElement);
		uint8_t ucR = static_cast<uint8_t>(uiStoredVal >> 16);
		uint8_t ucG = static_cast<uint8_t>(uiStoredVal >> 8);
		uint8_t ucB = static_cast<uint8_t>(uiStoredVal);
		uint8_t ucA = static_cast<uint8_t>(uiStoredVal >> 24);
		arData.r = static_cast<double>(ucR) / 255.0;
		arData.g = static_cast<double>(ucG) / 255.0;
		arData.b = static_cast<double>(ucB) / 255.0;
		arData.a = static_cast<double>(ucA) / 255.0;
	} else {
		arData = *reinterpret_cast<NiColorA*>(pElement);
	}
}
