#pragma once

#include "Fallout/Magic/ActiveEffect.h"

// The Dispel magic effect: ends the other effects on the target. It adds no fields to ActiveEffect.
class DispelEffect : public ActiveEffect {
public:
    DispelEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
    ~DispelEffect() override;

    ActiveEffect* Copy() override;
    void CopyData(ActiveEffect* apDestination) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;

    static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
};
