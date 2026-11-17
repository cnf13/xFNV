#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

#include <bit>
#include <stdint.h>

class hkpMaxSizeMotion;

void cleanupLoadedObjecthkpMaxSizeMotion(void* p);

// The motion of a rigid body: how the solver moves it.
class hkpMotion : public hkReferencedObject {
public:
	enum MotionType : int32_t {
		MOTION_INVALID = 0,
		MOTION_DYNAMIC = 1,
		MOTION_SPHERE_INERTIA = 2,
		MOTION_BOX_INERTIA = 3,
		MOTION_KEYFRAMED = 4,
		MOTION_FIXED = 5,
		MOTION_THIN_BOX_INERTIA = 6,
		MOTION_CHARACTER = 7,
		MOTION_MAX_ID = 8,
	};

	hkpMotion(const hkVector4& position, const hkQuaternion& rotation, bool wantDeactivation);

	hkpMotion(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {
		if (flag.m_finishing) {
			m_gravityFactor.m_value = std::bit_cast<int32_t>(1.0f) >> 16;
		}
	}

	~hkpMotion() override;

	// Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
	virtual void setMass(float mass);
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	virtual void setMassInv(float massInv);
	virtual void getInertiaLocal(hkMatrix3&) const = 0;
	virtual void getInertiaWorld(hkMatrix3&) const = 0;
	virtual void setInertiaLocal(const hkMatrix3&) = 0;
	virtual void setInertiaInvLocal(const hkMatrix3&) = 0;
	virtual void getInertiaInvLocal(hkMatrix3&) const = 0;
	virtual void getInertiaInvWorld(hkMatrix3&) const = 0;
	virtual void setCenterOfMassInLocal(const hkVector4&);
	virtual void setPosition(const hkVector4&);
	virtual void setRotation(const hkQuaternion&);
	virtual void setPositionAndRotation(const hkVector4&, const hkQuaternion&);
	virtual void setTransform(const hkTransform&);
	virtual void setLinearVelocity(const hkVector4&);
	virtual void setAngularVelocity(const hkVector4&);
	virtual void getProjectedPointVelocity(const hkVector4&, const hkVector4&, float&, float&) const = 0;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	virtual void applyLinearImpulse(const hkVector4& imp);
	virtual void applyPointImpulse(const hkVector4&, const hkVector4&) = 0;
	virtual void applyAngularImpulse(const hkVector4&) = 0;
	virtual void applyForce(float, const hkVector4&) = 0;
	virtual void applyForce(float, const hkVector4&, const hkVector4&) = 0;
	virtual void applyTorque(float, const hkVector4&) = 0;
	// Havok SDK signature (hkpDynamics.lib, tied by reloc,vtable).
	virtual void getMotionStateAndVelocitiesAndDeactivationType(hkpMotion* motionOut);

	MotionType getType() const { return m_type; }
	float getMass() const;
	// The motion state's translation.
	hkVector4* getTranslation();

	// Sets the two world-select bits of the inactive-frame counters, negated where the world's integrate counter is
	// behind the motion's.
	void setWorldSelectFlagsNeg(int32_t worldFlag0, int32_t worldFlag1, int32_t worldDeactivationIntegrateCounter) {
		if ((static_cast<uint32_t>(worldDeactivationIntegrateCounter) % 4) <
			(static_cast<uint32_t>(m_deactivationIntegrateCounter) % 4)) {
			worldFlag0 = (~worldFlag0) << 14;
		} else {
			worldFlag0 <<= 14;
		}

		if (worldDeactivationIntegrateCounter < m_deactivationIntegrateCounter) {
			worldFlag1 = (~worldFlag1) << 14;
		} else {
			worldFlag1 <<= 14;
		}

		m_deactivationNumInactiveFrames[0] =
			static_cast<uint16_t>((m_deactivationNumInactiveFrames[0] & 0x3fff) | worldFlag0);
		m_deactivationNumInactiveFrames[1] =
			static_cast<uint16_t>((m_deactivationNumInactiveFrames[1] & 0x3fff) | worldFlag1);
	}

	void setDeactivationClass(int32_t deactivationClass);

	const hkVector4& getCenterOfMassLocal() const { return m_motionState.m_sweptTransform.m_centerOfMassLocal; }
	const hkVector4& getCenterOfMassInWorld() const { return m_motionState.m_sweptTransform.m_centerOfMass1; }
	const hkVector4& getPosition() const { return m_motionState.m_transform.getTranslation(); }
	const hkQuaternion& getRotation() const { return m_motionState.m_sweptTransform.m_rotation1; }
	const hkTransform& getTransform() const { return m_motionState.m_transform; }
	void approxTransformAt(float time, hkTransform& transformOut);

	// The velocity of the world point p: the angular velocity crossed with p's offset from the center of mass, plus
	// the linear velocity.
	void getPointVelocity(const hkVector4& p, hkVector4& pointVelOut) const {
		hkVector4 relPos;
		relPos.setSub4(p, getCenterOfMassInWorld());
		pointVelOut.setCross(m_angularVelocity, relPos);
		pointVelOut.add4(m_linearVelocity);
	}

	hkSimdReal getMassInv() const { return m_inertiaAndMassInv.getSimdAt(3); }
	float getLinearDamping() { return m_motionState.m_linearDamping; }
	void setLinearDamping(float d) { m_motionState.m_linearDamping = d; }
	float getAngularDamping() { return m_motionState.m_angularDamping; }
	void setAngularDamping(float d) { m_motionState.m_angularDamping = d; }
	float getGravityFactor() { return m_gravityFactor; }

	// The SDK's hkHalf(const float&) written out: the top half of the single's bits.
	void setGravityFactor(float afGravityFactor) {
		union {
			float f;
			int32_t i;
		} kBits;
		kBits.f = afGravityFactor;
		m_gravityFactor.m_value = static_cast<int16_t>(kBits.i >> 16);
	}

	int32_t getDeactivationClass() { return m_motionState.m_deactivationClass; }

	// The SDK defaults the four integers to 0.
	void enableDeactivation(bool value, int32_t randomNumber, int32_t worldFlag0, int32_t worldFlag1,
		int32_t worldDeactivationIntegrateCounter) {
		if (value) {
			m_deactivationIntegrateCounter = static_cast<uint8_t>(0xf & randomNumber);
			setWorldSelectFlagsNeg(worldFlag0, worldFlag1, worldDeactivationIntegrateCounter);
		} else {
			m_deactivationIntegrateCounter = 0xff;
			m_deactivationNumInactiveFrames[0] = 0;
			m_deactivationNumInactiveFrames[1] = 0;
		}
	}

	bool isDeactivationEnabled() const { return m_deactivationIntegrateCounter != 0xff; }
	int32_t getNumInactiveFrames(int32_t select) { return m_deactivationNumInactiveFrames[select] & 0x7f; }

	// The inactive-frame count of the half the world's select flag does not name: the low seven bits when the
	// motion's flag (bits 14 and up) differs from the world's, else the next seven.
	int32_t getNumInactiveFramesMt(int32_t select, int32_t worldDeactivationNumInactiveFramesSelectFlag) {
		int32_t iDc = m_deactivationNumInactiveFrames[select];
		int32_t iMotionDeactivationFlag = iDc >> 14;
		int32_t iNumInactiveFrames;
		if (worldDeactivationNumInactiveFramesSelectFlag != iMotionDeactivationFlag) {
			iNumInactiveFrames = iDc & 0x7f;
		} else {
			iNumInactiveFrames = (iDc >> 7) & 0x7f;
		}
		return iNumInactiveFrames & 0x7f;
	}

	// Keeps the old count in bits 7 to 13, the count plus one (saturating at 64) in bits 0 to 6 and the world's flag
	// in bits 14 and up.
	void incrementNumInactiveFramesMt(int32_t select, int32_t worldDeactivationNumInactiveFramesSelectFlag) {
		int32_t iDc = m_deactivationNumInactiveFrames[select];
		iDc = iDc & 0x7f;
		int32_t iNewDc = iDc + 1 - (iDc >> 6);
		iDc = iNewDc | (iDc << 7) | (worldDeactivationNumInactiveFramesSelectFlag << 14);
		m_deactivationNumInactiveFrames[select] = static_cast<uint16_t>(iDc);
	}

	// Keeps the old count in bits 7 to 13, a zero count in bits 0 to 6 and the world's flag in bits 14 and up.
	void zeroNumInactiveFramesMt(int32_t select, int32_t worldDeactivationNumInactiveFramesSelectFlag) {
		int32_t iDc = m_deactivationNumInactiveFrames[select];
		iDc = iDc & 0x7f;
		iDc = (iDc << 7) | (worldDeactivationNumInactiveFramesSelectFlag << 14);
		m_deactivationNumInactiveFrames[select] = static_cast<uint16_t>(iDc);
	}

	hkMotionState* getMotionState() { return &m_motionState; }
	const hkMotionState* getMotionState() const { return &m_motionState; }
	const hkVector4& getLinearVelocity() const { return m_linearVelocity; }
	const hkVector4& getAngularVelocity() const { return m_angularVelocity; }

	hkEnum<MotionType, uint8_t> m_type;
	uint8_t m_deactivationIntegrateCounter;
	uint16_t m_deactivationNumInactiveFrames[2];
	uint8_t padding0E[2];
	hkMotionState m_motionState;
	hkVector4 m_inertiaAndMassInv;
	hkVector4 m_linearVelocity;
	hkVector4 m_angularVelocity;
	hkVector4 m_deactivationRefPosition[2];
	uint32_t m_deactivationRefOrientation[2];
	hkpMaxSizeMotion* m_savedMotion;
	uint16_t m_savedQualityTypeIndex;
	hkHalf m_gravityFactor;
};

// A keyframed body's motion.
class hkpKeyframedRigidMotion : public hkpMotion {
protected:
	// The SDK's protected default constructor, for hkpMaxSizeMotion: hkpMotion's finish constructor, not
	// finishing, and no saved motion (as hkpEntity's constructor leaves its m_motion).
	hkpKeyframedRigidMotion() : hkpMotion(hkFinishLoadedObjectFlag{0}) { m_savedMotion = nullptr; }

public:
	hkpKeyframedRigidMotion(const hkVector4& position, const hkQuaternion& rotation);
	hkpKeyframedRigidMotion(hkFinishLoadedObjectFlag flag) : hkpMotion(flag) {}
	~hkpKeyframedRigidMotion() override;

	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setMass(float m) override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void setMassInv(float mInv) override;
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
	void getInertiaInvWorld(hkMatrix3& inertiaInvOut) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void getProjectedPointVelocity(const hkVector4& pos, const hkVector4& normal, float& velOut,
		float& invVirtMassOut) const override;
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	void applyLinearImpulse(const hkVector4& imp) override;
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
	// Havok SDK signature (hkpDynamics.lib, tied by vtable).
	virtual void setStepPosition(float position, float timestep);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
	virtual void setStoredMotion(hkpMaxSizeMotion* savedMotion);

	static const hkClass& staticClass();
};

// The motion an entity embeds: large enough for any motion type, which is switched in place.
class hkpMaxSizeMotion : public hkpKeyframedRigidMotion {
public:
	// The SDK's inline constructors, which hkpEntity's constructors inline for their m_motion.
	hkpMaxSizeMotion() : hkpKeyframedRigidMotion() {}
	hkpMaxSizeMotion(hkFinishLoadedObjectFlag flag) : hkpKeyframedRigidMotion(flag) {}
	~hkpMaxSizeMotion() override;

	static const hkClass& staticClass();
};

extern hkClass hkpKeyframedRigidMotionClass;
extern hkTypeInfo hkpKeyframedRigidMotionTypeInfo;
extern hkClass hkpMaxSizeMotionClass;
extern hkTypeInfo hkpMaxSizeMotionTypeInfo;

// The class's reflection record.
extern hkClass hkpMotionClass;
