#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

#include <stdint.h>


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

// The blur image-space effect: blurs its input over iRadius texels, optionally through a bright pass first.
class ImageSpaceEffectBlur : public ImageSpaceEffect {
public:
    ImageSpaceEffectBlur();
    ~ImageSpaceEffectBlur() override;

    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    bool UpdateParams(ImageSpaceEffectParam* pParam) override;
    virtual void SetRadius(int32_t aiRadius);
    virtual void SetBrightPass(bool abBrightPass);

    int32_t iRadius;
    bool bBrightPass;

    static inline float fBlurRadius;
    // The width and height of the blur's render targets.
    static inline int32_t iBlurTexSize = 256;
    static inline float fSpecularMult = 1.0f;
    static inline float fEmmisiveMult = 1.0f;
    static inline float fBrightScale = 1.0f;
    static inline float fBrightClamp;
};

// The blur kernel the shaders read: 15 taps, offsets -7..7 in x and y, weights in z. UpdateParams writes
// the weights and passes the 2r+1 taps around entry 7; Setup passes 2r-1 taps around entry 5.
using ImageSpaceEffectBlurKernel = NiPoint4[15];
inline ImageSpaceEffectBlurKernel pgBlurWeightsA{
    {-7.0f, -7.0f},
    {-6.0f, -6.0f},
    {-5.0f, -5.0f},
    {-4.0f, -4.0f},
    {-3.0f, -3.0f},
    {-2.0f, -2.0f},
    {-1.0f, -1.0f},
    {},
    {1.0f, 1.0f},
    {2.0f, 2.0f},
    {3.0f, 3.0f},
    {4.0f, 4.0f},
    {5.0f, 5.0f},
    {6.0f, 6.0f},
    {7.0f, 7.0f}
};

// The precomputed kernels of radii 1 to 7 (entry r - 1), laid out as pgBlurWeightsA; UpdateParams interpolates their
// weights.
using ImageSpaceEffectBlurKernelTable = NiPoint4[7][15];
// Offsets -7..7 in x and y, the weights of the kernel of radius r - 1 in z (the zero weights outside radius 1 to 6
// are what aBlurKernels_Init stores at start-up).
inline ImageSpaceEffectBlurKernelTable aBlurKernels{
    // radius 1
    {{-7.0f, -7.0f, 0.0f, 0.0f},
     {-6.0f, -6.0f, 0.0f, 0.0f},
     {-5.0f, -5.0f, 0.0f, 0.0f},
     {-4.0f, -4.0f, 0.0f, 0.0f},
     {-3.0f, -3.0f, 0.0f, 0.0f},
     {-2.0f, -2.0f, 0.0f, 0.0f},
     {-1.0f, -1.0f, 0.10650672f, 0.0f},
     {0.0f, 0.0f, 0.78698575f, 0.0f},
     {1.0f, 1.0f, 0.10650672f, 0.0f},
     {2.0f, 2.0f, 0.0f, 0.0f},
     {3.0f, 3.0f, 0.0f, 0.0f},
     {4.0f, 4.0f, 0.0f, 0.0f},
     {5.0f, 5.0f, 0.0f, 0.0f},
     {6.0f, 6.0f, 0.0f, 0.0f},
     {7.0f, 7.0f, 0.0f, 0.0f}},
    // radius 2
    {{-7.0f, -7.0f, 0.0f, 0.0f},
     {-6.0f, -6.0f, 0.0f, 0.0f},
     {-5.0f, -5.0f, 0.0f, 0.0f},
     {-4.0f, -4.0f, 0.0f, 0.0f},
     {-3.0f, -3.0f, 0.0f, 0.0f},
     {-2.0f, -2.0f, 0.05448857f, 0.0f},
     {-1.0f, -1.0f, 0.24420148f, 0.0f},
     {0.0f, 0.0f, 0.40261993f, 0.0f},
     {1.0f, 1.0f, 0.24420148f, 0.0f},
     {2.0f, 2.0f, 0.05448857f, 0.0f},
     {3.0f, 3.0f, 0.0f, 0.0f},
     {4.0f, 4.0f, 0.0f, 0.0f},
     {5.0f, 5.0f, 0.0f, 0.0f},
     {6.0f, 6.0f, 0.0f, 0.0f},
     {7.0f, 7.0f, 0.0f, 0.0f}},
    // radius 3
    {{-7.0f, -7.0f, 0.0f, 0.0f},
     {-6.0f, -6.0f, 0.0f, 0.0f},
     {-5.0f, -5.0f, 0.0f, 0.0f},
     {-4.0f, -4.0f, 0.0f, 0.0f},
     {-3.0f, -3.0f, 0.03663277f, 0.0f},
     {-2.0f, -2.0f, 0.11128068f, 0.0f},
     {-1.0f, -1.0f, 0.21674521f, 0.0f},
     {0.0f, 0.0f, 0.27068216f, 0.0f},
     {1.0f, 1.0f, 0.21674521f, 0.0f},
     {2.0f, 2.0f, 0.11128068f, 0.0f},
     {3.0f, 3.0f, 0.03663277f, 0.0f},
     {4.0f, 4.0f, 0.0f, 0.0f},
     {5.0f, 5.0f, 0.0f, 0.0f},
     {6.0f, 6.0f, 0.0f, 0.0f},
     {7.0f, 7.0f, 0.0f, 0.0f}},
    // radius 4
    {{-7.0f, -7.0f, 0.0f, 0.0f},
     {-6.0f, -6.0f, 0.0f, 0.0f},
     {-5.0f, -5.0f, 0.0f, 0.0f},
     {-4.0f, -4.0f, 0.02763049f, 0.0f},
     {-3.0f, -3.0f, 0.06628214f, 0.0f},
     {-2.0f, -2.0f, 0.12383159f, 0.0f},
     {-1.0f, -1.0f, 0.18017383f, 0.0f},
     {0.0f, 0.0f, 0.20416367f, 0.0f},
     {1.0f, 1.0f, 0.18017383f, 0.0f},
     {2.0f, 2.0f, 0.12383159f, 0.0f},
     {3.0f, 3.0f, 0.06628214f, 0.0f},
     {4.0f, 4.0f, 0.02763049f, 0.0f},
     {5.0f, 5.0f, 0.0f, 0.0f},
     {6.0f, 6.0f, 0.0f, 0.0f},
     {7.0f, 7.0f, 0.0f, 0.0f}},
    // radius 5
    {{-7.0f, -7.0f, 0.0f, 0.0f},
     {-6.0f, -6.0f, 0.0f, 0.0f},
     {-5.0f, -5.0f, 0.022190502f, 0.0f},
     {-4.0f, -4.0f, 0.045588955f, 0.0f},
     {-3.0f, -3.0f, 0.07981137f, 0.0f},
     {-2.0f, -2.0f, 0.11906464f, 0.0f},
     {-1.0f, -1.0f, 0.15136077f, 0.0f},
     {0.0f, 0.0f, 0.16396722f, 0.0f},
     {1.0f, 1.0f, 0.15136077f, 0.0f},
     {2.0f, 2.0f, 0.11906464f, 0.0f},
     {3.0f, 3.0f, 0.07981137f, 0.0f},
     {4.0f, 4.0f, 0.045588955f, 0.0f},
     {5.0f, 5.0f, 0.022190502f, 0.0f},
     {6.0f, 6.0f, 0.0f, 0.0f},
     {7.0f, 7.0f, 0.0f, 0.0f}},
    // radius 6
    {{-7.0f, -7.0f, 0.0f, 0.0f},
     {-6.0f, -6.0f, 0.018543981f, 0.0f},
     {-5.0f, -5.0f, 0.03416691f, 0.0f},
     {-4.0f, -4.0f, 0.05633172f, 0.0f},
     {-3.0f, -3.0f, 0.08310858f, 0.0f},
     {-2.0f, -2.0f, 0.10971923f, 0.0f},
     {-1.0f, -1.0f, 0.12961796f, 0.0f},
     {0.0f, 0.0f, 0.13702281f, 0.0f},
     {1.0f, 1.0f, 0.12961796f, 0.0f},
     {2.0f, 2.0f, 0.10971923f, 0.0f},
     {3.0f, 3.0f, 0.08310858f, 0.0f},
     {4.0f, 4.0f, 0.05633172f, 0.0f},
     {5.0f, 5.0f, 0.03416691f, 0.0f},
     {6.0f, 6.0f, 0.018543981f, 0.0f},
     {7.0f, 7.0f, 0.0f, 0.0f}},
    // radius 7
    {{-7.0f, -7.0f, 0.015928362f, 0.0f},
     {-6.0f, -6.0f, 0.027077802f, 0.0f},
     {-5.0f, -5.0f, 0.042423215f, 0.0f},
     {-4.0f, -4.0f, 0.061254777f, 0.0f},
     {-3.0f, -3.0f, 0.081512466f, 0.0f},
     {-2.0f, -2.0f, 0.09996681f, 0.0f},
     {-1.0f, -1.0f, 0.112988554f, 0.0f},
     {0.0f, 0.0f, 0.1176958f, 0.0f},
     {1.0f, 1.0f, 0.112988554f, 0.0f},
     {2.0f, 2.0f, 0.09996681f, 0.0f},
     {3.0f, 3.0f, 0.081512466f, 0.0f},
     {4.0f, 4.0f, 0.061254777f, 0.0f},
     {5.0f, 5.0f, 0.042423215f, 0.0f},
     {6.0f, 6.0f, 0.027077802f, 0.0f},
     {7.0f, 7.0f, 0.015928362f, 0.0f}}
};

inline void ImageSpaceEffectFullScreenBlur::CheckCamera(NiCamera* apCam) {
    if (apCam) {
        NiPoint3 kDirDelta;
        NiPoint3 kDirection;
        NiPoint3 kLocDelta;
        NiPoint3 kLocation = apCam->GetWorldTranslate();
        kDirection = apCam->GetWorldDirection();
        kLocDelta = kLocation - ImageSpaceEffectFullScreenBlur::kCamLoc;
        kDirDelta = kDirection - ImageSpaceEffectFullScreenBlur::kCamDir;
        float fLocZ = kLocDelta.z / ImageSpaceEffectFullScreenBlur::kCamLoc.z;
        float fLocY = kLocDelta.y / ImageSpaceEffectFullScreenBlur::kCamLoc.y;
        float fLocX = kLocDelta.x / ImageSpaceEffectFullScreenBlur::kCamLoc.x;
        NiPoint3 kLocChange(fLocX, fLocY, fLocZ);
        float fDirZ = kDirDelta.z / ImageSpaceEffectFullScreenBlur::kCamDir.z;
        float fDirY = kDirDelta.y / ImageSpaceEffectFullScreenBlur::kCamDir.y;
        float fDirX = kDirDelta.x / ImageSpaceEffectFullScreenBlur::kCamDir.x;
        NiPoint3 kDirChange(fDirX, fDirY, fDirZ);
        ImageSpaceEffectFullScreenBlur::bMoving = kLocChange.x > 0.01 ||
            kLocChange.y > 0.01 ||
            kLocChange.z > 0.01 ||
            kDirChange.x > 0.01 ||
            kDirChange.y > 0.01 ||
            kDirChange.z > 0.01;
        ImageSpaceEffectFullScreenBlur::kCamLoc = kLocation;
        ImageSpaceEffectFullScreenBlur::kCamDir = kDirection;
    }
    else {
        ImageSpaceEffectFullScreenBlur::kCamDir = NiPoint3::UNIT_ALL;
        ImageSpaceEffectFullScreenBlur::bMoving = false;
    }
}
