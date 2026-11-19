#include "Fallout/Interface/FOVATSEffectManager.h"

#include "BSCore/BSTimer.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSNoise1D.h"
#include "BSMain/BSOcclusionQuery.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> gs_fScanlineFrequency_VATS("fScanlineFrequency:VATS", 125.0f);
SettingT<INISettingCollection> gs_fBlurRadius_VATS("fBlurRadius:VATS", 4.0f);
SettingT<INISettingCollection> gs_fBlurIntensity_VATS("fBlurIntensity:VATS", 0.5f);
SettingT<INISettingCollection> gs_fDistortVerticalScale_VATS("fDistortVerticalScale:VATS", 5.0f);
SettingT<INISettingCollection> gs_fDistortHorizontalScale_VATS("fDistortHorizontalScale:VATS", 1.0f);
SettingT<INISettingCollection> gs_fDistortDuration_VATS("fDistortDuration:VATS", 500.0f);
SettingT<INISettingCollection> gs_iDistortMaxInterval_VATS("iDistortMaxInterval:VATS", 4500);
SettingT<INISettingCollection> gs_fScanModeDuration_VATS("fScanModeDuration:VATS", 250.0f);
SettingT<INISettingCollection> gs_fScanModeVerticalScale_VATS("fScanModeVerticalScale:VATS", 2.0f);
SettingT<INISettingCollection> gs_fScanModeScanlineFrequency_VATS("fScanModeScanlineFrequency:VATS", 150.0f);
SettingT<INISettingCollection> gs_fBurstDuration_VATS("fBurstDuration:VATS", 200.0f);
SettingT<INISettingCollection> gs_fBurstIntensity_VATS("fBurstIntensity:VATS", 5.0f);
SettingT<INISettingCollection> gs_fPulseIntensity_VATS("fPulseIntensity:VATS", 0.3f);
SettingT<INISettingCollection> gs_fPulseRadius_VATS("fPulseRadius:VATS", 0.05f);
SettingT<INISettingCollection> gs_fPulseRate_VATS("fPulseRate:VATS", 0.0005f);
SettingT<INISettingCollection> fVATSTargetPulseRate("fVATSTargetPulseRate:VATS", 0.005f);
SettingT<INISettingCollection> fVATSColorR("fVATSColorR:VATS", 0.256f);
SettingT<INISettingCollection> fVATSColorG("fVATSColorG:VATS", 0.823f);
SettingT<INISettingCollection> fVATSColorB("fVATSColorB:VATS", 0.627f);
SettingT<INISettingCollection>
    gs_sScanlineTexture_VATS("sScanlineTexture:VATS", "Data\\Textures\\Pipboy3000\\PipboyScanlines.dds");

FOVATSEffectManager::VATS_TARGET::VATS_TARGET() : spObj(nullptr) {}

FOVATSEffectManager::FOVATSEffectManager()
  : EffectMode(FOVATSEffectManager::SINGLE_TARGET), SingleTarget(), iNumMultiTargets(0), iSelectedMultiTarget(-1),
    pMultiTargets(), spStencilProp(nullptr), bOcclusionEnabled(false), bEffectEnabled(true), iNumAdditionalLimbs(0),
    bDistortActive(false), fDistortPosition(0.0f), fDistortDuration(gs_fDistortDuration_VATS.Float()),
    bBurstActive(false), fPulseCurrentIntensity(0.0f), fPulseCurrentRadius(0.0f), bScanning(false),
    bExitScanMode(false), fPulseIntensity(0.0f), spScanlinesTexture(nullptr), uiPartialResultMult(1),
    spAccumulator(nullptr) {
    double fTime = static_cast<double>(appTimer.iLastTime);
    fNextDistortTime = static_cast<double>(rt::rand() % gs_iDistortMaxInterval_VATS.Int()) + fTime;
    SingleTarget.pRef = nullptr;
    BSShaderManager::GetTexture(gs_sScanlineTexture_VATS.String(), true, spScanlinesTexture, false, false);
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
        if ((SingleTarget.pRef) && (SingleTarget.pRef->IsDead(false))) {
            SetTarget(nullptr, BGSBodyPart::LIMB_NONE, false);
        }

        InitializationFlow("====== VATSEffectOnIdle ======");
        for (uint32_t i = 0; i < 16; ++i) {
            if ((SingleTarget.spObj.m_pObject) && (pEntireLimbQuery[i]) && (pPartialLimbQuery[i])) {
                NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                Renderer_->LockRenderer();
                if (pEntireLimbQuery[i]->GetResult(&itempResultEntire, true) &&
                    pPartialLimbQuery[i]->GetResult(&itempResultPartial, true) && !bValidOcclusionResult[i]) {
                    BSOcclusionQuery* v5 = pEntireLimbQuery[i];
                    if (v5) {
                        delete v5;
                    }
                    pEntireLimbQuery[i] = nullptr;
                    BSOcclusionQuery* v4 = pPartialLimbQuery[i];
                    if (v4) {
                        delete v4;
                    }
                    pPartialLimbQuery[i] = nullptr;
                    bValidOcclusionResult[i] = 1;
                    bWaitingForOcclusionQuery[i] = false;
                    uiTotalArea[i] = itempResultEntire;
                    uiVisibleArea[i] = uiPartialResultMult * itempResultPartial;
                }
                NiDX9Renderer* v2 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                v2->UnlockRenderer();
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
    if ((bEffectEnabled && !bScanning) && (apTarget)) {
        NiNode* v4 = apTarget->Get3D();
        if (v4 == SingleTarget.spObj.m_pObject) {
            float a3 = gs_fBurstIntensity_VATS.Float();
            float* v5 = &gs_fBurstDuration_VATS.Float();
            TriggerBurst(*v5, a3, true);
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
            BSOcclusionQuery* v12 = pEntireLimbQuery[aeLimb];
            if (v12) {
                delete v12;
            }
            pEntireLimbQuery[aeLimb] = nullptr;
        }
        if (pPartialLimbQuery[aeLimb]) {
            BSOcclusionQuery* v11 = pPartialLimbQuery[aeLimb];
            if (v11) {
                delete v11;
            }
            pPartialLimbQuery[aeLimb] = nullptr;
        }
    }
}

void FOVATSEffectManager::ClearMultiTargets() {
    int32_t i;
    for (i = 0; i < iNumMultiTargets; ++i) {
        pMultiTargets[i].pRef = nullptr;
        pMultiTargets[i].spObj = nullptr;
    }
    iNumMultiTargets = 0;
    iSelectedMultiTarget = (-1);
}

void FOVATSEffectManager::DrawSingleTarget(bool abQueryMode, FOVATSEffectManager::VATS_DRAW_MODE aeDrawMode) {
    if (SingleTarget.pRef) {
        if (((bEffectEnabled && !bScanning) || abQueryMode) &&
            aeDrawMode != FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) {
            IsolateBodyPart();
        }
        NiRenderer* pr = NiRenderer::GetRenderer();
        SceneGraph* psg = Main::GetWorldSceneGraph();
        Actor* pactor = static_cast<Actor*>(SingleTarget.pRef);
        BSCullingProcess* pculler = psg->pCuller;
        pculler->SetAccumulator(spAccumulator.m_pObject);
        pculler->SetCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pcam = psg->GetCamera();
        BaseProcess* pprocess = nullptr;
        NiNode* pweapon = nullptr;
        if (SingleTarget.pRef->IsActor()) {
            pprocess = pactor->pCurrentProcess;
        }
        if (pprocess) {
            BipedAnim* v3 = pactor->GetBiped();
            pweapon = pprocess->GetWeaponBone(v3);
        }
        if (((eLimb == BGSBodyPart::LIMB_WEAPON || aeDrawMode == FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) &&
             SingleTarget.pRef->IsActor() && (!bScanning || abQueryMode)) &&
            (pweapon) && (pprocess)) {
            BSShaderUtil::AccumulateScene(pcam, pweapon, *pculler);
        }

        if (eLimb != BGSBodyPart::LIMB_WEAPON || aeDrawMode == FOVATSEffectManager::VATS_DRAW_ENTIRE_TARGET) {
            bool bhidWeapon = false;
            if (pweapon) {
                bhidWeapon = !pweapon->GetAppCulled();
                pweapon->SetAppCulled(true);
            }
            BSShaderUtil::AccumulateScene(pcam, SingleTarget.spObj.m_pObject, *pculler);
            if (SingleTarget.pRef->IsActor()) {
                Actor* pactor_2 = static_cast<Actor*>(SingleTarget.pRef);
                TESActorBase* pActorBase = pactor_2->GetActorBase();
                BGSBodyPartData* pbodyPartData = pActorBase->GetBodyPartData();
                BGSBodyPart* ppart = pbodyPartData->GetBodyPart(eLimb);
                if (ppart) {
                    NiNode* pTargetRootNode = pactor_2->Get3D();
                    NiAVObject* plimbNode = pTargetRootNode->GetObjectByName(ppart->GetNodeName());
                    if (plimbNode) {
                        BSShaderUtil::AccumulateScene(pcam, plimbNode, *pculler);
                    }
                }
                if (iNumAdditionalLimbs > 0) {
                    for (int32_t i = 0; i < iNumAdditionalLimbs; ++i) {
                        TESActorBase* pBaseForm = pactor_2->GetActorBase();
                        BGSBodyPartData* pbodyPartData_2 = pBaseForm->GetBodyPartData();
                        BGSBodyPart* ppart_2 = pbodyPartData_2->GetBodyPart(pAdditionalLimbs[i]);
                        if (ppart_2) {
                            NiNode* v6 = pactor_2->Get3D();
                            NiAVObject* plimbNode_2 = v6->GetObjectByName(ppart_2->GetNodeName());
                            if (plimbNode_2) {
                                BSShaderUtil::AccumulateScene(pcam, plimbNode_2, *pculler);
                            }
                        }
                    }
                }
            }
            if ((pweapon) && (bhidWeapon)) {
                pweapon->SetAppCulled(false);
            }
        }
        BSShaderAccumulator* v4 = spAccumulator.m_pObject;
        BSShaderUtil::RenderScene(pcam, v4, false);
        pculler->SetAccumulator(nullptr);
        pculler->SetCullMode(BSCullingProcess::BSCP_CULL_NORMAL);
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
        TESObjectREFR* pRef = SingleTarget.pRef;
        BSCullingProcess* pCuller = pSceneGraph->GetCullingProcess();
        NiAccumulator* pAccumulator = spAccumulator.m_pObject;
        pCuller->SetAccumulator(pAccumulator);
        NiCamera* pCamera = pSceneGraph->GetCamera();
        NiRenderer* Renderer_ = NiRenderer::GetRenderer();
        Renderer_->SetCameraData(pCamera);
        pCuller->SetCamera(pCamera);
        const NiFrustum* ViewFrustum = &pCamera->GetViewFrustum();
        pCuller->SetFrustum(*ViewFrustum);
        bool bFlagsSet = false;
        NiAVObject* pRootObject = nullptr;
        for (int32_t i = 0; i < iNumMultiTargets; ++i) {
            ShadowSceneNode* shadowSceneNode = static_cast<ShadowSceneNode*>(pMultiTargets[i].spObj.m_pObject);
            if (i == iSelectedMultiTarget) {
                BSShaderProperty::SetFlagRecurse(shadowSceneNode, BSShaderProperty::BSSP_FLAG_VATS_SELECTION, 1);
                pRootObject = shadowSceneNode;
                bFlagsSet = true;
            }
            BSShaderUtil::AccumulateScene(pCamera, shadowSceneNode, *pCuller);
        }
        BSShaderAccumulator* v4 = spAccumulator.m_pObject;
        BSShaderUtil::RenderScene(pCamera, v4, false);
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
    float fX = afTime * gs_fPulseRate_VATS.Float();
    float fNoise = BSNoise1D::Noise(fX);
    float fPulse = fNoise + fNoise;
    fPulseCurrentIntensity = fPulse * gs_fPulseIntensity_VATS.Float();
    fPulseCurrentRadius = fPulse * gs_fPulseRadius_VATS.Float();
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
        NiNode* v2 = apTarget->Get3D();
        pMultiTargets[iNumMultiTargets].spObj = v2;
        ++iNumMultiTargets;
        if (!SingleTarget.pRef) {
            SingleTarget.pRef = apTarget;
            NiNode* v3 = apTarget->Get3D();
            SingleTarget.spObj = v3;
        }
        NiAVObject* v4 = pMultiTargets[iNumMultiTargets].spObj.m_pObject;
        BSShaderProperty::SetFlagRecurse(v4, BSShaderProperty::BSSP_FLAG_VATS_SELECTION, 0);
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
    fBurstStartTime = ((float)(&appTimer)->BSTimer::GetTime());
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
    SystemColorManager* Instance = SystemColorManager::GetInstance();
    Instance->GetColor(1, newcolor);
    ImageSpaceEffectInterface::eMode = 1;
    ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
    ImageSpaceManager_->GetEffect(ImageSpaceManager::EFFECT_INTERFACE)->bIsActive = true;
    ImageSpaceEffectInterface::spMaskTexture = static_cast<NiTexture*>(apSourceMask->GetTexture(0));
    if (bDistortActive) {
        ImageSpaceEffectInterface::fDistortVerticalScale = gs_fDistortVerticalScale_VATS.Float();
        ImageSpaceEffectInterface::fDistortHorizontalScale = gs_fDistortHorizontalScale_VATS.Float();
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
    ImageSpaceEffectInterface::fBlurIntensity = gs_fBlurIntensity_VATS.Float();
    ImageSpaceEffectInterface::fBlurRadius = gs_fBlurRadius_VATS.Float();
    ImageSpaceEffectInterface::fBlurIntensity = ImageSpaceEffectInterface::fBlurIntensity + fPulseCurrentIntensity;
    ImageSpaceEffectInterface::fBlurRadius = ImageSpaceEffectInterface::fBlurRadius + fPulseCurrentRadius;
    if (bBurstActive) {
        ImageSpaceEffectInterface::fBlurIntensity =
            (fBurstCurrentIntensity * 0.5 + 1.0f) * ImageSpaceEffectInterface::fBlurIntensity;
        ImageSpaceEffectInterface::fBlurRadius =
            (fBurstCurrentIntensity + 1.0f) * ImageSpaceEffectInterface::fBlurRadius;
    }
    ImageSpaceEffectInterface::fPulse = fPulseIntensity;
    ImageSpaceEffectInterface::fScanlineFrequency = gs_fScanlineFrequency_VATS.Float();
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
    SystemColorManager* Instance = SystemColorManager::GetInstance();
    Instance->GetColor(1, newcolor);
    ImageSpaceEffectVATSScan::fScanVerticalPosition = fScanPosition;
    ImageSpaceEffectVATSScan::fScanVerticalScale = gs_fScanModeVerticalScale_VATS.Float();
    ImageSpaceEffectVATSScan::fScanlineFrequency = gs_fScanModeScanlineFrequency_VATS.Float();
    ImageSpaceEffectVATSScan::kTint = newcolor;
    ImageSpaceEffectVATSScan::spScanlinesTexture = spScanlinesTexture;
    ImageSpaceEffectVATSScan::spMaskTexture = static_cast<NiTexture*>(apSourceMask->GetTexture(0));
    ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
    ImageSpaceManager_->GetEffect(ImageSpaceManager::EFFECT_VATS_SCAN)->bIsActive = true;
}

bool FOVATSEffectManager::OcclusionEnabled() {
    return (bool)((uint8_t)bOcclusionEnabled);
}

void FOVATSEffectManager::DrawTargetToTexture(
    BSRenderedTexture* apTarget,
    FOVATSEffectManager::VATS_OCCLUSION_QUERY aQueryType,
    bool abQueryMode,
    FOVATSEffectManager::VATS_DRAW_MODE aeDrawMode,
    NiRenderer::ClearFlags aeClearMode,
    bool abStopRenderTarget
) {
    float args;
    if (!SingleTarget.pRef->IsDead(false)) {
        float fTargetAlpha;
        Actor* pactor;
        NiDX9Renderer* pr = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        NiColorA prevcolor(0.0f, 0.0f, 0.0f, 0.0f);
        pr->GetBackgroundColor(prevcolor);
        NiColorA prevcolor_2(0.0f, 0.0f, 0.0f, 0.0f);
        NiColorA* fA = &prevcolor_2;
        pr->SetBackgroundColorAlpha(*fA);
        if (BSRenderedTexture::QOutsideFrame()) {
            NiRenderTargetGroup* Group = apTarget->GetGroup();
            BSRenderedTexture::StartOffscreen(aeClearMode, Group);
        }
        Lighting30Shader::bAlwaysWriteAlpha = true;
        ShadowLightShader::bAlwaysWriteAlpha = true;
        bool bIsOcclusionOn = bOcclusionEnabled;
        bool bEndQuery = false;
        if (SingleTarget.pRef->IsActor()) {
            pactor = static_cast<Actor*>(SingleTarget.pRef);
        }
        else {
            pactor = nullptr;
        }
        if (pactor) {
            fTargetAlpha = pactor->GetAlpha();
        }
        else {
            fTargetAlpha = 1.0f;
        }
        ExtraRefractionProperty* pRPE = nullptr;
        args = 0.0f;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_DETECTION, pPlayer, &args);
        if ((pactor) && (args > 0.0)) {
            pRPE = (&pactor->m_Extra)->GetRefractionPropertyExtra();
        }

        bool bHasRefraction = false;
        float fRefractPower = 0.0f;
        if ((pRPE) && (pactor)) {
            fRefractPower = pRPE->fRefractionPower;
            bHasRefraction = true;
            pactor->SetRefraction(false, 0.0f);
        }

        if (bIsOcclusionOn) {
            if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_PARTIAL) {
                BSOcclusionQuery* v10;
                if (pPartialLimbQuery[eLimb]) {
                    BSOcclusionQuery* v17 = pPartialLimbQuery[eLimb];
                    if (v17) {
                        delete v17;
                    }
                }
                BSOcclusionQuery* v16 = static_cast<BSOcclusionQuery*>(ourMalloc(sizeof(BSOcclusionQuery)));
                if (v16) {
                    v10 = ::new (v16) BSOcclusionQuery;
                }
                else {
                    v10 = nullptr;
                }
                pPartialLimbQuery[eLimb] = v10;
                bEndQuery = true;
                pPartialLimbQuery[eLimb]->Begin();
            }
            else {
                if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_FULL) {
                    BSOcclusionQuery* pOcclusionQuery;
                    if (pEntireLimbQuery[eLimb]) {
                        BSOcclusionQuery* pPreviousQuery = pEntireLimbQuery[eLimb];
                        if (pPreviousQuery) {
                            delete pPreviousQuery;
                        }
                    }
                    BSOcclusionQuery* v14 = static_cast<BSOcclusionQuery*>(ourMalloc(sizeof(BSOcclusionQuery)));
                    if (v14) {
                        pOcclusionQuery = ::new (v14) BSOcclusionQuery;
                    }
                    else {
                        pOcclusionQuery = nullptr;
                    }
                    pEntireLimbQuery[eLimb] = pOcclusionQuery;
                    bEndQuery = true;
                    pEntireLimbQuery[eLimb]->Begin();
                }
            }
        }
        spAccumulator.m_pObject->SetRenderMode(BSShaderManager::BSSM_RENDER_CONSTALPHA);
        BSShaderManager::SetRenderMode(8u);
        DrawTarget(abQueryMode, aeDrawMode);
        BSShaderManager::SetRenderMode(0);
        spAccumulator.m_pObject->SetRenderMode(BSShaderManager::BSSM_RENDER_NORMAL);
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
            else {
                if (aQueryType == FOVATSEffectManager::VATS_OCCLUSION_QUERY_FULL) {
                    if (pEntireLimbQuery[eLimb]) {
                        pEntireLimbQuery[eLimb]->End();
                    }
                    else {
                        Warning("pEntireLimbQuery is NULL even though we just issued one.  This should be impossible.");
                    }
                }
            }
        }
        if (abStopRenderTarget) {
            BSRenderedTexture::StopOffscreen();
        }
        if ((bHasRefraction) && (pactor)) {
            pactor->SetRefraction(true, fRefractPower);
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
    if ((!Interface::GetIsVATSRescanning()) && bEffectEnabled) {
        if (bScanning) {
            PrepareScanEffect(apSourceColors, apSourceMask, apTarget, abClearTarget);
        }
        else {
            PrepareHighlightEffect(apSourceColors, apSourceMask, apTarget, abClearTarget);
        }
    }
}

// Fetches the manager and the effect again before each call, as the game does.
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
    return (bool)((uint8_t)bScanning);
}

void FOVATSEffectManager::DisableScan() {
    if (bScanning) {
        bExitScanMode = true;
    }
}

void FOVATSEffectManager::TriggerDistort() {
    bDistortActive = true;
    fDistortStartTime = ((float)(&appTimer)->BSTimer::GetTime());
    fDistortPosition = (float)0.0;
}

void FOVATSEffectManager::TriggerScan() {
    if (!bScanning) {
        bScanning = true;
        fScanStartTime = (-0x1p+0f);
        fScanPosition = (float)0.0;
    }
}

void FOVATSEffectManager::UpdateTargetPulse(float afTime) {
    float x = afTime * (&::fVATSTargetPulseRate)->Float();
    float v4 = (float)((std::sinf(x) + 1.0) * 0.5);
    fPulseIntensity = v4;
}

void FOVATSEffectManager::SetSelectedMultiTarget(TESObjectREFR* apTarget) {
    int32_t i;
    int32_t iIndex = (-1);
    for (i = 0; i < iNumMultiTargets; ++i) {
        if (pMultiTargets[i].pRef == apTarget) {
            iIndex = i;
        }
    }
    if (iIndex == (-1)) {
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
    float v2;
    float v3 = afTime - fBurstStartTime;
    if (!(fBurstDuration < ((double)v3))) {
        v2 = (float)(1.0 - (v3 / fBurstDuration));
        fBurstCurrentIntensity = (fBurstMaximumIntensity * v2);
    }
    else {
        bBurstActive = false;
    }
}

void FOVATSEffectManager::UpdateDistort(float afTime) {
    int32_t v2;
    float v4 = afTime - fDistortStartTime;
    fDistortPosition = (v4 / fDistortDuration);
    if (fDistortPosition > ((&gs_fDistortVerticalScale_VATS)->Float() + 1.0)) {
        bDistortActive = false;
        v2 = rt::rand();
        fNextDistortTime = (float)(((double)(v2 % ((int32_t)(&gs_iDistortMaxInterval_VATS)->UInt()))) + afTime);
    }
}

void FOVATSEffectManager::DisableOcclusion() {
    InitializationFlow("--==OCCLUSION DISABLED==--\n");
    bOcclusionEnabled = false;
}

void FOVATSEffectManager::UpdateScan(float afTime) {
    float* v2;
    float a3;
    if (fScanStartTime == (-1.0)) {
        fScanStartTime = afTime;
    }
    float v5 = afTime - fScanStartTime;
    fScanPosition = (v5 / (&gs_fScanModeDuration_VATS)->Float());
    if (bExitScanMode && (fScanPosition > ((&gs_fScanModeVerticalScale_VATS)->Float() + 1.0))) {
        bScanning = false;
        bExitScanMode = false;
        a3 = (&gs_fBurstIntensity_VATS)->Float();
        v2 = &gs_fBurstDuration_VATS.Float();
        TriggerBurst(*v2, a3, true);
    }
}

void FOVATSEffectManager::ShowBipedObjectOnActor(Actor* apActor, BIPED_OBJECT aeObject, bool abShow) {
    NiNode* pBoneNode;
    BipedAnim* pBipedAnim = apActor->GetBiped();
    if (pBipedAnim) {
        pBoneNode = static_cast<NiNode*>(pBipedAnim->GetBipObject(aeObject)->pPartClone);
        if (pBoneNode) {
            pBoneNode->SetAppCulled((uint8_t)(!abShow));
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
        Actor* pactor = static_cast<Actor*>(SingleTarget.pRef);
        int32_t ilimb = eLimb;
        NiNode* pactorRoot = SingleTarget.pRef->Get3D();
        for (uint32_t n = 0; n < pactorRoot->GetArrayCount(); ++n) {
            NiAVObject* pobj = pactorRoot->GetAt(n);
            if (pobj) {
                if (Animation::FindSkinnedNode(pobj)) {
                    NiNode* pnode = pobj->IsNode();
                    NiGeometry* v17 = pobj->IsGeometry();
                    uint32_t n_2 = 0;
                    while (v17 || (pnode && n_2 < pnode->GetArrayCount())) {
                        NiSkinInstance* pskin;
                        if (pnode) {
                            NiGeometry* ptri_2;
                            if (pnode->GetAt(n_2)) {
                                NiAVObject* pobj_2 = pnode->GetAt(n_2);
                                ptri_2 = pobj_2->IsGeometry();
                            }
                            else {
                                ptri_2 = nullptr;
                            }
                            v17 = ptri_2;
                            ++n_2;
                        }
                        if (v17) {
                            pskin = v17->GetSkinInstance();
                        }
                        else {
                            pskin = nullptr;
                        }
                        NiObject* pkObject = pskin;
                        BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                            NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pskin)
                        );
                        if (pdSkinInstance) {
                            for (BGSBodyPart::LIMB_ENUM eindex = BGSBodyPart::LIMB_TORSO;
                                 eindex < BGSBodyPart::LIMB_COUNT;
                                 eindex = static_cast<BGSBodyPart::LIMB_ENUM>(eindex + 1)) {
                                EnableBodyPartOnSkinInstance(pdSkinInstance, eindex, eindex == eLimb);
                                for (int32_t i = 0; i < iNumAdditionalLimbs; ++i) {
                                    if (eindex == pAdditionalLimbs[i]) {
                                        EnableBodyPartOnSkinInstance(pdSkinInstance, eindex, true);
                                    }
                                }
                            }
                        }
                        v17 = nullptr;
                    }
                }
                else {
                    pobj->SetAppCulled(true);
                }
            }
        }
        if ((pactor) && (eLimb == BGSBodyPart::LIMB_TORSO)) {
            for (BGSBodyPart::LIMB_ENUM eindex_2 = BGSBodyPart::LIMB_TORSO; eindex_2 < BGSBodyPart::LIMB_COUNT;
                 eindex_2 = static_cast<BGSBodyPart::LIMB_ENUM>(eindex_2 + 1)) {
                TESActorBase* pActorBase = pactor->GetActorBase();
                BGSBodyPartData* pbodyPartData = pActorBase->GetBodyPartData();
                BGSBodyPart* ppart = pbodyPartData->GetBodyPart(eLimb);
                if (ppart) {
                    NiNode* v2 = pactor->Get3D();
                    NiAVObject* plimbNode = v2->GetObjectByName(ppart->GetNodeName());
                    if (plimbNode && eindex_2 == eLimb) {
                        plimbNode->SetAppCulled(false);
                    }
                    else {
                        if (plimbNode) {
                            plimbNode->SetAppCulled(true);
                        }
                    }
                }
            }
        }

        if (eLimb != BGSBodyPart::LIMB_LEFT_ARM_1 && eLimb != BGSBodyPart::LIMB_LEFT_ARM_2) {
            ShowBipedObjectOnActor(pactor, BO_LEFT_HAND, false);
        }
        if (eLimb != BGSBodyPart::LIMB_RIGHT_ARM_1 && eLimb != BGSBodyPart::LIMB_RIGHT_ARM_2) {
            ShowBipedObjectOnActor(pactor, BO_RIGHT_HAND, false);
        }
        if (eLimb != BGSBodyPart::LIMB_HEAD_1 && eLimb != BGSBodyPart::LIMB_HEAD_2) {
            ShowBipedObjectOnActor(pactor, BO_HAT, false);
            ShowBipedObjectOnActor(pactor, BO_EYE_GLASSES, false);
            ShowBipedObjectOnActor(pactor, BO_NOSE_RING, false);
            ShowBipedObjectOnActor(pactor, BO_EARRINGS, false);
            ShowBipedObjectOnActor(pactor, BO_HEADBAND, false);
            ShowBipedObjectOnActor(pactor, BO_MASK, false);
            ShowBipedObjectOnActor(pactor, BO_CHOKER, false);
            ShowBipedObjectOnActor(pactor, BO_MOUTH_OBJECT, false);
            ShowBipedObjectOnActor(pactor, BO_HAIR, false);
            ShowBipedObjectOnActor(pactor, BO_HEAD, false);
        }
    }
}

void FOVATSEffectManager::EnableBodyPartOnSkinInstance(
    BSDismemberSkinInstance* apInstance,
    BGSBodyPart::LIMB_ENUM aeLimb,
    bool abEnable
) {
    apInstance->SetBodyPartEnabled(aeLimb, abEnable);
    apInstance->SetBodyPartEnabled(1000 * aeLimb, abEnable);
}

void FOVATSEffectManager::RestoreBodyParts() {
    if (SingleTarget.pRef->IsActor() && eLimb != BGSBodyPart::LIMB_WEAPON && eLimb != BGSBodyPart::LIMB_COUNT) {
        Actor* pactor = static_cast<Actor*>(SingleTarget.pRef);
        ShowBipedObjectOnActor(pactor, BO_LEFT_HAND, true);
        ShowBipedObjectOnActor(pactor, BO_RIGHT_HAND, true);
        ShowBipedObjectOnActor(pactor, BO_HAT, true);
        ShowBipedObjectOnActor(pactor, BO_EYE_GLASSES, true);
        ShowBipedObjectOnActor(pactor, BO_NOSE_RING, true);
        ShowBipedObjectOnActor(pactor, BO_EARRINGS, true);
        ShowBipedObjectOnActor(pactor, BO_HEADBAND, true);
        ShowBipedObjectOnActor(pactor, BO_MASK, true);
        ShowBipedObjectOnActor(pactor, BO_CHOKER, true);
        ShowBipedObjectOnActor(pactor, BO_MOUTH_OBJECT, true);
        ShowBipedObjectOnActor(pactor, BO_HAIR, true);
        ShowBipedObjectOnActor(pactor, BO_HEAD, true);
        int32_t ilimb = eLimb;
        NiNode* pactorRoot = SingleTarget.pRef->Get3D();
        for (uint32_t n = 0; n < pactorRoot->GetArrayCount(); ++n) {
            NiAVObject* pobj = pactorRoot->GetAt(n);
            if (pobj) {
                if (Animation::FindSkinnedNode(pobj)) {
                    NiNode* pnode = pobj->IsNode();
                    NiGeometry* v16 = pobj->IsGeometry();
                    uint32_t n_2 = 0;
                    while (v16 || (pnode && n_2 < pnode->GetArrayCount())) {
                        NiSkinInstance* pskin;
                        if (pnode) {
                            NiGeometry* ptri_2;
                            if (pnode->GetAt(n_2)) {
                                NiAVObject* pobj_2 = pnode->GetAt(n_2);
                                ptri_2 = pobj_2->IsGeometry();
                            }
                            else {
                                ptri_2 = nullptr;
                            }
                            v16 = ptri_2;
                            ++n_2;
                        }
                        if (v16) {
                            pskin = v16->GetSkinInstance();
                        }
                        else {
                            pskin = nullptr;
                        }
                        NiObject* pkObject = pskin;
                        BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                            NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pskin)
                        );
                        if (pdSkinInstance) {
                            for (BGSBodyPart::LIMB_ENUM eindex = BGSBodyPart::LIMB_TORSO;
                                 eindex < BGSBodyPart::LIMB_COUNT;
                                 eindex = static_cast<BGSBodyPart::LIMB_ENUM>(eindex + 1)) {
                                pdSkinInstance->SetBodyPartEnabled(eindex, true);
                                pdSkinInstance->SetBodyPartEnabled(1000 * eindex, true);
                                pdSkinInstance->SetBodyPartEnabled(eindex + 100, false);
                                pdSkinInstance->SetBodyPartEnabled(eindex + 200, false);
                            }
                        }
                        v16 = nullptr;
                    }
                }
                else {
                    pobj->SetAppCulled(false);
                }
            }
        }
        if ((pactor) && (eLimb == BGSBodyPart::LIMB_TORSO)) {
            for (BGSBodyPart::LIMB_ENUM eindex_2 = BGSBodyPart::LIMB_TORSO; eindex_2 < BGSBodyPart::LIMB_COUNT;
                 eindex_2 = static_cast<BGSBodyPart::LIMB_ENUM>(eindex_2 + 1)) {
                TESActorBase* ActorBase = pactor->GetActorBase();
                BGSBodyPartData* pbodyPartData = ActorBase->GetBodyPartData();
                BGSBodyPart* ppart = pbodyPartData->GetBodyPart(eLimb);
                if (ppart) {
                    NiNode* v2 = pactor->Get3D();
                    NiAVObject* plimbNode = v2->GetObjectByName(ppart->GetNodeName());
                    if (plimbNode) {
                        plimbNode->SetAppCulled(false);
                    }
                }
            }
        }
    }
}

void FOVATSEffectManager::FreeOcclusionQueries() {
    uint32_t a2;
    for (uint32_t i = 0; i < 15; ++i) {
        bWaitingForOcclusionQuery[i] = false;
        a2 = 0;
        if (pPartialLimbQuery[i]) {
            pPartialLimbQuery[i]->GetResult(&a2, true);
            if (pPartialLimbQuery[i]) {
                delete pPartialLimbQuery[i];
            }
            pPartialLimbQuery[i] = nullptr;
        }
        if (pEntireLimbQuery[i]) {
            pEntireLimbQuery[i]->GetResult(&a2, true);
            if (pEntireLimbQuery[i]) {
                delete pEntireLimbQuery[i];
            }
            pEntireLimbQuery[i] = nullptr;
        }
    }
}

void ImageSpaceEffect::SetActive(bool abActive) {
    bIsActive = abActive;
}
