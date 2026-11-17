#include "Fallout/Interface/FOPipboyManager.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
SettingT<INISettingCollection> gs_fScanlineFrequencyPipboy_Pipboy("fScanlineFrequencyPipboy:Pipboy", 100.0f);
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
    float fTime = static_cast<float>((&appTimer)->BSTimer::GetTime());
    UpdateKnobs(fTime);
    UpdateIdleAnims();
    return true;
}

void FOPipboyManager::Initialize() {
    FNVD_INLINED_LOCAL(NiFixedString, v15, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    if (bUsePipboyMode.Bool() && !GetInitialized()) {
        if (pPlayer) {
            if (pPlayer->Get3D(true)) {
                NiGeometry* p3rdPersonScreenGeom = nullptr;
                NiGeometry* pPipboyScreen = nullptr;
                NiNode* v23 = nullptr;
                NiAVObject* v13 = pPlayer->Get3D(true);
                NiFixedString* v12 = ::new (static_cast<void*>(&apString)) NiFixedString("pipboyscreen");
                NiAVObject* v1 = v13->GetObjectByName(*v12);
                NiNode* pPipboyNode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, v1));
                apString.~NiFixedString();
                if (!pPipboyNode) {
                    NiAVObject* v11 = pPlayer->Get3D(true);
                    NiFixedString v16("pipboyscreen:0");
                    NiFixedString* v10 = &v16;
                    NiAVObject* v2 = v11->GetObjectByName(*v10);
                    pPipboyNode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, v2));
                }
                FORenderedMenu::Initialize();
                SetMenuRoot(InterfaceManager::Instance()->GetSceneGraph());
                if (pPipboyNode) {
                    pPipboyScreen = static_cast<NiGeometry*>(pPipboyNode->GetAtChecked(0));
                    spPipboyRoot.operator=(pPipboyNode->m_pkParent);
                }
                else {
                    NiAVObject* v9 = pPlayer->Get3D(true);
                    NiFixedString* v8 = ::new (static_cast<void*>(&v15)) NiFixedString("pipboyscreen:0");
                    NiAVObject* v3 = v9->GetObjectByName(*v8);
                    pPipboyScreen = static_cast<NiGeometry*>(NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, v3));
                    v15.~NiFixedString();
                    if (pPipboyScreen) {
                        spPipboyRoot.operator=(pPipboyScreen->m_pkParent);
                    }
                }
                SetScreenParent(pPlayer->Get3D(true));
                if (pPipboyNode) {
                    NiGeometry* pPipboyScreen_2 = static_cast<NiGeometry*>(pPipboyNode->GetAtChecked(0));
                    p3rdPersonScreenGeom =
                        static_cast<NiGeometry*>(NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pPipboyScreen_2));
                    sp3rdPersonPipboyRoot.operator=(pPipboyNode->m_pkParent);
                    sp3rdPersonScreen.operator=(static_cast<NiAVObject*>(p3rdPersonScreenGeom));
                }
                else {
                    if (pPipboyScreen) {
                        p3rdPersonScreenGeom = pPipboyScreen;
                        sp3rdPersonPipboyRoot.operator=(pPipboyScreen->m_pkParent);
                        sp3rdPersonScreen.operator=(static_cast<NiAVObject*>(p3rdPersonScreenGeom));
                    }
                }
                if (pPipboyScreen) {
                    if (p3rdPersonScreenGeom) {
                        PrepareGeometry(pPipboyScreen);
                        PrepareGeometry(p3rdPersonScreenGeom);
                        SetScreenGeometry(pPipboyScreen);
                    }
                }
                InitButtons();
                InitKnobs();
                InitLight();
                if (spPipboyRoot.m_pObject) {
                    NiUpdateData UpdateData(0.0f, false, false);
                    spPipboyRoot.m_pObject->Update(UpdateData);
                    spPipboyRoot.m_pObject->UpdateProperties();
                }
                if (sp3rdPersonPipboyRoot.m_pObject) {
                    NiUpdateData UpdateData_2(0.0f, false, false);
                    sp3rdPersonPipboyRoot.m_pObject->Update(UpdateData_2);
                    sp3rdPersonPipboyRoot.m_pObject->UpdateProperties();
                }
                NiColorA newcolor(0.0f, 0.0f, 0.0f, 0.0f);
                SystemColorManager* Instance = SystemColorManager::GetInstance();
                if (Instance->GetColor(4, newcolor)) {
                    SetBackgroundColor(newcolor);
                    SetTintColor(newcolor);
                }
                float* v6 = &fBlurIntensityPipboy.Float();
                SetBlurIntensity(*v6);
                SetBlurRadius(fBlurRadiusPipboy.Float());
                float* v7 = &gs_fScanlineFrequencyPipboy_Pipboy.Float();
                SetDoScanlines(true, *v7);
                bPulseAffectsLight = 1;
                ShowPipboyLightEffect(false, false, true);
                ShowPipboyLightEffect(true, false, true);
            }
        }
    }
}

void FOPipboyManager::Shutdown() {
    int32_t i;
    (&spPipboyRoot)->operator=(nullptr);
    (&sp3rdPersonPipboyRoot)->operator=(nullptr);
    (&spPartialScreenGeometry)->operator=(nullptr);
    for (i = 0; i < 3; ++i) {
        (&spButtons[i])->operator=(nullptr);
        (&spButtonGlow[i])->operator=(nullptr);
        (&spKnobs[i])->operator=(nullptr);
        fKnobPositions[i] = (float)0.0;
        fKnobTargets[i] = (float)0.0;
        fKnobRates[i] = (float)0.0;
    }
    (&sp3rdPersonScreen)->operator=(nullptr);
    FORenderedMenu::Shutdown();
}

void FOPipboyManager::OnMenuOpen() {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (GetInitialized()) {
        SetRefractive(false);
        ResetColors();
        Interface::bStartPipboyRenderBackground = true;
        FORenderedMenu::bSoundsActive = 1;
        Interface::IsolateMenuElements(true, true);
        pBackgroundHum.Stop();
        pBackgroundHum.Release();
        BSAudio* v1 = &BSAudio::QInstance();
        BSSoundHandle* arHandle = v1->GetSoundHandleByName(&a1, "UIPipBoyHumLP", 49u);
        pBackgroundHum.operator=(*arHandle);
        pBackgroundHum.Play(false);
        float fDefaultBurstIntensity = ::fDefaultBurstIntensity.Float();
        float* v2 = &fDefaultBurstDuration.Float();
        TriggerBurst(*v2, fDefaultBurstIntensity, 0.5f);
        FORenderedMenu::OnMenuOpen();
        TESImageSpaceModifier* Hit = TESImageSpaceModifier::GetGetHit();
        ImageSpaceModifierInstanceForm::Stop(Hit);
        TESImageSpaceModifier::AllowGetHit(false);
        BSShaderBloodSplatter::SetGlobalAlpha(0.0f);
    }
}

void FOPipboyManager::OnMenuClose() {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    bool IsRefractive = pPlayer->IsRefractive();
    SetRefractive(IsRefractive);
    Animation* panim = pPlayer->GetAnimation(true);
    if (panim) {
        if (!panim->SpecialIdleDonePlaying()) {
            panim->ClearGroup(AGS_SPECIAL_IDLE, 0.0f);
        }
        uint16_t AnimGroup = pPlayer->GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr);
        panim->PlayGroup(AnimGroup, ACTION_START, -1, AGS_NONE);
        uint16_t v5 = pPlayer->GetAnimGroup(ANIM_GROUP_IDLE, nullptr, false, nullptr);
        Animation* panim_2 = pPlayer->GetAnimation(false);
        panim_2->PlayGroup(v5, ACTION_START, -1, AGS_NONE);
        if (pPlayer->GetSitSleepState()) {
            panim->PlayGroup(0, ACTION_START, -1, AGS_IDLE);
        }
    }
    if (GetInitialized()) {
        BSAudio* v4 = &BSAudio::QInstance();
        BSSoundHandle* SoundHandleByName = v4->GetSoundHandleByName(&a1, "UIPipBoyAccessDown", 289u);
        SoundHandleByName->Play(false);
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
    bool v17[3];
    float v16 = afTime - fLastPipboyKnobUpdateTime;
    v17[0] = false;
    v17[1] = false;
    v17[2] = false;
    for (int32_t i = 0; i < 3; ++i) {
        if (fKnobPositions[i] < static_cast<double>(fKnobTargets[i])) {
            fKnobPositions[i] = fKnobRates[i] * v16 + fKnobPositions[i];
            if (fKnobTargets[i] < static_cast<double>(fKnobPositions[i])) {
                fKnobPositions[i] = fKnobTargets[i];
            }
            v17[i] = true;
        }
        if (fKnobPositions[i] > static_cast<double>(fKnobTargets[i])) {
            fKnobPositions[i] = fKnobPositions[i] - fKnobRates[i] * v16;
            if (fKnobTargets[i] > static_cast<double>(fKnobPositions[i])) {
                fKnobPositions[i] = fKnobTargets[i];
            }
            v17[i] = true;
        }
    }
    if (spKnobs->m_pObject) {
        if (v17[0]) {
            float fAngle = fKnobPositions[0];
            NiAVObject* v2 = spKnobs->m_pObject;
            v2->SetRotate(fAngle, static_cast<float>(1.0), static_cast<float>(0.0), static_cast<float>(0.0));
            NiUpdateData a2(0.0, false, false);
            NiAVObject* v3 = spKnobs->m_pObject;
            v3->Update(a2);
        }
    }
    if (spKnobs[1].m_pObject) {
        if (v17[1]) {
            float fAnglea = fKnobPositions[1];
            NiAVObject* v4 = spKnobs[1].m_pObject;
            v4->SetRotate(fAnglea, static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(1.0));
            NiUpdateData arData(0.0, false, false);
            NiAVObject* v5 = spKnobs[1].m_pObject;
            v5->Update(arData);
        }
    }
    if (spKnobs[2].m_pObject) {
        if (v17[2]) {
            float fAngleb = fKnobPositions[2];
            NiAVObject* v6 = spKnobs[2].m_pObject;
            v6->SetRotate(fAngleb, static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(1.0));
            NiUpdateData apMemory(0.0, false, false);
            NiAVObject* v7 = spKnobs[2].m_pObject;
            v7->Update(apMemory);
        }
    }
    fLastPipboyKnobUpdateTime = afTime;
}

void FOPipboyManager::SetRefractive(bool abRefractive) {
    if (GetScreenGeometry()) {
        if (Get3rdPersonScreen()) {
            if (abRefractive) {
                GetScreenGeometry()->SetAppCulled(true);
                NiAVObject* v2 = Get3rdPersonScreen();
                v2->SetAppCulled(true);
            }
            else {
                GetScreenGeometry()->SetAppCulled(false);
                NiAVObject* v3 = Get3rdPersonScreen();
                v3->SetAppCulled(false);
            }
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
    uint32_t result = (&appTimer)->BSTimer::GetTime();
    fShudderStartTime = ((float)result);
    fShudderCurrentPosition = (float)0.0;
    return result;
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
        float* pVertHoldChance = &fVertHoldChance.Float();
        float* pShudderChance = &fShudderChance.Float();
        if (fRandom < static_cast<double>(*pVertHoldChance) + static_cast<double>(*pShudderChance)) {
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
        fLightEffectFadeStartTime = ((float)(&appTimer)->BSTimer::GetTime());
        bLightEffectFading = true;
        if (abFadeOut) {
            fLightEffectFadeTarget = (float)0.0;
        }
        else {
            fLightEffectFadeTarget = (float)1.0;
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
    if (this->spButtonGlow[0].operator->()) {
        this->spButtonGlow[0]->SetAppCulled(true);
    }
    if (this->spButtonGlow[1].operator->()) {
        this->spButtonGlow[1]->SetAppCulled(true);
    }
    if (this->spButtonGlow[2].operator->()) {
        this->spButtonGlow[2]->SetAppCulled(true);
    }
    // The buttons count from 1.
    if (aiButton > 0 && this->spButtonGlow[aiButton - 1].operator->()) {
        this->spButtonGlow[aiButton - 1]->SetAppCulled(false);
    }
}

// ReInitialize is the public wrapper PlayerCharacter::PositionPlayer calls, and on PC it only re-creates the light.
void FOPipboyManager::ReInitialize() {
    this->InitLight();
}

void FOPipboyManager::ShowPipboyLightEffect(const bool ab3rdPerson, const bool abShow, const bool abUpdateVariable) {
    NiPointer<NiNode>* pspRoot = ab3rdPerson ? &this->sp3rdPersonPipboyRoot : &this->spPipboyRoot;
    NiNode* pRoot = pspRoot->operator->();
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
                // deliberately, as the game does.
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
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    if (bLightEffectFading) {
        FadeLight(afTime);
        ShowPipboyLightEffect(false, true, false);
        NiAVObject* proot = spPipboyRoot.m_pObject;
        float fdelta = afTime - fLightEffectFadeStartTime;
        float v7 = fdelta / fLightEffectFadeDuration.Float();
        float ffade = BSMIN(v7, 1.0f);
        ffade = BSMAX(0.0, ffade);
        if (fLightEffectFadeTarget == 0.0) {
            ffade = 1.0 - ffade;
        }
        NiFixedString* PipboyLightEffect = ::new (static_cast<void*>(&apString)) NiFixedString("PipboyLightEffect");
        NiAVObject* pLightEffect = proot->GetObjectByName(*PipboyLightEffect);
        apString.~NiFixedString();
        if (!pLightEffect) {
            NiFixedString v9("PipboyLightEffect:0");
            NiFixedString* PipboyLightEffect_0 = &v9;
            pLightEffect = proot->GetObjectByName(*PipboyLightEffect_0);
        }
        NiNode* plightNode = nullptr;
        if (pLightEffect) {
            plightNode = pLightEffect->IsNode();
        }
        if (plightNode) {
            if (plightNode->GetChildCount()) {
                NiAVObject* v4 = plightNode->GetAt(0);
                NiGeometry* peffect = v4->IsGeometry();
                if (peffect) {
                    int32_t Type = NiMaterialProperty::GetType();
                    NiMaterialProperty* pmp = static_cast<NiMaterialProperty*>(peffect->GetProperty(Type));
                    if (pmp) {
                        pmp->SetAlpha(ffade);
                    }
                    else {
                        int32_t v3 = NiShadeProperty::GetType();
                        BSShaderProperty* psp = static_cast<BSShaderProperty*>(peffect->GetProperty(v3));
                        psp->SetAlpha(ffade);
                    }
                    peffect->UpdateProperties();
                }
            }
        }
        if (fLightEffectFadeTarget == ffade) {
            bLightEffectFading = false;
            if (fLightEffectFadeTarget == 0.0) {
                ShowPipboyLightEffect(false, false, false);
            }
        }
    }
}

void FOPipboyManager::ResetPipBoyManager() {
    pIOManager->LoadQueuedPriority(IO_TASK_PRIORITY_VERY_LOW);
    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
    CurrentAIProcess->ResolveEquipmentQueue(pPlayer);
    CurrentAIProcess->Set3DUpdateFlag(RESET_MODEL);
    CurrentAIProcess->Update3dModel(pPlayer);
    Shutdown();
    Initialize();
    SetPipBoyManagerReset(false);
}

void FOPipboyManager::SetTabKnobPosition(uint32_t aiNewPos, bool abPlaySounds) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (fTabKnobPositions[aiNewPos] != fKnobTargets[2]) {
        fKnobTargets[2] = fTabKnobPositions[aiNewPos];
        fKnobRates[2] = fTabKnobMoveRate.Float();
        if (FORenderedMenu::bSoundsActive) {
            if (abPlaySounds) {
                BSSoundHandle psoundHand;
                BSAudio* v3 = &BSAudio::QInstance();
                BSSoundHandle* arHandle = v3->GetSoundHandleByName(&a1, "UIPipBoyTab", 289u);
                psoundHand.operator=(*arHandle);
                psoundHand.Play(false);
            }
        }
    }
}

NiAVObject* FOPipboyManager::Get3rdPersonScreen() {
    return (&sp3rdPersonScreen)->operator->();
}

NiAVObject* FOPipboyManager::Get3rdPersonPipboyRoot() {
    return sp3rdPersonPipboyRoot.m_pObject;
}

void FOPipboyManager::MoveKnob(FOPipboyManager::Knobs aiKnob, float afAmount, float afRate, bool abPlaySound) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (abPlaySound && aiKnob == FOPipboyManager::KNOB_SCROLL) {
        BSSoundHandle psoundHand;
        BSAudio* v5 = &BSAudio::QInstance();
        BSSoundHandle* arHandle = v5->GetSoundHandleByName(&a1, "UIPipBoyScroll", 289u);
        psoundHand.operator=(*arHandle);
        psoundHand.Play(false);
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
        NiColor kDiffuse;
        pLight->SetDiffuseColor(*FNVD_RECONSTRUCT(NiColor)(&kDiffuse, 0.0f, 0.0f, 0.0f));
        NiColor kAmbient;
        pLight->SetAmbientColor(*FNVD_RECONSTRUCT(NiColor)(&kAmbient, 0.0f, 0.0f, 0.0f));
        pLight->SetConstantAttenuation(0.0f);
        pLight->SetLinearAttenuation(0.0f);
        pLight->SetQuadraticAttenuation(0.0f);
        pLight->SetDimmer(1.0f);
        // The light radius lives in the specular colour's first channel.
        NiColor kSpecular;
        pLight->SetSpecularColor(*FNVD_RECONSTRUCT(NiColor)(&kSpecular, fScreenLightRadius.Float(), 0.0f, 0.0f));
        alignas(NiColorA) uint8_t aucColor[sizeof(NiColorA)];
        NiColorA& kColor = *::new (aucColor) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
        // 4 is the pip-boy colour's id.
        SystemColorManager::GetInstance()->GetColor(4, kColor);
        NiColor kScreenColor;
        kScreenColor.~NiColor();
        ::new (&kScreenColor) NiColor(kColor.r, kColor.g, kColor.b);
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
    if (!this->spButtonGlow[0].operator->()) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kGlow10("StatsGlow:0");
        this->spButtonGlow[0] = p3D->GetObjectByName(kGlow10);
    }
    p3D = pPlayer->Get3D(true);
    NiFixedString kGlow2("ItemsGlow");
    this->spButtonGlow[1] = p3D->GetObjectByName(kGlow2);
    if (!this->spButtonGlow[1].operator->()) {
        p3D = pPlayer->Get3D(true);
        NiFixedString kGlow20("ItemsGlow:0");
        this->spButtonGlow[1] = p3D->GetObjectByName(kGlow20);
    }
    p3D = pPlayer->Get3D(true);
    NiFixedString kGlow3("DataGlow");
    this->spButtonGlow[2] = p3D->GetObjectByName(kGlow3);
    if (!this->spButtonGlow[2].operator->()) {
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
    if (pLight && pLight->spLight.operator->()) {
        pLight->spLight.operator->()->SetDimmer(fFade);
    }
}

void FOPipboyManager::ResetColors() {
    NiColorA a2(static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(0.0));
    SystemColorManager* Instance = SystemColorManager::GetInstance();
    if (Instance->GetColor(4, a2)) {
        SetBackgroundColor(a2);
        SetTintColor(a2);
        InitLight();
    }
}

char FOPipboyManager::CheckOrangeButtonsClicked(char arg0, char a3, float* a4, float* a5, uint32_t* a6) {
    char v26 = 0;
    InterfaceManager* v27 = InterfaceManager::Instance();
    if (!GetScreenGeometry() || (!spButtons->m_pObject || (!spButtons[1].m_pObject || !spButtons[2].m_pObject))) {
        return 0;
    }
    if (InterfaceManager::IsInPipboyMenu() && InterfaceManager::IsPipboyMenuTopmost()) {
        if (spPipboyRoot.m_pObject) {
            NiPoint3 a2;
            NiPoint3 a1;
            NiPick apMemory(0, 8u);
            apMemory.SetObserveAppCullFlag(true);
            apMemory.SetTarget(GetScreenGeometry());
            apMemory.m_eSortType = NiPick::SORT;
            apMemory.SetIsReturnTexture(true);
            NiCamera* Camera = Main::GetWorldSceneGraph()->GetCamera();
            int32_t ScriptProcessingDelay = static_cast<int32_t>(v27->sRealCursorPos.z);
            double DataClipDist = v27->sRealCursorPos.x;
            Camera->WindowPointToRay(static_cast<int32_t>(DataClipDist), ScriptProcessingDelay, a1, a2, nullptr);
            if (apMemory.PickObjects(a1, a2, false)) {
                NiPick::Record* v21 = (&apMemory.m_pickResults)->GetAt(0);
                if (v21) {
                    NiGeometry* AVObject = static_cast<NiGeometry*>(v21->GetAVObject());
                    if (AVObject == GetScreenGeometry()) {
                        float* Filename = &(&v21->m_texture)->x;
                        float v17 = *Filename;
                        float v18 = Filename[1];
                        float RealScreenHeight = InterfaceManager::GetRealScreenHeight();
                        float v19 = RealScreenHeight * 1.3333330154418945;
                        *a4 = v17 * v19;
                        *a5 = v18 * RealScreenHeight;
                        v26 = 1;
                    }
                }
            }
            if (arg0 || a3) {
                int32_t v16 = -1;
                apMemory.m_eSortType = NiPick::SORT;
                apMemory.SetIsReturnTexture(false);
                for (int32_t i = 0; i < 3; ++i) {
                    apMemory.SetTarget(spButtons[i].m_pObject);
                    int32_t v13 = static_cast<int32_t>(v27->sRealCursorPos.z);
                    int32_t v11 = static_cast<int32_t>(v27->sRealCursorPos.x);
                    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
                    WorldSceneGraph->GetCamera()->WindowPointToRay(v11, v13, a1, a2, nullptr);
                    if (apMemory.PickObjects(a1, a2, false)) {
                        if ((&apMemory.m_pickResults)->GetAt(0)) {
                            if (arg0) {
                                iClickedOrangeButton = i;
                            }
                            else {
                                v16 = i;
                            }
                        }
                    }
                }
                if (a3) {
                    if (v16 == iClickedOrangeButton) {
                        *a6 = v16 + 1;
                        iClickedOrangeButton = -1;
                    }
                }
            }
        }
    }
    return v26;
}

void FOPipboyManager::InitKnobs() {
    double v3;
    FNVD_INLINED_LOCAL(NiFixedString, v20, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, v22, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiMatrix3 ecx0;
    NiMatrix3 prn;
    NiAVObject* v16 = pPlayer->Get3D(true);
    NiFixedString* v15 = ::new (static_cast<void*>(&apString)) NiFixedString("ScrollKnob");
    NiAVObject* pGeometry = v16->GetObjectByName(*v15);
    apString.~NiFixedString();
    if (!pGeometry) {
        NiAVObject* v14 = pPlayer->Get3D(true);
        NiFixedString v23("ScrollKnob:0");
        NiFixedString* v13 = &v23;
        pGeometry = v14->GetObjectByName(*v13);
    }
    if (pGeometry) {
        if (pGeometry->IsNode()) {
            NiNode* v1 = pGeometry->IsNode();
            spKnobs->operator=(static_cast<NiGeometry*>(v1->GetAt(0)));
        }
    }
    NiAVObject* v12 = pPlayer->Get3D(true);
    NiFixedString* v11 = ::new (static_cast<void*>(&v22)) NiFixedString("RadNeedle");
    spKnobs[1].operator=(static_cast<NiGeometry*>(v12->GetObjectByName(*v11)));
    v22.~NiFixedString();
    if (!spKnobs[1].m_pObject) {
        NiAVObject* v10 = pPlayer->Get3D(true);
        NiFixedString v21("RadNeedle:0");
        NiFixedString* v9 = &v21;
        spKnobs[1].operator=(static_cast<NiGeometry*>(v10->GetObjectByName(*v9)));
    }
    NiAVObject* v8 = pPlayer->Get3D(true);
    NiFixedString* v7 = ::new (static_cast<void*>(&v20)) NiFixedString("TabKnob");
    spKnobs[2].operator=(static_cast<NiGeometry*>(v8->GetObjectByName(*v7)));
    v20.~NiFixedString();
    if (!spKnobs[2].m_pObject) {
        NiAVObject* v6 = pPlayer->Get3D(true);
        NiFixedString v19("TabKnob:0");
        NiFixedString* v5 = &v19;
        spKnobs[2].operator=(static_cast<NiGeometry*>(v6->GetObjectByName(*v5)));
    }
    fTabKnobPositions[0] = fTabKnobMinPosition.Float();
    fTabKnobPositions[4] = fTabKnobMaxPosition.Float();
    float ftabKnobIncrement = (fTabKnobPositions[4] - fTabKnobPositions[0]) / 4.0;
    for (uint32_t i = 1; i < 4; ++i) {
        fTabKnobPositions[i] = fTabKnobPositions[i - 1] + ftabKnobIncrement;
    }
    fKnobPositions[2] = fTabKnobPositions[0];
    fKnobTargets[2] = fTabKnobPositions[0];
    fKnobRates[2] = 0.0f;
    if (pPlayer) {
        double v4 = pPlayer->GetActorFloatValue(ActorValue::RADIATION_RADS);
        v3 = 1.5707963267948966 - v4 / StatsMenu::GetRadMeterMax() * 3.1415927410125732;
    }
    else {
        v3 = 0.0;
    }
    fKnobTargets[1] = v3;
    fKnobPositions[1] = fKnobTargets[1];
    fKnobRates[1] = 0.0f;
    if (spKnobs->m_pObject) {
        ecx0 = (*&spKnobs->m_pObject->m_kLocal).m_Rotate;
        prn = (*&spKnobs->m_pObject->m_pkParent->m_kLocal).m_Rotate;
        spKnobs->m_pObject->m_pkParent->SetRotate(prn * ecx0);
        spKnobs->m_pObject->SetRotate(fKnobPositions[0], 1.0f, 0.0f, 0.0f);
    }
    if (spKnobs[1].m_pObject) {
        spKnobs[1].m_pObject->SetRotate(fKnobPositions[1], 0.0f, 0.0f, 1.0f);
    }
    if (spKnobs[2].m_pObject) {
        spKnobs[2].m_pObject->SetRotate(fKnobPositions[2], 0.0f, 0.0f, 1.0f);
    }
}

void FOPipboyManager::UpdateIdleAnims() {
    InterfaceManager* singleton;
    TESAnimGroup* animGroup;
    float fSecondsPassed;
    BSAnimGroupSequence* currentSequence;
    Animation* animation;
    if ((!Interface::IsConsoleVisible()) && (InterfaceManager::Instance()->ePipboyState == 3)) {
        singleton = InterfaceManager::Instance();
        if (singleton->GetCurrentMode() != 4) {
            animation = pPlayer->GetAnimation(true);
            currentSequence = animation->GetCurrentSequence(static_cast<ANIM_GROUP_SECTION>(0));
            if (currentSequence) {
                animGroup = currentSequence->GetAnimGroup();
                if ((animGroup->GetType() != 1) && animation->GroupLoaded(1u)) {
                    animation->PlayGroup(1u, static_cast<ACTION_FLAGS>(1), (-1), static_cast<ANIM_GROUP_SECTION>(-1));
                }
            }
            animation->SkipUpdate(AGS_SKIP_ALL_BUT_SPECIAL_IDLE);
            fSecondsPassed = (&appTimer)->GetSeconds();
            animation->Update(pPlayer, fSecondsPassed, (-0x1p+0f));
            animation->UpdateMovement(pPlayer);
        }
    }
}

void FORenderedMenu::SetBackgroundColor(const NiColorA& aColor) {
    this->BackgroundColor = aColor;
}

void FORenderedMenu::SetBlurIntensity(const float afIntensity) {
    this->fBlurIntensity = afIntensity;
}
