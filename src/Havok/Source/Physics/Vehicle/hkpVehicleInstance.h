#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/Brake/hkpVehicleBrake.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

struct hkStepInfo;
class hkpPhantom;
class hkpRigidBody;
class hkpTyremarksInfo;
class hkpVehicleData;
class hkpVehicleEngine;
struct hkpVehicleFrictionSolverAxleParams;
struct hkpVehicleFrictionSolverParams;
class hkpVehicleSteering;
class hkpVehicleSuspension;
class hkpVehicleVelocityDamper;
class hkpVelocityAccumulator;
class hkpWorld;

void cleanupLoadedObjecthkpVehicleInstance(void* p);

// The friction solver's state of the two axles.
struct hkpVehicleFrictionStatus {
	struct AxisStatus {
		// Havok SDK signature (hkpVehicle.lib, tied by target).
		static const hkClass& staticClass();

		float m_forward_slip_velocity;
		float m_side_slip_velocity;
		float m_skid_energy_density;
		float m_side_force;
		float m_delayed_forward_impulse;
		float m_sideRhs;
		float m_forwardRhs;
		float m_relativeSideForce;
		float m_relativeForwardForce;
	};

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	AxisStatus m_axis[2];
};

// A vehicle: a unary action on the chassis that steps the vehicle's components (driver input, steering, engine,
// transmission, brake, suspension, aerodynamics, wheel collision) each physics step.
class hkpVehicleInstance : public hkpUnaryAction {
public:
	// The per-wheel state: the contact the wheel collision found, the suspension and spin axes in world space, the
	// spin and the friction results. Layout from the exe's hkpVehicleInstanceWheelInfoClass record (0x126B180),
	// matching the PC PDB.
	struct WheelInfo {
		enum {
			MAX_NUM_SHAPE_KEYS = 8
		};

		// Resets the wheel's state.
		void init();
		// Copies other's members, member by member, and returns this.
		WheelInfo& operator=(const WheelInfo& other);

		static const hkClass& staticClass();

		hkContactPoint m_contactPoint;
		float m_contactFriction;
		hkpRigidBody* m_contactBody;
		// The SDK's hkpShapeKey, an unsigned 32-bit integer.
		uint32_t m_contactShapeKey[MAX_NUM_SHAPE_KEYS];
		hkVector4 m_hardPointWs;
		hkVector4 m_rayEndPointWs;
		float m_currentSuspensionLength;
		hkVector4 m_suspensionDirectionWs;
		hkVector4 m_spinAxisChassisSpace;
		hkVector4 m_spinAxisWs;
		hkQuaternion m_steeringOrientationChassisSpace;
		float m_spinVelocity;
		float m_spinAngle;
		float m_skidEnergyDensity;
		float m_sideForce;
		float m_forwardSlipVelocity;
		float m_sideSlipVelocity;
	};

	hkpVehicleInstance(hkpRigidBody* chassis);

	// The SDK's finish constructor: a loaded vehicle keeps the fields the load wrote.
	hkpVehicleInstance(hkFinishLoadedObjectFlag aFlag)
		: hkpUnaryAction(aFlag.m_finishing), m_wheelsInfo(aFlag), m_isFixed(aFlag), m_wheelsSteeringAngle(aFlag) {}

	~hkpVehicleInstance() override;

	void applyAction(const hkStepInfo& stepInfo) override;
	void getPhantoms(hkArray<hkpPhantom*>& phantomsOut) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;
	virtual void init();
	virtual void addToWorld(hkpWorld* world);
	virtual void removeFromWorld();
	// Parameter names from the Havok SDK; the PDB's (suspLen, spin_rotation) do not fit the exe's five arguments.
	virtual void calcCurrentPositionAndRotation(const hkpRigidBody* chassis, const hkpVehicleSuspension* suspension,
		int32_t wheelNo, hkVector4& posOut, hkQuaternion& rotOut);
	virtual float calcRPM() const;
	virtual float calcKMPH() const;
	virtual float calcMPH() const;
	virtual void handleFixedGroundAccum(hkpRigidBody* ground, hkpVelocityAccumulator& accum);

	// Updates the components, collides the wheels and simulates the vehicle for the step.
	void stepVehicle(const hkStepInfo& stepInfo);

	// Prepares the components before the wheels are collided.
	void updateBeforeCollisionDetection();

	// Simulates the vehicle for the step from the wheels' collision results.
	void stepVehicleUsingWheelCollideOutput(const hkStepInfo& stepInfo,
		const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo);

	// Runs the steering component on a copy of the steering angles and takes its result (thiscall, ret 8,
	// 0xE38D50). Protected in the PDB.
	void updateSteering(float deltaTime,
		const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo);
	// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc).
	void updateDriverInput(float deltaTime, hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo);
	// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc).
	void updateTransmission(float deltaTime, hkpVehicleTransmission::TransmissionOutput& transmissionInfo);
	// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc).
	void updateEngine(float deltaTime, const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo,
		const hkpVehicleTransmission::TransmissionOutput& transmissionInfo);
	// Havok SDK name and signature; the exe's perWheelBehavior (0xE375A0) calls it for each
	// wheel (thiscall, ret 0xC).
	void applySuspensionForcesAtWheel(float deltaTime, int32_t w_it, float suspensionForceAtWheel);
	// Havok SDK name and signature; simulateVehicle calls it first, with its
	// two-element ground body array and &frictionParams.m_axleParams.
	void perWheelBehavior(float deltaTime, const hkArray<float, hkContainerHeapAllocator>& suspensionForceAtWheel,
		const hkArray<float, hkContainerHeapAllocator>& totalLinearForceAtWheel, hkpRigidBody** groundBody,
		hkpVehicleFrictionSolverAxleParams* axleParams);
	// Havok SDK signature (hkpVehicle.lib, tied by reloc).
	void updateWheels(float deltaTime, const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo);
	// Havok SDK name and signature; simulateVehicle calls it with
	// frictionParams.m_axleParams and its ground body array.
	void applyResultsToGround(float deltaTime, const hkpVehicleFrictionSolverAxleParams* axleParams,
		hkpRigidBody** groundBody);
	// Havok SDK signature (hkpVehicle.lib, tied by reloc,string).
	void simulateVehicle(const hkStepInfo& stepInfo,
		const hkpVehicleAerodynamics::AerodynamicsDragOutput& aerodynamicsDragInfo,
		const hkArray<float, hkContainerHeapAllocator>& suspensionForceAtWheel,
		const hkArray<float, hkContainerHeapAllocator>& totalLinearForceAtWheel);
	void updateBrake(float deltaTime, const hkpVehicleDriverInput::FilteredDriverInputOutput& filteredDriverInputInfo,
		hkpVehicleBrake::WheelBreakingOutput& wheelBreakingInfo);
	// Havok SDK: m_aerodynamics->calcAerodynamics(deltaTime, this, aerodynamicsDragInfo).
	void updateAerodynamics(float deltaTime, hkpVehicleAerodynamics::AerodynamicsDragOutput& aerodynamicsDragInfo);
	// Havok SDK: makes chassis the action's entity; the exe's body is a jump to
	// hkpUnaryAction::setEntity (0xD2B210) (thiscall, ret 4, 0xE36F40).
	void setChassis(hkpRigidBody* chassis);
	// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc,string).
	void updateComponents(const hkStepInfo& stepInfo,
		const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo,
		hkpVehicleAerodynamics::AerodynamicsDragOutput& aerodynamicsDragInfo,
		hkArray<float, hkContainerHeapAllocator>& suspensionForceAtWheel,
		hkArray<float, hkContainerHeapAllocator>& totalLinearForceAtWheel);
	// The PDB's operator=: copies other's members and arrays and returns this (thiscall, ret 4, 0xE39480).
	hkpVehicleInstance& operator=(const hkpVehicleInstance& other);
	// Havok SDK name and signature; perWheelBehavior (0xE375A0) passes the wheel's axle's
	// entry of axleParams (thiscall, ret 0x14; eax is left as &axle_params, which the caller does not read).
	void getAxleParamsFromWheel(int32_t w_it, float linearForceAtWheel, float suspensionForceAtWheel,
		float estimatedCarSpeed, hkpVehicleFrictionSolverAxleParams& axle_params);
	// Havok SDK name and signature; simulateVehicle (0xE385E0) passes &frictionParams.m_chassis (thiscall, ret 8;
	// ecx is left as this, which the caller reads).
	void getExtraTorqueFactor(float deltaTime, hkpVelocityAccumulator& accumulatorForChassis) const;
	// Havok SDK name and signature; simulateVehicle passes &frictionParams.
	void prepareFrictionSolverParams(const hkStepInfo& stepInfo, hkpVehicleFrictionSolverParams& frictionParams);
	// Havok SDK name and signature; simulateVehicle passes its ground body array,
	// its two arrays of two accumulators and frictionParams.m_axleParams.
	void getAxleParams(const hkStepInfo& stepInfo, hkpRigidBody** groundBody, hkpVelocityAccumulator* groundAccum,
		hkpVelocityAccumulator* groundAccumAtIntegration, hkpVehicleFrictionSolverAxleParams* axleParams);
	// Havok SDK signature (hkpVehicle.lib, tied by bytes).
	void applyFrictionSolver(const hkStepInfo& stepInfo, hkpVehicleFrictionSolverParams& frictionParams);
	// Havok SDK name and signature; simulateVehicle passes &frictionParams.m_chassis.
	void applyResultsToChassis(const hkpVelocityAccumulator& accumulatorForChassis);
	// Havok SDK name and signature; simulateVehicle passes its ground body array
	// and &frictionParams.
	void applyResultsToWheelInfo(float deltaTime, hkpRigidBody* const* groundBody,
		const hkpVehicleFrictionSolverParams& frictionParams);
	// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc).
	void updateSuspension(float deltaTime, const hkpVehicleWheelCollide::CollisionDetectionWheelOutput* cdInfo,
		hkArray<float, hkContainerHeapAllocator>& suspensionForces);

	static const hkClass& staticClass();

	hkpVehicleData* m_data;
	hkpVehicleDriverInput* m_driverInput;
	hkpVehicleSteering* m_steering;
	hkpVehicleEngine* m_engine;
	hkpVehicleTransmission* m_transmission;
	hkpVehicleBrake* m_brake;
	hkpVehicleSuspension* m_suspension;
	hkpVehicleAerodynamics* m_aerodynamics;
	hkpVehicleWheelCollide* m_wheelCollide;
	hkpTyremarksInfo* m_tyreMarks;
	hkpVehicleVelocityDamper* m_velocityDamper;
	hkArray<WheelInfo> m_wheelsInfo;
	hkpVehicleFrictionStatus m_frictionStatus;
	hkpVehicleDriverInputStatus* m_deviceStatus;
	hkArray<hkBool> m_isFixed;
	float m_wheelsTimeSinceMaxPedalInput;
	hkBool m_tryingToReverse;
	float m_torque;
	float m_rpm;
	float m_mainSteeringAngle;
	hkArray<float> m_wheelsSteeringAngle;
	hkBool m_isReversing;
	int8_t m_currentGear;
	hkBool m_delayed;
	float m_clutchDelayCountdown;
};

extern hkClass hkpVehicleInstanceWheelInfoClass;
extern hkClass hkpVehicleInstanceClass;
extern hkTypeInfo hkpVehicleInstanceTypeInfo;
inline constexpr fnvd::Vtable hkpVehicleInstance_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpVehicleInstanceWheelInfoTypeInfo;
extern hkTypeInfo hkpVehicleFrictionStatusAxisStatusTypeInfo;
extern hkTypeInfo hkpVehicleFrictionStatusTypeInfo;
extern hkClass hkpVehicleFrictionStatusClass;
extern hkClass hkpVehicleFrictionStatusAxisStatusClass;
