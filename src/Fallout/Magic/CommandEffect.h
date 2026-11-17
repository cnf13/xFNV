#pragma once

#include "Fallout/Magic/ActiveEffect.h"

// The common part of the Command Creature and Command Humanoid effects, which put the target under the caster's
// orders. It adds no fields to ActiveEffect.
class CommandEffect : public ActiveEffect {
public:
    CommandEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
    ~CommandEffect() override;

    ActiveEffect* Copy() override;
    void Update(float afSecondsElapsed) override;
    bool CheckDeletedCaster(MagicCaster* apCaster) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;
    void Finish() override;

    // Puts apTarget under the orders of apCaster.
    static void InitCommandedActor(Actor* apTarget, Actor* apCaster);
    static void UpdateCommandedActor(Actor* apTarget, Actor* apCaster);
    static void CleanupCommandedActor(Actor* apTarget, Actor* apCaster, bool abClearCommandingActor);
};
