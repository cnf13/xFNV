#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"

#include <xmmintrin.h>

hkpSphereBoxAgent::hkpSphereBoxAgent(hkpContactMgr* mgr)
	: hkpIterativeLinearCastAgent(mgr), m_contactPointId(HK_INVALID_CONTACT_POINT) {}

// Nothing of its own to free.
hkpSphereBoxAgent::~hkpSphereBoxAgent() {}

void hkpSphereBoxAgent::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCdBodyPairCollector& collector) {
	staticGetPenetrations(bodyA, bodyB, input, collector);
}

// The agent keeps no state for the query: the exe inlines the static version (its code is 0xD3CC00's, with ret 0x10),
// so its body is written out here as staticGetClosestPoints has it. The exe's frame has no /GS cookie.
// x87 copy at 0xD3CBA4: the distance goes into the normal's w through fld/fstp.
// EXE-INLINED 0xD3CC00
FNVD_NO_STACK_COOKIE void hkpSphereBoxAgent::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN("SphereBox", nullptr);
	const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;
	hkpCdPoint point;
	point.m_cdBodyA = &bodyA;
	point.m_cdBodyB = &bodyB;
	hkVector4 posLocal;
	posLocal.setTransformedInversePos(*bodyB.m_motion.pMotion, posA);
	const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);
	const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
	const hkQuadReal absPos = _mm_and_ps(posLocal.m_quad, hkVector4_setAbs4_absMask);
	// Per axis, how far the centre lies inside the box's extent: negative where it lies outside.
	const hkQuadReal clipped = _mm_sub_ps(_mm_min_ps(absPos, boxB->m_halfExtents.m_quad), absPos);
	float distance;
	if ((_mm_movemask_ps(_mm_cmplt_ps(clipped, _mm_setzero_ps())) & 7) == 0) {
		// Inside: the face of the largest (least negative) of the centre's distances to the faces.
		const float dx = absPos[0] - boxB->m_halfExtents.m_quad[0];
		const float dy = absPos[1] - boxB->m_halfExtents.m_quad[1];
		const float dz = absPos[2] - boxB->m_halfExtents.m_quad[2];
		int32_t axis;
		float faceDistance;
		if (dy < dx) {
			if (dz < dx) {
				axis = 0;
				faceDistance = dx;
			} else {
				axis = 2;
				faceDistance = dz;
			}
		} else if (dz < dy) {
			axis = 1;
			faceDistance = dy;
		} else {
			axis = 2;
			faceDistance = dz;
		}
		point.m_contact.m_separatingNormal.setXYZ(bodyB.m_motion.pMotion->m_rotation.getColumn(axis));
		if (posLocal.m_quad[axis] < 0.0f) {
			point.m_contact.m_separatingNormal.m_quad =
				_mm_xor_ps(point.m_contact.m_separatingNormal.m_quad, hkQuadRealNegateMask);
		}
		distance = faceDistance - boxB->m_radius - sphereA->m_radius;
	} else {
		// Outside: the direction from the centre to the box's nearest point, normalised by the SDK's inline
		// lengthInverse3 (rsqrtss and one Newton step, 0 for a zero length).
		const hkQuadReal clipped2 = _mm_mul_ps(clipped, clipped);
		const hkQuadReal len2 = _mm_add_ss(_mm_shuffle_ps(clipped2, clipped2, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(clipped2, clipped2, 0x55), clipped2));
		// EXE-INLINED 0x62AF00
		const hkQuadReal approx = _mm_rsqrt_ss(len2);
		const hkQuadReal refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
			_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx)));
		const hkQuadReal invLen = _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_set_ss(0.0f)), refined);
		const hkQuadReal invLen4 = _mm_shuffle_ps(invLen, invLen, 0x00);
		const float length = _mm_cvtss_f32(_mm_mul_ss(len2, invLen4));
		distance = length - (boxB->m_radius + sphereA->m_radius);
		if (input.m_tolerance < distance) {
			HK_TIMER_END();
			return;
		}
		// The direction, pointing from the box to the sphere, in the box's space, then rotated to world space.
		const hkQuadReal normalLocal =
			_mm_xor_ps(_mm_xor_ps(_mm_mul_ps(invLen4, clipped), _mm_and_ps(posLocal.m_quad, hkQuadRealNegateMask)),
				hkQuadRealNegateMask);
		const hkRotation& rotationB = bodyB.m_motion.pMotion->m_rotation;
		point.m_contact.m_separatingNormal.m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0x55), rotationB.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0x00), rotationB.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0xAA), rotationB.m_col2.m_quad));
	}
	// The point lies on the sphere's surface, along the normal from its centre.
	const float scale = -distance - sphereA->m_radius;
	point.m_contact.m_position.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), point.m_contact.m_separatingNormal.m_quad), posA.m_quad);
	point.m_contact.m_separatingNormal.m_quad[3] = distance;
	point.m_contact.m_position.m_quad[3] = 1.0f;
	collector.addCdPoint(point);
	HK_TIMER_END();
}

// The exe's slot holds the base's body (0xDAF540): the class declares it only because the exe has no base vtable
// that would let vtable-check confirm the inheritance. Not registered.
void hkpSphereBoxAgent::linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	hkpIterativeLinearCastAgent::linearCast(bodyA, bodyB, input, collector, startCollector);
}

// The contact of the sphere (body A) and the box (body B), found in the box's space. With the centre inside the
// box, the contact is on the face nearest to it, always kept. Outside, it is on the line from the centre to the
// box's nearest point, kept only within the input's tolerance; beyond it, the agent's contact point is removed.
// x87 copy at 0xD3C855: the distance goes into the normal's w through fld/fstp.
void hkpSphereBoxAgent::processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	HK_TIMER_BEGIN("SphereBox", nullptr);
	hkpProcessCdPoint* point = result.m_firstFreeContactPoint.m_storage;
	const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;
	hkVector4 posLocal;
	posLocal.setTransformedInversePos(*bodyB.m_motion.pMotion, posA);
	const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);
	const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
	const hkQuadReal absPos = _mm_and_ps(posLocal.m_quad, hkVector4_setAbs4_absMask);
	// Per axis, how far the centre lies inside the box's extent: negative where it lies outside.
	const hkQuadReal clipped = _mm_sub_ps(_mm_min_ps(absPos, boxB->m_halfExtents.m_quad), absPos);
	float distance;
	bool inRange = true;
	if ((_mm_movemask_ps(_mm_cmplt_ps(clipped, _mm_setzero_ps())) & 7) == 0) {
		// Inside: the face of the largest (least negative) of the centre's distances to the faces.
		const float dx = absPos[0] - boxB->m_halfExtents.m_quad[0];
		const float dy = absPos[1] - boxB->m_halfExtents.m_quad[1];
		const float dz = absPos[2] - boxB->m_halfExtents.m_quad[2];
		int32_t axis;
		float faceDistance;
		if (dy < dx) {
			if (dz < dx) {
				axis = 0;
				faceDistance = dx;
			} else {
				axis = 2;
				faceDistance = dz;
			}
		} else if (dz < dy) {
			axis = 1;
			faceDistance = dy;
		} else {
			axis = 2;
			faceDistance = dz;
		}
		point->m_contact.m_separatingNormal.setXYZ(bodyB.m_motion.pMotion->m_rotation.getColumn(axis));
		if (posLocal.m_quad[axis] < 0.0f) {
			point->m_contact.m_separatingNormal.m_quad =
				_mm_xor_ps(point->m_contact.m_separatingNormal.m_quad, hkQuadRealNegateMask);
		}
		distance = faceDistance - boxB->m_radius - sphereA->m_radius;
	} else {
		// Outside: the direction from the centre to the box's nearest point, normalised by the SDK's inline
		// lengthInverse3 (rsqrtss and one Newton step, 0 for a zero length).
		const hkQuadReal clipped2 = _mm_mul_ps(clipped, clipped);
		const hkQuadReal len2 = _mm_add_ss(_mm_shuffle_ps(clipped2, clipped2, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(clipped2, clipped2, 0x55), clipped2));
		// EXE-INLINED 0x62AF00
		const hkQuadReal approx = _mm_rsqrt_ss(len2);
		const hkQuadReal refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
			_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx)));
		const hkQuadReal invLen = _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_set_ss(0.0f)), refined);
		const hkQuadReal invLen4 = _mm_shuffle_ps(invLen, invLen, 0x00);
		const float length = _mm_cvtss_f32(_mm_mul_ss(len2, invLen4));
		distance = length - (boxB->m_radius + sphereA->m_radius);
		if (input.m_tolerance < distance) {
			inRange = false;
		} else {
			// The direction, pointing from the box to the sphere, in the box's space, then rotated to world space.
			const hkQuadReal normalLocal =
				_mm_xor_ps(_mm_xor_ps(_mm_mul_ps(invLen4, clipped), _mm_and_ps(posLocal.m_quad, hkQuadRealNegateMask)),
					hkQuadRealNegateMask);
			const hkRotation& rotationB = bodyB.m_motion.pMotion->m_rotation;
			point->m_contact.m_separatingNormal.m_quad =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0x55),
										  rotationB.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0x00), rotationB.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0xAA), rotationB.m_col2.m_quad));
		}
	}
	if (inRange) {
		// The contact lies on the sphere's surface, along the normal from its centre.
		const float scale = -distance - sphereA->m_radius;
		point->m_contact.m_position.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), point->m_contact.m_separatingNormal.m_quad), posA.m_quad);
		point->m_contact.m_separatingNormal.m_quad[3] = distance;
		point->m_contact.m_position.m_quad[3] = 1.0f;
		if (m_contactPointId == 0xFFFF) {
			m_contactPointId = m_contactMgr->addContactPoint(bodyA, bodyB, input, result, nullptr, point->m_contact);
		}
		if (m_contactPointId != 0xFFFF) {
			point->m_contactPointId.m_storage = m_contactPointId;
			++result.m_firstFreeContactPoint.m_storage;
		}
	} else if (m_contactPointId != 0xFFFF) {
		m_contactMgr->removeContactPoint(m_contactPointId, *result.m_constraintOwner.m_storage);
		m_contactPointId = 0xFFFF;
	}
	HK_TIMER_END();
}

void hkpSphereBoxAgent::cleanup(hkpConstraintOwner& constraintOwner) {
	uint16_t m_contactPointId = this->m_contactPointId;
	if (m_contactPointId != 0xFFFF) {
		m_contactMgr->removeContactPointImpl(m_contactPointId, constraintOwner);
	}
	delete this;
}

// The closest point of the sphere (body A) to the box (body B), found in the box's space as processCollision finds
// its contact. With the centre inside the box, the point is on the face nearest to it, always reported. Outside, it
// is on the line from the centre to the box's nearest point, reported only within the input's tolerance.
// The inside path's setXYZ loads the local point's normal whole, its w lane not yet written, and stores that lane
// back unchanged; the w is overwritten with the distance before the point reaches the collector.
// x87 copy at 0xD3CE94: the distance goes into the normal's w through fld/fstp. The exe's frame has no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpSphereBoxAgent::staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN("SphereBox", nullptr);
	const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;
	hkpCdPoint point;
	point.m_cdBodyA = &bodyA;
	point.m_cdBodyB = &bodyB;
	hkVector4 posLocal;
	posLocal.setTransformedInversePos(*bodyB.m_motion.pMotion, posA);
	const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);
	const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
	const hkQuadReal absPos = _mm_and_ps(posLocal.m_quad, hkVector4_setAbs4_absMask);
	// Per axis, how far the centre lies inside the box's extent: negative where it lies outside.
	const hkQuadReal clipped = _mm_sub_ps(_mm_min_ps(absPos, boxB->m_halfExtents.m_quad), absPos);
	float distance;
	if ((_mm_movemask_ps(_mm_cmplt_ps(clipped, _mm_setzero_ps())) & 7) == 0) {
		// Inside: the face of the largest (least negative) of the centre's distances to the faces.
		const float dx = absPos[0] - boxB->m_halfExtents.m_quad[0];
		const float dy = absPos[1] - boxB->m_halfExtents.m_quad[1];
		const float dz = absPos[2] - boxB->m_halfExtents.m_quad[2];
		int32_t axis;
		float faceDistance;
		if (dy < dx) {
			if (dz < dx) {
				axis = 0;
				faceDistance = dx;
			} else {
				axis = 2;
				faceDistance = dz;
			}
		} else if (dz < dy) {
			axis = 1;
			faceDistance = dy;
		} else {
			axis = 2;
			faceDistance = dz;
		}
		point.m_contact.m_separatingNormal.setXYZ(bodyB.m_motion.pMotion->m_rotation.getColumn(axis));
		if (posLocal.m_quad[axis] < 0.0f) {
			point.m_contact.m_separatingNormal.m_quad =
				_mm_xor_ps(point.m_contact.m_separatingNormal.m_quad, hkQuadRealNegateMask);
		}
		distance = faceDistance - boxB->m_radius - sphereA->m_radius;
	} else {
		// Outside: the direction from the centre to the box's nearest point, normalised by the SDK's inline
		// lengthInverse3 (rsqrtss and one Newton step, 0 for a zero length).
		const hkQuadReal clipped2 = _mm_mul_ps(clipped, clipped);
		const hkQuadReal len2 = _mm_add_ss(_mm_shuffle_ps(clipped2, clipped2, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(clipped2, clipped2, 0x55), clipped2));
		// EXE-INLINED 0x62AF00
		const hkQuadReal approx = _mm_rsqrt_ss(len2);
		const hkQuadReal refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
			_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx)));
		const hkQuadReal invLen = _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_set_ss(0.0f)), refined);
		const hkQuadReal invLen4 = _mm_shuffle_ps(invLen, invLen, 0x00);
		const float length = _mm_cvtss_f32(_mm_mul_ss(len2, invLen4));
		distance = length - (boxB->m_radius + sphereA->m_radius);
		if (input.m_tolerance < distance) {
			HK_TIMER_END();
			return;
		}
		// The direction, pointing from the box to the sphere, in the box's space, then rotated to world space.
		const hkQuadReal normalLocal =
			_mm_xor_ps(_mm_xor_ps(_mm_mul_ps(invLen4, clipped), _mm_and_ps(posLocal.m_quad, hkQuadRealNegateMask)),
				hkQuadRealNegateMask);
		const hkRotation& rotationB = bodyB.m_motion.pMotion->m_rotation;
		point.m_contact.m_separatingNormal.m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0x55), rotationB.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0x00), rotationB.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(normalLocal, normalLocal, 0xAA), rotationB.m_col2.m_quad));
	}
	// The point lies on the sphere's surface, along the normal from its centre.
	const float scale = -distance - sphereA->m_radius;
	point.m_contact.m_position.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), point.m_contact.m_separatingNormal.m_quad), posA.m_quad);
	point.m_contact.m_separatingNormal.m_quad[3] = distance;
	point.m_contact.m_position.m_quad[3] = 1.0f;
	collector.addCdPoint(point);
	HK_TIMER_END();
}

// Reports the pair when the sphere (body A) reaches into the box (body B): the box's point nearest to the sphere's
// centre (the centre in the box's space, clamped to the half extents with its signs kept), taken back to world space,
// lies closer to the centre than the two convex radii together.
void hkpSphereBoxAgent::staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	HK_TIMER_BEGIN("SphereBox", nullptr);
	const hkpSphereShape* sphereA = static_cast<const hkpSphereShape*>(bodyA.m_shape);
	const hkpBoxShape* boxB = static_cast<const hkpBoxShape*>(bodyB.m_shape);
	const hkVector4& posA = bodyA.m_motion.pMotion->m_translation;
	hkVector4 posLocal;
	posLocal.setTransformedInversePos(*bodyB.m_motion.pMotion, posA);
	const float radiusSum = boxB->m_radius + sphereA->m_radius;
	const hkQuadReal nearest =
		_mm_xor_ps(_mm_min_ps(_mm_and_ps(hkVector4_setAbs4_absMask, posLocal.m_quad), boxB->m_halfExtents.m_quad),
			_mm_and_ps(hkQuadRealNegateMask, posLocal.m_quad));
	const hkTransform& transformB = *bodyB.m_motion.pMotion;
	const hkQuadReal nearestWorld =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(nearest, nearest, 0x55), transformB.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(nearest, nearest, 0x00), transformB.m_rotation.m_col0.m_quad)),
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(nearest, nearest, 0xAA), transformB.m_rotation.m_col2.m_quad),
				transformB.m_translation.m_quad));
	const hkQuadReal diff = _mm_sub_ps(nearestWorld, posA.m_quad);
	const hkQuadReal diff2 = _mm_mul_ps(diff, diff);
	const float distSquared = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(diff2, diff2, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(diff2, diff2, 0x55), diff2)));
	if (distSquared < radiusSum * radiusSum) {
		collector.addCdBodyPair(bodyA, bodyB);
	}
	HK_TIMER_END();
}

// Box against sphere (flipped, through the symmetric wrappers) and sphere against box; neither is predictive, the
// linear cast of the direct pair is the iterative agent's.
// Not registered (KEEP): an uninitialised read. The exe never writes the local AgentFuncs' padding bytes +0x12 and
// +0x13 (only [esp+0x20]/[esp+0x21]), and registerCollisionAgent (0xCFBBE0) copies that whole dword.
void hkpSphereBoxAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs f;
	f.m_createFunc = &hkpSphereBoxAgent::createBoxSphereAgent;
	f.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
	f.m_isFlipped.m_bool = 1;
	f.m_isPredictive.m_bool = 0;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_BOX, HK_SHAPE_SPHERE);
	f.m_createFunc = &hkpSphereBoxAgent::createSphereBoxAgent;
	f.m_getPenetrationsFunc = &hkpSphereBoxAgent::staticGetPenetrations;
	f.m_getClosestPointFunc = &hkpSphereBoxAgent::staticGetClosestPoints;
	f.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
	f.m_isFlipped.m_bool = 0;
	f.m_isPredictive.m_bool = 0;
	dispatcher->registerCollisionAgent(f, HK_SHAPE_SPHERE, HK_SHAPE_BOX);
}

// The agent of a sphere A and a box B. The exe inlines its constructor.
hkpCollisionAgent* hkpSphereBoxAgent::createSphereBoxAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	hkpSphereBoxAgent* agent =
		static_cast<hkpSphereBoxAgent*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpSphereBoxAgent)));
	agent->m_memSizeAndFlags = sizeof(hkpSphereBoxAgent);
	return ::new (agent) hkpSphereBoxAgent(mgr);
}

// The agent of a box A and a sphere B: the sphere-box agent run with the bodies swapped. The exe inlines its
// constructor (the sphere-box agent's, then the flipped agent's vtable; the exe leaves out the overwritten store).
hkpCollisionAgent* hkpSphereBoxAgent::createBoxSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>* agent =
		static_cast<hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>*>(hkMemoryRouter::getInstance()
				.m_heap->blockAlloc(sizeof(hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>)));
	agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>);
	return ::new (agent) hkpSymmetricAgentLinearCast<hkpSphereBoxAgent>(mgr);
}
