#pragma once

#include "Fallout/Magic/ActiveEffect.h"
#include "Reconstruction.hpp"

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

	// The associated item (pAssociatedItem); the exe's shared +0x48 getter (thiscall, ret 0,
	// 0x6733E0).
	TESForm* GetAssociatedItem();

	TESForm* pAssociatedItem;
};
inline constexpr fnvd::Vtable AssociatedItemEffect_vftable{};
