#include "Fallout/Magic/DisintegrateWeaponEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/BoundItemEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"

// Whether the disintegrate-weapon archetype took DisintegrateWeaponEffect::Instantiate: stores RegisterActiveEffect's
// whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::DISINTEGRATE_WEAPON,
    &DisintegrateWeaponEffect::Instantiate
);

DisintegrateWeaponEffect::DisintegrateWeaponEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect) {}

DisintegrateWeaponEffect::~DisintegrateWeaponEffect() {}

ActiveEffect* DisintegrateWeaponEffect::Copy() {
    DisintegrateWeaponEffect* pCopy;
    pCopy = new DisintegrateWeaponEffect(pCaster, pSpell, pEffect);
    ActiveEffect::CopyData(pCopy);
    return pCopy;
}

bool DisintegrateWeaponEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pTargetActor != nullptr;
}

void DisintegrateWeaponEffect::Start() {
    Actor* pActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pMagicTarget = ActiveEffect::GetTarget();
        pActor = pMagicTarget->GetTargetAsActor();
    }
    if (pActor) {
        BaseProcess* pProcess = pActor->MobileObject::GetCurrentAIProcess();
        ItemChange* pWeapon = pProcess->GetCurrentWeapon();
        if (pWeapon) {
            pActor->DamageEquipment(pWeapon, fMagnitude, false);
        }
        if (CombatController::GetShowVerboseMessages()) {
            double dMagnitude = fMagnitude;
            const char* pcWeaponName = pWeapon->GetFullName();
            Interface::PrintLine(
                "%s's %s takes %0.2f disintegrate weapon damage!",
                pActor->GetFullName(),
                pcWeaponName,
                dMagnitude
            );
        }
    }
}

ActiveEffect* DisintegrateWeaponEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new DisintegrateWeaponEffect(apCaster, apSpell, apEffect);
}
