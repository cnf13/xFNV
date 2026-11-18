#include "BSShader/ImageSpaceEffectVolumetricFog.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

ImageSpaceEffectVolumetricFog::ImageSpaceEffectVolumetricFog() : FogColor(0.0f, 0.0f, 0.0f, 0.0f) {}

// Frees the shader parameters; the base destroys the rest. The slot is cleared in place, as the object is going.
ImageSpaceEffectVolumetricFog::~ImageSpaceEffectVolumetricFog() {
    ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[0].m_pObject;
    if (pParam) {
        delete pParam;
        pEffectParamList.m_pBase[0].m_pObject = nullptr;
    }
}

void ImageSpaceEffectVolumetricFog::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    XFNV_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
    ImageSpaceShaderParam* pparam;
    SetEffectCount(1);
    pTextures.SetSize(2u);
    ImageSpaceShaderParam* v4 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    spParam.m_pObject = v4;
    if (v4) {
        pparam = ::new (v4) ImageSpaceShaderParam;
    }
    else {
        pparam = nullptr;
    }
    pparam->InitConstantMap(0, 1);
    pparam->SetPixelConstant(0, 1.0f, 1.0f, 1.0f, 1.0f);
    spParam.m_pObject = pparam;
    pEffectParamList.SetAt(0, spParam);
    SetEffectInputs(0, 1, -1, NiTexturingProperty::FILTER_BILERP, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(0, apManager->pEffectList.m_pBase[42], pparam, 0);
}

bool ImageSpaceEffectVolumetricFog::IsActive() {
    return pEffectList.m_pBase->m_pObject->IsActive();
}

bool ImageSpaceEffectVolumetricFog::UpdateParams(ImageSpaceEffectParam* apParam) {
    pEffectParamList.m_pBase->m_pObject->SetPixelConstant(0, FogColor.r, FogColor.g, FogColor.b, 1.0);
    return false;
}
