#include "Fallout/Interface/FOPipboyManager.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderBloodSplatter.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"

// The game settings this file defines, with their IDs and default values, in the game's initialization order.
SettingT<INISettingCollection> bUsePipboyMode("bUsePipboyMode:Pipboy", true);
SettingT<INISettingCollection> bEnableFlickerPipboy("bEnableFlickerPipboy:Pipboy", true);
SettingT<INISettingCollection> fScanlineScalePipboy("fScanlineFrequencyPipboy:Pipboy", 100.0f);
SettingT<INISettingCollection> fBlurIntensityPipboy("fBlurIntensityPipboy:Pipboy", 1.0f);
SettingT<INISettingCollection> fBlurRadiusPipboy("fBlurRadiusPipboy:Pipboy", 2.0f);
SettingT<INISettingCollection> fTabKnobMoveRate("fTabKnobMoveRate:Pipboy", 0.0075f);
SettingT<INISettingCollection> fTabKnobMinPosition("fTabKnobMinPosition:Pipboy", -0.5f);
SettingT<INISettingCollection> fTabKnobMaxPosition("fTabKnobMaxPosition:Pipboy", 1.5f);
SettingT<INISettingCollection> fScrollKnobIncrement("fScrollKnobIncrement:Pipboy", -0.1f);
SettingT<INISettingCollection> fScrollKnobRate("fScrollKnobRate:Pipboy", 0.0015f);
SettingT<INISettingCollection> fLightEffectFadeDuration("fLightEffectFadeDuration:Pipboy", 800.0f);
SettingT<INISettingCollection> fLightEffectLightFadeDuration("fLightEffectLightFadeDuration:Pipboy", 500.0f);

FOPipboyManager::FOPipboyManager()
  : FORenderedMenu(), spPipboyRoot(nullptr), sp3rdPersonPipboyRoot(nullptr), sp3rdPersonScreen(nullptr),
    spPartialScreenGeometry(nullptr), spButtons(), spButtonGlow(), spKnobs(), bScaledTexCoords(false), bPipboyUp(false),
    pBackgroundHum(), b1stPersonLightOn(false), b3rdPersonLightOn(false), bLightEffectFading(false) {
    for (int32_t i = 0; i < 3; ++i) {
        fKnobPositions[i] = 0.0f;
        fKnobTargets[i] = 0.0f;
        fKnobRates[i] = 0.0f;
    }
    bUsePartialScreenGeometry = true;
}

FOPipboyManager::~FOPipboyManager() {
    if (Interface::IsInGameLoadingMenuOpen()) {
        Interface::HideMenus();
    }
}

bool FOPipboyManager::OnIdle() {
    if (!FORenderedMenu::OnIdle()) {
        return false;
    }
    float fTime = static_cast<float>(appTimer.BSTimer::GetTime());
    UpdateKnobs(fTime);
    UpdateIdleAnims();
    return true;
}

void FOPipboyManager::Initialize() {
    if (bUsePipboyMode.Bool() && !GetInitialized() && pPlayer && pPlayer->Get3D(true)) {
        NiGeometry* p3rdPersonScreenGeom = nullptr;
        NiGeometry* pPipboyScreen = nullptr;
        NiAVObject* p3D = pPlayer->Get3D(true);
        NiNode* pPipboyNode = static_cast<NiNode*>(
            NiObject::DynamicCastStatic(&NiNode::ms_RTTI, p3D->GetObjectByName("pipboyscreen"))
        );
        if (!pPipboyNode) {
            p3D = pPlayer->Get3D(true);
            NiFixedString kName0("pipboyscreen:0");
            NiAVObject* pObject = p3D->GetObjectByName(kName0);
            pPipboyNode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        }
        FORenderedMenu::Initialize();
        SetMenuRoot(InterfaceManager::Instance()->GetSceneGraph());
        if (pPipboyNode) {
            pPipboyScreen = static_cast<NiGeometry*>(pPipboyNode->GetAtChecked(0));
            spPipboyRoot = pPipboyNode->m_pkParent;
        }
        else {
            p3D = pPlayer->Get3D(true);
            NiAVObject* pObject = p3D->GetObjectByName("pipboyscreen:0");
            pPipboyScreen = static_cast<NiGeometry*>(NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pObject));
            if (pPipboyScreen) {
                spPipboyRoot = pPipboyScreen->m_pkParent;
            }
        }
        SetScreenParent(pPlayer->Get3D(true));
        if (pPipboyNode) {
            NiGeometry* pFirstChild = static_cast<NiGeometry*>(pPipboyNode->GetAtChecked(0));
            p3rdPersonScreenGeom = static_cast<NiGeometry*>(
                NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pFirstChild)
            );
            sp3rdPersonPipboyRoot = pPipboyNode->m_pkParent;
            sp3rdPersonScreen = static_cast<NiAVObject*>(p3rdPersonScreenGeom);
        }
        else if (pPipboyScreen) {
            p3rdPersonScreenGeom = pPipboyScreen;
            sp3rdPersonPipboyRoot = pPipboyScreen->m_pkParent;
            sp3rdPersonScreen = static_cast<NiAVObject*>(p3rdPersonScreenGeom);
        }
        if (pPipboyScreen && p3rdPersonScreenGeom) {
            PrepareGeometry(pPipboyScreen);
            PrepareGeometry(p3rdPersonScreenGeom);
            SetScreenGeometry(pPipboyScreen);
        }

        InitButtons();
        InitKnobs();
        InitLight();
        if (spPipboyRoot.m_pObject) {
            NiUpdateData UpdateData(0.0f, false, false);
            spPipboyRoot->Update(UpdateData);
            spPipboyRoot->UpdateProperties();
        }
        if (sp3rdPersonPipboyRoot.m_pObject) {
            NiUpdateData UpdateData(0.0f, false, false);
            sp3rdPersonPipboyRoot->Update(UpdateData);
            sp3rdPersonPipboyRoot->UpdateProperties();
        }
        NiColorA kNewColor(0.0f, 0.0f, 0.0f, 0.0f);
        if (SystemColorManager::GetInstance()->GetColor(4, kNewColor)) {
            SetBackgroundColor(kNewColor);
            SetTintColor(kNewColor);
        }
        SetBlurIntensity(fBlurIntensityPipboy.Float());
        SetBlurRadius(fBlurRadiusPipboy.Float());
        SetDoScanlines(true, fScanlineScalePipboy.Float());
        bPulseAffectsLight = 1;
        ShowPipboyLightEffect(false, false, true);
        ShowPipboyLightEffect(true, false, true);
    }
}

void FOPipboyManager::Shutdown() {
    int32_t i;
    spPipboyRoot = nullptr;
    sp3rdPersonPipboyRoot = nullptr;
    spPartialScreenGeometry = nullptr;
    for (i = 0; i < 3; ++i) {
        spButtons[i] = nullptr;
        spButtonGlow[i] = nullptr;
        spKnobs[i] = nullptr;
        fKnobPositions[i] = 0.0f;
        fKnobTargets[i] = 0.0f;
        fKnobRates[i] = 0.0f;
    }
    sp3rdPersonScreen = nullptr;
    FORenderedMenu::Shutdown();
}

void FOPipboyManager::OnMenuOpen() {
    if (GetInitialized()) {
        SetRefractive(false);
        ResetColors();
        Interface::bStartPipboyRenderBackground = true;
        FORenderedMenu::bSoundsActive = 1;
        Interface::IsolateMenuElements(true, true);
        pBackgroundHum.Stop();
        pBackgroundHum.Release();
        pBackgroundHum = BSAudio::QInstance().GetSoundHandleByName("UIPipBoyHumLP", 49u);
        pBackgroundHum.Play(false);
        float fDefaultBurstIntensity = ::fDefaultBurstIntensity.Float();
        TriggerBurst(fDefaultBurstDuration.Float(), fDefaultBurstIntensity, 0.5f);
        FORenderedMenu::OnMenuOpen();
        ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier::GetGetHit());
        TESImageSpaceModifier::AllowGetHit(false);
        BSShaderBloodSplatter::SetGlobalAlpha(0.0f);
    }
}

void FOPipboyManager::OnMenuClose() {
    bool bRefractive = pPlayer->IsRefractive();
    SetRefractive(bRefractive);
    Animation* pAnim = pPlayer->GetAnimation(true);
    if (pAnim) {
        if (!pAnim->SpecialIdleDonePlaying()) {
            pAnim->ClearGroup(AGS_SPECIAL_IDLE, 0.0f);
        }
        uint16_t usIdleGroup = pPlayer->GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr);
        pAnim->PlayGroup(usIdleGroup, ACTION_START, -1, AGS_NONE);
        usIdleGroup = pPlayer->GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr);
        Animation* p3rdPersonAnim = pPlayer->GetAnimation(false);
        p3rdPersonAnim->PlayGroup(usIdleGroup, ACTION_START, -1, AGS_NONE);
        if (pPlayer->GetSitSleepState()) {
            pAnim->PlayGroup(0, ACTION_START, -1, AGS_IDLE);
        }
    }
    if (GetInitialized()) {
        BSAudio::QInstance().GetSoundHandleByName("UIPipBoyAccessDown", 289u).Play(false);
        pBackgroundHum.Stop();
        pBackgroundHum.Release();
        ActivateButton(0);
        Interface::RestoreMenuElements();
        FORenderedMenu::OnMenuClose();
        FORenderedMenu::bSoundsActive = 0;
        TESImageSpaceModifier::AllowGetHit(true);
    }
}

void FOPipboyManager::UpdateKnobs(float afTime) {
    bool bKnobMoved[3];
    float fElapsed = afTime - fLastPipboyKnobUpdateTime;
    bKnobMoved[0] = false;
    bKnobMoved[1] = false;
    bKnobMoved[2] = false;
    for (int32_t i = 0; i < 3; ++i) {
        if (fKnobPositions[i] < static_cast<double>(fKnobTargets[i])) {
            fKnobPositions[i] = fKnobRates[i] * fElapsed + fKnobPositions[i];
            if (fKnobTargets[i] < static_cast<double>(fKnobPositions[i])) {
                fKnobPositions[i] = fKnobTargets[i];
            }
            bKnobMoved[i] = true;
        }
        if (fKnobPositions[i] > static_cast<double>(fKnobTargets[i])) {
            fKnobPositions[i] = fKnobPositions[i] - fKnobRates[i] * fElapsed;
            if (fKnobTargets[i] > static_cast<double>(fKnobPositions[i])) {
                fKnobPositions[i] = fKnobTargets[i];
            }
            bKnobMoved[i] = true;
        }
    }
    if (spKnobs[0].m_pObject && bKnobMoved[0]) {
        float fAngle = fKnobPositions[0];
        spKnobs[0]->SetRotate(fAngle, static_cast<float>(1.0), static_cast<float>(0.0), static_cast<float>(0.0));
        NiUpdateData kUpdateData(0.0, false, false);
        spKnobs[0]->Update(kUpdateData);
    }

    if (spKnobs[1].m_pObject && bKnobMoved[1]) {
        float fAngle = fKnobPositions[1];
        spKnobs[1]->SetRotate(fAngle, static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(1.0));
        NiUpdateData kUpdateData(0.0, false, false);
        spKnobs[1]->Update(kUpdateData);
    }

    if (spKnobs[2].m_pObject && bKnobMoved[2]) {
        float fAngle = fKnobPositions[2];
        spKnobs[2]->SetRotate(fAngle, static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(1.0));
        NiUpdateData kUpdateData(0.0, false, false);
        spKnobs[2]->Update(kUpdateData);
    }

    fLastPipboyKnobUpdateTime = afTime;
}

void FOPipboyManager::SetRefractive(bool abRefractive) {
    if (GetScreenGeometry() && Get3rdPersonScreen()) {
        if (abRefractive) {
            GetScreenGeometry()->SetAppCulled(true);
            Get3rdPersonScreen()->SetAppCulled(true);
        }
        else {
            GetScreenGeometry()->SetAppCulled(false);
            Get3rdPersonScreen()->SetAppCulled(false);
        }
    }
}

void FOPipboyManager::SetVerticalHoldParams(float afParam1, float afParam2) {
    bVertHoldActive = true;
    fVertHoldDuration = afParam1;
    fVertHoldSpeed = afParam2 * 0.0010000000474974513;
    fVertHoldStartTime = static_cast<float>(appTimer.iLastTime);
    fVertHoldCurrentPosition = 0.0f;
}

uint32_t FOPipboyManager::SetShudderHoldParams(float afParam1, float afParam2, float afParam3) {
    bShudderActive = true;
    fShudderDuration = afParam1;
    fShudderIntensity = afParam2;
    fShudderFrequency = afParam3;
    uint32_t uiTime = appTimer.BSTimer::GetTime();
    fShudderStartTime = static_cast<float>(uiTime);
    fShudderCurrentPosition = 0.0f;
    return uiTime;
}

// The chance sum is kept unrounded (double) before the compare. Each effect's settings are read last argument first.
void FOPipboyManager::TriggerRandomEffect() {
    float fRandom = RandomFloat0To1();
    if (fVertHoldChance.Float() > fRandom) {
        float fSpeed = fDefaultVertHoldSpeed.Float();
        float fDuration = fDefaultVertHoldDuration.Float();
        InterfaceManager::Instance()->GetPipboy()->SetVerticalHoldParams(fDuration, fSpeed);
    }
    if (fVertHoldChance.Float() <= fRandom) {
        if (fRandom < static_cast<double>(fVertHoldChance.Float()) + static_cast<double>(fShudderChance.Float())) {
            float fFrequency = fDefaultShudderFrequency.Float();
            float fIntensity = fDefaultShudderIntensity.Float();
            float fDuration = fDefaultShudderDuration.Float();
            InterfaceManager::Instance()->GetPipboy()->SetShudderHoldParams(fDuration, fIntensity, fFrequency);
        }
    }
    float fIntensity = fDefaultBurstIntensity.Float();
    float fDuration = fDefaultBurstDuration.Float();
    InterfaceManager::Instance()->GetPipboy()->TriggerBurst(fDuration, fIntensity, 0.5f);
}

void FOPipboyManager::FadeLightEffect(bool abFadeOut) {
    if (b1stPersonLightOn) {
        fLightEffectFadeStartTime = static_cast<float>(appTimer.BSTimer::GetTime());
        bLightEffectFading = true;
        if (abFadeOut) {
            fLightEffectFadeTarget = 0.0f;
        }
        else {
            fLightEffectFadeTarget = 1.0f;
        }
    }
}

void FOPipboyManager::MoveKnobAbsolute(Knobs aiKnob, float afTarget, float afRate) {
    this->fKnobTargets[aiKnob] = afTarget;
    this->fKnobRates[aiKnob] = afRate;
}

void FOPipboyManager::ActivateButton(const uint32_t aiButton) {
    if (!this->GetInitialized()) {
        return;
    }
    if (this->spButtonGlow[0]) {
        this->spButtonGlow[0]->SetAppCulled(true);
    }
    if (this->spButtonGlow[1]) {
        this->spButtonGlow[1]->SetAppCulled(true);
    }
    if (this->spButtonGlow[2]) {
        this->spButtonGlow[2]->SetAppCulled(true);
    }
    // The buttons count from 1.
    if (aiButton > 0 && this->spButtonGlow[aiButton - 1]) {
        this->spButtonGlow[aiButton - 1]->SetAppCulled(false);
    }
}

// ReInitialize is the public wrapper PlayerCharacter::PositionPlayer calls, and on PC it only re-creates the light.
void FOPipboyManager::ReInitialize() {
    this->InitLight();
}

void FOPipboyManager::ShowPipboyLightEffect(const bool ab3rdPerson, const bool abShow, const bool abUpdateVariable) {
    NiPointer<NiNode>* pspRoot = ab3rdPerson ? &this->sp3rdPersonPipboyRoot : &this->spPipboyRoot;
    NiNode* pRoot = *pspRoot;
    if (pRoot) {
        NiFixedString kName("PipboyLightEffect");
        NiAVObject* pLightEffect = pRoot->GetObjectByName(kName);
        if (!pLightEffect) {
            NiFixedString kName0("PipboyLightEffect:0");
            pLightEffect = pRoot->GetObjectByName(kName0);
        }
        if (pLightEffect) {
            pLightEffect->SetAppCulled(!abShow);
            if (abUpdateVariable) {
                // A third-person change is recorded in b1stPersonLightOn and a first-person one in b3rdPersonLightOn,
                // deliberately.
                if (ab3rdPerson) {
                    this->b1stPersonLightOn = abShow;
                }
                else {
                    this->b3rdPersonLightOn = abShow;
                    NiNode* pNode = pLightEffect->IsNode();
                    if (pNode && pNode->GetChildCount()) {
                        NiGeometry* pGeometry = pNode->GetAt(0)->IsGeometry();
                        if (pGeometry) {
                            NiProperty* pMaterial = pGeometry->GetProperty(NiMaterialProperty::GetType());
                            if (pMaterial) {
                                static_cast<NiMaterialProperty*>(pMaterial)->SetAlpha(static_cast<float>(abShow));
                            }
                            else {
                                NiProperty* pShade = pGeometry->GetProperty(NiShadeProperty::GetType());
                                static_cast<BSShaderProperty*>(pShade)->SetAlpha(static_cast<float>(abShow));
                            }
                            pGeometry->UpdateProperties();
                        }
                    }
                }
            }
        }
    }
    else if (abUpdateVariable) {
        if (ab3rdPerson) {
            this->b1stPersonLightOn = abShow;
        }
        else {
            this->b3rdPersonLightOn = abShow;
        }
    }
}

void FOPipboyManager::UpdateLightEffect(float afTime) {
    if (bLightEffectFading) {
        FadeLight(afTime);
        ShowPipboyLightEffect(false, true, false);
        NiAVObject* pRoot = spPipboyRoot.m_pObject;
        float fDelta = afTime - fLightEffectFadeStartTime;
        float fRatio = fDelta / fLightEffectFadeDuration.Float();
        float fFade = BSMIN(fRatio, 1.0f);
        fFade = BSMAX(0.0, fFade);
        if (fLightEffectFadeTarget == 0.0) {
            fFade = 1.0 - fFade;
        }
        NiAVObject* pLightEffect = pRoot->GetObjectByName("PipboyLightEffect");
        if (!pLightEffect) {
            NiFixedString kName0("PipboyLightEffect:0");
            pLightEffect = pRoot->GetObjectByName(kName0);
        }
        NiNode* pLightNode = nullptr;
        if (pLightEffect) {
            pLightNode = pLightEffect->IsNode();
        }
        if (pLightNode && pLightNode->GetChildCount()) {
            NiGeometry* pEffect = pLightNode->GetAt(0)->IsGeometry();
            if (pEffect) {
                int32_t iType = NiMaterialProperty::GetType();
                NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(pEffect->GetProperty(iType));
                if (pMaterial) {
                    pMaterial->SetAlpha(fFade);
                }
                else {
                    iType = NiShadeProperty::GetType();
                    BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(pEffect->GetProperty(iType));
                    pShaderProp->SetAlpha(fFade);
                }
                pEffect->UpdateProperties();
            }
        }

        if (fLightEffectFadeTarget == fFade) {
            bLightEffectFading = false;
            if (fLightEffectFadeTarget == 0.0) {
                ShowPipboyLightEffect(false, false, false);
            }
        }
    }
}

void FOPipboyManager::ResetPipBoyManager() {
    pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_VERY_LOW);
    BaseProcess* pProcess = pPlayer->pCurrentProcess;
    pProcess->ResolveEquipmentQueue(pPlayer);
    pProcess->Set3DUpdateFlag(RESET_MODEL);
    pProcess->Update3dModel(pPlayer);
    Shutdown();
    Initialize();
    SetPipBoyManagerReset(false);
}

void FOPipboyManager::SetTabKnobPosition(uint32_t aiNewPos, bool abPlaySounds) {
    if (fTabKnobPositions[aiNewPos] != fKnobTargets[2]) {
        fKnobTargets[2] = fTabKnobPositions[aiNewPos];
        fKnobRates[2] = fTabKnobMoveRate.Float();
        if (FORenderedMenu::bSoundsActive && abPlaySounds) {
            BSSoundHandle soundHand;
            soundHand = BSAudio::QInstance().GetSoundHandleByName("UIPipBoyTab", 289u);
            soundHand.Play(false);
        }
    }
}

NiAVObject* FOPipboyManager::Get3rdPersonScreen() {
    return sp3rdPersonScreen;
}

NiAVObject* FOPipboyManager::Get3rdPersonPipboyRoot() {
    return sp3rdPersonPipboyRoot.m_pObject;
}

void FOPipboyManager::MoveKnob(FOPipboyManager::Knobs aiKnob, float afAmount, float afRate, bool abPlaySound) {
    if (abPlaySound && aiKnob == FOPipboyManager::KNOB_SCROLL) {
        BSSoundHandle soundHand;
        soundHand = BSAudio::QInstance().GetSoundHandleByName("UIPipBoyScroll", 289u);
        soundHand.Play(false);
    }
    fKnobTargets[aiKnob] = fKnobTargets[aiKnob] + afAmount;
    fKnobRates[aiKnob] = afRate;
}

void FOPipboyManager::InitLight() {
    if (!this->CreateMenuLight("Omni01", nullptr)) {
        this->CreateMenuLight("Omni01:0", nullptr);
    }
    NiPointLight* pLight = this->GetMenuLight();
    if (pLight) {
        pLight->SetDiffuseColor(NiColor(0.0f, 0.0f, 0.0f));
        pLight->SetAmbientColor(NiColor(0.0f, 0.0f, 0.0f));
        pLight->SetConstantAttenuation(0.0f);
        pLight->SetLinearAttenuation(0.0f);
        pLight->SetQuadraticAttenuation(0.0f);
        pLight->SetDimmer(1.0f);
        // The light radius lives in the specular colour's first channel.
        pLight->SetSpecularColor(NiColor(fScreenLightRadius.Float(), 0.0f, 0.0f));
        alignas(NiColorA) uint8_t aucColor[sizeof(NiColorA)];
        NiColorA& kColor = *::new (aucColor) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
        // 4 is the pip-boy colour's id.
        SystemColorManager::GetInstance()->GetColor(4, kColor);
        NiColor kScreenColor(kColor.r, kColor.g, kColor.b);
        this->SetScreenLightColor(kScreenColor);
        this->SetScreenLightBaseIntensity(1.0f);
        this->SetScreenLightFlickerIntensity(1.0f);
    }
}

// Finds the three buttons (the first child of each PipBoyButton0N node) and the three button glows on the first-person
// model, trying each name and then its ":0" variant, then shows no glow.
void FOPipboyManager::InitButtons() {
    NiAVObject* p3D = pPlayer->Get3D(true);
    NiFixedString kName("PipBoyButton01");
    NiAVObject* pFound = p3D->GetObjectByName(kName);
    if (!pFound) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kName0("PipBoyButton01:0");
        pFound = p3D->GetObjectByName(kName0);
    }
    if (pFound) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kButton1("PipBoyButton01");
        NiAVObject* pObject = p3D->GetObjectByName(kButton1);
        NiNode* pButton;
        pButton = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        if (!pButton) {
            p3D = pPlayer->Get3D(true);
            NiFixedString kButton10("PipBoyButton01:0");
            pObject = p3D->GetObjectByName(kButton10);
            pButton = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        }
        this->spButtons[0] = pButton->GetAtChecked(0);
        p3D = pPlayer->Get3D(true);
        NiFixedString kButton2("PipBoyButton02");
        pObject = p3D->GetObjectByName(kButton2);
        pButton = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        if (!pButton) {
            p3D = pPlayer->Get3D(true);
            NiFixedString kButton20("PipBoyButton02:0");
            pObject = p3D->GetObjectByName(kButton20);
            pButton = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        }
        this->spButtons[1] = pButton->GetAtChecked(0);
        p3D = pPlayer->Get3D(true);
        NiFixedString kButton3("PipBoyButton03");
        pObject = p3D->GetObjectByName(kButton3);
        pButton = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        if (!pButton) {
            p3D = pPlayer->Get3D(true);
            NiFixedString kButton30("PipBoyButton03:0");
            pObject = p3D->GetObjectByName(kButton30);
            pButton = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, pObject));
        }
        this->spButtons[2] = pButton->GetAtChecked(0);
    }
    p3D = pPlayer->Get3D(true);
    NiFixedString kStats("StatsGlow");
    pFound = p3D->GetObjectByName(kStats);
    if (!pFound) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kStats0("StatsGlow:0");
        pFound = p3D->GetObjectByName(kStats0);
    }
    if (!pFound) {
        return;
    }
    p3D = pPlayer->Get3D(true);
    NiFixedString kGlow1("StatsGlow");
    this->spButtonGlow[0] = p3D->GetObjectByName(kGlow1);
    if (!this->spButtonGlow[0]) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kGlow10("StatsGlow:0");
        this->spButtonGlow[0] = p3D->GetObjectByName(kGlow10);
    }
    p3D = pPlayer->Get3D(true);
    NiFixedString kGlow2("ItemsGlow");
    this->spButtonGlow[1] = p3D->GetObjectByName(kGlow2);
    if (!this->spButtonGlow[1]) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kGlow20("ItemsGlow:0");
        this->spButtonGlow[1] = p3D->GetObjectByName(kGlow20);
    }
    p3D = pPlayer->Get3D(true);
    NiFixedString kGlow3("DataGlow");
    this->spButtonGlow[2] = p3D->GetObjectByName(kGlow3);
    if (!this->spButtonGlow[2]) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kGlow30("DataGlow:0");
        this->spButtonGlow[2] = p3D->GetObjectByName(kGlow30);
    }
    this->ActivateButton(0);
}

void FOPipboyManager::FadeLight(float afTime) {
    float fElapsed = afTime - this->fLightEffectFadeStartTime;
    float fFade = fElapsed / fLightEffectLightFadeDuration.Float();
    fFade = BSMIN(fFade, 1.0f);
    fFade = BSMAX(0.0f, fFade);
    if (this->fLightEffectFadeTarget == 0.0) {
        fFade = 1.0f - fFade;
    }
    LightRefData* pLight = pPlayer->GetSpellEffectLight();
    if (pLight && pLight->spLight) {
        pLight->spLight->SetDimmer(fFade);
    }
}

void FOPipboyManager::ResetColors() {
    NiColorA kColor(static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(0.0));
    if (SystemColorManager::GetInstance()->GetColor(4, kColor)) {
        SetBackgroundColor(kColor);
        SetTintColor(kColor);
        InitLight();
    }
}

char FOPipboyManager::CheckOrangeButtonsClicked(
    char acPressed,
    char acDepressed,
    float* apX,
    float* apY,
    uint32_t* apMenuToSet
) {
    char cOverScreen = 0;
    InterfaceManager* pManager = InterfaceManager::Instance();
    if (!GetScreenGeometry() || !spButtons[0].m_pObject || !spButtons[1].m_pObject || !spButtons[2].m_pObject) {
        return 0;
    }
    if (InterfaceManager::IsInPipboyMenu() && InterfaceManager::IsPipboyMenuTopmost() && spPipboyRoot.m_pObject) {
        NiPoint3 kDir;
        NiPoint3 kOrigin;
        NiPick kPick(0, 8u);
        kPick.SetObserveAppCullFlag(true);
        kPick.SetTarget(GetScreenGeometry());
        kPick.m_eSortType = NiPick::SORT;
        kPick.SetIsReturnTexture(true);
        NiCamera* pCamera = Main::GetWorldSceneGraph()->GetCamera();
        int32_t iCursorY = static_cast<int32_t>(pManager->sRealCursorPos.z);
        double dCursorPosX = pManager->sRealCursorPos.x;
        pCamera->WindowPointToRay(static_cast<int32_t>(dCursorPosX), iCursorY, kOrigin, kDir, nullptr);
        if (kPick.PickObjects(kOrigin, kDir, false)) {
            NiPick::Record* pRecord = kPick.m_pickResults.GetAt(0);
            if (pRecord) {
                NiGeometry* pGeometry = static_cast<NiGeometry*>(pRecord->GetAVObject());
                if (pGeometry == GetScreenGeometry()) {
                    float* pTexture = &pRecord->m_texture.x;
                    float fU = pTexture[0];
                    float fV = pTexture[1];
                    float fScreenHeight = InterfaceManager::GetRealScreenHeight();
                    float fScreenWidth = fScreenHeight * 1.3333330154418945;
                    *apX = fU * fScreenWidth;
                    *apY = fV * fScreenHeight;
                    cOverScreen = 1;
                }
            }
        }
        if (acPressed || acDepressed) {
            int32_t iReleasedButton = -1;
            kPick.m_eSortType = NiPick::SORT;
            kPick.SetIsReturnTexture(false);
            for (int32_t i = 0; i < 3; ++i) {
                kPick.SetTarget(spButtons[i].m_pObject);
                iCursorY = static_cast<int32_t>(pManager->sRealCursorPos.z);
                int32_t iCursorX = static_cast<int32_t>(pManager->sRealCursorPos.x);
                Main::GetWorldSceneGraph()->GetCamera()->WindowPointToRay(iCursorX, iCursorY, kOrigin, kDir, nullptr);
                if (kPick.PickObjects(kOrigin, kDir, false) && kPick.m_pickResults.GetAt(0)) {
                    if (acPressed) {
                        iClickedOrangeButton = i;
                    }
                    else {
                        iReleasedButton = i;
                    }
                }
            }
            if (acDepressed && iReleasedButton == iClickedOrangeButton) {
                *apMenuToSet = iReleasedButton + 1;
                iClickedOrangeButton = -1;
            }
        }
    }

    return cOverScreen;
}

void FOPipboyManager::InitKnobs() {
    double dRadAngle;
    NiMatrix3 kRotate;
    NiMatrix3 kParentRotate;
    NiAVObject* p3D = pPlayer->Get3D(true);
    NiAVObject* pGeometry = p3D->GetObjectByName("ScrollKnob");
    if (!pGeometry) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kName0("ScrollKnob:0");
        pGeometry = p3D->GetObjectByName(kName0);
    }
    if (pGeometry && pGeometry->IsNode()) {
        NiNode* pNode = pGeometry->IsNode();
        *spKnobs = static_cast<NiGeometry*>(pNode->GetAt(0));
    }

    p3D = pPlayer->Get3D(true);
    spKnobs[1] = static_cast<NiGeometry*>(p3D->GetObjectByName("RadNeedle"));
    if (!spKnobs[1].m_pObject) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kName0("RadNeedle:0");
        spKnobs[1] = static_cast<NiGeometry*>(p3D->GetObjectByName(kName0));
    }
    p3D = pPlayer->Get3D(true);
    spKnobs[2] = static_cast<NiGeometry*>(p3D->GetObjectByName("TabKnob"));
    if (!spKnobs[2].m_pObject) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kName0("TabKnob:0");
        spKnobs[2] = static_cast<NiGeometry*>(p3D->GetObjectByName(kName0));
    }
    fTabKnobPositions[0] = fTabKnobMinPosition.Float();
    fTabKnobPositions[4] = fTabKnobMaxPosition.Float();
    float fTabKnobIncrement = (fTabKnobPositions[4] - fTabKnobPositions[0]) / 4.0;
    for (uint32_t i = 1; i < 4; ++i) {
        fTabKnobPositions[i] = fTabKnobPositions[i - 1] + fTabKnobIncrement;
    }
    fKnobPositions[2] = fTabKnobPositions[0];
    fKnobTargets[2] = fTabKnobPositions[0];
    fKnobRates[2] = 0.0f;
    if (pPlayer) {
        double dRads = pPlayer->GetActorFloatValue(ActorValue::RADIATION_RADS);
        dRadAngle = 1.5707963267948966 - dRads / StatsMenu::GetRadMeterMax() * 3.1415927410125732;
    }
    else {
        dRadAngle = 0.0;
    }
    fKnobTargets[1] = dRadAngle;
    fKnobPositions[1] = fKnobTargets[1];
    fKnobRates[1] = 0.0f;
    if (spKnobs[0].m_pObject) {
        kRotate = (*&spKnobs[0]->m_kLocal).m_Rotate;
        kParentRotate = (*&spKnobs[0]->m_pkParent->m_kLocal).m_Rotate;
        spKnobs[0]->m_pkParent->SetRotate(kParentRotate * kRotate);
        spKnobs[0]->SetRotate(fKnobPositions[0], 1.0f, 0.0f, 0.0f);
    }
    if (spKnobs[1].m_pObject) {
        spKnobs[1]->SetRotate(fKnobPositions[1], 0.0f, 0.0f, 1.0f);
    }
    if (spKnobs[2].m_pObject) {
        spKnobs[2]->SetRotate(fKnobPositions[2], 0.0f, 0.0f, 1.0f);
    }
}

void FOPipboyManager::UpdateIdleAnims() {
    if (!Interface::IsConsoleVisible() && InterfaceManager::Instance()->ePipboyState == 3) {
        if (InterfaceManager::Instance()->GetCurrentMode() != 4) {
            Animation* pAnimation = pPlayer->GetAnimation(true);
            BSAnimGroupSequence* pCurrentSequence = pAnimation->GetCurrentSequence(AGS_IDLE);
            if (pCurrentSequence) {
                TESAnimGroup* pAnimGroup = pCurrentSequence->GetAnimGroup();
                if (pAnimGroup->GetType() != 1 && pAnimation->GroupLoaded(1u)) {
                    pAnimation->PlayGroup(1u, ACTION_START, -1, AGS_NONE);
                }
            }
            pAnimation->SkipUpdate(AGS_SKIP_ALL_BUT_SPECIAL_IDLE);
            float fSecondsPassed = appTimer.GetSeconds();
            pAnimation->Update(pPlayer, fSecondsPassed, -0x1p+0f);
            pAnimation->UpdateMovement(pPlayer);
        }
    }
}

void FORenderedMenu::SetBackgroundColor(const NiColorA& aColor) {
    this->BackgroundColor = aColor;
}

void FORenderedMenu::SetBlurIntensity(const float afIntensity) {
    this->fBlurIntensity = afIntensity;
}
