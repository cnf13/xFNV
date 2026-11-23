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
    TESRaceForm* pCopy = dynamic_cast<TESRaceForm*>(apCopyFC);
    if (pCopy) {
        pFormRace = pCopy->pFormRace;
    }
}

bool TESRaceForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESRaceForm* pCompare = dynamic_cast<TESRaceForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    TESRace* pRace = TESRaceForm::GetFormRace();
    return pRace != pCompare->TESRaceForm::GetFormRace();
}

TESRace* TESRaceForm::GetFormRace() const {
    return pFormRace;
}

void TESRaceForm::InitItem(TESForm* apOwner) {
    if (pFormRace) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormRace));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(uiTempInt, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        pFormRace = dynamic_cast<TESRace*>(pForm);
        if (!pFormRace) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    Warning(
                        "MASTERFILE: Unable to find race (%08X) on owner object \"%s\".",
                        uiTempInt,
                        apOwner->GetFormEditorID()
                    );
                }
                else {
                    int32_t iFormID = apOwner->iFormID;
                    Warning("MASTERFILE: Unable to find race (%08X) on owner object (%08X).", uiTempInt, iFormID);
                }
            }
            else {
                Warning("MASTERFILE: Unable to find race (%08X) on unknown owner.", uiTempInt);
            }
        }
    }
}

void TESRaceForm::SetFormRace(TESRace* apRace) {
    pFormRace = apRace;
}

void TESRaceForm::SaveSubRecord() {
    if (pFormRace) {
        uint32_t uiFormID = pFormRace->TESForm::GetFormID();
        TESForm::AddChunk(RNAM_ID, uiFormID);
    }
}
