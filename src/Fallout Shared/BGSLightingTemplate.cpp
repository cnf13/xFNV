#include "Fallout Shared/BGSLightingTemplate.h"

#include "BSCore/NewOverloads.h"
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
            TESForm::LoadData(apFile, reinterpret_cast<char*>(&Data), sizeof(Data));
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
        Data.Endian();
    }
    TESForm::__SaveData(&Data, sizeof(Data));
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
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
    BGSLightingTemplate* pCopy = dynamic_cast<BGSLightingTemplate*>(apCopy);
    if (pCopy) {
        ourMemcpy(&Data, &pCopy->Data, sizeof(Data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool BGSLightingTemplate::Compare(TESForm* apCompare) {
    BGSLightingTemplate* pCompare = dynamic_cast<BGSLightingTemplate*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (!std::memcmp(&Data, &pCompare->Data, sizeof(Data))) {
        return CompareAllComponents(pCompare);
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
    BSEndian(iAmbient, false);
    BSEndian(iDirectional, false);
    BSEndian(iFog, false);
    BSEndian(fFogNear, false);
    BSEndian(fFogFar, false);
    BSEndian(iDirectionalXY, false);
    BSEndian(iDirectionalZ, false);
    BSEndian(fDirectionalFade, false);
    BSEndian(fClipDist, false);
    BSEndian(fFogPower, false);
}
