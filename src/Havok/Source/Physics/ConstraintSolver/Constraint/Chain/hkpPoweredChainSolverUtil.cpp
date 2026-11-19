#include "Havok/Source/Physics/ConstraintSolver/Constraint/Chain/hkpPoweredChainSolverUtil.h"

#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPoweredChainSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInternal.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <xmmintrin.h>

// hkMatrix6 subtraction: a1 -= a2, column by column; returns a1.
static hkMatrix6* subMatrix6(hkMatrix6& a1, const hkMatrix6& a2);
// The SDK's
// hkMatrix6SetMul(hkMatrix6& out, const hkMatrix6& a, const hkMatrix6& b) (hkMatrix6.h): each block of out is the sum
// of two hkMatrix3 products of a's row blocks and b's column blocks.
// Rename candidate: hkMatrix6SetMul.
static void mulMatrix6(hkMatrix6& out, const hkMatrix6& a, const hkMatrix6& b);

// Each of a1's twelve columns minus a2's, in memory order.
static hkMatrix6* subMatrix6(hkMatrix6& a1, const hkMatrix6& a2) {
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            hkMatrix3& rOut = a1.m_m[i][j];
            const hkMatrix3& rIn = a2.m_m[i][j];
            rOut.m_col0.m_quad = _mm_sub_ps(rOut.m_col0.m_quad, rIn.m_col0.m_quad);
            rOut.m_col1.m_quad = _mm_sub_ps(rOut.m_col1.m_quad, rIn.m_col1.m_quad);
            rOut.m_col2.m_quad = _mm_sub_ps(rOut.m_col2.m_quad, rIn.m_col2.m_quad);
        }
    }
    return &a1;
}

// Computes out[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j]. The second product is written straight into out and the
// first one, computed into a local, is added to it, column by column.
static void mulMatrix6(hkMatrix6& out, const hkMatrix6& a, const hkMatrix6& b) {
    hkMatrix3 apResult;
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            hkMatrix3& rOut = out.m_m[i][j];
            hkMatrix3_setMulMat3Mat3(&apResult, &a.m_m[i][0], &b.m_m[0][j]);
            hkMatrix3_setMulMat3Mat3(&rOut, &a.m_m[i][1], &b.m_m[1][j]);
            rOut.m_col0.m_quad = _mm_add_ps(rOut.m_col0.m_quad, apResult.m_col0.m_quad);
            rOut.m_col1.m_quad = _mm_add_ps(rOut.m_col1.m_quad, apResult.m_col1.m_quad);
            rOut.m_col2.m_quad = _mm_add_ps(apResult.m_col2.m_quad, rOut.m_col2.m_quad);
        }
    }
}

void hkPoweredChain_DisableMotorInMatrixRow_ThisConstraint(
    const hkp3dAngularMotorSolverInfo::Status* status,
    uint8_t disableNextConstraint,
    hkMatrix6& mtxDiag,
    hkMatrix6& mtxNextOffdiag
) {
    if (*status) {
        for (int32_t motor = 0; motor < 3; motor++) {
            if (*status & (hkp3dAngularMotorSolverInfo::ANGULAR_1ST << (2 * motor))) {
                // Row 3 + motor of the 6x6 matrix: row `motor` of the lower blocks, then 1 on the diagonal.
                for (int32_t c = 0; c < 3; c++) {
                    mtxDiag.m_m[1][0](motor, c) = 0.0f;
                }
                for (int32_t c = 0; c < 3; c++) {
                    mtxDiag.m_m[1][1](motor, c) = 0.0f;
                }
                mtxDiag.m_m[1][1](motor, motor) = 1.0f;
                if (disableNextConstraint) {
                    for (int32_t c = 0; c < 3; c++) {
                        mtxNextOffdiag.m_m[1][0](motor, c) = 0.0f;
                    }
                    for (int32_t c = 0; c < 3; c++) {
                        mtxNextOffdiag.m_m[1][1](motor, c) = 0.0f;
                    }
                }
            }
        }
    }
}

void hkPoweredChain_DisableMotorInMatrixRow_NextConstraint(
    const hkp3dAngularMotorSolverInfo::Status* motorsState,
    hkMatrix6& mtxNextOffdiag
) {
    if (*motorsState) {
        for (int32_t motor = 0; motor < 3; motor++) {
            if ((*motorsState >> (2 * motor)) & 3) {
                // Column 3 + motor of the 6x6 matrix: column `motor` of the right-hand blocks.
                (&mtxNextOffdiag.m_m[0][1].m_col0)[motor].setZero4();
                (&mtxNextOffdiag.m_m[1][1].m_col0)[motor].setZero4();
            }
        }
    }
}

void hkPoweredChain_OverwriteVelocityWithExplicitImpulse(
    int32_t modifiedConstraintIndex,
    int32_t modifiedCoordinateIndex,
    const hkp3dAngularMotorSolverInfo* motorsState,
    hkVector8* velocities
) {
    const hkp3dAngularMotorSolverInfo& state = motorsState[modifiedConstraintIndex];
    const hkp1dMotorSolverInfo& motor = state.m_motorInfos[modifiedCoordinateIndex];
    float impulse;
    if (((state.m_broken >> (2 * modifiedCoordinateIndex)) & 3) ==
        hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MAX_BREACHED) {
        impulse = motor.m_maxImpulsePerSubstep;
    }
    else {
        impulse = motor.m_minImpulsePerSubstep;
    }
    velocities[modifiedConstraintIndex].m_ang(modifiedCoordinateIndex) = impulse;
}

// One block row of the chain matrix's LU decomposition: lower = prevOffdiag^T, diagonalInv = (mtxDiag - lower *
// prevUpper)^-1 with each diagonal element clamped to at least HK_REAL_EPSILON before the inversion, upper =
// diagonalInv * mtxNextOffdiag; then this row's next off-diagonal and upper become the previous ones of the next row.
const hkMatrix6* hkPoweredChain_ComputeConstraintMatrixLuDecomposition_ForOneRow(
    uint8_t disableNextConstraint,
    hkpConstraintChainMatrix6Triple* triple,
    const hkp3dAngularMotorSolverInfo::Status* status,
    hkMatrix6& mtxPrevOffdiag,
    const hkMatrix6** triplePrevUpper
) {
    hkMatrix6 mtxDiag = triple->m_mtxDiag;
    hkMatrix6 mtxNextOffdiag = triple->m_mtxNextOffdiag;
    hkPoweredChain_DisableMotorInMatrixRow_ThisConstraint(status, disableNextConstraint, mtxDiag, mtxNextOffdiag);
    hkPoweredChain_DisableMotorInMatrixRow_NextConstraint(status, mtxPrevOffdiag);
    hkMatrix6SetTranspose(triple->m_lower, mtxPrevOffdiag);
    hkMatrix6 tmp;
    mulMatrix6(tmp, triple->m_lower, **triplePrevUpper);
    triple->m_diagonalInv = mtxDiag;
    subMatrix6(triple->m_diagonalInv, tmp);

    // The two diagonal blocks' diagonals, element k of m_m[0][0] then of m_m[1][1] for k = 0..2, in that order; NaN
    // becomes the epsilon too.
    hkMatrix6& rInv = triple->m_diagonalInv;
    for (int32_t i = 0; i < 6; ++i) {
        float* pValue = &rInv.m_m[i & 1][i & 1](i >> 1, i >> 1);
        float fValue = *pValue;
        if (!(fValue > 1.1920929e-07f)) {
            fValue = 1.1920929e-07f;
        }
        *pValue = fValue;
    }

    hkMatrix6 tmp2 = triple->m_diagonalInv;
    hkMatrix6SetInvert(triple->m_diagonalInv, tmp2);
    mulMatrix6(triple->m_upper, triple->m_diagonalInv, mtxNextOffdiag);
    mtxPrevOffdiag = triple->m_mtxNextOffdiag;
    *triplePrevUpper = &triple->m_upper;
    return &triple->m_upper;
}

// For each constraint from the last to the first, its angular velocity is recomputed from the chain's LU factors (tmp =
// g - upper * tmp, then offDiagTransp * tmp + mtxDiag * impCenter + mtxNextOffdiag * impLeft); for each motor of it in
// state MOTOR_BROKEN_MIN_BREACHED whose motor target is at least that velocity, or MOTOR_BROKEN_MAX_BREACHED whose
// target is at most it, the largest |velocity - target| wins and that motor's state goes back to MOTOR_NOT_BROKEN. The
// previous constraint's triple is taken through its index sign-extended from 16 bits, also when that index is -1 (only
// its successor is used then). It reads neither impulse nor motorsState.
// EXE-INLINED none: hkPadSpu<T>::operator T (each of info's fields)
void hkPoweredChain_ScanAndEnableMotors(
    const hkpChainSolverInfo& info,
    int32_t& modifiedConstraintIndex,
    int32_t& modifiedCoordinateIndex,
    float& impulse,
    hkp3dAngularMotorSolverInfo* motorsState
) {
    (void)impulse;
    (void)motorsState;
    float maxDifference = 0.0f;
    int32_t constraintIndex = -1;
    int32_t coordinate = -1;
    hkVector4 zero;
    zero.setZero4();
    hkVector8 impLeft;
    impLeft.m_lin = zero;
    impLeft.m_ang = zero;
    hkVector8 impCenter;
    impCenter.m_lin = zero;
    impCenter.m_ang = zero;
    hkVector8 tmp;
    tmp.m_lin = zero;
    tmp.m_ang = zero;
    hkVector8 mulResult;
    hkVector8 resultVelocityVec;
    hkMatrix6 offDiagTransp;

    hkVector8* gTemp = info.m_gTempBuffer;
    hkMatrix6SetMulV(mulResult, info.m_triples[info.m_numConstraints - 1].m_upper, tmp);
    int32_t i = info.m_numConstraints - 1;
    tmp.m_lin.setSub4(gTemp[i + 1].m_lin, mulResult.m_lin);
    tmp.m_ang.setSub4(gTemp[i + 1].m_ang, mulResult.m_ang);
    for (; i >= 0; --i) {
        const int32_t prev = i - 1;
        const hkpConstraintChainMatrix6Triple* pPrev = &info.m_triples[static_cast<int16_t>(prev)];
        impLeft = impCenter;
        impCenter = tmp;
        if (prev >= 0) {
            hkMatrix6SetMulV(mulResult, pPrev->m_upper, tmp);
            tmp.m_lin.setSub4(gTemp[i].m_lin, mulResult.m_lin);
            tmp.m_ang.setSub4(gTemp[i].m_ang, mulResult.m_ang);
        }
        else {
            tmp.m_lin = zero;
            tmp.m_ang = zero;
        }
        if (prev >= 0) {
            hkMatrix6SetTranspose(offDiagTransp, pPrev->m_mtxNextOffdiag);
        }
        else {
            // EXE-INLINED none: hkMatrix6::setIdentity
            offDiagTransp.m_m[0][0].m_col0 = zero;
            offDiagTransp.m_m[0][0].m_col0(0) = 1.0f;
            offDiagTransp.m_m[0][0].m_col1 = zero;
            offDiagTransp.m_m[0][0].m_col1(1) = 1.0f;
            offDiagTransp.m_m[0][0].m_col2 = zero;
            offDiagTransp.m_m[0][0].m_col2(2) = 1.0f;
            offDiagTransp.m_m[1][1].m_col0 = zero;
            offDiagTransp.m_m[1][1].m_col0(0) = 1.0f;
            offDiagTransp.m_m[1][1].m_col1 = zero;
            offDiagTransp.m_m[1][1].m_col1(1) = 1.0f;
            offDiagTransp.m_m[1][1].m_col2 = zero;
            offDiagTransp.m_m[1][1].m_col2(2) = 1.0f;
            offDiagTransp.m_m[0][1].m_col0 = zero;
            offDiagTransp.m_m[0][1].m_col1 = zero;
            offDiagTransp.m_m[0][1].m_col2 = zero;
            offDiagTransp.m_m[1][0].m_col0 = zero;
            offDiagTransp.m_m[1][0].m_col1 = zero;
            offDiagTransp.m_m[1][0].m_col2 = zero;
        }
        hkMatrix6SetMulV(resultVelocityVec, offDiagTransp, tmp);
        hkMatrix6SetMulV(mulResult, pPrev[1].m_mtxDiag, impCenter);
        resultVelocityVec.m_lin.add4(mulResult.m_lin);
        resultVelocityVec.m_ang.add4(mulResult.m_ang);
        hkMatrix6SetMulV(mulResult, pPrev[1].m_mtxNextOffdiag, impLeft);
        resultVelocityVec.m_lin.add4(mulResult.m_lin);
        resultVelocityVec.m_ang.add4(mulResult.m_ang);

        const hkp3dAngularMotorSolverInfo& state = info.m_motorsState[i];
        const int32_t status = state.m_broken;
        for (int32_t j = 0; j < 3; ++j) {
            const int32_t motorState = (status >> (2 * j)) & hkp3dAngularMotorSolverInfo::ANGULAR_1ST;
            if (motorState != hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MIN_BREACHED &&
                motorState != hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MAX_BREACHED) {
                continue;
            }
            // The accumulators are found by a byte offset from their base, as raw memory.
            // EXE-INLINED none: hkAddByteOffset<const hkpVelocityAccumulator>
            const hkpVelocityAccumulator* accA = reinterpret_cast<const hkpVelocityAccumulator*>(
                reinterpret_cast<const char*>(info.m_accumsBase.m_storage) + info.m_va.m_storage[i].m_offset
            );
            const hkpVelocityAccumulator* accB = reinterpret_cast<const hkpVelocityAccumulator*>(
                reinterpret_cast<const char*>(info.m_accumsBase.m_storage) + info.m_va.m_storage[i + 1].m_offset
            );
            const hkp1dMotorSolverInfo& motor = state.m_motorInfos[j];
            const float tau = motor.m_tau;
            const hkVector4 scratchA = accA->m_scratch123.m_col0;
            const hkVector4 scratchB = accB->m_scratch123.m_col0;
            const float tauFactor = info.m_solverInfo.m_invIntegrateVelocityFactor(0) * tau;
            const hkp2AngJacobian& jac = info.m_jAng[3 * i + j];
            const __m128 damping = _mm_shuffle_ps(_mm_load_ss(&motor.m_damping), _mm_load_ss(&motor.m_damping), 0);
            const __m128 tauSplat = _mm_shuffle_ps(_mm_set_ss(tauFactor), _mm_set_ss(tauFactor), 0);
            const __m128 velPart = _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(accA->m_angularVel.m_quad, accA->m_scratch123.m_col0.m_quad), jac.m_angular[0].m_quad
                    ),
                    _mm_mul_ps(
                        _mm_sub_ps(accB->m_angularVel.m_quad, accB->m_scratch123.m_col0.m_quad), jac.m_angular[1].m_quad
                    )
                ),
                damping
            );
            const __m128 posPart = _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(jac.m_angular[0].m_quad, scratchA.m_quad),
                    _mm_mul_ps(jac.m_angular[1].m_quad, scratchB.m_quad)
                ),
                tauSplat
            );
            const __m128 sum = _mm_add_ps(velPart, posPart);
            // x + y, then z + that; the order of the additions matters.
            const __m128 xy = _mm_add_ss(_mm_shuffle_ps(sum, sum, 0x55), sum);
            const float dot = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sum, sum, 0xAA), xy));
            // The motor's right-hand side sits in the w of the body B row.
            const float target = tau * jac.m_angular[1](3) - dot;
            const float velocity = resultVelocityVec.m_ang(j);
            if (motorState == hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MIN_BREACHED) {
                if (!(target >= velocity)) {
                    continue;
                }
            }
            else if (!(target <= velocity)) {
                continue;
            }
            const float difference = velocity - target;
            const float absDifference = std::fabs(difference);
            if (maxDifference < absDifference) {
                maxDifference = absDifference;
                constraintIndex = i;
                coordinate = j;
            }
        }
    }

    if (constraintIndex >= 0) {
        uint8_t& broken = info.m_motorsState[constraintIndex].m_broken;
        const int32_t shift = 2 * coordinate;
        broken = static_cast<uint8_t>(
            (broken & ~(hkp3dAngularMotorSolverInfo::ANGULAR_1ST << shift)) |
            (hkp3dAngularMotorSolverInfo::MOTOR_NOT_BROKEN << shift)
        );
        modifiedConstraintIndex = constraintIndex;
        modifiedCoordinateIndex = coordinate;
    }
}

// Builds each constraint's block row of the chain matrix from the Jacobians and the inverse masses of its two
// accumulators (A = accumulators[row], B = accumulators[row + 1]; each sum is horizontalAdd3's y + x then z): the
// symmetric linear and angular diagonal blocks with the cfm terms on their diagonals, the coupling block m_m[1][0] and
// its transpose, and the off-diagonal to the next constraint (zero for the last one); then the LU decomposition row by
// row. bufferSize is not read. The flag each row hands ..._ForOneRow is a byte.
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
) {
    const __m128 zero = _mm_setzero_ps();
    hkMatrix6 mtxPrevOffdiag;
    // EXE-INLINED none: hkMatrix6::setZero
    for (int32_t r = 0; r < 2; ++r) {
        for (int32_t c = 0; c < 2; ++c) {
            mtxPrevOffdiag.m_m[r][c].m_col0.m_quad = zero;
            mtxPrevOffdiag.m_m[r][c].m_col1.m_quad = zero;
            mtxPrevOffdiag.m_m[r][c].m_col2.m_quad = zero;
        }
    }
    const hkMatrix6* triplePrevUpper = &mtxPrevOffdiag;

    for (int32_t row = 0; row < numConstraints; ++row) {
        hkpConstraintChainMatrix6Triple& triple = triples[row];
        const hkp1Lin2AngJacobian* jac = &j[3 * row];
        const hkp2AngJacobian* jacAng = &jAng[3 * row];
        // The accumulators are addressed by the byte offsets in va.
        const hkpVelocityAccumulator* accA = hkAddByteOffsetConst(accumsBase, va[row].m_offset);
        const hkpVelocityAccumulator* accB = hkAddByteOffsetConst(accumsBase, va[row + 1].m_offset);
        const __m128 invA = accA->m_invMasses.m_quad;
        const __m128 invB = accB->m_invMasses.m_quad;
        hkMatrix6& mtxDiag = triple.m_mtxDiag;

        // The diagonal blocks, upper triangle mirrored, then the cfm terms on their diagonals.
        for (int32_t j = 0; j < 3; ++j) {
            for (int32_t k = j; k < 3; ++k) {
                const __m128 lin = _mm_mul_ps(jac[k].m_linear0.m_quad, jac[j].m_linear0.m_quad);
                const __m128 sumLin = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(invA, invA, 0xFF), lin),
                        _mm_mul_ps(_mm_shuffle_ps(invB, invB, 0xFF), lin)
                    ),
                    _mm_add_ps(
                        _mm_mul_ps(_mm_mul_ps(jac[k].m_angular[1].m_quad, jac[j].m_angular[1].m_quad), invB),
                        _mm_mul_ps(_mm_mul_ps(jac[k].m_angular[0].m_quad, jac[j].m_angular[0].m_quad), invA)
                    )
                );
                const float valueLin = _mm_cvtss_f32(_mm_add_ps(
                    _mm_shuffle_ps(sumLin, sumLin, 0xAA), _mm_add_ps(_mm_shuffle_ps(sumLin, sumLin, 0x55), sumLin)
                ));
                mtxDiag.m_m[0][0](j, k) = valueLin;
                mtxDiag.m_m[0][0](k, j) = valueLin;
                const __m128 sumAng = _mm_add_ps(
                    _mm_mul_ps(_mm_mul_ps(jacAng[k].m_angular[1].m_quad, jacAng[j].m_angular[1].m_quad), invB),
                    _mm_mul_ps(_mm_mul_ps(jacAng[j].m_angular[0].m_quad, jacAng[k].m_angular[0].m_quad), invA)
                );
                const float valueAng = _mm_cvtss_f32(_mm_add_ps(
                    _mm_shuffle_ps(sumAng, sumAng, 0xAA), _mm_add_ps(_mm_shuffle_ps(sumAng, sumAng, 0x55), sumAng)
                ));
                mtxDiag.m_m[1][1](j, k) = valueAng;
                mtxDiag.m_m[1][1](k, j) = valueAng;
            }
            float& diagLin = mtxDiag.m_m[0][0](j, j);
            float& diagAng = mtxDiag.m_m[1][1](j, j);
            diagLin = cfm.m_linMul * diagLin;
            diagAng = diagAng * cfm.m_angMul;
            diagLin = diagLin + cfm.m_linAdd;
            diagAng = diagAng + cfm.m_angAdd;
        }

        // The coupling of the angular motor rows with the linear rows.
        for (int32_t m = 0; m < 3; ++m) {
            for (int32_t k = 0; k < 3; ++k) {
                const __m128 sum = _mm_add_ps(
                    _mm_mul_ps(_mm_mul_ps(jac[k].m_angular[1].m_quad, jacAng[m].m_angular[1].m_quad), invB),
                    _mm_mul_ps(_mm_mul_ps(jacAng[m].m_angular[0].m_quad, jac[k].m_angular[0].m_quad), invA)
                );
                mtxDiag.m_m[1][0](m, k) = _mm_cvtss_f32(
                    _mm_add_ps(_mm_shuffle_ps(sum, sum, 0xAA), _mm_add_ps(_mm_shuffle_ps(sum, sum, 0x55), sum))
                );
            }
        }

        mtxDiag.m_m[0][1].setTranspose(mtxDiag.m_m[1][0]);

        hkMatrix6& mtxNextOffdiag = triple.m_mtxNextOffdiag;
        if (row >= numConstraints - 1) {
            // EXE-INLINED none: hkMatrix6::setZero
            for (int32_t r = 0; r < 2; ++r) {
                for (int32_t c = 0; c < 2; ++c) {
                    mtxNextOffdiag.m_m[r][c].m_col0.m_quad = zero;
                    mtxNextOffdiag.m_m[r][c].m_col1.m_quad = zero;
                    mtxNextOffdiag.m_m[r][c].m_col2.m_quad = zero;
                }
            }
        }
        else {
            // The coupling to the next constraint goes through their shared body B only.
            const hkp1Lin2AngJacobian* jacNext = jac + 3;
            const hkp2AngJacobian* jacAngNext = jacAng + 3;
            for (int32_t m = 0; m < 3; ++m) {
                for (int32_t k = 0; k < 3; ++k) {
                    const __m128 lin = _mm_mul_ps(
                        _mm_mul_ps(jacNext[k].m_linear0.m_quad, jac[m].m_linear0.m_quad),
                        _mm_shuffle_ps(invB, invB, 0xFF)
                    );
                    const __m128 sum00 = _mm_sub_ps(
                        _mm_mul_ps(_mm_mul_ps(jacNext[k].m_angular[0].m_quad, jac[m].m_angular[1].m_quad), invB), lin
                    );
                    mtxNextOffdiag.m_m[0][0](m, k) = _mm_cvtss_f32(_mm_add_ps(
                        _mm_shuffle_ps(sum00, sum00, 0xAA), _mm_add_ps(_mm_shuffle_ps(sum00, sum00, 0x55), sum00)
                    ));
                    const __m128 sum11 =
                        _mm_mul_ps(_mm_mul_ps(jacAngNext[k].m_angular[0].m_quad, jacAng[m].m_angular[1].m_quad), invB);
                    mtxNextOffdiag.m_m[1][1](m, k) = _mm_cvtss_f32(_mm_add_ps(
                        _mm_shuffle_ps(sum11, sum11, 0xAA), _mm_add_ps(_mm_shuffle_ps(sum11, sum11, 0x55), sum11)
                    ));
                    const __m128 sum01 =
                        _mm_mul_ps(_mm_mul_ps(jacAngNext[k].m_angular[0].m_quad, jac[m].m_angular[1].m_quad), invB);
                    mtxNextOffdiag.m_m[0][1](m, k) = _mm_cvtss_f32(_mm_add_ps(
                        _mm_shuffle_ps(sum01, sum01, 0xAA), _mm_add_ps(_mm_shuffle_ps(sum01, sum01, 0x55), sum01)
                    ));
                    const __m128 sum10 =
                        _mm_mul_ps(_mm_mul_ps(jacNext[k].m_angular[0].m_quad, jacAng[m].m_angular[1].m_quad), invB);
                    mtxNextOffdiag.m_m[1][0](m, k) = _mm_cvtss_f32(_mm_add_ps(
                        _mm_shuffle_ps(sum10, sum10, 0xAA), _mm_add_ps(_mm_shuffle_ps(sum10, sum10, 0x55), sum10)
                    ));
                }
            }
        }
    }

    for (int32_t row = 0; row < numConstraints; ++row) {
        hkPoweredChain_ComputeConstraintMatrixLuDecomposition_ForOneRow(
            row != numConstraints - 1, &triples[row], &motorsState[row].m_broken, mtxPrevOffdiag, &triplePrevUpper
        );
    }
}

// Starts the decomposition one row before the first modified constraint (at row 0 at least): that row's previous
// off-diagonal and upper are the row before's next off-diagonal and upper, or a zero matrix for both at row 0. The flag
// each row hands ..._ForOneRow (not the last row) is a byte.
void hkPoweredChain_UpdateLuDecomposition(
    int32_t firstConstraintToProcess,
    int32_t numConstraints,
    const hkp3dAngularMotorSolverInfo* motorsState,
    hkpConstraintChainMatrix6Triple* triples
) {
    int32_t firstRow = firstConstraintToProcess - 1;
    if (firstRow < 0) {
        firstRow = 0;
    }
    hkMatrix6 mtxPrevOffdiag;
    const hkMatrix6* triplePrevUpper;
    if (firstRow != 0) {
        mtxPrevOffdiag = triples[firstRow - 1].m_mtxNextOffdiag;
        triplePrevUpper = &triples[firstRow - 1].m_upper;
    }
    else {
        const __m128 zero = _mm_setzero_ps();
        for (int32_t r = 0; r < 2; ++r) {
            for (int32_t c = 0; c < 2; ++c) {
                mtxPrevOffdiag.m_m[r][c].m_col0.m_quad = zero;
                mtxPrevOffdiag.m_m[r][c].m_col1.m_quad = zero;
                mtxPrevOffdiag.m_m[r][c].m_col2.m_quad = zero;
            }
        }
        triplePrevUpper = &mtxPrevOffdiag;
    }
    for (int32_t row = firstRow; row < numConstraints; ++row) {
        hkPoweredChain_ComputeConstraintMatrixLuDecomposition_ForOneRow(
            row != numConstraints - 1, &triples[row], &motorsState[row].m_broken, mtxPrevOffdiag, &triplePrevUpper
        );
    }
}

// The schema's head holds the status buffer, the type byte's lane (HK_REAL_EPSILON is added to the float it holds
// before the type byte is stored over it), tau, damping, size, type and count, then the n + 1 accumulator offsets and
// the n motor states (each copied whole). The hysteresis and the status buffer are then stored again and the chain
// matrix is built from the Jacobians already in the schema.
int32_t hkPoweredChainBuildJacobian(
    const hkpPoweredChainBuildJacobianParams& params,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    const int32_t numConstraints = params.m_numConstraints;
    hkpJacobianSchema* pSchemaUnit = out.m_jacobianSchemas;
    hkpJacobianPoweredChainSchema* pSchema = reinterpret_cast<hkpJacobianPoweredChainSchema*>(pSchemaUnit);

    // EXE-INLINED none: hkpJacobianPoweredChainSchema::initPoweredChainSchema
    pSchema->m_statusFlagsOutput = params.m_childConstraintStatusWriteBackBuffer;
    XFNV_M128_F32(pSchemaUnit->m_quad, 0) = XFNV_M128_F32(pSchemaUnit->m_quad, 0) + 1.1920928955078125e-07;
    pSchema->m_tau = params.m_chainTau;
    pSchema->m_damping = params.m_chainDamping;
    pSchema->m_size = hkpJacobianPoweredChainSchema::getSize(static_cast<uint16_t>(numConstraints));
    pSchemaUnit->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_POWERED_CHAIN;
    pSchema->m_numConstraints = static_cast<uint16_t>(numConstraints);
    hkpVelocityAccumulatorOffset* pOffsets = pSchema->getAccumulatorOffsetsBase();
    const int32_t numBodies = numConstraints + 1;
    for (int32_t i = 0; i < numBodies; ++i) {
        pOffsets[i] = params.m_accumulators[i];
    }
    hkp3dAngularMotorSolverInfo* pStatus = pSchema->getChildConstraintStatusBase();
    for (int32_t i = 0; i < numConstraints; ++i) {
        pStatus[i] = params.m_motorsState[i];
    }

    pSchema->m_maxTorqueHysterisys = params.m_maxTorqueHysterisys;
    pSchema->m_statusFlagsOutput = params.m_childConstraintStatusWriteBackBuffer;
    hkPoweredChain_BuildConstraintMatrixAndLuDecomposition(
        numConstraints, params.m_cfm, params.m_motorsState,
        hkpJacobianPoweredChainSchema::getLinearJacobians(out.m_jacobianSchemas),
        hkpJacobianPoweredChainSchema::getAngularJacobians(out.m_jacobianSchemas, numConstraints),
        pSchema->getAccumulatorOffsetsBase(), params.m_accumsBase, pSchema->getMatrixBuffer(),
        numConstraints * static_cast<int32_t>(sizeof(hkpConstraintChainMatrix6Triple))
    );
    const uint32_t uiSize = pSchema->m_size;
    // The schema stream is raw memory: the next schema starts uiSize bytes on.
    out.m_jacobianSchemas =
        reinterpret_cast<hkpJacobianSchema*>(reinterpret_cast<char*>(out.m_jacobianSchemas) + uiSize);
    return static_cast<int32_t>(uiSize);
}

// tmp runs from the last constraint back: tmp = g[i + 1] - m_upper[i] * tmp (g is info.m_gTempBuffer). A motor breaches
// by its impulse less its maximum, or its minimum less its impulse, both bounds scaled by 1.05.
int32_t hkPoweredChain_ScanAndDisableMotors(
    const hkpChainSolverInfo& info,
    int32_t& constraintIndex,
    int32_t& motorIndex,
    float& impulse,
    hkp3dAngularMotorSolverInfo* motorsState
) {
    (void)motorsState;
    float maxViolation = 0.0f;
    int32_t bestConstraint = -1;
    int32_t bestMotor = -1;
    hkVector8 tmp;
    tmp.m_lin.m_quad = _mm_setzero_ps();
    tmp.m_ang.m_quad = _mm_setzero_ps();
    for (int32_t i = info.m_numConstraints.m_storage - 1; i >= 0; --i) {
        hkVector8 product;
        hkMatrix6SetMulV(product, info.m_triples.m_storage[i].m_upper, tmp);
        const hkVector8& g = info.m_gTempBuffer.m_storage[i + 1];
        tmp.m_lin.m_quad = _mm_sub_ps(g.m_lin.m_quad, product.m_lin.m_quad);
        tmp.m_ang.m_quad = _mm_sub_ps(g.m_ang.m_quad, product.m_ang.m_quad);
        const hkp3dAngularMotorSolverInfo& state = info.m_motorsState.m_storage[i];
        for (int32_t m = 0; m < 3; ++m) {
            if (((state.m_broken >> (2 * m)) & 3) == 0) {
                const float value = tmp.m_ang(m);
                const float overMax = value - state.m_motorInfos[m].m_maxImpulsePerSubstep * 1.0499999523162842;
                const float underMin = state.m_motorInfos[m].m_minImpulsePerSubstep * 1.0499999523162842 - value;
                const float violation = (underMin < overMax) ? overMax : underMin;
                if (maxViolation < violation) {
                    maxViolation = violation;
                    bestConstraint = i;
                    impulse = value;
                    bestMotor = m;
                }
            }
        }
    }
    if (bestConstraint >= 0) {
        hkp3dAngularMotorSolverInfo& state = info.m_motorsState.m_storage[bestConstraint];
        // MOTOR_BROKEN_MIN_BREACHED is also taken when the compare is unordered, as the game does.
        const int32_t motorState = (state.m_motorInfos[bestMotor].m_maxImpulsePerSubstep < impulse)
            ? hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MAX_BREACHED
            : hkp3dAngularMotorSolverInfo::MOTOR_BROKEN_MIN_BREACHED;
        const int32_t shift = 2 * bestMotor;
        state.m_broken = static_cast<uint8_t>((state.m_broken & ~(3 << shift)) | ((motorState & 3) << shift));
        constraintIndex = bestConstraint;
        motorIndex = bestMotor;
    }
    return bestConstraint;
}
