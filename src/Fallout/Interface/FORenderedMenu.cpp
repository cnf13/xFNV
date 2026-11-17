#include "Fallout/Interface/FORenderedMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSNoise1D.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ImageSpaceEffectInterface.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fDefaultBurstDuration("fDefaultBurstDuration:InterfaceFX", 200.0f);
SettingT<INISettingCollection> fDefaultBurstIntensity("fDefaultBurstIntensity:InterfaceFX", 3.0f);
SettingT<INISettingCollection> fMiniBurstDuration("fMiniBurstDuration:InterfaceFX", 150.0f);
SettingT<INISettingCollection> fMiniBurstIntensity("fMiniBurstIntensity:InterfaceFX", 2.0f);
SettingT<INISettingCollection> fDefaultVertHoldDuration("fDefaultVertHoldDuration:InterfaceFX", 500.0f);
SettingT<INISettingCollection> fDefaultVertHoldSpeed("fDefaultVertHoldSpeed:InterfaceFX", 5.5f);
SettingT<INISettingCollection> fDefaultShudderDuration("fDefaultShudderDuration:InterfaceFX", 250.0f);
SettingT<INISettingCollection> fDefaultShudderIntensity("fDefaultShudderIntensity:InterfaceFX", 0.05f);
SettingT<INISettingCollection> fDefaultShudderFrequency("fDefaultShudderFrequency:InterfaceFX", 0.05f);
SettingT<INISettingCollection> fPulseBrightenIntensity("fPulseBrightenIntensity:InterfaceFX", 0.1f);
SettingT<INISettingCollection> fPulseRadiusIntensity("fPulseRadiusIntensity:InterfaceFX", 0.02f);
SettingT<INISettingCollection> gs_fPulseRate_InterfaceFX("fPulseRate:InterfaceFX", 0.00025f);
SettingT<INISettingCollection> fVertHoldChance("fVertHoldChance:InterfaceFX", 0.05f);
SettingT<INISettingCollection> fShudderChance("fShudderChance:InterfaceFX", 0.2f);
SettingT<INISettingCollection> fScreenLightBurstModifier("fScreenLightBurstModifier:InterfaceFX", 0.25f);
SettingT<INISettingCollection> fScreenLightPulseModifier("fScreenLightPulseModifier:InterfaceFX", 1.0f);
SettingT<INISettingCollection> fScreenLightVertHoldRate("fScreenLightVertHoldRate:InterfaceFX", 0.05f);
static SettingT<INISettingCollection> fScreenLightBaseIntensity("fScreenLightBaseIntensity:InterfaceFX", 0.3f);
SettingT<INISettingCollection> fScreenLightRadius("fScreenLightRadius:InterfaceFX", 32.0f);
SettingT<INISettingCollection> fDefaultBlurRadius("fDefaultBlurRadius:InterfaceFX", 0.3f);
SettingT<INISettingCollection> fDefaultBlurIntensity("fDefaultBlurIntensity:InterfaceFX", 0.2f);
static SettingT<INISettingCollection> fDefaultScanlineScale("fDefaultScanlineFrequency:InterfaceFX", 100.0f);
SettingT<INISettingCollection> gs_fDistortVerticalScale_InterfaceFX("fDistortVerticalScale:InterfaceFX", 5.0f);
SettingT<INISettingCollection> gs_fDistortHorizontalScale_InterfaceFX("fDistortHorizontalScale:InterfaceFX", 1.0f);
SettingT<INISettingCollection> gs_fDistortDuration_InterfaceFX("fDistortDuration:InterfaceFX", 500.0f);
SettingT<INISettingCollection> gs_iDistortMaxInterval_InterfaceFX("iDistortMaxInterval:InterfaceFX", 4500);
SettingT<INISettingCollection>
    gs_sScanlineTexture_InterfaceFX("sScanlineTexture:InterfaceFX", "Data\\Textures\\Pipboy3000\\PipboyScanlines.dds");

FORenderedMenu::FORenderedMenu()
  : spScreenTriShape(nullptr), spScreenParent(nullptr), spPartialScreenGeometry(nullptr), spDefaultTex(nullptr),
    spMenuRoot(nullptr), pMenuTile(nullptr), spLight(nullptr), pLightShadowSceneNode(nullptr), spCamera(nullptr),
    bInitialized(false), bScaledTexCoords(false), bUsePartialScreenGeometry(false), ScreenLightColor(0.0f, 0.0f, 0.0f),
    bBurstActive(false), bVertHoldActive(false), bShudderActive(false), fPulseCurrentBrighten(0.0f),
    fPulseCurrentRadius(0.0f), bPulseAffectsLight(0), bDistortActive(false), BackgroundColor(0.0f, 0.0f, 0.0f, 0.0f),
    bMenuOpen(false), bMenuRendering(0), spScanlinesTexture(nullptr), TintColor(0.0f, 0.0f, 0.0f, 0.0f),
    spAccumulator(nullptr) {
    double fTime = static_cast<double>(appTimer.iLastTime);
    fNextDistortTime = static_cast<double>(rt::rand() % gs_iDistortMaxInterval_InterfaceFX.Int()) + fTime;
    fDistortDuration = gs_fDistortDuration_InterfaceFX.Float();
}

FORenderedMenu::~FORenderedMenu() {
    FORenderedMenu::Shutdown();
}

void FORenderedMenu::Draw(
    BSRenderedTexture* apTarget,
    NiRenderer::ClearFlags aeClearMode,
    BSRenderedTexture* apSwapTarget
) {
    NiRect<float> kViewport;
    NiFrustum oldfrust;
    NiFrustum frust;
    if (bInitialized) {
        if (spMenuRoot.m_pObject) {
            bool bconsoleCulled;
            NiRenderer* pr = NiRenderer::GetRenderer();
            InterfaceManager* pim = InterfaceManager::Instance();
            if (pim->IsDebugTextVisible()) {
                pim->GetDebugTextRoot()->SetAppCulled(true);
            }
            SceneGraph* pscene_graph = pim->GetSceneGraph();
            BSCullingProcess* pculler = pscene_graph->pCuller;
            pculler->SetAccumulator(static_cast<NiAccumulator*>(spAccumulator.m_pObject));
            NiColorA prevcolor(0.0f, 0.0f, 0.0f, 0.0f);
            pr->GetBackgroundColor(prevcolor);
            pr->SetBackgroundColor(BackgroundColor);
            NiCamera* pcamera = pscene_graph->GetCamera();
            NiPointer<BSRenderedTexture> sptempRenderTarget(static_cast<BSRenderedTexture*>(nullptr));
            if (apSwapTarget) {
                sptempRenderTarget.operator=(apSwapTarget);
            }
            else {
                NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
                sptempRenderTarget.operator=(TextureManager->BorrowRenderedTexture(
                    Renderer_, BSTextureManager::TEX_INTERFACE_TEMP, 0, nullptr, 0
                ));
            }
            if (BSRenderedTexture::QOutsideFrame()) {
                NiRenderTargetGroup* Group = sptempRenderTarget.m_pObject->GetGroup();
                BSRenderedTexture::StartOffscreen(aeClearMode, Group);
                // The viewport is built with NiColorA's constructor, components in the order left, right, top, bottom.
                ::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
                pcamera->SetViewPort(kViewport);
            }
            frust = pcamera->GetViewFrustum();
            oldfrust = frust;
            frust.m_bOrtho = true;
            frust.m_fLeft = 0.0f;
            frust.m_fRight = 1280.0f;
            frust.m_fTop = 960.0f;
            frust.m_fBottom = 0.0f;
            frust.m_fNear = 0.0f;
            frust.m_fFar = 10000.0f;
            pcamera->SetViewFrustum(frust);
            pr->SetCameraData(pcamera);
            Tile* pcursor = (Tile*)pim->pCursor;
            bool bcullcursor = false;
            if (pcursor) {
                bcullcursor = pcursor->GetModel()->GetAppCulled();
                pcursor->GetModel()->SetAppCulled(true);
            }
            NiAVObject* pconsoleRoot = (NiAVObject*)Interface::GetDebugTextRoot();
            if (pconsoleRoot) {
                bconsoleCulled = pconsoleRoot->GetAppCulled();
            }
            else {
                bconsoleCulled = false;
            }
            if (pconsoleRoot) {
                pconsoleRoot->SetAppCulled(true);
            }
            Tile* pHUD = (Tile*)HUDMainMenu::pme->pMenu;
            Tile* MenuTile = pHUD->GetMenuTile();
            bool bHUDCulled = MenuTile->GetModel()->GetAppCulled();
            Tile* v8 = pHUD->GetMenuTile();
            v8->GetModel()->SetAppCulled(true);
            if (this != Interface::GetPipboy()) {
                static_cast<NiAVObject*>(spMenuRoot.m_pObject)->NiAVObject::SetAppCulled(false);
            }
            HideMessageQuantityTutorialMenus();
            pculler->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
            NiAVObject* fB = static_cast<NiAVObject*>(spMenuRoot.m_pObject);
            BSShaderUtil::AccumulateScene(pscene_graph->GetCamera(), fB, *pculler);
            BSShaderAccumulator* fBa = static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject);
            BSShaderUtil::RenderScene(pscene_graph->GetCamera(), fBa, false);
            pculler->PopCullMode();
            UnhideMessageQuantityTutorialMenus();
            pculler->SetAccumulator(nullptr);
            if (this != Interface::GetPipboy()) {
                static_cast<NiAVObject*>(spMenuRoot.m_pObject)->NiAVObject::SetAppCulled(true);
            }
            Tile* v9 = pHUD->GetMenuTile();
            v9->GetModel()->SetAppCulled(bHUDCulled);
            if (pcursor) {
                pcursor->GetModel()->SetAppCulled(bcullcursor);
            }
            if (pconsoleRoot) {
                pconsoleRoot->SetAppCulled(bconsoleCulled);
            }
            BSRenderedTexture::StopOffscreen();
            DoEffect(sptempRenderTarget.m_pObject, apTarget, aeClearMode);
            if (!apSwapTarget) {
                BSRenderedTexture* fA = sptempRenderTarget.m_pObject;
                BSTextureManager* v10 = BSShaderManager::TextureManagerPtr();
                v10->ReturnRenderedTexture(fA);
            }
            pr->SetBackgroundColor(prevcolor);
            pcamera->SetViewFrustum(oldfrust);
            if (bUsePartialScreenGeometry) {
                ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
                ImageSpaceManager_->EnablePartialRender(false, nullptr);
            }
            PostDraw();
            if (pim->IsDebugTextVisible()) {
                pim->GetDebugTextRoot()->SetAppCulled(false);
            }
        }
    }
}

bool FORenderedMenu::OnIdle() {
    float Time = static_cast<float>(appTimer.GetTime());
    bool bMenuOpen = this->bMenuOpen;
    InterfaceManager* v1 = InterfaceManager::Instance();
    this->bMenuOpen = v1->IsInRenderedMenu();
    if (!this->bMenuOpen && !bMenuOpen) {
        return false;
    }
    UpdateLight(Time);
    if (bBurstActive) {
        UpdateBurst(Time);
    }
    if (bVertHoldActive) {
        UpdateVertHold(Time);
    }
    if (bShudderActive) {
        UpdateShudder(Time);
    }
    UpdatePulse(Time);
    if (bDistortActive) {
        UpdateDistort(Time);
    }
    else {
        if (fNextDistortTime < static_cast<double>(Time)) {
            SetScanlineVisible();
        }
    }
    return true;
}

bool FORenderedMenu::Pick(bool abPressed, float* apX, float* apY) {
    NiPoint3 a2;
    NiPoint3 a1;
    NiCamera* Camera;
    char v22 = 0;
    static_cast<NiAVObject*>(spMenuRoot.m_pObject)->NiAVObject::SetAppCulled(false);
    InterfaceManager* v23 = InterfaceManager::Instance();
    if (!spScreenTriShape.m_pObject) {
        return false;
    }
    NiPick kPick(0, 8u);
    if (GetCamera()) {
        Camera = GetCamera();
    }
    else {
        Camera = Main::GetWorldSceneGraph()->GetCamera();
    }
    kPick.SetObserveAppCullFlag(true);
    kPick.SetTarget(spScreenTriShape.m_pObject);
    kPick.m_eSortType = NiPick::SORT;
    kPick.SetIsReturnTexture(true);
    int32_t ScriptProcessingDelay = static_cast<int32_t>(v23->sRealCursorPos.z);
    double DataClipDist = v23->sRealCursorPos.x;
    Camera->WindowPointToRay(static_cast<int32_t>(DataClipDist), ScriptProcessingDelay, a1, a2, nullptr);
    if (kPick.PickObjects(a1, a2, false)) {
        NiPick::Record* pPickResult = (&kPick.m_pickResults)->GetAt(0);
        if (pPickResult) {
            if (pPickResult->GetAVObject() == spScreenTriShape.m_pObject) {
                // The record's texture coordinates, copied out of m_texture.
                float fUV_X = pPickResult->m_texture.x;
                float fUV_Y = pPickResult->m_texture.y;
                if (Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0)) {
                    float fUV_Xa = fUV_X * 0.6299999952316284;
                    fUV_X = fUV_Xa + 0.3700000047683716;
                    float fUV_Ya = fUV_Y * 0.6499999761581421;
                    fUV_Y = fUV_Ya + 0.3400000035762787;
                }
                NiRenderer* pRenderer = NiRenderer::GetRenderer();
                float uiHeight = static_cast<float>(pRenderer->GetDefaultBackBuffer()->GetHeight());
                float v16 = uiHeight * 1.3333330154418945;
                *apX = fUV_X * v16;
                *apY = fUV_Y * uiHeight;
                v22 = 1;
            }
        }
    }
    bool v11 = v22;
    return v11;
}

void FORenderedMenu::Initialize() {
    BSShaderAccumulator* v2;
    BSShaderAccumulator* apMemory =
        static_cast<BSShaderAccumulator*>(NiMemObject::operator new(sizeof(BSShaderAccumulator)));
    if (apMemory) {
        v2 = ::new (apMemory) BSShaderAccumulator(100, 1u, 759u);
    }
    else {
        v2 = nullptr;
    }
    spAccumulator.operator=(static_cast<NiRefObject*>(v2));
    static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject)
        ->BSShaderAccumulator::SetRenderMode(BSShaderManager::BSSM_RENDER_INTERFACE);
    NiPointer<NiTexture> spDefaultTex(static_cast<NiTexture*>(nullptr));
    BSShaderManager::GetTexture("Data\\Textures\\Pipboy3000\\screen.dds", true, spDefaultTex, true, false);
    NiTexture* pdefaultTex = spDefaultTex.m_pObject;
    if (!pdefaultTex) {
        Warning("MENUS: Default renderedmenu texture NOT FOUND.  Rendered menu may not work.");
    }
    SetDefaultTex(pdefaultTex);
    fBlurRadius = fDefaultBlurRadius.Float();
    fBlurIntensity = fDefaultBlurIntensity.Float();
    const char* v1 = gs_sScanlineTexture_InterfaceFX.String();
    BSShaderManager::GetTexture(v1, true, spScanlinesTexture, false, false);
    bInitialized = true;
}

void FORenderedMenu::Shutdown() {
    this->ClearLight();
    this->spScreenTriShape = nullptr;
    this->spMenuRoot = nullptr;
    this->spPartialScreenGeometry = nullptr;
    this->spDefaultTex = nullptr;
    this->spLight = nullptr;
    this->spCamera = nullptr;
    this->spAccumulator = nullptr;
    this->bInitialized = false;
}

void FORenderedMenu::OnMenuOpen() {
    if (pMenuTile) {
        Tile* Parent = pMenuTile;
        Menu* Menu_ = Parent->GetMenu();
        Menu_->SetRenderedMenu();
    }
    Tile* MenusRoot = Interface::GetMenusRoot();
    MenusRoot->UpdateClipwindows();
    if (spLight.operator->()) {
        NiAVObject* v4 = static_cast<NiAVObject*>(spLight.operator->());
        v4->SetAppCulled(false);
    }
    bMenuRendering = 1;
}

void FORenderedMenu::OnMenuClose() {
    NiRefObject* v1;
    bMenuRendering = (uint8_t)0;
    bBurstActive = false;
    bVertHoldActive = false;
    if ((&spLight)->operator->()) {
        v1 = (&spLight)->operator->();
        static_cast<NiAVObject*>(v1)->SetAppCulled(true);
    }
    SetScreenTexture(nullptr);
}

void FORenderedMenu::UpdateBurst(float afTime) {
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

void FORenderedMenu::UpdateVertHold(float afTime) {
    float v2 = afTime - fVertHoldStartTime;
    if (!(fVertHoldDuration < ((double)v2))) {
        fVertHoldCurrentPosition = (v2 * fVertHoldSpeed);
        if ((fVertHoldCurrentPosition >= 1.0) || (fVertHoldCurrentPosition < 0.0)) {
            fVertHoldCurrentPosition = (fVertHoldCurrentPosition - FloatFloor(fVertHoldCurrentPosition));
        }
    }
    else {
        bVertHoldActive = false;
        static_cast<FOPipboyManager*>(this)->SetShudderHoldParams(250.0f, 0.05000000074505806f, 0.05000000074505806f);
    }
}

void FORenderedMenu::UpdateShudder(float afTime) {
    float v2;
    float v3;
    float v4 = afTime - fShudderStartTime;
    if (!(fShudderDuration < ((double)v4))) {
        v3 = (float)(1.0 - (v4 / fShudderDuration));
        v2 = v4 * fShudderFrequency;
        fShudderCurrentPosition = ((std::sinf(v2) * fShudderIntensity) * v3);
    }
    else {
        bShudderActive = false;
    }
}

void FORenderedMenu::UpdatePulse(float afTime) {
    float a1 = afTime * (&gs_fPulseRate_InterfaceFX)->Float();
    double v2 = (double)BSNoise1D::Noise(a1);
    float v5 = (float)(v2 + v2);
    fPulseCurrentBrighten = (v5 * (&::fPulseBrightenIntensity)->Float());
    fPulseCurrentRadius = (v5 * (&::fPulseRadiusIntensity)->Float());
}

void FORenderedMenu::UpdateLight(float afTime) {
    NiColor finalColor;
    NiColor flightAdd;
    if (spLight.m_pObject) {
        flightAdd = NiColor_BLACK;
        if (bPulseAffectsLight) {
            double v5;
            double v7;
            if (bBurstActive) {
                v7 = fBurstCurrentIntensity * fScreenLightBurstModifier.Float();
            }
            else {
                v7 = 0.0;
            }
            float fburstintensity = v7;
            float fpulseintensity = fPulseCurrentBrighten * fScreenLightPulseModifier.Float();
            if (bVertHoldActive) {
                float v6 = afTime * fScreenLightVertHoldRate.Float();
                v5 = std::sinf(v6) + 0.5;
            }
            else {
                v5 = 1.0;
            }
            float fvertintensity = v5;
            float flightMod = (fburstintensity + fpulseintensity) * fvertintensity;
            float X = fScreenLightFlickerIntensity;
            flightAdd = ScreenLightColor * flightMod * X;
        }
        (ScreenLightColor * fScreenLightBaseIntensity).Add(&finalColor, &flightAdd);
        static_cast<NiLight*>(spLight.m_pObject)->NiLight::SetDiffuseColor(finalColor);
        NiUpdateData UpdateData(0.0f, false, false);
        static_cast<NiAVObject*>(spLight.m_pObject)->NiAVObject::Update(UpdateData);
    }
}

void FORenderedMenu::UpdateDistort(float afTime) {
    float fdelta = afTime - fDistortStartTime;
    fDistortPosition = fdelta / fDistortDuration;
    if (fDistortPosition > gs_fDistortVerticalScale_InterfaceFX.Float() + 1.0) {
        bDistortActive = false;
        int32_t v2 = rt::rand();
        fNextDistortTime = static_cast<double>(v2 % gs_iDistortMaxInterval_InterfaceFX.Int()) + afTime;
    }
}

void FORenderedMenu::DoEffect(
    BSRenderedTexture* apSource,
    BSRenderedTexture* apDest,
    NiRenderer::ClearFlags aeClearMode
) {
    InterfaceManager::Instance();
    ImageSpaceEffectInterface::fBlurRadius = fBlurRadius;
    ImageSpaceEffectInterface::fBlurIntensity = fBlurIntensity;
    ImageSpaceEffectInterface::fScanlineFrequency = fScanlineScale;
    ImageSpaceEffectInterface::fVerticalHold = 0.0f;
    ImageSpaceEffectInterface::spScanlinesTexture.operator=(spScanlinesTexture);
    ImageSpaceEffectInterface::spMaskTexture.operator=(static_cast<NiTexture*>(apSource->GetTexture(0)));
    if (this == Interface::GetPipboy()) {
        ImageSpaceEffectInterface::fTextureUsedPart = UI_717860();
    }
    else {
        ImageSpaceEffectInterface::fTextureUsedPart = pMenuTile->GetFloat(4016);
    }
    if (bBurstActive) {
        ImageSpaceEffectInterface::fBlurIntensity =
            (fBurstCurrentIntensity + 1.0f) * ImageSpaceEffectInterface::fBlurIntensity;
        ImageSpaceEffectInterface::fBlurRadius =
            (fBurstCurrentIntensity + 1.0f) * ImageSpaceEffectInterface::fBlurRadius;
    }
    if (bVertHoldActive) {
        ImageSpaceEffectInterface::fVerticalHold = UI_717860() * fVertHoldCurrentPosition;
    }
    if (bShudderActive) {
        ImageSpaceEffectInterface::fVerticalHold =
            UI_717860() * fShudderCurrentPosition + ImageSpaceEffectInterface::fVerticalHold;
    }
    ImageSpaceEffectInterface::fBlurIntensity = ImageSpaceEffectInterface::fBlurIntensity + fPulseCurrentBrighten;
    ImageSpaceEffectInterface::fBlurRadius = ImageSpaceEffectInterface::fBlurRadius + fPulseCurrentRadius;
    if (bDistortActive) {
        ImageSpaceEffectInterface::fDistortVerticalScale = gs_fDistortVerticalScale_InterfaceFX.Float();
        ImageSpaceEffectInterface::fDistortHorizontalScale = gs_fDistortHorizontalScale_InterfaceFX.Float();
        ImageSpaceEffectInterface::fDistortVerticalPosition = fDistortPosition;
        ImageSpaceEffectInterface::bDistortEverywhere = true;
    }
    else {
        ImageSpaceEffectInterface::fDistortVerticalScale = 0.0f;
        ImageSpaceEffectInterface::fDistortHorizontalScale = 0.0f;
        ImageSpaceEffectInterface::fDistortVerticalPosition = -1.0f;
    }
    ImageSpaceEffectInterface::kTint = TintColor;
    if (bUsePartialScreenGeometry && !spPartialScreenGeometry.m_pObject) {
        float kName = UI_717860();
        float arg0 = UI_717860();
        ImageSpaceManager* ImageSpaceManager_ = BSShaderManager::GetImageSpaceManager();
        ImageSpaceManager_->CreatePartialScreenGeometry(arg0, kName, spPartialScreenGeometry);
    }
    if (bUsePartialScreenGeometry) {
        NiScreenElements* v10 = spPartialScreenGeometry.m_pObject;
        ImageSpaceManager* v5 = BSShaderManager::GetImageSpaceManager();
        v5->EnablePartialRender(true, v10);
    }
    ImageSpaceEffectInterface::eClearFlag = aeClearMode;
    NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    ImageSpaceManager* v6 = BSShaderManager::GetImageSpaceManager();
    v6->RenderEffect(ImageSpaceManager::EFFECT_INTERFACE, Renderer_, apSource, apDest, nullptr, 1);
    ImageSpaceEffectInterface::spMaskTexture.operator=(static_cast<NiTexture*>(nullptr));
}

bool FORenderedMenu::GetInitialized() {
    return this->bInitialized;
}

uint8_t FORenderedMenu::IsMenuRendering() {
    return bMenuRendering;
}

void FORenderedMenu::SetScreenLightBaseIntensity(float afIntensity) {
    this->fScreenLightBaseIntensity = afIntensity;
}

void FORenderedMenu::SetScreenLightFlickerIntensity(float afIntensity) {
    this->fScreenLightFlickerIntensity = afIntensity;
}

void FORenderedMenu::SetDoScanlines(const bool abEnable, const float afScale) {
    this->bDoScanlines = abEnable;
    this->fScanlineScale = afScale;
}

void FORenderedMenu::SetScreenLightColor(NiColor& aColor) {
    this->ScreenLightColor = aColor;
}

void FORenderedMenu::SetTintColor(const NiColorA& aNewColor) {
    this->TintColor = aNewColor;
}

NiColorA* FORenderedMenu::GetTintColor(NiColorA* apResult) {
    *apResult = TintColor;
    return apResult;
}

void FORenderedMenu::SetScreenParent(NiAVObject* apScreenParent) {
    // The screen parent is held as a NiNode (its NiAVObject base is at offset 0, so the cast emits nothing).
    this->spScreenParent = static_cast<NiNode*>(apScreenParent);
}

void FORenderedMenu::SetScreenGeometry(NiGeometry* apScreenTriShape) {
    // The shape is held as a NiTriShape (its NiGeometry base is at offset 0, so the cast emits nothing).
    this->spScreenTriShape = static_cast<NiTriShape*>(apScreenTriShape);
}

NiGeometry* FORenderedMenu::GetScreenGeometry() {
    return spScreenTriShape.m_pObject;
}

void FORenderedMenu::PrepareGeometry(NiGeometry* apGeometry) {
    NiProperty* shadeProperty = apGeometry->GetProperty(NiShadeProperty::GetType());
    auto* noLightingProperty = static_cast<BSShaderNoLightingProperty*>(shadeProperty);
    auto* defaultTexture = static_cast<NiTexture*>(this->spDefaultTex.operator->());
    noLightingProperty->SetBaseMap(defaultTexture);

    if (!apGeometry->GetProperty(NiProperty::ALPHA)) {
        void* memory = NiMemObject::operator new(sizeof(NiAlphaProperty));
        NiAlphaProperty* alphaProperty = memory ? ::new (memory) NiAlphaProperty : nullptr;
        alphaProperty->SetAlphaBlending(false);
        alphaProperty->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        alphaProperty->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
        apGeometry->AttachProperty(alphaProperty);
    }

    NiProperty* materialProperty = apGeometry->GetProperty(NiMaterialProperty::GetType());
    if (materialProperty) {
        static_cast<NiMaterialProperty*>(materialProperty)->SetEmittance(NiColor_BLACK);
    }

    apGeometry->UpdateProperties();
    NiUpdateData updateData;
    updateData.~NiUpdateData();
    ::new (&updateData) NiUpdateData(0.0f, false, false);
    apGeometry->Update(updateData);
}

void FORenderedMenu::ClearLight() {
    if (!this->spLight) {
        return;
    }

    // The screen parent (the scene root) is asked twice whether it is a node.
    if (this->spScreenParent->IsNode()) {
        this->spScreenParent->IsNode()->DetachChild(static_cast<NiAVObject*>(this->spLight.m_pObject));
    }
    this->pLightShadowSceneNode->RemoveLight(static_cast<NiLight*>(this->spLight.m_pObject));
    this->spLight = nullptr;
}

void FORenderedMenu::SetScreenTexture(NiTexture* apTexture) {
    if (bInitialized) {
        if (spScreenTriShape.m_pObject) {
            int32_t Type = NiShadeProperty::GetType();
            NiAVObject* v2 = spScreenTriShape.m_pObject;
            BSShaderNoLightingProperty* Property = static_cast<BSShaderNoLightingProperty*>(v2->GetProperty(Type));
            if (apTexture) {
                Property->SetBaseMap(apTexture);
            }
            else {
                NiTexture* v3 = static_cast<NiTexture*>(spDefaultTex.operator->());
                Property->SetBaseMap(v3);
            }
        }
        else {
            Warning("MESHES: Trying to clear a rendered menu screen but the screen trishape is NULL.");
        }
    }
}

// The timer result is zero-extended to 64 bits, then converted and stored as a float.
void FORenderedMenu::TriggerBurst(float afBurstDuration, float afBurstIntensity, float afSoundChance) {
    this->bBurstActive = true;
    this->fBurstDuration = afBurstDuration;
    this->fBurstCurrentIntensity = afBurstIntensity;
    this->fBurstMaximumIntensity = afBurstIntensity;
    const uint64_t time = appTimer.GetTime();
    this->fBurstStartTime = static_cast<float>(time);

    if (FORenderedMenu::bSoundsActive && RandomBool(afSoundChance)) {
        BSSoundHandle soundHandle;
        alignas(BSSoundHandle) uint8_t lookupStorage[sizeof(BSSoundHandle)];
        auto* lookupResult = static_cast<BSSoundHandle*>(static_cast<void*>(lookupStorage));
        BSAudio* audio = &BSAudio::QInstance();
        BSSoundHandle* soundResult = audio->GetSoundHandleByName(
            lookupResult, "UIPipBoyStatic",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        soundHandle = *soundResult;
        lookupResult->~BSSoundHandle();
        soundHandle.Play(false);
    }
}

NiCamera* FORenderedMenu::GetCamera() {
    return static_cast<NiCamera*>((&spCamera)->operator->());
}

void FORenderedMenu::SetMenuRoot(NiAVObject* apMenuRoot) {
    spMenuRoot = apMenuRoot;
}

bool FORenderedMenu::CreateMenuLight(const char* apLightNodeName, ShadowSceneNode* apShadowSceneNode) {
    ShadowSceneNode* pSSnode;
    FNVD_INLINED_LOCAL(NiFixedString, strRenderedMenuLight, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    bool bsuccess = false;
    if (apShadowSceneNode) {
        pSSnode = apShadowSceneNode;
    }
    else {
        pSSnode = BSShaderManager::GetShadowSceneNode(0);
    }
    if (FORenderedMenu::GetParentNodeToRender()) {
        ClearLight();
        NiAVObject* pSceneRoot = FORenderedMenu::GetParentNodeToRender();
        NiFixedString* v10 = ::new (static_cast<void*>(&apString)) NiFixedString(apLightNodeName);
        NiAVObject* plightObject = pSceneRoot->GetObjectByName(*v10);
        apString.~NiFixedString();
        NiNode* plightNode = nullptr;
        if (plightObject) {
            plightNode = plightObject->IsNode();
        }
        if (plightNode) {
            NiPointLight* v9;
            void* apMemory = NiMemObject::operator new(sizeof(NiPointLight));
            if (apMemory) {
                v9 = ::new (apMemory) NiPointLight;
            }
            else {
                v9 = nullptr;
            }
            spLight.operator=(static_cast<NiRefObject*>(v9));
            NiPoint3 lightpos = plightNode->GetTranslate();
            static_cast<NiAVObject*>(spLight.m_pObject)->NiAVObject::SetTranslate(lightpos);
            NiLight* v4 = static_cast<NiLight*>(spLight.m_pObject);
            pSSnode->AddLight(v4, true);
            pLightShadowSceneNode = pSSnode;
            NiNode* pPipBoyNode = plightNode->m_pkParent;
            pPipBoyNode->AttachChild(static_cast<NiAVObject*>(spLight.m_pObject), 1);
            NiUpdateData foo(0.0f, false, false);
            spScreenTriShape.m_pObject->Update(foo);
            static_cast<NiAVObject*>(spLight.m_pObject)->NiAVObject::SetAppCulled(true);
            NiFixedString v15("RenderedMenuLight");
            NiFixedString* kName = &v15;
            static_cast<NiAVObject*>(spLight.m_pObject)->SetName(*kName);
            return true;
        }
        else {
            NiAVObject* pMenuNode = FORenderedMenu::GetParentNodeToRender();
            NiFixedString* cRenderedMenuLight =
                ::new (static_cast<void*>(&strRenderedMenuLight)) NiFixedString("RenderedMenuLight");
            NiAVObject* plight = pMenuNode->GetObjectByName(*cRenderedMenuLight);
            strRenderedMenuLight.~NiFixedString();
            if (plight) {
                spLight.operator=(
                    static_cast<NiRefObject*>(NiObject::DynamicCastStatic(&NiPointLight::ms_RTTI, plight))
                );
                if (spLight.m_pObject) {
                    return true;
                }
            }
            else {
                Warning(
                    "MENUS: FORenderedMenu::CreateMenuLight could not find light node '%s' in the rendered menu "
                    "with mesh '%s'",
                    apLightNodeName, (&FORenderedMenu::GetParentNodeToRender()->m_kName)->GetHandleString()
                );
            }
        }
    }
    return bsuccess;
}

NiPointLight* FORenderedMenu::GetMenuLight() {
    return static_cast<NiPointLight*>(spLight.m_pObject);
}

void FORenderedMenu::SetScanlineVisible() {
    bDistortActive = true;
    fDistortStartTime = ((float)(&appTimer)->BSTimer::GetTime());
    fDistortPosition = (float)0.0;
}

void FORenderedMenu::UnhideMessageQuantityTutorialMenus() {
    NiNode* node;
    NiNode* v2;
    NiNode* v3;
    TileMenu* menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1001));
    TileMenu* v5 = static_cast<TileMenu*>(Menu::GetMenuByClass(1016));
    TileMenu* v7 = static_cast<TileMenu*>(Menu::GetMenuByClass(1059));
    if (bHidMessage) {
        node = menuByClass->Tile::GetModel();
        node->NiAVObject::SetAppCulled(false);
    }
    if (bHidQuantity) {
        v2 = v5->Tile::GetModel();
        v2->NiAVObject::SetAppCulled(false);
    }
    if (bHidTutorial) {
        v3 = v7->Tile::GetModel();
        v3->NiAVObject::SetAppCulled(false);
    }
}

void FORenderedMenu::SetCamera(NiCamera* apCamera) {
    (&spCamera)->operator=(apCamera);
}

void FORenderedMenu::HideMessageQuantityTutorialMenus() {
    bool v7;
    bool v8;
    bool v9;
    Tile* MenuByClass = Menu::GetMenuByClass(1001);
    Tile* v11 = Menu::GetMenuByClass(1016);
    Tile* v13 = Menu::GetMenuByClass(1059);
    if (MenuByClass) {
        NiAVObject* Model_ = MenuByClass->GetModel();
        v9 = !Model_->GetAppCulled();
    }
    else {
        v9 = false;
    }
    bHidMessage = v9;
    if (v11) {
        NiAVObject* v2 = v11->GetModel();
        v8 = !v2->GetAppCulled();
    }
    else {
        v8 = false;
    }
    bHidQuantity = v8;
    if (v13) {
        NiAVObject* v3 = v13->GetModel();
        v7 = !v3->GetAppCulled();
    }
    else {
        v7 = false;
    }
    bHidTutorial = v7;
    if (bHidMessage) {
        NiAVObject* v4 = MenuByClass->GetModel();
        v4->SetAppCulled(true);
    }
    if (bHidQuantity) {
        NiAVObject* v5 = v11->GetModel();
        v5->SetAppCulled(true);
    }
    if (bHidTutorial) {
        NiAVObject* v6 = v13->GetModel();
        v6->SetAppCulled(true);
    }
}

void FORenderedMenu::CreateLight() {
    NiColor kSpecular;
    NiColor kAmbient;
    NiColor kDiffuse;
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    if (CreateMenuLight("Omni01", ShadowSceneNode_)) {
        NiPointLight* MenuLight = GetMenuLight();
        if (MenuLight) {
            MenuLight->SetDiffuseColor(*FNVD_RECONSTRUCT(NiColor)(&kDiffuse, 0.0f, 0.0f, 0.0f));
            MenuLight->SetAmbientColor(*FNVD_RECONSTRUCT(NiColor)(&kAmbient, 0.0f, 0.0f, 0.0f));
            MenuLight->NiPointLight::SetConstantAttenuation(0.0f);
            MenuLight->NiPointLight::SetLinearAttenuation(0.0f);
            MenuLight->NiPointLight::SetQuadraticAttenuation(0.0f);
            MenuLight->SetDimmer(1.0f);
            MenuLight->SetSpecularColor(*FNVD_RECONSTRUCT(NiColor)(&kSpecular, 10.0f, 0.0f, 0.0f));
            NiColor kScreenColor;
            kScreenColor.~NiColor();
            ::new (&kScreenColor) NiColor(1.0f, 1.0f, 1.0f);
            SetScreenLightColor(kScreenColor);
            SetScreenLightBaseIntensity(1.0f);
            SetScreenLightFlickerIntensity(1.0f);
            NiUpdateData a2(0.0f, false, false);
            MenuLight->Update(a2);
        }
    }
    else {
        Warning("MENUS: Failed to create terminal light.  Terminal will only receive ambient light.");
    }
}

void FORenderedMenu::TriggerShudder(float afDuration, float afIntensity, float afFrequency) {
    bShudderActive = true;
    fShudderDuration = afDuration;
    fShudderIntensity = afIntensity;
    fShudderFrequency = afFrequency;
    fShudderStartTime = static_cast<float>(appTimer.GetTime());
    fShudderCurrentPosition = 0.0f;
}

void FORenderedMenu::SetDefaultTex(NiTexture* apTex) {
    spDefaultTex = apTex;
}

void FORenderedMenu::SetNextDistortTime(float afTime) {
    fNextDistortTime = afTime;
}

void FORenderedMenu::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void ImageSpaceManager::EnablePartialRender(bool abEnable, NiScreenElements* apPartialScreenGeometry) {
    bEnablePartialRender = abEnable;
    spPartialScreenTriShape = apPartialScreenGeometry;
}
