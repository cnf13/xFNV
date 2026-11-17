#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"

#include <stdint.h>

class hkBool;
class hkpJacobianPoweredChainSchema;
class hkpJacobianSchema;
class hkpJacobianStiffSpringChainSchema;
class hkpSolverElemTemp;
struct hkpSolverInfo;
class hkpVelocityAccumulator;
class hkVector4;

namespace hkSolver {
struct SolverStepTemp;
}

// Uploads the solver's code on the SPU platforms; on PC it is an empty function that is still called.
void hkSolveUpload();
// Runs one constraint batch: the schemas of the batch's task over its accumulators for one solver step and micro step,
// writing the solver temporaries. Its only caller is hkCpuSolveConstraintBatchJob.
int32_t hkSolveConstraintsByTheSteps(const hkpSolverInfo& info, int32_t currentSolverStep,
	int32_t currentSolverMicroStep, const hkpJacobianSchema* schemas, hkpVelocityAccumulator* accumulators,
	hkpSolverElemTemp* elemTemp);
// Exports the solver results: the impulses and right-hand sides of the solved schemas written back from the solver
// temporaries. hkpConstraintSolverSetup::solve, hkCpuSolveExportResultsJob and hkCpuSolveConstraintsJob call it.
void hkExportImpulsesAndRhs(const hkpSolverInfo& i, const hkpSolverElemTemp* temp, const hkpJacobianSchema* schemas,
	const hkpVelocityAccumulator* accums);
void sub_DF0690();
void hkDebugPrintfAccumulators();
// The SDK's hkpSolve.h signature; defined in hkpSolver.cpp.
int32_t hkSolveConstraints(const hkpSolverInfo& info, hkpJacobianSchema* schemas, hkpVelocityAccumulator* accumulators,
	hkpSolverElemTemp* temp);
// The solver's case for a stiff spring chain schema (schema type 29 in hkSolveConstraintsByTheSteps).
void hkSolver_solveStiffSpringChain(const hkpSolverInfo& info, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianStiffSpringChainSchema* chain, hkpSolverElemTemp* sr);
// The solver's case for a powered chain schema (type 31).
void hkSolver_solvePoweredChain(const hkpSolverInfo& solverInfo, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianPoweredChainSchema* chain, hkpSolverElemTemp* sr);
void hkSolveApplyGravityByTheSteps(const hkpSolverInfo& info, hkpVelocityAccumulator* accumulators,
	hkpVelocityAccumulator* accumulatorsEnd);
// The SDK's hkpSolve.h signature.
int32_t hkSolveIntegrateVelocitiesByTheSteps(const hkpSolverInfo& info, int32_t currentSolverStep,
	hkpVelocityAccumulator* accumulators, hkpVelocityAccumulator* accumulatorsEnd, hkBool& outSolveFinished);
// The SDK's hkpSolve.h signature.
void hkSolveStepJacobians(const hkpSolverInfo& info, const hkpJacobianSchema* schemas,
	hkpVelocityAccumulator* accumulators, hkpSolverElemTemp* tmp);
// Integrates one accumulator's sub-step.
void updateSumVelocities(const hkVector4& factor, const hkpSolverInfo& info, bool lastStep, hkpVelocityAccumulator& acc,
	const hkVector4& gravity);

inline uint8_t HK_flyingcolors_solver;

// The solver's velocity bound (1e6 in each lane) the integrate steps
// compare the absolute linear and angular velocities against (hkSolveIntegrateVelocitiesByTheSteps and its kin).
inline hkQuadReal rdata_10D3DA0{1e6f, 1e6f, 1e6f, 1e6f};

namespace hkSolverExport {

// Havok SDK signature (hkpConstraintSolver.lib, tied by reloc).
void exportImpulsesAndRhs(const hkpSolverInfo& a1, const hkpSolverElemTemp* a2, const hkpJacobianSchema* a3,
	const hkpVelocityAccumulator* a4);

}  // namespace hkSolverExport
