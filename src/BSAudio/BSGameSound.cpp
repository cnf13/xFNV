#include "BSAudio/BSGameSound.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSCore/BSRandom.h"
#include "BSCore/MemoryManager.h"

BSGameSound::BSGameSound() {
    Init();
}

// apFilename is not read: the constructor only initialises, as the default one does. QueuedMessages starts empty
// through its own constructor.
BSGameSound::BSGameSound(const char* apFilename) {
    Init();
}

// QueuedMessages' own destructor empties the list.
BSGameSound::~BSGameSound() {}

bool BSGameSound::IsPlaying() {
    return ((((int32_t)iStateFlags) & 32) != 0);
}

bool BSGameSound::IsInDecay() {
    return ((((int32_t)iStateFlags) & 524288) != 0);
}

bool BSGameSound::IsPaused() {
    return ((((int32_t)iStateFlags) & 64) != 0);
}

float BSGameSound::GetVolume() {
    return fVolume;
}

void BSGameSound::SetInaudible(bool abIn) {
    int32_t v2;
    if (abIn) {
        v2 = ((int32_t)iStateFlags) | 131072;
    }
    else {
        v2 = (int32_t)(((int32_t)iStateFlags) & 0xFFFDFFFFu);
    }
    iStateFlags = (uint32_t)v2;
}

void BSGameSound::SetEnvironmentType(uint32_t aiEnvType) {
    iEnvironmentType = aiEnvType;
}

bool BSGameSound::PriorityTest(BSGameSound* apToCompare) {
    return (bool)((uint8_t)(((uint32_t)((int32_t)iPriority)) < apToCompare->iPriority));
}

bool BSGameSound::PriorityTest(uint32_t aiToCompare) {
    return (bool)((uint8_t)(((uint32_t)((int32_t)iPriority)) < aiToCompare));
}

bool BSGameSound::Prepare() {
    if ((iModifierFlags & 2) != 0) {
        uint32_t aiMax = iDurationInMS;
        if ((Singleton_guard & 1) == 0) {
            Singleton_guard |= 1u;
            BSRandom::Singleton.~BSRandom();
            ::new (&BSRandom::Singleton) BSRandom;
        }
        uint32_t pos = BSRandom::Singleton.UnsignedInt(aiMax);
        Seek(pos);
    }
    iStateFlags |= BSGameSound::STATE_READY;
    return 1;
}

// Copies apToCopy's file, attenuation, timing and callback state and takes a new sound ID from the audio manager.
// abPlayable is not read.
void BSGameSound::Copy(BSGameSound* apToCopy, bool abPlayable) {
    iSoundHash = apToCopy->iSoundHash;
    iDirectoryHash = apToCopy->iDirectoryHash;
    SetupPath(apToCopy->pFilename, false);
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    uint32_t uiSoundCacheIndex = pManager->uiSoundCacheIndex;
    pManager->uiSoundCacheIndex = pManager->uiSoundCacheIndex + 1;
    iSoundID = uiSoundCacheIndex;
    iStateFlags = apToCopy->iStateFlags;
    iModifierFlags = apToCopy->iModifierFlags;
    iDurationInMS = apToCopy->iDurationInMS;
    iStaticAttenuation = apToCopy->iStaticAttenuation;
    iReverbAttenuation = apToCopy->iReverbAttenuation;
    iSystemAttenuation = apToCopy->iSystemAttenuation;
    iDistanceAttenuation = apToCopy->iDistanceAttenuation;
    fVolume = apToCopy->fVolume;
    fBeginTime = apToCopy->fBeginTime;
    fEndTime = apToCopy->fEndTime;
    iWeatherFlags = apToCopy->iWeatherFlags;
    iSamplesPerSecond = apToCopy->iSamplesPerSecond;
    iModSamplesPerSecond = apToCopy->iModSamplesPerSecond;
    fMaxDist = apToCopy->fMaxDist;
    iSoundSize = apToCopy->iSoundSize;
    iLoopStart = apToCopy->iLoopStart;
    iLoopEnd = apToCopy->iLoopEnd;
    fnCompletionCallback = apToCopy->fnCompletionCallback;
    fnPlayedCallback = apToCopy->fnPlayedCallback;
    pSoundCompletionContext = apToCopy->pSoundCompletionContext;
    pSoundPlayContext = apToCopy->pSoundPlayContext;
}

bool BSGameSound::Update(uint32_t adwTimeDelta) {
    uint8_t v3;
    if ((Is3DSound() || Is2DRadiusSound()) &&
        (((((uint32_t)iSoundTypeFlags) & 0x100u) == 0) &&
         (((iStateFlags & 131072) == 0) &&
          (IsPlaying() &&
           (((((((uint32_t)iSoundTypeFlags) & 0x10u) != 0) ||
              (((((uint32_t)iSoundTypeFlags) & 0x4000000u) != 0) || ((((uint32_t)iSoundTypeFlags) & 0x2000000u) != 0)))
                 ? (v3 = 1)
                 : (v3 = 0)),
            (v3 &&
             (((((uint32_t)iSoundTypeFlags) & 0x2000000u) == 0) && (fDistanceToListener > ((double)fMaxDist))))))))) {
        Stop();
        SetInaudible((uint8_t)1u);
    }
    else {
        if ((((iStateFlags & 131072) != 0) && (!IsPaused())) && (fMaxDist > ((double)fDistanceToListener))) {
            SetInaudible((uint8_t)0);
            Play((bool)1);
        }
    }
    return (bool)1;
}

bool BSGameSound::CanPlayNow(uint32_t aiConditions, float afGamehour) {
    double v4;
    if (((aiConditions & ((int32_t)iWeatherFlags)) == 0) && (iWeatherFlags && aiConditions)) {
        return (bool)0;
    }
    float v5 = fEndTime - fBeginTime;
    if (!(v5 < 0.0)) {
        v4 = (double)v5;
    }
    else {
        v4 = v5 * (-1.0);
    }
    float v6 = (float)v4;
    return (bool)((uint8_t)((!(v6 >= 0.09375)) ||
                            (((!(fEndTime > ((double)fBeginTime))) ||
                              ((!(afGamehour < ((double)fBeginTime))) && (!(afGamehour > ((double)fEndTime))))) &&
                             ((!(fEndTime < ((double)fBeginTime))) ||
                              ((!(afGamehour < ((double)fBeginTime))) || (!(afGamehour > ((double)fEndTime))))))));
}

bool BSGameSound::OpenResult(bool abResult) {
    if (abResult) {
        iStateFlags |= 1;
        iStateFlags |= 256;
        iStateFlags &= (~4096);
    }
    else {
        iStateFlags &= (~256);
        iStateFlags |= 1;
        iStateFlags &= (~4096);
        iStateFlags |= 16;
        iDurationInMS = (uint32_t)1;
    }
    return (bool)((uint8_t)abResult);
}

bool BSGameSound::Is3DSound() const {
    return ((((uint32_t)iSoundTypeFlags) & 2u) != 0);
}

bool BSGameSound::Is2DRadiusSound() const {
    return (((((uint32_t)iSoundTypeFlags) & 1u) != 0) && ((((uint32_t)iSoundTypeFlags) & 0x8000000u) != 0));
}

// Tests the byte before the terminator (the last character of the path) for a separator; v7 - 1 is that byte,
// pFilename[-1] when the path is empty.
bool BSGameSound::SetupPath(char* apSrc, bool abSetID) {
    bool v6 = 1;
    if (!apSrc) {
        return 0;
    }
    if (pFilename != apSrc) {
        strcpy_s(pFilename, 260, apSrc);
    }
    BSAudio::pAudioInstance->FixSoundPath(pFilename);
    bool (*pPickSoundFileFromFolder)(char*) = BSAudio::pAudioInstance->pfnRandomFilenameCallback;
    size_t v7 = std::strlen(pFilename);
    bool bFolder = pFilename[v7 - 1] == '\\' || pFilename[v7 - 1] == '/';
    if (bFolder && !iDirectoryHash) {
        iDirectoryHash = BSGameSound::GetNameHash(pFilename);
    }
    if (abSetID && (bFolder && pPickSoundFileFromFolder)) {
        pPickSoundFileFromFolder(pFilename);
        BSAudio::pAudioInstance->FixSoundPath(pFilename);
    }
    else {
        if (bFolder && !abSetID) {
            v6 = 0;
        }
    }
    if (bFolder && !abSetID) {
        iSoundHash = 0;
    }
    else {
        iSoundHash = BSGameSound::GetNameHash(pFilename);
    }
    return v6;
}

bool BSGameSound::SetupPath(bool abSetID) {
    // Tests the address of pFilename, which is never null.
    return SetupPath(pFilename, abSetID);
}

void BSGameSound::SetIsMuted(char a2) {
    uint32_t v2;
    if (a2) {
        v2 = iStateFlags | 1024;
    }
    else {
        v2 = iStateFlags & (~1024);
    }
    iStateFlags = v2;
    SetVolume(fVolume);
}

void BSGameSound::Init() {
    iSoundID = 0;
    iSoundTypeFlags = BSGameSound::SOUND_FLAG_NONE;
    iModifierFlags = 0;
    iStateFlags = 0;
    iStateFlags |= BSGameSound::STATE_VALID;
    iDurationInMS = 0;
    iStaticAttenuation = 0;
    iReverbAttenuation = 0;
    iSystemAttenuation = 0;
    iDistanceAttenuation = 0;
    iFaderAttenuation = 0;
    fVolume = 1.0f;
    fBeginTime = 0.0f;
    fEndTime = 0.0f;
    iWeatherFlags = 0;
    iSamplesPerSecond = 0;
    fMaxDist = 1500.0f;
    fMinDist = 50.0f;
    iSoundSize = 0;
    iEnvironmentType = 0;
    iCleanupDelayInMS = 0;
    iSynchStartTime = 0;
    iSoundHash = 0;
    iDirectoryHash = 0;
    fDistanceToListener = 3.4028235e+38f;
    iFreqVariance = 0;
    iModSamplesPerSecond = 0;
    iLoopStart = 0;
    iLoopEnd = 0;
    fnCompletionCallback = 0;
    fnPlayedCallback = 0;
    pSoundCompletionContext = 0;
    pSoundPlayContext = 0;
    iStateFlags &= ~BSGameSound::STATE_IS_VALID_BEAM;
}

// The callback type is void(*)(void*, int): the unsigned duration is passed as its int.
void BSGameSound::HandlePlayCallback() {
    if (fnPlayedCallback) {
        fnPlayedCallback(pSoundPlayContext, static_cast<int32_t>(iDurationInMS));
        pSoundPlayContext = 0;
        fnPlayedCallback = 0;
    }
}

// Deleting a BSGameSound frees through the engine's heap.
void BSGameSound::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}

uint32_t BSGameSound::GetNameHash(const char* pString) {
    uint32_t v3 = 0;
    if (!pString) {
        return 0;
    }
    for (const char* i = pString; *++i; v3 = (v3 << 16) + ((v3 << 6) + *i) - v3) {
        ;
    }
    return v3;
}
