#pragma once

#include "Fallout/Magic/MagicProjectile.h"

class EffectSetting;
class MagicCaster;
class MagicItem;

// The spray spell projectile. Its own members are not declared.
class MagicSprayProjectile : public MagicProjectile {
public:
    MagicSprayProjectile(
        MagicCaster* apCaster,
        MagicItem* apSpell,
        EffectItem* apEffectItem,
        EffectSetting* apAVEffect,
        NiPoint3 aLocation,
        NiPoint3 aRotation
    );

    // Does nothing.
    void Update(float afDelta) override;
};
