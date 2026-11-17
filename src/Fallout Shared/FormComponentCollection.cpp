#include "Fallout Shared/FormComponentCollection.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSIdleCollection.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimation.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelList.h"
#include "Fallout Shared/TESProduceForm.h"
#include "Fallout Shared/TESQualityForm.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSoundFile.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESUsesForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "SDK/MSVC/rtti.h"

FormComponentCollection::FormComponentCollection() {
    ClearFormComponentArray();
}

void FormComponentCollection::Destruct() {}

void FormComponentCollection::InitFormComponentArray(TESForm* apForm) {
    ClearFormComponentArray();
    pFCArray[0] = apForm;
    pFCArray[1] = dynamic_cast<TESAIForm*>(apForm);
    pFCArray[3] = dynamic_cast<TESActorBaseData*>(apForm);
    pFCArray[4] = dynamic_cast<TESAttackDamageForm*>(apForm);
    pFCArray[5] = dynamic_cast<TESAttributes*>(apForm);
    pFCArray[6] = dynamic_cast<TESBipedModelForm*>(apForm);
    pFCArray[7] =
        static_cast<BaseFormComponent*>(FNVD_DYNAMIC_CAST_VOID(TESContainer)(apForm, RTTI_TESForm, RTTI_TESContainer));
    pFCArray[8] = dynamic_cast<TESDescription*>(apForm);
    pFCArray[9] = dynamic_cast<TESEnchantableForm*>(apForm);
    pFCArray[10] = dynamic_cast<TESFullName*>(apForm);
    pFCArray[11] = dynamic_cast<TESHealthForm*>(apForm);
    pFCArray[12] = dynamic_cast<TESTexture*>(apForm);
    pFCArray[13] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(TESLeveledList)(apForm, RTTI_TESForm, RTTI_TESLeveledList)
    );
    pFCArray[14] = dynamic_cast<TESModel*>(apForm);
    pFCArray[15] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(TESProduceForm)(apForm, RTTI_TESForm, RTTI_TESProduceForm)
    );
    pFCArray[16] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(TESQualityForm)(apForm, RTTI_TESForm, RTTI_TESQualityForm)
    );
    pFCArray[17] = dynamic_cast<TESRaceForm*>(apForm);
    pFCArray[18] = dynamic_cast<TESScriptableForm*>(apForm);
    pFCArray[19] =
        static_cast<BaseFormComponent*>(FNVD_DYNAMIC_CAST_VOID(TESSpellList)(apForm, RTTI_TESForm, RTTI_TESSpellList));
    pFCArray[20] =
        static_cast<BaseFormComponent*>(FNVD_DYNAMIC_CAST_VOID(TESUsesForm)(apForm, RTTI_TESForm, RTTI_TESUsesForm));
    pFCArray[21] = dynamic_cast<TESValueForm*>(apForm);
    pFCArray[22] = dynamic_cast<TESWeightForm*>(apForm);
    pFCArray[23] = dynamic_cast<TESSoundFile*>(apForm);
    pFCArray[24] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(TESReactionForm)(apForm, RTTI_TESForm, RTTI_TESReactionForm)
    );
    pFCArray[2] = dynamic_cast<TESAnimation*>(apForm);
    pFCArray[25] = dynamic_cast<TESModelList*>(apForm);
    pFCArray[26] = dynamic_cast<BGSClipRoundsForm*>(apForm);
    pFCArray[27] = dynamic_cast<BGSAmmoForm*>(apForm);
    pFCArray[28] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(BGSIdleCollection)(apForm, RTTI_TESForm, RTTI_BGSIdleCollection)
    );
    pFCArray[29] = dynamic_cast<BGSDestructibleObjectForm*>(apForm);
    pFCArray[30] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(BGSRepairItemList)(apForm, RTTI_TESForm, RTTI_BGSRepairItemList)
    );
    pFCArray[31] =
        static_cast<BaseFormComponent*>(FNVD_DYNAMIC_CAST_VOID(BGSEquipType)(apForm, RTTI_TESForm, RTTI_BGSEquipType));
    pFCArray[32] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(BGSBipedModelList)(apForm, RTTI_TESForm, RTTI_BGSBipedModelList)
    );
    pFCArray[33] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(TESImageSpaceModifiableForm)(apForm, RTTI_TESForm, RTTI_TESImageSpaceModifiableForm)
    );
    pFCArray[34] = dynamic_cast<TESImageSpaceModifiableCountForm<6>*>(apForm);
    pFCArray[35] = dynamic_cast<BGSMessageIcon*>(apForm);
    pFCArray[36] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(BGSTouchSpellForm)(apForm, RTTI_TESForm, RTTI_BGSTouchSpellForm)
    );
    pFCArray[37] = static_cast<BaseFormComponent*>(
        FNVD_DYNAMIC_CAST_VOID(BGSPickupPutdownSounds)(apForm, RTTI_TESForm, RTTI_BGSPickupPutdownSounds)
    );
}

void FormComponentCollection::InitializeDataAllComponents() {
    int32_t i;
    for (i = 0; i < 38; ++i) {
        if (pFCArray[i]) {
            pFCArray[i]->InitializeDataComponent();
        }
    }
}

void FormComponentCollection::ClearDataAllComponents() {
    int32_t i;
    for (i = 0; i < 38; ++i) {
        if (pFCArray[i]) {
            pFCArray[i]->ClearDataComponent();
        }
    }
}

bool FormComponentCollection::CompareAllComponents(FormComponentCollection* apFCC) {
    int32_t i;
    if (!apFCC) {
        return true;
    }
    for (i = 0; i < 38; ++i) {
        if (pFCArray[i]) {
            if (pFCArray[i]->CompareComponent(apFCC->pFCArray[i])) {
                return true;
            }
        }
        else {
            if (apFCC->pFCArray[i]) {
                return true;
            }
        }
    }
    return false;
}

void FormComponentCollection::ClearFormComponentArray() {
    ourMemset(this, 0, sizeof(FormComponentCollection));
}

void FormComponentCollection::CopyAllComponents(FormComponentCollection* apFCC) {
    int32_t i;
    if (apFCC) {
        for (i = 0; i < 38; ++i) {
            if ((pFCArray[i]) && (apFCC->pFCArray[i])) {
                pFCArray[i]->CopyComponent(apFCC->pFCArray[i]);
            }
        }
    }
}
