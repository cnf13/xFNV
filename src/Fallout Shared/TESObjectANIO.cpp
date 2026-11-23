#include "Fallout Shared/TESObjectANIO.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
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
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    if (pIdleAnim) {
        uint32_t uiIdleID = pIdleAnim->TESForm::GetFormID();
        TESForm::AddChunk(DATA_ID, uiIdleID);
    }
    TESForm::CloseForm();
}

void TESObjectANIO::InitItem() {
    if (!GetInitialized()) {
        TESModelTextureSwap::InitItem(this);
        if (pIdleAnim) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIdleAnim));
            TESForm::AddCompileIndex(itempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            pIdleAnim = dynamic_cast<TESIdleForm*>(pForm);
        }
        SetInitialized(true);
    }
}

void TESObjectANIO::Copy(TESForm* apCopy) {
    TESObjectANIO* pCopy = dynamic_cast<TESObjectANIO*>(apCopy);
    if (pCopy) {
        TESForm::CopyAllComponents(apCopy);
        pIdleAnim = pCopy->pIdleAnim;
    }
}

bool TESObjectANIO::Compare(TESForm* apCompare) {
    TESObjectANIO* pComp = dynamic_cast<TESObjectANIO*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (TESForm::CompareAllComponents(pComp)) {
        return true;
    }
    return pIdleAnim != pComp->pIdleAnim;
}

bool TESObjectANIO::GetUsesIdle(TESIdleForm* apIdle) {
    return pIdleAnim == apIdle;
}
