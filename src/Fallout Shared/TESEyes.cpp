#include "Fallout Shared/TESEyes.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESTexture.h"
#include "xfnv/Compiler.h"

TESEyes::TESEyes() : TESForm(), TESFullName(), TESTexture() {
    SetFormType(EYES_ID);
    TESEyes::InitializeData();
}

TESEyes::~TESEyes() {}

void TESEyes::InitializeData() {
    cFlags = (uint8_t)0;
}

bool TESEyes::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != 11) {
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
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case DATA_ID: apFile->GetChunkData(&cFlags, 1); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESEyes::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    TESForm::AddChunk(DATA_ID, cFlags);
    TESForm::CloseForm();
}

void TESEyes::Copy(TESForm* apCopy) {
    int8_t v2;
    int8_t v3;
    int8_t v4;
    TESEyes* v6 = dynamic_cast<TESEyes*>(apCopy);
    if (v6) {
        TESForm::CopyAllComponents(apCopy);
        v2 = (int8_t)v6->IsPlayable();
        SetPlayable(v2);
        v3 = (int8_t)v6->CanBeMale();
        SetCanBeMale(v3);
        v4 = (int8_t)v6->CanBeFemale();
        SetCanBeFemale(v4);
    }
}

bool TESEyes::Compare(TESForm* apCompare) {
    TESEyes* v7 = dynamic_cast<TESEyes*>(apCompare);
    if (!v7) {
        return true;
    }
    if (TESForm::CompareAllComponents(v7)) {
        return true;
    }
    int32_t isPlayable = (int32_t)v7->IsPlayable();
    if (((uint32_t)isPlayable) != ((uint32_t)((uint8_t)IsPlayable()))) {
        return true;
    }
    int32_t canBeMale = (int32_t)v7->CanBeMale();
    if (((uint32_t)canBeMale) != ((uint32_t)((uint8_t)CanBeMale()))) {
        return true;
    }
    int32_t canBeFemale = (int32_t)v7->CanBeFemale();
    return (bool)((uint8_t)(((uint32_t)canBeFemale) != ((uint32_t)((uint8_t)CanBeFemale()))));
}

bool TESEyes::IsPlayable() {
    return ((cFlags & 1) != 0);
}

bool TESEyes::CanBeEyesForNPC(TESNPC* apNPC) {
    bool bretval = false;
    if ((apNPC) && (apNPC->GetRace())) {
        TESEyes* item = this;
        if (apNPC->GetRace()->GetEyeColorList()->IsInList(item)) {
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

    return bretval;
}

void TESEyes::SetPlayable(bool abVal) {
    uint8_t v2;
    if (abVal) {
        v2 = (uint8_t)(cFlags | 1);
    }
    else {
        v2 = (uint8_t)(cFlags & 254);
    }
    cFlags = v2;
}

bool TESEyes::CanBeMale() {
    return ((cFlags & 2) == 0);
}

void TESEyes::SetCanBeMale(bool abVal) {
    uint8_t v2;
    if (abVal) {
        v2 = (uint8_t)(cFlags & 253);
    }
    else {
        v2 = (uint8_t)(cFlags | 2);
    }
    cFlags = v2;
}

bool TESEyes::CanBeFemale() {
    return ((cFlags & 4) == 0);
}

void TESEyes::SetCanBeFemale(bool abVal) {
    uint8_t v2;
    if (abVal) {
        v2 = (uint8_t)(cFlags & 251);
    }
    else {
        v2 = (uint8_t)(cFlags | 4);
    }
    cFlags = v2;
}

BSSimpleList<TESEyes*>* TESRace::GetEyeColorList() {
    return &EyeList;
}
