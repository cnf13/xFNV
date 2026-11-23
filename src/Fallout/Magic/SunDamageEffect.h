#pragma once

#include "Fallout/Magic/ActiveEffect.h"

#include <limits>

// The active effect of the sun's damage on an actor in the open (radiation-style hit effect with a glow).
class SunDamageEffect : public ActiveEffect {
public:
    SunDamageEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
    ~SunDamageEffect() override;

    ActiveEffect* Copy() override;
    void Update(float afSecondsElapsed) override;
    uint16_t GetSaveSize(Actor* apActor) override;
    void SaveGame(Actor*) override;
    void LoadGame(Actor*) override;
    bool IsCausingDamage() override;
    void CopyData(ActiveEffect* apDestination) override;
    bool CheckTarget(MagicTarget* apTarget) override;

    // The damage the effect does this update.
    float GetDamageAmount();
    void UpdateHitEffect(bool abDamaging);

    static ActiveEffect* Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);

    float fGlowEffect;
    bool bHitEffectApplied;
    bool bDoNotTakeDamageNextUpdate;

    static inline float fPlayerPainCountdown = std::numeric_limits<float>::max();
};
