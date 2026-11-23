#include "Fallout/Interface/FOHUDEffectManager.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "BSShader/ImageSpaceShaderMenuUnpack.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Fallout/Interface/BGSMenuPacker.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

#include <cmath>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseOptimizedMenuRendering("bUseOptimizedMenuRendering:InterfaceFX", false);
SettingT<INISettingCollection> fHUDShudderDefaultFrequencyX("fHUDShudderDefaultFrequencyX:InterfaceFX", 0.05f);
SettingT<INISettingCollection> fHUDShudderDefaultFrequencyY("fHUDShudderDefaultFrequencyY:InterfaceFX", 0.03f);
SettingT<INISettingCollection> fHUDShudderDefaultDuration("fHUDShudderDefaultDuration:InterfaceFX", 500.0f);
SettingT<INISettingCollection> fHUDShudderDefaultIntensity("fHUDShudderDefaultIntensity:InterfaceFX", 0.1f);
SettingT<INISettingCollection> fScanlineFrequencyHUD("fScanlineFrequencyHUD:InterfaceFX", 0.0f);
SettingT<INISettingCollection> fBlurRadiusHUD("fBlurRadiusHUD:InterfaceFX", 2.0f);
SettingT<INISettingCollection> fBlurIntensityHUD("fBlurIntensityHUD:InterfaceFX", 1.5f);
SettingT<INISettingCollection> fScanlineFrequencyMenus("fScanlineFrequencyMenus:InterfaceFX", 0.0f);
SettingT<INISettingCollection> fBlurRadiusMenus("fBlurRadiusMenus:InterfaceFX", 2.0f);
SettingT<INISettingCollection> fBlurIntensityMenus("fBlurIntensityMenus:InterfaceFX", 1.5f);

FOHUDEffectManager::FOHUDEffectManager() : spPartialScreenGeometry(nullptr), Packer() {
    pUnpackShaderParams = new ImageSpaceShaderParam();
    pUnpackShaderParams->InitConstantMap(0, 0);
    pUnpackShaderParams->bUseTexOffset[1] = false;
    pUnpackShaderParams->bUseTexOffset[2] = false;
    pUnpackShaderParams->iFiltermode = NiTexturingProperty::FILTER_BILERP;
    spPartialScreenGeometry = nullptr;
    bShudderActive = false;
    ShudderCurrentPosition = NiPoint2::ZERO;
    bHideHUD = false;
    fCurrentScanlineFrequencyHUD = fScanlineFrequencyHUD.Float();
    fCurrentBlurIntensityHUD = fBlurIntensityHUD.Float();
    fCurrentBlurRadiusHUD = fBlurRadiusHUD.Float();
}

FOHUDEffectManager::~FOHUDEffectManager() {
    delete pUnpackShaderParams;
}

void FOHUDEffectManager::SetScanlineFrequencyHUD(float abFrequency) {
    fCurrentScanlineFrequencyHUD = abFrequency;
}

void FOHUDEffectManager::SetBlurRadiusHUD(float abRadius) {
    fCurrentBlurRadiusHUD = abRadius;
}

void FOHUDEffectManager::SetBlurIntensityHUD(float abIntensity) {
    fCurrentBlurIntensityHUD = abIntensity;
}

void FOHUDEffectManager::TriggerShudder(float afDuration, float afIntensity, float afFrequency) {
    bShudderActive = true;
    ShudderDuration.x = afDuration;
    ShudderDuration.y = afDuration;
    ShudderIntensity.x = afIntensity;
    ShudderFrequency.x = afFrequency;
    fShudderStartTime = static_cast<float>(appTimer.BSTimer::GetTime());
    ShudderCurrentPosition.x = 0.0f;
    ShudderIntensity.y = 0.0f;
}

void FOHUDEffectManager::UpdateShudder(float afTime) {
    float fElapsed = afTime - fShudderStartTime;
    if (!(ShudderDuration.x < static_cast<double>(fElapsed)) || !(ShudderDuration.y < static_cast<double>(fElapsed))) {
        float fFalloffX = static_cast<float>(1.0 - fElapsed / ShudderDuration.x);
        float fFalloffY = static_cast<float>(1.0 - fElapsed / ShudderDuration.y);
        double dSinX = std::sinf(fElapsed * ShudderFrequency.x);
        ShudderCurrentPosition.x = static_cast<float>(dSinX * ShudderIntensity.x * fFalloffX);
        double dSinY = std::sinf(fElapsed * ShudderFrequency.y);
        ShudderCurrentPosition.y = static_cast<float>(dSinY * ShudderIntensity.y * fFalloffY);
    }
    else {
        bShudderActive = false;
        ImageSpaceShaderMenuUnpack::SetOffset(0.0, 0.0);
    }
}

void FOHUDEffectManager::OnIdle() {
    float fLastTime = static_cast<float>(static_cast<double>(appTimer.BSTimer::GetTime()));
    if (bShudderActive) {
        UpdateShudder(fLastTime);
    }
}

BGSMenuPacker* FOHUDEffectManager::GetPacker() {
    return &Packer;
}

void FOHUDEffectManager::TriggerShudder(
    const NiPoint2& aDuration,
    const NiPoint2& aIntensity,
    const NiPoint2& aFrequency
) {
    bShudderActive = true;
    float fDurationY = aDuration.y;
    ShudderDuration.x = aDuration.x;
    ShudderDuration.y = fDurationY;
    float fIntensityY = aIntensity.y;
    ShudderIntensity.x = aIntensity.x;
    ShudderIntensity.y = fIntensityY;
    float fFrequencyY = aFrequency.y;
    ShudderFrequency.x = aFrequency.x;
    ShudderFrequency.y = fFrequencyY;
    fShudderStartTime = static_cast<float>(appTimer.iLastTime);
    ShudderCurrentPosition = NiPoint2::ZERO;
}

ImageSpaceShaderMenuUnpack::ImageSpaceShaderMenuUnpack() {
    pTextures.SetSize(2u);
}

void ImageSpaceShaderMenuUnpack::SetOffset(float afX, float afY) {
    fOffsetX = afX;
    fOffsetY = afY;
    bChangedFlag = true;
}
