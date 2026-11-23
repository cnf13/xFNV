#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiSourceTexture;

// The VATS scan-line image-space effect.
class ImageSpaceEffectVATSScan : public ImageSpaceEffect {
public:
    enum EffectListEnum : int32_t {
        EFFECT_VATS_SCAN = 0x0,
        EFFECT_COUNT = 0x1,
    };

    enum TextureListEnum : int32_t {
        TEXTURE_DEST = 0x0,
        TEXTURE_SRC_COLORS = 0x1,
        TEXTURE_SRC_MASK = 0x2,
        TEXTURE_SCANLINES = 0x3,
        TEXTURE_SCANMASK = 0x4,
        TEXTURE_COUNT = 0x5,
    };

    ImageSpaceEffectVATSScan();
    ~ImageSpaceEffectVATSScan() override;

    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    void ReturnTextures() override;
    bool UpdateParams(ImageSpaceEffectParam* pParam) override;

    // The scan-line parameters Setup/UpdateParams hand to the shader.
    static inline float fScanVerticalScale = 2.0f;
    static inline float fScanlineFrequency = 150.0f;
    static inline float fScanVerticalPosition;
    static inline NiColorA kTint{};
    // FOVATSEffectManager::PrepareScanEffect copies the manager's scan-line texture into spScanlinesTexture and assigns
    // the mask's NiRenderedTexture to spMaskTexture.
    static NiPointer<NiTexture> spScanlinesTexture;
    static NiPointer<NiTexture> spMaskTexture;
    static NiPointer<NiSourceTexture> PipboyDistortEffectMap;
};
