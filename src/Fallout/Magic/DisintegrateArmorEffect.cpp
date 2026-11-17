#include "Fallout/Magic/DisintegrateArmorEffect.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"

// Whether the disintegrate-armor archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::DISINTEGRATE_ARMOR, &DisintegrateArmorEffect::Instantiate);

DisintegrateArmorEffect::DisintegrateArmorEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect), pArmor(nullptr) {}

DisintegrateArmorEffect::~DisintegrateArmorEffect() {
    delete pArmor;
}

ActiveEffect* DisintegrateArmorEffect::Copy() {
    DisintegrateArmorEffect* v2;
    void* v4 = ourMalloc(sizeof(DisintegrateArmorEffect));
    if (v4) {
        v2 = ::new (v4) DisintegrateArmorEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    ActiveEffect::CopyData(v2);
    return v2;
}

// Save version 0x37 added 4 bytes.
uint16_t DisintegrateArmorEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = ActiveEffect::GetSaveSize(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        usSize += 4;
    }
    return usSize;
}

void DisintegrateArmorEffect::SaveGame(Actor* a1) {
    BIPED_OBJECT earmorPosition;
    ActiveEffect::SaveGame(a1);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 55u) {
        earmorPosition = BO_NONE;
        if (pArmor) {
            if (pArmor->pContainerObj) {
                TESObjectARMO* parmor = dynamic_cast<TESObjectARMO*>(pArmor->pContainerObj);
                if (parmor) {
                    // The slots are counted as an integer and passed as FillsBipedSlot's BIPED_OBJECT.
                    for (uint32_t ibiped = 0; ibiped < 20; ++ibiped) {
                        if (parmor->FillsBipedSlot(static_cast<BIPED_OBJECT>(ibiped), 0, 0)) {
                            earmorPosition = static_cast<BIPED_OBJECT>(ibiped);
                            break;
                        }
                    }
                }
            }
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&earmorPosition, 4u);
    }
}

void DisintegrateArmorEffect::LoadGame(Actor* a1) {
    BIPED_OBJECT earmorPosition;
    ActiveEffect::LoadGame(a1);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 55u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&earmorPosition, 4u);
        if (earmorPosition != BO_NONE) {
            InventoryChanges* pinventory = (&a1->m_Extra)->GetContainerChanges();
            if (pinventory) {
                this->pArmor = pinventory->GetWornItem(earmorPosition, 1);
            }
            ItemChange* apThis = this->pArmor;
            if (apThis) {
                ItemChange* pArmor = this->pArmor;
                delete apThis;
            }
        }
    }
}

bool DisintegrateArmorEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor;
    if (apTarget) {
        pTargetActor = apTarget->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    return pTargetActor != nullptr;
}

void DisintegrateArmorEffect::Start() {
    Actor* pactor;
    if (pTarget) {
        pactor = pTarget->GetTargetAsActor();
    }
    else {
        pactor = 0;
    }
    if (pactor) {
        pArmor = pactor->GetRandomWornArmor(0);
        if (pArmor) {
            float SequenceWeight = fMagnitude;
            pactor->DamageEquipment(pArmor, SequenceWeight, 1);
            if (CombatController::GetShowVerboseMessages()) {
                double v3 = fMagnitude;
                const char* FullName = pArmor->GetFullName();
                Interface::PrintLine(
                    "%s's %s takes %0.2f disintegrate armor damage!", pactor->GetFullName(), FullName, v3
                );
            }
            ItemChange* apThis = pArmor;
            if (apThis) {
                delete apThis;
            }
        }
    }
}

ActiveEffect* DisintegrateArmorEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    DisintegrateArmorEffect* apMemory =
        static_cast<DisintegrateArmorEffect*>(ourMalloc(sizeof(DisintegrateArmorEffect)));
    if (apMemory) {
        return ::new (apMemory) DisintegrateArmorEffect(apCaster, apSpell, apEffect);
    }
    else {
        return 0;
    }
}
