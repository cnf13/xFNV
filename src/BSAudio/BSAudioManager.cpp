#include "BSAudio/BSAudioManager.h"

#include "BSAudio/AudioLoadTask.h"
#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSAudioManagerThread.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/SoundMessageStack.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSThread.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSCore/Xenon/BSThread_Xenon.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/BaseProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "platform/System.h"

SettingT<INIPrefSettingCollection> gs_fDefaultMasterVolume_Audio("fDefaultMasterVolume:Audio", 1.0f);
SettingT<INIPrefSettingCollection> gs_fDefaultFootVolume_Audio("fDefaultFootVolume:Audio", 0.5f);
SettingT<INIPrefSettingCollection> gs_fDefaultVoiceVolume_Audio("fDefaultVoiceVolume:Audio", 0.6f);
SettingT<INIPrefSettingCollection> gs_fDefaultEffectsVolume_Audio("fDefaultEffectsVolume:Audio", 1.0f);
SettingT<INIPrefSettingCollection> gs_fDefaultMusicVolume_Audio("fDefaultMusicVolume:Audio", 0.6f);
SettingT<INIPrefSettingCollection> gs_fDefaultRadioVolume_Audio("fDefaultRadioVolume:Audio", 0.5f);
SettingT<INISettingCollection> g_bUseAudioDebugInformation_Audio("bUseAudioDebugInformation:Audio", true);
SettingT<INISettingCollection> gs_fAudioDebugDelay_Audio("fAudioDebugDelay:Audio", 0.0f);
SettingT<INISettingCollection> gs_fMasterVolumeMult("fMasterVolumeMult:Audio", 1.3f);
SettingT<INISettingCollection> fDialogueFadeDecibels("fDialogueFadeDecibels:Audio", 6.0f);
SettingT<INISettingCollection> fDialogueFadeSecondsIn("fDialogueFadeSecondsIn:Audio", 2.0f);
SettingT<INISettingCollection> fDialogueFadeSecondsOut("fDialogueFadeSecondsOut:Audio", 1.0f);
static SettingT<INISettingCollection> iAttenuationEffectiveSilence("iAttenuationEffectiveSilence:Audio", 9000);
static SettingT<INISettingCollection> iAttenuationDialogMenuMute("iAttenuationDialogMenuMute:Audio", 3000);

SettingT<INISettingCollection> gs_bEnableAudioCache_Audio("bEnableAudioCache:Audio", true);
SettingT<INISettingCollection> gs_iAudioCacheSize_Audio("iAudioCacheSize:Audio", 2048);
SettingT<INISettingCollection> gs_iMaxSizeForCachedSound("iMaxSizeForCachedSound:Audio", 256);
SettingT<INISettingCollection> gs_iAudioHWThread_Audio("iAudioHWThread:Audio", 2);

int32_t Compare_AE5FB0(AudioDebugString* apA, AudioDebugString* apB) {
    if (apA->iSoundID >= apB->iSoundID) {
        return (int32_t)((uint8_t)(apB->iSoundID < apA->iSoundID));
    }

    return (-1);
}

void au_re_ExitThread(uint32_t dwExitCode, int32_t x0) {
    BSSystemThread::ExitCurrent(dwExitCode);
}

// The message lists and stacks and the task and cached-sound lists start empty through their own constructors.
BSAudioManager::BSAudioManager()
  : iMessageListIndex(0), iMessageProcessingListIndex(1), SoundList(37), SoundStateMap(37), StateUpdateMap(37),
    MovingRefrList(37), iCurrentCachSize(0), iWeatherFlags(0), fCurrentGamehour(0.0f), bPrecacheCompleted(false),
    bListenerMoved(false), dwAudioThreadID(0), pUpdateThread(nullptr), fVolumes{}, uiToFadeFlags(0),
    bDialogueFadeOut(false), bListenerUnderwater(false), uiSoundCacheIndex(1) {
    InitializeCriticalSection(&MessageCritSection);
    InitializeCriticalSection(&StateCritSection);
    InitializeCriticalSection(&CacheCritSection);
    InitializeCriticalSection(&ProcessingCritSection);
    InitializeCriticalSection(&CacheLoadTaskCritSection);
    iDelayTimerDelta = GetTickCount();
}

// The lists, the maps and the message stacks and lists are emptied by their own destructors.
BSAudioManager::~BSAudioManager() {
    DeleteCriticalSection(&CacheLoadTaskCritSection);
    DeleteCriticalSection(&ProcessingCritSection);
    DeleteCriticalSection(&CacheCritSection);
    DeleteCriticalSection(&StateCritSection);
    DeleteCriticalSection(&MessageCritSection);
}

void BSAudioManager::Shutdown() {
    Setting::Info* v1;
    Setting::Info* p_uValue;
    Setting::Info* bEnableAudioCache;
    NiTMapItem<int32_t, BSSoundInfo*>* it;
    NiTMapItem<int32_t, BSGameSound*>* v7;
    int32_t ikey;
    BSSoundInfo* ptmp;
    BSGameSound* psound;
    if (!BSAudio::pAudioInstance->bMultiThread) {
        Update(false);
        Update(false);
        Update(false);
    }
    BSSoundMessage* pMsg = nullptr;
    while (Messages[0].uiCount) {
        pMsg = Messages[0].RemoveHead();
        if (pMsg) {
            pMsg->~BSSoundMessage();
            MemoryManager_Singleton.Deallocate(pMsg);
        }
    }
    while (Messages[1].uiCount) {
        pMsg = Messages[1].RemoveHead();
        if (pMsg) {
            pMsg->~BSSoundMessage();
            MemoryManager_Singleton.Deallocate(pMsg);
        }
    }
    pMsg = nullptr;
    if (pUpdateThread) {
        delete pUpdateThread;
        pUpdateThread = nullptr;
    }
    // The sounds are deleted while an AudioLoadTask may still be opening one on the IO thread (a sound the quit's menu
    // closing played), so pausing waits for the running task and cancelling stops the queued ones (their Cancel marks a
    // listed sound stopped), as ModelLoader::CancelAllFiles does at the same quit.
    if (pIOManager) {
        pIOManager->Pause();
        pIOManager->CancelAllTasks();
    }
    psound = nullptr;
    ikey = 0;
    for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
        if (SoundList.m_ppkHashTable[i]) {
            v7 = SoundList.m_ppkHashTable[i];
            goto LABEL_20;
        }
    }
    v7 = nullptr;
LABEL_20:;
    NiTMapItem<int32_t, BSGameSound*>* pos = v7;
    while (pos) {
        SoundList.GetNext(pos, ikey, psound);
        SoundList.RemoveAt(psound->iSoundID);
        SetVoiceState(psound->iSoundID, nullptr);
        if (psound) {
            delete psound;
        }
    }
    if (pIOManager) {
        pIOManager->Resume();
    }
    MovingRefrList.RemoveAll_0();
    for (uint32_t j = 0; j < StateUpdateMap.m_uiHashSize; ++j) {
        if (StateUpdateMap.m_ppkHashTable[j]) {
            it = StateUpdateMap.m_ppkHashTable[j];
            goto LABEL_31;
        }
    }
    it = nullptr;
LABEL_31:;
    NiTMapItem<int32_t, BSSoundInfo*>* it_2 = it;
    BSSoundInfo* pdel = nullptr;
    ikey = 0;
    while (it_2) {
        StateUpdateMap.GetNext(it_2, ikey, ptmp);
        while (SoundStateMap.GetAt(ikey, pdel)) {
            while (SoundStateMap.RemoveAt(ikey)) {
                ;
            }
            BSSoundInfo* apMem = pdel;
            if (pdel) {
                pdel->~BSSoundInfo();
                MemoryManager_Singleton.Deallocate(apMem);
            }
        }
        BSSoundInfo* apThis = ptmp;
        if (ptmp) {
            ptmp->~BSSoundInfo();
            MemoryManager_Singleton.Deallocate(apThis);
        }
    }
    StateUpdateMap.RemoveAll_0();
    // The setting's address is never null.
    bEnableAudioCache = &gs_bEnableAudioCache_Audio.uValue;
    bool bCacheWasEnabled = bEnableAudioCache->b;
    // The setting's address is never null.
    p_uValue = &gs_bEnableAudioCache_Audio.uValue;
    p_uValue->b = false;
    AddToCache(nullptr);
    // The setting's address is never null.
    v1 = &gs_bEnableAudioCache_Audio.uValue;
    v1->b = bCacheWasEnabled;
    MemoryManager_Singleton.Deallocate(pMutedSoundTypes);
}

// As SuspendAudioThread, with MTERMINATE_THREAD and the thread closed after it acknowledges.
void BSAudioManager::StopAudioThread() {
    if (this->pUpdateThread) {
        this->pUpdateThread->Resume();
    }
    ComposeMessage(MTERMINATE_THREAD, 0, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    if (this->pUpdateThread) {
        BSAudioManagerThread* pUpdateThread = this->pUpdateThread;
        int32_t* lpAddend = &pUpdateThread->m_SuspendedSema.iCount;
        pUpdateThread->m_SuspendedSema.hSemaphore->Wait();
        InterlockedDecrementInt32(lpAddend);
        this->pUpdateThread->Close();
    }
}

BSSoundHandle*
BSAudioManager::GetSoundHandleByNumericID(BSSoundHandle* apHandle, uint32_t auiFormID, uint32_t aiTypeFlags) {
    char apFilename[268];
    void* pSettingsContext;
    XFNV_INLINED_LOCAL(BSSoundHandle, returnHandle, std::nullopt, std::nullopt);
    uint32_t isoundTypeFlags;
    bool (*pfncallback)(uint32_t, char*, uint32_t*, void**);
    isoundTypeFlags = 0;
    if (BSAudio::pAudioInstance->bAudioEnabled &&
        ((pfncallback = BSAudio::pAudioInstance->pfnIDCallback, (pSettingsContext = nullptr, pfncallback)) &&
         pfncallback(auiFormID, apFilename, &isoundTypeFlags, &pSettingsContext))) {
        if ((isoundTypeFlags & 1) != 0 && (aiTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0) {
            aiTypeFlags = (aiTypeFlags & ~3u) | 1;
        }
        if ((isoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D_RADIUS) != 0) {
            aiTypeFlags = (aiTypeFlags &
                           ~static_cast<BSGameSound::SoundFlags>(
                               BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_3D |
                               BSGameSound::SOUND_FLAG_IS_2D_RADIUS
                           )) |
                (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_RADIUS);
        }
        if ((isoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) {
            aiTypeFlags = (aiTypeFlags &
                           ~static_cast<BSGameSound::SoundFlags>(
                               BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_ENVELOPE_FAST
                           )) |
                BSGameSound::SOUND_FLAG_ENVELOPE_FAST;
        }
        else {
            if ((isoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0) {
                aiTypeFlags = (aiTypeFlags &
                               ~static_cast<BSGameSound::SoundFlags>(
                                   BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_ENVELOPE_SLOW
                               )) |
                    BSGameSound::SOUND_FLAG_ENVELOPE_SLOW;
            }
        }
        GetSoundHandleByFilePath(&returnHandle, apFilename, aiTypeFlags, static_cast<TESSound*>(pSettingsContext));
        // EXE-INLINED 0x418900
        apHandle->iSoundID = returnHandle.iSoundID;
        apHandle->bAssumeSuccess = returnHandle.bAssumeSuccess;
        apHandle->eState = returnHandle.eState;
        return apHandle;
    }

    apHandle->iSoundID = -1;
    apHandle->bAssumeSuccess = false;
    apHandle->eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
    return apHandle;
}

BSSoundHandle* BSAudioManager::GetSoundHandleByFilePath(
    BSSoundHandle* apResult,
    const char* apFilename,
    uint32_t aeTypeFlags,
    TESSound* apSound
) {
    XFNV_INLINED_LOCAL(BSSoundHandle, kHandle, std::nullopt, std::nullopt);
    if (!BSAudio::pAudioInstance->bAudioEnabled) {
        apResult->iSoundID = -1;
        apResult->bAssumeSuccess = false;
        apResult->eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
        return apResult;
    }
    int32_t uiSoundID = this->uiSoundCacheIndex++;
    if (!*apFilename) {
        Warning("Trying to create a sound with no filename\n");
        apResult->iSoundID = -1;
        apResult->bAssumeSuccess = false;
        apResult->eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
        return apResult;
    }
    if ((aeTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) == 0 && (aeTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) == 0) {
        Warning("Audio: Attempting to init %s without defining it as a 2D or 3D sound.", apFilename);
        aeTypeFlags |= BSGameSound::SOUND_FLAG_IS_2D;
    }
    BSGameSound* pGameSound = BSAudio::pAudioInstance->CreateNewGameSound(apFilename);
    pGameSound->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(aeTypeFlags);
    pGameSound->iSoundID = uiSoundID;
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_fAudioDebugDelay_Audio.uValue;
    // Truncates to a 64-bit integer and keeps the low 32 bits.
    pGameSound->iCleanupDelayInMS = static_cast<uint32_t>(static_cast<int64_t>(p_uValue->f * 1000.0));
    if (BSAudio::pAudioInstance->pListener) {
        BSAudioListener* pListener = BSAudio::pAudioInstance->pListener;
        pGameSound->SetEnvironmentType(pListener->GetEnvironmentType());
    }
    kHandle.iSoundID = uiSoundID;
    kHandle.bAssumeSuccess = false;
    kHandle.eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
    EnterCriticalSection(&this->StateCritSection);
    BSSoundInfo* pInfo;
    BSSoundInfo* pNewInfo = static_cast<BSSoundInfo*>(MemoryManager_Singleton.Allocate(sizeof(BSSoundInfo)));
    if (pNewInfo) {
        ::new (&pNewInfo->kDebugInfo) BSSoundInfo::Data;
        pNewInfo->iID = uiSoundID;
        pNewInfo->uiSoundType = 0;
        pNewInfo->uiDuration = 0;
        pNewInfo->bIsPlaying = false;
        pInfo = pNewInfo;
    }
    else {
        pInfo = nullptr;
    }
    SoundStateMap.SetAt(uiSoundID, pInfo);
    LeaveCriticalSection(&this->StateCritSection);
    uint32_t uiSettingsFlags = 0;
    uint32_t uiModifierFlags = 0;
    decltype(BSAudio::pfnSettingsCallback) pfnSettingsCallback = BSAudio::pAudioInstance->pfnSettingsCallback;
    if (apSound && pfnSettingsCallback) {
        uiSettingsFlags = pfnSettingsCallback(&kHandle, apSound, &uiModifierFlags);
    }
    pGameSound->iModifierFlags = uiModifierFlags;
    if (uiSettingsFlags) {
        if ((uiSettingsFlags & BSGameSound::SOUND_FLAG_IS_2D_RADIUS) != 0) {
            aeTypeFlags = (aeTypeFlags &
                           ~static_cast<BSGameSound::SoundFlags>(
                               BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_3D |
                               BSGameSound::SOUND_FLAG_IS_2D_RADIUS
                           )) |
                (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_RADIUS);
        }
        if ((uiSettingsFlags & BSGameSound::SOUND_FLAG_LOOP) != 0 ||
            ((uiSettingsFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0 ||
             (uiSettingsFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0)) {
            aeTypeFlags = (aeTypeFlags &
                           ~static_cast<BSGameSound::SoundFlags>(
                               BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_ENVELOPE_FAST |
                               BSGameSound::SOUND_FLAG_ENVELOPE_SLOW
                           )) |
                (uiSettingsFlags &
                 (BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_ENVELOPE_FAST |
                  BSGameSound::SOUND_FLAG_ENVELOPE_SLOW));
        }
        pGameSound->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(aeTypeFlags);
    }
    ComposeMessage(MADD_SOUND_TO_LIST, pGameSound->iSoundID, 0, pGameSound, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    if (apSound && pfnSettingsCallback) {
        uiSettingsFlags = pfnSettingsCallback(&kHandle, apSound, nullptr);
    }
    BSAudioLoopPointCallback pfnLoopPointCallback = BSAudio::pAudioInstance->GetLoopPointCallback();
    uint32_t uiLoopStart = 0;
    uint32_t uiLoopEnd = 0;
    if (pfnLoopPointCallback && pfnLoopPointCallback(apSound, &uiLoopStart, &uiLoopEnd)) {
        pGameSound->iLoopStart = uiLoopStart;
        pGameSound->iLoopEnd = uiLoopEnd;
    }
    // EXE-INLINED 0x418900
    apResult->iSoundID = kHandle.iSoundID;
    apResult->bAssumeSuccess = kHandle.bAssumeSuccess;
    apResult->eState = kHandle.eState;
    return apResult;
}

BSSoundHandle* BSAudioManager::SpawnSoundReference(
    BSSoundHandle* apResult,
    int32_t a2,
    int32_t a3,
    int32_t a4,
    int32_t a5,
    int32_t a6,
    int32_t a7
) {
    apResult->iSoundID = -1;
    apResult->bAssumeSuccess = false;
    apResult->eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
    return apResult;
}

void BSAudioManager::PrecacheSoundFromID(uint32_t auiFormID, int32_t aiFlags) {
    void* pSettings;
    uint32_t v6;
    v6 = aiFlags;
    bool (*pfnIDCallback)(uint32_t, char*, uint32_t*, void**) = BSAudio::pAudioInstance->pfnIDCallback;
    pSettings = nullptr;
    if ((BSAudio::pAudioInstance->bAudioEnabled) && (pfnIDCallback)) {
        char a1[264];
        if (pfnIDCallback(auiFormID, a1, &v6, &pSettings)) {
            // The callback's flag bits and the caller's, combined into the sound flags.
            PrecacheSound(a1, v6 | aiFlags, static_cast<TESSound*>(pSettings));
        }
    }
}

bool BSAudioManager::SetBeamEnd(uint32_t aiSoundID, float afX, float afY, float afZ) {
    NiPoint3 kEnd;
    kEnd.x = afX;
    kEnd.y = afY;
    kEnd.z = afZ;
    ComposeMessage(MSET_BEAM_END, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, kEnd);
    return false;
}

// Queues the completion callback for the sound: the context travels in the message's data, the callback in its pointer
// data, which is treated as an untyped pointer (the audio thread casts it back by message).
bool BSAudioManager::SetCompletionCallback(
    uint32_t aiSoundID,
    BSAudioManager::CompletionCallback apfnCallback,
    void* apSoundContext
) {
    ComposeMessage(
        MSET_COMPLETION_CALLBACK, aiSoundID, reinterpret_cast<intptr_t>(apSoundContext),
        reinterpret_cast<void*>(apfnCallback), NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
    return true;
}

// Queues the play callback for the sound: the context travels in the message's data, the callback in its untyped
// pointer data.
bool BSAudioManager::SetPlayCallback(
    uint32_t aiSoundID,
    BSAudioManager::PlayCallback apfnCallback,
    void* apSoundContext
) {
    ComposeMessage(
        MSET_PLAY_CALLBACK, aiSoundID, reinterpret_cast<intptr_t>(apSoundContext),
        reinterpret_cast<void*>(apfnCallback), NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
    return true;
}

// The vector's x is the follow-up action.
void BSAudioManager::FadeOut(uint32_t aiID, uint32_t aiMilliseconds, uint32_t aiFollowUpAction) {
    NiPoint3 v5;
    v5.x = static_cast<float>(aiFollowUpAction);
    v5.y = 0.0f;
    v5.z = 0.0f;
    ComposeMessage(
        MFADE_OUT, aiID, 0, reinterpret_cast<void*>(static_cast<uintptr_t>(aiMilliseconds)), NiPointer<NiAVObject>{}, v5
    );
}

uint32_t BSAudioManager::GetSoundType(uint32_t aiSoundID) {
    BSSoundInfo* si;
    uint32_t iretValue_2 = 0;
    EnterCriticalSection(&StateCritSection);
    if (SoundStateMap.GetAt(aiSoundID, si)) {
        uint32_t iretValue;
        if (si) {
            iretValue = si->uiSoundType;
        }
        else {
            iretValue = 0;
        }
        iretValue_2 = iretValue;
    }
    else {
        iretValue_2 = 0;
    }
    LeaveCriticalSection(&StateCritSection);
    return iretValue_2;
}

void BSAudioManager::StartAudioThread() {
    if (pUpdateThread) {
        if (!pUpdateThread->m_ThreadID) {
            Setting::Info* iAudioHWThread;
            pUpdateThread->Initialize(BSThread::MEDIUM_STACK_SIZE, "LibAudioUpdate", false);
            dwAudioThreadID = pUpdateThread->m_ThreadID;
            // The setting's address is never null.
            iAudioHWThread = &gs_iAudioHWThread_Audio.uValue;
            pUpdateThread->SetThreadProcessor(iAudioHWThread->uint);
            pUpdateThread->SetName("LibAudioUpdate");
            pUpdateThread->m_ThreadHandle->SetName("LibAudioUpdate");
        }
        pUpdateThread->Resume();
    }
}

BSSoundHandle*
BSAudioManager::GetSoundHandleByName(BSSoundHandle* apHandle, const char* apEditorID, uint32_t aiTypeFlags) {
    char apFilename[268];
    void* pSettingsContext;
    XFNV_INLINED_LOCAL(BSSoundHandle, returnHandle, std::nullopt, std::nullopt);
    uint32_t isoundTypeFlags;
    bool (*pfncallback)(const char*, char*, uint32_t*, void**);
    isoundTypeFlags = 0;
    if (BSAudio::pAudioInstance->bAudioEnabled &&
        ((pfncallback = BSAudio::pAudioInstance->pfnNameCallback, (pSettingsContext = nullptr, pfncallback)) &&
         pfncallback(apEditorID, apFilename, &isoundTypeFlags, &pSettingsContext))) {
        if ((isoundTypeFlags & 0x8000000) != 0) {
            aiTypeFlags |= static_cast<BSGameSound::SoundFlags>(
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_RADIUS
            );
        }
        if ((isoundTypeFlags & 0x2000000) != 0) {
            aiTypeFlags = (aiTypeFlags &
                           ~static_cast<BSGameSound::SoundFlags>(
                               BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_ENVELOPE_FAST
                           )) |
                BSGameSound::SOUND_FLAG_ENVELOPE_FAST;
        }
        else {
            if ((isoundTypeFlags & 0x4000000) != 0) {
                aiTypeFlags = (aiTypeFlags &
                               ~static_cast<BSGameSound::SoundFlags>(
                                   BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_ENVELOPE_SLOW
                               )) |
                    BSGameSound::SOUND_FLAG_ENVELOPE_SLOW;
            }
        }
        if (apFilename[0]) {
            GetSoundHandleByFilePath(&returnHandle, apFilename, aiTypeFlags, static_cast<TESSound*>(pSettingsContext));
            // The handle is copied field by field here, not through BSSoundHandle::operator=.
            apHandle->iSoundID = returnHandle.iSoundID;
            apHandle->bAssumeSuccess = returnHandle.bAssumeSuccess;
            apHandle->eState = returnHandle.eState;
        }
        else {
            apHandle->iSoundID = -1;
            apHandle->bAssumeSuccess = false;
            apHandle->eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
        }
        return apHandle;
    }

    apHandle->iSoundID = -1;
    apHandle->bAssumeSuccess = false;
    apHandle->eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
    return apHandle;
}

void BSAudioManager::Precache(const char* apToCache, uint32_t aiFlags) {
    void* pSettingsContext;
    uint32_t isoundTypeFlags;
    isoundTypeFlags = aiFlags;
    bool (*pfncallback)(const char*, char*, uint32_t*, void**) = BSAudio::pAudioInstance->pfnNameCallback;
    pSettingsContext = nullptr;
    if ((BSAudio::pAudioInstance->bAudioEnabled) && (pfncallback)) {
        char a1[264];
        if (pfncallback(apToCache, a1, &isoundTypeFlags, &pSettingsContext)) {
            PrecacheSound(a1, aiFlags, static_cast<TESSound*>(pSettingsContext));
        }
    }
}

void BSAudioManager::Update(bool abPlayGameWorldSounds) {
    if (!bPrecacheCompleted) {
        BSAudio::pAudioInstance->Precache("UIMenuMode", static_cast<BSGameSound::SoundFlags>(0));
        BSAudio::pAudioInstance->Precache("UIMenuFocus", BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE);
        BSAudio::pAudioInstance->Precache(
            "UIMenuCancel",
            static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE)
        );
        BSAudio::pAudioInstance->Precache("UIMenuTab", BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE);
        BSAudio::pAudioInstance->Precache(
            "UIMenuOK",
            static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE)
        );
        BSAudio::pAudioInstance->Precache("UIMenuPrevNext", BSGameSound::SOUND_FLAG_IS_2D);
        BSAudio::pAudioInstance->Precache(
            "WPNRifleAssaultFire3D",
            static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT)
        );
        BSAudio::pAudioInstance->Precache(
            "FXBulletImpactConc",
            static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT)
        );
        BSAudio::pAudioInstance->Precache(
            "UILoadScreenMicroFiche01",
            static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT)
        );
        BSAudio::pAudioInstance->Precache(
            "UILoadScreenInitial",
            static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT)
        );
        bPrecacheCompleted = true;
    }
    SynchVoiceStates();
    if (!BSAudio::pAudioInstance->bMultiThread) {
        QInstance().Process();
    }
}

void BSAudioManager::SynchVoiceStates() {
    int32_t ikey;
    BSSoundInfo* ptmp;
    EnterCriticalSection(&StateCritSection);
    if (StateUpdateMap.m_kAllocator.m_uiCount) {
        NiTMapItem<int32_t, BSSoundInfo*>* it;
        for (uint32_t i = 0; i < StateUpdateMap.m_uiHashSize; ++i) {
            if (StateUpdateMap.m_ppkHashTable[i]) {
                it = StateUpdateMap.m_ppkHashTable[i];
                goto LABEL_8;
            }
        }
        it = nullptr;
    LABEL_8:;
        BSSoundInfo* pdel = nullptr;
        while (it) {
            StateUpdateMap.GetNext(it, ikey, ptmp);
            while (SoundStateMap.GetAt(ikey, pdel)) {
                while (SoundStateMap.RemoveAt(ikey)) {
                    ;
                }
                BSSoundInfo* Block = pdel;
                if (pdel) {
                    pdel->~BSSoundInfo();
                    MemoryManager_Singleton.Deallocate(Block);
                }
            }
            if (ptmp->iID) {
                SoundStateMap.SetAt(ikey, ptmp);
            }
            else {
                BSSoundInfo* apMem = ptmp;
                if (ptmp) {
                    ptmp->~BSSoundInfo();
                    MemoryManager_Singleton.Deallocate(apMem);
                }
            }
        }
        StateUpdateMap.RemoveAll_0();
    }
    LeaveCriticalSection(&StateCritSection);
}

void BSAudioManager::SetConditions(float afCurrentGamehour, uint32_t aiWeatherConditions) {
    iWeatherFlags = aiWeatherConditions;
    fCurrentGamehour = afCurrentGamehour;
}

void BSAudioManager::UnMuteAllOfType(uint32_t aiTargetType, uint32_t aiFadeTimeMS) {
    // The engine carries the fade time in the message's pointer field.
    ComposeMessage(
        MUNMUTE_ALL_OFTYPE, 0, aiTargetType, reinterpret_cast<void*>(static_cast<uintptr_t>(aiFadeTimeMS)),
        NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

void BSAudioManager::PauseAllOfType(uint32_t aiType, bool abExactMatch) {
    if (abExactMatch) {
        ComposeMessage(MPAUSE_ALL_OFTYPE_EXACT, 0, aiType, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    }
    else {
        ComposeMessage(MPAUSE_ALL_OFTYPE, 0, aiType, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    }
}

void BSAudioManager::UnPauseAllOfType(uint32_t aiType, bool abExactMatch) {
    if (abExactMatch) {
        ComposeMessage(MUNPAUSE_ALL_OFTYPE_EXACT, 0, aiType, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    }
    else {
        ComposeMessage(MUNPAUSE_ALL_OFTYPE, 0, aiType, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    }
}

void BSAudioManager::SetListenerUnderwater(bool abIsUnderwater) {
    if (abIsUnderwater != bListenerUnderwater) {
        bListenerUnderwater = abIsUnderwater;
        ComposeMessage(MSET_UNDERWATER, 0, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    }
}

void BSAudioManager::PositionListener(NiPoint3 aNewPosition) {
    ComposeMessage(MSET_LISTENER_POSITION, 0, 0, nullptr, NiPointer<NiAVObject>{}, aNewPosition);
}

void BSAudioManager::KillAllOfType(uint32_t aiTargetType, uint32_t aiFadeTimeMS) {
    // The engine carries the fade time in the message's pointer field.
    ComposeMessage(
        MKILL_ALL_OFTYPE, 0, static_cast<int32_t>(aiTargetType),
        reinterpret_cast<void*>(static_cast<uintptr_t>(aiFadeTimeMS)), NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

void BSAudioManager::MuteAllOfType(uint32_t aiTargetType, uint32_t aiFadeTimeMS, uint32_t aiMillibels) {
    // The message's ID field carries the volume and its pointer field the fade time.
    ComposeMessage(
        MMUTE_ALL_OFTYPE, aiMillibels, static_cast<int32_t>(aiTargetType),
        reinterpret_cast<void*>(static_cast<uintptr_t>(aiFadeTimeMS)), NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

// The map's key is an int; the sound ID converts implicitly.
float BSAudioManager::GetVolume(uint32_t aiSoundID) {
    BSSoundInfo* si = nullptr;
    float fretValue = 0.0f;
    EnterCriticalSection(&StateCritSection);
    SoundStateMap.GetAt(aiSoundID, si);
    LeaveCriticalSection(&StateCritSection);
    if (si) {
        return si->fVolume;
    }
    return fretValue;
}

int32_t BSAudioManager::GetDuration(uint32_t aiSoundID) {
    BSSoundInfo* val[2];
    val[1] = nullptr;
    val[0] = nullptr;
    EnterCriticalSection(&StateCritSection);
    SoundStateMap.GetAt(aiSoundID, *val);
    LeaveCriticalSection(&StateCritSection);
    if (val[0]) {
        return val[0]->uiDuration;
    }

    return 0;
}

void BSAudioManager::FadeInPlay(uint32_t aiID, uint32_t aiMilliseconds) {
    if (!IsPlaying(aiID)) {
        ComposeMessage(MMUTE, aiID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
        ComposeMessage(MPLAY, aiID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
        // The engine carries the fade time in the message's pointer field.
        ComposeMessage(
            MFADE_IN, aiID, 0, reinterpret_cast<void*>(static_cast<uintptr_t>(aiMilliseconds)), NiPointer<NiAVObject>{},
            NiPoint3_ZERO
        );
    }
}

void BSAudioManager::SetTimeConstraints(uint32_t aiSoundID, uint8_t aiStart, uint8_t aiStop) {
    // The engine carries the stop value in the message's pointer field.
    ComposeMessage(
        MSET_TIME_CONSTRAINTS, aiSoundID, aiStart, reinterpret_cast<void*>(static_cast<uintptr_t>(aiStop)),
        NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

void BSAudioManager::SetPriority(uint32_t aiSoundID, uint32_t aiPriority) {
    ComposeMessage(
        MSET_PRIORITY, aiSoundID, static_cast<int32_t>(aiPriority), nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

void BSAudioManager::ClearSynchStartTime(uint32_t aiSoundID) {
    ComposeMessage(MCLEAR_SYNCH_START, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
}

// Passes the start time both as the message's data and as its pointer data (an integer, not an address).
void BSAudioManager::SetSynchStartTime(uint32_t aiSoundID, uint32_t aiStartTime) {
    ComposeMessage(
        MSET_SYNCH_START, aiSoundID, static_cast<int32_t>(aiStartTime),
        reinterpret_cast<void*>(static_cast<uintptr_t>(aiStartTime)), NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

void BSAudioManager::SetAtListener(uint32_t aiSoundID, bool abSetAtListener) {
    ComposeMessage(MSET_AT_LISTENER, aiSoundID, abSetAtListener, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
}

void BSAudioManager::Seek(uint32_t aiSoundID, uint32_t aiMilliseconds) {
    ComposeMessage(
        MSEEK, aiSoundID, static_cast<int32_t>(aiMilliseconds), nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
}

void BSAudioManager::VolumeChanged() {
    ComposeMessage(MVOLUME_LEVELS_CHANGED, 0, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
}

bool BSAudioManager::SetVolume(uint32_t aiSoundID, float afNewVolume) {
    ComposeMessage(
        MSET_VOLUME, aiSoundID, static_cast<int32_t>(afNewVolume * 1000.0), nullptr, NiPointer<NiAVObject>{},
        NiPoint3_ZERO
    );
    return true;
}

void BSAudioManager::SetReverbAttenuation(uint32_t aiSoundID, uint16_t aiReverbAtt) {
    ComposeMessage(MSET_REVERB_ATTENUATION, aiSoundID, aiReverbAtt, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
}

// Clamps each curve point to 100, queues an MSET_ATTENUATION_CURVE message built in a local (copied into the free
// message through Assign) and writes the five points into the queued message's curve. The free message is not tested
// against null, as the game does.
void BSAudioManager::SetAttenuationCurve(
    uint32_t aiSoundID,
    uint16_t aiPt1,
    uint16_t aiPt2,
    uint16_t aiPt3,
    uint16_t aiPt4,
    uint16_t aiPt5
) {
    uint16_t aiCurve[5];
    aiCurve[0] = aiPt1 > 100 ? 100 : aiPt1;
    aiCurve[1] = aiPt2 > 100 ? 100 : aiPt2;
    aiCurve[2] = aiPt3 > 100 ? 100 : aiPt3;
    aiCurve[3] = aiPt4 > 100 ? 100 : aiPt4;
    aiCurve[4] = aiPt5 > 100 ? 100 : aiPt5;
    if (BSAudio::pAudioInstance->bInitialized) {
        MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
        GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
        EnterCriticalSection(&MessageCritSection);
        BSSoundMessage* pMsg = GetFreeMessage();
        {
            // The message constructor's smart-pointer and vector arguments are copies.
            XFNV_INLINED_LOCAL(BSSoundMessage, kMsg, std::nullopt, std::nullopt);
            {
                NiPointer<NiAVObject> spData;
                NiPoint3 kVector = NiPoint3_ZERO;
                kMsg.spData.m_pObject = nullptr;
                kMsg.msg = MSET_ATTENUATION_CURVE;
                kMsg.iID = aiSoundID;
                kMsg.iData = 0;
                kMsg.pData = nullptr;
                kMsg.spData.operator=(spData);
                kMsg.vector3[0] = kVector.x;
                kMsg.vector3[1] = kVector.y;
                kMsg.vector3[2] = kVector.z;
            }
            BSSoundMessage::Assign(pMsg, kMsg);
            kMsg.~BSSoundMessage();
        }
        pMsg->curve[0] = aiCurve[0];
        pMsg->curve[1] = aiCurve[1];
        pMsg->curve[2] = aiCurve[2];
        pMsg->curve[3] = aiCurve[3];
        pMsg->curve[4] = aiCurve[4];
        Messages[iMessageListIndex].AddTail_0(&pMsg);
        LeaveCriticalSection(&MessageCritSection);
        GetThreadTLSData()->eMemContext = eMemContext;
    }
}

bool BSAudioManager::SetFrequencyVariance(uint32_t aiSoundID, uint8_t aiFreqVariance) {
    ComposeMessage(MSET_FREQUENCY_VARIANCE, aiSoundID, aiFreqVariance, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    return false;
}

bool BSAudioManager::SetModFrequency(uint32_t aiSoundID, float afNewFrequency) {
    ComposeMessage(
        MSET_MOD_FREQUENCY, aiSoundID, static_cast<int32_t>(afNewFrequency * 1000.0), nullptr, NiPointer<NiAVObject>{},
        NiPoint3_ZERO
    );
    return false;
}

bool BSAudioManager::SetFrequency(uint32_t aiSoundID, float afNewFrequency) {
    ComposeMessage(
        MSET_FREQUENCY, aiSoundID, static_cast<int32_t>(afNewFrequency * 1000.0), nullptr, NiPointer<NiAVObject>{},
        NiPoint3_ZERO
    );
    return false;
}

// Caps the attenuation at 10000, zero-extends the clamped 16-bit value into the message's data word and returns false.
bool BSAudioManager::SetStaticAttenuation(uint32_t aiSoundID, uint16_t aiStaticAttenutation) {
    uint16_t v5;
    if (aiStaticAttenutation <= 0x2710u) {
        v5 = aiStaticAttenutation;
    }
    else {
        v5 = 0x2710;
    }
    ComposeMessage(
        MSET_STATIC_ATTENUATION, aiSoundID, static_cast<int32_t>(v5), nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO
    );
    return false;
}

bool BSAudioManager::Pause(uint32_t aiSoundID) {
    ComposeMessage(MPAUSE, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    return false;
}

// Plays the sound aiSoundID aiDelay milliseconds from now (at most 10 seconds), unless it is playing already.
bool BSAudioManager::PlayAfter(uint32_t aiSoundID, uint32_t aiDelay, int32_t aiPlayFlags) {
    if (IsPlaying(aiSoundID)) {
        return true;
    }
    if (aiDelay > 10000) {
        aiDelay = 10000;
    }
    uint32_t uiTickCount = GetTickCount();
    ComposeMessage(
        MPLAY_AFTER, aiSoundID, static_cast<int32_t>(uiTickCount + aiDelay), nullptr, NiPointer<NiAVObject>{},
        NiPoint3_ZERO
    );
    return true;
}

// Plays the sound aiSoundID unless it is playing already; abLoopPlayback first forces it to loop.
bool BSAudioManager::Play(uint32_t aiSoundID, bool abLoopPlayback) {
    if (IsPlaying(aiSoundID)) {
        return true;
    }
    if (abLoopPlayback) {
        ComposeMessage(MFORCE_LOOP, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    }
    ComposeMessage(MPLAY, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    return true;
}

bool BSAudioManager::Stop(uint32_t aiSoundID) {
    ComposeMessage(MSTOP, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    return false;
}

bool BSAudioManager::Release(uint32_t aiSoundID) {
    ComposeMessage(MDELETE_SOUND, aiSoundID, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    return true;
}

void BSAudioManager::ClearSecondaryVolumes() {
    fVolumes[6] = (float)1.0;
    fVolumes[7] = (float)1.0;
    fVolumes[8] = (float)1.0;
    fVolumes[9] = (float)1.0;
    fVolumes[10] = (float)1.0;
    fVolumes[11] = (float)1.0;
    VolumeChanged();
}

// Takes a free message under the message critical section, fills it (the smart pointer through a temporary copy) and
// queues it on the current message list.
void BSAudioManager::ComposeMessage(
    SOUND_MSG aMsg,
    uint32_t aiID,
    intptr_t aiData,
    void* apPointerData,
    NiPointer<NiAVObject> aspSmartPointerData,
    NiPoint3 aVector
) {
    if (BSAudio::pAudioInstance->bInitialized) {
        MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
        GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
        EnterCriticalSection(&MessageCritSection);
        BSSoundMessage* pMsg = GetFreeMessage();
        if (pMsg) {
            {
                NiPointer<NiAVObject> spData = aspSmartPointerData;
                pMsg->msg = aMsg;
                pMsg->iID = aiID;
                pMsg->iData = aiData;
                pMsg->pData = apPointerData;
                pMsg->spData.operator=(spData);
                pMsg->vector3[0] = aVector.x;
                pMsg->vector3[1] = aVector.y;
                pMsg->vector3[2] = aVector.z;
            }
            Messages[iMessageListIndex].AddTail_0(&pMsg);
        }
        LeaveCriticalSection(&MessageCritSection);
        GetThreadTLSData()->eMemContext = eMemContext;
    }
}

// Queues the stop of the sounds moving with apObj; the message carries the flags in its pointer field.
void BSAudioManager::StopMovingSounds(NiAVObject* apObj, float afFadeSeconds, uint32_t aiFlags) {
    ComposeMessage(
        MSTOP_MOVING_SOUNDS, 0, static_cast<int32_t>(afFadeSeconds * 1000.0),
        reinterpret_cast<void*>(static_cast<uintptr_t>(aiFlags)), NiPointer<NiAVObject>(apObj), NiPoint3_ZERO
    );
}

bool BSAudioManager::SetBalance(uint32_t aiSoundID, int32_t aiNewBalance) {
    return false;
}

// Scales both distances by the double 1000.0, truncates each product to an int64 and passes the low half of each as one
// of the message's two payloads: the minimum travels in aiData, the maximum in apPointerData, which is untyped here and
// which the audio thread reads back as an integer.
void BSAudioManager::SetMinMax(uint32_t aiSoundID, float afMin, float afMax) {
    const int64_t iMaxScaled = static_cast<int64_t>(static_cast<double>(afMax) * 1000.0);
    const int64_t iMinScaled = static_cast<int64_t>(static_cast<double>(afMin) * 1000.0);
    ComposeMessage(
        MSET_MINMAX_DISTANCE, aiSoundID, static_cast<uint32_t>(iMinScaled),
        reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(iMaxScaled))), NiPointer<NiAVObject>{},
        NiPoint3_ZERO
    );
}

bool BSAudioManager::SetOrientation(uint32_t aiSoundID, float afX, float afY, float afZ) {
    return false;
}

BSSoundMessage* BSAudioManager::GetFreeMessage() {
    return (&FreeMessages)->Pop();
}

BSSoundMessage* BSAudioManager::GetFreeQueueMessage() {
    return (&FreeQueueMessages)->Pop();
}

bool BSAudioManager::IsPausedType(uint32_t aeFlags) {
    uint32_t i;
    for (i = 0; i < ((uint32_t)((int32_t)uiMutedTypesCount)); ++i) {
        if ((pMutedSoundTypes[i].usState == 1) &&
            ((((int32_t)pMutedSoundTypes[i].uiTypes) & aeFlags) ==
             ((uint32_t)((int32_t)pMutedSoundTypes[i].uiTypes)))) {
            return true;
        }
        if ((pMutedSoundTypes[i].usState == 2) && ((((int32_t)pMutedSoundTypes[i].uiTypes) & aeFlags) != 0)) {
            return true;
        }
    }
    return false;
}

bool BSAudioManager::ClearPausedType(uint32_t auiTypes, bool abExact) {
    uint32_t i;
    for (i = 0; i < ((uint32_t)((int32_t)uiMutedTypesCount)); ++i) {
        if ((auiTypes == ((uint32_t)((int32_t)pMutedSoundTypes[i].uiTypes))) &&
            (((!abExact) && (pMutedSoundTypes[i].usState == 2)) || (abExact && (pMutedSoundTypes[i].usState == 1)))) {
            pMutedSoundTypes[i].usState = (uint16_t)0;
            pMutedSoundTypes[i].uiTypes = (uint32_t)0;
            return true;
        }
    }
    return false;
}

void BSAudioManager::PrecacheSound(char* a1, uint32_t a2, TESSound* a3) {
    uint32_t ibegin;
    uint32_t imodFlags;
    uint32_t iend;
    XFNV_INLINED_LOCAL(BSSoundHandle, v16, std::nullopt, std::nullopt);
    if ((a1) && (BSAudio::pAudioInstance->bAudioEnabled)) {
        char pcDest[268];
        uint32_t ithisSoundID = uiSoundCacheIndex++;
        // MAYBE-INLINED 0xADD784: a write opening BSSoundHandle's uses: may be its inlined constructor
        v16.iSoundID = ithisSoundID;
        // MAYBE-INLINED 0xADD787: a write opening BSSoundHandle's uses: may be its inlined constructor
        v16.bAssumeSuccess = false;
        // MAYBE-INLINED 0xADD78B: a write opening BSSoundHandle's uses: may be its inlined constructor
        v16.eState = BSSoundHandle::ASSUMED_STATE_INITIALIZED;
        strcpy_s(pcDest, 260, a1);
        BSAudio::pAudioInstance->FixSoundPath(pcDest);
        uint32_t itestHash = BSGameSound::GetNameHash(pcDest);
        if (IsInCache(itestHash, true)) {}
        else {
            BSGameSound* pNewSound__pfnLoopPointsCallback = BSAudio::pAudioInstance->CreateNewGameSound(a1);
            pNewSound__pfnLoopPointsCallback->iSoundID = ithisSoundID;
            pNewSound__pfnLoopPointsCallback->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(a2);
            pNewSound__pfnLoopPointsCallback->SetupPath(false);
            uint32_t (*pfnSettingsCallback)(BSSoundHandle*, void*, uint32_t*) =
                BSAudio::pAudioInstance->pfnSettingsCallback;
            imodFlags = 0;
            if (pfnSettingsCallback) {
                pfnSettingsCallback(&v16, a3, &imodFlags);
            }
            BSAudioLoopPointCallback v15 = BSAudio::pAudioInstance->GetLoopPointCallback();
            ibegin = 0;
            iend = 0;
            if ((v15) && (v15(a3, &ibegin, &iend))) {
                uint32_t v5 = iend;
                pNewSound__pfnLoopPointsCallback->iLoopStart = ibegin;
                pNewSound__pfnLoopPointsCallback->iLoopEnd = v5;
            }

            pNewSound__pfnLoopPointsCallback->iModifierFlags = imodFlags;
            Precache(pNewSound__pfnLoopPointsCallback);
        }
    }
}

bool BSAudioManager::IsEnableAudioCache() {
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_bEnableAudioCache_Audio.uValue;
    return p_uValue->b;
}

void BSAudioManager::SetCacheEnabled(bool abEnableCache) {
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_bEnableAudioCache_Audio.uValue;
    p_uValue->b = abEnableCache;
    if (!abEnableCache) {
        AddToCache(nullptr);
    }
}

void BSAudioManager::SetAudioCacheSizeInK(uint32_t auiNewSize) {
    Setting::Info* p_uValue;
    if (!auiNewSize) {
        SetCacheEnabled(false);
    }
    if (auiNewSize >= 4096) {
        Warning("AUDIO: Attempted to set audio cache size to %i. Cache size must be < 4MB", auiNewSize);
    }
    else {
        // The setting's address is never null.
        p_uValue = &gs_iAudioCacheSize_Audio.uValue;
        p_uValue->uint = auiNewSize;
    }
}

bool BSAudioManager::IsValid(uint32_t aiSoundID) {
    uint8_t v4 = 0;
    BSSoundInfo* v3 = nullptr;
    (&SoundStateMap)->GetAt(aiSoundID, v3);
    if ((v3) && (v3->iID == aiSoundID)) {
        return true;
    }

    return (bool)v4;
}

uint32_t BSAudioManager::AudioCacheSize() {
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_iAudioCacheSize_Audio.uValue;
    return p_uValue->uint;
}

float BSAudioManager::GetCorrectedVolume(float afBaseVolume, uint32_t aiTypeFlags) {
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_fMasterVolumeMult.uValue;
    float v8 = ((fVolumes[0] * fVolumes[6]) * p_uValue->f) * afBaseVolume;
    if ((((int32_t)aiTypeFlags) & 0x800000u) != 0) {
        return ((v8 * fVolumes[4]) * fVolumes[10]);
    }

    if ((((int32_t)aiTypeFlags) & 8u) != 0) {
        return ((fVolumes[1] * fVolumes[7]) * v8);
    }

    if ((((int32_t)aiTypeFlags) & 4u) != 0) {
        return ((fVolumes[2] * fVolumes[8]) * v8);
    }

    if ((((int32_t)aiTypeFlags) & 0x800u) != 0) {
        return ((fVolumes[4] * fVolumes[10]) * v8);
    }

    if ((((int32_t)aiTypeFlags) & 0x100000u) != 0) {
        return ((fVolumes[5] * fVolumes[11]) * v8);
    }

    return ((fVolumes[3] * fVolumes[9]) * v8);
}

bool BSAudioManager::IsPlaying(uint32_t aiSoundID) {
    uint8_t bPlaying;
    BSSoundInfo* pInfo = nullptr;
    EnterCriticalSection(&StateCritSection);
    bool vCondition1;
    vCondition1 = (&SoundStateMap)->GetAt(aiSoundID, pInfo);
    if (vCondition1) {
        // bIsPlaying is read as a raw byte and the result normalised to 0 or 1; a bool load would let clang return the
        // byte itself.
        bPlaying = (uint8_t)(pInfo && static_cast<uint8_t>(pInfo->bIsPlaying) != 0);
    }
    else {
        bPlaying = 0;
    }
    LeaveCriticalSection(&StateCritSection);
    return (bool)bPlaying;
}

bool BSAudioManager::IsInCache(uint32_t aiSoundHash, bool abFuzzy) {
    if (!aiSoundHash) {
        return false;
    }
    EnterCriticalSection(&CacheCritSection);
    bool bResult = false;
    for (NiTListItem<BSGameSound*>* i = CachedSoundList.m_pkHead; i; i = i->m_pkNext) {
        BSGameSound* pSound = i->m_element;
        if (pSound) {
            char v4;
            if (aiSoundHash == pSound->iSoundHash ||
                (abFuzzy && (aiSoundHash != pSound->iDirectoryHash || !pSound->iDirectoryHash ? v4 = 0 : v4 = 1, v4))) {
                bResult = true;
                break;
            }
        }
    }
    LeaveCriticalSection(&CacheCritSection);
    return bResult;
}

void BSAudioManager::RemoveLoadTask(AudioLoadTask* a1) {
    EnterCriticalSection(&CacheLoadTaskCritSection);
    CacheLoadTaskList.Remove(a1);
    LeaveCriticalSection(&CacheLoadTaskCritSection);
}

void BSAudioManager::Process() {
    BSAudio::pAudioInstance->SynchTimer.Update();
    if (TryEnterCriticalSection(&ProcessingCritSection)) {
        BSAudioManager* v1 = &BSAudioManager::QInstance();
        v1->ProcessMessages();
        BSAudioManager* v2 = &BSAudioManager::QInstance();
        v2->ProcessSoundUpdates();
        LeaveCriticalSection(&ProcessingCritSection);
    }
}

uint32_t BSAudioManager::GetAudioCacheSize() {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    uint32_t result = v1->AudioCacheSize();
    GetThreadTLSData()->eMemContext = eMemContext;
    return result;
}

void BSAudioManager::Initialize() {
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &BSAudio::bEnableAudio.uValue;
    if (p_uValue->b) {
        BSAudioManagerThread* v1;
        iWeatherFlags = 0;
        uiMutedTypesCount = 8;
        pMutedSoundTypes = static_cast<BSAudioManager::MutedSoundType*>(MemoryManager_Singleton.Allocate(
            uiMutedTypesCount <= SIZE_MAX / sizeof(BSAudioManager::MutedSoundType)
                ? sizeof(BSAudioManager::MutedSoundType) * uiMutedTypesCount
                : SIZE_MAX
        ));
        std::memset(pMutedSoundTypes, 0, sizeof(BSAudioManager::MutedSoundType) * uiMutedTypesCount);
        InitializeAudio();
        for (uint32_t i = 0; i < 256; ++i) {
            BSSoundMessage* v3;
            BSSoundMessage* v8 = static_cast<BSSoundMessage*>(MemoryManager_Singleton.Allocate(sizeof(BSSoundMessage)));
            if (v8) {
                v3 = ::new (v8) BSSoundMessage;
            }
            else {
                v3 = nullptr;
            }
            FreeMessages.Push(v3);
        }
        FreeMessages.bBottomless = true;
        for (uint32_t j = 0; j < 256; ++j) {
            BSSoundMessage* v2;
            BSSoundMessage* v7 = static_cast<BSSoundMessage*>(MemoryManager_Singleton.Allocate(sizeof(BSSoundMessage)));
            if (v7) {
                v2 = ::new (v7) BSSoundMessage;
            }
            else {
                v2 = nullptr;
            }
            FreeQueueMessages.Push(v2);
        }
        FreeQueueMessages.bBottomless = true;
        GarbageMessages.bBottomless = false;
        BSAudioManagerThread* apMemory =
            static_cast<BSAudioManagerThread*>(MemoryManager_Singleton.Allocate(sizeof(BSAudioManagerThread)));
        if (apMemory) {
            v1 = ::new (apMemory) BSAudioManagerThread;
        }
        else {
            v1 = nullptr;
        }
        pUpdateThread = v1;
    }
}

void BSAudioManager::InitializeAudio() {
    Setting::Info* v1;
    Setting::Info* v2;
    Setting::Info* v3;
    Setting::Info* v4;
    Setting::Info* v5;
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &gs_fDefaultMasterVolume_Audio.uValue;
    fVolumes[0u] = p_uValue->f;
    // The setting's address is never null.
    v5 = &gs_fDefaultFootVolume_Audio.uValue;
    fVolumes[1] = v5->f;
    // The setting's address is never null.
    v4 = &gs_fDefaultVoiceVolume_Audio.uValue;
    fVolumes[2] = v4->f;
    // The setting's address is never null.
    v3 = &gs_fDefaultEffectsVolume_Audio.uValue;
    fVolumes[3] = v3->f;
    // The setting's address is never null.
    v2 = &gs_fDefaultMusicVolume_Audio.uValue;
    fVolumes[4] = v2->f;
    // The setting's address is never null.
    v1 = &gs_fDefaultRadioVolume_Audio.uValue;
    fVolumes[5] = v1->f;
    fVolumes[6] = 1.0f;
    fVolumes[7] = 1.0f;
    fVolumes[8] = 1.0f;
    fVolumes[9] = 1.0f;
    fVolumes[10] = 1.0f;
    fVolumes[11] = 1.0f;
}

void BSAudioManager::SuspendAudioThread() {
    if (this->pUpdateThread) {
        this->pUpdateThread->Resume();
    }
    ComposeMessage(MSUSPEND_THREAD, 0, 0, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO);
    if (this->pUpdateThread) {
        BSAudioManagerThread* pUpdateThread = this->pUpdateThread;
        int32_t* lpAddend = &pUpdateThread->m_SuspendedSema.iCount;
        pUpdateThread->m_SuspendedSema.hSemaphore->Wait();
        InterlockedDecrementInt32(lpAddend);
    }
}

void BSAudioManager::SetVoiceState(uint32_t aiID, BSGameSound* apSound) {
    Setting::Info* p_uValue;
    BSSoundInfo::Data v9;
    BSSoundInfo::Data apSource;
    BSSoundInfo kNewInfo;
    BSSoundInfo newInfo;
    BSSoundInfo::Data poldDebugInfo;
    BSSoundInfo::Data pDebugInfo;
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    newInfo.~BSSoundInfo();
    ::new (&newInfo) BSSoundInfo;
    pDebugInfo.~Data();
    ::new (&pDebugInfo) BSSoundInfo::Data;
    poldDebugInfo.~Data();
    ::new (&poldDebugInfo) BSSoundInfo::Data;
    BSSoundInfo* v38 = nullptr;
    // The setting's address is never null.
    p_uValue = &g_bUseAudioDebugInformation_Audio.uValue;
    bool bUseAudioDebugInformation = p_uValue->b;
    if (apSound) {
        uint32_t uiDuration = apSound->iDurationInMS;
        BSGameSound::SoundFlags uiTypeFlags = apSound->iSoundTypeFlags;
        uint32_t iSoundID = apSound->iSoundID;
        float v21 = apSound->GetVolume();
        bool v22 = apSound->IsPlaying();
        kNewInfo.kDebugInfo.~Data();
        ::new (&kNewInfo.kDebugInfo) BSSoundInfo::Data;
        kNewInfo.iID = iSoundID;
        kNewInfo.fFrequency = 1.0f;
        kNewInfo.fVolume = v21;
        kNewInfo.uiSoundType = uiTypeFlags;
        kNewInfo.uiDuration = uiDuration;
        kNewInfo.bIsPlaying = v22;
        newInfo.Copy(&kNewInfo);
        if (bUseAudioDebugInformation) {
            std::memset(&pDebugInfo, 0, sizeof(pDebugInfo));
            if (pDebugInfo.pFilePath != pDebugInfo.cFilePath) {
                pDebugInfo.pFilePath = pDebugInfo.cFilePath;
            }
            // No null check is made here: the game tests the address &apSound->pFilename, which is never null.
            char* pcDest = pDebugInfo.pFilePath;
            strcpy_s(pDebugInfo.pFilePath, 260, apSound->pFilename);
            if (apSound->Is3DSound() || apSound->Is2DRadiusSound()) {
                float v4;
                float v5;
                float v6;
                float fDistanceToListener = apSound->fDistanceToListener;
                if (!(fDistanceToListener > 3.4028234663852886e+37)) {
                    v6 = apSound->fDistanceToListener;
                }
                else {
                    v6 = -1.0f;
                }
                pDebugInfo.kPosition.z = v6;
                pDebugInfo.kPosition.x = apSound->GetEmitterPosition().x;
                pDebugInfo.kPosition.y = apSound->GetEmitterPosition().y;
                float fMinDist = apSound->fMinDist;
                if (!(fMinDist > 3.4028234663852886e+37)) {
                    v5 = apSound->fMinDist;
                }
                else {
                    v5 = -1.0f;
                }
                pDebugInfo.fMinAttenuationDist = v5;
                float fMaxDist = apSound->fMaxDist;
                if (!(fMaxDist > 3.4028234663852886e+37)) {
                    v4 = apSound->fMaxDist;
                }
                else {
                    v4 = -1.0f;
                }
                pDebugInfo.fMaxAttenuationDist = v4;
            }
            else {
                pDebugInfo.kPosition.z = 0.0f;
                pDebugInfo.kPosition.x = 0.0f;
                pDebugInfo.kPosition.y = 0.0f;
                pDebugInfo.fMinAttenuationDist = 0.0f;
                pDebugInfo.fMaxAttenuationDist = 0.0f;
            }
            uint16_t iStaticAttenuation = apSound->iStaticAttenuation;
            pDebugInfo.uiStaticAttenuation = iStaticAttenuation;
            uint16_t iSystemAttenuation = apSound->iSystemAttenuation;
            pDebugInfo.uiDistanceAttenuation = iSystemAttenuation;
            uint16_t iDistanceAttenuation = apSound->iDistanceAttenuation;
            pDebugInfo.uiFaderAttenuation = iDistanceAttenuation;
        }
    }
    EnterCriticalSection(&StateCritSection);
    BSSoundInfo* poldInfo = nullptr;
    if (StateUpdateMap.GetAt(aiID, poldInfo)) {
        if (bUseAudioDebugInformation) {
            apSource = pDebugInfo;
            BSSoundInfo* v12 = poldInfo;
            poldInfo->kDebugInfo.SetDebugInfo(&apSource);
            apSource.Free();
        }
        BSSoundInfo* v10 = poldInfo;
        poldInfo->iID = newInfo.iID;
        v10->fFrequency = newInfo.fFrequency;
        v10->fVolume = newInfo.fVolume;
        v10->uiSoundType = newInfo.uiSoundType;
        v10->uiDuration = newInfo.uiDuration;
        v10->bIsPlaying = newInfo.bIsPlaying;
    }
    else {
        BSSoundInfo* pnewInfo;
        BSSoundInfo* v28 = static_cast<BSSoundInfo*>(MemoryManager_Singleton.Allocate(sizeof(BSSoundInfo)));
        if (v28) {
            pnewInfo = ::new (v28) BSSoundInfo(&newInfo);
        }
        else {
            pnewInfo = nullptr;
        }
        if (bUseAudioDebugInformation) {
            v9 = pDebugInfo;
            pnewInfo->kDebugInfo.SetDebugInfo(&v9);
            v9.Free();
        }
        StateUpdateMap.SetAt(aiID, pnewInfo);
    }
    LeaveCriticalSection(&StateCritSection);
    poldDebugInfo.Free();
    pDebugInfo.Free();
    GetThreadTLSData()->eMemContext = eMemContext;
}

void BSAudioManager::FreeMessage(BSSoundMessage* apMsgToFree) {
    NiPointer<NiAVObject> spAVObject;
    spAVObject.operator=(apMsgToFree->spData);
    if (spAVObject.m_pObject) {
        {
            NiPointer<NiAVObject> spAVObject_2;
            apMsgToFree->spData.operator=(spAVObject_2);
        }
        NiObjectGarbage::Add(spAVObject.m_pObject);
        spAVObject.operator=(static_cast<NiAVObject*>(nullptr));
    }
    {
        NiPointer<NiAVObject> ptr;
        NiPoint3 v11 = NiPoint3_ZERO;
        apMsgToFree->msg = MINVALID_MSG;
        apMsgToFree->iID = 0;
        apMsgToFree->iData = 0;
        apMsgToFree->pData = nullptr;
        apMsgToFree->spData.operator=(ptr);
        apMsgToFree->vector3[0] = v11.x;
        apMsgToFree->vector3[1] = v11.y;
        apMsgToFree->vector3[2] = v11.z;
    }
    FreeMessages.Push(apMsgToFree);
}

void BSAudioManager::FreeQueueMessage(BSSoundMessage* apMsgToFree) {
    NiPointer<NiAVObject> spAVObject;
    spAVObject.operator=(apMsgToFree->spData);
    if (spAVObject.m_pObject) {
        {
            NiPointer<NiAVObject> spAVObject_2;
            apMsgToFree->spData.operator=(spAVObject_2);
        }
        NiObjectGarbage::Add(spAVObject.m_pObject);
        spAVObject.operator=(static_cast<NiAVObject*>(nullptr));
    }
    {
        NiPointer<NiAVObject> ptr;
        NiPoint3 v11 = NiPoint3_ZERO;
        apMsgToFree->msg = MINVALID_MSG;
        apMsgToFree->iID = 0;
        apMsgToFree->iData = 0;
        apMsgToFree->pData = nullptr;
        apMsgToFree->spData.operator=(ptr);
        apMsgToFree->vector3[0] = v11.x;
        apMsgToFree->vector3[1] = v11.y;
        apMsgToFree->vector3[2] = v11.z;
    }
    FreeQueueMessages.Push(apMsgToFree);
}

// ComposeMessage releases the NiPointer argument.
void BSAudioManager::SetObjectToFollow(uint32_t aiID, NiAVObject* apObj) {
    ComposeMessage(MREGISTER_MOVING_SOUND, aiID, 0, nullptr, NiPointer<NiAVObject>(apObj), NiPoint3_ZERO);
}

// aiFollowUpAction is taken but never read, as the game does.
void BSAudioManager::FadeIn(uint32_t aiID, uint32_t aiMilliseconds, uint32_t aiFollowUpAction) {
    // The engine carries the fade time in the message's pointer field.
    ComposeMessage(
        MFADE_IN, aiID, 0, reinterpret_cast<void*>(static_cast<uintptr_t>(aiMilliseconds)), NiPointer<NiAVObject>{},
        NiPoint3_ZERO
    );
}

// The cache list's RemoveHead, AddHead and RemoveTail are declared as the list's own specializations. The setting's
// whole byte is tested, so any nonzero byte is true; bool would assume 0/1.
void BSAudioManager::AddToCache(BSGameSound* apToCache) {
    Setting::Info* uiCacheSize;
    Setting::Info* p_uValue;
    Setting::Info* bEnableAudioCache;
    BSGameSound* pnewCopy;
    BSGameSound* pSoundCopy;
    pSoundCopy = nullptr;
    EnterCriticalSection(&CacheCritSection);
    int32_t count = CachedSoundList.m_uiCount;
    // The setting's address is never null.
    bEnableAudioCache = &gs_bEnableAudioCache_Audio.uValue;
    if (!static_cast<uint8_t>(bEnableAudioCache->b) && count > 0) {
        while (CachedSoundList.m_uiCount) {
            pSoundCopy = CachedSoundList.RemoveHead();
            if (pSoundCopy) {
                delete pSoundCopy;
            }
        }
    LABEL_25:;
        LeaveCriticalSection(&CacheCritSection);
        return;
    }
    // The setting's address is never null.
    p_uValue = &gs_bEnableAudioCache_Audio.uValue;
    if (!static_cast<uint8_t>(p_uValue->b)) {
        goto LABEL_25;
    }
    if (!apToCache) {
        goto LABEL_25;
    }
    uint32_t uiSoundSize = apToCache->iSoundSize;
    // The setting's address is never null.
    uiCacheSize = &gs_iMaxSizeForCachedSound.uValue;
    if (uiSoundSize > uiCacheSize->uint << 10 ||
        (apToCache->iSoundTypeFlags & BSGameSound::SOUND_FLAG_DONT_CACHE) != 0) {
        goto LABEL_25;
    }
    for (NiTListItem<BSGameSound*>* i = CachedSoundList.m_pkHead; i; i = i->m_pkNext) {
        pSoundCopy = i->m_element;
        if (apToCache->iSoundHash == pSoundCopy->iSoundHash) {
            goto LABEL_25;
        }
    }
    pnewCopy = BSAudio::pAudioInstance->CreateNewGameSound(nullptr);
    pnewCopy->iSoundTypeFlags = apToCache->iSoundTypeFlags;
    pnewCopy->Copy(apToCache, false);
    pnewCopy->iSoundTypeFlags =
        static_cast<BSGameSound::SoundFlags>(apToCache->iSoundTypeFlags | BSGameSound::SOUND_FLAG_CACHED);
    CachedSoundList.AddHead(pnewCopy);
    if ((apToCache->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT29) == 0) {
        iCurrentCachSize += pnewCopy->iSoundSize;
    }
    pSoundCopy = nullptr;
    for (int32_t iattempts = 0;; ++iattempts) {
        Setting::Info* v2;
        // The setting's address is never null.
        v2 = &gs_iAudioCacheSize_Audio.uValue;
        if (iCurrentCachSize <= v2->uint << 10 || !CachedSoundList.m_uiCount) {
            break;
        }
        pSoundCopy = CachedSoundList.RemoveTail();
        if (!pSoundCopy ||
            (((pSoundCopy->iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE) != 0 ||
              (pSoundCopy->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT29) != 0) &&
             iattempts <= 30)) {
            if (pSoundCopy) {
                CachedSoundList.AddHead(pSoundCopy);
            }
        }
        else {
            iCurrentCachSize -= pSoundCopy->iSoundSize;
            if (pSoundCopy) {
                delete pSoundCopy;
            }
        }
    }
    LeaveCriticalSection(&CacheCritSection);
}

BSGameSound* BSAudioManager::GetFromCache(uint32_t aiSoundHash, uint32_t aiDirectoryHash, uint32_t aiFlagsToApply) {
    BSGameSound* pSoundCopy;
    NiTListItem<BSGameSound*>* iter;
    BSGameSound* pcloseMatch;
    bool brandomize = false;
    if (aiDirectoryHash) {
        brandomize = aiSoundHash == 0;
    }
    EnterCriticalSection(&CacheCritSection);
    iter = CachedSoundList.m_pkHead;
    NiTListItem<BSGameSound*>* lastMatch = nullptr;
    NiTListItem<BSGameSound*>* altMatch = nullptr;
    BSGameSound* pretValue = nullptr;
    pcloseMatch = nullptr;
    BSGameSound* pcloseMatchAlternate = nullptr;
    while (iter) {
        pSoundCopy = iter->m_element;
        if (aiSoundHash == pSoundCopy->iSoundHash) {
            pretValue = BSAudio::pAudioInstance->CreateNewGameSound(nullptr);
            pretValue->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(aiFlagsToApply);
            pretValue->Copy(pSoundCopy, true);
            pretValue->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(aiFlagsToApply);
            CachedSoundList.RemovePos(iter);
            if ((pSoundCopy->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT29) != 0) {
                if (pSoundCopy) {
                    delete pSoundCopy;
                }
            }
            else {
                CachedSoundList.AddHead(pSoundCopy);
            }
            LeaveCriticalSection(&CacheCritSection);
            return pretValue;
        }
        if (aiDirectoryHash == pSoundCopy->iDirectoryHash && pSoundCopy->iDirectoryHash) {
            if ((pcloseMatch) && (brandomize && !(rt::rand() % 3))) {
                pcloseMatchAlternate = pcloseMatch;
                altMatch = lastMatch;
            }

            pcloseMatch = pSoundCopy;
            lastMatch = iter;
        }
        iter = iter->m_pkNext;
    }
    if ((brandomize) && (pcloseMatchAlternate) && (pcloseMatch) && (!(rt::rand() % 2))) {
        pcloseMatch = pcloseMatchAlternate;
        lastMatch = altMatch;
    }

    if (pcloseMatch) {
        CachedSoundList.RemovePos(lastMatch);
        CachedSoundList.AddHead(pcloseMatch);
        pretValue = BSAudio::pAudioInstance->CreateNewGameSound(nullptr);
        pretValue->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(aiFlagsToApply);
        pretValue->Copy(pcloseMatch, true);
        pretValue->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(aiFlagsToApply);
    }
    LeaveCriticalSection(&CacheCritSection);
    return pretValue;
}

void BSAudioManager::Precache(BSGameSound* apToCache) {
    AudioLoadTask* pTask_2;
    if (!BSAudio::pAudioInstance->bAudioEnabled ||
        (!apToCache->pFilename[0u] ||
         ((apToCache->iSoundTypeFlags & BSGameSound::SOUND_FLAG_DONT_CACHE) != 0 ||
          IsInCache(apToCache->iSoundHash, false)))) {
        if (apToCache) {
            goto LABEL_13;
        }
    }
    else {
        NiTListItem<AudioLoadTask*>* m_pkHead;
        char v7;
        EnterCriticalSection(&CacheLoadTaskCritSection);
        m_pkHead = CacheLoadTaskList.m_pkHead;
        v7 = 0;
        while (m_pkHead) {
            AudioLoadTask* m_element = m_pkHead->m_element;
            if (m_element->pSound->iSoundHash == apToCache->iSoundHash &&
                m_element->pSound->iDirectoryHash == apToCache->iDirectoryHash) {
                v7 = 1;
                break;
            }
            m_pkHead = m_pkHead->m_pkNext;
        }
        if (v7) {
            LeaveCriticalSection(&CacheLoadTaskCritSection);
            if (apToCache) {
            LABEL_13:;
                delete apToCache;
            }
        }
        else {
            AudioLoadTask* pTask;
            apToCache->iSoundTypeFlags =
                static_cast<BSGameSound::SoundFlags>(apToCache->iSoundTypeFlags | BSGameSound::SOUND_FLAG_CACHED);
            AudioLoadTask* apMemory =
                static_cast<AudioLoadTask*>(MemoryManager_Singleton.Allocate(sizeof(AudioLoadTask)));
            if (apMemory) {
                pTask = ::new (apMemory) AudioLoadTask(apToCache);
            }
            else {
                pTask = nullptr;
            }
            pTask_2 = pTask;
            apToCache->iStateFlags |= BSGameSound::STATE_OPENING;
            pIOManager->AddTask(pTask_2);
            CacheLoadTaskList.AddTail(pTask_2);
            LeaveCriticalSection(&CacheLoadTaskCritSection);
        }
    }
}

void BSAudioManager::ProcessMessages() {
    SoundMessageList* pcurrentMessages = &Messages[iMessageProcessingListIndex];
    // Tests the list's address against null.
    if (pcurrentMessages) {
        while (pcurrentMessages->uiCount && (pcurrentMessages->pHead && pcurrentMessages->pTail)) {
            BSSoundMessage* pMsg = pcurrentMessages->RemoveHead();
            if (ProcessMessage(pMsg)) {
                return;
            }
            GarbageMessages.Push(pMsg);
        }
        if ((pcurrentMessages->uiCount) && (!pcurrentMessages->pHead || !pcurrentMessages->pTail)) {
            pcurrentMessages->RemoveAll();
        }

        EnterCriticalSection(&MessageCritSection);
        iMessageListIndex = (iMessageListIndex + 1) % 2;
        iMessageProcessingListIndex = (iMessageListIndex + 1) % 2;
        for (BSSoundMessage* pMsg_2 = GarbageMessages.Pop(); pMsg_2; pMsg_2 = GarbageMessages.Pop()) {
            FreeMessage(pMsg_2);
        }
        LeaveCriticalSection(&MessageCritSection);
    }
}

void BSAudioManager::ProcessSoundUpdates() {
    int32_t ikey;
    BSGameSound* psound;
    uint32_t iLastUpdateTime = iDelayTimerDelta;
    float ffrequencyDelta = 0.0f;
    float fcurrentFrequency = 1.0f;
    if (BSAudio::pAudioInstance->pListener) {
        NiTMapItem<int32_t, BSGameSound*>* mapIt;
        iDelayTimerDelta = GetTickCount() - iLastUpdateTime;
        float ftimeScale = BSTimer::fGlobalTimeMultiplier;
        if (BSTimer::fGlobalTimeMultiplier < 0.25) {
            ftimeScale = 0.25f;
        }
        if (bIgnoreGlobalTimeMultiplier) {
            ftimeScale = 1.0f;
        }
        NiPointer<NiAVObject> spobj;
        bool bprocessResult = true;
        psound = nullptr;
        NiPoint3 newDistance = NiPoint3_ZERO;
        ikey = 0;
        for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
            if (SoundList.m_ppkHashTable[i]) {
                mapIt = SoundList.m_ppkHashTable[i];
                goto LABEL_12;
            }
        }
        mapIt = nullptr;
    LABEL_12:;
        while (mapIt) {
            SoundList.GetNext(mapIt, ikey, psound);
            if ((psound->iStateFlags & BSGameSound::STATE_OPENED) != 0 &&
                (psound->iStateFlags & BSGameSound::STATE_READY) == 0 &&
                (psound->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                psound->Prepare();
                AddToCache(psound);
                if (((psound->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                     (psound->iStateFlags & BSGameSound::STATE_VALID) != 0) &&
                    (psound->QueuedMessages.m_uiCount)) {
                    NiTPointerList<BSSoundMessage*>* pmsgQueue = &psound->QueuedMessages;
                    NiTPointerList<BSSoundMessage*> tmpQueue;
                    NiTListItem<BSSoundMessage*>* it = nullptr;
                    while (pmsgQueue->m_uiCount) {
                        BSSoundMessage* ptmpMessage = pmsgQueue->RemoveHead();
                        // EXE-INLINED 0xAE8BF0
                        NiTListItem<BSSoundMessage*>* pNode = tmpQueue.NewItem();
                        pNode->m_element = ptmpMessage;
                        tmpQueue.AddNodeTail(pNode);
                    }
                    it = tmpQueue.m_pkHead;
                    while (it) {
                        BSSoundMessage** p_m_element = &it->m_element;
                        it = it->m_pkNext;
                        BSSoundMessage* ptmpMessage_2 = *p_m_element;
                        if ((*p_m_element) && (ptmpMessage_2->msg == MSET_POS)) {
                            ProcessMessage(ptmpMessage_2);
                        }
                    }
                    while (tmpQueue.m_uiCount) {
                        BSSoundMessage* ptmpMessage_3 = tmpQueue.RemoveHead();
                        bprocessResult = ProcessMessage(ptmpMessage_3);
                        FreeQueueMessage(ptmpMessage_3);
                    }
                    tmpQueue.RemoveAll();
                }
            }
            if ((psound->iStateFlags & BSGameSound::STATE_OPENED) != 0 &&
                (psound->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                (psound->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                if ((psound->iStateFlags & BSGameSound::STATE_PLAY_LATER) != 0 ||
                    psound->CanPlayNow(0, fCurrentGamehour)) {
                    if ((psound->iStateFlags & BSGameSound::STATE_PLAY_LATER) != 0 &&
                        psound->CanPlayNow(0, fCurrentGamehour)) {
                        psound->iStateFlags &= ~BSGameSound::STATE_PLAY_LATER;
                    }
                }
                else {
                    psound->iStateFlags |= BSGameSound::STATE_PLAY_LATER;
                }
                psound->Update(0);
            }
            if ((psound->iStateFlags & BSGameSound::STATE_PLAYED) != 0 && !psound->IsPlaying() &&
                (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ONE_SHOT) != 0 && !psound->IsPaused() &&
                (psound->iStateFlags & BSGameSound::STATE_READY) != 0) {
                psound->iStateFlags |= BSGameSound::STATE_STOPPED;
            }
            if (((psound->iStateFlags & BSGameSound::STATE_STOPPED) != 0 &&
                 (psound->iStateFlags & BSGameSound::STATE_FADING) == 0 &&
                 (psound->iStateFlags & BSGameSound::STATE_OPENING) == 0) &&
                (psound->iCleanupDelayInMS)) {
                uint32_t v4;
                if (psound->iCleanupDelayInMS <= iDelayTimerDelta) {
                    v4 = 0;
                }
                else {
                    v4 = psound->iCleanupDelayInMS - iDelayTimerDelta;
                }
                psound->iCleanupDelayInMS = v4;
            }

            if ((psound->iStateFlags & BSGameSound::STATE_STOPPED) == 0 ||
                ((psound->iStateFlags & BSGameSound::STATE_FADING) != 0 ||
                 ((psound->iStateFlags & BSGameSound::STATE_OPENING) != 0 || psound->iCleanupDelayInMS))) {
                bool v3;
                if ((psound->iStateFlags & BSGameSound::STATE_READY) != 0) {
                    SetVoiceState(psound->iSoundID, psound);
                }
                if ((psound->Is3DSound() || psound->Is2DRadiusSound()) &&
                    ((psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ONE_SHOT) == 0 &&
                     ((psound->iStateFlags & BSGameSound::STATE_INAUDIBLE) == 0 &&
                      (psound->IsPlaying() &&
                       ((psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) != 0 ||
                                ((psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                                 (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0)
                            ? v3 = true
                            : v3 = false,
                        v3 &&
                            ((psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0 &&
                             psound->fMaxDist * 1.05 < psound->fDistanceToListener)))))) {
                    psound->Stop();
                    psound->SetInaudible(true);
                }
                else {
                    if ((psound->iStateFlags & BSGameSound::STATE_INAUDIBLE) != 0 && !psound->IsPaused() &&
                        psound->fMaxDist > static_cast<double>(psound->fDistanceToListener)) {
                        psound->SetInaudible(false);
                        psound->Play(true);
                    }
                }
                fcurrentFrequency = psound->GetFrequency();
                ffrequencyDelta = ftimeScale - fcurrentFrequency;
                bool bbattleSound = (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_BATTLE_SOUND) != 0;
                bool bslowing = ffrequencyDelta < 0.0;
                float v2 = std::fabs(static_cast<double>(ffrequencyDelta));
                if (v2 > 0.009999999776482582 && (!bbattleSound || bslowing) &&
                    (psound->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0 &&
                    (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0 &&
                    (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_SYSTEM_SOUND) == 0 &&
                    (psound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE) == 0) {
                    psound->SetFrequency(ftimeScale);
                }
            }
            else {
                psound->Update(0);
                void (*pcompCallback)(void*, bool) = psound->fnCompletionCallback;
                if (pcompCallback) {
                    pcompCallback(
                        psound->pSoundCompletionContext, (psound->iStateFlags & BSGameSound::STATE_VALID) != 0
                    );
                }
                SoundList.RemoveAt(psound->iSoundID);
                MovingRefrList.GetAt(static_cast<int32_t>(psound->iSoundID), spobj);
                MovingRefrList.RemoveAt(psound->iSoundID);
                if (spobj.m_pObject) {
                    NiObjectGarbage::Add(spobj.m_pObject);
                    spobj.operator=(static_cast<NiAVObject*>(nullptr));
                }
                if (psound->QueuedMessages.m_uiCount) {
                    NiTPointerList<BSSoundMessage*>* pmsgQueue_2 = &psound->QueuedMessages;
                    while (pmsgQueue_2 && pmsgQueue_2->m_uiCount) {
                        BSSoundMessage* ptmpMessage_4 = pmsgQueue_2->RemoveHead();
                        FreeQueueMessage(ptmpMessage_4);
                    }
                }
                SetVoiceState(psound->iSoundID, nullptr);
                if (psound) {
                    delete psound;
                }
                psound = nullptr;
            }
        }
        bListenerMoved = false;
        UpdateMovingSounds();
        BSAudio::pAudioInstance->pListener->Update();
        iDelayTimerDelta = GetTickCount();
    }
}

void BSAudioManager::UpdateMovingSounds() {
    BSGameSound* pSound = nullptr;
    NiPointer<NiAVObject> spNode;
    if (!BSAudio::pAudioInstance->pListener) {
        return;
    }
    BSAudioListener* pListener = BSAudio::pAudioInstance->pListener;
    NiPoint3 kListenerPos = pListener->GetPosition();
    NiTMapItem<int32_t, NiPointer<NiAVObject>>* pPos = MovingRefrList.GetFirstPos();
    int32_t ikey = 0;
    uint32_t uiRefCount = 0;
    while (pPos) {
        pSound = nullptr;
        MovingRefrList.GetNext(pPos, ikey, spNode);
        SoundList.GetAt(ikey, pSound);
        uiRefCount = spNode ? spNode->m_uiRefCount : 0;
        if (pSound && (pSound->IsPlaying() || (pSound->iStateFlags & BSGameSound::STATE_INAUDIBLE) != 0) && spNode &&
            uiRefCount > 2) {
            NiPoint3 loc = spNode->m_kWorld.m_Translate;
            if ((pSound->iSoundTypeFlags & BSGameSound::SOUND_FLAG_VOICE) != 0) {
                loc.z = loc.z + 200.0;
            }
            pSound->SetEmitterPosition(loc);
        }
        else if (!pSound) {
            MovingRefrList.RemoveAt(ikey);
            if (spNode) {
                NiPointer<NiAVObject> spOld = spNode;
                NiObjectGarbage::Add(spNode);
                spNode = nullptr;
            }
        }
        else if (!spNode) {
            NiPoint3 kPos;
            kPos.x = 38.0f;
            kPos.y = 0.0f;
            kPos.z = 0.0f;
            // ComposeMessage's pointer-data argument is used as an untyped value here: an MFADE_OUT message carries its
            // fade time (1000 ms) in it, as FadeOut's does.
            ComposeMessage(
                MFADE_OUT, pSound->iSoundID, 0, reinterpret_cast<void*>(static_cast<uintptr_t>(1000)),
                NiPointer<NiAVObject>{}, kPos
            );
            MovingRefrList.RemoveAt(ikey);
            if (spNode) {
                NiPointer<NiAVObject> spOld = spNode;
                NiObjectGarbage::Add(spNode);
                spNode = nullptr;
            }
        }
    }
}

// Runs one queued audio-thread command on the sound it names (or on every sound of the types it names). A command for a
// sound that is not ready yet is queued on the sound or composed again for the next pass. Returns true when the command
// suspended the audio thread, so that the caller stops walking its message list.
bool BSAudioManager::ProcessMessage(BSSoundMessage* apMsg) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    BSGameSound* ptmpSoundPtr = nullptr;
    uint32_t uicurrentTime = GetTickCount();
    bool bresetList = false;
    switch (apMsg->msg) {
        case MPLAY: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    if (!ptmpSoundPtr->CanPlayNow(0, fCurrentGamehour)) {
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_PLAY_LATER;
                    }
                    if ((uiToFadeFlags & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0) {
                        ptmpSoundPtr->iSystemAttenuation = 10000;
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                    if (bDialogueFadeOut &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_SYSTEM_SOUND) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_VOICE) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0) {
                        Setting::Info* pDecibels;
                        // The setting's address is never null.
                        pDecibels = &fDialogueFadeDecibels.uValue;
                        ptmpSoundPtr->iFaderAttenuation = static_cast<int32_t>(pDecibels->f * 100.0);
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                    if ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_VARIABLE_FREQUENCY) != 0) {
                        float fcurrentFreq = ptmpSoundPtr->GetFrequency();
                        float fvariance = ptmpSoundPtr->iFreqVariance / 100.0;
                        if ((Singleton_guard & 1) == 0) {
                            Singleton_guard |= 1u;
                            BSRandom::Singleton.~BSRandom();
                            ::new (&BSRandom::Singleton) BSRandom;
                        }
                        // EXE-INLINED 0x476B90
                        uint32_t uiRandom = BSRandom::Singleton.UnsignedInt(0xFFFFFFFF);
                        float fMax = fvariance * 2.0;
                        float fRandom =
                            static_cast<float>(uiRandom * (static_cast<double>(fMax) - 0.0f) / 4294967296.0 + 0.0f);
                        fcurrentFreq = fRandom - fvariance + fcurrentFreq;
                        ptmpSoundPtr->SetFrequency(fcurrentFreq);
                    }
                    if ((ptmpSoundPtr->iModifierFlags & 1) != 0 ||
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0) {
                        ptmpSoundPtr->SetIsMuted(bListenerUnderwater);
                    }
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    ptmpSoundPtr->Play(false);
                    bool bisMuted = IsPausedType(ptmpSoundPtr->iSoundTypeFlags);
                    if (bisMuted && (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0) {
                        ptmpSoundPtr->Pause();
                    }
                    else if (bisMuted && (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0) {
                        ptmpSoundPtr->SetIsMuted(1);
                    }
                    else if (
                        bisMuted && (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0 &&
                        ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) != 0 ||
                         (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0)
                    ) {
                        ptmpSoundPtr->Stop();
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_STOPPED;
                    }
                    ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_PLAYED;
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_OPENED) != 0) {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    ptmpSoundPtr->QueuedMessages.AddTail(pqueuedMsg);
                }
                else {
                    ComposeMessage(
                        MPLAY, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        case MPLAYLOOPING: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->Play(true);
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    ptmpSoundPtr->QueuedMessages.AddTail(pqueuedMsg);
                }
            }
            break;
        }
        case MPAUSE: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->Pause();
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    ptmpSoundPtr->QueuedMessages.AddTail(pqueuedMsg);
                }
            }
            break;
        }
        case MSTOP: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->Stop();
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    ptmpSoundPtr->QueuedMessages.AddTail(pqueuedMsg);
                }
            }
            break;
        }
        case MPLAY_AFTER: {
            // iData holds the tick count to start at.
            if (static_cast<uint32_t>(apMsg->iData) > uicurrentTime) {
                ComposeMessage(
                    MPLAY_AFTER, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                );
            }
            else if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    if (!ptmpSoundPtr->CanPlayNow(0, fCurrentGamehour)) {
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_PLAY_LATER;
                    }
                    if ((uiToFadeFlags & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0) {
                        ptmpSoundPtr->iSystemAttenuation = 10000;
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                    if (bDialogueFadeOut &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_SYSTEM_SOUND) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_VOICE) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0) {
                        Setting::Info* pDecibels;
                        // The setting's address is never null.
                        pDecibels = &fDialogueFadeDecibels.uValue;
                        ptmpSoundPtr->iFaderAttenuation = static_cast<int32_t>(pDecibels->f * 100.0);
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                    if ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_VARIABLE_FREQUENCY) != 0) {
                        float fcurrentFreq = ptmpSoundPtr->GetFrequency();
                        float fvariance = ptmpSoundPtr->iFreqVariance / 100.0;
                        if ((Singleton_guard & 1) == 0) {
                            Singleton_guard |= 1u;
                            BSRandom::Singleton.~BSRandom();
                            ::new (&BSRandom::Singleton) BSRandom;
                        }
                        // EXE-INLINED 0x476B90
                        uint32_t uiRandom = BSRandom::Singleton.UnsignedInt(0xFFFFFFFF);
                        float fMax = fvariance * 2.0;
                        float fRandom =
                            static_cast<float>(uiRandom * (static_cast<double>(fMax) - 0.0f) / 4294967296.0 + 0.0f);
                        fcurrentFreq = fRandom - fvariance + fcurrentFreq;
                        ptmpSoundPtr->SetFrequency(fcurrentFreq);
                    }
                    if ((ptmpSoundPtr->iModifierFlags & 1) != 0 ||
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0) {
                        ptmpSoundPtr->SetIsMuted(bListenerUnderwater);
                    }
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    if (!ptmpSoundPtr->IsPaused()) {
                        ptmpSoundPtr->Play(false);
                    }
                    bool bisMuted = IsPausedType(ptmpSoundPtr->iSoundTypeFlags);
                    if (bisMuted && (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0) {
                        ptmpSoundPtr->Pause();
                    }
                    else if (bisMuted && (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0) {
                        ptmpSoundPtr->SetIsMuted(1);
                    }
                    else if (
                        bisMuted && (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) == 0 &&
                        (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0 &&
                        ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) != 0 ||
                         (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0)
                    ) {
                        ptmpSoundPtr->Stop();
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_STOPPED;
                    }
                    ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_PLAYED;
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else {
                    ComposeMessage(
                        MPLAY, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        case MMUTE: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->iSystemAttenuation = 10000;
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                }
                else {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    // EXE-INLINED 0xAE8BF0
                    NiTListItem<BSSoundMessage*>* pNode = ptmpSoundPtr->QueuedMessages.NewItem();
                    pNode->m_element = pqueuedMsg;
                    ptmpSoundPtr->QueuedMessages.AddNodeTail(pNode);
                }
            }
            break;
        }
        case MUNMUTE: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->iSystemAttenuation = 0;
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                }
                else {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    // EXE-INLINED 0xAE8BF0
                    NiTListItem<BSSoundMessage*>* pNode = ptmpSoundPtr->QueuedMessages.NewItem();
                    pNode->m_element = pqueuedMsg;
                    ptmpSoundPtr->QueuedMessages.AddNodeTail(pNode);
                }
            }
            break;
        }
        // iData and pData hold the fade's start and end tick counts (pData as an untyped value), the vector's x the
        // command to compose when the fade ends.
        case MFADE_OUT: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    if (!apMsg->iData) {
                        apMsg->iData = uicurrentTime;
                        apMsg->pData =
                            reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(apMsg->pData) + uicurrentTime);
                    }
                    uint32_t uistartTime = static_cast<uint32_t>(apMsg->iData);
                    uint32_t uiendTime = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData));
                    if (uicurrentTime < uiendTime) {
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_FADING;
                        ptmpSoundPtr->iSystemAttenuation = static_cast<int32_t>(
                            (1.0 -
                             static_cast<double>(uiendTime - uicurrentTime) /
                                 static_cast<double>(uiendTime - uistartTime)) *
                            3500.0
                        );
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                        NiPoint3 kVector;
                        kVector.x = apMsg->vector3[0];
                        kVector.y = apMsg->vector3[1];
                        kVector.z = apMsg->vector3[2];
                        ComposeMessage(
                            MFADE_OUT, ptmpSoundPtr->iSoundID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{},
                            kVector
                        );
                    }
                    else {
                        ptmpSoundPtr->iSystemAttenuation = 10000;
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                        ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
                        int32_t iendMsg = static_cast<int32_t>(static_cast<int64_t>(apMsg->vector3[0]));
                        if (iendMsg > 0) {
                            ComposeMessage(
                                static_cast<SOUND_MSG>(iendMsg), ptmpSoundPtr->iSoundID, 0, nullptr,
                                NiPointer<NiAVObject>{}, NiPoint3_ZERO
                            );
                        }
                    }
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else if (uicurrentTime < static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData))) {
                    NiPoint3 kVector;
                    kVector.x = apMsg->vector3[0];
                    kVector.y = apMsg->vector3[1];
                    kVector.z = apMsg->vector3[2];
                    ComposeMessage(
                        MFADE_OUT, ptmpSoundPtr->iSoundID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, kVector
                    );
                }
                else {
                    ptmpSoundPtr->iSystemAttenuation = 10000;
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
                    int32_t iendMsg = static_cast<int32_t>(static_cast<int64_t>(apMsg->vector3[0]));
                    if (iendMsg > 0) {
                        ComposeMessage(
                            static_cast<SOUND_MSG>(iendMsg), ptmpSoundPtr->iSoundID, 0, nullptr,
                            NiPointer<NiAVObject>{}, NiPoint3_ZERO
                        );
                    }
                }
            }
            break;
        }
        // The vector's x is the target attenuation in decibels; iData and pData the fade's start and end tick counts.
        case MFADE_TO: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    uint32_t uitargetAttenuation =
                        static_cast<uint32_t>(static_cast<int64_t>(apMsg->vector3[0] * 100.0));
                    uint32_t uistartTime = static_cast<uint32_t>(apMsg->iData);
                    uint32_t uiendTime = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData));
                    if (uicurrentTime < uiendTime) {
                        ptmpSoundPtr->iFaderAttenuation = static_cast<int32_t>(
                            static_cast<double>(uitargetAttenuation) *
                            (1.0 -
                             static_cast<double>(uiendTime - uicurrentTime) /
                                 static_cast<double>(uiendTime - uistartTime))
                        );
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                        NiPoint3 kVector;
                        kVector.x = apMsg->vector3[0];
                        kVector.y = apMsg->vector3[1];
                        kVector.z = apMsg->vector3[2];
                        ComposeMessage(
                            MFADE_TO, ptmpSoundPtr->iSoundID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{},
                            kVector
                        );
                    }
                    else {
                        ptmpSoundPtr->iFaderAttenuation = uitargetAttenuation;
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    NiPoint3 kVector;
                    kVector.x = apMsg->vector3[0];
                    kVector.y = apMsg->vector3[1];
                    kVector.z = apMsg->vector3[2];
                    ComposeMessage(
                        MFADE_TO, ptmpSoundPtr->iSoundID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, kVector
                    );
                }
            }
            break;
        }
        case MFADE_IN: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    if (!apMsg->iData) {
                        apMsg->iData = uicurrentTime;
                        apMsg->pData =
                            reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(apMsg->pData) + uicurrentTime);
                    }
                    uint32_t uistartTime = static_cast<uint32_t>(apMsg->iData);
                    uint32_t uiendTime = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData));
                    if (uicurrentTime < uiendTime) {
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_FADING;
                        ptmpSoundPtr->iSystemAttenuation = static_cast<int32_t>(
                            static_cast<double>(uiendTime - uicurrentTime) /
                            static_cast<double>(uiendTime - uistartTime) * 3500.0
                        );
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                        ComposeMessage(
                            MFADE_IN, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                        );
                    }
                    else {
                        ptmpSoundPtr->iSystemAttenuation = 0;
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                        ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
                    }
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else if (
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    uicurrentTime >= static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData))
                ) {
                    ptmpSoundPtr->iSystemAttenuation = 0;
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
                    ComposeMessage(
                        MUNMUTE, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
                else {
                    ComposeMessage(
                        MFADE_IN, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        // The vector's x is the starting attenuation in decibels; iData and pData the fade's start and end tick counts.
        case MFADE_FROM: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    uint32_t uitargetAttenuation =
                        static_cast<uint32_t>(static_cast<int64_t>(apMsg->vector3[0] * 100.0));
                    uint32_t uistartTime = static_cast<uint32_t>(apMsg->iData);
                    uint32_t uiendTime = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData));
                    if (uicurrentTime < uiendTime) {
                        ptmpSoundPtr->iFaderAttenuation = static_cast<int32_t>(
                            static_cast<double>(uitargetAttenuation) *
                            (static_cast<double>(uiendTime - uicurrentTime) /
                             static_cast<double>(uiendTime - uistartTime))
                        );
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                        NiPoint3 kVector;
                        kVector.x = apMsg->vector3[0];
                        kVector.y = apMsg->vector3[1];
                        kVector.z = apMsg->vector3[2];
                        ComposeMessage(
                            MFADE_FROM, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, kVector
                        );
                    }
                    else {
                        ptmpSoundPtr->iFaderAttenuation = 0;
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    NiPoint3 kVector;
                    kVector.x = apMsg->vector3[0];
                    kVector.y = apMsg->vector3[1];
                    kVector.z = apMsg->vector3[2];
                    ComposeMessage(
                        MFADE_FROM, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, kVector
                    );
                }
            }
            break;
        }
        // iID is nonzero to fade the world's sounds out for dialogue, zero to fade them back in.
        case MDIALOGUE_FADE: {
            uint32_t uidelay;
            bDialogueFadeOut = apMsg->iID != 0;
            if (bDialogueFadeOut) {
                Setting::Info* pSecondsOut;
                // The setting's address is never null.
                pSecondsOut = &fDialogueFadeSecondsOut.uValue;
                uidelay = static_cast<uint32_t>(static_cast<int64_t>(pSecondsOut->f * 1000.0));
            }
            else {
                Setting::Info* pSecondsIn;
                // The setting's address is never null.
                pSecondsIn = &fDialogueFadeSecondsIn.uValue;
                uidelay = static_cast<uint32_t>(static_cast<int64_t>(pSecondsIn->f * 1000.0));
            }
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_SYSTEM_SOUND) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_VOICE) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0) {
                    Setting::Info* pDecibels;
                    // The setting's address is never null.
                    pDecibels = &fDialogueFadeDecibels.uValue;
                    NiPoint3 kVector;
                    kVector.x = pDecibels->f;
                    kVector.y = 0.0f;
                    kVector.z = 0.0f;
                    ComposeMessage(
                        bDialogueFadeOut ? MFADE_TO : MFADE_FROM, ptmpSoundPtr->iSoundID, uicurrentTime,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(uidelay + uicurrentTime)),
                        NiPointer<NiAVObject>{}, kVector
                    );
                }
            }
            break;
        }
        case MVOLUME_LEVELS_CHANGED: {
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if (ptmpSoundPtr) {
                    ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                }
            }
            break;
        }
        // iData is the volume in thousandths.
        case MSET_VOLUME: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->SetVolume(static_cast<int32_t>(apMsg->iData) / 1000.0);
                    SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    ComposeMessage(
                        MSET_VOLUME, apMsg->iID, apMsg->iData, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        // iData is the frequency in thousandths.
        case MSET_FREQUENCY: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->SetFrequency(static_cast<int32_t>(apMsg->iData) / 1000.0);
            }
            break;
        }
        case MSET_FREQUENCY_VARIANCE: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                    ptmpSoundPtr->iSoundTypeFlags | BSGameSound::SOUND_FLAG_VARIABLE_FREQUENCY
                );
                ptmpSoundPtr->iFreqVariance = static_cast<uint8_t>(apMsg->iData);
            }
            break;
        }
        // iData is the sound types; pData the fade time in milliseconds, or none to mute at once.
        case MMUTE_ALL_OFTYPE: {
            uint32_t uiID = static_cast<uint32_t>(apMsg->iID);
            uint32_t uiTypes = static_cast<uint32_t>(apMsg->iData);
            uiToFadeFlags |= uiTypes;
            uintptr_t uifadeTime = reinterpret_cast<uintptr_t>(apMsg->pData);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if ((uiTypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0) {
                    if (uifadeTime && uiID > 5000) {
                        ComposeMessage(
                            MFADE_OUT, ptmpSoundPtr->iSoundID, uicurrentTime,
                            reinterpret_cast<void*>(uifadeTime + uicurrentTime), NiPointer<NiAVObject>{}, NiPoint3_ZERO
                        );
                    }
                    else if (!uifadeTime) {
                        ptmpSoundPtr->SetIsMuted(1);
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                }
            }
            break;
        }
        // iData is the sound types; pData the fade time in milliseconds, or none to unmute at once.
        case MUNMUTE_ALL_OFTYPE: {
            uint32_t uiTypes = static_cast<uint32_t>(apMsg->iData);
            uiToFadeFlags &= ~uiTypes;
            uintptr_t uifadeTime = reinterpret_cast<uintptr_t>(apMsg->pData);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if ((uiTypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0) {
                    if (uifadeTime) {
                        ComposeMessage(
                            MFADE_IN, ptmpSoundPtr->iSoundID, uicurrentTime,
                            reinterpret_cast<void*>(uifadeTime + uicurrentTime), NiPointer<NiAVObject>{}, NiPoint3_ZERO
                        );
                    }
                    else {
                        ptmpSoundPtr->SetIsMuted(0);
                        ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
                    }
                }
            }
            break;
        }
        case MPAUSE_ALL_OFTYPE: {
            int32_t itypes = static_cast<int32_t>(apMsg->iData);
            SetPausedType(static_cast<BSGameSound::SoundFlags>(itypes), false);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if (ptmpSoundPtr->IsPlaying() && (itypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0) {
                    ptmpSoundPtr->Pause();
                }
                else if (
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0
                ) {
                    ptmpSoundPtr->SetIsMuted(1);
                }
            }
            break;
        }
        case MUNPAUSE_ALL_OFTYPE: {
            int32_t itypes = static_cast<int32_t>(apMsg->iData);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0 && ptmpSoundPtr->IsPaused() &&
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0) {
                    ptmpSoundPtr->Play(false);
                }
                else if (
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0
                ) {
                    ptmpSoundPtr->SetIsMuted(0);
                }
            }
            ClearPausedType(itypes, false);
            break;
        }
        case MPAUSE_ALL_OFTYPE_EXACT: {
            int32_t itypes = static_cast<int32_t>(apMsg->iData);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            SetPausedType(static_cast<BSGameSound::SoundFlags>(itypes), true);
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if (!ptmpSoundPtr->IsInDecay() && ptmpSoundPtr->IsPlaying() &&
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) == static_cast<uint32_t>(itypes) &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    ptmpSoundPtr->Pause();
                }
                else if (
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) == static_cast<uint32_t>(itypes) &&
                    ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0 ||
                     (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0 ||
                     (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) != 0)
                ) {
                    ptmpSoundPtr->SetIsMuted(1);
                }
            }
            break;
        }
        case MUNPAUSE_ALL_OFTYPE_EXACT: {
            int32_t itypes = static_cast<int32_t>(apMsg->iData);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                // The region flag is tested twice in both conditions, as the game does.
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0 && ptmpSoundPtr->IsPaused() &&
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) == static_cast<uint32_t>(itypes) &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0) {
                    ptmpSoundPtr->Play(false);
                }
                else if (
                    (itypes & ptmpSoundPtr->iSoundTypeFlags) == static_cast<uint32_t>(itypes) &&
                    ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0 ||
                     (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0 ||
                     (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0 ||
                     (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) != 0)
                ) {
                    ptmpSoundPtr->SetIsMuted(0);
                }
            }
            ClearPausedType(itypes, true);
            break;
        }
        case MSET_POS: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                NiPoint3 vin;
                vin.x = apMsg->vector3[0];
                vin.y = apMsg->vector3[1];
                vin.z = apMsg->vector3[2];
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->SetEmitterPosition(vin);
                }
                else {
                    // A queued position message is overwritten with the newer one.
                    BSSoundMessage* pqueuedMsg = nullptr;
                    bool bfound = false;
                    NiTListItem<BSSoundMessage*>* pIter = ptmpSoundPtr->QueuedMessages.m_pkHead;
                    while (pIter && !bfound) {
                        BSSoundMessage** ppElement = &pIter->m_element;
                        pIter = pIter->m_pkNext;
                        pqueuedMsg = *ppElement;
                        if (pqueuedMsg && pqueuedMsg->msg == MSET_POS) {
                            BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                            bfound = true;
                        }
                    }
                    if (!bfound) {
                        pqueuedMsg = GetFreeQueueMessage();
                        BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                        // EXE-INLINED 0xAE8BF0
                        NiTListItem<BSSoundMessage*>* pNode = ptmpSoundPtr->QueuedMessages.NewItem();
                        pNode->m_element = pqueuedMsg;
                        ptmpSoundPtr->QueuedMessages.AddNodeTail(pNode);
                    }
                }
            }
            break;
        }
        case MSET_BEAM_END: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                float fx = apMsg->vector3[0];
                float fy = apMsg->vector3[1];
                float fz = apMsg->vector3[2];
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->vBeamEnd.x = fx;
                    ptmpSoundPtr->vBeamEnd.y = fy;
                    ptmpSoundPtr->vBeamEnd.z = fz;
                    ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_IS_VALID_BEAM;
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    NiPointer<NiAVObject> spData;
                    spData.operator=(apMsg->spData);
                    NiPoint3 kVector;
                    kVector.x = apMsg->vector3[0];
                    kVector.y = apMsg->vector3[1];
                    kVector.z = apMsg->vector3[2];
                    ComposeMessage(MSET_BEAM_END, apMsg->iID, apMsg->iData, apMsg->pData, spData, kVector);
                    NiPointer<NiAVObject> spNull;
                    apMsg->spData.operator=(spNull);
                }
            }
            break;
        }
        case MSET_STATIC_ATTENUATION: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->iStaticAttenuation = static_cast<uint16_t>(apMsg->iData);
                ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
            }
            break;
        }
        case MSET_REVERB_ATTENUATION: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->iReverbAttenuation = static_cast<uint16_t>(apMsg->iData);
                ptmpSoundPtr->SetVolume(ptmpSoundPtr->fVolume);
            }
            break;
        }
        // iData and pData are the minimum and maximum distances in thousandths (pData carries an untyped 32-bit
        // integer).
        case MSET_MINMAX_DISTANCE: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                float afMax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData)) / 1000.0;
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    float afMin = static_cast<int32_t>(apMsg->iData) / 1000.0;
                    ptmpSoundPtr->SetMinMax(afMin, afMax);
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    ComposeMessage(
                        MSET_MINMAX_DISTANCE, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{},
                        NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        case MSET_ATTENUATION_CURVE: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->SetAttenuationCurve(
                        apMsg->curve[0], apMsg->curve[1], apMsg->curve[2], apMsg->curve[3], apMsg->curve[4]
                    );
                }
                else {
                    SetAttenuationCurve(
                        apMsg->iID, apMsg->curve[0], apMsg->curve[1], apMsg->curve[2], apMsg->curve[3], apMsg->curve[4]
                    );
                }
            }
            break;
        }
        // pData is the new sound. It replaces a cached copy of the same file where there is one, else it is loaded.
        case MADD_SOUND_TO_LIST: {
            ptmpSoundPtr = static_cast<BSGameSound*>(apMsg->pData);
            if (pIOManager) {
                if (ptmpSoundPtr) {
                    bool bneedsNameResolved = !ptmpSoundPtr->SetupPath(false);
                    BSGameSound* pCachedSound = nullptr;
                    if ((ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_DONT_CACHE) == 0) {
                        pCachedSound = GetFromCache(
                            ptmpSoundPtr->iSoundHash, ptmpSoundPtr->iDirectoryHash, ptmpSoundPtr->iSoundTypeFlags
                        );
                    }
                    if (pCachedSound) {
                        pCachedSound->iSoundTypeFlags = ptmpSoundPtr->iSoundTypeFlags;
                        if (bneedsNameResolved || ptmpSoundPtr->iSoundHash != pCachedSound->iSoundHash) {
                            if (ptmpSoundPtr->iDirectoryHash == pCachedSound->iDirectoryHash &&
                                pCachedSound->iDirectoryHash) {
                                Precache(ptmpSoundPtr);
                            }
                        }
                        else {
                            delete ptmpSoundPtr;
                        }
                        ptmpSoundPtr = pCachedSound;
                        pCachedSound->iSoundID = apMsg->iID;
                        SoundList.SetAt(apMsg->iID, ptmpSoundPtr);
                        SetVoiceState(ptmpSoundPtr->iSoundID, ptmpSoundPtr);
                    }
                    else {
                        if (!ptmpSoundPtr->pFilename[0]) {
                            Warning("Sound %s is trying to load no filename\n", ptmpSoundPtr->pFilename);
                        }
                        AudioLoadTask* pTask;
                        void* apMemory = MemoryManager_Singleton.Allocate(sizeof(AudioLoadTask));
                        if (apMemory) {
                            pTask = ::new (apMemory) AudioLoadTask(ptmpSoundPtr);
                        }
                        else {
                            pTask = nullptr;
                        }
                        ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_OPENING;
                        pIOManager->AddTask(pTask);
                        SoundList.SetAt(apMsg->iID, ptmpSoundPtr);
                    }
                }
            }
            else {
                ComposeMessage(
                    MADD_SOUND_TO_LIST, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                );
            }
            break;
        }
        case MDELETE_SOUND: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_STOPPED;
                ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
            }
            break;
        }
        // spData is the object the sound follows; it replaces the one registered for the sound.
        case MREGISTER_MOVING_SOUND: {
            NiPointer<NiAVObject> spObj;
            spObj.operator=(apMsg->spData);
            NiPointer<NiAVObject> spobjTest;
            if (MovingRefrList.GetAt(apMsg->iID, spobjTest)) {
                MovingRefrList.RemoveAt(apMsg->iID);
                if (spobjTest->m_uiRefCount == 1) {
                    NiObjectGarbage::Add(spobjTest.m_pObject);
                }
                spobjTest.operator=(static_cast<NiAVObject*>(nullptr));
            }
            MovingRefrList.SetAt(apMsg->iID, spObj);
            NiPointer<NiAVObject> spNull;
            apMsg->spData.operator=(spNull);
            break;
        }
        case MPURGE_MOVING_SOUNDS: {
            NiPointer<NiAVObject> spObj;
            NiTMapItem<int32_t, NiPointer<NiAVObject>>* pPos = nullptr;
            for (uint32_t i = 0; i < MovingRefrList.m_uiHashSize; ++i) {
                if (MovingRefrList.m_ppkHashTable[i]) {
                    pPos = MovingRefrList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                MovingRefrList.GetNext(pPos, ikey, spObj);
                MovingRefrList.RemoveAt(ikey);
                if (spObj->m_uiRefCount == 1) {
                    NiObjectGarbage::Add(spObj.m_pObject);
                }
                spObj.operator=(static_cast<NiAVObject*>(nullptr));
                SoundList.GetAt(ikey, ptmpSoundPtr);
                if (ptmpSoundPtr) {
                    ptmpSoundPtr->Stop();
                    ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_STOPPED;
                    ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
                }
            }
            break;
        }
        // spData is the object whose sounds stop; pData the sound types to stop (none: all of them).
        case MSTOP_MOVING_SOUNDS: {
            NiPointer<NiAVObject> spTarget;
            spTarget.operator=(apMsg->spData);
            NiPointer<NiAVObject> spObj;
            uint32_t uiflags = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMsg->pData));
            NiTMapItem<int32_t, NiPointer<NiAVObject>>* pPos = nullptr;
            for (uint32_t i = 0; i < MovingRefrList.m_uiHashSize; ++i) {
                if (MovingRefrList.m_ppkHashTable[i]) {
                    pPos = MovingRefrList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                MovingRefrList.GetNext(pPos, ikey, spObj);
                if (spObj.m_pObject == spTarget.m_pObject) {
                    SoundList.GetAt(ikey, ptmpSoundPtr);
                    if (ptmpSoundPtr) {
                        if (!uiflags || (ptmpSoundPtr->iSoundTypeFlags & uiflags) != 0) {
                            ptmpSoundPtr->Stop();
                        }
                        // The flags are tested with a logical and against the nonzero envelope flags, so this holds for
                        // any nonzero flags.
                        if (ptmpSoundPtr->iSoundTypeFlags != 0) {
                            ptmpSoundPtr->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                                ptmpSoundPtr->iSoundTypeFlags | BSGameSound::SOUND_FLAG_ONE_SHOT
                            );
                        }
                    }
                }
            }
            break;
        }
        case MKILL_ALL_OFTYPE: {
            uint32_t uiTypes = static_cast<uint32_t>(apMsg->iData);
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if ((uiTypes & ptmpSoundPtr->iSoundTypeFlags) != 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_MUSIC) == 0 &&
                    (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) == 0) {
                    ptmpSoundPtr->Stop();
                    ptmpSoundPtr->iStateFlags &= ~BSGameSound::STATE_FADING;
                    ptmpSoundPtr->iStateFlags |= BSGameSound::STATE_STOPPED;
                }
            }
            break;
        }
        case MADD_SOUND_TO_CACHE: {
            ptmpSoundPtr = static_cast<BSGameSound*>(apMsg->pData);
            if (ptmpSoundPtr) {
                AddToCache(ptmpSoundPtr);
                delete ptmpSoundPtr;
            }
            break;
        }
        case MTERMINATE_THREAD: {
            GarbageMessages.Push(apMsg);
            if (BSAudio::pAudioInstance->bMultiThread && GetCurrentThreadId() == dwAudioThreadID && pUpdateThread) {
                BSAudio::pAudioInstance->bMultiThread = false;
                pUpdateThread->ExitThread();
            }
            break;
        }
        case MSUSPEND_THREAD: {
            GarbageMessages.Push(apMsg);
            if (BSAudio::pAudioInstance->bMultiThread && GetCurrentThreadId() == dwAudioThreadID && pUpdateThread) {
                BSAudio::pAudioInstance->bMultiThread = false;
                LeaveCriticalSection(&ProcessingCritSection);
                pUpdateThread->Suspend();
                EnterCriticalSection(&ProcessingCritSection);
                bresetList = true;
            }
            else if (GetCurrentThreadId() != dwAudioThreadID) {
                platform::DebuggerBreak();
            }
            break;
        }
        case MSET_UNDERWATER: {
            NiTMapItem<int32_t, BSGameSound*>* pPos = nullptr;
            for (uint32_t i = 0; i < SoundList.m_uiHashSize; ++i) {
                if (SoundList.m_ppkHashTable[i]) {
                    pPos = SoundList.m_ppkHashTable[i];
                    break;
                }
            }
            int32_t ikey;
            while (pPos) {
                SoundList.GetNext(pPos, ikey, ptmpSoundPtr);
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0 &&
                    ((ptmpSoundPtr->iModifierFlags & 1) != 0 ||
                     (ptmpSoundPtr->iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0)) {
                    ptmpSoundPtr->SetIsMuted(bListenerUnderwater);
                }
            }
            break;
        }
        case MSET_LISTENER_POSITION: {
            NiPoint3 kPosition;
            kPosition.x = apMsg->vector3[0];
            kPosition.y = apMsg->vector3[1];
            kPosition.z = apMsg->vector3[2];
            if (BSAudio::pAudioInstance->pListener) {
                BSAudio::pAudioInstance->pListener->SetPosition(kPosition);
                bListenerMoved = true;
            }
            break;
        }
        case MDATA_REQUEST: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->ProcessDataRequest(static_cast<int32_t>(apMsg->iData));
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    ComposeMessage(
                        MDATA_REQUEST, apMsg->iID, apMsg->iData, nullptr, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        // pData is the callback, iData its context.
        case MSET_COMPLETION_CALLBACK: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                void* pContext = reinterpret_cast<void*>(apMsg->iData);
                ptmpSoundPtr->fnCompletionCallback = reinterpret_cast<void (*)(void*, bool)>(apMsg->pData);
                ptmpSoundPtr->pSoundCompletionContext = pContext;
            }
            break;
        }
        case MSET_PLAY_CALLBACK: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                void* pContext = reinterpret_cast<void*>(apMsg->iData);
                ptmpSoundPtr->fnPlayedCallback = reinterpret_cast<void (*)(void*, int32_t)>(apMsg->pData);
                ptmpSoundPtr->pSoundPlayContext = pContext;
            }
            break;
        }
        case MSET_SYNCH_START: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->iSynchStartTime = static_cast<uint32_t>(apMsg->iData);
                    ptmpSoundPtr->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                        ptmpSoundPtr->iSoundTypeFlags | BSGameSound::SOUND_FLAG_EXTERNAL_SYNC
                    );
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    ComposeMessage(
                        MSET_SYNCH_START, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        case MCLEAR_SYNCH_START: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->iSynchStartTime = 0;
                    ptmpSoundPtr->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                        ptmpSoundPtr->iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_EXTERNAL_SYNC
                    );
                }
                else if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_STOPPED) == 0) {
                    ComposeMessage(
                        MCLEAR_SYNCH_START, apMsg->iID, apMsg->iData, apMsg->pData, NiPointer<NiAVObject>{},
                        NiPoint3_ZERO
                    );
                }
            }
            break;
        }
        // iData and pData's low word are the start and stop times in units of 0.09375.
        case MSET_TIME_CONSTRAINTS: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->fBeginTime = static_cast<int32_t>(apMsg->iData) * 0.09375;
                ptmpSoundPtr->fEndTime = static_cast<uint16_t>(reinterpret_cast<uintptr_t>(apMsg->pData)) * 0.09375;
            }
            break;
        }
        case MSEEK: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if ((ptmpSoundPtr->iStateFlags & BSGameSound::STATE_READY) != 0 &&
                    (ptmpSoundPtr->iStateFlags & BSGameSound::STATE_VALID) != 0) {
                    ptmpSoundPtr->Seek(static_cast<uint32_t>(apMsg->iData));
                }
                else {
                    BSSoundMessage* pqueuedMsg = GetFreeQueueMessage();
                    BSSoundMessage::Assign(pqueuedMsg, *apMsg);
                    ptmpSoundPtr->QueuedMessages.AddTail(pqueuedMsg);
                }
            }
            break;
        }
        case MSET_AT_LISTENER: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                if (apMsg->iData) {
                    ptmpSoundPtr->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                        ptmpSoundPtr->iSoundTypeFlags | BSGameSound::SOUND_FLAG_FIRST_PERSON
                    );
                }
                else {
                    ptmpSoundPtr->iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                        ptmpSoundPtr->iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_FIRST_PERSON
                    );
                }
            }
            break;
        }
        case MSET_PRIORITY: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                ptmpSoundPtr->iPriority = static_cast<uint32_t>(apMsg->iData);
            }
            break;
        }
        // iData is the frequency scale in thousandths.
        case MSET_MOD_FREQUENCY: {
            if (SoundList.GetAt(apMsg->iID, ptmpSoundPtr) && ptmpSoundPtr) {
                float fscale = static_cast<int32_t>(apMsg->iData) / 1000.0;
                ptmpSoundPtr->iModSamplesPerSecond = static_cast<int32_t>(ptmpSoundPtr->iSamplesPerSecond * fscale);
            }
            break;
        }
        default: break;
    }
    GetThreadTLSData()->eMemContext = eMemContext;
    return bresetList;
}

BSSimpleList<AudioDebugString*>* BSAudioManager::GetDebugString(uint32_t auiPage) {
    BSSimpleList<AudioDebugString*>* pList;
    BSSoundInfo::Data kCopy;
    BSSoundInfo::Data kPlayCopy;
    BSSoundInfo::Data kSoundData;
    int32_t iKey;
    BSSoundInfo* pSoundInfo;
    NiTMapItem<int32_t, BSSoundInfo*>* kIter;
    BSSimpleList<AudioDebugString*>* pNewList = static_cast<BSSimpleList<AudioDebugString*>*>(
        MemoryManager_Singleton.Allocate(sizeof(BSSimpleList<AudioDebugString*>))
    );
    if (pNewList) {
        pNewList->m_item = nullptr;
        pNewList->m_pkNext = nullptr;
        pList = pNewList;
    }
    else {
        pList = nullptr;
    }
    BSGameSound::SoundFlags eAudioFlagToCheck = static_cast<BSGameSound::SoundFlags>(
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE |
        BSGameSound::SOUND_FLAG_FOOTSTEPS | BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
        BSGameSound::SOUND_FLAG_VARIABLE_FREQUENCY | BSGameSound::SOUND_FLAG_BATTLE_SOUND |
        BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_UNKBIT9 | BSGameSound::SOUND_FLAG_UNKBIT10 |
        BSGameSound::SOUND_FLAG_MUSIC | BSGameSound::SOUND_FLAG_REGION | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
        BSGameSound::SOUND_FLAG_IMPACT | BSGameSound::SOUND_FLAG_CACHED | BSGameSound::SOUND_FLAG_DONT_CACHE |
        BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE | BSGameSound::SOUND_FLAG_FIRST_PERSON |
        BSGameSound::SOUND_FLAG_MODULATED | BSGameSound::SOUND_FLAG_RADIO | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE |
        BSGameSound::SOUND_FLAG_UNKBIT22 | BSGameSound::SOUND_FLAG_MUSIC_B23 | BSGameSound::SOUND_FLAG_EXTERNAL_SYNC |
        BSGameSound::SOUND_FLAG_ENVELOPE_FAST | BSGameSound::SOUND_FLAG_ENVELOPE_SLOW |
        BSGameSound::SOUND_FLAG_IS_2D_RADIUS | BSGameSound::SOUND_FLAG_BEAM_EMITTER | BSGameSound::SOUND_FLAG_UNKBIT29 |
        BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN | BSGameSound::SOUND_FLAG_UNKBIT31
    );
    SynchVoiceStates();
    bool bCheckPaused = false;
    char* Destination = nullptr;
    if (auiPage >= 99) {
        if (auiPage == 99) {
            eAudioFlagToCheck = BSGameSound::SOUND_FLAG_MUSIC;
        }
    }
    else {
        char* cTextBuffer = static_cast<char*>(MemoryManager_Singleton.Allocate(260u));
        Destination = cTextBuffer;
        std::sprintf(cTextBuffer, "Page %i", auiPage + 1);
        switch (auiPage) {
            case 1u: {
                std::strcat(Destination, "(3D only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_IS_3D;
                break;
            }
            case 2u: {
                std::strcat(Destination, "(2D only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_IS_2D;
                break;
            }
            case 3u: {
                std::strcat(Destination, "(2D-Radius only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
                break;
            }
            case 4u: {
                std::strcat(Destination, "(Voices only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_VOICE;
                break;
            }
            case 5u: {
                std::strcat(Destination, "(Footsteps only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_FOOTSTEPS;
                break;
            }
            case 6u: {
                std::strcat(Destination, "(System only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_SYSTEM_SOUND;
                break;
            }
            case 7u: {
                std::strcat(Destination, "(Region only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_REGION;
                break;
            }
            case 8u: {
                std::strcat(Destination, "(Animation-driven only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN;
                break;
            }
            case 9u: {
                std::strcat(Destination, "(Beam-emitter only)");
                eAudioFlagToCheck = BSGameSound::SOUND_FLAG_BEAM_EMITTER;
                break;
            }
            case 10u: {
                std::strcat(Destination, "(Stopped or Silent only)");
                bCheckPaused = true;
                break;
            }
        }
        AudioDebugString* pPageString =
            static_cast<AudioDebugString*>(MemoryManager_Singleton.Allocate(sizeof(AudioDebugString)));
        if (pPageString) {
            pPageString->iSoundID = 0;
            pPageString->pString = Destination;
        }
        AudioDebugString* pPageItem = pPageString;
        pList->AddTail(pPageItem);
        Destination = static_cast<char*>(MemoryManager_Singleton.Allocate(260u));
        std::sprintf(Destination, "ID\tIsPlaying?");
        AudioDebugString* pHeaderString =
            static_cast<AudioDebugString*>(MemoryManager_Singleton.Allocate(sizeof(AudioDebugString)));
        if (pHeaderString) {
            pHeaderString->iSoundID = 1;
            pHeaderString->pString = Destination;
        }
        AudioDebugString* pHeaderItem = pHeaderString;
        pList->AddTail(pHeaderItem);
    }
    EnterCriticalSection(&StateCritSection);
    // This loop does what GetFirstPos does.
    NiTMapItem<int32_t, BSSoundInfo*>* pFirst = nullptr;
    for (uint32_t i = 0; i < SoundStateMap.m_uiHashSize; ++i) {
        if (SoundStateMap.m_ppkHashTable[i]) {
            pFirst = SoundStateMap.m_ppkHashTable[i];
            break;
        }
    }
    kIter = pFirst;
    kSoundData.~Data();
    ::new (&kSoundData) BSSoundInfo::Data;
    int32_t iLines = 1;
    uint32_t uiSoundID = 0;
    int32_t iMaxAttenuation = 0;
    while (kIter && iLines < 20) {
        SoundStateMap.GetNext(kIter, iKey, pSoundInfo);
        char* pLine = static_cast<char*>(MemoryManager_Singleton.Allocate(260u));
        iMaxAttenuation = 0;
        if (pSoundInfo) {
            uint32_t uiFaderAttenuation;
            uint32_t uiDistanceAttenuation;
            uint32_t uiStaticAttenuation;
            BSSoundInfo::Data* apSource = pSoundInfo->CopyData(&kCopy);
            kSoundData.SetDebugInfo(apSource);
            kCopy.Free();
            if (iMaxAttenuation <= kSoundData.uiStaticAttenuation) {
                uiStaticAttenuation = kSoundData.uiStaticAttenuation;
            }
            else {
                uiStaticAttenuation = iMaxAttenuation;
            }
            iMaxAttenuation = uiStaticAttenuation;
            if (uiStaticAttenuation <= kSoundData.uiDistanceAttenuation) {
                uiDistanceAttenuation = kSoundData.uiDistanceAttenuation;
            }
            else {
                uiDistanceAttenuation = iMaxAttenuation;
            }
            iMaxAttenuation = uiDistanceAttenuation;
            if (uiDistanceAttenuation <= kSoundData.uiFaderAttenuation) {
                uiFaderAttenuation = kSoundData.uiFaderAttenuation;
            }
            else {
                uiFaderAttenuation = iMaxAttenuation;
            }
            iMaxAttenuation = uiFaderAttenuation;
        }
        if (((bCheckPaused && (pSoundInfo && (!pSoundInfo->bIsPlaying || iMaxAttenuation > 3000))) ||
             (!bCheckPaused && (pSoundInfo && (pSoundInfo->bIsPlaying && iMaxAttenuation <= 3000)))) &&
            (pSoundInfo->uiSoundType & eAudioFlagToCheck) != 0) {
            char Source[268];
            BSSoundInfo::Data* pPlayData = pSoundInfo->CopyData(&kPlayCopy);
            kSoundData.SetDebugInfo(pPlayData);
            kPlayCopy.Free();
            uiSoundID = pSoundInfo->iID;
            if (kSoundData.pFilePath) {
                const char* Src;
                char Str[268];
                if (kSoundData.pFilePath != kSoundData.cFilePath) {
                    kSoundData.pFilePath = kSoundData.cFilePath;
                }
                strcpy_s(Str, 260, kSoundData.pFilePath);
                // Finds the last backslash of the path.
                for (Src = std::strstr(Str, "\\"); Src && std::strstr(Src + 1, "\\"); Src = std::strstr(Src, "\\")) {
                    ++Src;
                }
                if (Src && Src[1]) {
                    strcpy_s(kSoundData.pFilePath, 260, Src);
                    if ((pSoundInfo->uiSoundType & BSGameSound::SOUND_FLAG_IS_3D) != 0) {
                        const char* pState;
                        if (pSoundInfo->bIsPlaying) {
                            pState = "Play";
                        }
                        else {
                            pState = "Stop";
                        }
                        std::sprintf(
                            Source, "%i:%s\t [%s][Dist:%.2f] [%.0f/%.0f] [ST:%i][SY:%i][D:%i]", uiSoundID,
                            kSoundData.pFilePath, pState, kSoundData.kPosition.z, kSoundData.fMinAttenuationDist,
                            kSoundData.fMaxAttenuationDist, kSoundData.uiStaticAttenuation,
                            kSoundData.uiDistanceAttenuation, kSoundData.uiFaderAttenuation
                        );
                    }
                    else {
                        if ((pSoundInfo->uiSoundType & BSGameSound::SOUND_FLAG_IS_2D) == 0 ||
                            (pSoundInfo->uiSoundType & BSGameSound::SOUND_FLAG_IS_2D_RADIUS) != 0) {
                            if ((pSoundInfo->uiSoundType & BSGameSound::SOUND_FLAG_IS_2D) != 0 &&
                                (pSoundInfo->uiSoundType & BSGameSound::SOUND_FLAG_IS_2D_RADIUS) != 0) {
                                const char* pRadiusState;
                                if (pSoundInfo->bIsPlaying) {
                                    pRadiusState = "Play";
                                }
                                else {
                                    pRadiusState = "Stop";
                                }
                                std::sprintf(
                                    Source, "%i:%s\t [%s][2DRADIUS][Dist:%.2f][%.0f/%.0f][ST:%i][SY:%i][D:%i]",
                                    uiSoundID, kSoundData.pFilePath, pRadiusState, kSoundData.kPosition.z,
                                    kSoundData.fMinAttenuationDist, kSoundData.fMaxAttenuationDist,
                                    kSoundData.uiStaticAttenuation, kSoundData.uiDistanceAttenuation,
                                    kSoundData.uiFaderAttenuation
                                );
                            }
                            else {
                                const char* pErrorState;
                                if (pSoundInfo->bIsPlaying) {
                                    pErrorState = "Play";
                                }
                                else {
                                    pErrorState = "Stop";
                                }
                                std::sprintf(
                                    Source, "%i:%s\t [%s][ERROR] [ST:%i][SY:%i]", uiSoundID, kSoundData.pFilePath,
                                    pErrorState, kSoundData.uiStaticAttenuation, kSoundData.uiDistanceAttenuation
                                );
                            }
                        }
                        else {
                            const char* p2DState;
                            if (pSoundInfo->bIsPlaying) {
                                p2DState = "Play";
                            }
                            else {
                                p2DState = "Stop";
                            }
                            std::sprintf(
                                Source, "%i:%s\t [%s][2D] [ST:%i][SY:%i]", uiSoundID, kSoundData.pFilePath, p2DState,
                                kSoundData.uiStaticAttenuation, kSoundData.uiDistanceAttenuation
                            );
                        }
                    }
                    strcpy_s(pLine, 260, Source);
                    goto LABEL_100;
                }
                MemoryManager_Singleton.Deallocate(pLine);
            }
            else {
                const char* pPlaying;
                if (pSoundInfo->bIsPlaying) {
                    pPlaying = "Playing";
                }
                else {
                    pPlaying = "Stopped";
                }
                std::sprintf(Source, "%i\t%s", uiSoundID, pPlaying);
                std::strcat(pLine, Source);
            LABEL_100:;
                AudioDebugString* pLineString =
                    static_cast<AudioDebugString*>(MemoryManager_Singleton.Allocate(sizeof(AudioDebugString)));
                if (pLineString) {
                    pLineString->iSoundID = uiSoundID + 2;
                    pLineString->pString = pLine;
                }
                AudioDebugString* pLineItem = pLineString;
                pList->AddTail(pLineItem);
                ++iLines;
            }
        }
    }
    LeaveCriticalSection(&StateCritSection);
    pList->Sort(&Compare_AE5FB0);
    kSoundData.Free();
    return pList;
}

bool BSAudioManager::SetPausedType(BSGameSound::SoundFlags auiTypes, bool abExact) {
    for (uint32_t i = 0; i < uiMutedTypesCount; ++i) {
        if (!pMutedSoundTypes[i].usState) {
            pMutedSoundTypes[i].usState = 2 - abExact;
            pMutedSoundTypes[i].uiTypes = auiTypes;
            return true;
        }
    }
    return false;
}

BSAudioManager& BSAudioManager::QInstance() {
    static BSAudioManager _sInstance;
    return _sInstance;
}

// The deleting destructor frees through the engine's heap.
void BSAudioManager::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}

BSSoundInfo::Data::Data()
  : dword0(cPath04), pFilePath(cFilePath), fMinAttenuationDist(0.0f), fMaxAttenuationDist(0.0f),
    kPosition(NiPoint3_ZERO), dword224(0), float228(0.0f), float22C(0.0f), uiStaticAttenuation(0),
    uiDistanceAttenuation(0), uiFaderAttenuation(0) {
    cPath04[0] = 0;
    cFilePath[0] = 0;
}

void BSSoundInfo::Data::Free() {
    dword0 = nullptr;
    pFilePath = nullptr;
    fMinAttenuationDist = (float)0.0;
    fMaxAttenuationDist = (float)0.0;
    dword224 = (uint32_t)0;
    float228 = (float)0.0;
    float22C = (float)0.0;
    uiStaticAttenuation = (uint32_t)0;
    uiDistanceAttenuation = (uint32_t)0;
    uiFaderAttenuation = (uint32_t)0;
}

// Copies apSource's paths, distances, position and attenuations over this record and returns it; a null path pointer
// leaves the buffer's first byte cleared.
BSSoundInfo::Data* BSSoundInfo::Data::SetDebugInfo(BSSoundInfo::Data* apSource) {
    cPath04[0] = '\0';
    cFilePath[0] = '\0';
    if (apSource->dword0) {
        std::strcpy(dword0, apSource->dword0);
    }
    if (apSource->pFilePath) {
        std::strcpy(pFilePath, apSource->pFilePath);
    }
    fMinAttenuationDist = apSource->fMinAttenuationDist;
    fMaxAttenuationDist = apSource->fMaxAttenuationDist;
    kPosition = apSource->kPosition;
    dword224 = apSource->dword224;
    float228 = apSource->float228;
    float22C = apSource->float22C;
    uiStaticAttenuation = apSource->uiStaticAttenuation;
    uiDistanceAttenuation = apSource->uiDistanceAttenuation;
    uiFaderAttenuation = apSource->uiFaderAttenuation;
    return this;
}

BSSoundInfo::BSSoundInfo() : iID(0), fFrequency(0.0f), uiSoundType(0), uiDuration(0), bIsPlaying(false) {}

BSSoundInfo::BSSoundInfo(BSSoundInfo* si)
  : iID(si->iID), fFrequency(si->fFrequency), fVolume(si->fVolume), uiSoundType(si->uiSoundType),
    uiDuration(si->uiDuration), bIsPlaying(si->bIsPlaying) {
    kDebugInfo.SetDebugInfo(&si->kDebugInfo);
}

BSSoundInfo::~BSSoundInfo() {
    iID = 0;
    fFrequency = 0.0f;
    fVolume = 0.0f;
    uiSoundType = 0;
    uiDuration = 0;
    bIsPlaying = false;
    kDebugInfo.Free();
}

BSSoundInfo* BSSoundInfo::Copy(BSSoundInfo* a2) {
    iID = a2->iID;
    fFrequency = a2->fFrequency;
    fVolume = a2->fVolume;
    uiSoundType = a2->uiSoundType;
    uiDuration = a2->uiDuration;
    bIsPlaying = a2->bIsPlaying;
    kDebugInfo.SetDebugInfo(&a2->kDebugInfo);
    return this;
}

BSSoundInfo::Data* BSSoundInfo::CopyData(BSSoundInfo::Data* a1) {
    *a1 = kDebugInfo;
    return a1;
}

BSSoundMessage::BSSoundMessage() : msg(MINVALID_MSG), iID(0), iData(0), pData(nullptr), vector3{0.0f, 0.0f, 0.0f} {}

// spData's own destructor releases the reference after the object is handed to the garbage list.
BSSoundMessage::~BSSoundMessage() {
    if (spData.m_pObject) {
        NiObjectGarbage::Add(spData.m_pObject);
    }
}

BSSoundMessage* BSSoundMessage::Assign(BSSoundMessage* apThis, const BSSoundMessage& arOther) {
    apThis->msg = arOther.msg;
    apThis->iID = arOther.iID;
    apThis->iData = arOther.iData;
    apThis->pData = arOther.pData;
    apThis->spData.operator=(arOther.spData);
    for (uint32_t S2 = 0; S2 < 3; ++S2) {
        apThis->vector3[S2] = (&arOther)->vector3[S2];
    }
    for (uint32_t i = 0; i < 5; ++i) {
        apThis->curve[i] = (&arOther)->curve[i];
    }
    return apThis;
}

AudioLoadTask::AudioLoadTask(BSGameSound* apSound) : QueuedFileEntry(IO_TASK_PRIORITY_CRITICAL), pSound(apSound) {
    char cPath[268];
    StandardizeDataPath(pSound->pFilename, cPath, 260);
    SetFileName(cPath);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_SOUNDS);
}

// The destructor is empty; the base's destructor does the rest.
AudioLoadTask::~AudioLoadTask() {}

// A cached sound's task leaves CacheLoadTaskList (RemoveTask, as Finish does) before it deletes the sound or hands it
// to the cache; this differs from the game, which leaves it listed until Finish, so Precache on another thread would
// read the freed sound's hashes through the task.
void AudioLoadTask::Run() {
    BSGameSound::SoundFlags uiTypeFlags = pSound->iSoundTypeFlags;
    bool bCached = (uiTypeFlags & BSGameSound::SOUND_FLAG_CACHED) != 0;
    bool bOpenResult = false;
    bool bInCache = false;
    if (!pSound->iSoundHash && pSound->iDirectoryHash != 0) {
        pSound->SetupPath(true);
        uint32_t uiSoundHash = pSound->iSoundHash;
        bInCache = BSAudioManager::QInstance().IsInCache(uiSoundHash, false);
    }
    if (bInCache) {
        if (bCached) {
            BSAudioManager::QInstance().RemoveLoadTask(this);
            if (pSound) {
                delete pSound;
            }
        }
        else {
            pSound->iStateFlags &= ~BSGameSound::STATE_FADING;
            pSound->iStateFlags &= ~BSGameSound::STATE_OPENING;
            pSound->iStateFlags |= BSGameSound::STATE_STOPPED;
        }
    }
    else if (
        bCached && AudioLoadTask::uiLastID == pSound->iSoundHash && GetTickCount() - AudioLoadTask::uiLastTime < 1000
    ) {
        BSAudioManager::QInstance().RemoveLoadTask(this);
        if (pSound) {
            delete pSound;
        }
    }
    else {
        bOpenResult = pSound->Open();
        if (bCached) {
            BSAudioManager::QInstance().RemoveLoadTask(this);
            if (bOpenResult) {
                AudioLoadTask::uiLastID = pSound->iSoundHash;
                AudioLoadTask::uiLastTime = GetTickCount();
                BSGameSound* pAdded = pSound;
                BSAudioManager::QInstance().ComposeMessage(
                    MADD_SOUND_TO_CACHE, 0, 0, pAdded, NiPointer<NiAVObject>{}, NiPoint3_ZERO
                );
            }
            else if (pSound) {
                delete pSound;
            }
        }
    }
}

void AudioLoadTask::Finish() {
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    v1->RemoveLoadTask(this);
    pSound = pSound;
}

void AudioLoadTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    BSAudioManager* v3 = &BSAudioManager::QInstance();
    v3->RemoveLoadTask(this);
    if (pSound &&
        (static_cast<uint16_t>(pSound->iSoundTypeFlags) &
         static_cast<BSGameSound::SoundFlags>(
             BSGameSound::SOUND_FLAG_CACHED | BSGameSound::SOUND_FLAG_DONT_CACHE |
             BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE | BSGameSound::SOUND_FLAG_FIRST_PERSON |
             BSGameSound::SOUND_FLAG_MODULATED | BSGameSound::SOUND_FLAG_RADIO |
             BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE | BSGameSound::SOUND_FLAG_UNKBIT22 |
             BSGameSound::SOUND_FLAG_MUSIC_B23 | BSGameSound::SOUND_FLAG_EXTERNAL_SYNC |
             BSGameSound::SOUND_FLAG_ENVELOPE_FAST | BSGameSound::SOUND_FLAG_ENVELOPE_SLOW |
             BSGameSound::SOUND_FLAG_IS_2D_RADIUS | BSGameSound::SOUND_FLAG_BEAM_EMITTER |
             BSGameSound::SOUND_FLAG_UNKBIT29 | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN |
             BSGameSound::SOUND_FLAG_UNKBIT31
         )) != 0) {
        if (pSound) {
            delete pSound;
        }
    }
    else {
        if (pSound) {
            pSound->iStateFlags &= ~BSGameSound::STATE_FADING;
            pSound->iStateFlags |= BSGameSound::STATE_STOPPED;
        }
    }
}

bool AudioLoadTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    // The type description parameter is char*, though the callers pass constant text.
    return QueuedFileEntry::GetDescription(apDescription, aiBufferSize, const_cast<char*>("audio load task"));
}

uint8_t AudioLoadTask::GetFileIndex() {
    return (uint8_t)2;
}

// The deleting destructor frees through the engine's heap.
void AudioLoadTask::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}

// Both semaphores are BSSemaphore's default (count 0, maximum 1).
BSAudioManagerThread::BSAudioManagerThread() : m_bSuspended(true) {}

// The semaphores' own destructors close their handles.
BSAudioManagerThread::~BSAudioManagerThread() {}

uint32_t BSAudioManagerThread::ThreadProc() {
    BSSemaphore* lpAddend = &m_SuspendSema;
    m_SuspendSema.hSemaphore->Wait();
    InterlockedDecrementInt32(&lpAddend->iCount);
    m_bSuspended = false;
    while (true) {
        if (pIOManager) {
            BSAudioManager* v1 = &BSAudioManager::QInstance();
            v1->Process();
        }
        Sleep(1u);
    }
}

void BSAudioManagerThread::Resume() {
    if (m_bSuspended) {
        m_SuspendSema.hSemaphore->Release();
        InterlockedIncrementInt32(&m_SuspendSema.iCount);
    }
}

void BSAudioManagerThread::Suspend() {
    m_bSuspended = true;
    BSSemaphore* lpAddend = &m_SuspendedSema;
    m_SuspendedSema.hSemaphore->Release();
    InterlockedIncrementInt32(&lpAddend->iCount);
    m_SuspendSema.hSemaphore->Wait();
    InterlockedDecrementInt32(&m_SuspendSema.iCount);
    m_bSuspended = false;
}

void BSAudioManagerThread::ExitThread() {
    m_bSuspended = true;
    BSSemaphore* lpAddend = &m_SuspendedSema;
    m_SuspendedSema.hSemaphore->Release();
    InterlockedIncrementInt32(&lpAddend->iCount);
    au_re_ExitThread(0, 0);
}

// The deleting destructor frees through the engine's heap.
void BSAudioManagerThread::operator delete(void* apMemory) {
    (&MemoryManager_Singleton)->Deallocate(apMemory);
}
