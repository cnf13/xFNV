#include "Fallout Shared/TESSpellList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
            SpellItem* pspell = pCopyList->m_item;
            if (pspell) {
                pList->AddHead(pspell);
            }
            pCopyList = pCopyList->m_pkNext;
        }
        BSSimpleList<TESLevSpell*>* pCopyList_2 = &pCopy->leveledSpellList;
        BSSimpleList<TESLevSpell*>* pList_2 = &leveledSpellList;
        while (pCopyList_2) {
            TESLevSpell* pspell_2 = pCopyList_2->m_item;
            if (pspell_2) {
                pList_2->AddHead(pspell_2);
            }
            pCopyList_2 = pCopyList_2->m_pkNext;
        }
    }
}

bool TESSpellList::CompareComponent(BaseFormComponent* apCompareFC) {
    TESSpellList* pCompare = dynamic_cast<TESSpellList*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    uint32_t iCount = 0;
    for (SpellItem* pspell : *&spellList) {
        if (pspell) {
            if (!pCompare->HasSpell(pspell)) {
                return true;
            }
            ++iCount;
        }
    }
    if ((&pCompare->spellList)->ItemsInList() != iCount) {
        return true;
    }
    uint32_t iCount_2 = 0;
    for (TESLevSpell*& rpItem : *&leveledSpellList) {
        if (rpItem) {
            if (!pCompare->HasLeveledSpell(rpItem)) {
                return true;
            }
            ++iCount_2;
        }
    }
    return (&pCompare->leveledSpellList)->ItemsInList() != iCount_2;
}

uint16_t TESSpellList::GetSaveSize(uint32_t aiFlags) {
    uint16_t isize = 0;
    if ((aiFlags & 16) != 0) {
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            isize = 6;
        }
        int16_t v7 = isize + 2 + 4 * spellList.ItemsInList();
        isize = v7 + 4 * leveledSpellList.ItemsInList();
        if (bDebugSaveBuffer.Bool()) {
            SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
            if (pheader) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
                InitializationFlow(
                    "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                    pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 1307,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
            else {
                InitializationFlow(
                    "GetSaveSize(): %-5i ending at line %i in file %s", isize, 1307,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
        }
    }
    return isize;
}

// Saves the spell lists to the old-format save.
void TESSpellList::SaveGame(uint32_t aiFlags) {
    uint32_t ispellID_2;
    uint32_t ispellID;
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    if ((aiFlags & 16) != 0) {
        iblockSize = 0;
        void* pblockSize = nullptr;
        uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        if (bDebugSaveBuffer.Bool()) {
            itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            // 'BLOK'.
            iblockHeader = 0x424C4F4B;
            pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
            pblockSize = pSaveLoadGameOLD->m_pBuffer;
            pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
        }
        icount = 0;
        uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
        for (SpellItem* pspell : *&spellList) {
            if (pspell) {
                ispellID = pspell->iFormID;
                pSaveLoadGameOLD->SaveNumericID(&ispellID, 4u);
                ++icount;
            }
        }
        for (TESLevSpell* pspell_2 : *&leveledSpellList) {
            if (pspell_2) {
                ispellID_2 = pspell_2->iFormID;
                pSaveLoadGameOLD->SaveNumericID(&ispellID_2, 4u);
                ++icount;
            }
        }
        *pcount = icount;
        if (bDebugSaveBuffer.Bool()) {
            uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                uint32_t iFlags = pheader->iFlags;
                const char* v2 = pform->GetFormEditorID();
                InitializationFlow(
                    "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                    static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v2, iFlags, 1359,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
            else {
                InitializationFlow(
                    "SaveGame(): %-5i ending at line %i in file %s",
                    static_cast<int32_t>(BufferPointer - itempStartPosition), 1359,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp"
                );
            }
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
            if (pcurrent > pblockStart + 0xFFFF) {
                Warning(
                    "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1359
                );
            }
            *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
        }
    }
}

// Loads the spell lists from the old-format save.
void TESSpellList::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint32_t ispellID;
    uint32_t iblockHeader;
    if ((aiFlags & 16) != 0) {
        uint16_t icount;
        uint16_t iblockSize = 0;
        uint8_t* istartBufferPosition = nullptr;
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
            // 'BLOK'.
            if (iblockHeader != 0x424C4F4B) {
                LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
                if (pheader) {
                    TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                    Warning(
                        "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                        "Currently loading form is %08X %s with version %i and flags %08X",
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1371,
                        pheader->iFormID, pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                    );
                }
                else {
                    Warning(
                        "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                        "Current version is %i",
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1371,
                        pSaveLoadGameOLD->GetCurrentVersion()
                    );
                }
            }
            istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
        }
        spellList.RemoveAll();
        leveledSpellList.RemoveAll();
        pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
        for (int32_t i = 0; i < icount; ++i) {
            pSaveLoadGameOLD->LoadNumericID(&ispellID, 4u);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(ispellID);
            SpellItem* pspell = dynamic_cast<SpellItem*>(FormByNumericID);
            TESForm* v4 = TESForm::GetFormByNumericID(ispellID);
            TESLevSpell* pleveledSpell = dynamic_cast<TESLevSpell*>(v4);
            if (pspell) {
                spellList.AddHead(pspell);
            }
            else {
                if (pleveledSpell) {
                    bool badd = true;
                    if (pSaveLoadGameOLD->GetCurrentVersion() < 71u) {
                        badd = !leveledSpellList.IsInList(pleveledSpell);
                    }
                    if (badd) {
                        leveledSpellList.AddHead(pleveledSpell);
                    }
                }
                else {
                    Warning("MASTERFILE: Could not find spell %08X", ispellID);
                }
            }
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            uint8_t* pblockEnd = &istartBufferPosition[iblockSize];
            LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader_2) {
                TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
                if (icurrentBufferPosition <= pblockEnd) {
                    if (icurrentBufferPosition < pblockEnd) {
                        uint32_t iFlags = pheader_2->iFlags;
                        int32_t cVersion = pheader_2->cVersion;
                        const char* v6 = pform_2->GetFormEditorID();
                        Warning(
                            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently "
                            "loading form is %08X %s with version %i and flags %08X",
                            static_cast<int32_t>(pblockEnd - icurrentBufferPosition),
                            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1407,
                            pheader_2->iFormID, v6, cVersion, iFlags
                        );
                    }
                }
                else {
                    uint32_t v9 = pheader_2->iFlags;
                    int32_t v7 = pheader_2->cVersion;
                    const char* v5 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(icurrentBufferPosition - pblockEnd),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1407,
                        pheader_2->iFormID, v5, v7, v9
                    );
                }
            }
            else {
                if (icurrentBufferPosition <= pblockEnd) {
                    if (icurrentBufferPosition < pblockEnd) {
                        int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                        Warning(
                            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current "
                            "version is %i",
                            static_cast<int32_t>(pblockEnd - icurrentBufferPosition),
                            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1407,
                            CurrentVersion
                        );
                    }
                }
                else {
                    int32_t v11 = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(icurrentBufferPosition - pblockEnd),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSpellList.cpp", 1407, v11
                    );
                }
            }
        }
    }
}

bool TESSpellList::AddSpell(SpellItem* apSpell) {
    TESSpellList* v3;
    if (!apSpell) {
        return false;
    }
    if ((&spellList)->IsInList(apSpell)) {
        return false;
    }
    (&spellList)->AddHead(apSpell);
    TESActorBase* ActorBase = pPlayer->GetActorBase();
    if (ActorBase) {
        v3 = static_cast<TESSpellList*>(ActorBase);
    }
    else {
        v3 = nullptr;
    }
    if (this == v3 && !pDataHandler->GetSaveLoad()) {
        pPlayer->SpellLearned(apSpell);
    }
    return true;
}

bool TESSpellList::AddSpell(TESForm* apSpellIn) {
    if (pDataHandler->GetSaveLoad()) {
        AddSpell(static_cast<SpellItem*>(apSpellIn));
        return true;
    }
    else {
        SpellItem* pspell = dynamic_cast<SpellItem*>(apSpellIn);
        TESLevSpell* pleveledSpell = dynamic_cast<TESLevSpell*>(apSpellIn);
        if (pspell) {
            return AddSpell(pspell);
        }
        else {
            return pleveledSpell && AddSpell(pleveledSpell);
        }
    }
}

bool TESSpellList::AddSpell(TESLevSpell* apSpellIn) {
    TESSpellList* v3;
    if (!apSpellIn) {
        return false;
    }
    TESActorBase* ActorBase = pPlayer->GetActorBase();
    if (ActorBase) {
        v3 = static_cast<TESSpellList*>(ActorBase);
    }
    else {
        v3 = nullptr;
    }
    if (this == v3) {
        return false;
    }
    if ((&leveledSpellList)->IsInList(apSpellIn)) {
        return false;
    }
    (&leveledSpellList)->AddHead(apSpellIn);
    return true;
}

void TESSpellList::FreeSpellList() {
    for (BSSimpleList<SpellItem*>* pList = &spellList; pList && pList->m_item; pList->RemoveHead()) {
        ;
    }
    for (BSSimpleList<TESLevSpell*>* pList_2 = &leveledSpellList; pList_2 && pList_2->m_item; pList_2->RemoveHead()) {
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
        SpellItem* pSpell;
        pSpell = pNode->GetItem();
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

// Loads both lists when the save changed them (change flag 0x10): every saved form ID is resolved and added to its list
// when the form is of the list's type.
void TESSpellList::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (!apLoadGameBuffer->GetChangeFlags().CheckFlags(0x10)) {
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
        TESLevSpell* pLevSpell =
            dynamic_cast<TESLevSpell*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
        if (pLevSpell) {
            leveledSpellList.AddHead(pLevSpell);
        }
    }
}

void TESSpellList::InitItem(TESForm* apOwner) {
    TESFile* pownerFile;
    BSSimpleList<SpellItem*>* pLastList = nullptr;
    BSSimpleList<SpellItem*>* pList = &spellList;
    if (apOwner) {
        pownerFile = apOwner->GetFile(-1);
    }
    else {
        pownerFile = nullptr;
    }
    while (pList && pList->m_item) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
        TESForm::AddCompileIndex(tempint, pownerFile);
        TESForm* pform = TESForm::GetFormByNumericID(tempint);
        if (pform) {
            SpellItem* pSpell = dynamic_cast<SpellItem*>(pform);
            TESLevSpell* pleveledSpell = dynamic_cast<TESLevSpell*>(pform);
            if (pSpell) {
                pList->SetItem(pSpell);
                TESActorBase* pactorBase = dynamic_cast<TESActorBase*>(this);
                if (pactorBase) {
                    TESActorBase* ActorBase = pPlayer->GetActorBase();
                    if (pactorBase == ActorBase) {
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
                if (pleveledSpell) {
                    AddSpell(pleveledSpell);
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
                    const char* v3 = apOwner->GetFormEditorID();
                    Warning("MASTERFILE: Unable to find Actor Effect (%08X) for owner object \"%s\".", tempint, v3);
                }
                else {
                    int32_t CastingType = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Actor Effect (%08X) for owner object (%08X).", tempint, CastingType
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

void TESSpellList::SaveGameBGS(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(16)) {
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
