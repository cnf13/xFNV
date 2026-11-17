#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Fallout/Magic/ValueModifierEffect.h"

class EffectItem;
class MagicCaster;
class MagicItem;

// An actor value modifier effect that also applies to a body part (the limb condition effects): it adds the body
// part it targets to its base class.
class ValueAndConditionsEffect : public ValueModifierEffect {
public:
	ValueAndConditionsEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~ValueAndConditionsEffect() override;

	ActiveEffect* Copy() override;
	uint16_t GetSaveSize(Actor* apActor) override;
	void SaveGame(BGSSaveFormBuffer*) override;
	void SaveGame(Actor*) override;
	void LoadGame(BGSLoadFormBuffer*) override;
	void LoadGame(Actor*) override;
	void CopyData(ActiveEffect* apDestination) override;
	void ModifyActorValue(Actor* apActor, float afModifier, ActorValue::Index aeOverrideActorValue) override;

	void SetTargetBodyPart(ActorValue::Index aeWhichBodyPart);

	static ActiveEffect* Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);

	ActorValue::Index eTargetBodyPart;
};
