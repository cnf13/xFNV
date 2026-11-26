#include "Fallout Shared/TESReactionForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

void GROUP_REACTION::Endian() {
    BSEndian(pForm, false);
    BSEndian(iReaction, false);
    BSEndian(iFightReaction, false);
}

TESReactionForm::TESReactionForm() {
    TESReactionForm::InitializeDataComponent();
}

TESReactionForm::~TESReactionForm() {
    TESReactionForm::ClearDataComponent();
}

void TESReactionForm::InitializeDataComponent() {
    m_cGroupFormType = FACT_ID;
}

void TESReactionForm::ClearDataComponent() {
    FreeReactionList();
}

void TESReactionForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESReactionForm* pCopy = dynamic_cast<TESReactionForm*>(apCopyFC);
    if (!pCopy) {
        return;
    }
    FreeReactionList();
    BSSimpleList<GROUP_REACTION*>* pReactionList = QReactionList();
    BSSimpleList<GROUP_REACTION*>* pCopyList = pCopy->QReactionList();
    while (pCopyList && pCopyList->m_item) {
        GROUP_REACTION* pCopyReaction = pCopyList->m_item;
        GROUP_REACTION* pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
        pReaction->pForm = pCopyReaction->pForm;
        pReaction->iReaction = pCopyReaction->iReaction;
        pReaction->iFightReaction = pCopyReaction->iFightReaction;
        pReactionList->AddTail(pReaction);
        if (pReactionList->m_pkNext) {
            pReactionList = pReactionList->m_pkNext;
        }
        pCopyList = pCopyList->m_pkNext;
    }
}

bool TESReactionForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESReactionForm* pCompare = dynamic_cast<TESReactionForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    BSSimpleList<GROUP_REACTION*>* pList = &m_reactionList;
    BSSimpleList<GROUP_REACTION*>* pCompList = &pCompare->m_reactionList;
    while (pList) {
        if (!pCompList) {
            return true;
        }
        GROUP_REACTION* reaction = pList->m_item;
        GROUP_REACTION* compReaction = pCompList->m_item;
        if (reaction && compReaction) {
            if (reaction->pForm != compReaction->pForm) {
                return true;
            }
            if (reaction->iReaction != compReaction->iReaction) {
                return true;
            }
            if (reaction->iFightReaction != compReaction->iFightReaction) {
                return true;
            }
        }
        else {
            if (reaction || compReaction) {
                return true;
            }
        }
        pCompList = pCompList->m_pkNext;
        pList = pList->m_pkNext;
    }
    return pCompList && !pCompList->IsEmpty();
}

// The bytes the reactions take in a save: a two-byte count, six bytes of save-game block header when the save writes
// them and eight bytes per reaction, logged against the header of the form being saved when bDebugSaveBuffer is set.
uint16_t TESReactionForm::GetSaveSize(uint32_t aiFlags) {
    uint16_t usSize = 0;
    if ((aiFlags & CHANGE_FACTION_REACTIONS) != 0) {
        uint16_t usStart = usSize;
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            usSize += 4;
            usSize += 2;
        }
        BSSimpleList<GROUP_REACTION*>* pReactions = QReactionList();
        usSize += 2;
        usSize = static_cast<uint16_t>(usSize + 8 * pReactions->ItemsInList());
        if (bDebugSaveBuffer.Bool()) {
            SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                InitializationFlow(
                    "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                    usSize - usStart,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->iFlags,
                    1367,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp"
                );
            }
            else {
                InitializationFlow(
                    "GetSaveSize(): %-5i ending at line %i in file %s",
                    usSize - usStart,
                    1367,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp"
                );
            }
        }
    }
    return usSize;
}

void TESReactionForm::SetReaction(TESForm* apForm, int32_t aiReaction) {
    if (apForm) {
        bool foundit = false;
        for (GROUP_REACTION* fr : m_reactionList.ItemsUntilNull()) {
            if (fr->pForm == apForm) {
                if (aiReaction || fr->iFightReaction) {
                    fr->iReaction = aiReaction;
                }
                else {
                    m_reactionList.Remove(fr);
                    ourFree(fr);
                }
                foundit = true;
                break;
            }
        }
        if (!foundit && aiReaction) {
            GROUP_REACTION* fr = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
            fr->pForm = apForm;
            fr->iReaction = aiReaction;
            m_reactionList.AddTail(fr);
        }
    }
}

void TESReactionForm::InitItem(TESForm* apOwner) {
    TESFile* pownerFile;
    BSSimpleList<GROUP_REACTION*>* pList = &m_reactionList;
    BSSimpleList<GROUP_REACTION*>* pLastList = nullptr;
    if (apOwner) {
        pownerFile = apOwner->GetFile(-1);
    }
    else {
        pownerFile = nullptr;
    }
    while (pList && pList->m_item) {
        GROUP_REACTION* reaction = pList->m_item;
        uint32_t tempID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(reaction->pForm));
        TESForm::AddCompileIndex(tempID, pownerFile);
        reaction->pForm = TESForm::GetFormByNumericID(tempID);
        if (reaction->pForm) {
            pList = pList->m_pkNext;
        }
        else {
            const char* pcEditorID = apOwner->GetFormEditorID();
            Warning(
                "MASTERFILE: Form (%08X) not found for reaction of pForm (%08X) '%s'.\r\n",
                tempID,
                apOwner->iFormID,
                pcEditorID
            );
            if (!pList->m_pkNext && pLastList) {
                pLastList->Remove(reaction);
                pList = pLastList->m_pkNext;
            }
            else {
                pList->RemoveHead();
            }
            ourFree(reaction);
        }
    }
}

int32_t TESReactionForm::GetReaction(TESForm* apForm) {
    if (apForm) {
        for (BSSimpleList<GROUP_REACTION*>* pList = TESReactionForm::QReactionList(); pList && pList->m_item;
             pList = pList->m_pkNext) {
            if (pList->m_item->pForm == apForm) {
                return pList->m_item->iReaction;
            }
        }
    }
    return 0;
}

void TESReactionForm::SetFightReaction(TESForm* apForm, int32_t aiReaction) {
    if (apForm) {
        char cFound = 0;
        for (GROUP_REACTION* pReaction : QReactionList()->ItemsUntilNull()) {
            if (pReaction->pForm == apForm) {
                pReaction->iFightReaction = aiReaction;
                cFound = 1;
                break;
            }
        }
        if (!cFound && aiReaction) {
            GROUP_REACTION* pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
            pReaction->pForm = apForm;
            pReaction->iReaction = 0;
            pReaction->iFightReaction = aiReaction;
            QReactionList()->AddTail(pReaction);
        }
    }
}

// Adds aiReaction to the reaction to apForm: frees the entry when the sum is 0, adds a new one (its iFightReaction
// deliberately left as ourMalloc returned it) when apForm has none.
void TESReactionForm::ModReaction(TESForm* apForm, int32_t aiReaction) {
    if (!apForm || !aiReaction) {
        return;
    }
    GROUP_REACTION* pReaction = nullptr;
    BSSimpleList<GROUP_REACTION*>* pList = QReactionList();
    bool bFound = false;
    while (pList && pList->m_item) {
        pReaction = pList->m_item;
        if (pReaction->pForm == apForm) {
            bFound = true;
            break;
        }
        pList = pList->m_pkNext;
    }
    if (!bFound) {
        pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
        pReaction->pForm = apForm;
        pReaction->iReaction = aiReaction;
        QReactionList()->AddTail(pReaction);
    }
    else {
        pReaction->iReaction += aiReaction;
        if (!pReaction->iReaction) {
            QReactionList()->Remove(pReaction);
            ourFree(pReaction);
        }
    }
}

void TESReactionForm::FreeReactionList() {
    while (m_reactionList.m_item) {
        ourFree(m_reactionList.m_item);
        m_reactionList.RemoveHead();
    }
}

void TESReactionForm::Save() {
    GROUP_REACTION fileReact;
    for (GROUP_REACTION* fr : m_reactionList.ItemsUntilNull()) {
        if (!fr->pForm->GetDelete()) {
            // The file record holds the other form's ID in place of the pointer.
            fileReact.pForm = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(fr->pForm->iFormID));
            fileReact.iReaction = fr->iReaction;
            fileReact.iFightReaction = fr->iFightReaction;
            if (TESForm::QEndianSwapOnSave()) {
                fileReact.Endian();
            }
            // The file's record is 0xC bytes, the form's 4-byte ID first.
            TESFile::NarrowFormIDRecords(&fileReact, 1, 0xC, sizeof(fileReact), {{0, offsetof(GROUP_REACTION, pForm)}});
            TESForm::__AddChunkData(XNAM_ID, &fileReact, 12u);
            if (TESForm::QEndianSwapOnSave()) {
                fileReact.Endian();
            }
        }
    }
}

void TESReactionForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_REACTIONS)) {
        uint32_t isaveGameCount = 0;
        uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
        for (GROUP_REACTION* preaction : m_reactionList) {
            if (preaction) {
                apSaveGameBuffer->SaveFormID(preaction->pForm, 0u);
                apSaveGameBuffer
                    ->SaveDataEndian(reinterpret_cast<char*>(&preaction->iReaction), sizeof(preaction->iReaction), 0);
                apSaveGameBuffer->SaveDataEndian(
                    reinterpret_cast<char*>(&preaction->iFightReaction),
                    sizeof(preaction->iFightReaction),
                    0
                );
                ++isaveGameCount;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    }
}

void TESReactionForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_REACTIONS)) {
        FreeReactionList();
        uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
            GROUP_REACTION* preaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
            apLoadGameBuffer->LoadFormID(preaction->pForm);
            apLoadGameBuffer->LoadData(&preaction->iReaction, sizeof(preaction->iReaction));
            apLoadGameBuffer->LoadData(&preaction->iFightReaction, sizeof(preaction->iFightReaction));
            m_reactionList.AddHead(preaction);
        }
    }
}

int32_t TESReactionForm::GetFactionCombatRelation(TESForm* apForm) {
    if (apForm) {
        for (BSSimpleList<GROUP_REACTION*>* pList = QReactionList(); pList && pList->m_item; pList = pList->m_pkNext) {
            if (pList->m_item->pForm == apForm) {
                return pList->m_item->iFightReaction;
            }
        }
    }
    return 0;
}

// Writes the reactions to the old-format save: an optional save-game block header, a two-byte count patched in after
// the reactions, then each reaction's form ID and iReaction.
void TESReactionForm::SaveGame(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_FACTION_REACTIONS) == 0) {
        return;
    }
    uint16_t usBlockSize = 0;
    char* pBlockStart = nullptr;
    char* pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockTag = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockTag, sizeof(uiBlockTag));
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    uint16_t usCount = 0;
    uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
    for (BSSimpleList<GROUP_REACTION*>* pList = QReactionList(); pList; pList = pList->m_pkNext) {
        GROUP_REACTION* pReaction = pList->m_item;
        if (pReaction) {
            uint32_t uiFormID = pReaction->pForm->iFormID;
            pSaveLoadGameOLD->SaveNumericID(&uiFormID, 4u);
            pSaveLoadGameOLD->SaveGameDataOLD(&pReaction->iReaction, sizeof(pReaction->iReaction));
            ++usCount;
        }
    }
    *pCount = usCount;
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                1403,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                1403,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        if (pEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                1403
            );
        }
        // The save buffer is untyped bytes: the block's two-byte size slot sits right after its tag.
        *reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockStart);
    }
}

void TESReactionForm::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    if ((aiFlags & CHANGE_FACTION_REACTIONS) != 0) {
        uint16_t usBlockSize = 0;
        uint8_t* pBlockStart = nullptr;
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            int32_t iBlockTag;
            pSaveLoadGameOLD->LoadGameDataOLD(&iBlockTag, sizeof(iBlockTag));
            if (iBlockTag != 0x424C4F4B) {
                LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
                if (pHeader) {
                    TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                    Warning(
                        "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                        "Currently loading form is %08X %s with version %i and flags %08X",
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                        1415,
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
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                        1415,
                        pSaveLoadGameOLD->GetCurrentVersion()
                    );
                }
            }
            pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
        }
        FreeReactionList();
        uint16_t usCount;
        pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
        for (uint32_t i = 0; i < usCount; ++i) {
            uint32_t uiFormID;
            int32_t iReaction;
            pSaveLoadGameOLD->LoadNumericID(&uiFormID, 4u);
            pSaveLoadGameOLD->LoadGameDataOLD(&iReaction, sizeof(iReaction));
            // Stores the form ID cast to TESForm*; InitLoadGame resolves it to the form.
            SetReaction(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID)), iReaction);
        }
        if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
            uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                if (pEnd <= &pBlockStart[usBlockSize]) {
                    if (pEnd < &pBlockStart[usBlockSize]) {
                        int32_t iFlags = pHeader->iFlags;
                        int32_t cVersion = pHeader->cVersion;
                        const char* pEditorID = pForm->GetFormEditorID();
                        Warning(
                            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  "
                            "Currently loading form is %08X %s with version %i and flags %08X",
                            static_cast<int32_t>(&pBlockStart[usBlockSize] - pEnd),
                            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                            1433,
                            pHeader->iFormID,
                            pEditorID,
                            cVersion,
                            iFlags
                        );
                    }
                }
                else {
                    int32_t iFlags = pHeader->iFlags;
                    int32_t cVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  "
                        "Currently loading form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(pEnd - &pBlockStart[usBlockSize]),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                        1433,
                        pHeader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else if (pEnd <= &pBlockStart[usBlockSize]) {
                if (pEnd < &pBlockStart[usBlockSize]) {
                    int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  "
                        "Current version is %i",
                        static_cast<int32_t>(&pBlockStart[usBlockSize] - pEnd),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                        1433,
                        iCurrentVersion
                    );
                }
            }
            else {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  "
                    "Current version is %i",
                    static_cast<int32_t>(pEnd - &pBlockStart[usBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESReactionForm.cpp",
                    1433,
                    iCurrentVersion
                );
            }
        }
    }
}

// Resolves each reaction's form ID (LoadGame keeps it in pForm) and drops the reactions whose form is gone.
void TESReactionForm::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_REACTIONS)) {
        BSSimpleList<GROUP_REACTION*>* pList = &m_reactionList;
        BSSimpleList<GROUP_REACTION*>* pLastList = nullptr;
        while (pList && !pList->IsEmpty()) {
            GROUP_REACTION* pReaction = pList->m_item;
            bool bRemove = false;
            if (pReaction) {
                TESForm* pForm;
                if (pReaction->pForm) {
                    // pReaction->pForm holds a form ID rather than a TESForm* until it is resolved here.
                    pForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pReaction->pForm))
                    );
                }
                else {
                    pForm = nullptr;
                }
                pReaction->pForm = pForm;
                if (!pReaction->pForm) {
                    ourFree(pReaction);
                    bRemove = true;
                }
            }
            else {
                bRemove = true;
            }
            if (bRemove) {
                if (pLastList) {
                    pLastList->Remove(pList->m_item);
                    pList = pLastList->m_pkNext;
                }
                else {
                    pList->RemoveHead();
                }
            }
            else {
                pLastList = pList;
                pList = pList->m_pkNext;
            }
        }
    }
}

// With CHANGE_FACTION_REACTIONS in aiFlags, each reaction's other form (its form ID until now) is looked up; a reaction
// whose form is gone is removed from the head of the node it sits in and freed, and the same node is tested again.
void TESReactionForm::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    if ((aiFlags & CHANGE_FACTION_REACTIONS) != 0) {
        BSSimpleList<GROUP_REACTION*>* pIter = &m_reactionList;
        while (pIter) {
            GROUP_REACTION* preaction = pIter->m_item;
            if (preaction) {
                // The field holds the other form's ID until here.
                uint32_t iformID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(preaction->pForm));
                preaction->pForm = TESForm::GetFormByNumericID(iformID);
                if (!preaction->pForm) {
                    pIter->RemoveHead();
                    ourFree(preaction);
                }
                else {
                    pIter = pIter->m_pkNext;
                }
            }
            else {
                pIter = pIter->m_pkNext;
            }
        }
    }
}
