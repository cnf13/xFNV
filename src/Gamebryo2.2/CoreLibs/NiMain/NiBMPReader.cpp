#include "Gamebryo2.2/CoreLibs/NiMain/NiBMPReader.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiBMPReader::NiBMPReader()
  : m_uiWidth(0), m_uiHeight(0), m_kFormat(NiPixelFormat_RGB24), m_usBPP(0), m_uiClrUsed(0), m_uiOffBits(0),
    m_bFlipVert(false), m_bRLE(false) {}

NiBMPReader::~NiBMPReader() {}

bool NiBMPReader::CanReadFile(const char* pcFileExtension) const {
    return NiStricmp(pcFileExtension, ".bmp") == 0;
}

// KEEP: a paletted bitmap's palette is read into a 256-entry local for its m_uiClrUsed entries only, and the exe
// builds the NiPalette from all 256 entries, reading the rest uninitialised whenever m_uiClrUsed is under 256.
NiPixelData* NiBMPReader::ReadFile(NiFile& kIst, NiPixelData* pkOptDest) {
    bool bMipmap;
    uint32_t uiW;
    uint32_t uiH;
    uint32_t uiNumFaces;
    // The format is constructed and deliberately never destroyed, as the game does.
    NiPixelFormat kFmt;

    m_kReadCriticalSection.Lock();

    if (!ReadHeader(kIst, uiW, uiH, kFmt, bMipmap, uiNumFaces)) {
        m_kReadCriticalSection.Unlock();
        return nullptr;
    }

    NiPixelData* pkDest;
    if (pkOptDest && pkOptDest->m_puiWidth[0] == m_uiWidth && pkOptDest->m_puiHeight[0] == m_uiHeight &&
        pkOptDest->m_kPixelFormat == m_kFormat) {
        pkDest = pkOptDest;
    }
    else {
        pkDest = new NiPixelData(m_uiWidth, m_uiHeight, m_kFormat, 1u, 1u);
    }

    if (m_kFormat == NiPixelFormat_PAL8) {
        NiPalette::PaletteEntry akSrcPal[256];
        uint32_t uiSize = 1;
        kIst.BinaryRead(akSrcPal, m_uiClrUsed * sizeof(NiPalette::PaletteEntry), &uiSize, 1);

        // The file stores blue, green, red, reserved: swap red and blue and make the entries opaque.
        NiPalette::PaletteEntry* pkPal = akSrcPal;
        for (uint32_t i = 0; i < m_uiClrUsed; i++) {
            uint8_t ucTmp = pkPal->m_ucRed;
            pkPal->m_ucRed = pkPal->m_ucBlue;
            pkPal->m_ucBlue = ucTmp;
            pkPal->m_ucAlpha = 255;
            pkPal++;
        }

        NiPalette* pkPalette = new NiPalette(false, 256u, akSrcPal);
        // EXE-INLINED 0xA76A60
        pkDest->m_spPalette = pkPalette;
    }
    else {
        kIst.Seek(static_cast<int32_t>(m_uiClrUsed * sizeof(NiPalette::PaletteEntry)), NiFile::ms_iSeekCur);
    }

    // Skips to the pixels: 54 is the file and info headers' size (WINDOWS_BMPTOTALHEADER_SIZE).
    int32_t iSkipAmt = static_cast<int32_t>(m_uiOffBits - 54 - m_uiClrUsed * sizeof(NiPalette::PaletteEntry));
    if (iSkipAmt > 0) {
        kIst.Seek(iSkipAmt, NiFile::ms_iSeekCur);
    }

    uint32_t uiSizeImage = (((m_uiWidth * m_usBPP) + 31) >> 3 & ~3u) * m_uiHeight;
    uint32_t uiDestRowStride = pkDest->m_uiPixelStride * pkDest->m_puiWidth[0];

    if (m_usBPP == 4) {
        // Two pixels per input byte, each row padded to four bytes.
        uint32_t uiInRowStride = (m_uiWidth >> 1) + (m_uiWidth & 1);
        uint32_t uiInRowPad = (uiInRowStride & 3) ? 4 - (uiInRowStride & 3) : 0;
        uint8_t* pucInput = static_cast<uint8_t*>(_NiMalloc(uiInRowStride + uiInRowPad));

        uint8_t* pucScanLine;
        int32_t iRowStep;
        if (m_bFlipVert) {
            // EXE-INLINED 0x64DAC0
            pucScanLine =
                pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[0] + uiDestRowStride * (pkDest->m_puiHeight[0] - 1);
            iRowStep = -static_cast<int32_t>(uiDestRowStride);
        }
        else {
            // EXE-INLINED 0x64DAC0
            pucScanLine = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[0];
            iRowStep = static_cast<int32_t>(uiDestRowStride);
        }

        for (uint32_t i = 0; i < m_uiHeight; i++, pucScanLine += iRowStep) {
            uint32_t uiSize = 1;
            kIst.BinaryRead(pucInput, uiInRowStride + uiInRowPad, &uiSize, 1);
            uint8_t* pucPixel = pucScanLine;
            uint8_t* pucInputPixel = pucInput;
            for (uint32_t j = 0; j < uiInRowStride; j++) {
                *(pucPixel++) = *pucInputPixel >> 4;
                *(pucPixel++) = *pucInputPixel & 0x0F;
                pucInputPixel++;
            }
        }

        _NiFree(pucInput);
    }
    else if (m_bFlipVert) {
        // Bottom-up rows: fill the image from its last row.
        // EXE-INLINED 0x64DAC0
        uint8_t* pucScanLine = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[0] + uiDestRowStride * (m_uiHeight - 1);
        if (uiDestRowStride & 3) {
            uint32_t uiInRowPad = 4 - (uiDestRowStride & 3);
            uint8_t aucDummy[4];
            for (uint32_t i = 0; i < m_uiHeight; i++) {
                uint32_t uiSize = 1;
                kIst.BinaryRead(pucScanLine, uiDestRowStride, &uiSize, 1);
                uint32_t uiPadSize = 1;
                kIst.BinaryRead(aucDummy, uiInRowPad, &uiPadSize, 1);
                pucScanLine -= uiDestRowStride;
            }
        }
        else {
            for (uint32_t i = 0; i < m_uiHeight; i++) {
                uint32_t uiSize = 1;
                kIst.BinaryRead(pucScanLine, uiDestRowStride, &uiSize, 1);
                pucScanLine -= uiDestRowStride;
            }
        }
    }
    else if (uiDestRowStride & 3) {
        uint32_t uiInRowPad = 4 - (uiDestRowStride & 3);
        // EXE-INLINED 0x64DAC0
        uint8_t* pucScanLine = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[0];
        uint8_t aucDummy[4];
        for (uint32_t i = 0; i < m_uiHeight; i++, pucScanLine += uiDestRowStride) {
            uint32_t uiSize = 1;
            kIst.BinaryRead(pucScanLine, uiDestRowStride, &uiSize, 1);
            uint32_t uiPadSize = 1;
            kIst.BinaryRead(aucDummy, uiInRowPad, &uiPadSize, 1);
        }
    }
    else {
        uint32_t uiSize = 1;
        // EXE-INLINED 0x64DAC0
        kIst.BinaryRead(pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[0], uiSizeImage, &uiSize, 1);
    }

    // The file stores blue, green, red: swap red and blue.
    if (m_kFormat == NiPixelFormat_RGB24 || m_kFormat == NiPixelFormat_RGBA32) {
        uint32_t uiStride = pkDest->m_uiPixelStride;
        uint32_t uiSize = m_uiWidth * m_uiHeight;
        // EXE-INLINED 0x64DAC0
        uint8_t* pucTmp = pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[0];
        for (uint32_t i = 0; i < uiSize; i++, pucTmp += uiStride) {
            uint8_t ucSwap = pucTmp[0];
            pucTmp[0] = pucTmp[2];
            pucTmp[2] = ucSwap;
        }
    }

    m_kReadCriticalSection.Unlock();
    return pkDest;
}

// KEEP (niread): the signature, the header size, the width, the height and the compression are stream reads into
// locals nothing initialised, which the body then uses; a short read leaves them unwritten. The exe reads the
// signature into kIst's argument slot and keeps the first read's component size, then the section's address, in
// uiFaces' slot.
bool NiBMPReader::ReadHeader(
    NiFile& kIst,
    uint32_t& uiWidth,
    uint32_t& uiHeight,
    NiPixelFormat& kFormat,
    bool& bMipmap,
    uint32_t& uiFaces
) {
    kIst.SetEndianSwap(false);
    uiFaces = 1;

    uint32_t uiTempUnsignedInt;
    uint16_t usTempUnsignedShort;
    uint32_t uiSize = sizeof(usTempUnsignedShort);
    kIst.BinaryRead(&usTempUnsignedShort, sizeof(usTempUnsignedShort), &uiSize, 1);
    if (usTempUnsignedShort != 0x4D42) {
        return false;
    }

    m_kReadCriticalSection.Lock();

    uint32_t uiSize1 = sizeof(uiTempUnsignedInt);
    kIst.BinaryRead(&uiTempUnsignedInt, sizeof(uiTempUnsignedInt), &uiSize1, 1);
    uint32_t uiSize2 = sizeof(usTempUnsignedShort);
    kIst.BinaryRead(&usTempUnsignedShort, sizeof(usTempUnsignedShort), &uiSize2, 1);
    uint32_t uiSize3 = sizeof(usTempUnsignedShort);
    kIst.BinaryRead(&usTempUnsignedShort, sizeof(usTempUnsignedShort), &uiSize3, 1);
    uint32_t uiSize4 = sizeof(m_uiOffBits);
    kIst.BinaryRead(&m_uiOffBits, sizeof(m_uiOffBits), &uiSize4, 1);

    uint32_t uiHeaderSize;
    uint32_t uiSize5 = sizeof(uiHeaderSize);
    kIst.BinaryRead(&uiHeaderSize, sizeof(uiHeaderSize), &uiSize5, 1);

    int32_t iWidth;
    int32_t iHeight;
    uint32_t uiSize6 = sizeof(iWidth);
    kIst.BinaryRead(&iWidth, sizeof(iWidth), &uiSize6, 1);
    uint32_t uiSize7 = sizeof(iHeight);
    kIst.BinaryRead(&iHeight, sizeof(iHeight), &uiSize7, 1);
    uint32_t uiSize8 = sizeof(usTempUnsignedShort);
    kIst.BinaryRead(&usTempUnsignedShort, sizeof(usTempUnsignedShort), &uiSize8, 1);
    uint32_t uiSize9 = sizeof(m_usBPP);
    kIst.BinaryRead(&m_usBPP, sizeof(m_usBPP), &uiSize9, 1);

    uint32_t uiCompression;
    uint32_t uiSize10 = sizeof(uiCompression);
    kIst.BinaryRead(&uiCompression, sizeof(uiCompression), &uiSize10, 1);

    int32_t iTempInt;
    uint32_t uiSize11 = sizeof(uiTempUnsignedInt);
    kIst.BinaryRead(&uiTempUnsignedInt, sizeof(uiTempUnsignedInt), &uiSize11, 1);
    uint32_t uiSize12 = sizeof(iTempInt);
    kIst.BinaryRead(&iTempInt, sizeof(iTempInt), &uiSize12, 1);
    uint32_t uiSize13 = sizeof(iTempInt);
    kIst.BinaryRead(&iTempInt, sizeof(iTempInt), &uiSize13, 1);
    uint32_t uiSize14 = sizeof(m_uiClrUsed);
    kIst.BinaryRead(&m_uiClrUsed, sizeof(m_uiClrUsed), &uiSize14, 1);
    uint32_t uiSize15 = sizeof(uiTempUnsignedInt);
    kIst.BinaryRead(&uiTempUnsignedInt, sizeof(uiTempUnsignedInt), &uiSize15, 1);

    // OS/2 core headers are not supported. Gamebryo compares against sizeof(WINDOWS_BMPCOREHEADER_SIZE), the size of
    // its integer constant: 4, not the core header's 12.
    if (uiHeaderSize == 4) {
        m_kReadCriticalSection.Unlock();
        return false;
    }

    switch (m_usBPP) {
        case 4:
        case 8: m_kFormat = NiPixelFormat_PAL8; break;
        case 24: m_kFormat = NiPixelFormat_RGB24; break;
        case 32: m_kFormat = NiPixelFormat_RGBA32; break;
        default: m_kReadCriticalSection.Unlock(); return false;
    }

    // RLE compressed bitmaps (WINDOWS_BI_RLE4 = 2, WINDOWS_BI_RLE8 = 1) are not supported.
    if (uiCompression == 2 || uiCompression == 1) {
        m_kReadCriticalSection.Unlock();
        return false;
    }

    m_uiWidth = static_cast<uint32_t>(iWidth);
    // A positive height is a bottom-up bitmap.
    m_bFlipVert = iHeight >= 0;
    m_uiHeight = iHeight > 0 ? static_cast<uint32_t>(iHeight) : static_cast<uint32_t>(-iHeight);

    // Paletted bitmaps with no colour count use the whole palette.
    if (m_uiClrUsed == 0 && m_usBPP != 24 && m_usBPP != 32) {
        m_uiClrUsed = 1 << m_usBPP;
    }

    uiWidth = m_uiWidth;
    uiHeight = m_uiHeight;
    kFormat = m_kFormat;
    bMipmap = true;

    m_kReadCriticalSection.Unlock();
    return true;
}
