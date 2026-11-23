#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

// The image-space shader that unpacks the menu's packed regions. It declares no instance members (only the static
// PackedRegions, bChangedFlag, fOffsetX and fOffsetY), so it adds none to ImageSpaceShader.
class ImageSpaceShaderMenuUnpack : public ImageSpaceShader {
public:
    using kPackedRegions_t = NiTArray<void*, NiTMallocInterface<void*>>;

    ImageSpaceShaderMenuUnpack();

    // Loads the unpack programs: imagespace\1x\v\base.v.hlsl and imagespace\1x\p\copy.p.hlsl as COPYUNPACK, then the
    // stages and passes.
    void ReloadShaders() override;
    // The ImageSpaceEffect subobject's Render: draws the menu unpack geometry through the effect's programs and output
    // texture.
    void Render(
        NiScreenElements* apGeometry,
        NiDX9Renderer* apRenderer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    ) override;

    // Sets the unpack offset and marks it changed.
    static void SetOffset(float afX, float afY);

    static inline bool bChangedFlag = true;
    static inline float fOffsetX;
    static inline float fOffsetY;
    static kPackedRegions_t kPackedRegions;
};
