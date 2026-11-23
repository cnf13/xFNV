#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

BGSSaveFormBuffer::BGSSaveFormBuffer() : pForm(nullptr) {}

BGSSaveFormBuffer::~BGSSaveFormBuffer() {}

TESObjectREFR* BGSSaveFormBuffer::GetReference() {
    TESForm* pForm = GetForm();
    if (pForm && pForm->IsReference()) {
        return static_cast<TESObjectREFR*>(pForm);
    }

    return nullptr;
}

Actor* BGSSaveFormBuffer::GetActor() {
    TESObjectREFR* pRef = GetReference();
    if (pRef && pRef->IsActor()) {
        return static_cast<Actor*>(pRef);
    }

    return nullptr;
}

ENUM_FORM_ID BGSSaveFormBuffer::GetFormType() {
    return Header.FormInfo.GetFormType();
}

void BGSSaveFormBuffer::SetHeader(
    uint32_t aiFormID,
    BGSChangeFlags aiChangeFlags,
    ENUM_FORM_ID aeFormType,
    uint8_t acVersion
) {
    Header.FormIDIndex.SetNumericID(aiFormID);
    Header.iChangeFlags = aiChangeFlags;
    Header.FormInfo.SetFormType(aeFormType);
    Header.cVersion = acVersion;
}

uint32_t BGSSaveFormBuffer::GetOverhead() {
    uint32_t uiSize = BGSSaveGameBuffer::GetBufferPosition();
    SIZE_TYPE eSizeType = GetSizeTypeForSize(uiSize);
    return GetSizeTypeSize(eSizeType) + 9;
}

void BGSSaveFormBuffer::Save(BGSSaveLoadFile* apFile) {
    uint32_t uiSize = iBufferPosition;
    SIZE_TYPE eSizeType = GetSizeTypeForSize(uiSize);
    Header.FormInfo.SetSizeType(eSizeType);
    apFile->Write(&Header, 9u);
    SaveGameSizeToFile(apFile, uiSize, eSizeType);
    BGSSaveGameBuffer::Save(apFile, uiSize);
}

void BGSSaveFormBuffer::SaveMatrixAsQuaternion(NiMatrix3* apMatrix, int32_t aiSize, int32_t aiUnusuedMetaDataArg) {
    NiQuaternion kQuat;
    kQuat.FromRotation(*apMatrix);
    BGSSaveGameBuffer::SaveDataEndian(&kQuat, sizeof(kQuat), 0);
}
