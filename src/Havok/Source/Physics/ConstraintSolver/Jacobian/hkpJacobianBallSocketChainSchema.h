#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"

class hkpJacobianSchema;

// The head of a ball-socket chain's schema: the chain's size in bytes and its tau and damping. The Jacobians, the
// matrix and temp buffers and the accumulator offsets follow it in the stream (hkpconstraintsolver.pdb; the SDK ships
// no header).
class hkpJacobianBallSocketChainSchema {
public:
	hkpConstraintChainMatrixTriple* getMatrixBuffer() {
		return reinterpret_cast<hkpConstraintChainMatrixTriple*>(reinterpret_cast<char*>(this) +
			sizeof(hkpJacobianBallSocketChainSchema) + m_numConstraints * 3 * sizeof(hkp1Lin2AngJacobian));
	}

	hkVector4* getTempBuffer() { return reinterpret_cast<hkVector4*>(getMatrixBuffer() + m_numConstraints); }

	hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() {
		return reinterpret_cast<hkpVelocityAccumulatorOffset*>(getTempBuffer() + m_numConstraints + 1);
	}

	// For readers of a const schema (hkSolverExport::exportImpulsesAndRhs).
	const hkpVelocityAccumulatorOffset* getAccumulatorOffsetsBase() const {
		return const_cast<hkpJacobianBallSocketChainSchema*>(this)->getAccumulatorOffsetsBase();
	}

	// After the head come three Jacobian rows per constraint, then the matrix buffer (one block triple per constraint),
	// the temp buffer (one vector per body) and the accumulator offsets (one per body). The schema stream is raw memory
	// laid out by byte offsets, hence the casts.
	static hkp1Lin2AngJacobian* getJacobians(hkpJacobianSchema* schema) {
		return reinterpret_cast<hkp1Lin2AngJacobian*>(reinterpret_cast<char*>(schema) +
			sizeof(hkpJacobianBallSocketChainSchema));
	}

	// The schema's size for auNumConstraints constraints, up to the next multiple of 16.
	static uint32_t getSize(uint16_t auNumConstraints) {
		const uint32_t uiBytes = static_cast<uint32_t>(sizeof(hkpJacobianBallSocketChainSchema) +
			auNumConstraints *
				(3 * sizeof(hkp1Lin2AngJacobian) + sizeof(hkpConstraintChainMatrixTriple) + sizeof(hkVector4) +
					sizeof(hkpVelocityAccumulatorOffset)) +
			sizeof(hkVector4) + sizeof(hkpVelocityAccumulatorOffset));
		return (uiBytes + 15) & ~15u;
	}

	uint16_t m_padding;
	uint16_t m_numConstraints;
	uint32_t m_size;
	float m_tau;
	float m_damping;
};
