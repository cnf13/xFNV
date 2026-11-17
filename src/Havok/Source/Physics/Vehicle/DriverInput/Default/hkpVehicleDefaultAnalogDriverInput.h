#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"

#include <stdint.h>

void cleanupLoadedObjecthkpVehicleDefaultAnalogDriverInput(void* p);

// The status of an analog device: the stick position and the two buttons.
class hkpVehicleDriverInputAnalogStatus : public hkpVehicleDriverInputStatus {
public:
    hkpVehicleDriverInputAnalogStatus() {}
    hkpVehicleDriverInputAnalogStatus(hkFinishLoadedObjectFlag aFlag) : hkpVehicleDriverInputStatus(aFlag) {}
    ~hkpVehicleDriverInputAnalogStatus() override;

    hkpVehicleDriverInputStatus* clone() const override;

    static const hkClass& staticClass();

    float m_positionX;
    float m_positionY;
    hkBool m_handbrakeButtonPressed;
    hkBool m_reverseButtonPressed;
};

// The driver input of an analog device: the pedals follow the stick's y and the wheel its x, with a dead zone and a
// slope that changes at a point.
class hkpVehicleDefaultAnalogDriverInput : public hkpVehicleDriverInput {
public:
    hkpVehicleDefaultAnalogDriverInput();
    hkpVehicleDefaultAnalogDriverInput(hkFinishLoadedObjectFlag aFlag) : hkpVehicleDriverInput(aFlag) {}
    ~hkpVehicleDefaultAnalogDriverInput() override;

    void calcDriverInput(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInputStatus* deviceStatus,
        FilteredDriverInputOutput& filteredInputOut
    ) override;
    virtual float calcAcceleratorInput(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInputAnalogStatus* deviceStatus,
        FilteredDriverInputOutput& input
    ) const;
    virtual float calcBrakeInput(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInputAnalogStatus* deviceStatus,
        FilteredDriverInputOutput& FilteredDriverInputOutputOut
    ) const;
    virtual hkBool* calcTryingToReverse(
        hkBool* arResult,
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInputAnalogStatus* deviceStatus,
        FilteredDriverInputOutput& filteredDriverInputOutputOut
    );
    virtual float calcSteeringInput(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInputAnalogStatus* deviceStatus,
        FilteredDriverInputOutput& FilteredDriverInputOutputOut
    ) const;

    static const hkClass& staticClass();

    float m_slopeChangePointX;
    float m_initialSlope;
    float m_deadZone;
    hkBool m_autoReverse;
};

extern hkClass hkpVehicleDriverInputAnalogStatusClass;
extern hkTypeInfo hkpVehicleDriverInputAnalogStatusTypeInfo;
extern hkClass hkpVehicleDefaultAnalogDriverInputClass;
extern hkTypeInfo hkpVehicleDefaultAnalogDriverInputTypeInfo;
