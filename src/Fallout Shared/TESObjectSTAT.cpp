#include "Fallout Shared/TESObjectSTAT.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

TESObjectSTAT::TESObjectSTAT() : cPassThroughSoundIndex(-1), pRandomSound(nullptr) {
    SetFormType(STAT_ID);
}

TESObjectSTAT::~TESObjectSTAT() {}

// Reads the STAT record's chunks. The form type is read first and never used, as the game does.
bool TESObjectSTAT::Load(TESFile* apFile) {
    (void)apFile->GetTESForm();
    LoadForm(apFile);
    for (;;) {
        CHUNK_ID eChunk = apFile->GetTESChunk();
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                apFile->GetChunkData(pBuffer, 0x200);
                TESForm* pForm = this;
                pForm->SetFormEditorID(pBuffer);
                break;
            }
            case BRUS_ID: {
                uint32_t uiIndex = 0;
                apFile->GetChunkData<uint32_t>(uiIndex);
                cPassThroughSoundIndex = (uiIndex <= 10) ? static_cast<char>(uiIndex) : static_cast<char>(0xFF);
                break;
            }
            case RNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                pRandomSound = (TESSound*)uiFormID;
                break;
            }
            case MODS_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(
                    pBuffer, uiChunkSize, usFormVersion
                );
                break;
            }
            default: break;
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

void TESObjectSTAT::Save() {
    uint32_t v2;
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    TESForm::AddChunk(static_cast<CHUNK_ID>(1398100546), (uint8_t)cPassThroughSoundIndex);
    if (pRandomSound) {
        v2 = pRandomSound->TESForm::GetFormID();
        TESForm::AddChunk(RNAM_ID, v2);
    }
    TESForm::CloseForm();
}

void TESObjectSTAT::InitItem() {
    if (!GetInitialized()) {
        TESModelTextureSwap::InitItem(this);
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRandomSound));
        if (itempint) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pRandomSound = dynamic_cast<TESSound*>(FormByNumericID);
            if (!pRandomSound) {
                int32_t CastingType = iFormID;
                const char* v3 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find looping/random sound (%08X) for static '%s' (%08X).", itempint, v3,
                    CastingType
                );
            }
        }
        SetInitialized(true);
    }
}

void TESObjectSTAT::Copy(TESForm* apCopy) {
    TESObjectSTAT* v3 = dynamic_cast<TESObjectSTAT*>(apCopy);
    if (v3) {
        pRandomSound = v3->pRandomSound;
        TESForm::CopyAllComponents(v3);
        cPassThroughSoundIndex = v3->cPassThroughSoundIndex;
    }
}

bool TESObjectSTAT::Compare(TESForm* apCompare) {
    TESObjectSTAT* v4 = dynamic_cast<TESObjectSTAT*>(apCompare);
    if (!v4) {
        return true;
    }
    if (TESForm::CompareAllComponents(v4)) {
        return true;
    }
    if (cPassThroughSoundIndex == v4->cPassThroughSoundIndex) {
        return (bool)((uint8_t)(pRandomSound != v4->pRandomSound));
    }
    return true;
}
