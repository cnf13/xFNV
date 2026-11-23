#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Vehiclefriction/hkpVehicleFrictionSolverInternal.h"
#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

// What the vehicle friction solver takes for one axle: the contact frame in world space, the ground's velocity
// accumulators and the friction and force inputs. The SSE build's 16-byte vectors make it 0x60 bytes.
struct hkpVehicleFrictionSolverAxleParams {
    void initialize() {
        m_contactPointWs.setZero4();
        m_constraintNormalWs.setZero4();
        m_forwardDirWs.setZero4();
        m_frictionCoefficient = 0.0f;
        m_viscosityFrictionCoefficient = 0.0f;
        m_maxFrictionCoefficient = 0.0f;
        m_wheelDownForce = 0.0f;
        m_forwardForce = 0.0f;
        m_wheelFixed = 0;
        m_groundObject = nullptr;
        m_groundObjectAtLastIntegration = nullptr;
        m_slipVelocityFactor = 0.0f;
    }

    hkVector4 m_contactPointWs;
    hkVector4 m_constraintNormalWs;
    hkVector4 m_forwardDirWs;
    hkpVelocityAccumulator* m_groundObject;
    hkpVelocityAccumulator* m_groundObjectAtLastIntegration;
    float m_frictionCoefficient;
    float m_viscosityFrictionCoefficient;
    float m_maxFrictionCoefficient;
    float m_wheelDownForce;
    float m_forwardForce;
    float m_slipVelocityFactor;
    uint8_t m_wheelFixed;
};

// Everything the vehicle friction solver takes: both axles, the speed limit of positional friction and the chassis's
// velocity accumulators now and at the last integration. simulateVehicle builds it on its stack and
// passes &m_chassis (its address + 0xD0) to the steps that set up and apply the chassis's velocities.
struct hkpVehicleFrictionSolverParams {
    hkpVehicleFrictionSolverAxleParams m_axleParams[2];
    float m_maxVelocityForPositionalFriction;
    hkpVelocityAccumulator m_chassis;
    hkpVelocityAccumulator m_chassisAtLastIntegration;
};

// Builds the friction description of the vehicle's two axles from ci: the wheel distance, each axle's wheel radius,
// surface inertia, down force factors, chassis mass ratio and friction circle table.
void hkVehicleFrictionDescriptionInitValues(
    const hkpVehicleFrictionDescription::Cinfo& ci,
    hkpVehicleFrictionDescription& out
);
// A helper of hkVehicleFrictionApplyVehicleFriction: inverts the 2x2 matrix apMatrix (four floats) into apInverse when
// its determinant's square is at least afTolerance's; whether it did is written to apResult.
hkBool invert2x2(const float* apMatrix, float* apInverse, float afTolerance);
// Computes one axle's relative side and forward forces, each impulse times the inverse of its limit.
void hkVehicleFrictionSolverCalcRelativeForces(
    hkpVehicleFrictionStatus::AxisStatus* axis,
    const hkpVehicleFrictionAxisTmp* status
);
// Clips one axle's impulses to its friction circle.
void hkVehicleFrictionSolverClipImpulse(
    const hkpVehicleFrictionDescription::AxisDescription* apDescr,
    hkpVehicleFrictionStatus::AxisStatus* apStatus,
    hkpVehicleFrictionAxisTmp* apAxis
);
void hkVehicleFrictionApplyVehicleFriction(
    const hkpVehicleStepInfo& stepInfo,
    const hkpVehicleFrictionDescription& descr,
    hkpVehicleFrictionSolverParams& params,
    hkpVehicleFrictionStatus& status
);

// The SDK's name (release hkpConstraintSolver.lib, hkpVehicleFrictionSolver.obj: the file static
// `_HK_flyingcolors_vehicle`, one byte of .bss): the flag hkpProcessFlyingColors fills for
// hkVehicleFrictionApplyVehicleFriction, which does nothing while it is clear. Declared here while that body sits in
// hkpVehicleInstance.cpp.
inline uint8_t HK_flyingcolors_vehicle;
