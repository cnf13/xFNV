#pragma once

#include "BSMain/Setting.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectDepthOfField.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

class NiCamera;

// The depth-masked radial blur image-space effect.
class ImageSpaceEffectDepthRadialBlur : public ImageSpaceEffect {
public:
	enum EffectListEnum : int32_t {
		EFFECT_RADIAL_BLUR = 0x0,
		EFFECT_DEPTH_OF_FIELD = 0x1,
		EFFECT_COUNT = 0x2,
	};

	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_BLUR = 0x2,
		TEXTURE_DEPTH = 0x3,
		TEXTURE_COUNT = 0x4,
	};

	enum RadialBlurMode : int32_t {
		DOF_FRONT_BACK = 0x0,
		DOF_FRONT = 0x1,
		DOF_BACK = 0x2,
		DOF_NONE = 0x3,
	};

	ImageSpaceEffectDepthRadialBlur();
	~ImageSpaceEffectDepthRadialBlur() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void BorrowTextures(ImageSpaceEffectParam* pParam) override;
	void ReturnTextures() override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;

	// Switches the blur to aMode: the front weight is on for front-back and front, the back weight for front-back
	// and back.
	void SetMode(RadialBlurMode aMode);

	RadialBlurMode Mode;
	// The depth ramp: UpdateParams writes g, b and a from the camera's near plane, far - near and near * far each
	// frame.
	NiColorA Params3;
	// Pixel constant 1: r is the blur strength (IsActive tests it; ImageSpaceManager writes it each frame).
	NiColorA Params2;
	// Pixel constant 0: r and g are the depth band's near and far edges, b its centre distance (set by
	// ImageSpaceManager), a the fRenderDepthMaxDepth setting the constructor copies.
	NiColorA Params;
	// The radial blur's weights: UpdateParams reads r, g and b, and leaves a unused.
	NiColorA RadialParams;
	// EndParamModifiers keeps the largest depth radial blur range here, and the constructor starts it at 600.
	float fRange;
	float fMaskConst;
	ImageSpaceTexture MaskBuffer;
	ImageSpaceTexture pBufferA[2];
};

// The plain radial blur image-space effect: blurs the image outwards from a centre with a strength that ramps up and
// down.
class ImageSpaceEffectRadialBlur : public ImageSpaceEffect {
public:
	enum EffectListEnum : int32_t {
		EFFECT_RADIAL_BLUR = 0x0,
		EFFECT_COUNT = 0x1,
	};

	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_COUNT = 0x2,
	};

	ImageSpaceEffectRadialBlur();
	~ImageSpaceEffectRadialBlur() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;

	// Reads apBound's screen-space centre and the start bias of the radial blur's ramp; the
	// bias is one float the callee stores through afStartBias.
	static void GetBoundParams(const NiBound& aBound, NiPoint2& aCenter, float& afStartBias);

	float fStrength;
	float fRampup;
	float fStart;
	float fRampdown;
	float fDownstart;
	NiPoint2 Center;

	static inline NiCamera* pWorldCamera;
};

extern SettingT<INISettingCollection> bDoRadialBlur;
extern SettingT<RendererSettingCollection> iRadialBlurLevel;
