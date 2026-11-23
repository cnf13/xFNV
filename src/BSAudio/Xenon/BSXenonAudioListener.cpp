#include "BSAudio/BSAudioListener.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

BSAudioListener::~BSAudioListener() {
    iEvironmentType = 0;
}

NiPoint3& BSAudioListener::GetPosition() {
    return ListenerPosition;
}

NiPoint3& BSAudioListener::GetFrontOrientation() {
    return FrontOrientation;
}

NiPoint3& BSAudioListener::GetTopOrientation() {
    return TopOrientation;
}

BSAudioListener* BSAudioListener::SetGain(float afGain) {
    fGain = afGain;
    return this;
}

void BSAudioListener::SetEnvironmentType(uint32_t aiType) {
    iEvironmentType = aiType;
}

uint32_t BSAudioListener::GetEnvironmentType() {
    return iEvironmentType;
}

void BSAudioListener::SetRolloffFactor(float afRolloffFactor) {
    fRolloffFactor = afRolloffFactor;
}

// The deleting destructor frees through the engine's heap.
void BSAudioListener::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
