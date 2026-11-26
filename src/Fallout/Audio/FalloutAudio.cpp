#include "Fallout/Audio/FalloutAudio.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32AudioListener.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataSound.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Magic/ReanimateEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cmath>

SettingT<INISettingCollection> FalloutAudio::fMainMenuMusicVolume("fMainMenuMusicVolume:Audio", 0.6f);
SettingT<INISettingCollection> FalloutAudio::fASFadeInTime("fASFadeInTime:Audio", 3.0f);
SettingT<INISettingCollection> FalloutAudio::fASFadeOutTime("fASFadeOutTime:Audio", 5.0f);
SettingT<INISettingCollection> FalloutAudio::fASFadeStartTime("fASFadeStartTime:Audio", 2.0f);
SettingT<INISettingCollection> FalloutAudio::fRegionLoopFadeInTime("fRegionLoopFadeInTime:Audio", 0.6f);
SettingT<INISettingCollection> FalloutAudio::fRegionLoopFadeOutTime("fRegionLoopFadeOutTime:Audio", 1.0f);
SettingT<INISettingCollection> FalloutAudio::fWeaponInteriorNearVolumeMod("fWeaponInteriorNearVolumeMod", 1.0f);
SettingT<INISettingCollection>
    FalloutAudio::fWeaponInteriorNearAttenuationMod("fWeaponInteriorNearAttenuationMod", 3.0f);
SettingT<INISettingCollection> FalloutAudio::fWeaponInteriorFarVolumeMod("fWeaponInteriorFarVolumeMod", 0.1f);
SettingT<INISettingCollection>
    FalloutAudio::fWeaponInteriorFarAttenuationMod("fWeaponInteriorFarAttenuationMod", 0.75f);
SettingT<INISettingCollection> FalloutAudio::iMusicTransitionInMS("iMusicTransitionInMS:Audio", 1000);
SettingT<INISettingCollection> FalloutAudio::iMusicSynchOverride("iMusicSynchOverride:Audio", 0);
BSSoundHandle FalloutAudio::UnderwaterLoop;
BSSoundHandle FalloutAudio::kAmbientLoop;
FalloutAudio::CurrentRegionSounds_t FalloutAudio::CurrentRegionSounds;

void FalloutAudio::UpdateRegionSounds() {
    float ftimeOfDay;
    BGSAcousticSpace* pCellSpace;
    BSSoundHandle* plookup;
    BSSimpleList<TESRegionSound*>* soundList = pPlayer->GetRegionSounds();
    bool blistenerUnderwater = BSAudio::QInstance().IsListenerUnderwater();
    if (pPlayer) {
        if (pPlayer->GetParentCell()) {
            pCellSpace = pPlayer->GetParentCell()->GetAcousticSpace();
        }
        else {
            pCellSpace = nullptr;
        }
    }
    else {
        pCellSpace = nullptr;
    }
    if (FalloutAudio::pCurrentSpace &&
        !FalloutAudio::pCurrentSpace->IsInterior() &&
        pCellSpace &&
        pCellSpace->IsInterior()) {
        FalloutAudio::SetAcousticSpace(pCellSpace);
        FalloutAudio::bUpdateAcousticSpace = false;
        FalloutAudio::pCurrentCellSpace = nullptr;
    }

    if (FalloutAudio::bUpdateAcousticSpace) {
        if (FalloutAudio::pCurrentCellSpace == FalloutAudio::pCurrentSpace) {
            FalloutAudio::pCurrentCellSpace = nullptr;
            FalloutAudio::bUpdateAcousticSpace = false;
        }
        else {
            FalloutAudio::SetAcousticSpace(FalloutAudio::pCurrentCellSpace);
        }
    }
    if (!FalloutAudio::pNoActivationSound) {
        FalloutAudio::pNoActivationSound = pDataHandler->GetSound("UIActivateNothing");
        if (FalloutAudio::pNoActivationSound) {
            TESSound* pSound = FalloutAudio::pNoActivationSound;
            const char* pFilename = FalloutAudio::pNoActivationSound->GetFilename();
            BSAudio::QInstance().PrecacheSound(pFilename, 289u, pSound);
        }
    }
    if (FalloutAudio::pCurrentSpace ||
        !pPlayer ||
        !pPlayer->GetParentCell() ||
        pPlayer->GetParentCell()->GetAcousticSpace() == FalloutAudio::pCurrentSpace) {
        if (!pPlayer || (!pPlayer->GetParentCell() && !pPlayer->IsAcousticSpaceTransition())) {
            FalloutAudio::ClearAcousticSpace(nullptr);
        }
    }
    else {
        FalloutAudio::SetAcousticSpace(pPlayer->GetParentCell()->GetAcousticSpace());
    }
    if (FalloutAudio::pCurrentSpace && FalloutAudio::pCurrentSpace->pSoundRegion) {
        TESRegion* pregion = FalloutAudio::pCurrentSpace->pSoundRegion;
        TESRegionDataSound* pRSD = static_cast<TESRegionDataSound*>(pregion->pDataList->Find(REGION_DATA_SOUND_ID));
        if (pRSD) {
            soundList = &pRSD->sounds.soundList;
        }
    }

    if (FalloutAudio::pCurrentSpace) {
        uint32_t uiEnvType = FalloutAudio::pCurrentSpace->eEnvType;
        BSAudio::QInstance().SetEnvironment(uiEnvType);
    }
    else {
        BSAudio::QInstance().SetEnvironment(0);
    }
    if (blistenerUnderwater && pPlayer->GetParentCell() && !FalloutAudio::UnderwaterLoop.IsPlaying()) {
        FalloutAudio::UnderwaterLoop = BSAudio::QInstance().GetSoundHandleByName("AMBUnderwaterLP", 0x2101u);
        FalloutAudio::UnderwaterLoop.bAssumeSuccess = true;
        FalloutAudio::UnderwaterLoop.Play(false);
    }
    else if ((!blistenerUnderwater || !pPlayer->GetParentCell()) && FalloutAudio::UnderwaterLoop.IsPlaying()) {
        FalloutAudio::UnderwaterLoop.FadeOutAndRelease(300u);
    }
    ImpactMixer::InitFootsteps();
    ImpactMixer::InitGunshots();
    if (FalloutAudio::bRegionUpdateDisabled) {
        return;
    }
    Sky* psky = TES::pInstance->pSky;
    if (psky) {
        ftimeOfDay = psky->GetCurrentGameHour();
    }
    else {
        ftimeOfDay = 0.0f;
    }
    BSAudio::QInstance().SetConditions(ftimeOfDay, 0);
    uint32_t uiSynchTime = BSAudio::QInstance().GetSynchTime();
    if (BSAbs(static_cast<int32_t>(uiSynchTime - FalloutAudio::uiLastRegionUpdate)) <= 1000) {
        if (soundList->IsEmpty() && !FalloutAudio::CurrentRegionSounds.IsEmpty()) {
            uint32_t iKey;
            NiTMapItem<uint32_t, BSSoundHandle*>* pPos = FalloutAudio::CurrentRegionSounds.GetFirstPos();
            while (pPos) {
                FalloutAudio::CurrentRegionSounds.GetNext(pPos, iKey, plookup);
                FalloutAudio::CurrentRegionSounds.RemoveAt(iKey);
                pPos = FalloutAudio::CurrentRegionSounds.GetFirstPos();
                plookup->FadeOutAndRelease(300u);
                if (plookup) {
                    delete plookup;
                }
                plookup = nullptr;
            }
        }
        return;
    }
    if (pPlayer) {
        pPlayer->ClearClosestAudioMarkerInfo();
    }
    if (!Interface::IsTopMenuID(Interface::eStartMenu)) {
        TES::pInstance->UpdateRefSounds();
    }
    float fFadeInTime = FalloutAudio::fASFadeStartTime.Float();
    if (FalloutAudio::kAmbientLoop.IsPlaying()) {
        fFadeInTime = FalloutAudio::fASFadeInTime.Float();
    }
    if (FalloutAudio::pCurrentSpace && pPlayer) {
        TESSound* pambSound;
        if (FalloutAudio::pCurrentSpace->IsInterior()) {
            bool bShouldUseWalla = FalloutAudio::bUpdateAcousticSpace;
            if (!FalloutAudio::kAmbientLoop.IsValid()) {
                bShouldUseWalla = true;
            }
            if (bShouldUseWalla) {
                if (FalloutAudio::bUpdateAcousticSpace) {
                    FalloutAudio::pCurrentCellSpace = nullptr;
                    FalloutAudio::bUpdateAcousticSpace = false;
                }
                if (FalloutAudio::kAmbientLoop.IsPlaying()) {
                    FalloutAudio::kAmbientLoop.FadeOutAndRelease(
                        static_cast<int64_t>(FalloutAudio::fASFadeOutTime.Float() * 500.0)
                    );
                    FalloutAudio::kAmbientLoop.Clear();
                }
                FalloutAudio::bUseWalla = false;
                pambSound = FalloutAudio::pCurrentSpace->pDawnSound;
                if (pambSound) {
                    const char* pFilename = pambSound->GetFilename();
                    FalloutAudio::kAmbientLoop = BSAudio::QInstance()
                                                     .GetSoundHandleByFilename(pFilename, 0x11011u, pambSound);
                    FalloutAudio::kAmbientLoop.FadeInPlay(static_cast<int64_t>(fFadeInTime * 1000.0));
                }
            }
        }
        else {
            bool bHasChanged = FalloutAudio::bUpdateAcousticSpace;
            if (!FalloutAudio::bUpdateAcousticSpace) {
                switch (FalloutAudio::iCurrentTime) {
                    case 0: {
                        if (BSAudio::fDayTime.Float() < static_cast<double>(ftimeOfDay)) {
                            bHasChanged = true;
                        }
                        break;
                    }
                    case 1: {
                        if (BSAudio::fDuskTime.Float() < static_cast<double>(ftimeOfDay)) {
                            bHasChanged = true;
                        }
                        break;
                    }
                    case 2: {
                        if (BSAudio::fNightTime.Float() < static_cast<double>(ftimeOfDay)) {
                            bHasChanged = true;
                        }
                        break;
                    }
                    case 3: {
                        if (BSAudio::fDawnTime.Float() < static_cast<double>(ftimeOfDay)) {
                            if (BSAudio::fNightTime.Float() > static_cast<double>(ftimeOfDay)) {
                                bHasChanged = true;
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            if (bHasChanged) {
                if (FalloutAudio::bUpdateAcousticSpace) {
                    FalloutAudio::pCurrentCellSpace = nullptr;
                    FalloutAudio::bUpdateAcousticSpace = false;
                }
                else {
                    FalloutAudio::kAmbientLoop.FadeOutAndRelease(
                        static_cast<int64_t>(FalloutAudio::fASFadeOutTime.Float() * 1000.0)
                    );
                    FalloutAudio::kAmbientLoop.Clear();
                }
                FalloutAudio::bUseWalla = false;
                if (BSAudio::fDawnTime.Float() < static_cast<double>(ftimeOfDay)) {
                    if (!(BSAudio::fDayTime.Float() < static_cast<double>(ftimeOfDay))) {
                        pambSound = FalloutAudio::pCurrentSpace->pDawnSound;
                        FalloutAudio::iCurrentTime = 0;
                    }
                    else if (!(BSAudio::fDuskTime.Float() < static_cast<double>(ftimeOfDay))) {
                        pambSound = FalloutAudio::pCurrentSpace->pNoonSound;
                        FalloutAudio::iCurrentTime = 1;
                    }
                    else if (!(BSAudio::fNightTime.Float() < static_cast<double>(ftimeOfDay))) {
                        pambSound = FalloutAudio::pCurrentSpace->pDuskSound;
                        FalloutAudio::iCurrentTime = 2;
                    }
                    else {
                        pambSound = FalloutAudio::pCurrentSpace->pNightSound;
                        FalloutAudio::iCurrentTime = 3;
                    }
                }
                else {
                    pambSound = FalloutAudio::pCurrentSpace->pNightSound;
                    FalloutAudio::iCurrentTime = 3;
                }
                if (pambSound) {
                    const char* pFilename = pambSound->GetFilename();
                    FalloutAudio::kAmbientLoop = BSAudio::QInstance()
                                                     .GetSoundHandleByFilename(pFilename, 0x11011u, pambSound);
                    FalloutAudio::kAmbientLoop.FadeInPlay(static_cast<int64_t>(fFadeInTime * 1000.0));
                }
            }
        }
    }
    else {
        FalloutAudio::kAmbientLoop.FadeOutAndRelease(1000u);
        FalloutAudio::kAmbientLoop.Clear();
    }
    if (!soundList->IsEmpty() && !Interface::IsTopMenuID(Interface::eStartMenu)) {
        TESRegionSound* prgnSound;
        TESSound* psoundData;
        BSSimpleList<TESRegionSound*>* plistPtr = soundList;
        BSSoundHandle hndle(0);
        bool bcanPlay = true;
        bool bisLoop = false;
        BSAudioListener* plstnr = BSAudio::QInstance().pListener;
        while (plistPtr && !plistPtr->IsEmpty()) {
            bcanPlay = true;
            prgnSound = plistPtr->m_item;
            // GetSound looks up the sound by the form ID field; the exe reads it before testing prgnSound.
            psoundData = prgnSound->GetSound();
            if (prgnSound && psoundData) {
                float fBeginTime = static_cast<double>(psoundData->GetStartTime()) * 0.09375;
                float fEndTime = static_cast<double>(psoundData->GetEndTime()) * 0.09375;
                bisLoop = TESSound::ConvertLoopFlag(psoundData->GetData().iFlags) != 0;
                if (psky) {
                    if ((prgnSound->uiFlags & TESRegionSound::WEATHER_MASK) != 0 && psky->pCurrentWeather) {
                        uint32_t icurrentConditions = psky->pCurrentWeather->GetWeatherDataRaw(TESWeather::WD_DIVIDER2);
                        if ((prgnSound->uiFlags & TESRegionSound::WEATHER_MASK & icurrentConditions) == 0 &&
                            icurrentConditions) {
                            bcanPlay = false;
                        }
                    }

                    float fSpan = fEndTime - fBeginTime;
                    if (std::fabs(fSpan) >= 0.09375 &&
                        ((fEndTime > static_cast<double>(fBeginTime) &&
                          (ftimeOfDay < static_cast<double>(fBeginTime) ||
                           ftimeOfDay > static_cast<double>(fEndTime))) ||
                         (fEndTime < static_cast<double>(fBeginTime) &&
                          ftimeOfDay < static_cast<double>(fBeginTime) &&
                          ftimeOfDay > static_cast<double>(fEndTime)))) {
                        bcanPlay = false;
                    }
                }
                bool bisInMap = FalloutAudio::CurrentRegionSounds.GetAt(psoundData->iFormID, plookup);
                if (bisLoop && !bisInMap && bcanPlay) {
                    BSSoundHandle* phndle = new BSSoundHandle;
                    const char* pFilename = psoundData->GetFilename();
                    *phndle = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, 0x11011u, psoundData);
                    phndle->bAssumeSuccess = true;
                    FalloutAudio::CurrentRegionSounds.SetAt(psoundData->iFormID, phndle);
                    phndle->FadeInPlay(static_cast<int64_t>(FalloutAudio::fRegionLoopFadeInTime.Float() * 1000.0));
                }
                else if (bisLoop && bisInMap) {
                    if (!plookup->IsPlaying() || bcanPlay) {
                        if (plookup->IsValid() && !plookup->IsPlaying() && bcanPlay) {
                            plookup->FadeInPlay(
                                static_cast<int64_t>(FalloutAudio::fRegionLoopFadeInTime.Float() * 1000.0)
                            );
                        }
                    }
                    else {
                        plookup->FadeOutAndRelease(
                            static_cast<int64_t>(FalloutAudio::fRegionLoopFadeOutTime.Float() * 1000.0)
                        );
                        FalloutAudio::CurrentRegionSounds.RemoveAt(psoundData->iFormID);
                        if (plookup) {
                            delete plookup;
                        }
                    }
                }
                else if (!bisLoop && bcanPlay) {
                    float fchance = static_cast<double>(prgnSound->uiChance) / 2e+07;
                    if (RandomBool(fchance)) {
                        if ((psoundData->GetData().iFlags & TESSound::FLAG_2D) != 0) {
                            const char* pFilename = psoundData->GetFilename();
                            hndle = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, 0x11101u, psoundData);
                        }
                        else {
                            const char* pFilename = psoundData->GetFilename();
                            hndle = BSAudio::QInstance().GetSoundHandleByFilename(pFilename, 0x11102u, psoundData);
                        }
                        NiPoint3 loc = pPlayer->GetLocationOnReference();
                        if (plstnr) {
                            loc = plstnr->GetPosition();
                        }
                        float fOffsetX = static_cast<float>(Random() % 650 + 100);
                        float fOffsetY = static_cast<float>(Random() % 650 + 100);
                        if (RandomBool(0.5f)) {
                            fOffsetX = fOffsetX * -1.0;
                        }
                        if (RandomBool(0.5f)) {
                            fOffsetY = fOffsetY * -1.0;
                        }
                        loc.x = loc.x + fOffsetX;
                        loc.y = loc.y + fOffsetY;
                        loc.z = loc.z + 256.0;
                        hndle.SetPriority(255u);
                        hndle.SetPosition(loc.x, loc.y, loc.z);
                        hndle.Play(false);
                    }
                }
            }
            plistPtr = plistPtr->m_pkNext;
        }
        uint32_t ikey = 0;
        // The found flag is cleared and the list restarted once, before the map loop, not per entry.
        bool bisInList = false;
        NiTMapItem<uint32_t, BSSoundHandle*>* pPos = FalloutAudio::CurrentRegionSounds.GetFirstPos();
        plistPtr = soundList;
        while (pPos) {
            FalloutAudio::CurrentRegionSounds.GetNext(pPos, ikey, plookup);
            while (plistPtr && !plistPtr->IsEmpty()) {
                prgnSound = plistPtr->m_item;
                psoundData = prgnSound->GetSound();
                if (ikey == psoundData->iFormID) {
                    bisInList = true;
                    break;
                }
                plistPtr = plistPtr->m_pkNext;
            }
            if (!bisInList) {
                FalloutAudio::CurrentRegionSounds.RemoveAt(ikey);
                pPos = FalloutAudio::CurrentRegionSounds.GetFirstPos();
                plookup->FadeOutAndRelease(static_cast<int64_t>(FalloutAudio::fRegionLoopFadeOutTime.Float() * 1000.0));
                if (plookup) {
                    delete plookup;
                }
                plookup = nullptr;
            }
        }
    }
    FalloutAudio::uiLastRegionUpdate = BSAudio::QInstance().GetSynchTime();
}

void PlayMysteriousStrangerMusic() {
    BSSoundHandle kHandle = BSAudio::QInstance().GetSoundHandleByName(
        "MUSMysteriousStrangerA01",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_DONT_CACHE |
            BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE
    );
    kHandle.Play(false);
    BSAudio::QInstance().Precache(
        "MUSMysteriousStrangerA02",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE |
            BSGameSound::SOUND_FLAG_UNKBIT29
    );
}

void PlaySound_MUSMysteriousStrangerA02() {
    BSSoundHandle kHandle = BSAudio::QInstance().GetSoundHandleByName(
        "MUSMysteriousStrangerA02",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE
    );
    kHandle.Play(false);
}

void FalloutAudio::ClearAcousticSpace(BGSAcousticSpace* apSpace) {
    if (!FalloutAudio::bLockAcousticSpace &&
        (!apSpace || apSpace == FalloutAudio::pCurrentSpace) &&
        FalloutAudio::pCurrentSpace) {
        FalloutAudio::kAmbientLoop.FadeOutAndRelease(
            static_cast<int64_t>(FalloutAudio::fASFadeOutTime.Float() * 1000.0)
        );
        FalloutAudio::kAmbientLoop.Clear();
        FalloutAudio::pCurrentSpace = nullptr;
        FalloutAudio::bUpdateAcousticSpace = true;
        BSAudio::QInstance().SetEnvironment(0);
    }
}

void FalloutAudio::DisableRegionSoundUpdates(bool abIn) {
    FalloutAudio::bRegionUpdateDisabled = abIn;
}

void FalloutAudio::SetNextAcousticSpace(BGSAcousticSpace* apSpace) {
    FalloutAudio::pCurrentCellSpace = apSpace;
}

void FalloutAudio::SetAcousticSpace(BGSAcousticSpace* apSpace) {
    if (FalloutAudio::pCurrentSpace != apSpace) {
        FalloutAudio::kAmbientLoop.FadeOutAndRelease(
            static_cast<int64_t>(FalloutAudio::fASFadeOutTime.Float() * 1000.0)
        );
        FalloutAudio::kAmbientLoop.Clear();
        if (apSpace) {
            uint32_t uiEnvType = static_cast<uint32_t>(apSpace->eEnvType);
            BSAudio::QInstance().SetEnvironment(uiEnvType);
        }
        FalloutAudio::pCurrentSpace = apSpace;
        FalloutAudio::bUpdateAcousticSpace = true;
    }
}

void FalloutAudio::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {}

void FalloutAudio::SetMainMenuMusic(bool abMainMenuMusic) {
    FalloutAudio::bMainMenuMusic = abMainMenuMusic;
}

void FalloutAudio::MuteRadio() {
    FalloutAudio::bRadioMute = true;
}

void FalloutAudio::UnMuteRadio() {
    FalloutAudio::bRadioMute = false;
}

bool FalloutAudio::GetNewGameMusic() {
    return FalloutAudio::bNewGameMusic;
}

bool FalloutAudio::IsRadioMute() {
    return FalloutAudio::bRadioMute;
}

void FalloutAudio::SetNewGameMusic(bool abNewGameMusic) {
    FalloutAudio::bNewGameMusic = abNewGameMusic;
}

TESSound* FalloutAudio::GetNoActivationSound() {
    if (!pNoActivationSound) {
        pNoActivationSound = pDataHandler->GetSound("UIActivateNothing");
    }
    return pNoActivationSound;
}

void FalloutAudio::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pNoActivationSound, 0);
}

void FalloutAudio::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESForm* pForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
    pNoActivationSound = dynamic_cast<TESSound*>(pForm);
}

bool FalloutAudio::ResolveSoundID(
    uint32_t aiFormID,
    char* apFilename,
    uint32_t* aiSoundTypeFlags,
    void** apSettingsContext
) {
    TESSound* psound = pDataHandler->GetSound(aiFormID);
    if (!psound || !psound->GetFilename() || !apFilename) {
        return false;
    }
    BSstrcpy(apFilename, 260, psound->GetFilename());
    if ((psound->GetData().iFlags & TESSound::FLAG_2D) != 0) {
        *aiSoundTypeFlags |= 1u;
        *aiSoundTypeFlags &= ~2u;
    }
    else if ((*aiSoundTypeFlags & 1) == 0 && (*aiSoundTypeFlags & 2) == 0) {
        *aiSoundTypeFlags |= 2u;
        *aiSoundTypeFlags &= ~1u;
    }
    if ((psound->GetData().iFlags & TESSound::FLAG_RANDOM_FREQUENCY_SHIFT) != 0) {
        *aiSoundTypeFlags |= 64u;
    }
    if ((psound->GetData().iFlags & TESSound::FLAG_2D_RADIUS) != 0) {
        *aiSoundTypeFlags |= BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
    }
    *aiSoundTypeFlags |= TESSound::ConvertLoopFlag(psound->GetData().iFlags);
    *apSettingsContext = psound;
    return true;
}

bool FalloutAudio::ResolveSoundName(
    const char* apFormName,
    char* apFilename,
    uint32_t* aiSoundTypeFlags,
    void** apSettingsContext
) {
    if (!apFormName) {
        return false;
    }
    TESSound* psound = pDataHandler->GetSound(apFormName);
    if (psound && psound->GetFilename() && apFilename) {
        BSstrcpy(apFilename, 260, psound->GetFilename());
        if ((psound->GetData().iFlags & TESSound::FLAG_2D) != 0) {
            *aiSoundTypeFlags |= 1u;
            *aiSoundTypeFlags &= ~2u;
        }
        else if ((*aiSoundTypeFlags & 1) == 0 && (*aiSoundTypeFlags & 2) == 0) {
            *aiSoundTypeFlags |= 2u;
            *aiSoundTypeFlags &= ~1u;
        }
        if ((psound->GetData().iFlags & TESSound::FLAG_RANDOM_FREQUENCY_SHIFT) != 0) {
            *aiSoundTypeFlags |= 64u;
        }
        if ((psound->GetData().iFlags & TESSound::FLAG_2D_RADIUS) != 0) {
            *aiSoundTypeFlags |= BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
        }
        *aiSoundTypeFlags |= TESSound::ConvertLoopFlag(psound->GetData().iFlags);
        *apSettingsContext = psound;
        return true;
    }

    if (pDataHandler && !pDataHandler->GetSaveLoad() && pDataHandler->CountCompiled()) {
        Warning("Audio: Could not find TESSound corresponding to form name \"%s\"", apFormName);
    }

    return false;
}

uint32_t FalloutAudio::ApplySettingsFromMasterFile(
    BSSoundHandle& aSoundHandle,
    void* apSettingsContext,
    uint32_t* aiModifierFlags
) {
    if (!apSettingsContext) {
        return 0;
    }
    uint32_t iflags = 0;
    TESSound* psoundData = static_cast<TESSound*>(apSettingsContext);
    aSoundHandle.SetStaticAttenuation(psoundData->data.iAttenuation);
    uint32_t imin = psoundData->GetMinDistance();
    uint32_t imax = psoundData->GetMaxDistance();
    if (!imin) {
        imin = 14;
    }
    if (!imax) {
        imax = 60;
    }
    if (psoundData->GetStartTime() || psoundData->GetEndTime()) {
        uint8_t iEndTime = psoundData->GetEndTime();
        uint8_t iStartTime = psoundData->GetStartTime();
        aSoundHandle.SetTimeConstraints(iStartTime, iEndTime);
    }
    float fMax = static_cast<double>(imax) * 100.0;
    float fMin = static_cast<double>(imin) * 5.0;
    aSoundHandle.SetMinMax(fMin, fMax);
    uint16_t iCurve4 = psoundData->GetAttenuationCurveValue(4);
    uint16_t iCurve3 = psoundData->GetAttenuationCurveValue(3);
    uint16_t iCurve2 = psoundData->GetAttenuationCurveValue(2);
    uint16_t iCurve1 = psoundData->GetAttenuationCurveValue(1);
    uint16_t iCurve0 = psoundData->GetAttenuationCurveValue(0);
    aSoundHandle.SetAttenuationCurve(iCurve0, iCurve1, iCurve2, iCurve3, iCurve4);
    uint16_t iReverbAttenuation = psoundData->GetReverbAttenuation();
    aSoundHandle.SetReverbAttenuation(iReverbAttenuation);
    uint8_t iPriority = psoundData->GetPriority();
    aSoundHandle.SetPriority(iPriority);
    if ((psoundData->GetData().iFlags & TESSound::FLAG_RANDOM_FREQUENCY_SHIFT) != 0) {
        iflags |= BSGameSound::SOUND_FLAG_VARIABLE_FREQUENCY;
        // The char pitch is zero-extended into the byte parameter.
        aSoundHandle.SetFrequencyVariance(static_cast<uint8_t>(psoundData->GetData().iPitch));
    }
    if ((psoundData->GetData().iFlags & TESSound::FLAG_2D_RADIUS) != 0) {
        iflags |= BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
    }
    if (aiModifierFlags) {
        if ((psoundData->GetData().iFlags & TESSound::FLAG_MUTE_WHEN_SUBMERGED) != 0) {
            *aiModifierFlags |= 1u;
        }
        if ((psoundData->GetData().iFlags & TESSound::FLAG_START_AT_RANDOM_POSITION) != 0) {
            *aiModifierFlags |= 2u;
        }
    }
    uint32_t iLoopFlags = TESSound::ConvertLoopFlag(psoundData->GetData().iFlags);
    return iflags | iLoopFlags;
}

bool FalloutAudio::GetLoopPoints(void* apSettingsContext, uint32_t* aiLoopStart, uint32_t* aiLoopEnd) {
    if (!apSettingsContext) {
        return false;
    }
    TESSound* pSound = static_cast<TESSound*>(apSettingsContext);
    if (!pSound->GetData().iLoopBegin && !pSound->GetData().iLoopEnd) {
        return false;
    }
    *aiLoopStart = pSound->GetData().iLoopBegin;
    *aiLoopEnd = pSound->GetData().iLoopEnd;
    return true;
}

bool FalloutAudio::SoundDistanceTest(TESSound* apSound, NiPoint3 aLoc, float aMod) {
    BSWin32AudioListener* pListener = static_cast<BSWin32AudioListener*>(BSAudio::QInstance().pListener);
    if (!apSound || !pListener) {
        return false;
    }
    NiPoint3* pPos = &pListener->GetPosition();
    float x = pPos->x;
    float y = pPos->y;
    float z = pPos->z;
    float fMaxDist = static_cast<double>(apSound->GetMaxDistance()) * 100.0 * aMod;
    float fDeltaX = aLoc.x - x;
    if (!(static_cast<double>(fMaxDist) < std::fabs(fDeltaX))) {
        float fDeltaY = aLoc.y - y;
        if (!(static_cast<double>(fMaxDist) < std::fabs(fDeltaY))) {
            float fDeltaZ = aLoc.z - z;
            if (!(static_cast<double>(fMaxDist) < std::fabs(fDeltaZ))) {
                return true;
            }
        }
    }
    return false;
}
