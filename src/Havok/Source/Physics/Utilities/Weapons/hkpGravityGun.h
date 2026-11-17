#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "Reconstruction.hpp"

class hkTransform;
class hkpRigidBody;
class hkpWorld;

const void* getVtablehkpGravityGun();
void finishLoadedObjecthkpGravityGun(void* p, int32_t finishing);
void cleanupLoadedObjecthkpGravityGun(void* p);

// The first person gun that picks bodies up with a gravity beam and carries, drops or throws them. The picking,
// carrying and throwing helpers are private in the SDK and not declared here.
class hkpGravityGun : public hkpFirstPersonGun {
public:
	hkpGravityGun();
	hkpGravityGun(hkFinishLoadedObjectFlag flag);
	~hkpGravityGun() override;

	void stepGun(float timeStep, hkpWorld* world, const hkpRigidBody* body, const hkTransform& viewTransform, bool fire,
		bool fireRmb) override;
	void reset(hkpWorld* world) override;

	hkpRigidBody* pickBody(const hkTransform& viewTransform, const hkpRigidBody* characterBody, const hkpWorld* world);
	void gravityGunStep(float timeStep, const hkTransform& viewTransform, const hkpRigidBody* characterBody);
	hkpRigidBody* dropObject(int32_t index);
	void gravityGunPush(const hkTransform& viewTransform, const hkpRigidBody* characterBody, const hkpWorld* world);
	void gravityGunPutDown(const hkTransform& viewTransform, const hkpRigidBody* characterBody, const hkpWorld* world);
	void takeObject(hkpRigidBody* body);
	void gravityGunPull(const hkTransform& viewTransform, const hkpRigidBody* characterBody, const hkpWorld* world);

	// Havok SDK signature (hkpUtilities.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<hkpRigidBody*> m_grabbedBodies;
	int32_t m_maxNumObjectsPicked;
	float m_maxMassOfObjectPicked;
	float m_maxDistOfObjectPicked;
	float m_impulseAppliedWhenObjectNotPicked;
	float m_throwVelocity;
	hkVector4 m_capturedObjectPosition;
	hkVector4 m_capturedObjectsOffset;
};

inline constexpr fnvd::Vtable hkpGravityGun_vftable{};

extern hkClass hkpGravityGunClass;

extern hkTypeInfo hkpGravityGunTypeInfo;
