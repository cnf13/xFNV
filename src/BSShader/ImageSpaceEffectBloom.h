#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "Reconstruction.hpp"

// The bloom image space effect: multiplies the alpha, blurs it through two buffers and blends it back.
class ImageSpaceEffectBloom : public ImageSpaceEffect {
public:
	// Values from the Xbox PDB.
	enum EffectListEnum : int32_t {
		EFFECT_ALPHAMULT = 0x0,
		EFFECT_BLUR = 0x1,
		EFFECT_BLEND = 0x2,
		EFFECT_COUNT = 0x3,
	};

	// Values from the Xbox PDB.
	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC = 0x1,
		TEXTURE_BLUR1 = 0x2,
		TEXTURE_BLUR2 = 0x3,
		TEXTURE_COUNT = 0x4,
	};

	// Values from the Xbox PDB.
	enum BloomDataType : int32_t {
		BLOOM_BLUR_RADIUS = 0x0,
		BLOOM_ALPHA_ADD_INTERIOR = 0x1,
		BLOOM_ALPHA_ADD_EXTERIOR = 0x2,
		BLOOM_DATA_COUNT = 0x3,
	};

	ImageSpaceEffectBloom();
	~ImageSpaceEffectBloom() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void BorrowTextures(ImageSpaceEffectParam* pParam) override;
	void ReturnTextures() override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;
	virtual void SetParams(float* apfData);

	float* pfData;
	ImageSpaceTexture pBlurBufferA[2];
};
inline constexpr fnvd::Vtable ImageSpaceEffectBloom_vftable{};

// IDA names; not in the PDB.
inline float f0_6999 = 0.7f;
inline float f0_2 = 0.2f;
// Not named in the PDB or IDA; names chosen here. The data after f0_2, which no code of the exe references: an int
// (256) and three floats. The PDB's ImageSpaceEffectBloom has unreferenced statics (iBlurTexSize, pfDefaultBloomData)
// these may be; nothing places them.
inline int32_t data_11AE6D0 = 256;
using data_11AE6D4_t = float[3];
inline data_11AE6D4_t data_11AE6D4{0.03f, 0.2f, 0.7f};
