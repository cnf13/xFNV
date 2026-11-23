// xinput1_3.dll on SDL3, for the programs without the exe (CMakeLists.txt), which link this in place of xinput:
// XInputGetState and XInputSetState for user index 0, on the first gamepad SDL knows (any pad SDL maps looks like an
// Xbox pad: its south button is A). Other user indices report no controller.
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstring>

// The SDK's name of the exports' convention (WINAPI).
#include "SDK/Windows/windef.h"

namespace {

    // The SDK's XInput.h layouts and values.
    struct XINPUT_GAMEPAD {
        uint16_t wButtons;
        uint8_t bLeftTrigger;
        uint8_t bRightTrigger;
        int16_t sThumbLX;
        int16_t sThumbLY;
        int16_t sThumbRX;
        int16_t sThumbRY;
    };
    struct XINPUT_STATE {
        uint32_t dwPacketNumber;
        XINPUT_GAMEPAD Gamepad;
    };
    struct XINPUT_VIBRATION {
        uint16_t wLeftMotorSpeed;
        uint16_t wRightMotorSpeed;
    };
    constexpr uint32_t ERROR_SUCCESS_ = 0;
    constexpr uint32_t ERROR_BAD_ARGUMENTS_ = 160;
    constexpr uint32_t ERROR_DEVICE_NOT_CONNECTED_ = 1167;

    // XInput's button bits, by SDL gamepad button.
    struct ButtonBit {
        SDL_GamepadButton eButton;
        uint16_t uiBit;
    };
    constexpr ButtonBit kButtons[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, 0x0001},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0x0002},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0x0004},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0x0008},
        {SDL_GAMEPAD_BUTTON_START, 0x0010},
        {SDL_GAMEPAD_BUTTON_BACK, 0x0020},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, 0x0040},
        {SDL_GAMEPAD_BUTTON_RIGHT_STICK, 0x0080},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0x0100},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0x0200},
        {SDL_GAMEPAD_BUTTON_SOUTH, 0x1000},
        {SDL_GAMEPAD_BUTTON_EAST, 0x2000},
        {SDL_GAMEPAD_BUTTON_WEST, 0x4000},
        {SDL_GAMEPAD_BUTTON_NORTH, 0x8000},
    };

    SDL_Gamepad* gPad;
    XINPUT_GAMEPAD gLast;
    uint32_t gPacket;

    // The first gamepad, opened when one is there; dropped when it goes.
    SDL_Gamepad* Pad() {
        if (gPad && !SDL_GamepadConnected(gPad)) {
            SDL_CloseGamepad(gPad);
            gPad = nullptr;
        }
        if (!gPad && SDL_HasGamepad()) {
            int iCount = 0;
            if (SDL_JoystickID* pIds = SDL_GetGamepads(&iCount)) {
                if (iCount > 0) {
                    gPad = SDL_OpenGamepad(pIds[0]);
                }
                SDL_free(pIds);
            }
        }
        return gPad;
    }

    // SDL's stick Y grows downwards, XInput's upwards.
    int16_t FlipY(int16_t iValue) {
        return static_cast<int16_t>(iValue == -32768 ? 32767 : -iValue);
    }

} // namespace

extern "C" uint32_t WINAPI XInputGetState(uint32_t dwUserIndex, XINPUT_STATE* pState) {
    if (dwUserIndex > 3 || !pState) {
        return ERROR_BAD_ARGUMENTS_;
    }
    SDL_Gamepad* pPad = dwUserIndex == 0 ? Pad() : nullptr;
    if (!pPad) {
        return ERROR_DEVICE_NOT_CONNECTED_;
    }
    XINPUT_GAMEPAD kPad{};
    for (const ButtonBit& kButton : kButtons) {
        if (SDL_GetGamepadButton(pPad, kButton.eButton)) {
            kPad.wButtons |= kButton.uiBit;
        }
    }
    // Triggers 0..32767 to 0..255.
    kPad.bLeftTrigger = static_cast<uint8_t>(SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >> 7);
    kPad.bRightTrigger = static_cast<uint8_t>(SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) >> 7);
    kPad.sThumbLX = SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_LEFTX);
    kPad.sThumbLY = FlipY(SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_LEFTY));
    kPad.sThumbRX = SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_RIGHTX);
    kPad.sThumbRY = FlipY(SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_RIGHTY));
    // The packet number changes when the state does.
    if (memcmp(&kPad, &gLast, sizeof(kPad))) {
        gLast = kPad;
        ++gPacket;
    }
    pState->dwPacketNumber = gPacket;
    pState->Gamepad = kPad;
    return ERROR_SUCCESS_;
}

extern "C" uint32_t WINAPI XInputSetState(uint32_t dwUserIndex, XINPUT_VIBRATION* pVibration) {
    if (dwUserIndex > 3 || !pVibration) {
        return ERROR_BAD_ARGUMENTS_;
    }
    SDL_Gamepad* pPad = dwUserIndex == 0 ? Pad() : nullptr;
    if (!pPad) {
        return ERROR_DEVICE_NOT_CONNECTED_;
    }
    // XInput's motors run until set again; SDL's rumble lasts a given time, here its longest (about 65 s). The left
    // motor is the low-frequency one.
    SDL_RumbleGamepad(pPad, pVibration->wLeftMotorSpeed, pVibration->wRightMotorSpeed, 0xFFFF);
    return ERROR_SUCCESS_;
}
