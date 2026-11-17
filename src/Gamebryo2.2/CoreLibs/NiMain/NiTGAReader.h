#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

#include <stdint.h>

class NiFile;
class NiTGAReader;

// The image reader for Targa (.tga) files: the header of the file being read, the raw row and color map buffers and the
// row unpacker.
class NiTGAReader : public NiImageReader {
public:
    // The Targa image types m_ucImgType holds.
    enum { TGA_Map = 1, TGA_RGB = 2, TGA_Mono = 3, TGA_RLEMap = 9, TGA_RLERGB = 10, TGA_RLEMono = 11 };

    // The Gamebryo source's palette size limit (resources/gamebryo NiTGAReader.h); ReadFile compares against it.
    enum { MAXCOLORS = 16384 };

    // The row unpacker, one of the Unpack* functions.
    using Unpacker = void (*)(NiTGAReader*, uint8_t*, uint8_t*);

    NiTGAReader();
    ~NiTGAReader() override;

    bool CanReadFile(const char* pcFileExtension) const override;
    NiPixelData* ReadFile(NiFile& kIst, NiPixelData* pkOptDest) override;
    bool ReadHeader(
        NiFile& kIst,
        uint32_t& uiWidth,
        uint32_t& uiHeight,
        NiPixelFormat& kFormat,
        bool& bMipmap,
        uint32_t& uiFaces
    ) override;

    // Reads one run-length encoded row of m_usWidth pixels into pucDest. Returns the pixel count.
    uint32_t LoadRLERow(NiFile& kIst, uint8_t* pucDest, uint32_t uiDestSize);
    // Reads the color map of m_usPalLength entries of m_ucCoSize bits into m_pkColorMap.
    void GetColormap(NiFile& kIst);
    // The row unpackers; ReadHeader picks one by pixel size and color map.
    void Unpack4BitSourceRowPal(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack8BitSourceRowPal(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack8BitSourceRowGray(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack16BitSourceRowRGB(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack16BitSourceRowIndexed(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack16BitSourceRowIndexedAlpha(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack24BitSourceRow(uint8_t* pucSrc, uint8_t* pucDest);
    void Unpack32BitSourceRow(uint8_t* pucSrc, uint8_t* pucDest);

    uint8_t m_ucIDLength;
    uint8_t m_ucCoMapType;
    uint8_t m_ucImgType;
    uint16_t m_usMinPalIndex;
    uint16_t m_usPalLength;
    uint8_t m_ucCoSize;
    uint16_t m_usXOrigin;
    uint16_t m_usYOrigin;
    uint16_t m_usWidth;
    uint16_t m_usHeight;
    uint8_t m_ucPixelSize;
    uint8_t m_ucAttBits;
    uint8_t m_uiBytesPerPixel;
    bool m_bAlpha;
    bool m_bColormapped;
    bool m_bFlipVert;
    bool m_bRLE;
    NiPixelFormat m_kFormat;
    uint32_t m_uiRawDataMax;
    uint8_t* m_pucRawData;
    uint32_t m_uiColorMapMax;
    NiPalette::PaletteEntry* m_pkColorMap;
    Unpacker m_pfnUnpacker;
    uint32_t m_uiRLECount;
    bool m_bRLEncodedRun;
    uint8_t m_aucRLEBits[4];
};
