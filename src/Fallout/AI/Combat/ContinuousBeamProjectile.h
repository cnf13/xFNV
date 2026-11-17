#pragma once

#include "BSShader/BeamShaderProperty.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class BGSProjectile;
class NiGeometry;
class TESObjectWEAP;

// A continuous beam projectile: a beam held for as long as the weapon fires. Only some of its entry points are
// declared.
class ContinuousBeamProjectile : public Projectile {
public:
    // Constructs the projectile, with a process when abAdd.
    explicit ContinuousBeamProjectile(bool abAdd);
    ContinuousBeamProjectile(
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
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::Revert;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    using Projectile::FinishLoadGame;
    Projectile::PROJECTILE_TYPE GetProjectileType() override;
    void Initialize() override;
    void Process3D() override;
    void UpdateProjectile(float afDelta) override;
    ~ContinuousBeamProjectile() override;

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
    void SetKill(Actor* apActor);
    // Scales the beam geometry under ap3D from afCurrentLength to afDesiredLength.
    void ScaleBeam3D(NiAVObject* ap3D, float afDesiredLength, float afCurrentLength);
    void SetShotsFired(int32_t num);
    // Scales apGeometry's bound, particle emitters or vertices by afScaleFactor.
    void ScaleBeamGeometryLength(NiGeometry* apGeometry, float afScaleFactor);
    // The length of apGeometry. Same function as BeamProjectile::FindBeamGeometryLength.
    float FindBeamGeometryLength(NiGeometry* apGeometry);

    NiPointer<BeamShaderProperty> spBeamProperty;
    uint32_t iShotsFired;
};
