#include "Fallout/Magic/CureEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"

static char bCureDiseaseEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::CURE_DISEASE,
    &CureEffect::InstantiateCureDisease
);
static char bCureParalysisEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::CURE_PARALYSIS,
    &CureEffect::InstantiateCureParalysis
);
static char bCureAddictionEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::CURE_ADDICTION,
    &CureEffect::InstantiateCureAddiction
);

// Whether the cure-poison archetype took CurePoision; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CURE_POISON, &CurePoision);

ActiveEffect* CurePoision(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CureEffect(apCaster, apSpell, apEffect, MagicSystem::POISON);
}

CureEffect::CureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect, int32_t aiSpellType)
  : ActiveEffect(apCaster, apItem, apEffect), eSpellType(static_cast<MagicSystem::SpellType>(aiSpellType)),
    eArchetypeID(EffectArchetypes::ARCHETYPE_NONE) {}

CureEffect::CureEffect(
    MagicCaster* apCaster,
    MagicItem* apItem,
    EffectItem* apEffect,
    EffectArchetypes::ArchetypeID aeArchetype
)
  : ActiveEffect(apCaster, apItem, apEffect), eSpellType(MagicSystem::SPELL), eArchetypeID(aeArchetype) {}

CureEffect::~CureEffect() {}

// eArchetypeID -1 means the copy cures a spell type, built with the spell-type constructor; otherwise it cures an
// archetype, built with the archetype constructor. Either way the base data is then copied through a virtual call.
ActiveEffect* CureEffect::Copy() {
    ActiveEffect* pCopy = nullptr;
    if (eArchetypeID == -1) {
        pCopy = new CureEffect(pCaster, pSpell, pEffect, static_cast<int32_t>(eSpellType));
    }
    else {
        pCopy = new CureEffect(pCaster, pSpell, pEffect, eArchetypeID);
    }
    CopyData(pCopy);
    return pCopy;
}

bool CureEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pTargetActor != nullptr;
}

void CureEffect::Start() {
    if (eArchetypeID == EffectArchetypes::ARCHETYPE_NONE) {
        int32_t iSpellType = eSpellType;
        ActiveEffect::GetTarget()->Dispel_4(iSpellType);
    }
    else {
        int32_t iArchetypeID = eArchetypeID;
        ActiveEffect::GetTarget()->Dispel(static_cast<EffectArchetypes::ArchetypeID>(iArchetypeID), nullptr);
    }
}

ActiveEffect* CureEffect::InstantiateCureDisease(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CureEffect(apCaster, apSpell, apEffect, 1);
}

ActiveEffect* CureEffect::InstantiateCureParalysis(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CureEffect(apCaster, apSpell, apEffect, 24);
}

ActiveEffect* CureEffect::InstantiateCureAddiction(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CureEffect(apCaster, apSpell, apEffect, 10);
}
