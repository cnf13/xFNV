#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Fallout/Magic/ValueModifierEffect.h"

// A value modifier effect that also raises a second actor value (the shield's additional actor value) while it lasts.
class ShieldEffect : public ValueModifierEffect {
public:
    ShieldEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~ShieldEffect() override;

    ActiveEffect* Copy() override;
    void Update(float afSecondsElapsed) override;
    uint16_t GetSaveSize(Actor* apActor) override;
    void SaveGame(Actor* apActor) override;
    void LoadGame(Actor* apActor) override;
    void CopyData(ActiveEffect* apDestination) override;
    void Start() override;
    void Finish() override;

    // Adds afValue to apActor's additional actor value; picks the base or the modified value by the effect's spell type
    // and flags.
    void ModifyAdditionalActorValue(Actor* apActor, float afModifier);

    static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

    ActorValue::Index eAdditionalActorValue;
};
