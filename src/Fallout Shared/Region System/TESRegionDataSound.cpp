#include "Fallout Shared/Region System/TESRegionDataSound.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MediaSet.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>

TESSound* TESRegionSound::GetSound() const {
    return pDataHandler->GetSound(uiSound);
}

bool TESRegionDataSound::LoadRegionDataSound(TESFile* apFile) {
    CHUNK_ID eChunkID = apFile->GetTESChunk();
    if (eChunkID > RDSD_ID) {
        if (eChunkID == RDSI_ID) {
            uint32_t uiTempInt = 0;
            apFile->GetChunkData<uint32_t>(uiTempInt);
            if (uiTempInt) {
                sounds.pMediaSetIncidentals = uiTempInt;
            }
        }
        else {
            uint32_t uiID[2];
            if (eChunkID != RDMO_ID) {
                return false;
            }
            uiID[1] = 0;
            uiID[0] = 0;
            apFile->GetChunkData<uint32_t>(*uiID);
            if (uiID[0]) {
                if (apFile->QEndian()) {
                    // Swaps the sounds, not the form ID that was read.
                    sounds.Endian();
                }
                TESForm::AddCompileIndex(*uiID, apFile);
                TESForm* pForm = TESForm::GetFormByNumericID(uiID[0]);
                BGSMusicType* pMusic = dynamic_cast<BGSMusicType*>(pForm);
                if (!pMusic) {
                    Warning("MASTERFILE: Unable to find music type (%08X).", uiID[0]);
                }
                SetMusicType(pMusic);
            }
        }
    }
    else {
        switch (eChunkID) {
            case RDSD_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                uint32_t uiCount = uiSize / 12;
                if (uiSize == 4) {
                    return false;
                }
                TESRegionSound* pTmpArray = new TESRegionSound[uiCount];
                apFile->GetChunkData(pTmpArray, static_cast<uint32_t>(sizeof(TESRegionSound) * uiCount));
                for (uint32_t i = 0; i < uiCount; ++i) {
                    TESRegionSound* pTmpSound;
                    pTmpSound = new TESRegionSound;
                    TESRegionSound* pEntry = &pTmpArray[i];
                    pTmpSound->uiSound = pEntry->uiSound;
                    pTmpSound->uiFlags = pEntry->uiFlags;
                    pTmpSound->uiChance = pEntry->uiChance;
                    if (apFile->QEndian()) {
                        pTmpSound->Endian();
                    }
                    TESForm::AddCompileIndex(pTmpSound->uiSound, apFile);
                    if (pTmpSound && pTmpSound->uiSound && pDataHandler->GetSound(pTmpSound->uiSound)) {
                        sounds.soundList.AddTail(pTmpSound);
                    }
                    else {
                        ourFree(pTmpSound);
                    }
                }
                delete[] pTmpArray;
                break;
            }
            case RDSB_ID: {
                uint32_t uiTempInt = 0;
                apFile->GetChunkData<uint32_t>(uiTempInt);
                sounds.MediaSetBattleList.AddTail(uiTempInt);
                break;
            }
            case RDMD_ID: {
                uint32_t uiData = 0;
                apFile->GetChunkData(&uiData, 0);
                break;
            }
            default: {
                return false;
            }
        }
    }
    return true;
}

TESRegionSound::TESRegionSound() : uiSound(0), uiFlags(15), uiChance(0) {}

void TESRegionSound::Endian() {
    BSEndian(uiSound, false);
    BSEndian(uiFlags, false);
    BSEndian(uiChance, false);
}

// pType and pMediaSetIncidentals are deliberately left unset by the constructor.
TESRegionSounds::TESRegionSounds() {}

TESRegionSounds::~TESRegionSounds() {}

void TESRegionSounds::Endian() {
    BSEndian(pType, false);
}

TESRegionDataSound::TESRegionDataSound() : TESRegionData() {
    sounds.pType = nullptr;
    sounds.pMediaSetIncidentals = 0;
}

// Copies the music type and the incidental media set; the sound and battle lists start empty.
TESRegionDataSound::TESRegionDataSound(TESRegionDataSound* arData) : TESRegionData(arData) {
    TESRegionDataSound::SetMusicType(arData->GetMusicType());
    sounds.pMediaSetIncidentals = arData->sounds.pMediaSetIncidentals;
}

TESRegionDataSound::~TESRegionDataSound() {
    ClearSoundList();
}

void TESRegionDataSound::Save() {
    TESRegionData::Save();
    if (GetMusicType()) {
        BGSMusicType* pMusicType = GetMusicType();
        TESForm::AddChunk(RDMO_ID, pMusicType->iFormID);
    }
    if (sounds.pMediaSetIncidentals) {
        TESForm::AddChunk(RDSI_ID, sounds.pMediaSetIncidentals);
    }
    uint32_t uiCount = sounds.MediaSetBattleList.ItemsInList();
    if (uiCount) {
        BSSimpleList<uint32_t>* pTmpList = &sounds.MediaSetBattleList;
        for (uint32_t i = 0; pTmpList && pTmpList->m_item && i < uiCount; ++i) {
            TESForm::AddChunk(RDSB_ID, pTmpList->m_item);
            pTmpList = pTmpList->m_pkNext;
        }
    }
    uiCount = sounds.soundList.ItemsInList();
    if (uiCount) {
        BSSimpleList<TESRegionSound*>* pSoundList = &sounds.soundList;
        TESRegionSound* pTmpArray = new TESRegionSound[uiCount];
        for (uint32_t j = 0; pSoundList && pSoundList->m_item && j < uiCount; ++j) {
            pTmpArray[j] = *pSoundList->m_item;
            pSoundList = pSoundList->m_pkNext;
        }
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t k = 0; k < uiCount; ++k) {
                pTmpArray[k].Endian();
            }
        }
        TESForm::__AddChunkData(RDSD_ID, pTmpArray, static_cast<uint32_t>(sizeof(TESRegionSound) * uiCount));
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t m = 0; m < uiCount; ++m) {
                pTmpArray[m].Endian();
            }
        }
        delete[] pTmpArray;
    }
}

REGION_DATA_ID TESRegionDataSound::GetID() const {
    return REGION_DATA_SOUND_ID;
}

TESRegionData* TESRegionDataSound::Copy() {
    return new TESRegionDataSound(this);
}

TESRegionData* TESRegionDataSound::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataSound::BlendInto(const TESRegionData* apRegionData, uint32_t auiBlend) {
    if (apRegionData && apRegionData->GetID() == REGION_DATA_SOUND_ID && auiBlend) {
        // The source's downcast: GetID says apRegionData is sound data.
        const TESRegionDataSound* pSource = static_cast<const TESRegionDataSound*>(apRegionData);
        if (!bIgnore) {
            if (apRegionData->bIgnore) {
                return;
            }
            if (bOverride) {
                if (apRegionData->bOverride) {
                    int32_t iPriority = apRegionData->cPriority;
                    if (iPriority > cPriority) {
                        SetOverride(apRegionData->bOverride);
                        SetPriority(apRegionData->cPriority);
                        SetMusicType(pSource->GetMusicType());
                    }
                }
                return;
            }
            if (!apRegionData->bOverride) {
                int32_t iPriority = apRegionData->cPriority;
                if (iPriority > cPriority) {
                    SetMusicType(pSource->GetMusicType());
                }
                float fOwnWeight = static_cast<float>(
                    apRegionData->cPriority * (100 - cPriority) + cPriority * cPriority
                );
                float fWeightSum = static_cast<double>(
                                       apRegionData->cPriority * apRegionData->cPriority +
                                       cPriority * (100 - apRegionData->cPriority)
                                   ) +
                    fOwnWeight;
                float fNewPriority = fWeightSum * 0.004999999888241291;
                int32_t iNewPriority = static_cast<int32_t>(std::floor(fNewPriority));
                SetPriority(iNewPriority);
                return;
            }
        }
        SetOverride(apRegionData->bOverride);
        SetPriority(apRegionData->cPriority);
        SetMusicType(pSource->GetMusicType());
    }
}

// True when the region has no music type.
bool TESRegionDataSound::Validate() {
    return sounds.pType == nullptr;
}

bool TESRegionDataSound::SetMusicType(BGSMusicType* apMusicType) {
    if (!apMusicType || apMusicType == sounds.pType) {
        return false;
    }
    sounds.pType = apMusicType;
    return true;
}

MediaSet* TESRegionDataSound::GetMediaSetIncidentals() {
    return static_cast<MediaSet*>(TESForm::GetFormByNumericID(sounds.pMediaSetIncidentals));
}

MediaSet* TESRegionDataSound::GetMediaSetBattle() {
    BSSimpleList<uint32_t>* pList = &sounds.MediaSetBattleList;
    uint32_t pItem = 0;
    // Also tests the list's address, which is never null.
    if (!pList->m_item) {
        return nullptr;
    }
    while (pList && pList->m_item) {
        if (!RandomInt(0, 3) || !pItem) {
            pItem = pList->m_item;
        }
        pList = pList->m_pkNext;
    }
    if (!pItem) {
        return nullptr;
    }
    sounds.MediaSetBattleList.Remove(pItem);
    sounds.MediaSetBattleList.AddHead(pItem);
    return static_cast<MediaSet*>(TESForm::GetFormByNumericID(pItem));
}

// Keeps the media set's form ID, or 0 for none.
void TESRegionDataSound::SetMediaSetIncidentals(MediaSet* ms) {
    uint32_t uiID;
    if (ms) {
        uiID = ms->GetFormID();
    }
    else {
        uiID = 0;
    }
    sounds.pMediaSetIncidentals = uiID;
}

bool TESRegionDataSound::ClearSoundList() {
    TESRegionSound* pSound;
    BSSimpleList<TESRegionSound*>* pList;
    for (pList = &sounds.soundList; pList && pList->m_item; pList = &sounds.soundList) {
        pSound = pList->m_item;
        pList->RemoveHead();
        ourFree(pSound);
    }
    return sounds.soundList.IsEmpty();
}
