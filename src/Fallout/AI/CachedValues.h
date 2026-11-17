#pragma once

#include <cstdint>

// values of an actor that are expensive to compute, cached in the actor's
// current process; a set bit in iFlags means the value is up to date
class CachedValues {
public:
	// validity bits in iFlags
	enum Flags : uint32_t {
		CVF_RADIUS = 0x1,
		CVF_WIDTH = 0x2,
		CVF_LENGTH = 0x4,
		CVF_WEAPON_DPS = 0x8,
		CVF_MEDICINE_EFFECTIVENESS_MULT = 0x10,
		CVF_PARALYSIS = 0x20,
		CVF_HEALING_RATE = 0x40,
		CVF_ENDURANCE = 0x80,
		CVF_PERCEPTION_CONDITION = 0x100,
		CVF_EYE_LEVEL = 0x200,
		CVF_ACTOR_AGGRESSION = 0x400,
		CVF_ACTOR_ASSISTANCE = 0x800,
		CVF_WALK_SPEED = 0x1000,
		CVF_RUN_SPEED = 0x2000,
		CVF_CONDITION_PREVENTS_RUN = 0x4000,
		CVF_FORWARD_LENGTH = 0x8000,
		CVF_ACTOR_IS_GHOST = 0x10000000,
		CVF_HEALTH_DAMAGED = 0x20000000,
		CVF_FATIGUE_DAMAGED = 0x40000000,
		kCached_SurvivalSkillMult = 0x80000000,
	};

	CachedValues();

	// marks the value stale so the next getter recomputes it
	void SetDirty(CachedValues::Flags aiFlag);
	// marks the value fresh without recomputing it
	void SetUpToDate(CachedValues::Flags aiFlag);
	// sets or clears the validity bit directly
	void SetFlag(CachedValues::Flags aiFlag, bool abSet);
	// true while the value needs to be recomputed
	bool IsDirty(CachedValues::Flags aiFlag);
	// marks every value stale
	void Reset();
	void Copy(CachedValues* apSource);

	// 0x0
	float fCachedRadius;
	float fCachedWidth;
	float fCachedLength;
	float fCachedForwardLength;
	float fCachedWeaponDPS;
	float fCachedMedicineEffectivenessMult;
	float fCachedSurvivalEffectivenessMult;
	float fCachedParalysis;
	float fCachedHealingRate;
	float fCachedEndurance;
	float fCachedPerceptionCondition;
	float fCachedEyeLevel;
	int32_t eCachedActorAggression;
	int32_t eCachedActorAssistance;
	float fCachedWalkSpeed;
	float fCachedRunSpeed;
	bool bConditionPreventsRun;
	uint8_t padding41[3];
	uint32_t iFlags;
};
