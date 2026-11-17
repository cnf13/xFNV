#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// Treats the allocation as raw bytes, then stores its trailing width, height, and offset arrays.
void NiPixelData::AllocateData(uint32_t uiLevels, uint32_t uiFaces, uint32_t uiBytes) {
	uint32_t uiBytesPerFace = (uiBytes + 3u) & 0xFFFFFFFCu;
	uint32_t uiPixelBytes = uiBytesPerFace * uiFaces;
	uint32_t uiAllocationSize = uiPixelBytes + 12u * uiLevels + 4u;
	m_pucPixels = static_cast<uint8_t*>(_NiMalloc(uiAllocationSize));
	m_puiWidth = reinterpret_cast<uint32_t*>(m_pucPixels + uiPixelBytes);
	m_puiHeight = m_puiWidth + uiLevels;
	m_puiOffsetInBytes = m_puiHeight + uiLevels;
}

// FreeData releases the pixel allocation and resets all four storage pointers.
void NiPixelData::FreeData() {
	_NiFree(m_pucPixels);
	m_puiOffsetInBytes = nullptr;
	m_puiHeight = nullptr;
	m_puiWidth = nullptr;
	m_pucPixels = nullptr;
}
