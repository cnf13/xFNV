#include "Fallout/Magic/OpenEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "SDK/MSVC/rtti.h"

// Whether the open archetype took OpenEffect::Instantiate. Stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::OPEN, &OpenEffect::Instantiate);

OpenEffect::OpenEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect) {}

OpenEffect::~OpenEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* OpenEffect::Copy() {
    ActiveEffect* pCopy = new OpenEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

// A non-actor target whose reference is a door or a container.
bool OpenEffect::CheckTarget(MagicTarget* apTarget) {
    NonActorMagicTarget* pTarget = dynamic_cast<NonActorMagicTarget*>(apTarget);
    if (!pTarget || !pTarget->GetTargetStatsObject()) {
        return false;
    }
    TESObjectDOOR* pDoor = dynamic_cast<TESObjectDOOR*>(pTarget->GetTargetStatsObject()->GetObjectReference());
    TESObjectCONT* pContainer = dynamic_cast<TESObjectCONT*>(pTarget->GetTargetStatsObject()->GetObjectReference());
    return pDoor || pContainer;
}

void OpenEffect::Start() {
    NonActorMagicTarget* pNATarget = dynamic_cast<NonActorMagicTarget*>(pTarget);
    if (pNATarget && pNATarget->GetTargetStatsObject()) {
        LockData* pLock = pNATarget->GetTargetStatsObject()->GetLock();
        double dMagnitude = fMagnitude;
        LOCK_LEVEL eSpellLevel = static_cast<LOCK_LEVEL>(GetLockPickLevel(static_cast<int32_t>(dMagnitude)));
        int32_t iLockLevel = pNATarget->GetTargetStatsObject()->GetLockLevel();
        LOCK_LEVEL eLockLevel = static_cast<LOCK_LEVEL>(GetLockPickLevel(iLockLevel));
        if (pLock && eSpellLevel >= eLockLevel) {
            pNATarget->GetTargetStatsObject()->UnLock();
        }
    }
}

ActiveEffect* OpenEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new OpenEffect(apCaster, apSpell, apEffect);
}
