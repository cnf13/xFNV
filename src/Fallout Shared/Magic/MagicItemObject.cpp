#include "Fallout Shared/Magic/MagicItemObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "SDK/MSVC/rtti.h"

bool MagicItemObject::Load(TESFile* apFile) {
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case EFID_ID:
            case CIDA_ID:
                static_cast<EffectItemList*>(this)->Load(apFile, static_cast<TESFullName*>(this)->GetFullName());
                break;
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
        }
        // The item's own chunk: the effect list hands it to the derived item type.
        LoadMagicItemChunk(apFile, static_cast<CHUNK_ID>(uiChunk));
        if (uiChunk == GetChunkID()) {
            uint32_t uiDataSize = GetDataSize();
            void* pData = GetData();
            ourMemset(pData, 0, uiDataSize);
            uint32_t uiDataSize2 = GetDataSize();
            void* pData2 = GetData();
            apFile->GetChunkData(pData2, uiDataSize2);
#if XFNV_64BIT
            // A potion's ENIT record is 0x14 bytes in the file, the addiction's form ID at 8 and the consumption
            // sound's at 0x10; AlchemyItemData's pointers take 8 bytes here, so the record is spread over its fields.
            if (static_cast<TESForm*>(this)->GetFormType() == ALCH_ID) {
                TESFile::WidenFormIDRecords(
                    pData2, 1, 0x14, sizeof(AlchemyItemData),
                    {{8, offsetof(AlchemyItemData, pAddictionItem)},
                     {0x10, offsetof(AlchemyItemData, pConsumptionSound)}}
                );
            }
#endif
            if (apFile->QEndian()) {
                Endian();
            }
        }
    }
    while (apFile->NextChunk());
    if (ItemsInList() == 0) {
        Warning(
            "GAME EFFECTS: Effect Item (%08X) %s has no effects defined.", GetFormID(),
            static_cast<TESFullName*>(this)->GetFullName()
        );
    }
    return true;
}

void MagicItemObject::Save() {
    StartForm();
    TESFullName::Save();
    SaveMagicItemComponents();
    SaveMagicItemData();
    EffectItemList::Save();
    CloseForm();
}

// Resolves the item's loaded forms once.
void MagicItemObject::InitItem() {
    if (!GetInitialized()) {
        MagicItemInitItem(this);
        SetInitialized(true);
    }
}

void MagicItemObject::SetAltered(bool abValue) {
    TESForm::SetAltered(abValue);
}

void MagicItemObject::Copy(TESForm* apCopy) {
    MagicItemObject* pmagicItem = dynamic_cast<MagicItemObject*>(apCopy);
    if (pmagicItem) {
        CopyAllComponents(apCopy);
        CopyMagicItemData(pmagicItem);
        EffectItemList::Copy(pmagicItem);
    }
}

bool MagicItemObject::Compare(TESForm* apCompare) {
    MagicItemObject* pmagicItem = dynamic_cast<MagicItemObject*>(apCompare);
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
