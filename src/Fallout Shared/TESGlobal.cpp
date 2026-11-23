#include "Fallout Shared/TESGlobal.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

TESGlobal::TESGlobal() : TESForm(), strEditorID(), cType('s'), fValue(0.0f) {
    SetFormType(GLOB_ID);
}

TESGlobal::~TESGlobal() {}

// The GLOB record: the editor ID, the form flags, the bound data, the type character and the value.
bool TESGlobal::Load(TESFile* apFile) {
    // Compares the record's form type in full.
    if (apFile->GetTESForm() != static_cast<uint32_t>(GLOB_ID)) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t iChunk = apFile->GetTESChunk();
        if (!iChunk) {
            break;
        }
        // The switch compares the record code signed.
        switch (static_cast<int32_t>(iChunk)) {
            case 0x44494445: { // 'EDID'
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case 0x444E424F: // 'OBND'
                LoadObjectBound(apFile);
                break;
            case 0x4D414E46: // 'FNAM'
                // the type character only, at +0x20
                apFile->GetChunkData(&cType, 1u);
                break;
            case 0x56544C46: // 'FLTV'
                // the value, which shares +0x24 with the reference ID
                apFile->GetChunkData<uint32_t>(uRefID);
                break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESGlobal::Save() {
    TESForm::StartForm();
    TESForm::AddChunk(FNAM_ID, static_cast<uint8_t>(cType));
    TESForm::AddChunk(FLTV_ID, fValue);
    TESForm::CloseForm();
}

void TESGlobal::Copy(TESForm* apCopy) {
    TESGlobal* pGlobal = dynamic_cast<TESGlobal*>(apCopy);
    if (pGlobal) {
        TESForm::CopyAllComponents(apCopy);
        cType = pGlobal->GetType();
        fValue = pGlobal->TESGlobal::GetValue();
    }
}

bool TESGlobal::Compare(TESForm* apCompare) {
    TESGlobal* pGlobal = dynamic_cast<TESGlobal*>(apCompare);
    if (!pGlobal) {
        return true;
    }
    if (CompareAllComponents(pGlobal)) {
        return true;
    }
    int32_t iType = GetType();
    if (iType != pGlobal->GetType()) {
        return true;
    }
    double dValue = GetValue();
    return dValue != pGlobal->GetValue();
}

// The editor ID, or the engine's empty string without one.
const char* TESGlobal::GetFormEditorID() {
    if (strEditorID.String()) {
        return strEditorID.String();
    }
    return "";
}

bool TESGlobal::SetFormEditorID(const char* apEditorID) {
    strEditorID.Set(apEditorID, 0u);
    return true;
}

char TESGlobal::GetType() {
    return cType;
}

void TESGlobal::SetValue(float afValue) {
    fValue = afValue;
}
