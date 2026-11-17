#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"

// The value modifier effect that also darkens its target's 3D while it runs.
class DarknessEffect : public ValueModifierEffect {
public:
	DarknessEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	ActiveEffect* Copy() override;
	void FinishInitLoadGame(Actor* apActor) override;
	void Revert(Actor* apActor) override;
	void CopyData(ActiveEffect* apDestination) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;

	using ValueModifierEffect::Revert;

	// Allocates and constructs the effect; its parameters are passed on to Construct unchanged.
	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
