#include "Fallout/Interface/FORenderedMenuRaceSex.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/Tile.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ImageSpaceEffectTV.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/RaceSexMenu.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fRaceSexMenuDistortDuration("fRaceSexMenuDistortDuration:RenderedMenus", 50.0f);
SettingT<INISettingCollection> fRaceSexMenuHPos("fRaceSexMenuHPos:RenderedTerminal", 0.0f);
SettingT<INISettingCollection> fRaceSexMenuVPos("fRaceSexMenuVPos:RenderedTerminal", 0.38f);
SettingT<INISettingCollection> fRaceSexMenuZoom("fRaceSexMenuZoom:RenderedTerminal", 92.0f);
SettingT<INISettingCollection> fRaceSexMenuScale("fRaceSexMenuScale:RenderedTerminal", 0.5f);

FORenderedMenuRaceSex::~FORenderedMenuRaceSex() {}

bool FORenderedMenuRaceSex::QRequiresDepthBuffer() {
    return true;
}

NiAVObject* FORenderedMenuRaceSex::GetParentNodeToRender() {
    return BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
}

void FORenderedMenuRaceSex::Draw(
    BSRenderedTexture* apTarget,
    NiRenderer::ClearFlags aeClearMode,
    BSRenderedTexture* apSwapTarget
) {
    // The background is always tinted.
    bool bTintBackground = true;
    NiRenderer* pr = NiRenderer::GetRenderer();
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
    NiPointer<BSRenderedTexture> spswap(
        pTextureManager->BorrowRenderedTexture(pRenderer, BSTextureManager::TEX_INTERFACE_RACESEXMENU, 0, nullptr, 0)
    );
    NiColorA kPrevColor(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
    kColor = BackgroundColor;
    kColor.a = 1.0f;
    if (bTintBackground) {
        NiColorA kTint(0.125f, 0.161f, 0.141f, 1.0f);
        kColor = kTint;
    }
    pr->GetBackgroundColor(kPrevColor);
    pr->SetBackgroundColor(kColor);
    NiRenderTargetGroup* pGroup = spswap->GetGroup();
    BSRenderedTexture::StartOffscreen(7u, pGroup);
    BSCullingProcess kCuller(nullptr);
    NiPointer<BSShaderAccumulator> spaccum(InterfaceManager::Instance()->Get3dAccumulator());
    kCuller.SetAccumulator(spaccum.m_pObject);
    kCuller.PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(3);
    BSShaderUtil::AccumulateScene(spClonedPlayerCamera.m_pObject, pShadowSceneNode, kCuller);
    BSShaderUtil::RenderScene(spClonedPlayerCamera.m_pObject, spaccum.m_pObject, false);
    kCuller.PopCullMode();
    kCuller.SetAccumulator(nullptr);
    spaccum = static_cast<BSShaderAccumulator*>(nullptr);
    BSRenderedTexture::StopOffscreen();
    FORenderedMenu::Draw(apTarget, NiRenderer::CLEAR_NONE, spswap.m_pObject);
    BSRenderedTexture* pSwapTexture = spswap.m_pObject;
    pTextureManager = BSShaderManager::TextureManagerPtr();
    pTextureManager->ReturnRenderedTexture(pSwapTexture);
    spswap = static_cast<BSRenderedTexture*>(nullptr);
}

bool FORenderedMenuRaceSex::OnIdle() {
    if (FORenderedMenu::GetInitialized()) {
        Animation* pAnimation = pPlayer->GetAnimation(false);
        pAnimation->SkipUpdate(AGS_SKIP_ALL_BUT_SPECIAL_IDLE);
        pPlayer->UpdateAnimation();
        pAnimation = pPlayer->GetAnimation(false);
        pPlayer->UpdateAnimationMovement(pAnimation, 0.0);
        return FORenderedMenu::OnIdle();
    }

    Interface::EmergencyCloseAllMenusAndBreakStuff();
    return false;
}

bool FORenderedMenuRaceSex::Pick(bool abPressed, float* apX, float* apY) {
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    if (abPressed) {
        char bAllButtons = 1;
        for (int32_t i = 0; bAllButtons && i < 4; ++i) {
            if (!spButtons[i].m_pObject || !spBtnGlow[i].m_pObject) {
                bAllButtons = 0;
            }
        }
        if (bAllButtons) {
            NiPick kPick(0, 8u);
            kPick.SetObserveAppCullFlag(true);
            kPick.m_eSortType = NiPick::SORT;
            kPick.SetIsReturnTexture(false);
            int32_t iPickedButton = -1;
            for (int32_t j = 0; iPickedButton < 0 && j < 4; ++j) {
                NiPoint3 kDirection;
                NiPoint3 kOrigin;
                kPick.SetTarget(spButtons[j].m_pObject);
                int32_t iCursorY = static_cast<int32_t>(pInterfaceManager->sRealCursorPos.z);
                int32_t iCursorX = static_cast<int32_t>(pInterfaceManager->sRealCursorPos.x);
                NiCamera* pCamera = GetCamera();
                pCamera->WindowPointToRay(iCursorX, iCursorY, kOrigin, kDirection, nullptr);
                if (kPick.PickObjects(kOrigin, kDirection, false) && kPick.m_pickResults.GetAt(0)) {
                    iPickedButton = j;
                }
            }
            if (iPickedButton >= 0) {
                BSAudio::QInstance().GetSoundHandleByName("UIMenuMode", 289u).Play(false);
                RaceSexMenu* pRaceSexMenu = static_cast<RaceSexMenu*>(pMenuTile->GetMenu());
                pRaceSexMenu->SetActiveList(static_cast<RaceSexMenu::ListID>(iPickedButton));
                return true;
            }
        }
    }
    return FORenderedMenu::Pick(abPressed, apX, apY);
}

void FORenderedMenuRaceSex::Unknown07() {
    GetMenuRoot();
}

void FORenderedMenuRaceSex::Unknown08() {
    FORenderedMenuRaceSex::Unknown07();
}

void FORenderedMenuRaceSex::Initialize() {
    int32_t iScreenHeight;
    NiCamera* pcam;
    NiPoint3 kLightPos;
    NiMatrix3 mat2;
    NiMatrix3 rot;
    NiMatrix3 mat1;
    NiRect<float> port;
    bFakeDistortActive = false;
    double dTime = static_cast<double>(appTimer.iLastTime);
    int32_t iRandom = rt::rand();
    fNextFakeDistortTime = static_cast<double>(iRandom % iDefaultDistortMaxInterval.Int()) + dTime;
    fFakeDistortDuration = fDefaultDistortDuration.Float();
    NiColorA newcolor(0.0f, 0.0f, 0.0f, 0.0f);
    if (SystemColorManager::GetInstance()->GetColor(3, newcolor)) {
        SetTintColor(newcolor);
    }
    BSFadeNode* pgeom = nullptr;
    if (spGeometry.m_pObject) {
        pgeom = static_cast<BSFadeNode*>(spGeometry.m_pObject);
    }
    else {
        RaceSexMenu* pRaceSexMenu = static_cast<RaceSexMenu*>(pMenuTile->GetMenu());
        const char* pcProjectorFile = pRaceSexMenu->GetProjectorFile();
        pgeom = static_cast<BSFadeNode*>(
            pModelLoader->LoadFile(pcProjectorFile, LOD_MULT_NONE, true, false, false, false)
        );
        spGeometry = static_cast<NiNode*>(pgeom);
        SwapPlatformLanguageTextures(pgeom);
    }
    pgeom->SetScale(fRaceSexMenuScale.Float());
    SetScreenParent(pgeom);
    NiAVObject* pscreen = pgeom->GetObjectByName("Screen");
    SetScreenTexture(nullptr);
    bPulseAffectsLight = 0;
    spPlayer3D = static_cast<NiNode*>(nullptr);
    spClonedPlayerCamera = static_cast<NiCamera*>(nullptr);
    spButtons[0] = pgeom->GetObjectByName("SexBtn:0");
    spButtons[1] = pgeom->GetObjectByName("RaceBtn:0");
    spButtons[2] = pgeom->GetObjectByName("FaceBtn:0");
    spButtons[3] = pgeom->GetObjectByName("BodyBtn:0");
    spBtnGlow[0] = pgeom->GetObjectByName("SexGlow:0");
    spBtnGlow[1] = pgeom->GetObjectByName("RaceGlow:0");
    spBtnGlow[2] = pgeom->GetObjectByName("FaceGlow:0");
    spBtnGlow[3] = pgeom->GetObjectByName("BodyGlow:0");
    fDistortStart = 0.0f;
    fDistortPos = 0.0f;
    SetNextDistortTime(0.0f);
    mat1.MakeYRotation(1.57f);
    mat2.MakeZRotation(1.57f);
    pgeom->SetRotate(mat2 * mat1);
    float fzoom = 0.0f;
    if (m_Context != RSM_CONTEXT_CHARGEN && m_Context) {
        fzoom = fRaceSexMenuZoom.Float();
    }
    else {
        fzoom = 80.0f;
    }
    if (!Renderer::GetWidescreen() && !BSShaderManager::bLetterbox) {
        fzoom = fzoom * 0.75;
    }
    if (m_Context != RSM_CONTEXT_CHARGEN && m_Context) {
        float fHPos = fRaceSexMenuHPos.Float();
        pgeom->SetTranslate(fzoom, fRaceSexMenuVPos.Float(), fHPos);
    }
    else {
        pgeom->SetTranslate(fzoom, 0.0f, fRaceSexMenuHPos.Float());
    }
    NiUpdateData UpdateData1(0.0f, false, false);
    pgeom->Update(UpdateData1);
    pcam = new NiCamera;
    ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pcam->SetViewPort(port);
    rot.SetRow(0, 0.0f, 0.0f, 1.0f);
    rot.SetRow(1u, 1.0f, 0.0f, 0.0f);
    rot.SetRow(2u, 0.0f, 1.0f, 0.0f);
    pcam->SetRotate(NiMatrix3::IDENTITY);
    pcam->SetMinNearPlaneDist(1.0f);
    NiFrustum fr(false);
    fr.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    fr.m_fNear = BSSceneGraph::fNearDistance.Float();
    float fFarNearRatio = fr.m_fFar / fr.m_fNear;
    pcam->SetMaxFarNearRatio(fFarNearRatio);
    if (BSShaderManager::bLetterbox) {
        iScreenHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        iScreenHeight = Renderer::GetScreenHeight();
    }
    float fHeightByWidth = static_cast<double>(iScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float faspectDelta = 0.75f;
    double dDefaultFOV = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369;
    float theta = fRenderedTerminalFOV.Float() * dDefaultFOV;
    double dTan = std::tan(theta);
    fr.m_fLeft = -dTan * faspectDelta;
    fr.m_fRight = dTan * faspectDelta;
    fr.m_fBottom = -dTan * fHeightByWidth * faspectDelta;
    fr.m_fTop = dTan * fHeightByWidth * faspectDelta;
    fr.m_bOrtho = false;
    pcam->SetViewFrustum(fr);
    HandleEyeFinityFOVScale(pcam, fRenderedTerminalFOV.Float());
    NiUpdateData UpdateData2(0.0f, false, false);
    pcam->Update(UpdateData2);
    SetCamera(pcam);
    if (pscreen->IsGeometry()) {
        SetScreenGeometry(pscreen->IsGeometry());
    }
    else if (pscreen->IsNode()) {
        NiAVObject* pChild = pscreen->IsNode()->GetAt(0);
        SetScreenGeometry(pChild->IsGeometry());
    }
    SetScreenParent(pgeom);
    // Gets the property type before the screen geometry.
    int32_t iAlphaType = NiAlphaProperty::GetType();
    GetScreenGeometry()->RemoveProperty(iAlphaType);
    NiAlphaProperty* pAlphaProperty = new NiAlphaProperty;
    GetScreenGeometry()->AttachProperty(pAlphaProperty);
    const NiBound* pWorldBound = &pgeom->GetWorldBound();
    float fradius = pWorldBound->m_fRadius * 3.0;
    if (!BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pgeom, fradius)) {
        NiPointLight* plight = new NiPointLight;
        pgeom->AttachChild(plight, 1);
        ::new (&kLightPos) NiPoint3(0.0f, -50.0f, 0.0f);
        plight->SetTranslate(kLightPos);
        plight->SetDiffuseColor(NiColor(0.75f, 0.75f, 0.75f));
        plight->SetSpecularColor(NiColor(1000.0f, 0.0f, 0.0f));
        plight->SetDimmer(1.0f);
        NiUpdateData ud(0.0f, false, false);
        plight->Update(ud);
        BSShaderManager::GetShadowSceneNode(1)->AddLight(plight, false);
        CreateLight();
    }
    FORenderedMenu::Initialize();
}

void FORenderedMenuRaceSex::OnMenuOpen() {
    NiPoint3 kUp;
    NiRect<float> port;
    appMain->RenderMenuBackground();
    ImageSpaceManager::bEOFEnabled = false;
    if (GetInitialized()) {
        NiAVObject* pscreenParent = FORenderedMenu::GetParentNodeToRender();
        NiUpdateData foo(0.0f, false, false);
        pscreenParent->Update(foo);
        RaceSexMenu* prsm = static_cast<RaceSexMenu*>(pMenuTile->GetMenu());
        if (GetMenuLight()) {
            GetMenuLight()->SetAppCulled(false);
        }
        FORenderedMenu::OnMenuOpen();
        BSShaderManager::GetShadowSceneNode(1)->AttachChild(FORenderedMenu::GetParentNodeToRender(), 1);
        Controls::GetInstance()->ClearUserActions();
        pPlayer->pCurrentProcess->SetWeaponDrawn(pPlayer, false);
        pPlayer->SetWalkRunFlagDirty();
        pPlayer->ClearMoveMode(1024u);
        pPlayer->GetAnimation(false)->ClearGroup(AGS_WHOLE_BODY, 0.0f);
        pPlayer->GetAnimation(false)->SkipUpdate(AGS_SKIP_ALL_BUT_SPECIAL_IDLE);
        pPlayer->GetAnimation(false)->PlayGroup(0, ACTION_START, -1, AGS_NONE);
        pPlayer->Update(0.0f);
        spPlayer3D = static_cast<NiNode*>(pPlayer->Get3D(false));
        ptSavedPlayerTranslation = spPlayer3D->GetTranslate();
        mtxSavedPlayerRotation = (*&spPlayer3D->m_kLocal).m_Rotate;
        ptSavedBip01Translation = spPlayer3D->GetObjectByName("Bip01")->GetTranslate();
        spPlayer3DParent = spPlayer3D->m_pkParent;
        bPlayer3DCulled = spPlayer3D->GetAppCulled();
        NiNode* pPlayerParent = new NiNode(0);
        spPlayerParent = pPlayerParent;
        BSShaderManager::GetShadowSceneNode(3)->AttachChild(spPlayerParent.m_pObject, 1);
        spPlayerLights = static_cast<NiAVObject*>(
            pModelLoader
                ->LoadFile("Meshes\\Terminals\\PlayerFaceLights01.NIF", LOD_MULT_NONE, true, false, false, false)
        );
        spPlayerParent->AttachChild(spPlayer3D.m_pObject, 1);
        spPlayerParent->AttachChild(spPlayerLights.m_pObject, 1);
        const NiBound* pWorldBound = &spPlayer3D->GetWorldBound();
        float fradiusPlayer = pWorldBound->m_fRadius * 3.0;
        NiNode* pLightsNode = spPlayerLights->IsNode();
        BSShaderManager::GetShadowSceneNode(3)->AddAllLightsFromNode_Recurse(pLightsNode, fradiusPlayer);
        spPlayerParent->SetRotate(3.1415927f, 0.0f, 0.0f, 1.0f);
        spPlayer3D->SetTranslate(0.0f, -5.0f, 0.0f);
        spPlayer3D->SetRotate(NiMatrix3::IDENTITY);
        NiUpdateData bar(0.0f, false, false);
        spPlayerParent->Update(bar);
        prsm->SetPlayerNode(spPlayerParent.m_pObject);
        NiCamera* pCamera = new NiCamera;
        spClonedPlayerCamera = pCamera;
        ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
        spClonedPlayerCamera->SetViewPort(port);
        spClonedPlayerCamera->SetMinNearPlaneDist(1.0f);
        NiPoint3 ptlook(0.0f, 1.0f, 0.0f);
        ::new (&kUp) NiPoint3(0.0f, 0.0f, 1.0f);
        spClonedPlayerCamera->LookAtWorldPoint(ptlook, kUp);
        NiFrustum fr(false);
        fr.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
        fr.m_fNear = BSSceneGraph::fNearDistance.Float();
        float fFarNearRatio = fr.m_fFar / fr.m_fNear;
        spClonedPlayerCamera->SetMaxFarNearRatio(fFarNearRatio);
        double dDefaultFOV = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369;
        float theta = fRenderedTerminalFOV.Float() * dDefaultFOV;
        fr.m_fLeft = -std::tan(theta);
        fr.m_fRight = std::tan(theta);
        fr.m_fBottom = -std::tan(theta);
        fr.m_fTop = std::tan(theta);
        fr.m_bOrtho = false;
        spClonedPlayerCamera->SetViewFrustum(fr);
        NiUpdateData UpdateData2(0.0f, false, false);
        spClonedPlayerCamera->Update(UpdateData2);
        spPlayer3D->SetAppCulled(false);
        prsm->RotatePlayer(0.5235988f, true);
        prsm->ZoomPlayer(0.0f, true);
        LightButton(prsm->GetActiveList());
    }
}

void FORenderedMenuRaceSex::OnMenuClose() {
    ImageSpaceManager::bEOFEnabled = true;
    if (GetInitialized()) {
        if (GetMenuLight()) {
            GetMenuLight()->SetAppCulled(true);
            NiUpdateData foo(0.0f, false, false);
            GetMenuLight()->Update(foo);
        }
        spPlayerParent->DetachChild(spPlayerLights.m_pObject);
        spPlayerParent->DetachChild(spPlayer3D.m_pObject);
        BSShaderManager::GetShadowSceneNode(3)->DetachChild(spPlayerParent.m_pObject);
        spPlayerParent = static_cast<NiNode*>(nullptr);
        BSShaderManager::GetShadowSceneNode(1)->DetachChild(FORenderedMenu::GetParentNodeToRender());
        spPlayer3DParent->AttachChild(spPlayer3D.m_pObject, 1);
        spPlayer3D->GetObjectByName("Bip01")->SetTranslate(ptSavedBip01Translation);
        spPlayer3D->SetRotate(mtxSavedPlayerRotation);
        spPlayer3D->SetTranslate(ptSavedPlayerTranslation);
        spPlayer3D->SetAppCulled(bPlayer3DCulled);
        spPlayer3D = static_cast<NiNode*>(nullptr);
        spClonedPlayerCamera = static_cast<NiCamera*>(nullptr);
        spGeometry = static_cast<NiNode*>(nullptr);
        BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
        FORenderedMenu::OnMenuClose();
        // No button lit.
        LightButton(static_cast<uint32_t>(-1));
        spPlayerLights = static_cast<NiAVObject*>(nullptr);
    }
}

void FORenderedMenuRaceSex::UpdateDistort(float afTime) {
    double dElapsed = afTime - fDistortStart;
    if (fRaceSexMenuDistortDuration.Float() < dElapsed) {
        fDistortStart = afTime;
        fDistortPos = static_cast<double>(rt::rand() % 1000) * 0.0010000000474974513;
    }
    if (bFakeDistortActive) {
        UpdateFakeDistort(afTime);
    }
    else if (fNextFakeDistortTime < static_cast<double>(afTime)) {
        TriggerFakeDistort();
    }
}

void FORenderedMenuRaceSex::DoEffect(
    BSRenderedTexture* apSource,
    BSRenderedTexture* apDest,
    NiRenderer::ClearFlags aeClearMode
) {
    // The flag is set true, then false: the menu's own pulse, distortion and tint are never applied.
    bool bApplyEffects = false;
    if (bApplyEffects) {
        ImageSpaceEffectTV::fPulse = fPulseCurrentBrighten;
        ImageSpaceEffectTV::fDistortVerticalScale = fDefaultDistortVerticalScale.Float();
        ImageSpaceEffectTV::fDistortVerticalPosition = fDefaultDistortHorizontalScale.Float();
        ImageSpaceEffectTV::fDistortHorizontalScale = fDistortPos;
        NiColorA kResult;
        ImageSpaceEffectTV::Tint = *GetTintColor(&kResult);
    }
    else {
        ImageSpaceEffectTV::fDistortVerticalScale = 0.0f;
        ImageSpaceEffectTV::fDistortVerticalPosition = 0.0f;
        ImageSpaceEffectTV::fDistortHorizontalScale = -1.0f;
        ImageSpaceEffectTV::fPulse = 1.0f;
        NiColorA kTint(1.0f, 1.0f, 1.0f, 1.0f);
        ImageSpaceEffectTV::Tint = kTint;
    }
    if (bFakeDistortActive && bApplyEffects) {
        ImageSpaceEffectTV::fFakeDistortVerticalScale = fDefaultDistortVerticalScale.Float();
        ImageSpaceEffectTV::fFakeDistortVerticalPosition = fDefaultDistortHorizontalScale.Float();
        ImageSpaceEffectTV::fFakeDistortHorizontalScale = fFakeDistortPosition;
    }
    else {
        ImageSpaceEffectTV::fFakeDistortVerticalScale = 0.0f;
        ImageSpaceEffectTV::fFakeDistortVerticalPosition = 0.0f;
        ImageSpaceEffectTV::fFakeDistortHorizontalScale = -1.0f;
    }
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    BSShaderManager::GetImageSpaceManager()
        ->RenderEffect(ImageSpaceManager::EFFECT_TV, pRenderer, apSource, apDest, nullptr, 1);
}

void FORenderedMenuRaceSex::UpdateFakeDistort(float afTime) {
    fFakeDistortPosition = (afTime - fFakeDistortStartTime) / fFakeDistortDuration;
    if (fFakeDistortPosition > fDefaultDistortVerticalScale.Float() + 1.0) {
        bFakeDistortActive = false;
        int32_t iRandom = rt::rand();
        fNextFakeDistortTime = static_cast<double>(iRandom % iDefaultDistortMaxInterval.Int()) + afTime;
    }
}

void FORenderedMenuRaceSex::SetupLights(float afScale) {
    const NiBound* worldBound = &spPlayer3D->GetWorldBound();
    float fRadius = static_cast<float>(worldBound->m_fRadius * 3.0 * afScale);
    NiNode* pPlayerParent = spPlayerParent.m_pObject;
    ShadowSceneNode* shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D);
    shadowSceneNode->AddAllLightsFromNode_Recurse(pPlayerParent, fRadius);
}

// The index compare is unsigned.
void FORenderedMenuRaceSex::LightButton(uint32_t aiButton) {
    for (uint32_t i = 0; i < 4; ++i) {
        if (spBtnGlow[i]) {
            spBtnGlow[i]->SetAppCulled(i != aiButton);
        }
    }
}

void FORenderedMenuRaceSex::TriggerFakeDistort() {
    bFakeDistortActive = true;
    fFakeDistortStartTime = static_cast<float>(appTimer.GetTime());
    fFakeDistortPosition = 0.0f;
}

void RaceSexMenu::SetPlayerNode(NiNode* apNode) {
    spPlayerNode = apNode;
}
