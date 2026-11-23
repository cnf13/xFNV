#include "BSShader/BSShaderAccumulator.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSOcclusionQuery.h"
#include "BSShader/BSBatchRenderer.h"
#include "BSShader/BSInstanceRendering.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/DebugShader.h"
#include "BSShader/ImageSpaceEffectRefraction.h"
#include "BSShader/ImageSpaceEffectVolumetricFog.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderVolumetricFog.h"
#include "BSShader/ImageSpaceTexture.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/ReferenceVolume.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/VolumetricFogShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBackToFrontAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

#include <limits>

NiRTTI BSShaderAccumulator::ms_RTTI("BSShaderAccumulator", &NiAlphaAccumulator::ms_RTTI);
NiPointer<NiNode> BSShaderAccumulator::spRenderPassesScenegraph;

using NiTMapBase_NiTPointerAllocator_uint_NiAVObjectPtr_bool = NiTMapBase<
    NiTPointerAllocator<uint32_t>,
    NiAVObject*,
    bool>;

// Fifty empty occlusion query slots, idle sun tests, the instance renderer, one batch renderer (two when
// auiBatchRendererCount is 2) and the per-renderer pass state arrays; grey silhouettes.
BSShaderAccumulator::BSShaderAccumulator(
    int32_t aeBatchRendererCreationType,
    uint32_t auiBatchRendererCount,
    uint32_t auiPassesPerBatchRenderer
)
  : bDoDepthPasses(false), uiSunPixelCount(0), bWaitingForSunQueury(false), fPercentSunOccludedStored(1.0f),
    bAccumulate(true), b1stPerson(false), bRenderNoShader(true), usQueryCount(0), usQuerySize(50),
    ppOcclusionQuery(
        static_cast<BSOcclusionQuery**>(MemoryManager::Instance().Allocate(50 * sizeof(BSOcclusionQuery*)))
    ),
    uiLastOcclusionRef(0), bLastOcclusionResult(false), bRenderDecals(true),
    uiBatchRendererCount(auiBatchRendererCount), uiBatchRendererPassCount(auiPassesPerBatchRenderer),
    eRenderMode(BSShaderManager::BSSM_RENDER_NORMAL), kAlphaRenderPass(nullptr, 757, 1, 0, 0), kRoomSharedMap(37) {
    for (SunOcclusionTest& rTest : pSunTests) {
        rTest.fPercentOccluded = 0.0f;
        rTest.pOcclusionQuery = nullptr;
        rTest.bWaiting = false;
        rTest.uiPixelCount = 0;
        rTest.uiFrameCount = 0;
    }
    kSilhouetteColor.r = 0.3f;
    kSilhouetteColor.g = 0.3f;
    kSilhouetteColor.b = 0.3f;
    kSilhouetteColor.a = 1.0f;
    for (int32_t i = 0; i < 50; ++i) {
        ppOcclusionQuery[i] = nullptr;
    }
    pInstanceRenderer = new BSInstanceRendering();
    pBatchRenderer[0] = new BSBatchRenderer(uiBatchRendererPassCount, aeBatchRendererCreationType);
    pBatchRenderer[1] = uiBatchRendererCount == 2
        ? new BSBatchRenderer(uiBatchRendererPassCount, aeBatchRendererCreationType)
        : nullptr;
    puicurrentPassA = static_cast<uint32_t*>(
        MemoryManager::Instance().Allocate(sizeof(uint32_t) * uiBatchRendererCount)
    );
    picurrentBucketA = static_cast<int32_t*>(
        MemoryManager::Instance().Allocate(sizeof(int32_t) * uiBatchRendererCount)
    );
    pbcurrentFirstPassA = static_cast<bool*>(MemoryManager::Instance().Allocate(uiBatchRendererCount));
    pbcurrentActiveA = static_cast<bool*>(MemoryManager::Instance().Allocate(uiBatchRendererCount));
    iBatchRendererIndex = 0;
}

// The geometry caches keep their nodes for reuse and empty; the occlusion queries (built on the engine's heap by
// the accumulator's own code), the reference volumes, the instance and batch renderers and the pass state arrays are
// freed; the lists, the map and the pointers release the rest.
BSShaderAccumulator::~BSShaderAccumulator() {
    for (int32_t i = 0; i < 4; ++i) {
        for (PersistentPassList* pCache : {&pRenderedGeometryCache[i], &pRendered1stPersonGeometryCache[i]}) {
            pCache->CleanUnusedNodes();
            pCache->pNextFreeItem = pCache->m_pkHead;
            pCache->m_pkHead = nullptr;
            pCache->m_pkTail = nullptr;
            pCache->Allocator.uiCount = 0;
        }
    }
    MemoryManager::Instance().Deallocate(ppOcclusionQuery);
    while (ReferenceVolumeList.m_uiCount) {
        ReferenceVolume* pVolume = ReferenceVolumeList.RemoveHead();
        if (pVolume) {
            if (BSOcclusionQuery* pQuery = pVolume->pOcclusionQuery) {
                delete pQuery;
                pVolume->pOcclusionQuery = nullptr;
            }
            MemoryManager::Instance().Deallocate(pVolume);
        }
    }
    spBoundTestGeometry = nullptr;
    for (SunOcclusionTest& rTest : pSunTests) {
        if (BSOcclusionQuery* pQuery = rTest.pOcclusionQuery) {
            delete pQuery;
            rTest.pOcclusionQuery = nullptr;
        }
    }
    if (pInstanceRenderer) {
        delete pInstanceRenderer;
        pInstanceRenderer = nullptr;
    }
    delete pBatchRenderer[0];
    delete pBatchRenderer[1];
    MemoryManager::Instance().Deallocate(puicurrentPassA);
    MemoryManager::Instance().Deallocate(picurrentBucketA);
    MemoryManager::Instance().Deallocate(pbcurrentFirstPassA);
    MemoryManager::Instance().Deallocate(pbcurrentActiveA);
    spShadowLight = nullptr;
    kRoomSharedMap.RemoveAll();
    spWaterFogRenderTarget = nullptr;
}

void BSShaderAccumulator::FinishAccumulating() {
    FinishAccumulatingPreResolveDepth();
    FinishAccumulatingPostResolveDepth();
}

bool BSShaderAccumulator::RegisterObject(NiGeometry* apGeometry) {
    BSDismemberSkinInstance* m_pObject = static_cast<BSDismemberSkinInstance*>(apGeometry->m_spSkinInstance.m_pObject);
    if (m_pObject &&
        (static_cast<BSDismemberSkinInstance*>(apGeometry->m_spSkinInstance.m_pObject)->GetRTTI() ==
             &BSDismemberSkinInstance::ms_RTTI &&
         !m_pObject->bVisiblePartitions)) {
        return true;
    }
    BSShaderProperty* pshp = static_cast<BSShaderProperty*>(apGeometry->m_PropertyState.m_aspProps[3].m_pObject);
    if (!apGeometry->GetShader() || !pshp) {
        return true;
    }
    BSShaderAccumulatorRegisterObjectFunc v5 = BSShaderAccumulator::pRegisterObjectFunc[eRenderMode];
    BSShader* m_spShader = apGeometry->GetShader();
    NiPropertyState* p_m_PropertyState = &apGeometry->m_PropertyState;
    if (v5) {
        return v5(this, apGeometry, p_m_PropertyState, pshp, m_spShader);
    }

    return BSShaderAccumulator::pRegisterObjectFunc[0](this, apGeometry, p_m_PropertyState, pshp, m_spShader);
}

bool BSShaderAccumulator::AddShared(NiAVObject* apObject) {
    NiCriticalSection* p_BSCPSharedCrit = &BSCPSharedCrit;
    EnterCriticalSection(&BSCPSharedCrit.m_kCriticalSection);
    bool bfound;
    bfound = kRoomSharedMap.GetAt(apObject, bfound);
    if (!bfound) {
        kRoomSharedMap.SetAt(apObject, true);
    }
    LeaveCriticalSection(&p_BSCPSharedCrit->m_kCriticalSection);
    return !bfound;
}

void BSShaderAccumulator::ClearSharedMap() {
    NiCriticalSection* p_BSCPSharedCrit = &BSCPSharedCrit;
    EnterCriticalSection(&BSCPSharedCrit.m_kCriticalSection);
    kRoomSharedMap.RemoveAll();
    LeaveCriticalSection(&p_BSCPSharedCrit->m_kCriticalSection);
}

void BSShaderAccumulator::FinishAccumulatingPreResolveDepth() {
    BSShaderManager::SetRenderMode(static_cast<uint32_t>(this->eRenderMode));
    BSShaderManager::etRenderMode eMode = this->eRenderMode;
    if (eMode) {
        BSShaderAccumulatorFinishAccumulatingFunc pFinish = BSShaderAccumulator::pFinishAccumulatingFunc[eMode];
        if (pFinish) {
            pFinish(this);
        }
        else {
            BSShaderAccumulator::pFinishAccumulatingFunc[0](this);
        }
    }
    else {
        BSShaderAccumulator::FinishAccumulating_Standard_PreResolveDepth(this);
    }
}

// The standard pass only in normal render mode; then the renderer's current property state is cleared before the
// base finishes.
void BSShaderAccumulator::FinishAccumulatingPostResolveDepth() {
    if (eRenderMode == BSShaderManager::BSSM_RENDER_NORMAL) {
        FinishAccumulating_Standard_PostResolveDepth(this);
    }
    NiRenderer::ms_pkRenderer->m_pkCurrProp = nullptr;
    NiAccumulator::FinishAccumulating();
}

void BSShaderAccumulator::DoSunOcclusionQuery(NiTriShape* apSunGeometry, uint32_t aiTestNumber, NiCamera* apCamera) {
    if (!bWaitingForSunQueury && aiTestNumber < 3) {
        SunOcclusionTest* pTest = &pSunTests[aiTestNumber];
        if (!pTest->bWaiting) {
            bool bHasQuery = pTest->pOcclusionQuery != nullptr;
            if (!bHasQuery) {
                BSOcclusionQuery* pQuery = new BSOcclusionQuery;
                pTest->pOcclusionQuery = pQuery;
                bHasQuery = pQuery != nullptr;
            }
            if (bHasQuery) {
                BSShader* Shader = BSShaderManager::GetShader(1);
                ShadowLightShader::EnableConstantsForPass(9u, false);
                Shader->m_spVertexConstantMap->SortByEnabled();
                Shader->m_spPixelConstantMap->SortByEnabled();
                pTest->pOcclusionQuery->Begin();
                NiRenderer::ms_pkRenderer->SetCameraData(apCamera);
                BSShaderProperty::RenderPass renderPass(apSunGeometry, 9, 1, 0, 0);
                BSBatchRenderer::RenderPassImmediately(&renderPass, 9u, 0, 0, 0);
                pTest->pOcclusionQuery->End();
                pTest->bWaiting = true;
                pTest->uiFrameCount = 0;
            }
            else {
                pSunTests[aiTestNumber].fPercentOccluded = 0.0f;
                pTest->bWaiting = false;
                pTest->uiFrameCount = 0;
            }
        }
    }
}

bool BSShaderAccumulator::RenderVolumetricFog(NiCamera* apCamera, BSRenderedTexture* apCurRenderTarget) {
    uint32_t uiFogCount = pVolumetricFogGeometry.m_uiCount;
    if (uiFogCount) {
        if (NiRenderer::ms_pkRenderer->m_eFrameState) {
            BSRenderedTexture::StopOffscreen();
        }
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
        NiColorA kOrgColor(0.0f, 0.0f, 0.0f, 0.0f);
        ImageSpaceShaderVolumetricFog* pFogShader = static_cast<ImageSpaceShaderVolumetricFog*>(
            BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_VOLUMETRIC_FOG]
        );
        BSRenderedTexture* pvolumetricFogBuffer = pFogShader->GetTexture();
        BSShaderProperty::RenderPass renderpass(nullptr, 0, 1, 0, 0);
        bool busesinglepass = BSShaderManager::b30VFog;
        NiCamera* pOldCamera = this->m_pkCamera;
        this->m_pkCamera = apCamera;
        pRenderer->GetBackgroundColor(kOrgColor);
        NiColorA kClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        pRenderer->SetBackgroundColorAlpha(kClearColor);
        m_kItemsToSortAndRender = &pVolumetricFogGeometry;
        this->Sort();
        renderpass.usPassEnum = BSShaderManager::BSSM_VFOG_3;
        NiTListItem<NiGeometry*>* m_pkHead = pVolumetricFogGeometry.m_pkHead;
        if (m_pkHead) {
            while (true) {
                NiTListItem<NiGeometry*>* pNextItem = m_pkHead->m_pkNext;
                NiGeometry* pkItem = m_pkHead->m_element;
                NiShadeProperty* pShadeProp = static_cast<NiShadeProperty*>(
                    pkItem->m_PropertyState.m_aspProps[3].m_pObject
                );
                if (pShadeProp &&
                    pShadeProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_VOLUMETRIC_FOG) {
                    VolumetricFogShaderProperty* pFogProp = static_cast<VolumetricFogShaderProperty*>(pShadeProp);
                    NiPoint3 worldUp[2];
                    NiPoint3 worldDir;
                    NiRenderTargetGroup* Group = pvolumetricFogBuffer->GetGroup();
                    BSRenderedTexture::StartOffscreen(3u, Group);
                    NiCamera* m_pkCamera = this->m_pkCamera;
                    NiRect<float>* p_m_kPort = &m_pkCamera->m_kPort;
                    worldUp[0].x = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
                    NiFrustum* p_m_kViewFrustum = &m_pkCamera->m_kViewFrustum;
                    NiPoint3* p_m_Translate = &m_pkCamera->m_kWorld.m_Translate;
                    worldUp[0].y = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
                    worldUp[0].z = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
                    worldUp[1].x = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
                    worldUp[1].y = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
                    worldUp[1].z = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
                    worldDir.x = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
                    worldDir.y = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
                    double v22 = m_pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
                    NiRenderer::EFrameState m_eFrameState = pRenderer->m_eFrameState;
                    worldDir.z = v22;
                    if ((m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
                         m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
                        pRenderer->m_bRenderTargetGroupActive) {
                        pRenderer->Do_SetCameraData(
                            *p_m_Translate,
                            worldDir,
                            worldUp[1],
                            *worldUp,
                            *p_m_kViewFrustum,
                            *p_m_kPort
                        );
                    }
                    BSShaderManager::pCurrentRenderPass = &renderpass;
                    if (busesinglepass) {
                        BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(
                            renderpass.usPassEnum
                        );
                    }
                    else {
                        renderpass.usPassEnum = BSShaderManager::BSSM_VFOG_2;
                        BSShaderManager::eCurrentPass = BSShaderManager::BSSM_VFOG_2;
                        pkItem->RenderImmediate(NiRenderer::ms_pkRenderer);
                        BSShaderManager::pCurrentRenderPass = &renderpass;
                        renderpass.usPassEnum = BSShaderManager::BSSM_VFOG_1;
                        BSShaderManager::eCurrentPass = BSShaderManager::BSSM_VFOG_1;
                    }
                    pkItem->RenderImmediate(NiRenderer::ms_pkRenderer);
                    BSRenderedTexture::StopOffscreen();
                    NiRenderTargetGroup* v26 = apCurRenderTarget->GetGroup();
                    BSRenderedTexture::StartOffscreen(0, v26);
                    float v27 = pFogProp->FogProps.r;
                    float v28 = pFogProp->FogProps.g;
                    float v29 = pFogProp->FogProps.b;
                    ImageSpaceEffectVolumetricFog* pFogEffect = static_cast<ImageSpaceEffectVolumetricFog*>(
                        BSShaderManager::pImageSpaceManager->pEffectList
                            .m_pBase[ImageSpaceManager::EFFECT_VOLUMETRIC_FOG]
                    );
                    float v31 = pFogProp->FogProps.a;
                    pFogEffect->FogColor.r = v27;
                    pFogEffect->FogColor.g = v28;
                    pFogEffect->FogColor.b = v29;
                    pFogEffect->bParamsChanged = true;
                    pFogEffect->FogColor.a = v31;
                    BSShaderManager::pImageSpaceManager->RenderEffect(
                        ImageSpaceManager::EFFECT_VOLUMETRIC_FOG,
                        pRenderer,
                        apCurRenderTarget,
                        apCurRenderTarget,
                        nullptr,
                        1
                    );
                }
                if (!pNextItem) {
                    break;
                }
                m_pkHead = pNextItem;
            }
        }
        pVolumetricFogGeometry.RemoveAll();
        pRenderer->SetBackgroundColorAlpha(kOrgColor);
        this->m_pkCamera = pOldCamera;
        ImageSpaceShaderVolumetricFog* pFogShaderEnd = static_cast<ImageSpaceShaderVolumetricFog*>(
            BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_VOLUMETRIC_FOG]
        );
        pFogShaderEnd->ReturnVolumetricFogTexture();
    }
    return uiFogCount != 0;
}

void BSShaderAccumulator::ResetSunOcclusion() {
    double v1 = 0.0;
    float fpercentSunOccluded_2 = 0.0f;
    uint32_t uipixelCounter = 0x7FFFFFFF;
    char v6 = 1;
    for (int32_t i = 0; i < 3; ++i) {
        SunOcclusionTest& sunTest = pSunTests[i];
        if (sunTest.bWaiting) {
            EvaluateSunOcclusionResults(static_cast<uint32_t>(i));
            v1 = 0.0;
            if (sunTest.bWaiting) {
                ++sunTest.uiFrameCount;
                bWaitingForSunQueury = true;
                v6 = 0;
            }
        }
        if (v1 < sunTest.fPercentOccluded || sunTest.uiPixelCount) {
            if (sunTest.uiPixelCount < uipixelCounter) {
                uipixelCounter = sunTest.uiPixelCount;
            }
            float fpercentSunOccluded = sunTest.fPercentOccluded;
            if (fpercentSunOccluded_2 < static_cast<double>(fpercentSunOccluded)) {
                fpercentSunOccluded_2 = fpercentSunOccluded;
            }
        }
    }
    if (v6) {
        bWaitingForSunQueury = false;
        fPercentSunOccludedStored = fpercentSunOccluded_2;
        if (uipixelCounter < 10) {
            fPercentSunOccludedStored = 1.0f;
        }
        uiSunPixelCount = uipixelCounter;
        pSunTests[0].fPercentOccluded = v1;
        pSunTests[1].fPercentOccluded = v1;
        pSunTests[2].fPercentOccluded = v1;
        pSunTests[0].uiPixelCount = 0;
        pSunTests[1].uiPixelCount = 0;
        pSunTests[2].uiPixelCount = 0;
    }
}

void BSShaderAccumulator::RenderScreenSplatter() {
    BSShaderManager::eBSSMPassEnum v6;
    BSShaderManager::eBSSMPassEnum v7;
    uint32_t v2 = 0;
    if (uiBatchRendererCount) {
        v6 = BSShaderManager::BSSM_BLOOD_SPLATTER_FLARE;
        v7 = BSShaderManager::BSSM_BLOOD_SPLATTER;
        BSBatchRenderer** pBatchRenderers = this->pBatchRenderer;
        do {
            (*pBatchRenderers)->kActivePassIndexList.AddTail(static_cast<uint16_t>(v6));
            (*pBatchRenderers)->kActivePassIndexList.AddTail(static_cast<uint16_t>(v7));
            ++v2;
            ++pBatchRenderers;
        }
        while (v2 < uiBatchRendererCount);
    }
    RenderBatches(BSShaderManager::BSSM_BLOOD_SPLATTER_FLARE, BSShaderManager::BSSM_BLOOD_SPLATTER, 0, -1);
    uint32_t v4 = 0;
    if (uiBatchRendererCount) {
        BSBatchRenderer** v5 = this->pBatchRenderer;
        do {
            (*v5)->kActivePassIndexList.RemoveAll();
            ++v4;
            ++v5;
        }
        while (v4 < uiBatchRendererCount);
    }
}

// Renders the passes aeStart to aeEnd of every batch renderer (aeGeometryGroup -1), or of each renderer's geometry
// group aeGeometryGroup, in pass order: each step renders the next pass of the renderer whose current pass, then bucket
// (signed), is lowest, preferring a first pass on a tie. Shadow passes are skipped while rendering a water reflection
// or refraction.
void BSShaderAccumulator::RenderBatches(
    BSShaderManager::eBSSMPassEnum aePass,
    BSShaderManager::eBSSMPassEnum aePassEnd,
    int32_t abAlphaPass,
    int32_t aeGroup
) {
    BSBatchRenderer* pBatchRendererA[2];
    BSBatchRenderer::GeometryGroup* pGeometryGroupA[2];
    if (BSShaderManager::bUse30Lighting) {
        Lighting30Shader::ResetPerFrameConstantData();
    }
    uint32_t uiCount = uiBatchRendererCount;
    if (aeGroup <= -1) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            pGeometryGroupA[i] = nullptr;
        }
        for (uint32_t i = 0; i < uiCount; ++i) {
            pBatchRendererA[i] = pBatchRenderer[i];
        }
    }
    else {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSBatchRenderer::GeometryGroup* pGroup = pBatchRenderer[i]->pGeometryGroups[aeGroup];
            pGeometryGroupA[i] = pGroup;
            pBatchRendererA[i] = pGroup->pBatchRenderer;
        }
    }
    // The first test uses the count read above, the later ones reload it.
    if (uiCount) {
        uint32_t i = 0;
        do {
            BSBatchRenderer* pRenderer = pBatchRendererA[i];
            puicurrentPassA[i] = 0;
            if (pRenderer) {
                pRenderer->uiCurrentFirstPass = aePass;
                pRenderer->uiCurrentLastPass = aePassEnd;
                pbcurrentActiveA[i] = pRenderer->PrepareNextRenderPass(
                    puicurrentPassA[i],
                    picurrentBucketA[i],
                    pbcurrentFirstPassA[i]
                );
            }
            else {
                pbcurrentActiveA[i] = false;
            }
            ++i;
        }
        while (i < uiBatchRendererCount);
    }
    while (true) {
        bool bFoundActive = pbcurrentActiveA[0];
        uint32_t uiCurrent = 0;
        for (uint32_t i = 1; i < uiBatchRendererCount; ++i) {
            if (!bFoundActive && pbcurrentActiveA[i]) {
                bFoundActive = true;
                uiCurrent = i;
            }
            else if (pbcurrentActiveA[i]) {
                if (puicurrentPassA[i] < puicurrentPassA[uiCurrent]) {
                    uiCurrent = i;
                }
                else if (puicurrentPassA[i] == puicurrentPassA[uiCurrent]) {
                    if (picurrentBucketA[i] < picurrentBucketA[uiCurrent]) {
                        uiCurrent = i;
                    }
                    else if (picurrentBucketA[i] == picurrentBucketA[uiCurrent] && pbcurrentFirstPassA[i]) {
                        uiCurrent = i;
                    }
                }
            }
        }
        if (!bFoundActive) {
            break;
        }
        uint32_t& ruiPass = puicurrentPassA[uiCurrent];
        uint32_t uiPass = ruiPass;
        bool bShadowPass = IsShadowPass(uiPass);
        if (bShadowPass && (bWaterReflection || bWaterRefraction)) {
            pbcurrentActiveA[uiCurrent] = pBatchRendererA[uiCurrent]->SkipNextRenderPass(
                ruiPass,
                picurrentBucketA[uiCurrent],
                pbcurrentFirstPassA[uiCurrent],
                false
            );
        }
        else {
            pbcurrentActiveA[uiCurrent] = pBatchRendererA[uiCurrent]->RenderNextRenderPass(
                ruiPass,
                picurrentBucketA[uiCurrent],
                pbcurrentFirstPassA[uiCurrent],
                abAlphaPass
            );
        }
    }
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        BSBatchRenderer::GeometryGroup* pGroup = pGeometryGroupA[i];
        if (pGroup) {
            pGroup->ClearPasses();
        }
    }
    BSBatchRenderer::EndPass();
}

// Skipped in render mode 8 (BSSM_RENDER_CONST_ALPHA); renders the geometry groups 2 and 3 (GROUP_STATIC_DECAL,
// GROUP_DYNAMIC_DECAL) as alpha passes, and drops each lock by hand before the unlocked restore.
void BSShaderAccumulator::RenderDecals() {
    if (bRenderDecals && eRenderMode != 8) {
        float fDepthBias = BSShaderProperty::fDepthBiasUnit;
        if (!BSShaderManager::bUseDepthBias) {
            fDepthBias = 0.0f;
        }
        BSRenderState::SetZWriteEnable(false, BSRS_LOCK);
        BSRenderState::SetDepthBias(fDepthBias, BSRS_LOCK);
        RenderGeometryGroup(2, true);
        RenderGeometryGroup(3, true);
        --BSRenderState::iLock[BSRS_ZWRITEENABLE];
        BSRenderState::SetZWriteEnable(true, BSRS_NOLOCK);
        --BSRenderState::iLock[BSRS_DEPTHBIAS];
        BSRenderState::SetDepthBias(0.0f, BSRS_NOLOCK);
    }
}

// Renders the group through every batch renderer when the first renderer's group uses its pass list, else through
// RenderBatches over the passes BSSM_ZONLY to BSSM_BLOOD_SPLATTER.
void BSShaderAccumulator::RenderGeometryGroup(uint32_t auiGeometryGroup, int32_t abAlphaPass) {
    BSBatchRenderer::GeometryGroup* pGroup = pBatchRenderer[0]->pGeometryGroups[auiGeometryGroup];
    if (!pGroup) {
        return;
    }
    if (pGroup->ucFlags & BSBatchRenderer::GeometryGroup::FLAG_USE_PASS_LIST) {
        for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
            pBatchRenderer[i]->pGeometryGroups[auiGeometryGroup]->Render(abAlphaPass);
        }
    }
    else {
        RenderBatches(BSShaderManager::BSSM_ZONLY, BSShaderManager::BSSM_BLOOD_SPLATTER, abAlphaPass, auiGeometryGroup);
    }
}

void BSShaderAccumulator::RenderAlphaGeometry(BSShaderAccumulator::WATER_SIDE aeSide) {
    float fDepth[2];
    BSBatchRenderer::EndPass();
    // The batch renderers take the side as an integer aiIndex/aiSide.
    fDepth[0] = pBatchRenderer[0]->GetNextAlphaDepth(aeSide, m_pkCamera, m_bSortByClosestPoint);
    double NextAlphaDepth;
    if (uiBatchRendererCount == 2) {
        NextAlphaDepth = pBatchRenderer[1]->GetNextAlphaDepth(aeSide, m_pkCamera, m_bSortByClosestPoint);
    }
    else {
        NextAlphaDepth = -std::numeric_limits<float>::max();
    }
    fDepth[1] = NextAlphaDepth;
    // Renders from the batch renderer with the deeper next group until both are empty.
    while (fDepth[0] > -3.4028234663852886e+38 || fDepth[1] > -std::numeric_limits<float>::max()) {
        int32_t icurrentBR = !(fDepth[0] > fDepth[1]);
        pBatchRenderer[icurrentBR]->RenderNextAlpha(aeSide, m_pkCamera, m_bSortByClosestPoint, fDepth[-icurrentBR + 1]);
        fDepth[icurrentBR] = pBatchRenderer[icurrentBR]->GetNextAlphaDepth(aeSide, m_pkCamera, m_bSortByClosestPoint);
    }
    pBatchRenderer[0]->ClearAlphaLists(aeSide);
    if (uiBatchRendererCount == 2) {
        pBatchRenderer[1]->ClearAlphaLists(aeSide);
    }
    BSBatchRenderer::EndPass();
}

void BSShaderAccumulator::SetShadowSceneNode(ShadowSceneNode* apShadowSceneNode) {
    pActiveShadowSceneNode = apShadowSceneNode;
}

void BSShaderAccumulator::SetRenderMode(BSShaderManager::etRenderMode aeRenderMode) {
    eRenderMode = aeRenderMode;
}

void BSShaderAccumulator::SetUnderWater(bool abUnderWater) {
    bUnderWater = abUnderWater;
}

void BSShaderAccumulator::ClearRenderPasses() {
    m_kItems.RemoveAll();
}

void BSShaderAccumulator::ClearBloodSplatter() {
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        pBatchRenderer[i]->ClearRenderPasses(false);
    }
}

void BSShaderAccumulator::ClearActivePasses() {
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        pBatchRenderer[i]->kActivePassIndexList.RemoveAll();
    }
}

void BSShaderAccumulator::SortAlphaPasses() {
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        pBatchRenderer[i]->SortAlphaPasses(m_pkCamera, m_bSortByClosestPoint);
    }
}

bool BSShaderAccumulator::QWaterRefractionPassWithinRange(
    BSShaderManager::eBSSMPassEnum aeStartRange,
    BSShaderManager::eBSSMPassEnum aeEndRange
) {
    bool bFound = false;
    for (uint32_t i = 0; i < uiBatchRendererCount && !bFound; ++i) {
        bFound = pBatchRenderer[i]->QWaterRefractionPassWithinRange(aeStartRange, aeEndRange);
    }
    return bFound;
}

uint32_t BSShaderAccumulator::GetRefractivePassCount() {
    // Geometry group 6 is the refraction group. The count is tested signed before the pairs, so a count of 0x80000000
    // or more reads only the first renderer.
    const uint32_t uiCount = uiBatchRendererCount;
    uint32_t uiPassCount = 0;
    uint32_t i = 0;
    if (static_cast<int32_t>(uiCount) >= 2) {
        for (; i + 2 <= uiCount; i += 2) {
            uiPassCount += pBatchRenderer[i]->pGeometryGroups[6]->usCount;
            uiPassCount += pBatchRenderer[i + 1]->pGeometryGroups[6]->usCount;
        }
    }
    if (i < uiCount) {
        uiPassCount += pBatchRenderer[i]->pGeometryGroups[6]->usCount;
    }
    return uiPassCount;
}

void BSShaderAccumulator::StartGroupingAlphas(const NiBound& aBound, bool abZPrePass) {
    pBatchRenderer[iBatchRendererIndex % uiBatchRendererCount]
        ->StartGroupingAlphas(aBound, abZPrePass, m_pkCamera, m_bSortByClosestPoint);
}

// The batch renderer's StopGroupingAlphas does the work.
void BSShaderAccumulator::StopGroupingAlphas() {
    pBatchRenderer[iBatchRendererIndex % uiBatchRendererCount]->StopGroupingAlphas();
}

void BSShaderAccumulator::ClearAllBoundVolumes() {
    for (; ReferenceVolumeList.m_uiCount; --ReferenceVolumeList.m_uiCount) {
        ReferenceVolume* m_element = ReferenceVolumeList.m_pkHead->m_element;
        if (m_element) {
            BSOcclusionQuery* pOcclusionQuery = m_element->pOcclusionQuery;
            if (pOcclusionQuery) {
                delete pOcclusionQuery;
                m_element->pOcclusionQuery = nullptr;
            }
            MemoryManager::Instance().Deallocate(m_element);
        }
        NiTListItem<ReferenceVolume*>* pNode = ReferenceVolumeList.m_pkHead;
        ReferenceVolumeList.m_pkHead = pNode->m_pkNext;
        if (ReferenceVolumeList.m_pkHead) {
            ReferenceVolumeList.m_pkHead->m_pkPrev = nullptr;
        }
        else {
            ReferenceVolumeList.m_pkTail = nullptr;
        }
        ReferenceVolumeList.DeleteItem(pNode);
    }
}

void BSShaderAccumulator::RemoveBoundVolume(uint32_t auiReferenceID) {
    NiTListItem<ReferenceVolume*>* m_pkHead = ReferenceVolumeList.m_pkHead;
    NiTPointerList<ReferenceVolume*>* p_ReferenceVolumeList = &ReferenceVolumeList;
    if (m_pkHead) {
        ReferenceVolume* pvolume;
        ReferenceVolume* v7;
        while (true) {
            pvolume = m_pkHead->m_element;
            m_pkHead = m_pkHead->m_pkNext;
            v7 = pvolume;
            if (pvolume && (pvolume->uiReferenceID == auiReferenceID)) {
                break;
            }

            if (!m_pkHead) {
                return;
            }
        }
        if (pvolume->pOcclusionQuery) {
            delete pvolume->pOcclusionQuery;
            pvolume->pOcclusionQuery = nullptr;
        }
        MemoryManager::Instance().Deallocate(pvolume);
        p_ReferenceVolumeList->Remove(v7);
    }
}

bool BSShaderAccumulator::IsBoundVolumeOccluded(uint32_t auiReferenceID, bool abRunTest) {
    ReferenceVolume* pvolume;
    uint32_t v3 = auiReferenceID;
    if (auiReferenceID == uiLastOcclusionRef) {
        return bLastOcclusionResult;
    }
    NiTListItem<ReferenceVolume*>* m_pkHead = ReferenceVolumeList.m_pkHead;
    if (!m_pkHead) {
        return false;
    }
    while (true) {
        pvolume = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (pvolume && (pvolume->uiReferenceID == auiReferenceID)) {
            break;
        }

        if (!m_pkHead) {
            return false;
        }
    }
    if (pvolume->bWaiting && abRunTest) {
        if (BSShaderAccumulator::bRunBoundingSphereOcclusion && pvolume->pOcclusionQuery) {
            bool v8;
            uint32_t ddata;
            do {
                v8 = pvolume->pOcclusionQuery->GetResult(&ddata, true);
            }
            while (v8);
            if (!v8) {
                pvolume->bOccluded = (ddata == 0);
                pvolume->uiPixelCount = ddata;
            }
            else {
                delete pvolume->pOcclusionQuery;
                v3 = auiReferenceID;
                pvolume->pOcclusionQuery = nullptr;
                pvolume->uiPixelCount = 0xF423F;
                pvolume->bOccluded = false;
            }
        }
        else if (BSShaderAccumulator::bResetBoundingSphereOcclusion) {
            pvolume->uiPixelCount = 0xF423F;
            pvolume->bOccluded = false;
        }
        pvolume->bWaiting = false;
    }
    if (!pvolume->bWaiting) {
        uiLastOcclusionRef = v3;
        bLastOcclusionResult = pvolume->bOccluded;
    }
    return pvolume->bOccluded;
}

// The camera setup is done twice, with the axes taken from the world rotation's columns 0 to 2. The
// player pass deliberately keeps m_pkCamera (the world camera) for the location, axes and viewport and takes only the
// frustum from ap1stPersonCamera. Geometry groups 11 and 12 are GROUP_DEPTH_WORLD and GROUP_DEPTH_PLAYER.
void BSShaderAccumulator::RenderDepth(NiCamera* apWorldCamera, NiCamera* ap1stPersonCamera) {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    NiColorA kOrgColor;
    kOrgColor.r = 0.0f;
    kOrgColor.g = 0.0f;
    kOrgColor.b = 0.0f;
    kOrgColor.a = 0.0f;
    pRenderer->GetBackgroundColor(kOrgColor);
    NiColorA kWhite;
    kWhite.r = 1.0f;
    kWhite.g = 1.0f;
    kWhite.b = 1.0f;
    kWhite.a = 1.0f;
    pRenderer->SetBackgroundColor(kWhite);
    BSRenderedTexture* pDepthTexture = BSShaderManager::GetSceneDepthRenderedTexture();
    if (NiRenderer::ms_pkRenderer->m_eFrameState != NiRenderer::FRAMESTATE_OUTSIDE_FRAME) {
        BSRenderedTexture::StopOffscreen();
    }
    BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, pDepthTexture->BSRenderedTexture::GetGroup());

    m_pkCamera = apWorldCamera;
    pRenderer->SetCameraData(apWorldCamera);
    RenderGeometryGroup(11, false);

    if (ap1stPersonCamera) {
        NiCamera* pCamera = m_pkCamera;
        const NiMatrix3& kCamRotate = pCamera->m_kWorld.m_Rotate;
        NiPoint3 kCamRight;
        kCamRight.x = kCamRotate.m_pEntry[0][2];
        kCamRight.y = kCamRotate.m_pEntry[1][2];
        kCamRight.z = kCamRotate.m_pEntry[2][2];
        NiPoint3 kCamUp;
        kCamUp.x = kCamRotate.m_pEntry[0][1];
        kCamUp.y = kCamRotate.m_pEntry[1][1];
        kCamUp.z = kCamRotate.m_pEntry[2][1];
        NiPoint3 kCamDir;
        kCamDir.x = kCamRotate.m_pEntry[0][0];
        kCamDir.y = kCamRotate.m_pEntry[1][0];
        kCamDir.z = kCamRotate.m_pEntry[2][0];
        pRenderer->SetCameraData(
            pCamera->m_kWorld.m_Translate,
            kCamDir,
            kCamUp,
            kCamRight,
            ap1stPersonCamera->m_kViewFrustum,
            pCamera->m_kPort
        );
        RenderGeometryGroup(12, false);
    }
    BSRenderedTexture::StopOffscreen();
    pRenderer->SetBackgroundColor(kOrgColor);
}

// Renders the refraction normal groups into the refraction effect's texture: the world group first (when the effect has
// a texture and the world group has passes), then the refraction group (when it has passes and the first pass did not
// start an offscreen frame). Each pass clears the colour write mask, attaches the scene's depth buffer to the
// refraction target, halves the background alpha and, inside a frame with an active target group, sets the camera data
// from the accumulator's camera. With bShowRefractionDebugTexture set the refraction texture goes to the debug shader.
// Returns false on every path.
bool BSShaderAccumulator::RenderNormals(NiCamera* apCamera, BSRenderedTexture* apCurRenderTarget) {
    if (!apCurRenderTarget) {
        return false;
    }
    NiCamera* pSavedCamera = m_pkCamera;
    m_pkCamera = apCamera;
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    NiColorA kSavedBackground;
    kSavedBackground.r = 0.0f;
    kSavedBackground.g = 0.0f;
    kSavedBackground.b = 0.0f;
    kSavedBackground.a = 0.0f;
    bool bSetBackgroundAlpha = false;
    bool bStartedOffscreen = false;
    ImageSpaceEffectRefraction* pEffect = static_cast<ImageSpaceEffectRefraction*>(
        BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[2]
    );
    BSRenderedTexture* pRefractionTexture = pEffect->GetRefractionNormalsTexture(false);
    BSRenderState::SetColorWriteEnable(0xF, BSRS_NOLOCK);
    uint32_t uiWorldPasses = 0;
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        uiWorldPasses += pBatchRenderer[i]->pGeometryGroups[8]->usCount;
    }
    if (pRefractionTexture && static_cast<int32_t>(uiWorldPasses) > 0) {
        NiRenderTargetGroup* pTargetGroup = apCurRenderTarget->GetGroup();
        pRefractionTexture->GetGroup()->AttachDepthStencilBuffer(pTargetGroup->GetDepthStencilBuffer());
        pRenderer->GetBackgroundColor(kSavedBackground);
        NiColorA kHalfAlpha;
        kHalfAlpha.r = 0.5f;
        kHalfAlpha.g = 0.5f;
        kHalfAlpha.b = 0.0f;
        kHalfAlpha.a = 0.0f;
        pRenderer->SetBackgroundColor(kHalfAlpha);
        bSetBackgroundAlpha = true;
        if (pRenderer->m_eFrameState) {
            BSRenderedTexture::StopOffscreen();
        }
        BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_NONE, pRefractionTexture->GetGroup());
        NiCamera* pCamera = m_pkCamera;
        NiRenderer::EFrameState eFrameState = pRenderer->m_eFrameState;
        if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
             eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
            pRenderer->m_bRenderTargetGroupActive) {
            // Column 0 of the camera's world rotation.
            NiPoint3 kWorldDir;
            kWorldDir.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
            kWorldDir.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
            kWorldDir.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
            NiPoint3 kWorldUp;
            kWorldUp.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
            kWorldUp.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
            kWorldUp.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
            NiPoint3 kWorldRight;
            kWorldRight.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
            kWorldRight.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
            kWorldRight.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
            pRenderer->Do_SetCameraData(
                pCamera->m_kWorld.m_Translate,
                kWorldDir,
                kWorldUp,
                kWorldRight,
                pCamera->m_kViewFrustum,
                pCamera->m_kPort
            );
        }
        RenderGeometryGroup(8, false);
        bStartedOffscreen = true;
    }
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        pBatchRenderer[i]->pGeometryGroups[8]->ClearPasses();
    }
    uint32_t uiRefractionPasses = 0;
    for (uint32_t i = 0; i < uiBatchRendererCount; ++i) {
        uiRefractionPasses += pBatchRenderer[i]->pGeometryGroups[6]->usCount;
    }
    if (uiRefractionPasses) {
        bool bCreatedTexture = false;
        if (!bStartedOffscreen) {
            if (!pRefractionTexture) {
                pRefractionTexture = pEffect->GetRefractionNormalsTexture(true);
                bCreatedTexture = true;
            }
            NiRenderTargetGroup* pTargetGroup = apCurRenderTarget->GetGroup();
            pRefractionTexture->GetGroup()->AttachDepthStencilBuffer(pTargetGroup->GetDepthStencilBuffer());
            pRenderer->GetBackgroundColor(kSavedBackground);
            NiColorA kHalfAlpha;
            kHalfAlpha.r = 0.5f;
            kHalfAlpha.g = 0.5f;
            kHalfAlpha.b = 0.0f;
            kHalfAlpha.a = 0.0f;
            pRenderer->SetBackgroundColor(kHalfAlpha);
            bSetBackgroundAlpha = true;
            BSRenderedTexture::StopOffscreen();
            BSRenderedTexture::StartOffscreen(bCreatedTexture, pRefractionTexture->GetGroup());
            NiCamera* pCamera = m_pkCamera;
            NiRenderer::EFrameState eFrameState = pRenderer->m_eFrameState;
            if ((eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
                 eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
                pRenderer->m_bRenderTargetGroupActive) {
                NiPoint3 kWorldDir;
                kWorldDir.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
                kWorldDir.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
                kWorldDir.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
                NiPoint3 kWorldUp;
                kWorldUp.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
                kWorldUp.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
                kWorldUp.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
                NiPoint3 kWorldRight;
                kWorldRight.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
                kWorldRight.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
                kWorldRight.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
                pRenderer->Do_SetCameraData(
                    pCamera->m_kWorld.m_Translate,
                    kWorldDir,
                    kWorldUp,
                    kWorldRight,
                    pCamera->m_kViewFrustum,
                    pCamera->m_kPort
                );
            }
        }
        RenderGeometryGroup(6, false);
        if (bShowRefractionDebugTexture) {
            NiRenderedTexture* pDebugTexture = pRefractionTexture ? pRefractionTexture->spTexture[0].m_pObject
                                                                  : nullptr;
            static_cast<DebugShader*>(BSShaderManager::GetShader(9))->SetTextureToRender(pDebugTexture);
        }
    }
    if (bStartedOffscreen || uiRefractionPasses) {
        BSRenderedTexture::StopOffscreen();
    }
    if (pRefractionTexture) {
        NiRenderTargetGroup* pTargetGroup = apCurRenderTarget->GetGroup();
        pRefractionTexture->GetGroup()->AttachDepthStencilBuffer(pTargetGroup->GetDepthStencilBuffer());
    }
    if (bSetBackgroundAlpha) {
        pRenderer->SetBackgroundColor(kSavedBackground);
    }
    m_pkCamera = pSavedCamera;
    return false;
}

void BSShaderAccumulator::SetupDepth(
    NiCamera* apWorldCamera,
    NiCamera* ap1stPersonCamera,
    BSRenderedTexture* apCurRenderTarget
) {
    RenderDepth(apWorldCamera, ap1stPersonCamera);
}

void BSShaderAccumulator::SetCurrentWaterHeight(int32_t aiHeight) {
    iCurrentWaterHeight = aiHeight;
}

NiGeometry* BSShaderAccumulator::GetBoundTestGeometry() {
    return NiSmartPointerCast<NiGeometry>(spBoundTestGeometry);
}

void BSShaderAccumulator::SetWaterReflection(bool abWaterReflection) {
    bWaterReflection = abWaterReflection;
}

void BSShaderAccumulator::SetSilhouetteColor(NiColorA& aColor) {
    kSilhouetteColor = aColor;
}

void BSShaderAccumulator::SetWaterRefraction(bool abWaterRefraction) {
    bWaterRefraction = abWaterRefraction;
}

void BSShaderAccumulator::RenderTiles() {
    BSBatchRenderer::EndPass();
    int32_t m_iNumItems = this->m_iNumItems;
    this->m_iCurrItem = m_iNumItems;
    if (m_iNumItems) {
        NiGeometry** v7;
        int32_t v8;
        NiGeometry** m_ppkItems = this->m_ppkItems;
        int32_t v4 = m_iNumItems - 1;
        this->m_iCurrItem = v4;
        for (NiGeometry* i = m_ppkItems[v4]; i; i = v7[v8]) {
            uint32_t usPassEnum = kAlphaRenderPass.usPassEnum;
            kAlphaRenderPass.pGeometry = i;
            BSBatchRenderer::RenderPassImmediately(&kAlphaRenderPass, usPassEnum, 1, 1, 1);
            int32_t m_iCurrItem = this->m_iCurrItem;
            if (!m_iCurrItem) {
                break;
            }
            v7 = this->m_ppkItems;
            v8 = m_iCurrItem - 1;
            this->m_iCurrItem = v8;
        }
    }
    NiTPointerList<NiGeometry*>* m_kItemsToSortAndRender = this->m_kItemsToSortAndRender;
    if (m_kItemsToSortAndRender) {
        if (m_kItemsToSortAndRender->m_uiCount) {
            do {
                this->m_kItemsToSortAndRender->RemoveHead();
            }
            while (this->m_kItemsToSortAndRender->m_uiCount);
        }
    }
    else {
        for (; m_kItems.m_uiCount; --m_kItems.m_uiCount) {
            NiTListItem<NiGeometry*>* m_pkHead = m_kItems.m_pkHead;
            NiTListItem<NiGeometry*>* v15 = m_pkHead->m_pkNext;
            m_kItems.m_pkHead = m_pkHead->m_pkNext;
            if (v15) {
                v15->m_pkPrev = nullptr;
            }
            else {
                m_kItems.m_pkTail = nullptr;
            }
            m_kItems.DeleteItem(m_pkHead);
        }
    }
    BSBatchRenderer::EndPass();
}

void BSShaderAccumulator::ClearGroupPasses(int32_t aeGroup) {
    BSBatchRenderer** pBatchRenderers;
    uint32_t v3 = 0;
    if (uiBatchRendererCount) {
        pBatchRenderers = this->pBatchRenderer;
        do {
            (*pBatchRenderers)->pGeometryGroups[aeGroup]->ClearPasses();
            ++v3;
            ++pBatchRenderers;
        }
        while (v3 < uiBatchRendererCount);
    }
}

void BSShaderAccumulator::SetShadowLight(ShadowSceneLight* apLight) {
    ShadowSceneLight* m_pObject;
    m_pObject = spShadowLight.m_pObject;
    if (m_pObject != apLight) {
        if (m_pObject && (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount))) {
            m_pObject->DeleteThis();
        }

        spShadowLight.m_pObject = apLight;
        if (apLight) {
            InterlockedIncrementUInt32(&apLight->m_uiRefCount);
        }
    }
}

void BSShaderAccumulator::GetDebugStats(
    uint32_t& alGeom,
    uint32_t& alOccludedGeom,
    uint32_t& alTri,
    uint32_t& alPass,
    uint32_t& alPassTri,
    uint32_t& alMemory,
    uint32_t& alOcclusionGeom,
    uint32_t& alOcclusionTri,
    uint32_t& alWaitForQueryLoops,
    uint32_t& alWaitForBoundVolumeQueryLoops,
    uint32_t& alWaitForSunQueryLoops
) {
    alGeom = BSShaderAccumulator::kDebugData[0];
    alOccludedGeom = BSShaderAccumulator::kDebugData[1];
    alTri = BSShaderAccumulator::kDebugData[4];
    alPass = BSShaderAccumulator::kDebugData[5];
    alPassTri = BSShaderAccumulator::kDebugData[6];
    alMemory = BSShaderAccumulator::kDebugData[7];
    alOcclusionGeom = BSShaderAccumulator::kDebugData[2];
    alOcclusionTri = BSShaderAccumulator::kDebugData[3];
    alWaitForQueryLoops = BSShaderAccumulator::kDebugData[8];
    alWaitForBoundVolumeQueryLoops = BSShaderAccumulator::kDebugData[10];
    alWaitForSunQueryLoops = BSShaderAccumulator::kDebugData[9];
}

void BSShaderAccumulator::ClearAllQueries() {
    uint32_t ddata;
    int32_t i = 0;
    if (usQuerySize) {
        do {
            BSOcclusionQuery** v3 = &this->ppOcclusionQuery[i];
            if (*v3) {
                (*v3)->GetResult(&ddata, true);
                delete this->ppOcclusionQuery[i];
                this->ppOcclusionQuery[i] = nullptr;
            }
            ++i;
        }
        while (i < usQuerySize);
    }
    SunOcclusionTest* pSunTests = this->pSunTests;
    int32_t v7 = 3;
    do {
        if (pSunTests->pOcclusionQuery) {
            pSunTests->pOcclusionQuery->GetResult(&ddata, true);
            delete pSunTests->pOcclusionQuery;
            pSunTests->pOcclusionQuery = nullptr;
            pSunTests->bWaiting = false;
        }
        ++pSunTests;
        --v7;
    }
    while (v7);
    bool v9 = ReferenceVolumeList.m_uiCount == 0;
    bWaitingForSunQueury = false;
    NiTListItem<ReferenceVolume*>* m_pkHead = ReferenceVolumeList.m_pkHead;
    if (!v9) {
        do {
            ReferenceVolume* pvolume = m_pkHead->m_element;
            m_pkHead = m_pkHead->m_pkNext;
            if (pvolume) {
                BSOcclusionQuery* v12 = pvolume->pOcclusionQuery;
                if (v12) {
                    v12->GetResult(&ddata, true);
                    delete pvolume->pOcclusionQuery;
                    pvolume->pOcclusionQuery = nullptr;
                }
            }
        }
        while (ReferenceVolumeList.m_uiCount);
    }
    BSOcclusionQuery** v14 = this->ppOcclusionQuery;
    usQuerySize = 0;
    usQueryCount = 0;
    MemoryManager::Instance().Deallocate(v14);
    this->ppOcclusionQuery = nullptr;
}

void BSShaderAccumulator::AddAndUpdateBoundVolume(uint32_t auiReferenceID, const NiBound& aBound, bool abRunTest) {
    NiTListItem<ReferenceVolume*>* m_pkHead = ReferenceVolumeList.m_pkHead;
    while (m_pkHead) {
        ReferenceVolume* pvolume = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (pvolume && (pvolume->uiReferenceID == auiReferenceID)) {
            pvolume->Center = aBound.m_kCenter;
            pvolume->fRadius = aBound.m_fRadius;
            pvolume->bPerformTest = abRunTest;
            return;
        }
    }
    ReferenceVolume* v10;
    ReferenceVolume* v7 = static_cast<ReferenceVolume*>(MemoryManager::Instance().Allocate(sizeof(ReferenceVolume)));
    if (v7) {
        double m_fRadius = aBound.m_fRadius;
        v7->Center.x = aBound.m_kCenter.x;
        float abEnable = m_fRadius;
        v7->Center.y = aBound.m_kCenter.y;
        float z = aBound.m_kCenter.z;
        v7->fRadius = abEnable;
        v7->Center.z = z;
        v7->uiReferenceID = auiReferenceID;
        v7->pOcclusionQuery = nullptr;
        v7->bOccluded = false;
        v7->bWaiting = false;
        v7->bPerformTest = true;
        v7->uiPixelCount = 0;
        v10 = v7;
    }
    else {
        v10 = nullptr;
    }
    uint32_t* p_m_uiCount = &ReferenceVolumeList.m_uiCount;
    NiTListItem<ReferenceVolume*>* v12 = ReferenceVolumeList.NewItem();
    v12->m_element = v10;
    v12->m_pkPrev = nullptr;
    v12->m_pkNext = ReferenceVolumeList.m_pkHead;
    NiTListItem<ReferenceVolume*>* v13 = ReferenceVolumeList.m_pkHead;
    if (v13) {
        v13->m_pkPrev = v12;
        ++*p_m_uiCount;
    }
    else {
        ++*p_m_uiCount;
        ReferenceVolumeList.m_pkTail = v12;
    }
    ReferenceVolumeList.m_pkHead = v12;
}

// Builds the empty "RenderPasses" child the accumulated geometry hangs under and updates it.
NiNode* BSShaderAccumulator::CreateRenderPassTree() {
    NiNode* pNode = new NiNode(0);

    // The interned handle is kept in a NiFixedString local and its reference is dropped the way the destructor does it:
    // the handle is checked against NULL_STRING, then the count 8 bytes before the characters is dropped. That count is
    // treated as untyped raw memory (NiGlobalStringTable::DecRefCount spells it the same way).
    NiFixedString kName;
    kName.SetHandle(NiGlobalStringTable::AddString("RenderPasses"));
    pNode->SetName(kName);
    NiGlobalStringTable::GlobalStringHandle kHandle = kName.GetHandle();
    if (kHandle != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<uint32_t*>(const_cast<char*>(kHandle) - 8));
    }

    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    pNode->Update(kUpdateData);
    return pNode;
}

// GetResultNoWait returns a bool, but its byte is tested against 1 and then against 0 again: the second branch, which
// drops the query, is only reached by a result other than 0 and 1.
void BSShaderAccumulator::EvaluateSunOcclusionResults(uint32_t aiTestNumber) {
    if (aiTestNumber < 3) {
        SunOcclusionTest& kTest = pSunTests[aiTestNumber];
        if (kTest.bWaiting) {
            BSOcclusionQuery* pQuery = kTest.pOcclusionQuery;
            if (pQuery) {
                // The pixel count is read into aiTestNumber, overwriting it; kTest keeps the index.
                uint32_t& uiPixelCount = aiTestNumber;
                bool bResult = pQuery->GetResultNoWait(&uiPixelCount, true);
                if (bResult) {
                    if (uiPixelCount) {
                        uint32_t uiClamped = 1500;
                        if (uiPixelCount <= 1500) {
                            uiClamped = uiPixelCount;
                        }
                        kTest.uiPixelCount = uiPixelCount;
                        kTest.bWaiting = false;
                        float fPercentOccluded = 1.0 - static_cast<double>(uiClamped) / 1500.0;
                        kTest.fPercentOccluded = fPercentOccluded;
                    }
                    else {
                        kTest.uiPixelCount = 0;
                        kTest.bWaiting = false;
                        kTest.fPercentOccluded = 1.0f;
                    }
                }
                else if (bResult) {
                    delete kTest.pOcclusionQuery;
                    kTest.pOcclusionQuery = nullptr;
                    kTest.bWaiting = false;
                }
            }
        }
    }
}

void BSShaderAccumulator::ClearSunQueries() {
    uint32_t uiResult;
    SunOcclusionTest* pSunTests = this->pSunTests;
    int32_t v2 = 3;
    do {
        BSOcclusionQuery* pOcclusionQuery = pSunTests->pOcclusionQuery;
        uiResult = 0;
        if (pOcclusionQuery) {
            pOcclusionQuery->GetResult(&uiResult, true);
            delete pSunTests->pOcclusionQuery;
            pSunTests->pOcclusionQuery = nullptr;
            pSunTests->bWaiting = false;
        }
        ++pSunTests;
        --v2;
    }
    while (v2);
    bWaitingForSunQueury = false;
}

void BSShaderAccumulator::ClearRenderedGeometryCache() {
    for (int32_t i = 0; i < 4; ++i) {
        pRenderedGeometryCache[i].CleanUnusedNodes();
        pRenderedGeometryCache[i].pNextFreeItem = pRenderedGeometryCache[i].m_pkHead;
        pRenderedGeometryCache[i].m_pkHead = nullptr;
        pRenderedGeometryCache[i].m_pkTail = nullptr;
        pRenderedGeometryCache[i].Allocator.uiCount = 0;
        pRendered1stPersonGeometryCache[i].CleanUnusedNodes();
        pRendered1stPersonGeometryCache[i].pNextFreeItem = pRendered1stPersonGeometryCache[i].m_pkHead;
        pRendered1stPersonGeometryCache[i].m_pkHead = nullptr;
        pRendered1stPersonGeometryCache[i].m_pkTail = nullptr;
        pRendered1stPersonGeometryCache[i].Allocator.uiCount = 0;
    }
}

void BSShaderAccumulator::FinishAccumulating_Standard_PostResolveDepth(BSShaderAccumulator* apAccumulator) {
    if (apAccumulator->bWorldGeometry &&
        apAccumulator->QWaterRefractionPassWithinRange(
            BSShaderManager::BSSM_WATER_STENCIL,
            BSShaderManager::BSSM_WATER_SPECULAR_LIGHTING_Vc
        )) {
        if (BSShaderManager::pCurrentRenderTarget) {
            BSRenderedTexture::StopOffscreen();
            BSRenderedTexture* v1 = BSShaderManager::pWaterRefractionTexture;
            if (!BSShaderManager::pWaterRefractionTexture) {
                v1 = BSShaderManager::pTextureManager->BorrowRenderedTexture(
                    BSShaderManager::pRenderer.m_pObject,
                    BSTextureManager::TEX_MAIN_FIRSTPERSON,
                    0,
                    nullptr,
                    0
                );
                BSShaderManager::pWaterRefractionTexture = v1;
            }
            BSShaderManager::pImageSpaceManager->RenderEffect(
                ImageSpaceManager::EFFECT_SHADER_COPY,
                BSShaderManager::pRenderer.m_pObject,
                BSShaderManager::pCurrentRenderTarget,
                v1,
                nullptr,
                1
            );
            NiRenderTargetGroup* Group = BSShaderManager::pCurrentRenderTarget->GetGroup();
            BSRenderedTexture::StartOffscreen(0, Group);
        }
        BSShaderManager::pRenderer->SetCameraData(apAccumulator->m_pkCamera);
    }
    apAccumulator->RenderBatches(BSShaderManager::BSSM_WATER_STENCIL, BSShaderManager::BSSM_WATER_STENCIL_Vc, 0, -1);
    apAccumulator->RenderBatches(
        BSShaderManager::BSSM_WATER_WADING,
        BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING_Vc,
        0,
        -1
    );
    apAccumulator->RenderBatches(BSShaderManager::BSSM_WATER, BSShaderManager::BSSM_WATER_SPECULAR_LIGHTING_Vc, 0, -1);
    if (apAccumulator->bWorldGeometry) {
        BSShaderManager::pTextureManager->ReturnRenderedTexture(BSShaderManager::pWaterRefractionTexture);
        BSShaderManager::pWaterRefractionTexture = nullptr;
    }
    apAccumulator->RenderGeometryGroup(9u, 1);
    apAccumulator->RenderAlphaGeometry(static_cast<BSShaderAccumulator::WATER_SIDE>(apAccumulator->bUnderWater));
    BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
    BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
    apAccumulator
        ->RenderBatches(BSShaderManager::BSSM_PRECIPITATION_RAIN, BSShaderManager::BSSM_SELFILLUMALPHA_S, 0, -1);
    apAccumulator->RenderGeometryGroup(1u, 0);
}

void BSShaderAccumulator::FinishAccumulating_Standard_PreResolveDepth(BSShaderAccumulator* apAccumulator) {
    if (apAccumulator->m_pkCamera) {
        uint32_t eRenderMode = apAccumulator->eRenderMode;
        if (eRenderMode == 8) {
            apAccumulator->RenderBatches(BSShaderManager::BSSM_CONSTALPHA, BSShaderManager::BSSM_CONSTALPHA_S, 0, -1);
            apAccumulator
                ->RenderBatches(BSShaderManager::BSSM_3XCONSTALPHA, BSShaderManager::BSSM_3XCONSTALPHA_SVc, 0, -1);
            apAccumulator->ClearActivePasses();
        }
        else {
            if (eRenderMode == 7) {
                apAccumulator->RenderBatches(BSShaderManager::BSSM_DEPTHMAP, BSShaderManager::BSSM_DEPTHMAP_SAt, 0, -1);
                apAccumulator
                    ->RenderBatches(BSShaderManager::BSSM_3XDEPTHMAP, BSShaderManager::BSSM_3XDEPTHMAP_VcS, 0, -1);
                apAccumulator->ClearActivePasses();
            }
            else {
                apAccumulator->RenderGeometryGroup(10u, 0);
                if (BSShaderAccumulator::bShowRenderPassesScenegraph && apAccumulator->eRenderMode != 7) {
                    BSShaderAccumulator::spRenderPassesScenegraph = apAccumulator->CreateRenderPassTree();
                    BSShaderAccumulator::bShowRenderPassesScenegraph = false;
                }
                if (apAccumulator->bWaterReflection) {
                    apAccumulator->RenderBatches(
                        BSShaderManager::BSSM_ZONLY,
                        BSShaderManager::BSSM_DISTANT_TREE_SILHOUETTE,
                        0,
                        -1
                    );
                    BSRenderState::SetUserClipPlaneEnables(false, false, false, false, false, false, BSRS_NOLOCK);
                    apAccumulator->RenderBatches(BSShaderManager::BSSM_SKYBASEPRE, BSShaderManager::BSSM_VFOG_3, 0, -1);
                    BSRenderState::SetUserClipPlaneEnables(true, false, false, false, false, false, BSRS_NOLOCK);
                }
                else {
                    apAccumulator->RenderBatches(BSShaderManager::BSSM_ZONLY, BSShaderManager::BSSM_VFOG_3, 0, -1);
                }
                int32_t isaveaniso = NiRenderer::iMaxAnisotropy;
                NiRenderer::iMaxAnisotropy = 1;
                apAccumulator->RenderGeometryGroup(0, 0);
                NiRenderer::iMaxAnisotropy = isaveaniso;
                apAccumulator->RenderDecals();
                apAccumulator->RenderBatches(
                    BSShaderManager::BSSM_GRASS_DIRONLY_LF,
                    BSShaderManager::BSSM_GRASS_SHADOW_LSB,
                    0,
                    -1
                );
                apAccumulator->SortAlphaPasses();
                if (apAccumulator->bUnderWater) {
                    apAccumulator->RenderAlphaGeometry(BSShaderAccumulator::ABOVE_WATER);
                }
                else {
                    apAccumulator->RenderAlphaGeometry(BSShaderAccumulator::BELOW_WATER);
                }
            }
        }
    }
}

void BSShaderAccumulator::SetRunBoundingSphereTests(bool abEnable, bool abReset) {
    bRunBoundingSphereOcclusion = abEnable;
    bResetBoundingSphereOcclusion = abReset;
}

void BSShaderAccumulator::SetDisplayBoundingSpheres(bool abShow) {
    bShowBoundingSpheres = abShow;
}

bool BSShaderAccumulator::RegisterObject_Standard(
    BSShaderAccumulator* apAccumulator,
    NiGeometry* apGeometry,
    const NiPropertyState* apState,
    BSShaderProperty* apShaderProp,
    BSShader* apShader
) {
    if (!apAccumulator->bWaterReflection || apShader->iShaderType != 13) {
        if (apAccumulator->bDoDepthPasses) {
            BSShaderProperty::RenderPass* pDepthPass = apShaderProp->GetRenderDepthPass(apGeometry);
            if (pDepthPass) {
                if (apAccumulator->b1stPerson) {
                    apAccumulator->pBatchRenderer[iBatchRendererIndex]
                        ->RegisterPassGeometryGroup(pDepthPass, BSBatchRenderer::GG_RENDER_DEPTH_1STPERSON, 0);
                }
                else {
                    apAccumulator->pBatchRenderer[iBatchRendererIndex]
                        ->RegisterPassGeometryGroup(pDepthPass, BSBatchRenderer::GG_RENDER_DEPTH, 0);
                }
            }
        }
        BSBatchRenderer*
            pBatchRenderer = apAccumulator->pBatchRenderer[iBatchRendererIndex % apAccumulator->uiBatchRendererCount];
        int32_t iGroupingAlphasGroupCount = pBatchRenderer->iGroupingAlphas;
        bool bZPrePass = iGroupingAlphasGroupCount > 0 && pBatchRenderer->bAlphaGroupZPrePass;
        if (bZPrePass != ((apShaderProp->ulFlags[0] & BSS1_Z_Prepass) != 0)) {
            if (iGroupingAlphasGroupCount > 0 && pBatchRenderer->bAlphaGroupZPrePass) {
                if ((apShaderProp->ulFlags[0] & BSS1_Z_Prepass) == 0) {
                    apShaderProp->iLastRenderPassState = 0;
                }
                apShaderProp->ulFlags[0] |= BSS1_Z_Prepass;
            }
            else {
                if ((apShaderProp->ulFlags[0] & BSS1_Z_Prepass) != 0) {
                    apShaderProp->iLastRenderPassState = 0;
                }
                apShaderProp->ulFlags[0] &= ~BSS1_Z_Prepass;
            }
            BSShaderProperty::RenderPassArray* pRenderPassArray = apShaderProp->pRenderPassList;
            if (pRenderPassArray) {
                pRenderPassArray->iSize = 0;
            }
        }
        uint32_t eRenderMode = apAccumulator->eRenderMode;
        uint16_t usPassCount = 0;
        BSShaderProperty::RenderPassArray* pPasses = apShaderProp->GetRenderPasses(
            apGeometry,
            BSShaderManager::GetEnabledPasses(),
            usPassCount,
            eRenderMode,
            apAccumulator,
            1
        );
        if (apAccumulator->b1stPerson) {
            if ((apShaderProp->ulFlags[1] & BSS2_1st_person) == 0) {
                apShaderProp->iLastRenderPassState = 0;
            }
            apShaderProp->ulFlags[1] |= BSS2_1st_person;
        }
        else {
            if ((apShaderProp->ulFlags[1] & BSS2_1st_person) != 0) {
                apShaderProp->iLastRenderPassState = 0;
            }
            apShaderProp->ulFlags[1] &= ~BSS2_1st_person;
        }
        if (pPasses && pPasses->iSize) {
            for (uint32_t i = 0; i < pPasses->iSize; ++i) {
                BSShaderProperty::RenderPass* pPass = pPasses->pPassA.m_pBase[i];
                uint16_t usPassEnum = pPass->usPassEnum;
                if (!usPassEnum) {
                    continue;
                }
                switch (pPass->eAccumulationHint) {
                    case BSShaderProperty::BSSP_HINT_SORT: {
                        if (apAccumulator->bCellHasWater) {
                            NiBound* pBound = pPass->pGeometry->m_kWorldBound;
                            if (!pBound) {
                                pBound = &NiAVObject::NullBoundS;
                            }
                            if (static_cast<double>(apAccumulator->iCurrentWaterHeight) > pBound->m_kCenter.z) {
                                apAccumulator->pBatchRenderer[iBatchRendererIndex]->RegisterAlphaPass(pPass, 1u);
                                break;
                            }
                        }
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]->RegisterAlphaPass(pPass, 0);
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_IMMEDIATE: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]
                            ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_IMMEDIATE, 0);
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_NODEPTH: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]
                            ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_NO_DEPTH, 0);
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_DECAL: {
                        if (apAccumulator->bRenderDecals) {
                            apAccumulator->pBatchRenderer[iBatchRendererIndex]
                                ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_STATIC_DECAL, 1);
                        }
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_DYNAMIC_DECAL: {
                        if (apAccumulator->bRenderDecals) {
                            apAccumulator->pBatchRenderer[iBatchRendererIndex]
                                ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_DYNAMIC_DECAL, 0);
                        }
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_REFRACT: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]
                            ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_REFRACTIVE, 0);
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_1STPERSONNOREFRACT: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]
                            ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_1ST_PERSON_NONREFRACTIVE, 0);
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_VOLUMETRICFOG:
                    case BSShaderProperty::BSSP_HINT_GROUP_MORPH:
                    case BSShaderProperty::BSSP_HINT_GROUP_LODTREE:
                    case BSShaderProperty::BSSP_HINT_GROUP_LODBILLBOARD:
                    case BSShaderProperty::BSSP_HINT_GROUP_VELOCITY: {
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_LOD: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]
                            ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_LOD, 0);
                        break;
                    }
                    case BSShaderProperty::BSSP_HINT_GROUP_FADING: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]
                            ->RegisterPassGeometryGroup(pPass, BSBatchRenderer::GG_FADING, 0);
                        break;
                    }
                    default: {
                        apAccumulator->pBatchRenderer[iBatchRendererIndex]->RegisterPass(pPass, usPassEnum);
                        break;
                    }
                }
            }
            if (apAccumulator->b1stPerson && apAccumulator->bAccumulate) {
                BSShaderProperty::RenderPassArray* pSIBlockPasses = apShaderProp->GetSIBlockRenderPasses();
                if (pSIBlockPasses) {
                    for (uint32_t i = 0; i < pSIBlockPasses->iSize; ++i) {
                        BSShaderProperty::RenderPass* pPass = pSIBlockPasses->pPassA.m_pBase[i];
                        if (apAccumulator->bAccumulate) {
                            apAccumulator->pBatchRenderer[iBatchRendererIndex]->RegisterPass(pPass, 0);
                        }
                        else {
                            BSShader* pShader = BSShaderManager::GetShader(1);
                            ShadowLightShader::EnableConstantsForPass(pPass->usPassEnum, false);
                            if (pShader) {
                                pShader->m_spVertexConstantMap->SortByEnabled();
                                pShader->m_spPixelConstantMap->SortByEnabled();
                            }
                            BSShaderAccumulator::RenderPassImmediately(pPass, pPass->usPassEnum);
                        }
                    }
                }
            }
        }
    }
    return true;
}

// Returns true without registering geometry whose bound radius is under 50, unlit geometry, and geometry not flagged to
// show in the local map.
bool BSShaderAccumulator::RegisterObject_LocalMap(
    BSShaderAccumulator* apAccumulator,
    NiGeometry* apGeometry,
    const NiPropertyState* apState,
    BSShaderProperty* apShaderProp,
    BSShader* apShader
) {
    if (apGeometry->m_spModelData->m_kBound.m_fRadius < 50.0) {
        return true;
    }
    if (apShader->iShaderType == BSShaderManager::BSSM_SHADERTYPE_NOLIGHTING) {
        return true;
    }
    if (!(apShaderProp->ulFlags[1] & (1u << (BSShaderProperty::BSSP_FLAG_SHOW_IN_LOCAL_MAP - 32)))) {
        return true;
    }
    return BSShaderAccumulator::RegisterObject_Standard(apAccumulator, apGeometry, apState, apShaderProp, apShader);
}

bool BSShaderAccumulator::RegisterObject_ConstAlpha(
    BSShaderAccumulator* apAccumulator,
    NiGeometry* apGeometry,
    const NiPropertyState* apState,
    BSShaderProperty* apShaderProp,
    BSShader* apShader
) {
    if (apShader->iShaderType == BSShaderManager::BSSM_SHADERTYPE_NOLIGHTING) {
        return true;
    }
    return BSShaderAccumulator::RegisterObject_Standard(apAccumulator, apGeometry, apState, apShaderProp, apShader);
}

// The pass count GetRenderPasses fills is not read; each pass with a non-zero pass enum goes to the batch renderer
// the thread's batch renderer index selects.
bool BSShaderAccumulator::RegisterObject_ScreenSplatter(
    BSShaderAccumulator* apAccumulator,
    NiGeometry* apGeometry,
    const NiPropertyState* apState,
    BSShaderProperty* apShaderProp,
    BSShader* apShader
) {
    uint16_t usPassCount = 0;
    BSShaderProperty::RenderPassArray* pPasses = apShaderProp->GetRenderPasses(
        apGeometry,
        BSShaderManager::iEnabledPasses,
        usPassCount,
        apAccumulator->eRenderMode,
        apAccumulator,
        true
    );
    if (!pPasses) {
        return true;
    }
    for (uint32_t i = 0; i < pPasses->iSize; ++i) {
        BSShaderProperty::RenderPass* pPass = pPasses->pPassA.m_pBase[i];
        uint16_t usPassEnum = pPass->usPassEnum;
        if (usPassEnum) {
            BSBatchRenderer* pBatchRenderer = apAccumulator->pBatchRenderer[iBatchRendererIndex];
            pBatchRenderer->RegisterPass(pPass, usPassEnum);
        }
    }
    return true;
}

bool BSShaderAccumulator::RegisterObject_Frozen(
    BSShaderAccumulator* /*apAccumulator*/,
    NiGeometry* /*apGeometry*/,
    const NiPropertyState* /*apState*/,
    BSShaderProperty* /*apShaderProp*/,
    BSShader* /*apShader*/
) {
    return true;
}

void BSShaderAccumulator::FinishAccumulating_ScreenSplatter(BSShaderAccumulator* /*apAccumulator*/) {}

bool BSShaderAccumulator::IsShadowPass(uint32_t auiPassEnum) {
    // The SIMPLESHADOW variants of the 3-light shaders, of the 2x shaders, and four consecutive grass passes.
    if (auiPassEnum >= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW &&
        auiPassEnum <= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS) {
        return true;
    }
    if (auiPassEnum >= BSShaderManager::BSSM_2x_SIMPLESHADOW &&
        auiPassEnum <= BSShaderManager::BSSM_2x_SIMPLESHADOW_Sb) {
        return true;
    }
    return auiPassEnum - BSShaderManager::BSSM_GRASS_SHADOW_L <= 3;
}

void BSShaderAccumulator::FinishAccumulating_Standard(BSShaderAccumulator* apAccumulator) {
    BSShaderAccumulator::FinishAccumulating_Standard_PreResolveDepth(apAccumulator);
    BSShaderAccumulator::FinishAccumulating_Standard_PostResolveDepth(apAccumulator);
}

void BSShaderAccumulator::FinishAccumulating_WaterDepth(BSShaderAccumulator* apAccumulator) {
    if (apAccumulator->m_pkCamera) {
        BSRenderState::SetAlphaBlendEnable(false, BSRS_LOCK);
        apAccumulator
            ->RenderBatches(BSShaderManager::BSSM_WATER_DEPTH, BSShaderManager::BSSM_NLWATER_DEPTH_AtVc_S, 0, (-1));
        --BSRenderState::iLock[6];
        BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
    }
}

void BSShaderAccumulator::FinishAccumulating_Interface(BSShaderAccumulator* apAccumulator) {
    if (apAccumulator->m_pkCamera) {
        apAccumulator->m_kItemsToSortAndRender = nullptr;
        apAccumulator->Sort();
        apAccumulator->RenderTiles();
        BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
    }
}

void BSShaderAccumulator::RenderPassImmediately(BSShaderProperty::RenderPass* apRenderPass, uint32_t auiPassEnum) {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(auiPassEnum);
    BSShaderManager::pCurrentRenderPass = apRenderPass;
    BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(apRenderPass->pGeometry->GetProperty(3));
    // The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare).
    if (pShaderProp &&
        static_cast<uint32_t>(pShaderProp->iShaderPropertyType - 1) <=
            BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1) {
        float fDarkness = static_cast<BSShaderLightingProperty*>(pShaderProp)->fForcedDarkness;
        if (BSShaderManager::bUse30Lighting) {
            Lighting30Shader::ResetPerFrameConstantData();
        }
        if ((auiPassEnum < BSShaderManager::BSSM_DEPTHMAP || auiPassEnum > BSShaderManager::BSSM_DEPTHMAP_SAt) &&
            auiPassEnum != BSShaderManager::BSSM_3XDEPTHMAP &&
            auiPassEnum != BSShaderManager::BSSM_3XDEPTHMAP_S &&
            auiPassEnum != BSShaderManager::BSSM_3XDEPTHMAP_Vc &&
            auiPassEnum != BSShaderManager::BSSM_3XDEPTHMAP_VcS) {
            BSShaderLightingProperty::ResetLights(0);
        }
        for (uint8_t i = 0; i < apRenderPass->ucNumLights; ++i) {
            BSShaderLightingProperty::SetLight(i, apRenderPass->ppSceneLights[i], fDarkness);
        }
        if (BSShaderManager::bLiteBrite) {
            NiPoint4& kAmbient = BSShaderLightingProperty::pShaderLightConstants[1];
            kAmbient.x = NiColorA::BLACK.r;
            kAmbient.y = NiColorA::BLACK.g;
            kAmbient.z = NiColorA::BLACK.b;
            kAmbient.w = NiColorA::BLACK.a;
        }
    }
    apRenderPass->pGeometry->RenderImmediate(pRenderer);
}

bool BSShaderAccumulator::RegisterObject_WaterDepth(
    BSShaderAccumulator* apAccumulator,
    NiGeometry* apGeometry,
    const NiPropertyState* apState,
    BSShaderProperty* apShaderProp,
    BSShader* apShader
) {
    if (!apAccumulator->bWaterRefraction) {
        return true;
    }
    BSShaderProperty::RenderPassArray* v5 = apShaderProp->GetWaterFogPassList(apGeometry);
    if (!v5) {
        return true;
    }
    uint32_t v7 = 0;
    if (!v5->iSize) {
        return true;
    }
    do {
        BSBatchRenderer* v9 = apAccumulator->pBatchRenderer[iBatchRendererIndex];
        v9->RegisterPass(v5->pPassA.m_pBase[v7], v5->pPassA.m_pBase[v7]->usPassEnum);
        ++v7;
    }
    while (v7 < v5->iSize);
    return true;
}

bool BSShaderAccumulator::RegisterObject_Interface(
    BSShaderAccumulator* apAccumulator,
    NiGeometry* apGeometry,
    const NiPropertyState* apState,
    BSShaderProperty* apShaderProp,
    BSShader* apShader
) {
    if (apShader) {
        uint32_t* p_m_uiCount = &apAccumulator->m_kItems.m_uiCount;
        NiTListItem<NiGeometry*>* v6 = apAccumulator->m_kItems.NewItem();
        v6->m_element = apGeometry;
        v6->m_pkNext = nullptr;
        v6->m_pkPrev = apAccumulator->m_kItems.m_pkTail;
        NiTListItem<NiGeometry*>* m_pkTail = apAccumulator->m_kItems.m_pkTail;
        if (m_pkTail) {
            m_pkTail->m_pkNext = v6;
            ++*p_m_uiCount;
            apAccumulator->m_kItems.m_pkTail = v6;
            return true;
        }
        ++*p_m_uiCount;
        apAccumulator->m_kItems.m_pkHead = v6;
        apAccumulator->m_kItems.m_pkTail = v6;
    }
    return true;
}

void BSShaderAccumulator::FinishAccumulating_FirstPerson(BSShaderAccumulator* apAccumulator) {
    if (apAccumulator->m_pkCamera) {
        if (BSShaderAccumulator::bShowRenderPassesScenegraph) {
            BSShaderAccumulator::spRenderPassesScenegraph = apAccumulator->CreateRenderPassTree();
            BSShaderAccumulator::bShowRenderPassesScenegraph = false;
        }
        apAccumulator->m_kItemsToSortAndRender = nullptr;
        apAccumulator->Sort();
        apAccumulator->RenderBatches(BSShaderManager::BSSM_ZONLY, BSShaderManager::BSSM_GRASS_SHADOW_LSB, 0, -1);
        apAccumulator->RenderDecals();
        apAccumulator->SortAlphaPasses();
        apAccumulator->RenderAlphaGeometry(static_cast<BSShaderAccumulator::WATER_SIDE>(!apAccumulator->bUnderWater));
        apAccumulator->RenderAlphaGeometry(static_cast<BSShaderAccumulator::WATER_SIDE>(apAccumulator->bUnderWater));
        BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
        apAccumulator
            ->RenderBatches(BSShaderManager::BSSM_PRECIPITATION_RAIN, BSShaderManager::BSSM_SELFILLUMALPHA_S, 0, -1);
        apAccumulator->ClearActivePasses();
    }
}

void BSBatchRenderer::ClearAlphaLists(int32_t aiSide) {
    pAlphaGroups[aiSide]->ClearPasses();
    NiTPointerList<GeometryGroup*>& rList = kAlphaGroupLists[aiSide];
    NiTListItem<GeometryGroup*>* pItem = rList.m_pkHead;
    while (pItem) {
        GeometryGroup* pGroup = pItem->m_element;
        pItem = pItem->m_pkNext;
        delete pGroup;
    }
    rList.NiTListBase<GeometryGroup*>::RemoveAll();
    pCurrentAlphaGroup[aiSide] = nullptr;
}

void ImageSpaceShaderVolumetricFog::ReturnVolumetricFogTexture() {
    if (pTextures.m_pBase[2] && pTextures.m_pBase[2]->GetRenderedTexture()) {
        BSTextureManager* pTextureManager = BSShaderManager::pTextureManager;
        BSRenderedTexture* pRenderedTexture = pTextures.m_pBase[2]->GetRenderedTexture();
        pTextureManager->ReturnRenderedTexture(pRenderedTexture);
        pTextures.m_pBase[2]->SetTexture(static_cast<BSRenderedTexture*>(nullptr));
    }
}
