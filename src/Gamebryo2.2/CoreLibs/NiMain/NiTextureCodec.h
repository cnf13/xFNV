#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <stdint.h>

class NiPixelData;
class NiTextureCodec;

// DXT block decoder, including the explicit-alpha lookup table built by its constructor.
class NiTextureCodec : public NiMemObject {
public:
    enum : int32_t {
        DEST_PIXEL_STRIDE = 4,
    };

    // The PC constructor installs DecodeColorBlock in this single-address function slot. The implicit object argument
    // is represented explicitly rather than using the host's larger member pointer.
    using BlockDecoder = void (*)(NiTextureCodec* apThis, uint8_t* apDest);

    NiTextureCodec();

    void DecodeColorBlock(uint8_t* pucDest);
    // The block decoders ExpandCompressedData installs; their callers, going through the decoder slots, ignore the
    // return values.
    uint8_t* DecodeT3AlphaBlock(uint8_t* pucAlphas);
    int32_t DecodeT5AlphaBlock(uint8_t* pucAlphas);
    int32_t DecodeColorAlphaBlock(uint8_t* pucDest);
    // Decompresses a DXT1/3/5 pixel data into a new RGBA32 one.
    NiPixelData* ExpandCompressedData(const NiPixelData& kSrc);
    void DecodeBlock(uint8_t* pucDest, uint32_t uiCols, uint32_t uiRows);

    BlockDecoder m_pfnAlphaBlockDecoder;
    BlockDecoder m_pfnColorBlockDecoder;
    const uint8_t* m_pucSrcPtr;
    uint32_t m_uiDestRowStride;
    uint8_t m_aaucExplicitAlphaLookup[256][2];
};
