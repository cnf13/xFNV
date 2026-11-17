#include "Fallout Shared/TESRaceForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESRace.h"
#include "SDK/MSVC/rtti.h"

TESRaceForm::TESRaceForm() {
    TESRaceForm::InitializeDataComponent();
}

void TESRaceForm::InitializeDataComponent() {
    pFormRace = nullptr;
}

void TESRaceForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESRaceForm* pcopy = dynamic_cast<TESRaceForm*>(apCopyFC);
    if (pcopy) {
        pFormRace = pcopy->pFormRace;
    }
}

bool TESRaceForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESRaceForm* v5 = dynamic_cast<TESRaceForm*>(apCompareFC);
    if (!v5) {
        return true;
    }
    TESRace* v3 = TESRaceForm::GetFormRace();
    return (bool)((uint8_t)(v3 != v5->TESRaceForm::GetFormRace()));
}

TESRace* TESRaceForm::GetFormRace() const {
    return pFormRace;
}

void TESRaceForm::InitItem(TESForm* apOwner) {
    if (pFormRace) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormRace));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
        pFormRace = dynamic_cast<TESRace*>(FormByNumericID);
        if (!pFormRace) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* v4 = apOwner->GetFormEditorID();
                    Warning("MASTERFILE: Unable to find race (%08X) on owner object \"%s\".", itempint, v4);
                }
                else {
                    int32_t CastingType = apOwner->iFormID;
                    Warning("MASTERFILE: Unable to find race (%08X) on owner object (%08X).", itempint, CastingType);
                }
            }
            else {
                Warning("MASTERFILE: Unable to find race (%08X) on unknown owner.", itempint);
            }
        }
    }
}

void TESRaceForm::SetFormRace(TESRace* apRace) {
    pFormRace = apRace;
}

void TESRaceForm::SaveSubRecord() {
    uint32_t v1;
    if (pFormRace) {
        v1 = pFormRace->TESForm::GetFormID();
        TESForm::AddChunk(RNAM_ID, v1);
    }
}
