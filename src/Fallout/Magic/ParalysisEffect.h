#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"

// The Paralysis magic effect: holds the target still while it runs. It adds no fields to ValueModifierEffect.
class ParalysisEffect : public ValueModifierEffect {
public:
	ParalysisEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~ParalysisEffect() override;

	ActiveEffect* Copy() override;
	void Update(float afSecondsElapsed) override;
	void FinishInitLoadGame(Actor* apActor) override;
	void Revert(Actor* apActor) override;
	void CopyData(ActiveEffect* apDestination) override;
	void Start() override;
	void Finish() override;
	using ValueModifierEffect::Revert;

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
