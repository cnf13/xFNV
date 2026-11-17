#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

class hkpJacobianSchema;

// The head of a stiff-spring chain's schema: the chain's size in bytes and its tau and damping. The Jacobians, the
// matrix and temp buffers and the accumulator offsets follow it in the stream (hkpconstraintsolver.pdb; the SDK ships
// no header).
class hkpJacobianStiffSpringChainSchema {
public:
	hkpConstraintChainTriple* getMatrixBuffer() {
		return reinterpret_cast<hkpConstraintChainTriple*>(reinterpret_cast<char*>(this) +
			sizeof(hkpJacobianStiffSpringChainSchema) + m_numConstraints * sizeof(hkp1Lin2AngJacobian));
	}

	float* getTempBuffer() { return reinterpret_cast<float*>(getMatrixBuffer() + m_numConstraints); }

	hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() {
		return reinterpret_cast<hkpVelocityAccumulatorOffset*>(getTempBuffer() + m_numConstraints + 1);
	}

	// For readers of a const schema (hkSolverExport::exportImpulsesAndRhs).
	const hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() const {
		return const_cast<hkpJacobianStiffSpringChainSchema*>(this)->getAccumulatorOffsetsBase();
	}

	// After the head come one Jacobian row per constraint, then the matrix buffer (one triple per constraint), the temp
	// buffer (one float per body) and the accumulator offsets (one per body). The schema stream is raw memory laid out
	// by byte offsets, hence the casts.
	static hkp1Lin2AngJacobian* getJacobians(hkpJacobianSchema* schema) {
		return reinterpret_cast<hkp1Lin2AngJacobian*>(reinterpret_cast<char*>(schema) +
			sizeof(hkpJacobianStiffSpringChainSchema));
	}

	// The schema's size for auNumConstraints constraints, up to the next multiple of 16.
	static uint32_t getSize(uint16_t auNumConstraints) {
		const uint32_t uiBytes = static_cast<uint32_t>(sizeof(hkpJacobianStiffSpringChainSchema) +
			auNumConstraints *
				(sizeof(hkp1Lin2AngJacobian) + sizeof(hkpConstraintChainTriple) + sizeof(float) +
					sizeof(hkpVelocityAccumulatorOffset)) +
			sizeof(float) + sizeof(hkpVelocityAccumulatorOffset));
		return (uiBytes + 15) & ~15u;
	}

	uint16_t m_padding;
	uint16_t m_numConstraints;
	uint32_t m_size;
	float m_tau;
	float m_damping;
};
