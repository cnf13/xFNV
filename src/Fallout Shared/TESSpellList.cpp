#include "Fallout Shared/TESSpellList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLevSpell.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

TESSpellList::TESSpellList() : BaseFormComponent(), spellList(), leveledSpellList() {}

TESSpellList::~TESSpellList() {
    TESSpellList::ClearDataComponent();
}

void TESSpellList::ClearDataComponent() {
    FreeSpellList();
}

void TESSpellList::CopyComponent(BaseFormComponent* apCopyFC) {
    TESSpellList* pCopy = dynamic_cast<TESSpellList*>(apCopyFC);
    if (pCopy) {
        FreeSpellList();
        BSSimpleList<SpellItem*>* pCopyList = &pCopy->spellList;
        BSSimpleList<SpellItem*>* pList = &spellList;
        while (pCopyList) {
            SpellItem* pSpell = pCopyList->m_item;
            if (pSpell) {
                pList->AddHead(pSpell);
            }
            pCopyList = pCopyList->m_pkNext;
        }
        BSSimpleList<TESLevSpell*>* pCopyLevList = &pCopy->leveledSpellList;
        BSSimpleList<TESLevSpell*>* pLevList = &leveledSpellList;
        while (pCopyLevList) {
            TESLevSpell* pLevSpell = pCopyLevList->m_item;
            if (pLevSpell) {
                pLevList->AddHead(pLevSpell);
            }
            pCopyLevList = pCopyLevList->m_pkNext;
        }
    }
}

bool TESSpellList::CompareComponent(BaseFormComponent* apCompareFC) {
    TESSpellList* pCompare = dynamic_cast<TESSpellList*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    uint32_t uiCount = 0;
    for (SpellItem* pSpell : *&spellList) {
        if (pSpell) {
            if (!pCompare->HasSpell(pSpell)) {
                return true;
            }
            ++uiCount;
        }
    }
    if (pCompare->spellList.ItemsInList() != uiCount) {
        return true;
    }
    uiCount = 0;
    for (TESLevSpell*& rpItem : *&leveledSpellList) {
        if (rpItem) {
            if (!pCompare->HasLeveledSpell(rpItem)) {
                return true;
            }
            ++uiCount;
        }
    }
    return pCompare->leveledSpellList.ItemsInList() != uiCount;
}

uint16_t TESSpellList::GetSaveSize(uint32_t aiFlags) {
    uint16_t usSize = 0;
    if ((aiFlags & CHANGE_ACTOR_BASE_SPELLLIST) != 0) {
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            usSize = 6;
        }
        int16_t sListSize = usSize + 2 + 4 * spellList.ItemsInList();
        usSize = sListSize + 4 * leveledSpellList.ItemsInList();
        if (bDebugSaveBuffer.Bool()) {
            SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                InitializationFlow(
                    "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                    usSize,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->iFlags,
                    1307,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
            else {
                InitializationFlow(
                    "GetSaveSize(): %-5i ending at line %i in file %s",
                    usSize,
                    1307,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
        }
    }
    return usSize;
}

// Saves the spell lists to the old-format save.
void TESSpellList::SaveGame(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_ACTOR_BASE_SPELLLIST) != 0) {
        uint16_t usBlockSize = 0;
        void* pBlockSize = nullptr;
        uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        if (bDebugSaveBuffer.Bool()) {
            pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            // 'BLOK'.
            uint32_t uiBlockHeader = 0x424C4F4B;
            pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
            pBlockSize = pSaveLoadGameOLD->m_pBuffer;
            pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
        }
        uint16_t usCount = 0;
        uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
        for (SpellItem* pSpell : *&spellList) {
            if (pSpell) {
                uint32_t uiSpellID = pSpell->iFormID;
                pSaveLoadGameOLD->SaveNumericID(&uiSpellID, 4u);
                ++usCount;
            }
        }
        for (TESLevSpell* pLevSpell : *&leveledSpellList) {
            if (pLevSpell) {
                uint32_t uiSpellID = pLevSpell->iFormID;
                pSaveLoadGameOLD->SaveNumericID(&uiSpellID, 4u);
                ++usCount;
            }
        }
        *pCount = usCount;
        if (bDebugSaveBuffer.Bool()) {
            uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                const char* pEditorID = pForm->GetFormEditorID();
                InitializationFlow(
                    "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                    static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                    pHeader->iFormID,
                    pEditorID,
                    uiFlags,
                    1359,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
            else {
                InitializationFlow(
                    "SaveGame(): %-5i ending at line %i in file %s",
                    static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                    1359,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            uint8_t* pCurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            uint8_t* pBlockStart = static_cast<uint8_t*>(pBlockSize);
            if (pCurrent > pBlockStart + 0xFFFF) {
                Warning(
                    "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                    1359
                );
            }
            *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pCurrent - pBlockStart);
        }
    }
}

// Loads the spell lists from the old-format save.
void TESSpellList::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if ((aiFlags & CHANGE_ACTOR_BASE_SPELLLIST) != 0) {
        uint16_t usCount;
        uint16_t usBlockSize = 0;
        uint8_t* pStartBufferPosition = nullptr;
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            uint32_t uiBlockHeader;
            pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
            // 'BLOK'.
            if (uiBlockHeader != 0x424C4F4B) {
                LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
                if (pHeader) {
                    TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                    Warning(
                        "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                        "Currently loading form is %08X %s with version %i and flags %08X",
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                        1371,
                        pHeader->iFormID,
                        pForm->GetFormEditorID(),
                        pHeader->cVersion,
                        pHeader->iFlags
                    );
                }
                else {
                    Warning(
                        "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                        "Current version is %i",
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                        1371,
                        pSaveLoadGameOLD->GetCurrentVersion()
                    );
                }
            }
            pStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
        }
        spellList.RemoveAll();
        leveledSpellList.RemoveAll();
        pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
        for (int32_t i = 0; i < usCount; ++i) {
            uint32_t uiSpellID;
            pSaveLoadGameOLD->LoadNumericID(&uiSpellID, 4u);
            TESForm* pForm = TESForm::GetFormByNumericID(uiSpellID);
            SpellItem* pSpell = dynamic_cast<SpellItem*>(pForm);
            pForm = TESForm::GetFormByNumericID(uiSpellID);
            TESLevSpell* pLeveledSpell = dynamic_cast<TESLevSpell*>(pForm);
            if (pSpell) {
                spellList.AddHead(pSpell);
            }
            else if (pLeveledSpell) {
                bool bAdd = true;
                if (pSaveLoadGameOLD->GetCurrentVersion() < 71u) {
                    bAdd = !leveledSpellList.IsInList(pLeveledSpell);
                }
                if (bAdd) {
                    leveledSpellList.AddHead(pLeveledSpell);
                }
            }
            else {
                Warning("MASTERFILE: Could not find spell %08X", uiSpellID);
            }
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            uint8_t* pCurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            uint8_t* pBlockEnd = &pStartBufferPosition[usBlockSize];
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                if (pCurrentBufferPosition <= pBlockEnd) {
                    if (pCurrentBufferPosition < pBlockEnd) {
                        uint32_t uiFlags = pHeader->iFlags;
                        int32_t iVersion = pHeader->cVersion;
                        const char* pEditorID = pForm->GetFormEditorID();
                        Warning(
                            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently "
                            "loading form is %08X %s with version %i and flags %08X",
                            static_cast<int32_t>(pBlockEnd - pCurrentBufferPosition),
                            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                            1407,
                            pHeader->iFormID,
                            pEditorID,
                            iVersion,
                            uiFlags
                        );
                    }
                }
                else {
                    uint32_t uiFlags = pHeader->iFlags;
                    int32_t iVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(pCurrentBufferPosition - pBlockEnd),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                        1407,
                        pHeader->iFormID,
                        pEditorID,
                        iVersion,
                        uiFlags
                    );
                }
            }
            else if (pCurrentBufferPosition <= pBlockEnd) {
                if (pCurrentBufferPosition < pBlockEnd) {
                    int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current "
                        "version is %i",
                        static_cast<int32_t>(pBlockEnd - pCurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                        1407,
                        iVersion
                    );
                }
            }
            else {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(pCurrentBufferPosition - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp",
                    1407,
                    iVersion
                );
            }
        }
    }
}

bool TESSpellList::AddSpell(SpellItem* apSpell) {
    if (!apSpell) {
        return false;
    }
    if (spellList.IsInList(apSpell)) {
        return false;
    }
    spellList.AddHead(apSpell);
    TESActorBase* pActorBase = pPlayer->GetActorBase();
    TESSpellList* pPlayerSpellList = static_cast<TESSpellList*>(pActorBase);
    if (this == pPlayerSpellList && !pDataHandler->GetSaveLoad()) {
        pPlayer->SpellLearned(apSpell);
    }
    return true;
}

bool TESSpellList::AddSpell(TESForm* apSpellIn) {
    if (pDataHandler->GetSaveLoad()) {
        AddSpell(static_cast<SpellItem*>(apSpellIn));
        return true;
    }

    SpellItem* pSpell = dynamic_cast<SpellItem*>(apSpellIn);
    TESLevSpell* pLeveledSpell = dynamic_cast<TESLevSpell*>(apSpellIn);
    if (pSpell) {
        return AddSpell(pSpell);
    }

    return pLeveledSpell && AddSpell(pLeveledSpell);
}

bool TESSpellList::AddSpell(TESLevSpell* apSpellIn) {
    if (!apSpellIn) {
        return false;
    }
    TESActorBase* pActorBase = pPlayer->GetActorBase();
    TESSpellList* pPlayerSpellList = static_cast<TESSpellList*>(pActorBase);
    if (this == pPlayerSpellList) {
        return false;
    }
    if (leveledSpellList.IsInList(apSpellIn)) {
        return false;
    }
    leveledSpellList.AddHead(apSpellIn);
    return true;
}

void TESSpellList::FreeSpellList() {
    for (BSSimpleList<SpellItem*>* pList = &spellList; pList && pList->m_item; pList->RemoveHead()) {
        ;
    }
    for (BSSimpleList<TESLevSpell*>* pLevList = &leveledSpellList; pLevList && pLevList->m_item;
         pLevList->RemoveHead()) {
        ;
    }
}

void TESSpellList::Save() {
    for (SpellItem* pSpell : spellList.ItemsUntilNull()) {
        if (!pSpell->GetDelete()) {
            TESForm::AddChunk(SPLO_ID, pSpell->iFormID);
        }
    }
    for (TESLevSpell* pLevSpell : leveledSpellList.ItemsUntilNull()) {
        if (!pLevSpell->GetDelete()) {
            TESForm::AddChunk(SPLO_ID, pLevSpell->iFormID);
        }
    }
}

bool TESSpellList::HasSpell(SpellItem* apSpell) {
    for (BSSimpleList<SpellItem*>* pNode = GetSpellList(); pNode;) {
        SpellItem* pSpell = pNode->GetItem();
        if (pSpell == apSpell) {
            return true;
        }
        pNode = pNode->GetNext();
    }
    return false;
}

void TESSpellList::RemoveSpell(SpellItem* apSpell) {
    GetSpellList()->Remove(apSpell);
}

// Loads both lists when the save changed them: every saved form ID is resolved and added to its list
// when the form is of the list's type.
void TESSpellList::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (!apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_SPELLLIST)) {
        return;
    }
    spellList.RemoveAll();
    leveledSpellList.RemoveAll();
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        SpellItem* pSpell = dynamic_cast<SpellItem*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        if (pSpell) {
            spellList.AddHead(pSpell);
        }
    }
    uint32_t uiLevCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t j = 0; j < uiLevCount; ++j) {
        TESLevSpell* pLevSpell = dynamic_cast<TESLevSpell*>(
            TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID())
        );
        if (pLevSpell) {
            leveledSpellList.AddHead(pLevSpell);
        }
    }
}

void TESSpellList::InitItem(TESForm* apOwner) {
    BSSimpleList<SpellItem*>* pLastList = nullptr;
    BSSimpleList<SpellItem*>* pList = &spellList;
    TESFile* pOwnerFile = apOwner ? apOwner->GetFile(-1) : nullptr;
    while (pList && pList->m_item) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
        TESForm::AddCompileIndex(tempint, pOwnerFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        if (pForm) {
            SpellItem* pSpell = dynamic_cast<SpellItem*>(pForm);
            TESLevSpell* pLeveledSpell = dynamic_cast<TESLevSpell*>(pForm);
            if (pSpell) {
                pList->SetItem(pSpell);
                TESActorBase* pActorBase = dynamic_cast<TESActorBase*>(this);
                if (pActorBase) {
                    TESActorBase* pPlayerBase = pPlayer->GetActorBase();
                    if (pActorBase == pPlayerBase) {
                        pPlayer->SpellLearned(pSpell);
                    }
                }
                pLastList = pList;
                pList = pList->m_pkNext;
            }
            else {
                if (!pList->m_pkNext && pLastList) {
                    // The unresolved entry still holds its form ID.
                    SpellItem* pRemoved = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(tempint));
                    pLastList->Remove(pRemoved);
                    pList = pLastList->m_pkNext;
                }
                else {
                    pList->RemoveHead();
                }
                if (pLeveledSpell) {
                    AddSpell(pLeveledSpell);
                }
            }
        }
        else {
            if (!pList->m_pkNext && pLastList) {
                // The unresolved entry still holds its form ID.
                SpellItem* pRemoved = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(tempint));
                pLastList->Remove(pRemoved);
                pList = pLastList->m_pkNext;
            }
            else {
                pList->RemoveHead();
            }
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Actor Effect (%08X) for owner object \"%s\".",
                        tempint,
                        pEditorID
                    );
                }
                else {
                    int32_t iOwnerFormID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Actor Effect (%08X) for owner object (%08X).",
                        tempint,
                        iOwnerFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find Actor Effect (%08X) for unknown owner.", tempint);
            }
        }
    }
}

BSSimpleList<SpellItem*>* TESSpellList::GetSpellList() {
    return &spellList;
}

void TESSpellList::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_SPELLLIST)) {
        uint32_t uiSpellCount = 0;
        uint32_t uiSpellPosition = apSaveGameBuffer->StartVariableSizedValue();
        for (SpellItem* pSpell : spellList) {
            if (pSpell) {
                apSaveGameBuffer->SaveFormID(pSpell, 0);
                ++uiSpellCount;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(uiSpellCount, uiSpellPosition);
        uint32_t uiLevSpellCount = 0;
        uint32_t uiLevSpellPosition = apSaveGameBuffer->StartVariableSizedValue();
        for (TESLevSpell* pLevSpell : leveledSpellList) {
            if (pLevSpell) {
                apSaveGameBuffer->SaveFormID(pLevSpell, 0);
                ++uiLevSpellCount;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(uiLevSpellCount, uiLevSpellPosition);
    }
}

bool TESSpellList::HasLeveledSpell(TESLevSpell* apSpell) {
    for (TESLevSpell*& rpItem : *&leveledSpellList) {
        if (rpItem == apSpell) {
            return true;
        }
    }
    return false;
}
