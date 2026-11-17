#pragma once

#include "Fallout/Magic/ActiveEffect.h"

// The Disintegrate Weapon magic effect: destroys the weapon the target holds. It adds no fields to ActiveEffect.
class DisintegrateWeaponEffect : public ActiveEffect {
public:
	DisintegrateWeaponEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~DisintegrateWeaponEffect() override;

	ActiveEffect* Copy() override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;

	static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
};
