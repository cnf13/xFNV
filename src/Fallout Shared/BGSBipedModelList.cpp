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
    BGSBipedModelList* pCopy = dynamic_cast<BGSBipedModelList*>(apCopyFC);
    if (pCopy) {
        pListForm = pCopy->pListForm;
    }
}

bool BGSBipedModelList::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSBipedModelList* pComp = dynamic_cast<BGSBipedModelList*>(apCompareFC);
    if (!pComp) {
        return true;
    }
    return pComp->pListForm != pListForm;
}

BGSListForm* BGSBipedModelList::GetBipedModelList() {
    return pListForm;
}

void BGSBipedModelList::Save() {
    if (pListForm) {
        TESForm::AddChunk(BIPL_ID, pListForm->TESForm::GetFormID());
    }
}

void BGSBipedModelList::SetBipedModelList(BGSListForm* apList) {
    pListForm = apList;
}

void BGSBipedModelList::InitItem(TESForm* apOwner) {
    uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pListForm));
    if (uiTempInt) {
        TESFile* pOwnerFile = apOwner ? apOwner->GetFile(-1) : nullptr;
        TESForm::AddCompileIndex(uiTempInt, pOwnerFile);
        pListForm = dynamic_cast<BGSListForm*>(TESForm::GetFormByNumericID(uiTempInt));
        if (pListForm) {
            for (BSSimpleList<TESForm*>* pList = &pListForm->ListOfForms; pList && !pList->IsEmpty();
                 pList = pList->m_pkNext) {
                TESForm* pForm = pList->m_item;
                TESBipedModelForm* pBip = TESBipedModelForm::GetFormAsBipedModel(pForm);
                if (!pBip) {
                    int32_t iOwnerID = apOwner->iFormID;
                    const char* pOwnerEditorID = apOwner->GetFormEditorID();
                    int32_t iListID = pListForm->iFormID;
                    const char* pListEditorID = pListForm->GetFormEditorID();
                    int32_t iEntryID = pForm->iFormID;
                    Warning(
                        "MASTERFILE: Form '%s' (%08X) in form list '%s' (%08X) is not a biped model. "
                        "BipedModelList on '%s' (%08X) will be cleared.",
                        pForm->GetFormEditorID(),
                        iEntryID,
                        pListEditorID,
                        iListID,
                        pOwnerEditorID,
                        iOwnerID
                    );
                    pListForm = nullptr;
                    return;
                }
                pBip->SetAddOnModel(true);
            }
        }
        else if (apOwner) {
            if (apOwner->GetFormEditorIDLength()) {
                Warning(
                    "MASTERFILE: Unable to find biped model list (%08X) for owner object \"%s\".",
                    uiTempInt,
                    apOwner->GetFormEditorID()
                );
            }
            else {
                int32_t iOwnerID = apOwner->iFormID;
                Warning(
                    "MASTERFILE: Unable to find biped model list (%08X) for owner object (%08X).",
                    uiTempInt,
                    iOwnerID
                );
            }
        }
        else {
            Warning("MASTERFILE: Unable to find biped model list (%08X) for unknown owner.", uiTempInt);
        }
    }
}

// The form's biped model list component: armor and weapons have one, other forms none.
BGSBipedModelList* BGSBipedModelList::GetFormAsBipedModelList(TESForm* apForm) {
    BGSBipedModelList* pRetVal = nullptr;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case ARMO_ID: pRetVal = static_cast<TESObjectARMO*>(apForm); break;
            case WEAP_ID: pRetVal = static_cast<TESObjectWEAP*>(apForm); break;
        }
    }
    return pRetVal;
}
