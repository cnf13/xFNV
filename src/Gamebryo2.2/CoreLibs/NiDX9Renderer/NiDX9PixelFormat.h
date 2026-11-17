#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"

class NiPixelFormat;

// Conversions between Gamebryo pixel formats and Direct3D formats. Not in the Xbox pdb.
// Only the members called so far; the rest is not reconstructed yet.
class NiDX9PixelFormat {
public:
	// cdecl, 0xE7C1E0. Fills apkFormat from aeD3DFmt and returns it.
	static NiPixelFormat* InitFromD3DFormat(D3DFORMAT aeD3DFmt, NiPixelFormat* apkFormat);

	// cdecl, 0xE7BD20. The Direct3D format of akFormat.
	static D3DFORMAT DetermineD3DFormat(const NiPixelFormat& akFormat);

	// A new pixel format describing aeD3DFmt.
	// Inferred type: aeD3DFmt: D3DFORMAT: passed to NiDX9PixelFormat::InitFromD3DFormat 0 at 0xe7c51f
	static NiPixelFormat* CreateFromD3DFormat(D3DFORMAT aeD3DFmt);
};
