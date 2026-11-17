#pragma once

#include "BSShader/BeamShaderProperty.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSProjectile;
class NiGeometry;
class NiParticleSystem;
class TESObjectWEAP;

// A beam projectile: one that hits along its whole line at once. Only some of its entry points are declared.
class BeamProjectile : public Projectile {
public:
	// A projectile, with a process when abAdd.
	explicit BeamProjectile(bool abAdd);
	BeamProjectile(BGSProjectile* apProjectileBase, TESObjectREFR* apShooter, TESObjectWEAP* apFromWeapon,
		NiPoint3 aOrigin, float afZAngle, float afXAngle);

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::Revert;
	Projectile::PROJECTILE_TYPE GetProjectileType() override;
	void Initialize() override;
	void Process3D() override;
	void UpdateProjectile(float afDelta) override;

protected:
	bool AddImpact(TESObjectREFR* apCollidee, const NiPoint3& aContactPoint, const NiPoint3& aContactNormal,
		const hkpCollidable* apTargetCollidable, uint32_t aiKey, HavokMaterialType aeForceMaterial) override;

public:
	float FindBeamGeometryLength(NiParticleSystem* apObject);
	// Scales apObject's bound and geometry by afValue. Its caller ScaleBeam3D passes an object it tested as NiGeometry,
	// and the body reads it as one.
	void ScaleBeamGeometryLength(NiGeometry* apObject, float afValue);
	// Walks apObject's children within afRange.
	void ScaleBeam3D(NiAVObject* ap3D, float afDesiredLength, float afCurrentLength);

	NiPointer<BeamShaderProperty> spBeamProperty;
};
