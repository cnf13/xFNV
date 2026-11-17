#include "Fallout Shared/TESObjectANIO.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "SDK/MSVC/rtti.h"

TESObjectANIO::TESObjectANIO() : pIdleAnim(nullptr) {
    SetFormType(77u);
}

TESObjectANIO::~TESObjectANIO() {}

// Reads the ANIO record: the model, the texture swaps and the idle form.
bool TESObjectANIO::Load(TESFile* apFile) {
    const uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != ANIO_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        const CHUNK_ID eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case MODD_ID:
            case MOSD_ID:
            case MODL_ID:
            case MODT_ID:
                TESModel::LoadModelChunk(static_cast<TESModel*>(static_cast<TESModelTextureSwap*>(this)), apFile);
                break;
            case MODS_ID: {
                const uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
                apFile->GetChunkData(pBuffer, uiSize);
                const uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case DATA_ID: {
                uint32_t uiIdleForm = 0;
                apFile->GetChunkData<uint32_t>(uiIdleForm);
                // Stores the chunk's value in the pointer field unchanged.
                pIdleAnim = reinterpret_cast<TESIdleForm*>(uiIdleForm);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESObjectANIO::Save() {
    uint32_t v2;
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    if (pIdleAnim) {
        v2 = pIdleAnim->TESForm::GetFormID();
        TESForm::AddChunk(DATA_ID, v2);
    }
    TESForm::CloseForm();
}

void TESObjectANIO::InitItem() {
    if (!GetInitialized()) {
        TESModelTextureSwap::InitItem(this);
        if (pIdleAnim) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIdleAnim));
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pIdleAnim = dynamic_cast<TESIdleForm*>(FormByNumericID);
        }
        SetInitialized(true);
    }
}

void TESObjectANIO::Copy(TESForm* apCopy) {
    TESObjectANIO* v3 = dynamic_cast<TESObjectANIO*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(apCopy);
        pIdleAnim = v3->pIdleAnim;
    }
}

bool TESObjectANIO::Compare(TESForm* apCompare) {
    TESObjectANIO* v4 = dynamic_cast<TESObjectANIO*>(apCompare);
    if (!v4) {
        return true;
    }
    if (TESForm::CompareAllComponents(v4)) {
        return true;
    }
    return (bool)((uint8_t)(pIdleAnim != v4->pIdleAnim));
}

bool TESObjectANIO::GetUsesIdle(TESIdleForm* apIdle) {
    return (bool)((uint8_t)(pIdleAnim == apIdle));
}
