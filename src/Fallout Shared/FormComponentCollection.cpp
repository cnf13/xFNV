#include "Fallout Shared/FormComponentCollection.h"

#include "BSCore/NewOverloads.h"
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
    pFCArray[7] = dynamic_cast<TESContainer*>(apForm);
    pFCArray[8] = dynamic_cast<TESDescription*>(apForm);
    pFCArray[9] = dynamic_cast<TESEnchantableForm*>(apForm);
    pFCArray[10] = dynamic_cast<TESFullName*>(apForm);
    pFCArray[11] = dynamic_cast<TESHealthForm*>(apForm);
    pFCArray[12] = dynamic_cast<TESTexture*>(apForm);
    pFCArray[13] = dynamic_cast<TESLeveledList*>(apForm);
    pFCArray[14] = dynamic_cast<TESModel*>(apForm);
    pFCArray[15] = dynamic_cast<TESProduceForm*>(apForm);
    pFCArray[16] = dynamic_cast<TESQualityForm*>(apForm);
    pFCArray[17] = dynamic_cast<TESRaceForm*>(apForm);
    pFCArray[18] = dynamic_cast<TESScriptableForm*>(apForm);
    pFCArray[19] = dynamic_cast<TESSpellList*>(apForm);
    pFCArray[20] = dynamic_cast<TESUsesForm*>(apForm);
    pFCArray[21] = dynamic_cast<TESValueForm*>(apForm);
    pFCArray[22] = dynamic_cast<TESWeightForm*>(apForm);
    pFCArray[23] = dynamic_cast<TESSoundFile*>(apForm);
    pFCArray[24] = dynamic_cast<TESReactionForm*>(apForm);
    pFCArray[2] = dynamic_cast<TESAnimation*>(apForm);
    pFCArray[25] = dynamic_cast<TESModelList*>(apForm);
    pFCArray[26] = dynamic_cast<BGSClipRoundsForm*>(apForm);
    pFCArray[27] = dynamic_cast<BGSAmmoForm*>(apForm);
    pFCArray[28] = dynamic_cast<BGSIdleCollection*>(apForm);
    pFCArray[29] = dynamic_cast<BGSDestructibleObjectForm*>(apForm);
    pFCArray[30] = dynamic_cast<BGSRepairItemList*>(apForm);
    pFCArray[31] = dynamic_cast<BGSEquipType*>(apForm);
    pFCArray[32] = dynamic_cast<BGSBipedModelList*>(apForm);
    pFCArray[33] = dynamic_cast<TESImageSpaceModifiableForm*>(apForm);
    pFCArray[34] = dynamic_cast<TESImageSpaceModifiableCountForm<6>*>(apForm);
    pFCArray[35] = dynamic_cast<BGSMessageIcon*>(apForm);
    pFCArray[36] = dynamic_cast<BGSTouchSpellForm*>(apForm);
    pFCArray[37] = dynamic_cast<BGSPickupPutdownSounds*>(apForm);
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
            if (pFCArray[i] && apFCC->pFCArray[i]) {
                pFCArray[i]->CopyComponent(apFCC->pFCArray[i]);
            }
        }
    }
}
