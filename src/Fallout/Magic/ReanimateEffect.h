#pragma once

#include "Fallout/Magic/ActiveEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Reconstruction.hpp"

class bhkRigidBody;

// The effect that raises a dead actor. Its own members are not declared yet.
class ReanimateEffect : public ActiveEffect {
public:
	ReanimateEffect(MagicCaster* a1, MagicItem* a2, EffectItem* a3);

	uint16_t GetSaveSize(Actor* apActor) override;
	void SaveGame(Actor* apActor) override;
	void LoadGame(Actor* apActor) override;
	// Raises the dead target: alive again as a ghost facing the caster, a do-nothing package, and its spine body
	// keyframed from where it lies. Dispels itself without a caster or a dead actor target.
	void Start() override;
	// Runs the reanimation: lifts and turns the body toward the caster, plays the get-up, swaps in the reanimated
	// eyes, then brings the actor back as the caster's commanded actor.
	void Update(float afSecondsElapsed) override;
	ActiveEffect* Copy() override;
	// ActiveEffect slots 11, 13, 14, 19 and 22.
	void FinishInitLoadGame(Actor* apActor) override;
	void Revert(Actor* apActor) override;
	bool CheckDeletedCaster(MagicCaster* apCaster) override;
	bool CheckTarget(MagicTarget* apTarget) override;
	void Finish() override;

	// The reanimation state (iState, +0x4C); the exe's shared one-field getter 0x68A810
	// (thiscall, ret 0).
	int32_t GetState();

	static ActiveEffect* Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3);

	bhkRigidBody* pBody;
	// The PDB's int; the exe compares it signed (0x8276F6 cmp [ecx+0x4C], 0x32; jge).
	int32_t iState;
	float fSaveTime;
	NiPoint3 StartPos;
	NiQuaternion StartRot;
};
inline constexpr fnvd::Vtable ReanimateEffect_vftable{};
