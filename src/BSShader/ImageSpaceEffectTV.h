#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Reconstruction.hpp"

// The Pipboy's TV screen effect: scan lines and a distortion map over the screen. IDA spells the class
// ImageSpaceEffectPipboyScreen (an alias; the PDB's name is this one).
class ImageSpaceEffectTV : public ImageSpaceEffect {
public:
	ImageSpaceEffectTV();
	~ImageSpaceEffectTV() override;

	// Never active: the exe's slot 6 returns false, whatever bIsActive holds.
	bool IsActive() override;
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void ReturnTextures() override;
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;

	// The shader constants Setup/UpdateParams hand to the pixel shader. The statics are matched to the PDB's names by
	// the PDB's declaration order and the constant registers they fill (register 1: fPulse; register 2 and 3: the
	// vertical and horizontal scale and the vertical position, real then fake; register 4: the tint); the exe's map
	// has no names for them.
	static inline float fPulse = 1.0f;
	static inline float fDistortVerticalScale;
	static inline float fDistortHorizontalScale;
	static inline float fDistortVerticalPosition = 10.0f;
	static inline float fFakeDistortVerticalScale;
	static inline float fFakeDistortHorizontalScale;
	static inline float fFakeDistortVerticalPosition = 10.0f;
	static inline NiColorA Tint{};
	// The exe's map names them Texture::PipboyScanlines_0 and Texture::PipboyDistortEffectMap_0. PDB:
	// NiPointer<NiTexture> both (pdb-publics); Setup passes their addresses to BSShaderManager::GetTexture.
	static NiPointer<NiTexture> spScanlinesTexture;
	static NiPointer<NiTexture> spDistortTexture;
};
inline constexpr fnvd::Vtable ImageSpaceEffectTV_vftable{};
