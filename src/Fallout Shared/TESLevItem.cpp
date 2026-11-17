#include "Fallout Shared/TESLevItem.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/LEVELED_OBJECT_FILE.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"

// The entry the last LVLO chunk created; the COED chunk that follows gives it its extra data
// (TESLevItem::Load's function-local static plastload).
static LEVELED_OBJECT* plastload;

SettingT<GameSettingCollection> gs_iLevItemLevelDifferenceMax("iLevItemLevelDifferenceMax", 0);

TESLevItem::TESLevItem() : TESBoundObject(), TESLeveledList() {
    SetFormType(0x34u);
    TESForm::InitializeDataComponent();
}

TESLevItem::~TESLevItem() {}

// Reads the LVLI record: the leveled list chunks and the object bounds. The EDID buffer is initialised to "" (one
// zeroed byte, allocated even for an empty chunk), so an empty chunk (malformed data) does not leave it uninitialised.
bool TESLevItem::Load(TESFile* apFile) {
    const uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != LVLI_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        switch (apFile->GetTESChunk()) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case LVLF_ID: apFile->GetChunkData(&static_cast<TESLeveledList*>(this)->cLLFlags, 0); break;
            case LVLD_ID: {
                uint8_t cChanceNone = 0;
                apFile->GetChunkData(&cChanceNone, 0);
                static_cast<TESLeveledList*>(this)->SetChanceNone(cChanceNone);
                break;
            }
            case LVLG_ID: {
                uint32_t uiChanceGlobal = 0;
                apFile->GetChunkData<uint32_t>(uiChanceGlobal);
                // Stores the chunk's value in the pointer field unchanged.
                static_cast<TESLeveledList*>(this)->pChanceGlobal = reinterpret_cast<TESGlobal*>(uiChanceGlobal);
                break;
            }
            case COED_ID:
            case LVLO_ID: {
                // Re-reads the chunk: the COED chunk belongs to the entry the last LVLO chunk added.
                const uint32_t uiSubChunk = apFile->GetTESChunk();
                if (uiSubChunk == COED_ID) {
                    if (plastload) {
                        void* pMemory = ourMalloc(sizeof(ContainerItemExtra));
                        ContainerItemExtra* pExtra = nullptr;
                        if (pMemory) {
                            pExtra = ::new (pMemory) ContainerItemExtra;
                        }
                        pExtra->Load(apFile);
                        plastload->pItemExtra = pExtra;
                        plastload = nullptr;
                    }
                }
                else if (uiSubChunk == LVLO_ID) {
                    LEVELED_OBJECT_FILE kObject;
                    ourMemset(&kObject, 0, sizeof(kObject));
                    apFile->GetChunkData(&kObject, 0xC);
                    if (apFile->QEndian()) {
                        kObject.Endian();
                    }
                    // Passes the entry's form ID as the pointer argument, as the game does.
                    plastload = static_cast<TESLeveledList*>(this)->AddLeveledObject(
                        kObject.sLevel, kObject.sCount, reinterpret_cast<TESForm*>(kObject.iFormID), nullptr
                    );
                }
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    SetInitialized(false);
    return true;
}

void TESLevItem::Save() {
    TESForm::StartForm();
    TESForm::InitializeDataComponent();
    static_cast<TESLeveledList*>(this)->Save();
    TESForm::CloseForm();
}

void TESLevItem::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    TESLeveledList::SaveGame(apSaveGameBuffer);
}

void TESLevItem::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESLeveledList*>(this)->LoadGame(apLoadGameBuffer);
}

void TESLevItem::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESLeveledList::Revert(apLoadGameBuffer);
}

void TESLevItem::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<TESLeveledList*>(this)->InitItem(this);
        TESForm::SetInitialized(1);
    }
}

void TESLevItem::Copy(TESForm* apCopy) {
    if (dynamic_cast<TESLevItem*>(apCopy)) {
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESLevItem::Compare(TESForm* apCompare) {
    TESLevItem* v4 = dynamic_cast<TESLevItem*>(apCompare);
    return ((!v4) || (((int8_t)TESForm::CompareAllComponents(v4)) != 0));
}

int32_t TESLevItem::GetMaxLevelDifference() {
    return (&gs_iLevItemLevelDifferenceMax)->UInt();
}
