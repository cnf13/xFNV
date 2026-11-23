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
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::DISINTEGRATE_ARMOR,
    &DisintegrateArmorEffect::Instantiate
);

DisintegrateArmorEffect::DisintegrateArmorEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect), pArmor(nullptr) {}

DisintegrateArmorEffect::~DisintegrateArmorEffect() {
    delete pArmor;
}

ActiveEffect* DisintegrateArmorEffect::Copy() {
    DisintegrateArmorEffect* pCopy;
    pCopy = new DisintegrateArmorEffect(pCaster, pSpell, pEffect);
    ActiveEffect::CopyData(pCopy);
    return pCopy;
}

// Save version 0x37 added 4 bytes.
uint16_t DisintegrateArmorEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = ActiveEffect::GetSaveSize(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x37) {
        usSize += 4;
    }
    return usSize;
}

void DisintegrateArmorEffect::SaveGame(Actor* apActor) {
    BIPED_OBJECT eArmorPosition;
    ActiveEffect::SaveGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 55u) {
        eArmorPosition = BO_NONE;
        if (pArmor && pArmor->pContainerObj) {
            TESObjectARMO* parmor = dynamic_cast<TESObjectARMO*>(pArmor->pContainerObj);
            if (parmor) {
                // The slots are counted as an integer and passed as FillsBipedSlot's BIPED_OBJECT.
                for (uint32_t uiBiped = 0; uiBiped < 20; ++uiBiped) {
                    if (parmor->FillsBipedSlot(static_cast<BIPED_OBJECT>(uiBiped), false, nullptr)) {
                        eArmorPosition = static_cast<BIPED_OBJECT>(uiBiped);
                        break;
                    }
                }
            }
        }

        pSaveLoadGameOLD->SaveGameDataOLD(&eArmorPosition, sizeof(eArmorPosition));
    }
}

void DisintegrateArmorEffect::LoadGame(Actor* apActor) {
    BIPED_OBJECT eArmorPosition;
    ActiveEffect::LoadGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 55u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&eArmorPosition, sizeof(eArmorPosition));
        if (eArmorPosition != BO_NONE) {
            InventoryChanges* pInventory = apActor->m_Extra.GetContainerChanges();
            if (pInventory) {
                this->pArmor = pInventory->GetWornItem(eArmorPosition, true);
            }
            ItemChange* pWornArmor = this->pArmor;
            if (pWornArmor) {
                delete pWornArmor;
            }
        }
    }
}

bool DisintegrateArmorEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pTargetActor != nullptr;
}

void DisintegrateArmorEffect::Start() {
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pActor) {
        pArmor = pActor->GetRandomWornArmor(false);
        if (pArmor) {
            pActor->DamageEquipment(pArmor, fMagnitude, true);
            if (CombatController::GetShowVerboseMessages()) {
                double dMagnitude = fMagnitude;
                const char* pcArmorName = pArmor->GetFullName();
                Interface::PrintLine(
                    "%s's %s takes %0.2f disintegrate armor damage!",
                    pActor->GetFullName(),
                    pcArmorName,
                    dMagnitude
                );
            }
            ItemChange* pWornArmor = pArmor;
            if (pWornArmor) {
                delete pWornArmor;
            }
        }
    }
}

ActiveEffect* DisintegrateArmorEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new DisintegrateArmorEffect(apCaster, apSpell, apEffect);
}
