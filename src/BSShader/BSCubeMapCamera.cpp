#include "BSShader/BSCubeMapCamera.h"

#include "BSMain/BSCullingProcess.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <cmath>

NiRTTI BSCubeMapCamera::ms_RTTI("BSCubeMapCamera", &NiNode::ms_RTTI);

// A 90-degree perspective frustum from 1 to 2048 and its own shader accumulator.
BSCubeMapCamera::BSCubeMapCamera(int32_t aeCubeMapType)
  : eCubeMapType(static_cast<etCubeMapType>(aeCubeMapType)), spRenderedTexture{}, spRenderedCubeMap(nullptr),
    spCubeMapScene(nullptr), spAccumulator(nullptr) {
    NiFrustum kFrustum(false);
    // The constant is the float pi/4 widened to double; the tangent is stored as a float.
    float fTan = static_cast<float>(std::tan(0.7853981852531433));
    kFrustum.m_fLeft = -fTan;
    kFrustum.m_fBottom = -fTan;
    kFrustum.m_fRight = fTan;
    kFrustum.m_fTop = fTan;
    kFrustum.m_fNear = 1.0f;
    kFrustum.m_fFar = 2048.0f;
    m_kViewFrustum = kFrustum;
    spAccumulator = new BSShaderAccumulator(99, 1u, 759u);
    NiIncRefCount(spAccumulator);
}

// The six textures, the cube map, the scene and the accumulator are released.
BSCubeMapCamera::~BSCubeMapCamera() {
    for (NiRefObject*& rpTexture : spRenderedTexture) {
        if (rpTexture) {
            NiDecRefCount(rpTexture);
            rpTexture = nullptr;
        }
    }
    if (spRenderedCubeMap) {
        NiDecRefCount(spRenderedCubeMap);
        spRenderedCubeMap = nullptr;
    }
    if (spAccumulator) {
        NiDecRefCount(spAccumulator);
        spAccumulator = nullptr;
    }
    if (spCubeMapScene) {
        NiDecRefCount(spCubeMapScene);
        spCubeMapScene = nullptr;
    }
}

const NiRTTI* BSCubeMapCamera::GetRTTI() const {
    return &BSCubeMapCamera::ms_RTTI;
}

void BSCubeMapCamera::Click(int32_t aiFace) {
    switch (eCubeMapType) {
        case BSCubeMapCamera::BSCMC_CUBEMAP: {
            ClickCubeMap();
            break;
        }
        case BSCubeMapCamera::BSCMC_CUBEMAPMASK: {
            ClickCubeMapWithMask(aiFace);
            break;
        }
        default: {
            break;
        }
    }
    BSRenderedTexture* spRenderedCubeMap = this->spRenderedCubeMap;
    if (spRenderedCubeMap) {
        NiDecRefCount(spRenderedCubeMap);
        this->spRenderedCubeMap = nullptr;
    }
    // Releases the six pointers: decrement the count, then delete through the object's virtual DeleteThis.
    for (NiRefObject*& rpTexture : this->spRenderedTexture) {
        NiRefObject* pObject = rpTexture;
        if (pObject) {
            // The count is the raw 32-bit value the InterlockedDecrement import takes.
            if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&pObject->m_uiRefCount))) {
                pObject->DeleteThis();
            }
            rpTexture = nullptr;
        }
    }
}

void BSCubeMapCamera::SetupTransformation(NiRenderedCubeMap::FaceID aeFace) {
    NiPoint3 v2;
    NiPoint3 kWorldUp;
    NiPoint3 v4;
    NiPoint3 v5;
    NiPoint3 v6;
    NiPoint3 v7;
    NiPoint3 v8;
    NiPoint3 v9;
    NiPoint3 v10;
    NiPoint3 kWorldPt;
    NiPoint3 v12;
    switch (aeFace) {
        case NiRenderedCubeMap::FACE_POS_X: {
            kWorldUp.x = (-NiPoint3::UNIT_Y.x);
            kWorldUp.y = (-NiPoint3::UNIT_Y.y);
            kWorldUp.z = (-NiPoint3::UNIT_Y.z);
            kWorldPt.x = (m_kWorld.m_Translate.x - NiPoint3::UNIT_X.x);
            kWorldPt.y = (m_kWorld.m_Translate.y - NiPoint3::UNIT_X.y);
            kWorldPt.z = (m_kWorld.m_Translate.z - NiPoint3::UNIT_X.z);
            NiCamera::LookAtWorldPoint(kWorldPt, kWorldUp);
            break;
        }
        case NiRenderedCubeMap::FACE_NEG_X: {
            v5.x = (-NiPoint3::UNIT_Y.x);
            v5.y = (-NiPoint3::UNIT_Y.y);
            v5.z = (-NiPoint3::UNIT_Y.z);
            v9.x = (NiPoint3::UNIT_X.x + m_kWorld.m_Translate.x);
            v9.y = (m_kWorld.m_Translate.y + NiPoint3::UNIT_X.y);
            v9.z = (m_kWorld.m_Translate.z + NiPoint3::UNIT_X.z);
            NiCamera::LookAtWorldPoint(v9, v5);
            break;
        }
        case NiRenderedCubeMap::FACE_POS_Y: {
            v7.x = (m_kWorld.m_Translate.x - NiPoint3::UNIT_Y.x);
            v7.y = (m_kWorld.m_Translate.y - NiPoint3::UNIT_Y.y);
            v7.z = (m_kWorld.m_Translate.z - NiPoint3::UNIT_Y.z);
            NiCamera::LookAtWorldPoint(v7, NiPoint3::UNIT_Z);
            break;
        }
        case NiRenderedCubeMap::FACE_NEG_Y: {
            v2.x = (-NiPoint3::UNIT_Z.x);
            v2.y = (-NiPoint3::UNIT_Z.y);
            v2.z = (-NiPoint3::UNIT_Z.z);
            v4.x = (m_kWorld.m_Translate.x + NiPoint3::UNIT_Y.x);
            v4.y = (m_kWorld.m_Translate.y + NiPoint3::UNIT_Y.y);
            v4.z = (m_kWorld.m_Translate.z + NiPoint3::UNIT_Y.z);
            NiCamera::LookAtWorldPoint(v4, v2);
            break;
        }
        case NiRenderedCubeMap::FACE_POS_Z: {
            v6.x = (-NiPoint3::UNIT_Y.x);
            v6.y = (-NiPoint3::UNIT_Y.y);
            v6.z = (-NiPoint3::UNIT_Y.z);
            v8.x = (m_kWorld.m_Translate.x - NiPoint3::UNIT_Z.x);
            v8.y = (m_kWorld.m_Translate.y - NiPoint3::UNIT_Z.y);
            v8.z = (m_kWorld.m_Translate.z - NiPoint3::UNIT_Z.z);
            NiCamera::LookAtWorldPoint(v8, v6);
            break;
        }
        case NiRenderedCubeMap::FACE_NEG_Z: {
            v10.x = (-NiPoint3::UNIT_Y.x);
            v10.y = (-NiPoint3::UNIT_Y.y);
            v10.z = (-NiPoint3::UNIT_Y.z);
            v12.x = (m_kWorld.m_Translate.x + NiPoint3::UNIT_Z.x);
            v12.y = (m_kWorld.m_Translate.y + NiPoint3::UNIT_Z.y);
            v12.z = (m_kWorld.m_Translate.z + NiPoint3::UNIT_Z.z);
            NiCamera::LookAtWorldPoint(v12, v10);
            break;
        }
        default: {
            return;
        }
    }
}

void BSCubeMapCamera::RenderGeomForLight(ShadowSceneLight* apLight) {
    if (!apLight) {
        return;
    }
    NiRenderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
    pRenderer->SetCameraData(this);
    spAccumulator->StartAccumulating(this);
    spAccumulator->bAccumulate = true;
    // Raw storage the culling process is constructed in.
    alignas(NiCullingProcess) uint8_t acCuller[sizeof(NiCullingProcess)];
    NiCullingProcess* pCuller = reinterpret_cast<NiCullingProcess*>(acCuller);
    ::new (pCuller) NiCullingProcess(nullptr);
    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    Update(kUpdateData);
    pCuller->m_pkCamera = this;
    pCuller->SetFrustum(m_kViewFrustum);
    apLight->Display(*pCuller);
    spAccumulator->FinishAccumulating();
    // Takes a reference on the accumulator here when fewer than two are held.
    BSShaderAccumulator* pAccumulator = spAccumulator;
    if (pAccumulator->m_uiRefCount < 2) {
        pAccumulator->IncRefCount();
    }
    pCuller->~NiCullingProcess();
}

void BSCubeMapCamera::ClickCubeMap() {
    NiColorA kBackgroundColor = NiColorA::BLACK;
    uint32_t uiCulledFaces = 0;
    BSShaderManager::pRenderer->GetBackgroundColor(kBackgroundColor);
    // Render mode 7 is the depth map render mode.
    if (BSShaderManager::eRenderMode == 7) {
        uiCulledFaces = pShadowLight->usFrustumCull;
        NiColorA kDepthColor(4096.0f, 4096.0f, 4096.0f, 1.0f);
        BSShaderManager::pRenderer->SetBackgroundColorAlpha(kDepthColor);
    }
    for (int32_t iFace = 0; iFace < 6; ++iFace) {
        SetupTransformation(static_cast<NiRenderedCubeMap::FaceID>(iFace));
        // With no rendered cube map the texture is null, and the face is still set and the group still read through
        // the null pointers.
        NiRenderedCubeMap* pCubeMap = static_cast<NiRenderedCubeMap*>(
            spRenderedCubeMap ? spRenderedCubeMap->spTexture[0].m_pObject : nullptr
        );
        pCubeMap->SetCurrentCubeFace(static_cast<NiRenderedCubeMap::FaceID>(iFace));
        BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, spRenderedCubeMap->GetGroup());
        uint32_t uiFaceBit = 0;
        switch (iFace) {
            case NiRenderedCubeMap::FACE_POS_X: uiFaceBit = 1; break;
            case NiRenderedCubeMap::FACE_NEG_X: uiFaceBit = 2; break;
            case NiRenderedCubeMap::FACE_POS_Y: uiFaceBit = 4; break;
            case NiRenderedCubeMap::FACE_NEG_Y: uiFaceBit = 8; break;
            case NiRenderedCubeMap::FACE_POS_Z: uiFaceBit = 16; break;
            case NiRenderedCubeMap::FACE_NEG_Z: uiFaceBit = 32; break;
            default: break;
        }
        if (static_cast<uint16_t>(uiFaceBit & uiCulledFaces) == 0) {
            NiUpdateData kUpdateData;
            kUpdateData.fTime = 0.0f;
            kUpdateData.bUpdateControllers = false;
            kUpdateData.bParallelUpdate = false;
            kUpdateData.bFoundParticles = false;
            kUpdateData.bFoundMorphController = false;
            kUpdateData.bSceneGraphChange = false;
            Update(kUpdateData);
            if (BSShaderManager::eRenderMode == 7) {
                if (pShadowLight) {
                    BSShaderLightingProperty::SetLight(0, pShadowLight, 1.0f);
                    // spRenderedCubeMap is an NiPointer: the shadow light's dynamic shadow map.
                    BSRenderedTexture* pShadowMap = pShadowLight->spDynamicShadowMap.m_pObject;
                    BSRenderedTexture* pOld = spRenderedCubeMap;
                    if (pOld != pShadowMap) {
                        if (pOld) {
                            pOld->DecRefCount();
                        }
                        spRenderedCubeMap = pShadowMap;
                        if (pShadowMap) {
                            pShadowMap->IncRefCount();
                        }
                    }
                    RenderGeomForLight(pShadowLight);
                }
            }
            else if (spCubeMapScene) {
                // Takes a reference on the accumulator here when only one is held.
                BSShaderAccumulator* pAccumulator = spAccumulator;
                if (pAccumulator->m_uiRefCount == 1) {
                    pAccumulator->IncRefCount();
                }
                spAccumulator->StartAccumulating(this);
                spAccumulator->bAccumulate = true;
                BSCullingProcess kCuller(nullptr);
                kCuller.m_pkCamera = this;
                kCuller.SetFrustum(m_kViewFrustum);
                kCuller.SetAccumulator(spAccumulator);
                BSShaderUtil::AccumulateScene(this, spCubeMapScene, kCuller);
                BSShaderUtil::RenderScene(this, spAccumulator, false);
                kCuller.spAccumulator = nullptr;
            }
        }
        BSRenderedTexture::StopOffscreen();
    }
    BSShaderManager::pRenderer->SetBackgroundColorAlpha(kBackgroundColor);
}

void BSCubeMapCamera::ClickCubeMapWithMask(int32_t aiFace) {
    NiColorA kBackgroundColor = NiColorA::BLACK;
    uint32_t uiCulledFaces = 0;
    BSShaderManager::pRenderer->GetBackgroundColor(kBackgroundColor);
    // Unlike ClickCubeMap, this reads the accumulator's render mode.
    if (spAccumulator->eRenderMode == BSShaderManager::BSSM_RENDER_DEPTHMAP) {
        uiCulledFaces = pShadowLight->usFrustumCull;
        NiColorA kDepthColor(4096.0f, 4096.0f, 4096.0f, 1.0f);
        BSShaderManager::pRenderer->SetBackgroundColorAlpha(kDepthColor);
    }
    for (int32_t iFace = 0; iFace < 6; ++iFace) {
        if (aiFace != -1 && iFace != aiFace) {
            continue;
        }
        SetupTransformation(static_cast<NiRenderedCubeMap::FaceID>(iFace));
        // With no rendered cube map the texture is null, and the face is still set and the group still read through
        // the null pointers.
        NiRenderedCubeMap* pCubeMap = static_cast<NiRenderedCubeMap*>(
            spRenderedCubeMap ? spRenderedCubeMap->spTexture[0].m_pObject : nullptr
        );
        pCubeMap->SetCurrentCubeFace(static_cast<NiRenderedCubeMap::FaceID>(iFace));
        BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, spRenderedCubeMap->GetGroup());
        uint32_t uiFaceBit = 0;
        switch (iFace) {
            case NiRenderedCubeMap::FACE_POS_X: uiFaceBit = 1; break;
            case NiRenderedCubeMap::FACE_NEG_X: uiFaceBit = 2; break;
            case NiRenderedCubeMap::FACE_POS_Y: uiFaceBit = 4; break;
            case NiRenderedCubeMap::FACE_NEG_Y: uiFaceBit = 8; break;
            case NiRenderedCubeMap::FACE_POS_Z: uiFaceBit = 16; break;
            case NiRenderedCubeMap::FACE_NEG_Z: uiFaceBit = 32; break;
            default: break;
        }
        if (static_cast<uint16_t>(uiFaceBit & uiCulledFaces) == 0) {
            NiUpdateData kUpdateData;
            kUpdateData.fTime = 0.0f;
            kUpdateData.bUpdateControllers = false;
            kUpdateData.bParallelUpdate = false;
            kUpdateData.bFoundParticles = false;
            kUpdateData.bFoundMorphController = false;
            kUpdateData.bSceneGraphChange = false;
            Update(kUpdateData);
            if (BSShaderManager::eRenderMode == BSShaderManager::BSSM_RENDER_DEPTHMAP) {
                if (pShadowLight) {
                    BSShaderLightingProperty::SetLight(0, pShadowLight, 1.0f);
                    // spRenderedCubeMap is an NiPointer: the shadow light's dynamic shadow map.
                    BSRenderedTexture* pShadowMap = pShadowLight->spDynamicShadowMap.m_pObject;
                    BSRenderedTexture* pOld = spRenderedCubeMap;
                    if (pOld != pShadowMap) {
                        if (pOld) {
                            pOld->DecRefCount();
                        }
                        spRenderedCubeMap = pShadowMap;
                        if (pShadowMap) {
                            pShadowMap->IncRefCount();
                        }
                    }
                    RenderGeomForLight(pShadowLight);
                }
            }
            else if (spCubeMapScene) {
                // Takes a reference on the accumulator while only one is held, and drops it after rendering.
                bool bAddedRef = false;
                BSShaderAccumulator* pAccumulator = spAccumulator;
                if (pAccumulator->m_uiRefCount == 1) {
                    pAccumulator->IncRefCount();
                    bAddedRef = true;
                }
                spAccumulator->StartAccumulating(this);
                spAccumulator->bAccumulate = true;
                // BSCullingProcess's destructor runs at the block's end, after the accumulator's reference is dropped.
                BSCullingProcess kCuller(nullptr);
                kCuller.m_pkCamera = this;
                kCuller.SetFrustum(m_kViewFrustum);
                kCuller.SetAccumulator(spAccumulator);
                BSShaderUtil::AccumulateScene(this, spCubeMapScene, kCuller);
                BSShaderUtil::RenderScene(this, spAccumulator, false);
                kCuller.spAccumulator = nullptr;
                if (bAddedRef) {
                    BSShaderAccumulator* pHeld = spAccumulator;
                    pHeld->DecRefCount();
                }
            }
        }
        BSRenderState::SetAlphaBlendEnable(true, BSRS_LOCK);
        BSRenderState::SetAlphaBlendFunc(
            NiAlphaProperty::ALPHA_SRCALPHA,
            NiAlphaProperty::ALPHA_INVSRCALPHA,
            BSRS_NOLOCK
        );
        // Loads the mask before constructing the parameters, and the image space manager and the renderer before
        // InitConstantMap.
        NiSourceTexture* pMask = static_cast<NiSourceTexture*>(BSShaderManager::spWindowEnvironmentMap01.m_pObject);
        // Constructs the parameters in local storage and uses the local's address (the same pointer Construct returns).
        alignas(ImageSpaceShaderParam) uint8_t aucParam[sizeof(ImageSpaceShaderParam)];
        ImageSpaceShaderParam* pParam = ::new (aucParam) ImageSpaceShaderParam;
        ImageSpaceManager* pImageSpaceManager = BSShaderManager::pImageSpaceManager;
        NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
        pParam->InitConstantMap(2, 0);
        // The mask is rotated by the face's angle: the sine and cosine are computed in double and each rounded to
        // float. FACE_POS_Z is not rotated and FACE_NEG_Z is not masked.
        double dAngle = 0.0;
        bool bRotate = true;
        bool bRender = true;
        switch (iFace) {
            case NiRenderedCubeMap::FACE_POS_X: dAngle = 4.71238899230957; break;
            case NiRenderedCubeMap::FACE_NEG_X: dAngle = 1.5707963705062866; break;
            case NiRenderedCubeMap::FACE_POS_Y: dAngle = 0.0; break;
            case NiRenderedCubeMap::FACE_NEG_Y: dAngle = 3.1415927410125732; break;
            case NiRenderedCubeMap::FACE_POS_Z: bRotate = false; break;
            default: bRender = false; break;
        }
        if (bRender) {
            if (bRotate) {
                float fSin = static_cast<float>(std::sin(dAngle));
                float fCos = static_cast<float>(std::cos(dAngle));
                pParam->SetVertexConstant(0, fCos, fSin, 0.0f, 0.0f);
                pParam->SetVertexConstant(1, -fSin, fCos, 0.0f, 0.0f);
            }
            pImageSpaceManager->RenderEffect(
                ImageSpaceManager::EFFECT_SHADER_TEXTURE_MASK,
                pRenderer,
                pMask,
                spRenderedCubeMap,
                pParam,
                false
            );
        }
        BSRenderState::RestoreAlphaBlendEnable(BSRS_LOCK);
        BSRenderedTexture::StopOffscreen();
        pParam->~ImageSpaceShaderParam();
    }
    BSShaderManager::pRenderer->SetBackgroundColorAlpha(kBackgroundColor);
}
