#pragma once

#include "Fallout/Magic/ActiveEffect.h"
#include "Reconstruction.hpp"

// The Turn Undead magic effect: makes undead targets flee while the effect lasts.
class TurnUndeadEffect : public ActiveEffect {
public:
	TurnUndeadEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect);
	~TurnUndeadEffect() override;

	ActiveEffect* Copy() override;
	uint16_t GetSaveSize(Actor* apActor) override;
	void SaveGame(Actor*) override;
	void LoadGame(Actor*) override;
	bool CheckDeletedCaster(MagicCaster* apCaster) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Start() override;
	void Finish() override;

	static ActiveEffect* Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3);

	bool bTurned;
	uint8_t padding49[3];
};
inline constexpr fnvd::Vtable TurnUndeadEffect_vftable{};
