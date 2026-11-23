#pragma once

#include "Fallout/Magic/ActiveEffect.h"

// The Turn Undead magic effect: makes undead targets flee while the effect lasts.
class TurnUndeadEffect : public ActiveEffect {
public:
    TurnUndeadEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~TurnUndeadEffect() override;

    ActiveEffect* Copy() override;
    uint16_t GetSaveSize(Actor* apActor) override;
    void SaveGame(Actor*) override;
    void LoadGame(Actor*) override;
    bool CheckDeletedCaster(MagicCaster* apCaster) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;
    void Finish() override;

    static ActiveEffect* Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);

    bool bTurned;
};
