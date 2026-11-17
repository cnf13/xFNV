#pragma once

#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

class hkpRigidBodyCinfo;

// Sets the collidable's allowed penetration depth from the shape's extent.
void estimateAllowedPenetrationDepth(const hkVector4& arExtent, hkpCollidable* apCollidable);
void cleanupLoadedObjecthkpRigidBody(void* p);
// The type info's finish-loaded-object callback for hkpRigidBody.
void finishLoadedObjecthkpRigidBody(void* p, int32_t finishing);

// A Havok rigid body. Only the entry points this code calls are declared.
class hkpRigidBody : public hkpEntity {
public:
	hkpRigidBody(const hkpRigidBodyCinfo& info);
	// The SDK's finish constructor of a loaded object: the entity's, keeping the fields the load wrote.
	hkpRigidBody(hkFinishLoadedObjectFlag aFlag) : hkpEntity(aFlag) {}
	// The virtual functions hkpRigidBody adds beyond hkpEntity's: setShape overrides hkpWorldObject's, and clone is new
	// (a copy of the body).
	~hkpRigidBody() override;

	hkWorldOperation::Result setShape(const hkpShape* shape) override;
	virtual hkpRigidBody* clone() const;

protected:
	hkMotionState* getMotionState() override;

public:
	// The motion kept for the dynamic motion types. The const overload, which returns const hkpMotion*, is not
	// declared.
	hkpMotion* getStoredDynamicMotion();
	const hkTransform& getTransform() const { return getRigidMotion()->getTransform(); }
	// The body's position, its motion's translation.
	const hkVector4& getTranslation() const;
	// Warps the body to position.
	void setPosition(const hkVector4& position);
	// Warps the body to rotation.
	void setRotation(const hkQuaternion& rotation);
	// Warps the body to position and rotation.
	void setPositionAndRotation(const hkVector4& position, const hkQuaternion& rotation);

	const hkVector4& getLinearVelocity() const { return getRigidMotion()->getLinearVelocity(); }
	const hkVector4& getAngularVelocity() const { return getRigidMotion()->getAngularVelocity(); }
	// Sets the body's linear velocity.
	void setLinearVelocity(const hkVector4& newVel);
	// Sets the body's angular velocity.
	void setAngularVelocity(const hkVector4& newVel);
	// Sets the body's mass.
	void setMass(float m);

	void applyLinearImpulse(const hkVector4& imp) {
		activate();
		getRigidMotion()->applyLinearImpulse(imp);
	}

	void applyAngularImpulse(const hkVector4& imp) {
		activate();
		getRigidMotion()->applyAngularImpulse(imp);
	}

	bool isDeactivationEnabled() const;
	// Seeds the motion's deactivation counters when _enableDeactivation, else marks deactivation off.
	void enableDeactivation(bool _enableDeactivation);
	// The type of the body's motion.
	hkpMotion::MotionType getMotionType();
	void setFriction(float a1, float a2);
	void setRestitution(float a1);
	// The rest of the hkpRigidBody members are declared only. hkReal is float and hkTime float. getRigidMotion returns
	// the motion as the rigid body's, its storage being the entity's m_motion. The const accessors (getMass,
	// getPosition, getTransform...) call it.
	hkpMotion* getRigidMotion() const { return const_cast<hkpMaxSizeMotion*>(&m_motion); }
	float getMass() const { return getRigidMotion()->getMass(); }
	float getMassInv() const { return getRigidMotion()->getMassInv(); }
	void setMassInv(float mInv);
	void getInertiaLocal(hkMatrix3& inertia) const { getRigidMotion()->getInertiaLocal(inertia); }
	void getInertiaWorld(hkMatrix3& arInertiaOut) const { getRigidMotion()->getInertiaWorld(arInertiaOut); }
	void setInertiaLocal(const hkMatrix3& inertia);
	void setInertiaInvLocal(const hkMatrix3& inertiaInv);
	void getInertiaInvLocal(hkMatrix3& inertiaInv) const { getRigidMotion()->getInertiaInvLocal(inertiaInv); }
	void getInertiaInvWorld(hkMatrix3& inertiaInv) const { getRigidMotion()->getInertiaInvWorld(inertiaInv); }
	void setCenterOfMassLocal(const hkVector4& centerOfMass);
	const hkVector4& getCenterOfMassLocal() const { return getRigidMotion()->getCenterOfMassLocal(); }
	const hkVector4& getCenterOfMassInWorld() const { return getRigidMotion()->getCenterOfMassInWorld(); }
	// The position is the translation returned by getTranslation above.
	const hkVector4& getPosition() const { return getRigidMotion()->getPosition(); }
	const hkQuaternion& getRotation() const { return getRigidMotion()->getRotation(); }
	void setTransform(const hkTransform& transform);
	void setPositionAndRotationAsCriticalOperation(const hkVector4& position, const hkQuaternion& rotation);

	void approxTransformAt(float time, hkTransform& transformOut) const {
		getRigidMotion()->approxTransformAt(time, transformOut);
	}

	// The body's transform at the world's current time.
	void approxCurrentTransform(hkTransform& arTransformOut) const {
		getRigidMotion()->approxTransformAt(getWorld()->getCurrentTime(), arTransformOut);
	}

	void getPointVelocity(const hkVector4& p, hkVector4& vecOut) const {
		getRigidMotion()->getPointVelocity(p, vecOut);
	}

	void setLinearVelocityAsCriticalOperation(const hkVector4& newVel);
	void setAngularVelocityAsCriticalOperation(const hkVector4& newVel);

	void applyPointImpulse(const hkVector4& imp, const hkVector4& p) {
		activate();
		getRigidMotion()->applyPointImpulse(imp, p);
	}

	void applyLinearImpulseAsCriticalOperation(const hkVector4& imp);
	void applyPointImpulseAsCriticalOperation(const hkVector4& imp, const hkVector4& p);
	void applyAngularImpulseAsCriticalOperation(const hkVector4& imp);

	void applyForce(float afDeltaTime, const hkVector4& arForce) {
		activate();
		getRigidMotion()->applyForce(afDeltaTime, arForce);
	}

	void applyForce(float afDeltaTime, const hkVector4& arForce, const hkVector4& arPoint) {
		activate();
		getRigidMotion()->applyForce(afDeltaTime, arForce, arPoint);
	}

	void applyTorque(float deltaTime, const hkVector4& torque) {
		activate();
		getRigidMotion()->applyTorque(deltaTime, torque);
	}

	float getLinearDamping() const { return getRigidMotion()->getLinearDamping(); }
	void setLinearDamping(float d) { getRigidMotion()->setLinearDamping(d); }
	float getAngularDamping() const { return getRigidMotion()->getAngularDamping(); }
	void setAngularDamping(float d) { getRigidMotion()->setAngularDamping(d); }
	float getGravityFactor() const { return m_motion.m_gravityFactor; }

	// The SDK's hkHalf::operator=(float): the top half of the single's bits.
	void setGravityFactor(float afFactor) {
		union {
			float f;
			int32_t i;
		} kBits;
		kBits.f = afFactor;
		m_motion.m_gravityFactor.m_value = static_cast<int16_t>(kBits.i >> 16);
	}

	float getMaxLinearVelocity() const { return getRigidMotion()->getMotionState()->m_maxLinearVelocity; }

	void setMaxLinearVelocity(float maxVel) { getRigidMotion()->getMotionState()->m_maxLinearVelocity = maxVel; }

	float getMaxAngularVelocity() const { return getRigidMotion()->getMotionState()->m_maxAngularVelocity; }

	void setMaxAngularVelocity(float afMaxVelocity) {
		getRigidMotion()->getMotionState()->m_maxAngularVelocity = afMaxVelocity;
	}

	uint32_t getCollisionFilterInfo() const { return m_collidable.getCollisionFilterInfo(); }
	void setCollisionFilterInfo(uint32_t info) { m_collidable.setCollisionFilterInfo(info); }
	float getAllowedPenetrationDepth() const { return m_collidable.m_allowedPenetrationDepth; }
	void setAllowedPenetrationDepth(float afDepth) { m_collidable.m_allowedPenetrationDepth = afDepth; }
	float getFriction() const { return m_material.m_friction; }
	float getRestitution() const { return m_material.m_restitution; }
	void updateCachedShapeInfo(const hkpShape* shape, hkVector4& extentOut);
	// The construction info the body was made from, rebuilt from its state.
	void getCinfo(hkpRigidBodyCinfo& info) const;
	hkpCollidableQualityType getQualityType() const { return m_collidable.getQualityType(); }
	void setQualityType(hkpCollidableQualityType type) { m_collidable.setQualityType(type); }
	void setMotionType(hkpMotion::MotionType newState, hkpEntityActivation preferredActivationState,
		hkpUpdateCollisionFilterOnEntityMode collisionFilterUpdateMode);
	hkBool* checkPerformance(hkBool* arResult) const;
	hkpMotion* getStoredDynamicMotion_0();
	char* checkPerformance(char* apResult);

	// The const getStoredDynamicMotion overload is not declared. createDynamicRigidMotion builds the motion that a
	// dynamic body of the given type moves with.
	static void createDynamicRigidMotion(hkpMotion::MotionType motionType, const hkVector4& position,
		const hkQuaternion& rotation, float mass, const hkMatrix3& inertiaLocal, const hkVector4& centreOfMassLocal,
		float maxLinearVelocity, float maxAngularVelocity, hkpMaxSizeMotion* motionBufferOut);
	// Puts the body, warped to a new place, into the broadphase again and drops its collision information.
	static void updateBroadphaseAndResetCollisionInformationOfWarpedBody(hkpEntity* entity);
	static const hkClass& staticClass();
};

extern hkClass hkpRigidBodyClass;
extern hkTypeInfo hkpRigidBodyTypeInfo;
