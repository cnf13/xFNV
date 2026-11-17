#pragma once

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatCoverSearch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

enum ACTOR_SEGMENT_INVIEW : int32_t;
class Actor;
class ActorPathingMessageQueue;
class BGSExplosion;
class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class CombatController;
class CombatCoverLocation;
class CombatLocation;
class PathingAvoidNodeArray;
class PathingRequestCoveredMove;
class UnreachableCoverLocation;
class UnreachableLocation;

double CalculateCostForDistance(CombatController* a1, BGSWorldLocation* arg4, BGSWorldLocation* a2);

// What a combat location target is for: the type CombatState::SetLocationTargetRef (0x9A0790) and SetLocationTarget
// take. The PDB's enumerators; the exe passes and stores it as one byte (0x9A081B mov dl,[ebp+0xC]; 0x9A081E
// mov [ecx],dl), so its underlying type is uint8_t.
enum LOCATION_TARGET_TYPE : uint8_t {
	LOCATION_TARGET_DESTINATION = 0,
	LOCATION_TARGET_GUARD = 1,
	LOCATION_TARGET_CLUSTER = 2,
	LOCATION_TARGET_COMBAT_AREA = 3,
	LOCATION_TARGET_COUNT = 4,
};

// Why a cover location was rejected, or that it was selected (CombatState::CheckCoverLocation's result). The PDB's
// enumerators and values; the exe stores it as a dword.
enum COVER_STATE : int32_t {
	COVER_STATE_NONE = 0,
	COVER_STATE_MOVED = 1,
	COVER_STATE_NO_EDGES_OPEN = 2,
	COVER_STATE_UNREACHABLE = 3,
	COVER_STATE_COVER_INVALID = 4,
	COVER_STATE_FIRE_INVALID = 5,
	COVER_STATE_LEDGE_ANGLE = 6,
	COVER_STATE_TOO_FAR = 7,
	COVER_STATE_TOO_CLOSE = 8,
	COVER_STATE_CANT_RESERVE = 9,
	COVER_STATE_HOSTILES_NEAR = 10,
	COVER_STATE_COVER_THREATENED = 11,
	COVER_STATE_FIRE_THREATENED = 12,
	COVER_STATE_COVER_LOS = 13,
	COVER_STATE_FIRE_LOS = 14,
	COVER_STATE_LOCATION_TYPE_DIFFERENT = 15,
	COVER_STATE_CHECK_MOVEMENT_FAILED = 16,
	COVER_STATE_TARGET_TOO_FAR = 17,
	COVER_STATE_COVER_TOO_FAR = 18,
	COVER_STATE_SELECTED = 19,
	COVER_STATE_COUNT = 20,
};

// Where a combat state is heading: a reference and a world location, and how near to the target it should get
// (IDA: CombatState::LocationTarget).
class CombatLocationTarget {
public:
	CombatLocationTarget();

	// Saves cType, bNearTarget, fMinDistance, fMaxDistance, pRef's form ID, then WorldLoc.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

	// Resolves the stored target form ID to the reference once the game is loaded; the buffer is unused (the PDB's
	// InitLoadGame, IDA's InitLoadGameBGS, thiscall, ret 4, 0x9A3220).
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Inferred type: a2: BGSLoadGameBuffer* from 0x9a268d (parameter 0 of CombatState::LoadGame)
	void LoadGame(BGSLoadGameBuffer* a2);

	// IDA name ucRefType. PDB: char cType; the exe reads it zero-extended (movzx at 0x9AA780, 0x9AA7D9).
	uint8_t cType;
	bool bNearTarget;
	uint8_t padding02[2];
	TESObjectREFR* pRef;
	BGSWorldLocation WorldLoc;
	float fMinDistance;
	float fMaxDistance;
};

class CombatState {
public:
	// The PDB's CombatState::LocationTarget (src names it CombatLocationTarget above).
	using LocationTarget = CombatLocationTarget;

	// A combat item and its form (IDA's names; the PDB has none). Non-additive: replaces the forward declaration.
	struct TargetedObject {
		// The exe's constructor, folded with the pointer list's (0x96A2D0): clears pRef, then pObject.
		TargetedObject();

		TESForm* pRef;
		TESBoundObject* pObject;
	};

	CombatState(CombatController* apCombatController, float afTimerDelay);
	~CombatState();

	// Finds a combat location of type aiLocation.
	bool FindCombatLocation(int32_t aiLocation);
	// Picks a dodge point to the side, back or front of the attacker (the aiDodgeDirections bits 1, 2, 4, 8) that
	// has no threat, a clear path and movement, and keeps the last direction for 10 seconds; false when none.
	bool FindBestDodgeLocation(CombatLocation* apRetLocation, uint32_t aiDodgeDirections,
		uint32_t aiDodgeDirectionLOSCheck, float afDodgeDistance, float afMinDodgeDistance);
	// Reserves apCoverLocation in the combat manager's cover map unless it is reserved already.
	void ReserveCoverLocation(CombatCoverLocation* apCoverLocation);
	// Releases the secondary cover location.
	void UnreserveSecondaryCoverLocation();
	// Releases the combat state's own cover location.
	void UnreserveCoverLocation();
	// Releases apCoverLocation in the combat manager's cover map when it is reserved.
	void UnreserveCoverLocation(CombatCoverLocation* apCoverLocation);
	// Updates fThreatRatio.
	void UpdateThreatRatio();
	// Whether another target of the group blocks the line of sight from arCoverPathingLocation, apIgnoreTarget aside.
	bool CheckTargetsCoverLOS(PathingLocation& arCoverPathingLocation, Actor* apIgnoreTarget);
	// Whether the cover location with key aiCoverKey is on the unreachable list.
	bool IsCoverLocationUnreachable(uint32_t aiCoverKey);
	// Tries each open side of arCoverLocation as cover from arCoverSearchTargetLocation: reservable, valid, reachable,
	// in range, free of other targets and threats, hidden and (when asked) with a line of sight from its fire position.
	// On success stores the cover point in apRetLocation; otherwise areCoverState gets the worst reason found.
	bool CheckCoverLocation(PathingCoverLocation& arCoverLocation, CombatLocation* apRetLocation,
		CoverReservationInfo* apReservationInfo, NiPoint3& arCoverSearchTargetLocation, uint32_t aiCoverKey,
		uint32_t aiCurrentCoverLocationKey, int32_t aiLocation, Actor* apAttacker, Actor* apTarget, float afMinRange,
		float afMaxRange, COVER_STATE& areCoverState, float& arfSquaredDistance, bool abHideCover);
	// Whether a 360 degree line of sight to the target is buffered.
	bool GetHasBuffered360LineOfSight();
	CombatCoverLocation* GetSecondaryCoverLocation();
	// Reserves the secondary cover location.
	void ReserveSecondaryCoverLocation();
	// Runs the first update of the combat state.
	void DoInitialUpdate();
	// Handles a change of the combat target.
	void TargetChanged();
	// Writes the last seen target location to arLastSeenLocation and returns true when there is a target that was
	// in 360 degree line of sight within the last 120 s.
	bool GetLastSeenTargetLocation(NiPoint3& arLastSeenLocation);
	void SetLastFleeDoorTimeStamp(float afTime);
	CombatState* ClearFiringStartedTimeStamp();
	void SetThreatsConsideredTimeStamp(float afValue);
	void ClearCoverLocation(bool abInvalidateCover);
	void DamagedByAttacker(Actor* apAttacker, float afDamage);
	void Disarmed();
	void UpdateInventoryData();
	void UpdateLow();
	void UpdatePeriodicData();
	void UpdateImmediateData();
	void SetTargetBlocked(bool abValue);
	bool GetTargetBlocked();
	bool GetTargetSpaceDifferent();
	CombatCoverLocation* GetCoverLocation();
	TESBoundObject* GetUsableCombatItem(int32_t aiCombatItem);
	void SetInventoryDirty(bool abValue);
	bool GetHasTarget360LineOfSight();
	bool IsMeleeOnly();
	void Update();
	// The exe tests the whole byte; bool would assume 0/1.
	void ReloadingWeapon(TESObjectWEAP* apWeapon, bool abForced);
	void SetCoverLocation(CombatCoverLocation* apCombatCoverLocation);
	void ClearCombatLocation();
	void ClearLocationTarget();
	CombatTimer* GetUsableCombatItemTimer(int32_t aiCombatItem);
	void SetTargetFlushed(bool abValue);
	bool GetPermission(uint32_t aiPermission);
	// IDA name; not in the PDB. auiIndex indexes pCombatItems; the callers' loops test it unsigned (jae 5,
	// 0x997AA0) and 0x9A06C0 passes its own unsigned argument on.
	void RemoveTargetedObject(uint32_t auiIndex);
	bool ShouldUseMelee();
	void SetFlag(int32_t a2);

	TESObjectWEAP* GetWeaponByCombatType(COMBAT_WEAPON_TYPE aeType);
	// Stamps the time of the last attack.
	void PerformedMeleeAttack(Actor* apTarget);
	// Stamps the attack, and the firing start when it is unset (-FLT_MAX).
	void FiredProjectile(Projectile* apProjectile);
	// The PDB places this in CombatController.cpp.
	CombatTimeStamp GetLastTargetDetectedTimeStamp();
	bool GetHasBufferedDetection();
	void ClearSecondaryCoverSearchInfo();
	void ClearCoverSearchInfo();
	// Reserves the combat state's own cover location (the PDB's no-argument overload;
	// IDA: CombatState::980770).
	void ReserveCoverLocation();
	// Clears the combat location and sets apLocation (IDA: CombatState::9A0650).
	void SetCombatLocation(CombatLocation* apCombatLocation);
	void ClearSecondaryCoverLocation(bool abClearLocation);
	// The callers' loops test the index unsigned (jae 5, 0x9AA4E0, 0x9CA760).
	TargetedObject* GetTargetedObject(uint32_t auiIndex);
	void UpdateCombatArea(bool abForceUpdate);
	void BoostConfidence(bool abChangeFleeStatus);
	bool CheckMovement(const BGSWorldLocation& apWorldLocation, bool abCheckUnreachable, bool abPassive);
	// Checks the movement to arPoint in apSpace.
	bool CheckMovement(const NiPoint3& arPoint, TESForm* apSpace, bool abCheckLocationReachable,
		bool abIgnoreCombatAreaCheck);
	// Whether arCoverPoint is safe from apTarget in arCoverLocation: beyond the view cone, beyond melee reach, or
	// hidden by the cover's ledge. The exe tests the whole cone byte (thiscall, ret 0x10, 0x99CCD0).
	bool CheckCoverSafe(const PathingCoverLocation& arCoverLocation, const NiPoint3& arCoverPoint, Actor* apTarget,
		uint8_t abUseViewCone);
	// Whether the ledge of arCoverLocation hides arCoverPoint from apTarget at arTargetLocation: the cover's height
	// over the target's line is at least the attacker's own height.
	bool CheckLedgeCoverAngle(const PathingCoverLocation& arCoverLocation, const NiPoint3& arCoverPoint,
		const NiPoint3& arTargetLocation, Actor* apTarget);
	CombatTimeStamp GetLastTarget360LineOfSightTimeStamp();
	CombatTimeStamp GetLastAttackTimeStamp();
	CombatTimeStamp GetLastDamagedByAttackerTimeStamp();
	// Clears the combat weapons and the embedded weapons, keeping the array's buffer.
	void DeleteInventoryData();
	// Finishes the package task when apRef is the combat item being fetched, then clears apRef.
	void PreActivateReference(TESObjectREFR* apRef);
	// Clears the references to apRef.
	void ClearReferencePointer(TESObjectREFR* apRef, bool abSelf);
	void SetSecondaryCoverLocation(CombatCoverLocation* apCombatCoverLocation);
	void SetSecondaryCoverSearchInfo(CombatCoverSearchInfo* apCoverSearchInfo);
	// Records arWorldLocation as unreachable, stamped with the current combat time.
	void AddUnreachableLocation(const BGSWorldLocation& arWorldLocation);
	// The exe calls that shared +0x7C getter.
	ACTOR_SEGMENT_INVIEW GetTargetSegmentInView() { return eTargetSegmentInView; }
	// The exe calls that shared +0x78 setter (thiscall, ret 4).
	void SetExplosiveProjectileBlocked(BGSExplosion* apExplosion) { pExplosiveProjectileBlocked = apExplosion; }
	double GetMaxDPSForWeaponType(COMBAT_WEAPON_TYPE aeType);
	void ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel);
	void UpdateCombatStrength();
	float GetCombatStrength() { return fCombatStrength; }
	bool GetTargetUnreachable();
	void ClearTargetUnreachable();
	void CheckReferences();
	void ClearCovers();
	bool GetLastSeenTargetFiringLocation(NiPoint3& arLastSeenFiringLocation);
	float GetLocationTargetNearTargetDistance();
	// The location target's maximum distance applies with a target that is detected or searched for, or without a
	// target to a reference location target.
	bool CheckUseLocationTargetMaxDistance();
	// Whether the confidence, less the modifier when positive, exceeds the threat ratio while there is a target.
	bool CheckShouldFlee();
	bool GetTargetFlushed();
	bool TryReserveCoverLocation(CombatCoverLocation* apCoverLocation);
	// bBlocking; the exe's shared one-byte getter of +0x0 (thiscall, ret 0, 0x9373F0, registered as
	// BSNetworkSystemUtility::QOnline).
	bool GetBlocking();
	void ReachedDestination(const BGSWorldLocation& arDestination, int32_t aiLocation);
	double GetLocationTargetMinDistance();
	TESObjectWEAP* GetBestCombatWeapon();
	bool IsDetectionLost();
	void SetLocationTargetLocation(BGSWorldLocation* a1, uint8_t a2, float a3, float a4);
	void UpdateCombatPermissions();
	bool CheckShouldUpdateDetection(Actor* apAttacker, Actor* apTarget);
	void ActivatedReference(TESObjectREFR* apRef, TESBoundObject* apObject, int32_t aiCount,
		bool abActivatedScriptOnly);
	void UsedCombatItem(TESBoundObject* apCombatItem);
	bool FindGrenadeThrowLocation(CombatLocation* apRetLocation, bool& arbHighArc, TESObjectWEAP* apGrenade);
	bool FindBestCoverLocation(TESObjectWEAP* apWeapon, CombatCoverSearchInfo* apCoverSearchInfo,
		CombatCoverLocation* apCombatCoverLocation, CombatLocation* apRetLocation, int32_t aiLocation);
	void BuildTargetLOSLocationsArray(BSSimpleArray<NiPoint3, 1024>* apLocationsArray);
	bool FindBestAdvanceLocation(CombatCoverSearchInfo* apCoverSearchInfo, CombatCoverLocation* apCombatCoverLocation,
		CombatLocation* apRetLocation, int32_t aiLocation, const NiPoint3& arDestination, float afMinDistance,
		PathingAvoidNodeArray* apPathingAvoidNodeArray);
	void BuildCoveredMoveRequest(PathingRequestCoveredMove* apCoveredMoveRequest);
	bool FindCoveredMoveLocation(PathingCoverLocation* apCoverLocation, const NiPoint3& arDestination,
		uint32_t aiLocation, float afMinDistance, PathingAvoidNodeArray* apPathingAvoidNodeArray);
	bool FindOptimalLocation(float afMinRange, float afMaxRange, float afAbsoluteMinRange, float afAbsoluteMaxRange,
		CombatLocation* apRetLocation, bool abCheckTargetLOS);
	// The exe returns 1 in al on every path (`mov al, 1`); its callers test or store al.
	bool FindBestFleeLocation(CombatCoverLocation* arg0, CombatLocation* arg4, int32_t arg8);
	bool FindLOSLocation(CombatLocation* apRetLocation, int32_t a3);
	bool FindBetterWeapon();
	bool IsCoverLocationValid(CombatCoverLocation* apCoverLocation);
	void AddObjectToTarget(uint32_t auiIndex, TESObjectREFR* apReference, TESBoundObject* apObject);
	double GetLocationTargetDistance();
	float GetLocationTargetCheckMovementDistance();
	void AddUnreachableCoverLocation(uint32_t aiCoverKey);
	void BuildUnreachableCoverLocationsArray(BSSimpleArray<uint32_t, 1024>* apUnreachableCoverLocationsArray);
	bool IsLocationUnreachable(const BGSWorldLocation& arWorldLocation, float afRadius);
	bool QueuePathCheck(int32_t aiLocation);
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// The time stamp firing started at (thiscall, ret 4, the result through a hidden pointer, 0x9D1B70).
	CombatTimeStamp GetFiringStartedTimeStamp();
	void ClearLastDodgeDirection();
	bool TryReserveSecondaryCoverLocation();
	bool GetLocationUnreachable(int32_t aiLocation);
	// Clears the bits aiLocation of iLocationUnreachable.
	void ClearLocationUnreachable(uint32_t aiLocation);
	void SetLocationTargetRef(TESObjectREFR* apRef, LOCATION_TARGET_TYPE aeType, float afMin, float afMax);

	// Whether arCoverPathingLocation has a line of sight to arTargetLocation.
	static bool CheckCoverLOS(PathingLocation& arCoverPathingLocation, const NiPoint3& arTargetLocation,
		bool abSkipDistanceCheck);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static bool CheckTargetsCoverLOS_0(PathingLocation& arCoverPathingLocation,
		BSSimpleArray<NiPoint3, 1024>* apLocationsArray);
	static bool FindBestAdvanceLocation(PathingRequestCoveredMove* apCoveredMoveRequest, float afMinDistance,
		bool abBackgroundThread);

	bool bBlocking;
	uint8_t padding01[3];
	uint32_t iLocationUnreachable;
	float fThreatRatio;
	TESObjectWEAP* pCombatWeapons[6];
	BSSimpleArray<TESObjectWEAP*> EmbeddedWeaponsArray;
	TESObjectWEAP* pCurrentEmbeddedWeapon;
	uint32_t iHasCombatWeapon;
	// One entry per COMBAT_WEAPON_TYPE, then (+0x54) the DPS without a weapon: UpdateInventoryData clears all seven
	// (0x1C bytes) and stores CalcCombatWeaponDPS with no weapon in the last, whose iHasCombatWeapon bit is 1 << 6.
	float fCombatWeaponsDPS[7];
	float fMaxCombatWeaponDPS;
	float fMaxRangedWeaponDPS;
	float fMaxModifiedCombatWeaponDPS;
	float fDisarmedDPS;
	float fBetterWeaponOutOfRangeDistance;
	int32_t iTargetDetectionLevel;
	bool bTargetLineOfSight;
	bool bTarget360LineOfSight;
	bool bTargetBlocked;
	char cTargetFlushed;
	bool bTargetSpaceDifferent;
	bool bHasHandToHandEquip;
	bool bGrenadeThrowHighArc;
	bool bInventoryDirty;
	BGSExplosion* pExplosiveProjectileBlocked;
	ACTOR_SEGMENT_INVIEW eTargetSegmentInView;
	float fThreatScore;
	NiPoint3 LastSeenTargetLocation;
	uint32_t iCombatPermissions;
	CombatTimeStamp fLastTargetDetectedTimeStamp;
	CombatTimeStamp fLastTarget360LineOfSightTimeStamp;
	CombatTimeStamp fLastAttackTimeStamp;
	CombatTimeStamp fLastDamagedByAttackerTimeStamp;
	CombatTimeStamp fFiringStartedTimeStamp;
	CombatTimer AttackDialogueTimer;
	CombatTimer FireExplosiveDialogueTimer;
	CombatTimer FleeDialogueTimer;
	CombatTimeStamp fThreatsConsideredTimeStamp;
	CombatLocationTarget* pLocationTarget;
	// The PDB's char (the `c` prefix); the exe stores the low byte of the CONFIDENCE actor value in it
	// (DoInitialUpdate 0x997C8A), not a 0 or 1.
	uint8_t cStartingConfidence;
	// The PDB's unsigned char; the exe compares it unsigned (0x9A1060).
	uint8_t cCurrentConfidence;
	uint8_t paddingCA[2];
	float fConfidenceModifier;
	float fCombatStrength;
	float fDamagePerSecond;
	uint8_t cFleeState;
	uint8_t paddingD9[3];
	uint32_t iLastFleeDoor;
	CombatTimeStamp fLastFleeDoorTimeStamp;
	CombatCoverLocation* pCoverLocation;
	CombatCoverLocation* pSecondaryCoverLocation;
	CombatCoverSearchInfo CoverSearchInfo;
	CombatCoverSearchInfo SecondaryCoverSearchInfo;
	BSSimpleArray<UnreachableCoverLocation> UnreachableCoverLocations;
	BSSimpleArray<UnreachableLocation> UnreachableLocations;
	// The exe calls CombatTimeStamp::ElapsedTime (0x435E00) on it.
	CombatTimeStamp fLastDodgeSearch;
	// The PC type identifies this byte as bool, but the exe reads it with movsx and compares it with -1: a char.
	int8_t cLastDodgeDirection;
	uint8_t padding189[3];
	CombatLocation* pCombatLocation;
	TargetedObject* pCombatItems[5];
	TESBoundObject* pUsableCombatItems[2];
	CombatTimer UsableCombatItemTimers[2];
	uint32_t iHasCombatItem;
	Actor* pLastTarget;
	CombatController* pCombatController;
	bool bInitializing;
	uint8_t padding1C9[3];
	CombatTimer TargetUnreachableTimer;
	CombatTimer FindBetterWeaponTimer;
	CombatTimer ExplosiveProjectileBlockedTimer;
	CombatTimer CombatPermissionsTimer5sec;
	CombatTimer CombatPermissionsTimer10sec;
	CombatTimer GuardSneakTimer;
	CombatTimer DynamicCoverLostTimer;
	CombatTimer CombatStrengthUpdateTimer;
	CombatTimer ThreatRatioUpdateTimer;
	CombatTimer EmbeddedWeaponUpdateTimer;
	CombatTimer UpdateInventoryTimer;
	NiPointer<ActorPathingMessageQueue> spPathCheckQueue;
	int32_t iQueuedPathCheck;
};

// A cover spot the combat code tracks for an actor: where it is, how high, and who has reserved it.
class CombatCoverLocation {
public:
	CombatCoverLocation();
	~CombatCoverLocation();

	// The exe calls that shared one-field getter.
	bool IsReserved() { return bReserved; }
	void Copy(const CombatCoverLocation* apCopy);
	// Folded accessor, IDA: PackageSchedule::SetStartDayOfWeek.
	void SetReserved(bool abValue);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// IDA name BGSSaveGameBuffer::sub_9A2DE0: the PDB's SaveGame, which writes the PDB's
	// fields in order (thiscall, ret 4; CombatState::SaveGame passes its buffer).
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	bool HasFireLocation();
	bool IsLedgeCover();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	bool bDirtyFlag;
	bool bReserved;
	uint8_t cCoverHeight;
	// IDA: _BYTE byte3.
	uint8_t padding03;
	int32_t iLocation;
	NiPoint3 CoverSearchTargetLocation;
	PathingCoverLocation CoverLocation;
	uint32_t iCoverLocationKey;
	uint32_t iCoverLocationLastMovedTimeStamp;
	CoverReservationInfo ReservationInfo;
	// IDA: UInt16 unk76.
	uint8_t padding76[2];
};

// A world location combat found unreachable, with when it was found.
class UnreachableLocation {
public:
	// The default constructor.
	UnreachableLocation();

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	BGSWorldLocation Location;
	CombatTimeStamp fTimeStamp;
};

// The game settings CombatState.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> iMaxHiPerfNPCTargetCount;
extern SettingT<INISettingCollection> fHiPerfNPCTargetLOSTimer;
extern SettingT<INISettingCollection> fHiPerfPCTargetLOSTimer;
extern SettingT<INISettingCollection> iMaxHiPerfPCTargetCount;
extern SettingT<INISettingCollection> fLowPerfPCTargetLOSTimer;
extern SettingT<INISettingCollection> fLowPerfNPCTargetLOSTimer;
extern BSPerformanceTimer CombatAIAdvanceSearch;

// GameSettingCollection: 0xf5dfd2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fDisarmedPickupWeaponDistanceMult;

// IDA: BSPerfTimer::CombatAI::CombatItemSearch; the PDB's CombatAICombatItemSearch.
extern BSPerformanceTimer CombatAI_CombatItemSearch;

// IDA: InitPerformanceTimer::CombatAI::CoverSearch constructs it (0xFABCD0); the PDB's CombatAICoverSearch.
extern BSPerformanceTimer CombatAI_CoverSearch;
