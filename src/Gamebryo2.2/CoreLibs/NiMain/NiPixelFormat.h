#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

class NiStream;

// How the pixels of an image or texture are stored: the format, the bit depth and the meaning of up to four
// components.
class NiPixelFormat : public NiMemObject {
public:
    enum Format : uint32_t {
        FORMAT_RGB = 0,
        FORMAT_RGBA = 1,
        FORMAT_PAL = 2,
        FORMAT_PALALPHA = 3,
        FORMAT_DXT1 = 4,
        FORMAT_DXT3 = 5,
        FORMAT_DXT5 = 6,
        FORMAT_RGB24NONINTERLEAVED = 7,
        FORMAT_BUMP = 8,
        FORMAT_BUMPLUMA = 9,
        FORMAT_RENDERERSPECIFIC = 10,
        FORMAT_ONE_CHANNEL = 11,
        FORMAT_TWO_CHANNEL = 12,
        FORMAT_THREE_CHANNEL = 13,
        FORMAT_FOUR_CHANNEL = 14,
        FORMAT_DEPTH_STENCIL = 15,
        FORMAT_UNKNOWN = 16,
        FORMAT_MAX = 17,
    };

    enum Component : uint32_t {
        COMP_RED = 0,
        COMP_GREEN = 1,
        COMP_BLUE = 2,
        COMP_ALPHA = 3,
        COMP_COMPRESSED = 4,
        COMP_OFFSET_U = 5,
        COMP_OFFSET_V = 6,
        COMP_OFFSET_W = 7,
        COMP_OFFSET_Q = 8,
        COMP_LUMA = 9,
        COMP_HEIGHT = 10,
        COMP_VECTOR_X = 11,
        COMP_VECTOR_Y = 12,
        COMP_VECTOR_Z = 13,
        COMP_PADDING = 14,
        COMP_INTENSITY = 15,
        COMP_INDEX = 16,
        COMP_DEPTH = 17,
        COMP_STENCIL = 18,
        COMP_EMPTY = 19,
        COMP_MAX = 20,
        NUM_COMPS = 4,
    };

    enum Tiling : uint32_t {
        TILE_NONE = 0,
        TILE_XENON = 1,
        TILE_MAX = 2,
    };

    enum Representation : uint32_t {
        REP_FIXED = 0,
        REP_HALF = 1,
        REP_FLOAT = 2,
        REP_INDEX = 3,
        REP_COMPRESSED = 4,
        REP_UNKNOWN = 5,
        REP_MAX = 6,
    };

    // One component: what it holds, how it is encoded and its width.
    class NiComponentSpec {
    public:
        Component m_eComponent;
        Representation m_eRepresentation;
        uint8_t m_ucBitsPerComponent;
        bool m_bSigned;
    };

    NiPixelFormat();

    // Aggregate-style initialisation (`{}` for the NiMemObject base): every field in declaration order.
    NiPixelFormat(
        NiMemObject,
        uint8_t uFlags,
        uint8_t ucBitsPerPixel,
        Format eFormat,
        Tiling eTiling,
        uint32_t uiRendererHint,
        uint32_t uiExtraData,
        const NiComponentSpec (&akComponents)[NUM_COMPS]
    )
      : m_uFlags(uFlags), m_ucBitsPerPixel(ucBitsPerPixel), m_eFormat(eFormat), m_eTiling(eTiling),
        m_uiRendererHint(uiRendererHint), m_uiExtraData(uiExtraData) {
        for (uint32_t i = 0; i < NUM_COMPS; ++i) {
            m_akComponents[i] = akComponents[i];
        }
    }

    uint32_t GetMask(Component eComp) const;
    uint8_t GetShift(Component eComp) const;
    uint8_t GetBits(Component eComp) const;

    bool operator==(const NiPixelFormat& kOther) const;
    bool operator!=(const NiPixelFormat& kOther) const;
    bool GetComponent(
        uint32_t uiWhichComponent,
        Component& areComponent,
        Representation& eRep,
        uint8_t& ucBPC,
        bool& bSigned
    ) const;
    // The count of components before the first COMP_EMPTY.
    uint32_t GetNumComponents() const;
    // Writes the format to the stream.
    void SaveBinary(NiStream& kStream);
    int32_t GetCompressed() const;
    int32_t GetPalettized() const;
    void ConvertOldPixelFormat(NiPixelFormat::Format eFormat, uint8_t ucBPP, uint32_t* auiColorMasks);
    void LoadBinary(NiStream& kStream);

    // ucBitsPerPixel is an unsigned char value passed as uint32_t: NiDDSReader::ReadHeader passes the whole 32-bit
    // value it read, and only its low byte is used.
    static NiPixelFormat* CreateFromRGBAMasks(
        NiPixelFormat* apResult,
        uint32_t ucBitsPerPixel,
        uint32_t uiMaskRed,
        uint32_t uiMaskGreen,
        uint32_t uiMaskBlue,
        uint32_t uiMaskAlpha
    );

    // The count of set bits.
    static uint32_t CalcMaskSize(uint32_t uiMask) {
        uint32_t uiTotal = 0;
        while (uiMask != 0) {
            uiTotal += (uiMask & 1);
            uiMask = uiMask >> 1;
        }
        return uiTotal;
    }

    uint8_t m_uFlags;
    uint8_t m_ucBitsPerPixel;
    Format m_eFormat;
    Tiling m_eTiling;
    uint32_t m_uiRendererHint;
    uint32_t m_uiExtraData;
    NiComponentSpec m_akComponents[NUM_COMPS];

    static NiPixelFormat ARGB16;
    static NiPixelFormat BGR233;
    static NiPixelFormat BGRA2338;
    static NiPixelFormat RGB555;
    static NiPixelFormat RGBA5551;
    static NiPixelFormat BGRX5551;
    static NiPixelFormat BGRX4444;
    static NiPixelFormat BGR888;
    static NiPixelFormat RGBX8888;
    static NiPixelFormat RGBA1010102;
    static NiPixelFormat BGRA1010102;
    static NiPixelFormat RGBA1010102F;
    static NiPixelFormat GR1616;
    static NiPixelFormat PA88;
    static NiPixelFormat L8;
    static NiPixelFormat L16;
    static NiPixelFormat LA88;
    static NiPixelFormat LA44;
    static NiPixelFormat BUMPWQ8888;
    static NiPixelFormat BUMP1616;
    static NiPixelFormat BUMPW101111;
    static NiPixelFormat BUMPLUMAX8888;
    static NiPixelFormat BUMPWA1010102;
    static NiPixelFormat RENDERERSPECIFICCOMPRESSED;
    static NiPixelFormat RENDERERSPECIFIC16;
    static NiPixelFormat RENDERERSPECIFIC32;
    static NiPixelFormat RENDERERSPECIFIC64;
    static NiPixelFormat DEPTH16;
    static NiPixelFormat DEPTH32;
    static NiPixelFormat STENCILDEPTH115;
    static NiPixelFormat STENCILDEPTH824;
    static NiPixelFormat STENCILDEPTH824F;
    static NiPixelFormat XDEPTH824;
    static NiPixelFormat STENCILXDEPTH4424;
    static NiPixelFormat RGBA8888;
};

// The standard formats (NiPixelFormat's static members). Globals here because the class is incomplete inside its own
// body.
extern NiPixelFormat NiPixelFormat_BGR555;
extern NiPixelFormat NiPixelFormat_BGR565;
extern NiPixelFormat NiPixelFormat_BGRX8888;
extern NiPixelFormat NiPixelFormat_BGRA5551;
extern NiPixelFormat NiPixelFormat_BGRA4444;
extern NiPixelFormat NiPixelFormat_BGRA8888;
extern NiPixelFormat NiPixelFormat_DXT1;
extern NiPixelFormat NiPixelFormat_DXT3;
extern NiPixelFormat NiPixelFormat_DXT5;
extern NiPixelFormat NiPixelFormat_BUMP16;
extern NiPixelFormat NiPixelFormat_BUMPLUMA556;
extern NiPixelFormat NiPixelFormat_BUMPLUMA32;
extern NiPixelFormat NiPixelFormat_I8;
extern NiPixelFormat NiPixelFormat_A8;
extern NiPixelFormat NiPixelFormat_R16;
extern NiPixelFormat NiPixelFormat_RG32;
extern NiPixelFormat NiPixelFormat_RGBA64;
extern NiPixelFormat NiPixelFormat_R32;
extern NiPixelFormat NiPixelFormat_RG64;
extern NiPixelFormat NiPixelFormat_RGBA128;
extern NiPixelFormat NiPixelFormat_PAL4;
extern NiPixelFormat NiPixelFormat_PALA4;
extern NiPixelFormat NiPixelFormat_PAL8;
extern NiPixelFormat NiPixelFormat_PALA8;
extern NiPixelFormat NiPixelFormat_RGBA32;
extern NiPixelFormat NiPixelFormat_RGB24;
extern NiPixelFormat NiPixelFormat_RGBA16;
extern NiPixelFormat NiPixelFormat_BGRA16;
