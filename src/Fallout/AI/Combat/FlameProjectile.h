#pragma once

#include "Fallout/AI/Combat/Projectile.h"

// A flame projectile: a short-lived stream of fire. Only some of its entry points are declared.
class FlameProjectile : public Projectile {
public:
    // Constructs the projectile, with a process when abAdd.
    explicit FlameProjectile(bool abAdd);
    FlameProjectile(
        BGSProjectile* apProjectileBase,
        TESObjectREFR* apShooter,
        TESObjectWEAP* apFromWeapon,
        NiPoint3 aOrigin,
        float afZAngle,
        float afXAngle
    );

    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::InitLoadGame;
    // Clears fExpirationTimer and fConeAngle after the projectile's revert.
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::Revert;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::FinishLoadGame;
    Projectile::PROJECTILE_TYPE GetProjectileType() override;
    void Initialize() override;
    void Process3D() override;
    void UpdateProjectile(float afDelta) override;
    bool RunTargetPick() override;

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
    // fConeAngle.
    float GetConeAngle() {
        return fConeAngle;
    }
    void ResetExpirationTimer();

    // The widest cone angle of the flame particle systems at or under apObject. Takes any NiAVObject and recurses
    // through nodes.
    static float FindFlameConeAngle(NiAVObject* apObject);

    float fExpirationTimer;
    float fConeAngle;
};
