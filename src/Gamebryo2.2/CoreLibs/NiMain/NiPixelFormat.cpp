#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

NiPixelFormat NiPixelFormat_PAL4{
    {},
    1,
    4,
    NiPixelFormat::FORMAT_PAL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_INDEX, NiPixelFormat::REP_INDEX, 4, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_PALA4{
    {},
    1,
    4,
    NiPixelFormat::FORMAT_PALALPHA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_INDEX, NiPixelFormat::REP_INDEX, 4, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_PAL8{
    {},
    1,
    8,
    NiPixelFormat::FORMAT_PAL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_INDEX, NiPixelFormat::REP_INDEX, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_PALA8{
    {},
    1,
    8,
    NiPixelFormat::FORMAT_PALALPHA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_INDEX, NiPixelFormat::REP_INDEX, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_RGBA32{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true}}
};
NiPixelFormat NiPixelFormat_RGB24{
    {},
    1,
    24,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_RGBA16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 4, true}}
};
NiPixelFormat NiPixelFormat::ARGB16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 4, true}}
};
NiPixelFormat NiPixelFormat_BGRA16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 1, true}}
};
NiPixelFormat NiPixelFormat_BUMP16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_BUMP,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_BUMPLUMA32{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_BUMPLUMA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 8},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 8}}
};
NiPixelFormat NiPixelFormat_DXT1{
    {},
    1,
    0,
    NiPixelFormat::FORMAT_DXT1,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_COMPRESSED, NiPixelFormat::REP_COMPRESSED, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_DXT3{
    {},
    1,
    0,
    NiPixelFormat::FORMAT_DXT3,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_COMPRESSED, NiPixelFormat::REP_COMPRESSED, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_DXT5{
    {},
    1,
    0,
    NiPixelFormat::FORMAT_DXT5,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_COMPRESSED, NiPixelFormat::REP_COMPRESSED, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_RGBA128{
    {},
    1,
    128,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FLOAT, 32, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FLOAT, 32, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FLOAT, 32, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FLOAT, 32, true}}
};
NiPixelFormat NiPixelFormat_RGBA64{
    {},
    1,
    64,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_HALF, 16, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_HALF, 16, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_HALF, 16, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_HALF, 16, true}}
};
NiPixelFormat NiPixelFormat_R32{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FLOAT, 32, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_RG32{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_TWO_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_HALF, 16, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_HALF, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_RG64{
    {},
    1,
    64,
    NiPixelFormat::FORMAT_TWO_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FLOAT, 32, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FLOAT, 32, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_R16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_HALF, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_I8{
    {},
    1,
    8,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_INTENSITY, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_A8{
    {},
    1,
    8,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::BGR233{
    {},
    1,
    10,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 2, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 3, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 3, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 2, true}}
};
NiPixelFormat NiPixelFormat::BGRA2338{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 2, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 3, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 3, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true}}
};
NiPixelFormat NiPixelFormat::RGB555{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 1, true}}
};
NiPixelFormat NiPixelFormat_BGR555{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 1, true}}
};
NiPixelFormat NiPixelFormat_BGR565{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 6, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::RGBA5551{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 1, true}}
};
NiPixelFormat NiPixelFormat_BGRA5551{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 1, true}}
};
NiPixelFormat NiPixelFormat::BGRX5551{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 1, true}}
};
NiPixelFormat NiPixelFormat_BGRA4444{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 4, true}}
};
NiPixelFormat NiPixelFormat::BGRX4444{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_FIXED, 4, true}}
};
NiPixelFormat NiPixelFormat::BGR888{
    {},
    1,
    24,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_BGRA8888{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true}}
};
NiPixelFormat NiPixelFormat_BGRX8888{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 8, true}}
};
NiPixelFormat NiPixelFormat::RGBX8888{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGB,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 8, true}}
};
NiPixelFormat NiPixelFormat::RGBA1010102{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 2, true}}
};
NiPixelFormat NiPixelFormat::BGRA1010102{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 2, true}}
};
NiPixelFormat NiPixelFormat::RGBA1010102F{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_RED, NiPixelFormat::REP_FLOAT, 10, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FLOAT, 10, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FLOAT, 10, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 2, true}}
};
NiPixelFormat NiPixelFormat::GR1616{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_TWO_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 16, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat_BUMPLUMA556{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_BUMPLUMA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 5, true},
     {NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 6},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN}}
};
NiPixelFormat NiPixelFormat::PA88{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_TWO_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_INDEX, NiPixelFormat::REP_INDEX, 8, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::L8{
    {},
    1,
    8,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::L16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::LA88{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_TWO_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::LA44{
    {},
    1,
    8,
    NiPixelFormat::FORMAT_TWO_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::BUMPWQ8888{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_BUMP,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_OFFSET_W, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_OFFSET_Q, NiPixelFormat::REP_FIXED, 8, true}}
};
NiPixelFormat NiPixelFormat::BUMP1616{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_BUMP,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 16, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::BUMPW101111{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_BUMP,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 11, true},
     {NiPixelFormat::COMP_OFFSET_W, NiPixelFormat::REP_FIXED, 11, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::BUMPLUMAX8888{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_BUMPLUMA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_LUMA, NiPixelFormat::REP_FIXED, 8},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 8}}
};
NiPixelFormat NiPixelFormat::BUMPWA1010102{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_BUMP,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_OFFSET_U, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_OFFSET_V, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_OFFSET_W, NiPixelFormat::REP_FIXED, 10, true},
     {NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 2, true}}
};
NiPixelFormat NiPixelFormat::RENDERERSPECIFICCOMPRESSED{
    {},
    1,
    0,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::RENDERERSPECIFIC16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::RENDERERSPECIFIC32{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 32, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::RENDERERSPECIFIC64{
    {},
    1,
    64,
    NiPixelFormat::FORMAT_ONE_CHANNEL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_UNKNOWN, 64, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::DEPTH16{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FIXED, 16, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::DEPTH32{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FIXED, 32, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::STENCILDEPTH115{
    {},
    1,
    16,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_STENCIL, NiPixelFormat::REP_FIXED, 1, true},
     {NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FIXED, 15, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::STENCILDEPTH824{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_STENCIL, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FIXED, 24, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::STENCILDEPTH824F{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_STENCIL, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FLOAT, 24, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::XDEPTH824{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FIXED, 24, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};
NiPixelFormat NiPixelFormat::STENCILXDEPTH4424{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_DEPTH_STENCIL,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_STENCIL, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_PADDING, NiPixelFormat::REP_FIXED, 4, true},
     {NiPixelFormat::COMP_DEPTH, NiPixelFormat::REP_FIXED, 24, true},
     {NiPixelFormat::COMP_EMPTY, NiPixelFormat::REP_UNKNOWN, 0, true}}
};

NiPixelFormat::NiPixelFormat()
  : m_uFlags(1), m_ucBitsPerPixel(8), m_eFormat(FORMAT_PAL), m_eTiling(TILE_NONE), m_uiRendererHint(0xFFFFFFFF),
    m_uiExtraData(0), m_akComponents{
                          {COMP_INDEX, REP_INDEX, 8, true},
                          {COMP_EMPTY, REP_UNKNOWN, 0, true},
                          {COMP_EMPTY, REP_UNKNOWN, 0, true},
                          {COMP_EMPTY, REP_UNKNOWN, 0, true}
                      } {}

void NiPixelFormat::SaveBinary(NiStream& kStream) {
    uint32_t uiSize;
    // EXE-INLINED none: NiStreamSaveEnum
    uiSize = 4;
    kStream.Ostr()->BinaryWrite(&m_eFormat, sizeof(m_eFormat), &uiSize, 1);
    NiStreamSaveBinary(kStream, m_ucBitsPerPixel);
    NiStreamSaveBinary(kStream, m_uiRendererHint);
    NiStreamSaveBinary(kStream, m_uiExtraData);
    NiStreamSaveBinary(kStream, m_uFlags);
    // EXE-INLINED none: NiStreamSaveEnum
    uiSize = 4;
    kStream.Ostr()->BinaryWrite(&m_eTiling, sizeof(m_eTiling), &uiSize, 1);
    for (uint32_t i = 0; i < NUM_COMPS; ++i) {
        NiComponentSpec& kComp = m_akComponents[i];
        // EXE-INLINED none: NiStreamSaveEnum
        uiSize = 4;
        kStream.Ostr()->BinaryWrite(&kComp.m_eComponent, sizeof(kComp.m_eComponent), &uiSize, 1);
        // EXE-INLINED none: NiStreamSaveEnum
        uiSize = 4;
        kStream.Ostr()->BinaryWrite(&kComp.m_eRepresentation, sizeof(kComp.m_eRepresentation), &uiSize, 1);
        NiStreamSaveBinary(kStream, kComp.m_ucBitsPerComponent);
        // Copies the flag to a local byte and writes that.
        uint8_t kBool = kComp.m_bSigned;
        NiStreamSaveBinary(kStream, kBool);
    }
}

int32_t NiPixelFormat::GetCompressed() const {
    int32_t m_eFormat;
    m_eFormat = (int32_t)this->m_eFormat;
    return (int32_t)((uint8_t)((m_eFormat >= 4) && (m_eFormat <= 6)));
}

void NiPixelFormat::ConvertOldPixelFormat(NiPixelFormat::Format eFormat, uint8_t ucBPP, uint32_t* auiColorMasks) {
    m_ucBitsPerPixel = ucBPP;
    m_eFormat = eFormat;
    m_uiRendererHint = -1;
    m_uiExtraData = 0;
    m_uFlags = 1;
    m_eTiling = NiPixelFormat::TILE_NONE;
    switch (eFormat) {
        case NiPixelFormat::FORMAT_RGB: {
            if (ucBPP == 16) {
                if (*auiColorMasks != 0xF800) {
                    goto LABEL_19;
                }
                if (auiColorMasks[1] != 2016) {
                    goto LABEL_19;
                }
                if (auiColorMasks[2] != 31) {
                    goto LABEL_19;
                }
                auiColorMasks[3];
                if (auiColorMasks[3]) {
                    goto LABEL_19;
                }
                *this = NiPixelFormat_BGR565;
            }
            else {
                if (ucBPP != 24) {
                    goto LABEL_19;
                }
                if (*auiColorMasks == 255 &&
                    (auiColorMasks[1] == 0xFF00 &&
                     (auiColorMasks[2] == 0xFF0000 && (auiColorMasks[3], !auiColorMasks[3])))) {
                    *this = NiPixelFormat_RGB24;
                }
                else {
                    if (*auiColorMasks != 0xFF0000) {
                        goto LABEL_19;
                    }
                    if (auiColorMasks[1] != 0xFF00) {
                        goto LABEL_19;
                    }
                    if (auiColorMasks[2] != 255) {
                        goto LABEL_19;
                    }
                    auiColorMasks[3];
                    if (auiColorMasks[3]) {
                        goto LABEL_19;
                    }
                    *this = NiPixelFormat::BGR888;
                }
            }
            return;
        }
        case NiPixelFormat::FORMAT_RGBA: {
            if (ucBPP == 16) {
                uint32_t v5;
                v5 = *auiColorMasks;
                if (*auiColorMasks == 31 &&
                    (auiColorMasks[1] == 992 && (auiColorMasks[2] == 0x7C00 && auiColorMasks[3] == 0x8000))) {
                    *this = NiPixelFormat_RGBA16;
                    return;
                }
                if (v5 == 0x7C00 &&
                    (auiColorMasks[1] == 992 && (auiColorMasks[2] == 31 && auiColorMasks[3] == 0x8000))) {
                    *this = NiPixelFormat_BGRA16;
                    return;
                }
                if (v5 == 3840 && (auiColorMasks[1] == 240 && (auiColorMasks[2] == 15 && auiColorMasks[3] == 0xF000))) {
                    *this = NiPixelFormat_BGRA4444;
                    return;
                }
            LABEL_19:;
                m_eFormat = NiPixelFormat::FORMAT_UNKNOWN;
                return;
            }
            if (ucBPP != 32) {
                goto LABEL_19;
            }
            if (*auiColorMasks == 0xFF0000 &&
                (auiColorMasks[1] == 0xFF00 && (auiColorMasks[2] == 255 && auiColorMasks[3] == -0x1000000))) {
                *this = NiPixelFormat_BGRA8888;
            }
            else {
                *this = NiPixelFormat_RGBA32;
            }
            return;
        }
        case NiPixelFormat::FORMAT_PAL: {
            if (ucBPP != 8) {
                goto LABEL_52;
            }
            *this = NiPixelFormat_PAL8;
            return;
        }
        case NiPixelFormat::FORMAT_PALALPHA: {
            if (ucBPP != 8) {
                goto LABEL_52;
            }
            *this = NiPixelFormat_PALA8;
            return;
        }
        case NiPixelFormat::FORMAT_DXT1: {
            *this = NiPixelFormat_DXT1;
            return;
        }
        case NiPixelFormat::FORMAT_DXT3: {
            *this = NiPixelFormat_DXT3;
            return;
        }
        case NiPixelFormat::FORMAT_DXT5: {
            *this = NiPixelFormat_DXT5;
            return;
        }
        case NiPixelFormat::FORMAT_BUMP: {
            *this = NiPixelFormat_BUMP16;
            return;
        }
        case NiPixelFormat::FORMAT_BUMPLUMA: {
            *this = NiPixelFormat_BUMPLUMA32;
            return;
        }
        default: {
        LABEL_52:;
            m_eFormat = NiPixelFormat::FORMAT_UNKNOWN;
            return;
        }
    }
}

// KEEP (niread): every value but the member-direct ones is read into a stack local that a short read leaves
// unwritten and that is then stored (the format, the bits per pixel, the tiling, each component's enums and its
// signed flag). The exe also reuses its kStream argument slot for the mask pointer, the size words and the
// signed-flag byte, which src cannot express; kBool here is a local of its own.
void NiPixelFormat::LoadBinary(NiStream& kStream) {
    uint32_t uiSize;
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 3, 0, 3)) {
        uint32_t auiColorMasks[4];
        uint32_t uiFormat;
        // EXE-INLINED none: NiStreamLoadEnum
        uiSize = 4;
        kStream.Istr()->BinaryRead(&uiFormat, sizeof(uiFormat), &uiSize, 1);
        m_eFormat = static_cast<Format>(uiFormat);
        for (uint32_t i = 0; i < 4; ++i) {
            NiStreamLoadBinary(kStream, auiColorMasks[i]);
        }
        uint32_t uiBPP;
        NiStreamLoadBinary(kStream, uiBPP);
        m_ucBitsPerPixel = static_cast<uint8_t>(uiBPP);
        // The two old fast-compare words go into one slot and are dropped.
        uint32_t uiOldFastCompare;
        NiStreamLoadBinary(kStream, uiOldFastCompare);
        NiStreamLoadBinary(kStream, uiOldFastCompare);
        if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 11)) {
            // The tiling is read and dropped.
            uint32_t uiTiling;
            // EXE-INLINED none: NiStreamLoadEnum
            uiSize = 4;
            kStream.Istr()->BinaryRead(&uiTiling, sizeof(uiTiling), &uiSize, 1);
        }
        ConvertOldPixelFormat(m_eFormat, m_ucBitsPerPixel, auiColorMasks);
    }
    else {
        uint32_t uiFormat;
        // EXE-INLINED none: NiStreamLoadEnum
        uiSize = 4;
        kStream.Istr()->BinaryRead(&uiFormat, sizeof(uiFormat), &uiSize, 1);
        m_eFormat = static_cast<Format>(uiFormat);
        NiStreamLoadBinary(kStream, m_ucBitsPerPixel);
        NiStreamLoadBinary(kStream, m_uiRendererHint);
        NiStreamLoadBinary(kStream, m_uiExtraData);
        NiStreamLoadBinary(kStream, m_uFlags);
        uint32_t uiTiling;
        // EXE-INLINED none: NiStreamLoadEnum
        uiSize = 4;
        kStream.Istr()->BinaryRead(&uiTiling, sizeof(uiTiling), &uiSize, 1);
        m_eTiling = static_cast<Tiling>(uiTiling);
        for (uint32_t i = 0; i < NUM_COMPS; ++i) {
            NiComponentSpec& kComp = m_akComponents[i];
            uint32_t uiComponent;
            // EXE-INLINED none: NiStreamLoadEnum
            uiSize = 4;
            kStream.Istr()->BinaryRead(&uiComponent, sizeof(uiComponent), &uiSize, 1);
            kComp.m_eComponent = static_cast<Component>(uiComponent);
            uint32_t uiRepresentation;
            // EXE-INLINED none: NiStreamLoadEnum
            uiSize = 4;
            kStream.Istr()->BinaryRead(&uiRepresentation, sizeof(uiRepresentation), &uiSize, 1);
            kComp.m_eRepresentation = static_cast<Representation>(uiRepresentation);
            NiStreamLoadBinary(kStream, kComp.m_ucBitsPerComponent);
            uint8_t kBool;
            NiStreamLoadBinary(kStream, kBool);
            kComp.m_bSigned = kBool != 0;
        }
    }
}

NiPixelFormat NiPixelFormat::CreateFromRGBAMasks(
    uint32_t ucBitsPerPixel,
    uint32_t uiMaskRed,
    uint32_t uiMaskGreen,
    uint32_t uiMaskBlue,
    uint32_t uiMaskAlpha
) {
    NiPixelFormat kFormat;
    // SetLittleEndian(true) sets flag bit 0.
    kFormat.m_uFlags |= 1;
    kFormat.m_ucBitsPerPixel = static_cast<uint8_t>(ucBitsPerPixel);
    // INVALID_RENDERER_HINT.
    kFormat.m_uiRendererHint = 0xFFFFFFFF;
    kFormat.m_eTiling = NiPixelFormat::TILE_NONE;
    kFormat.m_eFormat = (uiMaskAlpha != 0) ? NiPixelFormat::FORMAT_RGBA : NiPixelFormat::FORMAT_RGB;

    NiPixelFormat::Component aeComps[4];
    uint8_t aucSizes[4];
    uint32_t auiMasks[4];

    auiMasks[0] = uiMaskRed;
    aeComps[0] = NiPixelFormat::COMP_RED;
    aucSizes[0] = static_cast<uint8_t>(CalcMaskSize(uiMaskRed));

    auiMasks[1] = uiMaskGreen;
    aeComps[1] = NiPixelFormat::COMP_GREEN;
    aucSizes[1] = static_cast<uint8_t>(CalcMaskSize(uiMaskGreen));

    auiMasks[2] = uiMaskBlue;
    aeComps[2] = NiPixelFormat::COMP_BLUE;
    aucSizes[2] = static_cast<uint8_t>(CalcMaskSize(uiMaskBlue));

    auiMasks[3] = uiMaskAlpha;
    aeComps[3] = NiPixelFormat::COMP_ALPHA;
    aucSizes[3] = static_cast<uint8_t>(CalcMaskSize(uiMaskAlpha));

    for (int32_t i = 0; i < 3; i++) {
        if (auiMasks[i] != 0) {
            for (int32_t j = i + 1; j < 3; j++) {
                if (auiMasks[i] > auiMasks[j]) {
                    uint32_t uiTempMask = auiMasks[j];
                    uint8_t ucTempSize = aucSizes[j];
                    NiPixelFormat::Component eTempComp = aeComps[j];
                    auiMasks[j] = auiMasks[i];
                    aucSizes[j] = aucSizes[i];
                    aeComps[j] = aeComps[i];
                    auiMasks[i] = uiTempMask;
                    aucSizes[i] = ucTempSize;
                    aeComps[i] = eTempComp;
                }
            }
        }
    }

    // Fills each component the way SetComponent does, storing the size, signed, the representation, then the component,
    // in that order.
    uint32_t uiComponent = 0;
    for (uint32_t ui = 0; ui < 4; ui++) {
        if (auiMasks[ui] != 0) {
            NiComponentSpec& kSpec = kFormat.m_akComponents[uiComponent];
            kSpec.m_ucBitsPerComponent = aucSizes[ui];
            kSpec.m_bSigned = true;
            kSpec.m_eRepresentation = NiPixelFormat::REP_FIXED;
            kSpec.m_eComponent = aeComps[ui];
            uiComponent++;
        }
    }

    for (uint32_t uj = uiComponent; uj < 4; uj++) {
        NiComponentSpec& kSpec = kFormat.m_akComponents[uiComponent];
        kSpec.m_bSigned = true;
        kSpec.m_ucBitsPerComponent = 0;
        kSpec.m_eRepresentation = NiPixelFormat::REP_UNKNOWN;
        kSpec.m_eComponent = NiPixelFormat::COMP_EMPTY;
        uiComponent++;
    }

    return kFormat;
}
