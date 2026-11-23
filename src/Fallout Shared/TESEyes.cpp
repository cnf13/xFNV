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
    cFlags = 0;
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
    TESEyes* pcopy = dynamic_cast<TESEyes*>(apCopy);
    if (pcopy) {
        TESForm::CopyAllComponents(apCopy);
        SetPlayable(pcopy->IsPlayable());
        SetCanBeMale(pcopy->CanBeMale());
        SetCanBeFemale(pcopy->CanBeFemale());
    }
}

bool TESEyes::Compare(TESForm* apCompare) {
    TESEyes* pcompare = dynamic_cast<TESEyes*>(apCompare);
    if (!pcompare) {
        return true;
    }
    if (TESForm::CompareAllComponents(pcompare)) {
        return true;
    }
    int32_t isPlayable = pcompare->IsPlayable();
    if (isPlayable != IsPlayable()) {
        return true;
    }
    int32_t canBeMale = pcompare->CanBeMale();
    if (canBeMale != CanBeMale()) {
        return true;
    }
    int32_t canBeFemale = pcompare->CanBeFemale();
    return canBeFemale != CanBeFemale();
}

bool TESEyes::IsPlayable() {
    return (cFlags & PLAYABLE) != 0;
}

bool TESEyes::CanBeEyesForNPC(TESNPC* apNPC) {
    bool bretval = false;
    if (apNPC && apNPC->GetRace()) {
        TESEyes* pEyes = this;
        if (apNPC->GetRace()->GetEyeColorList()->IsInList(pEyes)) {
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

    return bretval;
}

void TESEyes::SetPlayable(bool abVal) {
    if (abVal) {
        cFlags |= PLAYABLE;
    }
    else {
        cFlags &= ~PLAYABLE;
    }
}

bool TESEyes::CanBeMale() {
    return (cFlags & NOT_MALE) == 0;
}

void TESEyes::SetCanBeMale(bool abVal) {
    if (abVal) {
        cFlags &= ~NOT_MALE;
    }
    else {
        cFlags |= NOT_MALE;
    }
}

bool TESEyes::CanBeFemale() {
    return (cFlags & NOT_FEMALE) == 0;
}

void TESEyes::SetCanBeFemale(bool abVal) {
    if (abVal) {
        cFlags &= ~NOT_FEMALE;
    }
    else {
        cFlags |= NOT_FEMALE;
    }
}

BSSimpleList<TESEyes*>* TESRace::GetEyeColorList() {
    return &EyeList;
}
