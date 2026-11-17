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

static char bCommandHumanoidEffectInitialized =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::COMMAND_HUMANOID, &CommandHumanoidEffect::Instantiate);

CommandHumanoidEffect::CommandHumanoidEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : CommandEffect(apCaster, apItem, apEffect) {}

CommandHumanoidEffect::~CommandHumanoidEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* CommandHumanoidEffect::Copy() {
    ActiveEffect* v2;
    void* v4 = ourMalloc(sizeof(CommandHumanoidEffect));
    if (v4) {
        v2 = ::new (v4) CommandHumanoidEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    ActiveEffect* pThis = this;
    pThis->CopyData(v2);
    return v2;
}

bool CommandHumanoidEffect::CheckTarget(MagicTarget* apTarget) {
    Character* v6 = dynamic_cast<Character*>(apTarget);
    if (!v6) {
        return (bool)0;
    }
    if (v6->IsPc()) {
        return (bool)0;
    }
    float flt1C = fMagnitude;
    uint16_t level = v6->Actor::GetLevel();
    return (bool)((uint8_t)MagicFormulas::CheckTargetLevelMagnitude(level, flt1C));
}

ActiveEffect* CommandHumanoidEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* apMemory = ourMalloc(sizeof(CommandHumanoidEffect));
    if (apMemory) {
        return ::new (apMemory) CommandHumanoidEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
