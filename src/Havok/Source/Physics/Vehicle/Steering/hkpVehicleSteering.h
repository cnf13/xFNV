#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"

class hkpVehicleInstance;

// The part of a vehicle that turns the steering wheel input into the main steering angle and each wheel's angle. It is
// abstract.
class hkpVehicleSteering : public hkReferencedObject {
public:
    // What calcSteering produces for the step.
    struct SteeringAnglesOutput {
        float m_mainSteeringAngle;
        hkArray<float> m_wheelsSteeringAngle;
    };

    // The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
    // classes' finish constructors call it): it keeps the fields the load wrote.
    hkpVehicleSteering() = default;
    hkpVehicleSteering(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

    virtual void calcSteering(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredInfoOutput,
        SteeringAnglesOutput& steeringOutput
    ) = 0;
};

// The class's reflection record.
extern hkClass hkpVehicleSteeringClass;
