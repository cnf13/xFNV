#pragma once

#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class Actor;
class AlchemyItem;
class EnchantmentItem;

// An arrow projectile, which stays stuck in what it hits. Only the static members the reconstructed code calls are
// declared.
class ArrowProjectile : public MobileObject {
public:
	// The arrow's flight and collision state (PDB names and values).
	enum ArrowState : int32_t {
		STATE_FLYING = 0,
		STATE_COLLISION = 1,
		STATE_POST_COLLISION = 2,
		STATE_DELETING = 3,
	};

	// What an arrow hit: the collision type, point, normal, velocity and the object hit.
	struct CollisionData {
		CollisionData();

		uint32_t eCollisionType;
		NiPoint3 kPoint;
		NiPoint3 kNormal;
		NiPoint3 kVelocity;
		TESObjectREFR* pCollidee;
		NiAVObject* pClosestObject;
		NiMatrix3 kRotation;
	};

	ArrowProjectile();
	~ArrowProjectile() override;

	NiAVObject* Load3D(bool abBackgroundLoading) override;
	void Revert(uint32_t aiFlags) override;
	// TESForm slots 20, 22, 24, 26 and 27.
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	// TESObjectREFR slot 137.
	bool IsProjectile() override;
	// MobileObject slots 144, 147, 151, 152, 187 and 190.
	bool MovetoHigh() override;
	bool MovetoMiddleHigh() override;
	void Process(float afDelta) override;
	bool ChangeProcessLevel() override;
	bool IsTransient() override;
	void Update(float afDelta) override;

	// Sets the arrow's state.
	void SetState(ArrowState aeState);
	// Gives the arrow a character controller when it has none, then puts the controller in the cell's world.
	void InitHavok(Actor* apShooter, float afPower);
	bool GetAddedToInventory();
	bool GetPostForce();
	void SetPostForce(bool abValue);
	void ClearAttachedRef();
	TESObjectREFR* GetAttachedRef();
	float GetSpeed();
	// Pushes apTargetBody once, scaled by the arrow's speed and the body's layer and mass.
	void ApplyCollisionForce(NiPoint3 aContactPoint, NiPoint3 aDirection, bhkRigidBody* apTargetBody);
	char ProcessCollisionTerrain(bhkRigidBody* apBody);
	bool ProcessCollisionActor(bhkRigidBody* apArrowRB);
	bool ProcessSimulate(bhkRigidBody* apArrowRB);
	void CollideActor(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal, Actor* apCollidee);
	void CollideTerrain(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal);
	void Bounce(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal);
	void Simulate(Actor* apCollidee);
	bool ProcessCollision();
	bool ProcessCollisionReference(bhkRigidBody* apArrowRB);
	bool ProcessBounce(bhkRigidBody* apArrowRB);
	// Inferred type: afVelocity: float from 0x8cca0f (result of NiPoint3::Length)
	void PlayArrowCollisionSound(float afVelocity, NiPoint3 a3, bhkRigidBody* a4, char a5);

	// The collision group arrows use.
	static uint32_t GetArrowGroup();
	// The number of arrows stuck in references.
	static int32_t GetArrowCount();
	// Deletes the arrows stuck in apRef, at once when abInstant.
	static void DeleteAttachedArrows(TESObjectREFR* apRef, bool abInstant);
	// The object of apObject's scene graph hit by a pick of length afPickLength from aPickFrom along aDirection,
	// skipping the head with abIgnoreHead, or null; afPickLength shrinks to the hit fraction.
	static NiAVObject* PickSceneGraph(NiAVObject* apObject, NiPoint3 aPickFrom, NiPoint3 aDirection,
		float& afPickLength, bool abIgnoreHead);

	CollisionData* psCollisionData;
	ArrowState eState;
	float fFadeAlpha;
	float fAge;
	float fSpeed;
	float fDamage;
	float fWaterSpeed;
	Actor* pShooter;
	EnchantmentItem* pEnchantment;
	EnchantmentItem* pBowEnchantment;
	AlchemyItem* pPoison;
	// IDA: kDeltaS (the constructor constructs it, 0x8CAB20).
	NiPoint3 DeltaS;
	bool bNeedsFinishInitLoadGame;
	bool bAddedToInventory;
	bool bBowIgnoresWeaponsResist;
	bool bPostForce;
	uint32_t iLoadGameFlags;

	// The number of arrow projectiles in the world.
	static inline int32_t iArrowCount;
	static inline int32_t iArrowGroup;
};
inline constexpr fnvd::Vtable ArrowProjectile_vftable_TESChildCell{};
inline constexpr fnvd::Vtable ArrowProjectile_vftable{};
