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

void FOHUDEffectManager::TriggerShudder(float a2, float a3, float a4) {
    bShudderActive = true;
    ShudderDuration.x = a2;
    ShudderDuration.y = a2;
    ShudderIntensity.x = a3;
    ShudderFrequency.x = a4;
    fShudderStartTime = ((float)(&appTimer)->BSTimer::GetTime());
    ShudderCurrentPosition.x = (float)0.0;
    ShudderIntensity.y = (float)0.0;
}

void FOHUDEffectManager::UpdateShudder(float afTime) {
    double v2;
    double v3;
    float v4;
    float v5;
    float ecx0;
    float v8;
    float v9 = afTime - fShudderStartTime;
    if ((!(ShudderDuration.x < ((double)v9))) || (!(ShudderDuration.y < ((double)v9)))) {
        ecx0;
        ecx0 = (float)(1.0 - (v9 / ShudderDuration.x));
        v8 = (float)(1.0 - (v9 / ShudderDuration.y));
        v5 = v9 * ShudderFrequency.x;
        v2 = (double)std::sinf(v5);
        ShudderCurrentPosition.x = (float)((v2 * ShudderIntensity.x) * ecx0);
        v4 = v9 * ShudderFrequency.y;
        v3 = (double)std::sinf(v4);
        ShudderCurrentPosition.y = (float)((v3 * ShudderIntensity.y) * v8);
    }
    else {
        bShudderActive = false;
        ImageSpaceShaderMenuUnpack::SetOffset(0.0, 0.0);
    }
}

void FOHUDEffectManager::OnIdle() {
    float lastTime = (float)(double)(&appTimer)->BSTimer::GetTime();
    if (bShudderActive) {
        UpdateShudder(lastTime);
    }
}

BGSMenuPacker* FOHUDEffectManager::GetPacker() {
    return &Packer;
}

void FOHUDEffectManager::TriggerShudder_007F78F0(
    const NiPoint2& aDuration,
    const NiPoint2& aIntensity,
    const NiPoint2& aFrequency
) {
    bShudderActive = true;
    float y = aDuration.y;
    ShudderDuration.x = aDuration.x;
    ShudderDuration.y = y;
    float v5 = aIntensity.y;
    ShudderIntensity.x = aIntensity.x;
    ShudderIntensity.y = v5;
    float v6 = aFrequency.y;
    ShudderFrequency.x = aFrequency.x;
    ShudderFrequency.y = v6;
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
