#pragma once

#include "Havok/Source/Physics/Vehicle/Engine/hkpVehicleEngine.h"

// An engine with a torque and resistance curve through the minimum, optimal and maximum RPM, and a clutch slip RPM.
class hkpVehicleDefaultEngine : public hkpVehicleEngine {
public:
    hkpVehicleDefaultEngine();
    hkpVehicleDefaultEngine(hkFinishLoadedObjectFlag aFlag) : hkpVehicleEngine(aFlag) {}
    ~hkpVehicleDefaultEngine() override;

    void calcEngineInfo(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo,
        const hkpVehicleTransmission::TransmissionOutput& transmissionInfo,
        EngineOutput& engineOutput
    ) override;

    // Havok SDK signature (hkpVehicle.lib, tied by target).
    static const hkClass& staticClass();

    float m_minRPM;
    float m_optRPM;
    float m_maxRPM;
    float m_maxTorque;
    float m_torqueFactorAtMinRPM;
    float m_torqueFactorAtMaxRPM;
    float m_resistanceFactorAtMinRPM;
    float m_resistanceFactorAtOptRPM;
    float m_resistanceFactorAtMaxRPM;
    float m_clutchSlipRPM;
};

extern hkClass hkpVehicleDefaultEngineClass;

extern hkTypeInfo hkpVehicleDefaultEngineTypeInfo;
