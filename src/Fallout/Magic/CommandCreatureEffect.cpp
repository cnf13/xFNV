#include "Fallout/Magic/CommandCreatureEffect.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/CommandEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"

static char bCommandCreatureEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::COMMAND_CREATURE,
    &CommandCreatureEffect::Instantiate
);

CommandCreatureEffect::CommandCreatureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : CommandEffect(apCaster, apItem, apEffect) {}

CommandCreatureEffect::~CommandCreatureEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* CommandCreatureEffect::Copy() {
    ActiveEffect* pCopy;
    pCopy = new CommandCreatureEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

bool CommandCreatureEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    Actor* pCreature = pTargetActor && pTargetActor->IsCreature() ? pTargetActor : nullptr;
    if (!pCreature) {
        return false;
    }
    float fCommandMagnitude = fMagnitude;
    uint16_t usLevel = pCreature->GetLevel();
    return MagicFormulas::CheckTargetLevelMagnitude(usLevel, fCommandMagnitude);
}

ActiveEffect* CommandCreatureEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CommandCreatureEffect(apCaster, apSpell, apEffect);
}
