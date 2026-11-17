#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Actor;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class BSPerformanceTimer;
class CombatController;
class TESObjectREFR;

// Which sideways, forward and backward moves the actor's animations allow, and when that was last checked.
class CombatMovementRestrictions {
public:
	CombatMovementRestrictions();

	bool CanMove(uint16_t asMoveMode);
	void SetRestrictedMove(int16_t a2, char a3);
	// Sets bReset (thiscall, ret 4, 0x9CFC40; IDA: CombatProcedureAttackMelee::SetInitialized).
	// The exe stores 1 whatever abValue is; UpdateMovementRestrictions (0x9CFC00) calls it on MovementRestrictions.
	void SetReset(bool abValue);
	// Inferred type: a1: TESObjectREFR* from 0x9d02cb (parameter 0 of CombatMovementRestrictions::CheckMovementRestrictions)
	char IsLocationSafe(TESObjectREFR* a1, NiPoint3* arTarget);
	// Not named in the PDB or IDA; name chosen here. a3 selects the move to test: the exe switches on it with an
	// unsigned bound (cmp 3; ja, 0x9D022E) and Update (0x9D0610) passes cCurrentUpdate (uint32_t).
	// Inferred type: apActor: Actor* from 0x9d0645 (CombatController::pAttacker (parameter 0 of
	// CombatMovementRestrictions::Update)), its members used at 0x9d00b5
	int32_t CheckMovementRestrictions(Actor* apActor, uint32_t a3);
	void Update(CombatController* apCombatController);

	bool bReset;
	bool bInitialized;
	uint8_t padding02[2];
	uint32_t cCurrentUpdate;
	uint16_t sRestrictedMoves;
	bool bHasMoveLeftAnim;
	bool bHasMoveRightAnim;
	bool bHasMoveForwardAnim;
	bool bHasMoveBackwardAnim;
	uint8_t padding0E[2];
	NiPoint3 Location;
	CombatTimer UpdateTimer;
};

class CombatProcedureAttackMelee : public CombatProcedure {
public:
	// Values from the Xbox PDB.
	enum Maneuver : int32_t {
		ENGAGE = 0x0,
		IDLE = 0x1,
		DODGE = 0x2,
		CLOSE = 0x3,
		ADVANCE = 0x4,
		MANEUVER_COUNT = 0x5,
	};

	// Values from the Xbox PDB. Inside this class the name hides the CombatState class: spell that ::CombatState.
	enum CombatState : int32_t {
		ATTACK = 0x0,
		BLOCK = 0x1,
		HOLD = 0x2,
		DONE = 0x3,
		COMBAT_STATE_COUNT = 0x4,
	};

	CombatProcedureAttackMelee();
	~CombatProcedureAttackMelee() override;

	void Update() override;
	void Initialize(CombatController* apCombatController) override;
	void Shutdown() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// Updates the movement restrictions.
	void CheckUpdateMovementRestrictions();
	// Stops the attacker and updates the movement restrictions.
	void StopMoving();
	// Sets the maneuver state.
	void SetManeuverState(CombatProcedureAttackMelee::Maneuver aeManeuverState);
	// Starts the engage state.
	void StartEngage();
	// Picks the maneuver.
	void ManeuverDecision();
	// Picks the attack.
	void AttackDecision();
	// Handles the close state.
	void HandleClose();
	// Moves to a place facing the target.
	void MoveToFaceTargetLocation(NiPoint3& arFaceTargetLocation);
	// Handles the advance state.
	void HandleAdvance();
	// Picks the melee state.
	void MeleeDecision();
	// Handles the attack state.
	void HandleAttack();
	// Handles the block state.
	void HandleBlock();
	// Updates the movement restrictions.
	void UpdateMovementRestrictions();
	// Whether the target is moving away.
	bool IsTargetMovingAway();
	void SetCombatState(CombatProcedureAttackMelee::CombatState aeCombatState);
	bool WithinAttackRange(float afDistance, float afReach, ANIM_GROUP_ENUM* aeAttackWithinReach);
	// The reach of the current attack, looked up and cached on first use (thiscall, ret 0,
	// st0 result, 0x9CE560).
	float GetCurrentAttackReach();
	void StartAdvance();
	// eCombatState; the exe's shared one-field getter of +0x14 (thiscall, ret 0, 0x825C00, IDA: BSTimer::GetTime).
	CombatState GetCombatState();
	void StartAttack(ANIM_GROUP_ENUM aeAttack, bool abSwitchAttack);
	// Ends the attack once the hold timer runs out in state 2.
	void HandleHold();

protected:
	float GetFacingTargetDistance();

public:
	ANIM_GROUP_ENUM SelectAttack(bool abMidAttack, float afDistance, float afReach, bool abTargetAttacking,
		bool abTargetPostAttack, bool abTargetRecoiling, bool& abDoPowerAttack, bool& abSwitchAttack);
	void CheckStopBlocking();
	void HandleEngage();
	void HandleDodge();
	void HandleIdle();
	void StartClose();
	void CheckForwardPowerAttack(float afDistance, float afReach);
	bool CanDoPowerAttack(ANIM_GROUP_ENUM aePowerAttack);
	bool GetMovementRestrictionFlag(uint16_t a1);
	float GetMovingTargetVelocity();

	Maneuver eManeuverState;
	CombatState eCombatState;
	CombatState eLastCombatState;
	ANIM_GROUP_ENUM eCurrentAttack;
	CombatTimer MeleeTimer;
	CombatTimer ManeuverTimer;
	CombatTimer DodgeTimer;
	CombatTimer AdvanceTimer;
	CombatTimer CloseTimer;
	CombatTimer TargetUnreachableTimer;
	bool bRangedAllyWantsToShoot;
	uint8_t padding51[3];
	uint32_t iAdvanceMoveMode;
	float fCurrentAttackReach;
	float fFacingTargetDistance;
	CombatTimeStamp fAttackPickFailedTimestamp;
	CombatMovementRestrictions MovementRestrictions;
};

// IDA name; not in the PDB. Each maneuver's name, indexed by CombatProcedureAttackMelee::Maneuver (the exe reads
// [eManeuverState*4 + 0x11A5AA4]; the next table starts 5 entries on).
using pManeuverNames_t = const char* [5];
inline pManeuverNames_t pManeuverNames{"ENGAGE", "IDLE", "DODGE", "CLOSE", "ADVANCE"};

// IDA name; not in the PDB. Each melee state's name, indexed by CombatProcedureAttackMelee::CombatState (the exe
// reads [eCombatState*4 + 0x11A5AB8]; ePowerAttacks starts 4 entries on).
using pCombatStateNames_t = const char* [4];
inline pCombatStateNames_t pCombatStateNames{"ATTACK", "BLOCK", "HOLD", "DONE"};
inline constexpr fnvd::Vtable CombatProcedureAttackMelee_vftable{};

// IDA: BSPerfTimer::CombatAI::Melee; the PDB's CombatAIMelee.
extern BSPerformanceTimer CombatAI_Melee;
