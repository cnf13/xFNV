#include "BSShader/ImageSpaceEffectMenuBG.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

ImageSpaceEffectMenuBG::ImageSpaceEffectMenuBG() : pSepiaTexture(nullptr) {}

// The sepia texture (slot 2) and the first shader parameter are the effect's.
ImageSpaceEffectMenuBG::~ImageSpaceEffectMenuBG() {
    if (pSepiaTexture) {
        pSepiaTexture->~ImageSpaceTexture();
        MemoryManager_Singleton.Deallocate(pSepiaTexture);
        pTextures.SetAt(2u, nullptr);
    }
    if (pEffectParamList.m_pBase[0].m_pObject) {
        delete pEffectParamList.m_pBase[0].m_pObject;
        pEffectParamList.SetAt(0, nullptr);
    }
}

// One effect with new shader parameters (1 pixel constant) on the manager's menu background shader, and the sepia
// texture as texture 2 (loaded only without HDR).
void ImageSpaceEffectMenuBG::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    ImageSpaceShaderParam* pParam;
    SetEffectCount(1);
    pTextures.SetSize(3u);
    ImageSpaceTexture* pTexture =
        static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
    if (pTexture) {
        pTexture->bForceAniso = false;
        pTexture->bIsRenderedTexture = false;
        pTexture->bIsTextureBorrowed = false;
        pTexture->spTexture = nullptr;
        pTexture->eFilterMode = 0;
        pTexture->eClampMode = 0;
    }
    else {
        pTexture = nullptr;
    }
    pSepiaTexture = pTexture;
    if (!BSShaderManager::bHDR) {
        NiPointer<NiTexture> spSepiaTexture;
        BSShaderManager::GetTexture("Data\\Textures\\Interface\\Misc\\sepia.dds", true, spSepiaTexture, false, false);
        pSepiaTexture->SetTexture(spSepiaTexture.m_pObject);
    }
    pTextures.SetAt(2u, pSepiaTexture);
    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    if (pMemory) {
        pParam = ::new (pMemory) ImageSpaceShaderParam;
    }
    else {
        pParam = nullptr;
    }
    pParam->InitConstantMap(0, 1);
    pParam->SetPixelConstant(0, 0.0f, 0.0f, 0.0f, 0.0f);
    // The NiPointer's constructor and destructor do nothing, because the parameter class is not a NiRefObject.
    XFNV_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
    spParam.m_pObject = pParam;
    pEffectParamList.SetAt(0, spParam);
    SetEffectInputs(0, 1, 2, NiTexturingProperty::FILTER_BILERP, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(0, apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_MENUBG], pParam, 0);
}

// Whether its one effect is active.
bool ImageSpaceEffectMenuBG::IsActive() {
    return pEffectList.m_pBase->m_pObject->IsActive();
}

// Sets pixel constant 0 to (fBlendValue, 0, 0, 0).
bool ImageSpaceEffectMenuBG::UpdateParams(ImageSpaceEffectParam* apParam) {
    pEffectParamList.m_pBase[0].m_pObject->SetPixelConstant(0, fBlendValue, 0.0f, 0.0f, 0.0f);
    return true;
}

void ImageSpaceEffectMenuBG::SetBlendEnabled(bool abBlendEnabled) {
    bBlendEnabled = (bool)((uint8_t)abBlendEnabled);
}

void ImageSpaceEffectMenuBG::SetBlendValue(float afBlendValue) {
    if (bBlendEnabled) {
        fBlendValue = afBlendValue;
    }
}
