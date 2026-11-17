#pragma once

#include "Fallout/Magic/ActiveEffect.h"

class EffectItem;
class MagicItem;

// The Vampirism magic effect. It adds no fields to ActiveEffect.
class VampirismEffect : public ActiveEffect {
public:
	VampirismEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~VampirismEffect() override;

	ActiveEffect* Copy() override;
	void InitLoadGame(Actor* apActor) override;
	void Revert(Actor* apActor) override;
	void CopyData(ActiveEffect* apDestination) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	using ActiveEffect::InitLoadGame;
	using ActiveEffect::Revert;

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
