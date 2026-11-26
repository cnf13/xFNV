#include "Fallout/Dialogue/Conversation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

Conversation::Conversation() : pCurrentItem(nullptr) {}

Conversation::Conversation(TESObjectREFR* apSpeaker, TESObjectREFR* apListener, TESTopic* apTopic)
  : pCurrentItem(nullptr) {
    TESTopic::CreateConversation(apSpeaker, apListener, &lDialogueItems, apTopic);
}

Conversation::~Conversation() {
    ClearList();
}

void Conversation::ClearList() {
    while (!lDialogueItems.IsEmpty()) {
        DialogueItem* pDialogueItem = lDialogueItems.GetItem();
        if (pDialogueItem) {
            delete pDialogueItem;
        }
        lDialogueItems.RemoveHead();
    }
    pCurrentItem = nullptr;
}

DialogueItem* Conversation::GetCurrentItem() {
    if (pCurrentItem) {
        return pCurrentItem->GetItem();
    }
    return nullptr;
}

void Conversation::SetCurrentItem(DialogueItem* apDialogueItem) {
    for (BSSimpleList<DialogueItem*>* pList = &lDialogueItems; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (apDialogueItem == pList->m_item) {
            pCurrentItem = pList;
            return;
        }
    }
}

bool Conversation::GetNext() {
    if (pCurrentItem) {
        pCurrentItem = pCurrentItem->m_pkNext;
    }
    return pCurrentItem && pCurrentItem->m_item;
}

bool Conversation::FirstItem() {
    pCurrentItem = &lDialogueItems;
    return pCurrentItem && pCurrentItem->m_item;
}

int16_t Conversation::GetDialogueItemIndex(DialogueItem* apDialogueItem) {
    int16_t sIndex = 0;
    for (BSSimpleList<DialogueItem*>* pList = &lDialogueItems; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (apDialogueItem == pList->m_item) {
            return sIndex;
        }
        ++sIndex;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue item index, the dialogue item was not found in the dialogue items "
        "list."
    );
    return 0;
}

DialogueItem* Conversation::GetDialogueItemByIndex(int16_t aiIndex) {
    int16_t sIndex = 0;
    BSSimpleList<DialogueItem*>* pList = &lDialogueItems;
    while (pList && !pList->IsEmpty()) {
        if (sIndex == aiIndex) {
            return pList->m_item;
        }
        ++sIndex;
        pList = pList->m_pkNext;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue item by its index, the index was larger than the size of the "
        "dialogue items list."
    );
    return nullptr;
}

uint16_t Conversation::GetSaveSize() {
    uint16_t usSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block tag and the block size.
        usSize += 4;
        usSize += 2;
    }
    // The item count.
    usSize += 2;
    for (BSSimpleList<DialogueItem*>* pList = &lDialogueItems; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        usSize += pList->m_item->GetSaveSize();
    }
    // The current item's index.
    usSize += 2;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pFormHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pFormHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize,
                pFormHeader->iFormID,
                pForm->GetFormEditorID(),
                pFormHeader->iFlags,
                192,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize,
                192,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
    }
    return usSize;
}

void Conversation::SaveGame() {
    int16_t sBlockSize = 0;
    uint8_t* pBlockSize = nullptr;
    uint8_t* pBufferStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pBufferStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block tag, 'BLOK'.
        uint32_t uiBlockTag = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockTag, sizeof(uiBlockTag));
        pBlockSize = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    int16_t sCount = 0;
    uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&sCount, sizeof(sCount));
    for (BSSimpleList<DialogueItem*>* pList = &lDialogueItems; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        pList->m_item->SaveGame();
        ++sCount;
    }
    *pCount = sCount;
    int16_t sCurrentItem = -1;
    if (pCurrentItem) {
        sCurrentItem = GetDialogueItemIndex(pCurrentItem->m_item);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&sCurrentItem, sizeof(sCurrentItem));
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pFormHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pFormHeader->iFormID);
            int32_t iFlags = pFormHeader->iFlags;
            const char* pcEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pBufferStart),
                pFormHeader->iFormID,
                pcEditorID,
                iFlags,
                225,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pBufferStart),
                225,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pStart = pBlockSize;
        if (pEnd > pStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                225
            );
        }
        // The block's size goes into its slot written above: raw bytes of the save buffer, treated as untyped memory.
        *reinterpret_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pEnd - pStart);
    }
}

void Conversation::LoadGame() {
    uint32_t uiBlockTag;
    uint16_t usCount;
    int16_t sCurrentItem;
    uint16_t usBlockSize = 0;
    uint8_t* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockTag, sizeof(uiBlockTag));
        // The block tag, 'BLOK'.
        if (uiBlockTag != 0x424C4F4B) {
            LoadFormHeader* pFormHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pFormHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pFormHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                    231,
                    pFormHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pFormHeader->cVersion,
                    pFormHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                    231,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
    for (uint32_t uiLoop = 0; uiLoop < usCount; ++uiLoop) {
        DialogueItem* pDialogueItem;
        pDialogueItem = new DialogueItem;
        pDialogueItem->LoadGame();
        lDialogueItems.AddTail(pDialogueItem);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&sCurrentItem, sizeof(sCurrentItem));
    if (sCurrentItem == -1) {
        pCurrentItem = nullptr;
    }
    else {
        SetCurrentItem(GetDialogueItemByIndex(sCurrentItem));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pFormHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pFormHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pFormHeader->iFormID);
            if (pEnd > pBlockStart + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pEnd - (pBlockStart + usBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                    253,
                    pFormHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pFormHeader->cVersion,
                    pFormHeader->iFlags
                );
            }
            else if (pEnd < pBlockStart + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockStart + usBlockSize - pEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                    253,
                    pFormHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pFormHeader->cVersion,
                    pFormHeader->iFlags
                );
            }
        }
        else if (pEnd > pBlockStart + usBlockSize) {
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pEnd - (pBlockStart + usBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                253,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
        else if (pEnd < pBlockStart + usBlockSize) {
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockStart + usBlockSize - pEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp",
                253,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
    }
}

void Conversation::InitLoadGame() {
    for (BSSimpleList<DialogueItem*>* pList = &lDialogueItems; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        pList->m_item->InitLoadGame();
    }
}

void Conversation::SaveGame(BGSSaveFormBuffer* apBuffer) {
    int16_t sCurrentItem;
    uint32_t uiSaveGameCount = 0;
    uint32_t uiSaveGameBufferPos = apBuffer->StartVariableSizedValue();
    for (DialogueItem* pDialogueItem : lDialogueItems) {
        if (pDialogueItem) {
            pDialogueItem->SaveGame(apBuffer);
            ++uiSaveGameCount;
        }
    }
    apBuffer->SaveVariableSizedValue(uiSaveGameCount, uiSaveGameBufferPos);
    if (pCurrentItem) {
        sCurrentItem = GetDialogueItemIndex(pCurrentItem->m_item);
    }
    else {
        sCurrentItem = -1;
    }
    apBuffer->SaveDataEndian(&sCurrentItem, sizeof(sCurrentItem), 0);
}

void Conversation::LoadGame(BGSLoadFormBuffer* apBuffer) {
    uint32_t uiSaveGameCount = apBuffer->LoadVariableSizedValue();
    for (uint32_t uiLoop = 0; uiLoop < uiSaveGameCount; ++uiLoop) {
        DialogueItem* pDialogueItem;
        pDialogueItem = new DialogueItem;
        pDialogueItem->LoadGame(apBuffer);
        lDialogueItems.AddTail(pDialogueItem);
    }
    int16_t sCurrentItem = -1;
    apBuffer->LoadData(&sCurrentItem, sizeof(sCurrentItem));
    if (sCurrentItem != -1) {
        SetCurrentItem(GetDialogueItemByIndex(sCurrentItem));
    }
}

void Conversation::InitItem(BGSLoadGameBuffer* apLoadGameBuffer) {
    // Walks the dialogue items' list nodes; the first node is the conversation itself.
    BSSimpleList<DialogueItem*>* pNode = &lDialogueItems;
    while (pNode) {
        DialogueItem* pItem = pNode->m_item;
        if (pItem) {
            pItem->InitItem(apLoadGameBuffer);
        }
        pNode = pNode->m_pkNext;
    }
}

// Precaches each dialogue item's voice files. Walks the dialogue items' list nodes; the first node is the conversation
// itself.
void Conversation::PrecacheSounds() {
    for (BSSimpleList<DialogueItem*>* pNode = &lDialogueItems; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        if (pNode && pNode->m_item) {
            pNode->m_item->PrecacheSounds();
        }
    }
}

void Conversation::operator delete(void* apMemory) {
    ourFree(apMemory);
}
