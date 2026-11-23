#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

#include <stdint.h>

class NiScreenElements;

class ImageSpaceEffectWaterDisplacement : public ImageSpaceEffect {
public:
    using fDisplaceOffset_t = BSSimpleArray<NiPoint2, 1024>;
    using fLastDisplaceOffset_t = BSSimpleArray<NiPoint2, 1024>;

    ImageSpaceEffectWaterDisplacement();
    ~ImageSpaceEffectWaterDisplacement() override;

    void Render(
        NiScreenElements* apGeometry,
        NiDX9Renderer* apRenderer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    ) override;
    void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
    bool IsActive() override;

    static ImageSpaceEffectWaterDisplacement* GetInstance();

    NiPointer<NiScreenElements> spPartialScreenTriShape;
    float fTime;
    float fLastTime;
    float fTimeBetweenRipples;
    float fTimeFromLastRipple;
    float fTimeBetweenSimulations;
    float fTimeFromLastSimulation;

    static inline ImageSpaceEffectWaterDisplacement* pInstance;
    static inline bool bClearSimulation;
    static inline bool bWadingWater;
    static inline float fBlendAmount;
    static inline int32_t iNumRipplesThisFrame;
    static inline bool bRanSimulationThisFrame;
    static inline NiColorA RainVariables{};
    static inline NiColorA WadingVariables{};
    static inline NiPoint2 fTextureOffset{};
    static NiColorA TransMatrixRowOne;
    static NiColorA TransMatrixRowTwo;
    static fDisplaceOffset_t fDisplaceOffset;
    static fLastDisplaceOffset_t fLastDisplaceOffset;
};
