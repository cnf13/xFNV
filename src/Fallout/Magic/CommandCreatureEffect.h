#pragma once

#include "Fallout/Magic/CommandEffect.h"

class EffectItem;
class MagicCaster;
class MagicItem;

// The Command Creature magic effect: the CommandEffect that accepts creature targets. It adds no fields.
class CommandCreatureEffect : public CommandEffect {
public:
    CommandCreatureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~CommandCreatureEffect() override;

    ActiveEffect* Copy() override;
    bool CheckTarget(MagicTarget* apTarget) override;

    static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
