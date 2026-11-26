#pragma once

#include "Fallout/AI/Combat/Projectile.h"

#include <stdint.h>

// A missile projectile: one that flies along a ballistic or straight path. Only some of its entry points are declared.
class MissileProjectile : public Projectile {
public:
    enum ImpactResult : int32_t {
        IR_NONE = 0x0,
        IR_DESTROY = 0x1,
        IR_BOUNCE = 0x2,
        IR_IMPALE = 0x3,
        IR_STICK = 0x4,
    };

    // Constructs the projectile, with a process when abAdd.
    explicit MissileProjectile(bool abAdd);
    MissileProjectile(
        BGSProjectile* apProjectileBase,
        TESObjectREFR* apShooter,
        TESObjectWEAP* apFromWeapon,
        NiPoint3 aOrigin,
        float afZAngle,
        float afXAngle
    );

    // A projectile that stuck in something stays.
    bool IsPermanent() override;
    // Saves the projectile, then eImpactResult.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    // Loads the projectile, then eImpactResult.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::InitLoadGame;
    // Clears eImpactResult after the projectile's revert.
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::Revert;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::FinishLoadGame;
    Projectile::PROJECTILE_TYPE GetProjectileType() override;
    void Initialize() override;
    void Process3D() override;
    void UpdateProjectile(float afDelta) override;
    bool ProcessImpacts() override;
    ~MissileProjectile() override;

protected:
    bool AddImpact(
        TESObjectREFR* apCollidee,
        const NiPoint3& aContactPoint,
        const NiPoint3& aContactNormal,
        const hkpCollidable* apTargetCollidable,
        uint32_t aiKey,
        HavokMaterialType aeForceMaterial
    ) override;

public:
    void Initialize3D();
    // Picks eImpactResult after an impact is added.
    void DetermineImpactResult();
    ImpactResult GetImpactResult() {
        return eImpactResult;
    }
    // The bounce after an impact.
    bool ProcessBounce();
    // Impales the hit target.
    void ProcessImpale();
    // Sticks into the hit surface.
    void ProcessStick();

    ImpactResult eImpactResult;
    bool bWaitingToInitialize3D;
    float fStickTime;
    float fLocalRot;
};
