#include "Fallout Shared/BGSMessage.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"

// The game settings this file defines, in initialization order.
SettingT<GameSettingCollection> iMessageBoxMaxItems("iMessageBoxMaxItems", 10);

MESSAGEBOX_BUTTON::MESSAGEBOX_BUTTON() {}

MESSAGEBOX_BUTTON::~MESSAGEBOX_BUTTON() {}

BGSMessage::BGSMessage() {
    SetFormType(98u);
    BGSMessage::InitializeData();
}

BGSMessage::~BGSMessage() {
    BGSMessage::ClearData();
}

void BGSMessage::InitializeData() {
    pIcon = nullptr;
    iFlags = (uint32_t)1;
    iDisplayTime = (uint32_t)2;
}

void BGSMessage::ClearData() {
    SetIcon(nullptr);
    ClearButtonList();
}

// Reads the message's chunks. ITXT adds a button, whose conditions the next CTDA chunk fills; INAM holds the icon's
// form ID. A chunk shorter than four bytes leaves the INAM value unwritten (malformed data), so it is initialised to 0.
bool BGSMessage::Load(TESFile* apFile) {
    // Reads the record's form type and never checks it, as the game does.
    apFile->GetTESForm();
    this->LoadForm(apFile);
    this->SetInitialized(false);
    MESSAGEBOX_BUTTON* pLastButton = nullptr;
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case CIDA_ID:
                if (pLastButton) {
                    pLastButton->Conditions.Load(apFile);
                }
                break;
            case DESC_ID: TESDescription::LoadDescriptionData(this, apFile); break;
            case DNAM_ID: apFile->GetChunkData<uint32_t>(iFlags); break;
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case INAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetIcon(reinterpret_cast<BGSMenuIcon*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            case ITXT_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0);
                void* pMemory = ourMalloc(sizeof(MESSAGEBOX_BUTTON));
                MESSAGEBOX_BUTTON* pButton = pMemory ? ::new (pMemory) MESSAGEBOX_BUTTON : nullptr;
                pButton->Text.Set(pBuffer, 0);
                this->AddButton(pButton);
                pLastButton = pButton;
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case TNAM_ID: apFile->GetChunkData<uint32_t>(iDisplayTime); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSMessage::Save() {
    uint32_t iid;
    StartForm();
    TESDescription::Save();
    TESFullName::Save();
    if (pIcon) {
        iid = pIcon->iFormID;
    }
    else {
        iid = 0;
    }
    TESForm::AddChunk(INAM_ID, iid);
    TESForm::AddChunk(DNAM_ID, iFlags);
    if (!GetIsMessageBox()) {
        TESForm::AddChunk(TNAM_ID, iDisplayTime);
    }
    BSSimpleList<MESSAGEBOX_BUTTON*>* plist = &ButtonList;
    while (plist && !plist->IsEmpty()) {
        MESSAGEBOX_BUTTON* pitem = plist->m_item;
        plist = plist->m_pkNext;
        if (pitem->Text.pString) {
            char* v1 = pitem->Text.pString;
            uint32_t v3 = BSstrlen(v1) + 1;
            char* v2 = pitem->Text.pString;
            TESForm::AddChunkArray(ITXT_ID, v2, v3);
        }
        else {
            TESForm::AddChunk(ITXT_ID, 0u);
        }
        pitem->Conditions.Save();
    }
    CloseForm();
}

void BGSMessage::InitItem() {
    if (!GetInitialized()) {
        // pIcon holds the icon's form ID until InitItem resolves it.
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIcon));
        if (itempInt) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
            SetIcon(dynamic_cast<BGSMenuIcon*>(FormByNumericID));
            if (!pIcon) {
                int32_t CastingType = iFormID;
                const char* v3 = GetFormEditorID();
                Warning("MASTERFILE: Could not find icon (%08X) on weapon '%s' (%08X).", itempInt, v3, CastingType);
            }
        }
        BSSimpleList<MESSAGEBOX_BUTTON*>* plist = &ButtonList;
        int32_t iindex = 0;
        while (plist && !plist->IsEmpty()) {
            MESSAGEBOX_BUTTON* pitem = plist->m_item;
            plist = plist->m_pkNext;
            pitem->Conditions.InitItem(this);
            int32_t iprevwarnings = MessageHandler::QWarningCount();
            MessageHandler::IncDisableWarningCount(1);
            MessageHandler::SetWarningCount(0);
            MessageHandler::IncDisableWarningCount(0);
            int32_t iwarnings = MessageHandler::QWarningCount();
            MessageHandler::SetWarningCount(iprevwarnings);
            if (iwarnings) {
                int32_t v5 = iFormID;
                Warning(
                    "MASTERFILE: Errors were encountered during InitItem for conditions on menu item %d for "
                    "terminal '%s' (%08X)\n\nSee Warnings file for more information.",
                    iindex, GetFormEditorID(), v5
                );
            }
            ++iindex;
        }
        SetInitialized(1);
    }
}

void BGSMessage::Copy(TESForm* apCopy) {
    // apCopy is used as a BGSMessage without a type check, as the game does.
    BGSMessage* pCopy = static_cast<BGSMessage*>(apCopy);
    ClearData();
    SetIcon(pCopy->pIcon);
    SetIsMessageBox(pCopy->BGSMessage::GetIsMessageBox());
    SetAutoDisplay(pCopy->BGSMessage::IsMsgFlag2Set());
    iDisplayTime = pCopy->iDisplayTime;
    BSSimpleList<MESSAGEBOX_BUTTON*>* plist = &pCopy->ButtonList;
    while (plist && !plist->IsEmpty()) {
        MESSAGEBOX_BUTTON* pitem;
        MESSAGEBOX_BUTTON* pcopyitem = plist->m_item;
        plist = plist->m_pkNext;
        MESSAGEBOX_BUTTON* apMemory = static_cast<MESSAGEBOX_BUTTON*>(ourMalloc(sizeof(MESSAGEBOX_BUTTON)));
        if (apMemory) {
            pitem = ::new (apMemory) MESSAGEBOX_BUTTON;
        }
        else {
            pitem = 0;
        }
        pitem->Text.Copy(pcopyitem->Text);
        pitem->Conditions.Copy(&pcopyitem->Conditions, 0);
        AddButton(pitem);
    }
    CopyAllComponents(apCopy);
}

bool BGSMessage::Compare(TESForm* apCompare) {
    if (!apCompare) {
        return 1;
    }
    // apCopy is used as a BGSMessage without a type check, as the game does.
    BGSMessage* pCompare = static_cast<BGSMessage*>(apCompare);
    if (pIcon != pCompare->pIcon) {
        return 1;
    }
    int32_t IsMessageBox = GetIsMessageBox();
    if (IsMessageBox != pCompare->BGSMessage::GetIsMessageBox()) {
        return 1;
    }
    int32_t IsMsgFlag2Set = this->IsMsgFlag2Set();
    if (IsMsgFlag2Set != pCompare->BGSMessage::IsMsgFlag2Set()) {
        return 1;
    }
    if (iDisplayTime != pCompare->iDisplayTime) {
        return 1;
    }
    BSSimpleList<MESSAGEBOX_BUTTON*>* plist = &ButtonList;
    BSSimpleList<MESSAGEBOX_BUTTON*>* pcomplist = &pCompare->ButtonList;
    int32_t v7 = plist->ItemsInList();
    if (v7 != pcomplist->ItemsInList()) {
        return 1;
    }
    while (pcomplist && (!pcomplist->IsEmpty() && (plist && !plist->IsEmpty()))) {
        MESSAGEBOX_BUTTON* pcompitem = pcomplist->m_item;
        MESSAGEBOX_BUTTON* pitem = plist->m_item;
        if (static_cast<uint8_t>(pitem->Text.IsNotEqual(pcompitem->Text))) {
            return 1;
        }
        if (pitem->Conditions.Compare(&pcompitem->Conditions)) {
            return 1;
        }
        pcomplist = pcomplist->m_pkNext;
        plist = plist->m_pkNext;
    }
    return CompareAllComponents(apCompare);
}

void BGSMessage::ClearButtonList() {
    while (!ButtonList.IsEmpty()) {
        MESSAGEBOX_BUTTON* v2 = ButtonList.m_item;
        ButtonList.RemoveHead();
        if (v2) {
            delete v2;
        }
    }
}

bool BGSMessage::GetIsMessageBox() {
    return ((((int32_t)iFlags) & 1) != 0);
}

bool BGSMessage::IsMsgFlag2Set() {
    return ((((int32_t)iFlags) & 2) != 0);
}

void BGSMessage::AddButton(MESSAGEBOX_BUTTON* apItem) {
    if (apItem) {
        (&ButtonList)->AddTail(apItem);
    }
}

void BGSMessage::SetIcon(BGSMenuIcon* apIcon) {
    pIcon = apIcon;
}

BGSMenuIcon* BGSMessage::GetIcon() {
    return pIcon;
}

uint32_t BGSMessage::GetDisplayTime() {
    return iDisplayTime;
}

BSSimpleList<MESSAGEBOX_BUTTON*>* BGSMessage::GetButtonList() {
    return &ButtonList;
}

BGSMessage* BGSMessage::SetIsMessageBox(uint8_t abVal) {
    if (abVal) {
        iFlags |= 1;
    }
    else {
        iFlags &= ~1u;
    }
    return this;
}

BGSMessage* BGSMessage::SetAutoDisplay(uint8_t abVal) {
    if (abVal) {
        iFlags |= 2;
    }
    else {
        iFlags &= ~2u;
    }
    return this;
}
