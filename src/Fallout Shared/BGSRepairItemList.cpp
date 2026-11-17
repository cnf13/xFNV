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
    BGSRepairItemList* pcopy = dynamic_cast<BGSRepairItemList*>(apCopyFC);
    if (pcopy) {
        pListForm = pcopy->pListForm;
    }
}

bool BGSRepairItemList::CompareComponent(BaseFormComponent* apCompareFC) {
    void* v5 = dynamic_cast<BGSRepairItemList*>(apCompareFC);
    if (!v5) {
        return (bool)1;
    }
    BGSRepairItemList* pCompare = static_cast<BGSRepairItemList*>(v5);
    return pCompare->pListForm != pListForm;
}

void BGSRepairItemList::Save() {
    uint32_t v1;
    if (pListForm) {
        v1 = pListForm->TESForm::GetFormID();
        TESForm::AddChunk(static_cast<CHUNK_ID>(1280329042), v1);
    }
}

void BGSRepairItemList::InitItem(TESForm* apOwner) {
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
        if (!pListForm) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* v3 = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find repair item list (%08X) for owner object \"%s\".", itempint, v3
                    );
                }
                else {
                    int32_t CastingType = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find repair item list (%08X) for owner object (%08X).", itempint,
                        CastingType
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find repair item list (%08X) for unknown owner.", itempint);
            }
        }
    }
}

bool BGSRepairItemList::HasRepairItem(TESForm* apForm) {
    bool v4 = 0;
    if (pListForm) {
        BSSimpleList<TESForm*>* FormList = pListForm->GetFormList();
        return FormList->IsInList(apForm);
    }
    return v4;
}

// The repair list subobject of an armor or a weapon; each cast is null-checked.
BGSRepairItemList* BGSRepairItemList::GetFormAsRepairItemList(TESForm* apForm) {
    BGSRepairItemList* pretval = 0;
    if (apForm) {
        int32_t FormType = apForm->GetFormType();
        if (FormType == ARMO_ID) {
            pretval = static_cast<TESObjectARMO*>(apForm);
        }
        else {
            if (FormType == WEAP_ID) {
                pretval = static_cast<TESObjectWEAP*>(apForm);
            }
        }
    }
    return pretval;
}

bool BGSRepairItemList::CanItemRepairForm(TESForm* apRepairItem, TESForm* apForm) {
    float fHasJuryRiggingPerk;
    bool bretval = 0;
    if (!apRepairItem || !apForm) {
        return 0;
    }
    if (apRepairItem == apForm) {
        return 1;
    }
    BGSRepairItemList* prepairlist = BGSRepairItemList::GetFormAsRepairItemList(apForm);
    if (prepairlist) {
        if (prepairlist->HasRepairItem(apRepairItem)) {
            return 1;
        }
        else {
            fHasJuryRiggingPerk = 0.0;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_JURY_RIGGING, pPlayer, &fHasJuryRiggingPerk);
            if (fHasJuryRiggingPerk > 0.0) {
                int32_t FormType = apForm->GetFormType();
                if (FormType == apRepairItem->GetFormType() && !apRepairItem->GetQuestObject()) {
                    int32_t v8 = apForm->GetFormType();
                    if (v8 == ARMO_ID) {
                        TESObjectARMO* pRepairArmor = static_cast<TESObjectARMO*>(apRepairItem);
                        TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(apForm);
                        if (pRepairArmor->TESBipedModelForm::GetPlayable()) {
                            int32_t ArmorWeight = pRepairArmor->TESObjectARMO::GetArmorWeight();
                            if (ArmorWeight == pArmor->TESObjectARMO::GetArmorWeight()) {
                                if (pRepairArmor->eEquipType == pArmor->eEquipType) {
                                    return 1;
                                }
                            }
                        }
                    }
                    else {
                        TESObjectWEAP* pRepairWeapon = static_cast<TESObjectWEAP*>(apRepairItem);
                        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apForm);
                        if (v8 == WEAP_ID && pRepairWeapon->TESObjectWEAP::GetIsPlayable()) {
                            ActorValue::Index WeaponSkill = pRepairWeapon->TESObjectWEAP::GetWeaponSkill();
                            if (WeaponSkill == pWeapon->TESObjectWEAP::GetWeaponSkill()) {
                                WEAPON_TYPE WeaponType = pRepairWeapon->TESObjectWEAP::GetWeaponType();
                                if (WeaponType == pWeapon->TESObjectWEAP::GetWeaponType()) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return bretval;
}
