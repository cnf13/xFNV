#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpChainConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"

class hkp1Lin2AngJacobian;
class hkp2AngJacobian;
struct hkpChainSolverInfo;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpVelocityAccumulator;

// What hkPoweredChainBuildJacobian is given.
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

// Takes the disabled motors of one constraint out of the chain matrix: for each motor with either of its two state bits
// set, zeroes its row (3 + motor) of mtxDiag with a 1 on the diagonal and, when disableNextConstraint is set, the same
// row of mtxNextOffdiag. The flag is read as a byte only, so callers' upper bytes are ignored.
void hkPoweredChain_DisableMotorInMatrixRow_ThisConstraint(
    const hkp3dAngularMotorSolverInfo::Status* status,
    uint8_t disableNextConstraint,
    hkMatrix6& mtxDiag,
    hkMatrix6& mtxNextOffdiag
);
// Zeroes column (3 + motor) of the off-diagonal matrix for each disabled motor.
void hkPoweredChain_DisableMotorInMatrixRow_NextConstraint(
    const hkp3dAngularMotorSolverInfo::Status* motorsState,
    hkMatrix6& mtxNextOffdiag
);
// Writes the impulse bound a broken motor hit (the maximum for MOTOR_BROKEN_MAX_BREACHED, else the minimum) into the
// constraint's angular velocity component.
void hkPoweredChain_OverwriteVelocityWithExplicitImpulse(
    int32_t modifiedConstraintIndex,
    int32_t modifiedCoordinateIndex,
    const hkp3dAngularMotorSolverInfo* motorsState,
    hkVector8* velocities
);
// Calls ..._ThisConstraint and ..._NextConstraint. The disableNextConstraint flag is a byte, which it hands on to
// ..._ThisConstraint; returns &triple->m_upper, which it also stores in *triplePrevUpper.
const hkMatrix6* hkPoweredChain_ComputeConstraintMatrixLuDecomposition_ForOneRow(
    uint8_t disableNextConstraint,
    hkpConstraintChainMatrix6Triple* triple,
    const hkp3dAngularMotorSolverInfo::Status* status,
    hkMatrix6& mtxPrevOffdiag,
    const hkMatrix6** triplePrevUpper
);
// Re-enables the broken motor (MOTOR_BROKEN_MAX_BREACHED or MOTOR_BROKEN_MIN_BREACHED) whose chain velocity has come
// back furthest past its motor target, scanning from the last constraint to the first, and returns its constraint and
// coordinate; both stay as given when none qualifies. The body reads only the first three arguments; its only caller,
// hkSolver_solvePoweredChain, passes info.m_motorsState as the last.
void hkPoweredChain_ScanAndEnableMotors(
    const hkpChainSolverInfo& info,
    int32_t& modifiedConstraintIndex,
    int32_t& modifiedCoordinateIndex,
    float& impulse,
    hkp3dAngularMotorSolverInfo* motorsState
);
// Fills each triple's m_mtxDiag and m_mtxNextOffdiag from the Jacobians and the accumulators' inverse masses, then runs
// ..._ComputeConstraintMatrixLuDecomposition_ForOneRow on each row. bufferSize is never read, and the function leaves
// no result its caller reads.
void hkPoweredChain_BuildConstraintMatrixAndLuDecomposition(
    int32_t numConstraints,
    const hkpCfmParam& cfm,
    hkp3dAngularMotorSolverInfo* motorsState,
    const hkp1Lin2AngJacobian* j,
    const hkp2AngJacobian* jAng,
    const hkpVelocityAccumulatorOffset* va,
    const hkpVelocityAccumulator* accumsBase,
    hkpConstraintChainMatrix6Triple* triples,
    int32_t bufferSize
);
// Redoes the chain matrix's LU decomposition from the row before firstModifiedConstraint (row 0 at least) to the last,
// after a motor's state changed. Its only caller, hkSolver_solvePoweredChain, calls it after the motor scans with the
// modified constraint, the count, the motor states and the matrix triples.
void hkPoweredChain_UpdateLuDecomposition(
    int32_t firstConstraintToProcess,
    int32_t numConstraints,
    const hkp3dAngularMotorSolverInfo* motorsState,
    hkpConstraintChainMatrix6Triple* triples
);
// Builds the powered chain's schema from the params the chain data gathered, the query input and output. The only
// caller is hkpPoweredChainData::buildJacobian, which passes the params, the original query input and the output; it
// advances out.m_jacobianSchemas by the schema's size and returns that size.
int32_t hkPoweredChainBuildJacobian(
    const hkpPoweredChainBuildJacobianParams& params,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
);
// Back-substitutes the chain's impulses from info.m_gTempBuffer and finds the working motor whose impulse breaches its
// bounds (times 1.05) the most: marks it broken (max or min), writes its indices and impulse to constraintIndex,
// motorIndex and impulse, and returns the constraint index (-1, outputs untouched, when none breaches). The only
// caller, hkSolver_solvePoweredChain, passes info.m_motorsState as motorsState, which the body never reads.
int32_t hkPoweredChain_ScanAndDisableMotors(
    const hkpChainSolverInfo& info,
    int32_t& constraintIndex,
    int32_t& motorIndex,
    float& impulse,
    hkp3dAngularMotorSolverInfo* motorsState
);
