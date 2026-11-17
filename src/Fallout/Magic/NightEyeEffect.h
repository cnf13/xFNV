#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"
#include "Reconstruction.hpp"

// The Night Eye magic effect: brightens the player's view while it runs. It adds no fields to ValueModifierEffect.
class NightEyeEffect : public ValueModifierEffect {
public:
	NightEyeEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~NightEyeEffect() override;

	ActiveEffect* Copy() override;
	void Update(float afSecondsElapsed) override;
	void InitLoadGame(Actor* apActor) override;
	void Revert(Actor* apActor) override;
	void CopyData(ActiveEffect* apDestination) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;
	using ValueModifierEffect::InitLoadGame;
	using ValueModifierEffect::Revert;

	// Turns the player's night eye image space on.
	static void ApplyVisualEffect();
	static ActiveEffect* Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
inline constexpr fnvd::Vtable NightEyeEffect_vftable{};
