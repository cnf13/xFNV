#include "Fallout Shared/TESObjectMISC.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
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
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

TESObjectMISC::TESObjectMISC() {
    SetFormType(MISC_ID);
    TESObjectMISC::InitializeData();
    SetInitialized(true);
}

TESObjectMISC::~TESObjectMISC() {}

void TESObjectMISC::InitializeData() {
    pLoopingSound = nullptr;
}

// Reads the MISC record's chunks.
bool TESObjectMISC::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
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
            case DEST_ID:
            case DSTD_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
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
            case RNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetLoopingSound((TESSound*)uiFormID);
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

void TESObjectMISC::Save() {
    uint32_t v2;
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(static_cast<CHUNK_ID>(1329809741));
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    static_cast<BGSPickupPutdownSounds*>(this)->Save();
    TESForm::SaveData();
    if (pLoopingSound) {
        v2 = pLoopingSound->TESForm::GetFormID();
        TESForm::AddChunk(RNAM_ID, v2);
    }
    TESForm::CloseForm();
}

uint16_t TESObjectMISC::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESObjectMISC::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(aiFlags);
}

void TESObjectMISC::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESObjectMISC::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectMISC::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESObjectMISC::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        BGSPickupPutdownSounds::InitItem(this);
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLoopingSound));
        if (itempint) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pLoopingSound = dynamic_cast<TESSound*>(FormByNumericID);
            if (!pLoopingSound) {
                int32_t CastingType = iFormID;
                const char* v3 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find pLoopingSound (%08X) for container '%s' (%08X).", itempint, v3,
                    CastingType
                );
            }
        }
        SetInitialized(true);
    }
}

void TESObjectMISC::Copy(TESForm* apCopy) {
    TESSound* allCollector;
    TESObjectMISC* v4 = dynamic_cast<TESObjectMISC*>(apCopy);
    if (v4) {
        allCollector = v4->GetLoopingSound();
        SetLoopingSound(allCollector);
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESObjectMISC::Compare(TESForm* apCompare) {
    TESObjectMISC* v5 = dynamic_cast<TESObjectMISC*>(apCompare);
    if (!v5) {
        return true;
    }
    TESSound* allCollector = GetLoopingSound();
    return ((allCollector != v5->GetLoopingSound()) || (((int8_t)TESForm::CompareAllComponents(v5)) != 0));
}

TESSound* TESObjectMISC::GetLoopingSound() {
    return pLoopingSound;
}

void TESObjectMISC::SetLoopingSound(TESSound* apSound) {
    pLoopingSound = apSound;
}
