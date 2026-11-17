#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;
class NiRenderedCubeMap;

// The DX9 renderer's data of a rendered cube map. Not in the Xbox pdb.
// Only the members called so far; the rest is not reconstructed yet.
class NiDX9RenderedCubeMapData : public NiDX9RenderedTextureData {
public:
	NiDX9RenderedCubeMapData(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer);
	~NiDX9RenderedCubeMapData() override;

	// Creates the cube texture from the cube map's size and preferences; the pixel format
	// it used, null when the device fails. Unlike the 2D case it does not copy the format into the renderer data.
	// Inferred type: return: NiPixelFormat* from 0xe90188 (result of NiDX9TextureData::FindClosestPixelFormat), its
	// members used at 0xe90260; pkTexture: NiRenderedCubeMap* from 0xe901d5 (parameter 0 of
	// NiDX9RenderedCubeMapData::Create), its members used at 0xe900d9
	NiPixelFormat* CreateSurf(NiRenderedCubeMap* pkTexture);

	// Creates the cube render target of apCubeMap and the buffer data of its six faces.
	static NiDX9RenderedCubeMapData* Create(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer);

	// The renderer's rendered-cube-map memory statistics: the six faces' bytes and the page rounding the allocator adds.
	// IDA names; not in the PDB.
	static inline uint32_t ms_uiRenderedCubeMapMemory;
	static inline uint32_t ms_uiRenderedCubeMapPageWaste;
};

inline constexpr fnvd::Vtable NiDX9RenderedCubeMapData_vftable{};
