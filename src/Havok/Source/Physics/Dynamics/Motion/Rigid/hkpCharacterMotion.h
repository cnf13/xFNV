#pragma once

#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

// The motion of a character's rigid body: a body that does not rotate. It overrides the inertia accessors,
// getProjectedPointVelocity and the impulse and force functions; the destructor is hkpMotion's.
class hkpCharacterMotion : public hkpMotion {
public:
	hkpCharacterMotion(const hkVector4& position, const hkQuaternion& rotation);
	// The finish constructor: a1 is the hkFinishLoadedObjectFlag's m_finishing.
	explicit hkpCharacterMotion(int32_t a1);
	// The SDK's finish constructor, which getVtablehkpCharacterMotion builds: the one above with the flag's
	// m_finishing.
	hkpCharacterMotion(hkFinishLoadedObjectFlag flag) : hkpCharacterMotion(flag.m_finishing) {}

	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void getInertiaLocal(hkMatrix3& inertia) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void getInertiaWorld(hkMatrix3& inertia) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setInertiaLocal(const hkMatrix3& inertia) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setInertiaInvLocal(const hkMatrix3& inertiaInv) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void getInertiaInvLocal(hkMatrix3& inertiaInv) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void getInertiaInvWorld(hkMatrix3& inertiaInv) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void getProjectedPointVelocity(const hkVector4& pos, const hkVector4& normal, float& velOut,
		float& invVirtMassOut) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void applyPointImpulse(const hkVector4& imp, const hkVector4& p) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void applyAngularImpulse(const hkVector4& imp) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void applyForce(float deltaTime, const hkVector4& force) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void applyForce(float deltaTime, const hkVector4& force, const hkVector4& p) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void applyTorque(float deltaTime, const hkVector4& torque) override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();
};

extern hkClass hkpCharacterMotionClass;

extern hkTypeInfo hkpCharacterMotionTypeInfo;
