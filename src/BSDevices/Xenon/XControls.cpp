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

#include <cstdint>

static SettingT<INISettingCollection> fAnalogMaxRunZonePercent("fAnalogMaxRunZonePercent:Controls", 0.95f);
static SettingT<INISettingCollection> fAnalogRunZonePercent("fAnalogRunZonePercent:Controls", 0.8f);
static SettingT<INISettingCollection> fMinAnalogWalkSpeed("fMinAnalogWalkSpeed:Controls", 0.1f);
SettingT<INISettingCollection> gs_fMaxAnalogWalkSpeed_Controls("fMaxAnalogWalkSpeed:Controls", 1.8f);
static SettingT<INISettingCollection> fMinAnalogRunSpeed("fMinAnalogRunSpeed:Controls", 0.7f);
static SettingT<INISettingCollection> fMaxAnalogRunSpeed("fMaxAnalogRunSpeed:Controls", 1.0f);

LANGUAGE GetCurrentLanguage() {
    if (g_currentLanguage == -1) {
        char* pLanguage;
        pLanguage = sLanguage.uValue.str;
        if (!_strnicmp(pLanguage, "ENGLISH", 7u)) {
            g_currentLanguage = 0;
        }
        else {
            char* pLanguage2;
            pLanguage2 = sLanguage.uValue.str;
            if (!_strnicmp(pLanguage2, "FRENCH", 6u)) {
                g_currentLanguage = 2;
            }
            else {
                char* pLanguage3;
                pLanguage3 = sLanguage.uValue.str;
                if (!_strnicmp(pLanguage3, "ITALIAN", 7u)) {
                    g_currentLanguage = 4;
                }
                else {
                    char* pLanguage4;
                    pLanguage4 = sLanguage.uValue.str;
                    if (!_strnicmp(pLanguage4, "GERMAN", 6u)) {
                        g_currentLanguage = 1;
                    }
                    else {
                        char* pLanguage5;
                        pLanguage5 = sLanguage.uValue.str;
                        if (!_strnicmp(pLanguage5, "SPANISH", 7u)) {
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
        uint16_t usRightSpeed;
        uint16_t usLeftSpeed;
        _RUMBLE_EFFECT* pEffect;
        _RUMBLE_EFFECT* pNewEffect = static_cast<_RUMBLE_EFFECT*>(
            MemoryManager::Instance().Allocate(sizeof(_RUMBLE_EFFECT))
        );
        if (pNewEffect) {
            std::memset(pNewEffect, 0, sizeof(_RUMBLE_EFFECT));
            pEffect = pNewEffect;
        }
        else {
            pEffect = nullptr;
        }
        pEffect->uiDuration = timeInMillis;
        pEffect->uiWavelength = iWavelength;
        pEffect->pattern = aRumblePattern;
        pEffect->bAlternate = abAlternate;
        pEffect->RampUp.uiDuration = 0;
        pEffect->RampDown.uiDuration = 0;
        pEffect->uiSpeed = static_cast<int64_t>(rightMotor * 65535.0);
        pControllerVibration->InitRumble(RIGHT, pEffect);
        pEffect->uiSpeed = static_cast<int64_t>(leftMotor * 65535.0);
        pControllerVibration->InitRumble(LEFT, pEffect);
        if (static_cast<uint16_t>(static_cast<int32_t>(leftMotor)) <= 1u) {
            usLeftSpeed = static_cast<int32_t>(leftMotor * 65535.0);
        }
        else {
            usLeftSpeed = -1;
        }
        pVibration.wLeftMotorSpeed = usLeftSpeed;
        if (static_cast<uint16_t>(static_cast<int32_t>(rightMotor)) <= 1u) {
            usRightSpeed = static_cast<int32_t>(rightMotor * 65535.0);
        }
        else {
            usRightSpeed = -1;
        }
        pVibration.wRightMotorSpeed = usRightSpeed;
        XInputSetState(0, &pVibration);
    }
}

void Controls::ForceUserAction(Controls::USERACTIONS aeAction) {
    if (aeAction < Controls::UACTN_TOGGLEPOV) {
        int32_t iDevice = ucKeyBinds[0][aeAction] == 255;
        if (ucKeyBinds[iDevice][aeAction] == 255) {
            ++iDevice;
        }
        if (ucKeyBinds[iDevice][aeAction] != 255) {
            if (Controls::bUseController) {
                iDevice = 3;
            }
            if (iDevice > 3) {
                if (iDevice == 255) {
                    return;
                }
            }
            else {
                if (iDevice == 3) {
                    ForceControllerButton(static_cast<GAMEPAD_BUTTON>(ucKeyBinds[3][aeAction]));
                    return;
                }
                if (!iDevice) {
                    ucKeyboardState[0][ucKeyBinds[0][aeAction]] = ucKeyboardState[0u][ucKeyBinds[0u][aeAction]] | 128;
                    return;
                }
            }
            if (iDevice != 1) {
                if (ucKeyBinds[iDevice][aeAction] >= 32u) {
                    uint8_t ucBind = ucKeyBinds[iDevice][aeAction];
                    uint32_t uiCurrentPOV = kJoyStickState[0][0].rgdwPOV[0];
                    int32_t iBindPOV = 0x1194 * (ucBind - 32);
                    if (uiCurrentPOV != -1) {
                        int32_t iDifference = uiCurrentPOV - iBindPOV;
                        if (std::abs(iDifference) == 0x2328) {
                            iBindPOV += static_cast<int32_t>(static_cast<double>(iDifference) * 0.5);
                        }
                        else {
                            if (std::abs(iDifference) == 0x6978) {
                                iBindPOV = 0x7B0C;
                            }
                        }
                    }
                    kJoyStickState[0][0].rgdwPOV[0] = iBindPOV;
                }
                else {
                    kJoyStickState[0][0].rgbButtons[ucKeyBinds[iDevice][aeAction]] |= 128u;
                }
                return;
            }
            uint8_t ucButton = ucKeyBinds[1][aeAction];
            if (ucButton || !bSwapLeftRightMouseButtons) {
                if ((ucButton == 1) && bSwapLeftRightMouseButtons) {
                    ucButton = 0;
                }
            }
            else {
                ucButton = 1;
            }
            kMouseState[0].rgbButtons[ucButton] |= 128u;
        }
    }
}

void Controls::RemoveDuplicateControls(int32_t device, int32_t button) {
    int32_t i;
    for (i = 0; i < 28; ++i) {
        if (ucKeyBinds[device][i] == button) {
            ucKeyBinds[device][i] = static_cast<uint8_t>(-1);
        }
    }
}

void Controls::SaveControls() {
    char FileName[268];
    strcpy_s(FileName, 260, BSUtilities::pPersonalPath);
    strcat_s(FileName, 260, pAppPrefININame);
    for (int32_t iAction = 0; iAction < 28; ++iAction) {
        char String[68];
        BSsprintf(String, 64u, "%08X", GetKeybind(iAction));
        platform::IniWriteString(FileName, "Controls", userActionKeys[iAction], String);
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
        bSettingAControl = false;
        bUpdated = true;
        return true;
    }

    for (int32_t buttonNumber = 1; buttonNumber <= 3; ++buttonNumber) {
        if (GetMouseButton(buttonNumber, 1)) {
            nextKeystroke = buttonNumber;
            SetActionMapping(eActionToSet, 1u, buttonNumber);
            bSettingAControl = false;
            bUpdated = true;
            return true;
        }
    }
    bUpdated = false;
    bSettingAControl = true;
    return false;
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
