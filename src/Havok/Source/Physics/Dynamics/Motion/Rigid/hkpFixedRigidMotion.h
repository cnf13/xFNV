#pragma once

#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Reconstruction.hpp"

// The motion of a body that never moves. The exe's vtable has 29 slots; it overrides setLinearVelocity,
// setAngularVelocity and setStepPosition, and adds getPositionAndVelocities (slot 28); the destructor is
// hkpKeyframedRigidMotion's.
class hkpFixedRigidMotion : public hkpKeyframedRigidMotion {
public:
	hkpFixedRigidMotion(const hkVector4& position, const hkQuaternion& rotation);
	// The finish constructor: a1 is the hkFinishLoadedObjectFlag's m_finishing.
	explicit hkpFixedRigidMotion(int32_t a1);
	// The SDK's finish constructor, which getVtablehkpFixedRigidMotion builds: the one above with the flag's
	// m_finishing.
	hkpFixedRigidMotion(hkFinishLoadedObjectFlag flag) : hkpFixedRigidMotion(flag.m_finishing) {}

	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setLinearVelocity(const hkVector4& newVel) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setAngularVelocity(const hkVector4& newVel) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setStepPosition(float position, float timestep) override;
	// slot 28
	virtual void getPositionAndVelocities(hkpMotion* motionOut);

	// The reflection class hkpFixedRigidMotionClass.
	static const hkClass& staticClass();
};
inline constexpr fnvd::Vtable hkpFixedRigidMotion_vftable{};
