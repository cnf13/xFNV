#include "BSShader/ImageSpaceEffectTV.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

NiPointer<NiTexture> ImageSpaceEffectTV::spScanlinesTexture;
NiPointer<NiTexture> ImageSpaceEffectTV::spDistortTexture;

ImageSpaceEffectTV::ImageSpaceEffectTV() {
    pTextures.SetSize(4u);
}

// Releases the shared scan-line and distortion textures, frees the effect's own texture slots 1 to 3 and its
// shader parameters; the base destroys the rest. The parameter slot is cleared in place, as the object is going.
ImageSpaceEffectTV::~ImageSpaceEffectTV() {
    spScanlinesTexture = nullptr;
    spDistortTexture = nullptr;
    for (uint32_t i = 1; i <= 3; ++i) {
        ImageSpaceTexture* pTexture = pTextures.m_pBase[i];
        if (pTexture) {
            delete pTexture;
            pTextures.SetAt(i, nullptr);
        }
    }
    ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[0].m_pObject;
    if (pParam) {
        delete pParam;
        pEffectParamList.m_pBase[0].m_pObject = nullptr;
    }
}

bool ImageSpaceEffectTV::IsActive() {
    return false;
}

void ImageSpaceEffectTV::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(1);
    NiTPrimitiveArray<ImageSpaceTexture*>* pTextureList = &pTextures;
    pTextures.SetSize(4u);
    BSShaderManager::GetTexture(
        "Data\\Textures\\Pipboy3000\\PipboyDistortEffectMap.dds",
        true,
        ImageSpaceEffectTV::spDistortTexture,
        false,
        false
    );
    BSShaderManager::GetTexture(
        "Data\\Textures\\Pipboy3000\\PipboyScanlines.dds",
        true,
        ImageSpaceEffectTV::spScanlinesTexture,
        false,
        false
    );
    NiTexture* pDistortTexture = ImageSpaceEffectTV::spDistortTexture.m_pObject;
    NiTexture* pScanlineTexture = ImageSpaceEffectTV::spScanlinesTexture.m_pObject;
    ImageSpaceTexture* pTexture = new ImageSpaceTexture;
    pTextureList->SetAt(1u, pTexture);
    pTextures.m_pBase[1]->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    pTextures.m_pBase[1]->SetTexture(static_cast<NiTexture*>(nullptr));
    pTexture = new ImageSpaceTexture;
    pTextureList->SetAt(2u, pTexture);
    pTextures.m_pBase[2]->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    pTextures.m_pBase[2]->SetTexture(pDistortTexture);
    pTexture = new ImageSpaceTexture;
    pTextureList->SetAt(3u, pTexture);
    pTextures.m_pBase[3]->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
    pTextures.m_pBase[3]->SetTexture(pScanlineTexture);
    ImageSpaceTexture** m_pBase = pTextures.m_pBase;
    float foffsetx = 0.0f;
    float foffsety = 0.0f;
    if (*m_pBase) {
        NiTexture* ptex = (*m_pBase)->GetTexture();
        int32_t iWidth = static_cast<int32_t>(ptex->GetWidth());
        double dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        foffsetx = 1.0 / dWidth;
        int32_t iHeight = static_cast<int32_t>(ptex->GetHeight());
        double dHeight = iHeight;
        if (iHeight < 0) {
            dHeight += 4294967296.0f;
        }
        foffsety = 1.0 / dHeight;
    }
    ImageSpaceShaderParam* pparam = new ImageSpaceShaderParam;
    pparam->InitConstantMap(0, 5);
    pparam->bUseTexOffset[1] = false;
    pparam->bUseTexOffset[2] = false;
    pparam->SetPixelConstant(0, foffsetx, foffsety, 0.0f, 0.0f);
    pparam->SetPixelConstant(1, ImageSpaceEffectTV::fPulse, 0.0f, 0.0f, 0.0f);
    pparam->SetPixelConstant(
        2,
        ImageSpaceEffectTV::fDistortVerticalScale,
        ImageSpaceEffectTV::fDistortHorizontalScale,
        ImageSpaceEffectTV::fDistortVerticalPosition,
        0.0f
    );
    pparam->SetPixelConstant(
        3,
        ImageSpaceEffectTV::fFakeDistortVerticalScale,
        ImageSpaceEffectTV::fFakeDistortHorizontalScale,
        ImageSpaceEffectTV::fFakeDistortVerticalPosition,
        0.0f
    );
    pparam
        ->SetPixelConstant(4, ImageSpaceEffectTV::Tint.r, ImageSpaceEffectTV::Tint.g, ImageSpaceEffectTV::Tint.b, 1.0f);
    pEffectParamList.SetAt(0, pparam);
    SetEffectInputs(
        0,
        1,
        2,
        3,
        NiTexturingProperty::FILTER_NEAREST,
        NiTexturingProperty::FILTER_NEAREST,
        NiTexturingProperty::FILTER_NEAREST
    );
    SetEffect(0, apManager->pEffectList.m_pBase[77], pparam, 0);
}

void ImageSpaceEffectTV::ReturnTextures() {
    // Calls the NiSourceTexture* overload with null.
    pTextures.m_pBase[1]->SetTexture(static_cast<NiSourceTexture*>(nullptr));
}

bool ImageSpaceEffectTV::UpdateParams(ImageSpaceEffectParam* pParam) {
    double v2 = 0.0;
    float v11 = 0.0;
    float v10 = 0.0;
    ImageSpaceTexture** m_pBase = pTextures.m_pBase;
    // The same unsigned-to-float conversion sequence as in Setup.
    if (m_pBase[1]) {
        NiTexture* Texture = m_pBase[1]->GetTexture();
        int32_t iWidth = static_cast<int32_t>(Texture->GetWidth());
        double dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        v11 = 1.0 / dWidth;
        int32_t iHeight = static_cast<int32_t>(Texture->GetHeight());
        double dHeight = iHeight;
        if (iHeight < 0) {
            dHeight += 4294967296.0f;
        }
        v10 = 1.0 / dHeight;
        v2 = 0.0;
    }
    ImageSpaceShaderParam* m_pObject = pEffectParamList.m_pBase->m_pObject;
    float fW = v2;
    float fZ = v2;
    m_pObject->SetPixelConstant(0, v11, v10, fZ, fW);
    m_pObject->SetPixelConstant(1, ImageSpaceEffectTV::fPulse, 0.0, 0.0, 0.0);
    m_pObject->SetPixelConstant(
        2,
        ImageSpaceEffectTV::fDistortVerticalScale,
        ImageSpaceEffectTV::fDistortHorizontalScale,
        ImageSpaceEffectTV::fDistortVerticalPosition,
        0.0
    );
    m_pObject->SetPixelConstant(
        3,
        ImageSpaceEffectTV::fFakeDistortVerticalScale,
        ImageSpaceEffectTV::fFakeDistortHorizontalScale,
        ImageSpaceEffectTV::fFakeDistortVerticalPosition,
        0.0
    );
    m_pObject
        ->SetPixelConstant(4, ImageSpaceEffectTV::Tint.r, ImageSpaceEffectTV::Tint.g, ImageSpaceEffectTV::Tint.b, 1.0);
    return false;
}
