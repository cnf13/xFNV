// dinput8.dll on SDL3, for the programs without the exe (CMakeLists.txt), which link this in place of dinput8:
// DirectInput8Create, the keyboard and mouse devices Controls (src/BSDevices/Controls.cpp) creates with the methods it
// calls, and the data formats dinput8.lib would give. The game's input code stays as it is and reads SDL through the
// interfaces it was written for.
// - Keyboard: the immediate state from SDL's key state, the buffered events (DIK codes, the size the game sets with
//   DIPROP_BUFFERSIZE) from an SDL event watch. SDL's scancodes map to DIK codes by the inverse of SDL's own Windows
//   table, whose index is DIK's numbering (a set-1 scan code, 0x80 | code for an extended key).
// - Mouse: DIMOUSESTATE2 from SDL's motion, wheel (120 per notch) and button events; the exclusive level is SDL's
//   relative mouse mode, the background level leaves the cursor to the system (Main::HandleMouseMove).
// - The foreground level as DirectInput has it: Acquire fails while no window of the program has input focus, and an
//   acquired device is lost when the focus goes.
// - Joysticks: EnumDevices reports none; the gamepad is XInput's (XInputSDL.cpp).
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>

#include "platform/System.h"
#include "scancodes_windows.h"
// The SDK's names of the DLL's conventions: WINAPI for the export, STDMETHODCALLTYPE for the COM methods.
#include "SDK/Windows/objbase.h"

namespace {

    // The SDK's dinput.h values and layouts the game uses (DIRECTINPUT_VERSION 0x0800).
    struct Guid {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t Data4[8];
    };
    constexpr Guid GUID_SysMouse{0x6F1D2B60, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
    constexpr Guid GUID_SysKeyboard{0x6F1D2B61, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

    constexpr long DI_OK = 0;
    constexpr long DI_NOEFFECT = 1; // also S_FALSE
    constexpr long DI_BUFFEROVERFLOW = 1;
    constexpr long E_NOTIMPL_ = static_cast<long>(0x80004001u);
    constexpr long E_NOINTERFACE_ = static_cast<long>(0x80004002u);
    constexpr long E_POINTER_ = static_cast<long>(0x80004003u);
    constexpr long DIERR_OTHERAPPHASPRIO = static_cast<long>(0x80070005u);
    constexpr long DIERR_NOTACQUIRED = static_cast<long>(0x8007000Cu);
    constexpr long DIERR_INPUTLOST = static_cast<long>(0x8007001Eu);
    constexpr long DIERR_INVALIDPARAM = static_cast<long>(0x80070057u);
    constexpr long DIERR_DEVICENOTREG = static_cast<long>(0x80040154u);

    constexpr uint32_t DISCL_EXCLUSIVE = 0x1;
    constexpr uint32_t DISCL_BACKGROUND = 0x8;
    // The property GUIDs are small integers (MAKEDIPROP).
    constexpr uintptr_t DIPROP_BUFFERSIZE = 1;
    constexpr uint32_t WHEEL_DELTA = 120;

    struct DIOBJECTDATAFORMAT {
        const Guid* pguid;
        uint32_t dwOfs;
        uint32_t dwType;
        uint32_t dwFlags;
    };
    struct DIDATAFORMAT {
        uint32_t dwSize;
        uint32_t dwObjSize;
        uint32_t dwFlags;
        uint32_t dwDataSize;
        uint32_t dwNumObjs;
        DIOBJECTDATAFORMAT* rgodf;
    };
    struct DIPROPHEADER {
        uint32_t dwSize;
        uint32_t dwHeaderSize;
        uint32_t dwObj;
        uint32_t dwHow;
    };
    struct DIPROPDWORD {
        DIPROPHEADER diph;
        uint32_t dwData;
    };
    // The leading fields every DIDEVICEOBJECTDATA has; the caller's size gives the stride.
    struct DIDEVICEOBJECTDATA {
        uint32_t dwOfs;
        uint32_t dwData;
        uint32_t dwTimeStamp;
        uint32_t dwSequence;
    };
    struct DIMOUSESTATE2 {
        int32_t lX;
        int32_t lY;
        int32_t lZ;
        uint8_t rgbButtons[8];
    };

} // namespace

// dinput8.lib's predefined formats. The device is told by its GUID, not by the format, so the object tables are not
// modelled: only the sizes and the axis mode are filled in.
extern "C" const DIDATAFORMAT c_dfDIKeyboard{sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), 2, 256, 0, nullptr};
extern "C" const DIDATAFORMAT c_dfDIMouse2{
    sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), 2, sizeof(DIMOUSESTATE2), 0, nullptr
};
extern "C" const DIDATAFORMAT c_dfDIJoystick{sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), 1, 80, 0, nullptr};

namespace {

    // SDL_Scancode -> DIK code, the inverse of SDL's Windows table (the first index wins where two map to one
    // scancode).
    uint8_t gDik[SDL_SCANCODE_COUNT];

    void BuildDikTable() {
        for (int i = static_cast<int>(SDL_arraysize(windows_scancode_table)) - 1; i > 0; --i) {
            const SDL_Scancode code = windows_scancode_table[i];
            if (code != SDL_SCANCODE_UNKNOWN && code < SDL_SCANCODE_COUNT) {
                gDik[code] = static_cast<uint8_t>(i);
            }
        }
    }

    bool HasFocus() {
        return SDL_GetKeyboardFocus() != nullptr;
    }

    class Device;
    Device* gKeyboard;
    Device* gMouse;

    // The SDK's IDirectInputDevice8A, every slot in its order; the game calls the first 26 (through Poll).
    class Device {
    public:
        enum Kind { KEYBOARD, MOUSE };

        explicit Device(Kind aeKind) : eKind(aeKind) {
            pLock = SDL_CreateMutex();
            (aeKind == KEYBOARD ? gKeyboard : gMouse) = this;
        }

        virtual long STDMETHODCALLTYPE QueryInterface(const void*, void** ppvObj) {
            if (!ppvObj) {
                return E_POINTER_;
            }
            *ppvObj = nullptr;
            return E_NOINTERFACE_;
        }
        virtual uint32_t STDMETHODCALLTYPE AddRef() {
            return ++uiRefs;
        }
        virtual uint32_t STDMETHODCALLTYPE Release() {
            const uint32_t uiLeft = --uiRefs;
            if (!uiLeft) {
                Unacquire();
                (eKind == KEYBOARD ? gKeyboard : gMouse) = nullptr;
                SDL_DestroyMutex(pLock);
                delete[] pBuffer;
                delete this;
            }
            return uiLeft;
        }
        virtual long STDMETHODCALLTYPE GetCapabilities(void*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE EnumObjects(void*, void*, uint32_t) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE GetProperty(const void*, DIPROPHEADER*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE SetProperty(const void* rguidProp, const DIPROPHEADER* pdiph) {
            if (reinterpret_cast<uintptr_t>(rguidProp) != DIPROP_BUFFERSIZE) {
                return DI_OK;
            }
            if (!pdiph || pdiph->dwSize < sizeof(DIPROPDWORD)) {
                return DIERR_INVALIDPARAM;
            }
            SDL_LockMutex(pLock);
            delete[] pBuffer;
            uiCapacity = reinterpret_cast<const DIPROPDWORD*>(pdiph)->dwData;
            pBuffer = uiCapacity ? new DIDEVICEOBJECTDATA[uiCapacity] : nullptr;
            uiHead = uiCount = 0;
            bOverflow = false;
            SDL_UnlockMutex(pLock);
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE Acquire() {
            if (bAcquired) {
                return DI_NOEFFECT;
            }
            if (!(uiLevel & DISCL_BACKGROUND) && !HasFocus()) {
                return DIERR_OTHERAPPHASPRIO;
            }
            SDL_LockMutex(pLock);
            uiHead = uiCount = 0;
            bOverflow = false;
            iX = iY = iZ = 0;
            fRemX = fRemY = 0.0f;
            bAcquired = true;
            SDL_UnlockMutex(pLock);
            // No grab on a lost display: SDL's grab there crashes in libXi (platform::DisplayLost).
            if (eKind == MOUSE && (uiLevel & DISCL_EXCLUSIVE) && !platform::DisplayLost()) {
                SDL_SetWindowRelativeMouseMode(SDL_GetKeyboardFocus(), true);
            }
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE Unacquire() {
            if (!bAcquired) {
                return DI_NOEFFECT;
            }
            bAcquired = false;
            // A lost display holds no grab, and SDL's release of it there crashes in libXi (platform::DisplayLost).
            if (eKind == MOUSE && (uiLevel & DISCL_EXCLUSIVE) && !platform::DisplayLost()) {
                int iCount = 0;
                if (SDL_Window** ppWindows = SDL_GetWindows(&iCount)) {
                    for (int i = 0; i < iCount; ++i) {
                        SDL_SetWindowRelativeMouseMode(ppWindows[i], false);
                    }
                    SDL_free(ppWindows);
                }
            }
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE GetDeviceState(uint32_t cbData, void* lpvData) {
            const long hr = Usable();
            if (hr != DI_OK) {
                return hr;
            }
            if (eKind == KEYBOARD) {
                if (cbData < 256) {
                    return DIERR_INVALIDPARAM;
                }
                uint8_t* pKeys = static_cast<uint8_t*>(lpvData);
                memset(pKeys, 0, cbData);
                int iCount = 0;
                const bool* pDown = SDL_GetKeyboardState(&iCount);
                for (int i = 0; i < iCount; ++i) {
                    if (pDown[i] && gDik[i]) {
                        pKeys[gDik[i]] = 0x80;
                    }
                }
                return DI_OK;
            }
            if (cbData < sizeof(DIMOUSESTATE2)) {
                return DIERR_INVALIDPARAM;
            }
            DIMOUSESTATE2 kState{};
            SDL_LockMutex(pLock);
            kState.lX = iX;
            kState.lY = iY;
            kState.lZ = iZ;
            iX = iY = iZ = 0;
            SDL_UnlockMutex(pLock);
            const SDL_MouseButtonFlags uiButtons = SDL_GetMouseState(nullptr, nullptr);
            for (int i = 0; i < 5; ++i) {
                kState.rgbButtons[i] = (uiButtons & SDL_BUTTON_MASK(i + 1)) ? 0x80 : 0;
            }
            // DirectInput's order: left, right, middle, X1, X2; SDL's: left, middle, right, X1, X2.
            const uint8_t ucMiddle = kState.rgbButtons[1];
            kState.rgbButtons[1] = kState.rgbButtons[2];
            kState.rgbButtons[2] = ucMiddle;
            memcpy(lpvData, &kState, sizeof(kState));
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE
        GetDeviceData(uint32_t cbObjectData, DIDEVICEOBJECTDATA* rgdod, uint32_t* pdwInOut, uint32_t dwFlags) {
            const long hr = Usable();
            if (hr != DI_OK) {
                return hr;
            }
            if (!pdwInOut || cbObjectData < sizeof(DIDEVICEOBJECTDATA)) {
                return DIERR_INVALIDPARAM;
            }
            // DIGDD_PEEK: leave the items in the buffer.
            const bool bPeek = (dwFlags & 1) != 0;
            SDL_LockMutex(pLock);
            uint32_t uiTaken = 0;
            while (uiTaken < *pdwInOut && uiTaken < uiCount) {
                if (rgdod) {
                    DIDEVICEOBJECTDATA* pOut = reinterpret_cast<DIDEVICEOBJECTDATA*>(
                        reinterpret_cast<uint8_t*>(rgdod) + static_cast<size_t>(uiTaken) * cbObjectData
                    );
                    memset(pOut, 0, cbObjectData);
                    *pOut = pBuffer[(uiHead + uiTaken) % uiCapacity];
                }
                ++uiTaken;
            }
            if (!bPeek) {
                uiHead = uiCapacity ? (uiHead + uiTaken) % uiCapacity : 0;
                uiCount -= uiTaken;
            }
            const bool bWasOverflow = bOverflow;
            if (!bPeek) {
                bOverflow = false;
            }
            SDL_UnlockMutex(pLock);
            *pdwInOut = uiTaken;
            return bWasOverflow ? DI_BUFFEROVERFLOW : DI_OK;
        }
        virtual long STDMETHODCALLTYPE SetDataFormat(const void*) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE SetEventNotification(void*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE SetCooperativeLevel(void*, uint32_t dwFlags) {
            uiLevel = dwFlags;
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE GetObjectInfo(void*, uint32_t, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE GetDeviceInfo(void*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE RunControlPanel(void*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE Initialize(void*, uint32_t, const void*) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE CreateEffect(const void*, const void*, void**, void*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE EnumEffects(void*, void*, uint32_t) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE GetEffectInfo(void*, const void*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE GetForceFeedbackState(uint32_t*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE SendForceFeedbackCommand(uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE EnumCreatedEffectObjects(void*, void*, uint32_t) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE Escape(void*) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE Poll() {
            return bAcquired ? DI_NOEFFECT : DIERR_NOTACQUIRED;
        }
        virtual long STDMETHODCALLTYPE SendDeviceData(uint32_t, const void*, uint32_t*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE EnumEffectsInFile(const char*, void*, void*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE WriteEffectToFile(const char*, uint32_t, void*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE BuildActionMap(void*, const char*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE SetActionMap(void*, const char*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE GetImageInfo(void*) {
            return E_NOTIMPL_;
        }

        // The event watch's side: a key event into the buffer, a mouse event into the deltas.
        void OnEvent(const SDL_Event& kEvent) {
            if (!bAcquired || (!(uiLevel & DISCL_BACKGROUND) && !HasFocus())) {
                return;
            }
            SDL_LockMutex(pLock);
            switch (kEvent.type) {
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    // DirectInput reports a change of state only: no auto-repeat.
                    if (!kEvent.key.repeat && kEvent.key.scancode < SDL_SCANCODE_COUNT && gDik[kEvent.key.scancode]) {
                        Push(
                            gDik[kEvent.key.scancode], kEvent.type == SDL_EVENT_KEY_DOWN ? 0x80 : 0,
                            static_cast<uint32_t>(SDL_NS_TO_MS(kEvent.key.timestamp))
                        );
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    fRemX += kEvent.motion.xrel;
                    fRemY += kEvent.motion.yrel;
                    iX += static_cast<int32_t>(fRemX);
                    iY += static_cast<int32_t>(fRemY);
                    fRemX -= static_cast<int32_t>(fRemX);
                    fRemY -= static_cast<int32_t>(fRemY);
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    iZ += static_cast<int32_t>(
                        kEvent.wheel.y * WHEEL_DELTA * (kEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f)
                    );
                    break;
                default: break;
            }
            SDL_UnlockMutex(pLock);
        }

        // Focus lost: DirectInput unacquires a foreground device.
        void OnFocusLost() {
            if (bAcquired && !(uiLevel & DISCL_BACKGROUND)) {
                Unacquire();
                bLost = true;
            }
        }

        uint32_t uiRefs = 1;

    private:
        long Usable() {
            if (bLost) {
                bLost = false;
                return DIERR_INPUTLOST;
            }
            return bAcquired ? DI_OK : DIERR_NOTACQUIRED;
        }

        void Push(uint32_t uiOfs, uint32_t uiData, uint32_t uiTime) {
            if (!uiCapacity) {
                return;
            }
            if (uiCount == uiCapacity) {
                bOverflow = true;
                return;
            }
            pBuffer[(uiHead + uiCount) % uiCapacity] = {uiOfs, uiData, uiTime, ++uiSequence};
            ++uiCount;
        }

        Kind eKind;
        SDL_Mutex* pLock;
        uint32_t uiLevel = 0;
        bool bAcquired = false;
        bool bLost = false;
        DIDEVICEOBJECTDATA* pBuffer = nullptr;
        uint32_t uiCapacity = 0;
        uint32_t uiHead = 0;
        uint32_t uiCount = 0;
        uint32_t uiSequence = 0;
        bool bOverflow = false;
        int32_t iX = 0;
        int32_t iY = 0;
        int32_t iZ = 0;
        float fRemX = 0.0f;
        float fRemY = 0.0f;
    };

    bool SDLCALL WatchEvents(void*, SDL_Event* pEvent) {
        switch (pEvent->type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                if (gKeyboard) {
                    gKeyboard->OnEvent(*pEvent);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_WHEEL:
                if (gMouse) {
                    gMouse->OnEvent(*pEvent);
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (gKeyboard) {
                    gKeyboard->OnFocusLost();
                }
                if (gMouse) {
                    gMouse->OnFocusLost();
                }
                break;
            default: break;
        }
        return true;
    }

    // The SDK's IDirectInput8A, every slot in its order; the game calls CreateDevice, EnumDevices and Release.
    class DirectInput {
    public:
        virtual long STDMETHODCALLTYPE QueryInterface(const void*, void** ppvObj) {
            if (!ppvObj) {
                return E_POINTER_;
            }
            *ppvObj = nullptr;
            return E_NOINTERFACE_;
        }
        virtual uint32_t STDMETHODCALLTYPE AddRef() {
            return ++uiRefs;
        }
        virtual uint32_t STDMETHODCALLTYPE Release() {
            const uint32_t uiLeft = --uiRefs;
            if (!uiLeft) {
                delete this;
            }
            return uiLeft;
        }
        virtual long STDMETHODCALLTYPE CreateDevice(const Guid* rguid, Device** ppDevice, void*) {
            if (!rguid || !ppDevice) {
                return E_POINTER_;
            }
            *ppDevice = nullptr;
            if (!memcmp(rguid, &GUID_SysKeyboard, sizeof(Guid))) {
                *ppDevice = new Device(Device::KEYBOARD);
            }
            else if (!memcmp(rguid, &GUID_SysMouse, sizeof(Guid))) {
                *ppDevice = new Device(Device::MOUSE);
            }
            else {
                return DIERR_DEVICENOTREG;
            }
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE EnumDevices(uint32_t, void*, void*, uint32_t) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE GetDeviceStatus(const Guid*) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE RunControlPanel(void*, uint32_t) {
            return E_NOTIMPL_;
        }
        virtual long STDMETHODCALLTYPE Initialize(void*, uint32_t) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE FindDevice(const Guid*, const char*, Guid*) {
            return DIERR_DEVICENOTREG;
        }
        virtual long STDMETHODCALLTYPE EnumDevicesBySemantics(const char*, void*, void*, void*, uint32_t) {
            return DI_OK;
        }
        virtual long STDMETHODCALLTYPE ConfigureDevices(void*, void*, uint32_t, void*) {
            return E_NOTIMPL_;
        }

        uint32_t uiRefs = 1;
    };

} // namespace

extern "C" long WINAPI DirectInput8Create(void*, uint32_t, const void*, void** ppvOut, void*) {
    if (!ppvOut) {
        return E_POINTER_;
    }
    static bool bWatching = false;
    if (!bWatching) {
        bWatching = true;
        BuildDikTable();
        SDL_AddEventWatch(WatchEvents, nullptr);
    }
    *ppvOut = new DirectInput;
    return DI_OK;
}
