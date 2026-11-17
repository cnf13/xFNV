#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"

class NiDX9Renderer;
class NiRenderedCubeMap;

// The DX9 renderer's data of a rendered cube map. Only some of its members are declared.
class NiDX9RenderedCubeMapData : public NiDX9RenderedTextureData {
public:
    NiDX9RenderedCubeMapData(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer);
    ~NiDX9RenderedCubeMapData() override;

    // Creates the cube texture from the cube map's size and preferences; returns the pixel format it used, null when
    // the device fails. Unlike the 2D case it does not copy the format into the renderer data.
    NiPixelFormat* CreateSurf(NiRenderedCubeMap* pkTexture);

    // Creates the cube render target of apCubeMap and the buffer data of its six faces.
    static NiDX9RenderedCubeMapData* Create(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer);

    // The renderer's rendered-cube-map memory statistics: the six faces' bytes and the page rounding the allocator
    // adds.
    static inline uint32_t ms_uiRenderedCubeMapMemory;
    static inline uint32_t ms_uiRenderedCubeMapPageWaste;
};
