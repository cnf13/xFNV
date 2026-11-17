#pragma once

#include "Fallout/Magic/ActiveEffect.h"

// Locks a non-actor magic target; its constructor adds no fields to ActiveEffect.
class LockEffect : public ActiveEffect {
public:
	LockEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~LockEffect() override;

	ActiveEffect* Copy() override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;

	static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
};
