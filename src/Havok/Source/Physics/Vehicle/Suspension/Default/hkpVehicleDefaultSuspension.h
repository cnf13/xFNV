#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"

// A spring and damper suspension: a strength and a compression and relaxation damping per wheel.
class hkpVehicleDefaultSuspension : public hkpVehicleSuspension {
public:
    // One wheel's spring and damping.
    struct WheelSpringSuspensionParameters {
        // Havok SDK signature (hkpVehicle.lib, tied by target).
        static const hkClass& staticClass();

        float m_strength;
        float m_dampingCompression;
        float m_dampingRelaxation;
    };

    hkpVehicleDefaultSuspension(hkFinishLoadedObjectFlag aFlag)
      : hkpVehicleSuspension(aFlag), m_wheelSpringParams(aFlag) {}

    ~hkpVehicleDefaultSuspension() override;

    void calcSuspension(
        float deltaTime,
        const hkpVehicleInstance* vehicle,
        const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo,
        float* suspensionForceOut
    ) override;

    // Havok SDK signature (hkpVehicle.lib, tied by target).
    static const hkClass& staticClass();

    hkArray<WheelSpringSuspensionParameters> m_wheelSpringParams;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersTypeInfo;
extern hkClass hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass;
extern hkClass hkpVehicleDefaultSuspensionClass;
extern hkTypeInfo hkpVehicleDefaultSuspensionTypeInfo;
