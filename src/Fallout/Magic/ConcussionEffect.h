#pragma once

#include "Fallout/Magic/ActiveEffect.h"
#include "Reconstruction.hpp"

class Actor;
class TESImageSpaceModifier;

TESImageSpaceModifier* GetConcussionIMOD();

// The Concussion magic effect: staggers the target until the timer runs out.
class ConcussionEffect : public ActiveEffect {
public:
	ConcussionEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~ConcussionEffect() override;

	ActiveEffect* Copy() override;
	void Update(float afSecondsElapsed) override;
	bool CheckDeletedCaster(MagicCaster* apCaster) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;

	void UpdateSound(Actor* a2);

	static ConcussionEffect* Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	float fTimer;
};

inline constexpr fnvd::Vtable ConcussionEffect_vftable{};
inline char bConcussion_11DBE50;
