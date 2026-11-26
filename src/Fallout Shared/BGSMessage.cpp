#include "Fallout Shared/BGSMessage.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "xfnv/Compiler.h"

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
    iFlags = FLAG_MESSAGE_BOX;
    iDisplayTime = 2;
}

void BGSMessage::ClearData() {
    SetIcon(nullptr);
    ClearButtonList();
}

// Reads the message's chunks. ITXT adds a button, whose conditions the next CTDA chunk fills; INAM holds the icon's
// form ID. A chunk shorter than four bytes leaves the INAM value unwritten (malformed data), so it is initialised to 0.
bool BGSMessage::Load(TESFile* apFile) {
    // Reads the record's form type and never checks it.
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                MESSAGEBOX_BUTTON* pButton = new MESSAGEBOX_BUTTON;
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
    StartForm();
    TESDescription::Save();
    TESFullName::Save();
    uint32_t uiID = pIcon ? pIcon->iFormID : 0;
    TESForm::AddChunk(INAM_ID, uiID);
    TESForm::AddChunk(DNAM_ID, iFlags);
    if (!GetIsMessageBox()) {
        TESForm::AddChunk(TNAM_ID, iDisplayTime);
    }
    BSSimpleList<MESSAGEBOX_BUTTON*>* pList = &ButtonList;
    while (pList && !pList->IsEmpty()) {
        MESSAGEBOX_BUTTON* pItem = pList->m_item;
        pList = pList->m_pkNext;
        if (pItem->Text.pString) {
            uint32_t uiSize = BSstrlen(pItem->Text.pString) + 1;
            TESForm::AddChunkArray(ITXT_ID, pItem->Text.pString, uiSize);
        }
        else {
            TESForm::AddChunk(ITXT_ID, 0u);
        }
        pItem->Conditions.Save();
    }
    CloseForm();
}

void BGSMessage::InitItem() {
    if (!GetInitialized()) {
        // pIcon holds the icon's form ID until InitItem resolves it.
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIcon));
        if (uiTempInt) {
            TESForm::AddCompileIndex(uiTempInt, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            SetIcon(dynamic_cast<BGSMenuIcon*>(pForm));
            if (!pIcon) {
                int32_t iID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning("MASTERFILE: Could not find icon (%08X) on weapon '%s' (%08X).", uiTempInt, pEditorID, iID);
            }
        }
        BSSimpleList<MESSAGEBOX_BUTTON*>* pList = &ButtonList;
        int32_t iIndex = 0;
        while (pList && !pList->IsEmpty()) {
            MESSAGEBOX_BUTTON* pItem = pList->m_item;
            pList = pList->m_pkNext;
            pItem->Conditions.InitItem(this);
            int32_t iPrevWarnings = MessageHandler::QWarningCount();
            MessageHandler::IncDisableWarningCount(true);
            MessageHandler::SetWarningCount(0);
            MessageHandler::IncDisableWarningCount(false);
            int32_t iWarnings = MessageHandler::QWarningCount();
            MessageHandler::SetWarningCount(iPrevWarnings);
            if (iWarnings) {
                int32_t iID = iFormID;
                Warning(
                    "MASTERFILE: Errors were encountered during InitItem for conditions on menu item %d for "
                    "terminal '%s' (%08X)\n\nSee Warnings file for more information.",
                    iIndex,
                    GetFormEditorID(),
                    iID
                );
            }
            ++iIndex;
        }
        SetInitialized(true);
    }
}

void BGSMessage::Copy(TESForm* apCopy) {
    // apCopy is used as a BGSMessage without a type check.
    BGSMessage* pCopy = static_cast<BGSMessage*>(apCopy);
    ClearData();
    SetIcon(pCopy->pIcon);
    SetIsMessageBox(pCopy->BGSMessage::GetIsMessageBox());
    SetAutoDisplay(pCopy->BGSMessage::IsMsgFlag2Set());
    iDisplayTime = pCopy->iDisplayTime;
    BSSimpleList<MESSAGEBOX_BUTTON*>* pList = &pCopy->ButtonList;
    while (pList && !pList->IsEmpty()) {
        MESSAGEBOX_BUTTON* pCopyItem = pList->m_item;
        pList = pList->m_pkNext;
        MESSAGEBOX_BUTTON* pItem = new MESSAGEBOX_BUTTON;
        pItem->Text.Copy(pCopyItem->Text);
        pItem->Conditions.Copy(&pCopyItem->Conditions, nullptr);
        AddButton(pItem);
    }
    CopyAllComponents(apCopy);
}

bool BGSMessage::Compare(TESForm* apCompare) {
    if (!apCompare) {
        return true;
    }
    // apCopy is used as a BGSMessage without a type check.
    BGSMessage* pCompare = static_cast<BGSMessage*>(apCompare);
    if (pIcon != pCompare->pIcon) {
        return true;
    }
    int32_t iIsMessageBox = GetIsMessageBox();
    if (iIsMessageBox != pCompare->BGSMessage::GetIsMessageBox()) {
        return true;
    }
    int32_t iIsMsgFlag2Set = this->IsMsgFlag2Set();
    if (iIsMsgFlag2Set != pCompare->BGSMessage::IsMsgFlag2Set()) {
        return true;
    }
    if (iDisplayTime != pCompare->iDisplayTime) {
        return true;
    }
    BSSimpleList<MESSAGEBOX_BUTTON*>* pList = &ButtonList;
    BSSimpleList<MESSAGEBOX_BUTTON*>* pCompList = &pCompare->ButtonList;
    int32_t iCount = pList->ItemsInList();
    if (iCount != pCompList->ItemsInList()) {
        return true;
    }
    while (pCompList && !pCompList->IsEmpty() && pList && !pList->IsEmpty()) {
        MESSAGEBOX_BUTTON* pCompItem = pCompList->m_item;
        MESSAGEBOX_BUTTON* pItem = pList->m_item;
        if (static_cast<uint8_t>(pItem->Text.IsNotEqual(pCompItem->Text))) {
            return true;
        }
        if (pItem->Conditions.Compare(&pCompItem->Conditions)) {
            return true;
        }
        pCompList = pCompList->m_pkNext;
        pList = pList->m_pkNext;
    }
    return CompareAllComponents(apCompare);
}

void BGSMessage::ClearButtonList() {
    while (!ButtonList.IsEmpty()) {
        MESSAGEBOX_BUTTON* pButton = ButtonList.m_item;
        ButtonList.RemoveHead();
        if (pButton) {
            delete pButton;
        }
    }
}

bool BGSMessage::GetIsMessageBox() {
    return (iFlags & FLAG_MESSAGE_BOX) != 0;
}

bool BGSMessage::IsMsgFlag2Set() {
    return (iFlags & FLAG_AUTO_DISPLAY) != 0;
}

void BGSMessage::AddButton(MESSAGEBOX_BUTTON* apItem) {
    if (apItem) {
        ButtonList.AddTail(apItem);
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
        iFlags |= FLAG_MESSAGE_BOX;
    }
    else {
        iFlags &= ~FLAG_MESSAGE_BOX;
    }
    return this;
}

BGSMessage* BGSMessage::SetAutoDisplay(uint8_t abVal) {
    if (abVal) {
        iFlags |= FLAG_AUTO_DISPLAY;
    }
    else {
        iFlags &= ~FLAG_AUTO_DISPLAY;
    }
    return this;
}
