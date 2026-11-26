#include "Fallout/Interface/FOVATSEffectManager.h"

#include "BSCore/BSTimer.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSNoise1D.h"
#include "BSMain/BSOcclusionQuery.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectInterface.h"
#include "BSShader/ImageSpaceEffectVATSScan.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> fVATSScanlineFrequency("fScanlineFrequency:VATS", 125.0f);
SettingT<INISettingCollection> fVATSBlurRadius("fBlurRadius:VATS", 4.0f);
SettingT<INISettingCollection> fVATSBlurIntensity("fBlurIntensity:VATS", 0.5f);
SettingT<INISettingCollection> fVATSDistortVerticalScale("fDistortVerticalScale:VATS", 5.0f);
SettingT<INISettingCollection> fVATSDistortHorizontalScale("fDistortHorizontalScale:VATS", 1.0f);
SettingT<INISettingCollection> fVATSDistortDuration("fDistortDuration:VATS", 500.0f);
SettingT<INISettingCollection> iVATSDistortMaxInterval("iDistortMaxInterval:VATS", 4500);
SettingT<INISettingCollection> fVATSScanDuration("fScanModeDuration:VATS", 250.0f);
SettingT<INISettingCollection> fVATSScanVerticalScale("fScanModeVerticalScale:VATS", 2.0f);
SettingT<INISettingCollection> fVATSScanScanlineFrequency("fScanModeScanlineFrequency:VATS", 150.0f);
SettingT<INISettingCollection> fVATSBurstDuration("fBurstDuration:VATS", 200.0f);
SettingT<INISettingCollection> fVATSBurstIntensity("fBurstIntensity:VATS", 5.0f);
SettingT<INISettingCollection> fVATSPulseIntensity("fPulseIntensity:VATS", 0.3f);
SettingT<INISettingCollection> fVATSPulseRadius("fPulseRadius:VATS", 0.05f);
SettingT<INISettingCollection> fVATSPulseRate("fPulseRate:VATS", 0.0005f);
SettingT<INISettingCollection> fVATSTargetPulseRate("fVATSTargetPulseRate:VATS", 0.005f);
SettingT<INISettingCollection> fVATSColorR("fVATSColorR:VATS", 0.256f);
SettingT<INISettingCollection> fVATSColorG("fVATSColorG:VATS", 0.823f);
SettingT<INISettingCollection> fVATSColorB("fVATSColorB:VATS", 0.627f);
SettingT<INISettingCollection>
    sVATSScanlinesTexture("sScanlineTexture:VATS", "Data\\Textures\\Pipboy3000\\PipboyScanlines.dds");

FOVATSEffectManager::VATS_TARGET::VATS_TARGET() : spObj(nullptr) {}

FOVATSEffectManager::FOVATSEffectManager()
  : EffectMode(FOVATSEffectManager::SINGLE_TARGET), SingleTarget(), iNumMultiTargets(0), iSelectedMultiTarget(-1),
    pMultiTargets(), spStencilProp(nullptr), bOcclusionEnabled(false), bEffectEnabled(true), iNumAdditionalLimbs(0),
    bDistortActive(false), fDistortPosition(0.0f), fDistortDuration(fVATSDistortDuration.Float()), bBurstActive(false),
    fPulseCurrentIntensity(0.0f), fPulseCurrentRadius(0.0f), bScanning(false), bExitScanMode(false),
    fPulseIntensity(0.0f), spScanlinesTexture(nullptr), uiPartialResultMult(1), spAccumulator(nullptr) {
    double fTime = static_cast<double>(appTimer.iLastTime);
    fNextDistortTime = static_cast<double>(rt::rand() % iVATSDistortMaxInterval.Int()) + fTime;
    SingleTarget.pRef = nullptr;
    BSShaderManager::GetTexture(sVATSScanlinesTexture.String(), true, spScanlinesTexture, false, false);
    spAccumulator = new BSShaderAccumulator(101, 1u, 759u);
    spAccumulator->SetRenderDecals(false);
    for (int32_t i = 0; i < 16; ++i) {
        bWaitingForOcclusionQuery[i] = false;
        pEntireLimbQuery[i] = nullptr;
        pPartialLimbQuery[i] = nullptr;
        bValidOcclusionResult[i] = 0;
    }
}

FOVATSEffectManager::~FOVATSEffectManager() {
    SingleTarget.pRef = nullptr;
    for (int32_t i = 0; i < 16; ++i) {
        uiTotalArea[i] = 0;
        uiVisibleArea[i] = 0;
        delete pEntireLimbQuery[i];
        pEntireLimbQuery[i] = nullptr;
        delete pPartialLimbQuery[i];
        pPartialLimbQuery[i] = nullptr;
    }
    ClearMultiTargets();
}

void FOVATSEffectManager::OnIdle() {
    uint32_t itempResultPartial;
    uint32_t itempResultEntire;
    if (SingleTarget.spObj.m_pObject) {
        float fappTime = static_cast<float>(appTimer.iLastTime);
        if (bScanning) {
            UpdateScan(fappTime);
        }
        else {
            if (bDistortActive) {
                UpdateDistort(fappTime);
            }
            else {
                if (fNextDistortTime < static_cast<double>(fappTime)) {
                    TriggerDistort();
                }
            }
            UpdatePulse(fappTime);
            if (EffectMode == FOVATSEffectManager::MULTIPLE_TARGETS && iSelectedMultiTarget != -1) {
                UpdateTargetPulse(fappTime);
            }
        }
        if (bBurstActive) {
            UpdateBurst(fappTime);
        }
        if (SingleTarget.pRef && SingleTarget.pRef->IsDead(false)) {
            SetTarget(nullptr, BGSBodyPart::LIMB_NONE, false);
        }

        InitializationFlow("====== VATSEffectOnIdle ======");
        for (uint32_t i = 0; i < 16; ++i) {
            if (SingleTarget.spObj.m_pObject && pEntireLimbQuery[i] && pPartialLimbQuery[i]) {
                NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->LockRenderer();
                if (pEntireLimbQuery[i]->GetResult(&itempResultEntire, true) &&
                    pPartialLimbQuery[i]->GetResult(&itempResultPartial, true) &&
                    !bValidOcclusionResult[i]) {
                    BSOcclusionQuery* pQuery = pEntireLimbQuery[i];
                    if (pQuery) {
                        delete pQuery;
                    }
                    pEntireLimbQuery[i] = nullptr;
                    pQuery = pPartialLimbQuery[i];
                    if (pQuery) {
                        delete pQuery;
                    }
                    pPartialLimbQuery[i] = nullptr;
                    bValidOcclusionResult[i] = 1;
                    bWaitingForOcclusionQuery[i] = false;
                    uiTotalArea[i] = itempResultEntire;
                    uiVisibleArea[i] = uiPartialResultMult * itempResultPartial;
                }
                pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->UnlockRenderer();
            }
        }
    }
    else {
        bScanning = false;
        bExitScanMode = false;
    }
}

void FOVATSEffectManager::SetTarget(TESObjectREFR* apTarget, BGSBodyPart::LIMB_ENUM aeLimb, bool abDoOcclusion) {
    NiNode* pNode;
    if (apTarget) {
        if (abDoOcclusion) {
            InitializationFlow("OCCLUDED SetTarget issued with Target=%p LIMB %i", apTarget, aeLimb);
        }
    }
    else {
        InitializationFlow("\n----=============TARGET RESET===============----");
    }
    if (apTarget) {
        pNode = apTarget->Get3D();
    }
    else {
        pNode = nullptr;
    }
    iNumAdditionalLimbs = 0;
    if (pNode != SingleTarget.spObj.m_pObject && bScanning) {
        bScanning = false;
        bExitScanMode = false;
    }
    if (bEffectEnabled && !bScanning && apTarget) {
        pNode = apTarget->Get3D();
        if (pNode == SingleTarget.spObj.m_pObject) {
            float fBurstIntensity = fVATSBurstIntensity.Float();
            TriggerBurst(fVATSBurstDuration.Float(), fBurstIntensity, true);
        }
    }

    bOcclusionEnabled = abDoOcclusion;
    if (bOcclusionEnabled) {
        bWaitingForOcclusionQuery[aeLimb] = true;
    }
    eLimb = aeLimb;
    SingleTarget.pRef = apTarget;
    NiAVObject* pTarget3D;
    if (apTarget) {
        pTarget3D = apTarget->Get3D();
    }
    else {
        pTarget3D = nullptr;
    }
    SingleTarget.spObj = pTarget3D;
    bWaitingForOcclusionQuery[aeLimb] = false;
    bValidOcclusionResult[aeLimb] = 0;
    if (aeLimb != BGSBodyPart::LIMB_NONE) {
        if (pEntireLimbQuery[aeLimb]) {
            BSOcclusionQuery* pQuery = pEntireLimbQuery[aeLimb];
            if (pQuery) {
                delete pQuery;
            }
            pEntireLimbQuery[aeLimb] = nullptr;
        }
        if (pPartialLimbQuery[aeLimb]) {
            BSOcclusionQuery* pQuery = pPartialLimbQuery[aeLimb];
            if (pQuery) {
                delete pQuery;
            }
            pPartialLimbQuery[aeLimb] = nullptr;
        }
    }
}

void FOVATSEffectManager::ClearMultiTargets() {
    for (int32_t i = 0; i < iNumMultiTargets; ++i) {
        pMultiTargets[i].pRef = nullptr;
        pMultiTargets[i].spObj = nullptr;
    }
    iNumMultiTargets = 0;
    iSelectedMultiTarget = -1;
}

void FOVATSEffectManager::DrawSingleTarget(bool abQueryMode, FOVATSEffectManager::VATS_DRAW_MODE aeDrawMode) {
    if (SingleTarget.pRef) {
        if (((bEffectEnabled && !bScanning) || abQueryMode) &&
            aeDrawMode != FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) {
            IsolateBodyPart();
        }
        SceneGraph* psg = Main::GetWorldSceneGraph();
        Actor* pActor = static_cast<Actor*>(SingleTarget.pRef);
        BSCullingProcess* pCuller = psg->pCuller;
        pCuller->SetAccumulator(spAccumulator.m_pObject);
        pCuller->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pcam = psg->GetCamera();
        BaseProcess* pProcess = nullptr;
        NiNode* pWeapon = nullptr;
        if (SingleTarget.pRef->IsActor()) {
            pProcess = pActor->pCurrentProcess;
        }
        if (pProcess) {
            BipedAnim* pBiped = pActor->GetBiped();
            pWeapon = pProcess->GetWeaponBone(pBiped);
        }
        if ((eLimb == BGSBodyPart::LIMB_WEAPON || aeDrawMode == FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) &&
            SingleTarget.pRef->IsActor() &&
            (!bScanning || abQueryMode) &&
            pWeapon &&
            pProcess) {
            BSShaderUtil::AccumulateScene(pcam, pWeapon, *pCuller);
        }

        if (eLimb != BGSBodyPart::LIMB_WEAPON || aeDrawMode == FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) {
            bool bHidWeapon = false;
            if (pWeapon) {
                bHidWeapon = !pWeapon->GetAppCulled();
                pWeapon->SetAppCulled(true);
            }
            BSShaderUtil::AccumulateScene(pcam, SingleTarget.spObj.m_pObject, *pCuller);
            if (SingleTarget.pRef->IsActor()) {
                pActor = static_cast<Actor*>(SingleTarget.pRef);
                TESActorBase* pActorBase = pActor->GetActorBase();
                BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                BGSBodyPart* pPart = pBodyPartData->GetBodyPart(eLimb);
                if (pPart) {
                    NiNode* pTargetRootNode = pActor->Get3D();
                    NiAVObject* pLimbNode = pTargetRootNode->GetObjectByName(pPart->GetNodeName());
                    if (pLimbNode) {
                        BSShaderUtil::AccumulateScene(pcam, pLimbNode, *pCuller);
                    }
                }
                if (iNumAdditionalLimbs > 0) {
                    for (int32_t i = 0; i < iNumAdditionalLimbs; ++i) {
                        TESActorBase* pBaseForm = pActor->GetActorBase();
                        pBodyPartData = pBaseForm->GetBodyPartData();
                        pPart = pBodyPartData->GetBodyPart(pAdditionalLimbs[i]);
                        if (pPart) {
                            NiNode* pTargetRootNode = pActor->Get3D();
                            NiAVObject* pLimbNode = pTargetRootNode->GetObjectByName(pPart->GetNodeName());
                            if (pLimbNode) {
                                BSShaderUtil::AccumulateScene(pcam, pLimbNode, *pCuller);
                            }
                        }
                    }
                }
            }
            if (pWeapon && bHidWeapon) {
                pWeapon->SetAppCulled(false);
            }
        }
        BSShaderUtil::RenderScene(pcam, spAccumulator.m_pObject, false);
        pCuller->SetAccumulator(nullptr);
        pCuller->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
        if (((bEffectEnabled && !bScanning) || abQueryMode) &&
            aeDrawMode != FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) {
            RestoreBodyParts();
        }
    }
}

void FOVATSEffectManager::DrawMultipleTargets() {
    if (iNumMultiTargets > 0) {
        NiRenderer::GetRenderer();
        SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
        BSCullingProcess* pCuller = pSceneGraph->GetCullingProcess();
        NiAccumulator* pAccumulator = spAccumulator.m_pObject;
        pCuller->SetAccumulator(pAccumulator);
        NiCamera* pCamera = pSceneGraph->GetCamera();
        NiRenderer::GetRenderer()->SetCameraData(pCamera);
        pCuller->SetCamera(pCamera);
        pCuller->SetFrustum(pCamera->GetViewFrustum());
        bool bFlagsSet = false;
        NiAVObject* pRootObject = nullptr;
        for (int32_t i = 0; i < iNumMultiTargets; ++i) {
            ShadowSceneNode* pTargetNode = static_cast<ShadowSceneNode*>(pMultiTargets[i].spObj.m_pObject);
            if (i == iSelectedMultiTarget) {
                BSShaderProperty::SetFlagRecurse(pTargetNode, BSShaderProperty::BSSP_FLAG_VATS_SELECTION, 1);
                pRootObject = pTargetNode;
                bFlagsSet = true;
            }
            BSShaderUtil::AccumulateScene(pCamera, pTargetNode, *pCuller);
        }
        BSShaderUtil::RenderScene(pCamera, spAccumulator.m_pObject, false);
        pCuller->SetAccumulator(nullptr);
        if (bFlagsSet) {
            BSShaderProperty::SetFlagRecurse(pRootObject, BSShaderProperty::BSSP_FLAG_VATS_SELECTION, 0);
        }
    }
}

void FOVATSEffectManager::DrawTarget(bool abQueryMode, VATS_DRAW_MODE aeDrawMode) {
    if (EffectMode == SINGLE_TARGET) {
        DrawSingleTarget(abQueryMode, aeDrawMode);
    }
    else {
        DrawMultipleTargets();
    }
}

void FOVATSEffectManager::UpdatePulse(float afTime) {
    float fX = afTime * fVATSPulseRate.Float();
    float fNoise = BSNoise1D::Noise(fX);
    float fPulse = fNoise + fNoise;
    fPulseCurrentIntensity = fPulse * fVATSPulseIntensity.Float();
    fPulseCurrentRadius = fPulse * fVATSPulseRadius.Float();
}

void FOVATSEffectManager::SetMultiTarget(TESObjectREFR* apTarget) {
    bScanning = false;
    for (int32_t i = 0; i < iNumMultiTargets; ++i) {
        if (pMultiTargets[i].pRef == apTarget) {
            return;
        }
    }
    if (iNumMultiTargets + 1 < 32) {
        pMultiTargets[iNumMultiTargets].pRef = apTarget;
        pMultiTargets[iNumMultiTargets].spObj = apTarget->Get3D();
        ++iNumMultiTargets;
        if (!SingleTarget.pRef) {
            SingleTarget.pRef = apTarget;
            SingleTarget.spObj = apTarget->Get3D();
        }
        BSShaderProperty::SetFlagRecurse(
            pMultiTargets[iNumMultiTargets].spObj.m_pObject,
            BSShaderProperty::BSSP_FLAG_VATS_SELECTION,
            0
        );
    }
    else {
        Warning("Too many VATS targets.");
    }
}

void FOVATSEffectManager::TriggerBurst(float afBurstDuration, float afBurstIntensity, bool abPlaySound) {
    bBurstActive = true;
    fBurstDuration = afBurstDuration;
    fBurstCurrentIntensity = afBurstIntensity;
    fBurstMaximumIntensity = afBurstIntensity;
    fBurstStartTime = static_cast<float>(appTimer.BSTimer::GetTime());
}

void FOVATSEffectManager::SetEffectMode(const VATS_EFFECT_MODE& aMode) {
    EffectMode = aMode;
}

NiAVObject* FOVATSEffectManager::GetTarget() {
    return SingleTarget.spObj.m_pObject;
}

void FOVATSEffectManager::PrepareHighlightEffect(
    BSRenderedTexture* apSourceColors,
    BSRenderedTexture* apSourceMask,
    BSRenderedTexture* apTarget,
    bool abClearTarget
) {
    NiColorA newcolor(0.0f, 0.0f, 0.0f, 0.0f);
    SystemColorManager::GetInstance()->GetColor(1, newcolor);
    ImageSpaceEffectInterface::eMode = 1;
    BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_INTERFACE)->bIsActive = true;
    ImageSpaceEffectInterface::spMaskTexture = static_cast<NiTexture*>(apSourceMask->GetTexture(0));
    if (bDistortActive) {
        ImageSpaceEffectInterface::fDistortVerticalScale = fVATSDistortVerticalScale.Float();
        ImageSpaceEffectInterface::fDistortHorizontalScale = fVATSDistortHorizontalScale.Float();
        ImageSpaceEffectInterface::fDistortVerticalPosition = fDistortPosition;
        ImageSpaceEffectInterface::bDistortEverywhere = false;
    }
    else {
        ImageSpaceEffectInterface::fDistortVerticalScale = 0.0f;
        ImageSpaceEffectInterface::fDistortHorizontalScale = 0.0f;
        ImageSpaceEffectInterface::fDistortVerticalPosition = -1.0f;
    }
    ImageSpaceEffectInterface::fVerticalHold = 0.0f;
    ImageSpaceEffectInterface::spScanlinesTexture = spScanlinesTexture;
    ImageSpaceEffectInterface::kTint = newcolor;
    ImageSpaceEffectInterface::fBlurIntensity = fVATSBlurIntensity.Float();
    ImageSpaceEffectInterface::fBlurRadius = fVATSBlurRadius.Float();
    ImageSpaceEffectInterface::fBlurIntensity = ImageSpaceEffectInterface::fBlurIntensity + fPulseCurrentIntensity;
    ImageSpaceEffectInterface::fBlurRadius = ImageSpaceEffectInterface::fBlurRadius + fPulseCurrentRadius;
    if (bBurstActive) {
        ImageSpaceEffectInterface::fBlurIntensity = (fBurstCurrentIntensity * 0.5 + 1.0f) *
            ImageSpaceEffectInterface::fBlurIntensity;
        ImageSpaceEffectInterface::fBlurRadius = (fBurstCurrentIntensity + 1.0f) *
            ImageSpaceEffectInterface::fBlurRadius;
    }
    ImageSpaceEffectInterface::fPulse = fPulseIntensity;
    ImageSpaceEffectInterface::fScanlineFrequency = fVATSScanlineFrequency.Float();
    ImageSpaceEffectInterface::fTextureUsedPart = 1.0f;
    ImageSpaceEffectInterface::eClearFlag = static_cast<NiRenderer::ClearFlags>(abClearTarget);
}

void FOVATSEffectManager::PrepareScanEffect(
    BSRenderedTexture* apSourceColors,
    BSRenderedTexture* apSourceMask,
    BSRenderedTexture* apTarget,
    bool abClearTarget
) {
    NiColorA newcolor(0.0f, 0.0f, 0.0f, 0.0f);
    SystemColorManager::GetInstance()->GetColor(1, newcolor);
    ImageSpaceEffectVATSScan::fScanVerticalPosition = fScanPosition;
    ImageSpaceEffectVATSScan::fScanVerticalScale = fVATSScanVerticalScale.Float();
    ImageSpaceEffectVATSScan::fScanlineFrequency = fVATSScanScanlineFrequency.Float();
    ImageSpaceEffectVATSScan::kTint = newcolor;
    ImageSpaceEffectVATSScan::spScanlinesTexture = spScanlinesTexture;
    ImageSpaceEffectVATSScan::spMaskTexture = static_cast<NiTexture*>(apSourceMask->GetTexture(0));
    BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_VATS_SCAN)->bIsActive = true;
}

bool FOVATSEffectManager::OcclusionEnabled() {
    return bOcclusionEnabled;
}

void FOVATSEffectManager::DrawTargetToTexture(
    BSRenderedTexture* apTarget,
    FOVATSEffectManager::VATS_OCCLUSION_QUERY aQueryType,
    bool abQueryMode,
    FOVATSEffectManager::VATS_DRAW_MODE aeDrawMode,
    NiRenderer::ClearFlags aeClearMode,
    bool abStopRenderTarget
) {
    if (!SingleTarget.pRef->IsDead(false)) {
        Actor* pActor;
        NiDX9Renderer* pr = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        NiColorA prevcolor(0.0f, 0.0f, 0.0f, 0.0f);
        pr->GetBackgroundColor(prevcolor);
        NiColorA clearcolor(0.0f, 0.0f, 0.0f, 0.0f);
        pr->SetBackgroundColorAlpha(clearcolor);
        if (BSRenderedTexture::QOutsideFrame()) {
            NiRenderTargetGroup* pGroup = apTarget->GetGroup();
            BSRenderedTexture::StartOffscreen(aeClearMode, pGroup);
        }
        Lighting30Shader::bAlwaysWriteAlpha = true;
        ShadowLightShader::bAlwaysWriteAlpha = true;
        bool bIsOcclusionOn = bOcclusionEnabled;
        bool bEndQuery = false;
        if (SingleTarget.pRef->IsActor()) {
            pActor = static_cast<Actor*>(SingleTarget.pRef);
        }
        else {
            pActor = nullptr;
        }
        ExtraRefractionProperty* pRPE = nullptr;
        float fDetectionBoost = 0.0f;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_DETECTION, pPlayer, &fDetectionBoost);
        if (pActor && fDetectionBoost > 0.0) {
            pRPE = pActor->m_Extra.GetRefractionPropertyExtra();
        }

        bool bHasRefraction = false;
        float fRefractPower = 0.0f;
        if (pRPE && pActor) {
            fRefractPower = pRPE->fRefractionPower;
            bHasRefraction = true;
            pActor->SetRefraction(false, 0.0f);
        }

        if (bIsOcclusionOn) {
            if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_PARTIAL) {
                BSOcclusionQuery* pOcclusionQuery;
                if (pPartialLimbQuery[eLimb]) {
                    BSOcclusionQuery* pPreviousQuery = pPartialLimbQuery[eLimb];
                    if (pPreviousQuery) {
                        delete pPreviousQuery;
                    }
                }
                pOcclusionQuery = new BSOcclusionQuery;
                pPartialLimbQuery[eLimb] = pOcclusionQuery;
                bEndQuery = true;
                pPartialLimbQuery[eLimb]->Begin();
            }
            else if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_FULL) {
                BSOcclusionQuery* pOcclusionQuery;
                if (pEntireLimbQuery[eLimb]) {
                    BSOcclusionQuery* pPreviousQuery = pEntireLimbQuery[eLimb];
                    if (pPreviousQuery) {
                        delete pPreviousQuery;
                    }
                }
                pOcclusionQuery = new BSOcclusionQuery;
                pEntireLimbQuery[eLimb] = pOcclusionQuery;
                bEndQuery = true;
                pEntireLimbQuery[eLimb]->Begin();
            }
        }
        spAccumulator->SetRenderMode(BSShaderManager::BSSM_RENDER_CONSTALPHA);
        BSShaderManager::SetRenderMode(8u);
        DrawTarget(abQueryMode, aeDrawMode);
        BSShaderManager::SetRenderMode(0);
        spAccumulator->SetRenderMode(BSShaderManager::BSSM_RENDER_NORMAL);
        if (bEndQuery) {
            if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_PARTIAL) {
                if (pPartialLimbQuery[eLimb]) {
                    pPartialLimbQuery[eLimb]->End();
                    bWaitingForOcclusionQuery[eLimb] = true;
                }
                else {
                    Warning("pPartialLimbQuery is NULL even though we just issued one.  This should be impossible.");
                }
            }
            else if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_FULL) {
                if (pEntireLimbQuery[eLimb]) {
                    pEntireLimbQuery[eLimb]->End();
                }
                else {
                    Warning("pEntireLimbQuery is NULL even though we just issued one.  This should be impossible.");
                }
            }
        }
        if (abStopRenderTarget) {
            BSRenderedTexture::StopOffscreen();
        }
        if (bHasRefraction && pActor) {
            pActor->SetRefraction(true, fRefractPower);
        }

        Lighting30Shader::bAlwaysWriteAlpha = false;
        ShadowLightShader::bAlwaysWriteAlpha = false;
        pr->SetBackgroundColorAlpha(prevcolor);
    }
}

void FOVATSEffectManager::PrepareEffect(
    BSRenderedTexture* apSourceColors,
    BSRenderedTexture* apSourceMask,
    BSRenderedTexture* apTarget,
    bool abClearTarget
) {
    if (!Interface::GetIsVATSRescanning() && bEffectEnabled) {
        if (bScanning) {
            PrepareScanEffect(apSourceColors, apSourceMask, apTarget, abClearTarget);
        }
        else {
            PrepareHighlightEffect(apSourceColors, apSourceMask, apTarget, abClearTarget);
        }
    }
}

// Fetches the manager and the effect again before each call.
void FOVATSEffectManager::ShutdownEffect() {
    BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_INTERFACE)->SetActive(false);
    BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_VATS_SCAN)->SetActive(false);
    BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_INTERFACE)->ReturnTextures();
    BSShaderManager::GetImageSpaceManager()->GetEffect(ImageSpaceManager::EFFECT_VATS_SCAN)->ReturnTextures();
    ImageSpaceEffectInterface::eMode = 0;
}

// The areas are unsigned; they are divided as doubles and the quotient is stored as a float.
bool FOVATSEffectManager::GetVATSGeometryOcclusionPercent(uint32_t aeBodyPart, float& afResult_out) {
    if (bValidOcclusionResult[aeBodyPart]) {
        double dPercent;
        if (uiTotalArea[aeBodyPart]) {
            dPercent = static_cast<double>(uiVisibleArea[aeBodyPart]) / static_cast<double>(uiTotalArea[aeBodyPart]);
        }
        else {
            dPercent = 0.0;
        }
        afResult_out = static_cast<float>(dPercent);
        return true;
    }
    afResult_out = -1.0f;
    return false;
}

FOVATSEffectManager::VATS_EFFECT_MODE FOVATSEffectManager::GetEffectMode() {
    return EffectMode;
}

bool FOVATSEffectManager::IsScanning() {
    return bScanning;
}

void FOVATSEffectManager::DisableScan() {
    if (bScanning) {
        bExitScanMode = true;
    }
}

void FOVATSEffectManager::TriggerDistort() {
    bDistortActive = true;
    fDistortStartTime = static_cast<float>(appTimer.BSTimer::GetTime());
    fDistortPosition = 0.0f;
}

void FOVATSEffectManager::TriggerScan() {
    if (!bScanning) {
        bScanning = true;
        fScanStartTime = -0x1p+0f;
        fScanPosition = 0.0f;
    }
}

void FOVATSEffectManager::UpdateTargetPulse(float afTime) {
    float fX = afTime * fVATSTargetPulseRate.Float();
    fPulseIntensity = static_cast<float>((std::sinf(fX) + 1.0) * 0.5);
}

void FOVATSEffectManager::SetSelectedMultiTarget(TESObjectREFR* apTarget) {
    int32_t iIndex = -1;
    for (int32_t i = 0; i < iNumMultiTargets; ++i) {
        if (pMultiTargets[i].pRef == apTarget) {
            iIndex = i;
        }
    }
    if (iIndex == -1) {
        if (iNumMultiTargets < 32) {
            iSelectedMultiTarget = iNumMultiTargets;
            SetMultiTarget(apTarget);
        }
    }
    else {
        iSelectedMultiTarget = iIndex;
    }
}

void FOVATSEffectManager::UpdateBurst(float afTime) {
    float fElapsed = afTime - fBurstStartTime;
    if (fBurstDuration < static_cast<double>(fElapsed)) {
        bBurstActive = false;
    }
    else {
        float fRemaining = static_cast<float>(1.0 - fElapsed / fBurstDuration);
        fBurstCurrentIntensity = fBurstMaximumIntensity * fRemaining;
    }
}

void FOVATSEffectManager::UpdateDistort(float afTime) {
    float fElapsed = afTime - fDistortStartTime;
    fDistortPosition = fElapsed / fDistortDuration;
    if (fDistortPosition > fVATSDistortVerticalScale.Float() + 1.0) {
        bDistortActive = false;
        int32_t iRand = rt::rand();
        fNextDistortTime = static_cast<float>(
            static_cast<double>(iRand % static_cast<int32_t>(iVATSDistortMaxInterval.UInt())) + afTime
        );
    }
}

void FOVATSEffectManager::DisableOcclusion() {
    InitializationFlow("--==OCCLUSION DISABLED==--\n");
    bOcclusionEnabled = false;
}

void FOVATSEffectManager::UpdateScan(float afTime) {
    if (fScanStartTime == -1.0) {
        fScanStartTime = afTime;
    }
    float fElapsed = afTime - fScanStartTime;
    fScanPosition = fElapsed / fVATSScanDuration.Float();
    if (bExitScanMode && fScanPosition > fVATSScanVerticalScale.Float() + 1.0) {
        bScanning = false;
        bExitScanMode = false;
        float fBurstIntensity = fVATSBurstIntensity.Float();
        TriggerBurst(fVATSBurstDuration.Float(), fBurstIntensity, true);
    }
}

void FOVATSEffectManager::ShowBipedObjectOnActor(Actor* apActor, BIPED_OBJECT aeObject, bool abShow) {
    BipedAnim* pBipedAnim = apActor->GetBiped();
    if (pBipedAnim) {
        NiNode* pBoneNode = static_cast<NiNode*>(pBipedAnim->GetBipObject(aeObject)->pPartClone);

        if (pBoneNode) {
            pBoneNode->SetAppCulled(!abShow);
        }
    }
}

void* FOVATSEffectManager::ClearsData() {
    ClearsData_();
    return this;
}

void* FOVATSEffectManager::ClearsData_() {
    ::new (this) BGSLoadGameSubBuffer;
    return this;
}

FOVATSEffectManager* FOVATSEffectManager::Clears() {
    ClearsData();
    SingleTarget.pRef = nullptr;
    return this;
}

// iNumAdditionalLimbs is reloaded after storing the limb, so a store that lands on the count is seen.
void FOVATSEffectManager::TargetAdditionalLimb(BGSBodyPart::LIMB_ENUM aeLimb) {
    pAdditionalLimbs[iNumAdditionalLimbs] = aeLimb;
    ++iNumAdditionalLimbs;
}

void FOVATSEffectManager::IsolateBodyPart() {
    if (SingleTarget.pRef->IsActor() && eLimb != BGSBodyPart::LIMB_WEAPON && eLimb != BGSBodyPart::LIMB_COUNT) {
        Actor* pActor = static_cast<Actor*>(SingleTarget.pRef);
        NiNode* pActorRoot = SingleTarget.pRef->Get3D();
        for (uint32_t n = 0; n < pActorRoot->GetArrayCount(); ++n) {
            NiAVObject* pObj = pActorRoot->GetAt(n);
            if (pObj) {
                if (Animation::FindSkinnedNode(pObj)) {
                    NiNode* pNode = pObj->IsNode();
                    NiGeometry* pTri = pObj->IsGeometry();
                    uint32_t uiChild = 0;
                    while (pTri || (pNode && uiChild < pNode->GetArrayCount())) {
                        NiSkinInstance* pSkin;
                        if (pNode) {
                            if (pNode->GetAt(uiChild)) {
                                NiAVObject* pChild = pNode->GetAt(uiChild);
                                pTri = pChild->IsGeometry();
                            }
                            else {
                                pTri = nullptr;
                            }
                            ++uiChild;
                        }
                        if (pTri) {
                            pSkin = pTri->GetSkinInstance();
                        }
                        else {
                            pSkin = nullptr;
                        }
                        BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                            NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pSkin)
                        );
                        if (pdSkinInstance) {
                            for (BGSBodyPart::LIMB_ENUM eIndex = BGSBodyPart::LIMB_TORSO;
                                 eIndex < BGSBodyPart::LIMB_COUNT;
                                 eIndex = static_cast<BGSBodyPart::LIMB_ENUM>(eIndex + 1)) {
                                EnableBodyPartOnSkinInstance(pdSkinInstance, eIndex, eIndex == eLimb);
                                for (int32_t i = 0; i < iNumAdditionalLimbs; ++i) {
                                    if (eIndex == pAdditionalLimbs[i]) {
                                        EnableBodyPartOnSkinInstance(pdSkinInstance, eIndex, true);
                                    }
                                }
                            }
                        }
                        pTri = nullptr;
                    }
                }
                else {
                    pObj->SetAppCulled(true);
                }
            }
        }
        if (pActor && eLimb == BGSBodyPart::LIMB_TORSO) {
            for (BGSBodyPart::LIMB_ENUM eIndex = BGSBodyPart::LIMB_TORSO; eIndex < BGSBodyPart::LIMB_COUNT;
                 eIndex = static_cast<BGSBodyPart::LIMB_ENUM>(eIndex + 1)) {
                TESActorBase* pActorBase = pActor->GetActorBase();
                BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                BGSBodyPart* pPart = pBodyPartData->GetBodyPart(eLimb);
                if (pPart) {
                    NiNode* pTargetRootNode = pActor->Get3D();
                    NiAVObject* pLimbNode = pTargetRootNode->GetObjectByName(pPart->GetNodeName());
                    if (pLimbNode && eIndex == eLimb) {
                        pLimbNode->SetAppCulled(false);
                    }
                    else if (pLimbNode) {
                        pLimbNode->SetAppCulled(true);
                    }
                }
            }
        }

        if (eLimb != BGSBodyPart::LIMB_LEFT_ARM_1 && eLimb != BGSBodyPart::LIMB_LEFT_ARM_2) {
            ShowBipedObjectOnActor(pActor, BO_LEFT_HAND, false);
        }
        if (eLimb != BGSBodyPart::LIMB_RIGHT_ARM_1 && eLimb != BGSBodyPart::LIMB_RIGHT_ARM_2) {
            ShowBipedObjectOnActor(pActor, BO_RIGHT_HAND, false);
        }
        if (eLimb != BGSBodyPart::LIMB_HEAD_1 && eLimb != BGSBodyPart::LIMB_HEAD_2) {
            ShowBipedObjectOnActor(pActor, BO_HAT, false);
            ShowBipedObjectOnActor(pActor, BO_EYE_GLASSES, false);
            ShowBipedObjectOnActor(pActor, BO_NOSE_RING, false);
            ShowBipedObjectOnActor(pActor, BO_EARRINGS, false);
            ShowBipedObjectOnActor(pActor, BO_HEADBAND, false);
            ShowBipedObjectOnActor(pActor, BO_MASK, false);
            ShowBipedObjectOnActor(pActor, BO_CHOKER, false);
            ShowBipedObjectOnActor(pActor, BO_MOUTH_OBJECT, false);
            ShowBipedObjectOnActor(pActor, BO_HAIR, false);
            ShowBipedObjectOnActor(pActor, BO_HEAD, false);
        }
    }
}

void FOVATSEffectManager::EnableBodyPartOnSkinInstance(
    BSDismemberSkinInstance* apInstance,
    BGSBodyPart::LIMB_ENUM aeLimb,
    bool abEnable
) {
    apInstance->SetBodyPartEnabled(aeLimb, abEnable);
    apInstance->SetBodyPartEnabled(BSDismemberSkinInstance::TORSO_SECTION * aeLimb, abEnable);
}

void FOVATSEffectManager::RestoreBodyParts() {
    if (SingleTarget.pRef->IsActor() && eLimb != BGSBodyPart::LIMB_WEAPON && eLimb != BGSBodyPart::LIMB_COUNT) {
        Actor* pActor = static_cast<Actor*>(SingleTarget.pRef);
        ShowBipedObjectOnActor(pActor, BO_LEFT_HAND, true);
        ShowBipedObjectOnActor(pActor, BO_RIGHT_HAND, true);
        ShowBipedObjectOnActor(pActor, BO_HAT, true);
        ShowBipedObjectOnActor(pActor, BO_EYE_GLASSES, true);
        ShowBipedObjectOnActor(pActor, BO_NOSE_RING, true);
        ShowBipedObjectOnActor(pActor, BO_EARRINGS, true);
        ShowBipedObjectOnActor(pActor, BO_HEADBAND, true);
        ShowBipedObjectOnActor(pActor, BO_MASK, true);
        ShowBipedObjectOnActor(pActor, BO_CHOKER, true);
        ShowBipedObjectOnActor(pActor, BO_MOUTH_OBJECT, true);
        ShowBipedObjectOnActor(pActor, BO_HAIR, true);
        ShowBipedObjectOnActor(pActor, BO_HEAD, true);
        NiNode* pActorRoot = SingleTarget.pRef->Get3D();
        for (uint32_t n = 0; n < pActorRoot->GetArrayCount(); ++n) {
            NiAVObject* pObj = pActorRoot->GetAt(n);
            if (pObj) {
                if (Animation::FindSkinnedNode(pObj)) {
                    NiNode* pNode = pObj->IsNode();
                    NiGeometry* pTri = pObj->IsGeometry();
                    uint32_t uiChild = 0;
                    while (pTri || (pNode && uiChild < pNode->GetArrayCount())) {
                        NiSkinInstance* pSkin;
                        if (pNode) {
                            if (pNode->GetAt(uiChild)) {
                                NiAVObject* pChild = pNode->GetAt(uiChild);
                                pTri = pChild->IsGeometry();
                            }
                            else {
                                pTri = nullptr;
                            }
                            ++uiChild;
                        }
                        if (pTri) {
                            pSkin = pTri->GetSkinInstance();
                        }
                        else {
                            pSkin = nullptr;
                        }
                        BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                            NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pSkin)
                        );
                        if (pdSkinInstance) {
                            for (BGSBodyPart::LIMB_ENUM eIndex = BGSBodyPart::LIMB_TORSO;
                                 eIndex < BGSBodyPart::LIMB_COUNT;
                                 eIndex = static_cast<BGSBodyPart::LIMB_ENUM>(eIndex + 1)) {
                                pdSkinInstance->SetBodyPartEnabled(eIndex, true);
                                pdSkinInstance->SetBodyPartEnabled(
                                    BSDismemberSkinInstance::TORSO_SECTION * eIndex,
                                    true
                                );
                                pdSkinInstance->SetBodyPartEnabled(
                                    eIndex + BSDismemberSkinInstance::SECTION_CAP,
                                    false
                                );
                                pdSkinInstance->SetBodyPartEnabled(eIndex + BSDismemberSkinInstance::TORSO_CAP, false);
                            }
                        }
                        pTri = nullptr;
                    }
                }
                else {
                    pObj->SetAppCulled(false);
                }
            }
        }
        if (pActor && eLimb == BGSBodyPart::LIMB_TORSO) {
            for (BGSBodyPart::LIMB_ENUM eIndex = BGSBodyPart::LIMB_TORSO; eIndex < BGSBodyPart::LIMB_COUNT;
                 eIndex = static_cast<BGSBodyPart::LIMB_ENUM>(eIndex + 1)) {
                TESActorBase* pActorBase = pActor->GetActorBase();
                BGSBodyPartData* pBodyPartData = pActorBase->GetBodyPartData();
                BGSBodyPart* pPart = pBodyPartData->GetBodyPart(eLimb);
                if (pPart) {
                    NiNode* pTargetRootNode = pActor->Get3D();
                    NiAVObject* pLimbNode = pTargetRootNode->GetObjectByName(pPart->GetNodeName());
                    if (pLimbNode) {
                        pLimbNode->SetAppCulled(false);
                    }
                }
            }
        }
    }
}

void FOVATSEffectManager::FreeOcclusionQueries() {
    for (uint32_t i = 0; i < 15; ++i) {
        bWaitingForOcclusionQuery[i] = false;
        uint32_t uiResult = 0;
        if (pPartialLimbQuery[i]) {
            pPartialLimbQuery[i]->GetResult(&uiResult, true);
            if (pPartialLimbQuery[i]) {
                delete pPartialLimbQuery[i];
            }
            pPartialLimbQuery[i] = nullptr;
        }
        if (pEntireLimbQuery[i]) {
            pEntireLimbQuery[i]->GetResult(&uiResult, true);
            if (pEntireLimbQuery[i]) {
                delete pEntireLimbQuery[i];
            }
            pEntireLimbQuery[i] = nullptr;
        }
    }
}
