#include "BSAudio/BSGameSound.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSCore/BSRandom.h"
#include "BSCore/MemoryManager.h"

#include <limits>

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
    return (iStateFlags & STATE_PLAYING) != 0;
}

bool BSGameSound::IsInDecay() {
    return (iStateFlags & STATE_IN_DECAY_19) != 0;
}

bool BSGameSound::IsPaused() {
    return (iStateFlags & STATE_PAUSED) != 0;
}

float BSGameSound::GetVolume() {
    return fVolume;
}

void BSGameSound::SetInaudible(bool abIn) {
    if (abIn) {
        iStateFlags |= STATE_INAUDIBLE;
    }
    else {
        iStateFlags &= ~STATE_INAUDIBLE;
    }
}

void BSGameSound::SetEnvironmentType(uint32_t aiEnvType) {
    iEnvironmentType = aiEnvType;
}

bool BSGameSound::PriorityTest(BSGameSound* apToCompare) {
    return iPriority < apToCompare->iPriority;
}

bool BSGameSound::PriorityTest(uint32_t aiToCompare) {
    return iPriority < aiToCompare;
}

bool BSGameSound::Prepare() {
    if ((iModifierFlags & MODIFIER_START_AT_RANDOM_POSITION) != 0) {
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
    return true;
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
    uint8_t ucLoopOrEnvelope;
    if ((Is3DSound() || Is2DRadiusSound()) &&
        (((iSoundTypeFlags & SOUND_FLAG_ONE_SHOT) == 0) &&
         (((iStateFlags & STATE_INAUDIBLE) == 0) &&
          (IsPlaying() &&
           (((((iSoundTypeFlags & SOUND_FLAG_LOOP) != 0) ||
              (((iSoundTypeFlags & SOUND_FLAG_ENVELOPE_SLOW) != 0) ||
               ((iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST) != 0)))
                 ? (ucLoopOrEnvelope = 1)
                 : (ucLoopOrEnvelope = 0)),
            (ucLoopOrEnvelope &&
             (((iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST) == 0) &&
              (fDistanceToListener > static_cast<double>(fMaxDist))))))))) {
        Stop();
        SetInaudible(true);
    }
    else {
        if ((((iStateFlags & STATE_INAUDIBLE) != 0) && (!IsPaused())) &&
            (fMaxDist > static_cast<double>(fDistanceToListener))) {
            SetInaudible(false);
            Play(true);
        }
    }
    return true;
}

bool BSGameSound::CanPlayNow(uint32_t aiConditions, float afGamehour) {
    double dAbsSpan;
    if (((aiConditions & iWeatherFlags) == 0) && (iWeatherFlags && aiConditions)) {
        return false;
    }
    float fSpan = fEndTime - fBeginTime;
    if (!(fSpan < 0.0)) {
        dAbsSpan = fSpan;
    }
    else {
        dAbsSpan = fSpan * (-1.0);
    }
    float fAbsSpan = static_cast<float>(dAbsSpan);
    return (!(fAbsSpan >= 0.09375)) ||
        (((!(fEndTime > static_cast<double>(fBeginTime))) ||
          ((!(afGamehour < static_cast<double>(fBeginTime))) && (!(afGamehour > static_cast<double>(fEndTime))))) &&
         ((!(fEndTime < static_cast<double>(fBeginTime))) ||
          ((!(afGamehour < static_cast<double>(fBeginTime))) || (!(afGamehour > static_cast<double>(fEndTime))))));
}

bool BSGameSound::OpenResult(bool abResult) {
    if (abResult) {
        iStateFlags |= STATE_OPENED;
        iStateFlags |= STATE_VALID;
        iStateFlags &= ~STATE_OPENING;
    }
    else {
        iStateFlags &= ~STATE_VALID;
        iStateFlags |= STATE_OPENED;
        iStateFlags &= ~STATE_OPENING;
        iStateFlags |= STATE_STOPPED;
        iDurationInMS = 1;
    }
    return abResult;
}

bool BSGameSound::Is3DSound() const {
    return (iSoundTypeFlags & SOUND_FLAG_IS_3D) != 0;
}

bool BSGameSound::Is2DRadiusSound() const {
    return ((iSoundTypeFlags & SOUND_FLAG_IS_2D) != 0) && ((iSoundTypeFlags & SOUND_FLAG_IS_2D_RADIUS) != 0);
}

// Tests the byte before the terminator (the last character of the path) for a separator; stLength - 1 is that byte,
// pFilename[-1] when the path is empty.
bool BSGameSound::SetupPath(char* apSrc, bool abSetID) {
    bool bResult = true;
    if (!apSrc) {
        return false;
    }
    if (pFilename != apSrc) {
        strcpy_s(pFilename, 260, apSrc);
    }
    BSAudio::pAudioInstance->FixSoundPath(pFilename);
    bool (*pPickSoundFileFromFolder)(char*) = BSAudio::pAudioInstance->pfnRandomFilenameCallback;
    size_t stLength = std::strlen(pFilename);
    bool bFolder = pFilename[stLength - 1] == '\\' || pFilename[stLength - 1] == '/';
    if (bFolder && !iDirectoryHash) {
        iDirectoryHash = BSGameSound::GetNameHash(pFilename);
    }
    if (abSetID && (bFolder && pPickSoundFileFromFolder)) {
        pPickSoundFileFromFolder(pFilename);
        BSAudio::pAudioInstance->FixSoundPath(pFilename);
    }
    else {
        if (bFolder && !abSetID) {
            bResult = false;
        }
    }
    if (bFolder && !abSetID) {
        iSoundHash = 0;
    }
    else {
        iSoundHash = BSGameSound::GetNameHash(pFilename);
    }
    return bResult;
}

bool BSGameSound::SetupPath(bool abSetID) {
    // Tests the address of pFilename, which is never null.
    return SetupPath(pFilename, abSetID);
}

void BSGameSound::SetIsMuted(char abMuted) {
    if (abMuted) {
        iStateFlags |= STATE_MUTED;
    }
    else {
        iStateFlags &= ~STATE_MUTED;
    }
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
    fDistanceToListener = std::numeric_limits<float>::max();
    iFreqVariance = 0;
    iModSamplesPerSecond = 0;
    iLoopStart = 0;
    iLoopEnd = 0;
    fnCompletionCallback = nullptr;
    fnPlayedCallback = nullptr;
    pSoundCompletionContext = nullptr;
    pSoundPlayContext = nullptr;
    iStateFlags &= ~BSGameSound::STATE_IS_VALID_BEAM;
}

// The callback type is void(*)(void*, int): the unsigned duration is passed as its int.
void BSGameSound::HandlePlayCallback() {
    if (fnPlayedCallback) {
        fnPlayedCallback(pSoundPlayContext, static_cast<int32_t>(iDurationInMS));
        pSoundPlayContext = nullptr;
        fnPlayedCallback = nullptr;
    }
}

// Deleting a BSGameSound frees through the engine's heap.
void BSGameSound::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

uint32_t BSGameSound::GetNameHash(const char* pString) {
    uint32_t uiHash = 0;
    if (!pString) {
        return 0;
    }
    for (const char* i = pString; *++i; uiHash = (uiHash << 16) + ((uiHash << 6) + *i) - uiHash) {
        ;
    }
    return uiHash;
}
