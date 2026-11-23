#pragma once

#include "SDK/DirectX/d3d9.h"
#include "SDK/Windows/objbase.h"
#include "SDK/Windows/unknwn.h"

// DirectSound's COM interfaces, as the PC audio classes hold them. Not in the PDB (the PC build's DirectSound8 SDK):
// their vtable order is the SDK's dsound.h, which the exe's calls go through. IDirectSoundBuffer, IDirectSoundBuffer8,
// IDirectSound8, IDirectSound3DListener, IDirectSound3DBuffer, IDirectSoundFXParamEq, IDirectSoundFXDistortion and
// IDirectSoundFXI3DL2Reverb are defined below; IDirectSound src only holds as an opaque pointer.
class IDirectSound;
class IDirectSoundFXDistortion;
class IDirectSoundFXI3DL2Reverb;

// The format of a wave: the layout every audio buffer description points to (IDA: tWAVEFORMATEX). Two-byte packed
// as the SDK has it; not in the PDB.
#pragma pack(push, 2)
struct tWAVEFORMATEX {
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
};
#pragma pack(pop)

typedef tWAVEFORMATEX WAVEFORMATEX;

// The common part of a wave format, as a wave file's fmt chunk starts (PDB and IDA: waveformat_tag). Two-byte packed
// as the SDK has it.
#pragma pack(push, 2)
struct waveformat_tag {
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
};

// A PCM wave format: the 16 bytes of a PCM fmt chunk, WAVEFORMATEX without cbSize (PDB and IDA: pcmwaveformat_tag).
struct pcmwaveformat_tag {
    waveformat_tag wf;
    uint16_t wBitsPerSample;
};
#pragma pack(pop)

// The description of a DirectSound buffer to create (IDA: _DSBUFFERDESC); not in the PDB. Its size is 0x24 in the
// exe and 0x28 on x64 (lpwfxFormat is 8 bytes there): dwSize takes sizeof, since DirectSound refuses any other size.
struct _DSBUFFERDESC {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwBufferBytes;
    uint32_t dwReserved;
    tWAVEFORMATEX* lpwfxFormat;
    GUID guid3DAlgorithm;
};

typedef _DSBUFFERDESC DSBUFFERDESC;

// dsound.h's buffer flags (_DSBUFFERDESC::dwFlags), volume and frequency bounds and error codes, as the SDK has them.
constexpr uint32_t DSBCAPS_CTRL3D = 0x10;
constexpr uint32_t DSBCAPS_GETCURRENTPOSITION2 = 0x10000;
constexpr uint32_t DSBCAPS_MUTE3DATMAXDISTANCE = 0x20000;
constexpr int32_t DSBVOLUME_MIN = -10000;
constexpr uint32_t DSBFREQUENCY_MAX = 200000;
constexpr uint32_t DSBSTATUS_PLAYING = 0x1;
constexpr HRESULT DSERR_BUFFERTOOSMALL = static_cast<HRESULT>(0x887800B4);

// The description of one DirectSound effect (IDA: _DSEFFECTDESC, 0x20 bytes; the SDK's DSEFFECTDESC, which
// IDirectSoundBuffer8::SetFX takes); not in the PDB. The exe fills it in BSWin32GameSound::PrepareReverb (0xAF1430):
// dwSize 0x20 at +0 (0xAF1802), dwFlags 0 at +4 (0xAF1809), the 16-byte class GUID from 0x1101004 at +8
// (0xAF1810-0xAF182F) and the two reserved words 0 at +0x18/+0x1C (0xAF1832/0xAF1839), then SetFX at +0x54
// (0xAF1863).
struct _DSEFFECTDESC {
    uint32_t dwSize;
    uint32_t dwFlags;
    GUID guidDSFXClass;
    // DWORD_PTR in the SDK: 4 bytes in the exe, 8 on x64, where the SDK's struct (and so dwSize) is 0x28 bytes.
    uintptr_t dwReserved1;
    uintptr_t dwReserved2;
};

typedef _DSEFFECTDESC DSEFFECTDESC;

// The parameters of a parametric-equalizer effect (IDA: _DSFXParamEq, 0xC bytes; the SDK's DSFXParamEq, which
// IDirectSoundFXParamEq's methods take); not in the PDB.
struct _DSFXParamEq {
    float fCenter;
    float fBandwidth;
    float fGain;
};

typedef _DSFXParamEq DSFXParamEq;

// The parameters of a distortion effect (IDA: _DSFXDistortion, 0x14 bytes; the SDK's DSFXDistortion, which
// IDirectSoundFXDistortion's methods take); not in the PDB. The exe fills it in BSWin32GameSound::Prepare (0xAEC530):
// fGain from the sound's setting, then 0, 5000, 1135 and 5000 (0xAECFE2 writes fGain at +0x1F0, the last at +0x200).
struct _DSFXDistortion {
    float fGain;
    float fEdge;
    float fPostEQCenterFrequency;
    float fPostEQBandwidth;
    float fPreLowpassCutoff;
};

typedef _DSFXDistortion DSFXDistortion;

// The capabilities of the sound device (IDA: BSWin32Audio::BufferCaps, spelled as a DSBCAPS with a tail; the exe
// passes it to IDirectSound8::GetCaps with dwSize 0x60, which is the SDK's DSCAPS); not in the PDB.
struct _DSCAPS {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwMinSecondarySampleRate;
    uint32_t dwMaxSecondarySampleRate;
    uint32_t dwPrimaryBuffers;
    uint32_t dwMaxHwMixingAllBuffers;
    uint32_t dwMaxHwMixingStaticBuffers;
    uint32_t dwMaxHwMixingStreamingBuffers;
    uint32_t dwFreeHwMixingAllBuffers;
    uint32_t dwFreeHwMixingStaticBuffers;
    uint32_t dwFreeHwMixingStreamingBuffers;
    uint32_t dwMaxHw3DAllBuffers;
    uint32_t dwMaxHw3DStaticBuffers;
    uint32_t dwMaxHw3DStreamingBuffers;
    uint32_t dwFreeHw3DAllBuffers;
    uint32_t dwFreeHw3DStaticBuffers;
    uint32_t dwFreeHw3DStreamingBuffers;
    uint32_t dwTotalHwMemBytes;
    uint32_t dwFreeHwMemBytes;
    uint32_t dwMaxContigFreeHwMemBytes;
    uint32_t dwUnlockTransferRateHwBuffers;
    uint32_t dwPlayCpuOverheadSwBuffers;
    uint32_t dwReserved1;
    uint32_t dwReserved2;
};

typedef _DSCAPS DSCAPS;

// The capabilities of one sound buffer (IDA: _DSBCAPS, 0x14 bytes; the SDK's DSBCAPS, which GetCaps fills); not in
// the PDB.
struct _DSBCAPS {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwBufferBytes;
    uint32_t dwUnlockTransferRate;
    uint32_t dwPlayCpuOverhead;
};

typedef _DSBCAPS DSBCAPS;

// The parameters of a 3D listener (IDA: _DS3DLISTENER, 0x40 bytes; the SDK's DS3DLISTENER); not in the PDB.
struct _DS3DLISTENER {
    uint32_t dwSize;
    D3DVECTOR vPosition;
    D3DVECTOR vVelocity;
    D3DVECTOR vOrientFront;
    D3DVECTOR vOrientTop;
    float flDistanceFactor;
    float flRolloffFactor;
    float flDopplerFactor;
};

typedef _DS3DLISTENER DS3DLISTENER;

// The parameters of a 3D sound buffer (IDA: _DS3DBUFFER, 0x40 bytes; the SDK's DS3DBUFFER); not in the PDB.
struct _DS3DBUFFER {
    uint32_t dwSize;
    D3DVECTOR vPosition;
    D3DVECTOR vVelocity;
    uint32_t dwInsideConeAngle;
    uint32_t dwOutsideConeAngle;
    D3DVECTOR vConeOrientation;
    int32_t lConeOutsideVolume;
    float flMinDistance;
    float flMaxDistance;
    uint32_t dwMode;
};

typedef _DS3DBUFFER DS3DBUFFER;

// A DirectSound buffer (IDA: IDirectSoundBuffer): IUnknown's three slots, then dsound.h's IDirectSoundBuffer
// methods. The exe calls QueryInterface (0x0), Release (0x8), GetCurrentPosition (0x10), GetFormat (0x14), GetStatus
// (0x24), Lock (0x2C), Play (0x30), SetCurrentPosition (0x34), SetVolume (0x3C), Stop (0x48) and Unlock (0x4C)
// through it.
class IDirectSoundBuffer : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetCaps(_DSBCAPS* pDSBufferCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetCurrentPosition(uint32_t* pdwCurrentPlayCursor, uint32_t* pdwCurrentWriteCursor) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetFormat(tWAVEFORMATEX* pwfxFormat, uint32_t dwSizeAllocated, uint32_t* pdwSizeWritten) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVolume(int32_t* plVolume) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPan(int32_t* plPan) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrequency(uint32_t* pdwFrequency) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStatus(uint32_t* pdwStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE Initialize(IDirectSound* pDirectSound, const _DSBUFFERDESC* pcDSBufferDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE Lock(
        uint32_t dwOffset,
        uint32_t dwBytes,
        void** ppvAudioPtr1,
        uint32_t* pdwAudioBytes1,
        void** ppvAudioPtr2,
        uint32_t* pdwAudioBytes2,
        uint32_t dwFlags
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE Play(uint32_t dwReserved1, uint32_t dwPriority, uint32_t dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCurrentPosition(uint32_t dwNewPosition) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFormat(const tWAVEFORMATEX* pcfxFormat) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVolume(int32_t lVolume) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPan(int32_t lPan) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFrequency(uint32_t dwFrequency) = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
    virtual HRESULT STDMETHODCALLTYPE
    Unlock(void* pvAudioPtr1, uint32_t dwAudioBytes1, void* pvAudioPtr2, uint32_t dwAudioBytes2) = 0;
    virtual HRESULT STDMETHODCALLTYPE Restore() = 0;
};

// A DirectSound buffer with effects (IDA: IDirectSoundBuffer8): IDirectSoundBuffer's slots, then dsound.h's
// IDirectSoundBuffer8 methods. The exe calls SetFX (0x54, with a _DSEFFECTDESC) in BSWin32GameSound::Prepare
// (0xAEC530: 0xAECF53) and PrepareReverb (0xAF1430: 0xAF1863), and GetObjectInPath (0x5C) in both (0xAEC530:
// 0xAECF8F, 0xAF1430: 0xAF189F/0xAF18A2); AcquireResources (0x58) is the SDK's method between them, which the exe
// does not call.
class IDirectSoundBuffer8 : public IDirectSoundBuffer {
public:
    virtual HRESULT STDMETHODCALLTYPE
    SetFX(uint32_t dwEffectsCount, _DSEFFECTDESC* pDSFXDesc, uint32_t* pdwResultCodes) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    AcquireResources(uint32_t dwFlags, uint32_t dwEffectsCount, uint32_t* pdwResultCodes) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetObjectInPath(const GUID* rguidObject, uint32_t dwIndex, const GUID* rguidInterface, void** ppObject) = 0;
};

// A DirectSound device (IDA: IDirectSound8): IUnknown's three slots, then dsound.h's IDirectSound8 methods. The exe
// calls GetCaps (0x10) and SetCooperativeLevel (0x18) in BSWin32Audio::Init (0xAEA230: 0xAEA337 [edx+0x10],
// 0xAEA2FE [edx+0x18]), CreateSoundBuffer (0xC) in BSWin32GameSound::Prepare (0xAEC530: 0xAEC9CE) and Release (0x8)
// in the destructors (0xAEB009, 0xAEB042).
class IDirectSound8 : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE
    CreateSoundBuffer(const _DSBUFFERDESC* pcDSBufferDesc, IDirectSoundBuffer** ppDSBuffer, IUnknown* pUnkOuter) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCaps(_DSCAPS* pDSCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    DuplicateSoundBuffer(IDirectSoundBuffer* pDSBufferOriginal, IDirectSoundBuffer** ppDSBufferDuplicate) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND__* hwnd, uint32_t dwLevel) = 0;
    virtual HRESULT STDMETHODCALLTYPE Compact() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSpeakerConfig(uint32_t* pdwSpeakerConfig) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSpeakerConfig(uint32_t dwSpeakerConfig) = 0;
    virtual HRESULT STDMETHODCALLTYPE Initialize(const GUID* pcGuidDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE VerifyCertification(uint32_t* pdwCertified) = 0;
};

// A 3D listener (IDA: IDirectSound3DListener): IUnknown's three slots, then dsound.h's IDirectSound3DListener
// methods. The exe calls Release (0x8), SetOrientation (0x34), SetPosition (0x38), SetRolloffFactor (0x3C),
// SetVelocity (0x40) and CommitDeferredSettings (0x44) through it.
class IDirectSound3DListener : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(_DS3DLISTENER* pListener) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDistanceFactor(float* pflDistanceFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDopplerFactor(float* pflDopplerFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOrientation(D3DVECTOR* pvOrientFront, D3DVECTOR* pvOrientTop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR* pvPosition) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRolloffFactor(float* pflRolloffFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR* pvVelocity) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(const _DS3DLISTENER* pcListener, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDistanceFactor(float flDistanceFactor, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDopplerFactor(float flDopplerFactor, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetOrientation(float xFront, float yFront, float zFront, float xTop, float yTop, float zTop, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPosition(float x, float y, float z, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRolloffFactor(float flRolloffFactor, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVelocity(float x, float y, float z, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE CommitDeferredSettings() = 0;
};

// A 3D sound buffer (IDA: IDirectSound3DBuffer): IUnknown's three slots, then dsound.h's IDirectSound3DBuffer
// methods. The exe calls SetMinDistance (0x44) and SetMaxDistance (0x40) in BSWin32GameSound::SetMinMax (0xAEFDA0:
// 0xAEFE8D [edx+0x44], 0xAEFF06 [ecx+0x40]) and SetPosition (0x4C) in BSWin32GameSound::SetEmitterPosition (0xAEF090:
// 0xAEF2D8 [ecx+0x4c]).
class IDirectSound3DBuffer : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(_DS3DBUFFER* pDs3dBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConeAngles(uint32_t* pdwInsideConeAngle, uint32_t* pdwOutsideConeAngle) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConeOrientation(D3DVECTOR* pvOrientation) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConeOutsideVolume(int32_t* plConeOutsideVolume) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMaxDistance(float* pflMaxDistance) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMinDistance(float* pflMinDistance) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMode(uint32_t* pdwMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR* pvPosition) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR* pvVelocity) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(const _DS3DBUFFER* pcDs3dBuffer, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetConeAngles(uint32_t dwInsideConeAngle, uint32_t dwOutsideConeAngle, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConeOrientation(float x, float y, float z, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConeOutsideVolume(int32_t lConeOutsideVolume, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaxDistance(float flMaxDistance, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMinDistance(float flMinDistance, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMode(uint32_t dwMode, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPosition(float x, float y, float z, uint32_t dwApply) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVelocity(float x, float y, float z, uint32_t dwApply) = 0;
};

// The parametric-equalizer effect of a sound buffer (IDA: IDirectSoundFXParamEq; IDA types the BSWin32GameSound field
// that holds it as uint32_t): IUnknown's three slots, then dsound.h's IDirectSoundFXParamEq methods. The exe only
// clears the pointer (BSWin32GameSound's constructors, +0x204) and calls no method of the interface.
class IDirectSoundFXParamEq : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(const _DSFXParamEq* pcDsFxParamEq) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(_DSFXParamEq* pDsFxParamEq) = 0;
};

// The distortion effect of a sound buffer (IDA: IDirectSoundFXDistortion): IUnknown's three slots, then dsound.h's
// IDirectSoundFXDistortion methods. The exe calls SetAllParameters (0xC) in BSWin32GameSound::Prepare (0xAEC530:
// 0xAED03F, on the object at +0x208).
class IDirectSoundFXDistortion : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(const DSFXDistortion* pcDsFxDistortion) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXDistortion* pDsFxDistortion) = 0;
};

// The parameters of an I3DL2 reverb effect (the SDK's DSFXI3DL2Reverb); the exe never reads or writes one.
struct _DSFXI3DL2Reverb;

// The I3DL2 reverb effect of a sound buffer (IDA: IDirectSoundFXI3DL2Reverb): IUnknown's three slots, then dsound.h's
// IDirectSoundFXI3DL2Reverb methods. The exe calls SetPreset (0x14) in BSWin32GameSound::PrepareReverb (0xAF1430:
// 0xAF1952, on the object at +0x1E0).
class IDirectSoundFXI3DL2Reverb : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(const _DSFXI3DL2Reverb* pcDsFxI3DL2Reverb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(_DSFXI3DL2Reverb* pDsFxI3DL2Reverb) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPreset(uint32_t dwPreset) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPreset(uint32_t* pdwPreset) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetQuality(int32_t lQuality) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetQuality(int32_t* plQuality) = 0;
};

// dsound.dll's DirectSoundCreate8 (imported by ordinal 11 into the exe's slot 0xFDF034).
using DirectSoundCreate8_t = HRESULT(WINAPI*)(const GUID* pcGuidDevice, IDirectSound8** ppDS8, IUnknown* pUnkOuter);
namespace xfnv::imports {
    extern "C" DECLSPEC_IMPORT HRESULT WINAPI
    DirectSoundCreate8(const GUID* pcGuidDevice, IDirectSound8** ppDS8, IUnknown* pUnkOuter); // import slot 0xFDF034
} // namespace xfnv::imports
