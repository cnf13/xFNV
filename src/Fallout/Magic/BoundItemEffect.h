#pragma once

#include "Fallout/Magic/AssociatedItemEffect.h"

class ItemChange;

class BoundItemEffect : public AssociatedItemEffect {
public:
	BoundItemEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~BoundItemEffect() override;

	ActiveEffect* Copy() override;
	void Update(float) override;
	uint16_t GetSaveSize(Actor*) override;
	void SaveGame(Actor*) override;
	void LoadGame(Actor*) override;
	void InitLoadGame(Actor*) override;
	bool CheckTarget(MagicTarget*) override;
	void Start() override;
	void Finish() override;
	using AssociatedItemEffect::InitLoadGame;
	using AssociatedItemEffect::LoadGame;
	using AssociatedItemEffect::SaveGame;

	void DoSummon();

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	ItemChange* pDisplacedWeapon;
	ItemChange* pDisplacedItems[20];
	float fAppearTime;
	bool bItemSummoned;
	bool bSpellModelCached;
	bool bWeaponCached;
	bool bWornCached;
	bool bSetupItem;
	uint8_t paddingA9[3];
};

// Moves apItem's armor into the first other bound armor effect on
// apActor that displaces nothing in a slot both fill; true when one took it.
bool MoveItemToOtherBoundEffect(Actor* apActor, ItemChange* apItem, BoundItemEffect* apEffect);
