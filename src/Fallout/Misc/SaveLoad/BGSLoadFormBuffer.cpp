#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"

BGSLoadFormBuffer::BGSLoadFormBuffer() : iFormID(0), iSize(0), pForm(nullptr), iFlags(0) {}

BGSLoadFormBuffer::~BGSLoadFormBuffer() {}

uint8_t BGSLoadFormBuffer::GetVersion() {
    return Header.cVersion;
}

TESForm* BGSLoadFormBuffer::GetForm() {
    if (!pForm && GetFormPending()) {
        pForm = TESForm::GetFormByNumericID(iFormID);
    }
    return pForm;
}

TESObjectREFR* BGSLoadFormBuffer::GetReference() {
    TESForm* pLoadedForm = GetForm();
    if (pLoadedForm && pLoadedForm->IsReference()) {
        return static_cast<TESObjectREFR*>(pLoadedForm);
    }

    return nullptr;
}

Actor* BGSLoadFormBuffer::GetActor() {
    Actor* pActor = static_cast<Actor*>(GetReference());
    if (pActor && pActor->IsActor()) {
        return pActor;
    }

    return nullptr;
}

void BGSLoadFormBuffer::SetReconstructedForm(bool abValue) {
    if (abValue) {
        iFlags |= LOAD_FORM_RECONSTRUCTED_FORM;
    }
    else {
        iFlags &= ~static_cast<uint32_t>(LOAD_FORM_RECONSTRUCTED_FORM);
    }
}

bool BGSLoadFormBuffer::GetRevertOnly() {
    return (iFlags & LOAD_FORM_REVERT_ONLY) != 0;
}

void BGSLoadFormBuffer::SetRevertOnly(bool abValue) {
    if (abValue) {
        iFlags |= LOAD_FORM_REVERT_ONLY;
    }
    else {
        iFlags &= ~static_cast<uint32_t>(LOAD_FORM_REVERT_ONLY);
    }
}

bool BGSLoadFormBuffer::GetCellChanged() {
    return (iFlags & LOAD_FORM_CELL_CHANGED) != 0;
}

void BGSLoadFormBuffer::SetFormPending(bool abValue) {
    if (abValue) {
        iFlags |= LOAD_FORM_FORM_PENDING;
    }
    else {
        iFlags &= ~static_cast<uint32_t>(LOAD_FORM_FORM_PENDING);
    }
}

bool BGSLoadFormBuffer::GetFormPending() {
    return (iFlags & LOAD_FORM_FORM_PENDING) != 0;
}

void BGSLoadFormBuffer::SetChangeFlags(BGSChangeFlags aiChangeFlags) {
    Header.iChangeFlags = aiChangeFlags;
}

void BGSLoadFormBuffer::ClearBuffer() {
    ourFree(pBuffer);
    pBuffer = nullptr;
    iBufferSize = 0;
    iBufferPosition = 0;
}

ENUM_FORM_ID BGSLoadFormBuffer::GetFormType() {
    return Header.FormInfo.GetFormType();
}

uint32_t BGSLoadFormBuffer::LoadDataFromFile(BGSSaveLoadFile* apFile) {
    uint32_t uiRead = Load(apFile, iSize);
    if (uiRead != iSize) {
        return static_cast<uint32_t>(-1);
    }
    return uiRead;
}

void BGSLoadFormBuffer::SkipData(BGSSaveLoadFile* apFile, uint32_t aiAlreadyLoadedAmount) {
    if (iSize != aiAlreadyLoadedAmount) {
        apFile->SeekCurrent(iSize - aiAlreadyLoadedAmount);
    }
}

void BGSLoadFormBuffer::SetCellChanged(bool abValue) {
    if (abValue) {
        iFlags |= LOAD_FORM_CELL_CHANGED;
    }
    else {
        iFlags &= ~static_cast<uint32_t>(LOAD_FORM_CELL_CHANGED);
    }
}

void BGSLoadFormBuffer::LoadHeader(BGSSaveLoadFile* apFile) {
    apFile->Read(&Header, 9u);
    iFormID = Header.FormIDIndex.GetNumericID();
    SIZE_TYPE eSizeType = Header.FormInfo.GetSizeType();
    LoadGameSizeFromFile(apFile, iSize, eSizeType);
}

void BGSLoadFormBuffer::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSSaveLoadFormInfo::BGSSaveLoadFormInfo() : cData(0) {}

SIZE_TYPE BGSSaveLoadFormInfo::GetSizeType() {
    return static_cast<SIZE_TYPE>(static_cast<int32_t>(cData) >> 6);
}

BGSSaveLoadFormHeader::BGSSaveLoadFormHeader() : FormIDIndex(), iChangeFlags(), FormInfo(), cVersion(0) {}
