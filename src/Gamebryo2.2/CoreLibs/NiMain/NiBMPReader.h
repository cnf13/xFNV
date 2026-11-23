#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

// The image reader for Windows bitmap (.bmp) files.
class NiBMPReader : public NiImageReader {
public:
    NiBMPReader();
    ~NiBMPReader() override;

    bool CanReadFile(const char* pcFileExtension) const override;
    // Reads the whole bitmap into pkOptDest when it fits, else into a new NiPixelData.
    NiPixelData* ReadFile(NiFile& kIst, NiPixelData* pkOptDest) override;
    // Reads the file and info headers.
    bool ReadHeader(
        NiFile& kIst,
        uint32_t& uiWidth,
        uint32_t& uiHeight,
        NiPixelFormat& kFormat,
        bool& bMipmap,
        uint32_t& uiFaces
    ) override;

    uint32_t m_uiWidth;
    uint32_t m_uiHeight;
    NiPixelFormat m_kFormat;
    uint16_t m_usBPP;
    uint32_t m_uiClrUsed;
    uint32_t m_uiOffBits;
    bool m_bFlipVert;
    bool m_bRLE;
};
