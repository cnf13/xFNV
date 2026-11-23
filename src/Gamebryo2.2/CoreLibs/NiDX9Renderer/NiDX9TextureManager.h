#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class NiDX9Renderer;
class NiTexture;

// Creates and caches the Direct3D textures of the scene's textures.
class NiDX9TextureManager : public NiRefObject {
public:
    NiDX9TextureManager(NiDX9Renderer* pkRenderer);
    ~NiDX9TextureManager() override;

    virtual IDirect3DTexture9* GetTexture(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
        return nullptr;
    }

    virtual IDirect3DBaseTexture9*
    PrepareTextureForRendering(NiTexture* apTexture, bool& arbChanged, bool& arbMipmap, bool& arbNonPow2);
    virtual bool PrecacheTexture(NiTexture* apTexture, int32_t abForceLoad, int32_t abLocked);
    virtual uint32_t GetFormatFromTexture(NiTexture* apTexture);

    IDirect3DDevice9* m_pkD3DDevice9;
    NiDX9Renderer* m_pkRenderer;
};
