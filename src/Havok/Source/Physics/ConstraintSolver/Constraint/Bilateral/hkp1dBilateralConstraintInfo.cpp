#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

// Builds the linear bilateral row: both lever arms from m_pivotA, the angular parts rotated into each body's core
// space, the diagonal summed lane-wise. The diagonal is rounded to float before the division.
hkp1Lin2AngJacobian* hkp1dLinearBilateralConstraintInfo::buildJacobian(
    float error,
    hkp1Lin2AngJacobian* jacobian,
    const hkp1dLinearBilateralConstraintInfo* info,
    const hkpConstraintQueryIn* in
) {
    const hkpVelocityAccumulator* bodyA = in->m_bodyA;
    const hkpVelocityAccumulator* bodyB = in->m_bodyB;
    float fVirtMassFactor = in->m_virtMassFactor;
    jacobian->m_linear0.m_quad = info->m_constrainedDofW.m_quad;
    hkVector4 r0;
    r0.m_quad = _mm_sub_ps(info->m_pivotA.m_quad, bodyA->m_scratch0.m_quad);
    hkVector4 r1;
    r1.m_quad = _mm_sub_ps(info->m_pivotA.m_quad, bodyB->m_scratch0.m_quad);
    hkVector4 cross1;
    cross1.setCross(info->m_constrainedDofW, r1);
    hkVector4 cross0;
    cross0.setCross(r0, info->m_constrainedDofW);
    // Each body's core-from-world rotation times the cross product.
    jacobian->m_angular[0]._setMul3(bodyA->m_scratch123, cross0);
    jacobian->m_angular[1]._setMul3(bodyB->m_scratch123, cross1);
    float fDiag = jacobian->getDiag(*bodyA, *bodyB);
    XFNV_M128_F32(jacobian->m_angular[1].m_quad, 3) = fDiag;
    XFNV_M128_F32(jacobian->m_angular[0].m_quad, 3) = fVirtMassFactor / fDiag;
    XFNV_M128_F32(jacobian->m_linear0.m_quad, 3) = in->m_rhsFactor * error;
    return jacobian;
}
