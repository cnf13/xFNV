#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

class NiPixelFormat;

// Conversions between Gamebryo pixel formats and Direct3D formats. Only some of the members are declared.
class NiDX9PixelFormat {
public:
	// Fills apkFormat from aeD3DFmt and returns it.
	static NiPixelFormat* InitFromD3DFormat(D3DFORMAT aeD3DFmt, NiPixelFormat* apkFormat);

	// The Direct3D format of akFormat.
	static D3DFORMAT DetermineD3DFormat(const NiPixelFormat& akFormat);

	// A new pixel format describing aeD3DFmt.
	static NiPixelFormat* CreateFromD3DFormat(D3DFORMAT aeD3DFmt);
};
