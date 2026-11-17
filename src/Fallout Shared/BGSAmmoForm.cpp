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
    BGSAmmoForm* v3 = dynamic_cast<BGSAmmoForm*>(apCopyFC);
    if (v3) {
        pFormAmmo = v3->pFormAmmo;
    }
}

bool BGSAmmoForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSAmmoForm* v4 = dynamic_cast<BGSAmmoForm*>(apCompareFC);
    return (bool)((uint8_t)((!v4) || (pFormAmmo != v4->pFormAmmo)));
}

TESAmmo* BGSAmmoForm::GetAmmoHelper() {
    TESAmmo* pAmmoReturn = 0;
    if (GetAmmoForm()) {
        return GetAmmoForm();
    }
    if (GetAmmoFormList()) {
        uint32_t uiItemCount = GetAmmoFormList()->iScriptAddedFormCount;
        if ((&GetAmmoFormList()->ListOfForms)->ItemsInList() <= uiItemCount) {
            return static_cast<TESAmmo*>((&GetAmmoFormList()->ListOfForms)->m_item);
        }
        else {
            BSSimpleList<TESForm*>* ecx0 = &GetAmmoFormList()->ListOfForms;
            for (uint32_t i = 0; i < uiItemCount; ++i) {
                ecx0 = ecx0->m_pkNext;
            }
            return static_cast<TESAmmo*>(ecx0->m_item);
        }
    }
    return pAmmoReturn;
}

TESAmmo* BGSAmmoForm::GetAmmoForm() {
    if (pFormAmmo && (((int32_t)pFormAmmo->GetFormType()) == 41)) {
        return static_cast<TESAmmo*>(pFormAmmo);
    }
    else {
        return nullptr;
    }
}

BGSListForm* BGSAmmoForm::GetAmmoFormList() {
    if (pFormAmmo && (((int32_t)pFormAmmo->GetFormType()) == 85)) {
        return static_cast<BGSListForm*>(pFormAmmo);
    }
    else {
        return nullptr;
    }
}

bool BGSAmmoForm::IsRockItLauncher() {
    bool result = false;
    if (GetAmmoFormList()) {
        BGSListForm* AmmoFormList = GetAmmoFormList();
        TESForm* pFirstForm = AmmoFormList->ListOfForms.GetItem();
        if (pFirstForm) {
            if (!dynamic_cast<TESAmmo*>(pFirstForm)) {
                return true;
            }
        }
    }
    return result;
}

void BGSAmmoForm::Save() {
    uint32_t v1;
    if (pFormAmmo) {
        v1 = pFormAmmo->TESForm::GetFormID();
        TESForm::AddChunk(static_cast<CHUNK_ID>(810369358), v1);
    }
}

void BGSAmmoForm::InitItem(TESForm* apOwner) {
    if (pFormAmmo) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormAmmo));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, File);
        pFormAmmo = TESForm::GetFormByNumericID(itempint);
        if (pFormAmmo) {
            if (pFormAmmo->GetFormType() != AMMO_ID && pFormAmmo->GetFormType() != FLST_ID) {
                pFormAmmo = 0;
            }
        }
        if (!pFormAmmo) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* v3 = apOwner->GetFormEditorID();
                    Warning("MASTERFILE: Unable to find ammo (%08X) on owner object \"%s\".", itempint, v3);
                }
                else {
                    int32_t CastingType = apOwner->iFormID;
                    Warning("MASTERFILE: Unable to find ammo (%08X) on owner object (%08X).", itempint, CastingType);
                }
            }
            else {
                Warning("MASTERFILE: Unable to find ammo (%08X) on unknown owner.", itempint);
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
