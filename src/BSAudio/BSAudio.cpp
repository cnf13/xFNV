#include "BSAudio/BSAudio.h"

#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

SettingT<INISettingCollection> BSAudio::bEnableAudio("bEnableAudio:Audio", true);
SettingT<INISettingCollection> BSAudio::fDialogMinDistance("fDialogMinDistance:Audio", 70.0f);
SettingT<INISettingCollection> BSAudio::fDialogMaxDistance("fDialogMaxDistance:Audio", 50000.0f);
SettingT<INISettingCollection> gs_fDawnTime_Audio("fDawnTime:Audio", 5.0f);
SettingT<INISettingCollection> gs_fDayTime_Audio("fDayTime:Audio", 8.0f);
SettingT<INISettingCollection> gs_fDuskTime_Audio("fDuskTime:Audio", 18.0f);
SettingT<INISettingCollection> gs_fNightTime_Audio("fNightTime:Audio", 20.0f);
BSSoundHandle BSAudio::kVatsLoopHandle;
SettingT<INISettingCollection> gs_bMultiThreadAudio_Audio("bMultiThreadAudio:Audio", false);

AudioTick::AudioTick() : iMilliseconds(600000), iFlags(0) {
    iLastUpdateTime = GetTickCount();
}

// Resets the timer as it destroys it, as the game does.
AudioTick::~AudioTick() {
    iFlags = 0;
    iMilliseconds = 600000;
    iLastUpdateTime = GetTickCount();
}

int32_t AudioTick::IsPaused() {
    return (int32_t)((uint8_t)((iFlags & 1) != 0));
}

// Resets the timer: no flags, 0x927C0 (600000) milliseconds, the last update now. Returns GetTickCount's value.
uint32_t AudioTick::Restart() {
    iFlags = 0;
    iMilliseconds = 0x927C0;
    uint32_t uiTime = GetTickCount();
    iLastUpdateTime = static_cast<int32_t>(uiTime);
    return uiTime;
}

BSAudio::BSAudio()
  : bAudioEnabled(false), bInitialized(false), iMusicStartOffset(0), bSeekMusic(false), pListener(nullptr),
    pfnIDCallback(nullptr), pfnNameCallback(nullptr), pfnRandomFilenameCallback(nullptr), pfnSettingsCallback(nullptr),
    pfnSynchPausedCallback(nullptr), pfnSynchUnPausedCallback(nullptr) {
    Setting::Info* isMultiThreaded;
    // Takes the setting's address, which is never null.
    isMultiThreaded = &gs_bMultiThreadAudio_Audio.uValue;
    bMultiThread = isMultiThreaded->b;
}

// SynchTimer's own destructor runs after this.
BSAudio::~BSAudio() {}

// The audio flag is read from the global setting; the timer is restarted in its embedded storage.
void BSAudio::Init(void* pHandle) {
    AutoMemContext eMemContext(MEM_CONTEXT_AUDIO);
    if (bEnableAudio.uValue.b) {
        SynchTimer.Restart();
        if (bMultiThread) {
            BSAudioManager::QInstance().StartAudioThread();
        }
        bInitialized = true;
    }
    else {
        bAudioEnabled = false;
        bInitialized = false;
    }
}

void BSAudio::Shutdown() {
    BSAudioManager* singleton;
    BSAudioManager* v2;
    int32_t eMemContext = GetThreadMemContext();
    GetThreadMemContext() = 11;
    if (bAudioEnabled) {
        if (bMultiThread) {
            singleton = &BSAudioManager::QInstance();
            singleton->StopAudioThread();
        }
        bInitialized = (bool)0;
        v2 = &BSAudioManager::QInstance();
        v2->Shutdown();
    }
    GetThreadMemContext() = eMemContext;
}

void BSAudio::PauseWorldVoices() {
    PauseAllOfType(
        static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE), true
    );
}

void BSAudio::UnPauseWorldVoices() {
    UnPauseAllOfType(
        static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE), true
    );
}

void BSAudio::PauseAnimationSounds() {
    PauseAllOfType(BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN, true);
}

void BSAudio::UnPauseAnimationSounds() {
    UnPauseAllOfType(BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN, true);
}

void BSAudio::UnMute(uint32_t aiFadeTimeMS) {
    // Every sound type.
    UnMuteType(0xFFFFFFFFu, aiFadeTimeMS);
}

void BSAudio::UnMuteType(uint32_t aiType, uint32_t aiFadeTimeMS) {
    BSAudioManager::QInstance().UnMuteAllOfType(aiType, aiFadeTimeMS);
}

void BSAudio::PauseAllOfType(uint32_t aiType, bool abExactMatch) {
    BSAudioManager::QInstance().PauseAllOfType(aiType, abExactMatch);
}

void BSAudio::UnPauseAllOfType(uint32_t aiType, bool abExactMatch) {
    BSAudioManager::QInstance().UnPauseAllOfType(aiType, abExactMatch);
}

void BSAudio::EndAllSoundsOfType(uint32_t) {
    // The shipping build has no body here.
}

bool BSAudio::IsListenerUnderwater() {
    return BSAudioManager::QInstance().bListenerUnderwater;
}

void BSAudio::SetEnvironment(uint32_t aiEnviroType) {
    if (pListener != nullptr) {
        pListener->SetEnvironmentType(aiEnviroType);
    }
}

void BSAudio::SetListenerUnderwater(bool abIsUnderwater) {
    BSAudioManager::QInstance().SetListenerUnderwater(abIsUnderwater);
}

void BSAudio::PositionListener(NiPoint3 aNewPosition) {
    BSAudioManager::QInstance().PositionListener(aNewPosition);
}

// Builds a handle for the named sound and returns the caller's result object.
BSSoundHandle* BSAudio::GetSoundHandleByName(BSSoundHandle* apResult, const char* apSoundName, uint32_t aiTypeFlags) {
    AutoMemContext eMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().GetSoundHandleByName(apResult, apSoundName, aiTypeFlags);
    return apResult;
}

BSSoundHandle* BSAudio::GetSoundHandleByFilename(
    BSSoundHandle* apResult,
    const char* apFilename,
    uint32_t aiTypeFlags,
    void* apSettingsContext
) {
    int32_t eMemContext = GetThreadMemContext();
    GetThreadMemContext() = 11;
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->GetSoundHandleByFilePath(apResult, apFilename, aiTypeFlags, static_cast<TESSound*>(apSettingsContext));
    GetThreadMemContext() = eMemContext;
    return apResult;
}

BSSoundHandle* BSAudio::GetSoundHandleByNumericID(BSSoundHandle* apResult, uint32_t aiFormID, uint32_t aiTypeFlags) {
    int32_t uiHeapIndex = GetThreadMemContext();
    GetThreadMemContext() = 11;
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->GetSoundHandleByNumericID(apResult, aiFormID, aiTypeFlags);
    GetThreadMemContext() = uiHeapIndex;
    return apResult;
}

BSSoundHandle* BSAudio::PlaySound(
    BSSoundHandle* apResult,
    uint32_t auiFormID,
    BSGameSound::SoundFlags aeAudioFlags,
    NiPoint3 akPosition,
    int32_t aiUnused
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    BSAudioManager* v6 = &BSAudioManager::QInstance();
    v6->GetSoundHandleByNumericID(&apHandle, auiFormID, aeAudioFlags);
    apHandle.SetPosition(akPosition.x, akPosition.y, akPosition.z);
    apHandle.Play(1);
    // Copies the handle's three fields into the caller's buffer, leaving the padding after bAssumeSuccess alone.
    apResult->iSoundID = apHandle.iSoundID;
    apResult->bAssumeSuccess = apHandle.bAssumeSuccess;
    apResult->eState = apHandle.eState;
    GetThreadTLSData()->eMemContext = eMemContext;
    return apResult;
}

void BSAudio::SetConditions(float afCurrentGamehour, uint32_t aiWeatherConditions) {
    AutoMemContext eMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().SetConditions(afCurrentGamehour, aiWeatherConditions);
}

// Updates the audio manager while the audio heap context is active.
void BSAudio::Update(bool abPlayGameWorldSounds) {
    AutoMemContext eMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().Update(abPlayGameWorldSounds);
}

// Queues a sound for caching while the audio heap context is active.
void BSAudio::Precache(const char* apSoundName, uint32_t aiFlags) {
    AutoMemContext eMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().Precache(apSoundName, aiFlags);
}

void BSAudio::PrecacheSound(const char* apFilename, uint32_t aiFlags, TESSound* apSettingsContext) {
    int32_t eMemContext = GetThreadMemContext();
    GetThreadMemContext() = 11;
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->PrecacheSound(const_cast<char*>(apFilename), aiFlags, apSettingsContext);
    GetThreadMemContext() = eMemContext;
}

bool BSAudio::IsAudioEnabled() {
    return bAudioEnabled;
}

bool BSAudio::IsMultiThreaded() {
    return bMultiThread;
}

void BSAudio::SetMultiThreaded(bool abUseMultipleThreads) {
    Setting::Info* bMultiThreadAudio;
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    // Takes the setting's address, which is never null.
    if (bAudioEnabled &&
        (bMultiThreadAudio = &gs_bMultiThreadAudio_Audio.uValue, bMultiThreadAudio->b || !abUseMultipleThreads)) {
        if (!abUseMultipleThreads || bMultiThread) {
            if (!abUseMultipleThreads && bMultiThread) {
                BSAudioManager* v3 = &BSAudioManager::QInstance();
                v3->SuspendAudioThread();
            }
        }
        else {
            BSAudioManager* v2 = &BSAudioManager::QInstance();
            v2->StartAudioThread();
        }
        bMultiThread = abUseMultipleThreads;
        GetThreadTLSData()->eMemContext = eMemContext;
    }
    else {
        GetThreadTLSData()->eMemContext = eMemContext;
    }
}

void BSAudio::MuteType(uint32_t aiType, uint32_t aiFadeTimeMS, uint32_t aiMillibels) {
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->MuteAllOfType(aiType, aiFadeTimeMS, aiMillibels);
}

float BSAudio::GetMasterSecondaryVolume() {
    return (float)(BSAudioManager::QInstance().fVolumes[6]);
}

// The volume setters take the audio manager, store the volume in its slot of fVolumes and call VolumeChanged.
void BSAudio::SetMasterVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[0] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::SetFootVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[1] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::SetVoiceVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[2] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::SetEffectVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[3] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::SetMusicVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[4] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::SetRadioVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[5] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::SetMasterSecondaryVolume(float afNewVol) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    pManager->fVolumes[6] = afNewVol;
    pManager->VolumeChanged();
}

void BSAudio::VolumeChanged() {
    BSAudioManager::QInstance().VolumeChanged();
}

void BSAudio::UnPauseWorldSounds() {
    UnPauseAllOfType(static_cast<BSGameSound::SoundFlags>(0x3002u), 0);
}

void BSAudio::ClearSecondaryVolumes() {
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->ClearSecondaryVolumes();
}

void BSAudio::KillAllOfType(uint32_t aiType, uint32_t aiFadeTimeMS) {
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->KillAllOfType(aiType, aiFadeTimeMS);
}

void BSAudio::PauseWorldSounds() {
    PauseAllOfType(static_cast<BSGameSound::SoundFlags>(0x3002u), 0);
}

float BSAudio::GetFootSecondaryVolume() {
    return BSAudioManager::QInstance().fVolumes[7];
}

float BSAudio::GetVoiceSecondaryVolume() {
    return BSAudioManager::QInstance().fVolumes[8];
}

float BSAudio::GetEffectSecondaryVolume() {
    return BSAudioManager::QInstance().fVolumes[9];
}

float BSAudio::GetMusicSecondaryVolume() {
    return BSAudioManager::QInstance().fVolumes[10];
}

float BSAudio::GetRadioSecondaryVolume() {
    return BSAudioManager::QInstance().fVolumes[11];
}

void BSAudio::ExitDialogue() {
    NiPointer<NiAVObject> v2;
    BSAudio* v5 = this;
    NiPoint3 v3 = NiPoint3_ZERO;
    NiPointer<NiAVObject>* v7 = &v2;
    NiPointer<NiAVObject>* v6 = &v2;
    v2.m_pObject = nullptr;
    NiPointer<NiAVObject>* v4 = &v2;
    int32_t v8 = 0;
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    v8 = (-1);
    singleton->ComposeMessage(MDIALOGUE_FADE, 0, 0, nullptr, v2, v3);
}

void BSAudio::UnPauseSynchTimer() {
    if (pfnSynchUnPausedCallback) {
        pfnSynchUnPausedCallback();
    }
    SynchTimer.iFlags &= ~1u;
}

NiPoint3* BSAudio::GetListenerPosition(NiPoint3* apResult) {
    NiPoint3* v3;
    if (pListener) {
        v3 = &pListener->GetPosition();
    }
    else {
        v3 = &NiPoint3_ZERO;
    }
    *apResult = *v3;
    return apResult;
}

void BSAudio::EnterDialogue() {
    BSAudioManager::QInstance().ComposeMessage(
        MDIALOGUE_FADE, 1, 0, nullptr, NiPointer<NiAVObject>(nullptr), NiPoint3_ZERO
    );
}

void BSAudio::PauseSynchTimer() {
    if (pfnSynchPausedCallback) {
        pfnSynchPausedCallback();
    }
    SynchTimer.iFlags |= 1u;
}

void BSAudio::PrintDebugInfoThunk() {
    BSAudio::PrintDebugInfo();
}

BSSimpleList<AudioDebugString*>* BSAudio::GetDebugString(uint32_t aiPage) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    BSAudioManager* v2 = &BSAudioManager::QInstance();
    BSSimpleList<AudioDebugString*>* result = v2->GetDebugString(aiPage);
    GetThreadTLSData()->eMemContext = eMemContext;
    return result;
}

void BSAudio::StopMovingSounds(NiAVObject* apObj, float afFadeSeconds, uint32_t aiFlags) {
    BSAudioManager::QInstance().StopMovingSounds(apObj, afFadeSeconds, aiFlags);
}

BSAudio& BSAudio::QInstance() {
    return *pAudioInstance;
}

float BSAudio::GetMinDialogDist() {
    return (&BSAudio::fDialogMinDistance)->Float();
}

float BSAudio::GetMaxDialogDist() {
    return (&BSAudio::fDialogMaxDistance)->Float();
}

// The deleting destructor frees through the engine's heap.
void BSAudio::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}
