#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintUtil.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

#include <xmmintrin.h>

// Each body's point velocity at cp (its linear velocity plus its angular velocity crossed with cp's position
// relative to the centre of mass), dotted with the normal: the difference of A's and B's.
float hkpSimpleContactConstraintUtil::calculateSeparatingVelocity(const hkpRigidBody* bodyA, const hkpRigidBody* bodyB,
	const hkVector4& centerOfMassInWorldA, const hkVector4& centerOfMassInWorldB, const hkContactPoint* cp) {
	__m128 angA = bodyA->m_motion.m_angularVelocity.m_quad;
	__m128 angB = bodyB->m_motion.m_angularVelocity.m_quad;
	__m128 v5 = _mm_sub_ps(cp->m_position.m_quad, centerOfMassInWorldA.m_quad);
	__m128 v6 = _mm_sub_ps(cp->m_position.m_quad, centerOfMassInWorldB.m_quad);
	__m128 v7 =
		_mm_mul_ps(_mm_sub_ps(_mm_add_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(angA, angA, 201),
														_mm_shuffle_ps(v5, v5, 210)),
											 _mm_mul_ps(_mm_shuffle_ps(angA, angA, 210), _mm_shuffle_ps(v5, v5, 201))),
								  bodyA->m_motion.m_linearVelocity.m_quad),
					   _mm_add_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(angB, angB, 201), _mm_shuffle_ps(v6, v6, 210)),
									  _mm_mul_ps(_mm_shuffle_ps(angB, angB, 210), _mm_shuffle_ps(v6, v6, 201))),
						   bodyB->m_motion.m_linearVelocity.m_quad)),
			cp->m_separatingNormal.m_quad);
	return static_cast<float>(_mm_shuffle_ps(v7, v7, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]));
}
