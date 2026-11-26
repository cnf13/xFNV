#include "Fallout Shared/TESLevCharacter.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/LEVELED_OBJECT_FILE.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "xfnv/Compiler.h"

// The entry the last LVLO chunk added: the COED chunk that follows it fills in its extra data, and it is cleared again
// once that is done (TESLevCharacter::Load's function-local static plastload).
static LEVELED_OBJECT* plastload;

SettingT<GameSettingCollection> TESLevCharacter::iLevCharLevelDifferenceMax("iLevCharLevelDifferenceMax", 0);

TESLevCharacter::TESLevCharacter() : TESBoundAnimObject(), TESLeveledList(), TESModelTextureSwap() {
    cFormType = 45;
    SetInitialized(true);
}

TESLevCharacter::~TESLevCharacter() {}

// The EDID buffer is initialised to "" (one zeroed byte, allocated even for an empty chunk), so an empty chunk
// (malformed data) does not leave it uninitialised.
bool TESLevCharacter::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != LVLN_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID:
            case MOSD_ID:
            case MODD_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint32_t uiVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, uiVersion);
                break;
            }
            case COED_ID:
            case LVLO_ID: {
                // Reads the chunk code a second time to tell the two apart.
                uint32_t uiChunkAgain = apFile->GetTESChunk();
                if (uiChunkAgain == COED_ID) {
                    if (plastload) {
                        ContainerItemExtra* pExtra = new ContainerItemExtra;
                        pExtra->Load(apFile);
                        plastload->pItemExtra = pExtra;
                        plastload = nullptr;
                    }
                }
                else if (uiChunkAgain == LVLO_ID) {
                    LEVELED_OBJECT_FILE kObject;
                    ourMemset(&kObject, 0, sizeof(kObject));
                    apFile->GetChunkData(&kObject, 0xCu);
                    if (apFile->QEndian()) {
                        kObject.Endian();
                    }
                    // the form's ID stands in for the form until the forms are resolved
                    plastload = static_cast<TESLeveledList*>(this)->AddLeveledObject(
                        kObject.sLevel,
                        kObject.sCount,
                        reinterpret_cast<TESForm*>(static_cast<uintptr_t>(kObject.iFormID)),
                        nullptr
                    );
                }
                break;
            }
            case LVLF_ID: apFile->GetChunkData(&static_cast<TESLeveledList*>(this)->cLLFlags, 0u); break;
            case LVLD_ID: {
                uint8_t ucChanceNone = 0;
                apFile->GetChunkData(&ucChanceNone, 0u);
                static_cast<TESLeveledList*>(this)->SetChanceNone(ucChanceNone);
                break;
            }
            case LVLG_ID: {
                uint32_t uiChanceGlobalID = 0;
                apFile->GetChunkData(uiChanceGlobalID);
                // the global's ID stands in for the global until the forms are resolved
                static_cast<TESLeveledList*>(this)->pChanceGlobal = reinterpret_cast<TESGlobal*>(
                    static_cast<uintptr_t>(uiChanceGlobalID)
                );
                break;
            }
        }
    }
    while (apFile->NextChunk());
    SetInitialized(false);
    return true;
}

// TESLevCreature's Save slot holds the same address.
void TESLevCharacter::Save() {
    TESForm::StartForm();
    TESForm::InitializeDataComponent();
    TESLeveledList::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESForm::CloseForm();
}

// TESLevCreature's and TESLevItem's SaveGame slots hold the same address.
void TESLevCharacter::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    TESLeveledList::SaveGame(apSaveGameBuffer);
}

void TESLevCharacter::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESLeveledList*>(this)->LoadGame(apLoadGameBuffer);
}

void TESLevCharacter::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESLeveledList::Revert(apLoadGameBuffer);
}

void TESLevCharacter::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<TESLeveledList*>(this)->InitItem(this);
        static_cast<TESModelTextureSwap*>(this)->InitItem(this);
        TESForm::SetInitialized(true);
    }
}

void TESLevCharacter::Copy(TESForm* apCopy) {
    if (dynamic_cast<TESLevCharacter*>(apCopy)) {
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESLevCharacter::Compare(TESForm* apCompare) {
    TESLevCharacter* pComp = dynamic_cast<TESLevCharacter*>(apCompare);
    return !pComp || TESForm::CompareAllComponents(pComp);
}

int32_t TESLevCharacter::GetMaxLevelDifference() {
    return TESLevCharacter::iLevCharLevelDifferenceMax.UInt();
}
