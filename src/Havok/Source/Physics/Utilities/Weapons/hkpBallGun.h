#pragma once

#include "Havok/Source/Common/Base/Container/Queue/hkQueue.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "Reconstruction.hpp"

class hkpRigidBody;
class hkpRigidBodyCinfo;
class hkpShape;

const void* getVtablehkpBallGun();
void finishLoadedObjecthkpBallGun(void* p, int32_t finishing);
void cleanupLoadedObjecthkpBallGun(void* p);

// The demo framework's gun that fires balls: each shot adds a rigid sphere to the world, and the oldest bullets are
// removed once m_maxBulletsInWorld are out. The exe's vtable has 9 slots; it overrides the destructor (slot 0),
// fireGun (slot 4) and reset (slot 8). Layout from the exe's hkpBallGunClass record (size 0x60) and the PC PDB.
class hkpBallGun : public hkpFirstPersonGun {
public:
	hkpBallGun(hkFinishLoadedObjectFlag flag);
	hkpBallGun(int32_t numBullets);
	// Slot 0.
	~hkpBallGun() override;

	// Slot 4.
	void fireGun(hkpWorld* world, const hkTransform& viewTransform) override;
	// Slot 8: removes every bullet still queued, telling the listeners first and taking each out
	// of world when it is in it.
	void reset(hkpWorld* world) override;

protected:
	// Not reconstructed yet. The exe's out-of-line copy of the SDK's block: the volume mass properties of apShape at
	// mass afMass, then arInfo's inertia tensor, centre of mass and mass from them (cdecl, ret 4, 0xCF07A0).
	// Inferred type: apShape: hkpShape* from 0xe45287 (result of hkpSphereShape::Construct); afMass: float from
	// 0xcf07c0 (passed to hkpInertiaTensorComputer::computeShapeVolumeMassProperties 1)
	static hkpRigidBodyCinfo* computeMassProperties(const hkpShape* apShape, float afMass, hkpRigidBodyCinfo& arInfo);

public:
	// Havok SDK signature (hkpUtilities.lib, tied by target).
	static const hkClass& staticClass();

	float m_bulletRadius;
	float m_bulletVelocity;
	float m_bulletMass;
	float m_damageMultiplier;
	int32_t m_maxBulletsInWorld;
	hkVector4 m_bulletOffsetFromCenter;
	// The bullets in the world, oldest first.
	hkQueue<hkpRigidBody*>* m_addedBodies;
};
inline constexpr fnvd::Vtable hkpBallGun_vftable{};

// The class's reflection record.
extern hkClass hkpBallGunClass;
extern hkTypeInfo hkpBallGunTypeInfo;
