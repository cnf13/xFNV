#include "Fallout Shared/BGSMovableStatic.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

MOVABLE_STATIC_DATA::MOVABLE_STATIC_DATA() : cFlags(0) {}

MOVABLE_STATIC_DATA& MOVABLE_STATIC_DATA::Copy(MOVABLE_STATIC_DATA& aCopy) {
    cFlags = aCopy.cFlags;
    return *this;
}

bool MOVABLE_STATIC_DATA::Compare(MOVABLE_STATIC_DATA& aComp) {
    return cFlags != aComp.cFlags;
}

BGSMovableStatic::BGSMovableStatic() {
    cFormType = 34;
    BGSMovableStatic::InitializeData();
}

BGSMovableStatic::~BGSMovableStatic() {}

void BGSMovableStatic::InitializeData() {
    pSoundLoop = nullptr;
}

// Reads the movable static's chunks. Reads the record's form type first and discards it.
bool BGSMovableStatic::Load(TESFile* apFile) {
    apFile->GetTESForm();
    LoadForm(apFile);
    for (;;) {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODL_ID:
            case MODD_ID:
            case MOSD_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
                apFile->GetChunkData(pBuffer, uiSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case DSTD_ID:
            case DEST_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case DATA_ID: LoadData(apFile, reinterpret_cast<char*>(&data), 1); break;
            case SNAM_ID: {
                uint32_t uiSoundLoop = 0;
                apFile->GetChunkData<uint32_t>(uiSoundLoop);
                // Stores the chunk's value in the pointer field unchanged, as the game does.
                SetLoopingSound(reinterpret_cast<TESSound*>(uiSoundLoop));
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

void BGSMovableStatic::Save() {
    static_cast<TESObjectSTAT*>(this)->StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    BGSDestructibleObjectForm::Save();
    if (TESForm::QEndianSwapOnSave()) {}
    static_cast<TESObjectSTAT*>(this)->__SaveData(&data, 1u);
    if (TESForm::QEndianSwapOnSave()) {}
    if (pSoundLoop) {
        TESForm::AddChunk(SNAM_ID, pSoundLoop->iFormID);
    }
    static_cast<TESObjectSTAT*>(this)->CloseForm();
}

// Resolves the destructible data, the texture swaps and the looping sound's form ID loaded by Load.
void BGSMovableStatic::InitItem() {
    if (!GetInitialized()) {
        BGSDestructibleObjectForm::InitItem(static_cast<TESObjectSTAT*>(this));
        TESModelTextureSwap::InitItem(static_cast<TESObjectSTAT*>(this));
        // Load stores the sound's form ID in the pointer field.
        uint32_t uiSoundID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundLoop));
        if (uiSoundID) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiSoundID, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiSoundID);
            pSoundLoop = dynamic_cast<TESSound*>(pForm);
            if (!pSoundLoop) {
                uint32_t uiFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find looping sound (%08X) for movable static '%s' (%08X).", uiSoundID,
                    pEditorID, uiFormID
                );
            }
        }
        SetInitialized(true);
    }
}

void BGSMovableStatic::Copy(TESForm* apCopy) {
    BGSMovableStatic* pcopy = dynamic_cast<BGSMovableStatic*>(apCopy);
    if (pcopy) {
        SetLoopingSound(pcopy->GetLoopingSound());
        data.Copy(pcopy->data);
        static_cast<TESObjectSTAT*>(this)->CopyAllComponents(pcopy);
    }
}

bool BGSMovableStatic::Compare(TESForm* apCompare) {
    BGSMovableStatic* pcomp = dynamic_cast<BGSMovableStatic*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (static_cast<TESObjectSTAT*>(this)->CompareAllComponents(static_cast<TESObjectSTAT*>(pcomp))) {
        return true;
    }
    if (data.Compare(pcomp->data)) {
        return true;
    }
    TESSound* LoopingSound = GetLoopingSound();
    return LoopingSound != pcomp->GetLoopingSound();
}

TESSound* BGSMovableStatic::GetLoopingSound() {
    return pSoundLoop;
}

void BGSMovableStatic::SetLoopingSound(TESSound* apSound) {
    pSoundLoop = apSound;
}
