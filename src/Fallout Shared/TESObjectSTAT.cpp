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
#include "xfnv/Compiler.h"

TESObjectSTAT::TESObjectSTAT() : cPassThroughSoundIndex(-1), pRandomSound(nullptr) {
    SetFormType(STAT_ID);
}

TESObjectSTAT::~TESObjectSTAT() {}

// Reads the STAT record's chunks. The form type is read first and never used.
bool TESObjectSTAT::Load(TESFile* apFile) {
    apFile->GetTESForm();
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, 0x200);
                TESForm* pForm = this;
                pForm->SetFormEditorID(pBuffer);
                break;
            }
            case BRUS_ID: {
                uint32_t uiIndex = 0;
                apFile->GetChunkData<uint32_t>(uiIndex);
                cPassThroughSoundIndex = uiIndex <= 10 ? static_cast<char>(uiIndex) : static_cast<char>(0xFF);
                break;
            }
            case RNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                pRandomSound = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID));
                break;
            }
            case MODS_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)
                    ->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usFormVersion);
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
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    TESForm::AddChunk(BRUS_ID, static_cast<uint8_t>(cPassThroughSoundIndex));
    if (pRandomSound) {
        uint32_t uiFormID = pRandomSound->TESForm::GetFormID();
        TESForm::AddChunk(RNAM_ID, uiFormID);
    }
    TESForm::CloseForm();
}

void TESObjectSTAT::InitItem() {
    if (!GetInitialized()) {
        TESModelTextureSwap::InitItem(this);
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRandomSound));
        if (itempint) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pRandomSound = dynamic_cast<TESSound*>(pForm);
            if (!pRandomSound) {
                int32_t iStaticFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find looping/random sound (%08X) for static '%s' (%08X).",
                    itempint,
                    pEditorID,
                    iStaticFormID
                );
            }
        }
        SetInitialized(true);
    }
}

void TESObjectSTAT::Copy(TESForm* apCopy) {
    TESObjectSTAT* pStat = dynamic_cast<TESObjectSTAT*>(apCopy);
    if (pStat) {
        pRandomSound = pStat->pRandomSound;
        TESForm::CopyAllComponents(pStat);
        cPassThroughSoundIndex = pStat->cPassThroughSoundIndex;
    }
}

bool TESObjectSTAT::Compare(TESForm* apCompare) {
    TESObjectSTAT* pStat = dynamic_cast<TESObjectSTAT*>(apCompare);
    if (!pStat) {
        return true;
    }
    if (TESForm::CompareAllComponents(pStat)) {
        return true;
    }
    if (cPassThroughSoundIndex == pStat->cPassThroughSoundIndex) {
        return pRandomSound != pStat->pRandomSound;
    }
    return true;
}
