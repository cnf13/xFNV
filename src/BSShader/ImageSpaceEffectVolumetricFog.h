#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

// The volumetric fog image-space effect, tinted by FogColor.
class ImageSpaceEffectVolumetricFog : public ImageSpaceEffect {
public:
	enum EffectListEnum : int32_t {
		EFFECT_VOLUMETRIC_FOG = 0x0,
		EFFECT_COUNT = 0x1,
	};

	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_COUNT = 0x2,
	};

	ImageSpaceEffectVolumetricFog();
	~ImageSpaceEffectVolumetricFog() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;

	NiColorA FogColor;
};
