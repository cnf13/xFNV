#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

class hkpVehicleInstance;

// The part of a vehicle that damps the chassis's angular velocity each step. It is abstract.
class hkpVehicleVelocityDamper : public hkReferencedObject {
public:
	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleVelocityDamper() = default;
	hkpVehicleVelocityDamper(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

	virtual void applyVelocityDamping(float deltaTime, hkpVehicleInstance& vehicle) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleVelocityDamperClass;
