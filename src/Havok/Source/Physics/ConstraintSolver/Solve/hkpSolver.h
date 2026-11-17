#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkBool;
class hkVector4;
class hkpJacobianPoweredChainSchema;
class hkpJacobianSchema;
class hkpJacobianStiffSpringChainSchema;
class hkpSolverElemTemp;
struct hkpSolverInfo;
class hkpVelocityAccumulator;

namespace hkSolver {
struct SolverStepTemp;
}

// Uploads the solver's code on the SPU platforms; the PC's is an empty function the exe still
// calls (cdecl, ret 0, 0xD892D0).
void hkSolveUpload();
// Runs one constraint batch: the schemas of the batch's task over its
// accumulators for one solver step and micro step, writing the solver temporaries. Its only caller is
// hkCpuSolveConstraintBatchJob (cdecl, 6 stack arguments, 0xD8FB70).
int32_t hkSolveConstraintsByTheSteps(const hkpSolverInfo& info, int32_t currentSolverStep,
	int32_t currentSolverMicroStep, const hkpJacobianSchema* schemas, hkpVelocityAccumulator* accumulators,
	hkpSolverElemTemp* elemTemp);
// The SDK's export of the solver results: the impulses and right-hand sides of the solved
// schemas written back from the solver temporaries (hkpSolve.h; the PDB's file hkpSolverExport.cpp). The exe's copy is
// one jmp to its body at 0xDEF570 (cdecl, ret 0, 0xDF0680); hkpConstraintSolverSetup::solve (0xD88B50),
// hkCpuSolveExportResultsJob (0xD94730) and hkCpuSolveConstraintsJob (0xD947E0) call it.
void hkExportImpulsesAndRhs(const hkpSolverInfo& i, const hkpSolverElemTemp* temp, const hkpJacobianSchema* schemas,
	const hkpVelocityAccumulator* accums);
void sub_DF0690();
void hkDebugPrintfAccumulators();
// The SDK's hkpSolve.h signature; defined in hkpSolver.cpp.
int32_t hkSolveConstraints(const hkpSolverInfo& info, hkpJacobianSchema* schemas, hkpVelocityAccumulator* accumulators,
	hkpSolverElemTemp* temp);
// The solver's case for a stiff spring chain schema (schema type 29 in
// hkSolveConstraintsByTheSteps). PDB hkSolver_solveStiffSpringChain (hkpSolver.cpp; its S_GPROC32 sits between
// hkSolveUpload and hkSolver_solveBallAndSocketChain as 0xD893A0 does in the exe; cdecl, ret 0).
void hkSolver_solveStiffSpringChain(const hkpSolverInfo& info, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianStiffSpringChainSchema* chain, hkpSolverElemTemp* sr);
// The solver's case for a powered chain schema (type 31). PDB hkSolver_solvePoweredChain (hkpSolver.cpp; its
// S_GPROC32 follows hkSolveIntegrateVelocitiesByTheSteps as 0xD8A3A0 follows 0xD89F80; cdecl, ret 0).
void hkSolver_solvePoweredChain(const hkpSolverInfo& solverInfo, const hkSolver::SolverStepTemp& temp,
	hkpVelocityAccumulator* accums, hkpJacobianPoweredChainSchema* chain, hkpSolverElemTemp* sr);
// The SDK's hkpSolve.h signature; the exe's caller hkCpuSolveApplyGravityJob (0xD944D0)
// ignores eax (cdecl, ret 0, 0xD89ED0).
void hkSolveApplyGravityByTheSteps(const hkpSolverInfo& info, hkpVelocityAccumulator* accumulators,
	hkpVelocityAccumulator* accumulatorsEnd);
// The SDK's hkpSolve.h signature.
int32_t hkSolveIntegrateVelocitiesByTheSteps(const hkpSolverInfo& info, int32_t currentSolverStep,
	hkpVelocityAccumulator* accumulators, hkpVelocityAccumulator* accumulatorsEnd, hkBool& outSolveFinished);
// The SDK's hkpSolve.h signature.
void hkSolveStepJacobians(const hkpSolverInfo& info, const hkpJacobianSchema* schemas,
	hkpVelocityAccumulator* accumulators, hkpSolverElemTemp* tmp);
// Not named in the PDB or IDA; name chosen here. Integrates one accumulator's sub-step (usercall, ret 0: factor in
// edx, info and lastStep on the stack, acc in eax, gravity in esi; 0xD892F0). info's +0x30 read and acc's fields fit
// hkpSolverInfo::m_integrateVelocityFactor and hkpVelocityAccumulator's layout; lastStep is a byte test.
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
