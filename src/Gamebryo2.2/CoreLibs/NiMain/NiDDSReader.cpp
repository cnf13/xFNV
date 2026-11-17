#include "Gamebryo2.2/CoreLibs/NiMain/NiDDSReader.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStreamMacro.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// One 4-byte value read as one component: NiStreamLoadBinary with BinaryRead inlined, as the exe has it. The size
// slot is written before the read, so nothing is read uninitialised.
void NiStreamLoadBinary(NiFile* a1, void* a2) {
	uint32_t uiSize = 4;
	a1->BinaryRead(a2, 4, &uiSize, 1);
}

// NiDDSReader::Read24Bit: reads one mipmap level of one face into the pixel data, then swaps red and blue in place
// when the source has red in the high byte.
void Read24Bit(NiFile& kIst, NiPixelData* pkDest, NiPixelFormat kSrcFmt, NiPixelFormat kDestFmt, uint32_t uiMipMapIdx,
	uint32_t uiFaceIdx, bool bEndianConverted) {
	// EXE-INLINED 0x64DAC0
	uint8_t* pucDest = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[pkDest->m_uiMipmapLevels] * uiFaceIdx +
		pkDest->m_puiOffsetInBytes[uiMipMapIdx];
	// EXE-INLINED 0x861610
	uint32_t uiSize = pkDest->m_puiOffsetInBytes[uiMipMapIdx + 1] - pkDest->m_puiOffsetInBytes[uiMipMapIdx];
	NiStreamLoadBinary(kIst, pucDest, uiSize);
	FNVD_CONSTRUCTED_LOCAL(NiDevImageConverter::PixelBits, kSrcBits, nullptr, kSrcFmt);
	// EXE-INLINED 0x64DA80
	uint32_t uiWidth = pkDest->m_puiWidth[uiMipMapIdx];
	// EXE-INLINED 0x64DAA0
	uint32_t uiHeight = pkDest->m_puiHeight[uiMipMapIdx];
	if (kSrcFmt != kDestFmt && kSrcBits.m_uiRM == 0xFF0000) {
		const uint8_t* pucSrc = pucDest;
		for (uint32_t uiRow = uiHeight; uiRow; --uiRow) {
			for (uint32_t uiCol = uiWidth; uiCol; --uiCol) {
				uint8_t ucB = pucSrc[2];
				uint8_t ucG = pucSrc[1];
				uint8_t ucR = pucSrc[0];
				*pucDest++ = ucB;
				*pucDest++ = ucG;
				*pucDest++ = ucR;
				pucSrc += 3;
			}
		}
	}
}

// NiDDSReader::Read32Bit: reads one mipmap level of one face into the pixel data, then, when the formats differ,
// converts each 32-bit pixel in place to R, G, B, A bytes, widening each component to 8 bits by repeating its bits.
// The alpha byte is 0xFF when the source has no alpha component with bits.
void Read32Bit(NiFile& kIst, NiPixelData* pkDest, NiPixelFormat kSrcFmt, NiPixelFormat kDestFmt, uint32_t uiMipMapIdx,
	uint32_t uiFaceIdx, bool bEndianConverted) {
	// EXE-INLINED 0x64DAC0
	uint8_t* pucDest = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[pkDest->m_uiMipmapLevels] * uiFaceIdx +
		pkDest->m_puiOffsetInBytes[uiMipMapIdx];
	// EXE-INLINED 0x861610
	uint32_t uiSize = pkDest->m_puiOffsetInBytes[uiMipMapIdx + 1] - pkDest->m_puiOffsetInBytes[uiMipMapIdx];
	NiStreamLoadBinary(kIst, pucDest, uiSize);
	FNVD_CONSTRUCTED_LOCAL(NiDevImageConverter::PixelBits, kSrcBits, nullptr, kSrcFmt);
	// EXE-INLINED 0x64DA80
	uint32_t uiWidth = pkDest->m_puiWidth[uiMipMapIdx];
	// EXE-INLINED 0x64DAA0
	uint32_t uiHeight = pkDest->m_puiHeight[uiMipMapIdx];
	// The exe calls operator!= (0xA765C0) here, not operator==.
	if (!(kSrcFmt != kDestFmt)) {
		return;
	}
	// The pixels are raw memory, converted in place: each 32-bit source pixel is read whole before its four bytes
	// are written.
	const uint32_t* puiSrc = reinterpret_cast<const uint32_t*>(pucDest);
	bool bAlpha = false;
	for (uint32_t i = 0; i < 4; ++i) {
		if (kSrcFmt.m_akComponents[i].m_eComponent == NiPixelFormat::COMP_ALPHA) {
			bAlpha = kSrcFmt.m_akComponents[i].m_ucBitsPerComponent != 0;
			break;
		}
	}
	for (uint32_t uiRow = uiHeight; uiRow; --uiRow) {
		for (uint32_t uiCol = uiWidth; uiCol; --uiCol) {
			uint32_t uiPixel = *puiSrc++;

			uint8_t ucRBits = 8 - kSrcBits.m_ucRQ;
			uint32_t uiR = (kSrcBits.m_uiRM & uiPixel) >> kSrcBits.m_ucRS;
			uint32_t uiRRep = 0;
			uint8_t ucRLeft = kSrcBits.m_ucRQ;
			for (; ucRLeft >= ucRBits; ucRLeft -= ucRBits) {
				uiRRep = (uiR | uiRRep) << ucRBits;
			}
			uint8_t ucR = static_cast<uint8_t>(uiR) << kSrcBits.m_ucRQ |
				(uiRRep >> ucRBits | uiR >> (8 - ucRLeft - kSrcBits.m_ucRQ));

			uint8_t ucGBits = 8 - kSrcBits.m_ucGQ;
			uint32_t uiG = (kSrcBits.m_uiGM & uiPixel) >> kSrcBits.m_ucGS;
			uint32_t uiGRep = 0;
			uint8_t ucGLeft = kSrcBits.m_ucGQ;
			for (; ucGLeft >= ucGBits; ucGLeft -= ucGBits) {
				uiGRep = (uiG | uiGRep) << ucGBits;
			}
			uint8_t ucG = static_cast<uint8_t>(uiG) << kSrcBits.m_ucGQ |
				(uiGRep >> ucGBits | uiG >> (8 - ucGLeft - kSrcBits.m_ucGQ));

			uint8_t ucBBits = 8 - kSrcBits.m_ucBQ;
			uint32_t uiB = (kSrcBits.m_uiBM & uiPixel) >> kSrcBits.m_ucBS;
			uint32_t uiBRep = 0;
			uint8_t ucBLeft = kSrcBits.m_ucBQ;
			for (; ucBLeft >= ucBBits; ucBLeft -= ucBBits) {
				uiBRep = (uiB | uiBRep) << ucBBits;
			}
			uint8_t ucB = static_cast<uint8_t>(uiB) << kSrcBits.m_ucBQ |
				(uiBRep >> ucBBits | uiB >> (8 - ucBLeft - kSrcBits.m_ucBQ));

			uint8_t ucA = 0xFF;
			if (bAlpha) {
				uint8_t ucABits = 8 - kSrcBits.m_ucAQ;
				uint32_t uiA = (kSrcBits.m_uiAM & uiPixel) >> kSrcBits.m_ucAS;
				uint32_t uiARep = 0;
				uint8_t ucALeft = kSrcBits.m_ucAQ;
				for (; ucALeft >= ucABits; ucALeft -= ucABits) {
					uiARep = (uiA | uiARep) << ucABits;
				}
				ucA = static_cast<uint8_t>(uiA) << kSrcBits.m_ucAQ |
					(uiARep >> ucABits | uiA >> (8 - ucALeft - kSrcBits.m_ucAQ));
			}

			pucDest[0] = ucR;
			pucDest[1] = ucG;
			pucDest[2] = ucB;
			pucDest[3] = ucA;
			pucDest += 4;
		}
	}
}

NiDDSReader::NiDDSReader() : m_uiHeight(0), m_uiWidth(0), m_kOriginalFormat(NiPixelFormat_DXT1) {}

bool NiDDSReader::CanReadFile(const char* String1) const {
	return (NiStricmp(String1, ".dds") == 0);
}

// The exe reads each level of each face straight into the pixels when the file's format is the final one, else
// through the 16, 24 or 32-bit converters; other depths are skipped.
NiPixelData* NiDDSReader::ReadFile(NiFile& a1, NiPixelData* a2) {
	bool bMipmap;
	uint32_t uiNumFaces;
	uint32_t v31;
	uint32_t v32;
	uint32_t v36;
	NiDDSReader* v3 = this;
	FNVD_CONSTRUCTED_LOCAL(NiPixelFormat, kFmt, nullptr);
	_RTL_CRITICAL_SECTION* lpCriticalSection = &v3->m_kReadCriticalSection.m_kCriticalSection;
	EnterCriticalSection(&v3->m_kReadCriticalSection.m_kCriticalSection);
	if (!v3->ReadHeader(a1, v32, v31, kFmt, bMipmap, uiNumFaces)) {
		LeaveCriticalSection(&v3->m_kReadCriticalSection.m_kCriticalSection);
		return 0;
	}
	NiPixelData* pkDest = a2;
	if (!a2 ||
		(*a2->m_puiWidth != v3->m_uiWidth ||
			(*a2->m_puiHeight != v3->m_uiHeight ||
				(!a2->m_kPixelFormat.operator==(kFmt) ||
					(pkDest->m_uiMipmapLevels != v3->m_uiMipmapLevels || pkDest->m_uiFaces != uiNumFaces))))) {
		NiPixelData* v7 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
		if (v7) {
			pkDest = ::new (v7) NiPixelData(v3->m_uiWidth, v3->m_uiHeight, kFmt, v3->m_uiMipmapLevels, uiNumFaces);
		} else {
			pkDest = 0;
		}
	}
	uint32_t m_uiMipmapLevels = pkDest->m_uiMipmapLevels;
	uint32_t v28 = m_uiMipmapLevels;
	if (kFmt.operator==(v3->m_kOriginalFormat)) {
		if (uiNumFaces) {
			int32_t v9 = -1;
			int32_t kIst = -1;
			do {
				int32_t v10;
				switch (v9) {
				case 0: {
					v10 = 1;
					break;
				}
				case 1: {
					v10 = 2;
					break;
				}
				case 2: {
					v10 = 3;
					break;
				}
				case 3: {
					v10 = 4;
					break;
				}
				case 4: {
					v10 = 5;
					break;
				}
				default: {
					v10 = 0;
					break;
				}
				}
				uint32_t v11 = 0;
				if (v28) {
					do {
						uint32_t* m_puiOffsetInBytes = pkDest->m_puiOffsetInBytes;
						int32_t v13 = m_puiOffsetInBytes[v11 + 1] - m_puiOffsetInBytes[v11];
						int32_t v14 = m_puiOffsetInBytes[v11] + v10 * m_puiOffsetInBytes[pkDest->m_uiMipmapLevels];
						uint8_t* v15 = &pkDest->m_pucPixels[v14];
						uint32_t v22 = v13;
						NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = a1.m_pfnRead;
						v36 = 1;
						a1.m_uiAbsoluteCurrentPos += m_pfnRead(&a1, v15, v22, &v36, 1);
						++v11;
					} while (v11 < v28);
					v9 = kIst;
				}
				kIst = ++v9;
			} while (v9 + 1 < uiNumFaces);
		}
		goto LABEL_47;
	}
	if (uiNumFaces) {
		int32_t v18;
		int32_t kIsta;
		int32_t v37;
		int32_t v17 = -1;
		int32_t v29 = -1;
		while (1) {
			switch (v17) {
			case 0: {
				kIsta = 1;
				break;
			}
			case 1: {
				kIsta = 2;
				break;
			}
			case 2: {
				kIsta = 3;
				break;
			}
			case 3: {
				kIsta = 4;
				break;
			}
			case 4: {
				kIsta = 5;
				break;
			}
			default: {
				kIsta = 0;
				break;
			}
			}
			v18 = 0;
			v37 = 0;
			if (m_uiMipmapLevels) {
				break;
			}
		LABEL_46:;
			v29 = ++v17;
			if (v17 + 1 >= uiNumFaces) {
				goto LABEL_47;
			}
		}
		while (1) {
			uint8_t m_ucBitsPerPixel;
			m_ucBitsPerPixel = v3->m_kOriginalFormat.m_ucBitsPerPixel;
			if (m_ucBitsPerPixel == 16) {
				NiDDSReader::Read16Bit(a1, pkDest, v3->m_kOriginalFormat, kFmt, v18, kIsta, v3->m_bIsEndianConverted);
				goto LABEL_43;
			}
			if (m_ucBitsPerPixel == 24) {
				break;
			}
			if (m_ucBitsPerPixel == 32) {
				Read32Bit(a1, pkDest, v3->m_kOriginalFormat, kFmt, v18, kIsta, v3->m_bIsEndianConverted);
			LABEL_43:;
				v18 = v37;
				v3 = this;
			}
			m_uiMipmapLevels = v28;
			v37 = ++v18;
			if (v18 >= v28) {
				v17 = v29;
				goto LABEL_46;
			}
		}
		Read24Bit(a1, pkDest, v3->m_kOriginalFormat, kFmt, v18, kIsta, v3->m_bIsEndianConverted);
		goto LABEL_43;
	}
LABEL_47:;
	LeaveCriticalSection(lpCriticalSection);
	return pkDest;
}

bool NiDDSReader::ReadHeader(NiFile& kIst, uint32_t& uiWidth, uint32_t& uiHeight, NiPixelFormat& kFormat, bool& bMipmap,
	uint32_t& uiFaces) {
	m_kReadCriticalSection.Lock();

	uint32_t uiTemp;

	uint32_t uiMagicNumber;
	NiStreamLoadBinary(kIst, uiMagicNumber);
	if (uiMagicNumber != NI_DDS) {
		// A file written on the other endianness: swap and test again.
		// EXE-INLINED none: NiEndian::Swap32 of one value (the exe swaps the bytes of a stack copy one by one)
		uint32_t uiSwapped = (uiMagicNumber >> 24) | ((uiMagicNumber >> 8) & 0xFF00) |
			((uiMagicNumber << 8) & 0xFF0000) | (uiMagicNumber << 24);
		if (uiSwapped != NI_DDS) {
			m_kReadCriticalSection.Unlock();
			return false;
		}
		kIst.SetEndianSwap(true);
	}

	uint32_t uiSize;
	NiStreamLoadBinary(kIst, uiSize);
	if (uiSize != 124) {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	uint32_t uiFlags;
	NiStreamLoadBinary(kIst, uiFlags);
	const uint32_t uiRequiredFlags = NI_DDSD_CAPS | NI_DDSD_HEIGHT | NI_DDSD_PIXELFORMAT | NI_DDSD_WIDTH;
	if ((uiFlags & uiRequiredFlags) != uiRequiredFlags) {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	NiStreamLoadBinary(kIst, m_uiHeight);
	NiStreamLoadBinary(kIst, m_uiWidth);

	// The pitch or linear size, then the depth.
	NiStreamLoadBinary(kIst, uiTemp);
	NiStreamLoadBinary(kIst, uiTemp);

	// Volume textures are not supported.
	if ((uiFlags & NI_DDSD_DEPTH) && uiTemp) {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	uint32_t uiMipmapCount;
	NiStreamLoadBinary(kIst, uiMipmapCount);

	// The reserved dwords.
	kIst.Seek(4 * 11, NiFile::ms_iSeekCur);

	// The pixel format's size, flags and FourCC code.
	NiStreamLoadBinary(kIst, uiTemp);

	uint32_t uiFormatFlags;
	NiStreamLoadBinary(kIst, uiFormatFlags);

	uint32_t uiCompressionCode;
	NiStreamLoadBinary(kIst, uiCompressionCode);

	// The exe clears the flag here; the Gamebryo source does not.
	m_bIsEndianConverted = false;

	if (uiFormatFlags & NI_FOURCC) {
		switch (uiCompressionCode) {
		case NI_DXTC1:
			m_kOriginalFormat = NiPixelFormat_DXT1;
			break;
		case NI_DXTC3:
			m_kOriginalFormat = NiPixelFormat_DXT3;
			break;
		case NI_DXTC5:
			m_kOriginalFormat = NiPixelFormat_DXT5;
			break;
		case NI_R16:
			m_kOriginalFormat = NiPixelFormat_R16;
			break;
		case NI_RG32:
			m_kOriginalFormat = NiPixelFormat_RG32;
			break;
		case NI_RGBA64:
			m_kOriginalFormat = NiPixelFormat_RGBA64;
			break;
		case NI_R32:
			m_kOriginalFormat = NiPixelFormat_R32;
			break;
		case NI_RG64:
			m_kOriginalFormat = NiPixelFormat_RG64;
			break;
		case NI_RGBA128:
			m_kOriginalFormat = NiPixelFormat_RGBA128;
			break;
		default:
			m_kReadCriticalSection.Unlock();
			return false;
		}

		kFormat = m_kOriginalFormat;

		// The bit count and the four masks.
		kIst.Seek(4 * 5, NiFile::ms_iSeekCur);
	} else if (uiFormatFlags & NI_RGB) {
		uint32_t uiBitsPerPixel;
		NiStreamLoadBinary(&kIst, &uiBitsPerPixel);

		uint32_t uiRedMask;
		NiStreamLoadBinary(&kIst, &uiRedMask);

		uint32_t uiGreenMask;
		NiStreamLoadBinary(&kIst, &uiGreenMask);

		uint32_t uiBlueMask;
		NiStreamLoadBinary(&kIst, &uiBlueMask);

		uint32_t uiAlphaMask;
		NiStreamLoadBinary(&kIst, &uiAlphaMask);

		if (!(uiFormatFlags & NI_ALPHAPIXELS)) {
			uiAlphaMask = 0;
		}

		if (!ValidateRGBABitmasks(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask, uiBitsPerPixel)) {
			m_kReadCriticalSection.Unlock();
			return false;
		}

		// One stack temporary holds both results.
		FNVD_INLINED_LOCAL(NiPixelFormat, kTemp, std::nullopt, std::nullopt);
		NiPixelFormat* pkCreated =
			NiPixelFormat::CreateFromRGBAMasks(&kTemp, uiBitsPerPixel, uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask);
		kFormat = *pkCreated;
		// The exe copies the temporary, not kFormat, into m_kOriginalFormat.
		m_kOriginalFormat = *pkCreated;
		kFormat = *ComputeFinalFormat(&kTemp, kFormat);
	} else {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	uint32_t uiCaps1;
	NiStreamLoadBinary(&kIst, &uiCaps1);
	if (!(uiCaps1 & NI_DDSCAPS_COMPLEX) && !(uiCaps1 & (NI_DDSCAPS_TEXTURE | NI_DDSCAPS_MIPMAP))) {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	uint32_t uiCaps2;
	NiStreamLoadBinary(&kIst, &uiCaps2);

	// Volume textures are not supported.
	if ((uiCaps2 & NI_DDSCAPS2_VOLUME) != 0) {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	// A cube map must have all six faces.
	if ((uiCaps2 & NI_DDSCAPS2_CUBEMAP) != 0 &&
		(((uiCaps2 & NI_DDSCAPS2_CUBEMAP_POSITIVEX) == 0) || ((uiCaps2 & NI_DDSCAPS2_CUBEMAP_NEGATIVEX) == 0) ||
			((uiCaps2 & NI_DDSCAPS2_CUBEMAP_POSITIVEY) == 0) || ((uiCaps2 & NI_DDSCAPS2_CUBEMAP_NEGATIVEY) == 0) ||
			((uiCaps2 & NI_DDSCAPS2_CUBEMAP_POSITIVEZ) == 0) || ((uiCaps2 & NI_DDSCAPS2_CUBEMAP_NEGATIVEZ) == 0))) {
		m_kReadCriticalSection.Unlock();
		return false;
	}

	if ((uiCaps2 & NI_DDSCAPS2_CUBEMAP) != 0) {
		uiFaces = 6;
	} else {
		uiFaces = 1;
	}

	// The rest of the caps and the reserved dword.
	kIst.Seek(4 * 3, NiFile::ms_iSeekCur);

	uiWidth = m_uiWidth;
	uiHeight = m_uiHeight;

	if ((uiCaps1 & NI_DDSCAPS_COMPLEX) && (uiCaps1 & NI_DDSCAPS_MIPMAP) && (uiMipmapCount != 1)) {
		bMipmap = true;
		m_uiMipmapLevels = uiMipmapCount;
	} else {
		bMipmap = false;
		m_uiMipmapLevels = 1;
	}

	m_kReadCriticalSection.Unlock();
	return true;
}

// KEEP: the exe's caller (ReadHeader, 0xA8C501) pushes edx after the call, relying on this body leaving edx intact,
// which clang does not guarantee. Its address is in the class (protected member).
bool NiDDSReader::ValidateRGBABitmasks(uint32_t uiRedMask, uint32_t uiGreenMask, uint32_t uiBlueMask,
	uint32_t uiAlphaMask, uint32_t uiBitsPerPixel) {
	return uiBitsPerPixel == 16 || (uiBitsPerPixel == 24 || uiBitsPerPixel == 32);
}

NiPixelFormat* NiDDSReader::ComputeFinalFormat(NiPixelFormat* apResult, NiPixelFormat kSrcFormat) {
	switch (kSrcFormat.m_ucBitsPerPixel) {
	case 16:
		if (kSrcFormat.GetMask(NiPixelFormat::COMP_ALPHA)) {
			*apResult = NiPixelFormat_RGBA32;
		} else {
			*apResult = NiPixelFormat_RGB24;
		}
		return apResult;
	case 24:
		*apResult = NiPixelFormat_RGB24;
		return apResult;
	case 32:
		*apResult = NiPixelFormat_RGBA32;
		return apResult;
	default:
		*apResult = kSrcFormat;
		return apResult;
	}
}

// NiDDSReader::Read16Bit: reads one mipmap level of one face in chunks of up to 512 bytes and converts each 16-bit
// pixel to RGBA32 or RGB24 bytes (the destination format), widening each component to 8 bits by repeating its bits.
// Any other destination format stops after the first chunk's read.
// KEEP (niread): a short read of a chunk leaves the rest of the 512-byte stack buffer unwritten, and the body converts
// those bytes into the pixel data all the same.
void NiDDSReader::Read16Bit(NiFile& kIst, NiPixelData* pkDest, NiPixelFormat kSrcFmt, NiPixelFormat kDestFmt,
	uint32_t uiMipMapIdx, uint32_t uiFaceIdx, bool bEndianConverted) {
	// EXE-INLINED 0x64DAC0
	uint8_t* pucDest = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[pkDest->m_uiMipmapLevels] * uiFaceIdx +
		pkDest->m_puiOffsetInBytes[uiMipMapIdx];
	// EXE-INLINED 0x64DAA0
	uint32_t uiHeight = pkDest->m_puiHeight[uiMipMapIdx];
	// EXE-INLINED 0x64DA80
	uint32_t uiSize = uiHeight * pkDest->m_puiWidth[uiMipMapIdx] * 2;
	FNVD_CONSTRUCTED_LOCAL(NiDevImageConverter::PixelBits, kSrcBits, nullptr, kSrcFmt);
	uint32_t uiBytesRead = 0;
	while (uiBytesRead < uiSize) {
		uint8_t aucBuffer[512];
		uint32_t uiChunk = 512;
		if (uiSize - uiBytesRead < 512) {
			uiChunk = uiSize - uiBytesRead;
		}
		NiStreamLoadBinary(kIst, aucBuffer, uiChunk);
		bool bRGBA = kDestFmt == NiPixelFormat_RGBA32;
		if (!bRGBA && !(kDestFmt == NiPixelFormat_RGB24)) {
			return;
		}
		// The buffer is raw file data, read a 16-bit pixel at a time.
		const uint16_t* pusSrc = reinterpret_cast<const uint16_t*>(aucBuffer);
		for (uint32_t uiPixels = uiChunk >> 1; uiPixels; --uiPixels) {
			uint8_t ucRBits = 8 - kSrcBits.m_ucRQ;
			uint32_t uiR = (kSrcBits.m_uiRM & *pusSrc) >> kSrcBits.m_ucRS;
			uint32_t uiRRep = 0;
			uint8_t ucRLeft = kSrcBits.m_ucRQ;
			for (; ucRLeft >= ucRBits; ucRLeft -= ucRBits) {
				uiRRep = (uiR | uiRRep) << ucRBits;
			}
			*pucDest++ = static_cast<uint8_t>(uiR) << kSrcBits.m_ucRQ |
				(uiRRep >> ucRBits | uiR >> (8 - ucRLeft - kSrcBits.m_ucRQ));

			uint8_t ucGBits = 8 - kSrcBits.m_ucGQ;
			uint32_t uiG = (kSrcBits.m_uiGM & *pusSrc) >> kSrcBits.m_ucGS;
			uint32_t uiGRep = 0;
			uint8_t ucGLeft = kSrcBits.m_ucGQ;
			for (; ucGLeft >= ucGBits; ucGLeft -= ucGBits) {
				uiGRep = (uiG | uiGRep) << ucGBits;
			}
			*pucDest++ = static_cast<uint8_t>(uiG) << kSrcBits.m_ucGQ |
				(uiGRep >> ucGBits | uiG >> (8 - ucGLeft - kSrcBits.m_ucGQ));

			uint8_t ucBBits = 8 - kSrcBits.m_ucBQ;
			uint32_t uiB = (kSrcBits.m_uiBM & *pusSrc) >> kSrcBits.m_ucBS;
			uint32_t uiBRep = 0;
			uint8_t ucBLeft = kSrcBits.m_ucBQ;
			for (; ucBLeft >= ucBBits; ucBLeft -= ucBBits) {
				uiBRep = (uiB | uiBRep) << ucBBits;
			}
			*pucDest++ = static_cast<uint8_t>(uiB) << kSrcBits.m_ucBQ |
				(uiBRep >> ucBBits | uiB >> (8 - ucBLeft - kSrcBits.m_ucBQ));

			if (bRGBA) {
				uint8_t ucABits = 8 - kSrcBits.m_ucAQ;
				uint32_t uiA = (kSrcBits.m_uiAM & *pusSrc) >> kSrcBits.m_ucAS;
				uint32_t uiARep = 0;
				uint8_t ucALeft = kSrcBits.m_ucAQ;
				for (; ucALeft >= ucABits; ucALeft -= ucABits) {
					uiARep = (uiA | uiARep) << ucABits;
				}
				*pucDest++ = static_cast<uint8_t>(uiA) << kSrcBits.m_ucAQ |
					(uiARep >> ucABits | uiA >> (8 - ucALeft - kSrcBits.m_ucAQ));
			}
			++pusSrc;
		}
		uiBytesRead += uiChunk;
	}
}
