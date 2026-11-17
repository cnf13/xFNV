#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"
#include "Reconstruction.hpp"

// The Detect Life magic effect: shows the living actors around the player while it runs.
class DetectLifeEffect : public ValueModifierEffect {
public:
	DetectLifeEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~DetectLifeEffect() override;

	ActiveEffect* Copy() override;
	void Start() override;
	void Update(float) override;
	void Finish() override;

	static void CheckPlayerRunningDetectLife();
	static char GetPlayerDetectingLife();
	static ActiveEffect* Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
	static bool CheckActorDetected(Actor* apTarget, bool abCurrentlyDetected);

	// Whether the player has a Detect Life effect running.
	static inline bool bPlayerDetectingLife;
};
inline constexpr fnvd::Vtable DetectLifeEffect_vftable{};
