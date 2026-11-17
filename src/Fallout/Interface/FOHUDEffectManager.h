#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Interface/BGSMenuPacker.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"

class ImageSpaceShaderParam;

// The effects drawn over the HUD: the shudder, the blur and the scanlines, and the packing of the HUD into one
// texture.
class FOHUDEffectManager {
public:
    FOHUDEffectManager();
    ~FOHUDEffectManager();

    void SetScanlineFrequencyHUD(float abFrequency);
    void SetBlurRadiusHUD(float abRadius);
    void SetBlurIntensityHUD(float abIntensity);
    void TriggerShudder(float a2, float a3, float a4);
    void UpdateShudder(float afTime);
    void OnIdle();
    BGSMenuPacker* GetPacker();
    void TriggerShudder_007F78F0(const NiPoint2& aDuration, const NiPoint2& aIntensity, const NiPoint2& aFrequency);

    NiPointer<NiScreenElements> spPartialScreenGeometry;
    BGSMenuPacker Packer;
    bool bShudderActive;
    float fShudderStartTime;
    NiPoint2 ShudderDuration;
    NiPoint2 ShudderIntensity;
    NiPoint2 ShudderFrequency;
    NiPoint2 ShudderCurrentPosition;
    ImageSpaceShaderParam* pUnpackShaderParams;
    bool bHideHUD;
    float fCurrentBlurRadiusHUD;
    float fCurrentScanlineFrequencyHUD;
    float fCurrentBlurIntensityHUD;
};

// The game settings FOHUDEffectManager.cpp defines.
extern SettingT<INISettingCollection> fHUDShudderDefaultFrequencyY;
extern SettingT<INISettingCollection> bUseOptimizedMenuRendering;
extern SettingT<INISettingCollection> fBlurRadiusMenus;
extern SettingT<INISettingCollection> fBlurRadiusHUD;
extern SettingT<INISettingCollection> fScanlineFrequencyMenus;
extern SettingT<INISettingCollection> fBlurIntensityMenus;
extern SettingT<INISettingCollection> fHUDShudderDefaultFrequencyX;
extern SettingT<INISettingCollection> fBlurIntensityHUD;
extern SettingT<INISettingCollection> fHUDShudderDefaultDuration;
extern SettingT<INISettingCollection> fHUDShudderDefaultIntensity;
extern SettingT<INISettingCollection> fScanlineFrequencyHUD;
