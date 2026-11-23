#pragma once

#include "BSShader/ImageSpaceEffect.h"

struct ImageSpaceTexture;

// The full-screen effect behind the menus: blends the screen with a copy of itself and a sepia texture.
class ImageSpaceEffectMenuBG : public ImageSpaceEffect {
public:
    enum EffectListEnum : int32_t {
        EFFECT_BLEND = 0x0,
        EFFECT_COUNT = 0x1,
    };

    enum TextureListEnum : int32_t {
        TEXTURE_DEST = 0x0,
        TEXTURE_SRC = 0x1,
        TEXTURE_SEPIA = 0x2,
        TEXTURE_COUNT = 0x3,
    };

    ImageSpaceEffectMenuBG();
    ~ImageSpaceEffectMenuBG() override;

    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    bool IsActive() override;
    bool UpdateParams(ImageSpaceEffectParam* apParam) override;

    void SetBlendEnabled(bool abBlendEnabled);
    void SetBlendValue(float afBlendValue);

    ImageSpaceTexture* pSepiaTexture;
    float fBlendValue;
    bool bBlendEnabled;
};
