#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Reconstruction.hpp"

class hkp1Lin2AngJacobian;
class hkp2AngJacobian;
struct hkpChainSolverInfo;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpVelocityAccumulator;

// What hkPoweredChainBuildJacobian is given (hkpconstraintsolver.pdb; the SDK ships no header). The exe's 0xDF21C0
// reads the count at 0, tau and damping at 4/8, the cfm terms from 0xC, the accumulators at 0x1C/0x20, the motor
// states at 0x24, the hysteresis at 0x28 and the status buffer at 0x2C.
struct hkpPoweredChainBuildJacobianParams {
	int32_t m_numConstraints;
	float m_chainTau;
	float m_chainDamping;
	hkpCfmParam m_cfm;
	hkpVelocityAccumulatorOffset* m_accumulators;
	const hkpVelocityAccumulator* m_accumsBase;
	hkp3dAngularMotorSolverInfo* m_motorsState;
	float m_maxTorqueHysterisys;
	uint8_t* m_childConstraintStatusWriteBackBuffer;
	hkp2AngJacobian* m_jacobiansEnd;
};

// Takes the disabled motors of one constraint out of the chain matrix: for each motor with either of its two state
// bits set (`test al, 3` / `0xC` / `0x30`), zeroes its row (3 + motor) of mtxDiag with a 1 on the diagonal and, when
// disableNextConstraint is set, the same row of mtxNextOffdiag. The exe reads the flag as a byte only
// (`mov dl, [esp+0xC]`), its callers' upper bytes ignored.
void hkPoweredChain_DisableMotorInMatrixRow_ThisConstraint(const hkp3dAngularMotorSolverInfo::Status* status,
	uint8_t disableNextConstraint, hkMatrix6& mtxDiag, hkMatrix6& mtxNextOffdiag);
// Not tied to an address by the PDB; named after the PDB's function of hkpPoweredChainSolverUtil.cpp that it is: the
// module's symbols run ..._ThisConstraint, ..._NextConstraint, ..._OverwriteVelocityWithExplicitImpulse,
// ..._ComputeConstraintMatrixLuDecomposition_ForOneRow, the order of 0xDF06A0, 0xDF07C0, 0xDF0810 and 0xDF0870, and it
// is called right after ..._ThisConstraint (0xDF09F4) with the previous off-diagonal. Zeroes column (3 + motor) of
// mtxPrevOffdiag for each disabled motor.
void hkPoweredChain_DisableMotorInMatrixRow_NextConstraint(const hkp3dAngularMotorSolverInfo::Status* motorsState,
	hkMatrix6& mtxNextOffdiag);
// Writes the impulse bound a broken motor hit (the maximum for MOTOR_BROKEN_MAX_BREACHED, else the minimum) into the
// constraint's angular velocity component.
void hkPoweredChain_OverwriteVelocityWithExplicitImpulse(int32_t modifiedConstraintIndex,
	int32_t modifiedCoordinateIndex, const hkp3dAngularMotorSolverInfo* motorsState, hkVector8* velocities);
// Not tied to an address by the PDB; the PDB's function of hkpPoweredChainSolverUtil.cpp that follows
// ..._OverwriteVelocityWithExplicitImpulse (0xDF0810) in the module's symbols, whose locals (mtxDiag,
// mtxNextOffdiag, tmp, tmp2) are the exe's four hkMatrix6 locals; it calls ..._ThisConstraint and ..._NextConstraint.
// Its callers (0xDF19E0, 0xDF2040) store the flag as a byte, which it hands on to ..._ThisConstraint; returns
// &triple->m_upper, which it also stores in *triplePrevUpper.
const hkMatrix6* hkPoweredChain_ComputeConstraintMatrixLuDecomposition_ForOneRow(uint8_t disableNextConstraint,
	hkpConstraintChainMatrix6Triple* triple, const hkp3dAngularMotorSolverInfo::Status* status,
	hkMatrix6& mtxPrevOffdiag, const hkMatrix6** triplePrevUpper);
// Re-enables the broken motor (MOTOR_BROKEN_MAX_BREACHED or MOTOR_BROKEN_MIN_BREACHED) whose chain velocity has come
// back furthest past its motor target, scanning from the last constraint to the first, and returns its constraint and
// coordinate; both stay as given when none qualifies. Not tied to an address by the PDB; the PDB's function of
// hkpPoweredChainSolverUtil.cpp with its locals (impRight, impCenter, impLeft, offDiagTransp, resultVelocityVec).
// The exe's body reads only the first three arguments; its only caller, hkSolver_solvePoweredChain (0xD8A3A0), pushes
// info.m_motorsState as the last (the PDB's motorsState).
void hkPoweredChain_ScanAndEnableMotors(const hkpChainSolverInfo& info, int32_t& modifiedConstraintIndex,
	int32_t& modifiedCoordinateIndex, float& impulse, hkp3dAngularMotorSolverInfo* motorsState);
// Fills each triple's m_mtxDiag and m_mtxNextOffdiag from the Jacobians and the accumulators' inverse masses, then
// runs ..._ComputeConstraintMatrixLuDecomposition_ForOneRow on each row. The PDB names it
// (hkpPoweredChainSolverUtil.cpp) with bufferSize among its locals' names but no types; the parameters are the nine
// dwords the exe's only caller, hkPoweredChainBuildJacobian (0xDF21C0), pushes (cdecl, ret 0); the exe's body never
// reads bufferSize (numConstraints * 0x3C0) and leaves no result its caller reads.
void hkPoweredChain_BuildConstraintMatrixAndLuDecomposition(int32_t numConstraints, const hkpCfmParam& cfm,
	hkp3dAngularMotorSolverInfo* motorsState, const hkp1Lin2AngJacobian* j, const hkp2AngJacobian* jAng,
	const hkpVelocityAccumulatorOffset* va, const hkpVelocityAccumulator* accumsBase,
	hkpConstraintChainMatrix6Triple* triples, int32_t bufferSize);
// Redoes the chain matrix's LU decomposition from the row before firstModifiedConstraint (row 0 at least) to the
// last, after a motor's state changed. Not tied to an address by the PDB; the PDB's function of
// hkpPoweredChainSolverUtil.cpp whose locals (mtxPrevOffdiag, triplePrevUpper) are the exe's two, and the exe's only
// caller, hkSolver_solvePoweredChain (0xD8A547, 0xD8A611), calls it after the motor scans with the modified
// constraint, the count, the motor states and the matrix triples; neither call site reads eax.
void hkPoweredChain_UpdateLuDecomposition(int32_t firstConstraintToProcess, int32_t numConstraints,
	const hkp3dAngularMotorSolverInfo* motorsState, hkpConstraintChainMatrix6Triple* triples);
// PDB (hkpPoweredChainSolverUtil.cpp): the powered chain's schema from the params the chain data gathered, the query
// input and output. The exe's only caller (hkpPoweredChainData::buildJacobian, 0xE3C192) pushes the params' address,
// the original query input and the output; it advances out.m_jacobianSchemas by the schema's size and returns that
// size in eax (cdecl, ret 0).
int32_t hkPoweredChainBuildJacobian(const hkpPoweredChainBuildJacobianParams& params, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out);
// Not named in the PDB or IDA; name chosen here. Back-substitutes the chain's impulses from info.m_gTempBuffer and
// finds the working motor whose impulse breaches its bounds (times 1.05) the most: marks it broken (max or min),
// writes its indices and impulse to constraintIndex, motorIndex and impulse, and returns the constraint index (-1,
// outputs untouched, when none breaches). The exe's only caller, hkSolver_solvePoweredChain (0xD8A3A0), pushes
// info.m_motorsState as a fifth argument, which the body never reads.
int32_t hkPoweredChain_ScanAndDisableMotors(const hkpChainSolverInfo& info, int32_t& constraintIndex,
	int32_t& motorIndex, float& impulse, hkp3dAngularMotorSolverInfo* motorsState);
