#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix6.h"

// The chain solvers' records (ball-socket, stiff-spring and powered chains). The SDK ships no header for them;
// members from hkpconstraintsolver.pdb.

// A body's velocity accumulator as a byte offset from the accumulators' base. hkpBallSocketChainData::buildJacobian
// pushes one 4-byte offset per body into its hkArray.
class hkpVelocityAccumulatorOffset {
public:
	uint32_t m_offset;
};

// One row of the stiff-spring chain's tridiagonal system.
struct hkpConstraintChainTriple {
	float m_lower;
	float m_diagonal;
	float m_upper;
};

// One block row of the ball-socket chain's block-tridiagonal system.
struct hkpConstraintChainMatrixTriple {
	hkMatrix3 m_lower;
	hkMatrix3 m_diagonalInv;
	hkMatrix3 m_upper;
};

// One block row of the powered chain's six-dimensional system.
struct hkpConstraintChainMatrix6Triple {
	hkMatrix6 m_lower;
	hkMatrix6 m_diagonalInv;
	hkMatrix6 m_upper;
	hkMatrix6 m_mtxDiag;
	hkMatrix6 m_mtxNextOffdiag;
};

// The constraint-force-mixing terms added to and multiplied into the chain matrix's linear and angular diagonals.
struct hkpCfmParam {
	float m_linAdd;
	float m_linMul;
	float m_angAdd;
	float m_angMul;
};
