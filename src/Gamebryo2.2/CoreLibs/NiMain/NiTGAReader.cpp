#include "Gamebryo2.2/CoreLibs/NiMain/NiTGAReader.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStreamMacro.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

NiTGAReader::NiTGAReader()
	: m_ucIDLength(0), m_ucCoMapType(0), m_ucImgType(0), m_usMinPalIndex(0), m_usPalLength(0), m_ucCoSize(0),
	  m_usXOrigin(0), m_usYOrigin(0), m_usWidth(0), m_usHeight(0), m_ucPixelSize(0), m_ucAttBits(0),
	  m_uiBytesPerPixel(0), m_bAlpha(false), m_bColormapped(false), m_bFlipVert(false), m_bRLE(false),
	  m_kFormat(NiPixelFormat_RGB24), m_uiRawDataMax(0), m_pucRawData(nullptr), m_uiColorMapMax(0),
	  m_pkColorMap(nullptr), m_pfnUnpacker(nullptr), m_uiRLECount(0), m_bRLEncodedRun(false), m_aucRLEBits{} {}

NiTGAReader::~NiTGAReader() {
	_NiFree(m_pucRawData);
	MemoryManager_Singleton.Deallocate(m_pkColorMap);
}

bool NiTGAReader::CanReadFile(const char* pcFileExtension) const {
	return NiStricmp(pcFileExtension, ".tga") == 0 || NiStricmp(pcFileExtension, ".targa") == 0;
}

NiPixelData* NiTGAReader::ReadFile(NiFile& kIst, NiPixelData* pkOptDest) {
	bool v19;
	uint32_t v21;
	uint32_t v23;
	uint32_t v24;
	uint32_t v25;
	NiPixelData* kIsta;
	NiPixelFormat apMemory;
	_RTL_CRITICAL_SECTION* lpCriticalSection = &m_kReadCriticalSection.m_kCriticalSection;
	EnterCriticalSection(&m_kReadCriticalSection.m_kCriticalSection);
	if (!ReadHeader(kIst, v25, v24, apMemory, v19, v23)) {
		LeaveCriticalSection(&m_kReadCriticalSection.m_kCriticalSection);
		return 0;
	}
	uint8_t m_uiBytesPerPixel = this->m_uiBytesPerPixel;
	uint32_t m_usWidth = this->m_usWidth;
	uint32_t v8 = m_usWidth * m_uiBytesPerPixel;
	uint32_t v20 = v8;
	if (!m_uiBytesPerPixel) {
		if (m_ucPixelSize != 4) {
			goto LABEL_15;
		}
		v20 = m_usWidth >> 1;
		v8 = m_usWidth >> 1;
	}
	if (m_ucCoMapType) {
		if (m_usPalLength + static_cast<uint32_t>(m_usMinPalIndex) + 1 < MAXCOLORS) {
			GetColormap(kIst);
			goto LABEL_9;
		}
	LABEL_15:;
		LeaveCriticalSection(&m_kReadCriticalSection.m_kCriticalSection);
		return 0;
	}
	if (m_bColormapped) {
		goto LABEL_15;
	}
LABEL_9:;
	NiPixelData* v9 = pkOptDest;
	if (pkOptDest &&
		(*pkOptDest->m_puiWidth == this->m_usWidth &&
			(*pkOptDest->m_puiHeight == m_usHeight && pkOptDest->m_kPixelFormat.operator==(m_kFormat)))) {
		kIsta = pkOptDest;
	} else {
		NiPixelData* v10 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
		if (v10) {
			kIsta = ::new (v10) NiPixelData(this->m_usWidth, m_usHeight, m_kFormat, 1u, 1u);
		} else {
			kIsta = 0;
		}
		v9 = kIsta;
	}
	if (m_bColormapped) {
		NiPalette* v12;
		NiPalette* v11 = static_cast<NiPalette*>(NiMemObject::operator new(sizeof(NiPalette)));
		if (v11) {
			v12 = ::new (v11) NiPalette(m_bAlpha, m_usPalLength, m_pkColorMap);
		} else {
			v12 = 0;
		}
		v9->SetPalette(v12);
	}
	if (m_uiRawDataMax < v8) {
		uint8_t* m_pucRawData = this->m_pucRawData;
		m_uiRawDataMax = v8;
		_NiFree(m_pucRawData);
		this->m_pucRawData = static_cast<uint8_t*>(_NiMalloc(m_uiRawDataMax));
	}
	uint32_t* m_puiWidth = v9->m_puiWidth;
	uint8_t* v14 = &v9->m_pucPixels[*v9->m_puiOffsetInBytes];
	int32_t v15 = *m_puiWidth * v9->m_uiPixelStride;
	if (!v15) {
		v15 = *m_puiWidth >> 1;
	}
	if (m_bFlipVert) {
		v14 += v15 * (m_usHeight - 1);
		v15 = -v15;
	}
	uint32_t pkOptDesta = 0;
	if (m_bRLE) {
		if (m_usHeight) {
			do {
				LoadRLERow(kIst, m_pucRawData, m_uiRawDataMax);
				m_pfnUnpacker(this, this->m_pucRawData, v14);
				v14 += v15;
				++pkOptDesta;
			} while (pkOptDesta < m_usHeight);
		}
	} else {
		if (m_usHeight) {
			do {
				uint8_t* v17 = this->m_pucRawData;
				NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = kIst.m_pfnRead;
				v21 = 1;
				kIst.m_uiAbsoluteCurrentPos += m_pfnRead(&kIst, v17, v20, &v21, 1);
				m_pfnUnpacker(this, this->m_pucRawData, v14);
				v14 += v15;
				++pkOptDesta;
			} while (pkOptDesta < m_usHeight);
		}
	}
	LeaveCriticalSection(lpCriticalSection);
	return kIsta;
}

bool NiTGAReader::ReadHeader(NiFile& kIst, uint32_t& uiWidth, uint32_t& uiHeight, NiPixelFormat& kFormat, bool& bMipmap,
	uint32_t& uiFaces) {
	bool result;
	uint32_t v26;
	kIst.SetEndianSwap(0);
	NiCriticalSection* lpCriticalSection = &m_kReadCriticalSection;
	EnterCriticalSection(&m_kReadCriticalSection.m_kCriticalSection);
	uiFaces = 1;
	NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = kIst.m_pfnRead;
	v26 = 1;
	kIst.m_uiAbsoluteCurrentPos += m_pfnRead(&kIst, &m_ucIDLength, 1, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v10 = kIst.m_pfnRead;
	v26 = 1;
	kIst.m_uiAbsoluteCurrentPos += v10(&kIst, &m_ucCoMapType, 1, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v11 = kIst.m_pfnRead;
	v26 = 1;
	kIst.m_uiAbsoluteCurrentPos += v11(&kIst, &m_ucImgType, 1, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v12 = kIst.m_pfnRead;
	v26 = 2;
	kIst.m_uiAbsoluteCurrentPos += v12(&kIst, &m_usMinPalIndex, 2, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v13 = kIst.m_pfnRead;
	v26 = 2;
	kIst.m_uiAbsoluteCurrentPos += v13(&kIst, &m_usPalLength, 2, &v26, 1);
	v26 = 1;
	kIst.m_uiAbsoluteCurrentPos += kIst.m_pfnRead(&kIst, &m_ucCoSize, 1, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v14 = kIst.m_pfnRead;
	v26 = 2;
	kIst.m_uiAbsoluteCurrentPos += v14(&kIst, &m_usXOrigin, 2, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v15 = kIst.m_pfnRead;
	v26 = 2;
	kIst.m_uiAbsoluteCurrentPos += v15(&kIst, &m_usYOrigin, 2, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v16 = kIst.m_pfnRead;
	v26 = 2;
	kIst.m_uiAbsoluteCurrentPos += v16(&kIst, &this->m_usWidth, 2, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v17 = kIst.m_pfnRead;
	v26 = 2;
	kIst.m_uiAbsoluteCurrentPos += v17(&kIst, &m_usHeight, 2, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v18 = kIst.m_pfnRead;
	v26 = 1;
	kIst.m_uiAbsoluteCurrentPos += v18(&kIst, &m_ucPixelSize, 1, &v26, 1);
	NiBinaryStream::NIBINARYSTREAMREADFN v19 = kIst.m_pfnRead;
	v26 = 1;
	kIst.m_uiAbsoluteCurrentPos += v19(&kIst, &m_ucAttBits, 1, &v26, 1);
	if (m_ucIDLength) {
		kIst.Seek(m_ucIDLength, NiFile::ms_iSeekCur);
	}
	m_bFlipVert = (m_ucAttBits & 32) == 0;
	switch (m_ucImgType) {
	case TGA_Map: {
		this->m_bColormapped = 1;
		m_bRLE = 0;
		goto LABEL_9;
	}
	case TGA_RGB:
	case TGA_Mono: {
		m_bRLE = 0;
		goto LABEL_8;
	}
	case TGA_RLEMap: {
		this->m_bColormapped = 1;
		m_bRLE = 1;
		goto LABEL_9;
	}
	case TGA_RLERGB:
	case TGA_RLEMono: {
		uint8_t m_ucCoSize;
		NiPixelFormat* v22;
		m_bRLE = 1;
	LABEL_8:;
		this->m_bColormapped = 0;
	LABEL_9:;
		bool m_bColormapped = this->m_bColormapped;
		if (m_bColormapped) {
			m_ucCoSize = this->m_ucCoSize;
		} else {
			m_ucCoSize = m_ucPixelSize;
		}
		m_bAlpha = m_ucCoSize == 32;
		if (m_bColormapped) {
			if (m_usPalLength == 16) {
				v22 = &NiPixelFormat_PALA4;
				if (m_ucCoSize != 32) {
					v22 = &NiPixelFormat_PAL4;
				}
			} else {
				v22 = &NiPixelFormat_PALA8;
				if (m_ucCoSize != 32) {
					v22 = &NiPixelFormat_PAL8;
				}
			}
		} else {
			v22 = &NiPixelFormat_RGBA32;
			if (m_ucCoSize != 32) {
				v22 = &NiPixelFormat_RGB24;
			}
		}
		m_kFormat = *v22;
		switch (m_ucPixelSize) {
		case 4: {
			m_uiBytesPerPixel = 0;
			if (!m_bColormapped) {
				goto LABEL_34;
			}
			m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack4BitSourceRowPal>;
			break;
		}
		case 8: {
			m_uiBytesPerPixel = 1;
			if (m_bColormapped) {
				m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack8BitSourceRowPal>;
			} else {
				m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack8BitSourceRowGray>;
			}
			break;
		}
		case 15:
		case 16: {
			m_uiBytesPerPixel = 2;
			if (m_bColormapped) {
				if (m_ucCoSize == 32) {
					m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack16BitSourceRowIndexedAlpha>;
				} else {
					m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack16BitSourceRowIndexed>;
				}
			} else {
				m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack16BitSourceRowRGB>;
			}
			break;
		}
		case 24: {
			m_uiBytesPerPixel = 3;
			m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack24BitSourceRow>;
			break;
		}
		case 32: {
			m_uiBytesPerPixel = 4;
			m_pfnUnpacker = &fnvd::callMember<&NiTGAReader::Unpack32BitSourceRow>;
			break;
		}
		default: {
			break;
		}
		}
		uint32_t m_usWidth = this->m_usWidth;
		m_bRLEncodedRun = 0;
		m_uiRLECount = 0;
		uiWidth = m_usWidth;
		uiHeight = m_usHeight;
		kFormat = m_kFormat;
		bMipmap = 0;
		LeaveCriticalSection(&lpCriticalSection->m_kCriticalSection);
		result = 1;
		break;
	}
	default: {
	LABEL_34:;
		LeaveCriticalSection(&lpCriticalSection->m_kCriticalSection);
		result = 0;
		break;
	}
	}
	return result;
}

// A run's pixels are copied with a plain memcpy, so uiDestSize is unused. Returns the pixel count.
uint32_t NiTGAReader::LoadRLERow(NiFile& kIst, uint8_t* pucDest, uint32_t uiDestSize) {
	uint32_t uiPixelCol = 0;
	while (uiPixelCol < m_usWidth) {
		if (m_uiRLECount == 0) {
			uint8_t ucTag = 0;
			NiStreamLoadBinary(kIst, &ucTag, 1);
			m_bRLEncodedRun = (ucTag & 0x80) ? true : false;
			if (ucTag & 0x80) {
				m_bRLEncodedRun = true;
				m_uiRLECount = ucTag - 127;
				NiStreamLoadBinary(kIst, m_aucRLEBits, m_uiBytesPerPixel);
			} else {
				m_bRLEncodedRun = false;
				m_uiRLECount = ucTag + 1;
			}
		}
		uint32_t uiRunToCopy = m_usWidth - uiPixelCol;
		if (m_uiRLECount < uiRunToCopy) {
			uiRunToCopy = m_uiRLECount;
		}
		if (m_bRLEncodedRun) {
			for (uint32_t j = 0; j < uiRunToCopy; j++) {
				rt::memcpy(pucDest, m_aucRLEBits, m_uiBytesPerPixel);
				pucDest += m_uiBytesPerPixel;
			}
		} else {
			NiStreamLoadBinary(kIst, pucDest, m_uiBytesPerPixel * uiRunToCopy);
			pucDest += m_uiBytesPerPixel * uiRunToCopy;
		}
		m_uiRLECount -= uiRunToCopy;
		uiPixelCol += uiRunToCopy;
	}
	return uiPixelCol;
}

void NiTGAReader::GetColormap(NiFile& kIst) {
	uint32_t m_usPalLength = this->m_usPalLength;
	if (m_uiColorMapMax < m_usPalLength) {
		m_uiColorMapMax = m_usPalLength;
		MemoryManager_Singleton.Deallocate(this->m_pkColorMap);
		this->m_pkColorMap = static_cast<NiPalette::PaletteEntry*>(MemoryManager_Singleton.Allocate(m_uiColorMapMax <=
					SIZE_MAX / sizeof(NiPalette::PaletteEntry)
				? sizeof(NiPalette::PaletteEntry) * m_uiColorMapMax
				: SIZE_MAX));
	}
	uint32_t v4 = 4 * this->m_usPalLength;
	if (m_uiRawDataMax < v4) {
		m_uiRawDataMax = v4;
		_NiFree(this->m_pucRawData);
		this->m_pucRawData = static_cast<uint8_t*>(_NiMalloc(m_uiRawDataMax));
	}
	NiPalette::PaletteEntry* m_pkColorMap = this->m_pkColorMap;
	switch (m_ucCoSize) {
	case 8: {
		uint8_t* m_pucRawData = this->m_pucRawData;
		NiStreamLoadBinary<uint8_t>(kIst, m_pucRawData, this->m_usPalLength);
		for (uint32_t i = 0; i < this->m_usPalLength; ++m_pucRawData) {
			uint8_t v8 = *m_pucRawData;
			m_pkColorMap->m_ucBlue = *m_pucRawData;
			m_pkColorMap->m_ucGreen = v8;
			m_pkColorMap->m_ucRed = v8;
			m_pkColorMap->m_ucAlpha = -1;
			++i;
			++m_pkColorMap;
		}
		break;
	}
	case 15:
	case 16: {
		uint8_t* v9 = this->m_pucRawData;
		NiStreamLoadBinary<uint8_t>(kIst, v9, 2 * this->m_usPalLength);
		for (uint32_t j = 0; j < this->m_usPalLength; v9 += 2) {
			m_pkColorMap->m_ucRed = 2 * (v9[1] & 252);
			m_pkColorMap->m_ucGreen = (v9[1] << 6) + (*v9 >> 2 & 56);
			m_pkColorMap->m_ucBlue = 8 * *v9;
			m_pkColorMap->m_ucAlpha = -1;
			++j;
			++m_pkColorMap;
		}
		break;
	}
	case 24: {
		uint8_t* v11 = this->m_pucRawData;
		NiStreamLoadBinary<uint8_t>(kIst, v11, 3 * this->m_usPalLength);
		for (uint32_t k = 0; k < this->m_usPalLength; ++m_pkColorMap) {
			uint8_t v13 = *v11;
			uint8_t* v14 = v11 + 1;
			m_pkColorMap->m_ucBlue = v13;
			uint8_t v15 = *v14++;
			m_pkColorMap->m_ucGreen = v15;
			m_pkColorMap->m_ucRed = *v14;
			m_pkColorMap->m_ucAlpha = -1;
			++k;
			v11 = v14 + 1;
		}
		break;
	}
	case 32: {
		uint8_t* v16 = this->m_pucRawData;
		NiStreamLoadBinary<uint8_t>(kIst, v16, 4 * this->m_usPalLength);
		for (uint32_t m = 0; m < this->m_usPalLength; ++m_pkColorMap) {
			uint8_t v18 = *v16;
			uint8_t* v19 = v16 + 1;
			m_pkColorMap->m_ucBlue = v18;
			uint8_t v20 = *v19++;
			m_pkColorMap->m_ucGreen = v20;
			uint8_t v21 = *v19++;
			m_pkColorMap->m_ucRed = v21;
			m_pkColorMap->m_ucAlpha = *v19;
			++m;
			v16 = v19 + 1;
		}
		break;
	}
	default: {
		return;
	}
	}
}

// The row unpackers ReadHeader stores in m_pfnUnpacker: each converts one row of m_usWidth source pixels.
void NiTGAReader::Unpack4BitSourceRowPal(uint8_t* pucSrc, uint8_t* pucDest) {
	// Walks m_usWidth / 2 bytes.
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_usWidth >> 1); ++i) {
		*pucDest = *pucSrc - static_cast<uint8_t>(m_usMinPalIndex);
		++pucSrc;
		++pucDest;
	}
}

void NiTGAReader::Unpack8BitSourceRowPal(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		*pucDest = *pucSrc - static_cast<uint8_t>(m_usMinPalIndex);
		++pucSrc;
		++pucDest;
	}
}

void NiTGAReader::Unpack8BitSourceRowGray(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		uint8_t ucGray = *pucSrc;
		pucDest[2] = ucGray;
		pucDest[1] = ucGray;
		pucDest[0] = ucGray;
		pucDest += 3;
		++pucSrc;
	}
}

void NiTGAReader::Unpack16BitSourceRowRGB(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		pucDest[0] = 2 * (pucSrc[1] & 252);
		pucDest[1] = (pucSrc[1] << 6) + (pucSrc[0] >> 2 & 56);
		pucDest[2] = 8 * pucSrc[0];
		pucDest += 3;
		pucSrc += 2;
	}
}

void NiTGAReader::Unpack16BitSourceRowIndexed(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		NiPalette::PaletteEntry* pkEntry = &m_pkColorMap[pucSrc[0] + ((pucSrc[1] << 8) - m_usMinPalIndex)];
		pucDest[0] = pkEntry->m_ucRed;
		pucDest[1] = pkEntry->m_ucGreen;
		pucDest[2] = pkEntry->m_ucBlue;
		pucDest += 3;
		pucSrc += 2;
	}
}

void NiTGAReader::Unpack16BitSourceRowIndexedAlpha(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		NiPalette::PaletteEntry* pkEntry = &m_pkColorMap[pucSrc[0] + ((pucSrc[1] << 8) - m_usMinPalIndex)];
		pucDest[0] = pkEntry->m_ucRed;
		pucDest[1] = pkEntry->m_ucGreen;
		pucDest[2] = pkEntry->m_ucBlue;
		pucDest[3] = pkEntry->m_ucAlpha;
		pucDest += 4;
		pucSrc += 2;
	}
}

void NiTGAReader::Unpack24BitSourceRow(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		pucDest[0] = pucSrc[2];
		pucDest[1] = pucSrc[1];
		pucDest[2] = pucSrc[0];
		pucDest += 3;
		pucSrc += 3;
	}
}

void NiTGAReader::Unpack32BitSourceRow(uint8_t* pucSrc, uint8_t* pucDest) {
	for (uint32_t i = 0; i < m_usWidth; ++i) {
		pucDest[0] = pucSrc[2];
		pucDest[1] = pucSrc[1];
		pucDest[2] = pucSrc[0];
		pucDest[3] = pucSrc[3];
		pucDest += 4;
		pucSrc += 4;
	}
}
