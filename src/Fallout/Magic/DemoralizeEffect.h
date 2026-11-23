#pragma once

#include "Fallout/Magic/ActiveEffect.h"

class EffectItem;
class MagicCaster;
class MagicItem;

// The Demoralize magic effect: turns the target against its allies until the effect ends.
class DemoralizeEffect : public ActiveEffect {
public:
    DemoralizeEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~DemoralizeEffect() override;

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
