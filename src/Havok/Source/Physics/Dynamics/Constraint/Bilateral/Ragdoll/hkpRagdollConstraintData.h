#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <xmmintrin.h>

// A constraint that holds two bodies at a pivot and limits their relative rotation to a twist range and a cone (the
// joints of a ragdoll), with a motor per angle and friction. It overrides the destructor, isValid, getType,
// getRuntimeInfo and getConstraintInfo.
class hkpRagdollConstraintData : public hkpConstraintData {
public:
	// The motors' indices in m_ragdollMotors.m_motors.
	enum MotorIndex {
		MOTOR_TWIST = 0,
		MOTOR_PLANE = 1,
		MOTOR_CONE = 2,
	};

	// The atoms the constraint is made of.
	struct Atoms {
		// Havok SDK (inline).
		enum Axis {
			AXIS_TWIST = 0,
			AXIS_PLANES = 1,
			AXIS_CROSS_PRODUCT = 2,
		};

		Atoms();
		// The SDK's finish constructor: the loaded atoms keep their fields.
		Atoms(hkFinishLoadedObjectFlag) {}

		const hkpConstraintAtom* getAtoms() const { return &m_transforms; }

		// The bytes from the first atom to the end of the last: 304 on a 32-bit build; more on x64, where the motor
		// atom holds pointers.
		int32_t getSizeOfAllAtoms() const {
			return static_cast<int32_t>(reinterpret_cast<const char*>(&m_ballSocket + 1) -
				reinterpret_cast<const char*>(this));
		}

		// Copies the members one by one, leaving the motor atom's padding alone.
		Atoms& operator=(const Atoms& other);
		// The atoms copied from other (operator=), then a reference taken on each motor copied. Used by
		// hkpConstraintUtils::convertToPowered.
		void CopyAndAddMotorRefs(const Atoms& other);

		// Havok SDK signature (hkpDynamics.lib, tied by target).
		static const hkClass& staticClass();

		hkpSetLocalTransformsConstraintAtom m_transforms;
		hkpRagdollMotorConstraintAtom m_ragdollMotors;
		hkpAngFrictionConstraintAtom m_angFriction;
		hkpTwistLimitConstraintAtom m_twistLimit;
		hkpConeLimitConstraintAtom m_coneLimit;
		hkpConeLimitConstraintAtom m_planesLimit;
		hkpBallSocketConstraintAtom m_ballSocket;
	};

	// The solver results' indices (Havok SDK).
	enum {
		SOLVER_RESULT_MOTOR_0 = 0,
		SOLVER_RESULT_MOTOR_1 = 1,
		SOLVER_RESULT_MOTOR_2 = 2,
		SOLVER_RESULT_FRICTION_0 = 3,
		SOLVER_RESULT_FRICTION_1 = 4,
		SOLVER_RESULT_FRICTION_2 = 5,
		SOLVER_RESULT_TWIST = 6,
		SOLVER_RESULT_CONE = 7,
		SOLVER_RESULT_PLANE = 8,
		SOLVER_RESULT_LIN_0 = 9,
		SOLVER_RESULT_LIN_1 = 10,
		SOLVER_RESULT_LIN_2 = 11,
		SOLVER_RESULT_MAX = 12,
	};

	// The runtime memory of one instance; the solver results are 16-byte aligned.
	struct alignas(16) Runtime {
		hkpSolverResults m_solverResults[12];
		uint8_t m_initialized[3];
		float m_previousTargetAngles[3];
		float m_coneAngleOffset;
	};

	// Constructs the constraint data in caller-provided storage.
	hkpRagdollConstraintData();

	// The SDK's finish constructor: while finishing, turns cone limit stabilization back on when the loaded cone
	// limit has it.
	hkpRagdollConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {
		if (aFlag.m_finishing && m_atoms.m_coneLimit.m_memOffsetToAngleOffset != 0) {
			setConeLimitStabilization(true);
		}
	}

	~hkpRagdollConstraintData() override;

	void setMaxLinearImpulse(float maxImpulse) override;
	float getMaxLinearImpulse() const override;
	void setBodyToNotify(int32_t bodyIdx) override;
	uint8_t getNotifiedBodyIndex() const override;
	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

	// setInBodySpace is declared only.
	void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, const hkVector4& pivot,
		const hkVector4& twistAxisW, const hkVector4& planeAxisW);
	void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, const hkVector4& planeAxisA,
		const hkVector4& planeAxisB, const hkVector4& twistAxisA, const hkVector4& twistAxisB);
	void getConstraintFrameA(hkMatrix3& constraintFrameA) const;
	void getConstraintFrameB(hkMatrix3& constraintFrameB) const;
	void setTwistMinAngularLimit(float rad);
	void setTwistMaxAngularLimit(float rad);
	void setPlaneMaxAngularLimit(float rad);
	void setPlaneMinAngularLimit(float rad);
	void setConeAngularLimit(float rad);
	// Sets the cone limit atom's m_memOffsetToAngleOffset (0x116 in the object) to 0x38 when enable is set, else 0.
	void setConeLimitStabilization(hkBool enable);
	void setAsymmetricConeAngle(float cone_min, float cone_max);
	void setMaxFrictionTorque(float tmag);
	void setAngularLimitsTauFactor(float mag);
	float getAngularLimitsTauFactor() const;
	float getTwistMinAngularLimit() const;
	float getTwistMaxAngularLimit() const;
	float getPlaneMaxAngularLimit() const;
	float getPlaneMinAngularLimit() const;
	float getConeAngularLimit() const;
	hkBool getConeLimitStabilization() const;
	float getMaxFrictionTorque() const;
	void setTarget(const hkMatrix3& target_cbRca);
	void setTargetRelativeOrientationOfBodies(const hkRotation& target_bRa);
	void getTarget(hkMatrix3& target_cbRca);
	hkBool getMotorsActive() const;
	void setMotorsActive(hkpConstraintInstance* instance, hkBool toBeEnabled);
	hkpConstraintMotor* getTwistMotor() const;
	hkpConstraintMotor* getConeMotor() const;
	hkpConstraintMotor* getPlaneMotor() const;
	// Replaces the motor at index, taking a reference on newMotor before releasing the old one.
	void setMotor(MotorIndex index, hkpConstraintMotor* newMotor);
	void setTwistMotor(hkpConstraintMotor* motor);
	void setConeMotor(hkpConstraintMotor* motor);
	void setPlaneMotor(hkpConstraintMotor* motor);
	void setTargetRelativeOrientationOfBodies(hkMatrix3* a1);

	static Runtime* getRuntime(void* runtime) { return reinterpret_cast<Runtime*>(runtime); }
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	// The atoms start on a 16-byte boundary: the first atom holds hkTransforms, whose hkVector4s are 16-byte aligned,
	// so the compiler pads before m_atoms by itself.
	Atoms m_atoms;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpRagdollConstraintDataAtomsTypeInfo;
