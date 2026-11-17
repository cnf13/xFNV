#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkdWorld;
class hkpProjectileGun;
class hkpRigidBody;
class hkpWorld;

// The guns' class-registry callbacks (their bodies are in BSHavok/bhkWorld.cpp). The getVtable ones are Havok SDK
// signatures (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpProjectileGun();
void finishLoadedObjecthkpProjectileGun(void* p, int32_t finishing);
void cleanupLoadedObjecthkpProjectileGun(void* p);

// A projectile a gun fires: a rigid body that listens for its own contacts and is destroyed when it is done.
class hkpGunProjectile : public hkReferencedObject, public hkpContactListener {
public:
	// The bits of m_flags.
	enum Flags : uint8_t {
		HIT_LISTENING = 4,
		HIT = 2,
		DESTROYED = 1,
	};

	hkpGunProjectile(hkpProjectileGun* gun, hkpRigidBody* body);
	~hkpGunProjectile() override;

	// Releases the body; slot 3 is a jump to _destroyBody.
	virtual void onDestroy();
	// Marks the projectile hit.
	virtual void onHit(const hkpContactPointEvent& event) { m_flags.orWith(HIT); }
	virtual void onUpdate(float timeStep) {}

protected:
	// Hands every contact point to onHit, through the primary vtable (slot 4). Private in the PDB.
	void contactPointCallback(const hkpContactPointEvent& event) override { onHit(event); }

	// Removes the body from its world and drops the reference to it (thiscall, ret 0,
	// 0xE44CF0). Private in the PDB.
	void _destroyBody();

public:
	void setHitListening(bool enable);
	// Calls onDestroy (vtable +0xC) unless DESTROYED is set, then sets it (thiscall, ret 0,
	// 0xE44C70; the SDK's hkpGunProjectile::destroy, hkpProjectileGun.h:26; called on m_projectiles' entries).
	void destroy();
	void addToWorld(hkpWorld* world);
	void update(float timeStep);

	// DeletingDestruct's free (hkReferencedObject's); declared here because hkpContactListener declares its own and
	// the lookup would be ambiguous.
	static void operator delete(void* apMemory) { deleteObject(apMemory); }
	static void flyTrue(hkpRigidBody* body, float minVelocity, float timeStep);

	hkFlags<Flags, uint8_t> m_flags;
	hkpRigidBody* m_body;
	float m_existanceTime;
	hkpProjectileGun* m_gun;
};

// A gun that fires rigid body projectiles, keeps at most m_maxProjectiles of them and reloads between shots.
class hkpProjectileGun : public hkpFirstPersonGun {
public:
	hkpProjectileGun(hkpWorld* world, hkdWorld* destructionWorld);
	hkpProjectileGun(hkFinishLoadedObjectFlag aFlag);
	~hkpProjectileGun() override;

	void fireGun(hkpWorld* world, const hkTransform& viewTransform) override;
	void stepGun(float timeStep, hkpWorld* world, const hkpRigidBody* characterBody, const hkTransform& viewTransform,
		bool fire, bool fireRmb) override;
	// Drops every projectile; world is not read.
	void reset(hkpWorld* world) override;
	virtual void onGunFired(hkpWorld* world, const hkTransform& viewTransform) {}
	virtual void onUpdate(float timeStep, hkpWorld* world, const hkpRigidBody* characterBody,
		const hkTransform& viewTransform, bool fire, bool fireRmb);

protected:
	// Destroys and releases every projectile and empties m_projectiles.
	void clearProjectiles();

public:
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	void updateProjectiles(float timeStep);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	void clearHitProjectiles();
	void removeDestroyedProjectiles();
	void destroyAgedProjectiles(float time);
	hkpGunProjectile* getFirstActiveProjectile();
	void addProjectile(hkpGunProjectile* proj);

	static const hkClass& staticClass();
	static const hkClass& staticClass_00DFCE30();

	int32_t m_maxProjectiles;
	float m_reloadTime;
	float m_reload;
	hkArray<hkpGunProjectile*> m_projectiles;
	hkpWorld* m_world;
	hkdWorld* m_destructionWorld;
};

extern hkTypeInfo hkpPoweredChainDataTypeInfo;
extern hkClass hkpBallSocketChainDataClass;
extern hkTypeInfo hkpBallSocketChainDataTypeInfo;
extern hkClass hkpProjectileGunClass;
extern hkTypeInfo hkpProjectileGunTypeInfo;
inline constexpr fnvd::Vtable hkpGunProjectile_vftable{};
inline constexpr fnvd::Vtable hkpGunProjectile_vftable_hkpContactListener{};
inline constexpr fnvd::Vtable hkpProjectileGun_vftable{};
