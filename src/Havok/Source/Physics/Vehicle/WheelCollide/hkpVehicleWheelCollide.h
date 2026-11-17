#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"

class hkpPhantom;
class hkpRigidBody;
class hkpVehicleInstance;
class hkpWorld;

// The part of a vehicle that finds where each wheel touches the ground. Each kind of cast derives from it.
class hkpVehicleWheelCollide : public hkReferencedObject {
public:
	// What the collision detection finds for one wheel.
	struct CollisionDetectionWheelOutput {
		enum {
			MAX_NUM_SHAPE_KEYS = 0x8,
		};

		hkContactPoint m_contactPoint;
		float m_contactFriction;
		hkpRigidBody* m_contactBody;
		uint32_t m_contactShapeKey[MAX_NUM_SHAPE_KEYS];
		float m_currentSuspensionLength;
		float m_suspensionClosingSpeed;
		float m_suspensionScalingFactor;
	};

	enum WheelCollideType {
		INVALID_WHEEL_COLLIDE = 0x0,
		RAY_CAST_WHEEL_COLLIDE = 0x1,
		LINEAR_CAST_WHEEL_COLLIDE = 0x2,
		USER_WHEEL_COLLIDE1 = 0x3,
		USER_WHEEL_COLLIDE2 = 0x4,
		USER_WHEEL_COLLIDE3 = 0x5,
		USER_WHEEL_COLLIDE4 = 0x6,
		USER_WHEEL_COLLIDE5 = 0x7,
	};

	// The SDK's constructors.
	hkpVehicleWheelCollide() : m_alreadyUsed(false) {}

	hkpVehicleWheelCollide(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {
		if (aFlag.m_finishing) {
			m_type.m_storage = INVALID_WHEEL_COLLIDE;
		}
	}

	virtual void init(const hkpVehicleInstance* vehicle) = 0;
	virtual void collideWheels(float deltaTime, const hkpVehicleInstance* vehicle,
		CollisionDetectionWheelOutput* cdInfoOut) = 0;
	virtual void getPhantoms(hkArray<hkpPhantom*>& phantomsOut) = 0;
	virtual void updateBeforeCollisionDetection(const hkpVehicleInstance* vehicle) = 0;
	virtual hkpVehicleWheelCollide* clone(const hkpRigidBody& newChassis,
		const hkArray<hkpPhantom*>& newPhantoms) const = 0;
	virtual void addToWorld(hkpWorld* world) = 0;
	virtual void removeFromWorld() = 0;
	virtual void setCollisionFilterInfo(uint32_t filterInfo) = 0;

	virtual void wheelCollideCallback(const hkpVehicleInstance* vehicle, uint8_t wheelIndex,
		CollisionDetectionWheelOutput& cdInfo) {}

	hkBool m_alreadyUsed;
	hkEnum<WheelCollideType, uint8_t> m_type;
};

// The class's reflection record.
extern hkClass hkpVehicleWheelCollideClass;
