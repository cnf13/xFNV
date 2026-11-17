#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

class Actor;
class BGSLoadFormBuffer;
class BGSProjectile;
class BGSSaveFormBuffer;
class bhkCharacterController;
class bhkPickData;
class CombatController;
class ProjectilePickCache;
class ProjectileTarget;
class TESFaction;
class TESObjectCELL;
class TESObjectWEAP;

// The pick-cache object a projectile's launch may take. It is held through Projectile::spPickCache.
class ProjectilePickCache : public NiRefObject {
public:
    char* GetCache() {
        return pCache;
    }

    char* pCache;
};

// A projectile in flight. Only some of its entry points are declared.
class Projectile : public MobileObject {
public:
    enum ProjectileFlags : int32_t {
        HITSCAN_MODE = 0x1,
        NO_3D = 0x2,
        NO_CHARCONTROLLER = 0x4,
        NO_COLLISION = 0x8,
        TRACER = 0x10,
        FADING = 0x20,
        REORIENT = 0x40,
        POST_PROCESS = 0x80,
        CHECK_BEHIND_WALL = 0x100,
        TURNED_OFF = 0x200,
        ACTIVE_IN_INVENTORY = 0x400,
        INVENTORY_FREAKOUT = 0x800,
        ALWAYS_HIT_TARGET = 0x1000,
        SPAWN_EXPLOSION_AT_IMPACT = 0x2000,
        NO_PLAYER_PROXIMITY = 0x4000,
        KILL_ON_EXPLOSION = 0x8000,
        PROJECTILE_INITIALIZED = 0x10000,
        FAKE_SHOT = 0x20000,
        WHOLE_BODY_TARGETED = 0x40000,
        SHOOTER_IN_COMBAT = 0x80000,
        DETONATE = 0x100000,
    };

    enum PROJECTILE_TYPE : int32_t {
        PROJECTILE_TYPE_UNKNOWN = 0,
        PROJECTILE_TYPE_BEAM = 1,
        PROJECTILE_TYPE_FLAME = 2,
        PROJECTILE_TYPE_GRENADE = 3,
        PROJECTILE_TYPE_MISSILE = 4,
        PROJECTILE_TYPE_CONTINUOUS_BEAM = 5,
    };

    // AddImpact is protected, yet ProjectileListener::contactPointAddedCallback calls it on its projectile, so the
    // listener is a friend.
    friend class ProjectileListener;

    // One thing the projectile hit.
    struct ImpactData {
        ImpactData();
        ~ImpactData();

        // Resolves the loaded form IDs.
        void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
        void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
        // Loads the impact; the collidee pointer field holds the loaded form ID until it is resolved.
        void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
        // Resolves the collidee's form ID and marks it targeted.
        void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

        static void operator delete(void* apMemory);

        // The reference hit; it is compared with TESObjectREFR pointers.
        TESObjectREFR* pCollidee;
        NiPoint3 kLocation;
        NiPoint3 kNormal;
        // The Havok body hit: a reference is added to it, its collidable is read and it is mapped to its bhk wrapper.
        hkpWorldObject* pTargetWorldObject;
        HavokMaterialType eMaterial;
        // The limb that took the damage.
        BGSBodyPart::LIMB_ENUM eDamageLimb;
        bool bProcessed;
        bool bDoDamage;
        int16_t sTargetWorldObjectCount;
        int16_t sTargetWorldObjectIndex;
    };

    Projectile(
        BGSProjectile* apProjectileBase,
        TESObjectREFR* apShooter,
        TESObjectWEAP* apFromWeapon,
        NiPoint3 aOrigin,
        float afZAngle,
        float afXAngle
    );
    explicit Projectile(bool abAdd);

    bool Load(TESFile* apFile) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void SetActorCause(ActorCause* apActorCause) override;
    ActorCause* GetActorCause() override;
    void InitHavok() override;
    void MoveHavok(bool abForceRec) override;
    bool IsProjectile() override;
    bool MovetoHigh() override;
    bool MovetoLow() override;
    bool MovetoMiddleLow() override;
    bool MovetoMiddleHigh() override;
    void Process(float afDelta) override {}
    bool ChangeProcessLevel() override;
    bool IsTransient() override;
    void Update(float afDelta) override;
    void CleanUpPointersOnDisable() override;

    using MobileObject::FinishLoadGame;
    using MobileObject::InitLoadGame;
    using MobileObject::LoadGame;
    using MobileObject::Revert;
    using MobileObject::SaveGame;

    // The projectile's kind; Projectile's own implementation returns 0.
    virtual Projectile::PROJECTILE_TYPE GetProjectileType();
    // Sets the projectile up once its 3D is loaded.
    virtual void Initialize();
    virtual void Process3D() {}
    virtual void UpdateProjectile(float afTime) = 0;
    virtual bool ProcessImpacts();
    virtual bool IsMine();
    virtual void ReportHavokDeactivation() {}
    virtual bool TurnOff(Actor* apActionActor, bool abSilent);
    // Whether the projectile was placed in the editor.
    virtual bool IsPermanent();
    virtual bool RunTargetPick();

protected:
    virtual bool AddImpact(
        TESObjectREFR* apCollidee,
        const NiPoint3& aContactPoint,
        const NiPoint3& aContactNormal,
        const hkpCollidable* apTargetCollidable,
        uint32_t aiKey,
        HavokMaterialType aeForceMaterial
    );

public:
    ~Projectile() override;

    // Syncs the 3D with the reference.
    void Sync3DWithReference(bool abSyncRotation, bool abDoArcReorientation);
    // The speed of the projectile: its base form's speed through ComputeSpeed.
    float GetSpeed();
    // Moves the projectile by aDeltaMove over afDeltaTime, then records the distance moved.
    void Move(float afDeltaTime, NiPoint3 aDeltaMove);
    // Moves the projectile forward (along its local y axis) by its speed over afDeltaTime.
    void Move(float afDeltaTime);
    NiPoint3 GetMovementDirection();
    // Ends the projectile.
    void Kill();
    float GetExplosionTimer();
    TESObjectREFR* GetShooter();
    // The projectile's base form.
    BGSProjectile* GetProjectileBase();
    // fDistanceMoved.
    float GetDistanceMoved() {
        return fDistanceMoved;
    }
    float GetAge();
    void CheckExplosion(float afDelta);
    // Takes the heading and looking angles of apRotation.
    void SetRotation(const NiMatrix3* apRotation);
    ProjectilePickCache* GetPickCache();
    void SetPickCache(ProjectilePickCache* apCache);
    // Sets fPower.
    void SetPower(float afValue);
    void SetProjectileTarget(ProjectileTarget* apObject) {
        pTarget = apObject;
    }

protected:
    // Stores aDelta as the movement direction, then unitizes it; abAddDistance also adds its length to
    // fDistanceMoved.
    void SetMovementDelta(NiPoint3 aDelta, bool abAddDistance);

public:
    // Whether any of the flags in aeFlag is set.
    bool IsFlagSet(ProjectileFlags aeFlag);
    // The pass after the cell update; true when the projectile is done with the
    // post-process list.
    bool PostProcess(float afDelta);
    float GetAlpha();
    void SetFlag(Projectile::ProjectileFlags aeFlag, bool abValue);
    // Whether the projectile is a form from a plugin rather than one created at run time.
    bool IsEditorCreated();
    // Dims the projectile's light with its alpha while it fades out.
    void UpdateLight();
    float GetRange();
    void SetRange(float afRange);
    // Pushes the body of apTargetCollidable at aContactPoint along aDirection with the base form's force, scaled for
    // gameplay.
    void ApplyImpactForce(NiPoint3 aContactPoint, NiPoint3 aDirection, const hkpCollidable* apTargetCollidable);
    // Picks the water along apPickData's ray; afWaterHitRatio receives the hit fraction.
    bool PickAutoWater(bhkPickData* apPickData, float& afWaterHitRatio);
    // For a player's projectile, picks from the player's eye to the
    // projectile; on a target hit the range drops to zero. True on a hit.
    bool CheckShotBehindWall();
    TESObjectREFR* GetDesiredTarget();
    void SetExplosionTimer(float afExplosionTimer);
    void UpdateInstant();
    ItemChange* GetSecondaryItemChange();
    // Stores pSecondaryItemChange.
    void SetSecondaryItemChange(ItemChange* apObject) {
        pSecondaryItemChange = apObject;
    }
    // pWeaponSource.
    TESObjectWEAP* GetWeaponSource() {
        return pWeaponSource;
    }
    void SetAngularMomentumZ(float a2);
    void SetAngularMomentumX(float a2);
    void StopCountdownSound();
    void CombatHit(Actor* apCollidee, bool abPowerAttack);
    void SetConsistencyRecurse(NiAVObject* apObject);
    float GetDamage();
    void DetachLight();
    void AimAtPoint(NiPoint3 aTargetLocation);
    void SetSpeedMult(float afSpeedMult);
    Projectile::ImpactData* GetImpactData();
    void AimProjectile();
    void Init3D();
    void AddInitialVelocity();
    void UpdateIfNotInArrowsList(float a2);
    void HandleInventoryGrenadeDialogue();
    // Sets the action flag on the trigger zone or gas trap phantom within a unit of the projectile.
    bool TestProjectileInTriggerZone();
    // The impact's decals, particles, detection event and sounds on apCollidee.
    void SpawnCollisionEffects(
        TESObjectREFR* apCollidee,
        const NiPoint3& aContactPoint,
        const NiPoint3& aContactNormal,
        const hkpCollidable* apTargetCollidable,
        HavokMaterialType aeMaterial
    );
    void SpawnMuzzleFlash();
    bool GetMineReactsToTarget(Actor* apTarget, Actor* apShooterActor, TESFaction* apOwnerFaction);
    bool CheckExplosionProximity();
    void AttachLight();
    Projectile::ImpactData* GetImpactData(TESObjectREFR* apRef);
    void CheckRefDeleted(TESObjectREFR* apDeletedRef);
    void StartCountdownSound();
    bool FindBlastNodes(NiAVObject* apRoot, NiPoint3& aBlastPosition);

    // Fills apPoints with points on the cone of afConeAngle around the direction of afHeading and afLooking, afRange
    // from aOrigin, turned by afRotation.
    static void GetConeEndpoints(
        NiPoint3* apPoints,
        int32_t aiCount,
        NiPoint3 aOrigin,
        float afHeading,
        float afLooking,
        float afConeAngle,
        float afRange,
        float afRotation
    );
    // The projectile of apBase that apShooter fired, among the high-process objects; null when there is none.
    static Projectile* FindExistingProjectile(TESObjectREFR* apShooter, BGSProjectile* apBase);
    // Creates and launches a projectile of apProjectileBase from aOrigin at the given angles.
    static Projectile* Launch(
        BGSProjectile* apProjectileBase,
        TESObjectREFR* apShooter,
        CombatController* apShooterCombatController,
        TESObjectWEAP* apFromWeapon,
        NiPoint3 aOrigin,
        float afZAngle,
        float afXAngle,
        ProjectilePickCache* apPickCache,
        TESObjectREFR* apHomingTarget,
        bool abAlwaysHit,
        bool abDoNoDamage,
        float afSpreadHeading,
        float afSpreadLooking,
        TESObjectCELL* apParentCell
    );
    // Casts apPickData's ray against the shapes of apObject's biped-layer bodies and its children's, adding each hit
    // to the pick's all-hits collector; abIgnoreHead skips the head, abSort sorts the hits after. True on a hit.
    static bool PickSceneGraph(bhkPickData* apPickData, NiAVObject* apObject, bool abIgnoreHead, bool abSort);
    // Whether the projectile apProjectile (of apProjectileBase) can hit apCollidee through
    // apTargetCollidable.
    static bool IsValidImpact(
        TESObjectREFR* apCollidee,
        const hkpCollidable* apTargetCollidable,
        Projectile* apProjectile,
        BGSProjectile* apProjectileBase
    );
    // Runs the pick results of apPickData through the projectile's impact rules (apProjectile
    // or, without one, apProjectileBase) and returns the 3D it stops at, its hit fraction in *apHitFraction.
    static NiAVObject* ProcessTargetPick(
        bhkPickData* apPickData,
        Projectile* apProjectile,
        BGSProjectile* apProjectileBase,
        float* apHitFraction,
        bool bUseCharacterController,
        float afWaterHitRatio
    );
    static float ComputeSpeed(float afSpeed, float afPower, float afSpeedMult);

    BSSimpleList<ImpactData*> ImpactList;
    bool bImpactProcessed;
    NiTransform kFollowOffset;
    uint32_t iFlags;
    float fPower;
    float fSpeedMult;
    float fRange;
    float fAge;
    float fDamage;
    float fAlpha;
    float fExplosionTimer;
    float fBlinkTimer;
    float fSpreadHeading;
    float fSpreadLooking;
    float fWeaponCondition;
    TESObjectWEAP* pWeaponSource;
    TESObjectREFR* pShooter;
    TESObjectREFR* pDesiredTarget;
    NiPoint3 MovementDirection;
    float fDistanceMoved;
    NiPointer<NiLight> spLight;
    bool bSendAlarm;
    NiPointer<ProjectilePickCache> spPickCache;
    NiPointer<ActorCause> spActorCause;
    float fDecalSize;
    BSSoundHandle sndHandle;
    BSSoundHandle sndCountdown;
    ProjectileTarget* pTarget;
    ItemChange* pSecondaryItemChange;
    bool bSoundFired;
    float fSoundDist;
};

extern SettingT<INISettingCollection> bVATSProjectileDebug;
extern SettingT<INISettingCollection> bProjectileDebug;
extern SettingT<INISettingCollection> fProjectileDebugDuration;
