#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class TelekinesisEffect : public ValueModifierEffect {
public:
	TelekinesisEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~TelekinesisEffect() override;

	ActiveEffect* Copy() override;
	void Update(float) override;
	uint16_t GetSaveSize(Actor*) override;
	void SaveGame(Actor*) override;
	void LoadGame(Actor*) override;
	void FinishInitLoadGame(Actor*) override;
	void Revert(Actor*) override;
	bool CheckDeletedCaster(MagicCaster*) override;
	void CopyData(ActiveEffect*) override;
	void Start() override;
	void Finish() override;

	using ValueModifierEffect::LoadGame;
	using ValueModifierEffect::Revert;
	using ValueModifierEffect::SaveGame;

	// Releases the reference the player holds: thrown along the player's facing when
	// bThrow is set, else stopped.
	void DoThrow();
	// Creates the spring that holds the reference.
	void CreateSpring();

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);

	NiPointer<NiRefObject> spSpring;
	float fHoldDistance;
	float fMoveVelocity;
	TESObjectREFR* pRef;
	bool bThrow;
	bool bFirstUpdate;
	uint8_t padding5E[2];
};
