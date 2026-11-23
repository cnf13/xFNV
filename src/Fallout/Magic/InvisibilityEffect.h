#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"

// The Invisibility magic effect: makes the target's 3D refract the scene while it runs. It adds no fields to
// ValueModifierEffect.
class InvisibilityEffect : public ValueModifierEffect {
public:
    InvisibilityEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~InvisibilityEffect() override;

    ActiveEffect* Copy() override;
    void Update(float afSecondsElapsed) override;
    void FinishInitLoadGame(Actor* apActor) override;
    void Revert(Actor* apActor) override;
    void CopyData(ActiveEffect* apDestination) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;
    void Finish() override;
    using ValueModifierEffect::Revert;

    static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
