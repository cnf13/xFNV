#include "BSDevices/Xenon/XControls.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"

#include <stdint.h>

static SettingT<INISettingCollection> fAnalogMaxRunZonePercent("fAnalogMaxRunZonePercent:Controls", 0.95f);
static SettingT<INISettingCollection> fAnalogRunZonePercent("fAnalogRunZonePercent:Controls", 0.8f);
static SettingT<INISettingCollection> fMinAnalogWalkSpeed("fMinAnalogWalkSpeed:Controls", 0.1f);
SettingT<INISettingCollection> gs_fMaxAnalogWalkSpeed_Controls("fMaxAnalogWalkSpeed:Controls", 1.8f);
static SettingT<INISettingCollection> fMinAnalogRunSpeed("fMinAnalogRunSpeed:Controls", 0.7f);
static SettingT<INISettingCollection> fMaxAnalogRunSpeed("fMaxAnalogRunSpeed:Controls", 1.0f);

LANGUAGE GetCurrentLanguage() {
    if (g_currentLanguage == -1) {
        char* String1;
        String1 = sLanguage.uValue.str;
        if (!_strnicmp(String1, "ENGLISH", 7u)) {
            g_currentLanguage = 0;
        }
        else {
            char* str;
            str = sLanguage.uValue.str;
            if (!_strnicmp(str, "FRENCH", 6u)) {
                g_currentLanguage = 2;
            }
            else {
                char* v3;
                v3 = sLanguage.uValue.str;
                if (!_strnicmp(v3, "ITALIAN", 7u)) {
                    g_currentLanguage = 4;
                }
                else {
                    char* v2;
                    v2 = sLanguage.uValue.str;
                    if (!_strnicmp(v2, "GERMAN", 6u)) {
                        g_currentLanguage = 1;
                    }
                    else {
                        char* v1;
                        v1 = sLanguage.uValue.str;
                        if (!_strnicmp(v1, "SPANISH", 7u)) {
                            g_currentLanguage = 3;
                        }
                    }
                }
            }
        }
    }
    return static_cast<LANGUAGE>(g_currentLanguage);
}

void Controls::Rumble(
    float leftMotor,
    float rightMotor,
    int32_t timeInMillis,
    int32_t iWavelength,
    RUMBLE_PATTERN aRumblePattern,
    bool abAlternate,
    bool bOverride
) {
    _XINPUT_VIBRATION pVibration;
    if (bIsControllerEnabled &&
        (bOverride ||
         !(pControllerVibration->RightTimer.uiRumbleDuration + pControllerVibration->LeftTimer.uiRumbleDuration))) {
        uint16_t v8;
        uint16_t v9;
        _RUMBLE_EFFECT* v10;
        _RUMBLE_EFFECT* v12 = static_cast<_RUMBLE_EFFECT*>(MemoryManager_Singleton.Allocate(sizeof(_RUMBLE_EFFECT)));
        if (v12) {
            std::memset(v12, 0, sizeof(_RUMBLE_EFFECT));
            v10 = v12;
        }
        else {
            v10 = 0;
        }
        v10->uiDuration = timeInMillis;
        v10->uiWavelength = iWavelength;
        v10->pattern = aRumblePattern;
        v10->bAlternate = abAlternate;
        v10->RampUp.uiDuration = 0;
        v10->RampDown.uiDuration = 0;
        v10->uiSpeed = static_cast<int64_t>(rightMotor * 65535.0);
        pControllerVibration->InitRumble(RIGHT, v10);
        v10->uiSpeed = static_cast<int64_t>(leftMotor * 65535.0);
        pControllerVibration->InitRumble(LEFT, v10);
        if (static_cast<uint16_t>(static_cast<int32_t>(leftMotor)) <= 1u) {
            v9 = static_cast<int32_t>(leftMotor * 65535.0);
        }
        else {
            v9 = -1;
        }
        pVibration.wLeftMotorSpeed = v9;
        if (static_cast<uint16_t>(static_cast<int32_t>(rightMotor)) <= 1u) {
            v8 = static_cast<int32_t>(rightMotor * 65535.0);
        }
        else {
            v8 = -1;
        }
        pVibration.wRightMotorSpeed = v8;
        XInputSetState(0, &pVibration);
    }
}

void Controls::ForceUserAction(Controls::USERACTIONS aeAction) {
    if (aeAction < Controls::UACTN_TOGGLEPOV) {
        int32_t v9 = ucKeyBinds[0][aeAction] == 255;
        if (ucKeyBinds[v9][aeAction] == 255) {
            ++v9;
        }
        if (ucKeyBinds[v9][aeAction] != 255) {
            if (Controls::bUseController) {
                v9 = 3;
            }
            if (v9 > 3) {
                if (v9 == 255) {
                    return;
                }
                goto LABEL_24;
            }
            if (v9 == 3) {
                ForceControllerButton(static_cast<GAMEPAD_BUTTON>(ucKeyBinds[3][aeAction]));
                return;
            }
            if (!v9) {
                ucKeyboardState[0][ucKeyBinds[0][aeAction]] = ucKeyboardState[0u][ucKeyBinds[0u][aeAction]] | 128;
                return;
            }
            if (v9 != 1) {
            LABEL_24:;
                if (ucKeyBinds[v9][aeAction] >= 32u) {
                    uint8_t v2 = ucKeyBinds[v9][aeAction];
                    uint32_t v7 = kJoyStickState[0][0].rgdwPOV[0];
                    int32_t v6 = 0x1194 * (v2 - 32);
                    if (v7 != -1) {
                        int32_t Number = v7 - v6;
                        if (std::abs(Number) == 0x2328) {
                            v6 += static_cast<int32_t>(static_cast<double>(Number) * 0.5);
                        }
                        else {
                            if (std::abs(Number) == 0x6978) {
                                v6 = 0x7B0C;
                            }
                        }
                    }
                    uint8_t v3 = ucKeyBinds[v9][aeAction];
                    kJoyStickState[0][0].rgdwPOV[0] = v6;
                }
                else {
                    kJoyStickState[0][0].rgbButtons[ucKeyBinds[v9][aeAction]] |= 128u;
                }
                return;
            }
            uint8_t v8 = ucKeyBinds[1][aeAction];
            if (v8 || !bSwapLeftRightMouseButtons) {
                if (v8 == 1) {
                    if (bSwapLeftRightMouseButtons) {
                        v8 = 0;
                    }
                }
            }
            else {
                v8 = 1;
            }
            kMouseState[0].rgbButtons[v8] |= 128u;
        }
    }
}

void Controls::RemoveDuplicateControls(int32_t device, int32_t button) {
    int32_t i;
    for (i = 0; i < 28; ++i) {
        if (((uint32_t)ucKeyBinds[device][i]) == ((uint32_t)button)) {
            ucKeyBinds[device][i] = (uint8_t)((-1));
        }
    }
}

void Controls::SaveControls() {
    char FileName[268];
    strcpy_s(FileName, 260, BSUtilities::pPersonalPath);
    strcat_s(FileName, 260, pAppPrefININame);
    for (int32_t a2 = 0; a2 < 28; ++a2) {
        char String[68];
        BSsprintf(String, 64u, "%08X", GetKeybind(a2));
        platform::IniWriteString(FileName, "Controls", userActionKeys[a2], String);
    }
}

int32_t Controls::GetNextKeystroke(int32_t& nextKeystroke) {
    return pKeyboard && GetNextKeyEvent(nextKeystroke) == KEY_EVENT_DOWN;
}

bool Controls::SetAction(int32_t action) {
    int32_t nextKeystroke = 0;
    if (action != -1) {
        eActionToSet = action;
    }
    GetNextKeystroke(nextKeystroke);
    if (nextKeystroke) {
        SetActionMapping(eActionToSet, 0, nextKeystroke);
        bSettingAControl = 0;
        bUpdated = 1;
        return 1;
    }
    else {
        for (int32_t buttonNumber = 1; buttonNumber <= 3; ++buttonNumber) {
            if (GetMouseButton(buttonNumber, 1)) {
                nextKeystroke = buttonNumber;
                SetActionMapping(eActionToSet, 1u, buttonNumber);
                bSettingAControl = 0;
                bUpdated = 1;
                return 1;
            }
        }
        bUpdated = 0;
        bSettingAControl = 1;
        return 0;
    }
}

void Controls::KillRumble() {
    _XINPUT_VIBRATION pVibration;
    if (bIsControllerEnabled) {
        pVibration.wLeftMotorSpeed = 0;
        pVibration.wRightMotorSpeed = 0;
        XRumbleControl* pControllerVibration = this->pControllerVibration;
        pControllerVibration->LeftTimer.uiRumbleDuration = 0;
        pControllerVibration->RightTimer.uiRumbleDuration = 0;
        XInputSetState(0, &pVibration);
    }
}

void Controls::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Testing each action from UACTN_FORWARD to UACTN_RESTMENU for equality is the same as an unsigned "below
// UACTN_VATSMENU" (no negative value passes).
bool Controls::Remappable(Controls::USERACTIONS aeAction) {
    return static_cast<uint32_t>(aeAction) < static_cast<uint32_t>(Controls::UACTN_VATSMENU) ||
        (aeAction == Controls::UACTN_QUICKSAVE ||
         (aeAction == Controls::UACTN_QUICKLOAD ||
          (aeAction == Controls::UACTN_VATSMENU ||
           (aeAction == Controls::UACTN_GRAB || aeAction == Controls::UACTN_HOTKEY2))));
}
