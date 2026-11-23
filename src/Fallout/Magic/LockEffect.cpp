#include "Fallout/Magic/LockEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "SDK/MSVC/rtti.h"

// Whether the lock archetype took LockEffect::Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::LOCK, &LockEffect::Instantiate);

LockEffect::LockEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect) {}

LockEffect::~LockEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* LockEffect::Copy() {
    ActiveEffect* pCopy = new LockEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

bool LockEffect::CheckTarget(MagicTarget* apTarget) {
    NonActorMagicTarget* pnaTarget = dynamic_cast<NonActorMagicTarget*>(apTarget);
    if (!pnaTarget || !pnaTarget->GetTargetStatsObject()) {
        return false;
    }
    TESObjectDOOR* pDoor = dynamic_cast<TESObjectDOOR*>(pnaTarget->GetTargetStatsObject()->GetObjectReference());
    TESObjectCONT* pContainer = dynamic_cast<TESObjectCONT*>(pnaTarget->GetTargetStatsObject()->GetObjectReference());
    return pDoor || pContainer;
}

void LockEffect::Start() {
    NonActorMagicTarget* pnaTarget = dynamic_cast<NonActorMagicTarget*>(pTarget);
    if (pnaTarget && pnaTarget->GetTargetStatsObject()) {
        LockData* pLock = pnaTarget->GetTargetStatsObject()->GetLock();
        if (pLock && ((pLock->cFlags & 2) == 0 || (pLock->cFlags & LockData::FLAG_LOCKED) != 0)) {
            Dispel(false);
        }
        else {
            pLock = pnaTarget->GetTargetStatsObject()->AddLock();
            pLock->cLockLevel = static_cast<int32_t>(fMagnitude);
            pLock->pKey = nullptr;
            pLock->cFlags = 2;
            pnaTarget->GetTargetStatsObject()->AddLockChange();
            pnaTarget->GetTargetStatsObject()->Lock();
        }
    }
}

ActiveEffect* LockEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new LockEffect(apCaster, apSpell, apEffect);
}
