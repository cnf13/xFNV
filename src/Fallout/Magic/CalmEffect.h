#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"
#include "Reconstruction.hpp"

// The Calm magic effect: takes the target out of combat while it runs. It adds no fields to ValueModifierEffect.
class CalmEffect : public ValueModifierEffect {
public:
	CalmEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~CalmEffect() override;

	ActiveEffect* Copy() override;
	void Update(float afSecondsElapsed) override;
	bool CheckDeletedCaster(MagicCaster* apCaster) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;

	static CalmEffect* Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
};
inline constexpr fnvd::Vtable CalmEffect_vftable{};
