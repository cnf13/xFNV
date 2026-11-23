#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"

#include <stdint.h>

// A transmission with a gear ratio per gear, shift points by RPM, a clutch delay and a torque share per wheel.
class hkpVehicleDefaultTransmission : public hkpVehicleTransmission {
public:
    hkpVehicleDefaultTransmission();

    hkpVehicleDefaultTransmission(hkFinishLoadedObjectFlag aFlag)
      : hkpVehicleTransmission(aFlag), m_gearsRatio(aFlag), m_wheelsTorqueRatio(aFlag) {}

    ~hkpVehicleDefaultTransmission() override;

    void
    calcTransmission(float deltaTime, const hkpVehicleInstance* vehicle, TransmissionOutput& transmissionOut) override;
    virtual float
    calcMainTransmittedTorque(const hkpVehicleInstance* vehicle, TransmissionOutput& transmissionOut) const;
    virtual float calcTransmissionRPM(const hkpVehicleInstance* vehicle, TransmissionOutput& transmissionOut) const;
    virtual hkBool calcIsReversing(const hkpVehicleInstance* vehicle, TransmissionOutput& transmissionOut) const;
    virtual void
    updateCurrentGear(float deltaTime, const hkpVehicleInstance* vehicle, TransmissionOutput& transmissionOut);
    virtual float getCurrentRatio(const hkpVehicleInstance* vehicle, TransmissionOutput& transmissionOut) const;

    static float calculatePrimaryTransmissionRatio(
        const float vehicleTopSpeed,
        const float wheelRadius,
        const float maxEngineRpm,
        const float topGearRatio
    );
    // Havok SDK signature (hkpVehicle.lib, tied by target).
    static const hkClass& staticClass();

    float m_downshiftRPM;
    float m_upshiftRPM;
    float m_primaryTransmissionRatio;
    float m_clutchDelayTime;
    float m_reverseGearRatio;
    hkArray<float> m_gearsRatio;
    hkArray<float> m_wheelsTorqueRatio;
};

extern hkClass hkpVehicleDefaultTransmissionClass;

extern hkTypeInfo hkpVehicleDefaultTransmissionTypeInfo;
