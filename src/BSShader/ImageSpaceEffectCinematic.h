#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

// The cinematic (colour grading) image-space effect, with the night-eye overlay.
class ImageSpaceEffectCinematic : public ImageSpaceEffect {
public:
    // The parameters pfData points to until SetParams replaces them.
    using DefaultData = float[12];

    ImageSpaceEffectCinematic();
    ~ImageSpaceEffectCinematic() override;

    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    bool IsActive() override;
    bool UpdateParams(ImageSpaceEffectParam* pParam) override;
    virtual float* GetParams();
    virtual void SetParams(float* apfData, uint32_t auiEnables);
    virtual void SetNightEyeParams(float* apfData);
    virtual void SetNightEye(bool abNightEye);

    bool bNightEye;
    uint8_t padding59[3];
    float* pfNightEyeData;
    float* pfData;
    uint32_t uiEnables;
    float pfEnables[2][4];
    float pfParams[16];

    static inline DefaultData pfDefaultData{1.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static inline NiPoint4 pfDefaultNightEyeData{0.21f, 0.5f, 0.78f, 2.5f};
};

// A float that is never read.
inline float data_11AE6C4 = 0.03f;
