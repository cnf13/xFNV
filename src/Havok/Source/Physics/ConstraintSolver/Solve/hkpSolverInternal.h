#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkp1Lin2AngJacobian;
class hkp2AngJacobian;
struct hkp3dAngularMotorSolverInfo;
struct hkpConstraintChainMatrix6Triple;
struct hkpSolverInfo;
class hkpVelocityAccumulator;
class hkpVelocityAccumulatorOffset;
class hkVector8;

// The solver's internal records (hkpconstraintsolver.pdb; the SDK ships no header). hkSolver is a namespace there
// (no class record).
namespace hkSolver {

// The step hkSolveConstraints is on and whether it integrates positions in it.
struct SolverStepTemp {
	hkPadSpu<int32_t> m_currentStep;
	hkPadSpu<float> m_intregratePositions;
};

}  // namespace hkSolver

// What the chain solvers work on: the solver settings, the chain's size, tau and damping, and where its Jacobians,
// accumulator offsets, matrices, motor states and temp buffers are in the schema stream.
struct hkpChainSolverInfo {
	const hkpSolverInfo& m_solverInfo;
	hkPadSpu<int32_t> m_numConstraints;
	hkPadSpu<float> m_schemaTau;
	hkPadSpu<float> m_schemaDamping;
	hkPadSpu<hkp1Lin2AngJacobian*> m_j;
	hkPadSpu<hkp2AngJacobian*> m_jAng;
	hkPadSpu<hkpVelocityAccumulatorOffset*> m_va;
	hkPadSpu<hkpVelocityAccumulator*> m_accumsBase;
	hkPadSpu<hkpConstraintChainMatrix6Triple*> m_triples;
	hkPadSpu<hkp3dAngularMotorSolverInfo*> m_motorsState;
	hkPadSpu<hkVector8*> m_gTempBuffer;
	hkPadSpu<hkVector8*> m_velocitiesBuffer;
};
