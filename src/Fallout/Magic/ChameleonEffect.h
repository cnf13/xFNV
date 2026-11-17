#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"

// The chameleon effect: its actor value modifier makes the target blend into its surroundings. Only the members the
// ported code needs are declared.
class ChameleonEffect : public ValueModifierEffect {
public:
    ChameleonEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~ChameleonEffect() override;

    ActiveEffect* Copy() override;
    void FinishInitLoadGame(Actor* apActor) override;
    void Revert(Actor* apActor) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void CopyData(ActiveEffect* apDestination) override;
    bool CheckTarget(MagicTarget* apTarget) override;
    void Start() override;
    void Finish() override;

protected:
    void Update(float afSecondsElapsed) override;

public:
    static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
};
