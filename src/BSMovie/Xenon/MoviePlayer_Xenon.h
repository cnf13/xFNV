#pragma once

#include "BSMain/Setting.h"
#include "BSMovie/MoviePlayer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"

// The movie's texture set on PC: RAD's 3D image (the rad3d sample code the exe links), a grid of uiTextureSize
// square textures covering a uiWidth x uiHeight frame, with smaller power-of-2 textures for the right column and
// the bottom row. Open_RAD_3D_image_pow2 allocates the header and the texture pointers behind it in one block
// (0x48 + 4 per texture) and Movie::pTextureSet keeps it. The field names are chosen here from the uses in
// Open_RAD_3D_image_pow2 (0xEBF8E0), Lock_RAD_3D_image_pow2 (0xEBFDA0) and Blit_RAD_3D_image_pow2 (0xEBFF30).
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
IDirect3DTexture9* Create_texture(IDirect3DDevice9* pkD3DDevice9, D3DFORMAT eFormat, uint32_t uiPitch,
	uint32_t uiPixelSize, uint32_t uiWidth, uint32_t uiHeight);
// The image's header and texture pointers in one block
// (MemoryManager), null when a texture could not be made. a6 is stored at +0x4 and passed to Create_texture as its
// format.
// Inferred type: a2: uint32_t: unsigned uses from 0xebf8fe; a3: uint32_t: unsigned uses from 0xebf912; a5: uint32_t:
// unsigned uses from 0xebf8fe
BINKTEXTURESET* Open_RAD_3D_image_pow2(IDirect3DDevice9* pkD3DDevice9, uint32_t a2, uint32_t a3, int32_t a4,
	uint32_t a5, D3DFORMAT a6);
// Inferred type: a1: uint32_t: unsigned uses from 0xebfc87
int32_t Round_up_to_next_2_power(uint32_t a1);
// Inferred type: Block: BINKTEXTURESET* from 0xec104b (Movie::pTextureSet (this of Movie::Stop)), its members used at
// 0xebfd27
void Close_RAD_3D_image_pow2(BINKTEXTURESET* Block);
// IDA name; not in the PDB. Locks the image's next texture (uiLockIndex) and returns its bits, pitch, position and
// size through the non-null pointers; 0 after the last texture. The exe stores LockRect's pBits through a2 (RAD's
// Lock_RAD_3D_image hands the pixel buffer back the same way).
int32_t Lock_RAD_3D_image_pow2(BINKTEXTURESET* a1, void** a2, uint32_t* a3, uint32_t* a4, uint32_t* a5, uint32_t* a6,
	uint32_t* a7);
// Unlocks the texture Lock_RAD_3D_image_pow2 locked last
// (ppTextures[uiLockIndex - 1]) and returns UnlockRect's result.
int32_t Unlock_RAD_3D_image_pow2(BINKTEXTURESET* a1);
// Draws the image's textures through its device (a1->pDevice).
// Inferred type: a1: BINKTEXTURESET* from 0xec1390 (Movie::pTextureSet (this of Movie::Update)), its members used at
// 0xebff3b; a2: float from 0xec1390 (Movie::fXCoordOffset (this of Movie::Update)); a3: float from 0xec1390
// (Movie::fYCoordOffset (this of Movie::Update)); a4: float from 0xec1390 (Movie::fScale (this of Movie::Update)); a5:
// float from 0xec1390 (Movie::fScale (this of Movie::Update))
int32_t Blit_RAD_3D_image_pow2(BINKTEXTURESET* a1, float a2, float a3, float a4, float a5);
// Draws one textured screen-space quad (XYZRHW, one texture coordinate) at fX, fY of
// iWidth * fScaleX by iHeight * fScaleY through pkD3DDevice9, with the renderer's vertex and pixel shaders cleared.
// iWidth and iHeight are signed: the exe loads them with fild.
void Submit_vertices(IDirect3DDevice9* pkD3DDevice9, float fX, float fY, float fScaleX, float fScaleY, int32_t iWidth,
	int32_t iHeight);
// a1 is the device: the body calls its SetRenderState (slot 57),
// SetTextureStageState (67) and SetSamplerState (69).
// Inferred type: a1: IDirect3DDevice9* from 0xebff3c (BINKTEXTURESET::pDevice (parameter 0 of Blit_RAD_3D_image_pow2)),
// its members used at 0xec046a
int32_t Setup_Renderstate(IDirect3DDevice9* a1);
// Not named in the PDB or IDA; name chosen here. Blit_RAD_3D_image_pow2's grid walk without the render states,
// through DrawRectOutline. PDB: none; the exe has no caller of it and returns no value (eax is whatever the last step left:
// a loop counter, a1 or DrawRectOutline's result).
void DrawTextureSetTiles(BINKTEXTURESET* a1, float a2, float a3, float a4, float a5);
// Not named in the PDB or IDA; name chosen here. a1 is the device (DrawTextureSetTiles passes pDevice). a6 and a7 are signed:
// the exe loads them with fild. Returns DrawPrimitiveUP's result.
int32_t DrawRectOutline(IDirect3DDevice9* a1, float a2, float a3, float a4, float a5, int32_t a6, int32_t a7);
// Decodes bink's next frame into image's textures.
// Inferred type: bink: BINK* from 0xec10cd (Movie::pBink (this of Movie::Update)), its members used at 0xec1537
int32_t Decompress_frame(BINK* bink, BINKTEXTURESET* image, int32_t copyAll);

// A screen-space vertex with one texture coordinate (FVF D3DFVF_XYZRHW | D3DFVF_TEX1, 0x18 bytes), as DrawRectOutline
// builds them. Not in the PDB; the name is RAD's rad3d sample's.
struct POS_TEX_VERTEX {
	float sx;
	float sy;
	float sz;
	float rhw;
	float tu;
	float tv;
};

// INISettingCollection: 0xfc77bb calls 0x44f570.
extern SettingT<INISettingCollection> gs_iPreloadSizeLimit_General;
