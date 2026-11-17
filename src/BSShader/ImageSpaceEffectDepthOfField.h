#pragma once

#include "BSMain/Setting.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceTexture.h"

class NiScreenElements;

// An image-space effect with an on/off switch per sub-effect.
class ImageSpaceEffectOption : public ImageSpaceEffect {
public:
	ImageSpaceEffectOption();
	~ImageSpaceEffectOption() override;

	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;

	// Which sub-effects are on, one bool per effect.
	NiTPrimitiveArray<bool> pEffectOn;
};

// The depth of field image-space effect: blurs the front, the back or both of the focus range, through a mask.
class ImageSpaceEffectDepthOfField : public ImageSpaceEffectOption {
public:
	enum EffectListEnum : int32_t {
		EFFECT_DOWNSAMPLE = 0x0,
		EFFECT_BLUR = 0x1,
		EFFECT_DEPTH_OF_FIELD = 0x2,
		EFFECT_DEPTH_OF_FIELD_MASK_BLUR = 0x3,
		EFFECT_COUNT = 0x4,
	};

	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_BLUR = 0x2,
		TEXTURE_BLUR_SWAP = 0x3,
		TEXTURE_DEPTH = 0x4,
		TEXTURE_MASK = 0x5,
		TEXTURE_COUNT = 0x6,
	};

	enum DepthOfFieldMode : int32_t {
		DOF_FRONT_BACK = 0x0,
		DOF_FRONT = 0x1,
		DOF_BACK = 0x2,
		DOF_NONE = 0x3,
	};

	ImageSpaceEffectDepthOfField();
	~ImageSpaceEffectDepthOfField() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void BorrowTextures(ImageSpaceEffectParam* pParam) override;
	void ReturnTextures() override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;

	BSRenderedTexture* BorrowMaskTexture();
	void ReturnMaskTexture();
	bool DoDepth() const;
	// The sub-effects of a plain blur: everything but the mask blur (index 3 off).
	void DoStandard();
	// The sub-effects of a masked blur: everything but the depth of field pass (index 2 off).
	void DoStandardMask();

	float fDepth;
	float fRange;
	float fStrength;
	DepthOfFieldMode Mode;
	float fMaskConst;
	ImageSpaceTexture MaskBuffer;
	ImageSpaceTexture pBufferA[3];
};

extern SettingT<RendererSettingCollection> bDoDepthOfField;
extern SettingT<INISettingCollection> fRenderDepthMaxDepth;
