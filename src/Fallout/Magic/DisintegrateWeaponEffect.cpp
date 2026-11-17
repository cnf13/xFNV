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
    DisintegrateWeaponEffect* v2;
    void* v4 = ourMalloc(sizeof(DisintegrateWeaponEffect));
    if (v4) {
        v2 = ::new (v4) DisintegrateWeaponEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    ActiveEffect::CopyData(v2);
    return v2;
}

bool DisintegrateWeaponEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor;
    if (apTarget) {
        pTargetActor = apTarget->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    return pTargetActor != nullptr;
}

void DisintegrateWeaponEffect::Start() {
    MagicTarget* unk24;
    const char* v2;
    const char* fullName;
    float flt1C;
    double v5;
    BaseProcess* currentAIProcess;
    Actor* targetAsActor;
    ItemChange* v9;
    if (ActiveEffect::GetTarget()) {
        unk24 = ActiveEffect::GetTarget();
        targetAsActor = unk24->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor) {
        currentAIProcess = targetAsActor->MobileObject::GetCurrentAIProcess();
        v9 = currentAIProcess->GetCurrentWeapon();
        if (v9) {
            flt1C = fMagnitude;
            targetAsActor->DamageEquipment(v9, flt1C, false);
        }
        if (CombatController::GetShowVerboseMessages()) {
            v5 = (double)fMagnitude;
            fullName = v9->GetFullName();
            v2 = targetAsActor->GetFullName();
            Interface::PrintLine("%s's %s takes %0.2f disintegrate weapon damage!", v2, fullName, v5);
        }
    }
}

ActiveEffect* DisintegrateWeaponEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
    void* v5 = ourMalloc(sizeof(DisintegrateWeaponEffect));
    if (v5) {
        return ::new (v5) DisintegrateWeaponEffect(a1, a2, a3);
    }
    else {
        return nullptr;
    }
}
