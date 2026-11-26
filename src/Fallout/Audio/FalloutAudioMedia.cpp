#include "Fallout/Audio/FalloutAudioMedia.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkThreadMemoryRouter.h"
#include "BSMain/Setting.h"
#include "BSMovie/CAsyncStream.h"
#include "BSMovie/CBSAStream.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MediaLocationController.h"
#include "Fallout Shared/MediaSet.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataSound.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FAMThread.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/ATL/statreg.h"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "SDK/DirectShow/CAsyncReader.h"
#include "SDK/DirectShow/CBSAReader.h"
#include "SDK/DirectShow/CBaseFilter.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/control.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// This file's copy of ATL statreg.h's keyword pointers: eight .rdata dwords right after CLSID_FilterGraph, pointing to
// "S", "M", "D", "B", "Val", "ForceRemove", "NoRemove" and "Delete", which no code reads.
namespace ATL {} // namespace ATL

FAMThread::TrackData FAMThread::kNewTrack;
BSCriticalSection FalloutAudioMedia::kSoundLock;
BSSystemEvent* FalloutAudioMedia::hUpdate = new BSSystemEvent(false, false);

void FalloutAudioMedia::MediaPause() {
    FalloutAudioMedia::MediaAccessEnter();
    FalloutAudioMedia::cMediaAFlags |= MEDIA_FLAG_PAUSED;
    FalloutAudioMedia::cMediaBFlags |= MEDIA_FLAG_PAUSED;
    FalloutAudioMedia::MediaAccessLeave();
}

void FalloutAudioMedia::MediaUnPause() {
    FalloutAudioMedia::MediaAccessEnter();
    FalloutAudioMedia::cMediaAFlags &= ~MEDIA_FLAG_PAUSED;
    FalloutAudioMedia::cMediaBFlags &= ~MEDIA_FLAG_PAUSED;
    FalloutAudioMedia::MediaAccessLeave();
}

void FalloutAudioMedia::MediaAccessEnter() {
    FalloutAudioMedia::kSoundLock.Enter(nullptr);
}

void FalloutAudioMedia::MediaAccessLeave() {
    FalloutAudioMedia::kSoundLock.Leave();
}

void FalloutAudioMedia::MediaOpen(
    MEDIATYPE aeMediaType,
    const char* apFileName,
    uint32_t auiFadeTime,
    bool abLoop,
    bool abForce,
    float afAttenuation,
    uint32_t auiSynchTime
) {
    FalloutAudioMedia::MediaAccessEnter();
    if (aeMediaType == MEDIA_NONE) {
        FalloutAudioMedia::MediaClear();
        FalloutAudioMedia::MediaAccessLeave();
    }
    else if ((aeMediaType == MEDIA_MAINMENU || !FalloutAudio::bMainMenuMusic) && apFileName && *apFileName) {
        if (abForce ||
            ((aeMediaType >= FalloutAudioMedia::eMediaAType || FalloutAudioMedia::eMediaNextTrack != 1) &&
             (aeMediaType >= FalloutAudioMedia::eMediaBType || FalloutAudioMedia::eMediaNextTrack))) {
            {
                BSFile fileCheck(apFileName, NiFile::READ_ONLY, 0x4000u, false);
                if (!fileCheck.BSFile::Exist()) {
                    Warning("FalloutAudioMedia: File '%s' does not exist!", apFileName);
                    FalloutAudioMedia::MediaAccessLeave();
                    return;
                }
            }
            uint32_t ilength = BSAudio::QInstance().GetSynchTime();
            if (FalloutAudioMedia::eMediaNextTrack) {
                FalloutAudioMedia::fMediaBVolume = 0.0f;
                FalloutAudioMedia::fMediaBFadeStartVolume = 0.0f;
                FalloutAudioMedia::fMediaBFadeEndVolume = FalloutAudioMedia::MediaVolume(aeMediaType, afAttenuation);
                FalloutAudioMedia::fMediaBAttenuation = afAttenuation;
                FalloutAudioMedia::cMediaBFlags = (abLoop ? MEDIA_FLAG_LOOP : 0) | MEDIA_FLAG_FADING;
                BSstrcpy(FalloutAudioMedia::pMediaBFileName, 260, apFileName);
                FalloutAudioMedia::eMediaBType = aeMediaType;
                FalloutAudioMedia::uiMediaBSynchStartTime = auiSynchTime;
                FalloutAudioMedia::uiMediaBFadeTime = auiFadeTime;
                FalloutAudioMedia::uiMediaBFadeEnd = auiFadeTime + ilength;
                FalloutAudioMedia::uiMediaBLastUpdate = 0;
                FalloutAudioMedia::uiMediaBID = uiMediaNextID;
                FAMThread::kNewTrack.ID = uiMediaNextID++;
                FAMThread::kNewTrack.Track = static_cast<MEDIATRACK>(FalloutAudioMedia::eMediaNextTrack);
                (s_thread + FalloutAudioMedia::eMediaNextTrack)->AddNewTrack(&FAMThread::kNewTrack);
                FalloutAudioMedia::eMediaNextTrack = 0;
                FalloutAudioMedia::cMediaAFlags |= MEDIA_FLAG_FADE_OUT | MEDIA_FLAG_FADING;
                FalloutAudioMedia::fMediaAFadeStartVolume = FalloutAudioMedia::fMediaAVolume;
                FalloutAudioMedia::fMediaAFadeEndVolume = 0.0f;
                FalloutAudioMedia::uiMediaAFadeTime = FalloutAudioMedia::uiMediaBFadeTime;
                FalloutAudioMedia::uiMediaAFadeEnd = FalloutAudioMedia::uiMediaBFadeEnd;
            }
            else {
                FalloutAudioMedia::fMediaAVolume = 0.0f;
                FalloutAudioMedia::fMediaAFadeStartVolume = 0.0f;
                FalloutAudioMedia::fMediaAFadeEndVolume = FalloutAudioMedia::MediaVolume(aeMediaType, afAttenuation);
                FalloutAudioMedia::fMediaAAttenuation = afAttenuation;
                FalloutAudioMedia::cMediaAFlags = (abLoop ? MEDIA_FLAG_LOOP : 0) | MEDIA_FLAG_FADING;
                BSstrcpy(FalloutAudioMedia::pMediaAFileName, 260, apFileName);
                FalloutAudioMedia::eMediaAType = aeMediaType;
                FalloutAudioMedia::uiMediaASynchStartTime = auiSynchTime;
                FalloutAudioMedia::uiMediaAFadeTime = auiFadeTime;
                FalloutAudioMedia::uiMediaAFadeEnd = auiFadeTime + ilength;
                FalloutAudioMedia::uiMediaALastUpdate = 0;
                FalloutAudioMedia::uiMediaAID = uiMediaNextID;
                FAMThread::kNewTrack.ID = uiMediaNextID++;
                FAMThread::kNewTrack.Track = static_cast<MEDIATRACK>(FalloutAudioMedia::eMediaNextTrack);
                (s_thread + FalloutAudioMedia::eMediaNextTrack)->AddNewTrack(&FAMThread::kNewTrack);
                FalloutAudioMedia::eMediaNextTrack = 1;
                FalloutAudioMedia::cMediaBFlags |= MEDIA_FLAG_FADE_OUT | MEDIA_FLAG_FADING;
                FalloutAudioMedia::fMediaBFadeStartVolume = FalloutAudioMedia::fMediaBVolume;
                FalloutAudioMedia::fMediaBFadeEndVolume = 0.0f;
                FalloutAudioMedia::uiMediaBFadeTime = FalloutAudioMedia::uiMediaAFadeTime;
                FalloutAudioMedia::uiMediaBFadeEnd = FalloutAudioMedia::uiMediaAFadeEnd;
            }
            FalloutAudioMedia::MediaAccessLeave();
        }
        else {
            Warning("FalloutAudioMedia: Trying to open media out of priority");
            FalloutAudioMedia::MediaAccessLeave();
        }
    }
    else {
        FalloutAudioMedia::MediaAccessLeave();
    }
}

void FalloutAudioMedia::MediaOpen(BGSMusicType* aiMusicType) {
    if (aiMusicType) {
        float afAttenuation = aiMusicType->GetAttenuation();
        bool bLooping = aiMusicType->GetLooping();
        char* pcEntry = aiMusicType->GetRandomEntry();
        FalloutAudioMedia::MediaOpen(MEDIA_SPECIAL, pcEntry, 1000u, bLooping, false, afAttenuation, 0);
    }
}

void FalloutAudioMedia::MediaPlay() {
    MediaAccessEnter();
    cMediaAFlags &= ~MEDIA_FLAG_STOPPED;
    cMediaBFlags &= ~MEDIA_FLAG_STOPPED;
    MediaAccessLeave();
}

// Fades out the playing track: a nonzero eMediaNextTrack stops track A, otherwise track B.
void FalloutAudioMedia::MediaStop() {
    MediaAccessEnter();
    if (eMediaNextTrack) {
        cMediaAFlags |= MEDIA_FLAG_STOPPED;
        fMediaAVolume = MediaVolume(eMediaAType, fMediaAAttenuation);
        uiMediaASynchStartTime = 0;
        uiMediaAFadeTime = 0;
        uiMediaAFadeEnd = 0;
        uiMediaACurrentPosition = 0;
        cMediaBFlags |= MEDIA_FLAG_END_STREAM;
    }
    else {
        cMediaBFlags |= MEDIA_FLAG_STOPPED;
        fMediaBVolume = MediaVolume(eMediaBType, fMediaBAttenuation);
        uiMediaBSynchStartTime = 0;
        uiMediaBFadeTime = 0;
        uiMediaBFadeEnd = 0;
        uiMediaBCurrentPosition = 0;
        cMediaAFlags |= MEDIA_FLAG_END_STREAM;
    }
    MediaAccessLeave();
}

void FalloutAudioMedia::MediaMenuPause() {
    MediaAccessEnter();
    bMediaMenuPaused = true;
    MediaAccessLeave();
}

void FalloutAudioMedia::MediaMenuUnPause() {
    MediaAccessEnter();
    bMediaMenuPaused = false;
    MediaAccessLeave();
}

// Whether a track of aeType (any type when MEDIA_STANDBY) is playing; with abIncludeFading a track fading out counts as
// playing.
bool FalloutAudioMedia::MediaIsPlaying(MEDIATYPE aeType, bool abIncludeFading) {
    bool bPlaying;
    MediaAccessEnter();
    if (abIncludeFading) {
        if (aeType) {
            bPlaying = (uiMediaAID && eMediaAType == aeType && !(cMediaAFlags & MEDIA_FLAG_STOPPED)) ||
                (uiMediaBID && eMediaBType == aeType && !(cMediaBFlags & MEDIA_FLAG_STOPPED));
        }
        else {
            bPlaying = (uiMediaAID && !(cMediaAFlags & MEDIA_FLAG_STOPPED)) ||
                (uiMediaBID && !(cMediaBFlags & MEDIA_FLAG_STOPPED));
        }
    }
    else if (aeType) {
        bPlaying = (uiMediaAID && eMediaAType == aeType && !(cMediaAFlags & MEDIA_FLAG_FADE_OUT)) ||
            (uiMediaBID && eMediaBType == aeType && !(cMediaBFlags & MEDIA_FLAG_FADE_OUT));
    }
    else {
        bPlaying = (uiMediaAID && !(cMediaAFlags & MEDIA_FLAG_FADE_OUT)) ||
            (uiMediaBID && !(cMediaBFlags & MEDIA_FLAG_FADE_OUT));
    }
    MediaAccessLeave();
    return bPlaying;
}

void FalloutAudioMedia::Update() {
    FalloutAudioMedia::hUpdate->Set();
    if (!FalloutAudioMedia::bRadioOn) {
        if (FalloutAudioMedia::MediaIsPlaying(MEDIA_SPECIAL, true) ||
            FalloutAudioMedia::MediaIsPlaying(MEDIA_MAINMENU, true)) {
            if (FalloutAudioMedia::pCurrentMediaController) {
                FalloutAudioMedia::pCurrentMediaController = nullptr;
            }
        }
        else {
            MEDIATYPE eLatestType = FalloutAudioMedia::eMediaNextTrack ? FalloutAudioMedia::eMediaAType
                                                                       : FalloutAudioMedia::eMediaBType;
            uint32_t uiTicks = BSAudio::QInstance().GetSynchTime();
            bool bInCombat = pCombatManager ? pCombatManager->ShouldPlayCombatMusic() : false;
            if (!Interface::IsMenuVisible(Interface::eLoadingMenu) &&
                (!FalloutAudioMedia::pCurrentBattleMediaSet ||
                 !FalloutAudioMedia::pCurrentBattleMediaSet->Update(uiTicks, bInCombat, nullptr))) {
                if (FalloutAudioMedia::pCurrentBattleMediaSet) {
                    FalloutAudioMedia::pCurrentBattleMediaSet = nullptr;
                }
                if (!pPlayer ||
                    (pPlayer->GetMediaLocationController() &&
                     pPlayer->GetMediaLocationController()->Update(bInCombat))) {
                    if (pPlayer) {
                        if (pPlayer->IsDead(false)) {
                            FalloutAudioMedia::MediaFadeOut(1000, MEDIA_STANDBY);
                        }
                        else if (
                            FalloutAudioMedia::MediaIsPlaying(MEDIA_STANDBY, true) &&
                            !FalloutAudioMedia::MediaIsRadio() &&
                            !FalloutAudioMedia::pCurrentMediaController
                        ) {
                            FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_BATTLE);
                            FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_LOCATION_BATTLE);
                            FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_LOCATION);
                            FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_DUNGEON);
                        }
                    }
                }
                else {
                    FalloutAudioMedia::pCurrentMediaController = nullptr;
                    if (FalloutAudio::pCurrentSpace && FalloutAudio::pCurrentSpace->pSoundRegion) {
                        TESRegionDataSound* pRSD;
                        if (FalloutAudioMedia::MediaIsPlaying(MEDIA_LOCATION, true)) {
                            FalloutAudioMedia::MediaFadeOut(1000, MEDIA_LOCATION);
                        }
                        if (FalloutAudioMedia::MediaIsPlaying(MEDIA_DUNGEON, true)) {
                            FalloutAudioMedia::MediaFadeOut(1000, MEDIA_DUNGEON);
                        }
                        TESRegion* psoundRegion = FalloutAudio::pCurrentSpace->pSoundRegion;
                        if (FalloutAudioMedia::pCurrentRegionSoundData) {
                            pRSD = FalloutAudioMedia::pCurrentRegionSoundData;
                        }
                        else {
                            pRSD = static_cast<TESRegionDataSound*>(
                                psoundRegion->pDataList->Find(REGION_DATA_SOUND_ID)
                            );
                        }
                        if (pRSD) {
                            if (bInCombat && !FalloutAudioMedia::pCurrentBattleMediaSet) {
                                FalloutAudioMedia::pCurrentBattleMediaSet = pRSD->GetMediaSetBattle();
                                if (FalloutAudioMedia::pCurrentBattleMediaSet) {
                                    FalloutAudioMedia::pCurrentBattleMediaSet->Reset();
                                }
                            }
                            if (!FalloutAudioMedia::pCurrentBattleMediaSet ||
                                !FalloutAudioMedia::pCurrentBattleMediaSet->Update(uiTicks, bInCombat, nullptr)) {
                                FalloutAudioMedia::pCurrentBattleMediaSet = nullptr;
                                if (pRSD->GetMediaSetIncidentals() && eLatestType == MEDIA_STANDBY) {
                                    pRSD->GetMediaSetIncidentals()->Update(uiTicks, false, nullptr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void FalloutAudioMedia::MediaClose() {
    FalloutAudioMedia::MediaAccessEnter();
    FalloutAudioMedia::MediaClear();
    FalloutAudioMedia::MediaAccessLeave();
}

float FalloutAudioMedia::MediaVolume(MEDIATYPE aeMediaType, float attenuation) {
    float fVolume = 0.0f;
    if (aeMediaType == MEDIA_MAINMENU) {
        fVolume = FalloutAudio::fMainMenuMusicVolume.Float();
    }
    else if (aeMediaType == MEDIA_RADIO) {
        fVolume = static_cast<BSWin32Audio&>(BSAudio::QInstance()).GetRadioVolume();
    }
    else {
        fVolume = static_cast<BSWin32Audio&>(BSAudio::QInstance()).GetMusicVolume();
    }
    float fLevel = std::log10(fVolume) * 2000.0 + attenuation * 100.0;
    if (fLevel < -10000.0) {
        fLevel = -10000.0f;
    }
    else if (fLevel > 0.0) {
        fLevel = 0.0f;
    }
    float fExponent = fLevel / 2000.0;
    fVolume = std::pow(10.0f, fExponent);
    fVolume = static_cast<BSWin32Audio&>(BSAudio::QInstance()).GetMasterVolume() * fVolume;
    if (fVolume < 9.999999747378752e-05) {
        fVolume = 0.0001f;
    }
    else if (fVolume > 100.0) {
        fVolume = 100.0f;
    }
    return fVolume;
}

bool FalloutAudioMedia::MediaIsRadio() {
    MediaAccessEnter();
    bool bRadio = (uiMediaAID && eMediaAType == MEDIA_RADIO) || (uiMediaBID && eMediaBType == MEDIA_RADIO) || bRadioOn;
    MediaAccessLeave();
    return bRadio;
}

void FalloutAudioMedia::MediaClear() {
    fMediaAVolume = 0.0f;
    fMediaAFadeStartVolume = 0.0f;
    fMediaAFadeEndVolume = 0.0f;
    fMediaAAttenuation = 0.0f;
    cMediaAFlags = 0;
    BSstrcpy(pMediaAFileName, 260, "NONE");
    eMediaAType = MEDIA_STANDBY;
    uiMediaASynchStartTime = 0;
    uiMediaACurrentPosition = 0;
    uiMediaADuration = 0;
    uiMediaASubDuration = 0;
    uiMediaAFadeTime = 0;
    uiMediaAFadeEnd = 0;
    uiMediaALastUpdate = 0;
    uiMediaAID = 0;
    fMediaBVolume = 0.0f;
    fMediaBFadeStartVolume = 0.0f;
    fMediaBFadeEndVolume = 0.0f;
    fMediaBAttenuation = 0.0f;
    cMediaBFlags = 0;
    BSstrcpy(pMediaBFileName, 260, "NONE");
    eMediaBType = MEDIA_STANDBY;
    uiMediaBSynchStartTime = 0;
    uiMediaBCurrentPosition = 0;
    uiMediaBDuration = 0;
    uiMediaBSubDuration = 0;
    uiMediaBFadeTime = 0;
    uiMediaBFadeEnd = 0;
    uiMediaBLastUpdate = 0;
    uiMediaBID = 0;
    eMediaNextTrack = 0;
}

void FalloutAudioMedia::MediaFadeOut(int32_t aiFadeTime, MEDIATYPE aeType) {
    FalloutAudioMedia::MediaAccessEnter();
    uint32_t uiSynchTime = BSAudio::QInstance().GetSynchTime();
    if ((FalloutAudioMedia::eMediaAType == aeType || aeType == MEDIA_STANDBY) &&
        (FalloutAudioMedia::cMediaAFlags & MEDIA_FLAG_FADE_OUT) == 0) {
        FalloutAudioMedia::cMediaAFlags |= MEDIA_FLAG_FADE_OUT | MEDIA_FLAG_FADING;
        FalloutAudioMedia::fMediaAFadeStartVolume = FalloutAudioMedia::fMediaAVolume;
        FalloutAudioMedia::fMediaAFadeEndVolume = 0.0f;
        FalloutAudioMedia::uiMediaAFadeTime = aiFadeTime;
        FalloutAudioMedia::uiMediaAFadeEnd = aiFadeTime + uiSynchTime;
    }
    if ((FalloutAudioMedia::eMediaBType == aeType || aeType == MEDIA_STANDBY) &&
        (FalloutAudioMedia::cMediaBFlags & MEDIA_FLAG_FADE_OUT) == 0) {
        FalloutAudioMedia::cMediaBFlags |= MEDIA_FLAG_FADE_OUT | MEDIA_FLAG_FADING;
        FalloutAudioMedia::fMediaBFadeStartVolume = FalloutAudioMedia::fMediaBVolume;
        FalloutAudioMedia::fMediaBFadeEndVolume = 0.0f;
        FalloutAudioMedia::uiMediaBFadeTime = aiFadeTime;
        FalloutAudioMedia::uiMediaBFadeEnd = aiFadeTime + uiSynchTime;
    }
    FalloutAudioMedia::MediaAccessLeave();
}

uint32_t FalloutAudioMedia::MediaGetSubDuration(MEDIATYPE aeType) {
    FalloutAudioMedia::MediaAccessEnter();
    uint32_t uiSubDuration = 0;
    if (aeType == MEDIA_STANDBY) {
        if (FalloutAudioMedia::eMediaNextTrack == 1 && FalloutAudioMedia::uiMediaAID) {
            uiSubDuration = FalloutAudioMedia::uiMediaASubDuration;
        }
        else if (!FalloutAudioMedia::eMediaNextTrack && FalloutAudioMedia::uiMediaBID) {
            uiSubDuration = FalloutAudioMedia::uiMediaBSubDuration;
        }
    }
    if (FalloutAudioMedia::eMediaNextTrack == 1 &&
        FalloutAudioMedia::uiMediaAID &&
        FalloutAudioMedia::eMediaAType == aeType) {
        uiSubDuration = FalloutAudioMedia::uiMediaASubDuration;
    }
    else if (
        !FalloutAudioMedia::eMediaNextTrack && FalloutAudioMedia::uiMediaBID && FalloutAudioMedia::eMediaBType == aeType
    ) {
        uiSubDuration = FalloutAudioMedia::uiMediaBSubDuration;
    }
    FalloutAudioMedia::MediaAccessLeave();
    return uiSubDuration;
}

uint32_t FalloutAudioMedia::MediaGetPosition(MEDIATYPE aeType) {
    FalloutAudioMedia::MediaAccessEnter();
    uint32_t uiPosition = 0;
    if (aeType == MEDIA_STANDBY) {
        if (FalloutAudioMedia::eMediaNextTrack == 1 && FalloutAudioMedia::uiMediaAID) {
            uiPosition = FalloutAudioMedia::uiMediaACurrentPosition;
        }
        else if (!FalloutAudioMedia::eMediaNextTrack && FalloutAudioMedia::uiMediaBID) {
            uiPosition = FalloutAudioMedia::uiMediaBCurrentPosition;
        }
    }
    if (FalloutAudioMedia::eMediaNextTrack == 1 &&
        FalloutAudioMedia::uiMediaAID &&
        FalloutAudioMedia::eMediaAType == aeType) {
        uiPosition = FalloutAudioMedia::uiMediaACurrentPosition;
    }
    else if (
        !FalloutAudioMedia::eMediaNextTrack && FalloutAudioMedia::uiMediaBID && FalloutAudioMedia::eMediaBType == aeType
    ) {
        uiPosition = FalloutAudioMedia::uiMediaBCurrentPosition;
    }
    FalloutAudioMedia::MediaAccessLeave();
    return uiPosition;
}

void FalloutAudioMedia::Startup() {
    FAMThread* pThreads;
    // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and Shutdown's
    // delete[] goes through it.
    pThreads = new FAMThread[2];
    s_thread = pThreads;
    s_thread[0].Initialize(BSThread::SMALL_STACK_SIZE, "FalloutAudioMedia (1/2)", false);
    s_thread[0].SetThreadPriority(1);
    s_thread[1].Initialize(BSThread::SMALL_STACK_SIZE, "FalloutAudioMedia (2/2)", false);
    s_thread[1].SetThreadPriority(1);
    s_thread[0].SetThreadProcessor(0);
    s_thread[1].SetThreadProcessor(0);
}

void FalloutAudioMedia::Shutdown() {
    FAMThread* pThreads = s_thread;
    if (pThreads) {
        // delete[] s_thread: reads new[]'s count cookie (the untyped word before the elements); with it set the
        // elements are destroyed through delete[], else the bare block is freed.
        if (reinterpret_cast<size_t*>(pThreads)[-1]) {
            delete[] pThreads;
        }
        else {
            ourFree(reinterpret_cast<size_t*>(pThreads) - 1);
        }
    }
    s_thread = nullptr;
}

uint32_t FalloutAudioMedia::MediaGetDuration(MEDIATYPE aeType) {
    FalloutAudioMedia::MediaAccessEnter();
    uint32_t uiDuration = 0;
    if (aeType == MEDIA_STANDBY) {
        if (FalloutAudioMedia::eMediaNextTrack == 1 && FalloutAudioMedia::uiMediaAID) {
            uiDuration = FalloutAudioMedia::uiMediaADuration;
        }
        else if (!FalloutAudioMedia::eMediaNextTrack && FalloutAudioMedia::uiMediaBID) {
            uiDuration = FalloutAudioMedia::uiMediaBDuration;
        }
    }
    if (FalloutAudioMedia::eMediaNextTrack == 1 &&
        FalloutAudioMedia::uiMediaAID &&
        FalloutAudioMedia::eMediaAType == aeType) {
        uiDuration = FalloutAudioMedia::uiMediaADuration;
    }
    else if (
        !FalloutAudioMedia::eMediaNextTrack && FalloutAudioMedia::uiMediaBID && FalloutAudioMedia::eMediaBType == aeType
    ) {
        uiDuration = FalloutAudioMedia::uiMediaBDuration;
    }
    FalloutAudioMedia::MediaAccessLeave();
    return uiDuration;
}

uint32_t FalloutAudioMedia::ThreadProc(FalloutAudioMedia::MediaThreadInfo* input) {
    int32_t lEventCode;
    intptr_t lParam1;
    intptr_t lParam2;
    HRESULT hr;
    IFilterGraph* pFilterGraph;
    IMediaControl* pMediaControl;
    IGraphBuilder* pGraphBuilder;
    char cFileName[268];
    IBasicAudio* pBasicAudio;
    double dTime;
    IMediaEvent* pMediaEvent;
    IMediaPosition* pAudio;
    MEDIATRACK pMediaSoundFile = input->Track;
    uint32_t uiTrackID = input->ID;
    bool bStream = true;
    uint32_t uiTicks = 0;
    float fVolume = 0.0f;
    float fFadeStartVolume = 0.0f;
    float fTrackAttenuation = 0.0f;
    char cTrackFlags = 0;
    MEDIATYPE eTrackType = MEDIA_NONE;
    uint32_t uiTrackSynchStartTime = 0;
    uint32_t uiTrackCurrentPosition = 0;
    uint32_t uiTrackDuration = 0;
    uint32_t uiTrackSubDuration = 0;
    uint32_t uiTrackFadeTime = 0;
    uint32_t uiTrackFadeEnd = 0;
    uint32_t uiDeltaTest = BSMAX(500, FalloutAudio::iMusicSynchOverride.Int() + 500);
    FalloutAudioMedia::MediaAccessEnter();
    ++uiMediaThreadCount;
    if ((pMediaSoundFile == MEDIATRACK_A &&
         (uiTrackID != FalloutAudioMedia::uiMediaAID ||
          (FalloutAudioMedia::cMediaAFlags & MEDIA_FLAG_END_STREAM) != 0)) ||
        (pMediaSoundFile == MEDIATRACK_B &&
         (uiTrackID != FalloutAudioMedia::uiMediaBID ||
          (FalloutAudioMedia::cMediaBFlags & MEDIA_FLAG_END_STREAM) != 0))) {
        bStream = false;
    }
    uiTicks = BSAudio::QInstance().GetSynchTime();
    if (pMediaSoundFile) {
        FalloutAudioMedia::fMediaBVolume = fVolume;
        FalloutAudioMedia::uiMediaBLastUpdate = uiTicks;
        FalloutAudioMedia::uiMediaBCurrentPosition = uiTrackCurrentPosition;
        FalloutAudioMedia::uiMediaBDuration = uiTrackDuration;
        FalloutAudioMedia::uiMediaBSubDuration = uiTrackSubDuration;
        fFadeStartVolume = FalloutAudioMedia::fMediaBFadeStartVolume;
        fTrackAttenuation = FalloutAudioMedia::fMediaBAttenuation;
        cTrackFlags = FalloutAudioMedia::cMediaBFlags;
        eTrackType = FalloutAudioMedia::eMediaBType;
        uiTrackSynchStartTime = FalloutAudioMedia::uiMediaBSynchStartTime;
        uiTrackFadeTime = FalloutAudioMedia::uiMediaBFadeTime;
        uiTrackFadeEnd = FalloutAudioMedia::uiMediaBFadeEnd;
        BSstrcpy(cFileName, 260, FalloutAudioMedia::pMediaBFileName);
    }
    else {
        FalloutAudioMedia::fMediaAVolume = fVolume;
        FalloutAudioMedia::uiMediaALastUpdate = uiTicks;
        FalloutAudioMedia::uiMediaACurrentPosition = uiTrackCurrentPosition;
        FalloutAudioMedia::uiMediaADuration = uiTrackDuration;
        FalloutAudioMedia::uiMediaASubDuration = uiTrackSubDuration;
        fFadeStartVolume = FalloutAudioMedia::fMediaAFadeStartVolume;
        fTrackAttenuation = FalloutAudioMedia::fMediaAAttenuation;
        cTrackFlags = FalloutAudioMedia::cMediaAFlags;
        eTrackType = FalloutAudioMedia::eMediaAType;
        uiTrackSynchStartTime = FalloutAudioMedia::uiMediaASynchStartTime;
        uiTrackFadeTime = FalloutAudioMedia::uiMediaAFadeTime;
        uiTrackFadeEnd = FalloutAudioMedia::uiMediaAFadeEnd;
        BSstrcpy(cFileName, 260, FalloutAudioMedia::pMediaAFileName);
    }
    FalloutAudioMedia::MediaAccessLeave();
    pFilterGraph = nullptr;
    pGraphBuilder = nullptr;
    pBasicAudio = nullptr;
    CAsyncStream* pAsyncStream = nullptr;
    CBSAReader* pReader = nullptr;
    pAudio = nullptr;
    pMediaControl = nullptr;
    pMediaEvent = nullptr;
    dTime = 0.0;
    CBSAStream* pMediaSoundBuffer = new CBSAStream(cFileName);
    pAsyncStream = pMediaSoundBuffer;
    if (pMediaSoundBuffer->IsOpen()) {
        hr = 0;
        CMediaType kMediaType;
        kMediaType.majortype = CLSID_MFMediaType_Stream;
        if (std::strstr(cFileName, ".mp3")) {
            kMediaType.subtype = MEDIASUBTYPE_MPEG1Audio;
        }
        else {
            bStream = false;
            Warning("FalloutAudioMedia: file format wrong %s", cFileName);
        }
        if (bStream) {
            int32_t hrAddFilter;
            pReader = new CBSAReader(pAsyncStream, &kMediaType, &hr);
            static_cast<IUnknown*>(
                &static_cast<IBaseFilter&>(static_cast<CBaseFilter&>(static_cast<CAsyncReader&>(*pReader)))
            )
                ->AddRef();
            CoInitializeEx(nullptr, 0);
            hr = CoCreateInstance(
                &CLSID_FilterGraph,
                nullptr,
                1u,
                &CLSID_IFilterGraph,
                reinterpret_cast<void**>(&pFilterGraph)
            );
            if (hr < 0) {
                // The filter graph always exists on Windows. Where there is none (no DirectShow stand-in yet) the track
                // does not play.
                bStream = false;
            }
            else {
                if (pReader) {
                    hrAddFilter = pFilterGraph->AddFilter(pReader, nullptr);
                }
                else {
                    hrAddFilter = pFilterGraph->AddFilter(nullptr, nullptr);
                }
                hr = hrAddFilter;
                if (hrAddFilter < 0) {
                    bStream = false;
                }
                hr = pFilterGraph->QueryInterface(CLSID_IGraphBuilder, reinterpret_cast<void**>(&pGraphBuilder));
                if (hr < 0) {
                    bStream = false;
                }
                CBasePin* pBasePin = pReader->GetPin(0);
                IPin* pPin = static_cast<IPin*>(pBasePin);
                hr = pGraphBuilder->Render(pPin);
                if (hr < 0) {
                    bStream = false;
                }
                hr = pGraphBuilder->QueryInterface(CLSID_IBasicAudio, reinterpret_cast<void**>(&pBasicAudio));
                if (hr < 0) {
                    bStream = false;
                }
                hr = pFilterGraph->QueryInterface(CLSID_IMediaControl, reinterpret_cast<void**>(&pMediaControl));
                if (hr < 0) {
                    bStream = false;
                }
                hr = pGraphBuilder->QueryInterface(CLSID_IMediaPosition, reinterpret_cast<void**>(&pAudio));
                if (hr < 0) {
                    bStream = false;
                }
                hr = pGraphBuilder->QueryInterface(CLSID_IMediaEvent, reinterpret_cast<void**>(&pMediaEvent));
                if (hr < 0) {
                    bStream = false;
                }
            }
        }
    }
    else {
        Warning("FalloutAudioMedia: file not found");
        bStream = false;
    }
    if (pFilterGraph && pAudio) {
        pAudio->get_Duration(&dTime);
        uiTrackSubDuration = static_cast<int64_t>(dTime * 1000.0);
    }
    else {
        uiTrackSubDuration = 0;
    }
    while (bStream) {
        FalloutAudioMedia::hUpdate->Wait(100u);
        FalloutAudioMedia::MediaAccessEnter();
        uiTicks = BSAudio::QInstance().GetSynchTime();
        if (pFilterGraph && pAudio) {
            pAudio->get_CurrentPosition(&dTime);
            uiTrackCurrentPosition = static_cast<int64_t>(dTime * 1000.0);
        }
        else {
            uiTrackCurrentPosition = 0;
        }
        if (pFilterGraph && pAudio) {
            pAudio->get_Duration(&dTime);
            uiTrackDuration = static_cast<int64_t>(dTime * 1000.0);
        }
        else {
            uiTrackDuration = 0;
        }
        if (pMediaSoundFile) {
            FalloutAudioMedia::fMediaBVolume = fVolume;
            FalloutAudioMedia::uiMediaBLastUpdate = uiTicks;
            FalloutAudioMedia::uiMediaBCurrentPosition = uiTrackCurrentPosition;
            FalloutAudioMedia::uiMediaBDuration = uiTrackDuration;
            FalloutAudioMedia::uiMediaBSubDuration = uiTrackSubDuration;
            fFadeStartVolume = FalloutAudioMedia::fMediaBFadeStartVolume;
            fTrackAttenuation = FalloutAudioMedia::fMediaBAttenuation;
            cTrackFlags = FalloutAudioMedia::cMediaBFlags;
            eTrackType = FalloutAudioMedia::eMediaBType;
            uiTrackSynchStartTime = FalloutAudioMedia::uiMediaBSynchStartTime;
            uiTrackFadeTime = FalloutAudioMedia::uiMediaBFadeTime;
            uiTrackFadeEnd = FalloutAudioMedia::uiMediaBFadeEnd;
        }
        else {
            FalloutAudioMedia::fMediaAVolume = fVolume;
            FalloutAudioMedia::uiMediaALastUpdate = uiTicks;
            FalloutAudioMedia::uiMediaACurrentPosition = uiTrackCurrentPosition;
            FalloutAudioMedia::uiMediaADuration = uiTrackDuration;
            FalloutAudioMedia::uiMediaASubDuration = uiTrackSubDuration;
            fFadeStartVolume = FalloutAudioMedia::fMediaAFadeStartVolume;
            fTrackAttenuation = FalloutAudioMedia::fMediaAAttenuation;
            cTrackFlags = FalloutAudioMedia::cMediaAFlags;
            eTrackType = FalloutAudioMedia::eMediaAType;
            uiTrackSynchStartTime = FalloutAudioMedia::uiMediaASynchStartTime;
            uiTrackFadeTime = FalloutAudioMedia::uiMediaAFadeTime;
            uiTrackFadeEnd = FalloutAudioMedia::uiMediaAFadeEnd;
        }
        FalloutAudioMedia::MediaAccessLeave();
        float fVolumeGoal;
        if ((cTrackFlags & MEDIA_FLAG_FADE_OUT) != 0) {
            fVolumeGoal = 0.0f;
        }
        else {
            fVolumeGoal = FalloutAudioMedia::MediaVolume(eTrackType, fTrackAttenuation);
        }
        if ((cTrackFlags & MEDIA_FLAG_FADING) != 0 && uiTicks < uiTrackFadeEnd) {
            double dNewVolume;
            float fFadeClamped;
            float fFadeCapped;
            double dFadeFraction;
            if (uiTrackFadeTime) {
                dFadeFraction = (static_cast<double>(uiTrackFadeEnd) - static_cast<double>(uiTicks)) /
                    static_cast<double>(uiTrackFadeTime);
            }
            else {
                dFadeFraction = 0.0;
            }
            float fFadeTest = dFadeFraction;
            if (!(fFadeTest < 1.0)) {
                fFadeCapped = 1.0f;
            }
            else {
                fFadeCapped = dFadeFraction;
            }
            if (!(fFadeCapped > 0.0)) {
                fFadeClamped = 0.0f;
            }
            else {
                fFadeClamped = fFadeCapped;
            }
            if ((cTrackFlags & MEDIA_FLAG_FADE_OUT) != 0) {
                dNewVolume = fFadeClamped * fFadeStartVolume;
            }
            else {
                dNewVolume = (1.0 - fFadeClamped) * (fVolumeGoal - fFadeStartVolume) + fFadeStartVolume;
            }
            fVolume = dNewVolume;
        }
        else if (!(fVolumeGoal < static_cast<double>(fVolume))) {
            if (fVolumeGoal > static_cast<double>(fVolume)) {
                fVolume = fVolume + 0.05000000074505806;
                fVolume = !(fVolumeGoal > static_cast<double>(fVolume)) ? fVolumeGoal : fVolume;
            }
        }
        else {
            fVolume = fVolume - 0.05000000074505806;
            fVolume = !(fVolumeGoal < static_cast<double>(fVolume)) ? fVolumeGoal : fVolume;
        }
        if ((cTrackFlags & MEDIA_FLAG_FADE_OUT) != 0 && fVolume < 0.009999999776482582) {
            bStream = false;
        }
        if (pBasicAudio) {
            if (!(fVolume < 9.999999747378752e-05)) {
                int32_t iLevel = static_cast<int32_t>(std::log10(fVolume) * 2000.0);
                pBasicAudio->put_Volume(iLevel);
            }
            else {
                pBasicAudio->put_Volume(-10000);
            }
        }
        if (uiTrackSynchStartTime) {
            uint32_t uiPosition;
            int32_t iDelta;
            uint32_t i;
            if (pFilterGraph && pAudio) {
                pAudio->get_CurrentPosition(&dTime);
                uiPosition = static_cast<int64_t>(dTime * 1000.0);
            }
            else {
                uiPosition = 0;
            }
            uint32_t uiSynchTime = BSAudio::QInstance().GetSynchTime();
            for (i = uiSynchTime - uiTrackSynchStartTime; i > uiTrackDuration; i -= uiTrackDuration) {
                ;
            }
            if (uiSynchTime < uiTrackSynchStartTime) {
                i = 0;
                Warning(
                    "FalloutAudioMedia Synch Error: correct position is negative %i %i",
                    uiSynchTime,
                    uiTrackSynchStartTime
                );
            }
            if (uiPosition <= i) {
                iDelta = i - uiPosition;
            }
            else {
                iDelta = uiPosition - i;
            }
            if (iDelta > uiDeltaTest && (iDelta < 80000 || !uiPosition)) {
                Warning("FalloutAudioMedia Synch: %i = abs(-/+ %i +/- %i)", iDelta, i, uiPosition);
                uint32_t uiNewPosition = i + 50;
                if (pFilterGraph && pAudio) {
                    pAudio->put_CurrentPosition(static_cast<double>(uiNewPosition) / 1000.0);
                }
            }
        }
        if ((cTrackFlags & MEDIA_FLAG_STOPPED) != 0) {
            if (pGraphBuilder && pMediaControl) {
                pMediaControl->Stop();
            }
        }
        else if (
            (cTrackFlags & MEDIA_FLAG_PAUSED) != 0 ||
            FalloutAudioMedia::bMediaMenuPaused ||
            BSAudio::QInstance().IsSynchTimerPaused()
        ) {
            if (pGraphBuilder && pMediaControl) {
                pMediaControl->Pause();
            }
        }
        else if (pAudio && pMediaControl) {
            pMediaControl->Run();
        }
        if (pMediaEvent) {
            while (pMediaEvent->GetEvent(&lEventCode, &lParam1, &lParam2, 0) >= 0) {
                if (lEventCode > 0 && lEventCode <= 3) {
                    pMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);
                    if ((cTrackFlags & MEDIA_FLAG_LOOP) != 0) {
                        if (pGraphBuilder && pMediaControl) {
                            pMediaControl->Stop();
                        }

                        if (pFilterGraph && pAudio) {
                            pAudio->put_CurrentPosition(0.0);
                        }

                        if ((cTrackFlags & MEDIA_FLAG_STOPPED) == 0 && pAudio && pMediaControl) {
                            pMediaControl->Run();
                        }
                    }
                    else {
                        bStream = false;
                    }
                }
                pMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);
            }
        }
        else {
            bStream = false;
        }
        FalloutAudioMedia::MediaAccessEnter();
        if ((pMediaSoundFile == MEDIATRACK_A &&
             (uiTrackID != FalloutAudioMedia::uiMediaAID ||
              (FalloutAudioMedia::cMediaAFlags & MEDIA_FLAG_END_STREAM) != 0)) ||
            (pMediaSoundFile == MEDIATRACK_B &&
             (uiTrackID != FalloutAudioMedia::uiMediaBID ||
              (FalloutAudioMedia::cMediaBFlags & MEDIA_FLAG_END_STREAM) != 0))) {
            bStream = false;
        }
        FalloutAudioMedia::MediaAccessLeave();
    }
    FalloutAudioMedia::MediaAccessEnter();
    if (pMediaSoundFile || uiTrackID != FalloutAudioMedia::uiMediaAID) {
        if (pMediaSoundFile == MEDIATRACK_B && uiTrackID == FalloutAudioMedia::uiMediaBID) {
            FalloutAudioMedia::fMediaBVolume = 0.0f;
            FalloutAudioMedia::fMediaBFadeStartVolume = 0.0f;
            FalloutAudioMedia::fMediaBFadeEndVolume = 0.0f;
            FalloutAudioMedia::fMediaBAttenuation = 0.0f;
            FalloutAudioMedia::cMediaBFlags = 0;
            BSstrcpy(FalloutAudioMedia::pMediaBFileName, 260, "NONE");
            FalloutAudioMedia::eMediaBType = MEDIA_STANDBY;
            FalloutAudioMedia::uiMediaBSynchStartTime = 0;
            FalloutAudioMedia::uiMediaBCurrentPosition = 0;
            FalloutAudioMedia::uiMediaBDuration = 0;
            FalloutAudioMedia::uiMediaBSubDuration = 0;
            FalloutAudioMedia::uiMediaBFadeTime = 0;
            FalloutAudioMedia::uiMediaBFadeEnd = 0;
            FalloutAudioMedia::uiMediaBLastUpdate = 0;
            FalloutAudioMedia::uiMediaBID = 0;
        }
    }
    else {
        FalloutAudioMedia::fMediaAVolume = 0.0f;
        FalloutAudioMedia::fMediaAFadeStartVolume = 0.0f;
        FalloutAudioMedia::fMediaAFadeEndVolume = 0.0f;
        FalloutAudioMedia::fMediaAAttenuation = 0.0f;
        FalloutAudioMedia::cMediaAFlags = 0;
        BSstrcpy(FalloutAudioMedia::pMediaAFileName, 260, "NONE");
        FalloutAudioMedia::eMediaAType = MEDIA_STANDBY;
        FalloutAudioMedia::uiMediaASynchStartTime = 0;
        FalloutAudioMedia::uiMediaACurrentPosition = 0;
        FalloutAudioMedia::uiMediaADuration = 0;
        FalloutAudioMedia::uiMediaASubDuration = 0;
        FalloutAudioMedia::uiMediaAFadeTime = 0;
        FalloutAudioMedia::uiMediaAFadeEnd = 0;
        FalloutAudioMedia::uiMediaALastUpdate = 0;
        FalloutAudioMedia::uiMediaAID = 0;
    }
    --uiMediaThreadCount;
    FalloutAudioMedia::MediaAccessLeave();
    if (pMediaEvent) {
        pMediaEvent->Release();
        pMediaEvent = nullptr;
    }
    if (pAudio) {
        pAudio->Release();
        pAudio = nullptr;
    }
    if (pMediaControl) {
        pMediaControl->Release();
        pMediaControl = nullptr;
    }
    if (pBasicAudio) {
        pBasicAudio->Release();
        pBasicAudio = nullptr;
    }
    if (pGraphBuilder) {
        pGraphBuilder->Release();
        pGraphBuilder = nullptr;
    }
    if (pFilterGraph) {
        pFilterGraph->Release();
        pFilterGraph = nullptr;
    }
    if (pReader) {
        static_cast<IUnknown*>(
            &static_cast<IBaseFilter&>(static_cast<CBaseFilter&>(static_cast<CAsyncReader&>(*pReader)))
        )
            ->Release();
        pReader = nullptr;
    }
    if (pAsyncStream) {
        delete pAsyncStream;
    }
    return 0;
}

FAMThread::FAMThread() : m_event(new BSSystemEvent(false, false)), m_param{}, m_state(1) {
    m_havok_memory.pHavokThreadRouter = nullptr;
    m_havok_memory.pHavokThreadStackBuffer = nullptr;
    m_havok_memory.iThreadID = m_ThreadID;
}

FAMThread::~FAMThread() {
    m_state &= ~1u;
    AddNewTrack(&m_param);
    while ((m_state & 2) != 0) {
        BSSleep(10u);
    }
    delete m_event;
}

uint32_t FAMThread::ThreadProc() {
    m_state |= 2;
    while ((m_state & 1) != 0) {
        Wait();
        bhkThreadMemoryRouter::Initialize(&m_havok_memory);
        if ((m_state & 1) != 0) {
            FalloutAudioMedia::ThreadProc(&m_param);
        }
    }
    m_state &= ~2u;
    bhkThreadMemoryRouter::Quit(&m_havok_memory);
    return 0;
}

void FAMThread::AddNewTrack(FAMThread::TrackData* apTrack) {
    ourMemcpy(&m_param, apTrack, sizeof(m_param));
    m_event->Set();
}

void FAMThread::Wait() {
    m_event->Wait();
}

void FAMThread::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void* FAMThread::operator new[](size_t astSize) {
    return ourMalloc(static_cast<uint32_t>(astSize));
}

void FAMThread::operator delete[](void* apMemory) {
    ourFree(apMemory);
}
