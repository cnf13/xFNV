#pragma once

#include "Fallout/Magic/ActiveEffect.h"

class EffectItem;
class ItemChange;
class MagicCaster;
class MagicItem;

// The Disintegrate Armor magic effect: destroys the armor the target wears.
class DisintegrateArmorEffect : public ActiveEffect {
public:
    DisintegrateArmorEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
    ~DisintegrateArmorEffect() override;

    ActiveEffect* Copy() override;
    uint16_t GetSaveSize(Actor* apActor) override;
    void SaveGame(Actor*) override;
    void LoadGame(Actor*) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;
    using ActiveEffect::LoadGame;
    using ActiveEffect::SaveGame;

    static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

    // The worn armor's inventory entry: Start stores Actor::GetRandomWornArmor's ItemChange here.
    ItemChange* pArmor;
};
