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
SettingT<INISettingCollection> gs_fASFadeInTime_Audio("fASFadeInTime:Audio", 3.0f);
SettingT<INISettingCollection> gs_fASFadeOutTime_Audio("fASFadeOutTime:Audio", 5.0f);
SettingT<INISettingCollection> gs_fASFadeStartTime_Audio("fASFadeStartTime:Audio", 2.0f);
SettingT<INISettingCollection> gs_fRegionLoopFadeInTime_Audio("fRegionLoopFadeInTime:Audio", 0.6f);
SettingT<INISettingCollection> gs_fRegionLoopFadeOutTime_Audio("fRegionLoopFadeOutTime:Audio", 1.0f);
SettingT<INISettingCollection> gs_fWeaponInteriorNearVolumeMod("fWeaponInteriorNearVolumeMod", 1.0f);
SettingT<INISettingCollection> gs_fWeaponInteriorNearAttenuationMod("fWeaponInteriorNearAttenuationMod", 3.0f);
SettingT<INISettingCollection> gs_fWeaponInteriorFarVolumeMod("fWeaponInteriorFarVolumeMod", 0.1f);
SettingT<INISettingCollection> gs_fWeaponInteriorFarAttenuationMod("fWeaponInteriorFarAttenuationMod", 0.75f);
SettingT<INISettingCollection> FalloutAudio::iMusicTransitionInMS("iMusicTransitionInMS:Audio", 1000);
SettingT<INISettingCollection> FalloutAudio::iMusicSynchOverride("iMusicSynchOverride:Audio", 0);
BSSoundHandle FalloutAudio::UnderwaterLoop;
BSSoundHandle FalloutAudio::kAmbientLoop;
FalloutAudio::CurrentRegionSounds_t FalloutAudio::CurrentRegionSounds;

void FalloutAudio::UpdateRegionSounds() {
    float ftimeOfDay;
    BGSAcousticSpace* v54;
    FNVD_INLINED_LOCAL(BSSoundHandle, v55, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, v56, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSSoundHandle* plookup;
    BSSimpleList<TESRegionSound*>* soundList = pPlayer->GetRegionSounds();
    BSAudio* v0 = &BSAudio::QInstance();
    bool blistenerUnderwater = v0->IsListenerUnderwater();
    if (pPlayer) {
        BGSAcousticSpace* pAcousticSpace;
        if (pPlayer->GetParentCell()) {
            pAcousticSpace = pPlayer->GetParentCell()->GetAcousticSpace();
        }
        else {
            pAcousticSpace = nullptr;
        }
        v54 = pAcousticSpace;
    }
    else {
        v54 = nullptr;
    }
    if ((FalloutAudio::pCurrentSpace) && (!FalloutAudio::pCurrentSpace->IsInterior()) && (v54) && (v54->IsInterior())) {
        FalloutAudio::SetAcousticSpace(v54);
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
            TESSound* aPosition_8 = FalloutAudio::pNoActivationSound;
            const char* aPosition = FalloutAudio::pNoActivationSound->GetFilename();
            BSAudio* v1 = &BSAudio::QInstance();
            v1->PrecacheSound(aPosition, 289u, aPosition_8);
        }
    }
    if (FalloutAudio::pCurrentSpace ||
        (!pPlayer ||
         (!pPlayer->GetParentCell() || pPlayer->GetParentCell()->GetAcousticSpace() == FalloutAudio::pCurrentSpace))) {
        if (!pPlayer || (!pPlayer->GetParentCell() && !pPlayer->IsAcousticSpaceTransition())) {
            FalloutAudio::ClearAcousticSpace(nullptr);
        }
    }
    else {
        BGSAcousticSpace* AcousticSpace = pPlayer->GetParentCell()->GetAcousticSpace();
        FalloutAudio::SetAcousticSpace(AcousticSpace);
    }
    if ((FalloutAudio::pCurrentSpace) && (FalloutAudio::pCurrentSpace->pSoundRegion)) {
        TESRegion* pregion = FalloutAudio::pCurrentSpace->pSoundRegion;
        TESRegionDataSound* pRSD = static_cast<TESRegionDataSound*>(pregion->pDataList->Find(REGION_DATA_SOUND_ID));
        if (pRSD) {
            soundList = &pRSD->sounds.soundList;
        }
    }

    if (FalloutAudio::pCurrentSpace) {
        uint32_t aPosition_8a = FalloutAudio::pCurrentSpace->eEnvType;
        BSAudio* v3 = &BSAudio::QInstance();
        v3->SetEnvironment(aPosition_8a);
    }
    else {
        BSAudio* v4 = &BSAudio::QInstance();
        v4->SetEnvironment(0);
    }
    if (blistenerUnderwater && (pPlayer->GetParentCell() && !FalloutAudio::UnderwaterLoop.IsPlaying())) {
        BSAudio* v5 = &BSAudio::QInstance();
        BSSoundHandle* arHandle = v5->GetSoundHandleByName(&a1, "AMBUnderwaterLP", 0x2101u);
        FalloutAudio::UnderwaterLoop.operator=(*arHandle);
        FalloutAudio::UnderwaterLoop.bAssumeSuccess = true;
        FalloutAudio::UnderwaterLoop.Play(false);
    }
    else {
        if ((!blistenerUnderwater || !pPlayer->GetParentCell()) && FalloutAudio::UnderwaterLoop.IsPlaying()) {
            FalloutAudio::UnderwaterLoop.FadeOutAndRelease(300u);
        }
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
    BSAudio* v6 = &BSAudio::QInstance();
    v6->SetConditions(ftimeOfDay, 0);
    uint32_t SynchTime = (&BSAudio::QInstance())->GetSynchTime();
    if (BSAbs(static_cast<int32_t>(SynchTime - FalloutAudio::uiLastRegionUpdate)) <= 1000) {
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
    float fFadeInTime = gs_fASFadeStartTime_Audio.Float();
    if (FalloutAudio::kAmbientLoop.IsPlaying()) {
        fFadeInTime = gs_fASFadeInTime_Audio.Float();
    }
    if (FalloutAudio::pCurrentSpace && pPlayer) {
        TESSound* pambSound_2;
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
                    float* v8 = &gs_fASFadeOutTime_Audio.Float();
                    FalloutAudio::kAmbientLoop.FadeOutAndRelease(static_cast<int64_t>(*v8 * 500.0));
                    FalloutAudio::kAmbientLoop.Clear();
                }
                FalloutAudio::bUseWalla = false;
                TESSound* pambSound = FalloutAudio::pCurrentSpace->pDawnSound;
                if (pambSound) {
                    const char* aPositiona = pambSound->GetFilename();
                    BSAudio* v9 = &BSAudio::QInstance();
                    BSSoundHandle* kDawnSoundHandle =
                        v9->GetSoundHandleByFilename(&out, aPositiona, 0x11011u, pambSound);
                    FalloutAudio::kAmbientLoop.operator=(*kDawnSoundHandle);
                    FalloutAudio::kAmbientLoop.FadeInPlay(static_cast<int64_t>(fFadeInTime * 1000.0));
                }
            }
            goto LABEL_97;
        }
        bool bHasChanged = FalloutAudio::bUpdateAcousticSpace;
        if (!FalloutAudio::bUpdateAcousticSpace) {
            switch (FalloutAudio::iCurrentTime) {
                case 0: {
                    float* v10 = &gs_fDayTime_Audio.Float();
                    if (*v10 < static_cast<double>(ftimeOfDay)) {
                        bHasChanged = true;
                    }
                    break;
                }
                case 1: {
                    float* v11 = &gs_fDuskTime_Audio.Float();
                    if (*v11 < static_cast<double>(ftimeOfDay)) {
                        bHasChanged = true;
                    }
                    break;
                }
                case 2: {
                    float* v12 = &gs_fNightTime_Audio.Float();
                    if (*v12 < static_cast<double>(ftimeOfDay)) {
                        bHasChanged = true;
                    }
                    break;
                }
                case 3: {
                    float* v13 = &gs_fDawnTime_Audio.Float();
                    if (*v13 < static_cast<double>(ftimeOfDay)) {
                        float* v14 = &gs_fNightTime_Audio.Float();
                        if (*v14 > static_cast<double>(ftimeOfDay)) {
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
        if (!bHasChanged) {
            goto LABEL_97;
        }
        if (FalloutAudio::bUpdateAcousticSpace) {
            FalloutAudio::pCurrentCellSpace = nullptr;
            FalloutAudio::bUpdateAcousticSpace = false;
        }
        else {
            float* v15 = &gs_fASFadeOutTime_Audio.Float();
            FalloutAudio::kAmbientLoop.FadeOutAndRelease(static_cast<int64_t>(*v15 * 1000.0));
            FalloutAudio::kAmbientLoop.Clear();
        }
        FalloutAudio::bUseWalla = false;
        float* v16 = &gs_fDawnTime_Audio.Float();
        if (*v16 < static_cast<double>(ftimeOfDay)) {
            float* v17 = &gs_fDayTime_Audio.Float();
            if (!(*v17 < static_cast<double>(ftimeOfDay))) {
                pambSound_2 = FalloutAudio::pCurrentSpace->pDawnSound;
                FalloutAudio::iCurrentTime = 0;
                goto LABEL_93;
            }
            float* v18 = &gs_fDuskTime_Audio.Float();
            if (!(*v18 < static_cast<double>(ftimeOfDay))) {
                pambSound_2 = FalloutAudio::pCurrentSpace->pNoonSound;
                FalloutAudio::iCurrentTime = 1;
                goto LABEL_93;
            }
            float* v19 = &gs_fNightTime_Audio.Float();
            if (!(*v19 < static_cast<double>(ftimeOfDay))) {
                pambSound_2 = FalloutAudio::pCurrentSpace->pDuskSound;
                FalloutAudio::iCurrentTime = 2;
                goto LABEL_93;
            }
        }
        pambSound_2 = FalloutAudio::pCurrentSpace->pNightSound;
        FalloutAudio::iCurrentTime = 3;
    LABEL_93:;
        if (pambSound_2) {
            const char* aPositionb = pambSound_2->GetFilename();
            BSAudio* v20 = &BSAudio::QInstance();
            BSSoundHandle* pTODSoundHandle = v20->GetSoundHandleByFilename(&th_s, aPositionb, 0x11011u, pambSound_2);
            FalloutAudio::kAmbientLoop.operator=(*pTODSoundHandle);
            FalloutAudio::kAmbientLoop.FadeInPlay(static_cast<int64_t>(fFadeInTime * 1000.0));
        }
        goto LABEL_97;
    }
    FalloutAudio::kAmbientLoop.FadeOutAndRelease(1000u);
    FalloutAudio::kAmbientLoop.Clear();
LABEL_97:;
    if (!soundList->IsEmpty() && !Interface::IsTopMenuID(Interface::eStartMenu)) {
        TESRegionSound* prgnSound;
        TESSound* psoundData;
        BSSimpleList<TESRegionSound*>* plistPtr = soundList;
        BSSoundHandle hndle(0);
        bool bcanPlay = true;
        bool bisLoop = false;
        BSAudioListener* plstnr = (&BSAudio::QInstance())->pListener;
        while (plistPtr && !plistPtr->IsEmpty()) {
            bcanPlay = true;
            prgnSound = plistPtr->m_item;
            // The source keeps the resolved sound in the form ID field (TESRegionDataSound's InitItem); the exe
            // reads it before testing prgnSound.
#if FNVD_64BIT
            psoundData = prgnSound->GetSound();
#else
            psoundData = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(prgnSound->uiSound));
#endif
            if (prgnSound && psoundData) {
                float fBeginTime = static_cast<double>(psoundData->GetStartTime()) * 0.09375;
                float fEndTime = static_cast<double>(psoundData->GetEndTime()) * 0.09375;
                bisLoop = TESSound::ConvertLoopFlag(psoundData->GetData().iFlags) != 0;
                if (psky) {
                    if (((prgnSound->uiFlags & 0xFFFF) != 0) && (psky->pCurrentWeather)) {
                        uint32_t icurrentConditions = psky->pCurrentWeather->GetWeatherDataRaw(TESWeather::WD_DIVIDER2);
                        if ((((prgnSound->uiFlags & 0xFFFF) & icurrentConditions) == 0) && (icurrentConditions)) {
                            bcanPlay = false;
                        }
                    }

                    float fSpan = fEndTime - fBeginTime;
                    if (std::fabs(fSpan) >= 0.09375 &&
                        ((fEndTime > static_cast<double>(fBeginTime) &&
                          (ftimeOfDay < static_cast<double>(fBeginTime) ||
                           ftimeOfDay > static_cast<double>(fEndTime))) ||
                         (fEndTime < static_cast<double>(fBeginTime) &&
                          (ftimeOfDay < static_cast<double>(fBeginTime) &&
                           ftimeOfDay > static_cast<double>(fEndTime))))) {
                        bcanPlay = false;
                    }
                }
                bool bisInMap = FalloutAudio::CurrentRegionSounds.GetAt(psoundData->iFormID, plookup);
                if (bisLoop && (!bisInMap && bcanPlay)) {
                    BSSoundHandle* phndle;
                    BSSoundHandle* pMemory = static_cast<BSSoundHandle*>(ourMalloc(sizeof(BSSoundHandle)));
                    if (pMemory) {
                        phndle = ::new (pMemory) BSSoundHandle;
                    }
                    else {
                        phndle = nullptr;
                    }
                    TESSound* pSoundForm = psoundData;
                    const char* pFilename = psoundData->GetFilename();
                    BSAudio* pAudio = &BSAudio::QInstance();
                    BSSoundHandle* pSoundHandle =
                        pAudio->GetSoundHandleByFilename(&apResult, pFilename, 0x11011u, pSoundForm);
                    phndle->operator=(*pSoundHandle);
                    phndle->bAssumeSuccess = true;
                    FalloutAudio::CurrentRegionSounds.SetAt(psoundData->iFormID, phndle);
                    float* pfFadeIn = &gs_fRegionLoopFadeInTime_Audio.Float();
                    phndle->FadeInPlay(static_cast<int64_t>(*pfFadeIn * 1000.0));
                }
                else {
                    if (bisLoop && bisInMap) {
                        if (!plookup->IsPlaying() || bcanPlay) {
                            if (plookup->IsValid() && !plookup->IsPlaying() && bcanPlay) {
                                float* pfFadeIn = &gs_fRegionLoopFadeInTime_Audio.Float();
                                plookup->FadeInPlay(static_cast<int64_t>(*pfFadeIn * 1000.0));
                            }
                        }
                        else {
                            float* pfFadeOut = &gs_fRegionLoopFadeOutTime_Audio.Float();
                            plookup->FadeOutAndRelease(static_cast<int64_t>(*pfFadeOut * 1000.0));
                            FalloutAudio::CurrentRegionSounds.RemoveAt(psoundData->iFormID);
                            if (plookup) {
                                delete plookup;
                            }
                        }
                    }
                    else {
                        if (!bisLoop && bcanPlay) {
                            float fchance = static_cast<double>(prgnSound->uiChance) / 2e+07;
                            if (RandomBool(fchance)) {
                                if ((psoundData->GetData().iFlags & 64) != 0) {
                                    TESSound* pSoundForm = psoundData;
                                    const char* pFilename = psoundData->GetFilename();
                                    BSAudio* pAudio = &BSAudio::QInstance();
                                    BSSoundHandle* pSoundHandle =
                                        pAudio->GetSoundHandleByFilename(&v56, pFilename, 0x11101u, pSoundForm);
                                    hndle.operator=(*pSoundHandle);
                                }
                                else {
                                    TESSound* pSoundForm = psoundData;
                                    const char* pFilename = psoundData->GetFilename();
                                    BSAudio* pAudio = &BSAudio::QInstance();
                                    BSSoundHandle* pSoundHandle =
                                        pAudio->GetSoundHandleByFilename(&v55, pFilename, 0x11102u, pSoundForm);
                                    hndle.operator=(*pSoundHandle);
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
                }
            }
            plistPtr = plistPtr->m_pkNext;
        }
        uint32_t ikey = 0;
        // The found flag is cleared and the list restarted once, before the map loop, not per entry, as the game does.
        bool bisInList = false;
        NiTMapItem<uint32_t, BSSoundHandle*>* pPos = FalloutAudio::CurrentRegionSounds.GetFirstPos();
        plistPtr = soundList;
        while (pPos) {
            FalloutAudio::CurrentRegionSounds.GetNext(pPos, ikey, plookup);
            while (plistPtr && !plistPtr->IsEmpty()) {
                prgnSound = plistPtr->m_item;
#if FNVD_64BIT
                psoundData = prgnSound->GetSound();
#else
                psoundData = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(prgnSound->uiSound));
#endif
                if (ikey == psoundData->iFormID) {
                    bisInList = true;
                    break;
                }
                plistPtr = plistPtr->m_pkNext;
            }
            if (!bisInList) {
                FalloutAudio::CurrentRegionSounds.RemoveAt(ikey);
                pPos = FalloutAudio::CurrentRegionSounds.GetFirstPos();
                float* pfFadeOut = &gs_fRegionLoopFadeOutTime_Audio.Float();
                plookup->FadeOutAndRelease(static_cast<int64_t>(*pfFadeOut * 1000.0));
                if (plookup) {
                    delete plookup;
                }
                plookup = nullptr;
            }
        }
    }
    FalloutAudio::uiLastRegionUpdate = (&BSAudio::QInstance())->GetSynchTime();
}

void PlayMysteriousStrangerMusic() {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSAudio* v0 = &BSAudio::QInstance();
    v0->GetSoundHandleByName(
        &a1, "MUSMysteriousStrangerA01",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_DONT_CACHE |
            BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE
    );
    a1.Play(false);
    BSAudio* v1 = &BSAudio::QInstance();
    v1->Precache(
        "MUSMysteriousStrangerA02",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE |
            BSGameSound::SOUND_FLAG_UNKBIT29
    );
}

void PlaySound_MUSMysteriousStrangerA02() {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSAudio* v0 = &BSAudio::QInstance();
    v0->GetSoundHandleByName(
        &a1, "MUSMysteriousStrangerA02",
        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE
    );
    a1.Play(false);
}

void FalloutAudio::ClearAcousticSpace(BGSAcousticSpace* apSpace) {
    float* v1;
    BSAudio* singleton;
    if (((!FalloutAudio::bLockAcousticSpace) && ((!apSpace) || (apSpace == FalloutAudio::pCurrentSpace))) &&
        (FalloutAudio::pCurrentSpace)) {
        v1 = &gs_fASFadeOutTime_Audio.Float();
        (&FalloutAudio::kAmbientLoop)->FadeOutAndRelease((int64_t)((*v1) * 1000.0));
        (&FalloutAudio::kAmbientLoop)->Clear();
        FalloutAudio::pCurrentSpace = nullptr;
        FalloutAudio::bUpdateAcousticSpace = true;
        singleton = &BSAudio::QInstance();
        singleton->SetEnvironment(0);
    }
}

void FalloutAudio::DisableRegionSoundUpdates(bool abIn) {
    FalloutAudio::bRegionUpdateDisabled = (bool)((uint8_t)abIn);
}

void FalloutAudio::SetNextAcousticSpace(BGSAcousticSpace* apSpace) {
    FalloutAudio::pCurrentCellSpace = apSpace;
}

void FalloutAudio::SetAcousticSpace(BGSAcousticSpace* apSpace) {
    BSAudio* singleton;
    uint32_t sound1;
    int64_t v3;
    if (FalloutAudio::pCurrentSpace != apSpace) {
        v3 = (int64_t)((&gs_fASFadeOutTime_Audio)->Float() * 1000.0);
        (&FalloutAudio::kAmbientLoop)->FadeOutAndRelease(v3);
        (&FalloutAudio::kAmbientLoop)->Clear();
        if (apSpace) {
            sound1 = (uint32_t)apSpace->eEnvType;
            singleton = &BSAudio::QInstance();
            singleton->SetEnvironment(sound1);
        }
        FalloutAudio::pCurrentSpace = apSpace;
        FalloutAudio::bUpdateAcousticSpace = true;
    }
}

void FalloutAudio::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {}

void FalloutAudio::SetMainMenuMusic(bool abMainMenuMusic) {
    FalloutAudio::bMainMenuMusic = (bool)((uint8_t)abMainMenuMusic);
}

void FalloutAudio::MuteRadio() {
    FalloutAudio::bRadioMute = true;
}

void FalloutAudio::UnMuteRadio() {
    FalloutAudio::bRadioMute = false;
}

bool FalloutAudio::GetNewGameMusic() {
    return (bool)((int8_t)FalloutAudio::bNewGameMusic);
}

bool FalloutAudio::IsRadioMute() {
    return (bool)((int8_t)FalloutAudio::bRadioMute);
}

void FalloutAudio::SetNewGameMusic(bool abNewGameMusic) {
    FalloutAudio::bNewGameMusic = (bool)((int8_t)abNewGameMusic);
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
    if (!psound || (!psound->GetFilename() || !apFilename)) {
        return false;
    }
    const char* Filename = psound->GetFilename();
    BSstrcpy(apFilename, 260, Filename);
    if ((psound->GetData().iFlags & 64) != 0) {
        *aiSoundTypeFlags |= 1u;
        *aiSoundTypeFlags &= ~2u;
    }
    else {
        if ((*aiSoundTypeFlags & 1) == 0 && (*aiSoundTypeFlags & 2) == 0) {
            *aiSoundTypeFlags |= 2u;
            *aiSoundTypeFlags &= ~1u;
        }
    }
    if ((psound->GetData().iFlags & 1) != 0) {
        *aiSoundTypeFlags |= 64u;
    }
    if ((psound->GetData().iFlags & 2048) != 0) {
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
    if (psound && (psound->GetFilename() && apFilename)) {
        const char* Filename = psound->GetFilename();
        BSstrcpy(apFilename, 260, Filename);
        if ((psound->GetData().iFlags & 64) != 0) {
            *aiSoundTypeFlags |= 1u;
            *aiSoundTypeFlags &= ~2u;
        }
        else {
            if ((*aiSoundTypeFlags & 1) == 0 && (*aiSoundTypeFlags & 2) == 0) {
                *aiSoundTypeFlags |= 2u;
                *aiSoundTypeFlags &= ~1u;
            }
        }
        if ((psound->GetData().iFlags & 1) != 0) {
            *aiSoundTypeFlags |= 64u;
        }
        if ((psound->GetData().iFlags & 2048) != 0) {
            *aiSoundTypeFlags |= BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
        }
        *aiSoundTypeFlags |= TESSound::ConvertLoopFlag(psound->GetData().iFlags);
        *apSettingsContext = (void*)psound;
        return true;
    }

    if ((pDataHandler) && (!pDataHandler->GetSaveLoad()) && (pDataHandler->CountCompiled())) {
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
    if ((psoundData->GetData().iFlags & 1) != 0) {
        iflags |= 64u;
        // The char pitch is zero-extended into the byte parameter.
        aSoundHandle.SetFrequencyVariance(static_cast<uint8_t>(psoundData->GetData().iPitch));
    }
    if ((psoundData->GetData().iFlags & 0x800) != 0) {
        iflags |= 0x8000000u;
    }
    if (aiModifierFlags) {
        if ((psoundData->GetData().iFlags & 0x1000) != 0) {
            *aiModifierFlags |= 1u;
        }
        if ((psoundData->GetData().iFlags & 0x2000) != 0) {
            *aiModifierFlags |= 2u;
        }
    }
    uint32_t iLoopFlags = TESSound::ConvertLoopFlag(psoundData->GetData().iFlags);
    return iflags | iLoopFlags;
}

bool FalloutAudio::GetLoopPoints(void* apSettingsContext, uint32_t* aiLoopStart, uint32_t* aiLoopEnd) {
    TESSound::SOUND_DATA v4;
    TESSound::SOUND_DATA v5;
    TESSound::SOUND_DATA v6;
    TESSound::SOUND_DATA result;
    if (!apSettingsContext) {
        return false;
    }
    TESSound* v8 = static_cast<TESSound*>(apSettingsContext);
    if (!(&(result = static_cast<TESSound*>(apSettingsContext)->GetData()))->iLoopBegin &&
        !(&(v6 = v8->GetData()))->iLoopEnd) {
        return false;
    }
    *aiLoopStart = (&(v5 = v8->GetData()))->iLoopBegin;
    *aiLoopEnd = (&(v4 = v8->GetData()))->iLoopEnd;
    return true;
}

bool FalloutAudio::SoundDistanceTest(TESSound* apSound, NiPoint3 aLoc, float aMod) {
    BSAudio* v3 = &BSAudio::QInstance();
    BSWin32AudioListener* pListener = static_cast<BSWin32AudioListener*>(v3->pListener);
    if (!apSound || !pListener) {
        return false;
    }
    NiPoint3* v5 = &pListener->GetPosition();
    float x = v5->x;
    float y = v5->y;
    float z = v5->z;
    float v12 = static_cast<double>(apSound->GetMaxDistance()) * 100.0 * aMod;
    float v8 = aLoc.x - x;
    bool result = false;
    if (!(static_cast<double>(v12) < std::fabs(v8))) {
        float v7 = aLoc.y - y;
        if (!(static_cast<double>(v12) < std::fabs(v7))) {
            float v6 = aLoc.z - z;
            if (!(static_cast<double>(v12) < std::fabs(v6))) {
                return true;
            }
        }
    }
    return result;
}
