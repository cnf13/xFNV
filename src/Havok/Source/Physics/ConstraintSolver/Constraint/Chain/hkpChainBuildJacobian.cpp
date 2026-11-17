#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainBuildJacobian.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianBallSocketChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianStiffSpringChainSchema.h"

#include <xmmintrin.h>

// KEEP: an uninitialised read. The diagonal block is built in a local hkMatrix3 whose nine 3x3 floats are written
// (0xDD0CA3, 0xDD0CA8) but not the w lanes of its three columns; the exe copies the three whole columns into the
// triple's m_diagonalInv (movaps, 0xDD0E59-0xDD0E7C), so stack garbage reaches the schema (and the inversion,
// 0xCD55A0).
// The block's dot products are x87 copies of an SSE float, written as plain float stores.
void hkBallSocketChainBuildJacobian(int32_t numConstraints, float tau, float damping, float cfm,
	const hkpVelocityAccumulatorOffset* accumulators, const hkpVelocityAccumulator* accumBase,
	hkp1Lin2AngJacobian* jacobiansEnd, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {
	hkpJacobianSchema* pSchemaUnit = out.m_jacobianSchemas;
	hkpJacobianBallSocketChainSchema* pSchema = &pSchemaUnit->m_ballSocketChain;
	// EXE-INLINED none: hkpJacobianBallSocketChainSchema::initBallSocketChainSchema
	pSchemaUnit->m_quad[0] = pSchemaUnit->m_quad[0] + 1.1920928955078125e-07;
	pSchema->m_tau = tau;
	pSchema->m_damping = damping;
	pSchema->m_size = hkpJacobianBallSocketChainSchema::getSize(static_cast<uint16_t>(numConstraints));
	pSchemaUnit->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_BALL_SOCKET_CHAIN;
	pSchema->m_numConstraints = static_cast<uint16_t>(numConstraints);
	hkpVelocityAccumulatorOffset* pOffsetsOut = pSchema->getAccumulatorOffsetsBase();
	for (int32_t i = 0; i < numConstraints + 1; ++i) {
		pOffsetsOut[i] = accumulators[i];
	}

	hkp1Lin2AngJacobian* pJacobians = hkpJacobianBallSocketChainSchema::getJacobians(out.m_jacobianSchemas);
	const hkpVelocityAccumulatorOffset* pOffsets = pSchema->getAccumulatorOffsetsBase();
	hkpConstraintChainMatrixTriple* pTriple = pSchema->getMatrixBuffer();
	const __m128 kZero = _mm_setzero_ps();
	hkMatrix3 kPrevOffDiag;
	kPrevOffDiag.m_col0.m_quad = kZero;
	kPrevOffDiag.m_col1.m_quad = kZero;
	kPrevOffDiag.m_col2.m_quad = kZero;
	hkMatrix3 kPrevUpper;
	kPrevUpper.m_col0.m_quad = kZero;
	kPrevUpper.m_col1.m_quad = kZero;
	kPrevUpper.m_col2.m_quad = kZero;

	for (int32_t c = 0; c < numConstraints; ++c, ++pTriple) {
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccA =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumBase) +
				pOffsets[c].m_offset);
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccB =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumBase) +
				pOffsets[c + 1].m_offset);
		const hkp1Lin2AngJacobian* pJac = &pJacobians[3 * c];

		// The symmetric diagonal block: J_i M^-1 J_j^T, plus cfm on the diagonal.
		hkMatrix3 kDiag;
		for (int32_t i = 0; i < 3; ++i) {
			const __m128 invMassA = pAccA->m_invMasses.m_quad;
			const __m128 invMassB = pAccB->m_invMasses.m_quad;
			for (int32_t j = i; j < 3; ++j) {
				const __m128 lin = _mm_mul_ps(pJac[j].m_linear0.m_quad, pJac[i].m_linear0.m_quad);
				const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(invMassA, invMassA, 0xFF), lin),
												  _mm_mul_ps(_mm_shuffle_ps(invMassB, invMassB, 0xFF), lin)),
					_mm_add_ps(_mm_mul_ps(_mm_mul_ps(pJac[j].m_angular[0].m_quad, pJac[i].m_angular[0].m_quad),
								   invMassA),
						_mm_mul_ps(_mm_mul_ps(pJac[i].m_angular[1].m_quad, pJac[j].m_angular[1].m_quad), invMassB)));
				const float fDot = _mm_shuffle_ps(sum, sum, 0xAA)[0] + static_cast<float>(sum[1] + sum[0]);
				kDiag(i, j) = fDot;
				kDiag(j, i) = fDot;
			}
			kDiag(i, i) = kDiag(i, i) + cfm;
		}

		// The off-diagonal block to the next constraint, zero for the last one.
		hkMatrix3 kOffDiag;
		kOffDiag.m_col0.m_quad = kZero;
		kOffDiag.m_col1.m_quad = kZero;
		kOffDiag.m_col2.m_quad = kZero;
		if (c < numConstraints - 1) {
			const hkp1Lin2AngJacobian* pNext = &pJacobians[3 * (c + 1)];
			for (int32_t k = 0; k < 3; ++k) {
				for (int32_t m = 0; m < 3; ++m) {
					const __m128 invMassB = pAccB->m_invMasses.m_quad;
					const __m128 v =
						_mm_sub_ps(_mm_mul_ps(_mm_mul_ps(pNext[m].m_angular[0].m_quad, pJac[k].m_angular[1].m_quad),
									   invMassB),
							_mm_mul_ps(_mm_shuffle_ps(invMassB, invMassB, 0xFF),
								_mm_mul_ps(pNext[m].m_linear0.m_quad, pJac[k].m_linear0.m_quad)));
					// EXE-INLINED 0x5770E0
					kOffDiag(k, m) = _mm_shuffle_ps(v, v, 0xAA)[0] + static_cast<float>(v[1] + v[0]);
				}
			}
		}

		// The lower block is the previous off-diagonal block transposed, with zero w lanes.
		for (int32_t r = 0; r < 3; ++r) {
			for (int32_t col = 0; col < 3; ++col) {
				pTriple->m_lower(r, col) = kPrevOffDiag(col, r);
			}
		}
		pTriple->m_lower.m_col0.m_quad[3] = 0.0f;
		pTriple->m_lower.m_col1.m_quad[3] = 0.0f;
		pTriple->m_lower.m_col2.m_quad[3] = 0.0f;
		hkMatrix3 kProduct;
		hkMatrix3_setMulMat3Mat3(&kProduct, &pTriple->m_lower, &kPrevUpper);
		hkMatrix3& rDiagInv = pTriple->m_diagonalInv;
		rDiagInv.m_col0.m_quad = kDiag.m_col0.m_quad;
		rDiagInv.m_col1.m_quad = kDiag.m_col1.m_quad;
		rDiagInv.m_col2.m_quad = kDiag.m_col2.m_quad;
		rDiagInv.m_col0.m_quad = _mm_sub_ps(rDiagInv.m_col0.m_quad, kProduct.m_col0.m_quad);
		rDiagInv.m_col1.m_quad = _mm_sub_ps(rDiagInv.m_col1.m_quad, kProduct.m_col1.m_quad);
		rDiagInv.m_col2.m_quad = _mm_sub_ps(rDiagInv.m_col2.m_quad, kProduct.m_col2.m_quad);
		// Keep the diagonal above epsilon before inverting.
		for (int32_t i = 0; i < 3; ++i) {
			float fValue = rDiagInv(i, i);
			if (!(fValue > 1.1920929e-07f)) {
				fValue = 1.1920929e-07f;
			}
			rDiagInv(i, i) = fValue;
		}
		hkMatrix3_invertSymmetric(&rDiagInv);
		hkMatrix3_setMulMat3Mat3(&pTriple->m_upper, &rDiagInv, &kOffDiag);
		kPrevOffDiag = kOffDiag;
		kPrevUpper = pTriple->m_upper;
	}
	// EXE-INLINED none: hkAddByteOffset<hkpJacobianSchema>
	out.m_jacobianSchemas =
		reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(out.m_jacobianSchemas) + pSchema->m_size);
}

// The diagonal's value is x87 copies of an SSE float and reloads of the stored diagonal,
// written as plain float stores and reads.
void hkStiffSpringChainBuildJacobian(int32_t numConstraints, float tau, float damping, float cfm,
	const hkpVelocityAccumulatorOffset* accumulators, const hkpVelocityAccumulator* accumBase,
	hkp1Lin2AngJacobian* jacobiansEnd, const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {
	hkpJacobianSchema* pSchemaUnit = out.m_jacobianSchemas;
	hkpJacobianStiffSpringChainSchema* pSchema = &pSchemaUnit->m_stiffSpringChain;
	// EXE-INLINED none: hkpJacobianStiffSpringChainSchema::initStiffSpringChainSchema
	pSchemaUnit->m_quad[0] = pSchemaUnit->m_quad[0] + 1.1920928955078125e-07;
	pSchema->m_tau = tau;
	pSchema->m_damping = damping;
	pSchema->m_size = hkpJacobianStiffSpringChainSchema::getSize(static_cast<uint16_t>(numConstraints));
	pSchemaUnit->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_STIFF_SPRING_CHAIN;
	pSchema->m_numConstraints = static_cast<uint16_t>(numConstraints);
	hkpVelocityAccumulatorOffset* pOffsetsOut = pSchema->getAccumulatorOffsetsBase();
	for (int32_t i = 0; i < numConstraints + 1; ++i) {
		pOffsetsOut[i] = accumulators[i];
	}

	const hkpVelocityAccumulatorOffset* pOffsets = pSchema->getAccumulatorOffsetsBase();
	hkpConstraintChainTriple* pTriple = pSchema->getMatrixBuffer();
	const hkp1Lin2AngJacobian* pJac = hkpJacobianStiffSpringChainSchema::getJacobians(out.m_jacobianSchemas);
	// The previous row's off-diagonal and its ratio to the previous diagonal (the LU decomposition's lower factor).
	float fPrevOffDiag = 0.0f;
	float fPrevRatio = 0.0f;
	for (int32_t c = 0; c < numConstraints; ++c, ++pTriple, ++pJac) {
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccB =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumBase) +
				pOffsets[c + 1].m_offset);
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccA =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumBase) +
				pOffsets[c].m_offset);
		const __m128 invMassA = pAccA->m_invMasses.m_quad;
		const __m128 invMassB = pAccB->m_invMasses.m_quad;

		// The angular terms in x, y and z and the summed linear inverse masses in w, added up.
		const __m128 angular =
			_mm_add_ps(_mm_mul_ps(_mm_mul_ps(pJac->m_angular[1].m_quad, pJac->m_angular[1].m_quad), invMassB),
				_mm_mul_ps(_mm_mul_ps(pJac->m_angular[0].m_quad, pJac->m_angular[0].m_quad), invMassA));
		const __m128 masses = _mm_add_ps(invMassA, invMassB);
		const __m128 terms = _mm_shuffle_ps(angular, _mm_unpackhi_ps(angular, masses), 0xC4);
		const __m128 pairs = _mm_add_ps(_mm_shuffle_ps(terms, terms, 0x4E), terms);
		const float fEffMass = static_cast<float>(pairs[1] + pairs[0]) + 1.1920928955078125e-07;
		const float fDiag = fEffMass + cfm;

		float fOffDiag = 0.0f;
		if (c < numConstraints - 1) {
			const hkp1Lin2AngJacobian* pNext = pJac + 1;
			const __m128 v =
				_mm_sub_ps(_mm_mul_ps(_mm_mul_ps(pNext->m_angular[0].m_quad, pJac->m_angular[1].m_quad), invMassB),
					_mm_mul_ps(_mm_mul_ps(pNext->m_linear0.m_quad, pJac->m_linear0.m_quad),
						_mm_shuffle_ps(invMassB, invMassB, 0xFF)));
			fOffDiag = _mm_shuffle_ps(v, v, 0xAA)[0] + static_cast<float>(v[1] + v[0]);
		}

		pTriple->m_lower = fPrevOffDiag;
		pTriple->m_diagonal = fDiag - fPrevOffDiag * fPrevRatio;
		float fClamped = pTriple->m_diagonal;
		if (!(fClamped > 1.1920929e-07f)) {
			fClamped = 1.1920929e-07f;
		}
		pTriple->m_diagonal = fClamped;
		const float fRatio = fOffDiag / pTriple->m_diagonal;
		pTriple->m_upper = fRatio;
		fPrevRatio = fRatio;
		fPrevOffDiag = fOffDiag;
	}
	// EXE-INLINED none: hkAddByteOffset<hkpJacobianSchema>
	out.m_jacobianSchemas =
		reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(out.m_jacobianSchemas) + pSchema->m_size);
}

// hkBallSocketChainBuildJacobian's block decomposition (0xDD0AD0 inlines the same code): per constraint the 3x3
// diagonal block of J M^-1 J^T plus cfm, the off-diagonal block to the next constraint, and the block LU factors in
// place.
// KEEP: an uninitialised read. The diagonal block is built in a local hkMatrix3 whose nine 3x3 floats are written
// (0xDD07AC, 0xDD07B1) but not the w lanes of its three columns; the exe copies the three whole columns into the
// triple's m_diagonalInv (movaps, 0xDD095B-0xDD097E), so stack garbage reaches the triples (and the inversion,
// 0xCD55A0).
// The block's dot products are x87 copies of an SSE float, written as plain float stores.
void buildBlockTridiagonalLu(hkpConstraintChainMatrixTriple* triples, int32_t numConstraints, float cfm,
	const hkp1Lin2AngJacobian* jacobians, const hkpVelocityAccumulatorOffset* accumulators,
	const hkpVelocityAccumulator* accumsBase) {
	const __m128 kZero = _mm_setzero_ps();
	hkMatrix3 kPrevOffDiag;
	kPrevOffDiag.m_col0.m_quad = kZero;
	kPrevOffDiag.m_col1.m_quad = kZero;
	kPrevOffDiag.m_col2.m_quad = kZero;
	hkMatrix3 kPrevUpper;
	kPrevUpper.m_col0.m_quad = kZero;
	kPrevUpper.m_col1.m_quad = kZero;
	kPrevUpper.m_col2.m_quad = kZero;

	for (int32_t c = 0; c < numConstraints; ++c) {
		hkpConstraintChainMatrixTriple* pTriple = &triples[c];
		// The accumulator offsets are byte offsets into the untyped accumulator buffer.
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccA =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumsBase) +
				accumulators[c].m_offset);
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccB =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumsBase) +
				accumulators[c + 1].m_offset);
		const hkp1Lin2AngJacobian* pJac = &jacobians[3 * c];

		// The symmetric diagonal block: J_i M^-1 J_j^T, plus cfm on the diagonal.
		hkMatrix3 kDiag;
		for (int32_t i = 0; i < 3; ++i) {
			const __m128 invMassA = pAccA->m_invMasses.m_quad;
			const __m128 invMassB = pAccB->m_invMasses.m_quad;
			for (int32_t j = i; j < 3; ++j) {
				const __m128 lin = _mm_mul_ps(pJac[j].m_linear0.m_quad, pJac[i].m_linear0.m_quad);
				const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(invMassA, invMassA, 0xFF), lin),
												  _mm_mul_ps(_mm_shuffle_ps(invMassB, invMassB, 0xFF), lin)),
					_mm_add_ps(_mm_mul_ps(_mm_mul_ps(pJac[j].m_angular[0].m_quad, pJac[i].m_angular[0].m_quad),
								   invMassA),
						_mm_mul_ps(_mm_mul_ps(pJac[j].m_angular[1].m_quad, pJac[i].m_angular[1].m_quad), invMassB)));
				const float fDot =
					_mm_add_ss(_mm_shuffle_ps(sum, sum, 0xAA), _mm_add_ss(_mm_shuffle_ps(sum, sum, 0x55), sum))[0];
				kDiag(i, j) = fDot;
				kDiag(j, i) = fDot;
			}
			kDiag(i, i) = kDiag(i, i) + cfm;
		}

		// The off-diagonal block to the next constraint, zero for the last one.
		hkMatrix3 kOffDiag;
		kOffDiag.m_col0.m_quad = kZero;
		kOffDiag.m_col1.m_quad = kZero;
		kOffDiag.m_col2.m_quad = kZero;
		if (c < numConstraints - 1) {
			const hkp1Lin2AngJacobian* pNext = &jacobians[3 * (c + 1)];
			for (int32_t k = 0; k < 3; ++k) {
				for (int32_t m = 0; m < 3; ++m) {
					const __m128 invMassB = pAccB->m_invMasses.m_quad;
					const __m128 v =
						_mm_sub_ps(_mm_mul_ps(_mm_mul_ps(pNext[m].m_angular[0].m_quad, pJac[k].m_angular[1].m_quad),
									   invMassB),
							_mm_mul_ps(_mm_shuffle_ps(invMassB, invMassB, 0xFF),
								_mm_mul_ps(pNext[m].m_linear0.m_quad, pJac[k].m_linear0.m_quad)));
					// EXE-INLINED 0x5770E0
					kOffDiag(k, m) =
						_mm_add_ss(_mm_shuffle_ps(v, v, 0xAA), _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v))[0];
				}
			}
		}

		// The lower block is the previous off-diagonal block transposed, with zero w lanes, stored in the exe's order
		// (0xDD08FC-0xDD094F).
		hkMatrix3& rLower = pTriple->m_lower;
		rLower(0, 0) = kPrevOffDiag(0, 0);
		rLower(1, 1) = kPrevOffDiag(1, 1);
		rLower(2, 2) = kPrevOffDiag(2, 2);
		rLower.m_col0.m_quad[3] = 0.0f;
		rLower.m_col1.m_quad[3] = 0.0f;
		rLower.m_col2.m_quad[3] = 0.0f;
		rLower(1, 0) = kPrevOffDiag(0, 1);
		rLower(0, 1) = kPrevOffDiag(1, 0);
		rLower(2, 0) = kPrevOffDiag(0, 2);
		rLower(0, 2) = kPrevOffDiag(2, 0);
		rLower(2, 1) = kPrevOffDiag(1, 2);
		rLower(1, 2) = kPrevOffDiag(2, 1);
		hkMatrix3 kProduct;
		hkMatrix3_setMulMat3Mat3(&kProduct, &pTriple->m_lower, &kPrevUpper);
		hkMatrix3& rDiagInv = pTriple->m_diagonalInv;
		rDiagInv.m_col0.m_quad = kDiag.m_col0.m_quad;
		rDiagInv.m_col1.m_quad = kDiag.m_col1.m_quad;
		rDiagInv.m_col2.m_quad = kDiag.m_col2.m_quad;
		rDiagInv.m_col0.m_quad = _mm_sub_ps(rDiagInv.m_col0.m_quad, kProduct.m_col0.m_quad);
		rDiagInv.m_col1.m_quad = _mm_sub_ps(rDiagInv.m_col1.m_quad, kProduct.m_col1.m_quad);
		rDiagInv.m_col2.m_quad = _mm_sub_ps(rDiagInv.m_col2.m_quad, kProduct.m_col2.m_quad);
		// Keep the diagonal above epsilon before inverting.
		for (int32_t i = 0; i < 3; ++i) {
			float fValue = rDiagInv(i, i);
			if (!(fValue > 1.1920929e-07f)) {
				fValue = 1.1920929e-07f;
			}
			rDiagInv(i, i) = fValue;
		}
		hkMatrix3_invertSymmetric(&rDiagInv);
		hkMatrix3_setMulMat3Mat3(&pTriple->m_upper, &rDiagInv, &kOffDiag);
		kPrevOffDiag = kOffDiag;
		kPrevUpper = pTriple->m_upper;
	}
}

// hkStiffSpringChainBuildJacobian's decomposition (0xDD1180 inlines the same code): per constraint the diagonal of
// J M^-1 J^T plus cfm, the off-diagonal to the next constraint, and the LU factors in place. The diagonal's reload
// (0xDD1114) and its clamp's copies (0xDD1127, 0xDD1131, 0xDD1137) are x87 copies of floats, written as float stores.
void buildTridiagonalLu(hkpConstraintChainTriple* triples, const hkp1Lin2AngJacobian* jacobians, int32_t numConstraints,
	float cfm, const hkpVelocityAccumulatorOffset* accumulators, const hkpVelocityAccumulator* accumsBase) {
	// The previous row's off-diagonal and its ratio to the previous diagonal (the LU decomposition's lower factor).
	float fPrevOffDiag = 0.0f;
	float fPrevRatio = 0.0f;
	for (int32_t c = 0; c < numConstraints; ++c) {
		const hkp1Lin2AngJacobian* pJac = &jacobians[c];
		hkpConstraintChainTriple* pTriple = &triples[c];
		// The accumulator offsets are byte offsets into the untyped accumulator buffer.
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccB =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumsBase) +
				accumulators[c + 1].m_offset);
		// EXE-INLINED none: hkAddByteOffsetConst<hkpVelocityAccumulator>
		const hkpVelocityAccumulator* pAccA =
			reinterpret_cast<const hkpVelocityAccumulator*>(reinterpret_cast<const char*>(accumsBase) +
				accumulators[c].m_offset);
		const __m128 invMassA = pAccA->m_invMasses.m_quad;
		const __m128 invMassB = pAccB->m_invMasses.m_quad;

		// The angular terms in x, y and z and the summed linear inverse masses in w, added up.
		const __m128 angular =
			_mm_add_ps(_mm_mul_ps(_mm_mul_ps(pJac->m_angular[0].m_quad, pJac->m_angular[0].m_quad), invMassA),
				_mm_mul_ps(_mm_mul_ps(pJac->m_angular[1].m_quad, pJac->m_angular[1].m_quad), invMassB));
		const __m128 masses = _mm_add_ps(invMassA, invMassB);
		const __m128 terms = _mm_shuffle_ps(angular, _mm_unpackhi_ps(angular, masses), 0xC4);
		const __m128 pairs = _mm_add_ps(_mm_shuffle_ps(terms, terms, 0x4E), terms);
		const float fSum = _mm_add_ps(_mm_shuffle_ps(pairs, pairs, 0xB1), pairs)[0];
		const float fEffMass = fSum + 1.1920928955078125e-07;
		const float fDiag = fEffMass + cfm;

		float fOffDiag = 0.0f;
		if (c < numConstraints - 1) {
			const hkp1Lin2AngJacobian* pNext = pJac + 1;
			const __m128 v =
				_mm_sub_ps(_mm_mul_ps(_mm_mul_ps(pNext->m_angular[0].m_quad, pJac->m_angular[1].m_quad), invMassB),
					_mm_mul_ps(_mm_mul_ps(pNext->m_linear0.m_quad, pJac->m_linear0.m_quad),
						_mm_shuffle_ps(invMassB, invMassB, 0xFF)));
			fOffDiag = _mm_add_ss(_mm_shuffle_ps(v, v, 0xAA), _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v))[0];
		}

		pTriple->m_lower = fPrevOffDiag;
		pTriple->m_diagonal = fDiag - fPrevOffDiag * fPrevRatio;
		float fClamped = pTriple->m_diagonal;
		if (!(fClamped > 1.1920929e-07f)) {
			fClamped = 1.1920929e-07f;
		}
		pTriple->m_diagonal = fClamped;
		const float fRatio = fOffDiag / pTriple->m_diagonal;
		pTriple->m_upper = fRatio;
		fPrevRatio = fRatio;
		fPrevOffDiag = fOffDiag;
	}
}
