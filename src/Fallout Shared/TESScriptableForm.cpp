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
    TESScriptableForm* v5 = dynamic_cast<TESScriptableForm*>(apCompareFC);
    if (!v5) {
        return true;
    }
    Script* v3 = TESScriptableForm::GetFormScript();
    return (bool)((uint8_t)(v3 != v5->TESScriptableForm::GetFormScript()));
}

void TESScriptableForm::InitItem(TESForm* apOwner) {
    if (!bIsInitialized) {
        if (pFormScript) {
            uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormScript));
            TESFile* File = apOwner->GetFile(-1);
            TESForm::AddCompileIndex(tempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
            pFormScript = dynamic_cast<Script*>(FormByNumericID);
            if (!pFormScript) {
                if (apOwner) {
                    if (apOwner->GetFormEditorIDLength()) {
                        const char* v4 = apOwner->GetFormEditorID();
                        Warning("MASTERFILE: Unable to find script (%08X) on owner object \"%s\".", tempint, v4);
                    }
                    else {
                        int32_t CastingType = apOwner->iFormID;
                        Warning(
                            "MASTERFILE: Unable to find script (%08X) on owner object (%08X).", tempint, CastingType
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
    uint32_t v1;
    if (pFormScript) {
        v1 = pFormScript->TESForm::GetFormID();
        TESForm::AddChunk(static_cast<CHUNK_ID>(1230127955), v1);
    }
}
