#pragma once

#include "Fallout/Magic/ActiveEffect.h"
#include "Reconstruction.hpp"

class Script;
class ScriptLocals;

// The script spell effect: it runs its script, with locals of its own, while it lasts.
class ScriptEffect : public ActiveEffect {
public:
	ScriptEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~ScriptEffect() override;

	ActiveEffect* Copy() override;
	void Update(float afSecondsElapsed) override;
	uint16_t GetSaveSize(Actor* apActor) override;
	void SaveGame(BGSSaveFormBuffer*) override;
	void SaveGame(Actor*) override;
	void LoadGame(BGSLoadFormBuffer*) override;
	void LoadGame(Actor*) override;
	void Revert(Actor*) override;
	void CopyData(ActiveEffect* apDestination) override;
	void Start() override;
	void Finish() override;

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	Script* pScript;
	ScriptLocals* pEffectLocals;
};
inline constexpr fnvd::Vtable ScriptEffect_vftable{};
