#include "Havok/Source/Physics/Dynamics/Constraint/Response/hkpSimpleCollisionResponse.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/ConstraintSolver/SimpleConstraints/hkpSimpleConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

#include <cmath>
#include <xmmintrin.h>

// Each body's input is the contact's position relative to its centre of mass at time, its inverse inertia and masses
// and its transform; the solved velocities go back to the bodies that are not fixed, and the output's keyframed
// velocities are the contact's relative velocity along the normal with each body's motion over the step (its swept
// transform's displacement and rotation times the inverse step) in place of its velocity.
void hkpSimpleCollisionResponse::solveSingleContact(const hkContactPoint& cp, float time,
	hkpSimpleConstraintUtilCollideParams& params, hkpMotion* bodyA, hkpMotion* bodyB, hkpDynamicsContactMgr* contactMgr,
	hkpSimpleCollisionResponse::SolveSingleOutput& output) {
	hkpSimpleConstraintInfoInitInput inA;
	{
		const hkSweptTransform& swept = bodyA->m_motionState.m_sweptTransform;
		// EXE-INLINED none: hkSweptTransformUtil::calcCenterOfMassAt
		const float t = (time - swept.m_centerOfMass0.m_quad[3]) * swept.m_centerOfMass1.m_quad[3];
		const hkQuadReal tq = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
		const hkQuadReal com = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tq), swept.m_centerOfMass0.m_quad),
			_mm_mul_ps(swept.m_centerOfMass1.m_quad, tq));
		inA.m_invMasses = bodyA->m_inertiaAndMassInv;
		inA.m_massRelPos.m_quad = _mm_sub_ps(cp.m_position.m_quad, com);
		bodyA->getInertiaInvWorld(inA.m_invInertia);
		inA.m_transform = &bodyA->getTransform();
	}
	hkpSimpleConstraintInfoInitInput inB;
	{
		const hkSweptTransform& swept = bodyB->m_motionState.m_sweptTransform;
		// EXE-INLINED none: hkSweptTransformUtil::calcCenterOfMassAt
		const float t = (time - swept.m_centerOfMass0.m_quad[3]) * swept.m_centerOfMass1.m_quad[3];
		const hkQuadReal tq = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
		const hkQuadReal com = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tq), swept.m_centerOfMass0.m_quad),
			_mm_mul_ps(swept.m_centerOfMass1.m_quad, tq));
		inB.m_invMasses = bodyB->m_inertiaAndMassInv;
		inB.m_massRelPos.m_quad = _mm_sub_ps(cp.m_position.m_quad, com);
		bodyB->getInertiaInvWorld(inB.m_invInertia);
		inB.m_transform = &bodyB->getTransform();
	}

	hkpBodyVelocity velA;
	velA.m_linear = bodyA->m_linearVelocity;
	velA.m_angular = bodyA->m_angularVelocity;
	hkpBodyVelocity velB;
	velB.m_linear = bodyB->m_linearVelocity;
	velB.m_angular = bodyB->m_angularVelocity;

	// Each body's motion over the step as a velocity: the swept centre's displacement and the rotation, times the
	// inverse step (the centre's w).
	hkpBodyVelocity stepVelA;
	{
		const hkMotionState& ms = bodyA->m_motionState;
		const float invDt = ms.m_sweptTransform.m_centerOfMass1.m_quad[3];
		const hkQuadReal invDtq = _mm_shuffle_ps(_mm_set_ss(invDt), _mm_set_ss(invDt), 0);
		stepVelA.m_linear.m_quad = _mm_mul_ps(invDtq,
			_mm_sub_ps(ms.m_sweptTransform.m_centerOfMass1.m_quad, ms.m_sweptTransform.m_centerOfMass0.m_quad));
		stepVelA.m_angular.m_quad = _mm_mul_ps(invDtq, ms.m_deltaAngle.m_quad);
	}
	hkpBodyVelocity stepVelB;
	{
		const hkMotionState& ms = bodyB->m_motionState;
		const float invDt = ms.m_sweptTransform.m_centerOfMass1.m_quad[3];
		const hkQuadReal invDtq = _mm_shuffle_ps(_mm_set_ss(invDt), _mm_set_ss(invDt), 0);
		stepVelB.m_linear.m_quad = _mm_mul_ps(invDtq,
			_mm_sub_ps(ms.m_sweptTransform.m_centerOfMass1.m_quad, ms.m_sweptTransform.m_centerOfMass0.m_quad));
		stepVelB.m_angular.m_quad = _mm_mul_ps(invDtq, ms.m_deltaAngle.m_quad);
	}

	// The SDK's hkVector4Util::buildOrthonormal(normal, directions): the normal, a perpendicular to it, normalized,
	// and their cross product.
	hkMatrix3 directions;
	directions.m_col0 = cp.m_separatingNormal;
	{
		// EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
		const hkQuadReal& n = cp.m_separatingNormal.m_quad;
		int32_t min = 0;
		int32_t ok1 = 1;
		int32_t ok2 = 2;
		float a0 = static_cast<float>(std::fabs(n[0]));
		const float a1 = static_cast<float>(std::fabs(n[1]));
		const float a2 = static_cast<float>(std::fabs(n[2]));
		if (a1 < a0) {
			ok1 = 0;
			min = 1;
			a0 = a1;
		}
		if (a2 < a0) {
			ok2 = min;
		}
		directions.m_col1.m_quad = _mm_setzero_ps();
		directions.m_col1.m_quad[ok1] = n[ok2];
		directions.m_col1.m_quad[ok2] = -n[ok1];
	}
	directions.m_col1.normalize3();
	directions.m_col2.setCross(directions.m_col0, directions.m_col1);

	contactMgr->toiCollisionResponseBeginCallback(cp, inA, velA, inB, velB);
	hkpSimpleConstraintInfo info;
	hkSimpleConstraintUtil_InitInfo(inA, inB, directions, info);
	const float impulse = static_cast<float>(hkSimpleConstraintUtil_Collide(info, params, velA, velB));
	output.m_impulse = impulse;
	contactMgr->toiCollisionResponseEndCallback(cp, impulse, inA, velA, inB, velB);

	hkVector4 pointVelA;
	hkSimpleConstraintUtil_getPointVelocity(info, stepVelA, velB, pointVelA);
	hkVector4 pointVelB;
	hkSimpleConstraintUtil_getPointVelocity(info, velA, stepVelB, pointVelB);
	output.m_velocityKeyframedA = pointVelA.m_quad[0];
	output.m_velocityKeyframedB = pointVelB.m_quad[0];

	if (bodyA->getType() != hkpMotion::MOTION_FIXED) {
		bodyA->setLinearVelocity(velA.m_linear);
		bodyA->setAngularVelocity(velA.m_angular);
	}
	if (bodyB->getType() != hkpMotion::MOTION_FIXED) {
		bodyB->setLinearVelocity(velB.m_linear);
		bodyB->setAngularVelocity(velB.m_angular);
	}
}

// Per body: the solver input (the contact's position relative to the centre of mass, the inverse inertia in world
// space, the inverse masses, the rigid body's transform) and the velocity, its angular part in core space. The
// directions are the contact normal, a unit perpendicular and their cross product. The constraint data's begin
// callback, the solve and its end callback; then the velocities go back to the accumulators and, for bodies that are
// not fixed, to the rigid bodies' motions (the angular part as the solver left it, in core space).
void hkpSimpleCollisionResponse::solveSingleContact2(hkpSimpleContactConstraintData* constraintData,
	const hkContactPoint& cp, hkpSimpleConstraintUtilCollideParams& params, hkpRigidBody* rigidBodyA,
	hkpRigidBody* rigidBodyB, hkpVelocityAccumulator* bodyA, hkpVelocityAccumulator* bodyB,
	hkpSimpleCollisionResponse::SolveSingleOutput2& output) {
	hkpBodyVelocity vel[2];
	hkpSimpleConstraintInfoInitInput in[2];
	{
		hkpVelocityAccumulator* body = bodyA;
		hkpRigidBody* rigidBody = rigidBodyA;
		for (int32_t i = 0; i < 2; i++) {
			in[i].m_invMasses = body->m_invMasses;
			in[i].m_massRelPos.m_quad = _mm_sub_ps(cp.m_position.m_quad, body->m_scratch0.m_quad);
			vel[i].m_linear = body->m_linearVel;
			vel[i].m_angular._setRotatedInverseDir(body->m_scratch123, body->m_angularVel);
			hkMatrix3 coreFromWorld = body->m_scratch123;
			coreFromWorld.transpose();
			// The exe reloads the inverse masses after the transpose call.
			const hkQuadReal invMasses = body->m_invMasses.m_quad;
			hkMatrix3 scaled;
			scaled.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(invMasses, invMasses, 0), coreFromWorld.m_col0.m_quad);
			scaled.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(invMasses, invMasses, 85), coreFromWorld.m_col1.m_quad);
			scaled.m_col2.m_quad = _mm_mul_ps(_mm_shuffle_ps(invMasses, invMasses, 170), coreFromWorld.m_col2.m_quad);
			in[i].m_invInertia.setMul(scaled, body->m_scratch123);
			in[i].m_transform = &rigidBody->getTransform();
			body = bodyB;
			rigidBody = rigidBodyB;
		}
	}

	hkMatrix3 directions;
	directions.m_col0 = cp.m_separatingNormal;
	{
		// The SDK's perpendicular of the normal: the smallest component's lane zero, the other two swapped, one
		// negated.
		// EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
		int32_t ok1 = 1;
		int32_t ok2 = 2;
		int32_t min = 0;
		float a0 = std::fabs(cp.m_separatingNormal(0));
		const float a1 = std::fabs(cp.m_separatingNormal(1));
		const float a2 = std::fabs(cp.m_separatingNormal(2));
		if (a1 < a0) {
			ok1 = 0;
			min = 1;
			a0 = a1;
		}
		if (a2 < a0) {
			ok2 = min;
		}
		directions.m_col1.m_quad = _mm_setzero_ps();
		directions.m_col1(ok1) = cp.m_separatingNormal(ok2);
		directions.m_col1(ok2) = -cp.m_separatingNormal(ok1);
	}
	directions.m_col1.normalize3();
	directions.m_col2.setCross(cp.m_separatingNormal, directions.m_col1);

	constraintData->collisionResponseBeginCallback(cp, in[0], vel[0], in[1], vel[1]);
	hkpSimpleConstraintInfo info;
	hkSimpleConstraintUtil_InitInfo(in[0], in[1], directions, info);
	const float impulse = hkSimpleConstraintUtil_Collide(info, params, vel[0], vel[1]);
	output.m_impulse = impulse;
	constraintData->collisionResponseEndCallback(cp, impulse, in[0], vel[0], in[1], vel[1]);

	bodyA->m_linearVel = vel[0].m_linear;
	if (rigidBodyA->m_motion.m_type != hkpMotion::MOTION_FIXED) {
		rigidBodyA->m_motion.m_linearVelocity = vel[0].m_linear;
		rigidBodyA->m_motion.m_angularVelocity = vel[0].m_angular;
	}
	bodyB->m_linearVel = vel[1].m_linear;
	if (rigidBodyB->m_motion.m_type != hkpMotion::MOTION_FIXED) {
		rigidBodyB->m_motion.m_linearVelocity = vel[1].m_linear;
		rigidBodyB->m_motion.m_angularVelocity = vel[1].m_angular;
	}

	hkpVelocityAccumulator* body = bodyA;
	for (int32_t i = 0; i < 2; i++) {
		// The angular velocity back into world space: (y * col1 + x * col0) + z * col2.
		// EXE-INLINED 0xC7F700
		const hkQuadReal angular = vel[i].m_angular.m_quad;
		body->m_angularVel.m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(angular, angular, 85), body->m_scratch123.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(angular, angular, 0), body->m_scratch123.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(angular, angular, 170), body->m_scratch123.m_col2.m_quad));
		body = bodyB;
	}
}
