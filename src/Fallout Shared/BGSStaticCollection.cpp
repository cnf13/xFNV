#include "Fallout Shared/BGSStaticCollection.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"

BGSStaticCollection::BGSStaticCollection() {
    SetFormType(SCOL_ID);
}

BGSStaticCollection::~BGSStaticCollection() {
    BGSStaticCollection::ClearData();
}

void BGSStaticCollection::ClearData() {
    TESForm::InitializeDataComponent();
}

// Reads the model chunks (MODL/MODT/MOSD/MODD through TESModel::LoadModelChunk, MODS into the texture swap list), the
// editor ID and the object bound. LoadForm is called without SetInitialized, as the game does.
bool BGSStaticCollection::Load(TESFile* apFile) {
    LoadForm(apFile);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        switch (eChunk) {
            case MODL_ID:
            case MODT_ID:
            case MOSD_ID:
            case MODD_ID: {
                TESModel* pModel = static_cast<TESModel*>(this);
                TESModel::LoadModelChunk(pModel, apFile);
                break;
            }
            case MODS_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
                apFile->GetChunkData(pBuffer, uiSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

void BGSStaticCollection::Save() {
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    TESForm::CloseForm();
}

void BGSStaticCollection::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<TESModelTextureSwap*>(this)->InitItem(this);
        TESForm::SetInitialized(1);
    }
}

void BGSStaticCollection::Copy(TESForm* apCopy) {
    BGSStaticCollection* v3 = dynamic_cast<BGSStaticCollection*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(v3);
    }
}

bool BGSStaticCollection::Compare(TESForm* apCompare) {
    BGSStaticCollection* v4 = dynamic_cast<BGSStaticCollection*>(apCompare);
    return ((!v4) || (((int8_t)TESForm::CompareAllComponents(v4)) != 0));
}
