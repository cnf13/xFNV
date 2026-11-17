#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dBilateralSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPairContactSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSingleContactSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"

#include <stdint.h>
#include <xmmintrin.h>

// Walks one constraint's schemas after its header. Each bilateral or contact row's velocity is the dot product of its
// Jacobian with the two bodies' velocities (summed over x, y, z in the exe's lane order); contact rows are clamped to
// at most 0. A set-center-of-mass schema moves the bodies onto copies of their accumulators with the shifted linear
// velocities. A schema type the exe does not handle (4, 11, or outside 0..27) is read again for ever, as in the exe.
// The schema stream is raw memory the exe walks by each schema's type byte and size: each reinterpret_cast views the
// bytes at the cursor as the schema their type names, as hkpSolverExport.cpp does.
int32_t hkSolveGetToiViolatingConstraintVelocity(hkpSolverInfo& i, const hkpJacobianSchema* schemas,
	const hkpVelocityAccumulator* accums, int32_t maxNumVelocities, float* velocitiesOut) {
	hkpVelocityAccumulator tempAccA;
	hkpVelocityAccumulator tempAccB;
	int32_t numVelocities = 0;
	const hkpVelocityAccumulator* accA = &accums[schemas->getHeader().m_bodyAIndex];
	const hkpVelocityAccumulator* accB = &accums[schemas->getHeader().m_bodyBIndex];
	const hkpJacobianSchema* schema = schemas + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	while (true) {
		const int8_t type = static_cast<int8_t>(schema->m_bytes.m_type);
		switch (type) {
		case hkpJacobianSchema::SCHEMA_TYPE_END:
		case hkpJacobianSchema::SCHEMA_TYPE_HEADER:
		case hkpJacobianSchema::SCHEMA_TYPE_2D_FRICTION:
		case hkpJacobianSchema::SCHEMA_TYPE_3D_FRICTION:
		case hkpJacobianSchema::SCHEMA_TYPE_2D_ROLLING_FRICTION:
			return numVelocities;
		case hkpJacobianSchema::SCHEMA_TYPE_GOTO: {
			const hkpJacobianGotoSchema* gotoSchema = reinterpret_cast<const hkpJacobianGotoSchema*>(schema);
			schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(schema) +
				gotoSchema->m_offset);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR:
		case hkpJacobianSchema::SCHEMA_TYPE_SET_MASS:
		case hkpJacobianSchema::SCHEMA_TYPE_ADD_VELOCITY:
			// Skips this schema and the ones of the same type after it.
			do {
				schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(schema) +
					hkpJacobianSchema::s_schemaSize[static_cast<int8_t>(schema->m_bytes.m_type)]);
			} while (static_cast<int8_t>(schema->m_bytes.m_type) == type);
			continue;
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL:
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT: {
			if (numVelocities + 1 > maxNumVelocities) {
				return numVelocities;
			}
			const hkpJacobian1dBilateralSchema* bilateral =
				reinterpret_cast<const hkpJacobian1dBilateralSchema*>(schema);
			const hkp1Lin2AngJacobian& jac = bilateral->m_jac;
			const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_angular[1].m_quad, accB->m_angularVel.m_quad),
											  _mm_mul_ps(jac.m_angular[0].m_quad, accA->m_angularVel.m_quad)),
				_mm_mul_ps(_mm_sub_ps(accA->m_linearVel.m_quad, accB->m_linearVel.m_quad), jac.m_linear0.m_quad));
			const __m128 sumXY = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum);
			*velocitiesOut++ = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA), sumXY));
			++numVelocities;
			schema = reinterpret_cast<const hkpJacobianSchema*>(bilateral + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU: {
			if (numVelocities + 1 > maxNumVelocities) {
				return numVelocities;
			}
			const hkpJacobian1dBilateralUserTauSchema* userTau =
				reinterpret_cast<const hkpJacobian1dBilateralUserTauSchema*>(schema);
			const hkp1Lin2AngJacobian& jac = userTau->m_jac;
			const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_angular[1].m_quad, accB->m_angularVel.m_quad),
											  _mm_mul_ps(jac.m_angular[0].m_quad, accA->m_angularVel.m_quad)),
				_mm_mul_ps(_mm_sub_ps(accA->m_linearVel.m_quad, accB->m_linearVel.m_quad), jac.m_linear0.m_quad));
			const __m128 sumXY = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum);
			*velocitiesOut++ = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA), sumXY));
			++numVelocities;
			schema = reinterpret_cast<const hkpJacobianSchema*>(userTau + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT_WITH_ACCESSOR_CHECK:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT_WITH_ACCESSOR_CHECK: {
			if (numVelocities + 1 > maxNumVelocities) {
				return numVelocities;
			}
			const hkpJacobianSingleContactSchema* contact =
				reinterpret_cast<const hkpJacobianSingleContactSchema*>(schema);
			const hkp1Lin2AngJacobian& jac = contact->m_jac;
			const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_angular[1].m_quad, accB->m_angularVel.m_quad),
											  _mm_mul_ps(jac.m_angular[0].m_quad, accA->m_angularVel.m_quad)),
				_mm_mul_ps(_mm_sub_ps(accA->m_linearVel.m_quad, accB->m_linearVel.m_quad), jac.m_linear0.m_quad));
			const __m128 sumXY = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum);
			const float velocity = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA), sumXY));
			++numVelocities;
			*velocitiesOut++ = velocity > 0.0f ? 0.0f : velocity;
			schema = reinterpret_cast<const hkpJacobianSchema*>(contact + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT_WITH_ACCESSOR_CHECK: {
			if (numVelocities + 2 > maxNumVelocities) {
				return numVelocities;
			}
			const hkpJacobianPairContactSchema* pair = reinterpret_cast<const hkpJacobianPairContactSchema*>(schema);
			const hkp1Lin2AngJacobian& jac0 = pair->m_jac.m_jac0;
			const __m128 sum0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac0.m_angular[0].m_quad, accA->m_angularVel.m_quad),
											   _mm_mul_ps(jac0.m_angular[1].m_quad, accB->m_angularVel.m_quad)),
				_mm_mul_ps(_mm_sub_ps(accA->m_linearVel.m_quad, accB->m_linearVel.m_quad), jac0.m_linear0.m_quad));
			const __m128 sum0XY = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0x55), sum0);
			const float velocity0 = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xAA), sum0XY));
			numVelocities += 2;
			*velocitiesOut = velocity0 > 0.0f ? 0.0f : velocity0;
			const hkp1Lin2AngJacobian& jac1 = pair->m_jac.m_jac1;
			const __m128 sum1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac1.m_angular[0].m_quad, accA->m_angularVel.m_quad),
											   _mm_mul_ps(jac1.m_angular[1].m_quad, accB->m_angularVel.m_quad)),
				_mm_mul_ps(_mm_sub_ps(accA->m_linearVel.m_quad, accB->m_linearVel.m_quad), jac1.m_linear0.m_quad));
			const __m128 sum1XY = _mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0x55), sum1);
			const float velocity1 = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0xAA), sum1XY));
			velocitiesOut[1] = velocity1 > 0.0f ? 0.0f : velocity1;
			velocitiesOut += 2;
			schema = reinterpret_cast<const hkpJacobianSchema*>(pair + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_CENTER_OF_MASS: {
			const hkpJacobianSetCenterOfMassSchema* centerOfMass =
				reinterpret_cast<const hkpJacobianSetCenterOfMassSchema*>(schema);
			tempAccA = *accA;
			tempAccB = *accB;
			accA = &tempAccA;
			accB = &tempAccB;
			hkVector4 linearVelA;
			hkVector4 linearVelB;
			hkVector4 scratchA;
			hkVector4 scratchB;
			linearVelA.setMul3(centerOfMass->m_angToLinVelA, tempAccA.m_angularVel);
			linearVelB.setMul3(centerOfMass->m_angToLinVelB, tempAccB.m_angularVel);
			scratchA.setMul3(centerOfMass->m_angToLinVelA, tempAccA.m_scratch123.m_col0);
			scratchB.setMul3(centerOfMass->m_angToLinVelB, tempAccB.m_scratch123.m_col0);
			tempAccA.m_linearVel.m_quad = _mm_add_ps(tempAccA.m_linearVel.m_quad, linearVelA.m_quad);
			tempAccA.m_scratch0.m_quad = _mm_add_ps(tempAccA.m_scratch0.m_quad, scratchA.m_quad);
			tempAccB.m_linearVel.m_quad = _mm_add_ps(tempAccB.m_linearVel.m_quad, linearVelB.m_quad);
			tempAccB.m_scratch0.m_quad = _mm_add_ps(tempAccB.m_scratch0.m_quad, scratchB.m_quad);
			schema = reinterpret_cast<const hkpJacobianSchema*>(centerOfMass + 1);
			continue;
		}
		default:
			continue;
		}
	}
}
