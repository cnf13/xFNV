// The audio layer's DirectSound 8 (DirectSound.cpp): what its parts share. The game calls the COM interfaces of
// src/SDK/DirectX/dsound.h; this layer implements the methods the game calls (src/BSAudio/BSWin32Audio.cpp,
// BSWin32AudioListener.cpp, BSWin32GameSound.cpp) and fails the others with E_NOTIMPL.
#pragma once

#include "SDK/DirectX/dsound.h"

#include <cstdint>

namespace audio::ds {

    // dsound.h's values the layer uses, as the SDK has them.
    constexpr HRESULT DS_OK = 0;
    constexpr HRESULT DSERR_INVALIDPARAM = static_cast<HRESULT>(0x80070057);
    constexpr HRESULT DSERR_INVALIDCALL = static_cast<HRESULT>(0x88780032);
    constexpr HRESULT DSERR_BADFORMAT = static_cast<HRESULT>(0x88780064);
    constexpr HRESULT DSERR_CONTROLUNAVAIL = static_cast<HRESULT>(0x8878001E);
    constexpr HRESULT DSERR_NODRIVER = static_cast<HRESULT>(0x88780078);
    constexpr HRESULT DSERR_NOAGGREGATION = static_cast<HRESULT>(0x80040110);
    constexpr HRESULT DSERR_OUTOFMEMORY = static_cast<HRESULT>(0x8007000E);
    constexpr HRESULT DSERR_FXUNAVAILABLE = static_cast<HRESULT>(0x887800B0);
    constexpr HRESULT DSERR_OBJECTNOTFOUND = static_cast<HRESULT>(0x88781161);

    constexpr uint32_t DSBCAPS_PRIMARYBUFFER = 0x1;
    constexpr uint32_t DSBCAPS_LOCSOFTWARE = 0x8;
    constexpr uint32_t DSBCAPS_CTRLFREQUENCY = 0x20;
    constexpr uint32_t DSBCAPS_CTRLPAN = 0x40;
    constexpr uint32_t DSBCAPS_CTRLVOLUME = 0x80;
    constexpr uint32_t DSBCAPS_CTRLFX = 0x200;
    constexpr uint32_t DSBPLAY_LOOPING = 0x1;
    constexpr uint32_t DSBSTATUS_PLAYING = 0x1;
    constexpr uint32_t DSBSTATUS_LOOPING = 0x4;
    constexpr uint32_t DSBLOCK_FROMWRITECURSOR = 0x1;
    constexpr uint32_t DSBLOCK_ENTIREBUFFER = 0x2;
    constexpr int32_t DSBVOLUME_MAX = 0;
    constexpr int32_t DSBPAN_LEFT = -10000;
    constexpr int32_t DSBPAN_RIGHT = 10000;
    constexpr uint32_t DSBFREQUENCY_ORIGINAL = 0;
    constexpr uint32_t DSBFREQUENCY_MIN = 100;
    constexpr uint32_t DSBSIZE_MIN = 4;
    constexpr uint32_t DSBSIZE_MAX = 0x0FFFFFFF;
    // The shortest buffer DSBCAPS_CTRLFX takes, in milliseconds (DSBSIZE_FX_MIN).
    constexpr uint32_t DSBSIZE_FX_MIN = 150;
    constexpr uint32_t DSFXR_LOCSOFTWARE = 2;
    constexpr uint32_t DS3D_DEFERRED = 0x1;
    constexpr uint32_t DS3DMODE_NORMAL = 0;
    constexpr uint32_t DS3DMODE_HEADRELATIVE = 1;
    constexpr uint32_t DS3DMODE_DISABLE = 2;
    constexpr uint16_t WAVE_FORMAT_PCM = 1;

    // The 3D state of the listener and of a 3D buffer, as their interfaces set it (DS3DLISTENER, DS3DBUFFER less the
    // cone, which the game never sets).
    struct ListenerParams {
        D3DVECTOR position{0.0f, 0.0f, 0.0f};
        D3DVECTOR velocity{0.0f, 0.0f, 0.0f};
        D3DVECTOR front{0.0f, 0.0f, 1.0f};
        D3DVECTOR top{0.0f, 1.0f, 0.0f};
        float distanceFactor = 1.0f;
        float rolloffFactor = 1.0f;
        float dopplerFactor = 1.0f;
    };

    struct SourceParams {
        D3DVECTOR position{0.0f, 0.0f, 0.0f};
        D3DVECTOR velocity{0.0f, 0.0f, 0.0f};
        float minDistance = 1.0f;
        float maxDistance = 1000000000.0f;
        uint32_t mode = DS3DMODE_NORMAL;
    };

    // What a 3D buffer's settings and the listener's make of its sound (DirectSound3D.cpp): a gain, a DirectSound pan
    // (-10000 left to 10000 right) and a frequency ratio (Doppler).
    struct Spatial {
        float gain = 1.0f;
        int32_t pan = 0;
        float ratio = 1.0f;
    };

    // The 3D model; muteAtMax is the buffer's DSBCAPS_MUTE3DATMAXDISTANCE.
    Spatial Compute(const ListenerParams& listener, const SourceParams& source, bool muteAtMax);

    // The SDK's GUIDs the game passes.
    bool SameGuid(const GUID& a, const GUID& b);

} // namespace audio::ds
