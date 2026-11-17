#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"

// The sunbeams (god rays) image space effect: a stencil copy and a radial blur of the sun's disc into a swap texture.
class ImageSpaceEffectSunbeams : public ImageSpaceEffect {
public:
    enum EffectListEnum : int32_t {
        EFFECT_STENCILCOPY = 0x0,
        EFFECT_SUNBEAMS = 0x1,
        EFFECT_COUNT = 0x2,
    };

    enum TextureListEnum : int32_t {
        TEXTURE_DEST = 0x0,
        TEXTURE_SRC = 0x1,
        TEXTURE_SWAP = 0x2,
        TEXTURE_COUNT = 0x3,
    };

    ImageSpaceEffectSunbeams();
    ~ImageSpaceEffectSunbeams() override;

    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    void BorrowTextures(ImageSpaceEffectParam* apParam) override;
    void ReturnTextures() override;
    bool IsActive() override;
    bool UpdateParams(ImageSpaceEffectParam* apParam) override;

    ImageSpaceTexture SwapTex;

    static NiPoint2 kSunPos;
};
