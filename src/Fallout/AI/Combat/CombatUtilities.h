#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESAnimGroup.h"

#include <stdint.h>

class Actor;
class ActorValueOwner;
class BGSExplosion;
class BGSProjectile;
class BGSWorldLocation;
class BSPerformanceTimer;
class CombatController;
class CombatCoverLocation;
class CoverReservationInfo;
class ItemChange;
class NiAVObject;
class NiColorA;
class PathingCoverLocation;
class Projectile;
class TESAmmo;
class TESObjectREFR;
class TESObjectWEAP;

enum ACTOR_SEGMENT_INVIEW : int32_t;
enum COMBAT_WEAPON_TYPE : int32_t;

// The heading of a combat position slot: acSlot's share of 360 degrees over the slot count.
float GetAngleFromSlot(char acSlot);
// Whether combat slot acSlot is reserved in the slot mask aiSlots.
bool IsSlotReserved(uint32_t aiSlots, char acSlot);
// The slot acAmount slots from acSlot.
char RotateSlot(char acSlot, char acAmount);
char GetSlotFromAngle(float afAngle);
uint32_t ReserveSlot(uint32_t aiSlots, char acSlot);
char FindSlot(uint32_t aiSlots, char acDesiredSlot);
double NiATan(float afX);

// CombatUtilities has only static members.
class CombatUtilities {
public:
    static void Init();
    // Whether apTarget is within the strike angle of apAttacker; the angle between them, in
    // degrees, goes to afDelta when not null.
    static bool IsTargetWithinStrikeAngle(Actor* apAttacker, TESObjectREFR* apTarget, float* afDelta);
    // The actor apAttacker faces within afRadius.
    static Actor* GetFacingTarget(Actor* apAttacker, float afRadius);
    // The height offset the attacker fires from.
    static float GetAttackerFiringZOffset(TESObjectREFR* apAttacker, bool abIgnoreCrouch);
    // The point the attacker faces apTarget from.
    static NiPoint3 ComputeFaceTargetLocation(
        Actor* apAttacker,
        TESObjectREFR* apTarget,
        const NiPoint3& arTargetLocation,
        float afAttackerReach
    );
    static bool IsActorUsingMelee(Actor* apActor);
    // The damage per second of the weapon (with its condition afCondition and the inventory entry apItem) for apActor.
    static float CalculateItemDPS(
        ActorValueOwner* apActor,
        TESObjectWEAP* apWeapon,
        float afCondition,
        bool abApplyPerks,
        ItemChange* apItem,
        bool abAlwaysAddReload,
        bool abMaxJamChance,
        int32_t aiSkillOverride,
        float afCritChance,
        float afCritDamageMult,
        bool abDoubleDamage,
        bool abBonusDamage,
        TESAmmo* apAmmo
    );
    // The launch angle that carries a projectile at afSpeed afDeltaXY across and afDeltaZ up under afGravity (the
    // larger root when abLargeRoot), or pi/4 when no angle reaches.
    static float
    FindAngleOfProjectile(float afDeltaXY, float afDeltaZ, float afSpeed, float afGravity, bool abLargeRoot);
    static float GetDistanceBetweenFacingActors(TESObjectREFR* apAttacker, TESObjectREFR* apTarget, bool abMeleeCombat);
    // Whether arTargetLocation lies within afArcAngle of the attacker's aim and afLookingAngle
    // of its heading.
    static bool IsTargetWithinFiringArc(
        Actor* apAttacker,
        const NiPoint3& arTargetLocation,
        float afArcAngle,
        float afLookingAngle
    );
    static float GetAttackerTargetExtentSum(TESObjectREFR* apAttacker, TESObjectREFR* apTarget);
    // The reach of apWeapon in apAttacker's hands.
    static float GetWeaponReach(Actor* apAttacker, TESObjectWEAP* apWeapon);
    // The distance the attacker faces the target from: half the reach (the current weapon's when afAttackerReach
    // is -1) plus both extents, at least 0.
    static float ComputeFaceTargetDistance(Actor* apAttacker, TESObjectREFR* apTarget, float afAttackerReach);
    // The point apTarget is aimed at for the segment in view.
    static NiPoint3 GetTargetFiringLocation(TESObjectREFR* apTarget, ACTOR_SEGMENT_INVIEW aeTargetSegmentInView);
    // apTarget's character controller velocity, zero for a non-actor.
    static NiPoint3 GetTargetVelocity(TESObjectREFR* apTarget);
    // The angles to aim from aStartLocation to hit apTarget with a projectile of speed afSpeed and gravity afGravity
    // (the low arc); zero without a target.
    static NiPoint3 GetAngleToProjectedTarget(
        NiPoint3 aStartLocation,
        TESObjectREFR* apTarget,
        float afSpeed,
        float afGravity,
        ACTOR_SEGMENT_INVIEW aeTargetSegmentInView
    );
    // The same angles from apAttacker's own firing location at apTarget.
    static NiPoint3 GetAngleToProjectedTarget(
        TESObjectREFR* apAttacker,
        TESObjectREFR* apTarget,
        float afSpeed,
        float afGravity,
        ACTOR_SEGMENT_INVIEW aeTargetSegmentInView
    );
    // Where to aim from aStartLocation to hit a target moving at aTargetVelocity.
    static NiPoint3 GetProjectedPointForTarget(
        NiPoint3 aStartLocation,
        NiPoint3 aTargetLocation,
        NiPoint3 aTargetVelocity,
        float afProjectileSpeed,
        float afProjectileGravity,
        bool abHighArc
    );
    // Where apAttacker should aim apProjectile (at afProjectileSpeed) to hit apTarget, from apReleasePoint or the
    // attacker's own release point when null.
    static NiPoint3 GetProjectedPointForTarget(
        TESObjectREFR* apAttacker,
        TESObjectREFR* apTarget,
        NiPoint3* apReleasePoint,
        BGSProjectile* apProjectile,
        float afProjectileSpeed,
        bool abHighArc
    );
    // The aiming angles from aStartLocation to aProjectedPoint.
    static NiPoint3 GetAngleToProjectedPoint(NiPoint3 aStartLocation, NiPoint3 aProjectedPoint);
    // The same angles from apAttacker's own firing location to aProjectedPoint.
    static NiPoint3 GetAngleToProjectedPoint(TESObjectREFR* apAttacker, NiPoint3 aProjectedPoint);
    // Whether the throw of apProjectile from aStartLocation reaches aTargetedPoint; the hit
    // point, reference and distance fraction come back through the pointers.
    static bool CalculateProjectileLOS(
        Actor* apActor,
        BGSProjectile* apProjectile,
        float afProjectileSpeed,
        NiPoint3 aStartLocation,
        NiPoint3 aTargetedPoint,
        NiPoint3* apHitLocation,
        TESObjectREFR** apRefHit,
        float* apDistanceFraction
    );
    // The point to aim a projectile at for apTarget: the first of the two arcs the high-arc setting picks whose line of
    // sight passes the hit-fraction threshold, else the projected point.
    static bool CheckProjectileArc(
        Actor* apAttacker,
        Actor* apTarget,
        BGSProjectile* apProjectile,
        NiPoint3 aStartLocation,
        NiPoint3 aTargetLocation,
        NiPoint3 aTargetVelocity,
        bool& arbHighArc,
        NiPoint3& arTargetedPoint,
        float afMinHitFraction
    );
    // The point apAttacker fires from.
    static NiPoint3 GetAttackerFiringLocation(TESObjectREFR* apAttacker);
    // The range apProjectile reaches at afProjectileSpeed.
    static float GetProjectileMaximumRange(BGSProjectile* apProjectile, float afProjectileSpeed);
    // The combat ranges for apEquippedWeapon (512 and 1024 without one), scaled by the combat style unless the
    // weapon's range is fixed and capped by its projectile's range and explosion radius.
    static void ComputeOptimalWeaponRanges(
        CombatController* apCombatController,
        TESObjectWEAP* apEquippedWeapon,
        float& arfMinRange,
        float& arfMaxRange,
        float& arfAbsoluteMaxRange
    );
    // Draws a line-of-sight check for fDebugCombatProjectileLOSTime: a line from arStartLocation to the hit point
    // (afHitFraction of the way to arEndLocation when apHitObj is set) in apLOSColor (yellow by default), and with
    // a hit, a marker there and a line on to arEndLocation in apHitColor (red by default).
    static void AddLOSDebugGeomtry(
        NiPoint3& arStartLocation,
        NiPoint3& arEndLocation,
        NiAVObject* apHitObj,
        float afHitFraction,
        NiColorA* apLOSColor,
        NiColorA* apHitColor
    );
    static float GetCostForWeaponType(CombatController* apController, COMBAT_WEAPON_TYPE aeType);
    // The target's firing-height offset.
    static float
    GetTargetFiringZOffset(TESObjectREFR* apTarget, ACTOR_SEGMENT_INVIEW aeTargetSegmentInView, bool abIgnoreCrouch);
    // Calculates cover positions.
    static void CalculateCoverPositions(
        CombatController* apController,
        PathingCoverLocation* apCoverLocation,
        CoverReservationInfo* apCoverData,
        NiPoint3* apCoverPosition,
        NiPoint3* apFirePosition
    );
    // Whether apAttacker at arAttackerLocation sees apTarget at arTargetLocation;
    // aiCollisionLayer -1 uses the default layer.
    static bool CalculateLOS(
        Actor* apAttacker,
        TESObjectREFR* apTarget,
        const NiPoint3& arAttackerLocation,
        const NiPoint3& arTargetLocation,
        bool abIgnoreCrouch,
        int32_t aiCollisionLayer
    );
    static float GetWorldGravity();
    static float ComputeProjectileTime(float afDeltaXY, float afAngle, float afSpeed);
    static bool CalculateAttackerCombatPositions(Actor* apTarget, Actor* apAttacker);
    static float GetAngleForMeleeAttack(const ANIM_GROUP_ENUM eAttack);
    static char CalculateGrenadeThrow_(
        Actor* apActor,
        Actor* apTarget,
        BGSProjectile* apProjectile,
        NiPoint3* apReleasePoint,
        bool* isHighArc,
        NiPoint3* targetPointOut,
        float grenadeThrowHitFractionThreshold
    );
    static float FindAngleOfProjectile_0(Projectile* apProjectile, NiPoint3 akToTarget, float afSpeed, bool abHighArc);
    static double ComputeProjectileTime_(
        BGSProjectile* apProjectile,
        NiPoint3 aStartPoint,
        NiPoint3 aTargetedPoint,
        bool abHighArc,
        float afSpeed
    );
    static NiPoint3 GetAttackerFiringLocationSimple(TESObjectREFR* apAttacker);
    static NiPoint3 GetBackProjectedAttackerFiringLocation(TESObjectREFR* apAttacker);
    static NiPoint3 GetAngleToProjectedPoint(
        NiPoint3 aStartLocation,
        NiPoint3 aPivotLocation,
        float afPivotAngle,
        NiPoint3 aProjectedPoint
    );
    static void CalculateCoverPositions(
        CombatController* apCombatController,
        CombatCoverLocation* apCombatCoverLocation,
        NiPoint3* apCoverPosition,
        NiPoint3* apFirePosition
    );
    static void CalculateCoverPositions_1(
        float afActorRadius,
        PathingCoverLocation* apCoverLocation,
        CoverReservationInfo* apReservationInfo,
        NiPoint3* apCoverPosition,
        NiPoint3* apFirePosition
    );
    static bool CanActorExistInSpace(Actor* apActor, const BGSWorldLocation& arWorldLocation);
    static bool IsDangerousExplosion(BGSExplosion* apExplosion);
    // The projected aim point of apSource at akTargetLocation, for a target standing still: it passes its arguments on
    // to CombatUtilities::GetProjectedPointForTarget with a zero velocity.
    static NiPoint3 GetProjectedPointForTarget(
        Actor* apSource,
        const NiPoint3& akTargetLocation,
        NiPoint3* arSourcePosition,
        BGSProjectile* apProjectile,
        float afProjectileSpeed,
        bool abHighArc
    );
    static NiPoint3 GetProjectedPointForTarget(
        TESObjectREFR* apSource,
        NiPoint3 akTargetLocation,
        NiPoint3 akTargetVelocity,
        NiPoint3* arSourcePosition,
        BGSProjectile* apProjectile,
        float afProjectileSpeed,
        bool abHighArc
    );

    static inline float fWorldGravity;
    // GetAngleToProjectedPoint moves a projected point nearer the pivot than this out to this distance.
    static inline float fMinProjectedPointDistance = 1024.0f;
};

// The number of combat position slots (30).
inline int32_t MAX_SLOTS = 30;

// The game settings CombatUtilities.cpp defines.
extern SettingT<INISettingCollection> fDebugCombatProjectileLOSTime;
extern SettingT<INISettingCollection> bDebugCombatTextColorDark;
extern SettingT<INISettingCollection> bDebugCombatProjectileLOS;
extern SettingT<INISettingCollection> fDebugCombatTextSize;
extern SettingT<INISettingCollection> fLowPerfCombatantVoiceDistance;
inline int32_t SLOT_BORDER = 2;

extern BSPerformanceTimer CombatAI_LOS;
