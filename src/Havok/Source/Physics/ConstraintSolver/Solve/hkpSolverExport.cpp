#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dAngularFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dAngularLimitsSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dAngularMotorSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dAngularSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dBilateralSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dLinearLimitsSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dLinearMotorSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian1dPulleySchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian2dFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian2dRollingFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian3dFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianBallSocketChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPairContactSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPoweredChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSingleContactSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianStiffSpringChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverElemTemp.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"

#include <cmath>
#include <xmmintrin.h>

void hkExportImpulsesAndRhs(const hkpSolverInfo& i, const hkpSolverElemTemp* temp, const hkpJacobianSchema* schemas,
	const hkpVelocityAccumulator* accums) {
	hkSolverExport::exportImpulsesAndRhs(i, temp, schemas, accums);
}

void sub_DF0690() {
	;
}

// Writes back what the solver computed: for every Jacobian row the impulse applied (from the solver's temps) and the
// right-hand side still to go (the row's rhs, scaled, less the bodies' relative velocity along the row times the step
// time). The relative velocity is summed with SSE (x, then y, then z), everything else on the x87.
// The constraints whose impulse limit was breached are listed over the schemas already read, from the start of the
// buffer: the header first (its count stored when the end schema is reached), the elements after it. The schema
// stream is untyped memory the exe reads by each schema's type byte; each schema is cast to its type below.
// The temps (hkpSolverElemTemp) and accumulators are reached by the schema sizes and the chains' byte offsets.
void hkSolverExport::exportImpulsesAndRhs(const hkpSolverInfo& info, const hkpSolverElemTemp* temp,
	const hkpJacobianSchema* schemas, const hkpVelocityAccumulator* accums) {
	// The bodies' accumulators as an ADD_VELOCITY or SET_CENTER_OF_MASS schema changes them for the schemas after it.
	hkpVelocityAccumulator modifiedAccA;
	hkpVelocityAccumulator modifiedAccB;
	const float dampDivTauDt = info.m_dampDivTau * info.m_deltaTime;
	const float dampDivFrictionTauDt = info.m_dampDivFrictionTau * info.m_deltaTime;
	const float stepsDt = static_cast<double>(info.m_numSteps) * info.m_deltaTime;
	const float deltaTime = info.m_deltaTime;
	hkpSolverResults* results = nullptr;
	int32_t striding = 0;
	hkpConstraintInstance* constraintInstance = nullptr;
	const hkpVelocityAccumulator* accA = nullptr;
	const hkpVelocityAccumulator* accB = nullptr;
	int32_t limitBreached = 0;
	int32_t numBreached = 0;
	hkpImpulseLimitBreachedHeader* breachedHeader =
		reinterpret_cast<hkpImpulseLimitBreachedHeader*>(const_cast<hkpJacobianSchema*>(schemas));
	// EXE-INLINED none: hkpImpulseLimitBreachedHeader::getElem
	hkpImpulseLimitBreachedElem* breachedElem = reinterpret_cast<hkpImpulseLimitBreachedElem*>(breachedHeader + 1);
	const hkpJacobianSchema* schema = schemas;
	while (true) {
		// The Jacobian and the factors of the rows whose cases share their export code below.
		const hkp1Lin2AngJacobian* bilateralJac;
		const hkp1Lin2AngJacobian* scaledJac;
		const hkp2AngJacobian* angularJac;
		const hkp2AngJacobian* scaledAngularJac;
		float impulseFactor;
		int8_t scaledType;
		const int8_t type = static_cast<int8_t>(schema->m_bytes.m_type);
		switch (type) {
		case hkpJacobianSchema::SCHEMA_TYPE_END: {
			breachedHeader->m_numBreached = numBreached;
			return;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_HEADER: {
			const hkpJacobianHeaderSchema& header = schema->getHeader();
			striding = header.m_solverResultStriding;
			accA = &accums[header.m_bodyAIndex];
			results = header.m_solverResultInMainMemory;
			constraintInstance = header.m_constraintInstance;
			accB = &accums[header.m_bodyBIndex];
			schema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			if (!results) {
				// No results wanted: skip the constraint's simple schemas and their temps.
				int8_t nextType = static_cast<int8_t>(schema->m_bytes.m_type);
				while (nextType > hkpJacobianSchema::SCHEMA_TYPE_SIMPLE_BEGIN &&
					nextType < hkpJacobianSchema::SCHEMA_TYPE_SIMPLE_END) {
					temp += hkpJacobianSchema::s_schemaNumSolverElemTemps[nextType];
					schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(schema) +
						hkpJacobianSchema::s_schemaSize[nextType]);
					nextType = static_cast<int8_t>(schema->m_bytes.m_type);
				}
			}
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_GOTO: {
			const hkpJacobianGotoSchema* gotoSchema = reinterpret_cast<const hkpJacobianGotoSchema*>(schema);
			schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(schema) +
				gotoSchema->m_offset);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER: {
			// The exe multiplies the striding's low word, sign-extended.
			results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) +
				static_cast<int16_t>(striding) * schema->m_shiftSolverResults.m_numSolverResultsToSkip);
			schema++;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL: {
			const hkpJacobian1dBilateralSchema* bilateral =
				reinterpret_cast<const hkpJacobian1dBilateralSchema*>(schema);
			bilateralJac = &bilateral->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(bilateral + 1);
			goto exportBilateral;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT_WITH_ACCESSOR_CHECK: {
			const hkpJacobianSingleContactSchema* contact =
				reinterpret_cast<const hkpJacobianSingleContactSchema*>(schema);
			bilateralJac = &contact->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(contact + 1);
			goto exportBilateral;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT: {
			// The bilateral schema with the impulse limit in the second angular row's w (the PDB has no class of its
			// own for it; the exe reads it at 0x2C and moves on 0x30 bytes).
			const hkpJacobian1dBilateralSchema* bilateral =
				reinterpret_cast<const hkpJacobian1dBilateralSchema*>(schema);
			bilateralJac = &bilateral->m_jac;
			const float impulse = std::fabs(temp->m_impulseApplied);
			schema = reinterpret_cast<const hkpJacobianSchema*>(bilateral + 1);
			if (bilateralJac->m_angular[1].m_quad[3] * 0.9900000095367432 < impulse) {
				limitBreached = 1;
			}
			// A run of these schemas is one constraint: it is listed once, at the run's last schema.
			if (schema->m_bytes.m_type != hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT &&
				limitBreached) {
				numBreached++;
				breachedElem->m_type = 0;
				breachedElem->m_isContact = 0;
				breachedElem->m_constraintInstance = constraintInstance;
				breachedElem->m_solverResult = nullptr;
				breachedElem++;
				limitBreached = 0;
			}
			goto exportBilateral;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU: {
			const hkpJacobian1dBilateralUserTauSchema* userTau =
				reinterpret_cast<const hkpJacobian1dBilateralUserTauSchema*>(schema);
			bilateralJac = &userTau->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(userTau + 1);
			goto exportBilateral;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS: {
			const hkpJacobian1dLinearLimitsSchema* limits =
				reinterpret_cast<const hkpJacobian1dLinearLimitsSchema*>(schema);
			bilateralJac = &limits->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(limits + 1);
			goto exportBilateral;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT_WITH_ACCESSOR_CHECK: {
			// The contact schema with the impulse limit in the second angular row's w (the PDB has no class of its
			// own for it; the exe reads it at 0x2C and moves on 0x30 bytes).
			const hkpJacobianSingleContactSchema* contact =
				reinterpret_cast<const hkpJacobianSingleContactSchema*>(schema);
			bilateralJac = &contact->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(contact + 1);
			const float limit = bilateralJac->m_angular[1].m_quad[3] * 0.9900000095367432;
			if (limit < temp->m_impulseApplied) {
				numBreached++;
				breachedElem->m_type = 0;
				breachedElem->m_isContact = 1;
				breachedElem->m_constraintInstance = constraintInstance;
				breachedElem->m_solverResult = results;
				breachedElem++;
			}
		}
		exportBilateral:
			{
				const float rhs = bilateralJac->m_linear0.m_quad[3] * dampDivTauDt;
				const __m128 angVel =
					_mm_add_ps(_mm_mul_ps(accB->m_scratch123.m_col0.m_quad, bilateralJac->m_angular[1].m_quad),
						_mm_mul_ps(accA->m_scratch123.m_col0.m_quad, bilateralJac->m_angular[0].m_quad));
				const __m128 linDiff = _mm_sub_ps(accA->m_scratch0.m_quad, accB->m_scratch0.m_quad);
				results->m_impulseApplied = temp->m_impulseApplied;
				const __m128 vel = _mm_add_ps(angVel, _mm_mul_ps(linDiff, bilateralJac->m_linear0.m_quad));
				const float velSum = static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
					static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0]));
				const float scaledVel = velSum * stepsDt;
				results->m_internalSolverData = rhs - scaledVel;
				results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
				temp++;
				continue;
			}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION: {
			scaledType = type;
			const hkpJacobian1dFrictionSchema* friction = reinterpret_cast<const hkpJacobian1dFrictionSchema*>(schema);
			scaledJac = &friction->m_jac;
			impulseFactor = temp[1].m_usedImpulseFactor;
			schema = reinterpret_cast<const hkpJacobianSchema*>(friction + 1);
			goto exportScaled;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR: {
			const hkpJacobian1dLinearMotorSchema* motor =
				reinterpret_cast<const hkpJacobian1dLinearMotorSchema*>(schema);
			const hkpJacobianSchema* motorSchema = schema;
			scaledJac = &motor->m_jac;
			impulseFactor = temp[1].m_usedImpulseFactor;
			schema = reinterpret_cast<const hkpJacobianSchema*>(motor + 1);
			// The motor's rhs takes the solver's change to it.
			const_cast<hkpJacobian1dLinearMotorSchema*>(motor)->m_jac.m_linear0.m_quad[3] =
				temp[2].m_deltaRhs + motor->m_jac.m_linear0.m_quad[3];
			scaledType = static_cast<int8_t>(motorSchema->m_bytes.m_type);
		}
		exportScaled:
			{
				const hkpSolverElemTemp* nextTemp = &temp[hkpJacobianSchema::s_schemaNumSolverElemTemps[scaledType]];
				const float rhs = scaledJac->m_linear0.m_quad[3] * dampDivTauDt;
				const __m128 angVel =
					_mm_add_ps(_mm_mul_ps(accB->m_scratch123.m_col0.m_quad, scaledJac->m_angular[1].m_quad),
						_mm_mul_ps(accA->m_scratch123.m_col0.m_quad, scaledJac->m_angular[0].m_quad));
				const __m128 linDiff = _mm_sub_ps(accA->m_scratch0.m_quad, accB->m_scratch0.m_quad);
				results->m_impulseApplied = temp->m_impulseApplied;
				const __m128 vel = _mm_add_ps(angVel, _mm_mul_ps(linDiff, scaledJac->m_linear0.m_quad));
				temp = nextTemp;
				const float velSum = static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
					static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0]));
				const float scaledVel = velSum * stepsDt;
				const float unscaledRhs = rhs - scaledVel;
				results->m_internalSolverData = unscaledRhs * impulseFactor;
				results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
				continue;
			}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY: {
			const hkpJacobian1dPulleySchema* pulley = reinterpret_cast<const hkpJacobian1dPulleySchema*>(schema);
			const hkp2Lin2AngJacobian& jac = pulley->m_jac;
			const __m128 angVel = _mm_add_ps(_mm_mul_ps(accB->m_scratch123.m_col0.m_quad, jac.m_angular[1].m_quad),
				_mm_mul_ps(accA->m_scratch123.m_col0.m_quad, jac.m_angular[0].m_quad));
			const __m128 linVelB = _mm_mul_ps(accB->m_scratch0.m_quad, jac.m_linear[1].m_quad);
			results->m_impulseApplied = temp->m_impulseApplied;
			const __m128 vel =
				_mm_add_ps(angVel, _mm_sub_ps(_mm_mul_ps(accA->m_scratch0.m_quad, jac.m_linear[0].m_quad), linVelB));
			const float rhs = jac.m_linear[0].m_quad[3] * dampDivTauDt;
			schema = reinterpret_cast<const hkpJacobianSchema*>(pulley + 1);
			const float velSum = static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0]));
			const float scaledVel = velSum * stepsDt;
			results->m_internalSolverData = rhs - scaledVel;
			results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
			temp++;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR: {
			const hkpJacobian1dAngularSchema* angular = reinterpret_cast<const hkpJacobian1dAngularSchema*>(schema);
			angularJac = &angular->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(angular + 1);
			goto exportAngular;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS: {
			const hkpJacobian1dAngularLimitsSchema* limits =
				reinterpret_cast<const hkpJacobian1dAngularLimitsSchema*>(schema);
			angularJac = &limits->m_jac;
			schema = reinterpret_cast<const hkpJacobianSchema*>(limits + 1);
		}
		exportAngular:
			{
				const float rhs = angularJac->m_angular[1].m_quad[3];
				const __m128 angVelB = _mm_mul_ps(accB->m_scratch123.m_col0.m_quad, angularJac->m_angular[1].m_quad);
				results->m_impulseApplied = temp->m_impulseApplied;
				const __m128 vel =
					_mm_add_ps(angVelB, _mm_mul_ps(accA->m_scratch123.m_col0.m_quad, angularJac->m_angular[0].m_quad));
				results->m_internalSolverData = rhs * deltaTime -
					static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
						static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0])) *
						stepsDt;
				results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
				temp++;
				continue;
			}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION: {
			scaledType = type;
			const hkpJacobian1dAngularFrictionSchema* friction =
				reinterpret_cast<const hkpJacobian1dAngularFrictionSchema*>(schema);
			scaledAngularJac = &friction->m_jac;
			impulseFactor = temp[1].m_usedImpulseFactor;
			schema = reinterpret_cast<const hkpJacobianSchema*>(friction + 1);
			goto exportScaledAngular;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR: {
			const hkpJacobian1dAngularMotorSchema* motor =
				reinterpret_cast<const hkpJacobian1dAngularMotorSchema*>(schema);
			const hkpJacobianSchema* motorSchema = schema;
			scaledAngularJac = &motor->m_jac;
			impulseFactor = temp[1].m_usedImpulseFactor;
			schema = reinterpret_cast<const hkpJacobianSchema*>(motor + 1);
			// The motor's rhs takes the solver's change to it.
			const_cast<hkpJacobian1dAngularMotorSchema*>(motor)->m_jac.m_angular[1].m_quad[3] =
				temp[2].m_deltaRhs + motor->m_jac.m_angular[1].m_quad[3];
			scaledType = static_cast<int8_t>(motorSchema->m_bytes.m_type);
		}
		exportScaledAngular:
			{
				const hkpSolverElemTemp* nextTemp = &temp[hkpJacobianSchema::s_schemaNumSolverElemTemps[scaledType]];
				const float rhs = scaledAngularJac->m_angular[1].m_quad[3];
				const __m128 angVelB =
					_mm_mul_ps(accB->m_scratch123.m_col0.m_quad, scaledAngularJac->m_angular[1].m_quad);
				results->m_impulseApplied = temp->m_impulseApplied;
				temp = nextTemp;
				const __m128 vel = _mm_add_ps(angVelB,
					_mm_mul_ps(accA->m_scratch123.m_col0.m_quad, scaledAngularJac->m_angular[0].m_quad));
				const float unscaledRhs = rhs * deltaTime -
					static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
						static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0])) *
						stepsDt;
				results->m_internalSolverData = unscaledRhs * impulseFactor;
				results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
				continue;
			}
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT_WITH_ACCESSOR_CHECK: {
			const hkpJacobianPairContactSchema* pair = reinterpret_cast<const hkpJacobianPairContactSchema*>(schema);
			const hkpJacDouble2Bil& jac = pair->m_jac;
			const __m128 linDiff = _mm_sub_ps(accA->m_scratch0.m_quad, accB->m_scratch0.m_quad);
			const float rhs0 = jac.m_jac0.m_linear0.m_quad[3] * dampDivTauDt;
			const __m128 vel0 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_jac0.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad),
							   _mm_mul_ps(jac.m_jac0.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad)),
					_mm_mul_ps(jac.m_jac0.m_linear0.m_quad, linDiff));
			const float rhs1 = dampDivTauDt * jac.m_jac1.m_linear0.m_quad[3];
			const float velSum0 = static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0x55)[0] + vel0[0]));
			const __m128 vel1 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_jac1.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad),
							   _mm_mul_ps(jac.m_jac1.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad)),
					_mm_mul_ps(linDiff, jac.m_jac1.m_linear0.m_quad));
			results->m_impulseApplied = temp[0].m_impulseApplied;
			const float scaledVel0 = velSum0 * stepsDt;
			results->m_internalSolverData = rhs0 - scaledVel0;
			results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
			results->m_impulseApplied = temp[1].m_impulseApplied;
			schema = reinterpret_cast<const hkpJacobianSchema*>(pair + 1);
			const float velSum1 = static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0x55)[0] + vel1[0]));
			const float scaledVel1 = velSum1 * stepsDt;
			results->m_internalSolverData = rhs1 - scaledVel1;
			results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
			temp += 2;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_2D_FRICTION: {
			const hkpJacobian2dFrictionSchema* friction = reinterpret_cast<const hkpJacobian2dFrictionSchema*>(schema);
			const hkpJacDouble2Bil& jac = friction->m_jac;
			const float factor = temp[2].m_usedImpulseFactor;
			const __m128 linDiff = _mm_sub_ps(accA->m_scratch0.m_quad, accB->m_scratch0.m_quad);
			const float rhs0 = jac.m_jac0.m_linear0.m_quad[3] * dampDivFrictionTauDt;
			const __m128 vel0 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_jac0.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad),
							   _mm_mul_ps(jac.m_jac0.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad)),
					_mm_mul_ps(jac.m_jac0.m_linear0.m_quad, linDiff));
			const float rhs1 = jac.m_jac1.m_linear0.m_quad[3] * dampDivFrictionTauDt;
			const float impulse1 = temp[1].m_impulseApplied;
			const float velSum0 = static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0x55)[0] + vel0[0]));
			const __m128 vel1 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_jac1.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad),
							   _mm_mul_ps(jac.m_jac1.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad)),
					_mm_mul_ps(linDiff, jac.m_jac1.m_linear0.m_quad));
			hkpSolverResults* frictionResults = friction->m_solverResults;
			frictionResults[0].m_impulseApplied = temp[0].m_impulseApplied;
			const float scaledVel0 = velSum0 * stepsDt;
			const float unscaledRhs0 = rhs0 - scaledVel0;
			schema = reinterpret_cast<const hkpJacobianSchema*>(friction + 1);
			temp += 3;
			frictionResults[0].m_internalSolverData = unscaledRhs0 * factor;
			frictionResults[1].m_impulseApplied = impulse1;
			const float velSum1 = static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0x55)[0] + vel1[0]));
			const float scaledVel1 = velSum1 * stepsDt;
			const float unscaledRhs1 = rhs1 - scaledVel1;
			frictionResults[1].m_internalSolverData = factor * unscaledRhs1;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_3D_FRICTION: {
			const hkpJacobian3dFrictionSchema* friction = reinterpret_cast<const hkpJacobian3dFrictionSchema*>(schema);
			const hkpJacTriple2Bil1Ang& jac = friction->m_jac;
			const float factor = temp[3].m_usedImpulseFactor;
			hkpSolverResults* frictionResults = friction->m_solverResults;
			const float rhs1 = jac.m_jac1.m_linear0.m_quad[3] * dampDivFrictionTauDt;
			const __m128 linDiff = _mm_sub_ps(accA->m_scratch0.m_quad, accB->m_scratch0.m_quad);
			const float rhs2 = jac.m_jac2.m_angular[1].m_quad[3];
			const float rhs0 = jac.m_jac0.m_linear0.m_quad[3] * dampDivFrictionTauDt;
			const __m128 vel0 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_jac0.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad),
							   _mm_mul_ps(jac.m_jac0.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad)),
					_mm_mul_ps(jac.m_jac0.m_linear0.m_quad, linDiff));
			const float velSum0 = static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0x55)[0] + vel0[0]));
			const float scaledVel0 = velSum0 * stepsDt;
			const __m128 vel1 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac.m_jac1.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad),
							   _mm_mul_ps(jac.m_jac1.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad)),
					_mm_mul_ps(jac.m_jac1.m_linear0.m_quad, linDiff));
			const float unscaledRhs0 = rhs0 - scaledVel0;
			const float velSum1 = static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0xAA)[0] +
				static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0x55)[0] + vel1[0]));
			const float scaledRhs0 = unscaledRhs0 * factor;
			const float impulse1 = temp[1].m_impulseApplied;
			const __m128 vel2 = _mm_add_ps(_mm_mul_ps(jac.m_jac2.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad),
				_mm_mul_ps(accA->m_scratch123.m_col0.m_quad, jac.m_jac2.m_angular[0].m_quad));
			frictionResults[0].m_impulseApplied = temp[0].m_impulseApplied;
			frictionResults[0].m_internalSolverData = scaledRhs0;
			schema = reinterpret_cast<const hkpJacobianSchema*>(friction + 1);
			temp += 4;
			frictionResults[1].m_impulseApplied = impulse1;
			const float scaledVel1 = velSum1 * stepsDt;
			const float unscaledRhs1 = rhs1 - scaledVel1;
			frictionResults[1].m_internalSolverData = unscaledRhs1 * factor;
			frictionResults[2].m_impulseApplied = temp[-2].m_impulseApplied;
			const float unscaledRhs2 = rhs2 * dampDivFrictionTauDt -
				static_cast<float>(_mm_shuffle_ps(vel2, vel2, 0xAA)[0] +
					static_cast<float>(_mm_shuffle_ps(vel2, vel2, 0x55)[0] + vel2[0])) *
					stepsDt;
			frictionResults[2].m_internalSolverData = factor * unscaledRhs2;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_2D_ROLLING_FRICTION: {
			const hkpJacobian2dRollingFrictionSchema* friction =
				reinterpret_cast<const hkpJacobian2dRollingFrictionSchema*>(schema);
			const float factor = temp[2].m_usedImpulseFactor;
			const int32_t numTemps = hkpJacobianSchema::s_schemaNumSolverElemTemps[type];
			const float rhs0 = friction->m_jac0.m_angular[1].m_quad[3];
			const float rhs1 = friction->m_jac1.m_angular[1].m_quad[3];
			const __m128 vel0 =
				_mm_add_ps(_mm_mul_ps(friction->m_jac0.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad),
					_mm_mul_ps(friction->m_jac0.m_angular[0].m_quad, accA->m_scratch123.m_col0.m_quad));
			const float impulse1 = temp[1].m_impulseApplied;
			const float velSum0 = static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0xAA)[0]) +
				static_cast<float>(_mm_shuffle_ps(vel0, vel0, 0x55)[0] + vel0[0]);
			const __m128 vel1 =
				_mm_add_ps(_mm_mul_ps(friction->m_jac1.m_angular[1].m_quad, accB->m_scratch123.m_col0.m_quad),
					_mm_mul_ps(accA->m_scratch123.m_col0.m_quad, friction->m_jac1.m_angular[0].m_quad));
			hkpSolverResults* frictionResults = friction->m_solverResults;
			frictionResults[0].m_impulseApplied = temp[0].m_impulseApplied;
			const float unscaledRhs0 = rhs0 * deltaTime - static_cast<float>(velSum0) * stepsDt;
			schema = reinterpret_cast<const hkpJacobianSchema*>(friction + 1);
			frictionResults[0].m_internalSolverData = unscaledRhs0 * factor;
			frictionResults[1].m_impulseApplied = impulse1;
			const float unscaledRhs1 = rhs1 * deltaTime -
				static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0xAA)[0] +
					static_cast<float>(_mm_shuffle_ps(vel1, vel1, 0x55)[0] + vel1[0])) *
					stepsDt;
			frictionResults[1].m_internalSolverData = factor * unscaledRhs1;
			temp += numTemps;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_MASS: {
			schema =
				reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const hkpJacobianSetMassSchema*>(schema) +
					1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_ADD_VELOCITY: {
			const hkpJacobianAddVelocitySchema* addVelocity =
				reinterpret_cast<const hkpJacobianAddVelocitySchema*>(schema);
			schema = reinterpret_cast<const hkpJacobianSchema*>(addVelocity + 1);
			hkpVelocityAccumulator* modified;
			if (addVelocity->m_bodyIndex == 0) {
				modifiedAccA = *accA;
				accA = &modifiedAccA;
				modified = &modifiedAccA;
			} else {
				modifiedAccB = *accB;
				accB = &modifiedAccB;
				modified = &modifiedAccB;
			}
			modified->m_linearVel.m_quad = _mm_add_ps(addVelocity->m_deltaVel.m_quad, modified->m_linearVel.m_quad);
			modified->m_scratch0.m_quad = _mm_add_ps(addVelocity->m_deltaVel.m_quad, modified->m_scratch0.m_quad);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_CENTER_OF_MASS: {
			const hkpJacobianSetCenterOfMassSchema* centerOfMass =
				reinterpret_cast<const hkpJacobianSetCenterOfMassSchema*>(schema);
			schema = reinterpret_cast<const hkpJacobianSchema*>(centerOfMass + 1);
			modifiedAccA = *accA;
			accA = &modifiedAccA;
			modifiedAccB = *accB;
			accB = &modifiedAccB;
			hkVector4 linVelA;
			linVelA.setMul3(centerOfMass->m_angToLinVelA, modifiedAccA.m_angularVel);
			hkVector4 sumLinVelA;
			sumLinVelA.setMul3(centerOfMass->m_angToLinVelA, modifiedAccA.m_scratch123.m_col0);
			hkVector4 linVelB;
			linVelB.setMul3(centerOfMass->m_angToLinVelB, modifiedAccB.m_angularVel);
			hkVector4 sumLinVelB;
			sumLinVelB.setMul3(centerOfMass->m_angToLinVelB, modifiedAccB.m_scratch123.m_col0);
			modifiedAccA.m_linearVel.m_quad = _mm_add_ps(modifiedAccA.m_linearVel.m_quad, linVelA.m_quad);
			modifiedAccA.m_scratch0.m_quad = _mm_add_ps(modifiedAccA.m_scratch0.m_quad, sumLinVelA.m_quad);
			modifiedAccB.m_linearVel.m_quad = _mm_add_ps(modifiedAccB.m_linearVel.m_quad, linVelB.m_quad);
			modifiedAccB.m_scratch0.m_quad = _mm_add_ps(modifiedAccB.m_scratch0.m_quad, sumLinVelB.m_quad);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_STIFF_SPRING_CHAIN: {
			const hkpJacobianStiffSpringChainSchema* chain =
				reinterpret_cast<const hkpJacobianStiffSpringChainSchema*>(schema);
			// The Jacobians follow the chain's head in the stream.
			const hkp1Lin2AngJacobian* jac = reinterpret_cast<const hkp1Lin2AngJacobian*>(chain + 1);
			if (!results) {
				temp += chain->m_numConstraints;
			} else {
				const hkpVelocityAccumulatorOffset* offsets = chain->getAccumulatorOffsetsBase();
				for (int32_t i = 0; i < chain->m_numConstraints; i++) {
					// The accumulators of the constraint's two bodies, by their byte offsets from the first.
					const hkpVelocityAccumulator* chainAccA =
						reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
							offsets[i].m_offset);
					const hkpVelocityAccumulator* chainAccB =
						reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
							offsets[i + 1].m_offset);
					const float rhs = jac->m_linear0.m_quad[3] * deltaTime;
					const __m128 angVel =
						_mm_add_ps(_mm_mul_ps(chainAccB->m_scratch123.m_col0.m_quad, jac->m_angular[1].m_quad),
							_mm_mul_ps(chainAccA->m_scratch123.m_col0.m_quad, jac->m_angular[0].m_quad));
					const __m128 linDiff = _mm_sub_ps(chainAccA->m_scratch0.m_quad, chainAccB->m_scratch0.m_quad);
					results->m_impulseApplied = temp->m_impulseApplied;
					const __m128 vel = _mm_add_ps(angVel, _mm_mul_ps(linDiff, jac->m_linear0.m_quad));
					const float velSum = static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
						static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0]));
					const float scaledVel = velSum * stepsDt;
					temp++;
					jac++;
					results->m_internalSolverData = rhs - scaledVel;
					results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
				}
			}
			schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(chain) + chain->m_size);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_BALL_SOCKET_CHAIN: {
			const hkpJacobianBallSocketChainSchema* chain =
				reinterpret_cast<const hkpJacobianBallSocketChainSchema*>(schema);
			// The Jacobians, three per constraint, follow the chain's head in the stream.
			const hkp1Lin2AngJacobian* jac = reinterpret_cast<const hkp1Lin2AngJacobian*>(chain + 1);
			if (!results) {
				temp += 3 * chain->m_numConstraints;
			} else {
				const hkpVelocityAccumulatorOffset* offsets = chain->getAccumulatorOffsetsBase();
				for (int32_t i = 0; i < chain->m_numConstraints; i++) {
					// The accumulators of the constraint's two bodies, by their byte offsets from the first.
					const hkpVelocityAccumulator* chainAccA =
						reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
							offsets[i].m_offset);
					const hkpVelocityAccumulator* chainAccB =
						reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
							offsets[i + 1].m_offset);
					for (int32_t row = 0; row < 3; row++) {
						const float rhs = jac->m_linear0.m_quad[3] * deltaTime;
						const __m128 angVel =
							_mm_add_ps(_mm_mul_ps(jac->m_angular[0].m_quad, chainAccA->m_scratch123.m_col0.m_quad),
								_mm_mul_ps(jac->m_angular[1].m_quad, chainAccB->m_scratch123.m_col0.m_quad));
						const __m128 linDiff = _mm_sub_ps(chainAccA->m_scratch0.m_quad, chainAccB->m_scratch0.m_quad);
						results->m_impulseApplied = temp->m_impulseApplied;
						const __m128 vel = _mm_add_ps(angVel, _mm_mul_ps(linDiff, jac->m_linear0.m_quad));
						temp++;
						jac++;
						const float velSum = static_cast<float>(_mm_shuffle_ps(vel, vel, 0xAA)[0] +
							static_cast<float>(_mm_shuffle_ps(vel, vel, 0x55)[0] + vel[0]));
						const float scaledVel = velSum * stepsDt;
						results->m_internalSolverData = rhs - scaledVel;
						results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + striding);
					}
				}
			}
			schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(chain) + chain->m_size);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_POWERED_CHAIN: {
			const hkpJacobianPoweredChainSchema* chain = reinterpret_cast<const hkpJacobianPoweredChainSchema*>(schema);
			for (int32_t i = 0; i < chain->m_numConstraints; i++) {
				chain->m_statusFlagsOutput[i] = chain->getChildConstraintStatusBase()[i].m_broken;
			}
			// Three linear Jacobians per constraint follow the chain's head in the stream, then three angular ones per
			// constraint.
			const hkp1Lin2AngJacobian* linJac = reinterpret_cast<const hkp1Lin2AngJacobian*>(chain + 1);
			const hkp2AngJacobian* angJac =
				reinterpret_cast<const hkp2AngJacobian*>(linJac + 3 * chain->m_numConstraints);
			if (!results) {
				temp += 6 * chain->m_numConstraints;
			} else {
				const hkpVelocityAccumulatorOffset* offsets = chain->getAccumulatorOffsetsBase();
				for (int32_t i = 0; i < chain->m_numConstraints; i++) {
					// The accumulators of the constraint's two bodies, by their byte offsets from the first.
					const hkpVelocityAccumulator* chainAccA =
						reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
							offsets[i].m_offset);
					const hkpVelocityAccumulator* chainAccB =
						reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
							offsets[i + 1].m_offset);
					// The exe writes the three linear rows' results to the constraint's first result and the angular
					// rows' to the fourth to sixth, 8 bytes apart whatever the striding.
					hkpSolverResults* angularResults = results + 3;
					for (int32_t row = 0; row < 3; row++) {
						const float linRhs = linJac->m_linear0.m_quad[3] * deltaTime;
						const __m128 linVel = _mm_add_ps(_mm_add_ps(_mm_mul_ps(linJac->m_angular[1].m_quad,
																		chainAccB->m_scratch123.m_col0.m_quad),
															 _mm_mul_ps(linJac->m_angular[0].m_quad,
																 chainAccA->m_scratch123.m_col0.m_quad)),
							_mm_mul_ps(_mm_sub_ps(chainAccA->m_scratch0.m_quad, chainAccB->m_scratch0.m_quad),
								linJac->m_linear0.m_quad));
						results->m_impulseApplied = temp[row].m_impulseApplied;
						const float linVelSum = static_cast<float>(_mm_shuffle_ps(linVel, linVel, 0xAA)[0] +
							static_cast<float>(_mm_shuffle_ps(linVel, linVel, 0x55)[0] + linVel[0]));
						const float scaledLinVel = linVelSum * stepsDt;
						results->m_internalSolverData = linRhs - scaledLinVel;
						const __m128 angVel =
							_mm_add_ps(_mm_mul_ps(angJac->m_angular[1].m_quad, chainAccB->m_scratch123.m_col0.m_quad),
								_mm_mul_ps(angJac->m_angular[0].m_quad, chainAccA->m_scratch123.m_col0.m_quad));
						float angRhs = angJac->m_angular[1].m_quad[3] * deltaTime -
							static_cast<float>(_mm_shuffle_ps(angVel, angVel, 0xAA)[0] +
								static_cast<float>(_mm_shuffle_ps(angVel, angVel, 0x55)[0] + angVel[0])) *
								stepsDt;
						angularResults[row].m_impulseApplied = temp[3 + row].m_impulseApplied;
						angularResults[row].m_internalSolverData = angRhs;
						// The motor's state for this row (bits 2 * row and 2 * row + 1): a broken minimum bounds the rhs
						// from below, a broken maximum from above, each by the row's motor limit over the Jacobian's w,
						// per step time.
						const hkp3dAngularMotorSolverInfo* status = &chain->getChildConstraintStatusBase()[i];
						const hkp1dMotorSolverInfo& motor = status->m_motorInfos[row];
						if ((status->m_broken >> (2 * row) & 3) ==
							hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MIN_BREACHED) {
							const float limit = motor.m_minImpulsePerSubstep / angJac->m_angular[0].m_quad[3];
							const float limitRhs = limit * info.m_deltaTime / motor.m_tau * 1.5;
							if (!(limitRhs < angRhs)) {
								angRhs = limitRhs;
							}
							angularResults[row].m_internalSolverData = angRhs;
						}
						if ((status->m_broken >> (2 * row) & 3) ==
							hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MAX_BREACHED) {
							const float limit = motor.m_maxImpulsePerSubstep / angJac->m_angular[0].m_quad[3];
							const float limitRhs = limit * info.m_deltaTime / motor.m_tau * 1.5;
							if (limitRhs > angRhs) {
								angularResults[row].m_internalSolverData = angRhs;
							} else {
								angularResults[row].m_internalSolverData = limitRhs;
							}
						}
						angJac++;
						linJac++;
					}
					results = reinterpret_cast<hkpSolverResults*>(reinterpret_cast<char*>(results) + 6 * striding);
					temp += 6;
				}
			}
			schema = reinterpret_cast<const hkpJacobianSchema*>(reinterpret_cast<const char*>(chain) + chain->m_size);
			continue;
		}
		default: {
			// SIMPLE_BEGIN, SIMPLE_END and anything past POWERED_CHAIN: the exe reads the same schema again.
			continue;
		}
		}
	}
}
