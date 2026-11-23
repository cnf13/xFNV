#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

#include <stdint.h>

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
