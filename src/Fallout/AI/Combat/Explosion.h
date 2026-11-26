#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSExplosion;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
struct hkpRootCdPoint;
class MagicCaster;
class NonActorMagicCaster;
class TESObjectWEAP;

// An explosion reference. Only part of the class is declared.
class Explosion : public MobileObject {
public:
    // One limb an explosion reached and its distance; AddLimbTarget allocates 8 bytes and stores aeLimb
    // at +0, afDistance at +4.
    struct ExplodedLimb {
        BGSBodyPart::LIMB_ENUM eLimb;
        float fDistance;
    };

    // One affected reference and its processed limb list.
    struct ExplosionTarget {
        ExplosionTarget();
        ~ExplosionTarget();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        TESObjectREFR* pRef;
        bool bHitWeakPoint;
        bool bMobile;
        bool bProcessed;
        BSSimpleList<ExplodedLimb*> limbList;
    };

    Explosion();
    Explosion(
        BGSExplosion* apExplosionBase,
        TESObjectREFR* apOwnerRef,
        TESObjectCELL* apCell,
        NiPoint3 aOrigin,
        NiMatrix3 aRotation
    );
    ~Explosion() override;

    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void SetActorCause(ActorCause* apActorCause) override;
    ActorCause* GetActorCause() override;
    MagicCaster* GetMagicCaster() override;
    void InitHavok() override;
    bool IsExplosion() override;
    void Process(float afDelta) override {}
    bool ChangeProcessLevel() override;
    bool IsTransient() override;
    void Update(float afDelta) override;

    using MobileObject::FinishLoadGame;
    using MobileObject::InitLoadGame;
    using MobileObject::LoadGame;
    using MobileObject::Revert;
    using MobileObject::SaveGame;

    // Sets the explosion up once its 3D is loaded.
    virtual void Initialize();

    // Whether the explosion is underwater: bUnderWater.
    bool GetUnderWater();
    // Puts the impact decals of the explosion on what it hit.
    void ApplyImpactDecals();
    // The movement update that may not run while rendering.
    void UpdateNonRenderSafeMovement(float afDelta);
    // Dims the explosion's light as it ages: full at birth, out at half its lifetime.
    void UpdateLight();
    float GetAge();
    float GetLifetime();
    float GetRadius();
    bool IsDistant();
    // The reference that caused the explosion: pOwner.
    TESObjectREFR* GetOwner();
    // Stops and releases both explosion sounds.
    void StopSounds();
    void SetOuterRadius(float afRadius);
    // The explosion's base form.
    BGSExplosion* GetExplosionBase();
    // The listed target of apTargetRef, its weak-point and mobile flags filled in, or a new target added to the head
    // of the list.
    ExplosionTarget* AddTarget(TESObjectREFR* apTargetRef, bool abWeakPoint, bool abMobile);
    // Sets pWeaponSource.
    void SetWeaponSource(TESObjectWEAP* apSource);
    // Sets fLifetime.
    void SetLifetime(float afValue);
    void ProcessMovement(float afDelta);
    void Kill();
    // The limbs of apTarget the explosion reached, or null.
    BSSimpleList<ExplodedLimb*>* GetAffectedLimbList(TESObjectREFR* apTarget);
    // Sets ClosestPointNormal.
    void SetClosestPointNormal(NiPoint3 aNormal);
    TESObjectWEAP* GetSourceWeapon();
    // Attaches the explosion's 3D to the terrain manager's LOD root.
    void AttachDistant();
    void Init3D();
    void ApplyForces();
    void ProcessTargets();
    void PushActor(Actor* apTargetActor, float afExplosionPower);
    void StartSounds();
    void AttachLight();
    void UpdateRadiation();
    bool RunLOSPick(NiPoint3 aLocation, TESObjectREFR* apTarget);
    bool TestExplosionUnderWater();
    void FindTargets();
    void PushRigidBody(bhkRigidBody* apRigidBody);
    float GetDamage();
    void DetachLight();
    void CheckLimbTarget(Explosion::ExplosionTarget* apTarget, NiAVObject* apTargetObject, float afDistance);
    void AddLimbTarget(Explosion::ExplosionTarget* apTarget, BGSBodyPart::LIMB_ENUM aeLimb, float afDistance);
    void CheckRefDeleted(TESObjectREFR* apDeletedRef);

    // Creates an explosion of apExplosionBase at aOrigin turned by aRotation in apCell, caused by apOwnerRef and
    // centred on apExplodedRef; the explosion, else null.
    static Explosion* SpawnExplosion(
        BGSExplosion* apExplosionBase,
        TESObjectREFR* apOwnerRef,
        TESObjectREFR* apExplodedRef,
        TESObjectCELL* apCell,
        NiPoint3 aOrigin,
        NiMatrix3 aRotation
    );

    float fAge;
    float fLifetime;
    float fBuildTime;
    float fRadius;
    float fImageSpaceRadius;
    float fWeaponCondition;
    NiPointer<bhkSimpleShapePhantom> spPhantom;
    BSSimpleList<ExplosionTarget*> TargetList;
    BSSoundHandle Sound1;
    BSSoundHandle Sound2;
    NiPointer<NiPointLight> spLight;
    TESObjectREFR* pOwner;
    TESObjectREFR* pExplodedRef;
    NiPointer<ActorCause> spActorCause;
    bool bDecalsPlaced;
    bool bTargetsFound;
    bool bTargetsProcessed;
    bool bForcesApplied;
    bool bIgnoreImageSpaceSwap;
    bool bUnderWater;
    NonActorMagicCaster* pCaster;
    TESObjectWEAP* pWeaponSource;
    int32_t iFrameCount;
    NiPoint3 ClosestPoint;
    NiPoint3 ClosestPointNormal;
    float fCalculatedDamage;

    // The liquid action Init3D gives each molotov test body.
    static inline float fMolotovTestNeighborStrength = 15.0f;
    static inline float fMolotovTestNeighborDistance = 64.0f;
    static inline float fMolotovTestStickStrength = 15.0f;
    static inline float fMolotovTestInitialStickForce = 5000.0f;
};

inline float fMolotovTestLifetime = 15.0f;

inline float fMolotovTestForce = 1.0f;
