#include "Fallout/Magic/CommandHumanoidEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/Magic/CommandEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"

static char bCommandHumanoidEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::COMMAND_HUMANOID,
    &CommandHumanoidEffect::Instantiate
);

CommandHumanoidEffect::CommandHumanoidEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : CommandEffect(apCaster, apItem, apEffect) {}

CommandHumanoidEffect::~CommandHumanoidEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* CommandHumanoidEffect::Copy() {
    ActiveEffect* pCopy = new CommandHumanoidEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

bool CommandHumanoidEffect::CheckTarget(MagicTarget* apTarget) {
    Character* pCharacter = dynamic_cast<Character*>(apTarget);
    if (!pCharacter) {
        return false;
    }
    if (pCharacter->IsPc()) {
        return false;
    }
    float fEffectMagnitude = fMagnitude;
    uint16_t usLevel = pCharacter->Actor::GetLevel();
    return MagicFormulas::CheckTargetLevelMagnitude(usLevel, fEffectMagnitude);
}

ActiveEffect* CommandHumanoidEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CommandHumanoidEffect(apCaster, apSpell, apEffect);
}
