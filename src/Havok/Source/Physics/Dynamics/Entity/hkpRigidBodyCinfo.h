#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidableQualityType.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Reconstruction.hpp"

class hkLocalFrame;
struct hkpMassProperties;
class hkpShape;

// Everything a rigid body is built from: its shape, filter, initial position and velocity, mass properties and
// the solver settings (offsets checked against the constructor, 0xCF5EA0).
// The enumerations (collision response, motion type, solver deactivation, quality type) are stored as signed bytes.
class hkpRigidBodyCinfo {
public:
	// How eagerly the solver deactivates the body (m_solverDeactivation).
	enum SolverDeactivation : int32_t {
		SOLVER_DEACTIVATION_INVALID = 0,
		SOLVER_DEACTIVATION_OFF = 1,
		SOLVER_DEACTIVATION_LOW = 2,
		SOLVER_DEACTIVATION_MEDIUM = 3,
		SOLVER_DEACTIVATION_HIGH = 4,
		SOLVER_DEACTIVATION_MAX = 5,
	};

	// The SDK's defaults.
	hkpRigidBodyCinfo();

	void setMassProperties(const hkpMassProperties& mp);
	// Places the body at the transform's translation and rotation.
	void setTransform(const hkTransform& transform);

	uint32_t m_collisionFilterInfo;
	const hkpShape* m_shape;
	hkLocalFrame* m_localFrame;
	hkEnum<hkpMaterial::ResponseType, int8_t> m_collisionResponse;
	uint16_t m_contactPointCallbackDelay;
	hkVector4 m_position;
	hkQuaternion m_rotation;
	hkVector4 m_linearVelocity;
	hkVector4 m_angularVelocity;
	hkMatrix3 m_inertiaTensor;
	hkVector4 m_centerOfMass;
	float m_mass;
	float m_linearDamping;
	float m_angularDamping;
	float m_gravityFactor;
	float m_friction;
	float m_restitution;
	float m_maxLinearVelocity;
	float m_maxAngularVelocity;
	float m_allowedPenetrationDepth;
	hkEnum<hkpMotion::MotionType, int8_t> m_motionType;
	hkBool m_enableDeactivation;
	hkEnum<SolverDeactivation, int8_t> m_solverDeactivation;
	hkEnum<hkpCollidableQualityType, int8_t> m_qualityType;
	int8_t m_autoRemoveLevel;
	uint8_t m_responseModifierFlags;
	int8_t m_numShapeKeysInContactPointProperties;
	hkBool m_forceCollideOntoPpu;
};
