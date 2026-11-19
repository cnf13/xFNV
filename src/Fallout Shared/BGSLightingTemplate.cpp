#include "Fallout Shared/BGSLightingTemplate.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

BGSLightingTemplate::BGSLightingTemplate() {
    SetFormType(101);
    BGSLightingTemplate::InitializeData();
}

BGSLightingTemplate::~BGSLightingTemplate() {}

void BGSLightingTemplate::InitializeData() {
    ourMemset(&Data, 0, sizeof(Data));
}

// Reads the lighting template's record: the lighting data, the editor ID and the object bound.
bool BGSLightingTemplate::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != LGTM_ID) {
        return false;
    }
    TESForm::LoadForm(apFile);
    for (;;) {
        CHUNK_ID eTESChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eTESChunk == NO_CHUNK) {
            break;
        }
        if (eTESChunk == DATA_ID) {
            // LoadData copies the chunk's bytes to the address it is given, so the Data member's address is passed as
            // untyped memory.
            TESForm::LoadData(apFile, reinterpret_cast<char*>(&Data), 0x2C);
        }
        else if (eTESChunk == EDID_ID) {
            // Reserves the chunk's size on the stack and reads up to 0x200 bytes into it.
            char* pcEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
            apFile->GetChunkData(pcEditorID, 0x200);
            static_cast<TESForm*>(this)->SetFormEditorID(pcEditorID);
        }
        else if (eTESChunk == OBND_ID) {
            static_cast<TESForm*>(this)->LoadObjectBound(apFile);
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    if (apFile->QEndian()) {
        Data.Endian();
    }
    return true;
}

void BGSLightingTemplate::Save() {
    TESForm::StartForm();
    if (TESForm::QEndianSwapOnSave()) {
        (&Data)->Endian();
    }
    TESForm::__SaveData(&Data, 44);
    if (TESForm::QEndianSwapOnSave()) {
        (&Data)->Endian();
    }
    TESForm::CloseForm();
}

void BGSLightingTemplate::InitItem() {
    if (!TESForm::GetInitialized()) {
        TESForm::InitializeDataComponent();
        TESForm::SetInitialized(true);
    }
}

void BGSLightingTemplate::Copy(TESForm* apCopy) {
    BGSLightingTemplate* v3 = dynamic_cast<BGSLightingTemplate*>(apCopy);
    if (v3) {
        ourMemcpy(&Data, &v3->Data, sizeof(Data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool BGSLightingTemplate::Compare(TESForm* apCompare) {
    TESForm* v4 = dynamic_cast<BGSLightingTemplate*>(apCompare);
    if (!v4) {
        return true;
    }
    if (!std::memcmp(&Data, &v4[1], sizeof(Data))) {
        return CompareAllComponents(v4);
    }
    return true;
}

float BGSLightingTemplate::GetDataFogFarPlane() {
    return Data.fFogFar;
}

float BGSLightingTemplate::GetDataClipDist() {
    return Data.fClipDist;
}

float BGSLightingTemplate::GetDataFogPower() {
    return Data.fFogPower;
}

// Swaps every 4-byte field of a record read from a big-endian file, in field order; the interior offset is left
// alone.
void INTERIOR_DATA::Endian() {
    ENDIAN(iAmbient, false);
    ENDIAN(iDirectional, false);
    ENDIAN(iFog, false);
    ENDIAN(fFogNear, false);
    ENDIAN(fFogFar, false);
    ENDIAN(iDirectionalXY, false);
    ENDIAN(iDirectionalZ, false);
    ENDIAN(fDirectionalFade, false);
    ENDIAN(fClipDist, false);
    ENDIAN(fFogPower, false);
}
