#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"

// The bloom image space effect: multiplies the alpha, blurs it through two buffers and blends it back.
class ImageSpaceEffectBloom : public ImageSpaceEffect {
public:
    enum EffectListEnum : int32_t {
        EFFECT_ALPHAMULT = 0x0,
        EFFECT_BLUR = 0x1,
        EFFECT_BLEND = 0x2,
        EFFECT_COUNT = 0x3,
    };

    enum TextureListEnum : int32_t {
        TEXTURE_DEST = 0x0,
        TEXTURE_SRC = 0x1,
        TEXTURE_BLUR1 = 0x2,
        TEXTURE_BLUR2 = 0x3,
        TEXTURE_COUNT = 0x4,
    };

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

    // The alpha adds Setup starts with, and UpdateParams uses while it has no bloom data.
    static inline float fAlphaAddInterior = 0.7f;
    static inline float fAlphaAddExterior = 0.2f;
};
