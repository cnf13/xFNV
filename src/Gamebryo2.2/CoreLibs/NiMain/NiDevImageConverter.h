#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureCodec.h"

#include <stdint.h>

class NiPalette;

// The development image converter: reads image files through its readers and converts, mipmaps and bump-maps pixel
// data.
class NiDevImageConverter : public NiImageConverter {
public:
    // A pixel format's channel masks (M), shifts (S) and quantisation bits (Q), as the converters use them.
    class PixelBits {
    public:
        PixelBits() = default;
        PixelBits(const NiPixelFormat& kFmt);

        uint32_t m_uiRM;
        uint32_t m_uiGM;
        uint32_t m_uiBM;
        uint32_t m_uiAM;
        uint8_t m_ucRS;
        uint8_t m_ucGS;
        uint8_t m_ucBS;
        uint8_t m_ucAS;
        uint8_t m_ucRQ;
        uint8_t m_ucGQ;
        uint8_t m_ucBQ;
        uint8_t m_ucAQ;
    };

    // The platform a texture is converted for (FindClosestPixelFormat dispatches on it).
    enum Platform : int32_t {
        ANY = 0,
        PLAYSTATION2 = 1,
        XBOX = 2,
        GAMECUBE = 3,
        XENON = 4,
        PLAYSTATION3 = 5,
        DX9 = 6,
        NUM_PLATFORMS = 7,
    };

    // The signature of the per-format converters (ConvertPAL4To24, ConvertRGBA32To16, ...): converts one mipmap level
    // of one face from pucSrc into pucDest.
    using ConvertFunction = void (*)(
        uint32_t uiWidth,
        uint32_t uiHeight,
        uint32_t uiDestSize,
        uint8_t* pucDest,
        NiPalette* pkSrcPal,
        const PixelBits& kDestBits,
        const uint8_t* pucSrc,
        const PixelBits& kSrcBits
    );

    NiDevImageConverter();
    ~NiDevImageConverter() override;

    bool ReadImageFileInfo(
        const char* pcFilename,
        NiPixelFormat& kFmt,
        bool& bMipmap,
        uint32_t& uiWidth,
        uint32_t& uiHeight,
        uint32_t& uiNumFaces
    ) override;
    NiPixelData* ReadImageFile(const char* pcFilename, NiPixelData* pkOptDest, NiFile* apFile) override;
    bool CanConvertPixelData(const NiPixelFormat& kSrcFmt, const NiPixelFormat& kDestFmt) const override;
    NiPixelData* ConvertPixelData(
        const NiPixelData& kSrc,
        const NiPixelFormat& kDestFmt,
        NiPixelData* pkOptDest,
        bool bMipmap
    ) override;
    virtual bool CanReadImageFile(const char* pcFilename);
    virtual bool IsASupportedMipmapGenerationFormat(const NiPixelFormat& kSrcFmt);
    // Whether kSrcFmt is a format with alpha that mipmaps can be generated in.
    virtual bool IsAnAlphaSupportedMipmapGenerationFormat(const NiPixelFormat& kSrcFmt);
    // Whether kSrcFmt is a format without alpha that mipmaps can be generated in.
    virtual bool IsANonAlphaSupportedMipmapGenerationFormat(const NiPixelFormat& kSrcFmt);
    virtual NiPixelData* GenerateMipmapLevels(const NiPixelData* pkSrc, NiPixelData* pkOptDest);
    virtual NiPixelData*
    GenerateMipmapPyramidFromFiles(const char** apcFilenames, uint32_t uiNumLevels, const NiPixelData* pkSrc);
    virtual bool ConvertPixelDataFormat(NiPixelData& kDest, const NiPixelData& kSrc, int32_t iMipmapLevel);
    virtual NiPixelData* GenerateBumpMap(const NiPixelData& kSrc, const NiPixelFormat& kDestFmt);
    virtual void AddReader(NiImageReader* pkReader);
    virtual const NiPixelFormat* FindClosestPixelFormat(
        Platform ePlatform,
        const NiTexture::FormatPrefs& kFmtPrefs,
        const NiPixelFormat& kSrcFmt,
        const NiPixelFormat& kDesiredFmt
    );
    virtual NiPixelData* ConvertForPlatform(
        Platform ePlatform,
        const NiTexture::FormatPrefs& kPrefs,
        const NiPixelData* pkSrc,
        const NiPixelFormat& kDesiredFormat
    );
    virtual NiTPointerList<NiImageReader*>& GetImageReaderList();
    virtual const NiPixelFormat* FindClosestPixelFormatDX9(
        const NiTexture::FormatPrefs& kFmtPrefs,
        const NiPixelFormat& kSrcFmt,
        const NiPixelFormat& kDesiredFmt
    );
    virtual const NiPixelFormat* FindClosestPixelFormatXenon(
        const NiTexture::FormatPrefs& kFmtPrefs,
        const NiPixelFormat& kSrcFmt,
        const NiPixelFormat& kDesiredFmt
    );
    virtual const NiPixelFormat* FindClosestPixelFormatPS3(
        const NiTexture::FormatPrefs& kFmtPrefs,
        const NiPixelFormat& kSrcFmt,
        const NiPixelFormat& kDesiredFmt
    );

    NiTextureCodec m_kCodec;
    // The constructor adds its image readers (NiBMPReader, NiTGAReader, ...) at the head.
    NiTPointerList<NiImageReader*> m_kReaders;
};

void ConvertPAL4To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPAL4To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPALA4To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPAL8To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPAL8To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPALA8To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPAL8To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertPALA8To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGB24To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert24ToRGB24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* pvDest,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* pvSrc,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGBA16ToBGRA16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
// Forwards to ConvertRGBA16ToBGRA16: swapping the two channels is its own inverse.
void ConvertBGRA16ToRGBA16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGB24To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGB24To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGBA32To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGBA32To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertRGBA32To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert16ToRGB24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert16AlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert16NoAlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert24ToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert32ToRGB24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert32AlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void Convert32NoAlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertBUMPLUMA32To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertBUMPLUMA32To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertBUMP16To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
void ConvertBUMP16To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
);
bool NiIsPowerOf2(uint32_t uiI);
