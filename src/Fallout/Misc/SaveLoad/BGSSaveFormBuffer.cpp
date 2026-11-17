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
    Actor* v2 = static_cast<Actor*>(GetForm());
    if (v2 && v2->IsReference()) {
        return v2;
    }
    else {
        return 0;
    }
}

Actor* BGSSaveFormBuffer::GetActor() {
    Actor* v2 = static_cast<Actor*>(GetReference());
    if (v2 && v2->IsActor()) {
        return v2;
    }
    else {
        return nullptr;
    }
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
    (&Header.FormIDIndex)->SetNumericID(aiFormID);
    Header.iChangeFlags = aiChangeFlags;
    (&Header.FormInfo)->SetFormType(aeFormType);
    Header.cVersion = acVersion;
}

uint32_t BGSSaveFormBuffer::GetOverhead() {
    uint32_t v1 = BGSSaveGameBuffer::GetBufferPosition();
    SIZE_TYPE v2 = GetSizeTypeForSize(v1);
    return (uint32_t)((int32_t)(((uint32_t)GetSizeTypeSize(v2)) + 9));
}

void BGSSaveFormBuffer::Save(BGSSaveLoadFile* apFile) {
    uint32_t aiSize = iBufferPosition;
    SIZE_TYPE a2 = GetSizeTypeForSize(aiSize);
    Header.FormInfo.SetSizeType(a2);
    apFile->Write(&Header, 9u);
    SaveGameSizeToFile(apFile, aiSize, a2);
    BGSSaveGameBuffer::Save(apFile, aiSize);
}

void BGSSaveFormBuffer::SaveMatrixAsQuaternion(NiMatrix3* a2, int32_t a3, int32_t a4) {
    NiQuaternion ecx0;
    ecx0;
    (&ecx0)->FromRotation(*a2);
    BGSSaveGameBuffer* v6 = this;
    BGSSaveGameBuffer::SaveDataEndian(static_cast<char*>(static_cast<void*>(&ecx0)), 0x10u, 0);
}
