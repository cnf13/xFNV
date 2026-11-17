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

static char bCommandCreatureEffectInitialized =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::COMMAND_CREATURE, &CommandCreatureEffect::Instantiate);

CommandCreatureEffect::CommandCreatureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : CommandEffect(apCaster, apItem, apEffect) {}

CommandCreatureEffect::~CommandCreatureEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* CommandCreatureEffect::Copy() {
    ActiveEffect* v2;
    void* v4 = ourMalloc(sizeof(CommandCreatureEffect));
    if (v4) {
        v2 = ::new (v4) CommandCreatureEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    ActiveEffect* pThis = this;
    pThis->CopyData(v2);
    return v2;
}

bool CommandCreatureEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* v5;
    Actor* targetAsActor;
    if (apTarget) {
        targetAsActor = apTarget->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor && targetAsActor->IsCreature()) {
        v5 = targetAsActor;
    }
    else {
        v5 = nullptr;
    }
    if (!v5) {
        return false;
    }
    float flt1C = fMagnitude;
    uint16_t level = v5->GetLevel();
    return (bool)((uint8_t)MagicFormulas::CheckTargetLevelMagnitude(level, flt1C));
}

ActiveEffect* CommandCreatureEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    CommandCreatureEffect* apMemory = static_cast<CommandCreatureEffect*>(ourMalloc(sizeof(CommandCreatureEffect)));
    if (apMemory) {
        return ::new (apMemory) CommandCreatureEffect(apCaster, apSpell, apEffect);
    }

    return nullptr;
}
