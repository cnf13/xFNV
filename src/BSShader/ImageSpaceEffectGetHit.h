#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"

// The image-space effect played when the player is hit: blurs and double-vision copies of the frame that fade with
// damping constants.
class ImageSpaceEffectGetHit : public ImageSpaceEffect {
public:
    enum EffectListEnum : int32_t {
        EFFECT_COPY = 0x0,
        EFFECT_BLUR1 = 0x1,
        EFFECT_BLUR2 = 0x2,
        EFFECT_DOUBLEVISION = 0x3,
        EFFECT_COUNT = 0x4,
    };

    enum TextureListEnum : int32_t {
        TEXTURE_DEST = 0x0,
        TEXTURE_SRC = 0x1,
        TEXTURE_BLUR1 = 0x2,
        TEXTURE_BLUR2 = 0x3,
        TEXTURE_COUNT = 0x4,
    };

    enum GetHitDataType : int32_t {
        GETHIT_BLUR_RADIUS = 0x0,
        GETHIT_BLUR_DAMPING_CONSTANT = 0x1,
        GETHIT_DAMPING_CONSTANT = 0x2,
        GETHIT_DATA_COUNT = 0x3,
    };

    ImageSpaceEffectGetHit();
    ~ImageSpaceEffectGetHit() override;

    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    void BorrowTextures(ImageSpaceEffectParam* pParam) override;
    void ReturnTextures() override;
    bool IsActive() override;
    bool UpdateParams(ImageSpaceEffectParam* pParam) override;
    virtual void SetParams(float* apfData);

    float* pfData;
    ImageSpaceTexture pBufferA[2];

    static inline float fTriggerValue;
    static inline float fBlurAmount;
    static inline float fAmplitude;
    static inline float fTexOffsetHit;
    static inline float fTexOffsetBlocked;
};
