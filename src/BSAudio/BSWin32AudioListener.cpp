#include "BSAudio/BSWin32AudioListener.h"

#include "BSAudio/BSAudioListener.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <cstdint>

BSWin32AudioListener::BSWin32AudioListener(IDirectSound3DListener* apListener) : pListener(apListener) {
    fRolloffFactor = 0.0f;
}

BSWin32AudioListener::~BSWin32AudioListener() {
    if (pListener) {
        pListener->Release();
        pListener = nullptr;
    }
}

// The DirectSound listener needs no setup here.
void BSWin32AudioListener::Init() {}

void BSWin32AudioListener::SetPosition(const NiPoint3& arPosition) {
    uint32_t Time = timeGetTime();
    NiPoint3 kDelta = arPosition - ListenerPosition;
    double dDistance = kDelta.Length();
    fVelocity = dDistance / static_cast<double>(1000 * (Time - dwTimeLastUpdated)) * 0.014287755824625492;
    ListenerPosition = arPosition;
    dwTimeLastUpdated = Time;
}

NiPoint3& BSWin32AudioListener::GetPosition() {
    return ListenerPosition;
}

void BSWin32AudioListener::Update() {
    if (pListener) {
        float fY = ListenerPosition.y * 0.014287755824625492;
        float fZ = ListenerPosition.z * 0.014287755824625492;
        float fX = ListenerPosition.x * 0.014287755824625492;
        pListener->SetPosition(fX, fZ, fY, 1);
        pListener->SetOrientation(kFront.x, kFront.z, kFront.y, kTop.x, kTop.z, kTop.y, 1);
        pListener->CommitDeferredSettings();
    }
}

void BSWin32AudioListener::SetOrientation(const NiPoint3& arFront, const NiPoint3& arTop) {
    kTop = arTop;
    kFront = arFront;
}

void BSWin32AudioListener::SetVelocity(const NiPoint3& arVelocity) {
    vVelocity = arVelocity;
    if (pListener) {
        pListener->SetVelocity(arVelocity.x, arVelocity.y, arVelocity.z, 1);
    }
}

void BSWin32AudioListener::SetRolloffFactor(float afRolloffFactor) {
    if (pListener) {
        pListener->SetRolloffFactor(afRolloffFactor, 1);
    }
}
