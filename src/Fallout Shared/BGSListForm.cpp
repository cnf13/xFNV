#include "Fallout Shared/BGSListForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BGSListForm::BGSListForm() : iScriptAddedFormCount(0) {
    cFormType = 85;
}

BGSListForm::~BGSListForm() {
    BGSListForm::ClearData();
}

// Empties the list and forgets the forms scripts added.
void BGSListForm::ClearData() {
    BSSimpleList<TESForm*>* pList = GetFormList();
    pList->RemoveAll();
    iScriptAddedFormCount = 0;
}

// Reads the editor ID, the object bound and the LNAM entries. The LNAM entries stay raw form IDs in the list until
// InitItem resolves them. The editor-ID buffer is the chunk's size, and at most 0x200 bytes are read into it.
bool BGSListForm::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }

        if (eChunk == EDID_ID) {
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
            apFile->GetChunkData(pEditorID, 0x200);
            SetFormEditorID(pEditorID);
        }
        else if (eChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (eChunk == LNAM_ID) {
            uint32_t uiID = 0;
            apFile->GetChunkData(uiID);
            // Stores the raw form ID in the list's pointer slot; it is not a form until InitItem resolves it.
            TESForm* pForm = reinterpret_cast<TESForm*>(uiID);
            GetFormList()->AddTail(pForm);
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

void BGSListForm::Save() {
    StartForm();
    for (BSSimpleList<TESForm*>* pList = &ListOfForms; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        TESForm::AddChunk(LNAM_ID, pList->m_item->iFormID);
    }
    CloseForm();
}

void BGSListForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FORM_LIST_ADDED_FORM)) {
        BSSimpleList<TESForm*>* pList = &ListOfForms;
        apSaveGameBuffer
            ->SaveDataEndian(reinterpret_cast<char*>(&iScriptAddedFormCount), sizeof(iScriptAddedFormCount), 0);
        for (uint32_t i = 0; i < iScriptAddedFormCount; ++i) {
            TESForm* pForm = pList->m_item;
            pList = pList->m_pkNext;
            apSaveGameBuffer->SaveFormID(pForm, 0u);
        }
    }
}

void BGSListForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FORM_LIST_ADDED_FORM)) {
        if (iScriptAddedFormCount) {
            EmptyList();
        }
        uint32_t uiCount = 0;
        apLoadGameBuffer->LoadData(&uiCount, sizeof(uiCount));
        for (uint32_t i = 0; i < uiCount; ++i) {
            uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            AddForm(pForm);
        }
    }
}

void BGSListForm::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_FORM_LIST_ADDED_FORM)) {
        EmptyList();
    }
}

// Resolves the raw form IDs Load left in the list: the compile index is applied, the form looked up and the entry
// replaced by it, or the entry dropped with a warning when no form has that ID. pPrev is the last node that holds a
// resolved form, so a failed lookup removes the entry after it and resumes from there.
void BGSListForm::InitItem() {
    if (GetInitialized()) {
        return;
    }

    BSSimpleList<TESForm*>* pIter = GetFormList();
    BSSimpleList<TESForm*>* pPrev = nullptr;
    while (pIter && !pIter->IsEmpty()) {
        // The slot holds a raw form ID, read as the list item's 4 bytes (the untyped value it stored).
        uint32_t uiID = *reinterpret_cast<uint32_t*>(&pIter->GetItem());
        TESForm::AddCompileIndex(uiID, GetFile(-1));
        TESForm* pForm = TESForm::GetFormByNumericID(uiID);
        if (pForm) {
            pIter->SetItem(pForm);
            pPrev = pIter;
            pIter = pIter->GetNext();
        }
        else {
            Warning(
                "MASTERFILE: Unable to find form (%08X) on list form %s (%08X).",
                uiID,
                GetFormEditorID(),
                GetFormID()
            );
            if (pPrev) {
                TESForm* pRaw = reinterpret_cast<TESForm*>(uiID);
                pPrev->Remove(pRaw);
                pIter = pPrev->GetNext();
            }
            else {
                pIter->RemoveHead();
            }
        }
    }
    SetInitialized(true);
}

void BGSListForm::Copy(TESForm* apCopy) {
    BGSListForm* pCopy = dynamic_cast<BGSListForm*>(apCopy);
    if (pCopy) {
        BSSimpleList<TESForm*>* pList = &pCopy->ListOfForms;
        while (pList && !pList->IsEmpty()) {
            TESForm* pForm = pList->m_item;
            ListOfForms.AddTail(pForm);
            pList = pList->m_pkNext;
        }
        CopyAllComponents(pCopy);
    }
}

bool BGSListForm::Compare(TESForm* apCompare) {
    BGSListForm* pCompare = dynamic_cast<BGSListForm*>(apCompare);
    if (!pCompare) {
        return true;
    }
    int32_t iCount = ListOfForms.ItemsInList();
    if (iCount != pCompare->ListOfForms.ItemsInList()) {
        return true;
    }
    BSSimpleList<TESForm*>* pList = &ListOfForms;
    for (BSSimpleList<TESForm*>* pCompList = &pCompare->ListOfForms;
         pList && !pList->IsEmpty() && pCompList && !pCompList->IsEmpty();
         pCompList = pCompList->m_pkNext) {
        if (pList->m_item != pCompList->m_item) {
            return true;
        }
        pList = pList->m_pkNext;
    }
    return CompareAllComponents(pCompare);
}

BGSListForm* BGSListForm::EmptyList() {
    while (iScriptAddedFormCount) {
        BSSimpleList<TESForm*>* pList = GetFormList();
        pList->RemoveHead();
        --iScriptAddedFormCount;
    }
    return this;
}

int32_t BGSListForm::GetItemIndex(TESForm* apForm) {
    BSSimpleList<TESForm*>* pList;
    int32_t iIndex = 0;
    for (pList = GetFormList(); pList && !pList->IsEmpty() && pList->m_item != apForm; pList = pList->m_pkNext) {
        ++iIndex;
    }
    if (!pList) {
        return -1;
    }
    return iIndex;
}

void BGSListForm::AddForm(TESForm* apForm) {
    if (apForm && !ListOfForms.IsInList(apForm)) {
        ListOfForms.AddHead(apForm);
        ++iScriptAddedFormCount;
        AddChange(CHANGE_FORM_LIST_ADDED_FORM);
    }
}
