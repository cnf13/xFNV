#pragma once

// What the stand-in's files share: the game's own Direct3D 9 / D3DX declarations (the types and the exported
// prototypes the game calls), the pixel formats the texture functions convert between, the DXT codec and the box
// filter. Pixels travel between formats as one float RGBA vector each, as d3dx9 converts them.

#include <cstdint>
#include <cstdio>
#include <vector>

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

namespace d3dx {

// d3dx9's own error codes (d3dx9.h): MAKE_DDHRESULT(2900 + n).
constexpr HRESULT D3DXERR_INVALIDDATA = static_cast<HRESULT>(0x88760B59);

// One line on stderr, for what the stand-in refuses (a shader compile, an unsupported texture).
void Log(const char* apFormat, ...);

struct Color {
	float r, g, b, a;
};

// A Direct3D format the stand-in reads and writes: a packed RGB/luminance/alpha format by its bit masks, or a DXT
// block format.
struct FormatInfo {
	uint32_t format;
	// Bits per pixel; for a block format, bytes per 4x4 block.
	uint32_t bits;
	bool block;
	// The masks of the packed formats (a zero mask: the channel is absent; luminance sets r alone and copies it to
	// g and b).
	uint32_t rMask, gMask, bMask, aMask;
	bool luminance;
};

// The format's description, or nullptr when the stand-in has no reader or writer for it.
const FormatInfo* FindFormat(uint32_t aFormat);

// The bytes of one row of pixels (a row of 4x4 blocks for a block format) and the row count of a level.
uint32_t RowBytes(const FormatInfo& arInfo, uint32_t auiWidth);
uint32_t RowCount(const FormatInfo& arInfo, uint32_t auiHeight);

// A level of pixels as float colors, row by row.
struct Image {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	std::vector<Color> pixels;
	Color& at(uint32_t x, uint32_t y, uint32_t z = 0) { return pixels[(z * height + y) * width + x]; }
	const Color& at(uint32_t x, uint32_t y, uint32_t z = 0) const { return pixels[(z * height + y) * width + x]; }
};

// One 2D slice between memory in a format (rows apPitch bytes apart) and an Image slice z. abDither: d3dx9's ordered
// dither on the packed formats' quantization (D3DX_FILTER_DITHER), else rounding.
void DecodeSlice(const FormatInfo& arInfo, const uint8_t* apSrc, uint32_t auiPitch, Image& arImage, uint32_t auiZ);
void EncodeSlice(const FormatInfo& arInfo, const Image& arImage, uint32_t auiZ, uint8_t* apDst, uint32_t auiPitch,
	bool abDither);

// d3dx9's box filter: each texel of the next level is the mean of the 2x2(x2) texels it covers.
Image BoxDown(const Image& arSrc);

// An ID3DXBuffer holding a copy of the bytes, or nullptr when out of memory.
ID3DXBuffer* NewBuffer(uint32_t auiBytes, const void* apData = nullptr);

} // namespace d3dx
