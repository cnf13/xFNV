#include "Fallout Shared/BGSBipedModelList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "SDK/MSVC/rtti.h"

BGSBipedModelList::BGSBipedModelList() {
    BGSBipedModelList::InitializeDataComponent();
}

BGSBipedModelList::~BGSBipedModelList() {}

void BGSBipedModelList::InitializeDataComponent() {
    pListForm = nullptr;
}

void BGSBipedModelList::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSBipedModelList* pcopy = dynamic_cast<BGSBipedModelList*>(apCopyFC);
    if (pcopy) {
        pListForm = pcopy->pListForm;
    }
}

bool BGSBipedModelList::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSBipedModelList* pcomp = dynamic_cast<BGSBipedModelList*>(apCompareFC);
    if (!pcomp) {
        return 1;
    }
    BGSListForm* BipedModelList = pcomp->pListForm;
    return BipedModelList != pListForm;
}

BGSListForm* BGSBipedModelList::GetBipedModelList() {
    return pListForm;
}

void BGSBipedModelList::Save() {
    uint32_t v1;
    if (pListForm) {
        v1 = pListForm->TESForm::GetFormID();
        TESForm::AddChunk(static_cast<CHUNK_ID>(1280330050), v1);
    }
}

void BGSBipedModelList::SetBipedModelList(BGSListForm* apList) {
    pListForm = apList;
}

void BGSBipedModelList::InitItem(TESForm* apOwner) {
    uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pListForm));
    if (itempint) {
        TESFile* pownerFile;
        if (apOwner) {
            pownerFile = apOwner->GetFile(-1);
        }
        else {
            pownerFile = 0;
        }
        TESForm::AddCompileIndex(itempint, pownerFile);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
        pListForm = dynamic_cast<BGSListForm*>(FormByNumericID);
        if (pListForm) {
            for (BSSimpleList<TESForm*>* plist = &pListForm->ListOfForms; plist && !plist->IsEmpty();
                 plist = plist->m_pkNext) {
                TESForm* pform = plist->m_item;
                TESBipedModelForm* pbip = TESBipedModelForm::GetFormAsBipedModel(pform);
                if (!pbip) {
                    int32_t CastingType = apOwner->iFormID;
                    const char* v6 = apOwner->GetFormEditorID();
                    int32_t v5 = pListForm->iFormID;
                    const char* v4 = pListForm->GetFormEditorID();
                    int32_t v3 = pform->iFormID;
                    Warning(
                        "MASTERFILE: Form '%s' (%08X) in form list '%s' (%08X) is not a biped model. "
                        "BipedModelList on '%s' (%08X) will be cleared.",
                        pform->GetFormEditorID(), v3, v4, v5, v6, CastingType
                    );
                    pListForm = 0;
                    return;
                }
                pbip->SetAddOnModel(1);
            }
        }
        else {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* v7 = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find biped model list (%08X) for owner object \"%s\".", itempint, v7
                    );
                }
                else {
                    int32_t v8 = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find biped model list (%08X) for owner object (%08X).", itempint, v8
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find biped model list (%08X) for unknown owner.", itempint);
            }
        }
    }
}

// The form's biped model list component: armor and weapons have one, other forms none.
BGSBipedModelList* BGSBipedModelList::GetFormAsBipedModelList(TESForm* apForm) {
    BGSBipedModelList* pretval = 0;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case ARMO_ID: pretval = static_cast<TESObjectARMO*>(apForm); break;
            case WEAP_ID: pretval = static_cast<TESObjectWEAP*>(apForm); break;
        }
    }
    return pretval;
}
