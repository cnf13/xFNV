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
    double dTime = static_cast<double>(appTimer.iLastTime);
    fNextDistortTime = static_cast<double>(rt::rand() % gs_iDistortMaxInterval_InterfaceFX.Int()) + dTime;
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
    NiFrustum oldFrust;
    NiFrustum frust;
    if (bInitialized && spMenuRoot.m_pObject) {
        NiRenderer* pr = NiRenderer::GetRenderer();
        InterfaceManager* pim = InterfaceManager::Instance();
        if (pim->IsDebugTextVisible()) {
            pim->GetDebugTextRoot()->SetAppCulled(true);
        }
        SceneGraph* pSceneGraph = pim->GetSceneGraph();
        BSCullingProcess* pCuller = pSceneGraph->pCuller;
        pCuller->SetAccumulator(static_cast<NiAccumulator*>(spAccumulator.m_pObject));
        NiColorA prevColor(0.0f, 0.0f, 0.0f, 0.0f);
        pr->GetBackgroundColor(prevColor);
        pr->SetBackgroundColor(BackgroundColor);
        NiCamera* pCamera = pSceneGraph->GetCamera();
        NiPointer<BSRenderedTexture> spTempRenderTarget(static_cast<BSRenderedTexture*>(nullptr));
        if (apSwapTarget) {
            spTempRenderTarget = apSwapTarget;
        }
        else {
            NiDX9Renderer* pDX9Renderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            spTempRenderTarget = pTextureManager->BorrowRenderedTexture(
                pDX9Renderer,
                BSTextureManager::TEX_INTERFACE_TEMP,
                0,
                nullptr,
                0
            );
        }
        if (BSRenderedTexture::QOutsideFrame()) {
            NiRenderTargetGroup* pGroup = spTempRenderTarget->GetGroup();
            BSRenderedTexture::StartOffscreen(aeClearMode, pGroup);
            // The viewport is built with NiColorA's constructor, components in the order left, right, top, bottom.
            ::new (&kViewport) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
            pCamera->SetViewPort(kViewport);
        }
        frust = pCamera->GetViewFrustum();
        oldFrust = frust;
        frust.m_bOrtho = true;
        frust.m_fLeft = 0.0f;
        frust.m_fRight = 1280.0f;
        frust.m_fTop = 960.0f;
        frust.m_fBottom = 0.0f;
        frust.m_fNear = 0.0f;
        frust.m_fFar = 10000.0f;
        pCamera->SetViewFrustum(frust);
        pr->SetCameraData(pCamera);
        Tile* pCursor = pim->pCursor;
        bool bCullCursor = false;
        if (pCursor) {
            bCullCursor = pCursor->GetModel()->GetAppCulled();
            pCursor->GetModel()->SetAppCulled(true);
        }
        NiAVObject* pConsoleRoot = Interface::GetDebugTextRoot();
        bool bConsoleCulled = pConsoleRoot ? pConsoleRoot->GetAppCulled() : false;
        if (pConsoleRoot) {
            pConsoleRoot->SetAppCulled(true);
        }
        Tile* pHUD = HUDMainMenu::pme->pMenu;
        bool bHUDCulled = pHUD->GetMenuTile()->GetModel()->GetAppCulled();
        pHUD->GetMenuTile()->GetModel()->SetAppCulled(true);
        if (this != Interface::GetPipboy()) {
            static_cast<NiAVObject*>(spMenuRoot.m_pObject)->NiAVObject::SetAppCulled(false);
        }
        HideMessageQuantityTutorialMenus();
        pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiAVObject* pRoot = static_cast<NiAVObject*>(spMenuRoot.m_pObject);
        BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), pRoot, *pCuller);
        BSShaderAccumulator* pAccumulator = static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject);
        BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), pAccumulator, false);
        pCuller->PopCullMode();
        UnhideMessageQuantityTutorialMenus();
        pCuller->SetAccumulator(nullptr);
        if (this != Interface::GetPipboy()) {
            static_cast<NiAVObject*>(spMenuRoot.m_pObject)->NiAVObject::SetAppCulled(true);
        }
        pHUD->GetMenuTile()->GetModel()->SetAppCulled(bHUDCulled);
        if (pCursor) {
            pCursor->GetModel()->SetAppCulled(bCullCursor);
        }
        if (pConsoleRoot) {
            pConsoleRoot->SetAppCulled(bConsoleCulled);
        }
        BSRenderedTexture::StopOffscreen();
        DoEffect(spTempRenderTarget.m_pObject, apTarget, aeClearMode);
        if (!apSwapTarget) {
            BSRenderedTexture* pTempTexture = spTempRenderTarget.m_pObject;
            BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pTempTexture);
        }
        pr->SetBackgroundColor(prevColor);
        pCamera->SetViewFrustum(oldFrust);
        if (bUsePartialScreenGeometry) {
            BSShaderManager::GetImageSpaceManager()->EnablePartialRender(false, nullptr);
        }
        PostDraw();
        if (pim->IsDebugTextVisible()) {
            pim->GetDebugTextRoot()->SetAppCulled(false);
        }
    }
}

bool FORenderedMenu::OnIdle() {
    float fTime = static_cast<float>(appTimer.GetTime());
    bool bWasMenuOpen = this->bMenuOpen;
    this->bMenuOpen = InterfaceManager::Instance()->IsInRenderedMenu();
    if (!this->bMenuOpen && !bWasMenuOpen) {
        return false;
    }
    UpdateLight(fTime);
    if (bBurstActive) {
        UpdateBurst(fTime);
    }
    if (bVertHoldActive) {
        UpdateVertHold(fTime);
    }
    if (bShudderActive) {
        UpdateShudder(fTime);
    }
    UpdatePulse(fTime);
    if (bDistortActive) {
        UpdateDistort(fTime);
    }
    else if (fNextDistortTime < static_cast<double>(fTime)) {
        SetScanlineVisible();
    }
    return true;
}

bool FORenderedMenu::Pick(bool abPressed, float* apX, float* apY) {
    NiPoint3 kDir;
    NiPoint3 kOrigin;
    NiCamera* pCamera;
    char cPicked = 0;
    static_cast<NiAVObject*>(spMenuRoot.m_pObject)->NiAVObject::SetAppCulled(false);
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    if (!spScreenTriShape.m_pObject) {
        return false;
    }
    NiPick kPick(0, 8u);
    if (GetCamera()) {
        pCamera = GetCamera();
    }
    else {
        pCamera = Main::GetWorldSceneGraph()->GetCamera();
    }
    kPick.SetObserveAppCullFlag(true);
    kPick.SetTarget(spScreenTriShape.m_pObject);
    kPick.m_eSortType = NiPick::SORT;
    kPick.SetIsReturnTexture(true);
    int32_t iCursorY = static_cast<int32_t>(pInterfaceManager->sRealCursorPos.z);
    double dCursorX = pInterfaceManager->sRealCursorPos.x;
    pCamera->WindowPointToRay(static_cast<int32_t>(dCursorX), iCursorY, kOrigin, kDir, nullptr);
    if (kPick.PickObjects(kOrigin, kDir, false)) {
        NiPick::Record* pPickResult = kPick.m_pickResults.GetAt(0);
        if (pPickResult && pPickResult->GetAVObject() == spScreenTriShape.m_pObject) {
            // The record's texture coordinates, copied out of m_texture.
            float fUV_X = pPickResult->m_texture.x;
            float fUV_Y = pPickResult->m_texture.y;
            if (Interface::IsMenuIDVisible(Interface::eRaceSexMenu, 0)) {
                fUV_X = fUV_X * 0.6299999952316284;
                fUV_X = fUV_X + 0.3700000047683716;
                fUV_Y = fUV_Y * 0.6499999761581421;
                fUV_Y = fUV_Y + 0.3400000035762787;
            }
            float fHeight = static_cast<float>(NiRenderer::GetRenderer()->GetDefaultBackBuffer()->GetHeight());
            float fWidth = fHeight * 1.3333330154418945;
            *apX = fUV_X * fWidth;
            *apY = fUV_Y * fHeight;
            cPicked = 1;
        }
    }
    return cPicked;
}

void FORenderedMenu::Initialize() {
    BSShaderAccumulator* pAccumulator = new BSShaderAccumulator(100, 1u, 759u);
    spAccumulator = static_cast<NiRefObject*>(pAccumulator);
    static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject)
        ->BSShaderAccumulator::SetRenderMode(BSShaderManager::BSSM_RENDER_INTERFACE);
    NiPointer<NiTexture> spDefaultTex(static_cast<NiTexture*>(nullptr));
    BSShaderManager::GetTexture("Data\\Textures\\Pipboy3000\\screen.dds", true, spDefaultTex, true, false);
    NiTexture* pDefaultTex = spDefaultTex.m_pObject;
    if (!pDefaultTex) {
        Warning("MENUS: Default renderedmenu texture NOT FOUND.  Rendered menu may not work.");
    }
    SetDefaultTex(pDefaultTex);
    fBlurRadius = fDefaultBlurRadius.Float();
    fBlurIntensity = fDefaultBlurIntensity.Float();
    BSShaderManager::GetTexture(gs_sScanlineTexture_InterfaceFX.String(), true, spScanlinesTexture, false, false);
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
        pMenuTile->GetMenu()->SetRenderedMenu();
    }
    Interface::GetMenusRoot()->UpdateClipwindows();
    if (spLight) {
        NiSmartPointerCast<NiAVObject>(spLight)->SetAppCulled(false);
    }
    bMenuRendering = 1;
}

void FORenderedMenu::OnMenuClose() {
    bMenuRendering = 0;
    bBurstActive = false;
    bVertHoldActive = false;
    if (spLight) {
        NiRefObject* pLight = spLight;
        static_cast<NiAVObject*>(pLight)->SetAppCulled(true);
    }
    SetScreenTexture(nullptr);
}

void FORenderedMenu::UpdateBurst(float afTime) {
    float fElapsed = afTime - fBurstStartTime;
    if (!(fBurstDuration < static_cast<double>(fElapsed))) {
        float fFade = static_cast<float>(1.0 - fElapsed / fBurstDuration);
        fBurstCurrentIntensity = fBurstMaximumIntensity * fFade;
    }
    else {
        bBurstActive = false;
    }
}

void FORenderedMenu::UpdateVertHold(float afTime) {
    float fElapsed = afTime - fVertHoldStartTime;
    if (!(fVertHoldDuration < static_cast<double>(fElapsed))) {
        fVertHoldCurrentPosition = fElapsed * fVertHoldSpeed;
        if (fVertHoldCurrentPosition >= 1.0 || fVertHoldCurrentPosition < 0.0) {
            fVertHoldCurrentPosition = fVertHoldCurrentPosition - FloatFloor(fVertHoldCurrentPosition);
        }
    }
    else {
        bVertHoldActive = false;
        static_cast<FOPipboyManager*>(this)->SetShudderHoldParams(250.0f, 0.05000000074505806f, 0.05000000074505806f);
    }
}

void FORenderedMenu::UpdateShudder(float afTime) {
    float fElapsed = afTime - fShudderStartTime;
    if (!(fShudderDuration < static_cast<double>(fElapsed))) {
        float fFade = static_cast<float>(1.0 - fElapsed / fShudderDuration);
        float fPhase = fElapsed * fShudderFrequency;
        fShudderCurrentPosition = std::sinf(fPhase) * fShudderIntensity * fFade;
    }
    else {
        bShudderActive = false;
    }
}

void FORenderedMenu::UpdatePulse(float afTime) {
    float fNoiseTime = afTime * gs_fPulseRate_InterfaceFX.Float();
    double dNoise = BSNoise1D::Noise(fNoiseTime);
    float fPulse = static_cast<float>(dNoise + dNoise);
    fPulseCurrentBrighten = fPulse * fPulseBrightenIntensity.Float();
    fPulseCurrentRadius = fPulse * fPulseRadiusIntensity.Float();
}

void FORenderedMenu::UpdateLight(float afTime) {
    NiColor finalColor;
    NiColor kLightAdd;
    if (spLight.m_pObject) {
        kLightAdd = NiColor::BLACK;
        if (bPulseAffectsLight) {
            double dVertHold;
            double dBurst = bBurstActive ? fBurstCurrentIntensity * fScreenLightBurstModifier.Float() : 0.0;
            float fBurstIntensity = dBurst;
            float fPulseIntensity = fPulseCurrentBrighten * fScreenLightPulseModifier.Float();
            if (bVertHoldActive) {
                float fVertHoldPhase = afTime * fScreenLightVertHoldRate.Float();
                dVertHold = std::sinf(fVertHoldPhase) + 0.5;
            }
            else {
                dVertHold = 1.0;
            }
            float fVertIntensity = dVertHold;
            float fLightMod = (fBurstIntensity + fPulseIntensity) * fVertIntensity;
            float fFlickerIntensity = fScreenLightFlickerIntensity;
            kLightAdd = ScreenLightColor * fLightMod * fFlickerIntensity;
        }
        (ScreenLightColor * fScreenLightBaseIntensity).Add(&finalColor, &kLightAdd);
        static_cast<NiLight*>(spLight.m_pObject)->NiLight::SetDiffuseColor(finalColor);
        NiUpdateData UpdateData(0.0f, false, false);
        static_cast<NiAVObject*>(spLight.m_pObject)->NiAVObject::Update(UpdateData);
    }
}

void FORenderedMenu::UpdateDistort(float afTime) {
    float fDelta = afTime - fDistortStartTime;
    fDistortPosition = fDelta / fDistortDuration;
    if (fDistortPosition > gs_fDistortVerticalScale_InterfaceFX.Float() + 1.0) {
        bDistortActive = false;
        int32_t iRand = rt::rand();
        fNextDistortTime = static_cast<double>(iRand % gs_iDistortMaxInterval_InterfaceFX.Int()) + afTime;
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
    ImageSpaceEffectInterface::spScanlinesTexture = spScanlinesTexture;
    ImageSpaceEffectInterface::spMaskTexture = static_cast<NiTexture*>(apSource->GetTexture(0));
    if (this == Interface::GetPipboy()) {
        ImageSpaceEffectInterface::fTextureUsedPart = GetMenuHeightRatio();
    }
    else {
        ImageSpaceEffectInterface::fTextureUsedPart = pMenuTile->GetFloat(4016);
    }
    if (bBurstActive) {
        ImageSpaceEffectInterface::fBlurIntensity = (fBurstCurrentIntensity + 1.0f) *
            ImageSpaceEffectInterface::fBlurIntensity;
        ImageSpaceEffectInterface::fBlurRadius = (fBurstCurrentIntensity + 1.0f) *
            ImageSpaceEffectInterface::fBlurRadius;
    }
    if (bVertHoldActive) {
        ImageSpaceEffectInterface::fVerticalHold = GetMenuHeightRatio() * fVertHoldCurrentPosition;
    }
    if (bShudderActive) {
        ImageSpaceEffectInterface::fVerticalHold = GetMenuHeightRatio() * fShudderCurrentPosition +
            ImageSpaceEffectInterface::fVerticalHold;
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
        float fHeightRatio = GetMenuHeightRatio();
        float fWidthRatio = GetMenuHeightRatio();
        BSShaderManager::GetImageSpaceManager()
            ->CreatePartialScreenGeometry(fWidthRatio, fHeightRatio, spPartialScreenGeometry);
    }
    if (bUsePartialScreenGeometry) {
        NiScreenElements* pPartialScreenGeometry = spPartialScreenGeometry.m_pObject;
        BSShaderManager::GetImageSpaceManager()->EnablePartialRender(true, pPartialScreenGeometry);
    }
    ImageSpaceEffectInterface::eClearFlag = aeClearMode;
    NiDX9Renderer* pDX9Renderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    BSShaderManager::GetImageSpaceManager()
        ->RenderEffect(ImageSpaceManager::EFFECT_INTERFACE, pDX9Renderer, apSource, apDest, nullptr, 1);
    ImageSpaceEffectInterface::spMaskTexture = static_cast<NiTexture*>(nullptr);
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
    auto* defaultTexture = NiSmartPointerCast<NiTexture>(this->spDefaultTex);
    noLightingProperty->SetBaseMap(defaultTexture);

    if (!apGeometry->GetProperty(NiProperty::ALPHA)) {
        NiAlphaProperty* alphaProperty = new NiAlphaProperty;
        alphaProperty->SetAlphaBlending(false);
        alphaProperty->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        alphaProperty->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
        apGeometry->AttachProperty(alphaProperty);
    }

    NiProperty* materialProperty = apGeometry->GetProperty(NiMaterialProperty::GetType());
    if (materialProperty) {
        static_cast<NiMaterialProperty*>(materialProperty)->SetEmittance(NiColor::BLACK);
    }

    apGeometry->UpdateProperties();
    NiUpdateData updateData(0.0f, false, false);
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
            int32_t iType = NiShadeProperty::GetType();
            NiAVObject* pScreenTriShape = spScreenTriShape.m_pObject;
            BSShaderNoLightingProperty* pProperty = static_cast<BSShaderNoLightingProperty*>(
                pScreenTriShape->GetProperty(iType)
            );
            if (apTexture) {
                pProperty->SetBaseMap(apTexture);
            }
            else {
                pProperty->SetBaseMap(NiSmartPointerCast<NiTexture>(spDefaultTex));
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
        soundHandle = BSAudio::QInstance().GetSoundHandleByName(
            "UIPipBoyStatic",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        soundHandle.Play(false);
    }
}

NiCamera* FORenderedMenu::GetCamera() {
    return NiSmartPointerCast<NiCamera>(spCamera);
}

void FORenderedMenu::SetMenuRoot(NiAVObject* apMenuRoot) {
    spMenuRoot = apMenuRoot;
}

bool FORenderedMenu::CreateMenuLight(const char* apLightNodeName, ShadowSceneNode* apShadowSceneNode) {
    bool bSuccess = false;
    ShadowSceneNode* pSSNode = apShadowSceneNode ? apShadowSceneNode : BSShaderManager::GetShadowSceneNode(0);
    if (FORenderedMenu::GetParentNodeToRender()) {
        ClearLight();
        NiAVObject* pSceneRoot = FORenderedMenu::GetParentNodeToRender();
        NiAVObject* pLightObject = pSceneRoot->GetObjectByName(apLightNodeName);
        NiNode* pLightNode = nullptr;
        if (pLightObject) {
            pLightNode = pLightObject->IsNode();
        }
        if (pLightNode) {
            NiPointLight* pPointLight = new NiPointLight;
            spLight = static_cast<NiRefObject*>(pPointLight);
            NiPoint3 lightPos = pLightNode->GetTranslate();
            static_cast<NiAVObject*>(spLight.m_pObject)->NiAVObject::SetTranslate(lightPos);
            pSSNode->AddLight(static_cast<NiLight*>(spLight.m_pObject), true);
            pLightShadowSceneNode = pSSNode;
            NiNode* pPipBoyNode = pLightNode->m_pkParent;
            pPipBoyNode->AttachChild(static_cast<NiAVObject*>(spLight.m_pObject), 1);
            NiUpdateData foo(0.0f, false, false);
            spScreenTriShape->Update(foo);
            static_cast<NiAVObject*>(spLight.m_pObject)->NiAVObject::SetAppCulled(true);
            NiFixedString kName("RenderedMenuLight");
            static_cast<NiAVObject*>(spLight.m_pObject)->SetName(kName);
            return true;
        }

        NiAVObject* pMenuNode = FORenderedMenu::GetParentNodeToRender();
        NiAVObject* pLight = pMenuNode->GetObjectByName("RenderedMenuLight");
        if (pLight) {
            spLight = static_cast<NiRefObject*>(NiObject::DynamicCastStatic(&NiPointLight::ms_RTTI, pLight));
            if (spLight.m_pObject) {
                return true;
            }
        }
        else {
            Warning(
                "MENUS: FORenderedMenu::CreateMenuLight could not find light node '%s' in the rendered menu "
                "with mesh '%s'",
                apLightNodeName,
                FORenderedMenu::GetParentNodeToRender()->m_kName.GetHandleString()
            );
        }
    }
    return bSuccess;
}

NiPointLight* FORenderedMenu::GetMenuLight() {
    return static_cast<NiPointLight*>(spLight.m_pObject);
}

void FORenderedMenu::SetScanlineVisible() {
    bDistortActive = true;
    fDistortStartTime = static_cast<float>(appTimer.BSTimer::GetTime());
    fDistortPosition = 0.0f;
}

void FORenderedMenu::UnhideMessageQuantityTutorialMenus() {
    TileMenu* pMessageMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(1001));
    TileMenu* pQuantityMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(1016));
    TileMenu* pTutorialMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(1059));
    if (bHidMessage) {
        pMessageMenu->Tile::GetModel()->NiAVObject::SetAppCulled(false);
    }
    if (bHidQuantity) {
        pQuantityMenu->Tile::GetModel()->NiAVObject::SetAppCulled(false);
    }
    if (bHidTutorial) {
        pTutorialMenu->Tile::GetModel()->NiAVObject::SetAppCulled(false);
    }
}

void FORenderedMenu::SetCamera(NiCamera* apCamera) {
    spCamera = apCamera;
}

void FORenderedMenu::HideMessageQuantityTutorialMenus() {
    Tile* pMessageMenu = Menu::GetMenuByClass(1001);
    Tile* pQuantityMenu = Menu::GetMenuByClass(1016);
    Tile* pTutorialMenu = Menu::GetMenuByClass(1059);
    if (pMessageMenu) {
        bHidMessage = !pMessageMenu->GetModel()->GetAppCulled();
    }
    else {
        bHidMessage = false;
    }
    if (pQuantityMenu) {
        bHidQuantity = !pQuantityMenu->GetModel()->GetAppCulled();
    }
    else {
        bHidQuantity = false;
    }
    if (pTutorialMenu) {
        bHidTutorial = !pTutorialMenu->GetModel()->GetAppCulled();
    }
    else {
        bHidTutorial = false;
    }
    if (bHidMessage) {
        pMessageMenu->GetModel()->SetAppCulled(true);
    }
    if (bHidQuantity) {
        pQuantityMenu->GetModel()->SetAppCulled(true);
    }
    if (bHidTutorial) {
        pTutorialMenu->GetModel()->SetAppCulled(true);
    }
}

void FORenderedMenu::CreateLight() {
    if (CreateMenuLight("Omni01", BSShaderManager::GetShadowSceneNode(1))) {
        NiPointLight* pMenuLight = GetMenuLight();
        if (pMenuLight) {
            pMenuLight->SetDiffuseColor(NiColor(0.0f, 0.0f, 0.0f));
            pMenuLight->SetAmbientColor(NiColor(0.0f, 0.0f, 0.0f));
            pMenuLight->NiPointLight::SetConstantAttenuation(0.0f);
            pMenuLight->NiPointLight::SetLinearAttenuation(0.0f);
            pMenuLight->NiPointLight::SetQuadraticAttenuation(0.0f);
            pMenuLight->SetDimmer(1.0f);
            pMenuLight->SetSpecularColor(NiColor(10.0f, 0.0f, 0.0f));
            NiColor kScreenColor(1.0f, 1.0f, 1.0f);
            SetScreenLightColor(kScreenColor);
            SetScreenLightBaseIntensity(1.0f);
            SetScreenLightFlickerIntensity(1.0f);
            NiUpdateData kUpdateData(0.0f, false, false);
            pMenuLight->Update(kUpdateData);
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
