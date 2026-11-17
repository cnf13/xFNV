#pragma once

#include "Fallout/Magic/MagicProjectile.h"

class EffectSetting;
class MagicCaster;
class MagicItem;

// The spray spell projectile. Its own members are not declared.
class MagicSprayProjectile : public MagicProjectile {
public:
    MagicSprayProjectile(MagicCaster* a1, MagicItem* a2, EffectItem* a3, EffectSetting* a4, NiPoint3 a5, NiPoint3 a6);

    // Does nothing.
    void Update(float afDelta) override;
};
