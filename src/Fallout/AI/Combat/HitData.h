#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Actor;
struct DamageImpactData;
class ItemChange;
class Projectile;
class SpellItem;
class TESObjectREFR;
class TESObjectWEAP;
struct VATS_COMMAND;

int32_t BSMAX_3(uint16_t a1, int32_t a2);

class HitData {
public:
	// The bits of uiFlags.
	enum Flag : int32_t {
		HD_BLOCKED = 0x1,
		HD_BLOCK_WITH_WEAPON = 0x2,
		HD_CRITICAL = 0x4,
		HD_CRITICAL_ON_DEATH = 0x8,
		HD_FATAL = 0x10,
		HD_DISMEMBER_LIMB = 0x20,
		HD_EXPLODE_LIMB = 0x40,
		HD_CRIPPLE_LIMB = 0x80,
		HD_DISARM = 0x100,
		HD_DISABLE_WEAPON = 0x200,
		HD_SNEAK_ATTACK = 0x400,
		HD_IGNORE_CRITICAL = 0x800,
		HD_PREDICT_DAMAGE = 0x1000,
		HD_EXPLOSION = 0x2000,
	};

	HitData();

	// Applies armour and blocking to the damage.
	void ReduceDamage(bool abIgnoreBlocking);
	// Decides what the hit does to the target.
	void DetermineOutcome();
	// Whether uiFlags has any bit of aeFlag.
	bool IsFlagSet(Flag aeFlag);
	// Sets or clears the bits aeFlag of uiFlags. VATS calls it on VATS_COMMAND::pHitData. AdjustAttackDamage passes
	// TESObjectWEAP's critical flags byte as it is, so the value is not narrowed to a bool here.
	void SetFlag(Flag aeFlag, uint8_t abValue);
	void IncRefCount();
	void DecRefCount();
	void ClearData();
	void Copy(HitData* apSource);
	void SetTargetedLimb(Projectile* apProjectile);
	// Rolls a critical hit.
	void CheckCriticalStrike();

protected:
	void SetBonusMult(float afNewMult);

public:
	// Fills apData for a hit of afDamage on apTarget at the impact apImpactData, then reduces the damage and
	// decides the outcome. apSourceRef is not read.
	static void InitializeImpactData(HitData* apData, TESObjectREFR* apSourceRef, Actor* apTarget, float afDamage,
		DamageImpactData* apImpactData);
	static void InitializeHitData(HitData* apData, Actor* apAggressor, Actor* apTarget, ItemChange* apWeaponItem,
		bool abPowerAttack);
	static void InitializeHitData(HitData* apData, Actor* apAggressor, Actor* apTarget, ItemChange* apWeaponItem,
		Projectile* apProjectile);
	// Fills apData for the VATS action apCommand.
	static void InitializeHitData(HitData* apData, VATS_COMMAND* apCommand);
	// Fills apData for a hit of afDamage by apAggressor, who is also the source, on apTarget, then reduces the
	// damage and decides the outcome.
	static void InitializeHitData(HitData* apData, Actor* apAggressor, Actor* apTarget, float afDamage);
	static float PredictVATSCommandHealthDamage(VATS_COMMAND* apVATSCommand);
	static void InitializeHitData(HitData* apData, Actor* apAggressor, Actor* apTarget, ItemChange* apWeaponItem,
		bool abPowerAttack, Projectile* apProjectile);
	static void InitializeExplosionData(HitData* apData, Actor* apAggressor, Actor* apTarget,
		TESObjectREFR* apSourceRef, float afDamage);

	Actor* pAggressor;
	Actor* pTarget;
	// This pointer is read as a Projectile/Explosion union.
	TESObjectREFR* pSourceRef;
	ActorValue::Index eAttackSkill;
	BGSBodyPart::LIMB_ENUM eDamageLimb;
	float fHealthDamage;
	float fTotalDamage;
	float fFatigueDamage;
	float fTargetedLimbDamage;
	float fPercentBlocked;
	float fArmorDamage;
	float fDamageToWeapon;
	TESObjectWEAP* pWeapon;
	float fWeaponHealth;
	NiPoint3 kHitLocation;
	NiPoint3 kHitDirection;
	SpellItem* pCriticalEffect;
	VATS_COMMAND* pVATSCommand;
	uint32_t uiFlags;
	float fBonusMult;
	// The PC refcount is unsigned.
	uint32_t iRefCount;
};

// NiPointer of a hit takes and releases references through the hit's own IncRefCount and DecRefCount.
inline void NiIncRefCount(HitData* apData) {
	apData->IncRefCount();
}

inline void NiDecRefCount(HitData* apData) {
	apData->DecRefCount();
}
