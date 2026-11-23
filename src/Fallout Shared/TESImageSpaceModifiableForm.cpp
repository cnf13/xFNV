#include "Fallout Shared/TESImageSpaceModifiableForm.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "SDK/MSVC/rtti.h"

void TESImageSpaceModifiableForm::InitializeDataComponent() {
    pFormImageSpaceModifying = nullptr;
}

TESImageSpaceModifiableForm::TESImageSpaceModifiableForm() : BaseFormComponent() {
    TESImageSpaceModifiableForm::InitializeDataComponent();
}

void TESImageSpaceModifiableForm::Save() {
    if (pFormImageSpaceModifying) {
        TESForm::AddChunk(MNAM_ID, pFormImageSpaceModifying->iFormID);
    }
}

void TESImageSpaceModifiableForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESImageSpaceModifiableForm* pCopy = dynamic_cast<TESImageSpaceModifiableForm*>(apCopyFC);
    if (pCopy) {
        SetFormImageSpaceModifying(pCopy->pFormImageSpaceModifying);
    }
}

bool TESImageSpaceModifiableForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESImageSpaceModifiableForm* pCompare = dynamic_cast<TESImageSpaceModifiableForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    return pFormImageSpaceModifying != pCompare->pFormImageSpaceModifying;
}

void TESImageSpaceModifiableForm::InitItem(TESForm* apOwner) {
    if (pFormImageSpaceModifying) {
        uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormImageSpaceModifying));
        if (apOwner) {
            TESForm::AddCompileIndex(uiFormID, apOwner->GetFile(-1));
        }
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        pFormImageSpaceModifying = dynamic_cast<TESImageSpaceModifier*>(pForm);
        if (!pFormImageSpaceModifying) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    Warning(
                        "MASTERFILE: Unable to find imagespacemodifier (%08X) on owner object \"%s\".",
                        uiFormID,
                        apOwner->GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find imagespacemodifier (%08X) on owner object (%08X).",
                        uiFormID,
                        apOwner->iFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find imagespacemodifier (%08X) on unknown owner.", uiFormID);
            }
        }
    }
}

void TESImageSpaceModifiableForm::SetFormImageSpaceModifying(TESImageSpaceModifier* apMod) {
    pFormImageSpaceModifying = apMod;
}
// The generic InitItem and Save. They are defined here, not in the header, which cannot include TESForm.h and TESFile.h
// (TESWeather.h includes it before those are complete); N = 6 is the one instantiation.
template <uint32_t N>
void TESImageSpaceModifiableCountForm<N>::InitItem(TESForm* apOwner) {
    for (uint32_t i = 0; i < N; ++i) {
        if (pFormImageSpaceModifying[i]) {
            // Until InitItem runs, each slot holds the form ID Load read.
            uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormImageSpaceModifying[i]));
            if (apOwner) {
                TESForm::AddCompileIndex(uiFormID, apOwner->GetFile(-1));
            }
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            pFormImageSpaceModifying[i] = dynamic_cast<TESImageSpaceModifier*>(pForm);
            if (!pFormImageSpaceModifying[i]) {
                if (apOwner) {
                    if (apOwner->GetFormEditorIDLength()) {
                        Warning(
                            "MASTERFILE: Unable to find imagespacemodifier (%08X) on owner object \"%s\".",
                            uiFormID,
                            apOwner->GetFormEditorID()
                        );
                    }
                    else {
                        Warning(
                            "MASTERFILE: Unable to find imagespacemodifier (%08X) on owner object (%08X).",
                            uiFormID,
                            apOwner->iFormID
                        );
                    }
                }
                else {
                    Warning("MASTERFILE: Unable to find imagespacemodifier (%08X) on unknown owner.", uiFormID);
                }
            }
        }
    }
}

template <uint32_t N>
void TESImageSpaceModifiableCountForm<N>::Save() {
    for (uint32_t i = 0; i < N; ++i) {
        if (pFormImageSpaceModifying[i]) {
            // The per-slot chunk codes run from '\0IAD' up by one each.
            TESForm::AddChunk(static_cast<CHUNK_ID>(SIAD_ID + i), pFormImageSpaceModifying[i]->iFormID);
        }
    }
}

template void TESImageSpaceModifiableCountForm<6>::InitItem(TESForm* apOwner);
template void TESImageSpaceModifiableCountForm<6>::Save();
