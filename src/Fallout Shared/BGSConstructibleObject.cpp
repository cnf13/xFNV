#include "Fallout Shared/BGSConstructibleObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSValueNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BGSConstructibleObject::BGSConstructibleObject() : TESObjectMISC() {
    SetFormType(50u);
    BGSConstructibleObject::InitializeData();
}

BGSConstructibleObject::~BGSConstructibleObject() {
    TESForm::InitializeDataComponent();
}

void BGSConstructibleObject::InitializeData() {
    pRequiredItems = nullptr;
    pCreatedItem = nullptr;
}

// Reads the misc item's components plus the required items (RNAM) and the created item (CNAM).
bool BGSConstructibleObject::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk == FULL_ID) {
            TESFullName::LoadFullNameChunk(this, apFile);
        }
        else if (uiChunk == MODL_ID || uiChunk == MODD_ID || uiChunk == MODT_ID || uiChunk == MOSD_ID) {
            TESModel::LoadModelChunk(this, apFile);
        }
        else if (uiChunk == MODS_ID) {
            // A 16-byte aligned buffer of the chunk's size.
            uint32_t uiDataSize = apFile->GetChunkSize();
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiDataSize));
            apFile->GetChunkData(pBuffer, uiDataSize);
            UnpackTextureSwapChunkData(pBuffer, uiDataSize, apFile->GetFormVersion());
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == DATA_ID) {
            LoadData(apFile, nullptr, 0);
        }
        else if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (uiChunk == DEST_ID || uiChunk == DSTD_ID) {
            BGSDestructibleObjectForm::LoadChunk(this, apFile);
        }
        else if (uiChunk == SCRI_ID) {
            // Stores the form ID straight into the script pointer, then resolves it.
            uint32_t uiScriptFormID = 0;
            apFile->GetChunkData(uiScriptFormID);
            SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptFormID)));
            TESScriptableForm::InitItem(this);
        }
        else if (uiChunk == MICO_ID) {
            BGSMessageIcon::LoadMessageIcon(this, apFile);
        }
        else if (uiChunk == ICON_ID) {
            TESTexture::LoadTextureChunk(this, apFile);
        }
        else if (uiChunk == RNAM_ID) {
            uint32_t uiRequiredItemsID = 0;
            apFile->GetChunkData(uiRequiredItemsID);
            pRequiredItems = reinterpret_cast<BGSListForm*>(static_cast<uintptr_t>(uiRequiredItemsID));
        }
        else if (uiChunk == CNAM_ID) {
            uint32_t uiCreatedItemID = 0;
            apFile->GetChunkData(uiCreatedItemID);
            pCreatedItem = reinterpret_cast<TESBoundObject*>(static_cast<uintptr_t>(uiCreatedItemID));
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSConstructibleObject::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(MICO_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    TESForm::SaveData();
    if (pRequiredItems) {
        TESForm::AddChunk(RNAM_ID, pRequiredItems->TESForm::GetFormID());
    }
    if (pCreatedItem) {
        TESForm::AddChunk(CNAM_ID, pCreatedItem->TESForm::GetFormID());
    }
    TESForm::CloseForm();
}

void BGSConstructibleObject::InitItem() {
    if (!GetInitialized()) {
        TESObjectMISC::InitItem();
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRequiredItems));
        if (itempint) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pRequiredItems = dynamic_cast<BGSListForm*>(pForm);
            if (!pRequiredItems) {
                int32_t iObjectID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find item list (%08X) on constructible object '%s' (%08X).",
                    itempint,
                    pEditorID,
                    iObjectID
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCreatedItem));
        if (itempint) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pCreatedItem = dynamic_cast<TESBoundObject*>(pForm);
            if (!pCreatedItem) {
                int32_t iObjectID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find created item (%08X) on constructible object '%s' (%08X).",
                    itempint,
                    pEditorID,
                    iObjectID
                );
            }
        }
        SetInitialized(true);
    }
}

void BGSConstructibleObject::Copy(TESForm* apCopy) {
    BGSConstructibleObject* pCopy = dynamic_cast<BGSConstructibleObject*>(apCopy);
    if (pCopy) {
        TESObjectMISC::Copy(pCopy);
        pRequiredItems = pCopy->pRequiredItems;
        pCreatedItem = pCopy->pCreatedItem;
    }
}

bool BGSConstructibleObject::Compare(TESForm* apCompare) {
    BGSConstructibleObject* pCompare = dynamic_cast<BGSConstructibleObject*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (TESObjectMISC::Compare(pCompare)) {
        return true;
    }
    if (pRequiredItems == pCompare->pRequiredItems) {
        return pCreatedItem != pCompare->pCreatedItem;
    }
    return true;
}
