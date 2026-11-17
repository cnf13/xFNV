#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Vehiclefriction/hkpVehicleFrictionSolverInternal.h"
#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "Reconstruction.hpp"

// What the vehicle friction solver takes for one axle: the contact frame in world space, the ground's velocity
// accumulators and the friction and force inputs. The SSE build's 16-byte vectors make it 0x60 bytes.
struct hkpVehicleFrictionSolverAxleParams {
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
// surface inertia, down force factors, chassis mass ratio and friction circle table (the SDK's signature; cdecl,
// ret 0, 0xE593F0).
void hkVehicleFrictionDescriptionInitValues(const hkpVehicleFrictionDescription::Cinfo& ci,
	hkpVehicleFrictionDescription& out);
// Not named in the PDB or IDA; name chosen here. A helper of hkVehicleFrictionApplyVehicleFriction (its one caller,
// 0xE59A40): inverts the 2x2 matrix apMatrix (four floats) into apInverse when its determinant's square is at least
// afTolerance's; whether it did, built in apResult (a by-value hkBool, cdecl, the result pointer first).
hkBool* invert2x2(hkBool* apResult, const float* apMatrix, float* apInverse, float afTolerance);
// The PDB's hkVehicleFrictionSolverCalcRelativeForces (Vehiclefriction\hkpVehicleFrictionSolver.cpp), matched by what
// it computes (the PDB lists no parameters): one axle's relative side and forward forces, each impulse times the
// inverse of its limit. The exe takes apStatus in ecx and apAxis in eax; no exe code calls it.
void hkVehicleFrictionSolverCalcRelativeForces(hkpVehicleFrictionStatus::AxisStatus* axis,
	const hkpVehicleFrictionAxisTmp* status);
// Not named in the PDB or IDA; name chosen here. One axle's impulses clipped to its friction circle; the exe takes
// apDescr in edx, apStatus in edi and apAxis in esi.
void hkVehicleFrictionSolverClipImpulse(const hkpVehicleFrictionDescription::AxisDescription* apDescr,
	hkpVehicleFrictionStatus::AxisStatus* apStatus, hkpVehicleFrictionAxisTmp* apAxis);
// The SDK's hkVehicleFrictionApplyVehicleFriction (hkpVehicleFrictionSolver.h; cdecl, ret 0).
void hkVehicleFrictionApplyVehicleFriction(const hkpVehicleStepInfo& stepInfo,
	const hkpVehicleFrictionDescription& descr, hkpVehicleFrictionSolverParams& params,
	hkpVehicleFrictionStatus& status);

// The SDK's name (release hkpConstraintSolver.lib, hkpVehicleFrictionSolver.obj: the file static
// `_HK_flyingcolors_vehicle`, one byte of .bss): the flag hkpProcessFlyingColors fills for
// hkVehicleFrictionApplyVehicleFriction, which does nothing while it is clear. Declared here while that body sits in
// hkpVehicleInstance.cpp.
inline uint8_t HK_flyingcolors_vehicle;
