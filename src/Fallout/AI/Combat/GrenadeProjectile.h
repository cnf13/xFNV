#pragma once

#include "Fallout/AI/Combat/Projectile.h"

struct hkpContactPointAddedEvent;

// A grenade projectile: one that bounces and rests until it detonates. Only some of its entry points are declared.
class GrenadeProjectile : public Projectile {
public:
	// Constructs the projectile, with a process when abAdd.
	explicit GrenadeProjectile(bool abAdd);
	GrenadeProjectile(BGSProjectile* apProjectileBase, TESObjectREFR* apShooter, TESObjectWEAP* apFromWeapon,
		NiPoint3 aOrigin, float afZAngle, float afXAngle);

	void Process3D() override;
	bool TurnOff(Actor* apActionActor, bool abSilent) override;
	// A mine stays; otherwise as for any projectile.
	bool IsPermanent() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::InitLoadGame;
	// Clears bCollisionGroupReset after the projectile's revert.
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::Revert;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::FinishLoadGame;
	Projectile::PROJECTILE_TYPE GetProjectileType() override;
	void Initialize() override;
	void UpdateProjectile(float afDelta) override;
	bool IsMine() override;
	void ReportHavokDeactivation() override;
	~GrenadeProjectile() override;

protected:
	bool AddImpact(TESObjectREFR* apCollidee, const NiPoint3& aContactPoint, const NiPoint3& aContactNormal,
		const hkpCollidable* apTargetCollidable, uint32_t aiKey, HavokMaterialType aeForceMaterial) override;

public:
	void ReportCollision(hkpContactPointAddedEvent& arEvent);
	void ReportCollision(bhkCharacterController* apController, NiPoint3 akPos, NiPoint3 akNormal);
	// Bounces the projectile off its first impact: moves the 3D out of the surface by its bound radius, then sets the
	// velocity to the 3D's down axis reflected about the impact normal, scaled by the base form's speed and bounce
	// multiplier. Nothing while a save loads. Always true.
	bool ProcessBounce();

	bool bCollisionGroupReset;
};
