#include "Fallout Shared/TESHair.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESTexture.h"

TESHair::TESHair() : TESForm(), TESFullName(), TESModel(), TESTexture() {
    SetFormType(HAIR_ID);
    TESHair::InitializeData();
}

TESHair::~TESHair() {}

void TESHair::InitializeData() {
    cFlags = (uint8_t)0;
}

// Reads the hair's chunks: the editor ID, the bounds, the name, the model, the icon and the flags byte.
bool TESHair::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != HAIR_ID) {
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
                // A 16-byte aligned buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESTexture*>(this), apFile); break;
            case DATA_ID: apFile->GetChunkData(&cFlags, 1u); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESHair::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModel*>(this)->Save(MODL_ID, MODT_ID);
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    TESForm::AddChunk(DATA_ID, cFlags);
    TESForm::CloseForm();
}

void TESHair::Copy(TESForm* apCopy) {
    TESHair* v3 = dynamic_cast<TESHair*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(apCopy);
        cFlags = (uint8_t)((int8_t)v3->cFlags);
    }
}

bool TESHair::Compare(TESForm* apCompare) {
    TESHair* v4 = dynamic_cast<TESHair*>(apCompare);
    if (!v4) {
        return true;
    }
    if (TESForm::CompareAllComponents(v4)) {
        return true;
    }
    return (bool)((uint8_t)(((int8_t)v4->cFlags) != ((int8_t)cFlags)));
}

bool TESHair::IsPlayable() {
    return ((cFlags & 1) != 0);
}

bool TESHair::CanBeMale() {
    return ((cFlags & 2) == 0);
}

bool TESHair::CanBeFemale() {
    return ((cFlags & 4) == 0);
}

bool TESHair::CanBeHairForNPC(TESNPC* apNPC) {
    bool bretval = false;
    if (apNPC) {
        if (apNPC->GetRace()) {
            TESHair* item = this;
            if ((&apNPC->GetRace()->HairList)->IsInList(item)) {
                TESActorBase::SEX Sex = apNPC->GetSex();
                if (Sex) {
                    if (Sex == TESActorBase::SEX_FEMALE && CanBeFemale()) {
                        return true;
                    }
                }
                else {
                    if (CanBeMale()) {
                        return true;
                    }
                }
            }
        }
    }
    return bretval;
}
