#include "Fallout/Interface/FORenderedTerminal.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <cstdint>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection>
    fRenderedTerminalScanlineScale("fRenderedTerminalScanlineScale:RenderedTerminal", 130.0f);
SettingT<INISettingCollection> fRenderedTerminalZoom("fRenderedTerminalZoom:RenderedTerminal", 20.0f);
SettingT<INISettingCollection> fRenderedTerminalHPos("fRenderedTerminalHPos:RenderedTerminal", 0.0f);
SettingT<INISettingCollection> fRenderedTerminalVPos("fRenderedTerminalVPos:RenderedTerminal", 0.38f);
SettingT<INISettingCollection> fRenderedTerminalFOV("fRenderedTerminalFOV:RenderedTerminal", 0.15f);
SettingT<INISettingCollection> bUseRenderedTerminals("bUseRenderedTerminals:RenderedTerminal", true);
SettingT<INISettingCollection>
    g_fScreenLightBaseIntensity_RenderedTerminal("fScreenLightBaseIntensity:RenderedTerminal", 0.3f);
SettingT<INISettingCollection> gs_fScreenLightRadius_RenderedTerminal("fScreenLightRadius:RenderedTerminal", 32.0f);
SettingT<INISettingCollection> gs_fScreenLightColorR_RenderedTerminal("fScreenLightColorR:RenderedTerminal", 1.0f);
SettingT<INISettingCollection> gs_fScreenLightColorG_RenderedTerminal("fScreenLightColorG:RenderedTerminal", 1.0f);
SettingT<INISettingCollection> gs_fScreenLightColorB_RenderedTerminal("fScreenLightColorB:RenderedTerminal", 1.0f);
NiPointer<BSFadeNode> FORenderedTerminal::spGeometry;

void PlayTerminalShutdownSound() {
    BSAudioManager* v0 = &BSAudioManager::QInstance();
    v0->GetSoundHandleByName(
          "OBJComputerTerminalPowerDown",
          static_cast<BSGameSound::SoundFlags>(
              BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
          )
    )
        .Play(false);
}

FORenderedTerminal::FORenderedTerminal() : FORenderedMenu(), spExitButton(nullptr) {}

NiAVObject* FORenderedTerminal::GetParentNodeToRender() {
    return BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
}

void FORenderedTerminal::Draw(
    BSRenderedTexture* apTarget,
    NiRenderer::ClearFlags aeClearMode,
    BSRenderedTexture* apSwapTarget
) {
    NiNode* SynchTime = static_cast<NiNode*>(GetMenuRoot());
    if (SynchTime) {
        SynchTime->SetTranslate(OriginalPos);
        NiUpdateData a2(0.0, false, false);
        SynchTime->Update(a2);
    }
    FORenderedMenu::Draw(apTarget, NiRenderer::CLEAR_BACKBUFFER, nullptr);
}

bool FORenderedTerminal::OnIdle() {
    if (bFadingOut) {
        BSFadeNode* v1;
        BSFadeNode* v2;
        if (!FORenderedTerminal::spGeometry.m_pObject ||
            ((v1 = FORenderedTerminal::spGeometry.m_pObject, !static_cast<uint8_t>(v1->QFadingFlag())) ||
             (v2 = FORenderedTerminal::spGeometry.m_pObject, fLastFadePercent == v2->GetLastFadePercent()))) {
            InterfaceManager* v3 = InterfaceManager::Instance();
            v3->EnterRenderedMenu(nullptr);
            return true;
        }
        BSFadeNode* v5 = FORenderedTerminal::spGeometry.m_pObject;
        fLastFadePercent = v5->GetLastFadePercent();
    }
    return FORenderedMenu::OnIdle();
}

bool FORenderedTerminal::Pick(bool abPressed, float* apX, float* apY) {
    char v14 = 0;
    InterfaceManager* v15 = InterfaceManager::Instance();
    if (spExitButton.m_pObject && abPressed) {
        NiPoint3 a2;
        NiPoint3 a1;
        NiCamera* Camera = GetCamera();
        if (!Camera) {
            Camera = Main::GetWorldSceneGraph()->GetCamera();
        }
        NiPick apMemory(0, 8u);
        apMemory.SetObserveAppCullFlag(false);
        apMemory.SetTarget(spExitButton.m_pObject);
        apMemory.m_eSortType = NiPick::NO_SORT;
        int32_t ScriptProcessingDelay = static_cast<int32_t>(v15->sRealCursorPos.z);
        double DataClipDist = v15->sRealCursorPos.x;
        Camera->WindowPointToRay(static_cast<int32_t>(DataClipDist), ScriptProcessingDelay, a1, a2, nullptr);
        if (apMemory.PickObjects(a1, a2, false)) {
            NiPick::Record* v9 = (&apMemory.m_pickResults)->GetAt(0);
            if (v9) {
                NiTriStrips* AVObject = static_cast<NiTriStrips*>(v9->GetAVObject());
                if (AVObject == spExitButton.m_pObject) {
                    CloseMenu();
                    return false;
                }
            }
        }
    }
    return FORenderedMenu::Pick(abPressed, apX, apY);
}

void FORenderedTerminal::Unknown07() {
    NiNode* SynchTime = static_cast<NiNode*>(GetMenuRoot());
    if (SynchTime) {
        SynchTime->SetTranslate(OriginalPos);
        NiUpdateData a2(0.0, false, false);
        SynchTime->Update(a2);
    }
}

void FORenderedTerminal::PostDraw() {
    NiNode* MenuRoot = static_cast<NiNode*>(GetMenuRoot());
    if (MenuRoot) {
        MenuRoot->SetTranslate(30490.0f, 0.0f, 0.0f);
        NiUpdateData arData(0.0f, false, false);
        MenuRoot->Update(arData);
    }
}

void FORenderedTerminal::Unknown08() {
    FORenderedTerminal::PostDraw();
}

void FORenderedTerminal::Initialize() {
    NiAlphaProperty* v17;
    int32_t ScreenHeight;
    NiCamera* pcam;
    XFNV_INLINED_LOCAL(NiFixedString, v30, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiMatrix3 mat2;
    NiMatrix3 rot;
    NiMatrix3 mat1;
    NiRect<float> port;
    float* v1 = &fRenderedTerminalScanlineScale.Float();
    SetDoScanlines(true, *v1);
    bool bloaded = false;
    BSFadeNode* pgeom = nullptr;
    if (FORenderedTerminal::spGeometry.m_pObject) {
        pgeom = FORenderedTerminal::spGeometry.m_pObject;
    }
    else {
        pgeom =
            static_cast<BSFadeNode*>(pModelLoader->LoadFile(pTerminalFile, LOD_MULT_NONE, true, false, false, false));
        FORenderedTerminal::spGeometry.operator=(pgeom);
        SwapPlatformLanguageTextures(pgeom);
        bloaded = true;
    }
    NiFixedString* a5 = ::new (static_cast<void*>(&apString)) NiFixedString("screen");
    NiAVObject* pscreenObj = pgeom->GetObjectByName(*a5);
    apString.~NiFixedString();
    NiGeometry* pscreen = pscreenObj->IsGeometry();
    if (!pscreen) {
        NiNode* v2 = pscreenObj->IsNode();
        NiAVObject* v24 = v2->GetAt(0);
        pscreen = v24->IsGeometry();
    }
    NiFixedString* a4 = ::new (static_cast<void*>(&v30)) NiFixedString("PowerButton");
    NiAVObject* pexitButtonObj = pgeom->GetObjectByName(*a4);
    v30.~NiFixedString();
    if (pexitButtonObj->IsGeometry()) {
        spExitButton.operator=(static_cast<NiTriStrips*>(pexitButtonObj));
    }
    else {
        NiNode* v3 = pexitButtonObj->IsNode();
        spExitButton.operator=(static_cast<NiTriStrips*>(v3->GetAt(0)));
    }
    mat1.MakeYRotation(1.57f);
    mat2.MakeZRotation(1.57f);
    pgeom->SetRotate(mat2 * mat1);
    float fzoom = fRenderedTerminalZoom.Float();
    if (!Renderer::GetWidescreen() && !BSShaderManager::bLetterbox) {
        fzoom = fzoom * 0.75;
    }
    float fRenderedTerminalHPos = ::fRenderedTerminalHPos.Float();
    float* v5 = &fRenderedTerminalVPos.Float();
    pgeom->SetTranslate(fzoom, *v5, fRenderedTerminalHPos);
    NiUpdateData UpdateData1(0.0f, false, false);
    pgeom->Update(UpdateData1);
    NiCamera* apMemory = static_cast<NiCamera*>(NiMemObject::operator new(sizeof(NiCamera)));
    if (apMemory) {
        pcam = ::new (apMemory) NiCamera;
    }
    else {
        pcam = nullptr;
    }
    ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pcam->SetViewPort(port);
    rot.SetRow(0, 0.0f, 0.0f, 1.0f);
    rot.SetRow(1u, 1.0f, 0.0f, 0.0f);
    rot.SetRow(2u, 0.0f, 1.0f, 0.0f);
    pcam->SetRotate(NiMatrix3_IDENTITY);
    pcam->SetMinNearPlaneDist(1.0f);
    NiFrustum fr(false);
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    fr.m_fFar = WorldSceneGraph->GetFarDistance();
    fr.m_fNear = BSSceneGraph::fNearDistance.Float();
    float v20 = fr.m_fFar / fr.m_fNear;
    pcam->SetMaxFarNearRatio(v20);
    if (BSShaderManager::bLetterbox) {
        ScreenHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        ScreenHeight = Renderer::GetScreenHeight();
    }
    float fHeightByWidth = static_cast<double>(ScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float faspectDelta = 0.75f;
    double v18 = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369;
    float theta = fRenderedTerminalFOV.Float() * v18;
    double v6 = std::tan(theta);
    fr.m_fLeft = -v6 * faspectDelta;
    double v7 = std::tan(theta);
    fr.m_fRight = v7 * faspectDelta;
    double v8 = std::tan(theta);
    fr.m_fBottom = -v8 * fHeightByWidth * faspectDelta;
    double v9 = std::tan(theta);
    fr.m_fTop = v9 * fHeightByWidth * faspectDelta;
    fr.m_bOrtho = false;
    pcam->SetViewFrustum(fr);
    NiUpdateData UpdateData2(0.0f, false, false);
    pcam->Update(UpdateData2);
    PrepareGeometry(pscreen);
    SetScreenGeometry(pscreen);
    SetScreenParent(pgeom);
    int32_t fRenderedTerminalHPosa = NiAlphaProperty::GetType();
    pscreen->RemoveProperty(fRenderedTerminalHPosa);
    NiAlphaProperty* v27 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (v27) {
        v17 = ::new (v27) NiAlphaProperty;
    }
    else {
        v17 = nullptr;
    }
    pscreen->AttachProperty(v17);
    SetCamera(pcam);
    FORenderedMenu::Initialize();
    OriginalPos = GetMenuRoot()->GetTranslate();
    GetMenuRoot()->SetTranslate(30490.0f, 0.0f, 0.0f);
    const NiBound* WorldBound = &pgeom->GetWorldBound();
    float fradius = WorldBound->m_fRadius * 3.0;
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    if (!ShadowSceneNode_->AddAllLightsFromNode_Recurse(pgeom, fradius)) {
        InitLight();
    }
    SetScreenTexture(nullptr);
    bPulseAffectsLight = 0;
    FORenderedTerminal::spGeometry.m_pObject->SetLODMultType(LOD_MULT_RENDEREDMENU);
    NiColorA newcolor(0.0f, 0.0f, 0.0f, 0.0f);
    SystemColorManager* Instance = SystemColorManager::GetInstance();
    if (Instance->GetColor(3, newcolor)) {
        SetTintColor(newcolor);
    }
    fLastFadePercent = 1.0f;
}

void FORenderedTerminal::OnMenuOpen() {
    NiAVObject* pscreenParent = FORenderedMenu::GetParentNodeToRender();
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    ShadowSceneNode_->AttachChild(FORenderedMenu::GetParentNodeToRender(), 1);
    NiUpdateData foo(0.0f, false, false);
    pscreenParent->Update(foo);
    if (GetMenuLight()) {
        GetMenuLight()->SetAppCulled(false);
    }
    FORenderedMenu::OnMenuOpen();
    bFadingOut = false;
    fLastFadePercent = 0.0f;
    if (FORenderedTerminal::spGeometry.m_pObject) {
        FORenderedTerminal::spGeometry.m_pObject->SetRange(100000.0f, 100000.0f);
        FORenderedTerminal::spGeometry.m_pObject->TurnFadeNodeOff();
        FORenderedTerminal::spGeometry.m_pObject->SetFadingFlag(false);
    }
}

void FORenderedTerminal::OnMenuClose() {
    if (GetMenuLight()) {
        GetMenuLight()->SetAppCulled(true);
        NiUpdateData foo(0.0f, false, false);
        GetMenuLight()->Update(foo);
    }
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    ShadowSceneNode_->DetachChild(FORenderedMenu::GetParentNodeToRender());
    FORenderedMenu::OnMenuClose();
}

void FORenderedTerminal::CloseMenu() {
    PlayTerminalShutdownSound();
    Tile* Count = pMenuTile;
    if (Count) {
        delete Count;
    }
    pMenuTile = nullptr;
    InterfaceManager* v1 = InterfaceManager::Instance();
    v1->EnterRenderedMenu(nullptr);
}

void FORenderedTerminal::FadeOut() {
    BSFadeNode* v1;
    uint32_t iFadeNodeMinNearDistance;
    if ((&FORenderedTerminal::spGeometry)->m_pObject) {
        iFadeNodeMinNearDistance = LOD::iFadeNodeMinNearDistance::Get();
        LOD::iFadeNodeMinNearDistance::Set(0);
        v1 = (&FORenderedTerminal::spGeometry)->m_pObject;
        v1->SetRange(0.0, 0.0);
        LOD::iFadeNodeMinNearDistance::Set(iFadeNodeMinNearDistance);
    }
    bFadingOut = true;
    fLastFadePercent = (float)1.0;
}

void FORenderedTerminal::InitLight() {
    NiColor v9;
    NiColor dst;
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    if (CreateMenuLight("Omni01", ShadowSceneNode_)) {
        NiPointLight* plight = GetMenuLight();
        if (plight) {
            NiColor* v2 = XFNV_RECONSTRUCT(NiColor)(&dst, 0.0f, 0.0f, 0.0f);
            plight->SetAmbientColor(*v2);
            plight->SetConstantAttenuation(0.0f);
            plight->SetLinearAttenuation(0.0f);
            plight->SetQuadraticAttenuation(0.0f);
            plight->SetDimmer(1.0f);
            float* v3 = &gs_fScreenLightRadius_RenderedTerminal.Float();
            NiColor* v4 = XFNV_RECONSTRUCT(NiColor)(&v9, *v3, 0.0f, 0.0f);
            plight->SetSpecularColor(*v4);
            float fScreenLightColorB = gs_fScreenLightColorB_RenderedTerminal.Float();
            float fScreenLightColorG = gs_fScreenLightColorG_RenderedTerminal.Float();
            float* v5 = &gs_fScreenLightColorR_RenderedTerminal.Float();
            NiColor screenLightColor(*v5, fScreenLightColorG, fScreenLightColorB);
            SetScreenLightColor(screenLightColor);
            SetScreenLightBaseIntensity(g_fScreenLightBaseIntensity_RenderedTerminal.Float());
            SetScreenLightFlickerIntensity(1.0f);
            NiUpdateData foo(0.0f, false, false);
            plight->Update(foo);
        }
    }
    else {
        Warning("MENUS: Failed to create terminal light.  Terminal will only receive ambient light.");
    }
}

void FORenderedTerminal::ReleaseStaticGeometry() {
    BSFadeNode* vCondition1;
    vCondition1 = (&FORenderedTerminal::spGeometry)->operator->();
    if (vCondition1) {
        pModelLoader->ReleaseModel(pTerminalFile, true, 1);
    }
    (&FORenderedTerminal::spGeometry)->operator=(nullptr);
}
