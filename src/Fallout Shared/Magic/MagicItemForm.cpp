#include "Fallout Shared/Magic/MagicItemForm.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

// The record's chunks: the bound, the name, the effects (from 'EFID' or 'CTDA') and the editor ID; every chunk then
// goes to the magic item's own type-specific chunk loader, and the item's data chunk is cleared and read.
bool MagicItemForm::Load(TESFile* apFile) {
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk > EFID_ID) {
            if (uiChunk == OBND_ID) {
                LoadObjectBound(apFile);
            }
            else if (uiChunk == FULL_ID) {
                TESFullName::LoadFullNameChunk(this, apFile);
            }
        }
        else if (uiChunk == EFID_ID || uiChunk == CIDA_ID) {
            const char* pName = TESFullName::GetFullName();
            EffectItemList::Load(apFile, pName);
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        LoadMagicItemChunk(apFile, static_cast<CHUNK_ID>(uiChunk));
        if (uiChunk == static_cast<uint32_t>(GetChunkID())) {
            uint32_t uiDataSize = GetDataSize();
            void* pData = GetData();
            ourMemset(pData, 0, uiDataSize);
            uint32_t uiMaxSize = GetDataSize();
            void* pDataToRead = GetData();
            apFile->GetChunkData(pDataToRead, uiMaxSize);
            if (apFile->QEndian()) {
                Endian();
            }
        }
    }
    while (apFile->NextChunk());
    if (ItemsInList()) {
        return true;
    }
    const char* pName = TESFullName::GetFullName();
    Warning("GAME EFFECTS: Effect Item (%08X) %s has no effects defined.", GetFormID(), pName);
    return true;
}

void MagicItemForm::Save() {
    StartForm();
    TESFullName::Save();
    SaveMagicItemComponents();
    SaveMagicItemData();
    EffectItemList::Save();
    CloseForm();
}

void MagicItemForm::InitItem() {
    if (!GetInitialized()) {
        MagicItemInitItem(this);
        SetInitialized(true);
    }
}

void MagicItemForm::SetAltered(bool abValue) {
    TESForm::SetAltered(abValue);
}

void MagicItemForm::Copy(TESForm* apCopy) {
    MagicItemForm* pmagicItem = dynamic_cast<MagicItemForm*>(apCopy);
    if (pmagicItem) {
        CopyAllComponents(apCopy);
        CopyMagicItemData(pmagicItem);
        EffectItemList::Copy(pmagicItem);
    }
}

bool MagicItemForm::Compare(TESForm* apCompare) {
    MagicItemForm* pmagicItem = dynamic_cast<MagicItemForm*>(apCompare);
    if (!pmagicItem) {
        return true;
    }
    if (CompareAllComponents(apCompare)) {
        return true;
    }
    if (CompareMagicItemData(pmagicItem)) {
        return true;
    }
    return EffectItemList::Compare(&static_cast<EffectItemList&>(static_cast<MagicItem&>(*pmagicItem)));
}
