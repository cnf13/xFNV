#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

NiPixelFormat NiPixelFormat::PAL4{
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
NiPixelFormat NiPixelFormat::PALA4{
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
NiPixelFormat NiPixelFormat::PAL8{
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
NiPixelFormat NiPixelFormat::PALA8{
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
NiPixelFormat NiPixelFormat::RGBA32{
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
NiPixelFormat NiPixelFormat::RGB24{
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
NiPixelFormat NiPixelFormat::RGBA16{
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
NiPixelFormat NiPixelFormat::BGRA16{
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
NiPixelFormat NiPixelFormat::BUMP16{
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
NiPixelFormat NiPixelFormat::BUMPLUMA32{
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
NiPixelFormat NiPixelFormat::DXT1{
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
NiPixelFormat NiPixelFormat::DXT3{
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
NiPixelFormat NiPixelFormat::DXT5{
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
NiPixelFormat NiPixelFormat::RGBA128{
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
NiPixelFormat NiPixelFormat::RGBA64{
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
NiPixelFormat NiPixelFormat::R32{
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
NiPixelFormat NiPixelFormat::RG32{
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
NiPixelFormat NiPixelFormat::RG64{
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
NiPixelFormat NiPixelFormat::R16{
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
NiPixelFormat NiPixelFormat::I8{
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
NiPixelFormat NiPixelFormat::A8{
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
NiPixelFormat NiPixelFormat::BGR555{
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
NiPixelFormat NiPixelFormat::BGR565{
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
NiPixelFormat NiPixelFormat::BGRA5551{
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
NiPixelFormat NiPixelFormat::BGRA4444{
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
NiPixelFormat NiPixelFormat::BGRA8888{
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
NiPixelFormat NiPixelFormat::BGRX8888{
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
NiPixelFormat NiPixelFormat::BUMPLUMA556{
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
  : m_uFlags(LITTLE_ENDIAN_MASK), m_ucBitsPerPixel(8), m_eFormat(FORMAT_PAL), m_eTiling(TILE_NONE),
    m_uiRendererHint(0xFFFFFFFF), m_uiExtraData(0), m_akComponents{
                                                        {COMP_INDEX, REP_INDEX, 8, true},
                                                        {COMP_EMPTY, REP_UNKNOWN, 0, true},
                                                        {COMP_EMPTY, REP_UNKNOWN, 0, true},
                                                        {COMP_EMPTY, REP_UNKNOWN, 0, true}
                                                    } {}

void NiPixelFormat::SaveBinary(NiStream& kStream) {
    NiStreamSaveEnum(kStream, m_eFormat);
    NiStreamSaveBinary(kStream, m_ucBitsPerPixel);
    NiStreamSaveBinary(kStream, m_uiRendererHint);
    NiStreamSaveBinary(kStream, m_uiExtraData);
    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveEnum(kStream, m_eTiling);
    for (uint32_t i = 0; i < NUM_COMPS; ++i) {
        NiComponentSpec& kComp = m_akComponents[i];
        NiStreamSaveEnum(kStream, kComp.m_eComponent);
        NiStreamSaveEnum(kStream, kComp.m_eRepresentation);
        NiStreamSaveBinary(kStream, kComp.m_ucBitsPerComponent);
        // Copies the flag to a local byte and writes that.
        uint8_t kBool = kComp.m_bSigned;
        NiStreamSaveBinary(kStream, kBool);
    }
}

int32_t NiPixelFormat::GetCompressed() const {
    return m_eFormat >= FORMAT_DXT1 && m_eFormat <= FORMAT_DXT5;
}

void NiPixelFormat::ConvertOldPixelFormat(NiPixelFormat::Format eFormat, uint8_t ucBPP, uint32_t* auiColorMasks) {
    m_ucBitsPerPixel = ucBPP;
    m_eFormat = eFormat;
    m_uiRendererHint = -1;
    m_uiExtraData = 0;
    m_uFlags = LITTLE_ENDIAN_MASK;
    m_eTiling = NiPixelFormat::TILE_NONE;
    switch (eFormat) {
        case NiPixelFormat::FORMAT_RGB: {
            if (ucBPP == 16) {
                if (*auiColorMasks != 0xF800) {
                    break;
                }
                if (auiColorMasks[1] != 2016) {
                    break;
                }
                if (auiColorMasks[2] != 31) {
                    break;
                }
                auiColorMasks[3];
                if (auiColorMasks[3]) {
                    break;
                }
                *this = NiPixelFormat::BGR565;
            }
            else {
                if (ucBPP != 24) {
                    break;
                }
                if (*auiColorMasks == 255 &&
                    (auiColorMasks[1] == 0xFF00 &&
                     (auiColorMasks[2] == 0xFF0000 && (auiColorMasks[3], !auiColorMasks[3])))) {
                    *this = NiPixelFormat::RGB24;
                }
                else {
                    if (*auiColorMasks != 0xFF0000) {
                        break;
                    }
                    if (auiColorMasks[1] != 0xFF00) {
                        break;
                    }
                    if (auiColorMasks[2] != 255) {
                        break;
                    }
                    auiColorMasks[3];
                    if (auiColorMasks[3]) {
                        break;
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
                    *this = NiPixelFormat::RGBA16;
                    return;
                }
                if (v5 == 0x7C00 &&
                    (auiColorMasks[1] == 992 && (auiColorMasks[2] == 31 && auiColorMasks[3] == 0x8000))) {
                    *this = NiPixelFormat::BGRA16;
                    return;
                }
                if (v5 == 3840 && (auiColorMasks[1] == 240 && (auiColorMasks[2] == 15 && auiColorMasks[3] == 0xF000))) {
                    *this = NiPixelFormat::BGRA4444;
                    return;
                }
                break;
            }
            if (ucBPP != 32) {
                break;
            }
            if (*auiColorMasks == 0xFF0000 &&
                (auiColorMasks[1] == 0xFF00 && (auiColorMasks[2] == 255 && auiColorMasks[3] == -0x1000000))) {
                *this = NiPixelFormat::BGRA8888;
            }
            else {
                *this = NiPixelFormat::RGBA32;
            }
            return;
        }
        case NiPixelFormat::FORMAT_PAL: {
            if (ucBPP != 8) {
                break;
            }
            *this = NiPixelFormat::PAL8;
            return;
        }
        case NiPixelFormat::FORMAT_PALALPHA: {
            if (ucBPP != 8) {
                break;
            }
            *this = NiPixelFormat::PALA8;
            return;
        }
        case NiPixelFormat::FORMAT_DXT1: {
            *this = NiPixelFormat::DXT1;
            return;
        }
        case NiPixelFormat::FORMAT_DXT3: {
            *this = NiPixelFormat::DXT3;
            return;
        }
        case NiPixelFormat::FORMAT_DXT5: {
            *this = NiPixelFormat::DXT5;
            return;
        }
        case NiPixelFormat::FORMAT_BUMP: {
            *this = NiPixelFormat::BUMP16;
            return;
        }
        case NiPixelFormat::FORMAT_BUMPLUMA: {
            *this = NiPixelFormat::BUMPLUMA32;
            return;
        }
        default: {
            break;
        }
    }
    m_eFormat = NiPixelFormat::FORMAT_UNKNOWN;
}

// KEEP (niread): every value but the member-direct ones is read into a stack local that a short read leaves
// unwritten and that is then stored (the format, the bits per pixel, the tiling, each component's enums and its
// signed flag). The exe also reuses its kStream argument slot for the mask pointer, the size words and the
// signed-flag byte, which src cannot express; kBool here is a local of its own.
void NiPixelFormat::LoadBinary(NiStream& kStream) {
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 3, 0, 3)) {
        uint32_t auiColorMasks[4];
        NiStreamLoadEnum(kStream, m_eFormat);
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
            Tiling eTiling;
            NiStreamLoadEnum(kStream, eTiling);
        }
        ConvertOldPixelFormat(m_eFormat, m_ucBitsPerPixel, auiColorMasks);
    }
    else {
        NiStreamLoadEnum(kStream, m_eFormat);
        NiStreamLoadBinary(kStream, m_ucBitsPerPixel);
        NiStreamLoadBinary(kStream, m_uiRendererHint);
        NiStreamLoadBinary(kStream, m_uiExtraData);
        NiStreamLoadBinary(kStream, m_uFlags);
        NiStreamLoadEnum(kStream, m_eTiling);
        for (uint32_t i = 0; i < NUM_COMPS; ++i) {
            NiComponentSpec& kComp = m_akComponents[i];
            NiStreamLoadEnum(kStream, kComp.m_eComponent);
            NiStreamLoadEnum(kStream, kComp.m_eRepresentation);
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
    kFormat.m_uFlags |= NiPixelFormat::LITTLE_ENDIAN_MASK;
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
