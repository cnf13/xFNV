#include "Fallout Shared/TESScriptableForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

TESScriptableForm::TESScriptableForm() : BaseFormComponent() {
    TESScriptableForm::InitializeDataComponent();
}

void TESScriptableForm::InitializeDataComponent() {
    pFormScript = nullptr;
    bIsInitialized = 0;
}

void TESScriptableForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESScriptableForm* pCopy = dynamic_cast<TESScriptableForm*>(apCopyFC);
    if (pCopy) {
        pFormScript = pCopy->pFormScript;
    }
}

bool TESScriptableForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESScriptableForm* pCompare = dynamic_cast<TESScriptableForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    Script* pScript = TESScriptableForm::GetFormScript();
    return pScript != pCompare->TESScriptableForm::GetFormScript();
}

void TESScriptableForm::InitItem(TESForm* apOwner) {
    if (!bIsInitialized) {
        if (pFormScript) {
            uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormScript));
            TESForm::AddCompileIndex(tempint, apOwner->GetFile(-1));
            pFormScript = dynamic_cast<Script*>(TESForm::GetFormByNumericID(tempint));
            if (!pFormScript) {
                if (apOwner) {
                    if (apOwner->GetFormEditorIDLength()) {
                        Warning(
                            "MASTERFILE: Unable to find script (%08X) on owner object \"%s\".",
                            tempint,
                            apOwner->GetFormEditorID()
                        );
                    }
                    else {
                        Warning(
                            "MASTERFILE: Unable to find script (%08X) on owner object (%08X).",
                            tempint,
                            apOwner->iFormID
                        );
                    }
                }
                else {
                    Warning("MASTERFILE: Unable to find script (%08X) on unknown owner.", tempint);
                }
            }
        }
        bIsInitialized = 1;
    }
}

void TESScriptableForm::Save() {
    if (pFormScript) {
        TESForm::AddChunk(SCRI_ID, pFormScript->TESForm::GetFormID());
    }
}
