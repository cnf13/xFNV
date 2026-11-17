#pragma once

#include "Fallout/AI/MobileObject.h"

class ActiveEffect;
class EffectSetting;
class MagicCaster;
class MagicItem;
class MagicLight;
class MagicTarget;

// The PC RTTI and type derive this class from MobileObject, not the unrelated Projectile class.
class MagicProjectile : public MobileObject {
public:
	MagicProjectile();
	MagicProjectile(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffectItem, EffectSetting* apAVEffect,
		NiPoint3 aLocation, NiPoint3 aRotation);
	~MagicProjectile() override;

	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void Revert(uint32_t aiFlags) override {}
	NiAVObject* Load3D(bool abBackgroundLoading) override;
	bool IsProjectile() override;
	void Process(float afDelta) override;
	bool ChangeProcessLevel() override;
	bool IsTransient() override;
	void SetDisplayGeometry(bool abTF) override;

	virtual void Collide(NiPoint3 aContactPoint, bhkRigidBody* apTargetBody, TESObjectREFR* apCollidee, bool abFizzle) {
	}

	virtual void AddAreaTarget(MagicTarget* apTarget, ActiveEffect* apActiveEffect) {}
	virtual bool IsActive();
	virtual void BuildAreaEffectGeometry() {}
	virtual bool CheckDeletedCaster(MagicCaster* apCaster);
	virtual bool CheckAddTarget(MagicTarget* apSpellTarget, ActiveEffect* apEffect);
	virtual bool PostProcess();

	// Pushes apTargetBody from aContactPoint along aDirection, scaled by its layer's knock multiplier and its mass.
	void ApplyCollisionForce(NiPoint3 aContactPoint, NiPoint3 aDirection, bhkRigidBody* apTargetBody);
	// Casts a ray from the player's eyes to the projectile and, when it first hits something other than the
	// player, the projectile or an actor, collides the projectile there.
	void CheckShotBehindWall();
	MagicCaster* GetCaster();
	// pDominantEffect, through the shared one-field getter.
	EffectItem* GetDominantEffect();
	// pAVEffect, through the shared one-field getter.
	EffectSetting* GetAVEffect();
	void KillAreaEffectGeometry();

	// Loads and clones the model at apModelPath.
	static NiAVObject* Clone3D(const char* apModelPath);
	// The dummy base object a projectile is given.
	static TESBoundObject* GetDummyObjectReference();
	// Adds afProjectileWeight to fProjectileTracker.
	static void AddTrackedProjectile(float afProjectileWeight);
	// Takes afProjectileWeight, at least 0, off fProjectileTracker.
	static void RemoveTrackedProjectile(float afProjectileWeight);
	static void Launch(MagicCaster* apCaster, NiAVObject* apData3D, MagicLight* apLight, MagicItem* apSpell,
		EffectItem* apDominantEffect, EffectSetting* apAVEffect, NiPoint3 aOrigin, NiPoint3 aRotation);
	static NiAVObject* Clone3D_0(EffectSetting* apAVEffect);

	float fSpeed;
	float fDistanceMoved;
	float fAge;
	MagicCaster* pCaster;
	MagicItem* pSpell;
	EffectItem* pDominantEffect;
	EffectSetting* pAVEffect;
	float fLastUpdate;

	// The weight of the live magic projectiles.
	static inline float fProjectileTracker;
	static inline TESBoundObject* pDummyObjectRef;
};
