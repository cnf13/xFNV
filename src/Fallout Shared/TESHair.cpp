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
#include "xfnv/Compiler.h"

TESHair::TESHair() : TESForm(), TESFullName(), TESModel(), TESTexture() {
    SetFormType(HAIR_ID);
    TESHair::InitializeData();
}

TESHair::~TESHair() {}

void TESHair::InitializeData() {
    cFlags = 0;
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
    TESHair* pCopy = dynamic_cast<TESHair*>(apCopy);
    if (pCopy) {
        TESForm::CopyAllComponents(apCopy);
        cFlags = pCopy->cFlags;
    }
}

bool TESHair::Compare(TESForm* apCompare) {
    TESHair* pCompare = dynamic_cast<TESHair*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (TESForm::CompareAllComponents(pCompare)) {
        return true;
    }
    return pCompare->cFlags != cFlags;
}

bool TESHair::IsPlayable() {
    return (cFlags & FLAG_PLAYABLE) != 0;
}

bool TESHair::CanBeMale() {
    return (cFlags & FLAG_NOT_MALE) == 0;
}

bool TESHair::CanBeFemale() {
    return (cFlags & FLAG_NOT_FEMALE) == 0;
}

bool TESHair::CanBeHairForNPC(TESNPC* apNPC) {
    bool bRetVal = false;
    if (apNPC && apNPC->GetRace()) {
        if (apNPC->GetRace()->HairList.IsInList(this)) {
            TESActorBase::SEX eSex = apNPC->GetSex();
            if (eSex) {
                if (eSex == TESActorBase::SEX_FEMALE && CanBeFemale()) {
                    return true;
                }
            }
            else if (CanBeMale()) {
                return true;
            }
        }
    }

    return bRetVal;
}
