#include "Fallout/Interface/FORenderedMenuRaceSex.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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

void FORenderedMenuRaceSex::Draw(BSRenderedTexture* a1, NiRenderer::ClearFlags a2, BSRenderedTexture* a3) {
    // The background is always tinted.
    bool bTintBackground = true;
    NiRenderer* pr = NiRenderer::GetRenderer();
    NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
    NiPointer<BSRenderedTexture> spswap(
        TextureManager->BorrowRenderedTexture(Renderer_, BSTextureManager::TEX_INTERFACE_RACESEXMENU, 0, nullptr, 0)
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
    NiRenderTargetGroup* Group = spswap.m_pObject->GetGroup();
    BSRenderedTexture::StartOffscreen(7u, Group);
    BSCullingProcess a12(nullptr);
    InterfaceManager* v9 = InterfaceManager::Instance();
    NiPointer<BSShaderAccumulator> spaccum(v9->Get3dAccumulator());
    a12.SetAccumulator(spaccum.m_pObject);
    a12.PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(3);
    BSShaderUtil::AccumulateScene(spClonedPlayerCamera.m_pObject, ShadowSceneNode_, a12);
    BSShaderUtil::RenderScene(spClonedPlayerCamera.m_pObject, spaccum.m_pObject, false);
    a12.PopCullMode();
    a12.SetAccumulator(nullptr);
    spaccum.operator=(static_cast<BSShaderAccumulator*>(nullptr));
    BSRenderedTexture::StopOffscreen();
    FORenderedMenu::Draw(a1, NiRenderer::CLEAR_NONE, spswap.m_pObject);
    BSRenderedTexture* fB = spswap.m_pObject;
    BSTextureManager* v11 = BSShaderManager::TextureManagerPtr();
    v11->ReturnRenderedTexture(fB);
    spswap.operator=(static_cast<BSRenderedTexture*>(nullptr));
}

bool FORenderedMenuRaceSex::OnIdle() {
    Animation* animation;
    Animation* v3;
    if (FORenderedMenu::GetInitialized()) {
        animation = pPlayer->GetAnimation(false);
        animation->SkipUpdate(AGS_SKIP_ALL_BUT_SPECIAL_IDLE);
        pPlayer->UpdateAnimation();
        v3 = pPlayer->GetAnimation(false);
        pPlayer->UpdateAnimationMovement(v3, 0.0);
        return (bool)((uint8_t)FORenderedMenu::OnIdle());
    }

    Interface::EmergencyCloseAllMenusAndBreakStuff();
    return false;
}

bool FORenderedMenuRaceSex::Pick(bool a1, float* a2, float* a3) {
    XFNV_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
    InterfaceManager* v21 = InterfaceManager::Instance();
    if (a1) {
        char v20 = 1;
        for (int32_t i = 0; v20 && i < 4; ++i) {
            if (!spButtons[i].m_pObject || !spBtnGlow[i].m_pObject) {
                v20 = 0;
            }
        }
        if (v20) {
            NiPick apMemory(0, 8u);
            apMemory.SetObserveAppCullFlag(true);
            apMemory.m_eSortType = NiPick::SORT;
            apMemory.SetIsReturnTexture(false);
            int32_t v18 = -1;
            for (int32_t j = 0; v18 < 0 && j < 4; ++j) {
                NiPoint3 a2a;
                NiPoint3 a1a;
                apMemory.SetTarget(spButtons[j].m_pObject);
                int32_t ScriptProcessingDelay = static_cast<int32_t>(v21->sRealCursorPos.z);
                int32_t DataClipDist = static_cast<int32_t>(v21->sRealCursorPos.x);
                NiCamera* Camera = GetCamera();
                Camera->WindowPointToRay(DataClipDist, ScriptProcessingDelay, a1a, a2a, nullptr);
                if ((apMemory.PickObjects(a1a, a2a, false)) && ((&apMemory.m_pickResults)->GetAt(0))) {
                    v18 = j;
                }
            }
            if (v18 >= 0) {
                BSAudio* v5 = &BSAudio::QInstance();
                BSSoundHandle* SoundHandleByName = v5->GetSoundHandleByName(&arHandle, "UIMenuMode", 289u);
                SoundHandleByName->Play(false);
                RaceSexMenu* Menu_ = static_cast<RaceSexMenu*>(pMenuTile->GetMenu());
                Menu_->SetActiveList(static_cast<RaceSexMenu::ListID>(v18));
                return true;
            }
        }
    }
    return FORenderedMenu::Pick(a1, a2, a3);
}

void FORenderedMenuRaceSex::Unknown07() {
    GetMenuRoot();
}

void FORenderedMenuRaceSex::Unknown08() {
    FORenderedMenuRaceSex::Unknown07();
}

void FORenderedMenuRaceSex::Initialize() {
    NiAlphaProperty* v28;
    int32_t ScreenHeight;
    NiCamera* pcam;
    NiColor v46;
    NiColor v47;
    NiPoint3 dst;
    XFNV_INLINED_LOCAL(NiFixedString, v56, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v57, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v58, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v59, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v60, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v61, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v62, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v63, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiMatrix3 mat2;
    NiMatrix3 rot;
    NiMatrix3 mat1;
    NiRect<float> port;
    bFakeDistortActive = false;
    double Time = static_cast<double>(appTimer.iLastTime);
    int32_t v1 = rt::rand();
    fNextFakeDistortTime = static_cast<double>(v1 % gs_iDistortMaxInterval_InterfaceFX.Int()) + Time;
    fFakeDistortDuration = gs_fDistortDuration_InterfaceFX.Float();
    NiColorA newcolor(0.0f, 0.0f, 0.0f, 0.0f);
    SystemColorManager* Instance = SystemColorManager::GetInstance();
    if (Instance->GetColor(3, newcolor)) {
        SetTintColor(newcolor);
    }
    BSFadeNode* pgeom = nullptr;
    if (spGeometry.m_pObject) {
        pgeom = static_cast<BSFadeNode*>(spGeometry.m_pObject);
    }
    else {
        RaceSexMenu* pRaceSexMenu = static_cast<RaceSexMenu*>(pMenuTile->GetMenu());
        const char* v4 = pRaceSexMenu->GetProjectorFile();
        pgeom = static_cast<BSFadeNode*>(pModelLoader->LoadFile(v4, LOD_MULT_NONE, true, false, false, false));
        spGeometry.operator=(static_cast<NiNode*>(pgeom));
        SwapPlatformLanguageTextures(pgeom);
    }
    float* v5 = &fRaceSexMenuScale.Float();
    pgeom->SetScale(*v5);
    SetScreenParent(pgeom);
    NiFixedString* a10 = ::new (static_cast<void*>(&apString)) NiFixedString("Screen");
    NiAVObject* pscreen = pgeom->GetObjectByName(*a10);
    apString.~NiFixedString();
    SetScreenTexture(nullptr);
    bPulseAffectsLight = 0;
    spPlayer3D.operator=(static_cast<NiNode*>(nullptr));
    spClonedPlayerCamera.operator=(static_cast<NiCamera*>(nullptr));
    NiFixedString* a9 = ::new (static_cast<void*>(&v63)) NiFixedString("SexBtn:0");
    spButtons->operator=(pgeom->GetObjectByName(*a9));
    v63.~NiFixedString();
    NiFixedString* a8 = ::new (static_cast<void*>(&v62)) NiFixedString("RaceBtn:0");
    spButtons[1].operator=(pgeom->GetObjectByName(*a8));
    v62.~NiFixedString();
    NiFixedString* v40 = ::new (static_cast<void*>(&v61)) NiFixedString("FaceBtn:0");
    spButtons[2].operator=(pgeom->GetObjectByName(*v40));
    v61.~NiFixedString();
    NiFixedString* a7_8 = ::new (static_cast<void*>(&v60)) NiFixedString("BodyBtn:0");
    spButtons[3].operator=(pgeom->GetObjectByName(*a7_8));
    v60.~NiFixedString();
    NiFixedString* a7 = ::new (static_cast<void*>(&v59)) NiFixedString("SexGlow:0");
    spBtnGlow->operator=(pgeom->GetObjectByName(*a7));
    v59.~NiFixedString();
    NiFixedString* a6 = ::new (static_cast<void*>(&v58)) NiFixedString("RaceGlow:0");
    spBtnGlow[1].operator=(pgeom->GetObjectByName(*a6));
    v58.~NiFixedString();
    NiFixedString* a5 = ::new (static_cast<void*>(&v57)) NiFixedString("FaceGlow:0");
    spBtnGlow[2].operator=(pgeom->GetObjectByName(*a5));
    v57.~NiFixedString();
    NiFixedString* v35 = ::new (static_cast<void*>(&v56)) NiFixedString("BodyGlow:0");
    spBtnGlow[3].operator=(pgeom->GetObjectByName(*v35));
    v56.~NiFixedString();
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
        float angle = fRaceSexMenuHPos.Float();
        float* v8 = &fRaceSexMenuVPos.Float();
        pgeom->SetTranslate(fzoom, *v8, angle);
    }
    else {
        float* v7 = &fRaceSexMenuHPos.Float();
        pgeom->SetTranslate(fzoom, 0.0f, *v7);
    }
    NiUpdateData UpdateData1(0.0f, false, false);
    pgeom->Update(UpdateData1);
    void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
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
    float v32 = fr.m_fFar / fr.m_fNear;
    pcam->SetMaxFarNearRatio(v32);
    if (BSShaderManager::bLetterbox) {
        ScreenHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        ScreenHeight = Renderer::GetScreenHeight();
    }
    float fHeightByWidth = static_cast<double>(ScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float faspectDelta = 0.75f;
    double v30 = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369;
    float theta = fRenderedTerminalFOV.Float() * v30;
    double v9 = std::tan(theta);
    fr.m_fLeft = -v9 * faspectDelta;
    double v10 = std::tan(theta);
    fr.m_fRight = v10 * faspectDelta;
    double v11 = std::tan(theta);
    fr.m_fBottom = -v11 * fHeightByWidth * faspectDelta;
    double v12 = std::tan(theta);
    fr.m_fTop = v12 * fHeightByWidth * faspectDelta;
    fr.m_bOrtho = false;
    pcam->SetViewFrustum(fr);
    float* v13 = &fRenderedTerminalFOV.Float();
    NiCamera::HandleEyeFinityFOVScale(pcam, *v13);
    NiUpdateData UpdateData2(0.0f, false, false);
    pcam->Update(UpdateData2);
    SetCamera(pcam);
    if (pscreen->IsGeometry()) {
        NiGeometry* v14 = pscreen->IsGeometry();
        SetScreenGeometry(v14);
    }
    else {
        if (pscreen->IsNode()) {
            NiNode* v15 = pscreen->IsNode();
            NiAVObject* v29 = v15->GetAt(0);
            NiGeometry* v16 = v29->IsGeometry();
            SetScreenGeometry(v16);
        }
    }
    SetScreenParent(pgeom);
    // Gets the property type before the screen geometry.
    int32_t iAlphaType = NiAlphaProperty::GetType();
    GetScreenGeometry()->RemoveProperty(iAlphaType);
    NiAlphaProperty* v51 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (v51) {
        v28 = ::new (v51) NiAlphaProperty;
    }
    else {
        v28 = nullptr;
    }
    NiProperty* pkProperty = (NiProperty*)v28;
    GetScreenGeometry()->AttachProperty(v28);
    const NiBound* WorldBound = &pgeom->GetWorldBound();
    float fradius = WorldBound->m_fRadius * 3.0;
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    if (!ShadowSceneNode_->AddAllLightsFromNode_Recurse(pgeom, fradius)) {
        NiPointLight* plight;
        NiPointLight* v49 = static_cast<NiPointLight*>(NiMemObject::operator new(sizeof(NiPointLight)));
        if (v49) {
            plight = ::new (v49) NiPointLight;
        }
        else {
            plight = nullptr;
        }
        pgeom->AttachChild(plight, 1);
        NiPoint3* v19 = ::new (&dst) NiPoint3(0.0f, -50.0f, 0.0f);
        plight->SetTranslate(*v19);
        plight->SetDiffuseColor(*XFNV_RECONSTRUCT(NiColor)(&v47, 0.75f, 0.75f, 0.75f));
        plight->SetSpecularColor(*XFNV_RECONSTRUCT(NiColor)(&v46, 1000.0f, 0.0f, 0.0f));
        plight->SetDimmer(1.0f);
        NiUpdateData ud(0.0f, false, false);
        plight->Update(ud);
        ShadowSceneNode* v22 = BSShaderManager::GetShadowSceneNode(1);
        v22->AddLight(plight, false);
        CreateLight();
    }
    FORenderedMenu::Initialize();
}

void FORenderedMenuRaceSex::OnMenuOpen() {
    NiPoint3 v25;
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiRect<float> port;
    appMain->RenderMenuBackground();
    ImageSpaceManager::bEOFEnabled = false;
    if (GetInitialized()) {
        NiCamera* v16;
        NiNode* v20;
        NiAVObject* pscreenParent = FORenderedMenu::GetParentNodeToRender();
        NiUpdateData foo(0.0f, false, false);
        pscreenParent->Update(foo);
        RaceSexMenu* prsm = static_cast<RaceSexMenu*>(pMenuTile->GetMenu());
        if (GetMenuLight()) {
            GetMenuLight()->SetAppCulled(false);
        }
        FORenderedMenu::OnMenuOpen();
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
        ShadowSceneNode_->AttachChild(FORenderedMenu::GetParentNodeToRender(), 1);
        Controls* Instance = Controls::GetInstance();
        Instance->ClearUserActions();
        BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
        CurrentAIProcess->SetWeaponDrawn(pPlayer, false);
        pPlayer->SetWalkRunFlagDirty();
        pPlayer->ClearMoveMode(1024u);
        Animation* Animation_0 = pPlayer->GetAnimation(false);
        Animation_0->ClearGroup(AGS_WHOLE_BODY, 0.0f);
        Animation* v4 = pPlayer->GetAnimation(false);
        v4->SkipUpdate(AGS_SKIP_ALL_BUT_SPECIAL_IDLE);
        Animation* v5 = pPlayer->GetAnimation(false);
        v5->PlayGroup(0, ACTION_START, -1, AGS_NONE);
        pPlayer->Update(0.0f);
        spPlayer3D.operator=(static_cast<NiNode*>(pPlayer->Get3D(false)));
        ptSavedPlayerTranslation = spPlayer3D.m_pObject->GetTranslate();
        mtxSavedPlayerRotation = (*&spPlayer3D.m_pObject->m_kLocal).m_Rotate;
        NiNode* v22 = spPlayer3D.m_pObject;
        NiFixedString* a8 = ::new (static_cast<void*>(&apString)) NiFixedString("Bip01");
        ptSavedBip01Translation = v22->GetObjectByName(*a8)->GetTranslate();
        apString.~NiFixedString();
        spPlayer3DParent.operator=(spPlayer3D.m_pObject->m_pkParent);
        bPlayer3DCulled = spPlayer3D.m_pObject->GetAppCulled();
        void* apMemory = NiMemObject::operator new(sizeof(NiNode));
        if (apMemory) {
            v20 = ::new (apMemory) NiNode(0);
        }
        else {
            v20 = nullptr;
        }
        NiRefObject* v29 = (NiRefObject*)v20;
        spPlayerParent.operator=(v20);
        ShadowSceneNode* v6 = BSShaderManager::GetShadowSceneNode(3);
        v6->AttachChild(spPlayerParent.m_pObject, 1);
        spPlayerLights.operator=(static_cast<NiAVObject*>(pModelLoader->LoadFile(
            "Meshes\\Terminals\\PlayerFaceLights01.NIF", LOD_MULT_NONE, true, false, false, false
        )));
        NiNode* v19 = spPlayerParent.m_pObject;
        v19->AttachChild(spPlayer3D.m_pObject, 1);
        NiNode* v18 = spPlayerParent.m_pObject;
        v18->AttachChild(spPlayerLights.m_pObject, 1);
        const NiBound* WorldBound = &spPlayer3D.m_pObject->GetWorldBound();
        float fradiusPlayer = WorldBound->m_fRadius * 3.0;
        NiAVObject* v17 = spPlayerLights.m_pObject;
        NiNode* a3 = v17->IsNode();
        ShadowSceneNode* v8 = BSShaderManager::GetShadowSceneNode(3);
        v8->AddAllLightsFromNode_Recurse(a3, fradiusPlayer);
        spPlayerParent.m_pObject->SetRotate(3.1415927f, 0.0f, 0.0f, 1.0f);
        spPlayer3D.m_pObject->SetTranslate(0.0f, -5.0f, 0.0f);
        spPlayer3D.m_pObject->SetRotate(NiMatrix3_IDENTITY);
        NiUpdateData bar(0.0f, false, false);
        spPlayerParent.m_pObject->Update(bar);
        NiNode* a4 = spPlayerParent.m_pObject;
        prsm->SetPlayerNode(a4);
        NiCamera* v26 = static_cast<NiCamera*>(NiMemObject::operator new(sizeof(NiCamera)));
        if (v26) {
            v16 = ::new (v26) NiCamera;
        }
        else {
            v16 = nullptr;
        }
        NiRefObject* v27 = (NiRefObject*)v16;
        spClonedPlayerCamera.operator=(v16);
        ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
        spClonedPlayerCamera.m_pObject->SetViewPort(port);
        spClonedPlayerCamera.m_pObject->SetMinNearPlaneDist(1.0f);
        NiPoint3 ptlook(0.0f, 1.0f, 0.0f);
        NiPoint3* v9 = ::new (&v25) NiPoint3(0.0f, 0.0f, 1.0f);
        spClonedPlayerCamera.m_pObject->LookAtWorldPoint(ptlook, *v9);
        NiFrustum fr(false);
        SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
        fr.m_fFar = WorldSceneGraph->GetFarDistance();
        fr.m_fNear = BSSceneGraph::fNearDistance.Float();
        float v14 = fr.m_fFar / fr.m_fNear;
        spClonedPlayerCamera.m_pObject->SetMaxFarNearRatio(v14);
        double v13 = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369;
        float theta = fRenderedTerminalFOV.Float() * v13;
        fr.m_fLeft = -std::tan(theta);
        fr.m_fRight = std::tan(theta);
        fr.m_fBottom = -std::tan(theta);
        fr.m_fTop = std::tan(theta);
        fr.m_bOrtho = false;
        spClonedPlayerCamera.m_pObject->SetViewFrustum(fr);
        NiUpdateData UpdateData2(0.0f, false, false);
        spClonedPlayerCamera.m_pObject->Update(UpdateData2);
        spPlayer3D.m_pObject->SetAppCulled(false);
        prsm->RotatePlayer(0.5235988f, true);
        prsm->ZoomPlayer(0.0f, true);
        LightButton(prsm->GetActiveList());
    }
}

void FORenderedMenuRaceSex::OnMenuClose() {
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    ImageSpaceManager::bEOFEnabled = true;
    if (GetInitialized()) {
        if (GetMenuLight()) {
            GetMenuLight()->SetAppCulled(true);
            NiUpdateData foo(0.0f, false, false);
            GetMenuLight()->Update(foo);
        }
        NiNode* v10 = spPlayerParent.m_pObject;
        v10->DetachChild(spPlayerLights.m_pObject);
        NiNode* v9 = spPlayerParent.m_pObject;
        v9->DetachChild(spPlayer3D.m_pObject);
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(3);
        ShadowSceneNode_->DetachChild(spPlayerParent.m_pObject);
        spPlayerParent.operator=(static_cast<NiNode*>(nullptr));
        ShadowSceneNode* v2 = BSShaderManager::GetShadowSceneNode(1);
        v2->DetachChild(FORenderedMenu::GetParentNodeToRender());
        NiNode* v8 = spPlayer3DParent.m_pObject;
        v8->AttachChild(spPlayer3D.m_pObject, 1);
        NiNode* v7 = spPlayer3D.m_pObject;
        NiFixedString* v6 = ::new (static_cast<void*>(&apString)) NiFixedString("Bip01");
        NiAVObject* v3 = v7->GetObjectByName(*v6);
        v3->SetTranslate(ptSavedBip01Translation);
        apString.~NiFixedString();
        spPlayer3D.m_pObject->SetRotate(mtxSavedPlayerRotation);
        spPlayer3D.m_pObject->SetTranslate(ptSavedPlayerTranslation);
        spPlayer3D.m_pObject->SetAppCulled(bPlayer3DCulled);
        spPlayer3D.operator=(static_cast<NiNode*>(nullptr));
        spClonedPlayerCamera.operator=(static_cast<NiCamera*>(nullptr));
        spGeometry.operator=(static_cast<NiNode*>(nullptr));
        ShadowSceneNode* v4 = BSShaderManager::GetShadowSceneNode(1);
        v4->RemoveAllLights();
        FORenderedMenu::OnMenuClose();
        // No button lit.
        LightButton(static_cast<uint32_t>(-1));
        spPlayerLights.operator=(static_cast<NiAVObject*>(nullptr));
    }
}

void FORenderedMenuRaceSex::UpdateDistort(float a1) {
    double v2 = a1 - fDistortStart;
    if (fRaceSexMenuDistortDuration.Float() < v2) {
        fDistortStart = a1;
        fDistortPos = static_cast<double>(rt::rand() % 1000) * 0.0010000000474974513;
    }
    if (bFakeDistortActive) {
        UpdateFakeDistort(a1);
    }
    else {
        if (fNextFakeDistortTime < static_cast<double>(a1)) {
            TriggerFakeDistort();
        }
    }
}

void FORenderedMenuRaceSex::DoEffect(BSRenderedTexture* a1, BSRenderedTexture* a2, NiRenderer::ClearFlags a3) {
    // The flag is set true, then false, as the game does: the menu's own pulse, distortion and tint are never applied.
    bool bApplyEffects = false;
    if (bApplyEffects) {
        ImageSpaceEffectTV::fPulse = fPulseCurrentBrighten;
        ImageSpaceEffectTV::fDistortVerticalScale = gs_fDistortVerticalScale_InterfaceFX.Float();
        ImageSpaceEffectTV::fDistortVerticalPosition = gs_fDistortHorizontalScale_InterfaceFX.Float();
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
        ImageSpaceEffectTV::fFakeDistortVerticalScale = gs_fDistortVerticalScale_InterfaceFX.Float();
        ImageSpaceEffectTV::fFakeDistortVerticalPosition = gs_fDistortHorizontalScale_InterfaceFX.Float();
        ImageSpaceEffectTV::fFakeDistortHorizontalScale = fFakeDistortPosition;
    }
    else {
        ImageSpaceEffectTV::fFakeDistortVerticalScale = 0.0f;
        ImageSpaceEffectTV::fFakeDistortVerticalPosition = 0.0f;
        ImageSpaceEffectTV::fFakeDistortHorizontalScale = -1.0f;
    }
    NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
    ImageSpaceManager_->RenderEffect(ImageSpaceManager::EFFECT_TV, Renderer_, a1, a2, nullptr, 1);
}

void FORenderedMenuRaceSex::UpdateFakeDistort(float a1) {
    float v4 = a1 - fFakeDistortStartTime;
    fFakeDistortPosition = v4 / fFakeDistortDuration;
    if (fFakeDistortPosition > gs_fDistortVerticalScale_InterfaceFX.Float() + 1.0) {
        bFakeDistortActive = false;
        int32_t v2 = rt::rand();
        fNextFakeDistortTime = static_cast<double>(v2 % gs_iDistortMaxInterval_InterfaceFX.Int()) + a1;
    }
}

void FORenderedMenuRaceSex::SetupLights(float afScale) {
    NiNode* v2 = (&spPlayer3D)->m_pObject;
    const NiBound* worldBound = &v2->GetWorldBound();
    float v7 = (float)((worldBound->m_fRadius * 3.0) * afScale);
    NiNode* v5 = (&spPlayerParent)->m_pObject;
    ShadowSceneNode* shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D);
    shadowSceneNode->AddAllLightsFromNode_Recurse(v5, v7);
}

// The index compare is unsigned.
void FORenderedMenuRaceSex::LightButton(uint32_t aiButton) {
    for (uint32_t i = 0; i < 4; ++i) {
        if (spBtnGlow[i].operator->()) {
            spBtnGlow[i].operator->()->SetAppCulled(i != aiButton);
        }
    }
}

void FORenderedMenuRaceSex::TriggerFakeDistort() {
    bFakeDistortActive = true;
    fFakeDistortStartTime = (float)appTimer.GetTime();
    fFakeDistortPosition = 0.0f;
}

Controls* Controls::GetInstance() {
    return Controls::pInstance;
}

void RaceSexMenu::SetPlayerNode(NiNode* apNode) {
    spPlayerNode = apNode;
}
