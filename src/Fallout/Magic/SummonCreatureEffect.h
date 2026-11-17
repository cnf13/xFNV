#pragma once

#include "Fallout/Magic/AssociatedItemEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;

// qsort comparator of summon points by desirability, best first.
int32_t SummonPosSort(const void* p1, const void* p2);

class SummonCreatureEffect : public AssociatedItemEffect {
public:
	SummonCreatureEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	~SummonCreatureEffect() override;

	ActiveEffect* Copy() override;
	void Update(float) override;
	uint16_t GetSaveSize(Actor*) override;
	void SaveGame(Actor*) override;
	void LoadGame(Actor*) override;
	void InitLoadGame(Actor*) override;
	bool CheckDeletedCaster(MagicCaster*) override;
	bool CheckTarget(MagicTarget*) override;
	void Start() override;
	void Finish() override;
	using AssociatedItemEffect::InitLoadGame;
	using AssociatedItemEffect::LoadGame;
	using AssociatedItemEffect::SaveGame;

	void ClearSummonedCreature();
	int32_t GeneratePointCloud(NiPoint3** returnArray, Actor* apOwnerActor, Actor* apSummonedActor);
	// Places the summoned creature at the appear location and puts it under the target's orders; pOwner is not
	// read.
	void DoSummon(Actor* pOwner);
	void LimitPlayerSummonedCreatures();
	// Whether the summoned creature fits at location (thiscall, ret 8; apSummoned is unused).
	bool CheckSummonLocation(Actor* apSummoned, NiPoint3& location);
	bool GetBestSummonLocation(Actor* apOwnerActor, Actor* apSummonedActor, NiPoint3* returnLocation);

	static ActiveEffect* Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect);
	static float computeDesirability(Actor* summoner, const NiPoint3& aLocation);

	// The exe casts the loaded reference to Actor (InitLoadGame, 0x829900), not to Creature.
	Actor* pSummonedCreature;
	uint8_t bTargetSet;
	uint8_t padding51[3];
	float fAppearTime;
	NiPoint3 AppearLocation;
	NiPoint3 AppearRotation;
	bool bPlacement;
	bool bcached;
	uint8_t padding72[2];
};

inline Actor* summoner;
inline constexpr fnvd::Vtable SummonCreatureEffect_vftable{};
