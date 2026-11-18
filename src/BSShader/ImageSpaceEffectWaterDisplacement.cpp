#include "BSShader/ImageSpaceEffectWaterDisplacement.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

NiColorA ImageSpaceEffectWaterDisplacement::TransMatrixRowOne(NiColorA_BLACK);
NiColorA ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo(NiColorA_BLACK);
ImageSpaceEffectWaterDisplacement::fDisplaceOffset_t ImageSpaceEffectWaterDisplacement::fDisplaceOffset;
ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset_t ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset;

ImageSpaceEffectWaterDisplacement::ImageSpaceEffectWaterDisplacement()
  : spPartialScreenTriShape(nullptr), fTime(0.0f), fLastTime(0.0f), fTimeBetweenRipples(0.1f),
    fTimeFromLastRipple(0.1f), fTimeBetweenSimulations(1.0f), fTimeFromLastSimulation(0.0f) {
    pTextures.SetSize(2u);
    pInstance = this;
    NiRenderedTexture::bStaticForceD3DFormat = false;
    fBlendAmount = 0.0f;
    RainVariables.r = 0.1f;
    RainVariables.g = 0.6f;
    RainVariables.b = 0.985f;
    RainVariables.a = 0.01f;
    WadingVariables.r = 0.4f;
    WadingVariables.g = 0.6f;
    WadingVariables.b = 0.985f;
    WadingVariables.a = 0.05f;
}

// Frees the nine sub-effects' shader parameters; the NiPointer member and the base release the rest. The slots are
// cleared in place, as the object is going.
ImageSpaceEffectWaterDisplacement::~ImageSpaceEffectWaterDisplacement() {
    for (uint32_t i = 0; i < 9; ++i) {
        ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[i].m_pObject;
        if (pParam) {
            delete pParam;
            pEffectParamList.m_pBase[i].m_pObject = nullptr;
        }
    }
}

// The effect list holds each shader's ImageSpaceEffect base.
void ImageSpaceEffectWaterDisplacement::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    XFNV_INLINED_LOCAL(ImageSpaceTexture, aTex, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(ImageSpaceTexture, kGreyTex, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(ImageSpaceTexture, apTex, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(ImageSpaceTexture, apThis, std::nullopt, std::nullopt);
    fTimeBetweenSimulations = 0.025f;
    // EXE-INLINED 0x4EBB70
    aTex.bForceAniso = false;
    aTex.bIsRenderedTexture = false;
    aTex.bIsTextureBorrowed = false;
    aTex.spTexture = nullptr;
    aTex.eFilterMode = 0;
    aTex.eClampMode = 0;
    float fNewTime = this->fTime + BSShaderManager::fTimerValues[1];
    this->fTime = fNewTime;
    fTimeFromLastRipple = fNewTime + fTimeFromLastRipple;
    // EXE-INLINED 0x4EBB70
    kGreyTex.bForceAniso = false;
    kGreyTex.bIsRenderedTexture = false;
    kGreyTex.bIsTextureBorrowed = false;
    kGreyTex.spTexture = nullptr;
    kGreyTex.eFilterMode = 0;
    kGreyTex.eClampMode = 0;
    // EXE-INLINED 0x4EBB70
    apTex.bForceAniso = false;
    apTex.bIsRenderedTexture = false;
    apTex.bIsTextureBorrowed = false;
    apTex.spTexture = nullptr;
    apTex.eFilterMode = 0;
    apTex.eClampMode = 0;
    kGreyTex.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_GREY);
    BSRenderedTexture* pMemory = static_cast<BSRenderedTexture*>(NiMemObject::operator new(sizeof(BSRenderedTexture)));
    BSRenderedTexture* pRendered;
    if (pMemory) {
        BSRenderedTexture* pTarget = pTextures.m_pBase[1]->GetRenderedTexture();
        NiRenderTargetGroup* pGroup = pTarget->GetGroup();
        NiDepthStencilBuffer* pDepth = pGroup->GetDepthStencilBuffer();
        BSRenderedTexture* pGrey = kGreyTex.GetRenderedTexture();
        NiRenderedTexture* pTexture = pGrey->GetTexture(0);
        pRendered = ::new (pMemory) BSRenderedTexture(pTexture, 1, pDepth);
    }
    else {
        pRendered = nullptr;
    }
    apTex.SetTexture(pRendered);
    // EXE-INLINED 0x4EBB70
    apThis.bForceAniso = false;
    apThis.bIsRenderedTexture = false;
    apThis.bIsTextureBorrowed = false;
    apThis.spTexture = nullptr;
    apThis.eFilterMode = 0;
    apThis.eClampMode = 0;
    if (ImageSpaceEffectWaterDisplacement::bClearSimulation) {
        ImageSpaceShader* pShader = static_cast<ImageSpaceShader*>(pEffectList.m_pBase[0].m_pObject);
        ImageSpaceEffect* peffect = pShader;
        ImageSpaceShaderParam* m_pObject = pEffectParamList.m_pBase->m_pObject;
        peffect->SetTexture(0, &apTex, NiTexturingProperty::FILTER_NEAREST);
        peffect->SetTexture(1, pTextures.m_pBase[1], NiTexturingProperty::FILTER_BILERP);
        peffect->Render(apGeometry, apRenderer, m_pObject, 1);
        apThis = *pTextures.m_pBase[1];
        aTex = apTex;
        apTex = apThis;
        ImageSpaceEffectWaterDisplacement::bClearSimulation = false;
    }
    if (ImageSpaceEffectWaterDisplacement::bWadingWater) {
        ImageSpaceShader* pShader2 = static_cast<ImageSpaceShader*>(pEffectList.m_pBase[1].m_pObject);
        ImageSpaceEffect* peffect_2 = pShader2;
        ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase[1].m_pObject;
        pparam->SetPixelConstant(0, this->fTime, 0.0f, 0.0f, 0.0f);
        pparam->SetPixelConstant(1, ImageSpaceEffectWaterDisplacement::fBlendAmount, 0.0f, 0.0f, 0.0f);
        pparam->SetPixelConstant(
            2, ImageSpaceEffectWaterDisplacement::fTextureOffset.x, ImageSpaceEffectWaterDisplacement::fTextureOffset.y,
            0.0f, 0.0f
        );
        pparam->SetPixelConstant(
            4, ImageSpaceEffectWaterDisplacement::RainVariables.r, ImageSpaceEffectWaterDisplacement::RainVariables.g,
            ImageSpaceEffectWaterDisplacement::RainVariables.b, ImageSpaceEffectWaterDisplacement::RainVariables.a
        );
        pparam->SetPixelConstant(
            5, ImageSpaceEffectWaterDisplacement::WadingVariables.r,
            ImageSpaceEffectWaterDisplacement::WadingVariables.g, ImageSpaceEffectWaterDisplacement::WadingVariables.b,
            ImageSpaceEffectWaterDisplacement::WadingVariables.a
        );
        peffect_2->SetTexture(0, &apTex, NiTexturingProperty::FILTER_NEAREST);
        peffect_2->SetTexture(1, pTextures.m_pBase[1], NiTexturingProperty::FILTER_NEAREST);
        peffect_2->Render(apGeometry, apRenderer, pparam, 1);
        if (ImageSpaceEffectWaterDisplacement::bWadingWater) {
            ImageSpaceEffectWaterDisplacement::iNumRipplesThisFrame = 0;
            ImageSpaceShader* pShader3 = static_cast<ImageSpaceShader*>(pEffectList.m_pBase[2].m_pObject);
            ImageSpaceEffect* peffect_3 = pShader3;
            ImageSpaceShaderParam* pparam_2 = pEffectParamList.m_pBase[2].m_pObject;
            peffect_3->SetTexture(0, &apTex, NiTexturingProperty::FILTER_NEAREST);
            if (ImageSpaceEffectWaterDisplacement::fDisplaceOffset.iSize) {
                NiPoint2 tmp;
                if (!spPartialScreenTriShape.m_pObject) {
                    NiUpdateData UpdateData;
                    NiScreenElementsData* pDataMemory =
                        static_cast<NiScreenElementsData*>(NiMemObject::operator new(sizeof(NiScreenElementsData)));
                    NiScreenElementsData* pData;
                    if (pDataMemory) {
                        pData = ::new (pDataMemory) NiScreenElementsData(false, false, 1, 1, 1, 4, 1, 2, 1);
                    }
                    else {
                        pData = nullptr;
                    }
                    if (pData) {
                        NiIncRefCount(pData);
                    }
                    NiScreenElements* pElementsMemory =
                        static_cast<NiScreenElements*>(NiMemObject::operator new(sizeof(NiScreenElements)));
                    NiScreenElements* pElements;
                    if (pElementsMemory) {
                        pElements = ::new (pElementsMemory) NiScreenElements(pData);
                    }
                    else {
                        pElements = nullptr;
                    }
                    spPartialScreenTriShape = pElements;
                    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
                        ->Insert(4, 0, nullptr);
                    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
                        ->SetRectangle(0, 0.0f, 0.01f, 1.0f, -0.01f);
                    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
                        ->UpdateBound();
                    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
                        ->SetTextures(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
                    spPartialScreenTriShape.m_pObject->UpdateProperties();
                    NiAVObject* pObject = spPartialScreenTriShape.m_pObject;
                    UpdateData.fTime = 0.0f;
                    UpdateData.bUpdateControllers = false;
                    UpdateData.bParallelUpdate = false;
                    UpdateData.bFoundParticles = false;
                    UpdateData.bFoundMorphController = false;
                    UpdateData.bSceneGraphChange = false;
                    pObject->Update(UpdateData);
                    // EXE-INLINED none: NiFixedString(const char*) and ~NiFixedString
                    XFNV_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
                    kName.m_kHandle = NiGlobalStringTable::AddString("Partial Screen Trishape");
                    spPartialScreenTriShape.m_pObject->SetName(kName);
                    NiGlobalStringTable::ReleaseHandle(kName.m_kHandle);
                    if (pData) {
                        NiDecRefCount(pData);
                    }
                }
                // The count is compared signed, by casting the unsigned size.
                for (int32_t i = 0; i < static_cast<int32_t>(ImageSpaceEffectWaterDisplacement::fDisplaceOffset.iSize);
                     ++i) {
                    ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.r =
                        ImageSpaceEffectWaterDisplacement::fDisplaceOffset.pBuffer[i].x -
                        ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.pBuffer[i].x;
                    float* pLastY = &ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.pBuffer[i].y;
                    ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.r =
                        ImageSpaceEffectWaterDisplacement::fDisplaceOffset.pBuffer[i].y - *pLastY;
                    ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.b =
                        ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.pBuffer[i].x;
                    tmp.x = -ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.r;
                    tmp.y = ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.r;
                    tmp.Unitize();
                    ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.g = tmp.x;
                    ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.g = tmp.y;
                    ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.b = *pLastY;
                    pparam_2->SetVertexConstant(
                        0, ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.r, tmp.x,
                        ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.b,
                        ImageSpaceEffectWaterDisplacement::TransMatrixRowOne.a
                    );
                    pparam_2->SetVertexConstant(
                        1, ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.r,
                        ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.g,
                        ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.b,
                        ImageSpaceEffectWaterDisplacement::TransMatrixRowTwo.a
                    );
                    peffect_3->Render(spPartialScreenTriShape.m_pObject, apRenderer, pparam_2, 1);
                    ++ImageSpaceEffectWaterDisplacement::iNumRipplesThisFrame;
                }
                if (ImageSpaceEffectWaterDisplacement::fDisplaceOffset.pBuffer) {
                    ImageSpaceEffectWaterDisplacement::fDisplaceOffset._Deallocate(
                        ImageSpaceEffectWaterDisplacement::fDisplaceOffset.pBuffer
                    );
                    ImageSpaceEffectWaterDisplacement::fDisplaceOffset.pBuffer = nullptr;
                    ImageSpaceEffectWaterDisplacement::fDisplaceOffset.iReservedSize = 0;
                    ImageSpaceEffectWaterDisplacement::fDisplaceOffset.iSize = 0;
                }
                if (ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.pBuffer) {
                    ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset._Deallocate(
                        ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.pBuffer
                    );
                    ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.pBuffer = nullptr;
                    ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.iReservedSize = 0;
                    ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.iSize = 0;
                }
            }
        }
    }
    ImageSpaceEffectWaterDisplacement::bRanSimulationThisFrame = false;
    apThis = *pTextures.m_pBase[1];
    aTex = apTex;
    apTex = apThis;
    int32_t iSimulations = 0;
    while (fTimeBetweenSimulations < this->fTime - fLastTime) {
        fLastTime = fLastTime + fTimeBetweenSimulations;
        if (ImageSpaceEffectWaterDisplacement::bWadingWater) {
            ImageSpaceShader* pShader4 = static_cast<ImageSpaceShader*>(pEffectList.m_pBase[4].m_pObject);
            ++iSimulations;
            ImageSpaceEffect* peffect_4 = pShader4;
            ImageSpaceShaderParam* pparam_3 = pEffectParamList.m_pBase[4].m_pObject;
            pparam_3->SetPixelConstant(0, this->fTime, 0.0f, 0.0f, 0.0f);
            pparam_3->SetPixelConstant(1, ImageSpaceEffectWaterDisplacement::fBlendAmount, 0.0f, 0.0f, 0.0f);
            pparam_3->SetPixelConstant(
                2, ImageSpaceEffectWaterDisplacement::fTextureOffset.x,
                ImageSpaceEffectWaterDisplacement::fTextureOffset.y, 0.0f, 0.0f
            );
            pparam_3->SetPixelConstant(
                4, ImageSpaceEffectWaterDisplacement::RainVariables.r,
                ImageSpaceEffectWaterDisplacement::RainVariables.g, ImageSpaceEffectWaterDisplacement::RainVariables.b,
                ImageSpaceEffectWaterDisplacement::RainVariables.a
            );
            pparam_3->SetPixelConstant(
                5, ImageSpaceEffectWaterDisplacement::WadingVariables.r,
                ImageSpaceEffectWaterDisplacement::WadingVariables.g,
                ImageSpaceEffectWaterDisplacement::WadingVariables.b,
                ImageSpaceEffectWaterDisplacement::WadingVariables.a
            );
            peffect_4->SetTexture(0, &apTex, NiTexturingProperty::FILTER_NEAREST);
            peffect_4->SetTexture(1, &aTex, NiTexturingProperty::FILTER_BILERP);
            peffect_4->Render(apGeometry, apRenderer, pparam_3, 1);
            ImageSpaceEffectWaterDisplacement::bRanSimulationThisFrame = true;
            apThis = aTex;
            aTex = apTex;
            apTex = apThis;
        }
    }
    if (!(iSimulations % 2)) {
        NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
        ImageSpaceManager* pManager = BSShaderManager::pImageSpaceManager;
        BSRenderedTexture* pDestination = apTex.GetRenderedTexture();
        BSRenderedTexture* pSource = aTex.GetRenderedTexture();
        pManager->RenderEffect(ImageSpaceManager::EFFECT_TOP_LEVEL_END, pRenderer, pSource, pDestination, nullptr, 1);
    }
    if (kGreyTex.GetTexture()) {
        kGreyTex.ReturnRenderedTexture();
    }
    apThis.~ImageSpaceTexture();
    apTex.~ImageSpaceTexture();
    kGreyTex.~ImageSpaceTexture();
    aTex.~ImageSpaceTexture();
}

// Nine passes, each with a fresh ImageSpaceShaderParam (two vertex and six pixel constant registers) and the manager's
// effect 105 + i. Passes 2 to 5 skip SetBlurClearMode.
void ImageSpaceEffectWaterDisplacement::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(9);
    pTextures.SetSize(2);
    for (uint32_t i = 0; i < 9; ++i) {
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
        ImageSpaceShaderParam* pShaderParam = nullptr;
        if (pMemory) {
            pShaderParam = ::new (pMemory) ImageSpaceShaderParam;
        }
        pShaderParam->InitConstantMap(2, 6);
        pShaderParam->bUseTexOffset[1] = false;
        if (i < 2 || i > 5) {
            pShaderParam->SetBlurClearMode();
        }
        pEffectParamList.SetAt(i, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pShaderParam));
        SetEffect(i, apManager->pEffectList.m_pBase[105 + i], pShaderParam, 0);
    }
}

bool ImageSpaceEffectWaterDisplacement::IsActive() {
    return true;
}

ImageSpaceEffectWaterDisplacement* ImageSpaceEffectWaterDisplacement::GetInstance() {
    return ImageSpaceEffectWaterDisplacement::pInstance;
}
