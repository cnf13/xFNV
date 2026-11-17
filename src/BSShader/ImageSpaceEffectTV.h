#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

// The Pipboy's TV screen effect: scan lines and a distortion map over the screen.
class ImageSpaceEffectTV : public ImageSpaceEffect {
public:
	ImageSpaceEffectTV();
	~ImageSpaceEffectTV() override;

	// Never active: returns false whatever bIsActive holds, as the game does.
	bool IsActive() override;
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void ReturnTextures() override;
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;

	// The shader constants Setup/UpdateParams hand to the pixel shader. The constant registers they fill: register 1:
	// fPulse; register 2 and 3: the vertical and horizontal scale and the vertical position, real then fake; register
	// 4: the tint.
	static inline float fPulse = 1.0f;
	static inline float fDistortVerticalScale;
	static inline float fDistortHorizontalScale;
	static inline float fDistortVerticalPosition = 10.0f;
	static inline float fFakeDistortVerticalScale;
	static inline float fFakeDistortHorizontalScale;
	static inline float fFakeDistortVerticalPosition = 10.0f;
	static inline NiColorA Tint{};
	// Setup passes their addresses to BSShaderManager::GetTexture.
	static NiPointer<NiTexture> spScanlinesTexture;
	static NiPointer<NiTexture> spDistortTexture;
};
