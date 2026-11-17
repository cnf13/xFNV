#pragma once

#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout/Magic/ActiveEffect.h"

class EffectItem;
class MagicCaster;
class MagicItem;

ActiveEffect* CurePoision(MagicCaster* a1, MagicItem* a2, EffectItem* a3);

// The Cure magic effects (disease, paralysis, addiction, poison): removes the effects of one archetype from the target.
class CureEffect : public ActiveEffect {
public:
	CureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect, int32_t aiActorValue);
	// The archetype constructor (ConstructWithArchetype): the enum parameter tells it from the one above.
	CureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect,
		EffectArchetypes::ArchetypeID aeArchetype);
	~CureEffect() override;

	ActiveEffect* Copy() override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;

	static ActiveEffect* InstantiateCureDisease(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	static ActiveEffect* InstantiateCureParalysis(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	static ActiveEffect* InstantiateCureAddiction(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	MagicSystem::SpellType eSpellType;
	EffectArchetypes::ArchetypeID eArchetypeID;
};
