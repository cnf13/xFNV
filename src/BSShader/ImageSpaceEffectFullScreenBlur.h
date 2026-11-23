#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiCamera;

// The full-screen blur image-space effect: downsample, blur at one of several widths, then upsample or copy back.
class ImageSpaceEffectFullScreenBlur : public ImageSpaceEffect {
public:
    enum EffectListEnum : int32_t {
        EFFECT_DOWNSAMPLE = 0x0,
        EFFECT_BLUR3 = 0x1,
        EFFECT_BLUR5 = 0x2,
        EFFECT_BLUR7 = 0x3,
        EFFECT_BLUR9 = 0x4,
        EFFECT_BLUR11 = 0x5,
        EFFECT_BLUR13 = 0x6,
        EFFECT_BLUR15 = 0x7,
        EFFECT_BLUR_END = 0x8,
        EFFECT_UPSAMPLE = 0x8,
        EFFECT_BLENDABLE_UPSAMPLE = 0x9,
        EFFECT_COPY = 0xa,
        EFFECT_COUNT = 0xb,
    };

    enum TextureListEnum : int32_t {
        TEXTURE_DEST = 0x0,
        TEXTURE_SRC = 0x1,
        TEXTURE_BLUR1 = 0x2,
        TEXTURE_BLUR2 = 0x3,
        TEXTURE_COUNT = 0x4,
    };

    ImageSpaceEffectFullScreenBlur();
    ~ImageSpaceEffectFullScreenBlur() override;

    void Render(
        NiScreenElements* apGeometry,
        NiDX9Renderer* apRenderer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    ) override;
    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    void BorrowTextures(ImageSpaceEffectParam* pParam) override;
    void ReturnTextures() override;
    bool IsActive() override;
    bool UpdateParams(ImageSpaceEffectParam* apParam) override;

    static void CheckCamera(NiCamera* apCam);

    ImageSpaceTexture pBufferA[3];

    static inline float fRadius;
    static inline bool bMoving;
    static inline NiPoint3 kCamDir{};
    static inline NiPoint3 kCamLoc{};
};
