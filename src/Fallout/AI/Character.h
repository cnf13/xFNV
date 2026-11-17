#pragma once

#include "Fallout/AI/Actor.h"
#include "Reconstruction.hpp"

class BipedAnim;

// An NPC: an actor that wears armor and clothing, shown through its biped animation.
class Character : public Actor {
public:
	Character();
	// A character, with a process when abadd.
	explicit Character(bool abadd);
	~Character() override;

	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	// Loads the actor's state, then the character's trespass and guard flags.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(uint32_t aiFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	bool IsChild(bool abIgnoreHeight) override;
	// The worn-item model of the actor.
	BipedAnim* GetBiped() override;
	void SetBiped(BipedAnim* apBiped) override;
	bool IsNpc() override { return true; }

	void Update(float afDelta) override;

	bool IsActoraGuard() override;
	void SetActorGuard(bool abGuard) override;
	void SetEquippedWeight(float afvalue) override;
	float GetEquippedWeight() override { return fequippedweight; }
	float GetTurningSpeed() override;
	void InitiateSandmanPackage(Actor* apTarget, TESObjectREFR* apfurniture, FurnitureMark* aMarker,
		uint8_t auiIndex) override;
	TESRace* GetRace() override;
	int32_t GetActorType() override { return 1; }
	// Forgets the cached armor rating and threshold so they are recalculated.
	void ResetArmorRating() override;
	bool InitiateTresPassPackage(TrespassPackage* apPack) override;
	void InitiateSpectator() override;
	float GetArmorRating() override;
	float GetArmorThreshold() override;
	int32_t GetAttackDamage() override;
	bool IsTresPassing() override { return bTrespassing; }
	// Records whether the actor stands in a cell that is not its own.
	void SetTresPassing() override;
	void PrecacheData() override;

	virtual void InitiateCannibal(Actor* apTarget);
	virtual void InitiateDisMountHorsePackage() {}

	// Drops the character's 3D and rebuilds it.
	void Reset3D();
	float GetClothingValue();

	BipedAnim* pBiped;
	float fArmorRating;
	float fArmorThreshold;
	bool bTrespassing;
	bool bGuard;
	uint8_t padding1C2[2];
	float fequippedweight;
};
inline constexpr fnvd::Vtable Character_vftable{};
inline constexpr fnvd::Vtable Character_vftable_ActorValueOwner{};
inline constexpr fnvd::Vtable Character_vftable_CachedValuesOwner{};
inline constexpr fnvd::Vtable Character_vftable_MagicCaster{};
inline constexpr fnvd::Vtable Character_vftable_MagicTarget{};
inline constexpr fnvd::Vtable Character_vftable_TESChildCell{};
