#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"

#include <xmmintrin.h>

class hkp1Lin2AngJacobian;
class hkpConstraintMotor;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpJacobianSchema;

// The inputs of the one-dimensional Jacobian builders that hkSolverBuildJacobianFromAtomsNotContact calls, built on its
// stack.

// A linear constraint along a direction between two
// pivots.
struct alignas(16) hkp1dLinearBilateralConstraintInfo {
	// Writes the linear bilateral Jacobian row into r_eax: the direction, both bodies' angular parts, the diagonal (+
	// epsilon) and the virtual mass over it, and the rhs factor times a1. Returns r_eax.
	static hkp1Lin2AngJacobian* buildJacobian(float a1, hkp1Lin2AngJacobian* r_eax,
		const hkp1dLinearBilateralConstraintInfo* r_edi, const hkpConstraintQueryIn* r_esi);

	hkVector4 m_pivotA;
	hkVector4 m_pivotB;
	hkVector4 m_constrainedDofW;
};

// The linear constraint with its own tau and damping.
struct alignas(16) hkp1dLinearBilateralUserTauConstraintInfo : public hkp1dLinearBilateralConstraintInfo {
	float m_tau;
	float m_damping;
};

// The linear limit between m_min and m_max.
struct alignas(16) hkp1dLinearLimitInfo : public hkp1dLinearBilateralConstraintInfo {
	float m_min;
	float m_max;
};

// Linear friction at a pivot along a direction.
struct alignas(16) hkp1dLinearFrictionInfo {
	hkVector4 m_pivot;
	hkVector4 m_constrainedDofW;
	float m_maxFrictionForce;
	hkpSolverResults* m_lastSolverResults;
};

// An angular constraint: the axes in world space.
struct alignas(16) hkp1dAngularBilateralConstraintInfo {
	hkVector4 m_zeroErrorAxisAinW;
	hkVector4 m_perpZeroErrorAxisBinW;
	hkVector4 m_constrainedDofW;
};

// Angular friction about m_numFriction consecutive axes.
struct hkp1dAngularFrictionInfo {
	const hkVector4* m_constrainedDofW;
	hkpSolverResults* m_lastSolverResults;
	float m_maxFrictionTorque;
	hkPadSpu<int32_t> m_numFriction;
};

// The 0x20-byte schema (type 0x19) that sets the two bodies' inverse masses the solver sees.
void hkSetInvMassBuildJacobian(const hkVector4& invMassA, const hkVector4& invMassB, hkpConstraintQueryOut& out);
// The 0x20-byte schema (type 0x1A) that adds arVelocity to a body's linear velocity; aiIsBodyB selects the body.
void hkAddVelocityBuildJacobian(const hkVector4& deltaVel, int32_t bodyIndex, hkpConstraintQueryOut& out);
// The schema that moves the two bodies' centres of mass by the two skew matrices.
void hkSetCenterOfMassBuildJacobian(const hkMatrix3& angToLinVelA, const hkMatrix3& angToLinVelB,
	hkpConstraintQueryOut& out);
// The stiff spring, a linear constraint whose right-hand side is afLength.
void hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs(const hkp1dLinearBilateralConstraintInfo& info,
	const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out, float customRhs);
// The linear constraint.
void hk1dLinearBilateralConstraintBuildJacobian(const hkp1dLinearBilateralConstraintInfo& info,
	const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
// The soft linear constraint.
void hk1dLinearBilateralConstraintUserTauBuildJacobian(const hkp1dLinearBilateralUserTauConstraintInfo& info,
	const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
// The SDK's: the linear constraint along arInfo.m_constrainedDofW at arInfo.m_pivotA (for both bodies), written
// straight into apJac with no schema header; its right-hand side is afCustomRhs times the rhs factor.
void hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs_noSchema(const hkp1dLinearBilateralConstraintInfo& info,
	const hkpConstraintQueryIn& in, hkp1Lin2AngJacobian* jac, float customRhs);
// The SDK's: hkBallSocketConstraintBuildJacobian_noSchema with the error corrected per step capped at
// afMaxAllowedErrorDistance times the rhs factor.
void hkStabilizedBallSocketConstraintBuildJacobian_noSchema(const hkVector4& posA, const hkVector4& posB,
	float maxAllowedErrorDistance, const hkpConstraintQueryIn& in, hkp1Lin2AngJacobian* jac);
// The linear limit.
void hk1dLinearLimitBuildJacobian(const hkp1dLinearLimitInfo& info, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out);
// The linear friction.
void hk1dLinearFrictionBuildJacobian(const hkp1dLinearFrictionInfo& info, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out);
// The angular constraint about one axis.
void hk1dAngularBilateralConstraintBuildJacobian(const hkp1dAngularBilateralConstraintInfo& info,
	const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
// The angular friction.
void hk1dAngularFrictionBuildJacobian(const hkp1dAngularFrictionInfo& info, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out);
// Asks apMotor for its output from apInput.
void hkCalcMotorData(const hkpConstraintMotor* someMotor, const hkpConstraintMotorInput* input,
	hkpConstraintMotorOutput* output);
// Writes the motor's Jacobian at apJac and the virtual mass into apStatusOut.
void hk1dLinearVelocityMotorBeginJacobian(const hkVector4& directionOfConstraint, const hkVector4& pivot,
	const hkpConstraintQueryIn& in, hkpJacobianSchema* jac, hkp1dBilateralConstraintStatus* statusOut);
// Finishes the motor schema begun at apJac.
void hk1dLinearVelocityMotorCommitJacobian(hkp1dConstraintMotorInfo* info, const hkpConstraintQueryIn& in,
	hkpJacobianSchema* jac, hkpConstraintQueryOut& out);
void sub_DCEE70();

// An angular limit about m_constrainedDofW: the bounds, the angle computed so far and tau (hkpconstraintsolver.pdb).
// hk1dAngularLimitBuildJacobian reads the axis at 0 and the bounds, angle and tau from 0x10.
class hkp1dAngularLimitInfo {
public:
	hkVector4 m_constrainedDofW;
	hkPadSpu<float> m_min;
	hkPadSpu<float> m_max;
	hkPadSpu<float> m_computedAngle;
	hkPadSpu<float> m_tau;
};

// The pulley: both bodies' pivots, the pulley pivots, the rope length and the leverage on body B
// (hkpconstraintsolver.pdb).
struct hkpPulleyConstraintInfo {
	hkVector4 m_positionA;
	hkVector4 m_positionB;
	hkVector4 m_pulleyPivotA;
	hkVector4 m_pulleyPivotB;
	hkPadSpu<float> m_ropeLength;
	hkPadSpu<float> m_leverageOnBodyB;
};

// The builders, all taking (info, in, out) unless noted (bodies in hkp1dBilateralConstraintBuildJacobian.cpp and
// Motor/hkpMotorBuildJacobian.cpp).
