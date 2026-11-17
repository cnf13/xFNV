#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"

// The effect that changes the condition of one limb of its target.
class LimbConditionEffect : public ValueModifierEffect {
public:
    LimbConditionEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
    ~LimbConditionEffect() override;

    ActiveEffect* Copy() override;
    uint16_t GetSaveSize(Actor* apActor) override;
    void SaveGame(BGSSaveFormBuffer* apBuffer) override;
    void SaveGame(Actor* apActor) override;
    void LoadGame(BGSLoadFormBuffer* apBuffer) override;
    void LoadGame(Actor* apActor) override;
    void CopyData(ActiveEffect* apDestination) override;
    void ModifyActorValue(Actor* apActor, float afModifier, ActorValue::Index aeOverrideActorValue) override;

    void SetTargetBodyPart(uint32_t aeWhichBodyPart);

    static ActiveEffect* Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

    ActorValue::Index eTargetBodyPart;
};
