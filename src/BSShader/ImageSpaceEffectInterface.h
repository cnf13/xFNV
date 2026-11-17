#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Reconstruction.hpp"

// The interface's image-space effect, driven by the static settings below. Only the statics the reconstructed code
// reaches are declared.
class ImageSpaceEffectInterface : public ImageSpaceEffect {
public:
	ImageSpaceEffectInterface();

	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void ReturnTextures() override;
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;
	~ImageSpaceEffectInterface() override;

	// The interface effect's mode; 0 when it is off.
	static inline int32_t eMode;
	static inline float fBlurRadius = 2.0f;
	static inline float fBlurIntensity = 1.0f;
	static inline float fScanlineFrequency = 150.0f;
	static inline float fTextureUsedPart = 1.0f;
	static inline float fDistortHorizontalScale = 10.0f;
	static inline float fPulse = 1.0f;
	static inline float fVerticalHold;
	static inline NiRenderer::ClearFlags eClearFlag{};
	static inline float fDistortVerticalScale;
	static inline float fDistortVerticalPosition;
	static inline bool bDistortEverywhere;
	static NiPointer<NiTexture> spScanlinesTexture;
	// Setup passes its address to BSShaderManager::GetTexture's NiPointer<NiTexture>&.
	static NiPointer<NiTexture> spDistortTexture;
	static NiPointer<NiTexture> spMaskTexture;
	static inline NiColorA kTint{};
};
inline constexpr fnvd::Vtable ImageSpaceEffectInterface_vftable{};
