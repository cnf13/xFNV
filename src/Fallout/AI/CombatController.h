#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Package.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/FailedCombatProcedure.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class ActiveEffect;
class Actor;
class BSPerformanceTimer;
class CombatGoal;
class CombatGroup;
struct CombatLocationInfo;
class CombatPlan;
class CombatPlannerData;
class CombatProcedure;
class CombatState;
class ItemChange;
class Projectile;
class TESCombatStyle;
class TESObjectWEAP;

enum ACTOR_SEGMENT_INVIEW : int32_t;
enum ANIM_GROUP_ENUM : int32_t;
enum PROCESS_TYPE : int32_t;

bool IsPackageFlag4Set(TESPackage* apThis, [[maybe_unused]] void* apEdx);

// How a combat procedure wants the attacker to move.
enum COMBAT_MOVE_MODE : int32_t {
	COMBAT_MOVE_MODE_WALK = 0,
	COMBAT_MOVE_MODE_FAST_WALK = 1,
	COMBAT_MOVE_MODE_SNEAK = 2,
	COMBAT_MOVE_MODE_RUN = 3,
	COMBAT_MOVE_MODE_SLOW_RUN = 4,
	COMBAT_MOVE_MODE_COUNT = 5,
};

// The low-level state of a combat controller; IsFleeing tests FLEEING.
enum COMBAT_LOW_STATE : int32_t {
	COMBAT_LOW_STATE_NONE = 0,
	COMBAT_LOW_STATE_ATTACKING = 1,
	COMBAT_LOW_STATE_PURSUING = 2,
	COMBAT_LOW_STATE_INVESTIGATING = 3,
	COMBAT_LOW_STATE_SEARCHING = 4,
	COMBAT_LOW_STATE_FLEEING = 5,
	COMBAT_LOW_STATE_COUNT = 6,
};

// The package that runs an actor's combat.
class CombatController : public TESPackage {
public:
	CombatController(Actor* apActor, CombatGroup* apGroup, TESPackage* apPackage);
	~CombatController() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	using TESPackage::InitLoadGame;
	using TESPackage::LoadGame;
	using TESPackage::SaveGame;

	// Sets pCombatPlannerData.
	void SetCombatPlannerData(CombatPlannerData* apPlannerData);
	void UpdateDebugGeometry(bool abForceClear) {}
	// Refreshes the controller's current weapon data.
	void UpdateCurrentWeapon();
	void ClearReferencePointers(TESObjectREFR* apRef);
	// Stores the attacker's height, sneak height and whether it can sneak and move.
	void CalculateAttackerInfo();
	// The forward (Y) distance the power attack moves the attacker, 0 when the delta is not known.
	float GetPowerAttackForwardDistance(ANIM_GROUP_ENUM aePowerAttack);
	// Resets the move mode to 0 and applies it.
	void ClearMoveMode();
	float GetRunSpeedMult();
	// Reads the procedure kind the save data names, then rebuilds that procedure, initialised
	// with this controller and loaded.
	void LoadProcedure(CombatProcedure*& arpProcedure, BGSLoadFormBuffer* apLoadGameBuffer);
	// Writes the procedure's kind, then its own save data. SaveGame calls it with &pActionProcedure,
	// &pMovementProcedure and each background procedure.
	void SaveProcedure(CombatProcedure*& arpProcedure, BGSSaveFormBuffer* apSaveGameBuffer);

	bool IsFleeing();
	// The combat test IsAllowedToSwim makes before a fighting or
	// fleeing actor may swim.
	bool QSwimAllowed();
	void SetMovementProcedure(CombatProcedure* apMovementProcedure);
	void SetActionProcedure(CombatProcedure* apActionProcedure);
	// Passes the reload on to the combat state.
	void ReloadingWeapon(TESObjectWEAP* apWeapon, bool abForced);
	// The actor fought (pTarget).
	Actor* GetTarget();
	// Whether a search procedure runs; the whole byte is tested.
	uint8_t IsSearching();
	// Records how much of the target the last line-of-sight check saw.
	void SetTargetSegmentInView(ACTOR_SEGMENT_INVIEW aeInView);
	// Fills fPowerAttackRanges and fPowerAttackHitTimes for the five power attack animation
	// groups.
	void InitPowerAttackDistances();
	// Whether apActor is one of the controller's combat targets.
	bool IsActoraCombatTarget(Actor* apActor);
	// The weapon the controller fights with (pCurrentWeapon).
	TESObjectWEAP* GetCurrentWeapon();
	float GetAttackerHeight() { return fAttackerHeight; }
	float GetAttackerSneakHeight() { return fAttackerSneakHeight; }
	// The combat style in use.
	TESCombatStyle* GetCurrentCombatStyle();
	// The actor running the combat.
	Actor* GetAttacker();
	// Asks the controller's aim object for the point to aim at from arStartLocation into arOut; false without one.
	bool GetAimLocation(NiPoint3* arOut, const NiPoint3* arStartLocation);
	// The controller's combat group (pCombatGroup).
	CombatGroup* GetCombatGroup() { return pCombatGroup; }
	void DebugCombatPlan();
	void ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel);
	void UpdateCurrentCombatStyle();
	void ActivatedReference(TESObjectREFR* apRef, TESBoundObject* apObject, int32_t aiCount,
		bool abActivatedScriptOnly);
	// The attacker's movement for the power attack (the static overload for the attacker).
	bool GetPowerAttackDelta(ANIM_GROUP_ENUM aePowerAttack, NiPoint3& aNewPos);
	// Sends aiMoveMode to the attacker, sneaking when combat forces it, and remembers it.
	void SetActualMoveMode(uint32_t aiMoveMode);
	CombatProcedure* GetBackgroundProcedure(int32_t aiProcedureType);
	bool CombatPlanContainsAction(int32_t aiAction);
	// Resets the speed multipliers, lets a fast walk walk faster and a slow run run slower, then sets the move mode.
	void SetMoveMode(COMBAT_MOVE_MODE aeCombatMoveMode);
	void FinishedPackageTask();
	void SetIgnoreLookAtTarget(uint8_t acPriority);
	// Records apTarget as a target of the controller's combat group; for the player the compass first shows the
	// attacker as hostile, and a group still shared with the player is split off.
	void AddTarget(Actor* apTarget, int32_t iPriority, bool abHighPriority, float afDamageDone, float afDamageTaken);
	// Stops the player's compass showing apActor as hostile and drops apActor from the combat group's targets.
	void RemoveTarget(Actor* apActor);
	void SetUseIronSights(char acValue);
	bool GetAttackerCanSneak();
	void ClearProcedures();
	// Shuts down and deletes every background procedure, then empties the array and frees its buffer.
	void ClearBackgroundProcedures();
	void ForceCombat();
	float GetCombatPosition();
	void SetUpdatedDetection(bool abValue);
	void ForcePlanUpdate();
	void CheckCurrentWeapon();
	void PerformedMeleeAttack(Actor* apTarget);
	void DamagedByAttacker(Actor* apAttacker, float afDamage);
	bool GetProcedureFailed();
	bool GetProcedureFinished();
	void StartCalmed();
	bool ShouldStopCombat();
	void SetForceSneak(bool abValue);
	uint32_t GetLookAtTargetIndex(CombatProcedure* apCombatProcedure);
	void SetLookAtTarget(uint8_t acIndex, const NiPoint3& arLookAtTarget, uint8_t acPriority);
	void ClearLookAtTarget(uint8_t acIndex);
	int32_t IsNotDetectingTarget();
	bool IsSwitchingWeapons();
	// Returns uint8_t rather than bool because the whole byte is tested; bool would assume 0/1.
	uint8_t GetForceSneak();
	bool GetUpdatedDetection();
	void SetCombatPosition(float afCombatPosition);
	double GetPowerAttackRange(int32_t a2);
	double GetPowerAttackHitTime(int32_t a2);
	char ShouldInitPowerAttackDistances();
	void FirstUpdate();
	void TargetChanged();
	void ForceStopCombat();
	// Releases the combat state's cover location.
	void UnInitializeCombatPlanData();
	ACTOR_SEGMENT_INVIEW GetTargetSegmentInView();
	uint8_t HasLineOfSight();
	void SetIsBlocking(char a2);
	char IsBlocking();
	bool IsPlanUpdateForced();
	// Marks the cached inventory as needing a rescan.
	void SetInventoryDirty(bool abValue);
	// Reacts to the actor losing its weapon to apOpponent.
	void Disarmed(Actor* apOpponent);
	// An empty stub in this build. Actor::DoDamage passes the damage as afDamage, and DemoralizeEffect::Start calls it
	// with the caster and 0.
	void ForceFlee(Actor* apActor, float afDamage) {}
	// Called by ArrowProjectile::CollideActor and CollideTerrain. The function is empty.
	void ProjectileHitObstruction(const NiPoint3& aContactPoint, bool abActor) {}
	// Called by Cmd_ForceTakeCover. The body is empty, as the game does.
	void ForceTakeCover(Actor* apTarget, float afDuration) {}
	void ClearCombatPlan();
	void SetTarget(Actor* apTarget);
	bool StopCombat(Actor* apTarget);
	void TakeHit(Actor* apAttacker, ActiveEffect* apEffect);
	void FiredProjectile(Projectile* apProjectile);
	void UpdateLowMovement();
	void Update();
	void AddFailedCombatProcedure(CombatProcedure* apCombatProcedure);
	void InitializeCombatPlanData();
	void UpdateLookAtTarget();
	void UpdateLow();
	int32_t GetAttackSkill();
	void AddBackgroundProcedure(CombatProcedure* apBackgroundProcedure);
	void UpdatePackageData(TESPackage* apCurrentPackage);

	// Whether combat prints its verbose messages to the console.
	static bool GetShowVerboseMessages();
	// Sets whether combat prints its verbose messages.
	static void SetShowVerboseMessages(bool abShowVerboseMessages);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static bool CheckHasEquip(Actor* apAttacker, ItemChange* apWeapon);

	CombatGroup* pCombatGroup;
	CombatProcedure* pActionProcedure;
	CombatProcedure* pMovementProcedure;
	BSSimpleArray<CombatProcedure*, 1024> BackgroundProcedures;
	CombatState* pCombatState;
	CombatPlan* pCombatPlan;
	CombatPlannerData* pCombatPlannerData;
	CombatAction* pCurrentAction;
	CombatGoal* pCurrentGoal;
	int32_t iCurrentActionValue;
	CombatTimer PlanTimer;
	Actor* pAttacker;
	Actor* pTarget;
	bool bForceCombat;
	bool bUpdatedDetection;
	int8_t cUseIronSights;
	// Declared as uint8_t rather than bool because the whole byte is tested; bool would assume 0/1.
	uint8_t bForceSneak;
	uint32_t iMoveMode;
	float fAttackerHeight;
	float fAttackerSneakHeight;
	bool bAttackerCanSneak;
	bool bAttackerCanMove;
	uint8_t paddingD6[0x2];
	float fWalkSpeedMult;
	float fRunSpeedMult;
	float fCombatPosition;
	bool bFirstUpdate;
	uint8_t paddingE5[0x3];
	// The debug geometry the controller draws and its copy placed in the cell.
	NiPointer<NiNode> spDebugGeometry;
	NiPointer<NiNode> spCellDebugGeometry;
	TESObjectWEAP* pCurrentWeapon;
	TESCombatStyle* pCurrentCombatStyle;
	bool bDestructing;
	bool bDeterminedPowerAttackDeltas;
	uint8_t paddingFA[0x2];
	float fPowerAttackRanges[5];
	float fPowerAttackHitTimes[5];
	bool bHasForwardAttacks;
	bool bStopCombat;
	bool bStoppingCombat;
	uint8_t padding127[0x1];
	NiPoint3 kLookAtTargets[2];
	uint8_t ucLookAtTargetPriorities[2];
	uint8_t ucIgnoreLookAtTarget;
	uint8_t padding143[0x1];
	COMBAT_LOW_STATE eLowState;
	float fLowMovementDelta;
	// The last five procedures that failed; AddFailedCombatProcedure shifts them down and writes the newest into the
	// last.
	FailedCombatProcedure FailedCombatProcedures[5];

	static inline bool bProcessCombat = true;
	static inline bool bShowVerboseMessages;
	static inline int32_t iStaggerPeriodicUpdateCounter;
};

// The game settings CombatController.cpp defines.
extern SettingT<INISettingCollection> bDebugCombatAttackerText;
extern SettingT<INISettingCollection> bDebugCombatGuardRadius;
extern SettingT<INISettingCollection> bDebugCombatUnreachableLocations;
extern SettingT<INISettingCollection> bDebugCombatArea;

using kCombatLocationTypeData_t = CombatLocationInfo[22];
extern kCombatLocationTypeData_t kCombatLocationTypeData;

extern BSPerformanceTimer CombatAI_Planning;
