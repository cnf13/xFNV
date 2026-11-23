#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"

#include <stdint.h>

class NiDX9PersistentSrcTextureRendererData;
class NiDX9Renderer;

// The DX9 renderer's data of a source cube map. Only some of the members are declared.
class NiDX9SourceCubeMapData : public NiDX9SourceTextureData {
public:
    NiDX9SourceCubeMapData(NiSourceCubeMap* apCubeMap, NiDX9Renderer* apRenderer);
    ~NiDX9SourceCubeMapData() override;

    // Slot 45.
    void Update() override;

    // Creates the cube texture from the width, level count and pixel format; false when the device fails.
    bool CreateCubeMap();
    // Copies the levels of one face of the source pixel data into the cube texture's surfaces.
    void CopyDataToSurface(NiPixelData* apkPixels, NiSourceCubeMap::FaceID aeFace);
    // The same for the persistent renderer data.
    void
    CopyRendererDataToCubeMapFace(const NiDX9PersistentSrcTextureRendererData* apkData, NiSourceCubeMap::FaceID aeFace);
    bool UpdatePalette(NiPalette* pkNewPalette);

    // Creates the renderer data of apCubeMap.
    static NiDX9SourceCubeMapData* Create(NiSourceCubeMap* apCubeMap, NiDX9Renderer* apRenderer);

    // Gamebryo's own member, shadowing NiDX9SourceTextureData's: the source pixel data's revision this data was
    // last converted from (Create and Update read and write +0x84; Create allocates 0x88).
    uint32_t m_uiSourceRevID;

    static inline int32_t ms_uiCubeMapTextureMemory;
    static inline int32_t ms_uiCubeMapPageWaste;
};
