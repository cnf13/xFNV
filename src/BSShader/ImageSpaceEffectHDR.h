#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"

class ImageSpaceEffectHDR : public ImageSpaceEffect {
public:
    // The parameters pfData points to until SetParams replaces them (eye adaptation speed, blur passes, emissive
    // multiplier, target luminance and clamps, ...: HDRParams' order).
    using DefaultHDRData = float[15];

    ImageSpaceEffectHDR();
    ~ImageSpaceEffectHDR() override;

    void Render(
        NiScreenElements* apGeometry,
        NiDX9Renderer* apRenderer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    ) override;
    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    void Shutdown() override;
    bool IsActive() override;
    bool UpdateParams(ImageSpaceEffectParam* apParam) override;
    // PC source returns void and stores the pointer to the HDR float data record.
    virtual void SetParams(float* apfData);

    // The HDR values in the stored data record pfData.
    float GetBrightScale();
    float GetEyeAdaptSpeed();
    float GetTargetLUM();
    float GetUpperLUMClamp();
    float GetEmissiveHDRMult();
    float GetNumBlurPasses();
    float GetBrightClamp();

    // Gives back the adapted light's rendered texture and clears the slot, when it holds one.
    static void ClearAdaptedLight();

    // The HDR parameters by value; the PC code reads them through pfData.
    float fHDRParam[4];
    bool bUseAnisoDownsample;
    // The HDR parameter data SetParams stores.
    float* pfData;

    static ImageSpaceTexture kAdaptedLight;
    static inline bool bUseAlphaMask;
    static inline float fAlpha = 1.0f;
    // While set, UpdateParams drops the adapted light each frame.
    static inline bool bResetAdaptedLight;
    static inline bool bShowDebugTextures;
    static inline DefaultHDRData
        pfDefaultHDRData{0.5f, 7.0f, 999.0f, 1.0f, 1.0f, 1.0f, 2.25f, 0.225f, 1.0f, 0.0f, 0.0f, 1.5f, 1.5f, 1.0f, 0.0f};
};

// The constant groups Setup copies into its shader parameters.
using HDRConstantGroup40 = uint8_t[0x40];
inline HDRConstantGroup40 kHDRConstants11ADD48{0, 0, 128, 191, 0,   0,   128, 191, 0,   0,   128, 62,  0,   0,   0,
                                               0, 0, 0,   128, 63,  0,   0,   128, 191, 0,   0,   128, 62,  0,   0,
                                               0, 0, 0,   0,   128, 63,  0,   0,   128, 63,  0,   0,   128, 62,  0,
                                               0, 0, 0,   0,   0,   128, 191, 0,   0,   128, 63,  0,   0,   128, 62};
using HDRConstantGroup100 = uint8_t[0x100];
inline HDRConstantGroup100 kHDRConstants11ADD88{
    0, 0, 192, 191, 0, 0, 192, 191, 0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 0,   191, 0, 0, 192, 191, 0, 0, 128, 61,
    0, 0, 0,   0,   0, 0, 0,   63,  0, 0, 192, 191, 0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 192, 63,  0, 0, 192, 191,
    0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 192, 191, 0, 0, 0,   191, 0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 0,   191,
    0, 0, 0,   191, 0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 0,   63,  0, 0, 0,   191, 0, 0, 128, 61,  0, 0, 0,   0,
    0, 0, 192, 63,  0, 0, 0,   191, 0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 192, 191, 0, 0, 0,   63,  0, 0, 128, 61,
    0, 0, 0,   0,   0, 0, 0,   191, 0, 0, 0,   63,  0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 0,   63,  0, 0, 0,   63,
    0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 192, 63,  0, 0, 0,   63,  0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 192, 191,
    0, 0, 192, 63,  0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 0,   191, 0, 0, 192, 63,  0, 0, 128, 61,  0, 0, 0,   0,
    0, 0, 0,   63,  0, 0, 192, 63,  0, 0, 128, 61,  0, 0, 0,   0,   0, 0, 192, 63,  0, 0, 192, 63,  0, 0, 128, 61
};
using HDRConstantGroup90 = uint8_t[0x90];
inline HDRConstantGroup90 kHDRConstants11ADE88{
    0,  0,   0,   63, 0, 0, 0,   0,  10, 215, 163, 61, 0, 0, 0,   0,  0,  0,   192, 63, 0, 0, 0,   0,
    10, 215, 163, 61, 0, 0, 0,   0,  0,  0,   32,  64, 0, 0, 0,   0,  10, 215, 35,  61, 0, 0, 0,   0,
    0,  0,   0,   63, 0, 0, 128, 63, 10, 215, 35,  62, 0, 0, 0,   0,  0,  0,   192, 63, 0, 0, 128, 63,
    10, 215, 35,  62, 0, 0, 0,   0,  0,  0,   32,  64, 0, 0, 128, 63, 10, 215, 163, 61, 0, 0, 0,   0,
    0,  0,   0,   63, 0, 0, 0,   64, 10, 215, 35,  62, 0, 0, 0,   0,  0,  0,   192, 63, 0, 0, 0,   64,
    10, 215, 35,  62, 0, 0, 0,   0,  0,  0,   32,  64, 0, 0, 0,   64, 10, 215, 163, 61
};
