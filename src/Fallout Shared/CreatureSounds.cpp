#include "Fallout Shared/CreatureSounds.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESSound.h"

CreatureSounds::CreatureSounds() : pSoundListArray{} {}

CreatureSounds::~CreatureSounds() {
    FreeAllCreatureSoundLists();
}

void CreatureSounds::FreeCreatureSoundList(CREATURE_SOUND_TYPE aeType) {
    for (BSSimpleList<CREATURE_SOUND*>* pList = GetCreatureSoundList(aeType); pList && !pList->IsEmpty();
         pList->RemoveHead()) {
        ourFree(pList->m_item);
    }
    delete pSoundListArray[aeType];
    pSoundListArray[aeType] = nullptr;
}

void CreatureSounds::Copy(CreatureSounds* apCopy) {
    FreeAllCreatureSoundLists();
    if (apCopy) {
        for (char i = 0; i < 22; ++i) {
            for (BSSimpleList<CREATURE_SOUND*>* pIter =
                     apCopy->GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
                 pIter && !pIter->IsEmpty();
                 pIter = pIter->m_pkNext) {
                CREATURE_SOUND* pCopyFrom = pIter->m_item;
                CREATURE_SOUND* pCopyTo = static_cast<CREATURE_SOUND*>(ourMalloc(sizeof(CREATURE_SOUND)));
                CreatureSounds::CopyCreatureSound(pCopyTo, pCopyFrom);
                AddCreatureSound(pCopyTo, static_cast<CREATURE_SOUND_TYPE>(i));
            }
        }
    }
}

// Walks the type's list until an entry with a sound wins its percent roll; null when none does.
TESSound* CreatureSounds::PickCreatureSound(CREATURE_SOUND_TYPE aeType) {
    TESSound* pSound = nullptr;
    for (BSSimpleList<CREATURE_SOUND*>* pIter = GetCreatureSoundList(aeType); pIter && !pIter->IsEmpty() && !pSound;
         pIter = pIter->GetNext()) {
        CREATURE_SOUND* pEntry = pIter->GetItem();
        if (pEntry->pSound && Random() % 100 < pEntry->iChance) {
            pSound = pEntry->pSound;
        }
    }
    return pSound;
}

void CreatureSounds::FreeAllCreatureSoundLists() {
    for (int8_t i = 0; i < CREA_SOUND_COUNT; ++i) {
        FreeCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
    }
}

// Chunks 0x49445343 'CSDI' (sound form ID), 0x46445343 'CSDF' (sound file name, 0x104 bytes) and 0x43445343 'CSDC'
// (one chance byte). The 8-byte entry is freed again when no CSDC follows.
bool CreatureSounds::LoadCreatureSound(TESFile* apFile, CREATURE_SOUND_TYPE aeType, TESCreature* apOwner) {
    bool bLoaded = false;
    if (!apFile) {
        return bLoaded;
    }
    CREATURE_SOUND* pEntry = nullptr;
    if (apFile->GetTESChunk() == 0x49445343) {
        uint32_t uiID = 0;
        apFile->GetChunkData<uint32_t>(uiID);
        pEntry = static_cast<CREATURE_SOUND*>(ourMalloc(sizeof(CREATURE_SOUND)));
        if (uiID) {
            TESForm::AddCompileIndex(uiID, apFile);
            pEntry->pSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(uiID));
            if (!pEntry->pSound) {
                uint32_t uiOwnerID = apOwner->GetFormID();
                const char* pOwnerName = apOwner->GetFormEditorID();
                Warning(
                    "AUDIO: Unable to find CreatureSound ID (%08X) for creature '%s' (%08X).",
                    uiID,
                    pOwnerName,
                    uiOwnerID
                );
            }
        }
        else {
            pEntry->pSound = nullptr;
        }
        apFile->NextChunk();
    }
    else if (apFile->GetTESChunk() == 0x46445343) {
        char acFilename[0x104];
        apFile->GetChunkData(acFilename, 0x104);
        pEntry = static_cast<CREATURE_SOUND*>(ourMalloc(sizeof(CREATURE_SOUND)));
        pEntry->pSound = TESSound::FindSoundByFilename(acFilename);
        apFile->NextChunk();
    }
    if (pEntry) {
        if (apFile->GetTESChunk() == 0x43445343) {
            apFile->GetChunkData(&pEntry->iChance, 1);
            AddCreatureSound(pEntry, aeType);
            bLoaded = true;
        }
        else {
            ourFree(pEntry);
        }
    }
    return bLoaded;
}

void CreatureSounds::AddCreatureSound(CREATURE_SOUND* apSound, CREATURE_SOUND_TYPE aeType) {
    if (apSound && aeType >= CREA_SOUND_LEFT && aeType < CREA_SOUND_COUNT) {
        if (!pSoundListArray[aeType]) {
            pSoundListArray[aeType] = new BSSimpleList<CREATURE_SOUND*>;
        }
        BSSimpleList<CREATURE_SOUND*>* pList = GetCreatureSoundList(aeType);
        BSSimpleList<CREATURE_SOUND*>* pLastList = nullptr;
        while (pList && !pList->IsEmpty()) {
            int32_t iChance = pList->m_item->iChance;
            if (iChance > apSound->iChance) {
                break;
            }
            pLastList = pList;
            pList = pList->m_pkNext;
        }
        if (pList) {
            pList->AddHead(apSound);
        }
        else if (pLastList) {
            pLastList->AddTail(apSound);
        }
    }
}

BSSimpleList<CREATURE_SOUND*>* CreatureSounds::GetCreatureSoundList(CREATURE_SOUND_TYPE aeType) {
    BSSimpleList<CREATURE_SOUND*>* pList = nullptr;
    if (aeType >= CREA_SOUND_LEFT && aeType < CREA_SOUND_COUNT) {
        pList = pSoundListArray[aeType];
    }
    return pList;
}

// The sound types are walked as an int counter, and each is passed as its sound type.
void CreatureSounds::Save() {
    for (int32_t i = 0; i < CREA_SOUND_COUNT; ++i) {
        BSSimpleList<CREATURE_SOUND*>* pList = GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
        if (pList && !pList->IsEmpty()) {
            TESForm::AddChunk(CSDT_ID, static_cast<uint32_t>(i));
            while (pList && !pList->IsEmpty()) {
                CREATURE_SOUND* pData = pList->m_item;
                uint32_t uiTempInt = 0;
                if (pData->pSound) {
                    uiTempInt = pData->pSound->iFormID;
                }
                TESForm::AddChunk(CSDI_ID, uiTempInt);
                TESForm::AddChunk(CSDC_ID, pData->iChance);
                pList = pList->m_pkNext;
            }
        }
    }
}

bool CreatureSounds::Compare(CreatureSounds* apCompare) {
    if (!apCompare) {
        return true;
    }
    for (char i = 0; i < 22; ++i) {
        BSSimpleList<CREATURE_SOUND*>* pCompList;
        BSSimpleList<CREATURE_SOUND*>* pList = GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
        for (pCompList = apCompare->GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
             pList && !pList->IsEmpty() && pCompList && !pCompList->IsEmpty();
             pCompList = pCompList->m_pkNext) {
            CREATURE_SOUND* pItem = pList->m_item;
            CREATURE_SOUND* pCompItem = pCompList->m_item;
            if (pItem->iChance != pCompItem->iChance) {
                return true;
            }
            if (pItem->pSound != pCompItem->pSound) {
                return true;
            }
            pList = pList->m_pkNext;
        }
        if (pList || pCompList) {
            return true;
        }
    }
    return false;
}

void CreatureSounds::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Copies the two fields, not the padding after iChance.
void CreatureSounds::CopyCreatureSound(CREATURE_SOUND* apCopyTo, CREATURE_SOUND* apCopyFrom) {
    if (apCopyTo && apCopyFrom) {
        apCopyTo->pSound = apCopyFrom->pSound;
        apCopyTo->iChance = apCopyFrom->iChance;
    }
}
