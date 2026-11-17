#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "SDK/MSVC/string.h"

NiRTTI NiPixelData::ms_RTTI("NiPixelData", &NiObject::ms_RTTI);

NiPixelData::NiPixelData()
	: m_spPalette(nullptr), m_pucPixels(nullptr), m_puiWidth(nullptr), m_puiHeight(nullptr),
	  m_puiOffsetInBytes(nullptr), m_uiMipmapLevels(0), m_uiPixelStride(0), m_uiRevID(1), m_uiFaces(0) {}

NiPixelData::NiPixelData(uint32_t auiWidth, uint32_t auiHeight, const NiPixelFormat& akFormat, uint32_t auiMipmapLevels,
	uint32_t auiFaces)
	: m_kPixelFormat(akFormat), m_spPalette(nullptr), bNoConvert(false) {
	m_uiMipmapLevels = auiMipmapLevels ? auiMipmapLevels : ComputeMipmapLevels(auiWidth, auiHeight);
	m_uiFaces = auiFaces;
	uint32_t auiWidths[16];
	uint32_t auiHeights[16];
	// One entry more than there are levels: the end of the last level.
	uint32_t auiOffsetInBytes[17];
	auiOffsetInBytes[0] = 0;
	uint32_t uiWidth = auiWidth;
	uint32_t uiHeight = auiHeight;
	if (akFormat.m_eFormat >= NiPixelFormat::FORMAT_DXT1 && akFormat.m_eFormat <= NiPixelFormat::FORMAT_DXT5) {
		// Compressed: 8 (DXT1) or 16 bytes per 4x4 block.
		m_uiPixelStride = 0;
		uint32_t uiBlockBytes = (akFormat.m_eFormat == NiPixelFormat::FORMAT_DXT1) ? 8 : 16;
		for (uint32_t i = 0; i < m_uiMipmapLevels; i++) {
			uint32_t uiW = uiWidth ? uiWidth : 1;
			uint32_t uiH = uiHeight ? uiHeight : 1;
			auiWidths[i] = uiW;
			auiHeights[i] = uiH;
			auiOffsetInBytes[i + 1] =
				auiOffsetInBytes[i] + uiBlockBytes * ((((uiW + 3) & ~3u) * ((uiH + 3) & ~3u)) >> 4);
			uiWidth >>= 1;
			uiHeight >>= 1;
		}
	} else {
		m_uiPixelStride = akFormat.m_ucBitsPerPixel >> 3;
		for (uint32_t i = 0; i < m_uiMipmapLevels; i++) {
			uint32_t uiW = uiWidth ? uiWidth : 1;
			uint32_t uiH = uiHeight ? uiHeight : 1;
			auiWidths[i] = uiW;
			auiHeights[i] = uiH;
			// A stride of 0 is a four-bit format: two pixels per byte.
			if (m_uiPixelStride) {
				auiOffsetInBytes[i + 1] = auiOffsetInBytes[i] + uiW * uiH * m_uiPixelStride;
			} else {
				auiOffsetInBytes[i + 1] = auiOffsetInBytes[i] + ((uiW * uiH) >> 1);
			}
			uiWidth >>= 1;
			uiHeight >>= 1;
		}
	}
	AllocateData(m_uiMipmapLevels, auiFaces, auiOffsetInBytes[m_uiMipmapLevels]);
	rt::memcpy(m_puiWidth, auiWidths, sizeof(uint32_t) * m_uiMipmapLevels);
	rt::memcpy(m_puiHeight, auiHeights, sizeof(uint32_t) * m_uiMipmapLevels);
	rt::memcpy(m_puiOffsetInBytes, auiOffsetInBytes, sizeof(uint32_t) * (m_uiMipmapLevels + 1));
	m_uiRevID = 1;
}

NiPixelData::NiPixelData(const NiPixelData& a1)
	: m_kPixelFormat(a1.m_kPixelFormat), m_spPalette(nullptr), bNoConvert(false) {
	if (a1.m_spPalette.m_pObject) {
		m_spPalette = new NiPalette(*a1.m_spPalette.m_pObject);
	}
	m_uiMipmapLevels = a1.m_uiMipmapLevels;
	m_uiFaces = a1.m_uiFaces;
	m_uiPixelStride = a1.m_uiPixelStride;
	uint32_t uiTotalBytes = a1.m_puiOffsetInBytes[m_uiMipmapLevels];
	AllocateData(m_uiMipmapLevels, m_uiFaces, uiTotalBytes);
	rt::memcpy(m_puiWidth, a1.m_puiWidth, sizeof(uint32_t) * m_uiMipmapLevels);
	rt::memcpy(m_puiHeight, a1.m_puiHeight, sizeof(uint32_t) * m_uiMipmapLevels);
	rt::memcpy(m_puiOffsetInBytes, a1.m_puiOffsetInBytes, sizeof(uint32_t) * (m_uiMipmapLevels + 1));
	rt::memcpy(m_pucPixels, a1.m_pucPixels, uiTotalBytes);
	m_uiRevID = 1;
}

NiPixelData::~NiPixelData() {
	m_spPalette = nullptr;
	FreeData();
}

void NiPixelData::LoadBinary(NiStream& kStream) {
	NiBinaryStream* v12;
	uint32_t (*v13)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t);
	uint32_t* p_m_uiFaces;
	int32_t v25;
	uint32_t auiOffsetInBytes[16];
	char pvSrc[64];
	char v28[64];
	NiObject::LoadBinary(kStream);
	m_kPixelFormat.LoadBinary(kStream);
	kStream.ReadLinkID();
	NiBinaryStream* m_pkIstr = (&kStream)->Istr();
	uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
	v25 = 4;
	m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_uiMipmapLevels, 4, reinterpret_cast<uint32_t*>(&v25), 1);
	NiBinaryStream* v5 = (&kStream)->Istr();
	uint32_t (*v6)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v5->m_pfnRead;
	v25 = 4;
	v5->m_uiAbsoluteCurrentPos += v6(v5, &m_uiPixelStride, 4, reinterpret_cast<uint32_t*>(&v25), 1);
	for (uint32_t i = 0; i < m_uiMipmapLevels;
		v12->m_uiAbsoluteCurrentPos += v13(v12, &auiOffsetInBytes[i++], 4, reinterpret_cast<uint32_t*>(&v25), 1)) {
		NiBinaryStream* v8 = (&kStream)->Istr();
		uint32_t (*v9)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v8->m_pfnRead;
		v25 = 4;
		v8->m_uiAbsoluteCurrentPos += v9(v8, &pvSrc[4 * i], 4, reinterpret_cast<uint32_t*>(&v25), 1);
		NiBinaryStream* v10 = (&kStream)->Istr();
		uint32_t (*v11)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v10->m_pfnRead;
		v25 = 4;
		v10->m_uiAbsoluteCurrentPos += v11(v10, &v28[4 * i], 4, reinterpret_cast<uint32_t*>(&v25), 1);
		v12 = (&kStream)->Istr();
		v13 = v12->m_pfnRead;
		v25 = 4;
	}
	NiBinaryStream* v14 = (&kStream)->Istr();
	uint32_t* v24 = &auiOffsetInBytes[m_uiMipmapLevels];
	uint32_t (*v15)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v14->m_pfnRead;
	v25 = 4;
	v14->m_uiAbsoluteCurrentPos += v15(v14, v24, 4, reinterpret_cast<uint32_t*>(&v25), 1);
	if ((&kStream)->GetFileVersion() >= 0xA030006) {
		NiBinaryStream* v17 = (&kStream)->Istr();
		uint32_t (*v18)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v17->m_pfnRead;
		v25 = 4;
		p_m_uiFaces = &m_uiFaces;
		v17->m_uiAbsoluteCurrentPos += v18(v17, &m_uiFaces, 4, reinterpret_cast<uint32_t*>(&v25), 1);
	} else {
		p_m_uiFaces = &m_uiFaces;
		m_uiFaces = 1;
	}
	AllocateData(m_uiMipmapLevels, *p_m_uiFaces, auiOffsetInBytes[m_uiMipmapLevels]);
	size_t v19 = 4 * m_uiMipmapLevels;
	rt::memcpy(m_puiWidth, pvSrc, v19);
	rt::memcpy(m_puiHeight, v28, v19);
	rt::memcpy(m_puiOffsetInBytes, auiOffsetInBytes, sizeof(uint32_t) * m_uiMipmapLevels + 4);
	NiBinaryStream* v20 = (&kStream)->Istr();
	uint32_t v23 = *p_m_uiFaces * m_puiOffsetInBytes[m_uiMipmapLevels];
	uint32_t (*v21)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v20->m_pfnRead;
	uint8_t* m_pucPixels = this->m_pucPixels;
	v25 = 1;
	v20->m_uiAbsoluteCurrentPos += v21(v20, m_pucPixels, v23, reinterpret_cast<uint32_t*>(&v25), 1);
}

void NiPixelData::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
	m_spPalette = static_cast<NiPalette*>(kStream.GetObjectFromLinkID());
}

bool NiPixelData::RegisterStreamables(NiStream& kStream) {
	if (!NiObject::RegisterStreamables(kStream)) {
		return false;
	}
	if (m_spPalette.m_pObject) {
		m_spPalette.m_pObject->RegisterStreamables(kStream);
	}
	return true;
}

void NiPixelData::SaveBinary(NiStream& kStream) {
	uint32_t v28;
	uint32_t v29;
	NiStream* v2 = &kStream;
	NiObject::SaveBinary(kStream);
	m_kPixelFormat.SaveBinary(*v2);
	v2->SaveLinkID(m_spPalette.m_pObject);
	NiBinaryStream* m_pkOstr = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
	v29 = 4;
	m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &m_uiMipmapLevels, 4, &v29, 1);
	NiBinaryStream* v6 = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v7 = v6->m_pfnWrite;
	v29 = 4;
	v6->m_uiAbsoluteCurrentPos += v7(v6, &m_uiPixelStride, 4, &v29, 1);
	bool v8 = m_uiMipmapLevels == 0;
	v29 = 0;
	if (!v8) {
		do {
			NiBinaryStream* v9 = v2->Ostr();
			uint32_t v10 = v29;
			uint32_t* v26 = &m_puiWidth[v29];
			NiBinaryStream::NIBINARYSTREAMWRITEFN v11 = v9->m_pfnWrite;
			v28 = 4;
			v9->m_uiAbsoluteCurrentPos += v11(v9, v26, 4, &v28, 1);
			NiBinaryStream* v12 = v2->Ostr();
			NiBinaryStream::NIBINARYSTREAMWRITEFN v13 = v12->m_pfnWrite;
			uint32_t* v25 = &m_puiHeight[v10];
			v28 = 4;
			v12->m_uiAbsoluteCurrentPos += v13(v12, v25, 4, &v28, 1);
			NiBinaryStream* v14 = v2->Ostr();
			NiBinaryStream::NIBINARYSTREAMWRITEFN v15 = v14->m_pfnWrite;
			uint32_t* v22 = &m_puiOffsetInBytes[v10];
			v28 = 4;
			v14->m_uiAbsoluteCurrentPos += v15(v14, v22, 4, &v28, 1);
			++v29;
		} while (v29 < m_uiMipmapLevels);
	}
	NiBinaryStream* v16 = v2->Ostr();
	uint32_t* v27 = &m_puiOffsetInBytes[m_uiMipmapLevels];
	NiBinaryStream::NIBINARYSTREAMWRITEFN v17 = v16->m_pfnWrite;
	v29 = 4;
	v16->m_uiAbsoluteCurrentPos += v17(v16, v27, 4, &v29, 1);
	NiBinaryStream* v18 = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v19 = v18->m_pfnWrite;
	v29 = 4;
	v18->m_uiAbsoluteCurrentPos += v19(v18, &m_uiFaces, 4, &v29, 1);
	NiBinaryStream* v20 = v2->Ostr();
	uint32_t v24 = m_uiFaces * m_puiOffsetInBytes[m_uiMipmapLevels];
	NiBinaryStream::NIBINARYSTREAMWRITEFN v21 = v20->m_pfnWrite;
	uint8_t* m_pucPixels = this->m_pucPixels;
	v29 = 1;
	v20->m_uiAbsoluteCurrentPos += v21(v20, m_pucPixels, v24, &v29, 1);
}

bool NiPixelData::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject)) {
		return false;
	}
	NiPixelData* pkOther = static_cast<NiPixelData*>(pkObject);
	if (m_kPixelFormat != pkOther->m_kPixelFormat) {
		return false;
	}
	if (m_spPalette) {
		if (!m_spPalette->IsEqual(pkOther->m_spPalette)) {
			return false;
		}
	}
	if (m_uiMipmapLevels != pkOther->m_uiMipmapLevels || m_uiPixelStride != pkOther->m_uiPixelStride) {
		return false;
	}
	for (uint32_t i = 0; i < m_uiMipmapLevels; ++i) {
		if (m_puiWidth[i] != pkOther->m_puiWidth[i] || m_puiHeight[i] != pkOther->m_puiHeight[i] ||
			m_puiOffsetInBytes[i] != pkOther->m_puiOffsetInBytes[i]) {
			return false;
		}
	}
	if (m_puiOffsetInBytes[m_uiMipmapLevels] != pkOther->m_puiOffsetInBytes[m_uiMipmapLevels] ||
		m_uiFaces != pkOther->m_uiFaces) {
		return false;
	}
	// EXE-INLINED none: memcmp (a dword loop, then a byte tail; only equality reaches the result)
	const uint8_t* pucThis = m_pucPixels;
	const uint8_t* pucOther = pkOther->m_pucPixels;
	uint32_t uiSize = m_puiOffsetInBytes[m_uiMipmapLevels];
	for (uint32_t i = 0; i < uiSize; ++i) {
		if (pucThis[i] != pucOther[i]) {
			return false;
		}
	}
	return true;
}

void NiPixelData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	char* ViewerString = NiGetViewerString(NiPixelData::ms_RTTI.GetName());
	uint32_t m_usSize = pkStrings->m_usSize;
	uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
	char* v23 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(m_usSize, v23);
	char* ViewerString_6 = NiGetViewerString("Width", *m_puiWidth);
	uint32_t v8 = pkStrings->m_usSize;
	uint32_t v9 = pkStrings->m_usMaxSize;
	v23 = ViewerString_6;
	if (v8 >= v9) {
		pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v8, v23);
	char* v10 = NiGetViewerString("Height", *m_puiHeight);
	uint32_t v11 = pkStrings->m_usSize;
	v23 = v10;
	if (v11 >= pkStrings->m_usMaxSize) {
		pkStrings->SetSize(v11 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v11, v23);
	char* v12 = NiGetViewerString("m_uiMipmapLevels", m_uiMipmapLevels);
	uint32_t v13 = pkStrings->m_usSize;
	uint32_t v14 = pkStrings->m_usMaxSize;
	v23 = v12;
	if (v13 >= v14) {
		pkStrings->SetSize(v13 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v13, v23);
	char* v15 = NiGetViewerString("m_uiFaces", m_uiFaces);
	uint32_t v16 = pkStrings->m_usSize;
	uint32_t v17 = pkStrings->m_usMaxSize;
	v23 = v15;
	if (v16 >= v17) {
		pkStrings->SetSize(v16 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v16, v23);
	char* v18 = NiGetViewerString("TotalSizeInBytes", m_uiFaces * m_puiOffsetInBytes[m_uiMipmapLevels]);
	uint32_t v19 = pkStrings->m_usSize;
	uint32_t v20 = pkStrings->m_usMaxSize;
	v23 = v18;
	if (v19 >= v20) {
		pkStrings->SetSize(v19 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v19, v23);
	char* v21 = NiGetViewerString("FaceSizeInBytes", m_puiOffsetInBytes[m_uiMipmapLevels]);
	uint32_t v22 = pkStrings->m_usSize;
	v23 = v21;
	if (v22 >= pkStrings->m_usMaxSize) {
		pkStrings->SetSize(v22 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v22, v23);
}

uint32_t NiPixelData::GetHeight(uint32_t uiMipmapLevel, uint32_t uiFace) const {
	return m_puiHeight[uiMipmapLevel];
}

// The exe returns zero if either dimension is zero, otherwise the larger mip-chain length.
uint32_t NiPixelData::ComputeMipmapLevels(uint32_t uiWidth, uint32_t uiHeight) {
	if (!uiWidth || !uiHeight) {
		return 0;
	}
	uint32_t uiWidthLevels = 1;
	for (uint32_t uiMipWidth = uiWidth >> 1; uiMipWidth; uiMipWidth >>= 1) {
		++uiWidthLevels;
	}
	uint32_t uiHeightLevels = 1;
	for (uint32_t uiMipHeight = uiHeight >> 1; uiMipHeight; uiMipHeight >>= 1) {
		++uiHeightLevels;
	}
	if (uiHeightLevels >= uiWidthLevels) {
		return uiHeightLevels;
	}
	return uiWidthLevels;
}

NiObject* NiPixelData::CreateObject() {
	void* pkObject = NiMemObject::operator new(sizeof(NiPixelData));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPixelData;
}
