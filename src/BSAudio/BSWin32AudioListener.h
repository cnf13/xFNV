#pragma once

#include "BSAudio/BSAudioListener.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/Windows/mmsystem.h"

class IDirectSound3DListener;

// The audio listener on DirectSound: it wraps a DirectSound 3D listener.
class BSWin32AudioListener : public BSAudioListener {
public:
    // Stores the argument into pListener: the interface pointer itself.
    BSWin32AudioListener(IDirectSound3DListener* apListener);
    ~BSWin32AudioListener() override;

    void Init() override;
    void SetPosition(const NiPoint3&) override;
    NiPoint3& GetPosition() override;
    void Update() override;
    void SetOrientation(const NiPoint3&, const NiPoint3&) override;
    void SetVelocity(const NiPoint3&) override;
    void SetRolloffFactor(float afRolloffFactor) override;

    // The DirectSound 3D listener (IDirectSound3DListener8 is IDirectSound3DListener); the destructor releases it.
    IDirectSound3DListener* pListener;
    NiPoint3 kTop;
    NiPoint3 kFront;
};
