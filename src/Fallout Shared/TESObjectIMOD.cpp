#include "Fallout Shared/TESObjectIMOD.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "xfnv/Compiler.h"

TESObjectIMOD::TESObjectIMOD() {
    cFormType = IMOD_ID;
    SetInitialized(true);
}

TESObjectIMOD::~TESObjectIMOD() {}

// Reads the IMOD record's chunks.
bool TESObjectIMOD::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case MODL_ID:
            case MODD_ID:
            case MODT_ID:
            case MOSD_ID: TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile); break;
            case OBND_ID: {
                TESBoundObject* pObject = this;
                pObject->LoadObjectBound(apFile);
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, 0x200);
                TESForm* pForm = this;
                pForm->SetFormEditorID(pBuffer);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case DESC_ID: TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile); break;
            case DEST_ID:
            case DSTD_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case MODS_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(
                    pBuffer, uiChunkSize, usFormVersion
                );
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(static_cast<BGSMessageIcon*>(this), apFile); break;
            case SCRI_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                TESScriptableForm* pScriptable = static_cast<TESScriptableForm*>(this);
                pScriptable->SetFormScript((Script*)uiFormID);
                pScriptable->InitItem(this);
                break;
            }
            case YNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound((TESSound*)uiFormID);
                break;
            }
            case ZNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound((TESSound*)uiFormID);
                break;
            }
            case DATA_ID: LoadData(apFile, nullptr, 0); break;
            default: break;
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

void TESObjectIMOD::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(static_cast<CHUNK_ID>(1329809741));
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<TESDescription*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    static_cast<BGSPickupPutdownSounds*>(this)->Save();
    TESForm::SaveData();
    TESForm::CloseForm();
}

uint16_t TESObjectIMOD::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESObjectIMOD::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(aiFlags);
}

void TESObjectIMOD::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESObjectIMOD::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectIMOD::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESObjectIMOD::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<TESScriptableForm*>(this)->InitItem(this);
        static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
        static_cast<TESModelTextureSwap*>(this)->InitItem(this);
        static_cast<BGSPickupPutdownSounds*>(this)->InitItem(this);
        TESForm::SetInitialized(true);
    }
}

void TESObjectIMOD::Copy(TESForm* apCopy) {
    if (dynamic_cast<TESObjectIMOD*>(apCopy)) {
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESObjectIMOD::Compare(TESForm* apCompare) {
    TESObjectIMOD* v4 = dynamic_cast<TESObjectIMOD*>(apCompare);
    return ((!v4) || (((int8_t)TESForm::CompareAllComponents(v4)) != 0));
}
