#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "Reconstruction.hpp"

// The image-space effect that refracts the scene through a normal buffer (heat haze and similar distortion).
class ImageSpaceEffectRefraction : public ImageSpaceEffect {
public:
	// Values from the Xbox PDB.
	enum EffectListEnum : int32_t {
		EFFECT_REFRACTION = 0x0,
		EFFECT_COUNT = 0x1,
	};

	// Values from the Xbox PDB.
	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_NORMALS = 0x2,
		TEXTURE_COUNT = 0x3,
	};

	ImageSpaceEffectRefraction();
	~ImageSpaceEffectRefraction() override;

	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;

	// The effect's refraction normals texture, created on demand when abCreate is set.
	BSRenderedTexture* GetRefractionNormalsTexture(bool abCreate);

	ImageSpaceTexture spRefractionNormalBuffer;
};
inline constexpr fnvd::Vtable ImageSpaceEffectRefraction_vftable{};
