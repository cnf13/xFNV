#pragma once

#include "BSHavok/hkEntityCinfo.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

class bhkRigidBody551Cinfo;

// The creation info a bhkRigidBody builds its Havok body from: the entity info plus the Havok one for the body.
class bhkRigidBodyCinfo : public hkEntityCinfo {
public:
	// The serialized form (bhkSerialized.h): the 32-bit layout, 0xE0 bytes, which files from stream version 35 on
	// hold (bhkRigidBody::LoadCInfo; the game's own files are older and hold bhkRigidBody551Cinfo's).
	struct Serialized : hkEntityCinfo::Serialized {
		uint8_t padding1C[4];
		// hkpRigidBodyCinfo, at 0x20.
		uint32_t m_collisionFilterInfo;
		bhkSerializedPointer m_shape;
		bhkSerializedPointer m_localFrame;
		int8_t m_collisionResponse;
		uint8_t padding2D;
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
		int8_t m_motionType;
		int8_t m_enableDeactivation;
		int8_t m_solverDeactivation;
		int8_t m_qualityType;
		int8_t m_autoRemoveLevel;
		uint8_t m_responseModifierFlags;
		int8_t m_numShapeKeysInContactPointProperties;
		int8_t m_forceCollideOntoPpu;
		uint8_t paddingDC[4];
	};

	bhkRigidBodyCinfo();
	~bhkRigidBodyCinfo();

	// Sets the Havok body's motion type, an hkpMotion::MotionType.
	void SetMotionType(hkpMotion::MotionType aeType);
	// Sets the Havok body's rotation through hkQuaternion::operator=.
	void SetRotation(const hkQuaternion& arRot);
	// Stores arPos as the Havok body's position, hk.m_position.
	void SetPosition(const hkVector4& arPos);
	void SetFriction(float afFriction);
	// Sets the Havok body's mass, hk.m_mass.
	void SetMass(float afMass);
	void SetCollisionFilter(uint32_t aiFilter);
	// Sets the shape of the creation info and of the Havok body from a game shape's Havok
	// shape, the null-safe Gethk.
	void SetShape(const bhkShape* apShape);
	// Sets the shape of the creation info and of the Havok body from a Havok shape.
	void SetShape(const hkpShape* apShape);
	void SetRestitution(float afRest);
	// Byte-swaps every element of the creation info in place.
	void Endian();
	void DestroyPropertiesArray();
	void Scale(float afScale);
	// The quality type's name, null for a value without one.
	const char* GetQualityTypeString();
	void ConvertFromOld(bhkRigidBody551Cinfo* apOldCInfo);
	// Fills the info from its serialized form; the shapes and the local frame are left null (the world object's link
	// binds the shape, Init copies it into hk).
	void ReadSerialized(const Serialized& arForm);

	// Byte-swaps every element of arMatrix in place.
	static void EndianMatrix(hkMatrix3& arMatrix);

	// Returns afValue with its four bytes reversed.
	static float SwapFloat(float afValue) {
		union {
			float f;
			uint32_t ui;
		} kBits;
		kBits.f = afValue;
		kBits.ui = __builtin_bswap32(kBits.ui);
		return kBits.f;
	}

	hkpRigidBodyCinfo hk;
};

static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_collisionFilterInfo) == 0x20);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_contactPointCallbackDelay) == 0x2E);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_position) == 0x30);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_centerOfMass) == 0xA0);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_mass) == 0xB0);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_allowedPenetrationDepth) == 0xD0);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_motionType) == 0xD4);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_forceCollideOntoPpu) == 0xDB);
static_assert(sizeof(bhkRigidBodyCinfo::Serialized) == 0xE0);
