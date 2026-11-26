#include "BSDevices/Controls.h"

#include "BSCore/MemoryManager.h"
#include "BSDevices/Xenon/XControls.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"
#include "platform/System.h"

#include <SDL3/SDL_init.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>

// The mouse background status SetMouseBackgroundStatus recorded for UpdateValues: 1 or 0, -1 for none.
static std::atomic<int32_t> siPendingMouseBackground{-1};

SettingT<INISettingCollection> gs_bUseJoystick_Controls("bUse Joystick:Controls", true);
SettingT<INISettingCollection> gs_bBackgroundKeyboard_Controls("bBackground Keyboard:Controls", true);
SettingT<INISettingCollection> gs_bBackground_Mouse_Controls("bBackground Mouse:Controls", false);
SettingT<INISettingCollection> gs_fForegroundMouseMult_Controls("fForegroundMouseMult:Controls", 4.0f);
SettingT<INISettingCollection> gs_fForegroundMouseBase_Controls("fForegroundMouseBase:Controls", 0.2f);
SettingT<INISettingCollection> gs_fForegroundMouseAccelTop_Controls("fForegroundMouseAccelTop:Controls", 50.0f);
SettingT<INISettingCollection> gs_fForegroundMouseAccelBase_Controls("fForegroundMouseAccelBase:Controls", 0.2f);

XRumbleControl::XRumbleControl(int32_t aiNewDevice) : iDevice(aiNewDevice), dwOldTime(GetTickCount()) {}

XRumbleControl::~XRumbleControl() {}

void XRumbleControl::InitRumble(RUMBLE_SIDE aeLeftOrRight, _RUMBLE_EFFECT* apMotor) {
    if (aeLeftOrRight) {
        this->RightTimer.uiRumbleCounter = 0;
        this->RightTimer.uiRampCounter = 0;
        if (!apMotor->uiWavelength) {
            apMotor->uiWavelength = apMotor->uiDuration;
        }
        this->pRightEffect = apMotor;
        this->RightTimer.uiRumbleDuration = apMotor->uiDuration;
        this->RightTimer.uiRampUpDuration = apMotor->RampUp.uiDuration;
        this->RightTimer.uiRampDownDuration = apMotor->RampDown.uiDuration;
        apMotor->RampUp.wEndSpeed = apMotor->uiSpeed;
        apMotor->RampDown.wStartSpeed = apMotor->uiSpeed;
    }
    else {
        this->LeftTimer.uiRumbleCounter = 0;
        this->LeftTimer.uiRampCounter = 0;
        if (!apMotor->uiWavelength) {
            apMotor->uiWavelength = apMotor->uiDuration;
        }
        this->pLeftEffect = apMotor;
        this->LeftTimer.uiRumbleDuration = apMotor->uiDuration;
        this->LeftTimer.uiRampUpDuration = apMotor->RampUp.uiDuration;
        this->LeftTimer.uiRampDownDuration = apMotor->RampDown.uiDuration;
        apMotor->RampUp.wEndSpeed = apMotor->uiSpeed;
        apMotor->RampDown.wStartSpeed = apMotor->uiSpeed;
    }
}

void XRumbleControl::UpdateRumble(int32_t iTime) {
    _XINPUT_VIBRATION XInput;
    if (iTime >= 1) {
        XInput.wLeftMotorSpeed = 0;
        XInput.wRightMotorSpeed = 0;
        Rumble(LEFT, &XInput, static_cast<uint32_t>(iTime));
        Rumble(RIGHT, &XInput, static_cast<uint32_t>(iTime));
        XInputSetState(static_cast<uint32_t>(iDevice), &XInput);
    }
}

// The speed is built in a local that starts at 0; the constant pattern's cap is an unsigned compare.
uint16_t XRumbleControl::MainRumble(_RUMBLE_EFFECT* apMotorEffect, uint32_t aiRumbleCounter) {
    uint16_t wSpeed = 0;
    switch (apMotorEffect->pattern) {
        case CONSTANT: wSpeed = PatternConstant(apMotorEffect); break;
        case PERIODIC_SQUARE: wSpeed = PatternPeriodicSquare(apMotorEffect, aiRumbleCounter); break;
        default: break;
    }
    return wSpeed;
}

uint16_t XRumbleControl::PatternPeriodicSquare(_RUMBLE_EFFECT* apMotorEffect, uint32_t aiRumbleCounter) {
    std::sin(static_cast<double>(aiRumbleCounter) * 0.01745329238474369);
    if (static_cast<int32_t>(aiRumbleCounter % 360) >= 180) {
        return 0;
    }

    return apMotorEffect->uiSpeed;
}

uint16_t XRumbleControl::RampUp(_RUMBLE_EFFECT* MotorEffect, uint32_t iRumbleCounter) {
    if (MotorEffect->RampUp.uiDuration) {
        return iRumbleCounter *
            ((MotorEffect->RampUp.wEndSpeed - MotorEffect->RampUp.wStartSpeed) /
             static_cast<uint16_t>(MotorEffect->RampUp.uiDuration));
    }

    return 0;
}

uint16_t XRumbleControl::RampDown(_RUMBLE_EFFECT* MotorEffect, uint32_t iRumbleCounter) {
    if (MotorEffect->RampDown.uiDuration) {
        return iRumbleCounter *
            ((MotorEffect->RampDown.wEndSpeed - MotorEffect->RampDown.wStartSpeed) /
             static_cast<uint16_t>(MotorEffect->RampDown.uiDuration));
    }

    return 0;
}

void XRumbleControl::Rumble(RUMBLE_SIDE aeLeftOrRight, _XINPUT_VIBRATION* XInput, uint32_t iTime) {
    _RUMBLE_TIMER* pTimer;
    _RUMBLE_EFFECT* pEffect;
    uint16_t usSpeed = 0;
    if (aeLeftOrRight) {
        pTimer = &RightTimer;
        pEffect = pRightEffect;
    }
    else {
        pTimer = &LeftTimer;
        pEffect = pLeftEffect;
    }
    if (!pTimer->uiRumbleDuration) {
        return;
    }
    pTimer->uiRumbleCounter += iTime;
    if (pTimer->uiRumbleCounter >= pTimer->uiRumbleDuration) {
        RightTimer.uiRumbleDuration = 0;
        RightTimer.uiRumbleCounter = 0;
        LeftTimer.uiRumbleDuration = 0;
        LeftTimer.uiRumbleCounter = 0;
        XInput->wLeftMotorSpeed = 0;
        XInput->wRightMotorSpeed = 0;
        return;
    }
    float fHalfWave = static_cast<double>(pEffect->uiWavelength) / 2.0;
    float fWavePos = static_cast<double>(pTimer->uiRumbleCounter % pEffect->uiWavelength);
    bool bAlternate = aeLeftOrRight && pEffect->bAlternate;
    switch (pEffect->pattern) {
        case PERIODIC_SAWTOOTH:
            if (!bAlternate) {
                if (fHalfWave > fWavePos) {
                    usSpeed = static_cast<int32_t>(static_cast<double>(pEffect->uiSpeed) * (fWavePos / fHalfWave));
                }
                else {
                    usSpeed = static_cast<int32_t>(
                        static_cast<double>(pEffect->uiSpeed) * ((fWavePos - fHalfWave) / fHalfWave)
                    );
                }
            }
            else {
                if (fHalfWave > fWavePos) {
                    usSpeed = static_cast<int32_t>(
                        static_cast<double>(pEffect->uiSpeed) * (1.0 - fWavePos / fHalfWave)
                    );
                }
                else {
                    usSpeed = static_cast<int32_t>(
                        static_cast<double>(pEffect->uiSpeed) * (1.0 - (fWavePos - fHalfWave) / fHalfWave)
                    );
                }
            }
            break;
        case PERIODIC_TRIANGLE:
            // Computes the triangle's speed, then deliberately falls through into the square wave's case, which
            // overwrites it.
            if (!bAlternate) {
                if (fHalfWave > static_cast<double>(pTimer->uiRumbleCounter)) {
                    usSpeed = static_cast<int32_t>(static_cast<double>(pEffect->uiSpeed) * (fWavePos / fHalfWave));
                }
                else {
                    usSpeed = static_cast<int32_t>(
                        static_cast<double>(pEffect->uiSpeed) * (1.0 - (fWavePos - fHalfWave) / fHalfWave)
                    );
                }
            }
            else {
                if (fHalfWave > static_cast<double>(pTimer->uiRumbleCounter)) {
                    usSpeed = static_cast<int32_t>(
                        static_cast<double>(pEffect->uiSpeed) * (1.0 - fWavePos / fHalfWave)
                    );
                }
                else {
                    usSpeed = static_cast<int32_t>(
                        static_cast<double>(pEffect->uiSpeed) * (fWavePos - fHalfWave / fHalfWave)
                    );
                }
            }
            [[fallthrough]];
        case PERIODIC_SQUARE:
            if (!bAlternate) {
                if (fHalfWave < fWavePos) {
                    usSpeed = pEffect->uiSpeed;
                }
                else {
                    usSpeed = 0;
                }
            }
            else {
                if (fHalfWave < fWavePos) {
                    usSpeed = 0;
                }
                else {
                    usSpeed = pEffect->uiSpeed;
                }
            }
            break;
        default: usSpeed = pEffect->uiSpeed; break;
    }
    if (aeLeftOrRight) {
        XInput->wRightMotorSpeed = usSpeed;
    }
    else {
        XInput->wLeftMotorSpeed = usSpeed;
    }
}

Controls::Controls(HINSTANCE__* ahInstance) : uiFlags(0), uiJoyStickCount(0), usOverrideFlags(0) {
    bool bUsesGamepad;
    IDirectInputDevice8A* pDevice;
    DirectInput8Create_0(ahInstance, 2048u, &CLSID_IDirectInput8A, reinterpret_cast<void**>(&pDirectInput), nullptr);
    bUsesGamepad = gs_bUseJoystick_Controls.uValue.b;
    if (bUsesGamepad) {
        EnableJoysticks(true);
    }
    pKeyboard = nullptr;
    if (!pDirectInput->CreateDevice(&CLSID_GUID_SysKeyboard, &pDevice, nullptr)) {
        bool bBackgroundKeyboard;
        DIPROPDWORD kBufferSize;
        uiFlags |= FLAG_HAS_KEYBOARD;
        pKeyboard = pDevice;
        bBackgroundKeyboard = gs_bBackgroundKeyboard_Controls.uValue.b;
        pKeyboard->SetCooperativeLevel(GetActiveWindow_SDL(), bBackgroundKeyboard ? 22 : 21);
        pKeyboard->SetDataFormat(&c_dfDIKeyboard);
        kBufferSize.diph.dwSize = sizeof(DIPROPDWORD);
        kBufferSize.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        kBufferSize.diph.dwHow = 0;
        kBufferSize.diph.dwObj = 0;
        kBufferSize.dwData = 32;
        // DIPROP_BUFFERSIZE is MAKEDIPROP(1).
        pKeyboard->SetProperty(reinterpret_cast<const void*>(1), &kBufferSize.diph);
    }
    pMouse = nullptr;
    // HKEY_CURRENT_USER\Control Panel\Mouse's MouseSensitivity read with atoi (6 when missing), SwapMouseButton's
    // state (swapped and restored) and GetDoubleClickTime.
    const platform::MouseInfo kMouse = platform::MouseSettings();
    cMouseSensitivity = static_cast<uint8_t>(kMouse.sensitivity);
    bSwapLeftRightMouseButtons = kMouse.swapButtons;
    uiDoubleClickTime = kMouse.doubleClickTime;
    std::memset(uiLastClickedTime, 0, sizeof(uiLastClickedTime));
    std::memset(&kMouseCaps, 0, sizeof(kMouseCaps));
    kMouseCaps.dwSize = sizeof(DIDEVCAPS);
    bool bBackgroundMouse;
    bBackgroundMouse = gs_bBackground_Mouse_Controls.uValue.b;
    if (bBackgroundMouse) {
        uiFlags |= FLAG_BACKGROUND_MOUSE;
    }
    if (!pDirectInput->CreateDevice(&CLSID_GUID_SysMouse, &pDevice, nullptr)) {
        uiFlags |= FLAG_HAS_MOUSE;
        pMouse = pDevice;
        pMouse->GetCapabilities(&kMouseCaps);
        pMouse->SetCooperativeLevel(GetActiveWindow_SDL(), (uiFlags & FLAG_BACKGROUND_MOUSE) != 0 ? 6 : 5);
        pMouse->SetDataFormat(&c_dfDIMouse2);
    }
    std::memset(kJoyStickState, 0, sizeof(kJoyStickState));
    std::memset(ucKeyboardState, 0, sizeof(ucKeyboardState));
    std::memset(kMouseState, 0, sizeof(kMouseState));
    bSettingAControl = false;
    bUpdated = false;
    pControllerVibration = new XRumbleControl(0);
    // Freed through MemoryManager::Deallocate by the destructor, so allocated through the MemoryManager.
    pVibrationProperties = static_cast<_RUMBLE_EFFECT*>(MemoryManager::Instance().Allocate(sizeof(_RUMBLE_EFFECT)));
    std::memset(pVibrationProperties, 0, sizeof(_RUMBLE_EFFECT));
    pVibrationProperties->uiDuration = 0;
    pVibrationProperties->uiSpeed = 0;
    pVibrationProperties->pattern = PERIODIC_TRIANGLE;
    pVibrationProperties->RampUp.uiDuration = 0;
    pVibrationProperties->RampDown.uiDuration = 0;
    pControllerVibration->InitRumble(RIGHT, pVibrationProperties);
    pControllerVibration->InitRumble(LEFT, pVibrationProperties);
    bIsControllerEnabled = true;
    InitDefaultKeybinds(4);
    bIsControllerDisabled = false;
}

// Deliberately frees neither pControllerVibration nor the joystick devices here.
Controls::~Controls() {
    if ((uiFlags & FLAG_HAS_JOYSTICKS) != 0) {
        EnableJoysticks(false);
    }
    if (pKeyboard) {
        pKeyboard->Unacquire();
        pKeyboard->Release();
    }
    if (pMouse) {
        pMouse->Unacquire();
        pMouse->Release();
    }
    pDirectInput->Release();
    if (pVibrationProperties) {
        MemoryManager::Instance().Deallocate(pVibrationProperties);
    }
}

uint32_t Controls::HasInputInterfaces(int32_t aiDevice) {
    if (aiDevice == 0) {
        return HasKeyboard();
    }
    if (aiDevice == 1) {
        return HasMouse();
    }
    if (aiDevice == 2) {
        return HasJoySticks();
    }
    return 0;
}

void Controls::SyncLastButtonState(int32_t aiControl) {
    int32_t iButtons = MapToXInput(static_cast<GAMEPAD_BUTTON>(aiControl));
    kLastGamepadState.Gamepad.wButtons &= ~iButtons;
    kLastGamepadState.Gamepad.wButtons |= kCurrentGamepadState.Gamepad.wButtons & iButtons;
}

int32_t Controls::GetJoystickAxisCount(int32_t aiJoystick) {
    return static_cast<int32_t>(kJoyStickCaps[aiJoystick].dwAxes);
}

void Controls::InitDefaultKeybinds(int32_t aiDevice) {
    int32_t j;
    int32_t i;
    for (i = 0; i < 28; ++i) {
        if (aiDevice >= 4) {
            for (j = 0; j < 4; ++j) {
                ucKeyBinds[j][i] = static_cast<uint8_t>(-1);
            }
        }
        else {
            ucKeyBinds[aiDevice][i] = static_cast<uint8_t>(-1);
        }
    }
    if ((!aiDevice) || (aiDevice == 4)) {
        ucKeyBinds[0][UACTN_FORWARD] = W;
        ucKeyBinds[0][UACTN_BACK] = S;
        ucKeyBinds[0][UACTN_SLDLEFT] = A;
        ucKeyBinds[0][UACTN_SLDRIGHT] = D;
        ucKeyBinds[0][UACTN_ACTIVATE] = E;
        ucKeyBinds[0][UACTN_BLOCK] = LeftAlt;
        ucKeyBinds[0][UACTN_READY_RELOAD_WEAPON] = R;
        ucKeyBinds[0][UACTN_CROUCH] = LeftControl;
        ucKeyBinds[0][UACTN_RUN] = LeftShift;
        ucKeyBinds[0][UACTN_TOGGLERUN] = CapsLock;
        ucKeyBinds[0][UACTN_AUTOMOVE] = Q;
        ucKeyBinds[0][UACTN_JUMP] = SpaceBar;
        ucKeyBinds[0][UACTN_TOGGLEPOV] = F;
        ucKeyBinds[0][UACTN_RESTMENU] = T;
        ucKeyBinds[0][UACTN_MENUMODE] = Tab;
        ucKeyBinds[0][UACTN_VATSMENU] = V;
        ucKeyBinds[0][UACTN_HOTKEY1] = One;
        ucKeyBinds[0][UACTN_HOTKEY2] = Two;
        ucKeyBinds[0][UACTN_HOTKEY3] = Three;
        ucKeyBinds[0][UACTN_HOTKEY4] = Four;
        ucKeyBinds[0][UACTN_HOTKEY5] = Five;
        ucKeyBinds[0][UACTN_HOTKEY6] = Six;
        ucKeyBinds[0][UACTN_HOTKEY7] = Seven;
        ucKeyBinds[0][UACTN_HOTKEY8] = Eight;
        ucKeyBinds[0][UACTN_QUICKSAVE] = F5;
        ucKeyBinds[0][UACTN_QUICKLOAD] = F9;
        ucKeyBinds[0][UACTN_GRAB] = Z;
    }
    if ((aiDevice == 1) || (aiDevice == 4)) {
        ucKeyBinds[1][UACTN_USE] = 0;
        ucKeyBinds[1][UACTN_BLOCK] = 1;
        ucKeyBinds[1][UACTN_TOGGLEPOV] = 2;
    }
    if (((aiDevice == 2) || (aiDevice == 4)) && ((uiFlags & FLAG_HAS_JOYSTICKS) != 0)) {
        ucKeyBinds[2][UACTN_CROUCH] = 0;
        ucKeyBinds[2][UACTN_JUMP] = 1;
        ucKeyBinds[2][UACTN_READY_RELOAD_WEAPON] = 2;
        ucKeyBinds[2][UACTN_ACTIVATE] = 3;
        ucKeyBinds[2][UACTN_MENUMODE] = 4;
        ucKeyBinds[2][UACTN_TOGGLEPOV] = 5;
        ucKeyBinds[2][UACTN_GRAB] = 7;
        ucKeyBinds[2][UACTN_BLOCK] = 8;
        ucKeyBinds[2][UACTN_USE] = 9;
        ucKeyBinds[2][UACTN_RESTMENU] = 10;
        ucKeyBinds[2][UACTN_HOTKEY2] = 32;
        ucKeyBinds[2][UACTN_HOTKEY1] = 33;
        ucKeyBinds[2][UACTN_HOTKEY3] = 34;
        ucKeyBinds[2][UACTN_HOTKEY4] = 35;
        ucKeyBinds[2][UACTN_HOTKEY5] = 36;
        ucKeyBinds[2][UACTN_HOTKEY6] = 37;
        ucKeyBinds[2][UACTN_HOTKEY7] = 38;
        ucKeyBinds[2][UACTN_HOTKEY8] = 39;
    }
    if ((aiDevice == 3) || (aiDevice == 4)) {
        ucKeyBinds[3][UACTN_FORWARD] = GP_LTHUMB_UP;
        ucKeyBinds[3][UACTN_BACK] = GP_LTHUMB_DOWN;
        ucKeyBinds[3][UACTN_SLDLEFT] = GP_LTHUMB_LEFT;
        ucKeyBinds[3][UACTN_SLDRIGHT] = GP_LTHUMB_RIGHT;
        ucKeyBinds[3][UACTN_CROUCH] = GP_LTHUMB_BUTTON;
        ucKeyBinds[3][UACTN_JUMP] = GP_Y;
        ucKeyBinds[3][UACTN_READY_RELOAD_WEAPON] = GP_X;
        ucKeyBinds[3][UACTN_ACTIVATE] = GP_A;
        ucKeyBinds[3][UACTN_MENUMODE] = GP_B;
        ucKeyBinds[3][UACTN_TOGGLEPOV] = GP_LBUTTON;
        ucKeyBinds[3][UACTN_GRAB] = GP_RTHUMB_BUTTON;
        ucKeyBinds[3][UACTN_BLOCK] = GP_LTRIGGER;
        ucKeyBinds[3][UACTN_USE] = GP_RTRIGGER;
        ucKeyBinds[3][UACTN_RESTMENU] = GP_BACK;
        ucKeyBinds[3][UACTN_VATSMENU] = GP_RBUTTON;
        ucKeyBinds[3][UACTN_HOTKEY2] = GP_DPAD_UP;
    }
}

void Controls::ResetControlState(int32_t aiControl) {
    int32_t i;
    for (i = 0; i < 4; ++i) {
        if (ucKeyBinds[i][aiControl] != 255) {
            if (i) {
                if (i == 1) {
                    kMouseState[0].rgbButtons[ucKeyBinds[1][aiControl]] = 0;
                }
                else {
                    if (i == 2) {
                        if (ucKeyBinds[2][aiControl] >= 0x20u) {
                            kJoyStickState[0][0].rgdwPOV[0] = 0;
                        }
                        else {
                            kJoyStickState[0][0].rgbButtons[ucKeyBinds[i][aiControl]] = 0;
                        }
                    }
                }
            }
            else {
                ucKeyboardState[0][ucKeyBinds[0][aiControl]] = 0;
            }
        }
    }
    Controls::kCurrentGamepadState.Gamepad.bLeftTrigger = 0;
    Controls::kCurrentGamepadState.Gamepad.bRightTrigger = 0;
    Controls::kCurrentGamepadState.Gamepad.sThumbLX = 0;
    Controls::kCurrentGamepadState.Gamepad.sThumbLY = 0;
    Controls::kCurrentGamepadState.Gamepad.sThumbRX = 0;
    Controls::kCurrentGamepadState.Gamepad.sThumbRY = 0;
    Controls::kCurrentGamepadState.Gamepad.wButtons = 0;
}

int32_t Controls::MapToXInput(GAMEPAD_BUTTON aeButton) {
    int32_t iButton = 0;
    switch (aeButton) {
        case GP_DPAD_UP: iButton = XINPUT_GAMEPAD_DPAD_UP; break;
        case GP_DPAD_DOWN: iButton = XINPUT_GAMEPAD_DPAD_DOWN; break;
        case GP_DPAD_RIGHT: iButton = XINPUT_GAMEPAD_DPAD_RIGHT; break;
        case GP_DPAD_LEFT: iButton = XINPUT_GAMEPAD_DPAD_LEFT; break;
        case GP_START: iButton = XINPUT_GAMEPAD_START; break;
        case GP_BACK: iButton = XINPUT_GAMEPAD_BACK; break;
        case GP_LTHUMB_BUTTON: iButton = XINPUT_GAMEPAD_LEFT_THUMB; break;
        case GP_RTHUMB_BUTTON: iButton = XINPUT_GAMEPAD_RIGHT_THUMB; break;
        case GP_A: iButton = XINPUT_GAMEPAD_A; break;
        case GP_B: iButton = XINPUT_GAMEPAD_B; break;
        case GP_X: iButton = XINPUT_GAMEPAD_X; break;
        case GP_Y: iButton = XINPUT_GAMEPAD_Y; break;
        case GP_RBUTTON: iButton = XINPUT_GAMEPAD_RIGHT_SHOULDER; break;
        case GP_LBUTTON: iButton = XINPUT_GAMEPAD_LEFT_SHOULDER; break;
        default: break;
    }
    return iButton;
}

void Controls::ClearKeystrokes() {
    if (this->pKeyboard) {
        uint32_t uiItems = 0xFFFFFFFF;
        this->pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), nullptr, &uiItems, 0);
    }
}

HRESULT Controls::SetDeviceRange(int32_t aiJoystick, int32_t aiMin, int32_t aiMax) {
    IDirectInputDevice8A* pDevice = this->pDevices[aiJoystick];
    DIPROPRANGE kRange;
    kRange.diph.dwSize = sizeof(DIPROPRANGE);
    kRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    kRange.diph.dwHow = 0;
    kRange.diph.dwObj = 0;
    kRange.lMin = aiMin;
    kRange.lMax = aiMax;
    // DIPROP_RANGE is MAKEDIPROP(4).
    return pDevice->SetProperty(reinterpret_cast<const void*>(4), &kRange.diph);
}

KEY_EVENT_TYPE Controls::GetNextKeyEvent(int32_t& Key) {
    if (this->pKeyboard != nullptr) {
        uint32_t uiItems = 1;
        DIDEVICEOBJECTDATA kData;
        if (this->pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &kData, &uiItems, 0) == 0 && uiItems) {
            KEY_EVENT_TYPE eEvent = (kData.dwData & 0x80) ? KEY_EVENT_DOWN : KEY_EVENT_UP;
            Key = kData.dwOfs;
            return eEvent;
        }
    }
    return KEY_EVENT_NONE;
}

int32_t Controls::GetControllerButton(GAMEPAD_BUTTON aeButton, CONTROL_TOGGLE_FLAGS aeFlags) {
    if (!Controls::bUseController) {
        return 0;
    }
    uint8_t bResult = 0;
    int32_t iButton = 0;
    // 7849 is XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE.
    switch (aeButton) {
        case GP_LTHUMB_UP:
            if (Controls::kCurrentGamepadState.Gamepad.sThumbLY > 7849) {
                bResult = 1;
            }
            break;
        case GP_LTHUMB_DOWN:
            if (Controls::kCurrentGamepadState.Gamepad.sThumbLY < -7849) {
                bResult = 1;
            }
            break;
        case GP_LTHUMB_RIGHT:
            if (Controls::kCurrentGamepadState.Gamepad.sThumbLX > 7849) {
                bResult = 1;
            }
            break;
        case GP_LTHUMB_LEFT:
            if (Controls::kCurrentGamepadState.Gamepad.sThumbLX < -7849) {
                bResult = 1;
            }
            break;
        default: iButton = this->MapToXInput(aeButton); break;
    }
    if (aeButton == GP_LTRIGGER) {
        switch (aeFlags) {
            case isHeld:
                if (Controls::kCurrentGamepadState.Gamepad.bLeftTrigger > 0) {
                    bResult = 1;
                }
                break;
            case isPressed:
                if (Controls::kCurrentGamepadState.Gamepad.bLeftTrigger > 0 &&
                    Controls::kLastGamepadState.Gamepad.bLeftTrigger == 0) {
                    bResult = 1;
                }
                break;
            case isDepressed:
                if (Controls::kCurrentGamepadState.Gamepad.bLeftTrigger == 0 &&
                    Controls::kLastGamepadState.Gamepad.bLeftTrigger > 0) {
                    bResult = 1;
                }
                break;
            case isChanged:
                if (Controls::kCurrentGamepadState.Gamepad.bLeftTrigger !=
                    Controls::kLastGamepadState.Gamepad.bLeftTrigger) {
                    bResult = 1;
                }
                break;
            default: break;
        }
    }
    else if (aeButton == GP_RTRIGGER) {
        switch (aeFlags) {
            case isHeld:
                if (Controls::kCurrentGamepadState.Gamepad.bRightTrigger > 0) {
                    bResult = 1;
                }
                break;
            case isPressed:
                if (Controls::kCurrentGamepadState.Gamepad.bRightTrigger > 0 &&
                    Controls::kLastGamepadState.Gamepad.bRightTrigger == 0) {
                    bResult = 1;
                }
                break;
            case isDepressed:
                if (Controls::kCurrentGamepadState.Gamepad.bRightTrigger == 0 &&
                    Controls::kLastGamepadState.Gamepad.bRightTrigger > 0) {
                    bResult = 1;
                }
                break;
            case isChanged:
                if (Controls::kCurrentGamepadState.Gamepad.bRightTrigger !=
                    Controls::kLastGamepadState.Gamepad.bRightTrigger) {
                    bResult = 1;
                }
                break;
            default: break;
        }
    }
    else if (!bResult) {
        int32_t iCurrent = Controls::kCurrentGamepadState.Gamepad.wButtons & iButton;
        int32_t iLast = Controls::kLastGamepadState.Gamepad.wButtons & iButton;
        switch (aeFlags) {
            case isHeld:
                if (iCurrent != 0) {
                    bResult = 1;
                }
                break;
            case isPressed:
                if (iLast == 0 && iCurrent != 0) {
                    bResult = 1;
                }
                break;
            case isDepressed:
                if (iCurrent == 0 && iLast != 0) {
                    bResult = 1;
                }
                break;
            case isChanged:
                if (iCurrent != iLast) {
                    bResult = 1;
                }
                break;
            default: break;
        }
    }
    return bResult;
}

// Maps a scan code to the keystroke for the current language; -1 outside 0..0xFF.
int32_t Controls::TranslateKeystroke(int32_t keystrokeValue, int32_t shiftToggle) {
    int32_t result;
    if (keystrokeValue < 0 || keystrokeValue > 0xFF) {
        return -1;
    }
    if (shiftToggle == 255) {
        return translateShifted_English[keystrokeValue];
    }
    switch (GetCurrentLanguage()) {
        case LANGUAGE_GERMAN: {
            uint8_t ucKeystroke;
            if (shiftToggle) {
                ucKeystroke = translateShifted_German[keystrokeValue];
            }
            else {
                ucKeystroke = translateNormal_German[keystrokeValue];
            }
            result = ucKeystroke;
            break;
        }
        case LANGUAGE_FRENCH: {
            uint8_t ucKeystroke;
            if (shiftToggle) {
                ucKeystroke = translateShifted_French[keystrokeValue];
            }
            else {
                ucKeystroke = translateNormal_French[keystrokeValue];
            }
            result = ucKeystroke;
            break;
        }
        case LANGUAGE_SPANISH: {
            uint8_t ucKeystroke;
            if (shiftToggle) {
                ucKeystroke = translateShifted_Spanish[keystrokeValue];
            }
            else {
                ucKeystroke = translateNormal_Spanish[keystrokeValue];
            }
            result = ucKeystroke;
            break;
        }
        case LANGUAGE_ITALIAN: {
            uint8_t ucKeystroke;
            if (shiftToggle) {
                ucKeystroke = translateShifted_Italian[keystrokeValue];
            }
            else {
                ucKeystroke = translateNormal_Italian[keystrokeValue];
            }
            result = ucKeystroke;
            break;
        }
        default: {
            uint8_t ucKeystroke;
            if (shiftToggle) {
                ucKeystroke = translateShifted_English[keystrokeValue];
            }
            else {
                ucKeystroke = translateNormal_English[keystrokeValue];
            }
            result = ucKeystroke;
            break;
        }
    }
    return result;
}

int32_t Controls::GetControllerValue(int32_t aiController, int32_t aiKey) {
    int32_t result;
    switch (aiKey) {
        case 1: {
            result = kJoyStickState[aiController][0].lX;
            break;
        }
        case 2: {
            result = kJoyStickState[aiController][0].lY;
            break;
        }
        case 3: {
            result = kJoyStickState[aiController][0].lZ;
            break;
        }
        case 4: {
            result = kJoyStickState[aiController][0].lRx;
            break;
        }
        case 5: {
            result = kJoyStickState[aiController][0].lRy;
            break;
        }
        case 6: {
            result = kJoyStickState[aiController][0].lRz;
            break;
        }
        case 7: {
            result = Controls::bUseController ? Controls::kCurrentGamepadState.Gamepad.sThumbLX : 0;
            break;
        }
        case 8: {
            result = Controls::bUseController ? Controls::kCurrentGamepadState.Gamepad.sThumbLY : 0;
            break;
        }
        case 9: {
            result = Controls::bUseController ? Controls::kCurrentGamepadState.Gamepad.sThumbRX : 0;
            break;
        }
        case 10: {
            result = Controls::bUseController ? Controls::kCurrentGamepadState.Gamepad.sThumbRY : 0;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

int32_t Controls::GetMouseAxis(int32_t aiAxis) {
    switch (aiAxis) {
        case 1: return this->kMouseState[0].lX;
        case 2: return this->kMouseState[0].lY;
        case 3: return this->kMouseState[0].lZ;
    }
    return 0;
}

int32_t Controls::IsMouseInBackground() {
    return (uiFlags & FLAG_BACKGROUND_MOUSE) != 0;
}

int8_t Controls::GetMouseSensitivity() {
    return static_cast<int8_t>(cMouseSensitivity);
}

float Controls::GetForegroundMouseBase() {
    return gs_fForegroundMouseBase_Controls.Float();
}

float Controls::GetForegroundMouseMult() {
    return gs_fForegroundMouseMult_Controls.Float();
}

float Controls::GetForegroundMouseAccelTop() {
    return gs_fForegroundMouseAccelTop_Controls.Float();
}

float Controls::GetForegroundMouseAccelBase() {
    return gs_fForegroundMouseAccelBase_Controls.Float();
}

int32_t Controls::GetKeyboardButton(int32_t aiButton, int32_t aiFlags) {
    int32_t iResult = 0;
    switch (aiFlags) {
        case isHeld:
            if (this->ucKeyboardState[0][aiButton] & 0x80) {
                iResult = 1;
            }
            break;
        case isPressed:
            if (!(this->ucKeyboardState[1][aiButton] & 0x80) && (this->ucKeyboardState[0][aiButton] & 0x80)) {
                iResult = 1;
            }
            break;
        case isDepressed:
            if (!(this->ucKeyboardState[0][aiButton] & 0x80) && (this->ucKeyboardState[1][aiButton] & 0x80)) {
                iResult = 1;
            }
            break;
        case isChanged:
            if ((this->ucKeyboardState[0][aiButton] & 0x80) != (this->ucKeyboardState[1][aiButton] & 0x80)) {
                iResult = 1;
            }
            break;
        default: break;
    }
    return iResult;
}

int32_t Controls::GetMouseButton(int32_t buttonNumber, int32_t toggleFlags) {
    int32_t iResult = 0;
    // 8 and 9 are the wheel turning up and down.
    if (buttonNumber == 8 || buttonNumber == 9) {
        if (buttonNumber == 8 && this->kMouseState[0].lZ > 0) {
            return 1;
        }
        if (buttonNumber == 9 && this->kMouseState[0].lZ < 0) {
            return 1;
        }
        return 0;
    }
    if (buttonNumber == 0 && this->bSwapLeftRightMouseButtons) {
        buttonNumber = 1;
    }
    else if (buttonNumber == 1 && this->bSwapLeftRightMouseButtons) {
        buttonNumber = 0;
    }
    switch (toggleFlags) {
        case isHeld:
            if (this->kMouseState[0].rgbButtons[buttonNumber] & 0x80) {
                iResult = 1;
            }
            break;
        case isPressed:
            if (!(this->kMouseState[1].rgbButtons[buttonNumber] & 0x80) &&
                (this->kMouseState[0].rgbButtons[buttonNumber] & 0x80)) {
                iResult = 1;
            }
            break;
        case isDepressed:
            if (!(this->kMouseState[0].rgbButtons[buttonNumber] & 0x80) &&
                (this->kMouseState[1].rgbButtons[buttonNumber] & 0x80)) {
                iResult = 1;
            }
            break;
        case isChanged:
            if ((this->kMouseState[0].rgbButtons[buttonNumber] & 0x80) !=
                (this->kMouseState[1].rgbButtons[buttonNumber] & 0x80)) {
                iResult = 1;
            }
            break;
        case isDoubleClicked:
            if (buttonNumber >= 0 && buttonNumber <= 7 && this->bDoubleClicked[buttonNumber]) {
                iResult = 1;
            }
            break;
        default: break;
    }
    return iResult;
}

int32_t Controls::GetNumMouseButtons() {
    return kMouseCaps.dwButtons;
}

int32_t Controls::GetUserAction(USERACTIONS eAction, int32_t toggleFlags) {
    int8_t cSuppressed;
    int32_t i;
    int32_t buttonState;
    if (eAction >= UACTN_UP) {
        if (Controls::bUseController) {
            if ((eAction == UACTN_UP) && GetControllerButton(GP_START, Controls::isPressed)) {
                return 1;
            }
        }
        else {
            switch (static_cast<int32_t>(eAction)) {
                case UACTN_UP: {
                    if (GetKeyboardButton(_Escape, Controls::isPressed) ||
                        GetControllerButton(GP_START, Controls::isPressed)) {
                        return 1;
                    }
                    break;
                }
                case UACTN_DOWN: {
                    return GetKeyboardButton(Tilde, Controls::isPressed);
                }
                case UACTN_SCREENSHOT: {
                    return GetKeyboardButton(PrintScrn, Controls::isPressed);
                }
            }
        }
        return 0;
    }

    buttonState = 0;
    if (Controls::bUseController) {
        buttonState = GetButtonState(3, ucKeyBinds[3][eAction], static_cast<CONTROL_TOGGLE_FLAGS>(toggleFlags));
    }
    else {
        for (i = 0; i < 2; ++i) {
            if (ucKeyBinds[i][eAction] != 255) {
                buttonState += GetButtonState(
                    i,
                    ucKeyBinds[i][eAction],
                    static_cast<CONTROL_TOGGLE_FLAGS>(toggleFlags)
                );
            }
        }
    }
    if ((eAction == UACTN_MENUMODE) && (buttonState > 0)) {
        if ((usOverrideFlags & OVERRIDE_SUPPRESS_MENUMODE) != 0) {
            usOverrideFlags ^= OVERRIDE_SUPPRESS_MENUMODE;
            cSuppressed = 1;
        }
        else {
            cSuppressed = 0;
        }
        if (cSuppressed) {
            buttonState = 0;
            if (static_cast<uint32_t>(toggleFlags) <= 1) {
                usOverrideFlags |= OVERRIDE_SUPPRESS_MENUMODE;
            }
        }
    }
    return buttonState;
}

bool Controls::SetControl(uint32_t auiAction, uint32_t auiDevice, uint8_t aucKey) {
    int32_t i;
    if ((!auiDevice) &&
        ((aucKey == 1) ||
         ((aucKey == 41) ||
          ((aucKey == 183) ||
           ((aucKey == 2) ||
            ((aucKey == 3) ||
             ((aucKey == 4) ||
              ((aucKey == 5) ||
               ((aucKey == 6) ||
                ((aucKey == 7) ||
                 ((aucKey == 8) ||
                  ((aucKey == 9) ||
                   ((aucKey == 59) ||
                    ((aucKey == 60) ||
                     ((aucKey == 61) || ((aucKey == 62) || ((aucKey == 199) || (aucKey == 43)))))))))))))))))) {
        return false;
    }
    if ((auiDevice == 3) &&
        ((aucKey != 8) &&
         ((aucKey != 9) &&
          ((aucKey != 10) &&
           ((aucKey != 11) &&
            ((aucKey != 12) &&
             ((aucKey != 13) &&
              ((aucKey != 14) && ((aucKey != 15) && ((aucKey != 16) && ((aucKey != 7) && (aucKey != 17)))))))))))) {
        return false;
    }
    for (i = 0; i < 28; ++i) {
        if (ucKeyBinds[auiDevice][i] == aucKey) {
            ucKeyBinds[auiDevice][i] = ucKeyBinds[auiDevice][auiAction];
            break;
        }
    }
    ucKeyBinds[auiDevice][auiAction] = aucKey;
    return true;
}

void Controls::SetActionMapping(uint32_t auiAction, uint32_t auiDevice, uint8_t aucKey) {
    if (auiDevice || ((aucKey != 183) && ((aucKey != 1) && (aucKey != 41)))) {
        RemoveDuplicateControls(auiDevice, aucKey);
        ucKeyBinds[auiDevice][auiAction] = aucKey;
    }
}

int32_t Controls::GetButtonCount(int32_t aiJoystick) {
    return static_cast<int32_t>(kJoyStickCaps[aiJoystick].dwButtons);
}

int32_t Controls::GetJoyStickButton(int32_t aiJoystick, int32_t aiButton, Controls::CONTROL_TOGGLE_FLAGS aiFlags) {
    DIJOYSTATE* pCurrentState = kJoyStickState[aiJoystick];
    DIJOYSTATE* pPreviousState = &kJoyStickState[aiJoystick][1];
    int32_t iResult = 0;
    switch (aiFlags) {
        case Controls::isHeld: {
            iResult = (pCurrentState->rgbButtons[aiButton] & 128) != 0;
            break;
        }
        case Controls::isPressed: {
            if ((pPreviousState->rgbButtons[aiButton] & 128) == 0) {
                iResult = (pCurrentState->rgbButtons[aiButton] & 128) != 0;
            }
            break;
        }
        case Controls::isDepressed: {
            if ((pCurrentState->rgbButtons[aiButton] & 128) == 0) {
                iResult = (pPreviousState->rgbButtons[aiButton] & 128) != 0;
            }
            break;
        }
        case Controls::isChanged: {
            iResult = (pCurrentState->rgbButtons[aiButton] & 128) != (pPreviousState->rgbButtons[aiButton] & 128);
            break;
        }
        default: {
            return iResult;
        }
    }
    return iResult;
}

int32_t Controls::GetHatCount(int32_t aiJoystick) {
    return static_cast<int32_t>(kJoyStickCaps[aiJoystick].dwPOVs);
}

int32_t Controls::GetJoystickHat(int32_t aiJoystick, int32_t aiHat) {
    int32_t iPOV = static_cast<int32_t>(kJoyStickState[aiJoystick][0].rgdwPOV[aiHat]);
    if (static_cast<uint16_t>(iPOV) == 65535) {
        return (-1);
    }

    return (iPOV / 4500) + 32;
}

void Controls::UpdateValues() {
    Controls::kLastGamepadState = Controls::kCurrentGamepadState;
    Controls::bUseController = !XInputGetState(0, &Controls::kCurrentGamepadState) && !this->bIsControllerDisabled;
    for (int32_t i = 0; i < static_cast<int32_t>(this->uiJoyStickCount); ++i) {
        rt::memcpy(&this->kJoyStickState[i][1], &this->kJoyStickState[i][0], sizeof(DIJOYSTATE));
        HRESULT hr = this->pDevices[i]->Acquire();
        // DI_OK or DI_NOEFFECT.
        if (hr == 0 || hr == 1) {
            this->pDevices[i]->Poll();
            this->pDevices[i]->GetDeviceState(sizeof(DIJOYSTATE), &this->kJoyStickState[i][0]);
        }
        else {
            std::memset(&this->kJoyStickState[i][0], 0, sizeof(DIJOYSTATE));
        }
    }
    if (this->pKeyboard) {
        rt::memcpy(this->ucKeyboardState[1], this->ucKeyboardState[0], 0x100);
        HRESULT hr = this->pKeyboard->Acquire();
        if (hr == 0 || hr == 1) {
            this->pKeyboard->Poll();
            this->pKeyboard->GetDeviceState(sizeof(this->ucKeyboardState[0]), this->ucKeyboardState[0]);
        }
        else {
            std::memset(this->ucKeyboardState[0], 0, 0x100);
        }
    }
    // A background status another thread asked for (a departure; see SetMouseBackgroundStatus).
    if (const int32_t iPending = siPendingMouseBackground.exchange(-1); iPending >= 0) {
        this->SetMouseBackgroundStatus(iPending != 0);
    }
    if (this->pMouse) {
        rt::memcpy(&this->kMouseState[1], &this->kMouseState[0], sizeof(DIMOUSESTATE2));
        std::memset(this->bDoubleClicked, 0, sizeof(this->bDoubleClicked));
        HRESULT hr = this->pMouse->Acquire();
        if (hr == 0 || hr == 1) {
            this->pMouse->Poll();
            this->pMouse->GetDeviceState(sizeof(DIMOUSESTATE2), &this->kMouseState[0]);
        }
        else {
            std::memset(&this->kMouseState[0], 0, sizeof(DIMOUSESTATE2));
        }
        this->PostMouseRead();
    }
}

void Controls::Poll(float fTime) {
    if (Controls::bUseController && this->bIsControllerEnabled) {
        this->pControllerVibration->UpdateRumble(static_cast<int32_t>(fTime * 1000.0));
    }
}

void Controls::ClearUserActions() {
    int32_t iAction;
    for (iAction = 0; iAction < 28; ++iAction) {
        ResetControlState(iAction);
    }
}

uint32_t Controls::HasJoySticks() const {
    return uiJoyStickCount;
}

// Returns 0 or 1 in the full 32-bit value, and HasInputInterfaces returns that value as it is.
uint32_t Controls::HasKeyboard() const {
    return pKeyboard != nullptr;
}

uint32_t Controls::HasMouse() const {
    return pMouse != nullptr;
}

void Controls::RemoveKeybind(int32_t aiControl, int32_t aiDevice) {
    ucKeyBinds[aiDevice][aiControl] = static_cast<uint8_t>(-1);
}

int32_t Controls::GetKeybind(int32_t aiControl) {
    return ((ucKeyBinds[0u][aiControl] << 16) | (ucKeyBinds[1][aiControl] << 8)) | ucKeyBinds[3][aiControl];
}

void Controls::SetKeybind(int32_t aiControl, int32_t aiBinds) {
    ucKeyBinds[0][aiControl] = static_cast<uint8_t>((aiBinds & 0xFF0000u) >> 16);
    ucKeyBinds[1][aiControl] = static_cast<uint8_t>((aiBinds & 0xFF00u) >> 8);
    ucKeyBinds[3][aiControl] = static_cast<uint8_t>(aiBinds);
}

void Controls::PostMouseRead() {
    ;
}

void Controls::InitControlsFromIni() {
    char* EndPtr;
    char FileName[268];
    strcpy_s(FileName, 260, BSUtilities::pPersonalPath);
    strcat_s(FileName, 260, pAppPrefININame);
    for (int32_t iAction = 0; iAction < 28; ++iAction) {
        char ReturnedString[64];
        uint32_t uiSize = 64;
        if (platform::IniGetString(
                FileName,
                "Controls",
                userActionKeys[iAction],
                nullptr,
                std::span<char>(ReturnedString, uiSize)
            )) {
            EndPtr = nullptr;
            int32_t iBinds = std::strtol(ReturnedString, &EndPtr, 16);
            SetKeybind(iAction, iBinds);
        }
    }
}

int32_t Controls::A23650(int32_t aiJoystick) {
    if (static_cast<uint8_t>(kJoyStickInstances[aiJoystick].dwDevType >> 8) == 2) {
        return 2;
    }

    return (-1);
}

int32_t Controls::A232D0(int32_t aiJoystick, int32_t aiAxis) {
    int32_t result;
    switch (aiAxis) {
        case 1: {
            result = uiJoyStickFlags[aiJoystick][0] & 1;
            break;
        }
        case 2: {
            result = uiJoyStickFlags[aiJoystick][0] & 2;
            break;
        }
        case 3: {
            result = uiJoyStickFlags[aiJoystick][0] & 4;
            break;
        }
        case 4: {
            result = uiJoyStickFlags[aiJoystick][0] & 8;
            break;
        }
        case 5: {
            result = uiJoyStickFlags[aiJoystick][0] & 16;
            break;
        }
        case 6: {
            result = uiJoyStickFlags[aiJoystick][0] & 32;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

int32_t Controls::A23490(int32_t aiJoystick, char acButton) {
    return uiJoyStickFlags[aiJoystick][1] & (1 << acButton);
}

int32_t Controls::SetDeviceDeadzone(int32_t aiJoystick, float afDeadzone) {
    IDirectInputDevice8A* pDevice = pDevices[aiJoystick];
    DIPROPDWORD kDeadzone;
    kDeadzone.diph.dwSize = sizeof(DIPROPDWORD);
    kDeadzone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    kDeadzone.diph.dwHow = 0;
    kDeadzone.diph.dwObj = 0;
    kDeadzone.dwData = static_cast<int32_t>(afDeadzone * 10000.0);
    // DIPROP_DEADZONE is MAKEDIPROP(5).
    return pDevice->SetProperty(reinterpret_cast<const void*>(5), &kDeadzone.diph);
}

void Controls::SetMouseBackgroundStatus(bool abBackground) {
    // Another thread's call (a message box's) only records the status, and the main thread's next UpdateValues replaces
    // the device: replacing it here would free the device the main thread is reading (a departure). A call on the main
    // thread supersedes a recorded one.
    if (!SDL_IsMainThread()) {
        siPendingMouseBackground = abBackground ? 1 : 0;
        return;
    }
    siPendingMouseBackground = -1;
    IDirectInputDevice8A* pDevice;
    if (pMouse) {
        pMouse->Unacquire();
        pMouse->Release();
        if (!pDirectInput->CreateDevice(&CLSID_GUID_SysMouse, &pDevice, nullptr)) {
            pMouse = pDevice;
            if (abBackground) {
                pMouse->SetCooperativeLevel(GetActiveWindow_SDL(), 6);
                uiFlags |= FLAG_BACKGROUND_MOUSE;
            }
            else {
                pMouse->SetCooperativeLevel(GetActiveWindow_SDL(), 5);
                uiFlags &= ~FLAG_BACKGROUND_MOUSE;
            }
            pMouse->SetDataFormat(&c_dfDIMouse2);
        }
    }
}

void Controls::EnableJoysticks(bool abEnable) {
    if (abEnable) {
        uiFlags |= FLAG_HAS_JOYSTICKS;
        pDirectInput->EnumDevices(0, &DIEnumDevicesCallback, this, 1);
        if (!uiJoyStickCount) {
            uiFlags &= ~FLAG_HAS_JOYSTICKS;
        }
    }
    else {
        // The index is compared with the count signed.
        for (int32_t i = 0; i < static_cast<int32_t>(uiJoyStickCount); ++i) {
            pDevices[i]->Unacquire();
            pDevices[i]->Release();
        }
        uiFlags &= ~FLAG_HAS_JOYSTICKS;
        uiJoyStickCount = 0;
        std::memset(kJoyStickState, 0, sizeof(kJoyStickState));
    }
}

void Controls::ForceControllerButton(GAMEPAD_BUTTON aeButton) {
    if (Controls::bUseController) {
        switch (aeButton) {
            case GP_LTRIGGER: {
                Controls::kCurrentGamepadState.Gamepad.bLeftTrigger = -1;
                break;
            }
            case GP_RTRIGGER: {
                Controls::kCurrentGamepadState.Gamepad.bRightTrigger = -1;
                break;
            }
            case GP_LTHUMB_UP: {
                Controls::kCurrentGamepadState.Gamepad.sThumbLY = std::numeric_limits<int16_t>::max();
                break;
            }
            case GP_LTHUMB_DOWN: {
                Controls::kCurrentGamepadState.Gamepad.sThumbLY = -std::numeric_limits<int16_t>::max();
                break;
            }
            case GP_LTHUMB_RIGHT: {
                Controls::kCurrentGamepadState.Gamepad.sThumbLX = std::numeric_limits<int16_t>::max();
                break;
            }
            case GP_LTHUMB_LEFT: {
                Controls::kCurrentGamepadState.Gamepad.sThumbLX = -std::numeric_limits<int16_t>::max();
                break;
            }
            default: {
                if (aeButton < GP_RTHUMB_LEFT) {
                    Controls::kCurrentGamepadState.Gamepad.wButtons |= MapToXInput(aeButton);
                }
                break;
            }
        }
    }
}

int32_t CALLBACK DIEnumDevicesCallback(const DIDEVICEINSTANCEA* lpddi, void* apRef) {
    // The reference is the Controls whose EnableJoysticks enumerates the devices.
    Controls* pvRef = static_cast<Controls*>(apRef);
    IDirectInputDevice8A* pCreatedDevice;
    if (static_cast<uint8_t>(lpddi->dwDevType) != 20 && static_cast<uint8_t>(lpddi->dwDevType) != 21) {
        return 1;
    }
    rt::memcpy(&pvRef->kJoyStickInstances[pvRef->uiJoyStickCount], lpddi, sizeof(DIDEVICEINSTANCEA));
    pvRef->pDirectInput->CreateDevice(&lpddi->guidInstance, &pCreatedDevice, nullptr);
    IDirectInputDevice8A* pDevice = pCreatedDevice;
    pDevice->SetCooperativeLevel(GetActiveWindow_SDL(), 6);
    pDevice->SetDataFormat(&c_dfDIJoystick);
    std::memset(&pvRef->kJoyStickCaps[pvRef->uiJoyStickCount], 0, sizeof(DIDEVCAPS));
    pvRef->kJoyStickCaps[pvRef->uiJoyStickCount].dwSize = sizeof(DIDEVCAPS);
    pDevice->GetCapabilities(&pvRef->kJoyStickCaps[pvRef->uiJoyStickCount]);
    pvRef->uiJoyStickFlags[pvRef->uiJoyStickCount][0] = 0;
    pvRef->uiJoyStickFlags[pvRef->uiJoyStickCount][1] = 0;
    pDevice->EnumObjects(&DIEnumDeviceObjectsCallback, pvRef->uiJoyStickFlags[pvRef->uiJoyStickCount], 0);
    pvRef->pDevices[pvRef->uiJoyStickCount] = pDevice;
    pvRef->SetDeviceRange(pvRef->uiJoyStickCount, -100, 100);
    pvRef->SetDeviceDeadzone(pvRef->uiJoyStickCount, 0.1f);
    return ++pvRef->uiJoyStickCount != 8;
}

int32_t CALLBACK DIEnumDeviceObjectsCallback(const DIDEVICEOBJECTINSTANCEW* lpddoi, void* apRef) {
    // The reference is the joystick's two flag words DIEnumDevicesCallback passes: the axes found, and the buttons
    // found by instance number (dwType's push and toggle button bits, 4 and 8).
    uint32_t* puiFlags = static_cast<uint32_t*>(apRef);
    if (!std::memcmp(&lpddoi->guidType, &CLSID_GUID_XAxis, sizeof(lpddoi->guidType))) {
        *puiFlags |= 1u;
    }
    if (!std::memcmp(&lpddoi->guidType, &CLSID_GUID_YAxis, sizeof(lpddoi->guidType))) {
        *puiFlags |= 2u;
    }
    if (!std::memcmp(&lpddoi->guidType, &CLSID_GUID_ZAxis, sizeof(lpddoi->guidType))) {
        *puiFlags |= 4u;
    }
    if (!std::memcmp(&lpddoi->guidType, &CLSID_GUID_RxAxis, sizeof(lpddoi->guidType))) {
        *puiFlags |= 8u;
    }
    if (!std::memcmp(&lpddoi->guidType, &CLSID_GUID_RyAxis, sizeof(lpddoi->guidType))) {
        *puiFlags |= 16u;
    }
    if (!std::memcmp(&lpddoi->guidType, &CLSID_GUID_RzAxis, sizeof(lpddoi->guidType))) {
        *puiFlags |= 32u;
    }
    if ((lpddoi->dwType & 12) != 0) {
        puiFlags[1] |= 1 << static_cast<uint8_t>(lpddoi->dwType >> 8);
    }
    return 1;
}

Controls::USERACTIONS& operator++(Controls::USERACTIONS& arAction) {
    arAction = static_cast<Controls::USERACTIONS>(arAction + 1);
    return arAction;
}

int32_t return_1(int32_t a1, int32_t a2) {
    return 1;
}
