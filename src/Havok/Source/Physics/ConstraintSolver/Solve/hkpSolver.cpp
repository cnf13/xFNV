#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix6.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpPoweredChainSolverUtil.h"
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
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPairContactSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPoweredChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSingleContactSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianStiffSpringChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverElemTemp.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInternal.h"

#include <cmath>
#include <xmmintrin.h>

// The powered chain solver's velocities: for each constraint of the chain, the six rows' right-hand sides (the three
// linear rows from info.m_j, the three motor rows from info.m_jAng) into velocities[i], w zeroed. Called by
// hkSolver_solvePoweredChain with its hkpChainSolverInfo; returns the constraint count reached (0 when there are none).
static int32_t hkPoweredChain_BuildVelocities(const hkpChainSolverInfo& info, hkVector8* velocities);
// Recomputes one motor row's velocity, velocities[constraintIndex]'s angular component motorIndex, after the motor is
// enabled again; returns velocities.
static hkVector8* hkPoweredChain_computeMotorVelocity(int32_t constraintIndex, int32_t motorIndex,
	const hkpChainSolverInfo& info, hkVector8* velocities);
static uint8_t* copyByte(uint8_t* a1, uint8_t* a2);
static int32_t getNumSolverElemTemps(char* a1);
// The solver's case for a ball-and-socket chain schema (type 30).
static void hkSolver_solveBallAndSocketChain(const hkpSolverInfo& info, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianBallSocketChainSchema* chain, hkpSolverElemTemp* sr);
// The forward substitution of the powered chain's LU decomposition into g (info.m_numConstraints + 1 entries, the first
// zeroed). The one caller hkSolver_solvePoweredChain passes its hkpChainSolverInfo and its g buffer.
static void hkPoweredChain_ForwardSubstitute(const hkpChainSolverInfo& info, hkVector8* g);

// The constraint's two accumulators are info.m_accumsBase moved by the byte offsets of va[i] and va[i + 1] (raw memory
// addressed by byte offset, hence the casts); m_scratch0 and m_scratch123's first column hold what the chain solver
// keeps there of each body's linear and angular parts.
static int32_t hkPoweredChain_BuildVelocities(const hkpChainSolverInfo& info, hkVector8* velocities) {
	int32_t i = 0;
	if (info.m_numConstraints.m_storage > 0) {
		const hkp1Lin2AngJacobian* jac = info.m_j.m_storage;
		const hkp2AngJacobian* jacAng = info.m_jAng.m_storage;
		hkVector8* velocity = velocities;
		do {
			const hkpVelocityAccumulatorOffset* va = info.m_va.m_storage;
			const char* accumsBase = reinterpret_cast<const char*>(info.m_accumsBase.m_storage);
			// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
			const hkpVelocityAccumulator* accA =
				reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase + va[i].m_offset);
			// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
			const hkpVelocityAccumulator* accB =
				reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase + va[i + 1].m_offset);
			const __m128 scratchA0 = accA->m_scratch0.m_quad;
			const __m128 scratchA1 = accA->m_scratch123.m_col0.m_quad;
			const __m128 scratchB1 = accB->m_scratch123.m_col0.m_quad;
			const __m128 linDiff = _mm_sub_ps(_mm_sub_ps(accA->m_linearVel.m_quad, scratchA0),
				_mm_sub_ps(accB->m_linearVel.m_quad, accB->m_scratch0.m_quad));
			const __m128 scratchDiff = _mm_sub_ps(scratchA0, accB->m_scratch0.m_quad);
			const __m128 angB = _mm_sub_ps(accB->m_angularVel.m_quad, scratchB1);
			const __m128 angA = _mm_sub_ps(accA->m_angularVel.m_quad, scratchA1);
			const hkp1dMotorSolverInfo* motor = info.m_motorsState.m_storage[i].m_motorInfos;
			for (int32_t n = 0; n < 3; ++n) {
				// The linear row n.
				const float damping = info.m_schemaDamping.m_storage;
				const double tau = info.m_schemaTau.m_storage;
				const float tauFactor = info.m_solverInfo.m_invIntegrateVelocityFactor(0) * tau;
				const __m128 sum =
					_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(angA, jac->m_angular[0].m_quad),
														 _mm_mul_ps(angB, jac->m_angular[1].m_quad)),
											  _mm_mul_ps(linDiff, jac->m_linear0.m_quad)),
								   _mm_set1_ps(damping)),
						_mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(jac->m_angular[1].m_quad, scratchB1),
												  _mm_mul_ps(scratchA1, jac->m_angular[0].m_quad)),
									   _mm_mul_ps(scratchDiff, jac->m_linear0.m_quad)),
							_mm_set1_ps(tauFactor)));
				const float dot = static_cast<float>(sum[2] + static_cast<float>(sum[1] + sum[0]));
				velocity->m_lin(n) = static_cast<float>(tau * jac->m_linear0(3) - dot);
				// The motor row n.
				const float motorTau = motor[n].m_tau;
				const float motorTauFactor = info.m_solverInfo.m_invIntegrateVelocityFactor(0) * motorTau;
				const __m128 sumAng =
					_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(scratchA1, jacAng->m_angular[0].m_quad),
											  _mm_mul_ps(scratchB1, jacAng->m_angular[1].m_quad)),
								   _mm_set1_ps(motorTauFactor)),
						_mm_mul_ps(_mm_add_ps(_mm_mul_ps(angA, jacAng->m_angular[0].m_quad),
									   _mm_mul_ps(angB, jacAng->m_angular[1].m_quad)),
							_mm_set1_ps(motor[n].m_damping)));
				const float dotAng = static_cast<float>(sumAng[2] + static_cast<float>(sumAng[1] + sumAng[0]));
				velocity->m_ang(n) = static_cast<float>(motorTau * jacAng->m_angular[1](3) - dotAng);
				++jac;
				++jacAng;
			}
			velocity->m_lin(3) = 0.0f;
			velocity->m_ang(3) = 0.0f;
			++velocity;
			++i;
		} while (i < info.m_numConstraints.m_storage);
	}
	return i;
}

// The accumulators as in hkPoweredChain_BuildVelocities (byte offsets from the raw base).
static hkVector8* hkPoweredChain_computeMotorVelocity(int32_t constraintIndex, int32_t motorIndex,
	const hkpChainSolverInfo& info, hkVector8* velocities) {
	const hkpVelocityAccumulatorOffset* va = &info.m_va.m_storage[constraintIndex];
	const char* accumsBase = reinterpret_cast<const char*>(info.m_accumsBase.m_storage);
	// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
	const hkpVelocityAccumulator* accA = reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase + va[0].m_offset);
	// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
	const hkpVelocityAccumulator* accB = reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase + va[1].m_offset);
	const hkp1dMotorSolverInfo& motor = info.m_motorsState.m_storage[constraintIndex].m_motorInfos[motorIndex];
	const hkp2AngJacobian& jac = info.m_jAng.m_storage[3 * constraintIndex + motorIndex];
	const float motorTau = motor.m_tau;
	const float tauFactor = info.m_solverInfo.m_invIntegrateVelocityFactor(0) * motorTau;
	const __m128 scratchA1 = accA->m_scratch123.m_col0.m_quad;
	const __m128 scratchB1 = accB->m_scratch123.m_col0.m_quad;
	const __m128 sum = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(accA->m_angularVel.m_quad, scratchA1),
															jac.m_angular[0].m_quad),
												 _mm_mul_ps(_mm_sub_ps(accB->m_angularVel.m_quad, scratchB1),
													 jac.m_angular[1].m_quad)),
									  _mm_set1_ps(motor.m_damping)),
		_mm_mul_ps(_mm_add_ps(_mm_mul_ps(jac.m_angular[0].m_quad, scratchA1),
					   _mm_mul_ps(jac.m_angular[1].m_quad, scratchB1)),
			_mm_set1_ps(tauFactor)));
	const float dot = static_cast<float>(sum[2] + static_cast<float>(sum[1] + sum[0]));
	velocities[constraintIndex].m_ang(motorIndex) = static_cast<float>(motorTau * jac.m_angular[1](3) - dot);
	return velocities;
}

static uint8_t* copyByte(uint8_t* a1, uint8_t* a2) {
	*a1 = *a2;
	return a1;
}

// The table entry for the type in the schema's first byte (a signed byte index).
static int32_t getNumSolverElemTemps(char* a1) {
	return hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(*a1)];
}

// The solver's case for a ball-and-socket chain, every operation in the game's order and operand order. The forward
// pass solves the chain's block-tridiagonal system for the velocity errors of its links into the temp buffer; the
// backward pass applies each link's impulse to both of its bodies and adds it to the link's three solver results.
static void hkSolver_solveBallAndSocketChain(const hkpSolverInfo& info, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianBallSocketChainSchema* chain, hkpSolverElemTemp* sr) {
	const int32_t numConstraints = chain->m_numConstraints;
	// The schema stream is raw memory: the static accessor takes its head as an hkpJacobianSchema.
	const hkp1Lin2AngJacobian* jacobians =
		hkpJacobianBallSocketChainSchema::getJacobians(reinterpret_cast<hkpJacobianSchema*>(chain));
	const hkpConstraintChainMatrixTriple* matrices = chain->getMatrixBuffer();
	hkVector4* tempBuffer = chain->getTempBuffer();
	const hkpVelocityAccumulatorOffset* accumOffsets = chain->getAccumulatorOffsetsBase();
	tempBuffer[0].m_quad = _mm_setzero_ps();
	for (int32_t i = 0; i < numConstraints; i++) {
		// EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator> (the offsets count bytes from the accumulators'
		// base, in the raw accumulator buffer)
		const hkpVelocityAccumulator* accA =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
				accumOffsets[i].m_offset);
		// EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* accB =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accums) +
				accumOffsets[i + 1].m_offset);
		const hkp1Lin2AngJacobian* jac = &jacobians[3 * i];
		const __m128 scratch0A = accA->m_scratch0.m_quad;
		const __m128 scratch1A = accA->m_scratch123.m_col0.m_quad;
		const __m128 scratch0B = accB->m_scratch0.m_quad;
		const __m128 scratch1B = accB->m_scratch123.m_col0.m_quad;
		const __m128 angDiffB = _mm_sub_ps(accB->m_angularVel.m_quad, scratch1B);
		const __m128 angDiffA = _mm_sub_ps(accA->m_angularVel.m_quad, scratch1A);
		const __m128 linDiff = _mm_sub_ps(_mm_sub_ps(accA->m_linearVel.m_quad, scratch0A),
			_mm_sub_ps(accB->m_linearVel.m_quad, scratch0B));
		const __m128 scratch0Diff = _mm_sub_ps(scratch0A, scratch0B);
		const __m128 damping = _mm_set1_ps(chain->m_damping);
		const float tau = chain->m_tau;
		// x87: the product rounded to a float.
		const float factor = info.m_invIntegrateVelocityFactor.m_quad[0] * tau;
		const __m128 factorV = _mm_set1_ps(factor);

		// Each row's right-hand side: tau times the Jacobian's w less the dot of the Jacobian with the scratch
		// positions (times the factor) and velocity errors (times the damping), summed as (y + x) + z.
		const __m128 pos0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac[0].m_angular[0].m_quad, scratch1A),
										   _mm_mul_ps(jac[0].m_angular[1].m_quad, scratch1B)),
			_mm_mul_ps(scratch0Diff, jac[0].m_linear0.m_quad));
		const __m128 vel0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac[0].m_angular[0].m_quad, angDiffA),
										   _mm_mul_ps(jac[0].m_angular[1].m_quad, angDiffB)),
			_mm_mul_ps(jac[0].m_linear0.m_quad, linDiff));
		const __m128 dot0 = _mm_add_ps(_mm_mul_ps(pos0, factorV), _mm_mul_ps(vel0, damping));
		const __m128 dot0XY = _mm_add_ps(_mm_shuffle_ps(dot0, dot0, 0x55), dot0);
		const float dot0Sum = _mm_add_ps(_mm_shuffle_ps(dot0, dot0, 0xAA), dot0XY)[0];
		const float rhs0 = tau * jac[0].m_linear0.m_quad[3] - dot0Sum;

		const __m128 pos1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac[1].m_angular[1].m_quad, scratch1B),
										   _mm_mul_ps(jac[1].m_angular[0].m_quad, scratch1A)),
			_mm_mul_ps(scratch0Diff, jac[1].m_linear0.m_quad));
		const __m128 vel1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angDiffA, jac[1].m_angular[0].m_quad),
										   _mm_mul_ps(jac[1].m_angular[1].m_quad, angDiffB)),
			_mm_mul_ps(jac[1].m_linear0.m_quad, linDiff));
		const __m128 dot1 = _mm_add_ps(_mm_mul_ps(pos1, factorV), _mm_mul_ps(vel1, damping));
		const __m128 dot1XY = _mm_add_ps(_mm_shuffle_ps(dot1, dot1, 0x55), dot1);
		const float dot1Sum = _mm_add_ps(_mm_shuffle_ps(dot1, dot1, 0xAA), dot1XY)[0];
		const float rhs1 = tau * jac[1].m_linear0.m_quad[3] - dot1Sum;

		const __m128 pos2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac[2].m_angular[0].m_quad, scratch1A),
										   _mm_mul_ps(jac[2].m_angular[1].m_quad, scratch1B)),
			_mm_mul_ps(scratch0Diff, jac[2].m_linear0.m_quad));
		const __m128 vel2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(jac[2].m_angular[0].m_quad, angDiffA),
										   _mm_mul_ps(jac[2].m_angular[1].m_quad, angDiffB)),
			_mm_mul_ps(jac[2].m_linear0.m_quad, linDiff));
		const __m128 dot2 = _mm_add_ps(_mm_mul_ps(pos2, factorV), _mm_mul_ps(vel2, damping));
		const __m128 dot2XY = _mm_add_ps(_mm_shuffle_ps(dot2, dot2, 0x55), dot2);
		const float dot2Sum = _mm_add_ps(_mm_shuffle_ps(dot2, dot2, 0xAA), dot2XY)[0];
		const float rhs2 = tau * jac[2].m_linear0.m_quad[3] - dot2Sum;

		// Forward substitution: less the lower block times the previous link's solution, times the inverted
		// diagonal block.
		const hkMatrix3& lower = matrices[i].m_lower;
		const hkMatrix3& diagonalInv = matrices[i].m_diagonalInv;
		const __m128 prev = tempBuffer[i].m_quad;
		const __m128 lowerTimesPrev =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(prev, prev, 0x55), lower.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(prev, prev, 0x00), lower.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(prev, prev, 0xAA), lower.m_col2.m_quad));
		const __m128 rhs = _mm_sub_ps(_mm_setr_ps(rhs0, rhs1, rhs2, 0.0f), lowerTimesPrev);
		tempBuffer[i + 1].m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0x55), diagonalInv.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0x00), diagonalInv.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0xAA), diagonalInv.m_col2.m_quad));
	}

	// Back substitution from the last link, whose successor's solution is zero. Each component's impulse goes to body
	// A's velocities and, negated in the linear part, to body B's; the order of the stores matters, as the two
	// accumulators may be one.
	__m128 solX = _mm_setzero_ps();
	__m128 solY = _mm_setzero_ps();
	__m128 solZ = _mm_setzero_ps();
	for (int32_t i = numConstraints - 1; i >= 0; i--) {
		// EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
		hkpVelocityAccumulator* accA =
			reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<char*>(accums) + accumOffsets[i].m_offset);
		// EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
		hkpVelocityAccumulator* accB =
			reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<char*>(accums) + accumOffsets[i + 1].m_offset);
		const hkp1Lin2AngJacobian* jac = &jacobians[3 * i];
		const hkMatrix3& upper = matrices[i].m_upper;
		const __m128 sol = _mm_sub_ps(tempBuffer[i + 1].m_quad,
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(upper.m_col0.m_quad, solX), _mm_mul_ps(upper.m_col1.m_quad, solY)),
				_mm_mul_ps(upper.m_col2.m_quad, solZ)));

		solX = _mm_shuffle_ps(sol, sol, 0x00);
		const __m128 impulseA0 = _mm_mul_ps(accA->m_invMasses.m_quad, solX);
		accA->m_linearVel.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impulseA0, impulseA0, 0xFF), jac[0].m_linear0.m_quad),
				accA->m_linearVel.m_quad);
		const __m128 impulseB0 = _mm_mul_ps(accB->m_invMasses.m_quad, solX);
		accB->m_linearVel.m_quad = _mm_sub_ps(accB->m_linearVel.m_quad,
			_mm_mul_ps(_mm_shuffle_ps(impulseB0, impulseB0, 0xFF), jac[0].m_linear0.m_quad));
		const __m128 angA0 = _mm_add_ps(_mm_mul_ps(jac[0].m_angular[0].m_quad, impulseA0), accA->m_angularVel.m_quad);
		accB->m_angularVel.m_quad =
			_mm_add_ps(_mm_mul_ps(jac[0].m_angular[1].m_quad, impulseB0), accB->m_angularVel.m_quad);
		sr[3 * i].m_deltaRhs = sr[3 * i].m_deltaRhs + sol[0];
		accA->m_angularVel.m_quad = angA0;

		solY = _mm_shuffle_ps(sol, sol, 0x55);
		const __m128 impulseA1 = _mm_mul_ps(accA->m_invMasses.m_quad, solY);
		const __m128 impulseB1 = _mm_mul_ps(accB->m_invMasses.m_quad, solY);
		accA->m_linearVel.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impulseA1, impulseA1, 0xFF), jac[1].m_linear0.m_quad),
				accA->m_linearVel.m_quad);
		accB->m_linearVel.m_quad = _mm_sub_ps(accB->m_linearVel.m_quad,
			_mm_mul_ps(_mm_shuffle_ps(impulseB1, impulseB1, 0xFF), jac[1].m_linear0.m_quad));
		const __m128 angA1 = _mm_add_ps(_mm_mul_ps(jac[1].m_angular[0].m_quad, impulseA1), accA->m_angularVel.m_quad);
		const __m128 angB1 = _mm_add_ps(_mm_mul_ps(impulseB1, jac[1].m_angular[1].m_quad), accB->m_angularVel.m_quad);
		accA->m_angularVel.m_quad = angA1;
		accB->m_angularVel.m_quad = angB1;
		sr[3 * i + 1].m_deltaRhs = sr[3 * i + 1].m_deltaRhs + solY[0];

		solZ = _mm_shuffle_ps(sol, sol, 0xAA);
		const __m128 impulseA2 = _mm_mul_ps(accA->m_invMasses.m_quad, solZ);
		const __m128 impulseB2 = _mm_mul_ps(accB->m_invMasses.m_quad, solZ);
		accA->m_linearVel.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impulseA2, impulseA2, 0xFF), jac[2].m_linear0.m_quad),
				accA->m_linearVel.m_quad);
		accB->m_linearVel.m_quad = _mm_sub_ps(accB->m_linearVel.m_quad,
			_mm_mul_ps(_mm_shuffle_ps(impulseB2, impulseB2, 0xFF), jac[2].m_linear0.m_quad));
		const __m128 angA2 = _mm_add_ps(_mm_mul_ps(jac[2].m_angular[0].m_quad, impulseA2), accA->m_angularVel.m_quad);
		accB->m_angularVel.m_quad =
			_mm_add_ps(_mm_mul_ps(jac[2].m_angular[1].m_quad, impulseB2), accB->m_angularVel.m_quad);
		sr[3 * i + 2].m_deltaRhs = sr[3 * i + 2].m_deltaRhs + solZ[0];
		accA->m_angularVel.m_quad = angA2;
	}
}

// g[0] is zero; for each constraint, g[i + 1] = m_diagonalInv[i] * (velocities[i] - m_lower[i] * g[i]) (the forward
// substitution of the chain matrix's LU decomposition; velocities is info.m_velocitiesBuffer).
static void hkPoweredChain_ForwardSubstitute(const hkpChainSolverInfo& info, hkVector8* g) {
	g[0].m_lin.m_quad = _mm_setzero_ps();
	g[0].m_ang.m_quad = _mm_setzero_ps();
	for (int32_t i = 0; i < info.m_numConstraints.m_storage; ++i) {
		hkVector8 tmp;
		hkMatrix6SetMulV(tmp, info.m_triples.m_storage[i].m_lower, g[i]);
		const hkVector8& velocity = info.m_velocitiesBuffer.m_storage[i];
		tmp.m_lin.m_quad = _mm_sub_ps(velocity.m_lin.m_quad, tmp.m_lin.m_quad);
		tmp.m_ang.m_quad = _mm_sub_ps(velocity.m_ang.m_quad, tmp.m_ang.m_quad);
		hkMatrix6SetMulV(g[i + 1], info.m_triples.m_storage[i].m_diagonalInv, tmp);
	}
}

void hkSolveUpload() {
	;
}

// The solver's loop over one batch's schema stream (hkpSolve.h): each schema is solved inline on the two bodies of the
// last header schema, and the cursor walks the stream by each schema's size. The stream is raw memory read through each
// schema's class. getType and getNumSolverElemTemps take byte-pointer parameters, hence the type byte's address and the
// char view passed to them. A run of schemas of one type (a second pair contact, a 3d or 2d friction after a contact or
// a rolling friction, a header after a friction) goes back through the switch, which reaches the same code with the
// same calls.
int32_t hkSolveConstraintsByTheSteps(const hkpSolverInfo& info, int32_t currentSolverStep,
	int32_t currentSolverMicroStep, const hkpJacobianSchema* schemas, hkpVelocityAccumulator* accumulators,
	hkpSolverElemTemp* elemTemp) {
	if (!HK_flyingcolors_solver) {
		hkpProcessFlyingColors(&HK_flyingcolors_solver);
		if (!HK_flyingcolors_solver) {
			return 0;
		}
	}
	hkSolver::SolverStepTemp stepTemp;
	stepTemp.m_currentStep.m_storage = currentSolverStep;
	stepTemp.m_intregratePositions.m_storage = currentSolverMicroStep ? 0.0f : 1.0f;

	// The getType helper and the chain solvers take the stream as non-const.
	hkpJacobianSchema* pSchema = const_cast<hkpJacobianSchema*>(schemas);
	if (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_END) {
		return 1;
	}
	// Reads the first schema as a header whatever its type.
	hkpVelocityAccumulator* pAccA = &accumulators[pSchema->getHeader().m_bodyAIndex];
	hkpVelocityAccumulator* pAccB = &accumulators[pSchema->getHeader().m_bodyBIndex];
	pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	while (true) {
		__m128& linA = pAccA->m_linearVel.m_quad;
		__m128& angA = pAccA->m_angularVel.m_quad;
		__m128& invA = pAccA->m_invMasses.m_quad;
		__m128& s0A = pAccA->m_scratch0.m_quad;
		__m128& s1A = pAccA->m_scratch123.m_col0.m_quad;
		__m128& linB = pAccB->m_linearVel.m_quad;
		__m128& angB = pAccB->m_angularVel.m_quad;
		__m128& invB = pAccB->m_invMasses.m_quad;
		__m128& s0B = pAccB->m_scratch0.m_quad;
		__m128& s1B = pAccB->m_scratch123.m_col0.m_quad;
		switch (pSchema->m_bytes.m_type) {
		case hkpJacobianSchema::SCHEMA_TYPE_END: {
			return 1;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_HEADER: {
			pAccA = &accumulators[pSchema->getHeader().m_bodyAIndex];
			pAccB = &accumulators[pSchema->getHeader().m_bodyBIndex];
			pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_GOTO: {
			hkpJacobianGotoSchema* pS = reinterpret_cast<hkpJacobianGotoSchema*>(pSchema);
			// The offset is added to the cursor as a signed value; a backward jump is a negative one.
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) +
				static_cast<int32_t>(pS->m_offset));
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER: {
			++pSchema;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL: {
			uint8_t uType;
			do {
				hkpJacobian1dBilateralSchema* pS = reinterpret_cast<hkpJacobian1dBilateralSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad = a0;
				__m128 m_quad_1 = l0;
				__m128 v201 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, m_quad)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				float v202 = _mm_shuffle_ps(v201, v201, 85)[0] + v201[0];
				float v203 = _mm_shuffle_ps(v201, v201, 170)[0];
				__m128 v204 = _mm_shuffle_ps(m_quad_1, m_quad_1, 255);
				v204[0] = static_cast<float>(v204[0] - static_cast<float>(v203 + v202)) *
					_mm_shuffle_ps(m_quad, m_quad, 255)[0];
				__m128 v205 = v204;
				__m128 v206 = _mm_shuffle_ps(v205, v205, 0);
				__m128 v207 = _mm_mul_ps(invB, v206);
				__m128 v208 = _mm_mul_ps(invA, v206);
				linA = _mm_add_ps(linA, _mm_mul_ps(l0, _mm_shuffle_ps(v208, v208, 255)));
				linB = _mm_sub_ps(linB, _mm_mul_ps(m_quad_1, _mm_shuffle_ps(v207, v207, 255)));
				__m128 v209 = _mm_mul_ps(a0, v208);
				__m128 v210 = _mm_mul_ps(a1, v207);
				v207[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(angA, v209);
				angB = _mm_add_ps(angB, v210);
				elemTemp->m_deltaRhs = v207[0] + v205[0];
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT: {
			uint8_t uType;
			do {
				// The impulse limit is in the w of the second angular row; the schema has the bilateral's size.
				hkpJacobian1dBilateralSchema* pS = reinterpret_cast<hkpJacobian1dBilateralSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad_2 = a0;
				__m128 v212 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, m_quad_2)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				__m128 m_quad_3 = l0;
				__m128 v214 = _mm_shuffle_ps(l0, l0, 255);
				v214[0] = static_cast<float>(v214[0] -
							  static_cast<float>(_mm_shuffle_ps(v212, v212, 170)[0] +
								  static_cast<float>(_mm_shuffle_ps(v212, v212, 85)[0] + v212[0]))) *
					_mm_shuffle_ps(m_quad_2, m_quad_2, 255)[0];
				__m128 m_quad_4 = hkQuadReal0000;
				__m128 v216 = _mm_shuffle_ps(a1, a1, 255);
				m_quad_4[0] = static_cast<float>(0.0f - v216[0]) - elemTemp->m_deltaRhs;
				v216[0] = v216[0] - elemTemp->m_deltaRhs;
				__m128 v217 = _mm_min_ps(v216, _mm_max_ps(m_quad_4, v214));
				__m128 v218 = _mm_shuffle_ps(v217, v217, 0);
				__m128 v219 = _mm_mul_ps(invB, v218);
				__m128 v220 = _mm_mul_ps(invA, v218);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v220, v220, 255), l0), linA);
				linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v219, v219, 255), m_quad_3));
				__m128 v221 = _mm_mul_ps(a0, v220);
				__m128 v222 = _mm_mul_ps(a1, v219);
				v219[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(angA, v221);
				angB = _mm_add_ps(angB, v222);
				elemTemp->m_deltaRhs = v219[0] + v217[0];
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
				hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU: {
			uint8_t uType;
			do {
				hkpJacobian1dBilateralUserTauSchema* pS =
					reinterpret_cast<hkpJacobian1dBilateralUserTauSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v338 = pS->m_tau;
				float v367 = l0[3];
				__m128 v224 = _mm_set_ss(pS->m_damping);
				__m128 v225 = _mm_sub_ps(_mm_sub_ps(linA, s0A), _mm_sub_ps(linB, s0B));
				__m128 m_quad_5 = a1;
				__m128 v227 = _mm_sub_ps(angB, s1B);
				__m128 v228 = _mm_mul_ps(a0, _mm_sub_ps(angA, s1A));
				__m128 m_quad_6 = l0;
				double v230 = v338;
				float v339 = info.m_invIntegrateVelocityFactor.m_quad[0u] * v338;
				__m128 v231 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v224, v224, 0),
											 _mm_add_ps(_mm_add_ps(_mm_mul_ps(m_quad_5, v227), v228),
												 _mm_mul_ps(v225, m_quad_6))),
					_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v339), _mm_set_ss(v339), 0),
						_mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, s1B), _mm_mul_ps(a0, s1A)),
							_mm_mul_ps(_mm_sub_ps(s0A, s0B), l0))));
				float v384 = _mm_shuffle_ps(v231, v231, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v231, v231, 85)[0] + v231[0]);
				__m128 m_quad_7 = l0;
				float v340 = v230 * v367 - v384;
				__m128 v233 = _mm_set_ss(v340);
				v233[0] = v340 * _mm_shuffle_ps(a0, a0, 255)[0];
				__m128 v234 = _mm_shuffle_ps(v233, v233, 0);
				__m128 v235 = _mm_mul_ps(v234, invA);
				__m128 v236 = _mm_mul_ps(v234, invB);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v235, v235, 255), l0), linA);
				linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v236, v236, 255), m_quad_7));
				__m128 v237 = _mm_mul_ps(a0, v235);
				__m128 v238 = _mm_mul_ps(a1, v236);
				v236[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(v237, angA);
				angB = _mm_add_ps(angB, v238);
				elemTemp->m_deltaRhs = v236[0] + v233[0];
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
				hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS: {
			uint8_t uType;
			do {
				hkpJacobian1dLinearLimitsSchema* pS = reinterpret_cast<hkpJacobian1dLinearLimitsSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 v248 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(a0, angA)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				float v388 = _mm_shuffle_ps(l0, l0, 255)[0] -
					static_cast<float>(_mm_shuffle_ps(v248, v248, 170)[0] +
						static_cast<float>(_mm_shuffle_ps(v248, v248, 85)[0] + v248[0]));
				float v359 = v388;
				float v343 = -elemTemp->m_deltaRhs;
				float v312 = v388 - pS->m_minRhs;
				float v313 = v312 * a0[3];
				if (v343 < static_cast<double>(v313)) {
					__m128 v249 = _mm_shuffle_ps(_mm_set_ss(v313), _mm_set_ss(v313), 0);
					__m128 v250 = _mm_mul_ps(invB, v249);
					__m128 v251 = _mm_mul_ps(invA, v249);
					__m128 v252 = _mm_mul_ps(_mm_shuffle_ps(v250, v250, 255), l0);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v251, v251, 255), l0), linA);
					linB = _mm_sub_ps(linB, v252);
					__m128 v253 = _mm_mul_ps(a0, v251);
					__m128 v254 = _mm_mul_ps(v250, a1);
					v251[0] = elemTemp->m_deltaRhs;
					angA = _mm_add_ps(v253, angA);
					angB = _mm_add_ps(v254, angB);
					elemTemp->m_deltaRhs = v251[0] + v313;
				}
				float v314 = v359 - pS->m_maxRhs;
				float v315 = v314 * a0[3];
				if (v315 < static_cast<double>(v343)) {
					__m128 v255 = _mm_shuffle_ps(_mm_set_ss(v315), _mm_set_ss(v315), 0);
					__m128 v256 = _mm_mul_ps(invA, v255);
					__m128 v257 = _mm_mul_ps(invB, v255);
					__m128 v258 = _mm_mul_ps(_mm_shuffle_ps(v257, v257, 255), l0);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v256, v256, 255), l0), linA);
					linB = _mm_sub_ps(linB, v258);
					__m128 v259 = _mm_mul_ps(v257, a1);
					v255[0] = elemTemp->m_deltaRhs;
					angA = _mm_add_ps(_mm_mul_ps(v256, a0), angA);
					angB = _mm_add_ps(angB, v259);
					elemTemp->m_deltaRhs = v255[0] + v315;
				}
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION: {
			uint8_t uType;
			do {
				hkpJacobian1dFrictionSchema* pS = reinterpret_cast<hkpJacobian1dFrictionSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				double v240;
				__m128 v239 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(a0, angA)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				float v386 = _mm_shuffle_ps(l0, l0, 255)[0] -
					static_cast<float>(_mm_shuffle_ps(v239, v239, 170)[0] +
						static_cast<float>(_mm_shuffle_ps(v239, v239, 85)[0] + v239[0]));
				float v311 = a0[3] * v386;
				float v341 = std::fabs(static_cast<double>(v311));
				if (!(pS->m_maxImpulse < static_cast<double>(v341))) {
					v240 = 1.0;
				} else {
					float v342 = pS->m_maxImpulse / v341;
					v240 = v342;
					v311 = v311 * v342;
				}
				elemTemp[1].m_deltaRhs = v240;
				__m128 v241 = _mm_shuffle_ps(_mm_set_ss(v311), _mm_set_ss(v311), 0);
				__m128 v242 = _mm_mul_ps(v241, invA);
				__m128 v243 = _mm_mul_ps(v241, invB);
				__m128 v244 = _mm_mul_ps(_mm_shuffle_ps(v243, v243, 255), l0);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v242, v242, 255), l0), linA);
				linB = _mm_sub_ps(linB, v244);
				__m128 v245 = _mm_mul_ps(a0, v242);
				__m128 v246 = _mm_mul_ps(v243, a1);
				v242[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(v245, angA);
				angB = _mm_add_ps(angB, v246);
				elemTemp->m_deltaRhs = v242[0] + v311;
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR: {
			uint8_t uType;
			do {
				hkpJacobian1dLinearMotorSchema* pS = reinterpret_cast<hkpJacobian1dLinearMotorSchema*>(pSchema);
				const hkp1dMotorSolverInfo& motor = pS->m_motorSolverInfo;
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v344 = motor.m_velocity * stepTemp.m_intregratePositions.m_storage + elemTemp[2].m_deltaRhs;
				elemTemp[2].m_deltaRhs = v344;
				float v261 = motor.m_damping;
				float v360 = l0[3] + v344;
				float v345 = motor.m_tau;
				__m128 v262 = _mm_sub_ps(_mm_sub_ps(linA, s0A), _mm_sub_ps(linB, s0B));
				__m128 v263 = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
				__m128 v264 = _mm_mul_ps(_mm_sub_ps(angB, s1B), a1);
				__m128 m_quad_8 = l0;
				__m128 v266 = _mm_add_ps(_mm_add_ps(v264, v263), _mm_mul_ps(v262, m_quad_8));
				double v267 = v345;
				float v346 = v345 * info.m_invIntegrateVelocityFactor.m_quad[0u];
				__m128 v268 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(s1B, a1), _mm_mul_ps(s1A, a0)),
					_mm_mul_ps(_mm_sub_ps(s0A, s0B), l0));
				__m128 v269 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v346), _mm_set_ss(v346), 0), v268);
				v268[0] = v261;
				__m128 v270 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v268, v268, 0), v266), v269);
				float v375 = _mm_shuffle_ps(v270, v270, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v270, v270, 85)[0] + v270[0]);
				float v347 = v267 * v360 - v375;
				double v271 = a0[3] * v347;
				float v361 = v271;
				double v272 = v361;
				if (!(motor.m_maxImpulsePerSubstep < static_cast<double>(v361))) {
					if (!(motor.m_minImpulsePerSubstep > v272)) {
						elemTemp[1].m_deltaRhs = 1.0f;
					} else {
						elemTemp[1].m_deltaRhs = motor.m_minImpulsePerSubstep / v272;
						v361 = motor.m_minImpulsePerSubstep;
					}
				} else {
					elemTemp[1].m_deltaRhs = motor.m_maxImpulsePerSubstep / v272;
					v361 = motor.m_maxImpulsePerSubstep;
				}
				__m128 v273 = _mm_shuffle_ps(_mm_set_ss(v361), _mm_set_ss(v361), 0);
				__m128 v274 = _mm_mul_ps(v273, invA);
				__m128 v275 = _mm_mul_ps(v273, invB);
				__m128 v276 = _mm_mul_ps(_mm_shuffle_ps(v275, v275, 255), l0);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v274, v274, 255), l0), linA);
				linB = _mm_sub_ps(linB, v276);
				__m128 v277 = _mm_mul_ps(v275, a1);
				float fImpulseApplied = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(_mm_mul_ps(v274, a0), angA);
				angB = _mm_add_ps(v277, angB);
				elemTemp->m_deltaRhs = fImpulseApplied + v361;
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY: {
			hkpJacobian1dPulleySchema* pS = reinterpret_cast<hkpJacobian1dPulleySchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_linear[0].m_quad;
			const __m128& l1 = pS->m_jac.m_linear[1].m_quad;
			const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
			const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
			__m128 v280 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(a0, angA)),
				_mm_sub_ps(_mm_mul_ps(linA, l0), _mm_mul_ps(l1, linB)));
			hkSimdReal sDot;
			sDot.m_real = _mm_shuffle_ps(v280, v280, 170);
			sDot.m_real[0] = sDot.m_real[0] + static_cast<float>(_mm_shuffle_ps(v280, v280, 85)[0] + v280[0]);
			hkSimdReal sRhs;
			sRhs.m_real = _mm_shuffle_ps(l0, l0, 255);
			hkSimdReal sDelta = sRhs - sDot;
			hkSimdReal sInvDiag;
			sInvDiag.m_real = _mm_shuffle_ps(a0, a0, 255);
			hkSimdReal sImpulse = sDelta * sInvDiag;
			__m128 v282 = _mm_shuffle_ps(sImpulse.m_real, sImpulse.m_real, 0);
			__m128 v283 = _mm_mul_ps(invB, v282);
			__m128 v284 = _mm_mul_ps(invA, v282);
			__m128 v285 = _mm_mul_ps(l1, _mm_shuffle_ps(v283, v283, 255));
			linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v284, v284, 255), l0));
			linB = _mm_sub_ps(linB, v285);
			__m128 v286 = _mm_mul_ps(a0, v284);
			__m128 v287 = _mm_mul_ps(a1, v283);
			hkSimdReal sApplied;
			sApplied.m_real = _mm_set_ss(elemTemp->m_deltaRhs);
			angA = _mm_add_ps(angA, v286);
			angB = _mm_add_ps(angB, v287);
			hkSimdReal sSum = sApplied + sImpulse;
			elemTemp->m_deltaRhs = sSum.m_real[0];
			++elemTemp;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR: {
			do {
				hkpJacobian1dAngularSchema* pS = reinterpret_cast<hkpJacobian1dAngularSchema*>(pSchema);
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				__m128 m_quad_9 = pS->m_jac.m_angular[1].m_quad;
				__m128 v190 = _mm_add_ps(_mm_mul_ps(m_quad_9, angB), _mm_mul_ps(angA, a0));
				float v191 = _mm_shuffle_ps(v190, v190, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v190, v190, 85)[0] + v190[0]);
				__m128 v192 = _mm_shuffle_ps(m_quad_9, m_quad_9, 255);
				v192[0] = static_cast<float>(v192[0] - v191) * _mm_shuffle_ps(a0, a0, 255)[0];
				float v193 = v192[0];
				__m128 v194 = _mm_shuffle_ps(v192, v192, 0);
				__m128 v195 = _mm_mul_ps(_mm_mul_ps(v194, invA), a0);
				float fImpulseApplied = elemTemp->m_deltaRhs;
				__m128 v197 = _mm_mul_ps(_mm_mul_ps(v194, invB), m_quad_9);
				angA = _mm_add_ps(v195, angA);
				angB = _mm_add_ps(v197, angB);
				elemTemp->m_deltaRhs = fImpulseApplied + v193;
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS: {
			do {
				hkpJacobian1dAngularLimitsSchema* pS = reinterpret_cast<hkpJacobian1dAngularLimitsSchema*>(pSchema);
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v331 = pS->m_tau;
				float v304 = a1[3];
				__m128 m_damping_low = _mm_set_ss(info.m_damping);
				__m128 v159 = _mm_sub_ps(angB, s1B);
				__m128 v160 = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
				__m128 m_quad_10 = a1;
				double v162 = v331;
				float v332 = info.m_invIntegrateVelocityFactor.m_quad[0u] * v331;
				__m128 v163 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_damping_low, m_damping_low, 0),
											 _mm_add_ps(_mm_mul_ps(m_quad_10, v159), v160)),
					_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v332), _mm_set_ss(v332), 0),
						_mm_add_ps(_mm_mul_ps(a1, s1B), _mm_mul_ps(s1A, a0))));
				float v376 = _mm_shuffle_ps(v163, v163, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v163, v163, 85)[0] + v163[0]);
				float v378 = v162 * v304 - v376;
				float v334 = -elemTemp->m_deltaRhs;
				float v305 = v378 - pS->m_minRhs;
				float v306 = a0[3] * v305;
				if (v334 < static_cast<double>(v306)) {
					__m128 v164 = _mm_set_ss(v306);
					__m128 v165 = _mm_shuffle_ps(v164, v164, 0);
					__m128 v166 = _mm_mul_ps(_mm_mul_ps(invA, v165), a0);
					__m128 v167 = _mm_mul_ps(a1, _mm_mul_ps(invB, v165));
					float fImpulseApplied = elemTemp->m_deltaRhs;
					angA = _mm_add_ps(v166, angA);
					angB = _mm_add_ps(angB, v167);
					elemTemp->m_deltaRhs = fImpulseApplied + v164[0];
				} else {
					float v307 = v378 - pS->m_maxRhs;
					float v308 = a0[3] * v307;
					if (v308 < static_cast<double>(v334)) {
						__m128 v164 = _mm_set_ss(v308);
						__m128 v169 = _mm_shuffle_ps(v164, v164, 0);
						__m128 v170 = _mm_mul_ps(_mm_mul_ps(invA, v169), a0);
						__m128 v171 = _mm_mul_ps(a1, _mm_mul_ps(invB, v169));
						float fImpulseApplied = elemTemp->m_deltaRhs;
						angA = _mm_add_ps(v170, angA);
						angB = _mm_add_ps(v171, angB);
						elemTemp->m_deltaRhs = fImpulseApplied + v164[0];
					}
				}
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION: {
			do {
				hkpJacobian1dAngularFrictionSchema* pS = reinterpret_cast<hkpJacobian1dAngularFrictionSchema*>(pSchema);
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				double v153;
				__m128 v152 = _mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0));
				float v391 = _mm_shuffle_ps(a1, a1, 255)[0] -
					static_cast<float>(_mm_shuffle_ps(v152, v152, 170)[0] +
						static_cast<float>(_mm_shuffle_ps(v152, v152, 85)[0] + v152[0]));
				float v303 = a0[3] * v391;
				float v329 = std::fabs(static_cast<double>(v303));
				if (!(pS->m_maxImpulse < static_cast<double>(v329))) {
					v153 = 1.0;
				} else {
					float v330 = pS->m_maxImpulse / v329;
					v153 = v330;
					v303 = v303 * v330;
				}
				elemTemp[1].m_deltaRhs = v153;
				__m128 v154 = _mm_shuffle_ps(_mm_set_ss(v303), _mm_set_ss(v303), 0);
				__m128 v155 = _mm_mul_ps(v154, invA);
				__m128 v156 = _mm_mul_ps(a1, _mm_mul_ps(v154, invB));
				v154[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(angA, _mm_mul_ps(v155, a0));
				angB = _mm_add_ps(angB, v156);
				elemTemp->m_deltaRhs = v154[0] + v303;
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR: {
			do {
				hkpJacobian1dAngularMotorSchema* pS = reinterpret_cast<hkpJacobian1dAngularMotorSchema*>(pSchema);
				const hkp1dMotorSolverInfo& motor = pS->m_motorSolverInfo;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v335 = motor.m_velocity * stepTemp.m_intregratePositions.m_storage + elemTemp[2].m_deltaRhs;
				elemTemp[2].m_deltaRhs = v335;
				float v173 = motor.m_damping;
				__m128 v174 = _mm_sub_ps(angB, s1B);
				float v309 = a1[3] + v335;
				__m128 v175 = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
				__m128 v176 = a1;
				double v177 = motor.m_tau;
				__m128 v178 = _mm_add_ps(_mm_mul_ps(v174, v176), v175);
				float v336 = v177 * info.m_invIntegrateVelocityFactor.m_quad[0u];
				__m128 v179 = _mm_add_ps(_mm_mul_ps(s1B, a1), _mm_mul_ps(s1A, a0));
				__m128 v180 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v336), _mm_set_ss(v336), 0), v179);
				v179[0] = v173;
				__m128 v181 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v179, v179, 0), v178), v180);
				float v380 = _mm_shuffle_ps(v181, v181, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v181, v181, 85)[0] + v181[0]);
				float v337 = v177 * v309 - v380;
				float v310 = a0[3] * v337;
				double v182 = v310;
				if (!(motor.m_maxImpulsePerSubstep < static_cast<double>(v310))) {
					if (!(motor.m_minImpulsePerSubstep > v182)) {
						elemTemp[1].m_deltaRhs = 1.0f;
					} else {
						elemTemp[1].m_deltaRhs = motor.m_minImpulsePerSubstep / v182;
						v310 = motor.m_minImpulsePerSubstep;
					}
				} else {
					elemTemp[1].m_deltaRhs = motor.m_maxImpulsePerSubstep / v182;
					v310 = motor.m_maxImpulsePerSubstep;
				}
				__m128 v184 = _mm_shuffle_ps(_mm_set_ss(v310), _mm_set_ss(v310), 0);
				__m128 v185 = _mm_mul_ps(v184, invA);
				__m128 v186 = _mm_mul_ps(_mm_mul_ps(v184, invB), a1);
				float fImpulseApplied = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(angA, _mm_mul_ps(v185, a0));
				angB = _mm_add_ps(v186, angB);
				elemTemp->m_deltaRhs = fImpulseApplied + v310;
				elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT_WITH_ACCESSOR_CHECK: {
			do {
				hkpJacobianSchema* pCurrent = pSchema;
				hkpJacobianSingleContactSchema* pS = reinterpret_cast<hkpJacobianSingleContactSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad_11 = a0;
				__m128 m_quad_12 = l0;
				__m128 v65 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, m_quad_11)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				__m128 v66 = _mm_shuffle_ps(l0, l0, 255);
				v66[0] = static_cast<float>(v66[0] -
							 static_cast<float>(_mm_shuffle_ps(v65, v65, 170)[0] +
								 static_cast<float>(_mm_shuffle_ps(v65, v65, 85)[0] + v65[0]))) *
					_mm_shuffle_ps(m_quad_11, m_quad_11, 255)[0];
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
				__m128 m_quad_13 = hkQuadReal0000;
				m_quad_13[0] = 0.0f - elemTemp->m_deltaRhs;
				__m128 v68 = _mm_max_ps(m_quad_13, v66);
				__m128 v69 = _mm_shuffle_ps(v68, v68, 0);
				__m128 v70 = _mm_mul_ps(v69, invA);
				__m128 v71 = _mm_mul_ps(v69, invB);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v70, v70, 255), m_quad_12), linA);
				linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v71, v71, 255), m_quad_12));
				__m128 v72 = _mm_mul_ps(a0, v70);
				__m128 v73 = _mm_mul_ps(a1, v71);
				v71[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(v72, angA);
				angB = _mm_add_ps(angB, v73);
				elemTemp->m_deltaRhs = v71[0] + v68[0];
				// EXE-INLINED 0xD921D0
				elemTemp +=
					hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(pCurrent->m_bytes.m_type)];
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT_WITH_ACCESSOR_CHECK: {
			do {
				// The contact's maximum impulse is in the w of the second angular row.
				hkpJacobianSingleContactSchema* pS = reinterpret_cast<hkpJacobianSingleContactSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad_14 = a0;
				__m128 v75 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, m_quad_14)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				__m128 v76 = _mm_shuffle_ps(l0, l0, 255);
				v76[0] = static_cast<float>(v76[0] -
							 static_cast<float>(_mm_shuffle_ps(v75, v75, 170)[0] +
								 static_cast<float>(_mm_shuffle_ps(v75, v75, 85)[0] + v75[0]))) *
					_mm_shuffle_ps(m_quad_14, m_quad_14, 255)[0];
				__m128 m_quad_15 = hkQuadReal0000;
				m_quad_15[0] = 0.0f - elemTemp->m_deltaRhs;
				__m128 v78 = _mm_shuffle_ps(a1, a1, 255);
				v78[0] = v78[0] - elemTemp->m_deltaRhs;
				__m128 v79 = _mm_min_ps(v78, _mm_max_ps(m_quad_15, v76));
				__m128 v80 = _mm_shuffle_ps(v79, v79, 0);
				__m128 v81 = _mm_mul_ps(v80, invA);
				__m128 v82 = _mm_mul_ps(v80, invB);
				__m128 v83 = _mm_mul_ps(_mm_shuffle_ps(v82, v82, 255), l0);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v81, v81, 255), l0), linA);
				linB = _mm_sub_ps(linB, v83);
				__m128 v84 = _mm_mul_ps(a0, v81);
				__m128 v85 = _mm_mul_ps(v82, a1);
				v81[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(v84, angA);
				angB = _mm_add_ps(v85, angB);
				elemTemp->m_deltaRhs = v81[0] + v79[0];
				// EXE-INLINED 0xD921D0
				elemTemp += hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(pSchema->m_bytes.m_type)];
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT_WITH_ACCESSOR_CHECK: {
			hkpJacobianPairContactSchema* pS = reinterpret_cast<hkpJacobianPairContactSchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
			const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
			const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
			const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
			__m128 v12 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(a00, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
			float v395 = _mm_shuffle_ps(l0, l0, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v12, v12, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v12, v12, 85)[0] + v12[0]));
			__m128 v15 = l1;
			__m128 v16 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), v15));
			float v17 = _mm_shuffle_ps(v15, v15, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v16, v16, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v16, v16, 85)[0] + v16[0]));
			float v350 = v395 * a01[3] + v17 * pS->m_invJac01;
			float v296 = v395 * pS->m_invJac01 + v17 * a11[3];
			bool bFirstOnly;
			if (!(-elemTemp->m_deltaRhs < v350)) {
				float v297 = v17 * a10[3];
				bFirstOnly = -elemTemp[1].m_deltaRhs >= v297;
				if (!bFirstOnly) {
					// The second point alone.
					__m128 v38 = l1;
					__m128 v39 = _mm_shuffle_ps(_mm_set_ss(v297), _mm_set_ss(v297), 0);
					__m128 v40 = _mm_mul_ps(invA, v39);
					__m128 v41 = _mm_mul_ps(invB, v39);
					linA = _mm_add_ps(_mm_mul_ps(v38, _mm_shuffle_ps(v40, v40, 255)), linA);
					linB = _mm_sub_ps(linB, _mm_mul_ps(v38, _mm_shuffle_ps(v41, v41, 255)));
					__m128 v42 = _mm_mul_ps(a10, v40);
					__m128 v43 = _mm_mul_ps(a11, v41);
					v41[0] = elemTemp[1].m_deltaRhs;
					angA = _mm_add_ps(angA, v42);
					angB = _mm_add_ps(angB, v43);
					elemTemp[1].m_deltaRhs = v41[0] + v297;
				}
			} else {
				bFirstOnly = !(-elemTemp[1].m_deltaRhs < v296);
				if (!bFirstOnly) {
					// Both points.
					__m128 v18 = _mm_shuffle_ps(_mm_set_ss(v350), _mm_set_ss(v350), 0);
					__m128 v19 = _mm_mul_ps(v18, invA);
					__m128 v20 = _mm_mul_ps(v18, invB);
					__m128 v21 = _mm_mul_ps(_mm_shuffle_ps(v20, v20, 255), l0);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v19, v19, 255), l0), linA);
					linB = _mm_sub_ps(linB, v21);
					__m128 v22 = _mm_mul_ps(v20, a01);
					float fImpulseApplied = elemTemp->m_deltaRhs;
					angA = _mm_add_ps(_mm_mul_ps(v19, a00), angA);
					angB = _mm_add_ps(v22, angB);
					elemTemp->m_deltaRhs = fImpulseApplied + v350;
					__m128 v24 = _mm_shuffle_ps(_mm_set_ss(v296), _mm_set_ss(v296), 0);
					__m128 v25 = _mm_mul_ps(v24, invA);
					__m128 v26 = l1;
					__m128 v27 = _mm_mul_ps(v24, invB);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v25, v25, 255), v26), linA);
					linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v27, v27, 255), v26));
					__m128 v28 = _mm_mul_ps(a10, v25);
					__m128 v29 = _mm_mul_ps(a11, v27);
					v27[0] = elemTemp[1].m_deltaRhs;
					angA = _mm_add_ps(v28, angA);
					angB = _mm_add_ps(v29, angB);
					elemTemp[1].m_deltaRhs = v27[0] + v296;
				}
			}
			if (bFirstOnly) {
				// The first point alone, clamped to a non-negative accumulated impulse.
				__m128 v30 = _mm_set_ss(v395);
				__m128 m_quad_16 = hkQuadReal0000;
				v30[0] = v395 * _mm_shuffle_ps(a00, a00, 255)[0];
				m_quad_16[0] = 0.0f - elemTemp->m_deltaRhs;
				__m128 v32 = _mm_max_ps(m_quad_16, v30);
				__m128 v33 = _mm_shuffle_ps(v32, v32, 0);
				__m128 v34 = _mm_mul_ps(v33, invA);
				__m128 v35 = _mm_mul_ps(v33, invB);
				__m128 m_quad_17 = l0;
				linA = _mm_add_ps(linA, _mm_mul_ps(l0, _mm_shuffle_ps(v34, v34, 255)));
				linB = _mm_sub_ps(linB, _mm_mul_ps(m_quad_17, _mm_shuffle_ps(v35, v35, 255)));
				__m128 v37 = _mm_mul_ps(a01, v35);
				v35[0] = elemTemp->m_deltaRhs;
				angA = _mm_add_ps(_mm_mul_ps(v34, a00), angA);
				angB = _mm_add_ps(v37, angB);
				elemTemp->m_deltaRhs = v35[0] + v32[0];
			}
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			elemTemp += 2;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_2D_FRICTION: {
			hkpJacobian2dFrictionSchema* pS = reinterpret_cast<hkpJacobian2dFrictionSchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
			const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
			const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
			const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
			double v138;
			__m128 v136 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(a00, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
			float v393 = _mm_shuffle_ps(l0, l0, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v136, v136, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v136, v136, 85)[0] + v136[0]));
			__m128 v137 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
				_mm_mul_ps(l1, _mm_sub_ps(linA, linB)));
			float v385 = _mm_shuffle_ps(l1, l1, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v137, v137, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v137, v137, 85)[0] + v137[0]));
			float v366 = pS->m_maxImpulse * pS->m_maxImpulse;
			float v357 = a01[3] * v393 + v385 * pS->m_invJac01;
			float v302 = v393 * pS->m_invJac01 + v385 * a11[3];
			float v326 = v302 * v302 + v357 * v357;
			if (!(v366 < static_cast<double>(v326))) {
				v138 = 1.0;
			} else {
				float v327 = v366 / v326;
				float v328 = std::sqrt(static_cast<double>(v327));
				v138 = v328;
				v357 = v328 * v357;
				v302 = v328 * v302;
			}
			elemTemp[2].m_deltaRhs = v138;
			__m128 v139 = _mm_shuffle_ps(_mm_set_ss(v357), _mm_set_ss(v357), 0);
			__m128 v140 = _mm_mul_ps(invB, v139);
			__m128 v141 = _mm_mul_ps(invA, v139);
			__m128 v142 = _mm_mul_ps(_mm_shuffle_ps(v140, v140, 255), l0);
			linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v141, v141, 255), l0));
			linB = _mm_sub_ps(linB, v142);
			__m128 v143 = _mm_add_ps(angA, _mm_mul_ps(a00, v141));
			__m128 v144 = _mm_mul_ps(a01, v140);
			v140[0] = elemTemp->m_deltaRhs;
			angA = v143;
			angB = _mm_add_ps(angB, v144);
			elemTemp->m_deltaRhs = v140[0] + v357;
			__m128 v145 = l1;
			__m128 v146 = _mm_shuffle_ps(_mm_set_ss(v302), _mm_set_ss(v302), 0);
			__m128 v147 = _mm_mul_ps(invA, v146);
			__m128 v148 = _mm_mul_ps(invB, v146);
			linA = _mm_add_ps(_mm_mul_ps(v145, _mm_shuffle_ps(v147, v147, 255)), linA);
			linB = _mm_sub_ps(linB, _mm_mul_ps(v145, _mm_shuffle_ps(v148, v148, 255)));
			__m128 v149 = _mm_mul_ps(a10, v147);
			__m128 v150 = _mm_mul_ps(a11, v148);
			v148[0] = elemTemp[1].m_deltaRhs;
			angA = _mm_add_ps(v149, angA);
			angB = _mm_add_ps(angB, v150);
			elemTemp[1].m_deltaRhs = v148[0] + v302;
			elemTemp += 3;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_3D_FRICTION: {
			hkpJacobian3dFrictionSchema* pS = reinterpret_cast<hkpJacobian3dFrictionSchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
			const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
			const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
			const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
			const __m128& a20 = pS->m_jac.m_jac2.m_angular[0].m_quad;
			const __m128& a21 = pS->m_jac.m_jac2.m_angular[1].m_quad;
			double v118;
			__m128 v113 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(a00, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
			float v381 = _mm_shuffle_ps(l0, l0, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v113, v113, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v113, v113, 85)[0] + v113[0]));
			__m128 v114 = l1;
			__m128 v115 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), v114));
			float v389 = _mm_shuffle_ps(v114, v114, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v115, v115, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v115, v115, 85)[0] + v115[0]));
			__m128 v116 = _mm_add_ps(_mm_mul_ps(a21, angB), _mm_mul_ps(a20, angA));
			float v383 = _mm_shuffle_ps(a21, a21, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v116, v116, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v116, v116, 85)[0] + v116[0]));
			float v355 = v383 * a20[3];
			float v365 = a01[3] * v381 + pS->m_invJac01 * v389;
			float v301 = v381 * pS->m_invJac01 + v389 * a11[3];
			float v321 = v301 * v301 + v365 * v365 + v355 * v355;
			double v117 = v321;
			float v322 = pS->m_maxImpulse * pS->m_maxImpulse;
			if (!(v322 < v117)) {
				v118 = 1.0;
			} else {
				float v323 = std::sqrt(v117);
				float v324 = 1.0 / v323;
				float v325 = v324 * pS->m_maxImpulse;
				v118 = v325;
				v365 = v325 * v365;
				v301 = v325 * v301;
				v355 = v325 * v355;
			}
			elemTemp[3].m_deltaRhs = v118;
			__m128 m_quad_19 = l0;
			float v356 = pS->m_contactRadius * v355;
			__m128 v120 = _mm_shuffle_ps(_mm_set_ss(v365), _mm_set_ss(v365), 0);
			__m128 v121 = _mm_mul_ps(v120, invA);
			__m128 v122 = _mm_mul_ps(v120, invB);
			linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v121, v121, 255), l0), linA);
			linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v122, v122, 255), m_quad_19));
			__m128 v123 = _mm_mul_ps(v122, a01);
			float fImpulseApplied = elemTemp->m_deltaRhs;
			angA = _mm_add_ps(_mm_mul_ps(v121, a00), angA);
			angB = _mm_add_ps(v123, angB);
			elemTemp->m_deltaRhs = fImpulseApplied + v365;
			__m128 v125 = l1;
			__m128 v126 = _mm_shuffle_ps(_mm_set_ss(v301), _mm_set_ss(v301), 0);
			__m128 v127 = _mm_mul_ps(v126, invA);
			__m128 v128 = _mm_mul_ps(v126, invB);
			linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v127, v127, 255), v125), linA);
			linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v128, v128, 255), v125));
			__m128 v129 = _mm_mul_ps(a10, v127);
			__m128 v130 = _mm_mul_ps(a11, v128);
			v128[0] = elemTemp[1].m_deltaRhs;
			angA = _mm_add_ps(v129, angA);
			angB = _mm_add_ps(v130, angB);
			elemTemp[1].m_deltaRhs = v128[0] + v301;
			__m128 v131 = _mm_shuffle_ps(_mm_set_ss(v356), _mm_set_ss(v356), 0);
			__m128 v132 = _mm_mul_ps(v131, invA);
			__m128 v133 = _mm_mul_ps(a21, _mm_mul_ps(v131, invB));
			v131[0] = elemTemp[2].m_deltaRhs;
			angA = _mm_add_ps(_mm_mul_ps(v132, a20), angA);
			angB = _mm_add_ps(v133, angB);
			elemTemp[2].m_deltaRhs = v131[0] + v356;
			elemTemp += 4;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_2D_ROLLING_FRICTION: {
			hkpJacobian2dRollingFrictionSchema* pS = reinterpret_cast<hkpJacobian2dRollingFrictionSchema*>(pSchema);
			const __m128& a00 = pS->m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac0.m_angular[1].m_quad;
			const __m128& a10 = pS->m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac1.m_angular[1].m_quad;
			double v104;
			float v318 = pS->m_maxImpulse * pS->m_maxImpulse;
			__m128 v101 = _mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(angA, a00));
			double v102 = a00[3];
			float v387 = _mm_shuffle_ps(a01, a01, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v101, v101, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v101, v101, 85)[0] + v101[0]));
			float v300 = v102 * v387;
			__m128 v103 = _mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA));
			float v379 = _mm_shuffle_ps(a11, a11, 255)[0] -
				static_cast<float>(_mm_shuffle_ps(v103, v103, 170)[0] +
					static_cast<float>(_mm_shuffle_ps(v103, v103, 85)[0] + v103[0]));
			float v354 = a10[3] * v379;
			float v364 = v354 * v354 + v300 * v300;
			if (!(v318 < static_cast<double>(v364))) {
				v104 = 1.0;
			} else {
				float v319 = v318 / v364;
				float v320 = std::sqrt(static_cast<double>(v319));
				v104 = v320;
				v300 = v320 * v300;
				v354 = v320 * v354;
			}
			elemTemp[2].m_deltaRhs = v104;
			__m128 v105 = _mm_shuffle_ps(_mm_set_ss(v300), _mm_set_ss(v300), 0);
			__m128 v106 = _mm_mul_ps(v105, invA);
			__m128 v107 = _mm_mul_ps(a01, _mm_mul_ps(v105, invB));
			v105[0] = elemTemp->m_deltaRhs;
			angA = _mm_add_ps(angA, _mm_mul_ps(v106, a00));
			angB = _mm_add_ps(angB, v107);
			elemTemp->m_deltaRhs = v105[0] + v300;
			__m128 v108 = _mm_shuffle_ps(_mm_set_ss(v354), _mm_set_ss(v354), 0);
			__m128 v109 = _mm_mul_ps(v108, invA);
			__m128 v110 = _mm_mul_ps(a11, _mm_mul_ps(v108, invB));
			v108[0] = elemTemp[1].m_deltaRhs;
			angA = _mm_add_ps(_mm_mul_ps(v109, a10), angA);
			angB = _mm_add_ps(angB, v110);
			elemTemp[1].m_deltaRhs = v108[0] + v354;
			elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_MASS: {
			hkpJacobianSetMassSchema* pS = reinterpret_cast<hkpJacobianSetMassSchema*>(pSchema);
			invA = pS->m_invMass0.m_quad;
			invB = pS->m_invMass1.m_quad;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_ADD_VELOCITY: {
			hkpJacobianAddVelocitySchema* pS = reinterpret_cast<hkpJacobianAddVelocitySchema*>(pSchema);
			// Body A or B by m_bodyIndex (0 or 1), picked without a branch as the game does.
			const uintptr_t uAccA = reinterpret_cast<uintptr_t>(pAccA);
			const uintptr_t uMask = static_cast<uintptr_t>(-static_cast<intptr_t>(pS->m_bodyIndex));
			hkpVelocityAccumulator* pAcc = reinterpret_cast<hkpVelocityAccumulator*>(uAccA ^
				(uMask & (reinterpret_cast<uintptr_t>(pAccB) ^ uAccA)));
			float x = info.m_integrateVelocityFactor.m_quad[0u] * static_cast<double>(stepTemp.m_currentStep.m_storage);
			hkSimdReal sFactor(x);
			__m128 m_quad_20 = pS->m_deltaVel.m_quad;
			__m128 m_real = sFactor.m_real;
			pAcc->m_linearVel.m_quad = _mm_add_ps(pAcc->m_linearVel.m_quad, pS->m_deltaVel.m_quad);
			__m128 v292 = _mm_mul_ps(_mm_shuffle_ps(m_real, m_real, 0), m_quad_20);
			pAcc->m_linearVel.m_quad = _mm_add_ps(v292, pAcc->m_linearVel.m_quad);
			pAcc->m_scratch0.m_quad = _mm_add_ps(v292, pAcc->m_scratch0.m_quad);
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_CENTER_OF_MASS: {
			hkpJacobianSetCenterOfMassSchema* pS = reinterpret_cast<hkpJacobianSetCenterOfMassSchema*>(pSchema);
			hkVector4 linVelA;
			hkVector4 linVelB;
			hkVector4 scratchA;
			hkVector4 scratchB;
			linVelA.setMul3(pS->m_angToLinVelA, pAccA->m_angularVel);
			linVelB.setMul3(pS->m_angToLinVelB, pAccB->m_angularVel);
			scratchA.setMul3(pS->m_angToLinVelA, pAccA->m_scratch123.m_col0);
			scratchB.setMul3(pS->m_angToLinVelB, pAccB->m_scratch123.m_col0);
			linA = _mm_add_ps(linVelA.m_quad, linA);
			s0A = _mm_add_ps(s0A, scratchA.m_quad);
			linB = _mm_add_ps(linB, linVelB.m_quad);
			s0B = _mm_add_ps(scratchB.m_quad, s0B);
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_STIFF_SPRING_CHAIN: {
			hkpJacobianStiffSpringChainSchema* pS = &pSchema->m_stiffSpringChain;
			hkSolver_solveStiffSpringChain(info, stepTemp, accumulators, pS, elemTemp);
			elemTemp += pS->m_numConstraints;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_BALL_SOCKET_CHAIN: {
			hkpJacobianBallSocketChainSchema* pS = &pSchema->m_ballSocketChain;
			hkSolver_solveBallAndSocketChain(info, stepTemp, accumulators, pS, elemTemp);
			elemTemp += 3 * pS->m_numConstraints;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_POWERED_CHAIN: {
			hkpJacobianPoweredChainSchema* pS = reinterpret_cast<hkpJacobianPoweredChainSchema*>(pSchema);
			hkSolver_solvePoweredChain(info, stepTemp, accumulators, pS, elemTemp);
			elemTemp += 6 * pS->m_numConstraints;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
			continue;
		}
		default: {
			// Assert on an unknown schema type: trap, then return.
			__builtin_debugtrap();
			return 1;
		}
		}
	}
}

void hkDebugPrintfAccumulators() {
	;
}

// The solver's whole solve of one island (hkpSolve.h; the library's source is not shipped): one substep of gravity on
// the bodies, then for each step the schema stream solved m_numMicroSteps times and the solved velocities integrated.
// The gravity and integration loops have no end of their own, so only the HK_END accumulator stops them; the schema
// loop is hkSolveConstraintsByTheSteps's. The schema stream is raw memory read through each schema's class, each schema
// solved inline on the two bodies of the last header schema. Returns 0 when hkpProcessFlyingColors leaves its flag
// clear, else 1.
int32_t hkSolveConstraints(const hkpSolverInfo& info, hkpJacobianSchema* schemas, hkpVelocityAccumulator* accumulators,
	hkpSolverElemTemp* temp) {
	if (!HK_flyingcolors_solver) {
		hkpProcessFlyingColors(&HK_flyingcolors_solver);
		if (!HK_flyingcolors_solver) {
			return 0;
		}
	}
	const __m128 gravity = info.m_globalAccelerationPerSubStep.m_quad;

	// One substep of gravity on the rigid bodies, the scratch velocities of every body cleared.
	hkpVelocityAccumulator* accum = accumulators;
	while (reinterpret_cast<uintptr_t>(accum) < UINTPTR_MAX) {
		switch (accum->m_type.m_storage) {
		case hkpVelocityAccumulator::HK_RIGID_BODY:
			// Consecutive rigid bodies run in a loop of their own, which reads the next one's type before it tests the
			// end.
			do {
				const __m128 gravityStep = _mm_mul_ps(_mm_set1_ps(accum->m_gravityFactor), gravity);
				accum->m_scratch0.m_quad = _mm_setzero_ps();
				accum->m_scratch123.m_col0.m_quad = _mm_setzero_ps();
				accum->m_linearVel.m_quad = _mm_add_ps(accum->m_linearVel.m_quad, gravityStep);
				++accum;
			} while (accum->m_type.m_storage == hkpVelocityAccumulator::HK_RIGID_BODY &&
				reinterpret_cast<uintptr_t>(accum) < UINTPTR_MAX);
			continue;
		case hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY:
		case hkpVelocityAccumulator::HK_NO_GRAVITY_RIGID_BODY:
			accum->m_scratch0.m_quad = _mm_setzero_ps();
			accum->m_scratch123.m_col0.m_quad = _mm_setzero_ps();
			++accum;
			continue;
		case hkpVelocityAccumulator::HK_END:
			break;
		default:
			// Breaks here and then tests the end again without moving on, as the game does.
			__builtin_debugtrap();
			continue;
		}
		break;
	}

	// x87: the product rounded to a float.
	const float factor = info.m_invNumSteps * info.m_invIntegrateVelocityFactor.m_quad[0];
	const __m128 factorV = _mm_shuffle_ps(_mm_set_ss(factor), _mm_set_ss(factor), 0);
	for (int32_t currentStep = 0; currentStep < info.m_numSteps; ++currentStep) {
		hkSolver::SolverStepTemp stepTemp;
		stepTemp.m_intregratePositions.m_storage = 1.0f;
		stepTemp.m_currentStep.m_storage = currentStep;
		int32_t numMicroSteps = info.m_numMicroSteps;
		do {
			hkpJacobianSchema* pSchema = schemas;
			hkpSolverElemTemp* elemTemp = temp;
			if (pSchema->m_bytes.m_type != hkpJacobianSchema::SCHEMA_TYPE_END) {
				// Reads the first schema as a header whatever its type.
				hkpVelocityAccumulator* pAccA = &accumulators[pSchema->getHeader().m_bodyAIndex];
				hkpVelocityAccumulator* pAccB = &accumulators[pSchema->getHeader().m_bodyBIndex];
				pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
				bool bSchemasDone = false;
				while (!bSchemasDone) {
					__m128& linA = pAccA->m_linearVel.m_quad;
					__m128& angA = pAccA->m_angularVel.m_quad;
					__m128& invA = pAccA->m_invMasses.m_quad;
					__m128& s0A = pAccA->m_scratch0.m_quad;
					__m128& s1A = pAccA->m_scratch123.m_col0.m_quad;
					__m128& linB = pAccB->m_linearVel.m_quad;
					__m128& angB = pAccB->m_angularVel.m_quad;
					__m128& invB = pAccB->m_invMasses.m_quad;
					__m128& s0B = pAccB->m_scratch0.m_quad;
					__m128& s1B = pAccB->m_scratch123.m_col0.m_quad;
					// Each dot product below adds y to x, then z to that sum.
					switch (pSchema->m_bytes.m_type) {
					default:
						// An unknown schema type is asserted on (a debug trap), then treated as the end of the stream.
						__builtin_debugtrap();
						[[fallthrough]];
					case hkpJacobianSchema::SCHEMA_TYPE_END:
						bSchemasDone = true;
						continue;
					case hkpJacobianSchema::SCHEMA_TYPE_HEADER:
						pAccA = &accumulators[pSchema->getHeader().m_bodyAIndex];
						pAccB = &accumulators[pSchema->getHeader().m_bodyBIndex];
						pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
						continue;
					case hkpJacobianSchema::SCHEMA_TYPE_GOTO: {
						hkpJacobianGotoSchema* pS = reinterpret_cast<hkpJacobianGotoSchema*>(pSchema);
						// The offset is added to the cursor as a signed value; a backward jump is a negative one.
						pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) +
							static_cast<int32_t>(pS->m_offset));
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER:
						++pSchema;
						continue;
					case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL: {
						uint8_t uType;
						do {
							hkpJacobian1dBilateralSchema* pS = reinterpret_cast<hkpJacobian1dBilateralSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)),
								_mm_mul_ps(l0, _mm_sub_ps(linA, linB)));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const __m128 impulse =
								_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot), _mm_shuffle_ps(a0, a0, 0xFF));
							const __m128 impulse4 = _mm_shuffle_ps(impulse, impulse, 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							linA = _mm_add_ps(linA, _mm_mul_ps(l0, _mm_shuffle_ps(impA, impA, 0xFF)));
							linB = _mm_sub_ps(linB, _mm_mul_ps(l0, _mm_shuffle_ps(impB, impB, 0xFF)));
							const __m128 dAngA = _mm_mul_ps(a0, impA);
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(angA, dAngA);
							angB = _mm_add_ps(angB, dAngB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, impulse));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
							hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT: {
						uint8_t uType;
						do {
							// The impulse limit is in the w of the second angular row; the schema has the bilateral's
							// size.
							hkpJacobian1dBilateralSchema* pS = reinterpret_cast<hkpJacobian1dBilateralSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)),
								_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const __m128 impulse =
								_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot), _mm_shuffle_ps(a0, a0, 0xFF));
							const __m128 maxImpulse = _mm_shuffle_ps(a1, a1, 0xFF);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							const __m128 lower = _mm_sub_ps(_mm_sub_ps(hkQuadReal0000, maxImpulse), applied);
							const __m128 upper = _mm_sub_ps(maxImpulse, applied);
							const __m128 clamped = _mm_min_ps(upper, _mm_max_ps(lower, impulse));
							const __m128 impulse4 = _mm_shuffle_ps(clamped, clamped, 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0));
							linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0));
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							const __m128 appliedNow = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(_mm_mul_ps(impA, a0), angA);
							angB = _mm_add_ps(angB, dAngB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(appliedNow, clamped));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
							hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU: {
						uint8_t uType;
						do {
							hkpJacobian1dBilateralUserTauSchema* pS =
								reinterpret_cast<hkpJacobian1dBilateralUserTauSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const float tau = pS->m_tau;
							const float rhs = l0[3];
							const __m128 damping = _mm_set_ss(pS->m_damping);
							const __m128 linDiff = _mm_sub_ps(_mm_sub_ps(linA, s0A), _mm_sub_ps(linB, s0B));
							const __m128 angDiffA = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
							const __m128 angDiffB = _mm_mul_ps(_mm_sub_ps(angB, s1B), a1);
							// x87: tau times the inverse integration factor, rounded to a float.
							const float tauFactor = info.m_invIntegrateVelocityFactor.m_quad[0] * tau;
							const __m128 sum =
								_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(damping, damping, 0),
											   _mm_add_ps(_mm_add_ps(angDiffB, angDiffA), _mm_mul_ps(l0, linDiff))),
									_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(tauFactor), _mm_set_ss(tauFactor), 0),
										_mm_add_ps(_mm_add_ps(_mm_mul_ps(s1B, a1), _mm_mul_ps(s1A, a0)),
											_mm_mul_ps(l0, _mm_sub_ps(s0A, s0B)))));
							const float dot = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum)));
							// x87: tau times the rhs, less the dot product.
							const float rel = static_cast<double>(tau) * rhs - dot;
							const __m128 impulse = _mm_mul_ps(_mm_set_ss(rel), _mm_shuffle_ps(a0, a0, 0xFF));
							const __m128 impulse4 = _mm_shuffle_ps(impulse, impulse, 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0));
							linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0));
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(angA, _mm_mul_ps(impA, a0));
							angB = _mm_add_ps(angB, dAngB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, impulse));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
							hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS: {
						uint8_t uType;
						do {
							hkpJacobian1dLinearLimitsSchema* pS =
								reinterpret_cast<hkpJacobian1dLinearLimitsSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(linA, linB), l0),
								_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const float rel = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot));
							const float negApplied = -elemTemp->m_deltaRhs;
							// x87: each limit's violation, then its impulse, rounded to a float.
							const float minViolation = rel - pS->m_minRhs;
							const float minImpulse = minViolation * a0[3];
							if (negApplied < static_cast<double>(minImpulse)) {
								const __m128 impulse4 =
									_mm_shuffle_ps(_mm_set_ss(minImpulse), _mm_set_ss(minImpulse), 0);
								const __m128 impA = _mm_mul_ps(invA, impulse4);
								const __m128 impB = _mm_mul_ps(invB, impulse4);
								const __m128 dLinB = _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0);
								linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0), linA);
								linB = _mm_sub_ps(linB, dLinB);
								const __m128 dAngB = _mm_mul_ps(impB, a1);
								const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
								angA = _mm_add_ps(_mm_mul_ps(impA, a0), angA);
								angB = _mm_add_ps(angB, dAngB);
								_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(minImpulse)));
							}
							const float maxViolation = rel - pS->m_maxRhs;
							const float maxImpulse = maxViolation * a0[3];
							if (maxImpulse < static_cast<double>(negApplied)) {
								const __m128 impulse4 =
									_mm_shuffle_ps(_mm_set_ss(maxImpulse), _mm_set_ss(maxImpulse), 0);
								const __m128 impB = _mm_mul_ps(invB, impulse4);
								const __m128 impA = _mm_mul_ps(invA, impulse4);
								const __m128 dLinB = _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0);
								linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0));
								linB = _mm_sub_ps(linB, dLinB);
								const __m128 dAngB = _mm_mul_ps(impB, a1);
								const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
								angA = _mm_add_ps(_mm_mul_ps(impA, a0), angA);
								angB = _mm_add_ps(dAngB, angB);
								_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(maxImpulse)));
							}
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
							hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION: {
						uint8_t uType;
						do {
							hkpJacobian1dFrictionSchema* pS = reinterpret_cast<hkpJacobian1dFrictionSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)),
								_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const float rel = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot));
							// x87 from here to the stores: the impulse, scaled down to the maximum impulse.
							float impulse = a0[3] * rel;
							const float absImpulse = std::fabs(static_cast<double>(impulse));
							double usedFactor;
							if (!(pS->m_maxImpulse < static_cast<double>(absImpulse))) {
								usedFactor = 1.0;
							} else {
								const float scale = pS->m_maxImpulse / absImpulse;
								usedFactor = scale;
								impulse = impulse * scale;
							}
							elemTemp[1].m_usedImpulseFactor = usedFactor;
							const __m128 impulse4 = _mm_shuffle_ps(_mm_set_ss(impulse), _mm_set_ss(impulse), 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							const __m128 dLinB = _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0);
							linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0));
							linB = _mm_sub_ps(linB, dLinB);
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(_mm_mul_ps(impA, a0), angA);
							angB = _mm_add_ps(angB, dAngB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(impulse)));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
							hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR: {
						uint8_t uType;
						do {
							hkpJacobian1dLinearMotorSchema* pS =
								reinterpret_cast<hkpJacobian1dLinearMotorSchema*>(pSchema);
							const hkp1dMotorSolverInfo& motor = pS->m_motorSolverInfo;
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							// x87: the target moves on by the velocity when the step integrates positions.
							const float target =
								motor.m_velocity * stepTemp.m_intregratePositions.m_storage + elemTemp[2].m_deltaRhs;
							elemTemp[2].m_deltaRhs = target;
							const float damping = motor.m_damping;
							const float rhs = l0[3] + target;
							const float tau = motor.m_tau;
							const __m128 linDiff = _mm_sub_ps(_mm_sub_ps(linA, s0A), _mm_sub_ps(linB, s0B));
							const __m128 angDiffA = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
							const __m128 angDiffB = _mm_mul_ps(_mm_sub_ps(angB, s1B), a1);
							const __m128 velSum = _mm_add_ps(_mm_add_ps(angDiffB, angDiffA), _mm_mul_ps(linDiff, l0));
							const float tauFactor = tau * info.m_invIntegrateVelocityFactor.m_quad[0];
							const __m128 posSum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(s1B, a1), _mm_mul_ps(s1A, a0)),
								_mm_mul_ps(_mm_sub_ps(s0A, s0B), l0));
							const __m128 posTerm =
								_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(tauFactor), _mm_set_ss(tauFactor), 0), posSum);
							const __m128 sum =
								_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(damping), _mm_set_ss(damping), 0),
											   velSum),
									posTerm);
							const float dot = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum)));
							// x87 from here to the stores: the impulse clamped to the motor's range.
							const float rel = static_cast<double>(tau) * rhs - dot;
							float impulse = a0[3] * rel;
							const double impulseD = impulse;
							if (!(motor.m_maxImpulsePerSubstep < static_cast<double>(impulse))) {
								if (!(motor.m_minImpulsePerSubstep > impulseD)) {
									elemTemp[1].m_usedImpulseFactor = 1.0f;
								} else {
									elemTemp[1].m_usedImpulseFactor = motor.m_minImpulsePerSubstep / impulseD;
									impulse = motor.m_minImpulsePerSubstep;
								}
							} else {
								elemTemp[1].m_usedImpulseFactor = motor.m_maxImpulsePerSubstep / impulseD;
								impulse = motor.m_maxImpulsePerSubstep;
							}
							hkSimdReal sImpulse;
							sImpulse.m_real = _mm_set_ss(impulse);
							const __m128 impulse4 = _mm_shuffle_ps(sImpulse.m_real, sImpulse.m_real, 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							linA = _mm_add_ps(linA, _mm_mul_ps(l0, _mm_shuffle_ps(impA, impA, 0xFF)));
							linB = _mm_sub_ps(linB, _mm_mul_ps(l0, _mm_shuffle_ps(impB, impB, 0xFF)));
							const __m128 dAngA = _mm_mul_ps(impA, a0);
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							hkSimdReal sApplied;
							sApplied.m_real = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(dAngA, angA);
							angB = _mm_add_ps(angB, dAngB);
							const hkSimdReal sSum = sApplied + sImpulse;
							_mm_store_ss(&elemTemp->m_deltaRhs, sSum.m_real);
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
							hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY: {
						hkpJacobian1dPulleySchema* pS = reinterpret_cast<hkpJacobian1dPulleySchema*>(pSchema);
						const __m128& l0 = pS->m_jac.m_linear[0].m_quad;
						const __m128& l1 = pS->m_jac.m_linear[1].m_quad;
						const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
						const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
						const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)),
							_mm_sub_ps(_mm_mul_ps(l0, linA), _mm_mul_ps(l1, linB)));
						// The upper lanes deliberately hold z, which operator- reads: the sum goes into lane 0 of the z
						// splat.
						hkSimdReal sDot;
						const __m128 dotZ = _mm_shuffle_ps(sum, sum, 0xAA);
						sDot.m_real =
							_mm_move_ss(dotZ, _mm_add_ps(dotZ, _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum)));
						hkSimdReal sRhs;
						sRhs.m_real = _mm_shuffle_ps(l0, l0, 0xFF);
						const hkSimdReal sDelta = sRhs - sDot;
						hkSimdReal sInvDiag;
						sInvDiag.m_real = _mm_shuffle_ps(a0, a0, 0xFF);
						const hkSimdReal sImpulse = sDelta * sInvDiag;
						const __m128 impulse4 = _mm_shuffle_ps(sImpulse.m_real, sImpulse.m_real, 0);
						const __m128 impA = _mm_mul_ps(invA, impulse4);
						const __m128 impB = _mm_mul_ps(invB, impulse4);
						const __m128 dLinB = _mm_mul_ps(l1, _mm_shuffle_ps(impB, impB, 0xFF));
						linA = _mm_add_ps(_mm_mul_ps(l0, _mm_shuffle_ps(impA, impA, 0xFF)), linA);
						linB = _mm_sub_ps(linB, dLinB);
						const __m128 dAngA = _mm_mul_ps(impA, a0);
						const __m128 dAngB = _mm_mul_ps(a1, impB);
						hkSimdReal sApplied;
						sApplied.m_real = _mm_load_ss(&elemTemp->m_deltaRhs);
						angA = _mm_add_ps(angA, dAngA);
						angB = _mm_add_ps(angB, dAngB);
						const hkSimdReal sSum = sApplied + sImpulse;
						_mm_store_ss(&elemTemp->m_deltaRhs, sSum.m_real);
						++elemTemp;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR: {
						uint8_t uNextType;
						do {
							hkpJacobian1dAngularSchema* pS = reinterpret_cast<hkpJacobian1dAngularSchema*>(pSchema);
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const __m128 impulse =
								_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(a1, a1, 0xFF), dot), _mm_shuffle_ps(a0, a0, 0xFF));
							const __m128 impulse4 = _mm_shuffle_ps(impulse, impulse, 0);
							const __m128 impA = _mm_mul_ps(impulse4, invA);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(angA, _mm_mul_ps(a0, impA));
							angB = _mm_add_ps(angB, _mm_mul_ps(a1, impB));
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, impulse));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS: {
						uint8_t uNextType;
						do {
							hkpJacobian1dAngularLimitsSchema* pS =
								reinterpret_cast<hkpJacobian1dAngularLimitsSchema*>(pSchema);
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const float rhs = a1[3];
							const double tau = pS->m_tau;
							const __m128 damping = _mm_set_ss(info.m_damping);
							const __m128 angDiffB = _mm_sub_ps(angB, s1B);
							const __m128 angDiffA = _mm_mul_ps(a0, _mm_sub_ps(angA, s1A));
							// x87: tau times the inverse integration factor, rounded to a float.
							const float tauFactor = tau * info.m_invIntegrateVelocityFactor.m_quad[0];
							const __m128 sum = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(damping, damping, 0),
															  _mm_add_ps(_mm_mul_ps(angDiffB, a1), angDiffA)),
								_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(tauFactor), _mm_set_ss(tauFactor), 0),
									_mm_add_ps(_mm_mul_ps(s1B, a1), _mm_mul_ps(a0, s1A))));
							const float dot = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum)));
							// x87 from here to the stores: each limit's violation, then its impulse.
							const float rel = tau * rhs - dot;
							const float negApplied = -elemTemp->m_deltaRhs;
							const float minViolation = rel - pS->m_minRhs;
							const float minImpulse = minViolation * a0[3];
							if (!(negApplied < static_cast<double>(minImpulse))) {
								const float maxViolation = rel - pS->m_maxRhs;
								const float maxImpulse = maxViolation * a0[3];
								if (maxImpulse < static_cast<double>(negApplied)) {
									const __m128 impulse4 =
										_mm_shuffle_ps(_mm_set_ss(maxImpulse), _mm_set_ss(maxImpulse), 0);
									const __m128 dAngA = _mm_mul_ps(_mm_mul_ps(invA, impulse4), a0);
									const __m128 dAngB = _mm_mul_ps(_mm_mul_ps(invB, impulse4), a1);
									const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
									angA = _mm_add_ps(dAngA, angA);
									angB = _mm_add_ps(dAngB, angB);
									_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(maxImpulse)));
								}
							} else {
								const __m128 impulse4 =
									_mm_shuffle_ps(_mm_set_ss(minImpulse), _mm_set_ss(minImpulse), 0);
								const __m128 dAngB = _mm_mul_ps(_mm_mul_ps(invB, impulse4), a1);
								const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
								angA = _mm_add_ps(angA, _mm_mul_ps(a0, _mm_mul_ps(invA, impulse4)));
								angB = _mm_add_ps(dAngB, angB);
								_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(minImpulse)));
							}
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION: {
						uint8_t uNextType;
						do {
							hkpJacobian1dAngularFrictionSchema* pS =
								reinterpret_cast<hkpJacobian1dAngularFrictionSchema*>(pSchema);
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const float rel = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(a1, a1, 0xFF), dot));
							// x87 from here to the stores: the impulse, scaled down to the maximum impulse.
							float impulse = a0[3] * rel;
							const float absImpulse = std::fabs(static_cast<double>(impulse));
							double usedFactor;
							if (!(pS->m_maxImpulse < static_cast<double>(absImpulse))) {
								usedFactor = 1.0;
							} else {
								const float scale = pS->m_maxImpulse / absImpulse;
								usedFactor = scale;
								impulse = impulse * scale;
							}
							elemTemp[1].m_usedImpulseFactor = usedFactor;
							const __m128 impulse4 = _mm_shuffle_ps(_mm_set_ss(impulse), _mm_set_ss(impulse), 0);
							const __m128 dAngB = _mm_mul_ps(_mm_mul_ps(invB, impulse4), a1);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(angA, _mm_mul_ps(a0, _mm_mul_ps(invA, impulse4)));
							angB = _mm_add_ps(dAngB, angB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(impulse)));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR: {
						uint8_t uNextType;
						do {
							hkpJacobian1dAngularMotorSchema* pS =
								reinterpret_cast<hkpJacobian1dAngularMotorSchema*>(pSchema);
							const hkp1dMotorSolverInfo& motor = pS->m_motorSolverInfo;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							// x87: the target moves on by the velocity when the step integrates positions.
							const float target =
								motor.m_velocity * stepTemp.m_intregratePositions.m_storage + elemTemp[2].m_deltaRhs;
							elemTemp[2].m_deltaRhs = target;
							const float damping = motor.m_damping;
							const __m128 angDiffB = _mm_sub_ps(angB, s1B);
							const float rhs = a1[3] + target;
							const __m128 angDiffA = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
							const double tau = motor.m_tau;
							const __m128 velSum = _mm_add_ps(_mm_mul_ps(angDiffB, a1), angDiffA);
							const float tauFactor = tau * info.m_invIntegrateVelocityFactor.m_quad[0];
							const __m128 posSum = _mm_add_ps(_mm_mul_ps(s1B, a1), _mm_mul_ps(s1A, a0));
							const __m128 posTerm =
								_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(tauFactor), _mm_set_ss(tauFactor), 0), posSum);
							const __m128 sum =
								_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(damping), _mm_set_ss(damping), 0),
											   velSum),
									posTerm);
							const float dot = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum)));
							// x87 from here to the stores: the impulse clamped to the motor's range.
							const float rel = tau * rhs - dot;
							float impulse = a0[3] * rel;
							const double impulseD = impulse;
							if (!(motor.m_maxImpulsePerSubstep < static_cast<double>(impulse))) {
								if (!(motor.m_minImpulsePerSubstep > impulseD)) {
									elemTemp[1].m_usedImpulseFactor = 1.0f;
								} else {
									elemTemp[1].m_usedImpulseFactor = motor.m_minImpulsePerSubstep / impulseD;
									impulse = motor.m_minImpulsePerSubstep;
								}
							} else {
								elemTemp[1].m_usedImpulseFactor = motor.m_maxImpulsePerSubstep / impulseD;
								impulse = motor.m_maxImpulsePerSubstep;
							}
							const __m128 impulse4 = _mm_shuffle_ps(_mm_set_ss(impulse), _mm_set_ss(impulse), 0);
							const __m128 impA = _mm_mul_ps(impulse4, invA);
							const __m128 dAngB = _mm_mul_ps(_mm_mul_ps(impulse4, invB), a1);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(_mm_mul_ps(impA, a0), angA);
							angB = _mm_add_ps(dAngB, angB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(impulse)));
							elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
							uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT:
					case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT_WITH_ACCESSOR_CHECK: {
						do {
							hkpJacobianSchema* pCurrent = pSchema;
							hkpJacobianSingleContactSchema* pS =
								reinterpret_cast<hkpJacobianSingleContactSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)),
								_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const __m128 impulse =
								_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot), _mm_shuffle_ps(a0, a0, 0xFF));
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
							// The accumulated impulse stays non-negative.
							const __m128 lower = _mm_sub_ps(hkQuadReal0000, _mm_load_ss(&elemTemp->m_deltaRhs));
							const __m128 clamped = _mm_max_ps(lower, impulse);
							const __m128 impulse4 = _mm_shuffle_ps(clamped, clamped, 0);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0));
							linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0));
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(angA, _mm_mul_ps(impA, a0));
							angB = _mm_add_ps(dAngB, angB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, clamped));
							// EXE-INLINED 0xD921D0
							elemTemp += hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(pCurrent
									->m_bytes.m_type)];
						} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT:
					case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT_WITH_ACCESSOR_CHECK: {
						uint8_t uNextType;
						do {
							// The contact's maximum impulse is in the w of the second angular row.
							hkpJacobianSingleContactSchema* pS =
								reinterpret_cast<hkpJacobianSingleContactSchema*>(pSchema);
							const __m128& l0 = pS->m_jac.m_linear0.m_quad;
							const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
							const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
							const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a1), _mm_mul_ps(angA, a0)),
								_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
							const __m128 dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA),
								_mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum));
							const __m128 impulse =
								_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot), _mm_shuffle_ps(a0, a0, 0xFF));
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							const __m128 lower = _mm_sub_ps(hkQuadReal0000, applied);
							const __m128 upper = _mm_sub_ps(_mm_shuffle_ps(a1, a1, 0xFF), applied);
							const __m128 clamped = _mm_min_ps(upper, _mm_max_ps(lower, impulse));
							const __m128 impulse4 = _mm_shuffle_ps(clamped, clamped, 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							const __m128 dLinB = _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), l0);
							linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), l0), linA);
							linB = _mm_sub_ps(linB, dLinB);
							const __m128 dAngB = _mm_mul_ps(impB, a1);
							const __m128 appliedNow = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(_mm_mul_ps(impA, a0), angA);
							angB = _mm_add_ps(angB, dAngB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(appliedNow, clamped));
							// EXE-INLINED 0xD921D0
							elemTemp += hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(pSchema
									->m_bytes.m_type)];
							uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
							pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT:
					case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT_WITH_ACCESSOR_CHECK: {
						hkpJacobianPairContactSchema* pS = reinterpret_cast<hkpJacobianPairContactSchema*>(pSchema);
						const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
						const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
						const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
						const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
						const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
						const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
						const __m128 sum0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a01), _mm_mul_ps(angA, a00)),
							_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
						const __m128 dot0 = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0x55), sum0));
						const float rel0 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot0));
						const __m128 sum1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
							_mm_mul_ps(_mm_sub_ps(linA, linB), l1));
						const __m128 dot1 = _mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0x55), sum1));
						const float rel1 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l1, l1, 0xFF), dot1));
						// x87: both points' impulses through the pair's inverse 2x2 matrix.
						const float impulse0 = rel1 * pS->m_invJac01 + rel0 * a01[3];
						const float impulse1 = rel0 * pS->m_invJac01 + rel1 * a11[3];
						bool bFirstOnly;
						if (!(-elemTemp->m_deltaRhs < impulse0)) {
							const float impulse1Alone = rel1 * a10[3];
							bFirstOnly = -elemTemp[1].m_deltaRhs >= impulse1Alone;
							if (!bFirstOnly) {
								// The second point alone.
								const __m128 impulse4 =
									_mm_shuffle_ps(_mm_set_ss(impulse1Alone), _mm_set_ss(impulse1Alone), 0);
								const __m128 impB = _mm_mul_ps(invB, impulse4);
								const __m128 impA = _mm_mul_ps(invA, impulse4);
								linA = _mm_add_ps(linA, _mm_mul_ps(l1, _mm_shuffle_ps(impA, impA, 0xFF)));
								linB = _mm_sub_ps(linB, _mm_mul_ps(l1, _mm_shuffle_ps(impB, impB, 0xFF)));
								const __m128 dAngA = _mm_mul_ps(a10, impA);
								const __m128 dAngB = _mm_mul_ps(a11, impB);
								const __m128 applied = _mm_load_ss(&elemTemp[1].m_deltaRhs);
								angA = _mm_add_ps(angA, dAngA);
								angB = _mm_add_ps(angB, dAngB);
								_mm_store_ss(&elemTemp[1].m_deltaRhs, _mm_add_ps(applied, _mm_set_ss(impulse1Alone)));
							}
						} else {
							bFirstOnly = !(-elemTemp[1].m_deltaRhs < impulse1);
							if (!bFirstOnly) {
								// Both points.
								const __m128 impulse04 = _mm_shuffle_ps(_mm_set_ss(impulse0), _mm_set_ss(impulse0), 0);
								const __m128 impA0 = _mm_mul_ps(impulse04, invA);
								const __m128 impB0 = _mm_mul_ps(impulse04, invB);
								const __m128 dLinB0 = _mm_mul_ps(_mm_shuffle_ps(impB0, impB0, 0xFF), l0);
								linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impA0, impA0, 0xFF), l0), linA);
								linB = _mm_sub_ps(linB, dLinB0);
								const __m128 dAngB0 = _mm_mul_ps(impB0, a01);
								const __m128 applied0 = _mm_load_ss(&elemTemp->m_deltaRhs);
								angA = _mm_add_ps(_mm_mul_ps(impA0, a00), angA);
								angB = _mm_add_ps(dAngB0, angB);
								_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied0, _mm_set_ss(impulse0)));
								const __m128 impulse14 = _mm_shuffle_ps(_mm_set_ss(impulse1), _mm_set_ss(impulse1), 0);
								const __m128 impB1 = _mm_mul_ps(invB, impulse14);
								const __m128 impA1 = _mm_mul_ps(invA, impulse14);
								linA = _mm_add_ps(linA, _mm_mul_ps(l1, _mm_shuffle_ps(impA1, impA1, 0xFF)));
								linB = _mm_sub_ps(linB, _mm_mul_ps(l1, _mm_shuffle_ps(impB1, impB1, 0xFF)));
								const __m128 dAngA1 = _mm_mul_ps(a10, impA1);
								const __m128 dAngB1 = _mm_mul_ps(a11, impB1);
								const __m128 applied1 = _mm_load_ss(&elemTemp[1].m_deltaRhs);
								angA = _mm_add_ps(angA, dAngA1);
								angB = _mm_add_ps(angB, dAngB1);
								_mm_store_ss(&elemTemp[1].m_deltaRhs, _mm_add_ps(applied1, _mm_set_ss(impulse1)));
							}
						}
						if (bFirstOnly) {
							// The first point alone, clamped to a non-negative accumulated impulse.
							const __m128 impulse = _mm_mul_ps(_mm_set_ss(rel0), _mm_shuffle_ps(a00, a00, 0xFF));
							const __m128 lower = _mm_sub_ps(hkQuadReal0000, _mm_load_ss(&elemTemp->m_deltaRhs));
							const __m128 clamped = _mm_max_ps(lower, impulse);
							const __m128 impulse4 = _mm_shuffle_ps(clamped, clamped, 0);
							const __m128 impA = _mm_mul_ps(invA, impulse4);
							const __m128 impB = _mm_mul_ps(invB, impulse4);
							linA = _mm_add_ps(_mm_mul_ps(l0, _mm_shuffle_ps(impA, impA, 0xFF)), linA);
							linB = _mm_sub_ps(linB, _mm_mul_ps(l0, _mm_shuffle_ps(impB, impB, 0xFF)));
							const __m128 dAngA = _mm_mul_ps(a00, impA);
							const __m128 dAngB = _mm_mul_ps(impB, a01);
							const __m128 applied = _mm_load_ss(&elemTemp->m_deltaRhs);
							angA = _mm_add_ps(angA, dAngA);
							angB = _mm_add_ps(angB, dAngB);
							_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied, clamped));
						}
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						elemTemp += 2;
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_2D_FRICTION: {
						hkpJacobian2dFrictionSchema* pS = reinterpret_cast<hkpJacobian2dFrictionSchema*>(pSchema);
						const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
						const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
						const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
						const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
						const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
						const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
						const __m128 sum0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a01), _mm_mul_ps(angA, a00)),
							_mm_mul_ps(l0, _mm_sub_ps(linA, linB)));
						const __m128 dot0 = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0x55), sum0));
						const float rel0 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot0));
						const __m128 sum1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
							_mm_mul_ps(l1, _mm_sub_ps(linA, linB)));
						const __m128 dot1 = _mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0x55), sum1));
						const float rel1 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l1, l1, 0xFF), dot1));
						// x87 from here to the stores: both impulses, scaled down together to the maximum impulse.
						const float maxImpulseSqrd = pS->m_maxImpulse * pS->m_maxImpulse;
						float impulse0 = a01[3] * rel0 + rel1 * pS->m_invJac01;
						float impulse1 = rel0 * pS->m_invJac01 + rel1 * a11[3];
						const float impulseSqrd = impulse1 * impulse1 + impulse0 * impulse0;
						double usedFactor;
						if (!(maxImpulseSqrd < static_cast<double>(impulseSqrd))) {
							usedFactor = 1.0;
						} else {
							const float ratio = maxImpulseSqrd / impulseSqrd;
							const float scale = std::sqrt(static_cast<double>(ratio));
							usedFactor = scale;
							impulse0 = impulse0 * scale;
							impulse1 = impulse1 * scale;
						}
						elemTemp[2].m_usedImpulseFactor = usedFactor;
						const __m128 impulse04 = _mm_shuffle_ps(_mm_set_ss(impulse0), _mm_set_ss(impulse0), 0);
						const __m128 impA0 = _mm_mul_ps(invA, impulse04);
						const __m128 impB0 = _mm_mul_ps(invB, impulse04);
						const __m128 dLinB0 = _mm_mul_ps(_mm_shuffle_ps(impB0, impB0, 0xFF), l0);
						linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(impA0, impA0, 0xFF), l0));
						linB = _mm_sub_ps(linB, dLinB0);
						const __m128 newAngA0 = _mm_add_ps(angA, _mm_mul_ps(a00, impA0));
						const __m128 dAngB0 = _mm_mul_ps(impB0, a01);
						const __m128 applied0 = _mm_load_ss(&elemTemp->m_deltaRhs);
						angA = newAngA0;
						angB = _mm_add_ps(angB, dAngB0);
						_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied0, _mm_set_ss(impulse0)));
						const __m128 impulse14 = _mm_shuffle_ps(_mm_set_ss(impulse1), _mm_set_ss(impulse1), 0);
						const __m128 impB1 = _mm_mul_ps(invB, impulse14);
						const __m128 impA1 = _mm_mul_ps(invA, impulse14);
						linA = _mm_add_ps(linA, _mm_mul_ps(l1, _mm_shuffle_ps(impA1, impA1, 0xFF)));
						linB = _mm_sub_ps(linB, _mm_mul_ps(l1, _mm_shuffle_ps(impB1, impB1, 0xFF)));
						const __m128 dAngA1 = _mm_mul_ps(a10, impA1);
						const __m128 dAngB1 = _mm_mul_ps(a11, impB1);
						const __m128 applied1 = _mm_load_ss(&elemTemp[1].m_deltaRhs);
						angA = _mm_add_ps(angA, dAngA1);
						angB = _mm_add_ps(angB, dAngB1);
						_mm_store_ss(&elemTemp[1].m_deltaRhs, _mm_add_ps(applied1, _mm_set_ss(impulse1)));
						elemTemp += 3;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_3D_FRICTION: {
						hkpJacobian3dFrictionSchema* pS = reinterpret_cast<hkpJacobian3dFrictionSchema*>(pSchema);
						const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
						const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
						const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
						const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
						const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
						const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
						const __m128& a20 = pS->m_jac.m_jac2.m_angular[0].m_quad;
						const __m128& a21 = pS->m_jac.m_jac2.m_angular[1].m_quad;
						const __m128 sum0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(angB, a01), _mm_mul_ps(angA, a00)),
							_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
						const __m128 dot0 = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0x55), sum0));
						const float rel0 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 0xFF), dot0));
						const __m128 sum1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
							_mm_mul_ps(l1, _mm_sub_ps(linA, linB)));
						const __m128 dot1 = _mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0x55), sum1));
						const float rel1 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(l1, l1, 0xFF), dot1));
						const __m128 sum2 = _mm_add_ps(_mm_mul_ps(a21, angB), _mm_mul_ps(angA, a20));
						const __m128 dot2 = _mm_add_ps(_mm_shuffle_ps(sum2, sum2, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum2, sum2, 0x55), sum2));
						const float rel2 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(a21, a21, 0xFF), dot2));
						// x87 from here to the stores: the three impulses, scaled down together to the maximum
						// impulse.
						float impulse2 = rel2 * a20[3];
						float impulse0 = a01[3] * rel0 + rel1 * pS->m_invJac01;
						float impulse1 = rel0 * pS->m_invJac01 + rel1 * a11[3];
						const float impulseSqrd = impulse1 * impulse1 + impulse0 * impulse0 + impulse2 * impulse2;
						const double impulseSqrdD = impulseSqrd;
						const float maxImpulseSqrd = pS->m_maxImpulse * pS->m_maxImpulse;
						double usedFactor;
						if (!(maxImpulseSqrd < impulseSqrdD)) {
							usedFactor = 1.0;
						} else {
							const float length = std::sqrt(impulseSqrdD);
							const float invLength = 1.0 / length;
							const float scale = invLength * pS->m_maxImpulse;
							usedFactor = scale;
							impulse0 = scale * impulse0;
							impulse1 = scale * impulse1;
							impulse2 = scale * impulse2;
						}
						elemTemp[3].m_usedImpulseFactor = usedFactor;
						const float angularImpulse = pS->m_contactRadius * impulse2;
						const __m128 impulse04 = _mm_shuffle_ps(_mm_set_ss(impulse0), _mm_set_ss(impulse0), 0);
						const __m128 impA0 = _mm_mul_ps(impulse04, invA);
						const __m128 impB0 = _mm_mul_ps(impulse04, invB);
						linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impA0, impA0, 0xFF), l0), linA);
						linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(impB0, impB0, 0xFF), l0));
						const __m128 dAngB0 = _mm_mul_ps(impB0, a01);
						const __m128 applied0 = _mm_load_ss(&elemTemp->m_deltaRhs);
						angA = _mm_add_ps(_mm_mul_ps(impA0, a00), angA);
						angB = _mm_add_ps(dAngB0, angB);
						_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied0, _mm_set_ss(impulse0)));
						const __m128 impulse14 = _mm_shuffle_ps(_mm_set_ss(impulse1), _mm_set_ss(impulse1), 0);
						const __m128 impA1 = _mm_mul_ps(impulse14, invA);
						const __m128 impB1 = _mm_mul_ps(impulse14, invB);
						linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impA1, impA1, 0xFF), l1), linA);
						linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(impB1, impB1, 0xFF), l1));
						const __m128 dAngA1 = _mm_mul_ps(a10, impA1);
						const __m128 dAngB1 = _mm_mul_ps(a11, impB1);
						const __m128 applied1 = _mm_load_ss(&elemTemp[1].m_deltaRhs);
						angA = _mm_add_ps(dAngA1, angA);
						angB = _mm_add_ps(dAngB1, angB);
						_mm_store_ss(&elemTemp[1].m_deltaRhs, _mm_add_ps(applied1, _mm_set_ss(impulse1)));
						const __m128 impulse24 =
							_mm_shuffle_ps(_mm_set_ss(angularImpulse), _mm_set_ss(angularImpulse), 0);
						const __m128 dAngB2 = _mm_mul_ps(a21, _mm_mul_ps(invB, impulse24));
						const __m128 applied2 = _mm_load_ss(&elemTemp[2].m_deltaRhs);
						angA = _mm_add_ps(angA, _mm_mul_ps(a20, _mm_mul_ps(invA, impulse24)));
						angB = _mm_add_ps(angB, dAngB2);
						_mm_store_ss(&elemTemp[2].m_deltaRhs, _mm_add_ps(applied2, _mm_set_ss(angularImpulse)));
						elemTemp += 4;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_2D_ROLLING_FRICTION: {
						hkpJacobian2dRollingFrictionSchema* pS =
							reinterpret_cast<hkpJacobian2dRollingFrictionSchema*>(pSchema);
						const __m128& a00 = pS->m_jac0.m_angular[0].m_quad;
						const __m128& a01 = pS->m_jac0.m_angular[1].m_quad;
						const __m128& a10 = pS->m_jac1.m_angular[0].m_quad;
						const __m128& a11 = pS->m_jac1.m_angular[1].m_quad;
						// x87 for the scalars from here to the stores: both impulses, scaled down together to the
						// maximum impulse.
						const float maxImpulseSqrd = pS->m_maxImpulse * pS->m_maxImpulse;
						const __m128 sum0 = _mm_add_ps(_mm_mul_ps(angB, a01), _mm_mul_ps(angA, a00));
						const double invDiag0 = a00[3];
						const __m128 dot0 = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0x55), sum0));
						const float rel0 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(a01, a01, 0xFF), dot0));
						float impulse0 = invDiag0 * rel0;
						const __m128 sum1 = _mm_add_ps(_mm_mul_ps(angB, a11), _mm_mul_ps(angA, a10));
						const __m128 dot1 = _mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0xAA),
							_mm_add_ps(_mm_shuffle_ps(sum1, sum1, 0x55), sum1));
						const float rel1 = _mm_cvtss_f32(_mm_sub_ps(_mm_shuffle_ps(a11, a11, 0xFF), dot1));
						float impulse1 = a10[3] * rel1;
						const float impulseSqrd = impulse0 * impulse0 + impulse1 * impulse1;
						double usedFactor;
						if (!(maxImpulseSqrd < static_cast<double>(impulseSqrd))) {
							usedFactor = 1.0;
						} else {
							const float ratio = maxImpulseSqrd / impulseSqrd;
							const float scale = std::sqrt(static_cast<double>(ratio));
							usedFactor = scale;
							impulse0 = scale * impulse0;
							impulse1 = scale * impulse1;
						}
						elemTemp[2].m_usedImpulseFactor = usedFactor;
						const __m128 impulse04 = _mm_shuffle_ps(_mm_set_ss(impulse0), _mm_set_ss(impulse0), 0);
						const __m128 dAngB0 = _mm_mul_ps(_mm_mul_ps(invB, impulse04), a01);
						const __m128 applied0 = _mm_load_ss(&elemTemp->m_deltaRhs);
						angA = _mm_add_ps(angA, _mm_mul_ps(a00, _mm_mul_ps(invA, impulse04)));
						angB = _mm_add_ps(dAngB0, angB);
						_mm_store_ss(&elemTemp->m_deltaRhs, _mm_add_ps(applied0, _mm_set_ss(impulse0)));
						const __m128 impulse14 = _mm_shuffle_ps(_mm_set_ss(impulse1), _mm_set_ss(impulse1), 0);
						const __m128 dAngA1 = _mm_mul_ps(_mm_mul_ps(invA, impulse14), a10);
						const __m128 dAngB1 = _mm_mul_ps(_mm_mul_ps(invB, impulse14), a11);
						const __m128 applied1 = _mm_load_ss(&elemTemp[1].m_deltaRhs);
						angA = _mm_add_ps(dAngA1, angA);
						angB = _mm_add_ps(angB, dAngB1);
						_mm_store_ss(&elemTemp[1].m_deltaRhs, _mm_add_ps(applied1, _mm_set_ss(impulse1)));
						elemTemp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_SET_MASS: {
						hkpJacobianSetMassSchema* pS = reinterpret_cast<hkpJacobianSetMassSchema*>(pSchema);
						invA = pS->m_invMass0.m_quad;
						invB = pS->m_invMass1.m_quad;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_ADD_VELOCITY: {
						hkpJacobianAddVelocitySchema* pS = reinterpret_cast<hkpJacobianAddVelocitySchema*>(pSchema);
						// Body A or B by m_bodyIndex (0 or 1), picked without a branch as the game does.
						const uintptr_t uAccA = reinterpret_cast<uintptr_t>(pAccA);
						const uintptr_t uMask = static_cast<uintptr_t>(-static_cast<intptr_t>(pS->m_bodyIndex));
						hkpVelocityAccumulator* pAcc = reinterpret_cast<hkpVelocityAccumulator*>(uAccA ^
							(uMask & (reinterpret_cast<uintptr_t>(pAccB) ^ uAccA)));
						// x87: the integration factor times the step number, rounded to a float.
						const float x = info.m_integrateVelocityFactor.m_quad[0] *
							static_cast<double>(stepTemp.m_currentStep.m_storage);
						hkSimdReal sFactor(x);
						const __m128 deltaVel = pS->m_deltaVel.m_quad;
						const __m128 posDelta = _mm_mul_ps(deltaVel, _mm_shuffle_ps(sFactor.m_real, sFactor.m_real, 0));
						pAcc->m_linearVel.m_quad = _mm_add_ps(pAcc->m_linearVel.m_quad, deltaVel);
						pAcc->m_linearVel.m_quad = _mm_add_ps(pAcc->m_linearVel.m_quad, posDelta);
						pAcc->m_scratch0.m_quad = _mm_add_ps(pAcc->m_scratch0.m_quad, posDelta);
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_SET_CENTER_OF_MASS: {
						hkpJacobianSetCenterOfMassSchema* pS =
							reinterpret_cast<hkpJacobianSetCenterOfMassSchema*>(pSchema);
						hkVector4 linVelA;
						hkVector4 linVelB;
						hkVector4 scratchA;
						hkVector4 scratchB;
						linVelA.setMul3(pS->m_angToLinVelA, pAccA->m_angularVel);
						linVelB.setMul3(pS->m_angToLinVelB, pAccB->m_angularVel);
						scratchA.setMul3(pS->m_angToLinVelA, pAccA->m_scratch123.m_col0);
						scratchB.setMul3(pS->m_angToLinVelB, pAccB->m_scratch123.m_col0);
						linA = _mm_add_ps(linA, linVelA.m_quad);
						s0A = _mm_add_ps(s0A, scratchA.m_quad);
						linB = _mm_add_ps(linVelB.m_quad, linB);
						s0B = _mm_add_ps(s0B, scratchB.m_quad);
						pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_STIFF_SPRING_CHAIN: {
						hkpJacobianStiffSpringChainSchema* pS = &pSchema->m_stiffSpringChain;
						hkSolver_solveStiffSpringChain(info, stepTemp, accumulators, pS, elemTemp);
						elemTemp += pS->m_numConstraints;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_BALL_SOCKET_CHAIN: {
						hkpJacobianBallSocketChainSchema* pS = &pSchema->m_ballSocketChain;
						hkSolver_solveBallAndSocketChain(info, stepTemp, accumulators, pS, elemTemp);
						elemTemp += 3 * pS->m_numConstraints;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
						continue;
					}
					case hkpJacobianSchema::SCHEMA_TYPE_POWERED_CHAIN: {
						hkpJacobianPoweredChainSchema* pS = reinterpret_cast<hkpJacobianPoweredChainSchema*>(pSchema);
						hkSolver_solvePoweredChain(info, stepTemp, accumulators, pS, elemTemp);
						elemTemp += 6 * pS->m_numConstraints;
						pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
						continue;
					}
					}
				}
			}
			stepTemp.m_intregratePositions.m_storage = 0.0f;
		} while (--numMicroSteps > 0);

		// The solved velocities integrated into the scratch velocities. A rigid or no-gravity body's velocities are
		// first reset when NaN or past the limit in x, y or z, and slowed or stopped below its deactivation class's
		// thresholds; a rigid body adds its gravity. The last step scales the scratch velocities back instead.
		const bool bLastStep = currentStep == info.m_numSteps - 1;
		accum = accumulators;
		while (reinterpret_cast<uintptr_t>(accum) < UINTPTR_MAX) {
			switch (accum->m_type.m_storage) {
			case hkpVelocityAccumulator::HK_RIGID_BODY:
				// Consecutive rigid bodies run in a loop of their own, which reads the next one's type before it tests
				// the end.
				do {
					const hkpSolverInfo::DeactivationInfo& deactivation =
						info.m_deactivationInfo[accum->m_deactivationClass];
					const __m128 linearVel = accum->m_linearVel.m_quad;
					const __m128 angularVel = accum->m_angularVel.m_quad;
					const __m128 absLinear = _mm_and_ps(linearVel, hkVector4_setAbs4_absMask);
					const __m128 absAngular = _mm_and_ps(angularVel, hkVector4_setAbs4_absMask);
					const __m128 energy =
						_mm_add_ps(_mm_mul_ps(_mm_set1_ps(deactivation.m_angularVelocityThresholdInv), absAngular),
							_mm_mul_ps(_mm_set1_ps(deactivation.m_linearVelocityThresholdInv), absLinear));
					const int32_t valid = _mm_movemask_ps(_mm_cmplt_ps(absAngular, rdata_10D3DA0)) &
						_mm_movemask_ps(_mm_cmplt_ps(absLinear, rdata_10D3DA0)) &
						_mm_movemask_ps(_mm_cmpeq_ps(angularVel, angularVel)) &
						_mm_movemask_ps(_mm_cmpeq_ps(linearVel, linearVel));
					if ((valid & 7) != 7) {
						accum->m_linearVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
						accum->m_angularVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
					}
					if ((_mm_movemask_ps(_mm_cmple_ps(energy, _mm_set1_ps(info.m_one))) & 7) == 7) {
						if ((_mm_movemask_ps(_mm_cmple_ps(energy,
								 _mm_set1_ps(deactivation.m_relativeSleepVelocityThreshold))) &
								7) == 7) {
							accum->m_angularVel.m_quad = _mm_setzero_ps();
							accum->m_linearVel.m_quad = _mm_setzero_ps();
						} else {
							accum->m_angularVel.m_quad =
								_mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
									accum->m_angularVel.m_quad);
							accum->m_linearVel.m_quad =
								_mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
									accum->m_linearVel.m_quad);
						}
					}
					const float gravityFactor = accum->m_gravityFactor;
					__m128 linDelta = _mm_sub_ps(accum->m_linearVel.m_quad, accum->m_scratch0.m_quad);
					__m128 angDelta = _mm_sub_ps(accum->m_angularVel.m_quad, accum->m_scratch123.m_col0.m_quad);
					const __m128 integrateFactor = _mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
						info.m_integrateVelocityFactor.m_quad, 0x00);
					accum->m_scratch0.m_quad =
						_mm_add_ps(_mm_mul_ps(integrateFactor, linDelta), accum->m_scratch0.m_quad);
					accum->m_scratch123.m_col0.m_quad =
						_mm_add_ps(_mm_mul_ps(integrateFactor, angDelta), accum->m_scratch123.m_col0.m_quad);
					const __m128 gravityStep = _mm_mul_ps(_mm_set1_ps(gravityFactor), gravity);
					if (!bLastStep) {
						angDelta = _mm_add_ps(angDelta, accum->m_scratch123.m_col0.m_quad);
						linDelta = _mm_add_ps(_mm_add_ps(linDelta, accum->m_scratch0.m_quad), gravityStep);
					} else {
						accum->m_scratch0.m_quad = _mm_mul_ps(accum->m_scratch0.m_quad, factorV);
						accum->m_scratch123.m_col0.m_quad = _mm_mul_ps(accum->m_scratch123.m_col0.m_quad, factorV);
					}
					accum->m_linearVel.m_quad = linDelta;
					accum->m_angularVel.m_quad = angDelta;
					++accum;
				} while (accum->m_type.m_storage == hkpVelocityAccumulator::HK_RIGID_BODY &&
					reinterpret_cast<uintptr_t>(accum) < UINTPTR_MAX);
				continue;
			case hkpVelocityAccumulator::HK_NO_GRAVITY_RIGID_BODY: {
				const hkpSolverInfo::DeactivationInfo& deactivation =
					info.m_deactivationInfo[accum->m_deactivationClass];
				const __m128 linearVel = accum->m_linearVel.m_quad;
				const __m128 angularVel = accum->m_angularVel.m_quad;
				const __m128 absLinear = _mm_and_ps(linearVel, hkVector4_setAbs4_absMask);
				const __m128 absAngular = _mm_and_ps(angularVel, hkVector4_setAbs4_absMask);
				const __m128 energy =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(deactivation.m_angularVelocityThresholdInv), absAngular),
						_mm_mul_ps(_mm_set1_ps(deactivation.m_linearVelocityThresholdInv), absLinear));
				const int32_t valid = _mm_movemask_ps(_mm_cmplt_ps(absAngular, rdata_10D3DA0)) &
					_mm_movemask_ps(_mm_cmplt_ps(absLinear, rdata_10D3DA0)) &
					_mm_movemask_ps(_mm_cmpeq_ps(angularVel, angularVel)) &
					_mm_movemask_ps(_mm_cmpeq_ps(linearVel, linearVel));
				if ((valid & 7) != 7) {
					accum->m_linearVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
					accum->m_angularVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
				}
				if ((_mm_movemask_ps(_mm_cmple_ps(energy, _mm_set1_ps(info.m_one))) & 7) == 7) {
					if ((_mm_movemask_ps(_mm_cmple_ps(energy,
							 _mm_set1_ps(deactivation.m_relativeSleepVelocityThreshold))) &
							7) == 7) {
						accum->m_angularVel.m_quad = _mm_setzero_ps();
						accum->m_linearVel.m_quad = _mm_setzero_ps();
					} else {
						accum->m_angularVel.m_quad =
							_mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
								accum->m_angularVel.m_quad);
						accum->m_linearVel.m_quad = _mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
							accum->m_linearVel.m_quad);
					}
				}
			}
				[[fallthrough]];
			case hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY: {
				// No gravity: the factor times the zero vector.
				const __m128 gravityStep = _mm_mul_ps(_mm_set1_ps(accum->m_gravityFactor), hkVector4::getZero().m_quad);
				__m128 linDelta = _mm_sub_ps(accum->m_linearVel.m_quad, accum->m_scratch0.m_quad);
				__m128 angDelta = _mm_sub_ps(accum->m_angularVel.m_quad, accum->m_scratch123.m_col0.m_quad);
				const __m128 integrateFactor =
					_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad, info.m_integrateVelocityFactor.m_quad, 0x00);
				accum->m_scratch0.m_quad = _mm_add_ps(accum->m_scratch0.m_quad, _mm_mul_ps(integrateFactor, linDelta));
				accum->m_scratch123.m_col0.m_quad =
					_mm_add_ps(_mm_mul_ps(integrateFactor, angDelta), accum->m_scratch123.m_col0.m_quad);
				if (!bLastStep) {
					angDelta = _mm_add_ps(accum->m_scratch123.m_col0.m_quad, angDelta);
					linDelta = _mm_add_ps(_mm_add_ps(accum->m_scratch0.m_quad, linDelta), gravityStep);
				} else {
					accum->m_scratch0.m_quad = _mm_mul_ps(factorV, accum->m_scratch0.m_quad);
					accum->m_scratch123.m_col0.m_quad = _mm_mul_ps(accum->m_scratch123.m_col0.m_quad, factorV);
				}
				accum->m_linearVel.m_quad = linDelta;
				accum->m_angularVel.m_quad = angDelta;
				++accum;
				continue;
			}
			case hkpVelocityAccumulator::HK_END:
				break;
			default:
				// Tests the end again without moving on.
				continue;
			}
			break;
		}
	}
	return 1;
}

// The forward pass eliminates the chain's tridiagonal system row by row into the temp buffer (temp[0] is the zero
// start); the backward pass substitutes back from the last constraint, applies each constraint's impulse to its two
// bodies and adds it to its solver result.
void hkSolver_solveStiffSpringChain(const hkpSolverInfo& info, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianStiffSpringChainSchema* chain, hkpSolverElemTemp* sr) {
	const int32_t numConstraints = chain->m_numConstraints;
	const hkp1Lin2AngJacobian* jacobians =
		hkpJacobianStiffSpringChainSchema::getJacobians(reinterpret_cast<hkpJacobianSchema*>(chain));
	const hkpConstraintChainTriple* triples = chain->getMatrixBuffer();
	float* tempBuffer = chain->getTempBuffer();
	const hkpVelocityAccumulatorOffset* offsets = chain->getAccumulatorOffsetsBase();
	char* accumsBase = reinterpret_cast<char*>(accums);
	tempBuffer[0] = 0.0f;

	for (int32_t i = 0; i < numConstraints; i++) {
		// The accumulators are raw memory addressed by the byte offsets the schema stores.
		const hkpVelocityAccumulator* accA =
			reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase + offsets[i].m_offset);
		const hkpVelocityAccumulator* accB =
			reinterpret_cast<const hkpVelocityAccumulator*>(accumsBase + offsets[i + 1].m_offset);
		const hkp1Lin2AngJacobian& jac = jacobians[i];
		const float tau = chain->m_tau;
		const float velocityFactor = info.m_invIntegrateVelocityFactor.m_quad[0] * tau;
		const hkQuadReal damping = _mm_shuffle_ps(_mm_set_ss(chain->m_damping), _mm_set_ss(chain->m_damping), 0);
		const hkQuadReal factor = _mm_shuffle_ps(_mm_set_ss(velocityFactor), _mm_set_ss(velocityFactor), 0);
		const hkQuadReal velTerm =
			_mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(accB->m_angularVel.m_quad,
															accB->m_scratch123.m_col0.m_quad),
												 jac.m_angular[1].m_quad),
									  _mm_mul_ps(_mm_sub_ps(accA->m_angularVel.m_quad,
													 accA->m_scratch123.m_col0.m_quad),
										  jac.m_angular[0].m_quad)),
						   _mm_mul_ps(_mm_sub_ps(_mm_sub_ps(accA->m_linearVel.m_quad, accA->m_scratch0.m_quad),
										  _mm_sub_ps(accB->m_linearVel.m_quad, accB->m_scratch0.m_quad)),
							   jac.m_linear0.m_quad)),
				damping);
		const hkQuadReal posTerm =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(accB->m_scratch123.m_col0.m_quad, jac.m_angular[1].m_quad),
						   _mm_mul_ps(accA->m_scratch123.m_col0.m_quad, jac.m_angular[0].m_quad)),
				_mm_mul_ps(_mm_sub_ps(accA->m_scratch0.m_quad, accB->m_scratch0.m_quad), jac.m_linear0.m_quad));
		const hkQuadReal sum = _mm_add_ps(velTerm, _mm_mul_ps(posTerm, factor));
		// Sums (y + x) first, then z + that, each rounded to a float, as _ps operations.
		const hkQuadReal sumXY = _mm_add_ps(_mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1, 1, 1, 1)), sum);
		const float dot = _mm_add_ps(_mm_shuffle_ps(sum, sum, _MM_SHUFFLE(2, 2, 2, 2)), sumXY)[0];
		// The right-hand side is rounded to a float before the substitution.
		float rhs = tau * jac.m_linear0.m_quad[3] - dot;
		tempBuffer[i + 1] = (rhs - tempBuffer[i] * triples[i].m_lower) / triples[i].m_diagonal;
	}

	float impulse = 0.0f;
	for (int32_t i = numConstraints - 1; i >= 0; i--) {
		impulse = tempBuffer[i + 1] - triples[i].m_upper * impulse;
		// The accumulators are raw memory addressed by the byte offsets the schema stores.
		hkpVelocityAccumulator* accA = reinterpret_cast<hkpVelocityAccumulator*>(accumsBase + offsets[i].m_offset);
		hkpVelocityAccumulator* accB = reinterpret_cast<hkpVelocityAccumulator*>(accumsBase + offsets[i + 1].m_offset);
		const hkp1Lin2AngJacobian& jac = jacobians[i];
		const hkQuadReal impulse4 = _mm_shuffle_ps(_mm_set_ss(impulse), _mm_set_ss(impulse), 0);
		const hkQuadReal impA = _mm_mul_ps(accA->m_invMasses.m_quad, impulse4);
		const hkQuadReal impB = _mm_mul_ps(accB->m_invMasses.m_quad, impulse4);
		accA->m_linearVel.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impA, impA, 0xFF), jac.m_linear0.m_quad), accA->m_linearVel.m_quad);
		accB->m_linearVel.m_quad =
			_mm_sub_ps(accB->m_linearVel.m_quad, _mm_mul_ps(_mm_shuffle_ps(impB, impB, 0xFF), jac.m_linear0.m_quad));
		accA->m_angularVel.m_quad = _mm_add_ps(_mm_mul_ps(jac.m_angular[0].m_quad, impA), accA->m_angularVel.m_quad);
		accB->m_angularVel.m_quad = _mm_add_ps(_mm_mul_ps(jac.m_angular[1].m_quad, impB), accB->m_angularVel.m_quad);
		sr[i].m_impulseApplied += impulse;
	}
}

// The solver's case for a powered chain. It moves each motor row's target by the motor's velocity when positions are
// integrated, builds the chain's velocities and solves the LU-decomposed chain matrix for g, then disables the motors
// whose impulse breaches its bounds and re-enables the ones that came back, redoing the decomposition and g after each
// change. The back substitution from the last constraint applies each linear row's impulse to both bodies' velocities
// and each motor row's to their angular velocities, and adds the six impulses to the constraint's solver results.
void hkSolver_solvePoweredChain(const hkpSolverInfo& solverInfo, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianPoweredChainSchema* chain, hkpSolverElemTemp* sr) {
	const int32_t numConstraints = chain->m_numConstraints;
	// The schema stream's parts after the head. The stream is raw memory read through each part's type.
	hkp1Lin2AngJacobian* jacobians = reinterpret_cast<hkp1Lin2AngJacobian*>(chain + 1);
	hkp2AngJacobian* jacobiansAng = reinterpret_cast<hkp2AngJacobian*>(jacobians + 3 * numConstraints);
	hkpConstraintChainMatrix6Triple* triples =
		reinterpret_cast<hkpConstraintChainMatrix6Triple*>(jacobiansAng + 3 * numConstraints);
	hkVector8* gTempBuffer = reinterpret_cast<hkVector8*>(triples + numConstraints);
	hkVector8* velocities = gTempBuffer + (numConstraints + 1);
	hkpVelocityAccumulatorOffset* accumOffsets =
		reinterpret_cast<hkpVelocityAccumulatorOffset*>(velocities + numConstraints);
	hkp3dAngularMotorSolverInfo* motorsState =
		reinterpret_cast<hkp3dAngularMotorSolverInfo*>(accumOffsets + (numConstraints + 1));
	alignas(16) hkpChainSolverInfo info = {solverInfo, {numConstraints}, {chain->m_tau}, {chain->m_damping},
		{jacobians}, {jacobiansAng}, {accumOffsets}, {accums}, {triples}, {motorsState}, {gTempBuffer}, {velocities}};

	for (int32_t i = 0; i < info.m_numConstraints.m_storage; i++) {
		for (int32_t k = 0; k < 3; k++) {
			hkVector4& target = jacobiansAng[3 * i + k].m_angular[1];
			target(3) = motorsState[i].m_motorInfos[k].m_velocity * temp.m_intregratePositions.m_storage + target(3);
		}
	}
	hkPoweredChain_BuildVelocities(info, velocities);
	for (int32_t i = 0; i < info.m_numConstraints.m_storage; i++) {
		for (int32_t k = 0; k < 3; k++) {
			const int32_t state = (motorsState[i].m_broken >> (2 * k)) & 3;
			if (state == hkp3dAngularMotorSolverInfo::MOTOR_DISABLED) {
				velocities[i].m_ang(k) = 0.0f;
			} else if (state != hkp3dAngularMotorSolverInfo::MOTOR_NOT_BROKEN) {
				hkPoweredChain_OverwriteVelocityWithExplicitImpulse(i, k, motorsState, velocities);
			}
		}
	}

	int32_t constraintIndex;
	alignas(16) int32_t coordinate;
	float impulse;
	hkPoweredChain_ForwardSubstitute(info, gTempBuffer);
	impulse = 0.0f;
	constraintIndex = -1;
	coordinate = -1;
	hkPoweredChain_ScanAndEnableMotors(info, constraintIndex, coordinate, impulse, motorsState);
	while (constraintIndex >= 0) {
		hkPoweredChain_UpdateLuDecomposition(constraintIndex, info.m_numConstraints.m_storage, motorsState, triples);
		hkPoweredChain_computeMotorVelocity(constraintIndex, coordinate, info, velocities);
		hkPoweredChain_ForwardSubstitute(info, gTempBuffer);
		impulse = 0.0f;
		constraintIndex = -1;
		coordinate = -1;
		hkPoweredChain_ScanAndEnableMotors(info, constraintIndex, coordinate, impulse, motorsState);
	}

	hkPoweredChain_ForwardSubstitute(info, gTempBuffer);
	impulse = 0.0f;
	constraintIndex = -1;
	coordinate = -1;
	hkPoweredChain_ScanAndDisableMotors(info, constraintIndex, coordinate, impulse, motorsState);
	while (constraintIndex >= 0) {
		hkPoweredChain_UpdateLuDecomposition(constraintIndex, info.m_numConstraints.m_storage, motorsState, triples);
		hkPoweredChain_OverwriteVelocityWithExplicitImpulse(constraintIndex, coordinate, motorsState, velocities);
		hkPoweredChain_ForwardSubstitute(info, gTempBuffer);
		impulse = 0.0f;
		constraintIndex = -1;
		coordinate = -1;
		hkPoweredChain_ScanAndDisableMotors(info, constraintIndex, coordinate, impulse, motorsState);
	}

	// Back substitution: (lin, ang) = g[i + 1] - upper[i] * (the next constraint's solution, zero for the last), each
	// block product summed as (col1 * y + col0 * x) + col2 * z. The angular velocities of both bodies are read before
	// either is stored in the linear rows, since the two accumulators may be one.
	__m128 linX = _mm_setzero_ps();
	__m128 linY = _mm_setzero_ps();
	__m128 linZ = _mm_setzero_ps();
	__m128 angX = _mm_setzero_ps();
	__m128 angY = _mm_setzero_ps();
	__m128 angZ = _mm_setzero_ps();
	for (int32_t i = info.m_numConstraints.m_storage - 1; i >= 0; i--) {
		const hkMatrix6& upper = triples[i].m_upper;
		const hkVector8& g = gTempBuffer[i + 1];
		const hkMatrix3& u00 = upper.m_m[0][0];
		const hkMatrix3& u01 = upper.m_m[0][1];
		const hkMatrix3& u10 = upper.m_m[1][0];
		const hkMatrix3& u11 = upper.m_m[1][1];
		const __m128 a =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(u00.m_col1.m_quad, linY), _mm_mul_ps(u00.m_col0.m_quad, linX)),
				_mm_mul_ps(u00.m_col2.m_quad, linZ));
		const __m128 b =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(u01.m_col1.m_quad, angY), _mm_mul_ps(u01.m_col0.m_quad, angX)),
				_mm_mul_ps(u01.m_col2.m_quad, angZ));
		const __m128 c =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(u10.m_col1.m_quad, linY), _mm_mul_ps(u10.m_col0.m_quad, linX)),
				_mm_mul_ps(u10.m_col2.m_quad, linZ));
		const __m128 d =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(u11.m_col1.m_quad, angY), _mm_mul_ps(u11.m_col0.m_quad, angX)),
				_mm_mul_ps(u11.m_col2.m_quad, angZ));
		const __m128 lin = _mm_sub_ps(g.m_lin.m_quad, _mm_add_ps(b, a));
		const __m128 ang = _mm_sub_ps(g.m_ang.m_quad, _mm_add_ps(d, c));

		// EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator> (the offsets count bytes from the accumulators'
		// base, in the raw accumulator buffer)
		hkpVelocityAccumulator* accA =
			reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<char*>(accums) + accumOffsets[i].m_offset);
		// EXE-INLINED none: hkAddByteOffset<hkpVelocityAccumulator>
		hkpVelocityAccumulator* accB =
			reinterpret_cast<hkpVelocityAccumulator*>(reinterpret_cast<char*>(accums) + accumOffsets[i + 1].m_offset);
		const hkp1Lin2AngJacobian* jac = &jacobians[3 * i];
		const hkp2AngJacobian* jacAng = &jacobiansAng[3 * i];
		hkpSolverElemTemp* results = &sr[6 * i];

		// The three linear rows: the impulse times each body's inverse mass (w) along the row, opposite for body B,
		// and times its inverse inertia along the row's angular parts.
		linX = _mm_shuffle_ps(lin, lin, 0x00);
		linY = _mm_shuffle_ps(lin, lin, 0x55);
		linZ = _mm_shuffle_ps(lin, lin, 0xAA);
		const __m128 linSplat[3] = {linX, linY, linZ};
		for (int32_t k = 0; k < 3; k++) {
			const __m128 impulseA = _mm_mul_ps(linSplat[k], accA->m_invMasses.m_quad);
			const __m128 impulseB = _mm_mul_ps(linSplat[k], accB->m_invMasses.m_quad);
			accA->m_linearVel.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(impulseA, impulseA, 0xFF), jac[k].m_linear0.m_quad),
					accA->m_linearVel.m_quad);
			accB->m_linearVel.m_quad = _mm_sub_ps(accB->m_linearVel.m_quad,
				_mm_mul_ps(_mm_shuffle_ps(impulseB, impulseB, 0xFF), jac[k].m_linear0.m_quad));
			const __m128 angVelA =
				_mm_add_ps(_mm_mul_ps(jac[k].m_angular[0].m_quad, impulseA), accA->m_angularVel.m_quad);
			const __m128 angVelB =
				_mm_add_ps(_mm_mul_ps(jac[k].m_angular[1].m_quad, impulseB), accB->m_angularVel.m_quad);
			accA->m_angularVel.m_quad = angVelA;
			accB->m_angularVel.m_quad = angVelB;
			_mm_store_ss(&results[k].m_deltaRhs, _mm_add_ps(_mm_load_ss(&results[k].m_deltaRhs), linSplat[k]));
		}

		// The three motor rows: the impulse times each body's inverse inertia, times the row's angular part.
		angX = _mm_shuffle_ps(ang, ang, 0x00);
		angY = _mm_shuffle_ps(ang, ang, 0x55);
		angZ = _mm_shuffle_ps(ang, ang, 0xAA);
		const __m128 angSplat[3] = {angX, angY, angZ};
		for (int32_t k = 0; k < 3; k++) {
			accA->m_angularVel.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_mul_ps(angSplat[k], accA->m_invMasses.m_quad), jacAng[k].m_angular[0].m_quad),
					accA->m_angularVel.m_quad);
			accB->m_angularVel.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_mul_ps(angSplat[k], accB->m_invMasses.m_quad), jacAng[k].m_angular[1].m_quad),
					accB->m_angularVel.m_quad);
			_mm_store_ss(&results[3 + k].m_deltaRhs, _mm_add_ps(_mm_load_ss(&results[3 + k].m_deltaRhs), angSplat[k]));
		}
	}
}

// Clears each body's scratch velocities and adds one substep of gravity to the rigid bodies' linear velocities
// (hkpSolver.cpp; the SDK ships no source for it). A null end runs to the HK_END accumulator.
void hkSolveApplyGravityByTheSteps(const hkpSolverInfo& info, hkpVelocityAccumulator* accumulators,
	hkpVelocityAccumulator* accumulatorsEnd) {
	const __m128 gravity = info.m_globalAccelerationPerSubStep.m_quad;
	// A null end compares as the highest address.
	const uintptr_t end = accumulatorsEnd ? reinterpret_cast<uintptr_t>(accumulatorsEnd) : UINTPTR_MAX;
	hkpVelocityAccumulator* accum = accumulators;
	while (reinterpret_cast<uintptr_t>(accum) < end) {
		switch (accum->m_type.m_storage) {
		case hkpVelocityAccumulator::HK_RIGID_BODY:
			// Consecutive rigid bodies run in a loop of their own, which reads the next one's type before it tests the
			// end.
			do {
				accum->m_scratch0.m_quad = _mm_setzero_ps();
				accum->m_scratch123.m_col0.m_quad = _mm_setzero_ps();
				accum->m_linearVel.m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(accum->m_gravityFactor), gravity), accum->m_linearVel.m_quad);
				++accum;
			} while (accum->m_type.m_storage == hkpVelocityAccumulator::HK_RIGID_BODY &&
				reinterpret_cast<uintptr_t>(accum) < end);
			break;
		case hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY:
		case hkpVelocityAccumulator::HK_NO_GRAVITY_RIGID_BODY:
			accum->m_scratch0.m_quad = _mm_setzero_ps();
			accum->m_scratch123.m_col0.m_quad = _mm_setzero_ps();
			++accum;
			break;
		case hkpVelocityAccumulator::HK_END:
			return;
		default:
			// Breaks here and then tests the end again without moving on, as the game does.
			__builtin_debugtrap();
			break;
		}
	}
}

// Integrates each body's solved velocities into its scratch velocities (hkpSolver.cpp; the SDK ships no source for
// it). A rigid or no-gravity body's velocities are first reset when NaN or past the limit in x, y or z, and slowed
// or stopped below its deactivation class's thresholds; a rigid body adds its gravity. On the last step it sets
// outSolveFinished and scales the scratch velocities back. A null end runs to the HK_END accumulator. Returns 1.
int32_t hkSolveIntegrateVelocitiesByTheSteps(const hkpSolverInfo& info, int32_t currentSolverStep,
	hkpVelocityAccumulator* accumulators, hkpVelocityAccumulator* accumulatorsEnd, hkBool& outSolveFinished) {
	const __m128 gravity = info.m_globalAccelerationPerSubStep.m_quad;
	// x87: the product rounded to a float.
	const float factor = info.m_invNumSteps * info.m_invIntegrateVelocityFactor.m_quad[0];
	const __m128 factorV = _mm_set1_ps(factor);
	bool solveFinished = false;
	outSolveFinished.m_bool = 0;
	if (currentSolverStep >= info.m_numSteps - 1) {
		solveFinished = true;
		outSolveFinished.m_bool = 1;
	}
	// A null end compares as the highest address.
	const uintptr_t end = accumulatorsEnd ? reinterpret_cast<uintptr_t>(accumulatorsEnd) : UINTPTR_MAX;
	hkpVelocityAccumulator* accum = accumulators;
	while (reinterpret_cast<uintptr_t>(accum) < end) {
		switch (accum->m_type.m_storage) {
		case hkpVelocityAccumulator::HK_RIGID_BODY:
			// Consecutive rigid bodies run in a loop of their own, which reads the next one's type before it tests the
			// end.
			do {
				const hkpSolverInfo::DeactivationInfo& deactivation =
					info.m_deactivationInfo[accum->m_deactivationClass];
				const __m128 linearVel = accum->m_linearVel.m_quad;
				const __m128 angularVel = accum->m_angularVel.m_quad;
				const __m128 absLinear = _mm_and_ps(linearVel, hkVector4_setAbs4_absMask);
				const __m128 absAngular = _mm_and_ps(angularVel, hkVector4_setAbs4_absMask);
				const __m128 energy =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(deactivation.m_angularVelocityThresholdInv), absAngular),
						_mm_mul_ps(_mm_set1_ps(deactivation.m_linearVelocityThresholdInv), absLinear));
				const int32_t valid = _mm_movemask_ps(_mm_cmplt_ps(absAngular, rdata_10D3DA0)) &
					_mm_movemask_ps(_mm_cmplt_ps(absLinear, rdata_10D3DA0)) &
					_mm_movemask_ps(_mm_cmpeq_ps(angularVel, angularVel)) &
					_mm_movemask_ps(_mm_cmpeq_ps(linearVel, linearVel));
				if ((valid & 7) != 7) {
					accum->m_linearVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
					accum->m_angularVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
				}
				if ((_mm_movemask_ps(_mm_cmple_ps(energy, _mm_set1_ps(info.m_one))) & 7) == 7) {
					if ((_mm_movemask_ps(_mm_cmple_ps(energy,
							 _mm_set1_ps(deactivation.m_relativeSleepVelocityThreshold))) &
							7) == 7) {
						accum->m_angularVel.m_quad = _mm_setzero_ps();
						accum->m_linearVel.m_quad = _mm_setzero_ps();
					} else {
						accum->m_angularVel.m_quad =
							_mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
								accum->m_angularVel.m_quad);
						accum->m_linearVel.m_quad = _mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
							accum->m_linearVel.m_quad);
					}
				}
				const __m128 gravityStep = _mm_mul_ps(_mm_set1_ps(accum->m_gravityFactor), gravity);
				__m128 linDelta = _mm_sub_ps(accum->m_linearVel.m_quad, accum->m_scratch0.m_quad);
				__m128 angDelta = _mm_sub_ps(accum->m_angularVel.m_quad, accum->m_scratch123.m_col0.m_quad);
				accum->m_scratch0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
																	 info.m_integrateVelocityFactor.m_quad, 0x00),
														  linDelta),
					accum->m_scratch0.m_quad);
				accum->m_scratch123.m_col0.m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
											  info.m_integrateVelocityFactor.m_quad, 0x00),
								   angDelta),
						accum->m_scratch123.m_col0.m_quad);
				if (!solveFinished) {
					angDelta = _mm_add_ps(angDelta, accum->m_scratch123.m_col0.m_quad);
					linDelta = _mm_add_ps(_mm_add_ps(linDelta, accum->m_scratch0.m_quad), gravityStep);
				} else {
					accum->m_scratch0.m_quad = _mm_mul_ps(factorV, accum->m_scratch0.m_quad);
					accum->m_scratch123.m_col0.m_quad = _mm_mul_ps(factorV, accum->m_scratch123.m_col0.m_quad);
				}
				accum->m_linearVel.m_quad = linDelta;
				accum->m_angularVel.m_quad = angDelta;
				++accum;
			} while (accum->m_type.m_storage == hkpVelocityAccumulator::HK_RIGID_BODY &&
				reinterpret_cast<uintptr_t>(accum) < end);
			break;
		case hkpVelocityAccumulator::HK_NO_GRAVITY_RIGID_BODY: {
			const hkpSolverInfo::DeactivationInfo& deactivation = info.m_deactivationInfo[accum->m_deactivationClass];
			const __m128 linearVel = accum->m_linearVel.m_quad;
			const __m128 angularVel = accum->m_angularVel.m_quad;
			const __m128 absLinear = _mm_and_ps(linearVel, hkVector4_setAbs4_absMask);
			const __m128 absAngular = _mm_and_ps(angularVel, hkVector4_setAbs4_absMask);
			const __m128 energy =
				_mm_add_ps(_mm_mul_ps(_mm_set1_ps(deactivation.m_angularVelocityThresholdInv), absAngular),
					_mm_mul_ps(_mm_set1_ps(deactivation.m_linearVelocityThresholdInv), absLinear));
			const int32_t valid = _mm_movemask_ps(_mm_cmpeq_ps(angularVel, angularVel)) &
				_mm_movemask_ps(_mm_cmplt_ps(absAngular, rdata_10D3DA0)) &
				_mm_movemask_ps(_mm_cmpeq_ps(linearVel, linearVel)) &
				_mm_movemask_ps(_mm_cmplt_ps(absLinear, rdata_10D3DA0));
			if ((valid & 7) != 7) {
				accum->m_linearVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
				accum->m_angularVel.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
			}
			if ((_mm_movemask_ps(_mm_cmple_ps(energy, _mm_set1_ps(info.m_one))) & 7) == 7) {
				if ((_mm_movemask_ps(_mm_cmple_ps(energy, _mm_set1_ps(deactivation.m_relativeSleepVelocityThreshold))) &
						7) == 7) {
					accum->m_angularVel.m_quad = _mm_setzero_ps();
					accum->m_linearVel.m_quad = _mm_setzero_ps();
				} else {
					accum->m_angularVel.m_quad = _mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier),
						accum->m_angularVel.m_quad);
					accum->m_linearVel.m_quad =
						_mm_mul_ps(_mm_set1_ps(deactivation.m_slowObjectVelocityMultiplier), accum->m_linearVel.m_quad);
				}
			}
		}
			[[fallthrough]];
		case hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY: {
			// No gravity: the factor times the zero vector.
			const __m128 gravityStep = _mm_mul_ps(_mm_set1_ps(accum->m_gravityFactor), hkVector4::getZero().m_quad);
			__m128 linDelta = _mm_sub_ps(accum->m_linearVel.m_quad, accum->m_scratch0.m_quad);
			__m128 angDelta = _mm_sub_ps(accum->m_angularVel.m_quad, accum->m_scratch123.m_col0.m_quad);
			accum->m_scratch0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
																 info.m_integrateVelocityFactor.m_quad, 0x00),
													  linDelta),
				accum->m_scratch0.m_quad);
			accum->m_scratch123.m_col0.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
										  info.m_integrateVelocityFactor.m_quad, 0x00),
							   angDelta),
					accum->m_scratch123.m_col0.m_quad);
			if (!solveFinished) {
				angDelta = _mm_add_ps(angDelta, accum->m_scratch123.m_col0.m_quad);
				linDelta = _mm_add_ps(_mm_add_ps(linDelta, accum->m_scratch0.m_quad), gravityStep);
			} else {
				accum->m_scratch0.m_quad = _mm_mul_ps(accum->m_scratch0.m_quad, factorV);
				accum->m_scratch123.m_col0.m_quad = _mm_mul_ps(accum->m_scratch123.m_col0.m_quad, factorV);
			}
			accum->m_linearVel.m_quad = linDelta;
			accum->m_angularVel.m_quad = angDelta;
			++accum;
			break;
		}
		case hkpVelocityAccumulator::HK_END:
			return 1;
		default:
			// Tests the end again without moving on.
			break;
		}
	}
	return 1;
}

// The solver's step over the schema stream: the same walk as hkSolveConstraintsByTheSteps, with a step temp of step 0
// that integrates no positions and no license check. The stream is raw memory read through each schema's class (hence
// the casts). A schema that follows a pair contact, a 2d, 3d or rolling friction (another pair contact, a 3d or 2d
// friction, a header) goes back through the switch here, which reaches the same code with the same calls.
void hkSolveStepJacobians(const hkpSolverInfo& info, const hkpJacobianSchema* schemas,
	hkpVelocityAccumulator* accumulators, hkpSolverElemTemp* tmp) {
	hkSolver::SolverStepTemp stepTemp;
	stepTemp.m_intregratePositions.m_storage = 0.0f;
	stepTemp.m_currentStep.m_storage = 0;

	// The getType helper and the chain solvers take the stream as non-const.
	hkpJacobianSchema* pSchema = const_cast<hkpJacobianSchema*>(schemas);
	if (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_END) {
		return;
	}
	// Reads the first schema as a header whatever its type.
	hkpVelocityAccumulator* pAccA = &accumulators[pSchema->getHeader().m_bodyAIndex];
	hkpVelocityAccumulator* pAccB = &accumulators[pSchema->getHeader().m_bodyBIndex];
	pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	while (true) {
		__m128& linA = pAccA->m_linearVel.m_quad;
		__m128& angA = pAccA->m_angularVel.m_quad;
		__m128& invA = pAccA->m_invMasses.m_quad;
		__m128& s0A = pAccA->m_scratch0.m_quad;
		__m128& s1A = pAccA->m_scratch123.m_col0.m_quad;
		__m128& linB = pAccB->m_linearVel.m_quad;
		__m128& angB = pAccB->m_angularVel.m_quad;
		__m128& invB = pAccB->m_invMasses.m_quad;
		__m128& s0B = pAccB->m_scratch0.m_quad;
		__m128& s1B = pAccB->m_scratch123.m_col0.m_quad;
		switch (pSchema->m_bytes.m_type) {
		case hkpJacobianSchema::SCHEMA_TYPE_END: {
			return;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_HEADER: {
			pAccA = &accumulators[pSchema->getHeader().m_bodyAIndex];
			pAccB = &accumulators[pSchema->getHeader().m_bodyBIndex];
			pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_GOTO: {
			hkpJacobianGotoSchema* pS = reinterpret_cast<hkpJacobianGotoSchema*>(pSchema);
			// The offset is added to the cursor as a signed value; a backward jump is a negative one.
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) +
				static_cast<int32_t>(pS->m_offset));
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SHIFT_SOLVER_RESULTS_POINTER: {
			++pSchema;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL: {
			uint8_t uType;
			do {
				hkpJacobian1dBilateralSchema* pS = reinterpret_cast<hkpJacobian1dBilateralSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad_1 = l0;
				__m128 v183 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				const __m128 v184 =
					_mm_add_ps(_mm_shuffle_ps(v183, v183, 170), _mm_add_ps(_mm_shuffle_ps(v183, v183, 85), v183));
				const __m128 v185 =
					_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(m_quad_1, m_quad_1, 255), v184), _mm_shuffle_ps(a0, a0, 255));
				__m128 v187 = _mm_shuffle_ps(v185, v185, 0);
				__m128 v188 = _mm_mul_ps(v187, invA);
				__m128 v189 = _mm_mul_ps(invB, v187);
				linA = _mm_add_ps(linA, _mm_mul_ps(l0, _mm_shuffle_ps(v188, v188, 255)));
				linB = _mm_sub_ps(linB, _mm_mul_ps(m_quad_1, _mm_shuffle_ps(v189, v189, 255)));
				__m128 v190 = _mm_mul_ps(a1, v189);
				angA = _mm_add_ps(_mm_mul_ps(v188, a0), angA);
				angB = _mm_add_ps(angB, v190);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v185));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT: {
			uint8_t uType;
			do {
				// The impulse limit is in the w of the second angular row; the schema has the bilateral's size.
				hkpJacobian1dBilateralSchema* pS = reinterpret_cast<hkpJacobian1dBilateralSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad_3 = l0;
				__m128 v193 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				const __m128 v193Sum =
					_mm_add_ps(_mm_shuffle_ps(v193, v193, 170), _mm_add_ps(_mm_shuffle_ps(v193, v193, 85), v193));
				const __m128 v194 =
					_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 255), v193Sum), _mm_shuffle_ps(a0, a0, 255));
				const __m128 vApplied = _mm_load_ss(&tmp->m_deltaRhs);
				const __m128 v196 = _mm_shuffle_ps(a1, a1, 255);
				const __m128 m_quad_4 = _mm_sub_ps(_mm_sub_ps(hkQuadReal0000, v196), vApplied);
				__m128 v197 = _mm_min_ps(_mm_sub_ps(v196, vApplied), _mm_max_ps(m_quad_4, v194));
				__m128 v198 = _mm_shuffle_ps(v197, v197, 0);
				__m128 v199 = _mm_mul_ps(invB, v198);
				__m128 v200 = _mm_mul_ps(invA, v198);
				linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v200, v200, 255), l0));
				linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v199, v199, 255), m_quad_3));
				__m128 v201 = _mm_mul_ps(a0, v200);
				__m128 v202 = _mm_mul_ps(a1, v199);
				angA = _mm_add_ps(v201, angA);
				angB = _mm_add_ps(angB, v202);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v197));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
				hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_WITH_IMPULSE_LIMIT);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU: {
			uint8_t uType;
			do {
				hkpJacobian1dBilateralUserTauSchema* pS =
					reinterpret_cast<hkpJacobian1dBilateralUserTauSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v345 = l0[3];
				double v204 = pS->m_tau;
				float v205 = pS->m_damping;
				__m128 m_quad_5 = l0;
				__m128 v207 = _mm_sub_ps(angB, s1B);
				__m128 v208 = _mm_sub_ps(_mm_sub_ps(linA, s0A), _mm_sub_ps(linB, s0B));
				__m128 v209 = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
				__m128 m_quad_6 = a1;
				__m128 v383 = s0A;
				__m128 v376 = s1A;
				__m128 v381 = s0B;
				__m128 v378 = s1B;
				__m128 v211 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m_quad_6, v207), v209), _mm_mul_ps(m_quad_5, v208));
				__m128 v212 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, v378), _mm_mul_ps(v376, a0)),
					_mm_mul_ps(l0, _mm_sub_ps(v383, v381)));
				float v317 = v204 * info.m_invIntegrateVelocityFactor.m_quad[0u];
				__m128 v213 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v317), _mm_set_ss(v317), 0), v212);
				v212[0] = v205;
				__m128 v214 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v212, v212, 0), v211), v213);
				const __m128 v214Sum =
					_mm_add_ps(_mm_shuffle_ps(v214, v214, 170), _mm_add_ps(_mm_shuffle_ps(v214, v214, 85), v214));
				float v362 = v214Sum[0];
				__m128 m_quad_7 = l0;
				float v318 = v204 * v345 - v362;
				__m128 v217 = _mm_mul_ps(_mm_set_ss(v318), _mm_shuffle_ps(a0, a0, 255));
				__m128 v218 = _mm_shuffle_ps(v217, v217, 0);
				__m128 v219 = _mm_mul_ps(invB, v218);
				__m128 v220 = _mm_mul_ps(invA, v218);
				linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v220, v220, 255), l0));
				linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v219, v219, 255), m_quad_7));
				__m128 v221 = _mm_mul_ps(a1, v219);
				angA = _mm_add_ps(angA, _mm_mul_ps(v220, a0));
				angB = _mm_add_ps(angB, v221);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v217));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) ==
				hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS: {
			uint8_t uType;
			do {
				hkpJacobian1dLinearLimitsSchema* pS = reinterpret_cast<hkpJacobian1dLinearLimitsSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 v230 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				float v366 = _mm_sub_ps(_mm_shuffle_ps(l0, l0, 255),
					_mm_add_ps(_mm_shuffle_ps(v230, v230, 170), _mm_add_ps(_mm_shuffle_ps(v230, v230, 85), v230)))[0];
				float v337 = v366;
				float v321 = -tmp->m_deltaRhs;
				float v292 = v366 - pS->m_minRhs;
				float v293 = v292 * a0[3];
				if (v321 < static_cast<double>(v293)) {
					__m128 v231 = _mm_shuffle_ps(_mm_set_ss(v293), _mm_set_ss(v293), 0);
					__m128 v232 = _mm_mul_ps(invA, v231);
					__m128 v233 = _mm_mul_ps(invB, v231);
					__m128 v234 = _mm_mul_ps(_mm_shuffle_ps(v233, v233, 255), l0);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v232, v232, 255), l0), linA);
					linB = _mm_sub_ps(linB, v234);
					__m128 v235 = _mm_mul_ps(v233, a1);
					angA = _mm_add_ps(_mm_mul_ps(v232, a0), angA);
					angB = _mm_add_ps(angB, v235);
					_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v231));
				}
				float v294 = v337 - pS->m_maxRhs;
				float v295 = v294 * a0[3];
				if (v295 < static_cast<double>(v321)) {
					__m128 v236 = _mm_shuffle_ps(_mm_set_ss(v295), _mm_set_ss(v295), 0);
					__m128 v237 = _mm_mul_ps(invB, v236);
					__m128 v238 = _mm_mul_ps(invA, v236);
					__m128 v239 = _mm_mul_ps(_mm_shuffle_ps(v237, v237, 255), l0);
					linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v238, v238, 255), l0));
					linB = _mm_sub_ps(linB, v239);
					__m128 v240 = _mm_mul_ps(v237, a1);
					angA = _mm_add_ps(_mm_mul_ps(v238, a0), angA);
					angB = _mm_add_ps(v240, angB);
					_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v236));
				}
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION: {
			uint8_t uType;
			do {
				hkpJacobian1dFrictionSchema* pS = reinterpret_cast<hkpJacobian1dFrictionSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				double v223;
				__m128 v222 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				float v364 = _mm_sub_ps(_mm_shuffle_ps(l0, l0, 255),
					_mm_add_ps(_mm_shuffle_ps(v222, v222, 170), _mm_add_ps(_mm_shuffle_ps(v222, v222, 85), v222)))[0];
				float v291 = a0[3] * v364;
				float v319 = std::fabs(static_cast<double>(v291));
				if (!(pS->m_maxImpulse < static_cast<double>(v319))) {
					v223 = 1.0;
				} else {
					float v320 = pS->m_maxImpulse / v319;
					v223 = v320;
					v291 = v291 * v320;
				}
				tmp[1].m_deltaRhs = v223;
				__m128 v224 = _mm_shuffle_ps(_mm_set_ss(v291), _mm_set_ss(v291), 0);
				__m128 v225 = _mm_mul_ps(invA, v224);
				__m128 v226 = _mm_mul_ps(invB, v224);
				__m128 v227 = _mm_mul_ps(_mm_shuffle_ps(v226, v226, 255), l0);
				linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v225, v225, 255), l0));
				linB = _mm_sub_ps(linB, v227);
				__m128 v228 = _mm_mul_ps(v226, a1);
				angA = _mm_add_ps(_mm_mul_ps(v225, a0), angA);
				angB = _mm_add_ps(angB, v228);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v224));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*copyByte(&uType, &pSchema->m_bytes.m_type) == hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR: {
			uint8_t uType;
			uint8_t* pNextType;
			do {
				hkpJacobian1dLinearMotorSchema* pS = reinterpret_cast<hkpJacobian1dLinearMotorSchema*>(pSchema);
				const hkp1dMotorSolverInfo& motor = pS->m_motorSolverInfo;
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v322 = motor.m_velocity * stepTemp.m_intregratePositions.m_storage + tmp[2].m_deltaRhs;
				tmp[2].m_deltaRhs = v322;
				float v242 = motor.m_damping;
				float v338 = l0[3] + v322;
				float v323 = motor.m_tau;
				__m128 v243 = _mm_sub_ps(_mm_sub_ps(linA, s0A), _mm_sub_ps(linB, s0B));
				__m128 v244 = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
				__m128 v245 = _mm_mul_ps(_mm_sub_ps(angB, s1B), a1);
				__m128 m_quad_8 = l0;
				__m128 v380 = s0A;
				__m128 v384 = s1A;
				__m128 v382 = s0B;
				__m128 v386 = s1B;
				__m128 v247 = _mm_add_ps(_mm_add_ps(v245, v244), _mm_mul_ps(v243, m_quad_8));
				double v248 = v323;
				float v324 = v323 * info.m_invIntegrateVelocityFactor.m_quad[0u];
				__m128 v249 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(v386, a1), _mm_mul_ps(v384, a0)),
					_mm_mul_ps(_mm_sub_ps(v380, v382), l0));
				__m128 v250 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v324), _mm_set_ss(v324), 0), v249);
				v249[0] = v242;
				__m128 v251 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v249, v249, 0), v247), v250);
				float v353 =
					_mm_add_ps(_mm_shuffle_ps(v251, v251, 170), _mm_add_ps(_mm_shuffle_ps(v251, v251, 85), v251))[0];
				float v325 = v248 * v338 - v353;
				double v252 = a0[3] * v325;
				float v339 = v252;
				double v253 = v339;
				if (!(motor.m_maxImpulsePerSubstep < static_cast<double>(v339))) {
					if (!(motor.m_minImpulsePerSubstep > v253)) {
						tmp[1].m_deltaRhs = 1.0f;
					} else {
						tmp[1].m_deltaRhs = motor.m_minImpulsePerSubstep / v253;
						v339 = motor.m_minImpulsePerSubstep;
					}
				} else {
					tmp[1].m_deltaRhs = motor.m_maxImpulsePerSubstep / v253;
					v339 = motor.m_maxImpulsePerSubstep;
				}
				__m128 v254 = _mm_shuffle_ps(_mm_set_ss(v339), _mm_set_ss(v339), 0);
				__m128 v255 = _mm_mul_ps(v254, invA);
				__m128 v256 = _mm_mul_ps(v254, invB);
				__m128 v257 = _mm_mul_ps(_mm_shuffle_ps(v256, v256, 255), l0);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v255, v255, 255), l0), linA);
				linB = _mm_sub_ps(linB, v257);
				__m128 v258 = _mm_mul_ps(v256, a1);
				angA = _mm_add_ps(_mm_mul_ps(v255, a0), angA);
				angB = _mm_add_ps(v258, angB);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v254));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				pNextType = copyByte(&uType, &reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type);
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (*pNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_MOTOR);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_PULLEY: {
			hkpJacobian1dPulleySchema* pS = reinterpret_cast<hkpJacobian1dPulleySchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_linear[0].m_quad;
			const __m128& l1 = pS->m_jac.m_linear[1].m_quad;
			const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
			const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
			__m128 v261 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
				_mm_sub_ps(_mm_mul_ps(l0, linA), _mm_mul_ps(l1, linB)));
			hkSimdReal sDot;
			// z + (y + x) in the first lane, z in the others.
			const __m128 v261z = _mm_shuffle_ps(v261, v261, 170);
			sDot.m_real = _mm_move_ss(v261z, _mm_add_ps(v261z, _mm_add_ps(_mm_shuffle_ps(v261, v261, 85), v261)));
			hkSimdReal sRhs;
			sRhs.m_real = _mm_shuffle_ps(l0, l0, 255);
			hkSimdReal sDelta = sRhs - sDot;
			hkSimdReal sInvDiag;
			sInvDiag.m_real = _mm_shuffle_ps(a0, a0, 255);
			hkSimdReal sImpulse = sDelta * sInvDiag;
			__m128 v263 = _mm_shuffle_ps(sImpulse.m_real, sImpulse.m_real, 0);
			__m128 v264 = _mm_mul_ps(invB, v263);
			__m128 v265 = _mm_mul_ps(invA, v263);
			__m128 v266 = _mm_mul_ps(l1, _mm_shuffle_ps(v264, v264, 255));
			linA = _mm_add_ps(linA, _mm_mul_ps(l0, _mm_shuffle_ps(v265, v265, 255)));
			linB = _mm_sub_ps(linB, v266);
			__m128 v267 = _mm_mul_ps(a0, v265);
			__m128 v268 = _mm_mul_ps(a1, v264);
			hkSimdReal sApplied;
			sApplied.m_real = _mm_set_ss(tmp->m_deltaRhs);
			angA = _mm_add_ps(angA, v267);
			angB = _mm_add_ps(angB, v268);
			hkSimdReal sSum = sApplied + sImpulse;
			tmp->m_deltaRhs = sSum.m_real[0];
			++tmp;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR: {
			uint8_t uNextType;
			do {
				hkpJacobian1dAngularSchema* pS = reinterpret_cast<hkpJacobian1dAngularSchema*>(pSchema);
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				__m128 m_quad_9 = pS->m_jac.m_angular[1].m_quad;
				__m128 v172 = _mm_add_ps(_mm_mul_ps(angB, m_quad_9), _mm_mul_ps(angA, a0));
				const __m128 v173 =
					_mm_add_ps(_mm_shuffle_ps(v172, v172, 170), _mm_add_ps(_mm_shuffle_ps(v172, v172, 85), v172));
				const __m128 v174 =
					_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(m_quad_9, m_quad_9, 255), v173), _mm_shuffle_ps(a0, a0, 255));
				__m128 v176 = _mm_shuffle_ps(v174, v174, 0);
				__m128 v177 = _mm_mul_ps(_mm_mul_ps(v176, invA), a0);
				__m128 v179 = _mm_mul_ps(_mm_mul_ps(v176, invB), m_quad_9);
				angA = _mm_add_ps(v177, angA);
				angB = _mm_add_ps(v179, angB);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v174));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS: {
			uint8_t uNextType;
			do {
				hkpJacobian1dAngularLimitsSchema* pS = reinterpret_cast<hkpJacobian1dAngularLimitsSchema*>(pSchema);
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v284 = a1[3];
				double v143 = pS->m_tau;
				__m128 m_damping_low = _mm_set_ss(info.m_damping);
				__m128 v145 = _mm_sub_ps(angB, s1B);
				__m128 v146 = _mm_mul_ps(a0, _mm_sub_ps(angA, s1A));
				__m128 m_quad_10 = a1;
				__m128 v385 = s1A;
				__m128 v377 = s1B;
				float v311 = v143 * info.m_invIntegrateVelocityFactor.m_quad[0u];
				__m128 v148 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_damping_low, m_damping_low, 0),
											 _mm_add_ps(_mm_mul_ps(v145, m_quad_10), v146)),
					_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v311), _mm_set_ss(v311), 0),
						_mm_add_ps(_mm_mul_ps(v377, a1), _mm_mul_ps(a0, v385))));
				float v354 =
					_mm_add_ps(_mm_shuffle_ps(v148, v148, 170), _mm_add_ps(_mm_shuffle_ps(v148, v148, 85), v148))[0];
				float v356 = v143 * v284 - v354;
				float v313 = -tmp->m_deltaRhs;
				float v285 = v356 - pS->m_minRhs;
				float v286 = a0[3] * v285;
				if (v313 < static_cast<double>(v286)) {
					__m128 v149 = _mm_set_ss(v286);
					__m128 v150 = _mm_shuffle_ps(v149, v149, 0);
					__m128 v151 = _mm_mul_ps(_mm_mul_ps(invB, v150), a1);
					angA = _mm_add_ps(angA, _mm_mul_ps(a0, _mm_mul_ps(invA, v150)));
					angB = _mm_add_ps(v151, angB);
					_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v149));
				} else {
					float v287 = v356 - pS->m_maxRhs;
					float v288 = a0[3] * v287;
					if (v288 < static_cast<double>(v313)) {
						__m128 v149 = _mm_set_ss(v288);
						__m128 v153 = _mm_shuffle_ps(v149, v149, 0);
						__m128 v151 = _mm_mul_ps(_mm_mul_ps(invB, v153), a1);
						angA = _mm_add_ps(_mm_mul_ps(a0, _mm_mul_ps(invA, v153)), angA);
						angB = _mm_add_ps(v151, angB);
						_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v149));
					}
				}
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_LIMITS);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION: {
			uint8_t uNextType;
			do {
				hkpJacobian1dAngularFrictionSchema* pS = reinterpret_cast<hkpJacobian1dAngularFrictionSchema*>(pSchema);
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				double v138;
				__m128 m_quad_11 = a1;
				__m128 v137 = _mm_add_ps(_mm_mul_ps(angB, m_quad_11), _mm_mul_ps(angA, a0));
				float v369 = _mm_sub_ps(_mm_shuffle_ps(m_quad_11, m_quad_11, 255),
					_mm_add_ps(_mm_shuffle_ps(v137, v137, 170), _mm_add_ps(_mm_shuffle_ps(v137, v137, 85), v137)))[0];
				float v283 = a0[3] * v369;
				float v309 = std::fabs(static_cast<double>(v283));
				if (!(pS->m_maxImpulse < static_cast<double>(v309))) {
					v138 = 1.0;
				} else {
					float v310 = pS->m_maxImpulse / v309;
					v138 = v310;
					v283 = v283 * v310;
				}
				tmp[1].m_deltaRhs = v138;
				__m128 v139 = _mm_shuffle_ps(_mm_set_ss(v283), _mm_set_ss(v283), 0);
				__m128 v140 = _mm_mul_ps(_mm_mul_ps(invB, v139), a1);
				angA = _mm_add_ps(angA, _mm_mul_ps(a0, _mm_mul_ps(invA, v139)));
				angB = _mm_add_ps(v140, angB);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v139));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR: {
			uint8_t uNextType;
			do {
				hkpJacobian1dAngularMotorSchema* pS = reinterpret_cast<hkpJacobian1dAngularMotorSchema*>(pSchema);
				const hkp1dMotorSolverInfo& motor = pS->m_motorSolverInfo;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				float v314 = motor.m_velocity * stepTemp.m_intregratePositions.m_storage + tmp[2].m_deltaRhs;
				tmp[2].m_deltaRhs = v314;
				float v155 = motor.m_damping;
				__m128 v156 = _mm_sub_ps(angB, s1B);
				float v289 = a1[3] + v314;
				__m128 v157 = _mm_mul_ps(_mm_sub_ps(angA, s1A), a0);
				__m128 v158 = a1;
				double v159 = motor.m_tau;
				__m128 v375 = s1A;
				__m128 v379 = s1B;
				__m128 v160 = _mm_add_ps(_mm_mul_ps(v156, v158), v157);
				float v315 = v159 * info.m_invIntegrateVelocityFactor.m_quad[0u];
				__m128 v161 = _mm_add_ps(_mm_mul_ps(v379, a1), _mm_mul_ps(v375, a0));
				__m128 v162 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v315), _mm_set_ss(v315), 0), v161);
				v161[0] = v155;
				__m128 v163 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v161, v161, 0), v160), v162);
				float v358 =
					_mm_add_ps(_mm_shuffle_ps(v163, v163, 170), _mm_add_ps(_mm_shuffle_ps(v163, v163, 85), v163))[0];
				float v316 = v159 * v289 - v358;
				float v290 = a0[3] * v316;
				double v164 = v290;
				if (!(motor.m_maxImpulsePerSubstep < static_cast<double>(v290))) {
					if (!(motor.m_minImpulsePerSubstep > v164)) {
						tmp[1].m_deltaRhs = 1.0f;
					} else {
						tmp[1].m_deltaRhs = motor.m_minImpulsePerSubstep / v164;
						v290 = motor.m_minImpulsePerSubstep;
					}
				} else {
					tmp[1].m_deltaRhs = motor.m_maxImpulsePerSubstep / v164;
					v290 = motor.m_maxImpulsePerSubstep;
				}
				__m128 v166 = _mm_shuffle_ps(_mm_set_ss(v290), _mm_set_ss(v290), 0);
				__m128 v167 = _mm_mul_ps(v166, invA);
				__m128 v168 = _mm_mul_ps(_mm_mul_ps(v166, invB), a1);
				angA = _mm_add_ps(_mm_mul_ps(v167, a0), angA);
				angB = _mm_add_ps(v168, angB);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v166));
				tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
				uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_MOTOR);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT_WITH_ACCESSOR_CHECK: {
			do {
				hkpJacobianSchema* pCurrent = pSchema;
				hkpJacobianSingleContactSchema* pS = reinterpret_cast<hkpJacobianSingleContactSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 m_quad_13 = l0;
				__m128 v51 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				const __m128 v51Sum =
					_mm_add_ps(_mm_shuffle_ps(v51, v51, 170), _mm_add_ps(_mm_shuffle_ps(v51, v51, 85), v51));
				const __m128 v52 =
					_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 255), v51Sum), _mm_shuffle_ps(a0, a0, 255));
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
				const __m128 m_quad_14 = _mm_sub_ps(hkQuadReal0000, _mm_load_ss(&tmp->m_deltaRhs));
				__m128 v54 = _mm_max_ps(m_quad_14, v52);
				__m128 v55 = _mm_shuffle_ps(v54, v54, 0);
				__m128 v56 = _mm_mul_ps(invB, v55);
				__m128 v57 = _mm_mul_ps(invA, v55);
				linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v57, v57, 255), m_quad_13));
				linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v56, v56, 255), m_quad_13));
				__m128 v58 = _mm_mul_ps(a1, v56);
				angA = _mm_add_ps(angA, _mm_mul_ps(v57, a0));
				angB = _mm_add_ps(angB, v58);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v54));
				// EXE-INLINED 0xD921D0
				tmp += hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(pCurrent->m_bytes.m_type)];
			} while (pSchema->m_bytes.m_type == hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT_WITH_ACCESSOR_CHECK: {
			uint8_t uNextType;
			do {
				// The contact's maximum impulse is in the w of the second angular row.
				hkpJacobianSingleContactSchema* pS = reinterpret_cast<hkpJacobianSingleContactSchema*>(pSchema);
				const __m128& l0 = pS->m_jac.m_linear0.m_quad;
				const __m128& a0 = pS->m_jac.m_angular[0].m_quad;
				const __m128& a1 = pS->m_jac.m_angular[1].m_quad;
				__m128 v60 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1, angB), _mm_mul_ps(angA, a0)),
					_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
				const __m128 v60Sum =
					_mm_add_ps(_mm_shuffle_ps(v60, v60, 170), _mm_add_ps(_mm_shuffle_ps(v60, v60, 85), v60));
				const __m128 v61 =
					_mm_mul_ps(_mm_sub_ps(_mm_shuffle_ps(l0, l0, 255), v60Sum), _mm_shuffle_ps(a0, a0, 255));
				const __m128 vApplied = _mm_load_ss(&tmp->m_deltaRhs);
				const __m128 m_quad_16 = _mm_sub_ps(hkQuadReal0000, vApplied);
				const __m128 v63 = _mm_sub_ps(_mm_shuffle_ps(a1, a1, 255), vApplied);
				__m128 v64 = _mm_min_ps(v63, _mm_max_ps(m_quad_16, v61));
				__m128 v65 = _mm_shuffle_ps(v64, v64, 0);
				__m128 v66 = _mm_mul_ps(invA, v65);
				__m128 v67 = _mm_mul_ps(invB, v65);
				__m128 v68 = _mm_mul_ps(_mm_shuffle_ps(v67, v67, 255), l0);
				linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v66, v66, 255), l0), linA);
				linB = _mm_sub_ps(linB, v68);
				__m128 v69 = _mm_mul_ps(v67, a1);
				angA = _mm_add_ps(_mm_mul_ps(v66, a0), angA);
				angB = _mm_add_ps(angB, v69);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v64));
				// EXE-INLINED 0xD921D0
				tmp += hkpJacobianSchema::s_schemaNumSolverElemTemps[static_cast<int8_t>(pSchema->m_bytes.m_type)];
				uNextType = reinterpret_cast<hkpJacobianSchema*>(pS + 1)->m_bytes.m_type;
				pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			} while (uNextType == hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT:
		case hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT_WITH_ACCESSOR_CHECK: {
			hkpJacobianPairContactSchema* pS = reinterpret_cast<hkpJacobianPairContactSchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
			const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
			const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
			const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
			__m128 v8 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(angA, a00)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
			float v373 = _mm_sub_ps(_mm_shuffle_ps(l0, l0, 255),
				_mm_add_ps(_mm_shuffle_ps(v8, v8, 170), _mm_add_ps(_mm_shuffle_ps(v8, v8, 85), v8)))[0];
			float v392 = v373;
			__m128 v11 = l1;
			__m128 v12 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), v11));
			float v13 = _mm_sub_ps(_mm_shuffle_ps(v11, v11, 255),
				_mm_add_ps(_mm_shuffle_ps(v12, v12, 170), _mm_add_ps(_mm_shuffle_ps(v12, v12, 85), v12)))[0];
			float v328 = v373 * a01[3] + v13 * pS->m_invJac01;
			float v276 = v373 * pS->m_invJac01 + v13 * a11[3];
			bool bFirstOnly;
			if (!(-tmp->m_deltaRhs < v328)) {
				float v277 = v13 * a10[3];
				bFirstOnly = -tmp[1].m_deltaRhs >= v277;
				if (!bFirstOnly) {
					// The second point alone.
					__m128 v34 = _mm_shuffle_ps(_mm_set_ss(v277), _mm_set_ss(v277), 0);
					__m128 v35 = _mm_mul_ps(invB, v34);
					__m128 v36 = _mm_mul_ps(invA, v34);
					__m128 v37 = l1;
					linA = _mm_add_ps(linA, _mm_mul_ps(v37, _mm_shuffle_ps(v36, v36, 255)));
					linB = _mm_sub_ps(linB, _mm_mul_ps(v37, _mm_shuffle_ps(v35, v35, 255)));
					__m128 v38 = _mm_mul_ps(a10, v36);
					__m128 v39 = _mm_mul_ps(a11, v35);
					angA = _mm_add_ps(angA, v38);
					angB = _mm_add_ps(angB, v39);
					_mm_store_ss(&tmp[1].m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp[1].m_deltaRhs), v34));
				}
			} else {
				bFirstOnly = !(-tmp[1].m_deltaRhs < v276);
				if (!bFirstOnly) {
					// Both points.
					__m128 v14 = _mm_shuffle_ps(_mm_set_ss(v328), _mm_set_ss(v328), 0);
					__m128 v15 = _mm_mul_ps(v14, invA);
					__m128 v16 = _mm_mul_ps(v14, invB);
					__m128 v17 = _mm_mul_ps(_mm_shuffle_ps(v16, v16, 255), l0);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v15, v15, 255), l0), linA);
					linB = _mm_sub_ps(linB, v17);
					__m128 v18 = _mm_mul_ps(v16, a01);
					angA = _mm_add_ps(_mm_mul_ps(v15, a00), angA);
					angB = _mm_add_ps(v18, angB);
					_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v14));
					__m128 v20 = _mm_shuffle_ps(_mm_set_ss(v276), _mm_set_ss(v276), 0);
					__m128 v21 = _mm_mul_ps(v20, invA);
					__m128 v22 = l1;
					__m128 v23 = _mm_mul_ps(v20, invB);
					linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v21, v21, 255), v22), linA);
					linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v23, v23, 255), v22));
					__m128 v24 = _mm_mul_ps(a10, v21);
					__m128 v25 = _mm_mul_ps(a11, v23);
					angA = _mm_add_ps(v24, angA);
					angB = _mm_add_ps(v25, angB);
					_mm_store_ss(&tmp[1].m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp[1].m_deltaRhs), v20));
				}
			}
			if (bFirstOnly) {
				// The first point alone, clamped to a non-negative accumulated impulse.
				__m128 m_quad_17 = l0;
				const __m128 v27 = _mm_mul_ps(_mm_set_ss(v392), _mm_shuffle_ps(a00, a00, 255));
				const __m128 m_quad_18 = _mm_sub_ps(hkQuadReal0000, _mm_load_ss(&tmp->m_deltaRhs));
				__m128 v29 = _mm_max_ps(m_quad_18, v27);
				__m128 v30 = _mm_shuffle_ps(v29, v29, 0);
				__m128 v31 = _mm_mul_ps(v30, invA);
				__m128 v32 = _mm_mul_ps(v30, invB);
				linA = _mm_add_ps(_mm_mul_ps(l0, _mm_shuffle_ps(v31, v31, 255)), linA);
				linB = _mm_sub_ps(linB, _mm_mul_ps(m_quad_17, _mm_shuffle_ps(v32, v32, 255)));
				__m128 v33 = _mm_mul_ps(a01, v32);
				angA = _mm_add_ps(_mm_mul_ps(v31, a00), angA);
				angB = _mm_add_ps(angB, v33);
				_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v29));
			}
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			tmp += 2;
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_2D_FRICTION: {
			hkpJacobian2dFrictionSchema* pS = reinterpret_cast<hkpJacobian2dFrictionSchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
			const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
			const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
			const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
			double v122;
			__m128 v120 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(angA, a00)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
			float v371 = _mm_sub_ps(_mm_shuffle_ps(l0, l0, 255),
				_mm_add_ps(_mm_shuffle_ps(v120, v120, 170), _mm_add_ps(_mm_shuffle_ps(v120, v120, 85), v120)))[0];
			__m128 v121 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
				_mm_mul_ps(l1, _mm_sub_ps(linA, linB)));
			float v363 = _mm_sub_ps(_mm_shuffle_ps(l1, l1, 255),
				_mm_add_ps(_mm_shuffle_ps(v121, v121, 170), _mm_add_ps(_mm_shuffle_ps(v121, v121, 85), v121)))[0];
			float v344 = pS->m_maxImpulse * pS->m_maxImpulse;
			float v335 = a01[3] * v371 + v363 * pS->m_invJac01;
			float v282 = v371 * pS->m_invJac01 + v363 * a11[3];
			float v306 = v282 * v282 + v335 * v335;
			if (!(v344 < static_cast<double>(v306))) {
				v122 = 1.0;
			} else {
				float v307 = v344 / v306;
				float v308 = std::sqrt(static_cast<double>(v307));
				v122 = v308;
				v335 = v308 * v335;
				v282 = v308 * v282;
			}
			tmp[2].m_deltaRhs = v122;
			__m128 v123 = _mm_shuffle_ps(_mm_set_ss(v335), _mm_set_ss(v335), 0);
			__m128 v124 = _mm_mul_ps(invB, v123);
			__m128 v125 = _mm_mul_ps(invA, v123);
			__m128 v126 = _mm_mul_ps(_mm_shuffle_ps(v124, v124, 255), l0);
			linA = _mm_add_ps(linA, _mm_mul_ps(_mm_shuffle_ps(v125, v125, 255), l0));
			linB = _mm_sub_ps(linB, v126);
			__m128 v127 = _mm_add_ps(angA, _mm_mul_ps(a00, v125));
			__m128 v128 = _mm_mul_ps(a01, v124);
			angA = v127;
			angB = _mm_add_ps(angB, v128);
			_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v123));
			__m128 v129 = _mm_shuffle_ps(_mm_set_ss(v282), _mm_set_ss(v282), 0);
			__m128 v130 = _mm_mul_ps(invB, v129);
			__m128 v131 = _mm_mul_ps(invA, v129);
			__m128 v132 = l1;
			linA = _mm_add_ps(linA, _mm_mul_ps(v132, _mm_shuffle_ps(v131, v131, 255)));
			linB = _mm_sub_ps(linB, _mm_mul_ps(v132, _mm_shuffle_ps(v130, v130, 255)));
			__m128 v133 = _mm_mul_ps(a10, v131);
			__m128 v134 = _mm_mul_ps(a11, v130);
			angA = _mm_add_ps(angA, v133);
			angB = _mm_add_ps(angB, v134);
			_mm_store_ss(&tmp[1].m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp[1].m_deltaRhs), v129));
			tmp += 3;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_3D_FRICTION: {
			hkpJacobian3dFrictionSchema* pS = reinterpret_cast<hkpJacobian3dFrictionSchema*>(pSchema);
			const __m128& l0 = pS->m_jac.m_jac0.m_linear0.m_quad;
			const __m128& a00 = pS->m_jac.m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac.m_jac0.m_angular[1].m_quad;
			const __m128& l1 = pS->m_jac.m_jac1.m_linear0.m_quad;
			const __m128& a10 = pS->m_jac.m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac.m_jac1.m_angular[1].m_quad;
			const __m128& a20 = pS->m_jac.m_jac2.m_angular[0].m_quad;
			const __m128& a21 = pS->m_jac.m_jac2.m_angular[1].m_quad;
			double v102;
			__m128 v97 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a01, angB), _mm_mul_ps(angA, a00)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), l0));
			float v359 = _mm_sub_ps(_mm_shuffle_ps(l0, l0, 255),
				_mm_add_ps(_mm_shuffle_ps(v97, v97, 170), _mm_add_ps(_mm_shuffle_ps(v97, v97, 85), v97)))[0];
			__m128 v98 = l1;
			__m128 v99 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a11, angB), _mm_mul_ps(a10, angA)),
				_mm_mul_ps(_mm_sub_ps(linA, linB), v98));
			float v367 = _mm_sub_ps(_mm_shuffle_ps(v98, v98, 255),
				_mm_add_ps(_mm_shuffle_ps(v99, v99, 170), _mm_add_ps(_mm_shuffle_ps(v99, v99, 85), v99)))[0];
			__m128 v100 = _mm_add_ps(_mm_mul_ps(a21, angB), _mm_mul_ps(angA, a20));
			float v361 = _mm_sub_ps(_mm_shuffle_ps(a21, a21, 255),
				_mm_add_ps(_mm_shuffle_ps(v100, v100, 170), _mm_add_ps(_mm_shuffle_ps(v100, v100, 85), v100)))[0];
			float v333 = v361 * a20[3];
			float v343 = a01[3] * v359 + pS->m_invJac01 * v367;
			float v281 = v359 * pS->m_invJac01 + v367 * a11[3];
			float v301 = v281 * v281 + v343 * v343 + v333 * v333;
			double v101 = v301;
			float v302 = pS->m_maxImpulse * pS->m_maxImpulse;
			if (!(v302 < v101)) {
				v102 = 1.0;
			} else {
				float v303 = std::sqrt(v101);
				float v304 = 1.0 / v303;
				float v305 = v304 * pS->m_maxImpulse;
				v102 = v305;
				v343 = v305 * v343;
				v281 = v305 * v281;
				v333 = v305 * v333;
			}
			tmp[3].m_deltaRhs = v102;
			__m128 m_quad_20 = l0;
			float v334 = pS->m_contactRadius * v333;
			__m128 v104 = _mm_shuffle_ps(_mm_set_ss(v343), _mm_set_ss(v343), 0);
			__m128 v105 = _mm_mul_ps(v104, invA);
			__m128 v106 = _mm_mul_ps(v104, invB);
			linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v105, v105, 255), l0), linA);
			linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v106, v106, 255), m_quad_20));
			__m128 v107 = _mm_mul_ps(v106, a01);
			angA = _mm_add_ps(_mm_mul_ps(v105, a00), angA);
			angB = _mm_add_ps(v107, angB);
			_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v104));
			__m128 v109 = l1;
			__m128 v110 = _mm_shuffle_ps(_mm_set_ss(v281), _mm_set_ss(v281), 0);
			__m128 v111 = _mm_mul_ps(v110, invA);
			__m128 v112 = _mm_mul_ps(v110, invB);
			linA = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v111, v111, 255), v109), linA);
			linB = _mm_sub_ps(linB, _mm_mul_ps(_mm_shuffle_ps(v112, v112, 255), v109));
			__m128 v113 = _mm_mul_ps(a10, v111);
			__m128 v114 = _mm_mul_ps(a11, v112);
			angA = _mm_add_ps(v113, angA);
			angB = _mm_add_ps(v114, angB);
			_mm_store_ss(&tmp[1].m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp[1].m_deltaRhs), v110));
			__m128 v115 = _mm_shuffle_ps(_mm_set_ss(v334), _mm_set_ss(v334), 0);
			__m128 v116 = _mm_mul_ps(v115, invA);
			__m128 v117 = _mm_mul_ps(a21, _mm_mul_ps(v115, invB));
			angA = _mm_add_ps(_mm_mul_ps(v116, a20), angA);
			angB = _mm_add_ps(v117, angB);
			_mm_store_ss(&tmp[2].m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp[2].m_deltaRhs), v115));
			tmp += 4;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_2D_ROLLING_FRICTION: {
			hkpJacobian2dRollingFrictionSchema* pS = reinterpret_cast<hkpJacobian2dRollingFrictionSchema*>(pSchema);
			const __m128& a00 = pS->m_jac0.m_angular[0].m_quad;
			const __m128& a01 = pS->m_jac0.m_angular[1].m_quad;
			const __m128& a10 = pS->m_jac1.m_angular[0].m_quad;
			const __m128& a11 = pS->m_jac1.m_angular[1].m_quad;
			double v88;
			float v298 = pS->m_maxImpulse * pS->m_maxImpulse;
			__m128 v85 = _mm_add_ps(_mm_mul_ps(angB, a01), _mm_mul_ps(angA, a00));
			double v86 = a00[3];
			float v365 = _mm_sub_ps(_mm_shuffle_ps(a01, a01, 255),
				_mm_add_ps(_mm_shuffle_ps(v85, v85, 170), _mm_add_ps(_mm_shuffle_ps(v85, v85, 85), v85)))[0];
			float v280 = v86 * v365;
			__m128 v87 = _mm_add_ps(_mm_mul_ps(angB, a11), _mm_mul_ps(angA, a10));
			float v357 = _mm_sub_ps(_mm_shuffle_ps(a11, a11, 255),
				_mm_add_ps(_mm_shuffle_ps(v87, v87, 170), _mm_add_ps(_mm_shuffle_ps(v87, v87, 85), v87)))[0];
			float v332 = a10[3] * v357;
			float v342 = v332 * v332 + v280 * v280;
			if (!(v298 < static_cast<double>(v342))) {
				v88 = 1.0;
			} else {
				float v299 = v298 / v342;
				float v300 = std::sqrt(static_cast<double>(v299));
				v88 = v300;
				v280 = v300 * v280;
				v332 = v300 * v332;
			}
			tmp[2].m_deltaRhs = v88;
			__m128 v89 = _mm_shuffle_ps(_mm_set_ss(v280), _mm_set_ss(v280), 0);
			__m128 v90 = _mm_mul_ps(_mm_mul_ps(invB, v89), a01);
			angA = _mm_add_ps(angA, _mm_mul_ps(a00, _mm_mul_ps(invA, v89)));
			angB = _mm_add_ps(v90, angB);
			_mm_store_ss(&tmp->m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp->m_deltaRhs), v89));
			__m128 v92 = _mm_shuffle_ps(_mm_set_ss(v332), _mm_set_ss(v332), 0);
			__m128 v93 = _mm_mul_ps(_mm_mul_ps(invA, v92), a10);
			__m128 v94 = _mm_mul_ps(_mm_mul_ps(invB, v92), a11);
			angA = _mm_add_ps(v93, angA);
			angB = _mm_add_ps(angB, v94);
			_mm_store_ss(&tmp[1].m_deltaRhs, _mm_add_ps(_mm_load_ss(&tmp[1].m_deltaRhs), v92));
			tmp += getNumSolverElemTemps(reinterpret_cast<char*>(pSchema));
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_MASS: {
			hkpJacobianSetMassSchema* pS = reinterpret_cast<hkpJacobianSetMassSchema*>(pSchema);
			invA = pS->m_invMass0.m_quad;
			invB = pS->m_invMass1.m_quad;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_ADD_VELOCITY: {
			hkpJacobianAddVelocitySchema* pS = reinterpret_cast<hkpJacobianAddVelocitySchema*>(pSchema);
			// Body A or B by m_bodyIndex (0 or 1), picked without a branch as the game does.
			const uintptr_t uAccA = reinterpret_cast<uintptr_t>(pAccA);
			const uintptr_t uMask = static_cast<uintptr_t>(-static_cast<intptr_t>(pS->m_bodyIndex));
			hkpVelocityAccumulator* pAcc = reinterpret_cast<hkpVelocityAccumulator*>(uAccA ^
				(uMask & (reinterpret_cast<uintptr_t>(pAccB) ^ uAccA)));
			float x = info.m_integrateVelocityFactor.m_quad[0u] * static_cast<double>(stepTemp.m_currentStep.m_storage);
			hkSimdReal sFactor(x);
			__m128 m_quad_21 = pS->m_deltaVel.m_quad;
			__m128 m_real = sFactor.m_real;
			pAcc->m_linearVel.m_quad = _mm_add_ps(pAcc->m_linearVel.m_quad, pS->m_deltaVel.m_quad);
			__m128 v273 = _mm_mul_ps(_mm_shuffle_ps(m_real, m_real, 0), m_quad_21);
			pAcc->m_linearVel.m_quad = _mm_add_ps(v273, pAcc->m_linearVel.m_quad);
			pAcc->m_scratch0.m_quad = _mm_add_ps(v273, pAcc->m_scratch0.m_quad);
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_SET_CENTER_OF_MASS: {
			hkpJacobianSetCenterOfMassSchema* pS = reinterpret_cast<hkpJacobianSetCenterOfMassSchema*>(pSchema);
			hkVector4 linVelA;
			hkVector4 linVelB;
			hkVector4 scratchA;
			hkVector4 scratchB;
			linVelA.setMul3(pS->m_angToLinVelA, pAccA->m_angularVel);
			linVelB.setMul3(pS->m_angToLinVelB, pAccB->m_angularVel);
			scratchA.setMul3(pS->m_angToLinVelA, pAccA->m_scratch123.m_col0);
			scratchB.setMul3(pS->m_angToLinVelB, pAccB->m_scratch123.m_col0);
			linA = _mm_add_ps(linA, linVelA.m_quad);
			s0A = _mm_add_ps(scratchA.m_quad, s0A);
			linB = _mm_add_ps(linB, linVelB.m_quad);
			s0B = _mm_add_ps(s0B, scratchB.m_quad);
			pSchema = reinterpret_cast<hkpJacobianSchema*>(pS + 1);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_STIFF_SPRING_CHAIN: {
			hkpJacobianStiffSpringChainSchema* pS = &pSchema->m_stiffSpringChain;
			hkSolver_solveStiffSpringChain(info, stepTemp, accumulators, pS, tmp);
			tmp += pS->m_numConstraints;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_BALL_SOCKET_CHAIN: {
			hkpJacobianBallSocketChainSchema* pS = &pSchema->m_ballSocketChain;
			hkSolver_solveBallAndSocketChain(info, stepTemp, accumulators, pS, tmp);
			tmp += 3 * pS->m_numConstraints;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
			continue;
		}
		case hkpJacobianSchema::SCHEMA_TYPE_POWERED_CHAIN: {
			hkpJacobianPoweredChainSchema* pS = reinterpret_cast<hkpJacobianPoweredChainSchema*>(pSchema);
			hkSolver_solvePoweredChain(info, stepTemp, accumulators, pS, tmp);
			tmp += 6 * pS->m_numConstraints;
			pSchema = reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(pSchema) + pS->m_size);
			continue;
		}
		default: {
			// Assert on an unknown schema type: trap, then return.
			__builtin_debugtrap();
			return;
		}
		}
	}
}

// One accumulator's sub-step of the solver's velocity integration: the velocity change since the last sub-step
// (velocity minus the sums kept in m_scratch0 and m_scratch123's first column), scaled by
// info.m_integrateVelocityFactor's x, goes into the sums. On the last step the sums are scaled by factor and the
// velocities become the changes; else the velocities are the changes plus the new sums, the linear one plus gravity
// times m_gravityFactor.
void updateSumVelocities(const hkVector4& factor, const hkpSolverInfo& info, bool lastStep, hkpVelocityAccumulator& acc,
	const hkVector4& gravity) {
	const __m128 gravityStep = _mm_mul_ps(_mm_set1_ps(acc.m_gravityFactor), gravity.m_quad);
	const __m128 linearChange = _mm_sub_ps(acc.m_linearVel.m_quad, acc.m_scratch0.m_quad);
	const __m128 angularChange = _mm_sub_ps(acc.m_angularVel.m_quad, acc.m_scratch123.m_col0.m_quad);
	acc.m_scratch0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
													  info.m_integrateVelocityFactor.m_quad, 0),
										   linearChange),
		acc.m_scratch0.m_quad);
	acc.m_scratch123.m_col0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(info.m_integrateVelocityFactor.m_quad,
															   info.m_integrateVelocityFactor.m_quad, 0),
													angularChange),
		acc.m_scratch123.m_col0.m_quad);
	if (lastStep) {
		acc.m_scratch0.m_quad = _mm_mul_ps(acc.m_scratch0.m_quad, factor.m_quad);
		acc.m_scratch123.m_col0.m_quad = _mm_mul_ps(factor.m_quad, acc.m_scratch123.m_col0.m_quad);
		acc.m_angularVel.m_quad = angularChange;
		acc.m_linearVel.m_quad = linearChange;
		return;
	}
	acc.m_angularVel.m_quad = _mm_add_ps(acc.m_scratch123.m_col0.m_quad, angularChange);
	acc.m_linearVel.m_quad = _mm_add_ps(_mm_add_ps(linearChange, acc.m_scratch0.m_quad), gravityStep);
}
