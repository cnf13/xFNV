#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"

#include <stdint.h>

// The rendered menu of the pip-boy: its 3D model, knobs, buttons, light and screen.
class FOPipboyManager : public FORenderedMenu {
public:
    // The knobs of the pip-boy model, indexing spKnobs and the knob arrays.
    enum Knobs : int32_t {
        KNOB_SCROLL = 0,
        KNOB_RAD = 1,
        KNOB_TAB = 2,
    };

    // Constructs a pip-boy manager in apMemory (0x170 bytes).
    FOPipboyManager();
    ~FOPipboyManager() override;

    // Runs the base screen update, then moves the knobs and updates the idle animations; false when the base
    // update returned false.
    bool OnIdle() override;
    // Loads the pip-boy models, knobs, buttons and light.
    void Initialize() override;
    // Releases the pip-boy's models and light.
    void Shutdown() override;
    void OnMenuOpen() override;
    void OnMenuClose() override;

    // Moves the knobs to their scroll positions.
    void UpdateKnobs(float afTime);
    // Sets whether the pip-boy screen is refractive.
    void SetRefractive(bool abRefractive);
    void SetVerticalHoldParams(float afParam1, float afParam2);
    // Sets the shudder hold parameters.
    uint32_t SetShudderHoldParams(float afParam1, float afParam2, float afParam3);
    // Starts a vertical hold or a shudder effect by chance, then a burst.
    void TriggerRandomEffect();
    // The glow geometry of button aiButton (0-based).
    NiAVObject* GetButtonGlow(uint32_t aiButton);
    void FadeLightEffect(bool abFadeOut);
    // Turns a knob towards an absolute position at the given rate (aiKnob is a Knobs value).
    void MoveKnobAbsolute(Knobs aiKnob, float afTarget, float afRate);
    // Hides the three button glows, then shows the glow of button aiButton (1-based; 0 shows none).
    void ActivateButton(const uint32_t aiButton);
    // Re-creates the screen light (after the player is positioned).
    void ReInitialize();
    // Shows or hides the pip-boy light of the first- or third-person model, recording the state when
    // abUpdateVariable.
    void ShowPipboyLightEffect(const bool ab3rdPerson, const bool abShow, const bool abUpdateVariable);
    // True while the pip-boy light fades.
    bool QPipboyLightFading();
    // Steps the pip-boy light's fade at afTime (the timer's milliseconds).
    void UpdateLightEffect(float afTime);
    // True while the first- or third-person pip-boy light is on.
    bool QPipboyLightOn();
    // Flags the pip-boy manager for a reset.
    void SetPipBoyManagerReset(bool abFlag);
    // True when the pip-boy manager was flagged for a reset.
    bool GetPipBoyManagerReset();
    // Resets the pip-boy manager.
    void ResetPipBoyManager();
    void SetTabKnobPosition(uint32_t aiNewPos, bool abPlaySounds);
    NiAVObject* Get3rdPersonScreen();
    NiAVObject* Get3rdPersonPipboyRoot();
    void MoveKnob(FOPipboyManager::Knobs aiKnob, float afAmount, float afRate, bool abPlaySound);

protected:
    // Creates the screen light and resets its colours, attenuation and intensities.
    void InitLight();
    // Finds the buttons and button glows on the first-person model.
    void InitButtons();
    // Fades the player's spell effect light over fLightEffectLightFadeDuration seconds from
    // fLightEffectFadeStartTime: in towards a target of 1, out towards 0.
    void FadeLight(float afTime);

public:
    void ResetColors();
    char CheckOrangeButtonsClicked(char arg0, char a3, float* a4, float* a5, uint32_t* a6);
    void InitKnobs();

    // Updates the pip-boy's idle animations.
    static void UpdateIdleAnims();

    NiPointer<NiNode> spPipboyRoot;
    NiPointer<NiNode> sp3rdPersonPipboyRoot;
    NiPointer<NiAVObject> sp3rdPersonScreen;
    NiPointer<NiScreenElements> spPartialScreenGeometry;
    NiPointer<NiAVObject> spButtons[3];
    NiPointer<NiAVObject> spButtonGlow[3];
    NiPointer<NiGeometry> spKnobs[3];
    float fKnobPositions[3];
    float fKnobTargets[3];
    float fKnobRates[3];
    float fTabKnobPositions[5];
    bool bScaledTexCoords;
    uint8_t padding145[0x3];
    float fScrollWheelCurrentRotation;
    float fScrollWheelTarget;
    bool bPipboyUp;
    uint8_t padding151[0x3];
    BSSoundHandle pBackgroundHum;
    bool b1stPersonLightOn;
    bool b3rdPersonLightOn;
    bool bLightEffectFading;
    uint8_t padding163;
    float fLightEffectFadeStartTime;
    float fLightEffectFadeTarget;
    bool bResetPipboyManager;
    uint8_t padding16D[0x3];
};

inline float fLastPipboyKnobUpdateTime;

// The game settings FOPipboyManager.cpp defines.
extern SettingT<INISettingCollection> fTabKnobMoveRate;
extern SettingT<INISettingCollection> bEnableFlickerPipboy;
extern SettingT<INISettingCollection> fLightEffectFadeDuration;
extern SettingT<INISettingCollection> bUsePipboyMode;
extern SettingT<INISettingCollection> fBlurIntensityPipboy;
extern SettingT<INISettingCollection> fScrollKnobIncrement;
extern SettingT<INISettingCollection> fLightEffectLightFadeDuration;
extern SettingT<INISettingCollection> fBlurRadiusPipboy;
extern SettingT<INISettingCollection> fTabKnobMinPosition;
extern SettingT<INISettingCollection> fTabKnobMaxPosition;
extern SettingT<INISettingCollection> fScrollKnobRate;
inline int32_t iClickedOrangeButton = -1;

extern SettingT<INISettingCollection> gs_fScanlineFrequencyPipboy_Pipboy;
