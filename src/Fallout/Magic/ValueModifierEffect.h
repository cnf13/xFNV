#pragma once

#include "Fallout/Magic/ActiveEffect.h"

class TESObjectREFR;

class ValueModifierEffect : public ActiveEffect {
public:
	ValueModifierEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~ValueModifierEffect() override;

	ActiveEffect* Copy() override;
	void Finalize() override;
	void Update(float) override;
	uint16_t GetSaveSize(Actor*) override;
	void SaveGame(Actor*) override;
	void LoadGame(Actor*) override;
	void FinishInitLoadGame(Actor*) override;
	bool IsCausingDamage() override;
	void CopyData(ActiveEffect*) override;
	bool CheckTarget(MagicTarget*) override;
	void Start() override;
	void Continue() override;
	void Finish() override;
	virtual void SetActorValue(ActorValue::Index aeWhichActorValue) { eWhichActorValue = aeWhichActorValue; }
	virtual ActorValue::Index GetActorValue() { return eWhichActorValue; }
	virtual void ModifyActorValue(Actor*, float, ActorValue::Index);
	using ActiveEffect::LoadGame;
	using ActiveEffect::SaveGame;

	void ShowAttributeReductionMessage();

protected:
	// Returns afMagnitude, or for a negative change of a S.P.E.C.I.A.L., derived attribute or skill of an effect
	// without conditions, the part of it that leaves the target's value at 0.
	float CheckAdjustNoNegativeMagnitude(float afMagnitude);

public:
	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	ActorValue::Index eWhichActorValue;
};
