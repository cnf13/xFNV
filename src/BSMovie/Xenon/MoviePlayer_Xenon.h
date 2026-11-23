#pragma once

#include "BSMain/Setting.h"
#include "BSMovie/MoviePlayer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

// The movie's texture set on PC: RAD's 3D image (the rad3d sample code), a grid of uiTextureSize square textures
// covering a uiWidth x uiHeight frame, with smaller power-of-2 textures for the right column and the bottom row.
// Open_RAD_3D_image_pow2 allocates the header and the texture pointers behind it in one block and Movie::pTextureSet
// keeps it.
struct BINKTEXTURESET {
    IDirect3DDevice9* pDevice;
    D3DFORMAT eFormat;
    uint32_t uiPixelSize;
    uint32_t uiWidth;
    uint32_t uiHeight;
    // The next texture Lock_RAD_3D_image_pow2 hands out; it wraps to 0 after the last.
    uint32_t uiLockIndex;
    uint32_t unk18;
    // The bytes per row of the whole frame, rounded up to 16.
    uint32_t uiPitch;
    uint32_t uiTextureCount;
    // The full-size textures per row and per column.
    uint32_t uiColumns;
    uint32_t uiRows;
    // The power-of-2 sizes of the right column's and the bottom row's textures (0 when the frame divides evenly).
    uint32_t uiRemnantWidth;
    uint32_t uiRemnantHeight;
    uint32_t uiWidthRemainder;
    uint32_t uiHeightRemainder;
    uint32_t uiTextureSize;
    // Points at the uiTextureCount texture pointers right behind this header, row by row.
    IDirect3DTexture9** ppTextures;
    uint32_t unk44;
};

// Creates a uiWidth x uiHeight dynamic texture of eFormat (a managed one when the
// default-pool texture fails and the device is not Direct3D9Ex), clears its uiWidth * uiPixelSize bytes per row and
// returns it (null when both creations fail). uiPitch is not read.
IDirect3DTexture9* Create_texture(
    IDirect3DDevice9* pkD3DDevice9,
    D3DFORMAT eFormat,
    uint32_t uiPitch,
    uint32_t uiPixelSize,
    uint32_t uiWidth,
    uint32_t uiHeight
);
// Returns the image's header and texture pointers in one block (MemoryManager), null when a texture could not be made.
// eFormat is stored in the block and passed to Create_texture as its format.
BINKTEXTURESET* Open_RAD_3D_image_pow2(
    IDirect3DDevice9* pkD3DDevice9,
    uint32_t uiWidth,
    uint32_t uiHeight,
    int32_t iPixelSize,
    uint32_t uiTextureSize,
    D3DFORMAT eFormat
);
int32_t Round_up_to_next_2_power(uint32_t uiValue);
void Close_RAD_3D_image_pow2(BINKTEXTURESET* Block);
// Locks the image's next texture (uiLockIndex) and returns its bits, pitch, position and size through the non-null
// pointers; 0 after the last texture. LockRect's pBits is stored through ppBits (RAD's Lock_RAD_3D_image hands the
// pixel buffer back the same way).
int32_t Lock_RAD_3D_image_pow2(
    BINKTEXTURESET* pkImage,
    void** ppBits,
    uint32_t* puiPitch,
    uint32_t* puiX,
    uint32_t* puiY,
    uint32_t* puiWidth,
    uint32_t* puiHeight
);
// Unlocks the texture Lock_RAD_3D_image_pow2 locked last
// (ppTextures[uiLockIndex - 1]) and returns UnlockRect's result.
int32_t Unlock_RAD_3D_image_pow2(BINKTEXTURESET* pkImage);
// Draws the image's textures through its device (pkImage->pDevice). Movie::Update passes its pTextureSet,
// fXCoordOffset, fYCoordOffset and fScale twice.
int32_t Blit_RAD_3D_image_pow2(BINKTEXTURESET* pkImage, float fXOffset, float fYOffset, float fScaleX, float fScaleY);
// Draws one textured screen-space quad (XYZRHW, one texture coordinate) at fX, fY of iWidth * fScaleX by iHeight *
// fScaleY through pkD3DDevice9, with the renderer's vertex and pixel shaders cleared. iWidth and iHeight are signed.
void Submit_vertices(
    IDirect3DDevice9* pkD3DDevice9,
    float fX,
    float fY,
    float fScaleX,
    float fScaleY,
    int32_t iWidth,
    int32_t iHeight
);
// pkD3DDevice9 is the device: the body calls its SetRenderState, SetTextureStageState and SetSamplerState.
int32_t Setup_Renderstate(IDirect3DDevice9* pkD3DDevice9);
// Blit_RAD_3D_image_pow2's grid walk without the render states, through DrawRectOutline.
void DrawTextureSetTiles(BINKTEXTURESET* pkImage, float fXOffset, float fYOffset, float fScaleX, float fScaleY);
// pkD3DDevice9 is the device (DrawTextureSetTiles passes pDevice). iWidth and iHeight are signed. Returns
// DrawPrimitiveUP's result.
int32_t DrawRectOutline(
    IDirect3DDevice9* pkD3DDevice9,
    float fX,
    float fY,
    float fScaleX,
    float fScaleY,
    int32_t iWidth,
    int32_t iHeight
);
// Decodes bink's next frame into image's textures.
int32_t Decompress_frame(BINK* bink, BINKTEXTURESET* image, int32_t copyAll);

// A screen-space vertex with one texture coordinate (FVF D3DFVF_XYZRHW | D3DFVF_TEX1, 0x18 bytes), as DrawRectOutline
// builds them.
struct POS_TEX_VERTEX {
    float sx;
    float sy;
    float sz;
    float rhw;
    float tu;
    float tv;
};

extern SettingT<INISettingCollection> gs_iPreloadSizeLimit_General;
