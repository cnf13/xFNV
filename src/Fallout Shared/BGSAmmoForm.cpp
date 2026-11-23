#include "Fallout Shared/BGSAmmoForm.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"

BGSAmmoForm::BGSAmmoForm() {
    BGSAmmoForm::InitializeDataComponent();
}

void BGSAmmoForm::InitializeDataComponent() {
    pFormAmmo = nullptr;
}

void BGSAmmoForm::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSAmmoForm* pCopy = dynamic_cast<BGSAmmoForm*>(apCopyFC);
    if (pCopy) {
        pFormAmmo = pCopy->pFormAmmo;
    }
}

bool BGSAmmoForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSAmmoForm* pCompare = dynamic_cast<BGSAmmoForm*>(apCompareFC);
    return !pCompare || pFormAmmo != pCompare->pFormAmmo;
}

TESAmmo* BGSAmmoForm::GetAmmoHelper() {
    TESAmmo* pAmmoReturn = nullptr;
    if (GetAmmoForm()) {
        return GetAmmoForm();
    }
    if (GetAmmoFormList()) {
        uint32_t uiItemCount = GetAmmoFormList()->iScriptAddedFormCount;
        if (GetAmmoFormList()->ListOfForms.ItemsInList() <= uiItemCount) {
            return static_cast<TESAmmo*>(GetAmmoFormList()->ListOfForms.m_item);
        }

        BSSimpleList<TESForm*>* pList = &GetAmmoFormList()->ListOfForms;
        for (uint32_t i = 0; i < uiItemCount; ++i) {
            pList = pList->m_pkNext;
        }
        return static_cast<TESAmmo*>(pList->m_item);
    }
    return pAmmoReturn;
}

TESAmmo* BGSAmmoForm::GetAmmoForm() {
    if (pFormAmmo && pFormAmmo->GetFormType() == AMMO_ID) {
        return static_cast<TESAmmo*>(pFormAmmo);
    }

    return nullptr;
}

BGSListForm* BGSAmmoForm::GetAmmoFormList() {
    if (pFormAmmo && pFormAmmo->GetFormType() == FLST_ID) {
        return static_cast<BGSListForm*>(pFormAmmo);
    }

    return nullptr;
}

bool BGSAmmoForm::IsRockItLauncher() {
    if (GetAmmoFormList()) {
        TESForm* pFirstForm = GetAmmoFormList()->ListOfForms.GetItem();
        if (pFirstForm && !dynamic_cast<TESAmmo*>(pFirstForm)) {
            return true;
        }
    }
    return false;
}

void BGSAmmoForm::Save() {
    if (pFormAmmo) {
        TESForm::AddChunk(NAM0_ID, pFormAmmo->TESForm::GetFormID());
    }
}

void BGSAmmoForm::InitItem(TESForm* apOwner) {
    if (pFormAmmo) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormAmmo));
        TESForm::AddCompileIndex(uiTempInt, apOwner->GetFile(-1));
        pFormAmmo = TESForm::GetFormByNumericID(uiTempInt);
        if (pFormAmmo && pFormAmmo->GetFormType() != AMMO_ID && pFormAmmo->GetFormType() != FLST_ID) {
            pFormAmmo = nullptr;
        }

        if (!pFormAmmo) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    Warning(
                        "MASTERFILE: Unable to find ammo (%08X) on owner object \"%s\".",
                        uiTempInt,
                        apOwner->GetFormEditorID()
                    );
                }
                else {
                    int32_t iOwnerFormID = apOwner->iFormID;
                    Warning("MASTERFILE: Unable to find ammo (%08X) on owner object (%08X).", uiTempInt, iOwnerFormID);
                }
            }
            else {
                Warning("MASTERFILE: Unable to find ammo (%08X) on unknown owner.", uiTempInt);
            }
        }
    }
}

// The length of the editor ID; bIsReadingName makes a re-entrant call on the same thread (from GetFormEditorID)
// return 0.
uint32_t TESForm::GetFormEditorIDLength() {
    if (bIsReadingName) {
        return 0;
    }
    bIsReadingName = true;
    const char* pEditorID = GetFormEditorID();
    bIsReadingName = false;
    return pEditorID ? BSstrlen(pEditorID) : 0;
}
