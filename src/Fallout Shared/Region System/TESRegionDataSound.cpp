#include "Fallout Shared/Region System/TESRegionDataSound.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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
#if FNVD_64BIT
    return pDataHandler->GetSound(uiSound);
#else
    return reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSound));
#endif
}

bool TESRegionDataSound::LoadRegionDataSound(TESFile* a1) {
    CHUNK_ID entrySize = a1->GetTESChunk();
    if (entrySize > RDSD_ID) {
        if (entrySize == RDSI_ID) {
            uint32_t tempint = 0;
            a1->GetChunkData<uint32_t>(tempint);
            if (tempint) {
                sounds.pMediaSetIncidentals = tempint;
            }
        }
        else {
            uint32_t aiID[2];
            if (entrySize != RDMO_ID) {
                return false;
            }
            aiID[1] = 0;
            aiID[0] = 0;
            a1->GetChunkData<uint32_t>(*aiID);
            if (aiID[0]) {
                if (a1->QEndian()) {
                    // Swaps the sounds, not the form ID that was read, as the game does.
                    sounds.Endian();
                }
                TESForm::AddCompileIndex(*aiID, a1);
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(aiID[0]);
                BGSMusicType* pmusic = dynamic_cast<BGSMusicType*>(FormByNumericID);
                if (!pmusic) {
                    Warning("MASTERFILE: Unable to find music type (%08X).", aiID[0]);
                }
                SetMusicType(pmusic);
            }
        }
    }
    else {
        switch (entrySize) {
            case RDSD_ID: {
                TESRegionSound* tmpArray_2;
                uint32_t uisize = a1->GetChunkSize();
                uint32_t uicount = uisize / 12;
                if (uisize == 4) {
                    return false;
                }
                TESRegionSound* tmpArray = static_cast<TESRegionSound*>(ourMalloc(
                    uicount <= SIZE_MAX / sizeof(TESRegionSound) ? sizeof(TESRegionSound) * uicount : SIZE_MAX
                ));
                if (tmpArray) {
                    rt::eh_vector_constructor_iterator(
                        tmpArray, sizeof(TESRegionSound), uicount, &fnvd::constructElement<TESRegionSound>
                    );
                    tmpArray_2 = tmpArray;
                }
                else {
                    tmpArray_2 = nullptr;
                }
                a1->GetChunkData(tmpArray_2, static_cast<uint32_t>(sizeof(TESRegionSound) * uicount));
                for (uint32_t i = 0; i < uicount; ++i) {
                    TESRegionSound* tmpSound;
                    TESRegionSound* v9 = static_cast<TESRegionSound*>(ourMalloc(sizeof(TESRegionSound)));
                    if (v9) {
                        tmpSound = ::new (v9) TESRegionSound;
                    }
                    else {
                        tmpSound = nullptr;
                    }
                    TESRegionSound* tmpSound_2 = tmpSound;
                    TESRegionSound* v4 = &tmpArray_2[i];
                    tmpSound->uiSound = v4->uiSound;
                    tmpSound->uiFlags = v4->uiFlags;
                    tmpSound->uiChance = v4->uiChance;
                    if (a1->QEndian()) {
                        tmpSound_2->Endian();
                    }
                    TESForm::AddCompileIndex(tmpSound_2->uiSound, a1);
                    if (tmpSound_2 && (tmpSound_2->uiSound && pDataHandler->GetSound(tmpSound_2->uiSound))) {
#if !FNVD_64BIT
                        TESSound* ptmpSound = pDataHandler->GetSound(tmpSound_2->uiSound);
                        tmpSound_2->uiSound = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptmpSound));
#endif
                        sounds.soundList.AddTail(tmpSound_2);
                    }
                    else {
                        ourFree(tmpSound_2);
                    }
                }
                ourFree(tmpArray_2);
                break;
            }
            case RDSB_ID: {
                uint32_t tempint_2 = 0;
                a1->GetChunkData<uint32_t>(tempint_2);
                sounds.MediaSetBattleList.AddTail(tempint_2);
                break;
            }
            case RDMD_ID: {
                uint32_t uiData = 0;
                a1->GetChunkData(&uiData, 0);
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
    ENDIAN(uiSound, false);
    ENDIAN(uiFlags, false);
    ENDIAN(uiChance, false);
}

// pType and pMediaSetIncidentals are deliberately left unset by the constructor, as the game does.
TESRegionSounds::TESRegionSounds() {}

TESRegionSounds::~TESRegionSounds() {}

void TESRegionSounds::Endian() {
    ENDIAN(pType, false);
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
        BGSMusicType* v1 = GetMusicType();
        TESForm::AddChunk(RDMO_ID, v1->iFormID);
    }
    if (sounds.pMediaSetIncidentals) {
        TESForm::AddChunk(RDSI_ID, sounds.pMediaSetIncidentals);
    }
    uint32_t uicount = sounds.MediaSetBattleList.ItemsInList();
    if (uicount) {
        BSSimpleList<uint32_t>* tmpList = &sounds.MediaSetBattleList;
        for (uint32_t i = 0; tmpList && (tmpList->m_item && i < uicount); ++i) {
            TESForm::AddChunk(RDSB_ID, tmpList->m_item);
            tmpList = tmpList->m_pkNext;
        }
    }
    uint32_t uicount_2 = sounds.soundList.ItemsInList();
    if (uicount_2) {
        TESRegionSound* tmpArray_2;
        BSSimpleList<TESRegionSound*>* tmpList_2 = &sounds.soundList;
        TESRegionSound* tmpArray = static_cast<TESRegionSound*>(
            ourMalloc(uicount_2 <= SIZE_MAX / sizeof(TESRegionSound) ? sizeof(TESRegionSound) * uicount_2 : SIZE_MAX)
        );
        if (tmpArray) {
            rt::eh_vector_constructor_iterator(
                tmpArray, sizeof(TESRegionSound), static_cast<int32_t>(uicount_2),
                &fnvd::constructElement<TESRegionSound>
            );
            tmpArray_2 = tmpArray;
        }
        else {
            tmpArray_2 = nullptr;
        }
        for (uint32_t j = 0; tmpList_2 && (tmpList_2->m_item && j < uicount_2); ++j) {
            tmpArray_2[j] = *tmpList_2->m_item;
            tmpList_2 = tmpList_2->m_pkNext;
        }
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t k = 0; k < uicount_2; ++k) {
                tmpArray_2[k].Endian();
            }
        }
        TESForm::__AddChunkData(RDSD_ID, tmpArray_2, static_cast<uint32_t>(sizeof(TESRegionSound) * uicount_2));
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t m = 0; m < uicount_2; ++m) {
                tmpArray_2[m].Endian();
            }
        }
        ourFree(tmpArray_2);
    }
}

REGION_DATA_ID TESRegionDataSound::GetID() const {
    return REGION_DATA_SOUND_ID;
}

TESRegionData* TESRegionDataSound::Copy() {
    void* v4 = ourMalloc(sizeof(TESRegionDataSound));
    if (v4) {
        return ::new (v4) TESRegionDataSound(this);
    }
    else {
        return nullptr;
    }
}

TESRegionData* TESRegionDataSound::Blend(const TESRegionData* apRegionData) {
    TESRegionData* v3 = Copy();
    v3->BlendInto(apRegionData, 2u);
    return v3;
}

void TESRegionDataSound::BlendInto(const TESRegionData* a1, uint32_t a2) {
    if (a1 && (a1->GetID() == REGION_DATA_SOUND_ID && a2)) {
        // The source's downcast: GetID says a1 is sound data.
        const TESRegionDataSound* pSource = static_cast<const TESRegionDataSound*>(a1);
        if (bIgnore) {
            goto LABEL_6;
        }
        if (a1->bIgnore) {
            return;
        }
        if (bOverride) {
            if (a1->bOverride) {
                int32_t iPriority = a1->cPriority;
                if (iPriority > cPriority) {
                    SetOverride(a1->bOverride);
                    uint8_t v6 = a1->cPriority;
                    SetPriority(v6);
                    SetMusicType(pSource->GetMusicType());
                }
            }
            return;
        }
        if (a1->bOverride) {
        LABEL_6:;
            SetOverride(a1->bOverride);
            uint8_t v3 = a1->cPriority;
            SetPriority(v3);
            SetMusicType(pSource->GetMusicType());
            return;
        }
        int32_t v8 = a1->cPriority;
        if (v8 > cPriority) {
            SetMusicType(pSource->GetMusicType());
        }
        int32_t v10 = cPriority;
        int32_t v11 = cPriority * v10;
        int32_t v12 = 100 - cPriority;
        float v18 = static_cast<float>(a1->cPriority * v12 + v11);
        int32_t v13 = 100 - a1->cPriority;
        int32_t v14 = cPriority * v13;
        int32_t v15 = a1->cPriority;
        float v19 = static_cast<double>(a1->cPriority * v15 + v14) + v18;
        float fnewPriority = v19 * 0.004999999888241291;
        int32_t v16 = static_cast<int32_t>(std::floor(fnewPriority));
        SetPriority(v16);
    }
}

// True when the region has no music type.
bool TESRegionDataSound::Validate() {
    return sounds.pType == nullptr;
}

bool TESRegionDataSound::SetMusicType(BGSMusicType* a2) {
    if ((!a2) || (a2 == sounds.pType)) {
        return false;
    }
    sounds.pType = a2;
    return true;
}

MediaSet* TESRegionDataSound::GetMediaSetIncidentals() {
    return static_cast<MediaSet*>(TESForm::GetFormByNumericID((int32_t)sounds.pMediaSetIncidentals));
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
    TESRegionSound* v3;
    BSSimpleList<TESRegionSound*>* i;
    for (i = &sounds.soundList; i && i->m_item; i = &sounds.soundList) {
        v3 = i->m_item;
        i->RemoveHead();
        ourFree(v3);
    }
    return (bool)((uint8_t)(&sounds.soundList)->IsEmpty());
}
