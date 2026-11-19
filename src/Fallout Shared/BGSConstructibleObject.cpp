#include "Fallout Shared/BGSConstructibleObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSPortalSharedNode.h"
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
            // Stores the form ID straight into the script pointer, then resolves it, as the game does.
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
    uint32_t v2;
    uint32_t v3;
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(static_cast<CHUNK_ID>(1329809741));
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    TESForm::SaveData();
    if (pRequiredItems) {
        v3 = pRequiredItems->TESForm::GetFormID();
        TESForm::AddChunk(RNAM_ID, v3);
    }
    if (pCreatedItem) {
        v2 = pCreatedItem->TESForm::GetFormID();
        TESForm::AddChunk(CNAM_ID, v2);
    }
    TESForm::CloseForm();
}

void BGSConstructibleObject::InitItem() {
    if (!GetInitialized()) {
        TESObjectMISC::InitItem();
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRequiredItems));
        if (itempint) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pRequiredItems = dynamic_cast<BGSListForm*>(FormByNumericID);
            if (!pRequiredItems) {
                int32_t CastingType = iFormID;
                const char* v5 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find item list (%08X) on constructible object '%s' (%08X).", itempint, v5,
                    CastingType
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCreatedItem));
        if (itempint) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempint);
            pCreatedItem = dynamic_cast<TESBoundObject*>(v4);
            if (!pCreatedItem) {
                int32_t v8 = iFormID;
                const char* v6 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find created item (%08X) on constructible object '%s' (%08X).", itempint, v6,
                    v8
                );
            }
        }
        SetInitialized(true);
    }
}

void BGSConstructibleObject::Copy(TESForm* apCopy) {
    BGSConstructibleObject* pcopy = dynamic_cast<BGSConstructibleObject*>(apCopy);
    if (pcopy) {
        TESObjectMISC::Copy(pcopy);
        pRequiredItems = pcopy->pRequiredItems;
        pCreatedItem = pcopy->pCreatedItem;
    }
}

bool BGSConstructibleObject::Compare(TESForm* apCompare) {
    BGSConstructibleObject* v4 = dynamic_cast<BGSConstructibleObject*>(apCompare);
    if (!v4) {
        return true;
    }
    if (TESObjectMISC::Compare(v4)) {
        return true;
    }
    if (pRequiredItems == v4->pRequiredItems) {
        return (bool)((uint8_t)(pCreatedItem != v4->pCreatedItem));
    }
    return true;
}
