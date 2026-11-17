#pragma once

#include "BSMain/Setting.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

// The motion blur image-space effect: smears the frame along the per-pixel velocity buffer.
class ImageSpaceEffectMotionBlur : public ImageSpaceEffect {
public:
	enum EffectListEnum : int32_t {
		EFFECT_MOTION_BLUR = 0x0,
		EFFECT_COUNT = 0x1,
	};

	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_VELOCITY = 0x2,
		TEXTURE_COUNT = 0x3,
	};

	ImageSpaceEffectMotionBlur();
	~ImageSpaceEffectMotionBlur() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void Shutdown() override;
	void ReturnTextures() override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;

	void SetFullScreenBlurConst(float afBlur);
	float GetObjectBlurConst();

	NiColorA Params;
	ImageSpaceTexture VelocityBuffer;

	static inline bool bInEditor;
};

extern SettingT<INISettingCollection> bDoMotionBlur;
