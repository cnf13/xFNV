#pragma once

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatCoverSearch.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/UnreachableCoverLocation.h"
#include "Fallout/AI/ProcessArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

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
class UnreachableLocation;

enum ACTOR_SEGMENT_INVIEW : int32_t;

double CalculateCostForDistance(CombatController* apCombatController, BGSWorldLocation* apFrom, BGSWorldLocation* apTo);

// What a combat location target is for: the type CombatState::SetLocationTargetRef and SetLocationTarget take. It is
// passed and stored as one byte, so its underlying type is uint8_t.
enum LOCATION_TARGET_TYPE : uint8_t {
    LOCATION_TARGET_DESTINATION = 0,
    LOCATION_TARGET_GUARD = 1,
    LOCATION_TARGET_CLUSTER = 2,
    LOCATION_TARGET_COMBAT_AREA = 3,
    LOCATION_TARGET_COUNT = 4,
};

// One bit per combat location: 1 << COMBAT_VARIABLE.
enum LOCATION_UNREACHABLE_FLAGS : int32_t {
    UNREACHABLE_TARGET_LOCATION = 0x1,
    UNREACHABLE_NEAR_TARGET_LOCATION = 0x2,
    UNREACHABLE_COVER_LOCATION = 0x4,
    UNREACHABLE_DODGE_LOCATION = 0x8,
    UNREACHABLE_FLANK_LOCATION = 0x10,
    UNREACHABLE_FALLBACK_LOCATION = 0x20,
    UNREACHABLE_ADVANCE_LOCATION = 0x40,
    UNREACHABLE_DESTINATION_LOCATION = 0x80,
    UNREACHABLE_GUARD_LOCATION = 0x100,
    UNREACHABLE_CLUSTER_LOCATION = 0x200,
    UNREACHABLE_HIDE_LOCATION = 0x400,
    UNREACHABLE_GRENADE_LOCATION = 0x800,
    UNREACHABLE_FLEE_LOCATION = 0x1000,
    UNREACHABLE_LOS_LOCATION = 0x2000,
    UNREACHABLE_SAFE_LOCATION = 0x4000,
    UNREACHABLE_INVESTIGATE_LOCATION = 0x8000,
    UNREACHABLE_COMBAT_AREA_LOCATION = 0x10000,
};

// Why a cover location was rejected, or that it was selected (CombatState::CheckCoverLocation's result).
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

// Where a combat state is heading: a reference and a world location, and how near to the target it should get.
class CombatLocationTarget {
public:
    CombatLocationTarget();

    // Saves cType, bNearTarget, fMinDistance, fMaxDistance, pRef's form ID, then WorldLoc.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

    // Resolves the stored target form ID to the reference once the game is loaded; the buffer is unused.
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

    // Declared unsigned because the value is read zero-extended.
    uint8_t cType;
    bool bNearTarget;
    TESObjectREFR* pRef;
    BGSWorldLocation WorldLoc;
    float fMinDistance;
    float fMaxDistance;
};

class CombatState {
public:
    using LocationTarget = CombatLocationTarget;

    // A combat item and its form.
    struct TargetedObject {
        // Clears pRef, then pObject.
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
    bool FindBestDodgeLocation(
        CombatLocation* apRetLocation,
        uint32_t aiDodgeDirections,
        uint32_t aiDodgeDirectionLOSCheck,
        float afDodgeDistance,
        float afMinDodgeDistance
    );
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
    bool CheckCoverLocation(
        PathingCoverLocation& arCoverLocation,
        CombatLocation* apRetLocation,
        CoverReservationInfo* apReservationInfo,
        NiPoint3& arCoverSearchTargetLocation,
        uint32_t aiCoverKey,
        uint32_t aiCurrentCoverLocationKey,
        int32_t aiLocation,
        Actor* apAttacker,
        Actor* apTarget,
        float afMinRange,
        float afMaxRange,
        COVER_STATE& areCoverState,
        float& arfSquaredDistance,
        bool abHideCover
    );
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
    void ReloadingWeapon(TESObjectWEAP* apWeapon, bool abForced);
    void SetCoverLocation(CombatCoverLocation* apCombatCoverLocation);
    void ClearCombatLocation();
    void ClearLocationTarget();
    CombatTimer* GetUsableCombatItemTimer(int32_t aiCombatItem);
    void SetTargetFlushed(bool abValue);
    bool GetPermission(uint32_t aiPermission);
    // auiIndex indexes pCombatItems and is unsigned.
    void RemoveTargetedObject(uint32_t auiIndex);
    bool ShouldUseMelee();
    void SetFlag(int32_t aiFlag);

    TESObjectWEAP* GetWeaponByCombatType(COMBAT_WEAPON_TYPE aeType);
    // Stamps the time of the last attack.
    void PerformedMeleeAttack(Actor* apTarget);
    // Stamps the attack, and the firing start when it is unset (-FLT_MAX).
    void FiredProjectile(Projectile* apProjectile);
    CombatTimeStamp GetLastTargetDetectedTimeStamp();
    bool GetHasBufferedDetection();
    void ClearSecondaryCoverSearchInfo();
    void ClearCoverSearchInfo();
    // Reserves the combat state's own cover location.
    void ReserveCoverLocation();
    // Clears the combat location and sets apCombatLocation.
    void SetCombatLocation(CombatLocation* apCombatLocation);
    void ClearSecondaryCoverLocation(bool abClearLocation);
    // Callers test the index as unsigned, against 5.
    TargetedObject* GetTargetedObject(uint32_t auiIndex);
    void UpdateCombatArea(bool abForceUpdate);
    void BoostConfidence(bool abChangeFleeStatus);
    bool CheckMovement(const BGSWorldLocation& apWorldLocation, bool abCheckUnreachable, bool abPassive);
    // Checks the movement to arPoint in apSpace.
    bool CheckMovement(
        const NiPoint3& arPoint,
        TESForm* apSpace,
        bool abCheckLocationReachable,
        bool abIgnoreCombatAreaCheck
    );
    // Whether arCoverPoint is safe from apTarget in arCoverLocation: beyond the view cone, beyond melee reach, or
    // hidden by the cover's ledge. Tests the whole abUseViewCone byte.
    bool CheckCoverSafe(
        const PathingCoverLocation& arCoverLocation,
        const NiPoint3& arCoverPoint,
        Actor* apTarget,
        uint8_t abUseViewCone
    );
    // Whether the ledge of arCoverLocation hides arCoverPoint from apTarget at arTargetLocation: the cover's height
    // over the target's line is at least the attacker's own height.
    bool CheckLedgeCoverAngle(
        const PathingCoverLocation& arCoverLocation,
        const NiPoint3& arCoverPoint,
        const NiPoint3& arTargetLocation,
        Actor* apTarget
    );
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
    ACTOR_SEGMENT_INVIEW GetTargetSegmentInView() {
        return eTargetSegmentInView;
    }
    void SetExplosiveProjectileBlocked(BGSExplosion* apExplosion) {
        pExplosiveProjectileBlocked = apExplosion;
    }
    double GetMaxDPSForWeaponType(COMBAT_WEAPON_TYPE aeType);
    void ChangeProcessLevel(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel);
    void UpdateCombatStrength();
    float GetCombatStrength() {
        return fCombatStrength;
    }
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
    bool GetBlocking();
    void ReachedDestination(const BGSWorldLocation& arDestination, int32_t aiLocation);
    double GetLocationTargetMinDistance();
    TESObjectWEAP* GetBestCombatWeapon();
    bool IsDetectionLost();
    void SetLocationTargetLocation(
        BGSWorldLocation* apWorldLocation,
        uint8_t acType,
        float afMinDistance,
        float afMaxDistance
    );
    void UpdateCombatPermissions();
    bool CheckShouldUpdateDetection(Actor* apAttacker, Actor* apTarget);
    void
    ActivatedReference(TESObjectREFR* apRef, TESBoundObject* apObject, int32_t aiCount, bool abActivatedScriptOnly);
    void UsedCombatItem(TESBoundObject* apCombatItem);
    bool FindGrenadeThrowLocation(CombatLocation* apRetLocation, bool& arbHighArc, TESObjectWEAP* apGrenade);
    bool FindBestCoverLocation(
        TESObjectWEAP* apWeapon,
        CombatCoverSearchInfo* apCoverSearchInfo,
        CombatCoverLocation* apCombatCoverLocation,
        CombatLocation* apRetLocation,
        int32_t aiLocation
    );
    void BuildTargetLOSLocationsArray(BSSimpleArray<NiPoint3, 1024>* apLocationsArray);
    bool FindBestAdvanceLocation(
        CombatCoverSearchInfo* apCoverSearchInfo,
        CombatCoverLocation* apCombatCoverLocation,
        CombatLocation* apRetLocation,
        int32_t aiLocation,
        const NiPoint3& arDestination,
        float afMinDistance,
        PathingAvoidNodeArray* apPathingAvoidNodeArray
    );
    void BuildCoveredMoveRequest(PathingRequestCoveredMove* apCoveredMoveRequest);
    bool FindCoveredMoveLocation(
        PathingCoverLocation* apCoverLocation,
        const NiPoint3& arDestination,
        uint32_t aiLocation,
        float afMinDistance,
        PathingAvoidNodeArray* apPathingAvoidNodeArray
    );
    bool FindOptimalLocation(
        float afMinRange,
        float afMaxRange,
        float afAbsoluteMinRange,
        float afAbsoluteMaxRange,
        CombatLocation* apRetLocation,
        bool abCheckTargetLOS
    );
    // Returns true on every path; its callers test or store the result.
    bool
    FindBestFleeLocation(CombatCoverLocation* apCombatCoverLocation, CombatLocation* apRetLocation, int32_t aiLocation);
    bool FindLOSLocation(CombatLocation* apRetLocation, int32_t aiLocation);
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
    // The time stamp firing started at.
    CombatTimeStamp GetFiringStartedTimeStamp();
    void ClearLastDodgeDirection();
    bool TryReserveSecondaryCoverLocation();
    bool GetLocationUnreachable(int32_t aiLocation);
    // Clears the bits aiLocation of iLocationUnreachable.
    void ClearLocationUnreachable(uint32_t aiLocation);
    void SetLocationTargetRef(TESObjectREFR* apRef, LOCATION_TARGET_TYPE aeType, float afMin, float afMax);

    // Whether arCoverPathingLocation has a line of sight to arTargetLocation.
    static bool
    CheckCoverLOS(PathingLocation& arCoverPathingLocation, const NiPoint3& arTargetLocation, bool abSkipDistanceCheck);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static bool
    CheckTargetsCoverLOS_0(PathingLocation& arCoverPathingLocation, BSSimpleArray<NiPoint3, 1024>* apLocationsArray);
    static bool FindBestAdvanceLocation(
        PathingRequestCoveredMove* apCoveredMoveRequest,
        float afMinDistance,
        bool abBackgroundThread
    );

    bool bBlocking;
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
    // Holds the low byte of the CONFIDENCE actor value (set in DoInitialUpdate), not a 0 or 1.
    uint8_t cStartingConfidence;
    // Compared as unsigned.
    uint8_t cCurrentConfidence;
    float fConfidenceModifier;
    float fCombatStrength;
    float fDamagePerSecond;
    uint8_t cFleeState;
    uint32_t iLastFleeDoor;
    CombatTimeStamp fLastFleeDoorTimeStamp;
    CombatCoverLocation* pCoverLocation;
    CombatCoverLocation* pSecondaryCoverLocation;
    CombatCoverSearchInfo CoverSearchInfo;
    CombatCoverSearchInfo SecondaryCoverSearchInfo;
    BSSimpleArray<UnreachableCoverLocation> UnreachableCoverLocations;
    BSSimpleArray<UnreachableLocation> UnreachableLocations;
    CombatTimeStamp fLastDodgeSearch;
    // Declared as a signed char rather than bool because it is read sign-extended and compared with -1.
    int8_t cLastDodgeDirection;
    CombatLocation* pCombatLocation;
    TargetedObject* pCombatItems[5];
    TESBoundObject* pUsableCombatItems[2];
    CombatTimer UsableCombatItemTimers[2];
    uint32_t iHasCombatItem;
    Actor* pLastTarget;
    CombatController* pCombatController;
    bool bInitializing;
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

    bool IsReserved() {
        return bReserved;
    }
    void Copy(const CombatCoverLocation* apCopy);
    void SetReserved(bool abValue);
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Writes the fields in declaration order; CombatState::SaveGame passes its buffer.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    bool HasFireLocation();
    bool IsLedgeCover();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    bool bDirtyFlag;
    bool bReserved;
    uint8_t cCoverHeight;
    int32_t iLocation;
    NiPoint3 CoverSearchTargetLocation;
    PathingCoverLocation CoverLocation;
    uint32_t iCoverLocationKey;
    uint32_t iCoverLocationLastMovedTimeStamp;
    CoverReservationInfo ReservationInfo;
};

// A world location combat found unreachable, with when it was found.
class UnreachableLocation {
public:
    UnreachableLocation();

    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    BGSWorldLocation Location;
    CombatTimeStamp fTimeStamp;
};

// The game settings CombatState.cpp defines.
extern SettingT<INISettingCollection> iMaxHiPerfNPCTargetCount;
extern SettingT<INISettingCollection> fHiPerfNPCTargetLOSTimer;
extern SettingT<INISettingCollection> fHiPerfPCTargetLOSTimer;
extern SettingT<INISettingCollection> iMaxHiPerfPCTargetCount;
extern SettingT<INISettingCollection> fLowPerfPCTargetLOSTimer;
extern SettingT<INISettingCollection> fLowPerfNPCTargetLOSTimer;
extern BSPerformanceTimer CombatAIAdvanceSearch;

extern SettingT<GameSettingCollection> fPickupWeaponDisarmedDistanceMult;

extern BSPerformanceTimer CombatAI_CombatItemSearch;

extern BSPerformanceTimer CombatAI_CoverSearch;
