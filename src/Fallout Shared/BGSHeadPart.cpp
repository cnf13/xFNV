#include "Fallout Shared/BGSHeadPart.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BGSHeadPart::BGSHeadPart() : TESForm(), TESFullName(), TESModelTextureSwap(), ExtraParts() {
    SetFormType(9u);
    BGSHeadPart::InitializeData();
}

BGSHeadPart::~BGSHeadPart() {
    BGSHeadPart::ClearData();
}

void BGSHeadPart::InitializeData() {
    cFlags = 0;
}

void BGSHeadPart::ClearData() {
    BSSimpleList<BGSHeadPart*>* pList = GetExtraPartList();
    pList->RemoveAll();
}

// Reads the head part's chunks. MODS holds the texture swaps, which are unpacked with the record's form version; HNAM
// names an extra part.
bool BGSHeadPart::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != HDPT_ID) {
        return false;
    }
    this->LoadForm(apFile);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case DATA_ID: apFile->GetChunkData(&cFlags, 1u); break;
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case HNAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                this->AddExtraPart(reinterpret_cast<BGSHeadPart*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            case MODL_ID:
            case MODD_ID:
            case MOSD_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            case MODS_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint16_t usVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usVersion);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSHeadPart::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESForm::AddChunk(DATA_ID, this->BGSHeadPart::cFlags);
    BSSimpleList<BGSHeadPart*>* pList = &ExtraParts;
    while (pList && !pList->IsEmpty()) {
        BGSHeadPart* pPart = pList->m_item;
        pList = pList->m_pkNext;
        TESForm::AddChunk(HNAM_ID, pPart->iFormID);
    }
    CloseForm();
}

// Before InitItem the extra parts list holds the form IDs the file stored.
void BGSHeadPart::InitItem() {
    if (!GetInitialized()) {
        TESModelTextureSwap::InitItem(this);
        BSSimpleList<BGSHeadPart*>* pList = &ExtraParts;
        BSSimpleList<BGSHeadPart*>* pLastList = nullptr;
        while (pList && !pList->IsEmpty()) {
            uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            BGSHeadPart* pPart = dynamic_cast<BGSHeadPart*>(pForm);
            if (pPart) {
                pList->SetItem(pPart);
                pLastList = pList;
                pList = pList->m_pkNext;
            }
            else {
                int32_t iHeadPartID = iFormID;
                Warning(
                    "Unable to find extra part (%08X) for head part '%s' (%08X).",
                    uiTempInt,
                    GetFormEditorID(),
                    iHeadPartID
                );
                if (pLastList) {
                    BGSHeadPart* pFormID = reinterpret_cast<BGSHeadPart*>(static_cast<uintptr_t>(uiTempInt));
                    pLastList->Remove(pFormID);
                    pList = pLastList->m_pkNext;
                }
                else {
                    pList->RemoveHead();
                }
            }
        }
        SetInitialized(true);
    }
}

void BGSHeadPart::Copy(TESForm* apCopy) {
    BGSHeadPart* pCopy = dynamic_cast<BGSHeadPart*>(apCopy);
    if (pCopy) {
        CopyAllComponents(apCopy);
        BGSHeadPart::cFlags = pCopy->BGSHeadPart::cFlags;
        ExtraParts.RemoveAll();
        BSSimpleList<BGSHeadPart*>* pList = &pCopy->ExtraParts;
        while (pList && !pList->IsEmpty()) {
            BGSHeadPart* pPart = pList->m_item;
            pList = pList->m_pkNext;
            AddExtraPart(pPart);
        }
    }
}

bool BGSHeadPart::Compare(TESForm* apCompare) {
    BGSHeadPart* pComp = dynamic_cast<BGSHeadPart*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (pComp->BGSHeadPart::cFlags != BGSHeadPart::cFlags) {
        return true;
    }
    BSSimpleList<BGSHeadPart*>* pList = &pComp->ExtraParts;
    while (pList && !pList->IsEmpty()) {
        BGSHeadPart* pPart = pList->m_item;
        pList = pList->m_pkNext;
        if (!ExtraParts.IsInList(pPart)) {
            return true;
        }
    }
    return false;
}

BSSimpleList<BGSHeadPart*>* BGSHeadPart::GetExtraPartList() {
    return &ExtraParts;
}

void BGSHeadPart::AddExtraPart(BGSHeadPart* apPart) {
    ExtraParts.AddHead(apPart);
}

bool BGSHeadPart::IsPlayable() {
    return (cFlags & FLAG_PLAYABLE) != 0;
}
