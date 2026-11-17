#include "BSAudio/BSWin32AudioListener.h"

#include "BSAudio/BSAudioListener.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

BSWin32AudioListener::BSWin32AudioListener(IDirectSound3DListener* apListener) : pListener(apListener) {
	fRolloffFactor = 0.0f;
}

BSWin32AudioListener::~BSWin32AudioListener() {
	if (pListener) {
		pListener->Release();
		pListener = 0;
	}
}

// The DirectSound listener needs no setup here.
void BSWin32AudioListener::Init() {}

void BSWin32AudioListener::SetPosition(const NiPoint3& a1) {
	uint32_t Time = timeGetTime();
	NiPoint3 kDelta = a1 - ListenerPosition;
	double v2 = kDelta.Length();
	fVelocity = v2 / static_cast<double>(1000 * (Time - dwTimeLastUpdated)) * 0.014287755824625492;
	ListenerPosition = a1;
	dwTimeLastUpdated = Time;
}

// The exe's own copy of BSAudioListener::GetPosition (0xAEAAD0).
NiPoint3& BSWin32AudioListener::GetPosition() {
	return ListenerPosition;
}

void BSWin32AudioListener::Update() {
	if (pListener) {
		float v3 = ListenerPosition.y * 0.014287755824625492;
		float v2 = ListenerPosition.z * 0.014287755824625492;
		float v1 = ListenerPosition.x * 0.014287755824625492;
		pListener->SetPosition(v1, v2, v3, 1);
		pListener->SetOrientation(kFront.x, kFront.z, kFront.y, kTop.x, kTop.z, kTop.y, 1);
		pListener->CommitDeferredSettings();
	}
}

void BSWin32AudioListener::SetOrientation(const NiPoint3& a1, const NiPoint3& a2) {
	kTop = a2;
	kFront = a1;
}

void BSWin32AudioListener::SetVelocity(const NiPoint3& a1) {
	vVelocity = a1;
	if (pListener) {
		pListener->SetVelocity(a1.x, a1.y, a1.z, 1);
	}
}

void BSWin32AudioListener::SetRolloffFactor(float afRolloffFactor) {
	if (pListener) {
		pListener->SetRolloffFactor(afRolloffFactor, 1);
	}
}
