#pragma once

#include "Fallout/Magic/ActiveEffect.h"

class AssociatedItemEffect : public ActiveEffect {
public:
	AssociatedItemEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~AssociatedItemEffect() override;

	ActiveEffect* Copy() override;
	uint16_t GetSaveSize(Actor*) override;
	void SaveGame(Actor*) override;
	void LoadGame(Actor*) override;
	void InitLoadGame(Actor*) override;
	void CopyData(ActiveEffect*) override;
	using ActiveEffect::InitLoadGame;
	using ActiveEffect::LoadGame;
	using ActiveEffect::SaveGame;

	TESForm* GetAssociatedItem();

	TESForm* pAssociatedItem;
};
