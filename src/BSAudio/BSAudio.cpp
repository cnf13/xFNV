#include "BSAudio/BSAudio.h"

#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
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
SettingT<INISettingCollection> BSAudio::fDawnTime("fDawnTime:Audio", 5.0f);
SettingT<INISettingCollection> BSAudio::fDayTime("fDayTime:Audio", 8.0f);
SettingT<INISettingCollection> BSAudio::fDuskTime("fDuskTime:Audio", 18.0f);
SettingT<INISettingCollection> BSAudio::fNightTime("fNightTime:Audio", 20.0f);
BSSoundHandle BSAudio::kVatsLoopHandle;
SettingT<INISettingCollection> bMultiThreadAudio("bMultiThreadAudio:Audio", false);

AudioTick::AudioTick() : iMilliseconds(600000), iFlags(0) {
    iLastUpdateTime = GetTickCount();
}

// Resets the timer as it destroys it.
AudioTick::~AudioTick() {
    iFlags = 0;
    iMilliseconds = 600000;
    iLastUpdateTime = GetTickCount();
}

int32_t AudioTick::IsPaused() {
    return (iFlags & FLAG_PAUSED) != 0;
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
    isMultiThreaded = &bMultiThreadAudio.uValue;
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
    BSAudioManager* pAudioManager;
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    if (bAudioEnabled) {
        if (bMultiThread) {
            singleton = &BSAudioManager::QInstance();
            singleton->StopAudioThread();
        }
        bInitialized = false;
        pAudioManager = &BSAudioManager::QInstance();
        pAudioManager->Shutdown();
    }
}

void BSAudio::PauseWorldVoices() {
    PauseAllOfType(
        static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE),
        true
    );
}

void BSAudio::UnPauseWorldVoices() {
    UnPauseAllOfType(
        static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE),
        true
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

BSSoundHandle BSAudio::GetSoundHandleByName(const char* apSoundName, uint32_t aiTypeFlags) {
    AutoMemContext eMemContext(MEM_CONTEXT_AUDIO);
    return BSAudioManager::QInstance().GetSoundHandleByName(apSoundName, aiTypeFlags);
}

BSSoundHandle BSAudio::GetSoundHandleByFilename(const char* apFilename, uint32_t aiTypeFlags, void* apSettingsContext) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSSoundHandle kHandle = BSAudioManager::QInstance().GetSoundHandleByFilename(
        apFilename,
        aiTypeFlags,
        static_cast<TESSound*>(apSettingsContext)
    );
    return kHandle;
}

BSSoundHandle BSAudio::GetSoundHandleByNumericID(uint32_t aiFormID, uint32_t aiTypeFlags) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSSoundHandle kHandle = BSAudioManager::QInstance().GetSoundHandleByNumericID(aiFormID, aiTypeFlags);
    return kHandle;
}

BSSoundHandle
BSAudio::PlaySound(uint32_t auiFormID, BSGameSound::SoundFlags aeAudioFlags, NiPoint3 akPosition, int32_t aiUnused) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSSoundHandle kHandle = BSAudioManager::QInstance().GetSoundHandleByNumericID(auiFormID, aeAudioFlags);
    kHandle.SetPosition(akPosition.x, akPosition.y, akPosition.z);
    kHandle.Play(true);
    return kHandle;
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
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().PrecacheSound(const_cast<char*>(apFilename), aiFlags, apSettingsContext);
}

bool BSAudio::IsAudioEnabled() {
    return bAudioEnabled;
}

bool BSAudio::IsMultiThreaded() {
    return bMultiThread;
}

void BSAudio::SetMultiThreaded(bool abUseMultipleThreads) {
    Setting::Info* bMultiThreadAudio;
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    // Takes the setting's address, which is never null.
    if (bAudioEnabled &&
        (bMultiThreadAudio = &::bMultiThreadAudio.uValue, bMultiThreadAudio->b || !abUseMultipleThreads)) {
        if (!abUseMultipleThreads || bMultiThread) {
            if (!abUseMultipleThreads && bMultiThread) {
                BSAudioManager::QInstance().SuspendAudioThread();
            }
        }
        else {
            BSAudioManager::QInstance().StartAudioThread();
        }
        bMultiThread = abUseMultipleThreads;
    }
}

void BSAudio::MuteType(uint32_t aiType, uint32_t aiFadeTimeMS, uint32_t aiMillibels) {
    BSAudioManager::QInstance().MuteAllOfType(aiType, aiFadeTimeMS, aiMillibels);
}

float BSAudio::GetMasterSecondaryVolume() {
    return BSAudioManager::QInstance().fVolumes[6];
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
    UnPauseAllOfType(
        static_cast<BSGameSound::SoundFlags>(
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_REGION | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER
        ),
        false
    );
}

void BSAudio::ClearSecondaryVolumes() {
    BSAudioManager::QInstance().ClearSecondaryVolumes();
}

void BSAudio::KillAllOfType(uint32_t aiType, uint32_t aiFadeTimeMS) {
    BSAudioManager::QInstance().KillAllOfType(aiType, aiFadeTimeMS);
}

void BSAudio::PauseWorldSounds() {
    PauseAllOfType(
        static_cast<BSGameSound::SoundFlags>(
            BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_REGION | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER
        ),
        false
    );
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
    NiPointer<NiAVObject> spObject;
    NiPoint3 kVector = NiPoint3::ZERO;
    spObject.m_pObject = nullptr;
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->ComposeMessage(MDIALOGUE_FADE, 0, 0, nullptr, spObject, kVector);
}

void BSAudio::UnPauseSynchTimer() {
    if (pfnSynchUnPausedCallback) {
        pfnSynchUnPausedCallback();
    }
    SynchTimer.iFlags &= ~AudioTick::FLAG_PAUSED;
}

NiPoint3 BSAudio::GetListenerPosition() {
    return pListener ? pListener->GetPosition() : NiPoint3::ZERO;
}

void BSAudio::EnterDialogue() {
    BSAudioManager::QInstance()
        .ComposeMessage(MDIALOGUE_FADE, 1, 0, nullptr, NiPointer<NiAVObject>(nullptr), NiPoint3::ZERO);
}

void BSAudio::PauseSynchTimer() {
    if (pfnSynchPausedCallback) {
        pfnSynchPausedCallback();
    }
    SynchTimer.iFlags |= AudioTick::FLAG_PAUSED;
}

void BSAudio::PrintDebugInfoThunk() {
    BSAudio::PrintDebugInfo();
}

BSSimpleList<AudioDebugString*>* BSAudio::GetDebugString(uint32_t aiPage) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSSimpleList<AudioDebugString*>* result = BSAudioManager::QInstance().GetDebugString(aiPage);
    return result;
}

void BSAudio::StopMovingSounds(NiAVObject* apObj, float afFadeSeconds, uint32_t aiFlags) {
    BSAudioManager::QInstance().StopMovingSounds(apObj, afFadeSeconds, aiFlags);
}

BSAudio& BSAudio::QInstance() {
    return *pAudioInstance;
}

float BSAudio::GetMinDialogDist() {
    return BSAudio::fDialogMinDistance.Float();
}

float BSAudio::GetMaxDialogDist() {
    return BSAudio::fDialogMaxDistance.Float();
}

// The deleting destructor frees through the engine's heap.
void BSAudio::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
