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
    BSAudioManager::QInstance()
        .GetSoundHandleByName(
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
    NiNode* pMenuRoot = static_cast<NiNode*>(GetMenuRoot());
    if (pMenuRoot) {
        pMenuRoot->SetTranslate(OriginalPos);
        NiUpdateData kUpdateData(0.0, false, false);
        pMenuRoot->Update(kUpdateData);
    }
    FORenderedMenu::Draw(apTarget, NiRenderer::CLEAR_BACKBUFFER, nullptr);
}

bool FORenderedTerminal::OnIdle() {
    if (bFadingOut) {
        if (!FORenderedTerminal::spGeometry.m_pObject ||
            !static_cast<uint8_t>(FORenderedTerminal::spGeometry->QFadingFlag()) ||
            fLastFadePercent == FORenderedTerminal::spGeometry->GetLastFadePercent()) {
            InterfaceManager::Instance()->EnterRenderedMenu(nullptr);
            return true;
        }
        fLastFadePercent = FORenderedTerminal::spGeometry->GetLastFadePercent();
    }
    return FORenderedMenu::OnIdle();
}

bool FORenderedTerminal::Pick(bool abPressed, float* apX, float* apY) {
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    if (spExitButton.m_pObject && abPressed) {
        NiPoint3 kDir;
        NiPoint3 kOrigin;
        NiCamera* pCamera = GetCamera();
        if (!pCamera) {
            pCamera = Main::GetWorldSceneGraph()->GetCamera();
        }
        NiPick kPick(0, 8u);
        kPick.SetObserveAppCullFlag(false);
        kPick.SetTarget(spExitButton.m_pObject);
        kPick.m_eSortType = NiPick::NO_SORT;
        int32_t iCursorY = static_cast<int32_t>(pInterfaceManager->sRealCursorPos.z);
        double dCursorX = pInterfaceManager->sRealCursorPos.x;
        pCamera->WindowPointToRay(static_cast<int32_t>(dCursorX), iCursorY, kOrigin, kDir, nullptr);
        if (kPick.PickObjects(kOrigin, kDir, false)) {
            NiPick::Record* pPickResult = kPick.m_pickResults.GetAt(0);
            if (pPickResult) {
                NiTriStrips* pPicked = static_cast<NiTriStrips*>(pPickResult->GetAVObject());
                if (pPicked == spExitButton.m_pObject) {
                    CloseMenu();
                    return false;
                }
            }
        }
    }
    return FORenderedMenu::Pick(abPressed, apX, apY);
}

void FORenderedTerminal::Unknown07() {
    NiNode* pMenuRoot = static_cast<NiNode*>(GetMenuRoot());
    if (pMenuRoot) {
        pMenuRoot->SetTranslate(OriginalPos);
        NiUpdateData kUpdateData(0.0, false, false);
        pMenuRoot->Update(kUpdateData);
    }
}

void FORenderedTerminal::PostDraw() {
    NiNode* pMenuRoot = static_cast<NiNode*>(GetMenuRoot());
    if (pMenuRoot) {
        pMenuRoot->SetTranslate(30490.0f, 0.0f, 0.0f);
        NiUpdateData kUpdateData(0.0f, false, false);
        pMenuRoot->Update(kUpdateData);
    }
}

void FORenderedTerminal::Unknown08() {
    FORenderedTerminal::PostDraw();
}

void FORenderedTerminal::Initialize() {
    NiMatrix3 mat2;
    NiMatrix3 rot;
    NiMatrix3 mat1;
    NiRect<float> port;
    SetDoScanlines(true, fRenderedTerminalScanlineScale.Float());
    BSFadeNode* pGeom = nullptr;
    if (FORenderedTerminal::spGeometry.m_pObject) {
        pGeom = FORenderedTerminal::spGeometry.m_pObject;
    }
    else {
        pGeom = static_cast<BSFadeNode*>(
            pModelLoader->LoadFile(pTerminalFile, LOD_MULT_NONE, true, false, false, false)
        );
        FORenderedTerminal::spGeometry = pGeom;
        SwapPlatformLanguageTextures(pGeom);
    }
    NiAVObject* pScreenObj = pGeom->GetObjectByName("screen");
    NiGeometry* pScreen = pScreenObj->IsGeometry();
    if (!pScreen) {
        pScreen = pScreenObj->IsNode()->GetAt(0)->IsGeometry();
    }
    NiAVObject* pExitButtonObj = pGeom->GetObjectByName("PowerButton");
    if (pExitButtonObj->IsGeometry()) {
        spExitButton = static_cast<NiTriStrips*>(pExitButtonObj);
    }
    else {
        NiNode* pNode = pExitButtonObj->IsNode();
        spExitButton = static_cast<NiTriStrips*>(pNode->GetAt(0));
    }
    mat1.MakeYRotation(1.57f);
    mat2.MakeZRotation(1.57f);
    pGeom->SetRotate(mat2 * mat1);
    float fZoom = fRenderedTerminalZoom.Float();
    if (!Renderer::GetWidescreen() && !BSShaderManager::bLetterbox) {
        fZoom = fZoom * 0.75;
    }
    float fHPos = fRenderedTerminalHPos.Float();
    pGeom->SetTranslate(fZoom, fRenderedTerminalVPos.Float(), fHPos);
    NiUpdateData UpdateData1(0.0f, false, false);
    pGeom->Update(UpdateData1);
    NiCamera* pCam = new NiCamera;
    ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pCam->SetViewPort(port);
    rot.SetRow(0, 0.0f, 0.0f, 1.0f);
    rot.SetRow(1u, 1.0f, 0.0f, 0.0f);
    rot.SetRow(2u, 0.0f, 1.0f, 0.0f);
    pCam->SetRotate(NiMatrix3::IDENTITY);
    pCam->SetMinNearPlaneDist(1.0f);
    NiFrustum fr(false);
    fr.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    fr.m_fNear = BSSceneGraph::fNearDistance.Float();
    float fFarNearRatio = fr.m_fFar / fr.m_fNear;
    pCam->SetMaxFarNearRatio(fFarNearRatio);
    int32_t iScreenHeight = BSShaderManager::bLetterbox ? BSShaderManager::iLetterboxHeight
                                                        : Renderer::GetScreenHeight();
    float fHeightByWidth = static_cast<double>(iScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fAspectDelta = 0.75f;
    double dFOV = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369;
    float fTheta = fRenderedTerminalFOV.Float() * dFOV;
    double dTan = std::tan(fTheta);
    fr.m_fLeft = -dTan * fAspectDelta;
    dTan = std::tan(fTheta);
    fr.m_fRight = dTan * fAspectDelta;
    dTan = std::tan(fTheta);
    fr.m_fBottom = -dTan * fHeightByWidth * fAspectDelta;
    dTan = std::tan(fTheta);
    fr.m_fTop = dTan * fHeightByWidth * fAspectDelta;
    fr.m_bOrtho = false;
    pCam->SetViewFrustum(fr);
    NiUpdateData UpdateData2(0.0f, false, false);
    pCam->Update(UpdateData2);
    PrepareGeometry(pScreen);
    SetScreenGeometry(pScreen);
    SetScreenParent(pGeom);
    pScreen->RemoveProperty(NiAlphaProperty::GetType());
    NiAlphaProperty* pAlphaProp = new NiAlphaProperty;
    pScreen->AttachProperty(pAlphaProp);
    SetCamera(pCam);
    FORenderedMenu::Initialize();
    OriginalPos = GetMenuRoot()->GetTranslate();
    GetMenuRoot()->SetTranslate(30490.0f, 0.0f, 0.0f);
    float fRadius = pGeom->GetWorldBound().m_fRadius * 3.0;
    if (!BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pGeom, fRadius)) {
        InitLight();
    }
    SetScreenTexture(nullptr);
    bPulseAffectsLight = 0;
    FORenderedTerminal::spGeometry->SetLODMultType(LOD_MULT_RENDEREDMENU);
    NiColorA newColor(0.0f, 0.0f, 0.0f, 0.0f);
    if (SystemColorManager::GetInstance()->GetColor(3, newColor)) {
        SetTintColor(newColor);
    }
    fLastFadePercent = 1.0f;
}

void FORenderedTerminal::OnMenuOpen() {
    NiAVObject* pScreenParent = FORenderedMenu::GetParentNodeToRender();
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(FORenderedMenu::GetParentNodeToRender(), 1);
    NiUpdateData foo(0.0f, false, false);
    pScreenParent->Update(foo);
    if (GetMenuLight()) {
        GetMenuLight()->SetAppCulled(false);
    }
    FORenderedMenu::OnMenuOpen();
    bFadingOut = false;
    fLastFadePercent = 0.0f;
    if (FORenderedTerminal::spGeometry.m_pObject) {
        FORenderedTerminal::spGeometry->SetRange(100000.0f, 100000.0f);
        FORenderedTerminal::spGeometry->TurnFadeNodeOff();
        FORenderedTerminal::spGeometry->SetFadingFlag(false);
    }
}

void FORenderedTerminal::OnMenuClose() {
    if (GetMenuLight()) {
        GetMenuLight()->SetAppCulled(true);
        NiUpdateData foo(0.0f, false, false);
        GetMenuLight()->Update(foo);
    }
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(FORenderedMenu::GetParentNodeToRender());
    FORenderedMenu::OnMenuClose();
}

void FORenderedTerminal::CloseMenu() {
    PlayTerminalShutdownSound();
    Tile* pTile = pMenuTile;
    if (pTile) {
        delete pTile;
    }
    pMenuTile = nullptr;
    InterfaceManager::Instance()->EnterRenderedMenu(nullptr);
}

void FORenderedTerminal::FadeOut() {
    if (FORenderedTerminal::spGeometry.m_pObject) {
        uint32_t uiMinNearDistance = LOD::iFadeNodeMinNearDistance::Get();
        LOD::iFadeNodeMinNearDistance::Set(0);
        FORenderedTerminal::spGeometry->SetRange(0.0, 0.0);
        LOD::iFadeNodeMinNearDistance::Set(uiMinNearDistance);
    }
    bFadingOut = true;
    fLastFadePercent = 1.0f;
}

void FORenderedTerminal::InitLight() {
    if (CreateMenuLight("Omni01", BSShaderManager::GetShadowSceneNode(1))) {
        NiPointLight* pLight = GetMenuLight();
        if (pLight) {
            pLight->SetAmbientColor(NiColor(0.0f, 0.0f, 0.0f));
            pLight->SetConstantAttenuation(0.0f);
            pLight->SetLinearAttenuation(0.0f);
            pLight->SetQuadraticAttenuation(0.0f);
            pLight->SetDimmer(1.0f);
            pLight->SetSpecularColor(NiColor(gs_fScreenLightRadius_RenderedTerminal.Float(), 0.0f, 0.0f));
            float fScreenLightColorB = gs_fScreenLightColorB_RenderedTerminal.Float();
            float fScreenLightColorG = gs_fScreenLightColorG_RenderedTerminal.Float();
            NiColor screenLightColor(
                gs_fScreenLightColorR_RenderedTerminal.Float(),
                fScreenLightColorG,
                fScreenLightColorB
            );
            SetScreenLightColor(screenLightColor);
            SetScreenLightBaseIntensity(g_fScreenLightBaseIntensity_RenderedTerminal.Float());
            SetScreenLightFlickerIntensity(1.0f);
            NiUpdateData foo(0.0f, false, false);
            pLight->Update(foo);
        }
    }
    else {
        Warning("MENUS: Failed to create terminal light.  Terminal will only receive ambient light.");
    }
}

void FORenderedTerminal::ReleaseStaticGeometry() {
    BSFadeNode* pGeometry = FORenderedTerminal::spGeometry;
    if (pGeometry) {
        pModelLoader->ReleaseModel(pTerminalFile, true, 1);
    }
    FORenderedTerminal::spGeometry = nullptr;
}
