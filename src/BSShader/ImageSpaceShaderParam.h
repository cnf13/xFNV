#pragma once

#include "BSShader/ImageSpaceEffectParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

#include <stdint.h>

class NiD3DShaderConstantMap;
class NiVector4;

// The parameters of an ImageSpaceShader pass: texture filter modes, clear flags and colour, the vertex and pixel
// constant groups and their maps, the input textures and the render states.
class ImageSpaceShaderParam : public ImageSpaceEffectParam {
public:
    ImageSpaceShaderParam();
    ~ImageSpaceShaderParam() override;

    // Clears the constant groups, builds new vertex and pixel constant maps and gives each a "Constant Group" entry
    // over aiVSize + 3 (the three reserved registers) and aiPSize new registers.
    void InitConstantMap(int32_t aiVSize, int32_t aiPSize);
    // Frees the constant groups.
    void ClearConstantGroups();
    // Sets render state auiState to auiValue in pRenderStates, via the map's SetAt.
    void SetRenderState(uint32_t auiState, uint32_t auiValue);
    void CopyConstantGroup(int32_t aiIndex, void* pvSrc, uint32_t stCount);
    // Sets the pixel constant aiIndex to the four values.
    void SetPixelConstant(int32_t aiIndex, float afVal0, float afVal1, float afVal2, float afVal3);
    // Sets the vertex constant aiIndex (after the three reserved registers) to the four values.
    void SetVertexConstant(int32_t aiIndex, float afVal0, float afVal1, float afVal2, float afVal3);
    void SetTextureOffsets(int32_t auiTexture, int32_t auiWidth, int32_t auiHeight);
    void SetBlurClearMode();
    void SetPixelConstants(int32_t registerIndex, NiVector4* apValue);

    // Deleting a parameter frees it with MemoryManager::Deallocate.
    static void operator delete(void* apMemory);

    NiTexturingProperty::FilterMode iFiltermode;
    NiRenderer::ClearFlags iClearFlags;
    NiColorA fClearColor;
    float* pfVertexConstantGroup;
    int32_t iVSize;
    float* pfPixelConstantGroup;
    int32_t iPSize;
    NiPointer<NiD3DShaderConstantMap> spVertexConstantMap;
    NiPointer<NiD3DShaderConstantMap> spPixelConstantMap;
    bool bUseTexOffset[3];
    NiTObjectArray<NiPointer<NiTexture>> pInputTextures;
    NiTPrimitiveArray<NiTexturingProperty::FilterMode> pInputFilterModes;
    NiTMap<uint32_t, uint32_t> pRenderStates;
};

// The parameters are not reference counted: the NiPointer arrays of them (ImageSpaceEffect::pEffectParamList) only
// store and drop the pointer, so NiPointer's reference counting is nothing for them.
inline void NiIncRefCount(ImageSpaceShaderParam*) {}

inline void NiDecRefCount(ImageSpaceShaderParam*) {}
