#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Callback/hkpCallbackConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"

#include <cmath>
#include <xmmintrin.h>

// Computes the motor's target, force bounds, tau and damping for one degree of freedom, by motor type; a callback
// motor's function writes the output itself. Unlike the Havok SDK, the position motor's minimum force is its negated
// maximum force, and the velocity motor's damping is its tau, as the game does.
void hkCalcMotorData(const hkpConstraintMotor* someMotor, const hkpConstraintMotorInput* input,
	hkpConstraintMotorOutput* output) {
	float fTargetVelocity;
	float fTargetPosition;
	float fMaxForce;
	float fMinForce;
	float fTau;
	float fDamping;
	switch (someMotor->m_type.m_storage) {
	case hkpConstraintMotor::TYPE_POSITION: {
		const hkpPositionConstraintMotor* pMotor = static_cast<const hkpPositionConstraintMotor*>(someMotor);
		const hkpConstraintQueryStepInfo* pStepInfo = input->m_stepInfo;
		float fDeltaTarget = input->m_deltaTarget;
		float fPositionError = input->m_positionError;
		// This product is kept at double precision for the sum below and rounded to a float only for the difference, as
		// the game does.
		double dProportional = pMotor->m_proportionalRecoveryVelocity * fPositionError * pStepInfo->m_frameDeltaTime;
		float fRemaining = fPositionError - static_cast<float>(dProportional);
		float fConstant = pMotor->m_constantRecoveryVelocity * pStepInfo->m_frameDeltaTime;
		float fAbsRemaining = std::fabs(fRemaining);
		float fRecovered;
		if (fConstant < fAbsRemaining) {
			float fStep = fRemaining > 0.0f ? fConstant : -fConstant;
			fRecovered = dProportional + fStep;
		} else {
			fRecovered = fPositionError;
		}
		float fAbsError = std::fabs(fPositionError);
		float fLow = -fAbsError - input->m_lastResults.m_internalSolverData;
		float fHigh = fAbsError - input->m_lastResults.m_internalSolverData;
		if (fLow > fRecovered) {
			fRecovered = fLow;
		} else if (fHigh < fRecovered) {
			fRecovered = fHigh;
		}
		fTargetVelocity = (fRecovered + fDeltaTarget) * input->m_stepInfo->m_frameInvDeltaTime;
		fTargetPosition = input->m_lastResults.m_internalSolverData;
		fMaxForce = pMotor->m_maxForce;
		fMinForce = -fMaxForce;
		fTau = pMotor->m_tau;
		fDamping = pMotor->m_damping;
		break;
	}
	case hkpConstraintMotor::TYPE_VELOCITY: {
		const hkpVelocityConstraintMotor* pMotor = static_cast<const hkpVelocityConstraintMotor*>(someMotor);
		if (pMotor->m_useVelocityTargetFromConstraintTargets.m_bool) {
			fTargetVelocity = (input->m_positionError + input->m_deltaTarget) * input->m_stepInfo->m_frameInvDeltaTime;
		} else {
			fTargetVelocity = pMotor->m_velocityTarget;
		}
		fTargetPosition = input->m_lastResults.m_internalSolverData;
		fTau = pMotor->m_tau;
		fDamping = pMotor->m_tau;
		fMaxForce = pMotor->m_maxForce;
		fMinForce = pMotor->m_minForce;
		break;
	}
	case hkpConstraintMotor::TYPE_SPRING_DAMPER: {
		const hkpSpringDamperConstraintMotor* pMotor = static_cast<const hkpSpringDamperConstraintMotor*>(someMotor);
		fTargetVelocity = 0.0f;
		fTargetPosition = input->m_positionError + input->m_deltaTarget;
		float fInvVirtualMass = 1.0f / input->m_virtualMass;
		float fDeltaTime = input->m_stepInfo->m_substepDeltaTime;
		float fSpringTau = pMotor->m_springConstant * fDeltaTime * fDeltaTime * fInvVirtualMass;
		if (fSpringTau < 0.0f) {
			fTau = 0.0f;
		} else if (fSpringTau > 1.0f) {
			fTau = 1.0f;
		} else {
			fTau = fSpringTau;
		}
		float fSpringDamping = fInvVirtualMass * (fDeltaTime * pMotor->m_springDamping);
		if (fSpringDamping < 0.0f) {
			fDamping = 0.0f;
		} else if (fSpringDamping > 1.0f) {
			fDamping = 1.0f;
		} else {
			fDamping = fSpringDamping;
		}
		fMaxForce = pMotor->m_maxForce;
		fMinForce = pMotor->m_minForce;
		break;
	}
	case hkpConstraintMotor::TYPE_CALLBACK: {
		const hkpCallbackConstraintMotor* pMotor = static_cast<const hkpCallbackConstraintMotor*>(someMotor);
		pMotor->m_callbackFunc(*pMotor, input, output);
		return;
	}
	default:
		return;
	}
	output->m_targetVelocity = fTargetVelocity;
	output->m_targetPosition = fTargetPosition;
	output->m_maxForce = fMaxForce;
	output->m_minForce = fMinForce;
	output->m_tau = fTau;
	output->m_damping = fDamping;
}

void hk1dLinearVelocityMotorBeginJacobian(const hkVector4& directionOfConstraint, const hkVector4& pivot,
	const hkpConstraintQueryIn& in, hkpJacobianSchema* jac, hkp1dBilateralConstraintStatus* statusOut) {
	hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
	hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
	(*jac).m_quad = directionOfConstraint.m_quad;
	__m128 v7 = _mm_sub_ps(pivot.m_quad, m_bodyA->m_scratch0.m_quad);
	__m128 v8 = _mm_sub_ps(pivot.m_quad, m_bodyB->m_scratch0.m_quad);
	__m128 m_quad = directionOfConstraint.m_quad;
	__m128 v10 = _mm_shuffle_ps(m_quad, m_quad, 201);
	__m128 v11 = _mm_shuffle_ps(m_quad, m_quad, 210);
	__m128 v12 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v11), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 210), v10));
	__m128 v13 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v8, v8, 210), v10), _mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), v11));
	jac[1].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 85), m_bodyA->m_scratch123.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), m_bodyA->m_scratch123.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(v12, v12, 170), m_bodyA->m_scratch123.m_col2.m_quad));
	__m128 v14 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v13, v13, 85), m_bodyB->m_scratch123.m_col1.m_quad),
		_mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), m_bodyB->m_scratch123.m_col0.m_quad));
	__m128 v15 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 170), m_bodyB->m_scratch123.m_col2.m_quad);
	__m128 v16 = jac[1].m_quad;
	__m128 v17 = _mm_add_ps(v14, v15);
	jac[2].m_quad = v17;
	__m128 v18 = m_bodyB->m_invMasses.m_quad;
	__m128 v19 = m_bodyA->m_invMasses.m_quad;
	__m128 v20 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(v16, v16), v19), _mm_mul_ps(_mm_mul_ps(v17, v17), v18));
	__m128 v21 = _mm_shuffle_ps(v20, _mm_unpackhi_ps(v20, _mm_add_ps(v19, v18)), 196);
	__m128 v22 = _mm_add_ps(_mm_shuffle_ps(v21, v21, 78), v21);
    float v23 = (_mm_shuffle_ps(v22, v22, 177)[0] + v22[0]) + 1.1920928955078125e-07;
    jac[2].m_quad[3] = v23;
	float v24 = 1.0 / v23;
	jac[1].m_quad[3] = v24;
	statusOut->m_virtualMass = v24;
}

void hk1dLinearVelocityMotorCommitJacobian(hkp1dConstraintMotorInfo* info, const hkpConstraintQueryIn& in,
	hkpJacobianSchema* jac, hkpConstraintQueryOut& out) {
	jac->m_quad[3] = in.m_substepInvDeltaTime * info->m_targetPosition;
	hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas + 5;
	m_jacobianSchemas[-5].m_quad[0] = out.m_jacobianSchemas->m_quad[0] + 1.1920928955078125e-07;
	m_jacobianSchemas[-5].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR;
	double m_maxForce = info->m_maxForce;
	out.m_jacobianSchemas = m_jacobianSchemas;
	m_jacobianSchemas[-2].m_quad[0] = m_maxForce * in.m_microStepDeltaTime;
	m_jacobianSchemas[-2].m_quad[1] = info->m_minForce * in.m_microStepDeltaTime;
	m_jacobianSchemas[-2].m_quad[2] = info->m_targetVelocity;
	m_jacobianSchemas[-2].m_quad[3] = info->m_tau;
	m_jacobianSchemas[-1].m_quad[0] = info->m_damping;
}
