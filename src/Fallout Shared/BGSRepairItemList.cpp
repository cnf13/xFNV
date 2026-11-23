#include "Fallout Shared/BGSRepairItemList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "SDK/MSVC/rtti.h"

BGSRepairItemList::BGSRepairItemList() {
    BGSRepairItemList::InitializeDataComponent();
}

BGSRepairItemList::~BGSRepairItemList() {}

void BGSRepairItemList::InitializeDataComponent() {
    pListForm = nullptr;
}

void BGSRepairItemList::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSRepairItemList* pCopy = dynamic_cast<BGSRepairItemList*>(apCopyFC);
    if (pCopy) {
        pListForm = pCopy->pListForm;
    }
}

bool BGSRepairItemList::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSRepairItemList* pCompare = dynamic_cast<BGSRepairItemList*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    return pCompare->pListForm != pListForm;
}

void BGSRepairItemList::Save() {
    if (pListForm) {
        uint32_t uiFormID = pListForm->TESForm::GetFormID();
        TESForm::AddChunk(REPL_ID, uiFormID);
    }
}

void BGSRepairItemList::InitItem(TESForm* apOwner) {
    uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pListForm));
    if (uiTempInt) {
        TESFile* pOwnerFile = apOwner ? apOwner->GetFile(-1) : nullptr;
        TESForm::AddCompileIndex(uiTempInt, pOwnerFile);
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        pListForm = dynamic_cast<BGSListForm*>(pForm);
        if (!pListForm) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pcEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find repair item list (%08X) for owner object \"%s\".",
                        uiTempInt,
                        pcEditorID
                    );
                }
                else {
                    int32_t iFormID = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find repair item list (%08X) for owner object (%08X).",
                        uiTempInt,
                        iFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find repair item list (%08X) for unknown owner.", uiTempInt);
            }
        }
    }
}

bool BGSRepairItemList::HasRepairItem(TESForm* apForm) {
    if (pListForm) {
        BSSimpleList<TESForm*>* pFormList = pListForm->GetFormList();
        return pFormList->IsInList(apForm);
    }
    return false;
}

// The repair list subobject of an armor or a weapon; each cast is null-checked.
BGSRepairItemList* BGSRepairItemList::GetFormAsRepairItemList(TESForm* apForm) {
    BGSRepairItemList* pRetval = nullptr;
    if (apForm) {
        int32_t iFormType = apForm->GetFormType();
        if (iFormType == ARMO_ID) {
            pRetval = static_cast<TESObjectARMO*>(apForm);
        }
        else if (iFormType == WEAP_ID) {
            pRetval = static_cast<TESObjectWEAP*>(apForm);
        }
    }
    return pRetval;
}

bool BGSRepairItemList::CanItemRepairForm(TESForm* apRepairItem, TESForm* apForm) {
    bool bRetval = false;
    if (!apRepairItem || !apForm) {
        return false;
    }
    if (apRepairItem == apForm) {
        return true;
    }
    BGSRepairItemList* pRepairList = BGSRepairItemList::GetFormAsRepairItemList(apForm);
    if (pRepairList) {
        if (pRepairList->HasRepairItem(apRepairItem)) {
            return true;
        }

        float fHasJuryRiggingPerk = 0.0;
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_JURY_RIGGING, pPlayer, &fHasJuryRiggingPerk);
        if (fHasJuryRiggingPerk > 0.0) {
            int32_t iFormType = apForm->GetFormType();
            if (iFormType == apRepairItem->GetFormType() && !apRepairItem->GetQuestObject()) {
                iFormType = apForm->GetFormType();
                if (iFormType == ARMO_ID) {
                    TESObjectARMO* pRepairArmor = static_cast<TESObjectARMO*>(apRepairItem);
                    TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(apForm);
                    if (pRepairArmor->TESBipedModelForm::GetPlayable()) {
                        int32_t iArmorWeight = pRepairArmor->TESObjectARMO::GetArmorWeight();
                        if (iArmorWeight == pArmor->TESObjectARMO::GetArmorWeight() &&
                            pRepairArmor->eEquipType == pArmor->eEquipType) {
                            return true;
                        }
                    }
                }
                else {
                    TESObjectWEAP* pRepairWeapon = static_cast<TESObjectWEAP*>(apRepairItem);
                    TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apForm);
                    if (iFormType == WEAP_ID && pRepairWeapon->TESObjectWEAP::GetIsPlayable()) {
                        ActorValue::Index eWeaponSkill = pRepairWeapon->TESObjectWEAP::GetWeaponSkill();
                        if (eWeaponSkill == pWeapon->TESObjectWEAP::GetWeaponSkill()) {
                            WEAPON_TYPE eWeaponType = pRepairWeapon->TESObjectWEAP::GetWeaponType();
                            if (eWeaponType == pWeapon->TESObjectWEAP::GetWeaponType()) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return bRetval;
}
