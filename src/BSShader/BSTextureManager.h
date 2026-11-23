#pragma once

#include "BSShader/BSRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiCamera;
class NiDX9Renderer;

// Hands out and takes back the engine's render-target textures. Only some of its members are declared.
class BSTextureManager {
public:
    // The render-target types, numbered as on PC: the PC adds TEX_TESWATER_BLUR at 22, so every later type is one above
    // its Xbox value.
    enum eTextureType : uint32_t {
        TEX_HDR_DOWNSAMPLE = 0,
        TEX_HDR_DOWNSAMPLE_ANISOINTERMEDIATE = 1,
        TEX_HDR_BLURSWAP = 2,
        TEX_IMAGESPACE_SWAP = 3,
        TEX_MAIN_IMAGESPACE = 4,
        TEX_MAIN_FIRSTPERSON = 5,
        TEX_MAIN_MENUBG = 6,
        TEX_TESWATER_FFT = 7,
        TEX_TESWATER_GREY = 8,
        TEX_TESWATER_REFLECTION = 9,
        TEX_TESWATER_LOD_REFLECTION = 10,
        TEX_TESWATER_REFRACTION = 11,
        TEX_TESWATER_RAIN = 12,
        TEX_TESWATER_SCRAMBLEMAP = 13,
        TEX_TESWATER_IFFT_BUTTERFLYMAP = 14,
        TEX_TESWATER_HIGH_W = 15,
        TEX_TESWATER_HIGH_AMPLITUDE = 16,
        TEX_TESWATER_DEPTH = 17,
        TEX_TESWATER_NOISE_HEIGHTMAP = 18,
        TEX_TESWATER_NOISE_NORMALMAP = 19,
        TEX_TESWATER_DISPLACEMENT = 20,
        TEX_TESWATER_DEBUG_DISPLAY = 21,
        TEX_TESWATER_BLUR = 22,
        TEX_TREES_CANOPYSHADOWMASK = 23,
        TEX_BLURFULL_BUFFER = 24,
        TEX_BLURFULL_SWAP = 25,
        TEX_BLUR20_BUFFER = 26,
        TEX_GETHIT_BUFFER = 27,
        TEX_LOCALMAP_FINAL = 28,
        TEX_LOCALMAP_COLOR = 29,
        TEX_LOCALMAP_NORMALS = 30,
        TEX_REFRACTION_NORMALS = 31,
        TEX_VFOG = 32,
        TEX_SCENE_DEPTH_BUFFER = 33,
        TEX_VFOG_SWAP = 34,
        TEX_VELOCITY = 35,
        TEX_VELOCITY_SWAP = 36,
        TEX_DOF_BLUR = 37,
        TEX_DOF_BLUR_SWAP = 38,
        TEX_DOF_MASK = 39,
        TEX_DEPTH_RADIALBLUR_BLUR = 40,
        TEX_CUBEMAP_CAMERA = 41,
        TEX_SHADOWS_SHADOWMAP = 42,
        TEX_SHADOWS_SHADOWMAPFRUSTUM = 43,
        TEX_ENVMAP_WINDOWS = 44,
        TEX_INTERFACE_RENDEREDMENU = 0x2D,
        TEX_INTERFACE_RENDEREDMENU_SWAP = 46,
        TEX_INTERFACE_VATS = 0x2F,
        TEX_INTERFACE_RACESEXMENU = 48,
        TEX_INTERFACE_TEMP = 49,
        TEX_NOISE_BUFFER = 50,
        TEX_DECAL_OCCLUSION = 51,
        TEX_COUNT = 52,
        TEX_UNKNOWN = 0xFFFFFFFF,
        // Group bounds of the texture types; TEX_TESWATER_BLUR is taken as the last water type.
        TEX_HDR_FIRST = 0,
        TEX_HDR_LAST = 2,
        TEX_IMAGESPACE_FIRST = 3,
        TEX_IMAGESPACE_LAST = 3,
        TEX_MAIN_FIRST = 4,
        TEX_MAIN_LAST = 6,
        TEX_TESWATER_FIRST = 7,
        TEX_TESWATER_LAST = 22,
        TEX_TREES_FIRST = 23,
        TEX_TREES_LAST = 23,
        TEX_BLUR_FIRST = 24,
        TEX_BLUR_LAST = 25,
        TEX_BLUR20_FIRST = 26,
        TEX_BLUR20_LAST = 26,
        TEX_GETHIT_FIRST = 27,
        TEX_GETHIT_LAST = 27,
        TEX_LOCALMAP_FIRST = 28,
        TEX_LOCALMAP_LAST = 30,
        TEX_REFRACTION_FIRST = 31,
        TEX_REFRACTION_LAST = 31,
        TEX_VFOG_FIRST = 32,
        TEX_VFOG_LAST = 34,
        TEX_VELOCITY_FIRST = 35,
        TEX_VELOCITY_LAST = 36,
        TEX_DOF_FIRST = 37,
        TEX_DOF_LAST = 39,
        TEX_DEPTH_RADIALBLUR_FIRST = 40,
        TEX_DEPTH_RADIALBLUR_LAST = 40,
        TEX_CUBEMAP_FIRST = 41,
        TEX_CUBEMAP_LAST = 41,
        TEX_SHADOWS_FIRST = 42,
        TEX_SHADOWS_LAST = 43,
        TEX_ENVMAP_FIRST = 44,
        TEX_ENVMAP_LAST = 44,
        TEX_INTERFACE_FIRST = 45,
        TEX_INTERFACE_LAST = 49,
        TEX_NOISE_FIRST = 50,
        TEX_NOISE_LAST = 50,
        TEX_DECAL_FIRST = 51,
        TEX_DECAL_LAST = 51,
    };

    // The creation flags of a rendered texture. Each flag is the bit one above its index, 1 << (index + 1).
    enum eRenderedTextureFlags : uint32_t {
        BT_FLAG_CLEAR = 0x2,
        BT_FLAG_NODEPTH = 0x4,
        BT_FLAG_NOTRENDERABLE = 0x8,
        BT_FLAG_CUBEMAP = 0x10,
        BT_FLAG_PRECREATE = 0x20,
        BT_FLAG_MULTISAMPLE = 0x40,
        BT_FLAG_NOTDISCARDABLEDEPTH = 0x80,
        BT_FLAG_UNIQUEDEPTHBUFFER = 0x100,
        BT_FLAG_TILED = 0x200,
    };

    // One pooled render target and the parameters it was created with.
    struct RenderedTextureData {
        NiPointer<BSRenderedTexture> spRenderedTexture;
        D3DFORMAT iD3DFormat;
        D3DFORMAT iEDRAMFormat;
        uint32_t uFlags;
        bool bUsedThisFrame;
        eTextureType eType;
    };

    // Builds the empty lists, then the shadow depth buffer: the default target group's own depth buffer when it
    // has one, else a new one at least the frustum shadow map's size (with multisampling, one the group's size
    // that the group takes, and one at least the frustum shadow map's and main image space's size).
    BSTextureManager();
    ~BSTextureManager();

    // Hands out a rendered texture of type aeType from the manager's pool.
    BSRenderedTexture* BorrowRenderedTexture(
        NiDX9Renderer* apRenderer,
        eTextureType aeType,
        uint32_t auiAdditionalFlags,
        NiRenderedTexture* apAliasTex,
        uint32_t uiAliasOffset
    );
    // Hands out a pooled rendered texture of the given size (the screen's when either is 0) and formats, creating
    // one when the pool has none.
    BSRenderedTexture* BorrowRenderedTexture(
        NiDX9Renderer* apRenderer,
        uint32_t auiWidth,
        uint32_t auiHeight,
        uint32_t auFlags,
        D3DFORMAT aiD3DFormat,
        D3DFORMAT aiEDRAMFormat,
        eTextureType aeType,
        D3DMULTISAMPLE_TYPE aeMSAAType,
        NiRenderedTexture* apAliasTex,
        uint32_t uiAliasOffset
    );
    // Creates a rendered texture of the given size and formats.
    BSRenderedTexture* CreateRenderedTexture(
        NiDX9Renderer* apRenderer,
        uint32_t auiWidth,
        uint32_t auiHeight,
        D3DFORMAT aiD3DFormat,
        D3DFORMAT aiEDRAMFormat,
        uint32_t auFlags,
        eTextureType aeType,
        D3DMULTISAMPLE_TYPE aeMSAAType,
        NiRenderedTexture* apAliasTex,
        uint32_t uiAliasOffset
    );
    // Frees the unused pooled textures not used this frame (and not flagged BT_FLAG_TILED), and clears the
    // used-this-frame marks of the rest; does nothing in menu mode.
    void Update(NiCamera* apCamera, bool abMenuMode);
    // Takes a rendered texture back into the manager's pool.
    void ReturnRenderedTexture(BSRenderedTexture* apTexToReturn);
    // Takes a frustum shadow map back into the manager's pool.
    void ReturnFrustumShadowTexture(BSRenderedTexture* apTexToReturn);
    // Hands out a frustum shadow map from the manager's pool.
    BSRenderedTexture* BorrowFrustumShadowTexture();
    // The depth buffer the shadow maps share.
    NiDepthStencilBuffer* GetShadowDepthBuffer();
    // Releases spSharedDepthBuffer and clears it; ShadowSceneNode::ReleaseAllShadowMaps calls it after returning the
    // shadow maps.
    void ClearShadowDepthBuffer();
    // Creates the render targets flagged for precreation.
    void PrecreateTextures(NiDX9Renderer* apRenderer);

protected:
    // The size, formats, creation flags and multisample type of a render target of type aeType (the multisample
    // type is cleared unless the flags ask for BT_FLAG_MULTISAMPLE).
    void LookupTextureParameters(
        NiDX9Renderer* apRenderer,
        eTextureType aeType,
        uint32_t& auiWidth,
        uint32_t& auiHeight,
        D3DFORMAT& aiD3DFormat,
        D3DFORMAT& aiEDRAMFormat,
        uint32_t& auFlags,
        D3DMULTISAMPLE_TYPE& aeMSAAType
    );

public:
    // Looks up the parameters of a rendered texture of type aeType and creates one.
    BSRenderedTexture* CreateRenderedTexture(
        NiDX9Renderer* apRenderer,
        BSTextureManager::eTextureType aeType,
        NiRenderedTexture* apAliasTex,
        uint32_t uiAliasOffset
    );
    void SetFrustumShadowTextureCount(NiDX9Renderer* apRenderer, uint32_t auiMaxShadows);

    NiTPointerList<NiPointer<BSRenderedTexture>> spUnusedAliasedTextureList[5];
    NiTPointerList<NiPointer<BSRenderedTexture>> spUsedAliasedTextureList[5];
    NiTPointerList<RenderedTextureData*> unusedRTPool;
    NiTPointerList<RenderedTextureData*> usedRTPool;
    NiTPointerList<NiPointer<BSRenderedTexture>> unusedFrustumShadowRTPool;
    NiTPointerList<NiPointer<BSRenderedTexture>> usedFrustumShadowRTPool;
    NiPointer<NiDepthStencilBuffer> spShadowDepthBuffer;
    // This depth buffer is only ever released (constructor, destructor, ShadowSceneNode::ReleaseAllShadowMaps).
    NiPointer<NiDepthStencilBuffer> spSharedDepthBuffer;
};

inline char bSkipShadowTextureRelease = 1;
