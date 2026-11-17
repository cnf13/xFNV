#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereTriangle/hkpSphereTriangleAgent.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpFlagCdBodyPairCollector.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "SDK/Windows/winbase.h"

#include <xmmintrin.h>

hkpSphereTriangleAgent::hkpSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr)
	: hkpIterativeLinearCastAgent(mgr), m_contactPointId(HK_INVALID_CONTACT_POINT) {
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triB->m_vertexA, m_closestPointTriangleCache);
}

// The exe's slot 0 is hkBaseObject's deleting destructor (0xDE10B0): the contact point is removed in cleanup(),
// nothing else to free.
hkpSphereTriangleAgent::~hkpSphereTriangleAgent() {}

// IDA name _sub_D3D6B0 (hkpSphereTriangleAgent's vtable slot 4). The exe inlines getClosestPointInl: the sphere is
// bodyA's translation, the triangle's vertices go through hkVector4::_setTransformedPos (SSE, (y*c1 + x*c0) +
// (z*c2 + t)). The timer macros write into the stream's raw buffer, which the exe treats as untyped (hence the
// casts), as in hkpBvTreeAgent. The SSE steps are intrinsics; the scalar steps are float locals where the exe
// stores. The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpSphereTriangleAgent::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN("SphereTri", nullptr);
	const hkpConvexShape* sphereA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	const hkVector4& sphereCentre = bodyA.m_motion.pMotion->m_translation;

	hkpCdPoint event;
	event.m_cdBodyA = &bodyA;
	event.m_cdBodyB = &bodyB;
	alignas(16) hkVector4 triVertices[3];
	hkpCollideTriangleUtil::ClosestPointTriangleResult result;
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		// The exe copies the transform into its frame before the SSE loads; its copy of the translation shares the
		// frame slot of result.hitDirection, so the result passed to closestPointTriangle starts out holding it.
		_mm_store_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0], c3);
		const hkVector4* vertices = &triB->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	hkpCollideTriangleUtil::closestPointTriangle(sphereCentre, triVertices, m_closestPointTriangleCache, result,
		nullptr);
	// The exe stores the radius sum (x87) as a float local, then the sum with the tolerance as another.
	const float radiusSum = sphereA->m_radius + triB->m_radius;
	const float distance = result.distance;
	const float limit = input.m_tolerance + radiusSum;
	// fcomp / test ah, 0x41 / jne: only limit > distance (not NaN) reports the point.
	if (limit > distance) {
		const float scale = triB->m_radius - distance;
		const __m128 hitDirection = _mm_load_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0]);
		const __m128 position = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection),
			_mm_load_ps(&reinterpret_cast<const float*>(&sphereCentre.m_quad)[0]));
		_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], hitDirection);
		event.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
		_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], position);
		collector.addCdPoint(event);
	}
	HK_TIMER_END();
}

// The SDK's getPenetrations: the sphere's centre in the triangle's space (hkVector4::_setTransformedInversePos,
// SSE), the closest point on the triangle with the agent's cache, and the pair reported when the radius sum (x87,
// stored as a float local) is beyond the distance. The exe's frame has no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpSphereTriangleAgent::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	HK_TIMER_BEGIN("SphereTri", nullptr);
	const hkpConvexShape* sphereA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	hkVector4 position;
	{
		// EXE-INLINED 0xC7F690
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = t.m_rotation.m_col0.m_quad;
		const __m128 c1 = t.m_rotation.m_col1.m_quad;
		const __m128 c2 = t.m_rotation.m_col2.m_quad;
		const __m128 d = _mm_sub_ps(bodyA.m_motion.pMotion->m_translation.m_quad, t.m_translation.m_quad);
		const __m128 lo = _mm_unpacklo_ps(c0, c1);
		const __m128 row0 = _mm_movelh_ps(lo, c2);
		const __m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), c2, 0xD4);
		const __m128 row2 = _mm_shuffle_ps(_mm_unpackhi_ps(c0, c1), c2, 0xE4);
		position.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, 0x55), row1),
										 _mm_mul_ps(_mm_shuffle_ps(d, d, 0x00), row0)),
			_mm_mul_ps(row2, _mm_shuffle_ps(d, d, 0xAA)));
	}
	hkpCollideTriangleUtil::ClosestPointTriangleResult result;
	hkpCollideTriangleUtil::closestPointTriangle(position, &triB->m_vertexA, m_closestPointTriangleCache, result,
		nullptr);
	const float radiusSum = triB->m_radius + sphereA->m_radius;
	// fcompp / test ah, 0x41 / jne: only radiusSum > distance (not NaN) reports the pair.
	if (radiusSum > result.distance) {
		collector.addCdBodyPair(bodyA, bodyB);
	}
	HK_TIMER_END();
}

// The SDK's processCollision with getClosestPointInternal inlined: the triangle's vertices go through
// hkVector4::_setTransformedPos (SSE, (y*c1 + x*c0) + (z*c2 + t)) and the closest point lands in the next free
// contact point of the output. With features found the triangle welds the point through its vtable (the exe makes no
// m_weldClosestPoints test here); a rejected point removes the agent's contact point and returns without the timer's
// end command, as the exe does. The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpSphereTriangleAgent::processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	HK_TIMER_BEGIN("SphereTri", nullptr);
	hkpProcessCdPoint& point = *result.m_firstFreeContactPoint.m_storage;
	const hkpConvexShape* sphereA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	const hkVector4& sphereCentre = bodyA.m_motion.pMotion->m_translation;

	alignas(16) hkVector4 triVertices[3];
	hkpCollideTriangleUtil::ClosestPointTriangleResult cpResult;
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = t.m_rotation.m_col0.m_quad;
		const __m128 c1 = t.m_rotation.m_col1.m_quad;
		const __m128 c2 = t.m_rotation.m_col2.m_quad;
		const __m128 c3 = t.m_translation.m_quad;
		// The exe copies the transform into its frame before the SSE loads; its copy of the translation shares the
		// frame slot of cpResult.hitDirection, so the result passed to closestPointTriangle starts out holding it.
		cpResult.hitDirection.m_quad = c3;
		const hkVector4* vertices = &triB->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = vertices[i].m_quad;
			// EXE-INLINED none: hkVector4::_setTransformedPos
			triVertices[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
												   _mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
		}
	}

	hkpFeatureOutput features;
	hkpCollideTriangleUtil::closestPointTriangle(sphereCentre, triVertices, m_closestPointTriangleCache, cpResult,
		&features);
	// The exe stores the radius sum (x87) as a float local, then the sum with the tolerance as another.
	const float radiusSum = sphereA->m_radius + triB->m_radius;
	const float distance = cpResult.distance;
	const float limit = input.m_tolerance + radiusSum;
	// fcomp / test ah, 0x41 / jne: only limit > distance (not NaN) reports the point.
	if (limit > distance) {
		const float scale = triB->m_radius - distance;
		const __m128 hitDirection = cpResult.hitDirection.m_quad;
		point.m_contact.m_position.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection), sphereCentre.m_quad);
		point.m_contact.m_separatingNormal.m_quad = hitDirection;
		point.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
		// The exe copies numFeatures to a byte local, whose address the weld gets.
		uint8_t numFeatures = features.numFeatures;
		if (numFeatures > 0) {
			hkVector4 weldNormal = point.m_contact.m_separatingNormal;
			hkVector4 weldPosition = point.m_contact.m_position;
			const hkpConvexShape* weldShape = static_cast<const hkpConvexShape*>(bodyB.m_shape);
			const int32_t weldResult =
				weldShape->weldContactPointImpl(features.featureIds, numFeatures, weldPosition, bodyB.m_motion.pMotion,
					static_cast<const hkpConvexShape*>(bodyA.m_shape), bodyA.m_motion.pMotion, weldNormal);
			if (weldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
				if (m_contactPointId != HK_INVALID_CONTACT_POINT) {
					m_contactMgr->removeContactPointImpl(m_contactPointId, *result.m_constraintOwner.m_storage);
					m_contactPointId = HK_INVALID_CONTACT_POINT;
				}
				return;
			}
			if (weldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
				point.m_contact.setNormal(weldNormal);
			}
		}
		if (m_contactPointId == HK_INVALID_CONTACT_POINT) {
			m_contactPointId = m_contactMgr->addContactPointImpl(bodyA, bodyB, input, result, nullptr, point.m_contact);
		}
		if (m_contactPointId != HK_INVALID_CONTACT_POINT) {
			point.m_contactPointId.m_storage = m_contactPointId;
			result.m_firstFreeContactPoint.m_storage++;
		}
	} else if (m_contactPointId != HK_INVALID_CONTACT_POINT) {
		m_contactMgr->removeContactPointImpl(m_contactPointId, *result.m_constraintOwner.m_storage);
		m_contactPointId = HK_INVALID_CONTACT_POINT;
	}
	HK_TIMER_END();
}

void hkpSphereTriangleAgent::cleanup(hkpConstraintOwner& constraintOwner) {
	uint16_t m_contactPointId = this->m_contactPointId;
	if (m_contactPointId != 0xFFFF) {
		m_contactMgr->removeContactPointImpl(m_contactPointId, constraintOwner);
	}
	delete this;
}

// The sphere's centre goes into the triangle's space (the exe inlines hkVector4::setTransformedInversePos: the
// transposed rotation, (y*rowY + x*rowX) + rowZ*z), and the pair is reported when the closest point lies within the
// radius sum. The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpSphereTriangleAgent::staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	HK_TIMER_BEGIN("SphereTri", nullptr);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	const hkpConvexShape* sphereA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
	alignas(16) hkpCollideTriangleUtil::ClosestPointTriangleCache cache;
	hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triB->m_vertexA, cache);

	hkVector4 position;
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 col1 = t.m_rotation.m_col1.m_quad;
		const __m128 col2 = t.m_rotation.m_col2.m_quad;
		// EXE-INLINED 0xC7F690
		const __m128 d = _mm_sub_ps(bodyA.m_motion.pMotion->m_translation.m_quad, t.m_translation.m_quad);
		const __m128 lo = _mm_unpacklo_ps(t.m_rotation.m_col0.m_quad, col1);
		const __m128 rowX = _mm_movelh_ps(lo, col2);
		const __m128 rowY = _mm_shuffle_ps(_mm_movehl_ps(rowX, lo), col2, 0xD4);
		const __m128 rowZ = _mm_shuffle_ps(_mm_unpackhi_ps(t.m_rotation.m_col0.m_quad, col1), col2, 0xE4);
		position.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(d, d, 0x55), rowY),
										 _mm_mul_ps(_mm_shuffle_ps(d, d, 0x00), rowX)),
			_mm_mul_ps(rowZ, _mm_shuffle_ps(d, d, 0xAA)));
	}
	hkpCollideTriangleUtil::ClosestPointTriangleResult result;
	hkpCollideTriangleUtil::closestPointTriangle(position, &triB->m_vertexA, cache, result, nullptr);
	// The exe stores the radius sum (fld triangle, fadd sphere) as a float local; fcompp / test ah, 0x41 / jne: only
	// radiusSum > distance (not NaN) reports the pair.
	const float radiusSum = triB->m_radius + sphereA->m_radius;
	if (radiusSum > result.distance) {
		collector.addCdBodyPair(bodyA, bodyB);
	}
	HK_TIMER_END();
}

// The flipped agent goes in first, for a triangle against a sphere; the two bool stores before each fill are the
// source's, though each init overwrites them.
// Not registered: an uninitialised read. The exe writes only bytes +0x10 and +0x11 of af's flags dword
// (0xD3DC4C, 0xD3DC50, 0xD3DC75, 0xD3DC79; the inits store the same two bytes), and registerCollisionAgent
// (0xCFBBE0) copies the whole dword (0xCFBC1E), so the two padding bytes after m_isPredictive, never written,
// land in the dispatcher's table.
void hkpSphereTriangleAgent::registerAgent(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs af;
	af.m_isFlipped.m_bool = false;
	af.m_isPredictive.m_bool = false;
	symmetricInitAgentFunc(af);
	dispatcher->registerCollisionAgent(af, HK_SHAPE_TRIANGLE, HK_SHAPE_SPHERE);

	af.m_isFlipped.m_bool = false;
	af.m_isPredictive.m_bool = false;
	initAgentFunc(af);
	dispatcher->registerCollisionAgent(af, HK_SHAPE_SPHERE, HK_SHAPE_TRIANGLE);
}

// The same two registrations, through the dispatcher's second agent table (0xCFBCC0).
// Not registered: an uninitialised read. The exe writes only bytes +0x10 and +0x11 of af's flags dword
// (0xD3DCAC, 0xD3DCB0, 0xD3DCD5, 0xD3DCD9; the inits store the same two bytes), and registerCollisionAgent2
// (0xCFBCC0) copies the whole dword (0xCFBCFD), so the two padding bytes after m_isPredictive, never written,
// land in the dispatcher's table.
void hkpSphereTriangleAgent::registerAgent2(hkpCollisionDispatcher* dispatcher) {
	hkpCollisionDispatcher::AgentFuncs af;
	af.m_isFlipped.m_bool = false;
	af.m_isPredictive.m_bool = false;
	symmetricInitAgentFunc(af);
	dispatcher->registerCollisionAgent2(af, HK_SHAPE_TRIANGLE, HK_SHAPE_SPHERE);

	af.m_isFlipped.m_bool = false;
	af.m_isPredictive.m_bool = false;
	initAgentFunc(af);
	dispatcher->registerCollisionAgent2(af, HK_SHAPE_SPHERE, HK_SHAPE_TRIANGLE);
}

// A sphere against a triangle: the agent's own statics and hkpIterativeLinearCastAgent's linear cast; not flipped,
// not predictive.
// Not registered (0xD3DBE0): the exe's callers registerAgent (0xD3DC40) and registerAgent2 (0xD3DCA0) read edx
// after the call (0xD3DC89, 0xD3DCE9), relying on this body leaving it untouched; edx is caller-saved for clang, so
// src cannot promise that.
void hkpSphereTriangleAgent::initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
	af.m_createFunc = &hkpSphereTriangleAgent::createSphereTriangleAgent;
	af.m_getPenetrationsFunc = &hkpSphereTriangleAgent::staticGetPenetrations;
	af.m_getClosestPointFunc = &hkpSphereTriangleAgent::staticGetClosestPoints;
	af.m_linearCastFunc = &hkpIterativeLinearCastAgent::staticLinearCast;
	af.m_isFlipped.m_bool = false;
	af.m_isPredictive.m_bool = false;
}

// A triangle against a sphere: the flipped agent's statics; flipped, not predictive.
void hkpSphereTriangleAgent::symmetricInitAgentFunc(hkpCollisionDispatcher::AgentFuncs& af) {
	af.m_createFunc = &hkpSphereTriangleAgent::createTriangleSphereAgent;
	af.m_getPenetrationsFunc = &hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>::staticGetPenetrations;
	af.m_getClosestPointFunc = &hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>::staticGetClosestPoints;
	af.m_linearCastFunc = &hkpSymmetricAgentLinearCast<hkpCapsuleTriangleAgent>::staticLinearCast;
	af.m_isFlipped.m_bool = true;
	af.m_isPredictive.m_bool = false;
}

// The exe inlines getClosestPointInl with welding: the cache is set up here, the triangle's vertices go through
// hkVector4::_setTransformedPos (SSE, (y*c1 + x*c0) + (z*c2 + t)), and with m_weldClosestPoints and features found
// the triangle welds the point through its vtable. The timer macros write into the stream's raw buffer, which the
// exe treats as untyped (hence the casts), as in hkpBvTreeAgent. The SSE steps are intrinsics. IDA name
// _sub_D3D8C0. The exe's frame has no EH and no /GS cookie.
FNVD_NO_STACK_COOKIE void hkpSphereTriangleAgent::staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	HK_TIMER_BEGIN("SphereTri", nullptr);
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	hkpCollideTriangleUtil::ClosestPointTriangleCache cache;
	hkpCollideTriangleUtil::setupClosestPointTriangleCache(&triB->m_vertexA, cache);

	hkpCdPoint event;
	event.m_cdBodyA = &bodyA;
	event.m_cdBodyB = &bodyB;
	const hkpConvexShape* sphereA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
	const hkVector4& sphereCentre = bodyA.m_motion.pMotion->m_translation;
	alignas(16) hkVector4 triVertices[3];
	hkpCollideTriangleUtil::ClosestPointTriangleResult result;
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		// The exe copies the transform into its frame before the SSE loads; its copy of the translation shares the
		// frame slot of result.hitDirection, so the result passed to closestPointTriangle starts out holding it.
		_mm_store_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0], c3);
		const hkVector4* vertices = &triB->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}

	hkpFeatureOutput features;
	hkpCollideTriangleUtil::closestPointTriangle(sphereCentre, triVertices, cache, result, &features);
	// The exe stores the radius sum (x87) as a float local, then the sum with the tolerance as another.
	const float radiusSum = sphereA->m_radius + triB->m_radius;
	const float distance = result.distance;
	const float limit = input.m_tolerance + radiusSum;
	// fcomp / test ah, 0x41 / jne: only limit > distance (not NaN) reports the point.
	if (limit > distance) {
		const float scale = triB->m_radius - distance;
		// The exe copies numFeatures to a byte local before the welding test.
		uint8_t numFeatures = features.numFeatures;
		const __m128 hitDirection = _mm_load_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0]);
		const __m128 position = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), hitDirection),
			_mm_load_ps(&reinterpret_cast<const float*>(&sphereCentre.m_quad)[0]));
		_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_separatingNormal.m_quad)[0], hitDirection);
		event.m_contact.m_separatingNormal.m_quad[3] = distance - radiusSum;
		_mm_store_ps(&reinterpret_cast<float*>(&event.m_contact.m_position.m_quad)[0], position);
		bool reject = false;
		if (input.m_weldClosestPoints.m_storage != 0 && numFeatures > 0) {
			hkVector4 weldNormal = event.m_contact.m_separatingNormal;
			hkVector4 weldPosition = event.m_contact.m_position;
			const hkpConvexShape* weldShape = triB;
			const int32_t weldResult = weldShape->weldContactPointImpl(features.featureIds, numFeatures, weldPosition,
				bodyB.m_motion.pMotion, sphereA, bodyA.m_motion.pMotion, weldNormal);
			if (weldResult == hkpConvexShape::WELD_RESULT_REJECT_CONTACT_POINT) {
				reject = true;
			} else if (weldResult == hkpConvexShape::WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED) {
				event.m_contact.setNormal(weldNormal);
			}
		}
		if (!reject) {
			collector.addCdPoint(event);
		}
	}
	HK_TIMER_END();
}

// The SDK's new hkpSphereTriangleAgent(bodyA, bodyB, input, mgr): the class allocator's operator new, then the
// constructor.
hkpCollisionAgent* hkpSphereTriangleAgent::createSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	hkpSphereTriangleAgent* agent = static_cast<hkpSphereTriangleAgent*>(hkMemoryRouter::getInstance()
			.m_heap->blockAlloc(sizeof(hkpSphereTriangleAgent)));
	agent->m_memSizeAndFlags = sizeof(hkpSphereTriangleAgent);
	return ::new (agent) hkpSphereTriangleAgent(bodyA, bodyB, mgr);
}

// The SDK's getClosestPoint, getClosestPointInl inlined: the triangle's vertices go through
// hkVector4::_setTransformedPos (SSE, (y*c1 + x*c0) + (z*c2 + t)); the scalar steps are x87, rounded at the float
// locals where the exe stores.
FNVD_NO_STACK_COOKIE hkpSphereTriangleAgent::ClosestPointResult
hkpSphereTriangleAgent::getClosestPoint(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCollideTriangleUtil::ClosestPointTriangleCache& cache, hkContactPoint& cpoint) {
	const hkpConvexShape* sphereA = static_cast<const hkpConvexShape*>(bodyA.m_shape);
	const hkVector4& sphereCentre = bodyA.m_motion.pMotion->m_translation;
	const hkpTriangleShape* triB = static_cast<const hkpTriangleShape*>(bodyB.m_shape);
	alignas(16) hkVector4 triVertices[3];
	hkpCollideTriangleUtil::ClosestPointTriangleResult result;
	{
		const hkTransform& t = *bodyB.m_motion.pMotion;
		const __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
		const __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
		const __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
		const __m128 c3 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0]);
		// The exe copies the transform into its frame before the SSE loads; its copy of the translation shares the
		// frame slot of result.hitDirection, so the result passed to closestPointTriangle starts out holding it.
		_mm_store_ps(&reinterpret_cast<float*>(&result.hitDirection.m_quad)[0], c3);
		const hkVector4* vertices = &triB->m_vertexA;
		for (int32_t i = 2; i >= 0; i--) {
			const __m128 p = _mm_load_ps(&reinterpret_cast<const float*>(&vertices[i].m_quad)[0]);
			// EXE-INLINED none: hkVector4::_setTransformedPos
			const __m128 r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0x55), c1),
											_mm_mul_ps(_mm_shuffle_ps(p, p, 0x00), c0)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(p, p, 0xAA), c2), c3));
			_mm_store_ps(&reinterpret_cast<float*>(&triVertices[i].m_quad)[0], r);
		}
	}
	hkpCollideTriangleUtil::ClosestPointTriangleStatus status =
		hkpCollideTriangleUtil::closestPointTriangle(sphereCentre, triVertices, cache, result, 0);
	float radiusSum = sphereA->m_radius + triB->m_radius;
	float limit = input.m_tolerance + radiusSum;
	if (!(limit > result.distance)) {
		return ST_CP_MISS;
	}
	float scale = triB->m_radius - result.distance;
	cpoint.m_separatingNormal.m_quad = result.hitDirection.m_quad;
	cpoint.m_position.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), result.hitDirection.m_quad), sphereCentre.m_quad);
	cpoint.m_separatingNormal(3) = result.distance - radiusSum;
	return static_cast<ClosestPointResult>((status != hkpCollideTriangleUtil::HIT_TRIANGLE_FACE) + 1);
}

// The SDK's new hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>(bodyA, bodyB, input, mgr): the agent is built
// for the swapped pair, so the triangle is bodyA's shape.
hkpCollisionAgent* hkpSphereTriangleAgent::createTriangleSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpContactMgr* mgr) {
	hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>* agent =
		static_cast<hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>*>(hkMemoryRouter::getInstance()
				.m_heap->blockAlloc(sizeof(hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>)));
	agent->m_memSizeAndFlags = sizeof(hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>);
	return ::new (agent) hkpSymmetricAgentLinearCast<hkpSphereTriangleAgent>(bodyA, bodyB, input, mgr);
}
