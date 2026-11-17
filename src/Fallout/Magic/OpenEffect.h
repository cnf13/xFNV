#pragma once

#include "Fallout/Magic/ActiveEffect.h"

// Opens a locked non-actor magic target; its constructor adds no fields to ActiveEffect.
class OpenEffect : public ActiveEffect {
public:
	OpenEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~OpenEffect() override;

	ActiveEffect* Copy() override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;

	static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
};
