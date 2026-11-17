#pragma once

#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimer.h"

class CombatProcedureAttackRanged : public CombatProcedure {
public:
	// Values from the Xbox PDB. The exe compares the state signed (GetTargetedPoint).
	enum ATTACK_STATE : int32_t {
		ATTACK_STATE_IDLE = 0x0,
		ATTACK_STATE_FIRING = 0x1,
		ATTACK_STATE_COOLDOWN = 0x2,
		ATTACK_STATE_SUPPRESSIVE_FIRE = 0x3,
	};

	CombatProcedureAttackRanged();
	~CombatProcedureAttackRanged() override;

	void Update() override;
	void Shutdown() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	bool GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) override;
	void Disarmed(Actor* apOpponent) override;
	// The PC FiredProjectile slot is inherited unchanged from CombatProcedure.
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// Whether the targeted point is set and within the weapon's aim arc (at least 15 degrees) of the attacker.
	bool AimAtTarget();
	// Ends the attacker's firing.
	void StopFiring();
	// The point to aim at from apReleasePoint (thiscall, ret 8, the result through a hidden
	// pointer, 0x9D1C90).
	NiPoint3 CalculateTargetedPoint(NiPoint3* apReleasePoint);
	// Starts the attacker's firing.
	void StartFiring(bool abAutomatic);

	ATTACK_STATE eState;
	NiPoint3 TargetedPoint;
	CombatTimer ExplosiveProjectileBlockedTimer;
	CombatTimer FiringTimer;
	CombatTimer CooldownTimer;
	CombatTimer FireDelayTimer;
	CombatTimer SuppressiveFireTimer;
};
inline constexpr fnvd::Vtable CombatProcedureAttackRanged_vftable{};
