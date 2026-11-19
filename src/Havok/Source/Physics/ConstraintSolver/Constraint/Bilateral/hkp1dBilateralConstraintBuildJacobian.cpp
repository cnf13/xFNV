#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Each schema the builders write starts by adding HK_REAL_EPSILON to the float its first lane holds, then stores the
// type in its first byte.
void hkSetInvMassBuildJacobian(const hkVector4& invMassA, const hkVector4& invMassB, hkpConstraintQueryOut& out) {
    hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
    pSchema[0].m_quad = invMassA.m_quad;
    double typeLane = XFNV_M128_F32(pSchema[0].m_quad, 0) + 1.1920928955078125e-07;
    pSchema[1].m_quad = invMassB.m_quad;
    XFNV_M128_F32(pSchema[0].m_quad, 0) = typeLane;
    pSchema[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SET_MASS;
    out.m_jacobianSchemas = pSchema + sizeof(hkpJacobianSetMassSchema) / sizeof(hkpJacobianSchema);
}

// The body index goes into the second unit's first word.
void hkAddVelocityBuildJacobian(const hkVector4& deltaVel, int32_t bodyIndex, hkpConstraintQueryOut& out) {
    hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
    pSchema[0].m_quad = deltaVel.m_quad;
    double typeLane = XFNV_M128_F32(pSchema[0].m_quad, 0) + 1.1920928955078125e-07;
    XFNV_M128_F32(pSchema[1].m_quad, 0) = std::bit_cast<float>(bodyIndex);
    XFNV_M128_F32(pSchema[0].m_quad, 0) = typeLane;
    pSchema[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_ADD_VELOCITY;
    out.m_jacobianSchemas = pSchema + sizeof(hkpJacobianAddVelocitySchema) / sizeof(hkpJacobianSchema);
}

// The two matrices' columns fill six units.
void hkSetCenterOfMassBuildJacobian(
    const hkMatrix3& angToLinVelA,
    const hkMatrix3& angToLinVelB,
    hkpConstraintQueryOut& out
) {
    hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
    pSchema[0].m_quad = angToLinVelA.m_col0.m_quad;
    pSchema[1].m_quad = angToLinVelA.m_col1.m_quad;
    pSchema[2].m_quad = angToLinVelA.m_col2.m_quad;
    pSchema[3].m_quad = angToLinVelB.m_col0.m_quad;
    pSchema[4].m_quad = angToLinVelB.m_col1.m_quad;
    pSchema[5].m_quad = angToLinVelB.m_col2.m_quad;
    XFNV_M128_F32(pSchema[0].m_quad, 0) = XFNV_M128_F32(pSchema[0].m_quad, 0) + 1.1920928955078125e-07;
    pSchema[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SET_CENTER_OF_MASS;
    out.m_jacobianSchemas = pSchema + sizeof(hkpJacobianSetCenterOfMassSchema) / sizeof(hkpJacobianSchema);
}

void hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs(
    const hkp1dLinearBilateralConstraintInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out,
    float customRhs
) {
    hkVector4 v9;
    hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
    hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
    float m_virtMassFactor = in.m_virtMassFactor;
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    m_jacobianSchemas->m_quad = info.m_constrainedDofW.m_quad;
    __m128 v7 = _mm_sub_ps(info.m_pivotA.m_quad, m_bodyA->m_scratch0.m_quad);
    __m128 v8 = _mm_sub_ps(info.m_pivotA.m_quad, m_bodyB->m_scratch0.m_quad);
    v9.m_quad = info.m_constrainedDofW.m_quad;
    __m128 v10 = _mm_shuffle_ps(v9.m_quad, v9.m_quad, 201);
    __m128 v11 = _mm_shuffle_ps(v9.m_quad, v9.m_quad, 210);
    __m128 v12 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v11), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 210), v10));
    __m128 v13 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v8, v8, 210), v10), _mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), v11));
    m_jacobianSchemas[1].m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v12, v12, 85), m_bodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), m_bodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v12, v12, 170), m_bodyA->m_scratch123.m_col2.m_quad)
    );
    __m128 v14 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v13, v13, 85), m_bodyB->m_scratch123.m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), m_bodyB->m_scratch123.m_col0.m_quad)
    );
    __m128 v15 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 170), m_bodyB->m_scratch123.m_col2.m_quad);
    __m128 m_quad = m_jacobianSchemas[1].m_quad;
    __m128 v17 = _mm_add_ps(v14, v15);
    m_jacobianSchemas[2].m_quad = v17;
    __m128 m_quad_1 = m_bodyB->m_invMasses.m_quad;
    __m128 m_quad_2 = m_bodyA->m_invMasses.m_quad;
    __m128 v20 =
        _mm_add_ps(_mm_mul_ps(_mm_mul_ps(v17, v17), m_quad_1), _mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_quad_2));
    __m128 v21 = _mm_shuffle_ps(v20, _mm_unpackhi_ps(v20, _mm_add_ps(m_quad_2, m_quad_1)), 196);
    __m128 v22 = _mm_add_ps(_mm_shuffle_ps(v21, v21, 78), v21);
    float v24 = (XFNV_M128_F32(_mm_shuffle_ps(v22, v22, 177), 0) + XFNV_M128_F32(v22, 0)) + 1.1920928955078125e-07;
    XFNV_M128_F32(m_jacobianSchemas[2].m_quad, 3) = v24;
    XFNV_M128_F32(m_jacobianSchemas[1].m_quad, 3) = m_virtMassFactor / v24;
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) = in.m_rhsFactor * customRhs;
    hkpJacobianSchema* m_jacobianSchemas_1 = out.m_jacobianSchemas + 3;
    XFNV_M128_F32(m_jacobianSchemas_1[-3].m_quad, 0) =
        XFNV_M128_F32(out.m_jacobianSchemas->m_quad, 0) + 1.1920928955078125e-07;
    m_jacobianSchemas_1[-3].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL;
    out.m_jacobianSchemas = m_jacobianSchemas_1;
}

void hk1dLinearBilateralConstraintBuildJacobian(
    const hkp1dLinearBilateralConstraintInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    hkVector4 v10;
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
    hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
    float m_virtMassFactor = in.m_virtMassFactor;
    double m_rhsFactor = in.m_rhsFactor;
    m_jacobianSchemas->m_quad = info.m_constrainedDofW.m_quad;
    float m_rhsFactor_1 = m_rhsFactor;
    __m128 v8 = _mm_sub_ps(info.m_pivotA.m_quad, m_bodyA->m_scratch0.m_quad);
    __m128 v9 = _mm_sub_ps(info.m_pivotA.m_quad, m_bodyB->m_scratch0.m_quad);
    v10.m_quad = info.m_constrainedDofW.m_quad;
    __m128 v11 = _mm_shuffle_ps(v10.m_quad, v10.m_quad, 201);
    __m128 v12 = _mm_shuffle_ps(v10.m_quad, v10.m_quad, 210);
    __m128 v13 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), v12), _mm_mul_ps(_mm_shuffle_ps(v8, v8, 210), v11));
    __m128 v14 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v9, v9, 210), v11), _mm_mul_ps(_mm_shuffle_ps(v9, v9, 201), v12));
    m_jacobianSchemas[1].m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v13, v13, 85), m_bodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), m_bodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v13, v13, 170), m_bodyA->m_scratch123.m_col2.m_quad)
    );
    __m128 v15 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v14, v14, 85), m_bodyB->m_scratch123.m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), m_bodyB->m_scratch123.m_col0.m_quad)
    );
    __m128 v16 = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 170), m_bodyB->m_scratch123.m_col2.m_quad);
    __m128 m_quad = m_jacobianSchemas[1].m_quad;
    __m128 v18 = _mm_add_ps(v15, v16);
    m_jacobianSchemas[2].m_quad = v18;
    __m128 m_quad_1 = m_bodyB->m_invMasses.m_quad;
    __m128 m_quad_2 = m_bodyA->m_invMasses.m_quad;
    __m128 v21 =
        _mm_add_ps(_mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_quad_2), _mm_mul_ps(_mm_mul_ps(v18, v18), m_quad_1));
    __m128 v22 = _mm_shuffle_ps(v21, _mm_unpackhi_ps(v21, _mm_add_ps(m_quad_2, m_quad_1)), 196);
    __m128 v23 = _mm_add_ps(_mm_shuffle_ps(v22, v22, 78), v22);
    float v27 = (XFNV_M128_F32(_mm_shuffle_ps(v23, v23, 177), 0) + XFNV_M128_F32(v23, 0)) + 1.1920928955078125e-07;
    XFNV_M128_F32(m_jacobianSchemas[2].m_quad, 3) = v27;
    XFNV_M128_F32(m_jacobianSchemas[1].m_quad, 3) = m_virtMassFactor / v27;
    __m128 v24 = _mm_mul_ps(_mm_sub_ps(info.m_pivotA.m_quad, info.m_pivotA.m_quad), info.m_constrainedDofW.m_quad);
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) =
        (XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v24, v24, 85), 0) + XFNV_M128_F32(v24, 0))) *
        m_rhsFactor_1;
    __m128 v25 = _mm_mul_ps(_mm_sub_ps(info.m_pivotB.m_quad, info.m_pivotA.m_quad), info.m_constrainedDofW.m_quad);
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) = in.m_rhsFactor *
        (XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v25, v25, 85), 0) + XFNV_M128_F32(v25, 0)));
    hkpJacobianSchema* m_jacobianSchemas_1 = out.m_jacobianSchemas + 3;
    XFNV_M128_F32(m_jacobianSchemas_1[-3].m_quad, 0) =
        XFNV_M128_F32(out.m_jacobianSchemas->m_quad, 0) + 1.1920928955078125e-07;
    m_jacobianSchemas_1[-3].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL;
    out.m_jacobianSchemas = m_jacobianSchemas_1;
}

void hk1dLinearBilateralConstraintUserTauBuildJacobian(
    const hkp1dLinearBilateralUserTauConstraintInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    hkVector4 v8;
    hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
    float m_rhsFactor = in.m_rhsFactor;
    hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    m_jacobianSchemas->m_quad = info.m_constrainedDofW.m_quad;
    __m128 v6 = _mm_sub_ps(info.m_pivotA.m_quad, m_bodyA->m_scratch0.m_quad);
    __m128 v7 = _mm_sub_ps(info.m_pivotB.m_quad, m_bodyB->m_scratch0.m_quad);
    v8.m_quad = info.m_constrainedDofW.m_quad;
    __m128 v9 = _mm_shuffle_ps(v8.m_quad, v8.m_quad, 201);
    __m128 v10 = _mm_shuffle_ps(v8.m_quad, v8.m_quad, 210);
    __m128 v11 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 201), v10), _mm_mul_ps(_mm_shuffle_ps(v6, v6, 210), v9));
    __m128 v12 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 210), v9), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v10));
    m_jacobianSchemas[1].m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 85), m_bodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), m_bodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v11, v11, 170), m_bodyA->m_scratch123.m_col2.m_quad)
    );
    __m128 v13 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v12, v12, 85), m_bodyB->m_scratch123.m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), m_bodyB->m_scratch123.m_col0.m_quad)
    );
    __m128 v14 = _mm_mul_ps(_mm_shuffle_ps(v12, v12, 170), m_bodyB->m_scratch123.m_col2.m_quad);
    __m128 m_quad = m_jacobianSchemas[1].m_quad;
    __m128 v16 = _mm_add_ps(v13, v14);
    m_jacobianSchemas[2].m_quad = v16;
    __m128 m_quad_1 = m_bodyB->m_invMasses.m_quad;
    __m128 m_quad_2 = m_bodyA->m_invMasses.m_quad;
    __m128 v19 =
        _mm_add_ps(_mm_mul_ps(_mm_mul_ps(v16, v16), m_quad_1), _mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_quad_2));
    __m128 v20 = _mm_shuffle_ps(v19, _mm_unpackhi_ps(v19, _mm_add_ps(m_quad_2, m_quad_1)), 196);
    __m128 v21 = _mm_add_ps(_mm_shuffle_ps(v20, v20, 78), v20);
    float v24 = (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 177), 0) + XFNV_M128_F32(v21, 0)) + 1.1920928955078125e-07;
    XFNV_M128_F32(m_jacobianSchemas[2].m_quad, 3) = v24;
    XFNV_M128_F32(m_jacobianSchemas[1].m_quad, 3) = 1.0 / v24;
    __m128 v22 = _mm_mul_ps(_mm_sub_ps(info.m_pivotB.m_quad, info.m_pivotA.m_quad), info.m_constrainedDofW.m_quad);
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) =
        (XFNV_M128_F32(_mm_shuffle_ps(v22, v22, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v22, v22, 85), 0) + XFNV_M128_F32(v22, 0))) *
        m_rhsFactor;
    hkpJacobianSchema* m_jacobianSchemas_1 = out.m_jacobianSchemas + 4;
    float m_damping = info.m_damping;
    XFNV_M128_F32(m_jacobianSchemas_1[-1].m_quad, 0) = info.m_tau;
    XFNV_M128_F32(m_jacobianSchemas_1[-1].m_quad, 1) = m_damping;
    XFNV_M128_F32(m_jacobianSchemas_1[-4].m_quad, 0) =
        XFNV_M128_F32(m_jacobianSchemas_1[-4].m_quad, 0) + 1.1920928955078125e-07;
    m_jacobianSchemas_1[-4].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_BILATERAL_USER_TAU;
    out.m_jacobianSchemas = m_jacobianSchemas_1;
}

// The linear part is the direction, the angular parts the lever arm from each body's centre of mass to the pivot
// crossed with it (cross(armA, dir) and cross(dir, armB)) and taken into the body's inertia space (m_scratch123);
// the inverse of the effective mass plus FLT_EPSILON goes into the second angular part's w and the virtual mass
// factor over it into the first's.
void hk1dLinearBilateralConstraintBuildJacobianWithCustomRhs_noSchema(
    const hkp1dLinearBilateralConstraintInfo& info,
    const hkpConstraintQueryIn& in,
    hkp1Lin2AngJacobian* jac,
    float customRhs
) {
    const hkpVelocityAccumulator* pBodyA = in.m_bodyA;
    const hkpVelocityAccumulator* pBodyB = in.m_bodyB;
    const float fVirtMassFactor = in.m_virtMassFactor;
    jac->m_linear0.m_quad = info.m_constrainedDofW.m_quad;
    const __m128 kArmA = _mm_sub_ps(info.m_pivotA.m_quad, pBodyA->m_scratch0.m_quad);
    const __m128 kArmB = _mm_sub_ps(info.m_pivotA.m_quad, pBodyB->m_scratch0.m_quad);
    const __m128 kDir = info.m_constrainedDofW.m_quad;
    const __m128 kDirYzx = _mm_shuffle_ps(kDir, kDir, 201);
    const __m128 kDirZxy = _mm_shuffle_ps(kDir, kDir, 210);
    const __m128 kCrossA = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 201), kDirZxy), _mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 210), kDirYzx)
    );
    const __m128 kCrossB = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 210), kDirYzx), _mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 201), kDirZxy)
    );
    const __m128 kAngA = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 85), pBodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0), pBodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 170), pBodyA->m_scratch123.m_col2.m_quad)
    );
    jac->m_angular[0].m_quad = kAngA;
    const __m128 kAngB = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 85), pBodyB->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0), pBodyB->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 170), pBodyB->m_scratch123.m_col2.m_quad)
    );
    jac->m_angular[1].m_quad = kAngB;
    const __m128 kInvMassesA = pBodyA->m_invMasses.m_quad;
    const __m128 kInvMassesB = pBodyB->m_invMasses.m_quad;
    const __m128 kMass = _mm_add_ps(
        _mm_mul_ps(_mm_mul_ps(kAngA, kAngA), kInvMassesA), _mm_mul_ps(_mm_mul_ps(kAngB, kAngB), kInvMassesB)
    );
    // The three angular lanes and the sum of the inverse masses' w lanes, added across.
    const __m128 kTerms = _mm_shuffle_ps(kMass, _mm_unpackhi_ps(kMass, _mm_add_ps(kInvMassesA, kInvMassesB)), 196);
    const __m128 kPairs = _mm_add_ps(_mm_shuffle_ps(kTerms, kTerms, 78), kTerms);
    const float fDiag =
        (XFNV_M128_F32(_mm_shuffle_ps(kPairs, kPairs, 177), 0) + XFNV_M128_F32(kPairs, 0)) + 1.1920928955078125e-07;
    XFNV_M128_F32(jac->m_angular[1].m_quad, 3) = fDiag;
    XFNV_M128_F32(jac->m_angular[0].m_quad, 3) = fVirtMassFactor / fDiag;
    XFNV_M128_F32(jac->m_linear0.m_quad, 3) = in.m_rhsFactor * customRhs;
}

// Builds the three ball-and-socket rows (hkBallSocketConstraintBuildJacobian_noSchema), then measures their right-hand
// sides. When that length exceeds the allowed error (afMaxAllowedErrorDistance times the rhs factor), it moves both
// pivots towards their bodies' centres of mass by f = (allowed / length)^(1/4), builds the rows again at the moved
// pivots and scales the first right-hand sides by f.
void hkStabilizedBallSocketConstraintBuildJacobian_noSchema(
    const hkVector4& posA,
    const hkVector4& posB,
    float maxAllowedErrorDistance,
    const hkpConstraintQueryIn& in,
    hkp1Lin2AngJacobian* jac
) {
    const hkpVelocityAccumulator* pBodyA = in.m_bodyA;
    const hkpVelocityAccumulator* pBodyB = in.m_bodyB;
    hkBallSocketConstraintBuildJacobian_noSchema(posA, posB, in, jac);
    // Lane w is never written: only x, y and z are summed.
    __m128 kRhs = _mm_undefined_ps();
    XFNV_M128_F32(kRhs, 0) = XFNV_M128_F32(jac[0].m_linear0.m_quad, 3);
    XFNV_M128_F32(kRhs, 1) = XFNV_M128_F32(jac[1].m_linear0.m_quad, 3);
    XFNV_M128_F32(kRhs, 2) = XFNV_M128_F32(jac[2].m_linear0.m_quad, 3);
    const __m128 kRhsSquared = _mm_mul_ps(kRhs, kRhs);
    const float fAllowed = in.m_rhsFactor * maxAllowedErrorDistance;
    const float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(
        (XFNV_M128_F32(kRhsSquared, 2) +
         static_cast<float>(XFNV_M128_F32(kRhsSquared, 1) + XFNV_M128_F32(kRhsSquared, 0)))
    )));
    if (!(fAllowed < fLength)) {
        return;
    }
    const float fRatio = fAllowed / fLength;
    const float fRoot = std::sqrt(fRatio);
    const float fFactor = std::sqrt(fRoot);
    const __m128 kFactor = _mm_set1_ps(fFactor);
    const __m128 kPosB = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, kFactor), pBodyB->m_scratch0.m_quad), _mm_mul_ps(posB.m_quad, kFactor)
    );
    const __m128 kPosA = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, kFactor), pBodyA->m_scratch0.m_quad), _mm_mul_ps(posA.m_quad, kFactor)
    );
    const __m128 kSeparation = _mm_sub_ps(kPosB, kPosA);
    // EXE-INLINED 0xD6C970
    for (int32_t i = 0; i < 3; ++i) {
        const float fVirtMassFactor = in.m_virtMassFactor;
        const float fRhsFactor = in.m_rhsFactor;
        hkp1Lin2AngJacobian& rJac = jac[i];
        const __m128 kDir = hkTransformIdentity.m_rotation.getColumn(i).m_quad;
        rJac.m_linear0.m_quad = kDir;
        const __m128 kArmA = _mm_sub_ps(kPosA, pBodyA->m_scratch0.m_quad);
        const __m128 kArmB = _mm_sub_ps(kPosB, pBodyB->m_scratch0.m_quad);
        const __m128 kDirYzx = _mm_shuffle_ps(kDir, kDir, 201);
        const __m128 kDirZxy = _mm_shuffle_ps(kDir, kDir, 210);
        const __m128 kCrossA = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 201), kDirZxy),
            _mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 210), kDirYzx)
        );
        const __m128 kCrossB = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 210), kDirYzx),
            _mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 201), kDirZxy)
        );
        const __m128 kAngA = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0), pBodyA->m_scratch123.m_col0.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 85), pBodyA->m_scratch123.m_col1.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 170), pBodyA->m_scratch123.m_col2.m_quad)
        );
        rJac.m_angular[0].m_quad = kAngA;
        const __m128 kAngB = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0), pBodyB->m_scratch123.m_col0.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 85), pBodyB->m_scratch123.m_col1.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 170), pBodyB->m_scratch123.m_col2.m_quad)
        );
        rJac.m_angular[1].m_quad = kAngB;
        const __m128 kInvMassesA = pBodyA->m_invMasses.m_quad;
        const __m128 kInvMassesB = pBodyB->m_invMasses.m_quad;
        const __m128 kMass = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(kAngA, kAngA), kInvMassesA), _mm_mul_ps(_mm_mul_ps(kAngB, kAngB), kInvMassesB)
        );
        const __m128 kTerms = _mm_shuffle_ps(kMass, _mm_unpackhi_ps(kMass, _mm_add_ps(kInvMassesA, kInvMassesB)), 196);
        const __m128 kPairs = _mm_add_ps(_mm_shuffle_ps(kTerms, kTerms, 78), kTerms);
        const float fDiag =
            (XFNV_M128_F32(_mm_shuffle_ps(kPairs, kPairs, 177), 0) + XFNV_M128_F32(kPairs, 0)) + 1.1920928955078125e-07;
        XFNV_M128_F32(rJac.m_angular[1].m_quad, 3) = fDiag;
        XFNV_M128_F32(rJac.m_angular[0].m_quad, 3) = fVirtMassFactor / fDiag;
        const __m128 kAlong = _mm_mul_ps(kSeparation, kDir);
        XFNV_M128_F32(rJac.m_linear0.m_quad, 3) =
            (XFNV_M128_F32(kAlong, 2) + static_cast<float>(XFNV_M128_F32(kAlong, 1) + XFNV_M128_F32(kAlong, 0))) *
            fRhsFactor;
    }
    XFNV_M128_F32(jac[0].m_linear0.m_quad, 3) = XFNV_M128_F32(kRhs, 0) * fFactor;
    XFNV_M128_F32(jac[1].m_linear0.m_quad, 3) = XFNV_M128_F32(kRhs, 1) * fFactor;
    XFNV_M128_F32(jac[2].m_linear0.m_quad, 3) = fFactor * XFNV_M128_F32(kRhs, 2);
}

void hk1dLinearLimitBuildJacobian(
    const hkp1dLinearLimitInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    hkVector4 v9;
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
    hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
    float m_virtMassFactor = in.m_virtMassFactor;
    double m_rhsFactor = in.m_rhsFactor;
    m_jacobianSchemas->m_quad = info.m_constrainedDofW.m_quad;
    float m_rhsFactor_1 = m_rhsFactor;
    __m128 v7 = _mm_sub_ps(info.m_pivotA.m_quad, m_bodyA->m_scratch0.m_quad);
    __m128 v8 = _mm_sub_ps(info.m_pivotB.m_quad, m_bodyB->m_scratch0.m_quad);
    v9.m_quad = info.m_constrainedDofW.m_quad;
    __m128 v10 = _mm_shuffle_ps(v9.m_quad, v9.m_quad, 201);
    __m128 v11 = _mm_shuffle_ps(v9.m_quad, v9.m_quad, 210);
    __m128 v12 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v11), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 210), v10));
    __m128 v13 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v8, v8, 210), v10), _mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), v11));
    m_jacobianSchemas[1].m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v12, v12, 85), m_bodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), m_bodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v12, v12, 170), m_bodyA->m_scratch123.m_col2.m_quad)
    );
    __m128 v14 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v13, v13, 85), m_bodyB->m_scratch123.m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), m_bodyB->m_scratch123.m_col0.m_quad)
    );
    __m128 v15 = _mm_mul_ps(_mm_shuffle_ps(v13, v13, 170), m_bodyB->m_scratch123.m_col2.m_quad);
    __m128 m_quad = m_jacobianSchemas[1].m_quad;
    __m128 v17 = _mm_add_ps(v14, v15);
    m_jacobianSchemas[2].m_quad = v17;
    __m128 m_quad_1 = m_bodyB->m_invMasses.m_quad;
    __m128 m_quad_2 = m_bodyA->m_invMasses.m_quad;
    __m128 v20 =
        _mm_add_ps(_mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_quad_2), _mm_mul_ps(_mm_mul_ps(v17, v17), m_quad_1));
    __m128 v21 = _mm_shuffle_ps(v20, _mm_unpackhi_ps(v20, _mm_add_ps(m_quad_2, m_quad_1)), 196);
    __m128 v22 = _mm_add_ps(_mm_shuffle_ps(v21, v21, 78), v21);
    float v26 = (XFNV_M128_F32(_mm_shuffle_ps(v22, v22, 177), 0) + XFNV_M128_F32(v22, 0)) + 1.1920928955078125e-07;
    XFNV_M128_F32(m_jacobianSchemas[2].m_quad, 3) = v26;
    XFNV_M128_F32(m_jacobianSchemas[1].m_quad, 3) = m_virtMassFactor / v26;
    __m128 v23 = _mm_mul_ps(_mm_sub_ps(info.m_pivotB.m_quad, info.m_pivotA.m_quad), info.m_constrainedDofW.m_quad);
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) =
        (XFNV_M128_F32(_mm_shuffle_ps(v23, v23, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v23, v23, 85), 0) + XFNV_M128_F32(v23, 0))) *
        m_rhsFactor_1;
    float v29 = -in.m_rhsFactor;
    hkpJacobianSchema* m_jacobianSchemas_1 = out.m_jacobianSchemas;
    float m_max = info.m_max;
    double m_min = info.m_min;
    out.m_jacobianSchemas += 4;
    XFNV_M128_F32(m_jacobianSchemas_1[3].m_quad, 0) = m_min * v29;
    XFNV_M128_F32(m_jacobianSchemas_1[3].m_quad, 1) = v29 * m_max;
    XFNV_M128_F32(m_jacobianSchemas_1->m_quad, 0) =
        XFNV_M128_F32(m_jacobianSchemas_1->m_quad, 0) + 1.1920928955078125e-07;
    m_jacobianSchemas_1->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_LINEAR_LIMITS;
}

void hk1dLinearFrictionBuildJacobian(
    const hkp1dLinearFrictionInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    hkVector4 v8;
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
    hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
    float m_virtMassFactor = in.m_virtMassFactor;
    m_jacobianSchemas->m_quad = info.m_constrainedDofW.m_quad;
    __m128 v6 = _mm_sub_ps(info.m_pivot.m_quad, m_bodyA->m_scratch0.m_quad);
    __m128 v7 = _mm_sub_ps(info.m_pivot.m_quad, m_bodyB->m_scratch0.m_quad);
    v8.m_quad = info.m_constrainedDofW.m_quad;
    __m128 v9 = _mm_shuffle_ps(v8.m_quad, v8.m_quad, 201);
    __m128 v10 = _mm_shuffle_ps(v8.m_quad, v8.m_quad, 210);
    __m128 v11 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 201), v10), _mm_mul_ps(_mm_shuffle_ps(v6, v6, 210), v9));
    __m128 v12 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 210), v9), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v10));
    m_jacobianSchemas[1].m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 85), m_bodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), m_bodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v11, v11, 170), m_bodyA->m_scratch123.m_col2.m_quad)
    );
    __m128 v13 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v12, v12, 85), m_bodyB->m_scratch123.m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), m_bodyB->m_scratch123.m_col0.m_quad)
    );
    __m128 v14 = _mm_mul_ps(_mm_shuffle_ps(v12, v12, 170), m_bodyB->m_scratch123.m_col2.m_quad);
    __m128 m_quad = m_jacobianSchemas[1].m_quad;
    __m128 v16 = _mm_add_ps(v13, v14);
    m_jacobianSchemas[2].m_quad = v16;
    __m128 m_quad_1 = m_bodyB->m_invMasses.m_quad;
    __m128 m_quad_2 = m_bodyA->m_invMasses.m_quad;
    __m128 v19 =
        _mm_add_ps(_mm_mul_ps(_mm_mul_ps(v16, v16), m_quad_1), _mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_quad_2));
    __m128 v20 = _mm_shuffle_ps(v19, _mm_unpackhi_ps(v19, _mm_add_ps(m_quad_2, m_quad_1)), 196);
    __m128 v21 = _mm_add_ps(_mm_shuffle_ps(v20, v20, 78), v20);
    float v23 = (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 177), 0) + XFNV_M128_F32(v21, 0)) + 1.1920928955078125e-07;
    XFNV_M128_F32(m_jacobianSchemas[2].m_quad, 3) = v23;
    XFNV_M128_F32(m_jacobianSchemas[1].m_quad, 3) = m_virtMassFactor / v23;
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) = info.m_lastSolverResults->m_internalSolverData * in.m_rhsFactor;
    hkpJacobianSchema* m_jacobianSchemas_1 = out.m_jacobianSchemas;
    out.m_jacobianSchemas += 4;
    XFNV_M128_F32(m_jacobianSchemas_1[3].m_quad, 0) = info.m_maxFrictionForce * in.m_microStepDeltaTime;
    XFNV_M128_F32(m_jacobianSchemas_1->m_quad, 0) =
        XFNV_M128_F32(m_jacobianSchemas_1->m_quad, 0) + 1.1920928955078125e-07;
    m_jacobianSchemas_1->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_FRICTION;
}

void hk1dAngularBilateralConstraintBuildJacobian(
    const hkp1dAngularBilateralConstraintInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    hkVector4 v3;
    v3.m_quad = info.m_perpZeroErrorAxisBinW.m_quad;
    __m128 v4 = _mm_xor_ps(v3.m_quad, hkQuadRealNegateMask);
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
    double m_virtMassFactor = in.m_virtMassFactor;
    hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
    m_jacobianSchemas->m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 85), m_bodyA->m_scratch123.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 0), m_bodyA->m_scratch123.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v3.m_quad, v3.m_quad, 170), m_bodyA->m_scratch123.m_col2.m_quad)
    );
    __m128 v9 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 85), m_bodyB->m_scratch123.m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), m_bodyB->m_scratch123.m_col0.m_quad)
    );
    __m128 v10 = _mm_mul_ps(_mm_shuffle_ps(v4, v4, 170), m_bodyB->m_scratch123.m_col2.m_quad);
    __m128 m_quad = m_jacobianSchemas->m_quad;
    __m128 v12 = _mm_add_ps(v9, v10);
    m_jacobianSchemas[1].m_quad = v12;
    __m128 v13 = _mm_add_ps(
        _mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_bodyA->m_invMasses.m_quad),
        _mm_mul_ps(_mm_mul_ps(v12, v12), m_bodyB->m_invMasses.m_quad)
    );
    float v15 = (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 170), 0) +
                 (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0))) +
        1.1920928955078125e-07;
    XFNV_M128_F32(m_jacobianSchemas->m_quad, 3) = m_virtMassFactor / v15;
    float v16 = XFNV_M128_F32(info.m_constrainedDofW.m_quad, 1) * XFNV_M128_F32(info.m_zeroErrorAxisAinW.m_quad, 1) +
        XFNV_M128_F32(info.m_zeroErrorAxisAinW.m_quad, 0) * XFNV_M128_F32(info.m_constrainedDofW.m_quad, 0) +
        XFNV_M128_F32(info.m_constrainedDofW.m_quad, 2) * XFNV_M128_F32(info.m_zeroErrorAxisAinW.m_quad, 2);
    XFNV_M128_F32(m_jacobianSchemas[1].m_quad, 3) = -v16 * in.m_rhsFactor;
    hkpJacobianSchema* m_jacobianSchemas_1 = out.m_jacobianSchemas;
    out.m_jacobianSchemas += 2;
    XFNV_M128_F32(m_jacobianSchemas_1->m_quad, 0) =
        XFNV_M128_F32(m_jacobianSchemas_1->m_quad, 0) + 1.1920928955078125e-07;
    m_jacobianSchemas_1->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR;
}

void hk1dAngularFrictionBuildJacobian(
    const hkp1dAngularFrictionInfo& info,
    const hkpConstraintQueryIn& in,
    hkpConstraintQueryOut& out
) {
    hkpJacobianSchema* m_jacobianSchemas = out.m_jacobianSchemas;
    const hkVector4* m_constrainedDofW = info.m_constrainedDofW;
    int32_t v15 = info.m_numFriction.m_storage - 1;
    if (v15 < 0) {
        out.m_jacobianSchemas = m_jacobianSchemas;
    }
    else {
        bool v14;
        const hkpSolverResults* pLastResults = info.m_lastSolverResults;
        do {
            hkpVelocityAccumulator* m_bodyA = in.m_bodyA;
            hkpVelocityAccumulator* m_bodyB = in.m_bodyB;
            __m128 v8 = _mm_xor_ps(m_constrainedDofW->m_quad, hkQuadRealNegateMask);
            m_jacobianSchemas->m_quad = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(m_constrainedDofW->m_quad, m_constrainedDofW->m_quad, 85),
                        m_bodyA->m_scratch123.m_col1.m_quad
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(m_constrainedDofW->m_quad, m_constrainedDofW->m_quad, 0),
                        m_bodyA->m_scratch123.m_col0.m_quad
                    )
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(m_constrainedDofW->m_quad, m_constrainedDofW->m_quad, 170),
                    m_bodyA->m_scratch123.m_col2.m_quad
                )
            );
            __m128 v9 = _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v8, v8, 85), m_bodyB->m_scratch123.m_col1.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), m_bodyB->m_scratch123.m_col0.m_quad)
            );
            __m128 v10 = _mm_mul_ps(_mm_shuffle_ps(v8, v8, 170), m_bodyB->m_scratch123.m_col2.m_quad);
            __m128 m_quad = m_jacobianSchemas->m_quad;
            __m128 v12 = _mm_add_ps(v9, v10);
            m_jacobianSchemas[1].m_quad = v12;
            __m128 v13 = _mm_add_ps(
                _mm_mul_ps(_mm_mul_ps(m_quad, m_quad), m_bodyA->m_invMasses.m_quad),
                _mm_mul_ps(_mm_mul_ps(v12, v12), m_bodyB->m_invMasses.m_quad)
            );
            m_jacobianSchemas += 3;
            ++m_constrainedDofW;
            float v16 = (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 170), 0) +
                         (XFNV_M128_F32(_mm_shuffle_ps(v13, v13, 85), 0) + XFNV_M128_F32(v13, 0))) +
                1.1920928955078125e-07;
            ++pLastResults;
            v14 = --v15 < 0;
            XFNV_M128_F32(m_jacobianSchemas[-3].m_quad, 3) = 1.0 / v16;
            XFNV_M128_F32(m_jacobianSchemas[-2].m_quad, 3) =
                pLastResults[-1].m_internalSolverData * in.m_substepInvDeltaTime;
            XFNV_M128_F32(m_jacobianSchemas[-1].m_quad, 0) = in.m_microStepDeltaTime * info.m_maxFrictionTorque;
            XFNV_M128_F32(m_jacobianSchemas[-3].m_quad, 0) =
                XFNV_M128_F32(m_jacobianSchemas[-3].m_quad, 0) + 1.1920928955078125e-07;
            m_jacobianSchemas[-3].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_1D_ANGULAR_FRICTION;
        }
        while (!v14);
        out.m_jacobianSchemas = m_jacobianSchemas;
    }
}

void sub_DCEE70() {
    ;
}
