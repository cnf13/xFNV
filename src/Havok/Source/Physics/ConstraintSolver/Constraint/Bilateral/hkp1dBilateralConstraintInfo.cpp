#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"

#include <xmmintrin.h>

// The SDK's linear bilateral row (hk1dLinearBilateralConstraintBuildJacobian's inlined part): both lever arms from
// m_pivotA, the angular parts rotated into each body's core space, the diagonal summed lane-wise as the exe's
// shuffles do. The diagonal is rounded to float before the division, as the exe stores and reloads it.
hkp1Lin2AngJacobian* hkp1dLinearBilateralConstraintInfo::buildJacobian(float a1, hkp1Lin2AngJacobian* r_eax,
	const hkp1dLinearBilateralConstraintInfo* r_edi, const hkpConstraintQueryIn* r_esi) {
	const hkpVelocityAccumulator* bodyA = r_esi->m_bodyA;
	const hkpVelocityAccumulator* bodyB = r_esi->m_bodyB;
	float fVirtMassFactor = r_esi->m_virtMassFactor;
	r_eax->m_linear0.m_quad = r_edi->m_constrainedDofW.m_quad;
	hkVector4 r0;
	r0.m_quad = _mm_sub_ps(r_edi->m_pivotA.m_quad, bodyA->m_scratch0.m_quad);
	hkVector4 r1;
	r1.m_quad = _mm_sub_ps(r_edi->m_pivotA.m_quad, bodyB->m_scratch0.m_quad);
	hkVector4 cross1;
	cross1.setCross(r_edi->m_constrainedDofW, r1);
	hkVector4 cross0;
	cross0.setCross(r0, r_edi->m_constrainedDofW);
	// EXE-INLINED none: hkVector4::_setMul3 (each body's core-from-world rotation times the cross product)
	const hkRotation& rotA = bodyA->m_scratch123;
	r_eax->m_angular[0].m_quad =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(cross0.m_quad, cross0.m_quad, 0x55), rotA.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(cross0.m_quad, cross0.m_quad, 0x00), rotA.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(cross0.m_quad, cross0.m_quad, 0xAA), rotA.m_col2.m_quad));
	const hkRotation& rotB = bodyB->m_scratch123;
	r_eax->m_angular[1].m_quad =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(cross1.m_quad, cross1.m_quad, 0x55), rotB.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(cross1.m_quad, cross1.m_quad, 0x00), rotB.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(cross1.m_quad, cross1.m_quad, 0xAA), rotB.m_col2.m_quad));
	// EXE-INLINED none: hkp1Lin2AngJacobian::getDiag (the angular terms times the inverse inertias, the inverse
	// masses' w lanes added)
	__m128 ang0 = r_eax->m_angular[0].m_quad;
	__m128 ang1 = r_eax->m_angular[1].m_quad;
	__m128 invMassesA = bodyA->m_invMasses.m_quad;
	__m128 invMassesB = bodyB->m_invMasses.m_quad;
	__m128 terms =
		_mm_add_ps(_mm_mul_ps(_mm_mul_ps(ang0, ang0), invMassesA), _mm_mul_ps(_mm_mul_ps(ang1, ang1), invMassesB));
	__m128 lanes = _mm_shuffle_ps(terms, _mm_unpackhi_ps(terms, _mm_add_ps(invMassesA, invMassesB)), 0xC4);
	__m128 sum2 = _mm_add_ps(_mm_shuffle_ps(lanes, lanes, 0x4E), lanes);
	__m128 sum = _mm_add_ps(_mm_shuffle_ps(sum2, sum2, 0xB1), sum2);
	float fSum = _mm_cvtss_f32(sum);
	// HK_REAL_EPSILON, a double constant in the exe (0x10C9300).
	float fDiag = fSum + 1.1920928955078125e-07;
	r_eax->m_angular[1].m_quad[3] = fDiag;
	r_eax->m_angular[0].m_quad[3] = fVirtMassFactor / fDiag;
	r_eax->m_linear0.m_quad[3] = r_esi->m_rhsFactor * a1;
	return r_eax;
}
