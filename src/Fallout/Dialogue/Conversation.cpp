#include "Fallout/Dialogue/Conversation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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
    for (;;) {
        bool bEmpty;
        bEmpty = lDialogueItems.IsEmpty();
        if (bEmpty) {
            break;
        }
        DialogueItem* pDialogueItem;
        pDialogueItem = lDialogueItems.GetItem();
        if (pDialogueItem) {
            delete pDialogueItem;
        }
        lDialogueItems.RemoveHead();
    }
    pCurrentItem = nullptr;
}

DialogueItem* Conversation::GetCurrentItem() {
    if (pCurrentItem) {
        DialogueItem* pItem;
        pItem = pCurrentItem->GetItem();
        return pItem;
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
    return (bool)((uint8_t)(pCurrentItem && pCurrentItem->m_item));
}

bool Conversation::FirstItem() {
    pCurrentItem = &lDialogueItems;
    return (bool)((uint8_t)(pCurrentItem && pCurrentItem->m_item));
}

int16_t Conversation::GetDialogueItemIndex(DialogueItem* apDialogueItem) {
    int16_t iindex = 0;
    for (BSSimpleList<DialogueItem*>* plist = &lDialogueItems; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        if (apDialogueItem == plist->m_item) {
            return iindex;
        }
        ++iindex;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue item index, the dialogue item was not found in the dialogue items "
        "list."
    );
    return 0;
}

DialogueItem* Conversation::GetDialogueItemByIndex(int16_t aiIndex) {
    int16_t iindex = 0;
    BSSimpleList<DialogueItem*>* plist = &lDialogueItems;
    while (plist && !plist->IsEmpty()) {
        if (iindex == aiIndex) {
            return plist->m_item;
        }
        ++iindex;
        plist = plist->m_pkNext;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue item by its index, the index was larger than the size of the "
        "dialogue items list."
    );
    return 0;
}

uint16_t Conversation::GetSaveSizeTES() {
    uint16_t usSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block tag and the block size.
        usSize += 4;
        usSize += 2;
    }
    // The item count.
    usSize += 2;
    for (BSSimpleList<DialogueItem*>* plist = &lDialogueItems; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        usSize += plist->m_item->GetSaveSize();
    }
    // The current item's index.
    usSize += 2;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* CurrentlySavingFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (CurrentlySavingFormHeader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlySavingFormHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", usSize,
                CurrentlySavingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                CurrentlySavingFormHeader->iFlags, 192,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", usSize, 192,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
    }
    return usSize;
}

void Conversation::SaveGameTES() {
    int16_t sBlockSize = 0;
    uint8_t* pBlockSize = nullptr;
    uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block tag, 'BLOK'.
        uint32_t uiBlockTag = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockTag, 4u);
        pBlockSize = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, 2u);
    }
    int16_t sCount = 0;
    uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&sCount, 2u);
    for (BSSimpleList<DialogueItem*>* plist = &lDialogueItems; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        plist->m_item->SaveGameTES();
        ++sCount;
    }
    *pCount = sCount;
    int16_t DialogueItemIndex = -1;
    if (pCurrentItem) {
        DialogueItemIndex = GetDialogueItemIndex(pCurrentItem->m_item);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&DialogueItemIndex, 2u);
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* v8 = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* CurrentlySavingFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (CurrentlySavingFormHeader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlySavingFormHeader->iFormID);
            int32_t iFlags = CurrentlySavingFormHeader->iFlags;
            const char* v2 = FormByNumericID->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(v8 - BufferPointer), CurrentlySavingFormHeader->iFormID, v2, iFlags, 225,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(v8 - BufferPointer), 225,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 225
            );
        }
        // The block's size goes into its slot written above: raw bytes of the save buffer, treated as untyped memory.
        *reinterpret_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pEnd - pStart);
    }
}

void Conversation::LoadGameTES() {
    uint32_t uiBlockTag;
    uint16_t usCount;
    int16_t sCurrentItem;
    uint16_t usBlockSize = 0;
    uint8_t* BufferPointer = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockTag, 4u);
        // The block tag, 'BLOK'.
        if (uiBlockTag != 0x424C4F4B) {
            LoadFormHeader* CurrentlyLoadingFormHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (CurrentlyLoadingFormHeader) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlyLoadingFormHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 231,
                    CurrentlyLoadingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                    CurrentlyLoadingFormHeader->cVersion, CurrentlyLoadingFormHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 231,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, 2u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, 2u);
    for (uint32_t iloop = 0; iloop < usCount; ++iloop) {
        DialogueItem* pdialogueItem;
        DialogueItem* apMemory = static_cast<DialogueItem*>(ourMalloc(sizeof(DialogueItem)));
        if (apMemory) {
            pdialogueItem = ::new (apMemory) DialogueItem;
        }
        else {
            pdialogueItem = 0;
        }
        DialogueItem* pdialogueItem_2 = pdialogueItem;
        pdialogueItem_2->LoadGame();
        lDialogueItems.AddTail(pdialogueItem_2);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&sCurrentItem, 2u);
    if (sCurrentItem == -1) {
        pCurrentItem = 0;
    }
    else {
        SetCurrentItem(GetDialogueItemByIndex(sCurrentItem));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* CurrentlyLoadingFormHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (CurrentlyLoadingFormHeader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlyLoadingFormHeader->iFormID);
            if (pEnd > BufferPointer + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pEnd - (BufferPointer + usBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 253,
                    CurrentlyLoadingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                    CurrentlyLoadingFormHeader->cVersion, CurrentlyLoadingFormHeader->iFlags
                );
            }
            else if (pEnd < BufferPointer + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>((BufferPointer + usBlockSize) - pEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 253,
                    CurrentlyLoadingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                    CurrentlyLoadingFormHeader->cVersion, CurrentlyLoadingFormHeader->iFlags
                );
            }
        }
        else {
            if (pEnd > BufferPointer + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(pEnd - (BufferPointer + usBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 253,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
            else if (pEnd < BufferPointer + usBlockSize) {
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>((BufferPointer + usBlockSize) - pEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\Conversation.cpp", 253,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
    }
}

void Conversation::InitLoadGameTES() {
    for (BSSimpleList<DialogueItem*>* plist = &lDialogueItems; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        plist->m_item->InitLoadGame();
    }
}

void Conversation::SaveGameBGS(BGSSaveFormBuffer* a2) {
    int16_t scurrentItem;
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = a2->StartVariableSizedValue();
    for (DialogueItem* pdialogueItem : lDialogueItems) {
        if (pdialogueItem) {
            pdialogueItem->SaveGame(a2);
            ++isaveGameCount;
        }
    }
    a2->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    if (pCurrentItem) {
        scurrentItem = GetDialogueItemIndex(pCurrentItem->m_item);
    }
    else {
        scurrentItem = -1;
    }
    int16_t sSavedItem = scurrentItem;
    a2->SaveDataEndian(&sSavedItem, 2u, 0);
}

void Conversation::LoadGameBGS(BGSLoadFormBuffer* arg0) {
    int16_t scurrentItem;
    uint32_t isaveGameCount = arg0->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        DialogueItem* pdialogueItem;
        DialogueItem* apMemory = static_cast<DialogueItem*>(ourMalloc(sizeof(DialogueItem)));
        if (apMemory) {
            pdialogueItem = ::new (apMemory) DialogueItem;
        }
        else {
            pdialogueItem = 0;
        }
        DialogueItem* pdialogueItem_2 = pdialogueItem;
        pdialogueItem_2->LoadGameBGS(arg0);
        lDialogueItems.AddTail(pdialogueItem_2);
    }
    scurrentItem = -1;
    arg0->LoadData(&scurrentItem, 2u);
    if (scurrentItem != -1) {
        SetCurrentItem(GetDialogueItemByIndex(scurrentItem));
    }
}

void Conversation::InitItem(BGSLoadGameBuffer* apLoadGameBuffer) {
    // Walks the dialogue items' list nodes; the first node is the conversation itself.
    BSSimpleList<DialogueItem*>* pNode = &lDialogueItems;
    while (pNode) {
        DialogueItem* m_item = pNode->m_item;
        if (m_item) {
            m_item->InitItem(apLoadGameBuffer);
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
