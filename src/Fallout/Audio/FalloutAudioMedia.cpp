#include "Fallout/Audio/FalloutAudioMedia.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
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
    FalloutAudioMedia::cMediaAFlags |= 4;
    FalloutAudioMedia::cMediaBFlags |= 4;
    FalloutAudioMedia::MediaAccessLeave();
}

void FalloutAudioMedia::MediaUnPause() {
    FalloutAudioMedia::MediaAccessEnter();
    FalloutAudioMedia::cMediaAFlags &= ~4;
    FalloutAudioMedia::cMediaBFlags &= ~4;
    FalloutAudioMedia::MediaAccessLeave();
}

void FalloutAudioMedia::MediaAccessEnter() {
    (&FalloutAudioMedia::kSoundLock)->Enter(nullptr);
}

void FalloutAudioMedia::MediaAccessLeave() {
    (&FalloutAudioMedia::kSoundLock)->Leave();
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
    FNVD_INLINED_LOCAL(BSFile, fileCheck, std::nullopt, std::nullopt);
    FalloutAudioMedia::MediaAccessEnter();
    if (aeMediaType == MEDIA_NONE) {
        FalloutAudioMedia::MediaClear();
        FalloutAudioMedia::MediaAccessLeave();
    }
    else {
        if ((aeMediaType == MEDIA_MAINMENU || !FalloutAudio::bMainMenuMusic) && (apFileName && *apFileName)) {
            if (abForce ||
                ((aeMediaType >= FalloutAudioMedia::eMediaAType || FalloutAudioMedia::eMediaNextTrack != 1) &&
                 (aeMediaType >= FalloutAudioMedia::eMediaBType || FalloutAudioMedia::eMediaNextTrack))) {
                const char* pRealFilename = apFileName;
                ::new (&fileCheck) BSFile(apFileName, NiFile::READ_ONLY, 0x4000u, false);
                if (fileCheck.BSFile::Exist()) {
                    fileCheck.~BSFile();
                    uint32_t ilength = BSAudio::QInstance().GetSynchTime();
                    if (FalloutAudioMedia::eMediaNextTrack) {
                        FalloutAudioMedia::fMediaBVolume = 0.0f;
                        FalloutAudioMedia::fMediaBFadeStartVolume = 0.0f;
                        FalloutAudioMedia::fMediaBFadeEndVolume =
                            FalloutAudioMedia::MediaVolume(aeMediaType, afAttenuation);
                        FalloutAudioMedia::fMediaBAttenuation = afAttenuation;
                        FalloutAudioMedia::cMediaBFlags = (abLoop ? 32 : 0) | 16;
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
                        FalloutAudioMedia::cMediaAFlags |= 24u;
                        FalloutAudioMedia::fMediaAFadeStartVolume = FalloutAudioMedia::fMediaAVolume;
                        FalloutAudioMedia::fMediaAFadeEndVolume = 0.0f;
                        FalloutAudioMedia::uiMediaAFadeTime = FalloutAudioMedia::uiMediaBFadeTime;
                        FalloutAudioMedia::uiMediaAFadeEnd = FalloutAudioMedia::uiMediaBFadeEnd;
                    }
                    else {
                        FalloutAudioMedia::fMediaAVolume = 0.0f;
                        FalloutAudioMedia::fMediaAFadeStartVolume = 0.0f;
                        FalloutAudioMedia::fMediaAFadeEndVolume =
                            FalloutAudioMedia::MediaVolume(aeMediaType, afAttenuation);
                        FalloutAudioMedia::fMediaAAttenuation = afAttenuation;
                        FalloutAudioMedia::cMediaAFlags = (abLoop ? 32 : 0) | 16;
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
                        FalloutAudioMedia::cMediaBFlags |= 24u;
                        FalloutAudioMedia::fMediaBFadeStartVolume = FalloutAudioMedia::fMediaBVolume;
                        FalloutAudioMedia::fMediaBFadeEndVolume = 0.0f;
                        FalloutAudioMedia::uiMediaBFadeTime = FalloutAudioMedia::uiMediaAFadeTime;
                        FalloutAudioMedia::uiMediaBFadeEnd = FalloutAudioMedia::uiMediaAFadeEnd;
                    }
                    FalloutAudioMedia::MediaAccessLeave();
                }
                else {
                    Warning("FalloutAudioMedia: File '%s' does not exist!", apFileName);
                    FalloutAudioMedia::MediaAccessLeave();
                    fileCheck.~BSFile();
                }
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
}

void FalloutAudioMedia::MediaOpen(BGSMusicType* aiMusicType) {
    if (aiMusicType) {
        float afAttenuation = aiMusicType->GetAttenuation();
        bool Looping = aiMusicType->GetLooping();
        char* RandomEntry = aiMusicType->GetRandomEntry();
        FalloutAudioMedia::MediaOpen(MEDIA_SPECIAL, RandomEntry, 1000u, Looping, false, afAttenuation, 0);
    }
}

void FalloutAudioMedia::MediaPlay() {
    MediaAccessEnter();
    cMediaAFlags &= ~2;
    cMediaBFlags &= ~2;
    MediaAccessLeave();
}

// Fades out the playing track: a nonzero eMediaNextTrack stops track A, otherwise track B.
void FalloutAudioMedia::MediaStop() {
    MediaAccessEnter();
    if (eMediaNextTrack) {
        cMediaAFlags |= 2;
        fMediaAVolume = MediaVolume(eMediaAType, fMediaAAttenuation);
        uiMediaASynchStartTime = 0;
        uiMediaAFadeTime = 0;
        uiMediaAFadeEnd = 0;
        uiMediaACurrentPosition = 0;
        cMediaBFlags |= 1;
    }
    else {
        cMediaBFlags |= 2;
        fMediaBVolume = MediaVolume(eMediaBType, fMediaBAttenuation);
        uiMediaBSynchStartTime = 0;
        uiMediaBFadeTime = 0;
        uiMediaBFadeEnd = 0;
        uiMediaBCurrentPosition = 0;
        cMediaAFlags |= 1;
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

// Whether a track of aeType (any type when MEDIA_STANDBY) is playing; abIncludeFading tests the stopping flag (2)
// instead of flag 8.
bool FalloutAudioMedia::MediaIsPlaying(MEDIATYPE aeType, bool abIncludeFading) {
    bool bPlaying;
    MediaAccessEnter();
    if (abIncludeFading) {
        if (aeType) {
            bPlaying = (uiMediaAID && eMediaAType == aeType && !(cMediaAFlags & 2)) ||
                (uiMediaBID && eMediaBType == aeType && !(cMediaBFlags & 2));
        }
        else {
            bPlaying = (uiMediaAID && !(cMediaAFlags & 2)) || (uiMediaBID && !(cMediaBFlags & 2));
        }
    }
    else if (aeType) {
        bPlaying = (uiMediaAID && eMediaAType == aeType && !(cMediaAFlags & 8)) ||
            (uiMediaBID && eMediaBType == aeType && !(cMediaBFlags & 8));
    }
    else {
        bPlaying = (uiMediaAID && !(cMediaAFlags & 8)) || (uiMediaBID && !(cMediaBFlags & 8));
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
            bool bInCombat;
            MEDIATYPE latestType;
            if (FalloutAudioMedia::eMediaNextTrack) {
                latestType = FalloutAudioMedia::eMediaAType;
            }
            else {
                latestType = FalloutAudioMedia::eMediaBType;
            }
            uint32_t ticks = (&BSAudio::QInstance())->GetSynchTime();
            if (pCombatManager) {
                bInCombat = pCombatManager->ShouldPlayCombatMusic();
            }
            else {
                bInCombat = false;
            }
            if (!Interface::IsMenuVisible(Interface::eLoadingMenu) &&
                (!FalloutAudioMedia::pCurrentBattleMediaSet ||
                 !FalloutAudioMedia::pCurrentBattleMediaSet->Update(ticks, bInCombat, nullptr))) {
                MediaLocationController* MediaLocationController_;
                if (FalloutAudioMedia::pCurrentBattleMediaSet) {
                    FalloutAudioMedia::pCurrentBattleMediaSet = nullptr;
                }
                if (!pPlayer ||
                    (pPlayer->GetMediaLocationController() &&
                     (MediaLocationController_ = pPlayer->GetMediaLocationController(),
                      MediaLocationController_->Update(bInCombat)))) {
                    if (pPlayer) {
                        if (pPlayer->IsDead(false)) {
                            FalloutAudioMedia::MediaFadeOut(1000, MEDIA_STANDBY);
                        }
                        else {
                            if (FalloutAudioMedia::MediaIsPlaying(MEDIA_STANDBY, true) &&
                                !FalloutAudioMedia::MediaIsRadio() && !FalloutAudioMedia::pCurrentMediaController) {
                                FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_BATTLE);
                                FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_LOCATION_BATTLE);
                                FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_LOCATION);
                                FalloutAudioMedia::MediaFadeOut(0x1388, MEDIA_DUNGEON);
                            }
                        }
                    }
                }
                else {
                    FalloutAudioMedia::pCurrentMediaController = nullptr;
                    if (FalloutAudio::pCurrentSpace) {
                        if (FalloutAudio::pCurrentSpace->pSoundRegion) {
                            TESRegionDataSound* pRSD;
                            if (FalloutAudioMedia::MediaIsPlaying(MEDIA_LOCATION, true)) {
                                FalloutAudioMedia::MediaFadeOut(1000, MEDIA_LOCATION);
                            }
                            if (FalloutAudioMedia::MediaIsPlaying(MEDIA_DUNGEON, true)) {
                                FalloutAudioMedia::MediaFadeOut(1000, MEDIA_DUNGEON);
                            }
                            TESRegion* CombatStyle = FalloutAudio::pCurrentSpace->pSoundRegion;
                            if (FalloutAudioMedia::pCurrentRegionSoundData) {
                                pRSD = FalloutAudioMedia::pCurrentRegionSoundData;
                            }
                            else {
                                pRSD = static_cast<TESRegionDataSound*>(
                                    CombatStyle->pDataList->Find(REGION_DATA_SOUND_ID)
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
                                    !FalloutAudioMedia::pCurrentBattleMediaSet->Update(ticks, bInCombat, nullptr)) {
                                    FalloutAudioMedia::pCurrentBattleMediaSet = nullptr;
                                    if (pRSD->GetMediaSetIncidentals()) {
                                        if (latestType == MEDIA_STANDBY) {
                                            MediaSet* v1 = pRSD->GetMediaSetIncidentals();
                                            v1->Update(ticks, false, nullptr);
                                        }
                                    }
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
    uint32_t SynchTime = BSAudio::QInstance().GetSynchTime();
    if ((FalloutAudioMedia::eMediaAType == aeType || aeType == MEDIA_STANDBY) &&
        (FalloutAudioMedia::cMediaAFlags & 8) == 0) {
        FalloutAudioMedia::cMediaAFlags |= 24u;
        FalloutAudioMedia::fMediaAFadeStartVolume = FalloutAudioMedia::fMediaAVolume;
        FalloutAudioMedia::fMediaAFadeEndVolume = 0.0f;
        FalloutAudioMedia::uiMediaAFadeTime = aiFadeTime;
        FalloutAudioMedia::uiMediaAFadeEnd = aiFadeTime + SynchTime;
    }
    if ((FalloutAudioMedia::eMediaBType == aeType || aeType == MEDIA_STANDBY) &&
        (FalloutAudioMedia::cMediaBFlags & 8) == 0) {
        FalloutAudioMedia::cMediaBFlags |= 24u;
        FalloutAudioMedia::fMediaBFadeStartVolume = FalloutAudioMedia::fMediaBVolume;
        FalloutAudioMedia::fMediaBFadeEndVolume = 0.0f;
        FalloutAudioMedia::uiMediaBFadeTime = aiFadeTime;
        FalloutAudioMedia::uiMediaBFadeEnd = aiFadeTime + SynchTime;
    }
    FalloutAudioMedia::MediaAccessLeave();
}

uint32_t FalloutAudioMedia::MediaGetSubDuration(MEDIATYPE aeType) {
    FalloutAudioMedia::MediaAccessEnter();
    uint32_t v2 = 0;
    if (aeType == MEDIA_STANDBY) {
        if (FalloutAudioMedia::eMediaNextTrack == 1 && FalloutAudioMedia::uiMediaAID) {
            v2 = FalloutAudioMedia::uiMediaASubDuration;
        }
        else {
            if (!FalloutAudioMedia::eMediaNextTrack) {
                if (FalloutAudioMedia::uiMediaBID) {
                    v2 = FalloutAudioMedia::uiMediaBSubDuration;
                }
            }
        }
    }
    if (FalloutAudioMedia::eMediaNextTrack == 1 &&
        (FalloutAudioMedia::uiMediaAID && FalloutAudioMedia::eMediaAType == aeType)) {
        v2 = FalloutAudioMedia::uiMediaASubDuration;
    }
    else {
        if (!FalloutAudioMedia::eMediaNextTrack) {
            if (FalloutAudioMedia::uiMediaBID) {
                if (FalloutAudioMedia::eMediaBType == aeType) {
                    v2 = FalloutAudioMedia::uiMediaBSubDuration;
                }
            }
        }
    }
    FalloutAudioMedia::MediaAccessLeave();
    return v2;
}

uint32_t FalloutAudioMedia::MediaGetPosition(MEDIATYPE aeType) {
    FalloutAudioMedia::MediaAccessEnter();
    uint32_t v2 = 0;
    if (aeType == MEDIA_STANDBY) {
        if (FalloutAudioMedia::eMediaNextTrack == 1 && FalloutAudioMedia::uiMediaAID) {
            v2 = FalloutAudioMedia::uiMediaACurrentPosition;
        }
        else {
            if (!FalloutAudioMedia::eMediaNextTrack) {
                if (FalloutAudioMedia::uiMediaBID) {
                    v2 = FalloutAudioMedia::uiMediaBCurrentPosition;
                }
            }
        }
    }
    if (FalloutAudioMedia::eMediaNextTrack == 1 &&
        (FalloutAudioMedia::uiMediaAID && FalloutAudioMedia::eMediaAType == aeType)) {
        v2 = FalloutAudioMedia::uiMediaACurrentPosition;
    }
    else {
        if (!FalloutAudioMedia::eMediaNextTrack) {
            if (FalloutAudioMedia::uiMediaBID) {
                if (FalloutAudioMedia::eMediaBType == aeType) {
                    v2 = FalloutAudioMedia::uiMediaBCurrentPosition;
                }
            }
        }
    }
    FalloutAudioMedia::MediaAccessLeave();
    return v2;
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
    uint32_t v2 = 0;
    if (aeType == MEDIA_STANDBY) {
        if (FalloutAudioMedia::eMediaNextTrack == 1 && FalloutAudioMedia::uiMediaAID) {
            v2 = FalloutAudioMedia::uiMediaADuration;
        }
        else {
            if (!FalloutAudioMedia::eMediaNextTrack) {
                if (FalloutAudioMedia::uiMediaBID) {
                    v2 = FalloutAudioMedia::uiMediaBDuration;
                }
            }
        }
    }
    if (FalloutAudioMedia::eMediaNextTrack == 1 &&
        (FalloutAudioMedia::uiMediaAID && FalloutAudioMedia::eMediaAType == aeType)) {
        v2 = FalloutAudioMedia::uiMediaADuration;
    }
    else {
        if (!FalloutAudioMedia::eMediaNextTrack) {
            if (FalloutAudioMedia::uiMediaBID) {
                if (FalloutAudioMedia::eMediaBType == aeType) {
                    v2 = FalloutAudioMedia::uiMediaBDuration;
                }
            }
        }
    }
    FalloutAudioMedia::MediaAccessLeave();
    return v2;
}

uint32_t FalloutAudioMedia::ThreadProc(FalloutAudioMedia::MediaThreadInfo* input) {
    CBSAStream* pMediaSoundBuffer;
    int32_t lEventCode;
    intptr_t lParam1;
    intptr_t lParam2;
    HRESULT hr;
    IFilterGraph* ppv;
    IMediaControl* v38;
    IGraphBuilder* v40;
    char Str[268];
    IBasicAudio* v47;
    double v49;
    IMediaEvent* v51;
    IMediaPosition* pAudio;
    MEDIATRACK pMediaSoundFile = input->Track;
    uint32_t uiTrackID = input->ID;
    bool bStream = true;
    uint32_t ticks = 0;
    float volume = 0.0f;
    float v45 = 0.0f;
    float fTrackAttenuation = 0.0f;
    char cTrackFlags = 0;
    MEDIATYPE eTrackType = MEDIA_NONE;
    uint32_t uiTrackSynchStartTime = 0;
    uint32_t uiTrackCurrentPosition = 0;
    uint32_t uiTrackDuration = 0;
    uint32_t uiTrackSubDuration = 0;
    uint32_t uiTrackFadeTime = 0;
    uint32_t uiTrackFadeEnd = 0;
    uint32_t iDeltaTest = BSMAX(500, FalloutAudio::iMusicSynchOverride.Int() + 500);
    FalloutAudioMedia::MediaAccessEnter();
    ++uiMediaThreadCount;
    if ((pMediaSoundFile == MEDIATRACK_A &&
         (uiTrackID != FalloutAudioMedia::uiMediaAID || (FalloutAudioMedia::cMediaAFlags & 1) != 0)) ||
        (pMediaSoundFile == MEDIATRACK_B &&
         (uiTrackID != FalloutAudioMedia::uiMediaBID || (FalloutAudioMedia::cMediaBFlags & 1) != 0))) {
        bStream = false;
    }
    ticks = (&BSAudio::QInstance())->GetSynchTime();
    if (pMediaSoundFile) {
        FalloutAudioMedia::fMediaBVolume = volume;
        FalloutAudioMedia::uiMediaBLastUpdate = ticks;
        FalloutAudioMedia::uiMediaBCurrentPosition = uiTrackCurrentPosition;
        FalloutAudioMedia::uiMediaBDuration = uiTrackDuration;
        FalloutAudioMedia::uiMediaBSubDuration = uiTrackSubDuration;
        v45 = FalloutAudioMedia::fMediaBFadeStartVolume;
        fTrackAttenuation = FalloutAudioMedia::fMediaBAttenuation;
        cTrackFlags = FalloutAudioMedia::cMediaBFlags;
        eTrackType = FalloutAudioMedia::eMediaBType;
        uiTrackSynchStartTime = FalloutAudioMedia::uiMediaBSynchStartTime;
        uiTrackFadeTime = FalloutAudioMedia::uiMediaBFadeTime;
        uiTrackFadeEnd = FalloutAudioMedia::uiMediaBFadeEnd;
        BSstrcpy(Str, 260, FalloutAudioMedia::pMediaBFileName);
    }
    else {
        FalloutAudioMedia::fMediaAVolume = volume;
        FalloutAudioMedia::uiMediaALastUpdate = ticks;
        FalloutAudioMedia::uiMediaACurrentPosition = uiTrackCurrentPosition;
        FalloutAudioMedia::uiMediaADuration = uiTrackDuration;
        FalloutAudioMedia::uiMediaASubDuration = uiTrackSubDuration;
        v45 = FalloutAudioMedia::fMediaAFadeStartVolume;
        fTrackAttenuation = FalloutAudioMedia::fMediaAAttenuation;
        cTrackFlags = FalloutAudioMedia::cMediaAFlags;
        eTrackType = FalloutAudioMedia::eMediaAType;
        uiTrackSynchStartTime = FalloutAudioMedia::uiMediaASynchStartTime;
        uiTrackFadeTime = FalloutAudioMedia::uiMediaAFadeTime;
        uiTrackFadeEnd = FalloutAudioMedia::uiMediaAFadeEnd;
        BSstrcpy(Str, 260, FalloutAudioMedia::pMediaAFileName);
    }
    FalloutAudioMedia::MediaAccessLeave();
    ppv = nullptr;
    v40 = nullptr;
    v47 = nullptr;
    CAsyncStream* pMediaSoundBuffer_2 = nullptr;
    CBSAReader* v48 = nullptr;
    pAudio = nullptr;
    v38 = nullptr;
    v51 = nullptr;
    v49 = 0.0;
    CBSAStream* apMemory = static_cast<CBSAStream*>(ourMalloc(sizeof(CBSAStream)));
    if (apMemory) {
        pMediaSoundBuffer = ::new (apMemory) CBSAStream(Str);
    }
    else {
        pMediaSoundBuffer = nullptr;
    }
    pMediaSoundBuffer_2 = (CAsyncStream*)pMediaSoundBuffer;
    if (pMediaSoundBuffer->IsOpen()) {
        hr = 0;
        CMediaType kMediaType;
        kMediaType.majortype = CLSID_MFMediaType_Stream;
        if (std::strstr(Str, ".mp3")) {
            kMediaType.subtype = MEDIASUBTYPE_MPEG1Audio;
        }
        else {
            bStream = false;
            Warning("FalloutAudioMedia: file format wrong %s", Str);
        }
        if (bStream) {
            int32_t v2;
            IPin* pPin;
            CBSAReader* pReader;
            CBSAReader* v17 = static_cast<CBSAReader*>(ourMalloc(sizeof(CBSAReader)));
            if (v17) {
                pReader = ::new (v17) CBSAReader(pMediaSoundBuffer_2, &kMediaType, &hr);
            }
            else {
                pReader = nullptr;
            }
            v48 = pReader;
            static_cast<IUnknown*>(
                &static_cast<IBaseFilter&>(static_cast<CBaseFilter&>(static_cast<CAsyncReader&>(*pReader)))
            )
                ->AddRef();
            CoInitializeEx(nullptr, 0);
            hr = CoCreateInstance(&CLSID_FilterGraph, nullptr, 1u, &CLSID_IFilterGraph, reinterpret_cast<void**>(&ppv));
            if (hr < 0) {
                // The filter graph always exists on Windows. Where there is none (no DirectShow stand-in yet) the track
                // does not play.
                bStream = false;
            }
            else {
                if (v48) {
                    v2 = ppv->AddFilter(v48, nullptr);
                }
                else {
                    v2 = ppv->AddFilter(nullptr, nullptr);
                }
                hr = v2;
                if (v2 < 0) {
                    bStream = false;
                }
                hr = ppv->QueryInterface(CLSID_IGraphBuilder, reinterpret_cast<void**>(&v40));
                if (hr < 0) {
                    bStream = false;
                }
                CBasePin* v14 = v48->GetPin(0);
                if (v14) {
                    pPin = static_cast<IPin*>(v14);
                }
                else {
                    pPin = nullptr;
                }
                hr = v40->Render(pPin);
                if (hr < 0) {
                    bStream = false;
                }
                hr = v40->QueryInterface(CLSID_IBasicAudio, reinterpret_cast<void**>(&v47));
                if (hr < 0) {
                    bStream = false;
                }
                hr = ppv->QueryInterface(CLSID_IMediaControl, reinterpret_cast<void**>(&v38));
                if (hr < 0) {
                    bStream = false;
                }
                hr = v40->QueryInterface(CLSID_IMediaPosition, reinterpret_cast<void**>(&pAudio));
                if (hr < 0) {
                    bStream = false;
                }
                hr = v40->QueryInterface(CLSID_IMediaEvent, reinterpret_cast<void**>(&v51));
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
    if (ppv && pAudio) {
        pAudio->get_Duration(&v49);
        uiTrackSubDuration = static_cast<int64_t>(v49 * 1000.0);
    }
    else {
        uiTrackSubDuration = 0;
    }
    while (bStream) {
        float volumeGoal;
        FalloutAudioMedia::hUpdate->Wait(100u);
        FalloutAudioMedia::MediaAccessEnter();
        ticks = (&BSAudio::QInstance())->GetSynchTime();
        if (ppv && pAudio) {
            pAudio->get_CurrentPosition(&v49);
            uiTrackCurrentPosition = static_cast<int64_t>(v49 * 1000.0);
        }
        else {
            uiTrackCurrentPosition = 0;
        }
        if (ppv && pAudio) {
            pAudio->get_Duration(&v49);
            uiTrackDuration = static_cast<int64_t>(v49 * 1000.0);
        }
        else {
            uiTrackDuration = 0;
        }
        if (pMediaSoundFile) {
            FalloutAudioMedia::fMediaBVolume = volume;
            FalloutAudioMedia::uiMediaBLastUpdate = ticks;
            FalloutAudioMedia::uiMediaBCurrentPosition = uiTrackCurrentPosition;
            FalloutAudioMedia::uiMediaBDuration = uiTrackDuration;
            FalloutAudioMedia::uiMediaBSubDuration = uiTrackSubDuration;
            v45 = FalloutAudioMedia::fMediaBFadeStartVolume;
            fTrackAttenuation = FalloutAudioMedia::fMediaBAttenuation;
            cTrackFlags = FalloutAudioMedia::cMediaBFlags;
            eTrackType = FalloutAudioMedia::eMediaBType;
            uiTrackSynchStartTime = FalloutAudioMedia::uiMediaBSynchStartTime;
            uiTrackFadeTime = FalloutAudioMedia::uiMediaBFadeTime;
            uiTrackFadeEnd = FalloutAudioMedia::uiMediaBFadeEnd;
        }
        else {
            FalloutAudioMedia::fMediaAVolume = volume;
            FalloutAudioMedia::uiMediaALastUpdate = ticks;
            FalloutAudioMedia::uiMediaACurrentPosition = uiTrackCurrentPosition;
            FalloutAudioMedia::uiMediaADuration = uiTrackDuration;
            FalloutAudioMedia::uiMediaASubDuration = uiTrackSubDuration;
            v45 = FalloutAudioMedia::fMediaAFadeStartVolume;
            fTrackAttenuation = FalloutAudioMedia::fMediaAAttenuation;
            cTrackFlags = FalloutAudioMedia::cMediaAFlags;
            eTrackType = FalloutAudioMedia::eMediaAType;
            uiTrackSynchStartTime = FalloutAudioMedia::uiMediaASynchStartTime;
            uiTrackFadeTime = FalloutAudioMedia::uiMediaAFadeTime;
            uiTrackFadeEnd = FalloutAudioMedia::uiMediaAFadeEnd;
        }
        FalloutAudioMedia::MediaAccessLeave();
        if ((cTrackFlags & 8) != 0) {
            volumeGoal = 0.0f;
        }
        else {
            volumeGoal = FalloutAudioMedia::MediaVolume(eTrackType, fTrackAttenuation);
        }
        float v27 = volumeGoal;
        if ((cTrackFlags & 16) != 0 && ticks < uiTrackFadeEnd) {
            double v3;
            float v9;
            float v10;
            double v11;
            if (uiTrackFadeTime) {
                v11 = (static_cast<double>(uiTrackFadeEnd) - static_cast<double>(ticks)) /
                    static_cast<double>(uiTrackFadeTime);
            }
            else {
                v11 = 0.0;
            }
            float v24 = v11;
            if (!(v24 < 1.0)) {
                v10 = 1.0f;
            }
            else {
                v10 = v11;
            }
            if (!(v10 > 0.0)) {
                v9 = 0.0f;
            }
            else {
                v9 = v10;
            }
            if ((cTrackFlags & 8) != 0) {
                v3 = v9 * v45;
            }
            else {
                v3 = (1.0 - v9) * (v27 - v45) + v45;
            }
            volume = v3;
        }
        else {
            if (!(v27 < static_cast<double>(volume))) {
                if (v27 > static_cast<double>(volume)) {
                    float v7;
                    volume = volume + 0.05000000074505806;
                    if (!(v27 > static_cast<double>(volume))) {
                        v7 = v27;
                    }
                    else {
                        v7 = volume;
                    }
                    volume = v7;
                }
            }
            else {
                float v8;
                volume = volume - 0.05000000074505806;
                if (!(v27 < static_cast<double>(volume))) {
                    v8 = v27;
                }
                else {
                    v8 = volume;
                }
                volume = v8;
            }
        }
        if ((cTrackFlags & 8) != 0 && volume < 0.009999999776482582) {
            bStream = false;
        }
        if (v47) {
            if (!(volume < 9.999999747378752e-05)) {
                int32_t v6 = static_cast<int32_t>(std::log10(volume) * 2000.0);
                v47->put_Volume(v6);
            }
            else {
                v47->put_Volume(-10000);
            }
        }
        if (uiTrackSynchStartTime) {
            uint32_t v19;
            int32_t v20;
            uint32_t i;
            if (ppv && pAudio) {
                pAudio->get_CurrentPosition(&v49);
                v19 = static_cast<int64_t>(v49 * 1000.0);
            }
            else {
                v19 = 0;
            }
            uint32_t SynchTime = (&BSAudio::QInstance())->GetSynchTime();
            for (i = SynchTime - uiTrackSynchStartTime; i > uiTrackDuration; i -= uiTrackDuration) {
                ;
            }
            if (SynchTime < uiTrackSynchStartTime) {
                i = 0;
                Warning(
                    "FalloutAudioMedia Synch Error: correct position is negative %i %i", SynchTime,
                    uiTrackSynchStartTime
                );
            }
            if (v19 <= i) {
                v20 = i - v19;
            }
            else {
                v20 = v19 - i;
            }
            if (v20 > iDeltaTest && (v20 < 80000 || !v19)) {
                Warning("FalloutAudioMedia Synch: %i = abs(-/+ %i +/- %i)", v20, i, v19);
                uint32_t v23 = i + 50;
                if (ppv) {
                    if (pAudio) {
                        pAudio->put_CurrentPosition(static_cast<double>(v23) / 1000.0);
                    }
                }
            }
        }
        if ((cTrackFlags & 2) != 0) {
            if (v40) {
                if (v38) {
                    v38->Stop();
                }
            }
        }
        else {
            BSAudio* v4;
            if ((cTrackFlags & 4) != 0 ||
                (FalloutAudioMedia::bMediaMenuPaused || (v4 = &BSAudio::QInstance(), v4->IsSynchTimerPaused()))) {
                if (v40) {
                    if (v38) {
                        v38->Pause();
                    }
                }
            }
            else {
                if (pAudio) {
                    if (v38) {
                        v38->Run();
                    }
                }
            }
        }
        if (v51) {
            while (v51->GetEvent(&lEventCode, &lParam1, &lParam2, 0) >= 0) {
                if (lEventCode > 0 && lEventCode <= 3) {
                    v51->FreeEventParams(lEventCode, lParam1, lParam2);
                    if ((cTrackFlags & 32) != 0) {
                        if (v40) {
                            if (v38) {
                                v38->Stop();
                            }
                        }
                        if (ppv) {
                            if (pAudio) {
                                pAudio->put_CurrentPosition(0.0);
                            }
                        }
                        if ((cTrackFlags & 2) == 0) {
                            if (pAudio) {
                                if (v38) {
                                    v38->Run();
                                }
                            }
                        }
                    }
                    else {
                        bStream = false;
                    }
                }
                v51->FreeEventParams(lEventCode, lParam1, lParam2);
            }
        }
        else {
            bStream = false;
        }
        FalloutAudioMedia::MediaAccessEnter();
        if ((pMediaSoundFile == MEDIATRACK_A &&
             (uiTrackID != FalloutAudioMedia::uiMediaAID || (FalloutAudioMedia::cMediaAFlags & 1) != 0)) ||
            (pMediaSoundFile == MEDIATRACK_B &&
             (uiTrackID != FalloutAudioMedia::uiMediaBID || (FalloutAudioMedia::cMediaBFlags & 1) != 0))) {
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
    if (v51) {
        v51->Release();
        v51 = nullptr;
    }
    if (pAudio) {
        pAudio->Release();
        pAudio = nullptr;
    }
    if (v38) {
        v38->Release();
        v38 = nullptr;
    }
    if (v47) {
        v47->Release();
        v47 = nullptr;
    }
    if (v40) {
        v40->Release();
        v40 = nullptr;
    }
    if (ppv) {
        ppv->Release();
        ppv = nullptr;
    }
    if (v48) {
        static_cast<IUnknown*>(&static_cast<IBaseFilter&>(static_cast<CBaseFilter&>(static_cast<CAsyncReader&>(*v48))))
            ->Release();
        v48 = nullptr;
    }
    if (pMediaSoundBuffer_2) {
        delete pMediaSoundBuffer_2;
    }
    return 0;
}

int32_t BSAudio::IsSynchTimerPaused() {
    return SynchTimer.IsPaused();
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

void FAMThread::AddNewTrack(FAMThread::TrackData* a1) {
    ourMemcpy(&m_param, a1, sizeof(m_param));
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
