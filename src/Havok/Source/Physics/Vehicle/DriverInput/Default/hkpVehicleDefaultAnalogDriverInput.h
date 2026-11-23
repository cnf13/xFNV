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
    // A whole copy of other, the size and flags and the reference count included.
    hkpVehicleDriverInputAnalogStatus(const hkpVehicleDriverInputAnalogStatus& other)
      : hkpVehicleDriverInputStatus(other), m_positionX(other.m_positionX), m_positionY(other.m_positionY),
        m_handbrakeButtonPressed(other.m_handbrakeButtonPressed), m_reverseButtonPressed(other.m_reverseButtonPressed) {
        m_memSizeAndFlags = other.m_memSizeAndFlags;
        m_referenceCount = other.m_referenceCount;
    }
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
    virtual hkBool calcTryingToReverse(
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
