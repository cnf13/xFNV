#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiD3DRenderState;
class NiD3DTextureStage;
class NiD3DTextureStageGroup;
class NiDX9Renderer;
class NiGlobalConstantEntry;
// NiTPool.h includes this header.
template <class T> class NiTObjectPool;

NiSmartPointer(NiD3DTextureStage);

// One texture stage of a pass: the texture, its filter and clamp mode and its transform. Reference counted by hand;
// a stage whose count drops to zero goes back to the pool.
class NiD3DTextureStage : public NiMemObject {
public:
	// The source of the stage's texture transform (bits 28-31 of m_uiTextureTransformFlags) and the calculation
	// applied to it (the low bits). Values from the Xbox PDB.
	enum TSTextureTransformFlags : int32_t {
		TSTTF_IGNORE = 0x0,
		TSTTF_GLOBAL = 0x10000000,
		TSTTF_CONSTANT = 0x20000000,
		TSTTF_SOURCE_MASK = static_cast<int32_t>(0xF0000000),
		TSTTF_NI_NO_CALC = 0x0,
		TSTTF_NI_WORLD_PARALLEL = 0x1,
		TSTTF_NI_WORLD_PERSPECTIVE = 0x2,
		TSTTF_NI_WORLD_SPHERE_MAP = 0x3,
		TSTTF_NI_CAMERA_SPHERE_MAP = 0x4,
		TSTTF_NI_SPECULAR_CUBE_MAP = 0x5,
		TSTTF_NI_DIFFUSE_CUBE_MAP = 0x6,
	};

	// The texture last set on each stage (16 entries; BSBatchRenderer::BeginPass clears them all).
	using ShadowedTextureArray = NiTexture* [16];

	NiD3DTextureStage();
	~NiD3DTextureStage();

	void ApplyTexture();
	void ApplyTextureTransform();
	// Sets the U/V address modes for a non-power-of-two texture.
	void ModifyClampMode(bool bNonPow2);
	// Sets the stage's texture states unless the stage already shows this texture and bAllowRepeats is false;
	// false when the stage states could not be set.
	bool ConfigureStage(bool abForceUpdateSamplerStates = false);
	void SetTexture(NiTexture* pkTexture) { m_spTexture = pkTexture; }
	void IncRefCount() { m_uiRefCount++; }

	void DecRefCount() {
		if (--m_uiRefCount == 0) {
			ReturnStageToPool();
		}
	}

	// Sets the mag, min and mip filters the renderer maps eFilter to, then records eFilter.
	void SetFilterMode(NiTexturingProperty::FilterMode eFilter);
	// Sets the U and V address modes the renderer maps eClamp to.
	void SetClampMode(NiTexturingProperty::ClampMode eClamp);
	// The stage's texture coordinate index: the group's D3DTSS_TEXCOORDINDEX state when
	// the group has one, else the stage number.
	uint32_t GetTexCoordIndex();
	// IDA name; not in the PDB. Whether the stage's D3DTSS_TEXCOORDINDEX state is saved
	// (thiscall, ret 0, result al, 0xE8D3A0).
	bool GetSaveTexCoordIndex();
	// A renderer-owned stage goes back to the pool, any other is deleted.
	void ReturnStageToPool();

	static void CreateNewStage(NiPointer<NiD3DTextureStage>& aspD3DTextureStage);
	static void DisableStage(uint32_t uiStage);

	static void InitializePools();
	static void ShutdownPools();
	// Holds the renderer and a reference on its device, and caches the renderer's render state.
	static void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);
	static char CalculateTransformationMatrix(int32_t a1, _D3DMATRIX* a2);

	// NiTexturingProperty::FilterMode (not reconstructed yet).
	uint32_t m_eFilter;
	uint32_t m_uiStage;
	NiTexture* m_spTexture;
	uint32_t m_uiTextureFlags;
	NiD3DTextureStageGroup* m_pkTextureStageGroup;
	uint32_t m_uiTextureTransformFlags;
	NiGlobalConstantEntry* m_pkGlobalEntry;
	D3DMATRIX m_kTextureTransformation;
	uint16_t m_usObjTextureFlags;
	bool m_bTextureTransform;
	bool m_bRendererOwned;
	uint32_t m_uiRefCount;
	// Fields 0x64-0x67: names unknown; 0x67 selects an sRGB read of the texture.
	bool bApply;
	bool bUnk65;
	bool bUnk66;
	bool bSRGBTexture;

	static inline ShadowedTextureArray pShadowedTextures{};
	static inline IDirect3DDevice9* ms_pkD3DDevice;
	static inline NiDX9Renderer* ms_pkD3DRenderer;
	static inline NiD3DRenderState* ms_pkD3DRenderState;
	// The stages InitializePools allocates and CreateNewStage and ReturnStageToPool take from and give back.
	static inline NiTObjectPool<NiD3DTextureStage>* ms_pkTextureStagePool;
	static inline D3DMATRIX ms_kCalculatedTextureTransformation{};
};

// NiPointer<NiD3DTextureStage> counts through the stage's own (non-atomic) reference count.
inline void NiIncRefCount(NiD3DTextureStage* apStage) {
	apStage->IncRefCount();
}

inline void NiDecRefCount(NiD3DTextureStage* apStage) {
	apStage->DecRefCount();
}
