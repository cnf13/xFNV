#pragma once

#include "Fallout/Magic/ValueModifierEffect.h"

class EffectItem;
class MagicCaster;
class MagicItem;

// The Frenzy magic effect: a value modifier that remembers whether the target was already in combat when it started.
class FrenzyEffect : public ValueModifierEffect {
public:
	FrenzyEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~FrenzyEffect() override;

	ActiveEffect* Copy() override;
	uint16_t GetSaveSize(Actor* apActor) override;
	void SaveGame(Actor* apActor) override;
	void LoadGame(Actor* apActor) override;
	bool CheckDeletedCaster(MagicCaster* apCaster) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	bool bStartedInCombat;
	uint8_t padding4D[3];
};
